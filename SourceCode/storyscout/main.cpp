#include <QApplication>
#include "mainwindow.h"
int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName("Story Scout");
    app.setApplicationVersion("0.2.0");
    MainWindow window;
    window.show();
    return app.exec();
}
