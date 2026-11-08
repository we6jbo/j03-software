#pragma once
#include <QMainWindow>
#include <QJsonArray>
#include <QMap>
#include <QString>
#include <QVector>
class QListWidget;
class QLineEdit;
class QTextEdit;
class QLabel;
class QCloseEvent;
namespace Ui { class MainWindow; }
class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;
private:
    Ui::MainWindow *ui;
    QListWidget *list = nullptr;
    QLabel *status = nullptr;
    QLineEdit *filter = nullptr;
    QMap<QString,QLineEdit*> edits;
    QMap<QString,QTextEdit*> multiline;
    QJsonArray people;
    int current=-1;
    bool dirty=false;
    QString root;
    void createEditor();
    void loadPeople();
    void showRecord(int i);
    void rememberCurrent();
    void refreshList();
    QString sourceFor(const QString &name) const;
    bool saveAll();
    void screenshot();
    void registerScreenshot(const QString &path);
    void closeEvent(QCloseEvent *event) override;
};
