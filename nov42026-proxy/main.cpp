// Findagrave Qt Proxy: teaching-oriented annotated revision of version 2.2.5.
// Copyright remains with the original source author(s).
// Proposed licensing choices, pending author confirmation:
// SPDX-License-Identifier: Apache-2.0 OR MIT OR GPL-3.0-only
// NOTE: "GPT3" was interpreted as GPLv3. Verify before external publication.
//
// WHAT TO READ FIRST
//   recommend()                 -> blue auto-push / timeout recheck / opt-in gate
//   parseNewestHtml()           -> optional grouped human-review JSON
//   writeHumanInteractionReview() -> family groups, uncertain relationships
//   pauseForGit()               -> entering/leaving Git tab
//   initGit(), navigateGit()    -> private local source history
//   commitAndPushGit()          -> private GitHub push only after verification
//
#include <QApplication>
#include <QMainWindow>
#include <QTcpServer>
#include <QTcpSocket>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrl>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QSaveFile>
#include <QDateTime>
#include <QCryptographicHash>
#include <QRandomGenerator>
#include <QStandardPaths>
#include <QListWidget>
#include <QTextEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QComboBox>
#include <QCheckBox>
#include <QSpinBox>
#include <QLabel>
#include <QSplitter>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFileDialog>
#include <QMessageBox>
#include <QCloseEvent>
#include <QPointer>
#include <QTimer>
#include <QElapsedTimer>
#include "InfoArtRecorder.h"
#include <QRegularExpression>
#include <QHostAddress>
#include <QUrlQuery>
#include <QSet>
#include <QTabWidget>
#include <QDesktopServices>
#include <QLineEdit>
#include <QProcess>
#include <QTextBrowser>
#include <QFileDevice>
#include <QThread>
#include <QClipboard>
#include <QTextStream>
#include <QTabBar>
#include <QDateTime>
#include <cstdio>
#include <syslog.h>
#include <unistd.h>
#include <functional>
#include <utility>

static const QString root = QDir::homePath()+"/.treeresearchoct2326";
static const QString conf = root+"/proxy";
static const QString pages = root+"/proxy-pages";
static const QString journal = root+"/proxy123.txt";
static const QString settingsPath = conf+"/settings.json";
static const QString rulesPath = conf+"/rules.json";
static const QString appVersion="2.2.5";
static const QString solverPath=root+"/useful/proxysolver.txt";
static const QString vibePath=root+"/useful/vibefinds.txt";
// SECURITY: Never store proxy credentials in a Git-tracked source file.
// Export FINDAGRAVE_PROXY_PASSWORD in the environment before launching the program.
static QByteArray proxyPassword(){ return qgetenv("FINDAGRAVE_PROXY_PASSWORD"); }

static QJsonObject readObj(const QString &p) { QFile f(p); if(!f.open(QIODevice::ReadOnly))return {};auto d=QJsonDocument::fromJson(f.readAll());return d.isObject()?d.object():QJsonObject(); }
static bool saveObj(const QString &p,const QJsonObject &o) {QSaveFile f(p);if(!f.open(QIODevice::WriteOnly))return false; f.write(QJsonDocument(o).toJson(QJsonDocument::Indented));return f.commit();}
static QString stamp() {return QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs);}
static void logEvent(const QString &action,const QJsonObject &fields={}) {QJsonObject o=fields;o["time"]=stamp();o["action"]=action;QFile f(journal);if(f.open(QIODevice::Append|QIODevice::Text)){f.write("EVENT ");f.write(QJsonDocument(o).toJson(QJsonDocument::Compact));f.write("\n");f.flush();} QByteArray msg=("#MFFA2 "+QString::fromUtf8(QJsonDocument(o).toJson(QJsonDocument::Compact))).toUtf8();syslog(LOG_INFO,"%s",msg.constData());}
static QByteArray md5(const QByteArray &s){return QCryptographicHash::hash(s,QCryptographicHash::Md5).toHex();}
static QString safeFilename(const QUrl &u,const QByteArray &contentType){QString h=u.host().toLower();QString hash=QString::fromLatin1(QCryptographicHash::hash(u.toString(QUrl::FullyEncoded).toUtf8(),QCryptographicHash::Sha256).toHex().left(16));return h+"-"+hash+(contentType.contains("html")?".html":contentType.contains("text")?".txt":".bin");}
static bool allowed(const QString &host){QString h=host.toLower();return h=="findagrave.com"||h=="www.findagrave.com";}
static QByteArray statusReason(int c){switch(c){case 200:return "OK";case 204:return "No Content";case 301:return "Moved Permanently";case 302:return "Found";case 400:return "Bad Request";case 403:return "Forbidden";case 404:return "Not Found";case 407:return "Proxy Authentication Required";case 408:return "Request Timeout";case 429:return "Too Many Requests";case 502:return "Bad Gateway";case 503:return "Service Unavailable";default:return "Response";}}
class ProxyWindow;
class ProxyClient : public QObject {
 Q_OBJECT
public:
 ProxyClient(QTcpSocket *incoming,ProxyWindow *win);
 ~ProxyClient() override = default;
 void start();
 void approve(const QByteArray &body,bool accepted);
 QString reviewUrl() const{return currentUrl;}
private:
 QPointer<QTcpSocket> client,upstream;
 QPointer<QNetworkReply> reply;
 ProxyWindow *window;
 QByteArray pending, responseBody, responseHeaders, responseMime;
 QString currentUrl, method;
 int responseCode=200;
 bool inTunnel=false, replied=false, waiting=false;
 QTimer *timeout=nullptr;
 void handleRequest();
 void writeResponse(int code,const QByteArray &body,const QByteArray &type="text/plain",const QByteArray &extra={});
 void finishResponse(const QByteArray &body);
 bool authorized(const QByteArray &header,const QByteArray &requestLine);
};
class ProxyWindow : public QMainWindow {
 Q_OBJECT
public:
 QTcpServer server;
 QNetworkAccessManager network;
 QJsonObject settings,rules;
 QListWidget *events=nullptr,*files=nullptr;
 QPlainTextEdit *editor=nullptr;
 QComboBox *mode=nullptr;
 QLabel *status=nullptr;
 QPlainTextEdit *solverEditor=nullptr;
 QLineEdit *versionEdit=nullptr;
 QLabel *updateStatus=nullptr;
 QString armedHash;
 QPointer<QProcess> installProcess;
 QTimer updateWatcher;
 bool automaticFeaturesPaused=false;
 QPointer<QTimer> hoTimer;
 QPointer<QTimer> visTimer;
 QPointer<QWidget> gitTab;
 QPointer<QLabel> gitStatus,gitFreshness;
 QPointer<QPlainTextEdit> gitOutput;
 QPointer<QPushButton> gitRecommended;
 int lastVisibilityState=-1;
 int gitHistoryOffset=0;
 bool gitNavigating=false;
 QString gitRepository()const{return QDir::homePath()+"/.j03-software";}
 QString gitSourceDirectory()const{return root+"/sourcecode/proxy";}
 QString gitProjectRelative()const{return "nov42026-proxy";}
 bool syncGitSource();
 bool verifyGitTarget();
 void initGit();
 void navigateGit(int delta);
 void commitAndPushGit();
 void refreshGitStatus();
 bool runGit(const QStringList &args,QString *result=nullptr);
 void logGit(const QString &line);
 void pauseForGit(bool paused);
 void writeHumanInteractionReview(const QJsonObject &importRecord);
 void recommendGit(QPushButton *button);
 bool updating=false;
 QLabel *hckStatus=nullptr;
 QPlainTextEdit *hckLog=nullptr;
 QTimer *hckTimer=nullptr;
 QElapsedTimer hckElapsed;
 int hckCount=0;
 void refreshSolver(){
   QDir().mkpath(root+"/useful");
   QFile v(vibePath);QString findings;
   if(v.open(QIODevice::ReadOnly|QIODevice::Text))findings=QString::fromUtf8(v.read(131072));
   else findings="[vibefinds.txt is not present yet. No diagnostic report has been supplied.]";
   QString target=settings.value("update_version").toString("2.3");
   QString prefix="Hey chatgpt, we're currently running "+appVersion+" and heres the instructions for makinng the next version. Also vibe reports that "+findings
     +"\n\nCurrent installed proxy: "+appVersion+". Target update version: "+target
     +". Scanner diagnostics, if available: "+root+"/useful/proxysolver.txt"
     +". Note: seed_window_complete is normal queue completion, not a new connectivity failure.\n";
   QFile f(solverPath);
   QString existing;
   if(f.open(QIODevice::ReadOnly|QIODevice::Text)) existing=QString::fromUtf8(f.read(262144));
   QString tail; const QString marker="\n\n--- USER NOTES ---\n"; int markerAt=existing.indexOf(marker); if(markerAt>=0)tail=existing.mid(markerAt+marker.size()); else if(!existing.isEmpty())tail=existing; QString body=prefix+marker+tail;
   if(existing!=body){QSaveFile out(solverPath);if(out.open(QIODevice::WriteOnly)){out.write(body.toUtf8());out.commit();}}
   solverEditor->setPlainText(body);
   logEvent("solver_prompt_read",{{"path",solverPath},{"version",appVersion},{"vibefinds_present",v.exists()}});
 }
 void saveSolver(){QSaveFile f(solverPath);if(!f.open(QIODevice::WriteOnly)){updateStatus->setText("Cannot write prompt");return;}f.write(solverEditor->toPlainText().toUtf8());if(!f.commit()){updateStatus->setText("Prompt save failed");return;}logEvent("solver_prompt_saved",{{"path",solverPath}});updateStatus->setText("Prompt saved");}
 QString updateScript()const{return QDir::homePath()+"/Downloads/findagrave"+settings.value("update_version").toString(appVersion)+".sh";}
 QString updateZip()const{return QDir::homePath()+"/Downloads/findagrave"+settings.value("update_version").toString(appVersion)+".zip";}
 QString stopFile()const{return "/tmp/rhqus/"+settings.value("update_version").toString(appVersion)+"/stop.txt";}
 bool validVersion(const QString &v)const{static const QRegularExpression re("^[0-9]+(\\.[0-9]+){1,3}$");return re.match(v).hasMatch();}
 bool updateFilesSafe(QString &reason){
   for(const QString &path:{updateScript(),updateZip()}){
     QFileInfo f(path);
     if(!f.exists()||!f.isFile()||f.isSymLink()){reason="Missing/unsafe file: "+path;return false;}
     if(!f.isReadable() || f.ownerId()!=uint(getuid())){reason="File not owned/readable by current user: "+path;return false;}
   }
   QFile script(updateScript());if(!script.open(QIODevice::ReadOnly)){reason="Cannot read update script";return false;}
   QString digest=QString::fromLatin1(QCryptographicHash::hash(script.readAll(),QCryptographicHash::Sha256).toHex());
   if(settings.value("approved_update_script_sha256").toString()!=digest){reason="Update script needs approval (SHA256 changed)";return false;}
   return true;
 }
 void approveScript(){
   QString v=versionEdit->text().trimmed();if(!validVersion(v)){QMessageBox::warning(this,"Version","Use digits and dots, e.g. 2.3");return;}
   settings["update_version"]=v;settings["approved_update_script_sha256"]="";saveObj(settingsPath,settings);
   QString reason;
   QFileInfo fi(updateScript());if(!fi.exists()||fi.isSymLink()||fi.ownerId()!=uint(getuid())){updateStatus->setText("Script missing or unsafe");return;}
   QFile f(updateScript());if(!f.open(QIODevice::ReadOnly))return;
   QString hash=QString::fromLatin1(QCryptographicHash::hash(f.readAll(),QCryptographicHash::Sha256).toHex());
   if(QMessageBox::question(this,"Approve downloaded installer?","Only approve scripts you have inspected and trust.\n\nScript: "+updateScript()+"\nSHA256: "+hash+"\n\nThis script will execute with your user privileges when the stop-file appears.")!=QMessageBox::Yes)return;
   settings["approved_update_script_sha256"]=hash;saveObj(settingsPath,settings);updateStatus->setText("Approved SHA256 for "+v);logEvent("update_script_approved",{{"version",v},{"sha256",hash}});
 }
 void checkUpdate(){
   if(automaticFeaturesPaused || updating || !settings.value("auto_update_enabled").toBool())return;
   QString ver=settings.value("update_version").toString(appVersion);
   if(!validVersion(ver))return;
   QString path=stopFile();QFileInfo stop(path);if(!stop.exists())return;
   if(!stop.isFile()||stop.isSymLink()||stop.ownerId()!=uint(getuid())){logEvent("update_stop_unsafe",{{"path",path}});return;}
   QString reason;if(!updateFilesSafe(reason)){updateStatus->setText(reason);logEvent("update_not_started",{{"version",ver},{"reason",reason}});return;}
   QString claimed=path+".processing."+QString::number(getpid());
   if(!QFile::rename(path,claimed))return;
   updating=true;logEvent("update_trigger_claimed",{{"version",ver},{"stop_file",path},{"installer",updateScript()}});
   if(!QFile::remove(claimed)){logEvent("update_stop_remove_failed",{{"path",claimed}});}
   auto *proc=new QProcess(this);installProcess=proc;
   proc->setProgram("/usr/bin/bash");proc->setArguments({updateScript()});proc->setWorkingDirectory(QDir::homePath()+"/Downloads");
   proc->setProcessChannelMode(QProcess::MergedChannels);
   connect(proc,&QProcess::readyReadStandardOutput,this,[this,proc]{QString output=QString::fromUtf8(proc->readAllStandardOutput()); // Don't log arbitrary script output: may contain credentials.
     updateStatus->setText(output.right(250));logEvent("update_installer_output",{{"bytes",output.toUtf8().size()}});
   });
   connect(proc,&QProcess::finished,this,[this,proc,ver](int code,QProcess::ExitStatus exitStatus){
     logEvent("update_install_finished",{{"version",ver},{"exit_code",code},{"normal",exitStatus==QProcess::NormalExit}});
     proc->deleteLater();installProcess=nullptr;
     if(code!=0||exitStatus!=QProcess::NormalExit){updating=false;updateStatus->setText("Install failed; old proxy remains running");return;}
     QString binary=QDir::homePath()+"/nov42026-proxy/nov42026-proxy";
     QProcess verify;verify.start(binary,{"--version"});
     if(!verify.waitForFinished(2500)||verify.exitCode()!=0){updating=false;updateStatus->setText("New binary verification failed; old proxy remains running");logEvent("update_version_check_failed",{{"binary",binary}});return;}
     QString reported=QString::fromUtf8(verify.readAllStandardOutput()).trimmed();
     if(reported!=ver){updating=false;updateStatus->setText("Version mismatch: "+reported+" expected "+ver);logEvent("update_version_mismatch",{{"reported",reported},{"expected",ver}});return;}
     logEvent("update_handoff_start",{{"version",ver}});
     server.close(); // Release 55193 before launching replacement.
     qint64 newPid=0;
     if(!QProcess::startDetached(binary,{},QDir::homePath(),&newPid)){
       server.listen(QHostAddress::LocalHost,55193);updating=false;logEvent("update_launch_failed");return;
     }
     QTimer::singleShot(2200,this,[this,newPid,ver]{
       QTcpSocket probe;probe.connectToHost(QHostAddress::LocalHost,55193);
       if(probe.waitForConnected(1600)){
         logEvent("update_new_proxy_listening",{{"version",ver},{"pid",double(newPid)}});
         logEvent("update_old_proxy_exiting",{{"version",appVersion}});
         QApplication::quit();
       }else{
         server.listen(QHostAddress::LocalHost,55193);updating=false;
         updateStatus->setText("New proxy did not listen. Old proxy resumed where possible.");
         logEvent("update_health_check_failed",{{"version",ver},{"old_listener_restored",server.isListening()}});
       }
     });
   });
   proc->start();if(!proc->waitForStarted(1200)){updating=false;logEvent("update_process_start_failed");updateStatus->setText("Installer failed to start");proc->deleteLater();installProcess=nullptr;}
 }

