#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QApplication>
#include <QCloseEvent>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMessageBox>
#include <QProcess>
#include <QPushButton>
#include <QRegularExpression>
#include <QScrollArea>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QStandardPaths>
#include <QSaveFile>
#include <QScreen>
#include <QTimer>
#include <QUrl>
#include <QDebug>
#include <QPainter>
#include <QColor>
#include <QFont>

static constexpr const char *PROVENANCE = "AKA_TE324543,TG654147";
static constexpr const char *VERSION = "0.2.1";
static const QStringList fields={"name","tg","birth_date","birth_place","death_date","death_place","father","mother","step_father","step_mother","spouses","children","parent_links","description","website"};

MainWindow::MainWindow(QWidget *parent):QMainWindow(parent),ui(new Ui::MainWindow) {
    ui->setupUi(this);
    root=QDir::homePath()+"/storyscout";
    QDir().mkpath(root);
    if(!QFile::exists(root+"/include.txt")) {
        QFile inc(root+"/include.txt");
        if(inc.open(QIODevice::WriteOnly|QIODevice::NewOnly|QIODevice::Text)) inc.close();
    }
    createEditor();
    loadPeople();
}
MainWindow::~MainWindow(){delete ui;}
QString MainWindow::sourceFor(const QString &name) const {
    static const QStringList family={"luedtke","lüdtke","luedke","ludtke","hedtke","rice","stafford","shapley","mcquiston","mckissock","platt","hillman","beck","mccabe","mcavoy","mcevoy","sloan","schlorf"};
    const QString lower=name.toLower();
    for(const QString &word:family){
        const QRegularExpression re("(^|[^\\p{L}])"+QRegularExpression::escape(word)+"([^\\p{L}]|$)",QRegularExpression::CaseInsensitiveOption);
        if(lower.contains(re)) return "https://luedtkerice.j03.page/";
    }
    return "https://j03.page/";
}
void MainWindow::createEditor(){
    auto *outer=ui->contentLayout;
    auto *tools=new QHBoxLayout;
    filter=new QLineEdit(this);filter->setPlaceholderText("Search by name or TG code");
    auto *add=new QPushButton("Add person",this);
    auto *save=new QPushButton("Save all",this);
    auto *capture=new QPushButton("Screenshot",this);
    tools->addWidget(filter,1);tools->addWidget(add);tools->addWidget(save);tools->addWidget(capture);
    outer->addLayout(tools);
    auto *split=new QHBoxLayout;
    list=new QListWidget(this);list->setMinimumWidth(310);list->setMaximumWidth(380);split->addWidget(list);
    auto *scroll=new QScrollArea(this);scroll->setWidgetResizable(true);
    auto *panel=new QWidget(scroll);auto *form=new QFormLayout(panel);
    const QMap<QString,QString> titles={{"name","Full name"},{"tg","TG / AKA codes"},{"birth_date","Date of birth"},{"birth_place","Place of birth"},{"death_date","Date of death"},{"death_place","Place of death"},{"father","Father and TG"},{"mother","Mother and TG"},{"step_father","Stepfather and TG"},{"step_mother","Stepmother and TG"},{"spouses","Spouses and TG (one per line)"},{"children","Children by spouse and TG (one per line)"},{"parent_links","Recorded parent links (TG when known)"},{"description","Full description / evidence"},{"website","Source website"}};
    for(const QString &key:fields){
        if(key=="description"||key=="children"||key=="spouses"||key=="parent_links"){
            auto *edit=new QTextEdit(panel);edit->setMinimumHeight(key=="description"?170:85);multiline[key]=edit;form->addRow(titles.value(key),edit);
            connect(edit,&QTextEdit::textChanged,this,[this](){dirty=true;});
        }else{
            auto *edit=new QLineEdit(panel);edits[key]=edit;form->addRow(titles.value(key),edit);
            connect(edit,&QLineEdit::textEdited,this,[this](){dirty=true;});
        }
    }
    scroll->setWidget(panel);split->addWidget(scroll,1);outer->addLayout(split,1);
    status=new QLabel("",this);outer->addWidget(status);
    connect(filter,&QLineEdit::textChanged,this,[this](){refreshList();});
    connect(list,&QListWidget::currentRowChanged,this,[this](int row){
        rememberCurrent();if(row<0)return;
        const int idx=list->item(row)->data(Qt::UserRole).toInt();showRecord(idx);
    });
    connect(add,&QPushButton::clicked,this,[this](){
        rememberCurrent();QJsonObject obj;for(const QString &k:fields)obj[k]="";
        obj["name"]="New person";obj["website"]="https://j03.page/";
        people.append(obj);dirty=true;filter->clear();refreshList();showRecord(people.size()-1);
        for(int i=0;i<list->count();++i)if(list->item(i)->data(Qt::UserRole).toInt()==current){list->setCurrentRow(i);break;}
    });
    connect(save,&QPushButton::clicked,this,[this](){rememberCurrent();saveAll();});
    connect(capture,&QPushButton::clicked,this,&MainWindow::screenshot);
}
void MainWindow::loadPeople(){
    const QString dbpath=root+"/story.db";
    if(QFile::exists(dbpath)){
        auto db=QSqlDatabase::addDatabase("QSQLITE","storyscout-read");db.setDatabaseName(dbpath);
        if(db.open()){
            QSqlQuery q(db);
            if(q.exec("SELECT data_json FROM people ORDER BY id")){
                while(q.next()){const auto doc=QJsonDocument::fromJson(q.value(0).toByteArray());if(doc.isObject())people.append(doc.object());}
            }
            db.close();
        }
        db=QSqlDatabase();QSqlDatabase::removeDatabase("storyscout-read");
    }
    if(people.isEmpty()){
        QString seedPath=QCoreApplication::applicationDirPath()+"/seed_people.json";
        if(!QFile::exists(seedPath))seedPath=QCoreApplication::applicationDirPath()+"/../seed_people.json";
        QFile file(seedPath);
        if(file.open(QIODevice::ReadOnly)){
            auto doc=QJsonDocument::fromJson(file.readAll());if(doc.isArray())people=doc.array();
        }
        if(people.isEmpty())status->setText("No seed records found. You can add people manually.");
    }
    refreshList();if(list->count())list->setCurrentRow(0);
}
void MainWindow::rememberCurrent(){
    if(current<0||current>=people.size())return;
    QJsonObject obj=people[current].toObject();
    for(auto it=edits.begin();it!=edits.end();++it)obj[it.key()]=it.value()->text();
    for(auto it=multiline.begin();it!=multiline.end();++it)obj[it.key()]=it.value()->toPlainText();
    people[current]=obj;
}
void MainWindow::showRecord(int i){
    if(i<0||i>=people.size())return;
    current=i;QJsonObject p=people[i].toObject();
    for(auto it=edits.begin();it!=edits.end();++it)it.value()->setText(p.value(it.key()).toString());
    for(auto it=multiline.begin();it!=multiline.end();++it)it.value()->setPlainText(p.value(it.key()).toString());
    status->setText(QString("Record %1 of %2 | %3").arg(i+1).arg(people.size()).arg(PROVENANCE));
}
void MainWindow::refreshList(){
    const int previous=current;
    list->blockSignals(true);list->clear();
    const QString needle=filter->text().trimmed();
    for(int i=0;i<people.size();++i){auto p=people[i].toObject();const QString label=p.value("name").toString();const QString tg=p.value("tg").toString();
        if(!needle.isEmpty()&&!label.contains(needle,Qt::CaseInsensitive)&&!tg.contains(needle,Qt::CaseInsensitive))continue;
        auto *item=new QListWidgetItem(label+(tg.isEmpty()?"":"   ["+tg+"]"),list);item->setData(Qt::UserRole,i);
    }
    for(int n=0;n<list->count();++n)if(list->item(n)->data(Qt::UserRole).toInt()==previous){list->setCurrentRow(n);break;}
    list->blockSignals(false);
}
bool MainWindow::saveAll(){
    const QString path=root+"/story.db";
    QString include;
    QFile inc(root+"/include.txt");if(inc.open(QIODevice::ReadOnly|QIODevice::Text))include=QString::fromUtf8(inc.readAll());
    QJsonArray normalized;
    for(const auto &v:people){
        QJsonObject p=v.toObject();const QString name=p.value("name").toString().trimmed();
        if(name.isEmpty()){QMessageBox::warning(this,"Cannot save","Every person needs a name.");return false;}
        QString url=p.value("website").toString().trimmed();if(url.isEmpty())url=sourceFor(name);
        QUrl parsed(url);if(!parsed.isValid()||!parsed.host().length()||(parsed.scheme()!="https"&&parsed.scheme()!="http")){
            QMessageBox::warning(this,"Cannot save","Invalid website for: "+name);return false;}
        p["website"]=url;p["include_text"]=include;p["provenance"]=PROVENANCE;normalized.append(p);
    }
    if(QFile::exists(path)) {
        const QString backup=path+".backup-"+QDateTime::currentDateTime().toString("yyyyMMdd-HHmmsszzz");
        if(!QFile::copy(path,backup)) {
            QMessageBox::critical(this,"Backup failed","Refusing to replace database without creating a backup: "+backup);
            return false;
        }
    }
    auto db=QSqlDatabase::addDatabase("QSQLITE","storyscout-write");db.setDatabaseName(path);
    bool ok=false;QString error;
    if(!db.open())error=db.lastError().text();
    else {
        QSqlQuery q(db);
        if(!q.exec("CREATE TABLE IF NOT EXISTS people (id INTEGER PRIMARY KEY, display_name TEXT NOT NULL, tg_codes TEXT, website TEXT NOT NULL, data_json TEXT NOT NULL, updated_at TEXT NOT NULL)"))error=q.lastError().text();
        else if(!db.transaction())error=db.lastError().text();
        else if(!q.exec("DELETE FROM people"))error=q.lastError().text();
        else {
            ok=true;
            for(int i=0;i<normalized.size();++i){const auto p=normalized[i].toObject();
                q.prepare("INSERT INTO people(id,display_name,tg_codes,website,data_json,updated_at) VALUES(?,?,?,?,?,?)");
                q.addBindValue(i+1);q.addBindValue(p.value("name").toString());q.addBindValue(p.value("tg").toString());q.addBindValue(p.value("website").toString());q.addBindValue(QString::fromUtf8(QJsonDocument(p).toJson(QJsonDocument::Compact)));q.addBindValue(QDateTime::currentDateTime().toString(Qt::ISODate));
                if(!q.exec()){error=q.lastError().text();ok=false;break;}
            }
            if(ok){if(!db.commit()){error=db.lastError().text();ok=false;}}else db.rollback();
        }
        db.close();
    }
    db=QSqlDatabase();QSqlDatabase::removeDatabase("storyscout-write");
    if(ok){people=normalized;dirty=false;status->setText(QString("Saved %1 records to %2").arg(people.size()).arg(path));}
    else QMessageBox::critical(this,"Save failed",error);
    return ok;
}
void MainWindow::registerScreenshot(const QString &path){
    auto *proc=new QProcess(this);
    connect(proc,QOverload<int,QProcess::ExitStatus>::of(&QProcess::finished),this,[this,proc,path](int code,QProcess::ExitStatus st){
        if(st!=QProcess::NormalExit||code!=0){qWarning()<<"infoartrecorder registration failed for"<<path<<proc->readAllStandardError();status->setText("Screenshot saved; registry unavailable or failed (see terminal).");}
        proc->deleteLater();
    });
    connect(proc,&QProcess::errorOccurred,this,[this,proc](QProcess::ProcessError){qWarning()<<"infoartrecorder unavailable"<<proc->errorString();status->setText("Screenshot saved; registration unavailable (see terminal).");proc->deleteLater();});
    proc->start("infoartrecorder",{"add",path,"--project","storyscout"});
}
void MainWindow::screenshot(){
    const QString folder=QDir::homePath()+"/.38bTqEvQc/storyscout";
    QDir().mkpath(folder);
    const QString path=folder+"/storyscout_"+QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss_zzz")+"_v"+VERSION+".png";
    // Capture the real application, then append a readable attribution footer.
    // This does NOT redact genealogy data. Review the entire image before approval.
    const QPixmap window=grab();
    if(window.isNull()) { status->setText("Screenshot failed: empty window capture"); return; }
    const qreal dpr=window.devicePixelRatioF();
    const int footerLogicalHeight=112;
    const int footerPx=qRound(footerLogicalHeight*dpr);
    QImage img(window.width(),window.height()+footerPx,QImage::Format_ARGB32_Premultiplied);
    img.fill(QColor("#ffffff"));
    {
        QPainter painter(&img);
        painter.drawPixmap(0,0,window);
        painter.fillRect(0,window.height(),img.width(),footerPx,QColor("#ffffff"));
        painter.setPen(QColor("#111111"));
        QFont font("DejaVu Sans",12,QFont::Bold);
        font.setPixelSize(qMax(18,qRound(19*dpr)));
        painter.setFont(font);
        const int inset=qRound(18*dpr);
        const int line=qRound(30*dpr);
        const int top=window.height()+qRound(14*dpr);
        painter.drawText(inset,top+line,QStringLiteral("Jeremiah O'Neal | Story Scout screenshot | version ")+VERSION);
        painter.drawText(inset,top+line*2,QStringLiteral("infoartrecorder | Pending review; not automatically approved"));
        painter.drawText(inset,top+line*3,QStringLiteral("ntfy-Jeremiah_B_ONeal_GPLv3@ntfy.sh"));
    }
    // Use normal user ownership; never replace existing captures.
    if(QFile::exists(path)) { status->setText("Screenshot filename collision; not overwritten"); return; }
    if(img.save(path,"PNG")) {
        status->setText("Screenshot saved: "+path+" (pending review)");
        registerScreenshot(path);
    } else {
        qWarning()<<"Screenshot write failed"<<path;
        status->setText("Screenshot failed; see terminal.");
    }
}
void MainWindow::closeEvent(QCloseEvent *event){
    rememberCurrent();
    if(!dirty){event->accept();return;}
    const auto answer=QMessageBox::question(this,"Unsaved records","Save changes before closing?",QMessageBox::Save|QMessageBox::Discard|QMessageBox::Cancel,QMessageBox::Save);
    if(answer==QMessageBox::Cancel){event->ignore();return;}
    if(answer==QMessageBox::Save&&!saveAll()){event->ignore();return;}
    event->accept();
}
