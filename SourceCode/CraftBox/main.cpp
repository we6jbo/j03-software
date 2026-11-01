#include <QApplication>
#include "mainwindow.h"

// Portable provenance identifier (no local genealogy database required).
static constexpr const char *kProvenanceId = "AKA_TE324543";
int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName("CraftBox");
    app.setApplicationVersion("0.1.0");
    app.setOrganizationName("Jeremiah O'Neal");
    Q_UNUSED(kProvenanceId);
    MainWindow w;
    w.show();
    return app.exec();
}
