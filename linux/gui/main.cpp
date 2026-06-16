#include "MainWindow.hpp"
#include <QApplication>
#include <clocale>

int main(int argc, char** argv) {
    std::setlocale(LC_ALL, "");
    QApplication app(argc, argv);
    app.setApplicationName("rawaccel");
    app.setOrganizationName("rawaccel");

    MainWindow win;
    win.show();
    return app.exec();
}
