#include <QApplication>
#include <QCoreApplication>
#include "mainwindow.h"
int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    QCoreApplication::setApplicationName("KinStory1108");
    QCoreApplication::setApplicationVersion("0.1.0");
    MainWindow w;
    w.show();
    return app.exec();
}
