#include <QTest>
#include <QCoreApplication>
#include <QElapsedTimer>
#include <QFile>
#include <QLabel>
#include <QListWidget>
#include <QProcess>
#include <QPushButton>
#include <QTemporaryDir>
#include <QTemporaryFile>

#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <cstdlib>
#include <future>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include "aiws/build_worker.hpp"
#include "aiws/gui/main_window.hpp"
#include "aiws/ipc_server.hpp"

namespace {

using std::chrono::milliseconds;
using std::chrono::seconds;

// ---- helpers for the MainWindow-level tests ---------------------------------

QString make_file(QTemporaryDir& dir, const QString& name) {
    const QString path = dir.filePath(name);
    QFile f(path);
    if (f.open(QIODevice::WriteOnly)) {
        f.write("alpha beta gamma delta");
    }
    return path;
}

QPushButton* find_button(aiws::gui::MainWindow& w, const QString& text) {
    for (auto* b : w.findChildren<QPushButton*>()) {
        if (b->text() == text) return b;
    }
    return nullptr;
}

QLabel* find_label(aiws::gui::MainWindow& w, const QString& prefix) {
    for (auto* l : w.findChildren<QLabel*>()) {
        if (l->text().startsWith(prefix)) return l;
    }
    return nullptr;
}

QString aiws_send_path() {
    return QCoreApplication::applicationDirPath() + "/aiws_send";
}

// ---- helpers for the BuildWorker-level tests --------------------------------

aiws::Workspace make_workspace(int documents) {
    aiws::Workspace ws;
    for (int i = 0; i < documents; ++i) {
        const std::string id = "/doc" + std::to_string(i);
        ws.add_document(aiws::Document{id, id, "alpha beta gamma delta"});
    }
    return ws;
}

// Records what the worker delivers. Must outlive the worker using it.
class Recorder {
public:
    aiws::BuildWorker::DoneFn on_done() {
        return [this](std::uint64_t generation, std::shared_ptr<aiws::ProcessingCore>) {
            {
                std::lock_guard<std::mutex> lock(mutex_);
                done_.push_back(generation);
                threads_.push_back(std::this_thread::get_id());
            }
            changed_.notify_all();
        };
    }
    aiws::BuildWorker::FailFn on_failed() {
        return [](std::uint64_t, std::string) {};
    }
    bool wait_for_done(std::size_t count, milliseconds timeout) {
        std::unique_lock<std::mutex> lock(mutex_);
        return changed_.wait_for(lock, timeout, [&] { return done_.size() >= count; });
    }
    std::vector<std::uint64_t> done() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return done_;
    }
    std::vector<std::thread::id> threads() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return threads_;
    }

private:
    mutable std::mutex mutex_;
    std::condition_variable changed_;
    std::vector<std::uint64_t> done_;
    std::vector<std::thread::id> threads_;
};

}  // namespace

class StudentTests : public QObject {
    Q_OBJECT

private slots:
    // R4 first: if shutdown hangs, this test must be the one that notices.
    void test_r4_idle_shutdown_joins();

    void test_r1_single_worker_thread();
    void test_r1_gui_not_blocked();
    void test_r2_outdated_request_never_built();
    void test_r3_cancelled_build_stops_early();
    void test_r3_failure_reporting();
    void test_r4_exit_during_build();
    void test_r5_state_transitions();
    void test_workspace_change_mid_build_cancels();
    void test_reset_during_build();

    void test_cr1_remote_add_success();
    void test_cr1_remote_add_invalid_file();
    void test_cr1_remote_add_unreachable();
};

// R4: with no build running, destroying the worker returns. A watchdog turns a
// hang into a failure after 3 seconds instead of waiting for the ctest timeout.
void StudentTests::test_r4_idle_shutdown_joins() {
    Recorder rec;
    auto worker = std::make_unique<aiws::BuildWorker>(rec.on_done(), rec.on_failed());

    worker->submit(aiws::BuildRequest{make_workspace(1), 1, 0});
    QVERIFY(rec.wait_for_done(1, seconds(5)));
    QTRY_VERIFY_WITH_TIMEOUT(worker->idle(), 5000);  // now blocked on its condition variable

    std::promise<void> closed;
    std::future<void> closed_future = closed.get_future();
    std::thread closer([&worker, &closed] {
        worker.reset();
        closed.set_value();
    });
    if (closed_future.wait_for(seconds(3)) != std::future_status::ready) {
        QWARN("BuildWorker did not stop at shutdown: exit would hang");
        std::_Exit(1);  // the closer thread is stuck in join(); leave immediately
    }
    closer.join();
}

