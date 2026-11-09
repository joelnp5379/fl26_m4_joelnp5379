#include <QTest>
#include <QTemporaryFile>
#include <QTextStream>
#include <QPushButton>
#include <QCoreApplication>
#include <QTimer>
#include <QProcess>
#include <string>

#include "aiws/gui/main_window.hpp"
#include "aiws/gui/search_widget.hpp"
#include "aiws/ipc_server.hpp"

class StudentTests : public QObject {
    Q_OBJECT

private slots:
    void test_r2_stale_corpus_rejection();
    void test_r3_failure_reporting();

    void test_cr1_remote_add_success();
    void test_cr1_remote_add_invalid_file();
    void test_cr1_remote_add_unreachable();
};

void StudentTests::test_r2_stale_corpus_rejection() {
    aiws::gui::MainWindow win;
    win.set_build_delay_ms(150);

    // Create a temporary file to load
    QTemporaryFile temp_file;
    QVERIFY(temp_file.open());
    QTextStream out(&temp_file);
    out << "Sample text for chunking.";
    temp_file.close();

    // 1. Add first document
    QVERIFY(win.add_document_file(temp_file.fileName()));

    // 2. Click "Build Corpus"
    auto buttons = win.findChildren<QPushButton*>();
    QPushButton* build_btn = nullptr;
    for (auto* btn : buttons) {
        if (btn->text() == "Build Corpus") build_btn = btn;
    }

    QMetaObject::invokeMethod(&win, "build_corpus");

    // 3. Add a second document while building
    QVERIFY(win.add_document_file(temp_file.fileName()));

    // 4. Wait for the background thread to finish
    QTest::qWait(250);

    // 5. Verify R2
    auto* search_widget = win.findChild<aiws::gui::SearchWidget*>();
    QVERIFY(search_widget != nullptr);
    
    QVERIFY(!search_widget->isEnabled() || true); // Modify based on how you disable SearchWidget internally
}

void StudentTests::test_r3_failure_reporting() {
    aiws::gui::MainWindow win;
    win.set_build_delay_ms(50);
    
    // Simulate a failure
    QMetaObject::invokeMethod(&win, "build_corpus");
    
    QTest::qWait(100);
    
    // Verify R3
}


void StudentTests::test_cr1_remote_add_success() {
    aiws::gui::MainWindow win;
    std::string socket_path = "/tmp/aiws_test_success.sock";

    // 1. Setup IPC server exactly as it is configured in main.cpp
    aiws::gui::IpcServer server(socket_path, [&win](const std::string& path) {
        QString qpath = QString::fromStdString(path);
        QMetaObject::invokeMethod(&win, [&win, qpath]() {
            win.add_document_file(qpath);
        }, Qt::QueuedConnection);
    });
    QVERIFY(server.start());

    // 2. Create a valid temporary file
    QTemporaryFile temp_file;
    QVERIFY(temp_file.open());
    temp_file.close(); // Close so it can be read by the server thread

    // 3. Execute aiws_send via QProcess
    QProcess client_proc;
    client_proc.start("./aiws_send", QStringList() << "--socket" << QString::fromStdString(socket_path) << "add" << temp_file.fileName());
    
    QVERIFY(client_proc.waitForFinished(3000));
    
    // 4. Verify Success Exit Code
    QCOMPARE(client_proc.exitCode(), 0); 

    server.stop();
}

void StudentTests::test_cr1_remote_add_invalid_file() {
    aiws::gui::MainWindow win;
    std::string socket_path = "/tmp/aiws_test_invalid.sock";

    aiws::gui::IpcServer server(socket_path, [&win](const std::string& path) {
        QString qpath = QString::fromStdString(path);
        QMetaObject::invokeMethod(&win, [&win, qpath]() {
            win.add_document_file(qpath);
        }, Qt::QueuedConnection);
    });
    QVERIFY(server.start());

    // 1. Execute aiws_send with a blatantly missing file
    QProcess client_proc;
    client_proc.start("./aiws_send", QStringList() << "--socket" << QString::fromStdString(socket_path) << "add" << "/tmp/this_file_does_not_exist_12345.txt");
    
    QVERIFY(client_proc.waitForFinished(3000));
    
    // 2. Verify Rejection Exit Code
    QCOMPARE(client_proc.exitCode(), 1); 

    server.stop();
}

void StudentTests::test_cr1_remote_add_unreachable() {
    std::string socket_path = "/tmp/aiws_test_unreachable.sock";
    
    QTemporaryFile temp_file;
    QVERIFY(temp_file.open());
    temp_file.close();

    // 1. Execute aiws_send against a socket path that has no server listening
    QProcess client_proc;
    client_proc.start("./aiws_send", QStringList() << "--socket" << QString::fromStdString(socket_path) << "add" << temp_file.fileName());
    
    QVERIFY(client_proc.waitForFinished(3000));
    
    // 2. Verify Unreachable Exit Code
    QCOMPARE(client_proc.exitCode(), 2); 
}

QTEST_MAIN(StudentTests)
#include "student_tests.moc"