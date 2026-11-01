#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QDateTime>
#include <QDir>
#include <QFileInfo>
#include <QDebug>
#include <QStandardPaths>
#include <QScreen>
#include <QGuiApplication>
#include <QtGlobal>
#include <QUrl>
#include <QSignalBlocker>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    ui->provenanceLabel->setText(QString("Version %1 | Jeremiah O'Neal | %2")
                                    .arg(Version, ProvenanceCode));
    sound.setSource(QUrl("qrc:/beep.wav"));
    sound.setLoopCount(1);
    sound.setVolume(0.85f);
    connect(&sound, &QSoundEffect::statusChanged, this, [this] {
        if (sound.status() == QSoundEffect::Error)
            qWarning() << "Alarm audio could not be loaded; visual alarm still works.";
    });

    clockTimer.setInterval(200);
    connect(&clockTimer, &QTimer::timeout, this, [this] {
        if (mode == Mode::Clock) showClock();
    });
    clockTimer.start();

    countdownTimer.setInterval(25);
    connect(&countdownTimer, &QTimer::timeout, this, &MainWindow::updateCountdown);
    alarmTimer.setInterval(450);
    connect(&alarmTimer, &QTimer::timeout, this, &MainWindow::playNextBeep);

    connect(ui->startButton, &QPushButton::clicked, this, &MainWindow::startCountdown);
    connect(ui->screenshotButton, &QPushButton::clicked, this, &MainWindow::takeScreenshot);
    showClock();
}

MainWindow::~MainWindow()
{
    clockTimer.stop();
    countdownTimer.stop();
    alarmTimer.stop();
    sound.stop();
    for (QProcess *proc : screenshotRegistrations) {
        // Registration is best-effort; never leave a running child behind on exit.
        if (proc->state() != QProcess::NotRunning) {
            proc->kill();
            proc->waitForFinished(300);
        }
    }
    delete ui;
}

void MainWindow::showClock()
{
    ui->mainDisplay->setText(QDateTime::currentDateTime().toString("hh:mm:ss AP"));
    ui->modeLabel->setText("CURRENT TIME");
    ui->startButton->setEnabled(true);
    ui->startButton->setVisible(true);
}

void MainWindow::startCountdown()
{
    if (mode != Mode::Clock) return;
    mode = Mode::Countdown;
    elapsed.start();
    ui->startButton->setEnabled(false);
    ui->modeLabel->setText("COUNTDOWN");
    ui->mainDisplay->setText(QString::number(CountdownSeconds));
    countdownTimer.start();
}

void MainWindow::updateCountdown()
{
    if (mode != Mode::Countdown) return;
    const qint64 remainingMs = qMax<qint64>(0, CountdownSeconds * 1000LL - elapsed.elapsed());
    const int remainingSecs = static_cast<int>((remainingMs + 999) / 1000);
    ui->mainDisplay->setText(QString::number(remainingSecs));
    if (remainingMs == 0) {
        countdownTimer.stop();
        ui->mainDisplay->setText("0");
        enterAlarm();
    }
}

void MainWindow::enterAlarm()
{
    mode = Mode::Alarm;
    beepsPlayed = 0;
    ui->modeLabel->setText("COUNTDOWN COMPLETE");
    ui->mainDisplay->setText("ALARM");
    playNextBeep(); // beep one immediately at countdown completion
    alarmTimer.start();
}

void MainWindow::playNextBeep()
{
    if (mode != Mode::Alarm) return;
    if (beepsPlayed >= BeepCount) {
        alarmTimer.stop();
        finishAlarm();
        return;
    }
    ++beepsPlayed;
    if (sound.status() == QSoundEffect::Ready) {
        sound.stop();
        sound.play();
    } else {
        qWarning() << "Beep" << beepsPlayed << "of" << BeepCount
                   << "could not play; audio device or asset unavailable.";
    }
    // A timer tick after the fifth 180 ms beep gives it time to finish.
}

void MainWindow::finishAlarm()
{
    sound.stop();
    mode = Mode::Clock;
    showClock();
}

void MainWindow::takeScreenshot()
{
    const QString directory = QDir::homePath() + "/.38bTqEvQc/tenvocalix";
    if (!QDir().mkpath(directory)) {
        qWarning() << "Could not create screenshot directory:" << directory;
        return;
    }
    const QString dateString = QDateTime::currentDateTime().toString("yyyyMMdd-HHmmss-zzz");
    const QString base = QString("tenvocalix-v%1-%2").arg(Version, dateString);
    QString filename = directory + "/" + base + ".png";
    int suffix = 1;
    while (QFileInfo::exists(filename))
        filename = directory + "/" + base + "-" + QString::number(suffix++) + ".png";
    if (!this->grab().save(filename, "PNG")) {
        qWarning() << "Screenshot save failed:" << filename;
        return;
    }
    qInfo() << "Screenshot saved:" << filename;

    const QString executable = QStandardPaths::findExecutable("infoartrecorder");
    if (executable.isEmpty()) {
        qWarning() << "infoartrecorder unavailable; screenshot preserved pending manual registration:" << filename;
        return;
    }
    auto *proc = new QProcess(this);
    screenshotRegistrations.append(proc);
    connect(proc, &QProcess::finished, this, [this, proc, filename](int exitCode, QProcess::ExitStatus exitStatus) {
        if (exitStatus != QProcess::NormalExit || exitCode != 0)
            qWarning() << "Screenshot registry failed; file preserved:" << filename
                       << proc->readAllStandardError();
        else
            qInfo() << "Screenshot submitted for registry review:" << filename;
        screenshotRegistrations.removeAll(proc);
        proc->deleteLater();
    });
    connect(proc, &QProcess::errorOccurred, this, [filename](QProcess::ProcessError error) {
        qWarning() << "Screenshot registration error:" << error << filename;
    });
    proc->start(executable, {"add", filename, "--project", "tenvocalix"});
}
