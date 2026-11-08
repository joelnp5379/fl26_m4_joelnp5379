#include "aiws/gui/main_window.hpp"

#include <QApplication>
#include <QMessageBox>

//ExitCallback function
void confirm_exit(QWidget* parent) {
    // Display a confirmation dialog to the user
    auto answer = QMessageBox::question(parent, 
                                        "Exit AI Workspace", 
                                        "Are you sure you want to quit the application?",
                                        QMessageBox::Yes | QMessageBox::No);
    
    if (answer == QMessageBox::Yes) {
        QApplication::quit();
    }
}

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);

    aiws::gui::MainWindow window;

    window.set_exit_callback(&confirm_exit);

    window.show();

    return app.exec();
}
