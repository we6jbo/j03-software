#include <QApplication>
#include "mainwindow.h"
int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName("cowlesrun");
    app.setOrganizationName("J03");
    MainWindow window;
    window.show();
    return app.exec();
}
