#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "function_classified.txt" // Bundled snapshot, no runtime dependency on nov8-2026 file
#include <QApplication>
#include <QClipboard>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QLabel>
#include <QPlainTextEdit>
#include <QProcess>
#include <QPushButton>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QTabWidget>
#include <QTextStream>
#include <QTimer>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QWidget>
#include <QCryptographicHash>
#include <QSet>
#include <algorithm>

static constexpr const char *kProvenance = "TG654147,TG564843,TG333041,TG323932,TG610982,TG148675";
static constexpr const char *kProject = "KinStory1108";
static const QString kDataDir = QStringLiteral("/home/we6jbo/nov8-2026/");

static QString cleaned(QString s) {
    s.replace("&ndash;", "-").replace("&amp;", "&");
    s.replace(QRegularExpression("\\s+"), " ");
    return s.trimmed();
}
static QString normalized(QString s) {
    s = cleaned(s).toCaseFolded();
    s.remove(QRegularExpression("[^\\p{L}\\p{N} ]"));
    s.replace(QRegularExpression("\\s+"), " ");
    return s.trimmed();
}
static QStringList words(const QString &s) {
    return s.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
}
static int wordCountBefore(const QString &s, int pos) {
    return words(s.left(std::max(0,pos))).size();
}
static int nearestDistanceWords(const QString &s, int namePos, const QString &needle) {
    int best = 1000000;
    if (needle.isEmpty()) return best;
    int cursor=0;
    while (true) {
        int at=s.indexOf(needle,cursor,Qt::CaseInsensitive);
        if(at<0)break;
        int a=wordCountBefore(s,namePos),b=wordCountBefore(s,at);
        best=std::min(best,std::abs(a-b));
        cursor=at+std::max<qsizetype>(1,needle.size());
    }
    return best;
}
static bool closeEnough(const QString &s, int namePos, const QString &needle) {
    // Whichever gives the larger allowance: <=45 character distance OR <=30 word distance.
    if (needle.isEmpty()) return true;
    int cursor=0;
    while (true) {
        int at=s.indexOf(needle,cursor,Qt::CaseInsensitive);
        if(at<0)break;
        if (std::abs(at-namePos)<=45 || nearestDistanceWords(s,namePos,needle)<=30) return true;
        cursor=at+std::max<qsizetype>(1,needle.size());
    }
    return false;
}
static QStringList aliasesFor(QString s) {
    QStringList result{cleaned(s)};
    // Dates/suffixes in research-derived display names are editorial labels, not name spelling.
    s.replace("&ndash;", "-");
    s.remove(QRegularExpression("\\s*\\(m\\.\\s*\\d{4}\\)"));
    s.remove(QRegularExpression("\\s+\\d{4}\\s*-\\s*\\d{4}.*$"));
    s.remove(QRegularExpression("\\s+\\d{4}.*$"));
    s = s.trimmed();
    if (!result.contains(s))result << s;
    return result;
}
static int locateName(const QString &doc, const QString &name) {
    for(const QString &variant: aliasesFor(name)) {
        if (variant.size()<4) continue;
        int p=doc.indexOf(variant,0,Qt::CaseInsensitive);
        if(p>=0)return p;
    }
    return -1;
}

