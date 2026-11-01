#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QCryptographicHash>
#include <QDateTime>
#include <QDir>
#include <QFileInfo>
#include <QFont>
#include <QPushButton>
#include <QImage>
#include <QMessageBox>
#include <QPainter>
#include <QRandomGenerator>
#include <QStandardPaths>
#include <QStatusBar>
#include <QVector>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow) {
    ui->setupUi(this);
    connect(ui->openButton, &QPushButton::clicked, this, &MainWindow::openBox);
    connect(ui->screenshotButton, &QPushButton::clicked, this, &MainWindow::captureScreenshot);
    statusBar()->showMessage("Click the box to reveal three original make-believe candies.");
    setMinimumSize(600, 460);
}
MainWindow::~MainWindow() { delete ui; }

QStringList MainWindow::candyNames() const {
    // Original made-up words; not intended to refer to existing brands.
    return {"Zizzlewhirl Pebble", "Floopberry Prism", "Nimblenog Swirl",
            "Quibberglow Drop", "Tafflune Spark", "Blooplix Ribbon",
            "Wobblefern Twist", "Pufflewink Orb", "Dazzlenib Cluster",
            "Mizzlepop Comet", "Snorklefluff Cube", "Twinklemoss Wafer",
            "Glorbinoodle Gem", "Fizzlequark Loop", "Jumblewisp Bite",
            "Crinklepuff Stardust", "Plimblebean Curl", "Zonkletwist Pearl"};
}

void MainWindow::openBox() {
    const QStringList candies = candyNames();
    QVector<int> pool;
    for (int i=0; i<candies.size(); ++i) pool.append(i);
    // Select without replacement; exactly three different items per click.
    QStringList result;
    for (int i=0; i<3; ++i) {
        const int index = QRandomGenerator::global()->bounded(pool.size());
        result << candies.at(pool.takeAt(index));
    }
    ui->candyOne->setText(QStringLiteral("✦  ") + result.at(0));
    ui->candyTwo->setText(QStringLiteral("✿  ") + result.at(1));
    ui->candyThree->setText(QStringLiteral("◆  ") + result.at(2));
    ui->resultHeading->setText("You discovered three candies!");
    statusBar()->showMessage("A fresh assortment! Click again for another set.", 4000);
}

void MainWindow::captureScreenshot() {
    const QString root = QDir::homePath() + "/.38bTqEvQc/craftbox";
    if (!QDir().mkpath(root)) {
        statusBar()->showMessage("Screenshot failed: cannot create capture folder.", 6000);
        return;
    }
    QImage capture = grab().toImage().convertToFormat(QImage::Format_ARGB32_Premultiplied);
    // Burn attribution into pixels, beneath the app window.
    QImage composed(capture.width(), capture.height() + 36, QImage::Format_ARGB32_Premultiplied);
    composed.fill(Qt::white);
    {
        QPainter painter(&composed);
        painter.drawImage(0, 0, capture);
        painter.setPen(QColor("#232323"));
        painter.setFont(QFont("Sans", 9));
        painter.drawText(QRect(10, capture.height(), capture.width()-20, 36),
                         Qt::AlignVCenter | Qt::AlignLeft,
                         "Jeremiah O'Neal | CraftBox 0.1 | Review pending | No publication permission implied");
    }
    const QByteArray stamp = QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss_zzz").toUtf8();
    const QByteArray seed = stamp + QByteArray::number(QRandomGenerator::global()->generate64());
    const QString tag = QString::fromLatin1(QCryptographicHash::hash(seed, QCryptographicHash::Sha256).toHex().left(16));
    const QString path = root + "/craftbox_v0.1_main-window_" + QString::fromUtf8(stamp) + "_" + tag + ".png";
    if (!composed.save(path, "PNG")) {
        statusBar()->showMessage("Screenshot failed: unable to save PNG.", 6000);
        return;
    }
    // Capture is intentionally local only, no upload and no modal popup.
    statusBar()->showMessage("Screenshot saved locally for review: " + QFileInfo(path).fileName(), 7000);
}