 QPointer<ProxyClient> waitingClient;
 QTabWidget *tabs=nullptr;
 QLabel *hoId=nullptr,*hoStatus=nullptr; QPlainTextEdit *hoPrompt=nullptr,*hoInput=nullptr;
 QString currentHandoffFile,currentHandoffId,currentHandoffKind,currentHandoffUrl;
 QLabel *hoNeedUrl=nullptr;
 QPushButton *approveBtn=nullptr,*fetchBtn=nullptr,*parseBtn=nullptr,*hoBtn=nullptr,*lastRec=nullptr;
 int hoTabIndex=-1;
 QListWidget *queueList=nullptr; QLineEdit *qUrl=nullptr,*qName=nullptr; QPushButton *addQBtn=nullptr;
 int queueTabIndex=-1;
 QByteArray responseData;
 QString selectedUrl;
 ProxyWindow(){
  QDir().mkpath(conf);QDir().mkpath(pages);
  if(!QFile::exists(settingsPath))saveObj(settingsPath,{{"listen_host","127.0.0.1"},{"listen_port",55193},{"auth_username",""},{"mode","auto"},{"review_timeout_seconds",90},{"allowed_hosts",QJsonArray{"findagrave.com","www.findagrave.com"}},{"https_inspection",false},{"save_snapshots",true}});
  if(!QFile::exists(rulesPath))saveObj(rulesPath,{});
  settings=readObj(settingsPath);rules=readObj(rulesPath);
  if(!QFile::exists(journal)){QFile f(journal);if(f.open(QIODevice::WriteOnly|QIODevice::Text)){f.write("# Findagrave Qt local proxy: configuration and Mistral Vibe journal\n# Edit settings: ~/.treeresearchoct2326/proxy/settings.json\n# URL overrides: ~/.treeresearchoct2326/proxy/rules.json\n# Saved pages: ~/.treeresearchoct2326/proxy-pages/\n# Every EVENT line is machine-readable JSON for Mistral Vibe. Never publish this log.\n# Basic and Digest credentials are checked; password is embedded in the C++ binary.\n# CONNECT traffic is encrypted: HTTPS HTML cannot be viewed/edited in this edition.\n# 403/429 and robots.txt may be viewed, but access restrictions must not be overridden.\n# Running this proxy does not bypass site permissions.\n");}}
  setWindowTitle("Findagrave Qt Proxy • 127.0.0.1:55193");resize(1150,720);
  tabs=new QTabWidget;setCentralWidget(tabs);
  auto *monitor=new QWidget;tabs->addTab(monitor,"1. Proxy Monitor");auto *outer=new QVBoxLayout(monitor);
  auto *chat=new QWidget;tabs->addTab(chat,"2. ChatGPT");auto *chatLayout=new QVBoxLayout(chat);
  chatLayout->addWidget(new QLabel("Open the project conversation in your default browser:"));
  auto *openChat=new QPushButton("Open Findagrave ChatGPT conversation");chatLayout->addWidget(openChat);
  chatLayout->addWidget(new QLabel("https://chatgpt.com/c/6ae9fb5b-3bd4-83e8-87ce-5fa5f5010085"));chatLayout->addStretch();
  connect(openChat,&QPushButton::clicked,this,[this]{bool ok=QDesktopServices::openUrl(QUrl("https://chatgpt.com/c/6ae9fb5b-3bd4-83e8-87ce-5fa5f5010085"));logEvent(ok?"chatgpt_open_requested":"chatgpt_open_failed");});
  auto *solver=new QWidget;tabs->addTab(solver,"3. Solver / Updater");auto *solverLayout=new QVBoxLayout(solver);
  solverLayout->addWidget(new QLabel("Prompt: "+solverPath));
  solverEditor=new QPlainTextEdit;solverLayout->addWidget(solverEditor);
  auto *promptActions=new QHBoxLayout;solverLayout->addLayout(promptActions);
  auto *refresh=new QPushButton("Read solver + Vibe findings");auto *savePrompt=new QPushButton("Save prompt");
  auto *copy=new QPushButton("Copy prompt");promptActions->addWidget(refresh);promptActions->addWidget(savePrompt);promptActions->addWidget(copy);
  connect(refresh,&QPushButton::clicked,this,&ProxyWindow::refreshSolver);
  connect(savePrompt,&QPushButton::clicked,this,&ProxyWindow::saveSolver);
  connect(copy,&QPushButton::clicked,this,[this]{QApplication::clipboard()->setText(solverEditor->toPlainText());});
  auto *updateRow=new QHBoxLayout;solverLayout->addLayout(updateRow);
  updateRow->addWidget(new QLabel("Target version:") );
  versionEdit=new QLineEdit(settings.value("update_version").toString("2.3"));updateRow->addWidget(versionEdit);
  auto *approveUpdate=new QPushButton("Approve local update script SHA256");updateRow->addWidget(approveUpdate);
  auto *enableUpdate=new QPushButton("Enable / disable watching");updateRow->addWidget(enableUpdate);
  updateStatus=new QLabel;solverLayout->addWidget(updateStatus);
  connect(approveUpdate,&QPushButton::clicked,this,&ProxyWindow::approveScript);
  connect(enableUpdate,&QPushButton::clicked,this,[this]{QString v=versionEdit->text().trimmed();if(!validVersion(v)){QMessageBox::warning(this,"Invalid version","Expected e.g. 2.3");return;}settings["update_version"]=v;settings["auto_update_enabled"]=!settings.value("auto_update_enabled").toBool();saveObj(settingsPath,settings);updateStatus->setText(settings.value("auto_update_enabled").toBool()?"Watching: "+stopFile():"Auto update watch disabled");logEvent("update_watch_toggled",{{"enabled",settings.value("auto_update_enabled")},{"version",v}});});
  refreshSolver();
  updateStatus->setText(settings.value("auto_update_enabled").toBool()?"Watching: "+stopFile():"Auto update disabled until enabled in GUI");
  connect(&updateWatcher,&QTimer::timeout,this,&ProxyWindow::checkUpdate);updateWatcher.start(3000);
  // Tab 4: human handoff — findagrave drops joneal/handoff-*.json; human answers here; we write joneal/done-*.json
  auto *ho=new QWidget;tabs->addTab(ho,"4. Human Handoff");auto *hoL=new QVBoxLayout(ho);
  hoL->addWidget(new QLabel("Pending handoff request from findagrave (joneal/handoff-*.json):"));
  hoId=new QLabel("(none)");hoL->addWidget(hoId);
  hoNeedUrl=new QLabel("URL needed: (none)");hoL->addWidget(hoNeedUrl);
  fetchBtn=new QPushButton("Fetch needed URL now (wget; protocol requires automatic pushing when enabled)");hoL->addWidget(fetchBtn);
  parseBtn=new QPushButton("Parse newest saved HTML in html/ into import JSON");hoL->addWidget(parseBtn);
  connect(fetchBtn,&QPushButton::clicked,this,[this]{fetchNeededUrl();});
  connect(parseBtn,&QPushButton::clicked,this,[this]{parseNewestHtml();});
  hoPrompt=new QPlainTextEdit;hoPrompt->setReadOnly(true);hoPrompt->setMaximumHeight(140);hoL->addWidget(hoPrompt);
  hoL->addWidget(new QLabel("Your response (paste import JSON for manual_import, or {\"url\":..,\"name\":..} for new_lead):"));
  hoInput=new QPlainTextEdit;hoL->addWidget(hoInput);
  hoBtn=new QPushButton("Submit -> joneal/done-<id>.json");hoL->addWidget(hoBtn);
  hoTabIndex=tabs->indexOf(ho);
  hoStatus=new QLabel("");hoL->addWidget(hoStatus);
  connect(hoBtn,&QPushButton::clicked,this,[this]{submitHandoff();});
  hoTimer=new QTimer(this);
  connect(hoTimer,&QTimer::timeout,this,[this]{
    if(automaticFeaturesPaused)return;
    pollHandoffs();refreshQueueList();processNewRecords();
    if(currentHandoffFile.isEmpty()&&waitingClient.isNull())writeWhatsNext();
  });
  hoTimer->start(2000);
  auto *qOpenBtn=new QPushButton("Queue next person -> tab 5");hoL->addWidget(qOpenBtn);
  connect(qOpenBtn,&QPushButton::clicked,this,[this]{recommend(addQBtn,queueTabIndex);});
  // Tab 5: queue — findagrave stays the only writer of useful/state.json; adds/removes go through joneal/done-*.json
  auto *qt=new QWidget;tabs->addTab(qt,"5. Queue");auto *qtL=new QVBoxLayout(qt);
  qtL->addWidget(new QLabel("findagrave queue (useful/state.json) plus unconsumed add/remove requests:"));
  queueList=new QListWidget;qtL->addWidget(queueList);
  auto *qRow=new QHBoxLayout;qtL->addLayout(qRow);
  qUrl=new QLineEdit;qUrl->setPlaceholderText("memorial URL, e.g. https://www.findagrave.com/memorial/123/slug");qRow->addWidget(qUrl);
  qName=new QLineEdit;qName->setPlaceholderText("name");qName->setMaximumWidth(220);qRow->addWidget(qName);
  addQBtn=new QPushButton("Add to queue (writes joneal/done-*.json)");qtL->addWidget(addQBtn);
  auto *rmQBtn=new QPushButton("Remove selected URL from queue");qtL->addWidget(rmQBtn);
  auto *qStatus=new QLabel("");qtL->addWidget(qStatus);
  queueTabIndex=tabs->indexOf(qt);
  connect(addQBtn,&QPushButton::clicked,this,[this]{addToQueue(false);});
  connect(rmQBtn,&QPushButton::clicked,this,[this]{addToQueue(true);});
  // Tab 6: settings
  auto *stw=new QWidget;tabs->addTab(stw,"6. Settings");auto *stL=new QVBoxLayout(stw);
  auto *apCb=new QCheckBox("Auto-push the blue (recommended) button");apCb->setChecked(settings.value("auto_push_blue_enabled").toBool(false));stL->addWidget(apCb);
  auto *delayRow=new QHBoxLayout;stL->addLayout(delayRow);
  delayRow->addWidget(new QLabel("after (seconds):"));
  auto *delaySpin=new QSpinBox;delaySpin->setRange(1,3600);delaySpin->setValue(settings.value("auto_push_blue_delay_seconds").toInt(5));delayRow->addWidget(delaySpin);delayRow->addStretch();
  connect(apCb,&QCheckBox::toggled,this,[this,apCb,delaySpin]{settings["auto_push_blue_enabled"]=apCb->isChecked();settings["auto_push_blue_delay_seconds"]=delaySpin->value();saveObj(settingsPath,settings);logEvent("settings_changed",{{"auto_push_blue_enabled",apCb->isChecked()},{"auto_push_blue_delay_seconds",delaySpin->value()}});writeDiagnostics("settings_changed");});
  connect(delaySpin,QOverload<int>::of(&QSpinBox::valueChanged),this,[this,delaySpin]{settings["auto_push_blue_delay_seconds"]=delaySpin->value();saveObj(settingsPath,settings);});
  auto *wnBtn=new QPushButton("Write whatsnext.json now");stL->addWidget(wnBtn);
  connect(wnBtn,&QPushButton::clicked,this,[this]{writeWhatsNext();addEvent("whatsnext_written","root/whatsnext.json");});
   stL->addStretch();
   // Tab 8 (placed before Handoff Check in the construction order): Git.
   // Entering this tab pauses the automatic fetch, parse, queue polling,
   // proxy update checks, blue-button clicks and handoff watch checks.
   gitTab=new QWidget;tabs->addTab(gitTab,"Git");
   auto *gitLayout=new QVBoxLayout(gitTab);
   gitLayout->addWidget(new QLabel("PRIVATE source repository only. Never commit genealogy records, configuration, or credentials."));
   auto *gitButtons=new QHBoxLayout;gitLayout->addLayout(gitButtons);
   auto *gitInit=new QPushButton("1. Init: commit + push to j03-software");gitButtons->addWidget(gitInit);
   auto *gitPrevious=new QPushButton("<< Previous version");gitButtons->addWidget(gitPrevious);
   auto *gitNext=new QPushButton("Next version >>");gitButtons->addWidget(gitNext);
   auto *gitCommit=new QPushButton("3. Sync changes + push to j03-software");gitButtons->addWidget(gitCommit);
   gitStatus=new QLabel("Git not checked");gitLayout->addWidget(gitStatus);
   gitFreshness=new QLabel("Last commit: unknown");gitLayout->addWidget(gitFreshness);
   gitOutput=new QPlainTextEdit;gitOutput->setReadOnly(true);
   gitOutput->setPlaceholderText("Git commands and results appear here (secrets are not displayed).");gitLayout->addWidget(gitOutput);
   connect(gitInit,&QPushButton::clicked,this,[this]{initGit();});
   connect(gitPrevious,&QPushButton::clicked,this,[this]{navigateGit(+1);});
   connect(gitNext,&QPushButton::clicked,this,[this]{navigateGit(-1);});
   connect(gitCommit,&QPushButton::clicked,this,[this]{commitAndPushGit();});
   connect(tabs,&QTabWidget::currentChanged,this,[this,gitInit,gitCommit](int index){
     const bool inGit=(tabs->widget(index)==gitTab);
     pauseForGit(inGit);
     if(inGit){refreshGitStatus();recommendGit(QDir(gitRepository()+"/.git").exists()?gitInit:gitInit);}
   });
   // Tab 7: First check at 5 min, then 1 min x5, then every 2 min until 1 hour has passed.
  auto *hck=new QWidget;tabs->addTab(hck,"7. Handoff Check");auto *hckL=new QVBoxLayout(hck);
  hckL->addWidget(new QLabel("Scheduled handoff checks (first at 5 min, then 1 min x5, then every 2 min for 1 hour):"));
  hckStatus=new QLabel("Handoff check: not run yet");hckL->addWidget(hckStatus);
  hckLog=new QPlainTextEdit;hckLog->setReadOnly(true);hckLog->setPlaceholderText("Check results appear here.");hckL->addWidget(hckLog);
  auto *hckBtnRow=new QHBoxLayout;hckL->addLayout(hckBtnRow);
  auto *hckNow=new QPushButton("Check now");hckBtnRow->addWidget(hckNow);
  auto *shotBtn=new QPushButton("Screenshot");hckBtnRow->addWidget(shotBtn);hckBtnRow->addStretch();
  connect(hckNow,&QPushButton::clicked,this,[this]{handoffCheck("manual");});
  // One Screenshot button (lower-left of this tab). Captures only this tab's widget, which contains
  // no genealogy records, so the born-1981/1960/1928/1926 exclusion holds by construction.
  // Success shows a small nonmodal toast (no image preview, no modal); failure a closable dialog.
  connect(shotBtn,&QPushButton::clicked,this,[this,hck]{
    try{
      const QString path=InfoArtRecorder::capture(hck,"nov42026-proxy",appVersion,"handoff-check-tab");
      auto *toast=new QLabel("Screenshot saved (pending_review, not published): "+path,this);
      toast->setWordWrap(true);toast->setStyleSheet("QLabel{background:#101820;color:#fff;padding:8px;border-radius:4px;}");
      toast->setMaximumSize(420,120);toast->adjustSize();toast->move(8,qMax(0,this->height()-toast->height()-8));toast->show();toast->raise();
      QTimer::singleShot(4000,toast,&QLabel::deleteLater);
      addEvent("screenshot_saved",path);
    }catch(const std::exception &e){
      QMessageBox::warning(this,"Screenshot failed",QString::fromStdString(e.what()));
    }
  });
  hckTimer=new QTimer(this);hckElapsed.start();
  connect(hckTimer,&QTimer::timeout,this,[this]{
    if(automaticFeaturesPaused)return;
   handoffCheck("scheduled");
    if(hckElapsed.elapsed()>=3600000){hckTimer->stop();hckStatus->setText(hckStatus->text()+" — 1-hour watch finished");addEvent("handoff_watch_finished",QString::number(hckCount)+" checks");}
    else if(hckCount<=5)hckTimer->setInterval(60000);
    else hckTimer->setInterval(120000);
  });
  hckTimer->setInterval(300000);hckTimer->start();
  QTimer::singleShot(0,this,[this]{addEvent("handoff_watch_started","first check 5min, then 1min x5, then 2min until 1h");});
  // Visibility watchdog: if the window is hidden/minimized it is re-shown; while a handoff is
  // pending the window is re-raised every 15 s (GNOME/Wayland ignores one-shot activateWindow).
  visTimer=new QTimer(this);
  connect(visTimer,&QTimer::timeout,this,[this]{
    if(automaticFeaturesPaused)return;
   const int st=(isVisible()?1:0)|(isMinimized()?2:0)|(isActiveWindow()?4:0);
    if(!isVisible()||isMinimized()){showNormal();raise();activateWindow();}
    else if(!currentHandoffFile.isEmpty()){raise();activateWindow();}
    if(st!=lastVisibilityState||!currentHandoffFile.isEmpty()){lastVisibilityState=st;addEvent("window_visibility",QString("visible=%1 minimized=%2 active=%3 handoff_pending=%4").arg(isVisible()).arg(isMinimized()).arg(isActiveWindow()).arg(currentHandoffFile.isEmpty()?0:1));}
  });
  visTimer->start(15000);
  QTimer::singleShot(0,this,[this]{addEvent("window_visibility",QString("startup visible=%1 minimized=%2 active=%3").arg(isVisible()).arg(isMinimized()).arg(isActiveWindow()));showNormal();raise();activateWindow();});
  auto *top=new QHBoxLayout;outer->addLayout(top);
  top->addWidget(new QLabel("Mode:"));mode=new QComboBox;mode->addItems({"Auto-pass","Wait for review"});mode->setCurrentIndex(settings.value("mode").toString()=="wait"?1:0);top->addWidget(mode);
  auto *reload=new QPushButton("Reload saved files");top->addWidget(reload);
  auto *save=new QPushButton("Save edited response as URL override");top->addWidget(save);
  approveBtn=new QPushButton("Approve / send edited response");top->addWidget(approveBtn);
  auto *rejectBtn=new QPushButton("Reject pending response");top->addWidget(rejectBtn);
  auto *split=new QSplitter(Qt::Horizontal);outer->addWidget(split);
  events=new QListWidget;files=new QListWidget;editor=new QPlainTextEdit;
  events->setMinimumWidth(310);files->setMinimumWidth(200);editor->setPlaceholderText("HTTP responses / saved text pages; HTTPS CONNECT is opaque without a trusted TLS inspection layer.");
  split->addWidget(events);split->addWidget(files);split->addWidget(editor);split->setStretchFactor(2,2);
  status=new QLabel("Starting local proxy...");outer->addWidget(status);
  connect(mode,&QComboBox::currentIndexChanged,this,[this]{settings["mode"]=mode->currentIndex()==0?"auto":"wait";saveObj(settingsPath,settings);logEvent("mode_changed",{{"mode",settings.value("mode")}});});
  connect(reload,&QPushButton::clicked,this,&ProxyWindow::reloadFiles);
  connect(files,&QListWidget::itemClicked,this,[this](QListWidgetItem *item){QString name=item->text();QFile f(pages+"/"+name);if(f.open(QIODevice::ReadOnly)){editor->setPlainText(QString::fromUtf8(f.readAll()));selectedUrl.clear();status->setText("Viewing saved file: "+name);}});
  connect(events,&QListWidget::itemClicked,this,[this](QListWidgetItem *item){selectedUrl=item->data(Qt::UserRole).toString();if(waitingClient && selectedUrl==waitingClient->reviewUrl()){editor->setPlainText(QString::fromUtf8(responseData));status->setText("Pending: "+selectedUrl);}});
  connect(save,&QPushButton::clicked,this,[this]{if(selectedUrl.isEmpty()||!selectedUrl.startsWith("http://")){QMessageBox::information(this,"URL override","Select a reviewed plaintext HTTP URL first. HTTPS tunnel bodies are not available.");return;}QUrl u(selectedUrl);if(!allowed(u.host())||u.path()=="/robots.txt"){QMessageBox::warning(this,"Restricted","No robots.txt replacement or non-Findagrave overrides.");return;}QString ruleKey=selectedUrl;auto btn=QMessageBox::question(this,"URL override","Also apply to similar URLs (prefix match)?",QMessageBox::Yes|QMessageBox::No|QMessageBox::Cancel,QMessageBox::No);if(btn==QMessageBox::Cancel)return;if(btn==QMessageBox::Yes)ruleKey=selectedUrl+"*";QString name=safeFilename(u,"text/html");QFile f(pages+"/"+name);if(!f.open(QIODevice::WriteOnly|QIODevice::Truncate)){QMessageBox::warning(this,"Save failed",f.errorString());return;}f.write(editor->toPlainText().toUtf8());f.close();rules[ruleKey]=name;saveObj(rulesPath,rules);logEvent("override_saved",{{"url",ruleKey},{"file",name}});reloadFiles();});
  connect(approveBtn,&QPushButton::clicked,this,[this]{if(waitingClient){auto p=waitingClient;waitingClient=nullptr;p->approve(editor->toPlainText().toUtf8(),true);}});
  connect(rejectBtn,&QPushButton::clicked,this,[this]{if(waitingClient){auto p=waitingClient;waitingClient=nullptr;p->approve({},false);}});
  connect(&server,&QTcpServer::newConnection,this,[this]{while(server.hasPendingConnections()){auto *s=server.nextPendingConnection();auto *c=new ProxyClient(s,this);c->setParent(this);c->start();}});
  bool ok=server.listen(QHostAddress::LocalHost,55193);
  status->setText(ok?"Listening on 127.0.0.1:55193. HTTPS: tunnel only.":"Cannot listen: "+server.errorString());
  logEvent(ok?"proxy_started":"proxy_bind_failed",{{"port",55193},{"error",ok?"":server.errorString()},{"version",appVersion}});
  writeDiagnostics(ok?"proxy_started":"proxy_bind_failed");
  if(!ok){
    // Never kill an existing listener or leave a second, nonfunctional GUI running.
    QTcpSocket probe; probe.connectToHost(QHostAddress::LocalHost,55193);
    const bool occupied=probe.waitForConnected(500);
    probe.abort();
    logEvent(occupied?"duplicate_instance_skipped":"bind_failed_other",{{"port",55193},{"version",appVersion},{"error",server.errorString()}});
    fprintf(stderr,"Findagrave Qt Proxy %s: port 127.0.0.1:55193 %s; not starting duplicate. Existing process unchanged.\n",appVersion.toUtf8().constData(),occupied?"already in use":"unavailable");
    QTimer::singleShot(0,qApp,&QCoreApplication::quit);
    return;
  }
  reloadFiles();
 }
 void reloadFiles(){files->clear();QDir d(pages);files->addItems(d.entryList(QDir::Files,QDir::Name));}
 // Tab 7 handoff watch: reports whether handoffs are showing up in joneal/ (active or seen in the last hour).
 void handoffCheck(const QString &why){
  hckCount++;
  QDir jd(root+"/joneal");
  bool showing=!currentHandoffFile.isEmpty();
  const auto entries=jd.entryList({"handoff-*"},QDir::Files,QDir::Name);
  for(const auto &f:entries){if(f.endsWith(".consumed"))continue;QFileInfo fi(jd.filePath(f));if(fi.lastModified().msecsTo(QDateTime::currentDateTime())<3600000){showing=true;break;}}
  const QString phrase=showing?QString("hand offs are showing up"):QString("hand offs are not showing up");
  hckStatus->setText("Handoff check #"+QString::number(hckCount)+" ("+why+" "+QDateTime::currentDateTime().toString("HH:mm:ss")+"): "+phrase);
  hckLog->appendPlainText(stamp()+" check #"+QString::number(hckCount)+" ["+why+"]: "+phrase);
  addEvent("handoff_check",phrase);
 }
 void addEvent(const QString &action,const QString &url,int code=0){QString s=stamp()+"  "+action+"  "+url+(code?"  ["+QString::number(code)+"]":"");auto *item=new QListWidgetItem(s,events);item->setData(Qt::UserRole,url);events->scrollToBottom();if(events->count()>1500)delete events->takeItem(0);status->setText("Latest: "+action+" | "+url.left(90));logEvent(action,{{"url",url},{"status",code}});writeDiagnostics(action);}
 // Diagnostics snapshot for post-testing review. Never writes the proxy password, Authorization headers, cookies, or tokens.
 void writeDiagnostics(const QString &action){
   QDir().mkpath(root+"/diagnostics");
   QJsonObject d;
   d["schema"]="proxy-diagnostics-v1";d["written_at"]=stamp();d["version"]=appVersion;
   d["listen"]="127.0.0.1:55193";d["last_action"]=action;
   d["mode"]=settings.value("mode").toString("auto");
   d["auto_update_enabled"]=settings.value("auto_update_enabled").toBool(false);
   d["https_inspection"]=settings.value("https_inspection").toBool(false);
   d["save_snapshots"]=settings.value("save_snapshots").toBool(true);
   QJsonArray hosts;for(const auto &h:settings.value("allowed_hosts").toArray())hosts.append(h.toString());d["allowed_hosts"]=hosts;
   d["auth_username_configured"]=!settings.value("auth_username").toString().isEmpty();
   QJsonArray rl;for(const auto &k:rules.keys())rl.append(k);d["override_rules"]=rl;
   d["waiting_for_review"]=!waitingClient.isNull();
   if(waitingClient)d["pending_review_url"]=waitingClient->reviewUrl();
   QJsonArray ev;QFile jf(journal);if(jf.open(QIODevice::ReadOnly|QIODevice::Text)){const QStringList lines=QString::fromUtf8(jf.readAll()).split('\n');const int start=qMax(0,lines.size()-40);for(int i=start;i<lines.size();++i){const QString l=lines[i].trimmed();if(l.startsWith("EVENT ")){QJsonObject o=QJsonDocument::fromJson(l.mid(6).toUtf8()).object();if(!o.isEmpty())ev.append(o);}}}
   d["recent_events"]=ev;
   QJsonArray pf;QDir pd(pages);for(const auto &fi:pd.entryList(QDir::Files,QDir::Name))pf.append(fi);d["saved_pages"]=pf;
   QDir jd2(root+"/joneal");int hp=0;for(const auto &f:jd2.entryList({"handoff-*.json"},QDir::Files,QDir::Name))if(!f.contains(".handled"))hp++;
   d["handoff_pending_count"]=hp;d["handoff_active_file"]=currentHandoffFile;d["handoff_last_id"]=currentHandoffId;
   QFile sf(root+"/useful/state.json");if(sf.open(QIODevice::ReadOnly)){QJsonObject st=QJsonDocument::fromJson(sf.readAll()).object();d["findagrave_queue_length"]=st.value("queue").toArray().size();}
   d["person_record_files"]=QDir(root+"/people").entryList({"*.json"},QDir::Files,QDir::Name).size();
   QSaveFile out(root+"/diagnostics/proxy.json");
   if(out.open(QIODevice::WriteOnly)){out.write(QJsonDocument(d).toJson(QJsonDocument::Indented));out.commit();}
 }
 void pollHandoffs(){
   QDir d(root+"/joneal");if(!d.exists())return;
   QString best;
   for(const auto &f:d.entryList({"handoff-*.json"},QDir::Files,QDir::Name)){if(f.contains(".handled")||f.contains(".consumed"))continue;if(best.isEmpty()||f>best)best=f;}
   if(best.isEmpty()||best==currentHandoffFile)return;
   QFile f(d.filePath(best));if(!f.open(QIODevice::ReadOnly|QIODevice::Text))return;
   QJsonObject o=QJsonDocument::fromJson(f.readAll()).object();
   currentHandoffFile=best;currentHandoffId=o.value("id").toString();currentHandoffKind=o.value("kind").toString();
   hoId->setText(best+"  id="+currentHandoffId+"  kind="+currentHandoffKind);
   hoPrompt->setPlainText(o.value("prompt").toString()+"\nURL: "+o.value("url").toString());
   currentHandoffUrl=o.value("url").toString();hoNeedUrl->setText("URL needed: "+currentHandoffUrl);
   hoInput->clear();hoStatus->setText("Waiting for your input");
   recommend(fetchBtn,hoTabIndex);
   addEvent("handoff_presented",currentHandoffId);
   // Desktop notification so the request is seen even when the WM blocks focus stealing.
   QProcess::startDetached("notify-send",{QString("Findagrave handoff %1 (%2)").arg(currentHandoffId,currentHandoffKind),QString("New handoff request — open tab '4. Human Handoff' in the Findagrave Qt Proxy window."),"-u","critical","-t","20000","-a","nov42026-proxy"});
   addEvent("notify_sent",currentHandoffId);
   writeDiagnostics("handoff_presented");
 }
 // Focus + highlight helpers: raise the window, switch to the suggested tab (blue tab text),
 // and paint the recommended button blue. Clears the previous highlight first.
 void focusWin(){showNormal();raise();activateWindow();}
 void recommend(QPushButton *b,int tabIndex){
   if(lastRec&&lastRec!=b)lastRec->setStyleSheet("");
   if(b){b->setStyleSheet("QPushButton{background-color:#1e6fff;color:white;font-weight:bold;}");lastRec=b;}
   for(int i=0;i<tabs->count();++i)tabs->tabBar()->setTabTextColor(i,palette().color(QPalette::WindowText));
   if(tabIndex>=0&&tabIndex<tabs->count()){tabs->tabBar()->setTabTextColor(tabIndex,QColor("#1e6fff"));tabs->setCurrentIndex(tabIndex);}
   focusWin();
   // (1) AUTO-PUSH: Schedule a click when a blue recommended button appears.
   // Five seconds translates to 5000 milliseconds. This is not a continuous loop.
   if(b && settings.value("auto_push_blue_enabled").toBool(false)){
     const int delay=qBound(1,settings.value("auto_push_blue_delay_seconds").toInt(5),3600);
     QPointer<QPushButton> safeButton(b);
     QTimer::singleShot(delay*1000,this,[this,safeButton](){
       if(automaticFeaturesPaused || !safeButton || lastRec!=safeButton)return;
       // (2) OPTIONAL: The original code did NOT recheck the enabled setting here.
       // Uncomment to cancel a pending click when the checkbox is switched off:
       // if(!settings.value("auto_push_blue_enabled").toBool(false))return;
       // (3) OPTIONAL HUMAN APPROVAL: Uncomment this together with
       // writeHumanInteractionReview(imp) in parseNewestHtml() below.
       // This prevents automatic Submit, but still allows manual clicks.
       // if(safeButton==hoBtn)return;
       addEvent("auto_button_push",safeButton->text());
       safeButton->click();
     });
   }
 }
 // whatsnext.json: written whenever idle (no pending handoff, no review waiting). findagrave displays it; Vibe reads it to pick the next step.
 void writeWhatsNext(){
   QJsonObject d;d["schema"]="whatsnext-v1";d["written_at"]=stamp();
   QJsonArray done;QFile jf(journal);if(jf.open(QIODevice::ReadOnly|QIODevice::Text)){const QStringList lines=QString::fromUtf8(jf.readAll()).split('\n');const int start=qMax(0,lines.size()-12);for(int i=start;i<lines.size();++i){const QString l=lines[i].trimmed();if(l.startsWith("EVENT ")){QJsonObject o=QJsonDocument::fromJson(l.mid(6).toUtf8()).object();if(!o.isEmpty())done.append(o);}}}
   d["done"]=done;
   QDir jd(root+"/joneal");int hp=0;for(const auto &f:jd.entryList({"handoff-*.json"},QDir::Files,QDir::Name))if(!f.contains(".handled"))hp++;
   int ql=0;QFile sf(root+"/useful/state.json");if(sf.open(QIODevice::ReadOnly)){ql=QJsonDocument::fromJson(sf.readAll()).object().value("queue").toArray().size();}
   QJsonArray next;
   if(!waitingClient.isNull())next.append("A response is awaiting review in tab 1 (Approve/Reject).");
   if(hp>0)next.append("Tab 4: "+QString::number(hp)+" handoff request(s) waiting - fetch/parse/submit in '4. Human Handoff'.");
   if(ql>0)next.append("Queue has "+QString::number(ql)+" URL(s) - run ./findagrave to attempt the next workable one, or add more in tab '5. Queue'.");
   if(next.isEmpty())next.append("Nothing pending right now: add leads in tab 5, capture blocked seeds in tab 4, or research via plan.md sources.");
   d["next"]=next;d["handoff_pending"]=hp;d["queue_length"]=ql;d["waiting_for_review"]=!waitingClient.isNull();
   QSaveFile out(root+"/whatsnext.json");if(out.open(QIODevice::WriteOnly)){out.write(QJsonDocument(d).toJson(QJsonDocument::Indented));out.commit();}
 }
 void submitHandoff(){
   if(automaticFeaturesPaused)return;
   if(currentHandoffFile.isEmpty()){hoStatus->setText("No pending handoff");return;}
   QDir().mkpath(root+"/joneal");
   QJsonObject o;o["id"]=currentHandoffId;o["kind"]=currentHandoffKind;o["at"]=stamp();
   QJsonParseError pe;QJsonDocument pd=QJsonDocument::fromJson(hoInput->toPlainText().toUtf8(),&pe);
   if(pe.error==QJsonParseError::NoError&&pd.isObject())o["payload"]=pd.object();else o["payload"]=hoInput->toPlainText();
   QSaveFile out(root+"/joneal/"+QString("done-%1.json").arg(currentHandoffId));
   if(out.open(QIODevice::WriteOnly)){out.write(QJsonDocument(o).toJson(QJsonDocument::Indented));out.commit();}
   QFile::rename(root+"/joneal/"+currentHandoffFile,root+"/joneal/"+currentHandoffFile+".handled");
   addEvent("handoff_submitted",currentHandoffId);
   hoStatus->setText("Wrote done-"+currentHandoffId+".json — findagrave consumes it on its next action");
   recommend(nullptr,0);
   events->setStyleSheet("QListWidget{border:3px solid #1e6fff;}");
   QTimer::singleShot(9000,this,[this]{if(events)events->setStyleSheet("");});
   currentHandoffFile.clear();currentHandoffId.clear();currentHandoffKind.clear();currentHandoffUrl.clear();
   hoId->setText("(none)");hoPrompt->clear();hoInput->clear();hoNeedUrl->setText("URL needed: (none)");
 }
 // Protocol requires automatic pushing when enabled. The wget request can also be manual.
 // Site restrictions, rate limits and response codes remain binding. Log each attempt.
 void logHtmlFetch(const QString &file,const QString &url,bool ok){
   QDir().mkpath(root+"/html");
   QJsonArray arr;QFile f(root+"/html/index.json");
   if(f.open(QIODevice::ReadOnly)){QJsonDocument d=QJsonDocument::fromJson(f.readAll());if(d.isArray())arr=d.array();}
   QJsonObject o;o["file"]=file;o["url"]=url;o["at"]=stamp();o["method"]="wget_button_click";o["ok"]=ok;o["triggered_by"]="handoff:"+currentHandoffId;
   arr.append(o);
   QSaveFile out(root+"/html/index.json");if(out.open(QIODevice::WriteOnly)){out.write(QJsonDocument(arr).toJson(QJsonDocument::Indented));out.commit();}
 }
 void fetchNeededUrl(){
   if(automaticFeaturesPaused)return;
   if(currentHandoffUrl.isEmpty()){hoStatus->setText("No URL needed right now");return;}
   QDir().mkpath(root+"/html");
   static const QString UA="Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/120.0.0.0 Safari/537.36";
   QRegularExpression re("/memorial/(\\d+)");auto m=re.match(currentHandoffUrl);
   QString id=m.hasMatch()?m.captured(1):QString::fromLatin1(QCryptographicHash::hash(currentHandoffUrl.toUtf8(),QCryptographicHash::Sha256).toHex().left(12));
   QString fname=id+".html",path=root+"/html/"+fname;
   hoStatus->setText("Fetching "+currentHandoffUrl+" -> html/"+fname);
   addEvent("wget_started",currentHandoffUrl);
   QStringList args={"--user-agent="+UA,"--timeout=20","--tries=1","-q","-O",path,currentHandoffUrl};
   bool started=QProcess::startDetached("wget",args);
   if(!started){addEvent("wget_failed",currentHandoffUrl);logHtmlFetch(fname,currentHandoffUrl,false);hoStatus->setText("wget failed to start");return;}
   QTimer::singleShot(25000,this,[this,fname,path](){finishFetchCheck(fname,path);});
 }
 void finishFetchCheck(const QString &fname,const QString &path){
   if(automaticFeaturesPaused){
     QTimer::singleShot(1000,this,[this,fname,path]{finishFetchCheck(fname,path);});
     return;
   }
   QFileInfo fi(path);const bool ok=fi.exists()&&fi.size()>1000;
   logHtmlFetch(fname,currentHandoffUrl,ok);
   addEvent(ok?"wget_done":"wget_failed",currentHandoffUrl);
   hoStatus->setText(ok?"Saved html/"+fname+"; next: Parse newest saved HTML":"Fetch failed; capture in browser instead");
   if(ok)recommend(parseBtn,hoTabIndex);
 }
 void parseNewestHtml(){
   if(automaticFeaturesPaused)return;
   QDir d(root+"/html");auto files=d.entryList({"*.html"},QDir::Files,QDir::Time);
   if(files.isEmpty()){hoStatus->setText("No saved HTML in html/ yet");return;}
   QString fname=files.first();QFile f(d.filePath(fname));
   if(!f.open(QIODevice::ReadOnly|QIODevice::Text)){hoStatus->setText("Cannot read "+fname);return;}
   QString h=QString::fromUtf8(f.readAll());
   auto cap=[&](const QString &pat,int g=1){QRegularExpression re(pat,QRegularExpression::DotMatchesEverythingOption|QRegularExpression::CaseInsensitiveOption);auto m=re.match(h);return m.hasMatch()?m.captured(g).trimmed():QString();};
   auto clean=[&](QString s){s.remove(QRegularExpression("<[^>]*>"));return s.simplified();};
   QString name=clean(cap("<h1[^>]*>(.*?)</h1>"));
   if(name.isEmpty()){auto m=QRegularExpression("og:title\" content=\"([^\"]*)\"").match(h);if(m.hasMatch())name=clean(m.captured(1)).section(" (" ,0,0);}
   QJsonObject imp;
   imp["url"]=currentHandoffUrl;
   imp["name"]=name;
   imp["birth_date"]=clean(cap("id=\"birthDateLabel\"[^>]*>(.*?)</"));
   imp["death_date"]=clean(cap("id=\"deathDateLabel\"[^>]*>(.*?)</"));
   imp["birth_place"]=clean(cap("itemprop=\"birthPlace\"[^>]*>(.*?)</"));
   imp["death_place"]=clean(cap("itemprop=\"deathPlace\"[^>]*>(.*?)</"));
   QJsonArray rel;
   QRegularExpression lm("<a[^>]+href=\"([^\"]*/memorial/(\\d+)[^\"]*)\"[^>]*>(.*?)</a>",QRegularExpression::DotMatchesEverythingOption);
   auto it=lm.globalMatch(h);int selfId=-1;{QRegularExpression re("/memorial/(\\d+)");auto m=re.match(currentHandoffUrl);if(m.hasMatch())selfId=m.captured(1).toInt();}
   QSet<QString> seenIds;
   while(it.hasNext()&&rel.size()<5){auto m=it.next();QString mid=m.captured(2);QString t=clean(m.captured(3));
     if(mid.toInt()==selfId||seenIds.contains(mid)||t.size()<4||t.size()>110)continue;seenIds.insert(mid);
     QJsonObject p;p["name"]=t;p["url"]="https://www.findagrave.com/memorial/"+mid;p["relationship_to_record"]="unknown";rel.append(p);}
   imp["related_people"]=rel;
   hoInput->setPlainText(QJsonDocument(imp).toJson(QJsonDocument::Indented));
   hoStatus->setText("Parsed html/"+fname+" into import JSON ("+name+") — review, edit if needed, then Submit");
   addEvent("html_parsed",fname);
   // (3) OPTIONAL GROUPED HUMAN REVIEW. Uncomment this AND the hoBtn
   // exclusion in recommend() to require a person to approve submission.
   // writeHumanInteractionReview(imp);
   recommend(hoBtn,hoTabIndex);
 }
 void refreshQueueList(){
   if(!queueList)return;
   queueList->clear();
   QFile sf(root+"/useful/state.json");
   if(sf.open(QIODevice::ReadOnly)){QJsonObject st=QJsonDocument::fromJson(sf.readAll()).object();
     for(const auto &v:st.value("queue").toArray()){QJsonObject e=v.toObject();
       queueList->addItem(e.value("url").toString()+"   <- "+e.value("from_name").toString()+" ["+e.value("relation_hint").toString()+"]");}}
   QDir jd(root+"/joneal");
   for(const auto &f:jd.entryList({"done-*.json"},QDir::Files,QDir::Name)){
     if(f.contains(".consumed"))continue;
     QFile df(jd.filePath(f));if(!df.open(QIODevice::ReadOnly))continue;
     QJsonObject o=QJsonDocument::fromJson(df.readAll()).object();QString k=o.value("kind").toString();
     if(k=="new_lead"||k=="remove_lead"){QJsonObject p=o.value("payload").toObject();
       queueList->addItem("(pending "+k+") "+p.value("url").toString()+"   "+p.value("name").toString());}
   }
 }
 void addToQueue(bool remove){
   QString url=qUrl->text().trimmed();
   QRegularExpression re("^https?://(www\\.)?findagrave\\.com/memorial/\\d+",QRegularExpression::CaseInsensitiveOption);
   if(!re.match(url).hasMatch()){if(queueList->currentItem()&&remove)url=queueList->currentItem()->text().section("   ",0,0);else{if(!remove)return;}}
   QString id=QDateTime::currentDateTimeUtc().toString("yyyyMMdd-hhmmss-zzz")+"-q";
   QJsonObject o;o["id"]=id;o["kind"]=remove?"remove_lead":"new_lead";o["at"]=stamp();
   QJsonObject p;p["url"]=url;p["name"]=qName->text().trimmed();o["payload"]=p;
   QDir().mkpath(root+"/joneal");
   QSaveFile out(root+"/joneal/"+QString("done-%1.json").arg(id));
   if(out.open(QIODevice::WriteOnly)){out.write(QJsonDocument(o).toJson(QJsonDocument::Indented));out.commit();}
   addEvent(remove?"queue_remove_requested":"queue_add_requested",url);
   qUrl->clear();qName->clear();refreshQueueList();
 }
 void saveJsonFile(const QString &path,const QJsonObject &o){QSaveFile f(path);if(f.open(QIODevice::WriteOnly)){f.write(QJsonDocument(o).toJson(QJsonDocument::Indented));f.commit();}}
 // TA ids are TEMPORARY local markers only - never TG/AKA codes.
 QString taFor(QJsonObject &reg,const QString &mid){
   if(mid.isEmpty())return QString();
   QJsonObject ppl=reg.value("people").toObject();
   for(auto it=ppl.begin();it!=ppl.end();++it){QJsonObject e=it.value().toObject();
     if(QString::number(qint64(e.value("memorial_id").toDouble()))==mid)return it.key();}
   QString t;
   do{t="TA"+QString::number(QRandomGenerator::global()->generate64()%900000+100000);}while(ppl.contains(t));
   QJsonObject e;e["name"]="";e["memorial_id"]=mid.toLongLong();e["status"]="temporary_marker_not_TG";e["source"]="auto-generated from findagrave-scan.json record";
   ppl.insert(t,e);reg["people"]=ppl;return t;
 }
 // Every new findagrave-scan.json record -> people/<memorial-id>.json + TA registry entries; family enriched from html/<id>.html when saved.
 void processNewRecords(){
   QDir().mkpath(root+"/people");
   QFile sf(root+"/findagrave-scan.json");
   if(!sf.open(QIODevice::ReadOnly|QIODevice::Text))return;
   QJsonArray recs=QJsonDocument::fromJson(sf.readAll()).object().value("records").toArray();
   if(recs.isEmpty())return;
   QJsonObject proc;QFile pf(root+"/people/.processed.json");
   if(pf.open(QIODevice::ReadOnly))proc=QJsonDocument::fromJson(pf.readAll()).object();
   QJsonArray doneArr=proc.value("processed").toArray();QSet<QString> done;for(const auto &v:doneArr)done.insert(v.toString());
   QJsonObject reg;QFile rf(root+"/people/ta_registry.json");
   if(rf.open(QIODevice::ReadOnly))reg=QJsonDocument::fromJson(rf.readAll()).object();
   if(!reg.contains("people")){reg["schema"]="ta-registry-v1";reg["note"]="TA ids are TEMPORARY local markers, never TG/AKA codes; permanent TG only via the project registry process.";reg["people"]=QJsonObject();}
   QRegularExpression midRe("/memorial/(\\d+)");
   bool changed=false;
   for(const auto &rv:recs){
     QJsonObject r=rv.toObject();QString url=r.value("url").toString();
     if(done.contains(url))continue;
     auto m=midRe.match(url);if(!m.hasMatch())continue;QString mid=m.captured(1);
     QString ta=taFor(reg,mid);
     {QJsonObject ppl=reg.value("people").toObject();QJsonObject e=ppl.value(ta).toObject();
      if(e.value("name").toString().isEmpty()){e["name"]=r.value("name");e["source"]="findagrave.com/memorial/"+mid;ppl.insert(ta,e);reg["people"]=ppl;}}
     QString gender="unknown";QJsonArray parents,spouses,children,siblings;
     QFile hf(root+"/html/"+mid+".html");
     if(hf.open(QIODevice::ReadOnly|QIODevice::Text)){
       QString h=QString::fromUtf8(hf.readAll());
       QRegularExpression gm("itemprop=\"gender\"[^>]*>\\s*(?:<[^>]+>\\s*)*([A-Za-z]+)",QRegularExpression::CaseInsensitiveOption);
       auto g=gm.match(h);if(g.hasMatch()&&(g.captured(1)=="Male"||g.captured(1)=="Female"))gender=g.captured(1);
       QRegularExpression lm("<a[^>]+href=\"([^\"]*/memorial/(\\d+)[^\"]*)\"[^>]*>(.*?)</a>",QRegularExpression::DotMatchesEverythingOption);
       auto it=lm.globalMatch(h);QSet<QString> seen;
       while(it.hasNext()){auto mm=it.next();QString rmid=mm.captured(2);QString t=mm.captured(3);t.remove(QRegularExpression("<[^>]*>"));t=t.simplified();
         if(rmid==mid||seen.contains(rmid)||t.size()<4||t.size()>110)continue;seen.insert(rmid);
         QString ctx=h.mid(qMax(0,mm.capturedStart()-2000),2000).toLower();QString role="unknown";
         const QList<QPair<QString,QString>> kws={{"spouse","spouse"},{"children","child"},{"parents","parent"},{"siblings","sibling"}};
         for(const auto &kv:kws)if(ctx.contains(kv.first)){role=kv.second;break;}
         QJsonObject p;p["ta"]=taFor(reg,rmid);p["name"]=t;p["memorial_id"]=rmid.toLongLong();p["relationship"]=role;
         if(role=="parent")parents.append(p);else if(role=="spouse")spouses.append(p);else if(role=="child")children.append(p);else if(role=="sibling")siblings.append(p);}
     }
     QJsonArray rel;
     for(const auto &pv:r.value("related_people").toArray()){QJsonObject p0=pv.toObject();QString purl=p0.value("url").toString();
       auto m2=midRe.match(purl);QString pmid=m2.hasMatch()?m2.captured(1):QString();
       QJsonObject p;p["ta"]=taFor(reg,pmid);p["name"]=p0.value("name");if(!pmid.isEmpty())p["memorial_id"]=pmid.toLongLong();p["relationship"]=p0.value("relationship_to_record").toString("unknown");rel.append(p);}
     QString allText=r.value("name").toString()+" "+r.value("birth_date").toString()+" "+r.value("death_date").toString();
     for(const auto &pv:r.value("related_people").toArray())allText+=" "+pv.toObject().value("name").toString();
     for(const auto &a:{parents,spouses,children,siblings})for(const auto &pv:a)allText+=" "+pv.toObject().value("name").toString();
     bool prot=false;for(const auto &y:{"1981","1960","1926","1928"})if(allText.contains(y))prot=true;
     QJsonObject rec2;
     rec2["schema"]="person-record-v1";rec2["ta"]=ta;rec2["name"]=r.value("name");rec2["gender"]=gender;
     rec2["birth"]=QJsonObject{{"date",r.value("birth_date")},{"place",r.value("birth_place")}};
     rec2["death"]=QJsonObject{{"date",r.value("death_date")},{"place",r.value("death_place")}};
     rec2["memorial"]=url;rec2["source_method"]=r.value("source_method");rec2["registry_match"]=r.value("registry_match");rec2["registry_match_status"]=r.value("registry_match_status");
     rec2["verification"]="unverified; Find a Grave user-submitted memorial (see html/index.json)";
     if(!parents.isEmpty())rec2["parents"]=parents;
     if(!spouses.isEmpty())rec2["spouses"]=spouses;
     if(!children.isEmpty())rec2["children"]=children;
     if(!siblings.isEmpty())rec2["siblings"]=siblings;
     if(!rel.isEmpty())rec2["related_people"]=rel;
     if(prot){rec2["protected_year_flag"]=true;rec2["protected_year_note"]="contains a protected birth year (1981/1960/1926/1928 per user ruling) - LOCAL ONLY, never outward";}
     saveJsonFile(root+"/people/"+mid+".json",rec2);
     done.insert(url);doneArr.append(url);changed=true;
     addEvent("person_record_written",r.value("name").toString()+" -> people/"+mid+".json ("+ta+")");
   }
   if(changed){proc["processed"]=doneArr;saveJsonFile(root+"/people/.processed.json",proc);saveJsonFile(root+"/people/ta_registry.json",reg);writeDiagnostics("person_records_updated");}
 }
 bool waitMode()const{return mode->currentIndex()==1;}
 void review(ProxyClient *c,const QString &url,const QByteArray &body){if(waitingClient){addEvent("review_busy_rejected",url);c->approve({},false);return;}waitingClient=c;selectedUrl=url;responseData=body;editor->setPlainText(QString::fromUtf8(body));addEvent("awaiting_review",url);status->setText("Waiting for approval: "+url);recommend(approveBtn,0);}
};
// ============================================================================
// GIT TAB: upload ONLY approved proxy source files to an EXISTING GitHub repo.
//
// Source of truth: ~/.treeresearchoct2326/sourcecode/proxy/
// Existing checkout: ~/.j03-software/
// GitHub target: https://github.com/we6jbo/j03-software.git, branch main
// Destination inside the checkout: nov42026-proxy/
//
// IMPORTANT: "Init" does NOT create a second repo. It synchronizes the allowlisted
// source files, commits the project folder, and PUSHES to the existing repo.
// No genealogy records, live proxy configuration, JSON/logs, or credentials staged.
// This is a synchronous operation; the UI may be briefly unresponsive during git.
// ============================================================================
void ProxyWindow::logGit(const QString &line){
  if(gitOutput)gitOutput->appendPlainText(QDateTime::currentDateTime().toString("HH:mm:ss ")+line);
}
void ProxyWindow::pauseForGit(bool paused){
  if(automaticFeaturesPaused==paused)return;
  automaticFeaturesPaused=paused;
  QDir().mkpath(root);
  QFile f(root+"/paused_to.log");
  if(f.open(QIODevice::Append|QIODevice::Text)){
    f.write((stamp()+QString(paused?" PAUSED Git tab opened\n":" RESUMED Git tab closed\n")).toUtf8());
    f.flush();
  }
  // Detached wget processes already launched can complete while this tab is open.
  if(!paused && lastRec && settings.value("auto_push_blue_enabled").toBool(false))
    recommend(lastRec,tabs->currentIndex());
}
void ProxyWindow::recommendGit(QPushButton *button){
  if(gitRecommended)gitRecommended->setStyleSheet("");
  gitRecommended=button;
  if(button)button->setStyleSheet("QPushButton{background:#ffe26b;color:#202020;font-weight:bold;}");
}
bool ProxyWindow::runGit(const QStringList &args,QString *result){
  const QString dir=gitRepository();
  QString command="git";
  for(const auto &a:args)command+=" "+a;
  logGit("$ "+command);
  QProcess p;p.setWorkingDirectory(dir);p.setProgram("git");p.setArguments(args);
  p.start();
  if(!p.waitForStarted(4000)){logGit("Failed to start git");return false;}
  if(!p.waitForFinished(30000)){p.kill();p.waitForFinished();logGit("Timed out");return false;}
  QString out=QString::fromUtf8(p.readAllStandardOutput()+p.readAllStandardError()).trimmed();
  if(result)*result=out;
  if(!out.isEmpty())logGit(out.left(6000));
  if(p.exitCode()!=0)logGit("exit code="+QString::number(p.exitCode()));
  return p.exitStatus()==QProcess::NormalExit && p.exitCode()==0;
}
bool ProxyWindow::verifyGitTarget(){
  const QString checkout=gitRepository();
  if(!QDir(checkout+"/.git").exists()){
    logGit("Missing existing checkout: "+checkout+". Clone j03-software there first; no new repo will be initialized.");
    return false;
  }
  QString remote,branch,dirty;
  if(!runGit({"remote","get-url","origin"},&remote))return false;
  remote=remote.trimmed();
  if(remote!="https://github.com/we6jbo/j03-software.git" &&
     remote!="git@github.com:we6jbo/j03-software.git"){
    logGit("BLOCKED: unexpected origin: "+remote);return false;
  }
  if(!runGit({"branch","--show-current"},&branch)||branch.trimmed()!="main"){
    logGit("BLOCKED: expected branch main; checkout/switch branches manually first.");return false;
  }
  if(!runGit({"status","--porcelain"},&dirty))return false;
  if(!dirty.trimmed().isEmpty()){
    logGit("BLOCKED: existing checkout has uncommitted changes. Resolve these manually so other projects are not swept into the proxy commit.");
    return false;
  }
  // Ensure push cannot accidentally carry unpushed commits from other projects.
  QString upstream;
  if(!runGit({"rev-parse","--verify","refs/remotes/origin/main"},&upstream)){
    logGit("BLOCKED: origin/main missing. Fetch/inspect the existing repo manually first.");return false;
  }
  QString head;
  if(!runGit({"rev-parse","HEAD"},&head)||head.trimmed()!=upstream.trimmed()){
    logGit("BLOCKED: local HEAD differs from origin/main. Pull/fetch/reconcile manually before pressing Init.");return false;
  }
  // Verify fresh remote state before pushing, so stale local tracking info
  // cannot cause an accidental non-fast-forward or unrelated history push.
  if(!runGit({"fetch","--quiet","origin","main"}))return false;
  QString fetched;
  if(!runGit({"rev-parse","refs/remotes/origin/main"},&fetched)||fetched.trimmed()!=head.trimmed()){
    logGit("BLOCKED: remote main has changed. Reconcile manually.");return false;
  }
  return true;
}
bool ProxyWindow::syncGitSource(){
  const QString src=gitSourceDirectory();
  const QString target=gitRepository()+"/"+gitProjectRelative();
  const QStringList files={"main.cpp","InfoArtRecorder.h","InfoArtRecorder.cpp", "CMakeLists.txt", "README.md"};
  if(!QFileInfo(src+"/main.cpp").isFile()){
    logGit("Source missing: "+src+"/main.cpp");return false;
  }
  if(!QDir().mkpath(target)){logGit("Cannot create project folder: "+target);return false;}
  QFileInfo targetInfo(target);
  if(targetInfo.isSymLink()){logGit("BLOCKED: destination folder is symlink");return false;}
  for(const QString &name:files){
    QFileInfo in(src+"/"+name),out(target+"/"+name);
    if(!in.exists())continue;
    if(!in.isFile()||in.isSymLink()||out.isSymLink()){
      logGit("BLOCKED: unsafe source/destination: "+name);return false;
    }
    QFile source(in.absoluteFilePath());
    if(!source.open(QIODevice::ReadOnly)){logGit("Cannot read "+name);return false;}
    QByteArray bytes=source.readAll();
    // No silent overwrites of files not previously tracked in the repository.
    if(out.exists()){
      QFile old(out.absoluteFilePath());
      if(!old.open(QIODevice::ReadOnly)){logGit("Cannot inspect existing "+name);return false;}
      if(old.readAll()==bytes)continue;
      QString tracked;
      if(!runGit({"ls-files","--error-unmatch","--",gitProjectRelative()+"/"+name},&tracked)){
        logGit("BLOCKED: destination has a different untracked file: "+name);return false;
      }
    }
    QSaveFile dest(out.absoluteFilePath());
    if(!dest.open(QIODevice::WriteOnly)||dest.write(bytes)!=bytes.size()||!dest.commit()){
      logGit("Failed to copy "+name);return false;
    }
    logGit("Copied "+name+" -> "+gitProjectRelative()+"/");
  }
  return true;
}
void ProxyWindow::refreshGitStatus(){
  if(!QDir(gitRepository()+"/.git").exists()){
    if(gitStatus)gitStatus->setText("Existing checkout not found: "+gitRepository());
    if(gitFreshness)gitFreshness->setText("No Git status available");return;
  }
  QString state,unixTime;
  runGit({"status","--short","--",gitProjectRelative()},&state);
  if(gitStatus)gitStatus->setText(state.isEmpty()?"Project files clean (j03-software)":"Project changes:\n"+state.left(600));
  if(runGit({"log","-1","--format=%ct","--",gitProjectRelative()},&unixTime)){
    bool ok=false;auto sec=unixTime.trimmed().toLongLong(&ok);
    if(ok){auto age=QDateTime::currentDateTimeUtc().toSecsSinceEpoch()-sec;
      gitFreshness->setText(QString("Last proxy commit %1 (%2 minutes ago). %3")
        .arg(QDateTime::fromSecsSinceEpoch(sec).toLocalTime().toString(Qt::ISODate))
        .arg(qMax<qint64>(0,age/60))
        .arg(age>=0&&age<=7200?"COMMITTED within 2 hours":"NOT committed within 2 hours"));
    }else if(gitFreshness)gitFreshness->setText("No proxy-specific commit found");
  }
}
void ProxyWindow::initGit(){
  logGit("Init selected: sync local source into existing j03-software, COMMIT and PUSH.");
  if(!verifyGitTarget())return;
  // Existing repository visibility must be explicitly acknowledged. This can
  // publish source code if the existing j03-software repository is public.
  if(QMessageBox::question(this,"Publish to existing GitHub repo",
    "Copy allowlisted source files from:\n"+gitSourceDirectory()+
    "\n\nInto existing checkout:\n"+gitRepository()+"/"+gitProjectRelative()+
    "\n\nThen commit and PUSH to we6jbo/j03-software (main)?\n"
    "WARNING: This repository may be PUBLIC. No private repository is created.",
    QMessageBox::Yes|QMessageBox::No,QMessageBox::No)!=QMessageBox::Yes)return;
  if(!syncGitSource())return;
  if(!runGit({"add","--",gitProjectRelative()}))return;
  QString staged;
  if(!runGit({"diff","--cached","--name-only"},&staged))return;
  if(staged.trimmed().isEmpty()){
    logGit("No changed files to commit; checking remote push status.");
  }else{
    for(const auto &name:staged.split('\n',Qt::SkipEmptyParts))
      if(!name.startsWith(gitProjectRelative()+"/")){
        logGit("BLOCKED: staged file outside project: "+name);return;
      }
    if(!runGit({"commit","-m","Update nov42026 Findagrave proxy source"}))return;
  }
  if(!runGit({"push","origin","main"}))return;
  logGit("SUCCESS: source committed and pushed to https://github.com/we6jbo/j03-software ("+gitProjectRelative()+"/)");
  gitHistoryOffset=0;gitNavigating=false;
  refreshGitStatus();
}
void ProxyWindow::navigateGit(int delta){
  // Browse only commits affecting this project, not the history of other projects.
  if(!QDir(gitRepository()+"/.git").exists()){logGit("Missing existing checkout");return;}
  QString versions;
  if(!runGit({"log","--format=%H","--max-count=100","--",gitProjectRelative()},&versions))return;
  const auto commits=versions.split('\n',Qt::SkipEmptyParts);
  int target=gitHistoryOffset+delta;
  if(target<0||target>=commits.size()){logGit("No more proxy versions in this direction");return;}
  QString dirty;
  if(!runGit({"status","--porcelain"},&dirty)||!dirty.trimmed().isEmpty()){
    logGit("BLOCKED: commit or restore all checkout changes before version navigation");return;
  }
  if(QMessageBox::question(this,"Preview source history",
     "Restore only proxy source files from commit "+commits[target].left(12)+
     " into the j03-software working copy? This does not push or change HEAD.",
     QMessageBox::Yes|QMessageBox::No,QMessageBox::No)!=QMessageBox::Yes)return;
  if(runGit({"restore","--source="+commits[target],"--worktree","--",gitProjectRelative()})){
    gitHistoryOffset=target;gitNavigating=true;refreshGitStatus();
    logGit("Restored proxy working-copy files only; inspect before committing.");
  }
}
void ProxyWindow::commitAndPushGit(){
  // Same safe operation as Init. Git is already initialized in ~/.j03-software.
  initGit();
}
// ============================================================================
// OPTIONAL HUMAN INTERACTION JSON: write one grouped review bundle per handoff.
// Enable it by UNCOMMENTING writeHumanInteractionReview(imp) in parseNewestHtml().
// Missing facts stay null. Do NOT invent parent, spouse or child relationships.
// The existing parser only extracts generic related_people (relationship unknown).
// A more capable parser would need verified relationship labels to populate
// family groups. This function retains all available source facts and prompts.
// ============================================================================
void ProxyWindow::writeHumanInteractionReview(const QJsonObject &record){
  auto field=[](const QJsonObject &o,const QString &k)->QJsonValue{
    const QString v=o.value(k).toString().trimmed();
    return v.isEmpty()?QJsonValue(QJsonValue::Null):QJsonValue(v);
  };
  QJsonObject group;
  group["name"]=field(record,"name");
  group["birthdate"]=field(record,"birth_date");
  group["birth_location"]=field(record,"birth_place");
  group["death_date"]=field(record,"death_date");
  group["death_location"]=field(record,"death_place");
  group["father"]=QJsonValue(QJsonValue::Null);
  group["mother"]=QJsonValue(QJsonValue::Null);
  group["source_url"]=record.value("url");
  QJsonArray spouses,unassignedChildren,unverified;
  // Merge available family relationships from the local person record (if any).
  // These are tentative relationships until they have been manually checked.
  const auto match=QRegularExpression("/memorial/(\\d+)").match(record.value("url").toString());
  if(match.hasMatch()){
    const auto person=readObj(root+"/people/"+match.captured(1)+".json");
    const QJsonArray parents=person.value("parents").toArray();
    for(const auto &p:parents){
      const auto parent=p.toObject();const QString gender=parent.value("gender").toString().toLower();
      if(gender=="male" && group.value("father").isNull())group["father"]=parent;
      else if(gender=="female" && group.value("mother").isNull())group["mother"]=parent;
      else unverified.append(parent); // Unknown gender is not assumed.
    }
    for(const auto &v:person.value("spouses").toArray()){
      const QJsonObject other=v.toObject();
      QJsonObject spouse;
      spouse["name"]=field(other,"name");
      spouse["birthdate"]=field(other,"birth_date");
      spouse["birth_location"]=field(other,"birth_place");
      spouse["death_date"]=field(other,"death_date");
      spouse["death_location"]=field(other,"death_place");
      spouse["source_url"]=other.value("url");
      spouse["children"]=QJsonArray();
      spouse["verification_required"]=true;
      spouses.append(spouse);
    }
    for(const auto &v:person.value("children").toArray())unassignedChildren.append(v);
  }
  // The supplied parser does not reliably associate each child with a spouse.
  // Store unassigned children separately rather than inventing associations.
  // A person can later move a child into spouse.children[] after verification.
  for(const auto &v:record.value("related_people").toArray()){
    const QJsonObject related=v.toObject();
    const QString role=related.value("relationship_to_record").toString().toLower();
    if(role=="child")unassignedChildren.append(related);
    else if(role=="spouse"){
      QJsonObject spouse=related;spouse["children"]=QJsonArray();
      spouse["verification_required"]=true;spouses.append(spouse);
    }else unverified.append(related);
  }
  group["spouses"]=spouses;
  group["children_not_yet_assigned_to_spouse"]=unassignedChildren;
  group["other_unverified_relatives"]=unverified;
  group["review_status"]="pending_human_review";
  const QString destination=root+"/human_interaction.json";
  QJsonObject bundle=readObj(destination);
  bundle["schema"]="family-review-v1";
  bundle["last_updated_utc"]=stamp();
  bundle["human_approval_required"]=true;
  QJsonArray groups=bundle.value("family_groups").toArray();
  bool replaced=false;
  for(int i=0;i<groups.size();++i){
    if(groups[i].toObject().value("source_url")==record.value("url")){
      groups[i]=group;replaced=true;break;
    }
  }
  if(!replaced)groups.append(group);
  bundle["family_groups"]=groups;
  bundle["review_questions"]=QJsonArray{
    "Verify dates and locations with source evidence.",
    "Identify mother and father only when supported.",
    "Assign each child to the correct spouse only after verification.",
    "Review the imported data before manually submitting."
  };
  if(saveObj(destination,bundle))addEvent("human_interaction_review_written",destination);
}

