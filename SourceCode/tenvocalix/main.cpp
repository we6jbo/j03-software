#include <QApplication>
#include "mainwindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("Tenvocalix");
    app.setApplicationVersion("0.1.0");
    app.setOrganizationName("Jeremiah O'Neal");
    MainWindow window;
    window.show();
    return app.exec();
}