// R1: every build, however many, runs on one thread that is not the GUI thread.
void StudentTests::test_r1_single_worker_thread() {
    Recorder rec;
    aiws::BuildWorker worker(rec.on_done(), rec.on_failed());

    for (std::uint64_t generation = 1; generation <= 3; ++generation) {
        worker.submit(aiws::BuildRequest{make_workspace(1), generation, 0});
        QVERIFY(rec.wait_for_done(generation, seconds(5)));
    }
    const auto threads = rec.threads();
    QCOMPARE(threads.size(), std::size_t{3});
    QVERIFY(threads[0] == threads[1]);
    QVERIFY(threads[1] == threads[2]);
    QVERIFY(threads[0] != std::this_thread::get_id());
}

// R1: Build Corpus returns at once, the GUI stays usable, and Build is not
// available a second time while a build runs.
void StudentTests::test_r1_gui_not_blocked() {
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    aiws::gui::MainWindow win;
    win.set_build_delay_ms(400);
    QVERIFY(win.add_document_file(make_file(dir, "a.txt")));

    auto* build = find_button(win, "Build Corpus");
    QVERIFY(build != nullptr);
    QVERIFY2(build->isEnabled(),
             qPrintable(QString("state=%1 docs_in_list=%2")
                            .arg(win.corpus_state_name())
                            .arg(win.findChild<QListWidget*>()->count())));

    QElapsedTimer t;
    t.start();
    build->click();
    QVERIFY(t.elapsed() < 200);  // the click returned: no build on the GUI thread
    QCOMPARE(win.corpus_state_name(), QString("Building"));
    QVERIFY(!build->isEnabled());

    QVERIFY(win.add_document_file(make_file(dir, "b.txt")));  // GUI still usable
    QVERIFY(t.elapsed() < 300);
}

// R2: a request that is outdated before the worker takes it is never built.
void StudentTests::test_r2_outdated_request_never_built() {
    Recorder rec;
    aiws::BuildWorker worker(rec.on_done(), rec.on_failed());

    worker.submit(aiws::BuildRequest{make_workspace(4), 1, 100});
    QTRY_VERIFY_WITH_TIMEOUT(!worker.has_pending(), 2000);  // request 1 is running
    worker.submit(aiws::BuildRequest{make_workspace(2), 2, 100});
    worker.submit(aiws::BuildRequest{make_workspace(1), 3, 100});  // replaces 2 before it is taken

    QVERIFY(rec.wait_for_done(1, seconds(5)));
    QTRY_VERIFY_WITH_TIMEOUT(worker.idle(), 5000);
    const std::vector<std::uint64_t> expected{3};
    const std::vector<std::uint64_t> actual = rec.done();
    QVERIFY(actual == expected);
}

// R3: a build that is no longer wanted stops within about one document, and the
// next build does not wait for it to run to the end.
void StudentTests::test_r3_cancelled_build_stops_early() {
    Recorder rec;
    aiws::BuildWorker worker(rec.on_done(), rec.on_failed());

    worker.submit(aiws::BuildRequest{make_workspace(10), 1, 150});
    QTRY_VERIFY_WITH_TIMEOUT(!worker.has_pending(), 2000);

    QElapsedTimer t;
    t.start();
    worker.submit(aiws::BuildRequest{make_workspace(1), 2, 150});
    QVERIFY(rec.wait_for_done(1, seconds(5)));
    QVERIFY2(t.elapsed() < 1000, "next build waited for the useless one");

    const std::vector<std::uint64_t> expected{2};
    const std::vector<std::uint64_t> actual = rec.done();
    QVERIFY(actual == expected);
}

