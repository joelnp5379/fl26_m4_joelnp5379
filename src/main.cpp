#include "aiws/gui/main_window.hpp"
#include "aiws/ipc_server.hpp"

#include <QApplication>
#include <QMessageBox>
#include <QCommandLineParser>
#include <QCommandLineOption>
#include <QMetaObject>

#include <cstdlib>
#include <iostream>
#include <string>
#include <unistd.h>

void confirm_exit(QWidget* parent) {
    auto response = QMessageBox::question(
        parent, "Confirm Exit", "Are you sure you want to exit?",
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No
    );
    if (response == QMessageBox::Yes) {
        QApplication::quit();
    }
}

std::string get_default_socket() {
    const char* user = std::getenv("USER");
    if (user != nullptr && user[0] != '\0') {
        return "/tmp/aiws-" + std::string(user) + ".sock";
    }
    return "/tmp/aiws-" + std::to_string(getuid()) + ".sock";
}

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);

    QCommandLineParser parser;
    parser.addHelpOption();

    QCommandLineOption delay_opt("build-delay-ms", "Build delay per document in milliseconds.", "ms", "0");
    QCommandLineOption socket_opt("ipc-socket", "Path to IPC Unix domain socket.", "path", QString::fromStdString(get_default_socket()));

    parser.addOption(delay_opt);
    parser.addOption(socket_opt);
    parser.process(app);

    bool ok = false;
    int delay_ms = parser.value(delay_opt).toInt(&ok);
    if (!ok || delay_ms < 0) {
        std::cerr << "Invalid build delay parameter.\n";
        return 1;
    }

    std::string socket_path = parser.value(socket_opt).toStdString();

    aiws::gui::MainWindow window;
    window.set_build_delay_ms(delay_ms);
    window.set_exit_callback(&confirm_exit);

    // Initialize IPC Server with thread-safe queueing to the GUI thread
    aiws::gui::IpcServer ipc_server(socket_path, [&window](const std::string& path) {
        QString qpath = QString::fromStdString(path);
        bool dispatched = QMetaObject::invokeMethod(&window, [&window, qpath]() {
            window.add_document_file(qpath);
        }, Qt::QueuedConnection);

        if (!dispatched) {
            throw std::runtime_error("Failed to dispatch document addition to GUI thread.");
        }
    });

    if (!ipc_server.start()) {
        std::cerr << "Failed to bind and start IPC socket server at: " << socket_path << "\n";
        return 1;
    }

    window.show();
    int exit_code = app.exec();
    
    ipc_server.stop();
    return exit_code;
}
