#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QLineEdit>
#include <QLabel>
#include <QPushButton>
#include <QPixmap>
#include <QMessageBox>
#include <QRegularExpression>
#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QStringList>
#include <QStatusBar>
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <cmath>

namespace {
// Input is MM:SS (minutes may exceed 59), or HH:MM:SS.
bool parseDuration(const QString &text, qint64 &seconds) {
    static const QRegularExpression re(R"(^\s*(\d{1,4}):(\d{2})(?::(\d{2}))?\s*$)");
    auto m = re.match(text);
    if (!m.hasMatch()) return false;
    bool ok = false;
    qint64 first = m.captured(1).toLongLong(&ok);
    if (!ok) return false;
    qint64 middle = m.captured(2).toLongLong(&ok);
    if (!ok) return false;
    if (m.captured(3).isEmpty()) {
        if (middle > 59) return false;
        seconds = first * 60 + middle;
    } else {
        qint64 last = m.captured(3).toLongLong(&ok);
        if (!ok || middle > 59 || last > 59) return false;
        seconds = first * 3600 + middle * 60 + last;
    }
    return seconds > 0;
}
QString formatDuration(qint64 seconds) {
    const qint64 hours = seconds / 3600;
    const qint64 minutes = (seconds % 3600) / 60;
    const qint64 secs = seconds % 60;
    if (hours) return QString("%1:%2:%3").arg(hours).arg(minutes, 2, 10, QChar('0')).arg(secs, 2, 10, QChar('0'));
    return QString("%1:%2").arg(minutes).arg(secs, 2, 10, QChar('0'));
}
}

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow) {
    ui->setupUi(this);
    inputs = {ui->run1Edit, ui->run2Edit, ui->run3Edit, ui->run4Edit};
    resultLabel = ui->averageLabel;
    const QString path = QCoreApplication::applicationDirPath() + "/../share/cowlesrun/killdeer_mountains_illustration.png";
    QStringList candidates = {":/art/killdeer_mountains_illustration.png", path};
    for (const auto &p : candidates) {
        QPixmap image(p);
        if (!image.isNull()) { ui->mountainImage->setPixmap(image.scaled(232, 146, Qt::KeepAspectRatio, Qt::SmoothTransformation)); break; }
    }
    ui->mountainImage->setToolTip("Artistic illustration inspired by the Killdeer Mountains, North Dakota; not a location photograph.");
    for (QLineEdit *edit : inputs) connect(edit, &QLineEdit::returnPressed, this, &MainWindow::calculate);
    connect(ui->calculateButton, &QPushButton::clicked, this, &MainWindow::calculate);
    connect(ui->clearButton, &QPushButton::clicked, this, &MainWindow::clearEntries);
    QMenu *help = menuBar()->addMenu("Help");
    QAction *about = help->addAction("About cowlesrun");
    connect(about, &QAction::triggered, this, [this] {
        QMessageBox::about(this, "About cowlesrun", "cowlesrun v0.2.0\nFour-day Cowles Mountain summit time average\n\nProvenance identifiers: " + QString::fromUtf8(PROVENANCE_CODES) + "\nArtwork: original AI-generated artistic illustration of the Killdeer Mountains landscape; not a photograph.\nNo network connection or account required.");
    });
    statusBar()->showMessage("Enter between one and four summit times. Times are not stored.");
}
MainWindow::~MainWindow() { delete ui; }
void MainWindow::calculate() {
    qint64 sum = 0;
    int count = 0;
    for (int i=0; i<4; ++i) {
        const QString value = inputs[i]->text().trimmed();
        if (value.isEmpty()) continue;
        qint64 seconds = 0;
        if (!parseDuration(value, seconds)) {
            QMessageBox::warning(this, "Invalid run time", QString("Run %1 must use MM:SS or HH:MM:SS. Seconds must be 00-59, and time must be greater than zero.").arg(i+1));
            inputs[i]->setFocus();
            return;
        }
        sum += seconds;
        ++count;
    }
    if (!count) { resultLabel->setText("Enter at least one run time."); return; }
    const qint64 roundedMean = static_cast<qint64>(std::llround(static_cast<double>(sum) / count));
    resultLabel->setText(QString("Average: %1   (%2 of 4 days)").arg(formatDuration(roundedMean)).arg(count));
    statusBar()->showMessage("Mean rounded to the nearest second; no information was saved.");
}
void MainWindow::clearEntries() {
    for (auto *input : inputs) input->clear();
    resultLabel->setText("Average: --:--");
    inputs[0]->setFocus();
}