// R3: a failing build is reported, and the application stays usable.
// The same file added twice gives duplicate document ids, so rebuild() throws.
void StudentTests::test_r3_failure_reporting() {
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    aiws::gui::MainWindow win;
    const QString f = make_file(dir, "a.txt");
    QVERIFY(win.add_document_file(f));
    QVERIFY(win.add_document_file(f));

    auto* build = find_button(win, "Build Corpus");
    auto* status = find_label(win, "Current status");
    QVERIFY(build && status);

    build->click();
    QTRY_VERIFY2_WITH_TIMEOUT(status->text().contains("failed"),
                              qPrintable(status->text() + " | state=" + win.corpus_state_name()),
                              5000);
    QCOMPARE(win.corpus_state_name(), QString("NotBuilt"));
    QVERIFY(build->isEnabled());
    QVERIFY(win.add_document_file(make_file(dir, "c.txt")));
}

// R4: destroying the window during a build joins within about one document,
// not the whole build; repeated use does not hang or pile up threads.
void StudentTests::test_r4_exit_during_build() {
    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    QElapsedTimer t;
    t.start();
    {
        aiws::gui::MainWindow win;
        win.set_build_delay_ms(400);  // 3 documents: 1.2 s if it ran to the end
        QVERIFY(win.add_document_file(make_file(dir, "a.txt")));
        QVERIFY(win.add_document_file(make_file(dir, "b.txt")));
        QVERIFY(win.add_document_file(make_file(dir, "c.txt")));
        find_button(win, "Build Corpus")->click();
    }
    QVERIFY2(t.elapsed() < 900, "window destruction waited for the whole build");

    for (int i = 0; i < 5; ++i) {
        aiws::gui::MainWindow w2;
        w2.set_build_delay_ms(20);
        QVERIFY(w2.add_document_file(make_file(dir, "a.txt")));
        find_button(w2, "Build Corpus")->click();
        QTRY_COMPARE_WITH_TIMEOUT(w2.corpus_state_name(), QString("Current"), 5000);
    }
}

// R5: the corpus lifecycle, one event at a time.
void StudentTests::test_r5_state_transitions() {
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    aiws::gui::MainWindow win;
    win.set_build_delay_ms(50);
    auto* build = find_button(win, "Build Corpus");
    auto* corpus = find_label(win, "Corpus:");
    QVERIFY(build && corpus);

    QCOMPARE(win.corpus_state_name(), QString("NotBuilt"));
    QVERIFY(win.add_document_file(make_file(dir, "a.txt")));
    QCOMPARE(win.corpus_state_name(), QString("NotBuilt"));

    build->click();
    QCOMPARE(win.corpus_state_name(), QString("Building"));
    // Decision 1: a build request while building is ignored.
    QMetaObject::invokeMethod(&win, "build_corpus");
    QCOMPARE(win.corpus_state_name(), QString("Building"));
    QTRY_COMPARE_WITH_TIMEOUT(win.corpus_state_name(), QString("Current"), 5000);
    QCOMPARE(corpus->text(), QString("Corpus: 1 chunks"));

    QVERIFY(win.add_document_file(make_file(dir, "b.txt")));
    QCOMPARE(win.corpus_state_name(), QString("NotBuilt"));
    QCOMPARE(corpus->text(), QString("Corpus: Not built"));

    build->click();
    QTRY_COMPARE_WITH_TIMEOUT(win.corpus_state_name(), QString("Current"), 5000);
    QCOMPARE(corpus->text(), QString("Corpus: 2 chunks"));

    build->click();  
    QCOMPARE(win.corpus_state_name(), QString("Building"));
    QTRY_COMPARE_WITH_TIMEOUT(win.corpus_state_name(), QString("Current"), 5000);

    find_button(win, "Reset")->click();
    QCOMPARE(win.corpus_state_name(), QString("NotBuilt"));
    QCOMPARE(corpus->text(), QString("Corpus: Not built"));
}

// Decision 2 / R2 / R3
void StudentTests::test_workspace_change_mid_build_cancels() {
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    aiws::gui::MainWindow win;
    win.set_build_delay_ms(200);
    QVERIFY(win.add_document_file(make_file(dir, "a.txt")));
    auto* build = find_button(win, "Build Corpus");
    auto* corpus = find_label(win, "Corpus:");
    auto* status = find_label(win, "Current status");
    QVERIFY(build && corpus && status);

    build->click();
    QCOMPARE(win.corpus_state_name(), QString("Building"));
    QVERIFY(win.add_document_file(make_file(dir, "b.txt")));

    QCOMPARE(win.corpus_state_name(), QString("NotBuilt"));  // immediately
    QVERIFY(status->text().contains("cancelled"));
    QVERIFY(build->isEnabled());

    QTest::qWait(600);  // longer than the cancelled build would have taken
    QCOMPARE(win.corpus_state_name(), QString("NotBuilt"));
    QCOMPARE(corpus->text(), QString("Corpus: Not built"));
}

