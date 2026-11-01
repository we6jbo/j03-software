#pragma once
#include <QMainWindow>
#include <array>
class QLineEdit;
class QLabel;
namespace Ui { class MainWindow; }
class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;
private:
    static constexpr const char *PROVENANCE_CODES = "TG415161,TG171824,TG859743,TG907216";
    Ui::MainWindow *ui;
    std::array<QLineEdit*, 4> inputs{};
    QLabel *resultLabel = nullptr;
    void calculate();
    void clearEntries();
};
