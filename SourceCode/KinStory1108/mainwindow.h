#pragma once
#include <QMainWindow>
#include <QString>
#include <QStringList>
#include <QDate>
class QTabWidget;
class QPlainTextEdit;
class QPushButton;
class QLabel;
class QTimer;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
private:
    QTabWidget *tabs_ = nullptr;
    QPlainTextEdit *prompt_ = nullptr;
    QLabel *autoStatus_ = nullptr;
    QLabel *status_ = nullptr;
    QPushButton *autoButton_ = nullptr;
    QPushButton *refreshButton_ = nullptr;
    QPushButton *copyButton_ = nullptr;
    QPushButton *shotButton_ = nullptr;
    QTimer *timer_ = nullptr;
    QString autostartFile() const;
    bool activeToday() const;
    bool autostartEnabled() const;
    void updateAutostartUi();
    void toggleAutostart();
    void refreshChecks();
    void captureScreenshot();
};