// Reset during a build: usable at once, and the old build's result never shows.
void StudentTests::test_reset_during_build() {
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    aiws::gui::MainWindow win;
    win.set_build_delay_ms(100);
    for (const char* name : {"a.txt", "b.txt", "c.txt"}) {
        QVERIFY(win.add_document_file(make_file(dir, name)));
    }
    auto* corpus = find_label(win, "Corpus:");
    auto* status = find_label(win, "Current status");
    QVERIFY(corpus && status);

    find_button(win, "Build Corpus")->click();
    find_button(win, "Reset")->click();
    QCOMPARE(win.corpus_state_name(), QString("NotBuilt"));
    QVERIFY(status->text().contains("Application reset."));
    QCOMPARE(win.findChild<QListWidget*>()->count(), 0);

    QVERIFY(win.add_document_file(make_file(dir, "d.txt")));
    find_button(win, "Build Corpus")->click();
    QTRY_COMPARE_WITH_TIMEOUT(win.corpus_state_name(), QString("Current"), 5000);
    QCOMPARE(corpus->text(), QString("Corpus: 1 chunks"));  // not the 3-document build
}

// CR-1

void StudentTests::test_cr1_remote_add_success() {
    aiws::gui::MainWindow win;
    const std::string socket_path = "/tmp/aiws_test_success.sock";

    aiws::gui::IpcServer server(socket_path, [&win](const std::string& path) {
        QString qpath = QString::fromStdString(path);
        QMetaObject::invokeMethod(&win, [&win, qpath]() {
            win.add_document_file(qpath);
        }, Qt::QueuedConnection);
    });
    QVERIFY(server.start());

    QTemporaryFile temp_file;
    QVERIFY(temp_file.open());
    temp_file.close();

    QProcess client;
    client.start(aiws_send_path(),
                 QStringList() << "--socket" << QString::fromStdString(socket_path)
                               << "add" << temp_file.fileName());
    QVERIFY(client.waitForFinished(3000));
    QCOMPARE(client.exitCode(), 0);

    auto* list = win.findChild<QListWidget*>();
    QVERIFY(list != nullptr);
    QTRY_COMPARE_WITH_TIMEOUT(list->count(), 1, 3000);

    server.stop();
}

void StudentTests::test_cr1_remote_add_invalid_file() {
    aiws::gui::MainWindow win;
    const std::string socket_path = "/tmp/aiws_test_invalid.sock";

    aiws::gui::IpcServer server(socket_path, [&win](const std::string& path) {
        QString qpath = QString::fromStdString(path);
        QMetaObject::invokeMethod(&win, [&win, qpath]() {
            win.add_document_file(qpath);
        }, Qt::QueuedConnection);
    });
    QVERIFY(server.start());

    QProcess client;
    client.start(aiws_send_path(),
                 QStringList() << "--socket" << QString::fromStdString(socket_path)
                               << "add" << "/tmp/this_file_does_not_exist_12345.txt");
    QVERIFY(client.waitForFinished(3000));
    QCOMPARE(client.exitCode(), 1);

    QTest::qWait(50);
    auto* list = win.findChild<QListWidget*>();
    QVERIFY(list != nullptr);
    QCOMPARE(list->count(), 0);

    server.stop();
}

void StudentTests::test_cr1_remote_add_unreachable() {
    const std::string socket_path = "/tmp/aiws_test_unreachable.sock";

    QTemporaryFile temp_file;
    QVERIFY(temp_file.open());
    temp_file.close();

    QProcess client;
    client.start(aiws_send_path(),
                 QStringList() << "--socket" << QString::fromStdString(socket_path)
                               << "add" << temp_file.fileName());
    QVERIFY(client.waitForFinished(3000));
    QCOMPARE(client.exitCode(), 2);
}

QTEST_MAIN(StudentTests)
#include "student_tests.moc"