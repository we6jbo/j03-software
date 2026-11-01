#pragma once
#include <QMainWindow>
#include <QElapsedTimer>
#include <QTimer>
#include <QSoundEffect>
#include <QProcess>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

private:
    enum class Mode { Clock, Countdown, Alarm };
    static constexpr int CountdownSeconds = 10;
    static constexpr int BeepCount = 5;
    static constexpr const char *ProvenanceCode = "TG654890";
    static constexpr const char *Version = "0.1.0";

    Ui::MainWindow *ui;
    Mode mode = Mode::Clock;
    QTimer clockTimer;
    QTimer countdownTimer;
    QTimer alarmTimer;
    QElapsedTimer elapsed;
    QSoundEffect sound;
    int beepsPlayed = 0;
    QList<QProcess *> screenshotRegistrations;

    void showClock();
    void startCountdown();
    void updateCountdown();
    void enterAlarm();
    void playNextBeep();
    void finishAlarm();
    void takeScreenshot();
};
