#pragma once
#include <QMainWindow>
#include <QStringList>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;
private slots:
    void openBox();
    void captureScreenshot();
private:
    Ui::MainWindow *ui;
    QStringList candyNames() const;
};