MainWindow::MainWindow(QWidget *parent):QMainWindow(parent) {
    setWindowTitle("KinStory1108 v0.1.1 | TG654147");
    resize(1000,700);
    auto *central = new QWidget(this);
    auto *mainLayout = new QVBoxLayout(central);
    auto *header = new QLabel("KinStory1108 | November 7–8, 2026 only | Attribution: Jeremiah O'Neal | TG654147",central);
    header->setWordWrap(true);
    mainLayout->addWidget(header);
    tabs_ = new QTabWidget(central);
    auto *autoPage = new QWidget(tabs_);
    auto *autoLayout = new QVBoxLayout(autoPage);
    autoStatus_ = new QLabel(autoPage);
    autoStatus_->setWordWrap(true);
    autoButton_ = new QPushButton("Toggle login autostart",autoPage);
    autoLayout->addWidget(autoStatus_); autoLayout->addWidget(autoButton_); autoLayout->addStretch();
    tabs_->addTab(autoPage,"Autostart");
    auto *checkPage = new QWidget(tabs_);
    auto *checkLayout = new QVBoxLayout(checkPage);
    auto *help = new QLabel("Read-only checks of 1min_speech.md, blog.md and github.md; copy the suggestions into ChatGPT.",checkPage);
    help->setWordWrap(true);
    prompt_ = new QPlainTextEdit(checkPage); prompt_->setReadOnly(true);
    auto *buttons = new QHBoxLayout();
    refreshButton_ = new QPushButton("Refresh checks",checkPage);
    copyButton_ = new QPushButton("Copy to clipboard",checkPage);
    buttons->addWidget(refreshButton_); buttons->addWidget(copyButton_);
    checkLayout->addWidget(help);checkLayout->addWidget(prompt_,1);checkLayout->addLayout(buttons);
    tabs_->addTab(checkPage,"ChatGPT checks");
    mainLayout->addWidget(tabs_,1);
    auto *bottom=new QHBoxLayout();
    status_=new QLabel(central);
    status_->setWordWrap(true);
    shotButton_=new QPushButton("Screenshot",central);
    bottom->addWidget(status_,1);bottom->addWidget(shotButton_);
    mainLayout->addLayout(bottom);
    setCentralWidget(central);
    connect(autoButton_,&QPushButton::clicked,this,&MainWindow::toggleAutostart);
    connect(refreshButton_,&QPushButton::clicked,this,&MainWindow::refreshChecks);
    connect(copyButton_,&QPushButton::clicked,this,[this] { QApplication::clipboard()->setText(prompt_->toPlainText());status_->setText("Prompt copied to clipboard."); });
    connect(shotButton_,&QPushButton::clicked,this,&MainWindow::captureScreenshot);
    timer_=new QTimer(this);
    connect(timer_,&QTimer::timeout,this,[this]{updateAutostartUi();refreshChecks();});
    timer_->start(60000);
    updateAutostartUi();refreshChecks();
}
QString MainWindow::autostartFile() const {
    QString conf=qEnvironmentVariable("XDG_CONFIG_HOME",QDir::homePath()+"/.config");
    return conf+"/autostart/kinstory1108.desktop";
}
bool MainWindow::activeToday() const { return (QDate::currentDate()==QDate(2026,11,7) || QDate::currentDate()==QDate(2026,11,8)); }
bool MainWindow::autostartEnabled() const {
    QFile f(autostartFile());if(!f.open(QIODevice::ReadOnly))return false;
    return QString::fromUtf8(f.readAll()).contains("X-KinStory1108-Managed=true");
}
void MainWindow::updateAutostartUi() {
    bool on=autostartEnabled();
    autoStatus_->setText(QString("Login autostart is %1. It launches on 2026-11-07 and 2026-11-08 (local dates). The launcher removes managed autostart on later dates.").arg(on?"ON":"OFF"));
    autoButton_->setText(on?"Disable autostart":"Enable autostart");
    autoButton_->setEnabled(activeToday() || on);
}
void MainWindow::toggleAutostart() {
    QString path=autostartFile();
    if(autostartEnabled()) { QFile::remove(path);status_->setText("Managed autostart removed."); }
    else if(activeToday()) {
        QDir().mkpath(QFileInfo(path).absolutePath());
        QFile f(path);
        if(f.open(QIODevice::WriteOnly|QIODevice::Truncate)) {
            QString launcher=QDir::homePath()+"/Projects/KinStory1108/run-kinstory1108.sh";
            QTextStream out(&f);
            out<<"[Desktop Entry]\nType=Application\nName=KinStory1108\nExec="<<launcher<<"\nTerminal=false\nX-GNOME-Autostart-enabled=true\nX-KinStory1108-Managed=true\n";
            f.close();status_->setText("Login autostart enabled.");
        } else status_->setText("Cannot write autostart desktop entry: "+f.errorString());
    }
    updateAutostartUi();
}
void MainWindow::refreshChecks() {
    if(!activeToday()) {
        prompt_->setPlainText("Hey chatgpt,\nKinStory1108 only performs editorial checks on November 7 or 8, 2026 (local date). No files have been modified.\nProvenance: TG654147");
        refreshButton_->setEnabled(false);copyButton_->setEnabled(false);
        status_->setText("Inactive outside 2026-11-07 and 2026-11-08. No files changed.");
        return;
    }
    refreshButton_->setEnabled(true);copyButton_->setEnabled(true);
    struct Target {QString category;QString file;};
    const Target targets[]={{"1min_speech","1min_speech.md"},{"in_blog","blog.md"},{"in_github_link","github.md"}};
    QStringList issues;
    int ok=0, total=0;
    for (const Target &t:targets) {
        QFile f(kDataDir+t.file);
        if(!f.open(QIODevice::ReadOnly|QIODevice::Text)) {
            issues << QString("In %1, please create the file or make it readable. It is required for the %2 category.").arg(f.fileName(),t.category);
            continue;
        }
        QString doc=QString::fromUtf8(f.readAll());
        for(const auto &e:kConnections) {
            if(QString::fromUtf8(e.publicationCategory)!=t.category)continue;
            ++total;
            QString name=cleaned(QString::fromUtf8(e.name));
            QString year=QString::fromUtf8(e.birthYear).trimmed();
            QString connector=cleaned(QString::fromUtf8(e.connector));
            int at=locateName(doc,name);
            if(at<0) {
                issues<<QString("In %1, please include the existing research entry '%2' (category %3), or flag it for editorial exclusion if ambiguous, private, duplicate or not a real person's name. Do not invent facts.").arg(f.fileName(),name,t.category);
                continue;
            }
            bool passed=true;
            if(!year.isEmpty() && year!="####" && !closeEnough(doc,at,year)) {
                issues<<QString("In %1, place birth year %2 within 45 characters OR 30 words of '%3' (whichever allows a greater distance), or label it explicitly unconfirmed.").arg(f.fileName(),year,name);
                passed=false;
            }
            if(!connector.isEmpty() && connector!="Memorial" && connector!="Add to Map" && connector!="Mary" && !closeEnough(doc,at,connector)) {
                issues<<QString("In %1, place the connecting person '%2' within 45 characters OR 30 words of '%3', or explicitly state that their relationship remains unverified.").arg(f.fileName(),connector,name);
                passed=false;
            }
            if(passed)++ok;
        }
    }
    QStringList out;
    out<<"Hey chatgpt,"<<"Please review the following genealogy editorial issues using only supported evidence. Do not invent birth years, relationships, story details or citations. The classified source is an unconfirmed research-lead list, not verified genealogy. Preserve protected birth years (####) and do not publish private or possibly living-person information. Do not add new names to function_classified.txt.";
    out<<"Use these existing categories: 1min_speech (one combined approximately one-minute Cowles Mountain speech, ending with j03.page), in_blog (interesting five-paragraph blog material), and in_github_link (research notes, subject to review). Preserve exclusive speech pair choices.";
    out<<QString("Check summary: %1 entries met their proximity checks among %2 entries in readable matching documents; %3 requests/warnings. Nearness is not genealogical verification.").arg(ok).arg(total).arg(issues.size());
    if(issues.isEmpty())out<<"No proximity problems found in readable documents. Please still review narrative interest, evidence, duplicates, uncertain relationships, and privacy before publication.";
    else out<<issues;
    out<<"Provenance identifier: TG654147. Source entries remain unconfirmed.";
    prompt_->setPlainText(out.join("\n\n"));
    status_->setText(QString("Checked %1 category entries. %2 warnings. Read-only.").arg(total).arg(issues.size()));
}
void MainWindow::captureScreenshot() {
    const QString dir=QDir::homePath()+"/.38bTqEvQc/kinstory1108";
    if(!QDir().mkpath(dir)) { status_->setText("Screenshot directory unavailable.");return; }
    const QString time=QDateTime::currentDateTime().toString("yyyyMMdd-HHmmss-zzz");
    const QString path=dir+"/KinStory1108_"+time+"_v0.1.1.png";
    if(!grab().save(path,"PNG")) {status_->setText("Could not save screenshot.");return;}
    // Never automatically approve or publish. infoartrecorder keeps review state.
    const QString tool=QStandardPaths::findExecutable("infoartrecorder");
    if(tool.isEmpty()) {status_->setText("PNG saved. infoartrecorder missing; registration pending (see local log).");}
    else {
        auto *p=new QProcess(this);
        connect(p,QOverload<int,QProcess::ExitStatus>::of(&QProcess::finished),this,[this,p,path](int code,QProcess::ExitStatus st){
            QFile log(QDir::homePath()+"/.38bTqEvQc/kinstory1108/registration.log");
            if(log.open(QIODevice::WriteOnly|QIODevice::Append|QIODevice::Text)) {
                QTextStream s(&log);s<<QDateTime::currentDateTime().toString(Qt::ISODate)<<" "<<path<<" exit="<<code<<" status="<<int(st)<<"\n";
            }
            status_->setText(code==0 && st==QProcess::NormalExit?"Screenshot saved; registration command succeeded (review pending).":"Screenshot saved; registration failed. See registration.log.");
            p->deleteLater();
        });
        p->start(tool,{"add",path,"--project","KinStory1108"});
        if(!p->waitForStarted(1000)) {status_->setText("Screenshot saved; could not start infoartrecorder.");p->deleteLater();}
        else status_->setText("Screenshot saved. Registering with infoartrecorder...");
    }
}