ProxyClient::ProxyClient(QTcpSocket *incoming,ProxyWindow *win):QObject(win),client(incoming),window(win){incoming->setParent(this);}
void ProxyClient::start(){connect(client,&QTcpSocket::readyRead,this,[this]{if(inTunnel)return;pending+=client->readAll();if(pending.size()>65536){writeResponse(400,"Headers too large");return;}if(pending.contains("\r\n\r\n"))handleRequest();});connect(client,&QTcpSocket::disconnected,this,&QObject::deleteLater);}
bool ProxyClient::authorized(const QByteArray &h,const QByteArray &requestLine){QByteArray x=h.trimmed();QByteArray user=window->settings.value("auth_username").toString().toUtf8();QByteArray credentials=user+":"+proxyPassword();
 if(x.startsWith("Basic "))return QByteArray::fromBase64(x.mid(6))==credentials;
 if(!x.startsWith("Digest "))return false;
 QMap<QByteArray,QByteArray> f;QRegularExpression re(R"FG((\w+)\s*=\s*(?:"([^"]*)"|([^,\s]+)))FG");QString body=QString::fromUtf8(x.mid(7));auto it=re.globalMatch(body);while(it.hasNext()){auto m=it.next();f[m.captured(1).toUtf8().toLower()]=(!m.captured(2).isNull()?m.captured(2):m.captured(3)).toUtf8();}
 QByteArray nonce=f.value("nonce"),uri=f.value("uri"),qop=f.value("qop");if(nonce.isEmpty()||uri.isEmpty()||f.value("username")!=user||f.value("realm")!="Findagrave Local Proxy"||f.value("algorithm","MD5").compare("MD5",Qt::CaseInsensitive)!=0)return false;
 QByteArray m=requestLine.split(' ').value(0);QByteArray ha1=md5(user+":Findagrave Local Proxy:"+proxyPassword()),ha2=md5(m+":"+uri),calc;
 if(qop=="auth")calc=md5(ha1+":"+nonce+":"+f.value("nc")+":"+f.value("cnonce")+":auth:"+ha2);else if(qop.isEmpty())calc=md5(ha1+":"+nonce+":"+ha2);else return false;
 return calc==f.value("response").toLower();
}
void ProxyClient::writeResponse(int code,const QByteArray &body,const QByteArray &type,const QByteArray &extra){if(!client)return;QByteArray out="HTTP/1.1 "+QByteArray::number(code)+" "+statusReason(code)+"\r\nContent-Type: "+type+"\r\nContent-Length: "+QByteArray::number(body.size())+"\r\nConnection: close\r\n"+extra+"\r\n"+body;client->write(out);client->flush();client->disconnectFromHost();}
void ProxyClient::handleRequest(){auto end=pending.indexOf("\r\n\r\n");if(end<0)return;QByteArray headerBlock=pending.left(end);auto lines=headerBlock.split('\n');if(lines.isEmpty()){writeResponse(400,"Invalid request");return;}QByteArray requestLine=lines.takeFirst().trimmed(),auth;auto fields=requestLine.split(' ');if(fields.size()<3){writeResponse(400,"Invalid request line");return;}method=QString::fromLatin1(fields[0]);QByteArray target=fields[1];for(auto line:lines){line=line.trimmed();if(line.toLower().startsWith("proxy-authorization:"))auth=line.mid(20).trimmed();}
 if(!authorized(auth,requestLine)){writeResponse(407,"Proxy credentials required","text/plain","Proxy-Authenticate: Basic realm=\"Findagrave Local Proxy\"\r\nProxy-Authenticate: Digest realm=\"Findagrave Local Proxy\", nonce=\""+QByteArray::number(QDateTime::currentSecsSinceEpoch())+"\", qop=\"auth\", algorithm=MD5\r\n");window->addEvent("auth_rejected","local-client",407);return;}
 window->addEvent("proxy_authenticated", method=="CONNECT" ? "CONNECT "+QString::fromUtf8(target) : "local HTTP request");
 if(method=="CONNECT"){
  auto hostPort=target.split(':');QString host=QString::fromLatin1(hostPort.value(0));bool is443=hostPort.size()==2&&hostPort.value(1)=="443";
  if(!allowed(host)||!is443){window->addEvent("destination_rejected", "CONNECT "+QString::fromUtf8(target),403);writeResponse(403,"Only Findagrave HTTPS hosts on port 443 are allowed");return;}
  currentUrl="https://"+host+"/ (CONNECT)";window->addEvent("https_tunnel",currentUrl);
  QTcpSocket *peer=new QTcpSocket(this);upstream=peer;
  connect(peer,&QTcpSocket::connected,this,[this,peer]{if(!client)return;window->addEvent("https_tunnel_connected",currentUrl,200);client->write("HTTP/1.1 200 Connection Established\r\n\r\n");client->flush();inTunnel=true;QByteArray leftover=pending.mid(pending.indexOf("\r\n\r\n")+4);pending.clear();if(!leftover.isEmpty())peer->write(leftover);connect(client,&QTcpSocket::readyRead,peer,[this,peer]{if(client)peer->write(client->readAll());});connect(peer,&QTcpSocket::readyRead,this,[this,peer]{if(client)client->write(peer->readAll());});});
  connect(peer,&QTcpSocket::errorOccurred,this,[this](QAbstractSocket::SocketError){if(!inTunnel)writeResponse(502,"Upstream connection failed");else if(client)client->disconnectFromHost();});
  connect(peer,&QTcpSocket::disconnected,this,[this]{if(client)client->disconnectFromHost();});connect(client,&QTcpSocket::disconnected,peer,&QTcpSocket::disconnectFromHost);peer->connectToHost(host,443);return;
 }
 QUrl url(QString::fromUtf8(target));if(method!="GET"&&method!="HEAD"){window->addEvent("method_rejected",method,403);writeResponse(403,"Only GET and HEAD are supported");return;}
 if(!url.isValid()||url.scheme()!="http"||!allowed(url.host())){window->addEvent("destination_rejected", "HTTP destination not allowlisted",403);writeResponse(403,"Only Findagrave HTTP destinations are allowed");return;}
 currentUrl=url.toString(QUrl::FullyEncoded);window->addEvent("http_requested",currentUrl);
 QNetworkRequest req(url);req.setAttribute(QNetworkRequest::RedirectPolicyAttribute,QNetworkRequest::ManualRedirectPolicy);req.setTransferTimeout(18000);
 QNetworkReply *r=window->network.get(req);reply=r;connect(r,&QNetworkReply::finished,this,[this,r,url]{
  if(!client){r->deleteLater();return;}
  responseCode=r->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();if(!responseCode)responseCode=502;
  responseMime=r->header(QNetworkRequest::ContentTypeHeader).toByteArray();if(responseMime.isEmpty())responseMime="application/octet-stream";
  responseBody=r->read(3000001);if(responseBody.size()>3000000){r->deleteLater();writeResponse(502,"Response too large");return;}
  responseHeaders.clear();auto location=r->rawHeader("Location");if(!location.isEmpty())responseHeaders+="Location: "+location+"\r\n";
  r->deleteLater();window->addEvent("http_response",currentUrl,responseCode);
  bool protectedResponse=responseCode==403||responseCode==429||url.path()=="/robots.txt";
  if(window->settings.value("save_snapshots").toBool(true)&&responseBody.size()&&responseCode==200){auto name=safeFilename(url,responseMime);QFile f(pages+"/"+name);if(f.open(QIODevice::WriteOnly|QIODevice::Truncate)){f.write(responseBody);f.close();window->reloadFiles();}}
  if(!protectedResponse && responseCode==200){
    QString ruleKey;int best=0;
    for(auto it=window->rules.begin();it!=window->rules.end();++it){const QString k=it.key();bool match=(k.endsWith('*'))?currentUrl.startsWith(k.left(k.size()-1)):(k==currentUrl);if(match&&k.size()>=best){best=k.size();ruleKey=k;}}
    if(!ruleKey.isEmpty()){
      QString fname=window->rules.value(ruleKey).toString();if(QFileInfo(fname).fileName()==fname&&!fname.contains("..")){QFile f(pages+"/"+fname);if(f.open(QIODevice::ReadOnly)){responseBody=f.readAll();window->addEvent("local_override_applied",currentUrl+" (rule "+ruleKey+")");}}
    }
  }
  if(!protectedResponse && responseCode==200 && responseMime.contains("text/html")&&window->waitMode()){
    waiting=true;window->review(this,currentUrl,responseBody);timeout=new QTimer(this);timeout->setSingleShot(true);connect(timeout,&QTimer::timeout,this,[this]{if(waiting)approve({},false);});timeout->start(qBound(1,window->settings.value("review_timeout_seconds").toInt(90),300)*1000);return;
  }
  finishResponse(responseBody);
 });
}
void ProxyClient::finishResponse(const QByteArray &body){if(replied)return;replied=true;if(!client)return;QByteArray head="HTTP/1.1 "+QByteArray::number(responseCode)+" "+statusReason(responseCode)+"\r\nContent-Type: "+responseMime+"\r\nContent-Length: "+QByteArray::number(method=="HEAD"?0:body.size())+"\r\nConnection: close\r\n"+responseHeaders+"\r\n";client->write(head);if(method!="HEAD")client->write(body);client->flush();client->disconnectFromHost();}
void ProxyClient::approve(const QByteArray &body,bool accepted){if(!waiting)return;waiting=false;if(timeout)timeout->stop();if(!accepted){window->addEvent("review_rejected",currentUrl);writeResponse(403,"Response rejected by local reviewer");replied=true;return;}window->addEvent("review_approved",currentUrl);finishResponse(body);}
int main(int argc,char **argv){
 if(argc==2 && QString::fromLocal8Bit(argv[1])=="--version"){
   fprintf(stdout,"%s\n",appVersion.toUtf8().constData());return 0;
 }
 if(argc==2 && QString::fromLocal8Bit(argv[1])=="--check-port"){
   QCoreApplication app(argc,argv);
   QTcpServer test;
   if(test.listen(QHostAddress::LocalHost,55193)){
     test.close();fprintf(stdout,"FREE: 127.0.0.1:55193\n");return 0;
   }
   QTcpSocket probe;probe.connectToHost(QHostAddress::LocalHost,55193);
   bool active=probe.waitForConnected(500);probe.abort();
   fprintf(stdout,"%s: 127.0.0.1:55193 (%s)\n",active?"IN USE":"UNAVAILABLE",test.errorString().toUtf8().constData());
   return active?2:3;
 }
 openlog("nov42026-proxy",LOG_PID|LOG_NDELAY,LOG_USER);
 QApplication app(argc,argv);app.setApplicationName("nov42026-proxy");
 ProxyWindow win;
 if(!win.server.isListening())return 2;
 win.show();return app.exec();
}
#include "main.moc"
