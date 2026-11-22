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
#include <QTextStream>
#include <string>

#include "aiws/gui/main_window.hpp"
#include "aiws/gui/search_widget.hpp"
#include "aiws/ipc_server.hpp"

namespace {

// Writes a small readable text file into dir and returns its absolute path.
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

}  // namespace

class StudentTests : public QObject {
    Q_OBJECT

private slots:
    void test_r1_gui_not_blocked();
    void test_r2_stale_corpus_rejection();
    void test_r2_reset_during_build();
    void test_r3_failure_reporting();
    void test_r4_exit_during_build();

    void test_cr1_remote_add_success();
    void test_cr1_remote_add_invalid_file();
    void test_cr1_remote_add_unreachable();
};

// R1: starting a build returns immediately, the GUI stays usable, and a
// second click cannot start (or block on) another build.
void StudentTests::test_r1_gui_not_blocked() {
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    aiws::gui::MainWindow win;
    win.set_build_delay_ms(400);
    QVERIFY(win.add_document_file(make_file(dir, "a.txt")));

    auto* build = find_button(win, "Build Corpus");
    QVERIFY(build != nullptr);
    QVERIFY(build->isEnabled());

    QElapsedTimer t;
    t.start();
    build->click();
    QVERIFY(t.elapsed() < 200);   // click returned: no build on the GUI thread
    QVERIFY(!build->isEnabled()); // cannot start a second build
    build->click();               // disabled: must not freeze the window

    QVERIFY(win.add_document_file(make_file(dir, "b.txt")));  // GUI still usable
    QVERIFY(t.elapsed() < 300);

    QTRY_VERIFY_WITH_TIMEOUT(build->isEnabled(), 5000);
}

// R2: a build that finishes after the Workspace changed must be discarded.
void StudentTests::test_r2_stale_corpus_rejection() {
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    aiws::gui::MainWindow win;
    win.set_build_delay_ms(100);
    QVERIFY(win.add_document_file(make_file(dir, "a.txt")));

    auto* build = find_button(win, "Build Corpus");
    auto* corpus = find_label(win, "Corpus:");
    auto* status = find_label(win, "Current status");
    QVERIFY(build && corpus && status);

    build->click();
    QVERIFY(win.add_document_file(make_file(dir, "b.txt")));  // changes mid-build

    QTRY_VERIFY_WITH_TIMEOUT(status->text().contains("discarded"), 5000);
    QCOMPARE(corpus->text(), QString("Corpus: Not built"));
    QVERIFY(build->isEnabled());
}

// R2: Reset while a build runs must also make that build stale.
void StudentTests::test_r2_reset_during_build() {
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    aiws::gui::MainWindow win;
    win.set_build_delay_ms(100);
    QVERIFY(win.add_document_file(make_file(dir, "a.txt")));

    auto* corpus = find_label(win, "Corpus:");
    auto* status = find_label(win, "Current status");
    auto* reset = find_button(win, "Reset");
    QVERIFY(corpus && status && reset);

    find_button(win, "Build Corpus")->click();
    reset->click();

    QTRY_VERIFY_WITH_TIMEOUT(status->text().contains("discarded")
                                 || status->text().contains("built successfully"),
                             5000);
    QVERIFY(status->text().contains("discarded"));
    QCOMPARE(corpus->text(), QString("Corpus: Not built"));
}

// R3: a failing build is reported and the application stays usable.
// Adding the same file twice gives duplicate document IDs, so rebuild() throws.
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
    QTRY_VERIFY_WITH_TIMEOUT(status->text().contains("failed"), 5000);
    QTRY_VERIFY_WITH_TIMEOUT(build->isEnabled(), 5000);  // leaves building state
    QVERIFY(win.add_document_file(make_file(dir, "c.txt")));
}

// R4: destroying the window mid-build joins quickly; repeated use is fine.
void StudentTests::test_r4_exit_during_build() {
    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    QElapsedTimer t;
    t.start();
    {
        aiws::gui::MainWindow win;
        win.set_build_delay_ms(5000);
        QVERIFY(win.add_document_file(make_file(dir, "a.txt")));
        find_button(win, "Build Corpus")->click();
    }  // destroyed mid-build
    QVERIFY(t.elapsed() < 2000);

    for (int i = 0; i < 5; ++i) {
        aiws::gui::MainWindow w2;
        w2.set_build_delay_ms(50);
        QVERIFY(w2.add_document_file(make_file(dir, "a.txt")));
        auto* b = find_button(w2, "Build Corpus");
        QVERIFY(b != nullptr);
        b->click();
        QTRY_VERIFY_WITH_TIMEOUT(b->isEnabled(), 5000);
    }
}

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

    QTest::qWait(50);  // let any queued call run
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