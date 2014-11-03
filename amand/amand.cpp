// amand.cpp
//
// amand(8) Monitoring Daemon.
//
//   (C) Copyright 2012 Fred Gleason <fredg@paravelsystems.com>
//
//      $Id: amand.cpp,v 1.19 2013/11/19 00:14:40 cvs Exp $
//
//   This program is free software; you can redistribute it and/or modify
//   it under the terms of the GNU General Public License version 2 as
//   published by the Free Software Foundation.
//
//   This program is distributed in the hope that it will be useful,
//   but WITHOUT ANY WARRANTY; without even the implied warranty of
//   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//   GNU General Public License for more details.
//
//   You should have received a copy of the GNU General Public
//   License along with this program; if not, write to the Free Software
//   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//

#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <syslog.h>
#include <sys/types.h>
#include <signal.h>
#include <unistd.h>
#include <errno.h>

#include <QtCore/QCoreApplication>
#include <QtCore/QProcess>
#include <QtCore/QDateTime>
#include <QtCore/QFile>
#include <QtCore/QProcess>
#include <QtCore/QVariant>
#include <QtGui/QApplication>
#include <QtNetwork/QTcpServer>
#include <QtSql/QSqlQuery>
#include <QtSql/QSqlError>

#include "cmdswitch.h"
#include "amand.h"

void SigHandler(int signo)
{
  switch(signo) {
  case SIGTERM:
  case SIGINT:
    unlink(AMAND_PID_FILE);
    exit(0);
  }
}


MainObject::MainObject(QObject *parent)
  :QObject(parent)
{
  FILE *f=NULL;

  debug=false;
  main_replication_test_state=true;

  //
  // Read Command Options
  //
  CmdSwitch *cmd=
    new CmdSwitch(qApp->argc(),qApp->argv(),"amand",AMAND_USAGE);
  for(unsigned i=0;i<cmd->keys();i++) {
    if(cmd->key(i)=="-d") {
      debug=true;
    }
  }
  delete cmd;

  //
  // Open Syslog
  //
  if(debug) {
    openlog("amand",LOG_PERROR,LOG_USER);
  }
  else {
    openlog("amand",0,LOG_USER);
  }

  //
  // Load Configuration
  //
  main_config=new Config("/etc/aman.conf");
  if(!main_config->load()) {
    syslog(LOG_ERR,"exiting due to configuration errors");
    exit(256);
  }

  //
  // State Object
  //
  main_state=new State();

  //
  // Snapshot Directory
  //
  main_snapshot_dir=new QDir(AM_SNAPSHOT_DIR);
  if(!main_snapshot_dir->exists()) {
    syslog(LOG_ERR,"snapshot directory \"%s\" missing",AM_SNAPSHOT_DIR);
    exit(256);
  }
  if(!main_snapshot_dir->isReadable()) {
    syslog(LOG_ERR,"snapshot directory \"%s\" not accessible",AM_SNAPSHOT_DIR);
    exit(256);
  }

  //
  // Audio RSync Process
  //
  main_audio_process=new QProcess(this);
  connect(main_audio_process,SIGNAL(finished(int,QProcess::ExitStatus)),
	  this,SLOT(rsyncFinishedData(int,QProcess::ExitStatus)));
  connect(main_audio_process,SIGNAL(error(QProcess::ProcessError)),
	  this,SLOT(rsyncErrorData(QProcess::ProcessError)));
  main_audio_holdoff_timer=new QTimer(this);
  main_audio_holdoff_timer->setSingleShot(true);
  connect(main_audio_holdoff_timer,SIGNAL(timeout()),
	  this,SLOT(startAudioCopy()));

  //
  // Secure Shell Identity
  //
  if(!QFile::exists(main_config->secureShellIdentity(Am::This))) {
    syslog(LOG_ERR,"ssh(1) identity file \"%s\" not found",
	   (const char *)main_config->secureShellIdentity(Am::This).toAscii());
    exit(256);
  }

  //
  // Create Ping Tables
  //
  if(!OpenMysql(Am::This,Config::PublicAddress)) {
    exit(256);
  }
  for(int i=0;i<Am::LastInstance;i++) {
    QString sql="create table if not exists "+
      main_config->pingTablename((Am::Instance)i)+" (VALUE int not null primary key)";
    QSqlQuery *q=new QSqlQuery(sql,Db());
    if(!q->isActive()) {
      syslog(LOG_ERR,"unable to create MySQL table %s at %s [%s]",
	     (const char *)main_config->pingTablename((Am::Instance)i).
	     toAscii(),
	     (const char *)main_config->address((Am::Instance)i,
						Config::PublicAddress).
	     toString().toAscii(),
	     (const char *)q->lastError().text().toAscii());
    }
    delete q;

    sql="delete from "+main_config->pingTablename((Am::Instance)i);
    q=new QSqlQuery(sql,Db());
    if(!q->isActive()) {
      syslog(LOG_ERR,"unable to delete from MySQL table %s at %s [%s]",
	     (const char *)main_config->pingTablename((Am::Instance)i).
	     toAscii(),
	     (const char *)main_config->address((Am::Instance)i,
						Config::PublicAddress).
	     toString().toAscii(),
	     (const char *)q->lastError().text().toAscii());
    }
    delete q;

    sql="insert into "+main_config->pingTablename((Am::Instance)i)+
      " set VALUE=0";
    q=new QSqlQuery(sql,Db());
    if(!q->isActive()) {
      syslog(LOG_ERR,"unable to insert into MySQL table %s at %s [%s]",
	     (const char *)main_config->pingTablename((Am::Instance)i).
	     toAscii(),
	     (const char *)main_config->address((Am::Instance)i,
						Config::PublicAddress).
	     toString().toAscii(),
	     (const char *)q->lastError().text().toAscii());
    }
    delete q;
  }
  CloseMysql();

  //
  // Command Server
  //
  QTcpServer *server=new QTcpServer(this);
  server->listen(QHostAddress::Any,AM_CMD_TCP_PORT);
  std::map<int,QString> cmds;
  std::map<int,int> upper_limits;
  std::map<int,int> lower_limits;

  cmds[Am::DisconnectCommand]="DC";
  upper_limits[Am::DisconnectCommand]=0;
  lower_limits[Am::DisconnectCommand]=0;

  cmds[Am::StateCommand]="ST";
  upper_limits[Am::StateCommand]=0;
  lower_limits[Am::StateCommand]=0;

  cmds[Am::GenerateSnapshotCommand]="GS";
  upper_limits[Am::GenerateSnapshotCommand]=0;
  lower_limits[Am::GenerateSnapshotCommand]=0;

  cmds[Am::LoadSnapshotCommand]="LS";
  upper_limits[Am::LoadSnapshotCommand]=1;
  lower_limits[Am::LoadSnapshotCommand]=1;

  cmds[Am::SetMetadataCommand]="SM";
  upper_limits[Am::SetMetadataCommand]=2;
  lower_limits[Am::SetMetadataCommand]=2;

  cmds[Am::MakeMasterCommand]="MM";
  upper_limits[Am::MakeMasterCommand]=0;
  lower_limits[Am::MakeMasterCommand]=0;

  cmds[Am::MakeSlaveCommand]="MS";
  upper_limits[Am::MakeSlaveCommand]=0;
  lower_limits[Am::MakeSlaveCommand]=0;

  cmds[Am::MakeIdleCommand]="MI";
  upper_limits[Am::MakeIdleCommand]=0;
  lower_limits[Am::MakeIdleCommand]=0;

  cmds[Am::StartAudioSlaveCommand]="AS";
  upper_limits[Am::StartAudioSlaveCommand]=0;
  lower_limits[Am::StartAudioSlaveCommand]=0;

  cmds[Am::StopAudioSlaveCommand]="AI";
  upper_limits[Am::StopAudioSlaveCommand]=0;
  lower_limits[Am::StopAudioSlaveCommand]=0;

  main_cmd_server=
    new StreamCmdServer(cmds,upper_limits,lower_limits,server,this);
  connect(main_cmd_server,SIGNAL(commandReceived(int,int,const QStringList &)),
	  this,SLOT(commandReceivedData(int,int,const QStringList &)));

  //
  // Replication Tester
  //
  main_repl_test=new ReplicationTest(Config::PrivateAddress,main_config,this);
  connect(main_repl_test,SIGNAL(testComplete(bool,int)),
	  this,SLOT(replicationTestCompleteData(bool,int)));

  //
  // Auto Rotation
  //
  main_auto_rotate_state=false;
  main_auto_rotate_timer=new QTimer(this);
  main_auto_rotate_timer->setSingleShot(true);
  connect(main_auto_rotate_timer,SIGNAL(timeout()),this,SLOT(autoRotateData()));
  ScheduleAutoRotation();
  
  //
  // Start Ping Monitor
  //
  main_monitor=new PingMonitor(main_config,this);
  main_monitor->setThisDbState(main_state->dbState());
  main_monitor->setThisAudioState(main_state->audioState());
  connect(main_monitor,
	  SIGNAL(thatStateChanged(bool,bool,bool,State::ClusterState,
				  const QString &,int,
				  State::ClusterState,bool)),
	  this,
	  SLOT(thatStateChangedData(bool,bool,bool,State::ClusterState,
				    const QString &,int,
				    State::ClusterState,bool)));
  if(!main_monitor->start()) {
    syslog(LOG_ERR,"aborting due to errors");
    exit(256);
  }

  //
  // Detach
  //
  if(!debug) {
    daemon(0,0);
  }

  //
  // Set Signals
  //
  signal(SIGTERM,SigHandler);
  signal(SIGINT,SigHandler);
  signal(SIGHUP,SigHandler);

  //
  // Write the PID file
  //
  if((f=fopen(AMAND_PID_FILE,"w"))==NULL) {
    syslog(LOG_ERR,"unable to write PID file at \"%s\"",AMAND_PID_FILE);
    exit(255);
  }
  fprintf(f,"%d",getpid());
  fclose(f);

  //
  // Start MySQL Monitoring
  //
  QTimer *timer=new QTimer(this);
  connect(timer,SIGNAL(timeout()),this,SLOT(checkStatusData()));
  timer->start(AM_MYSQL_MONITOR_INTERVAL);

  //
  // Start Audio Monitoring
  //
  ScheduleAudioCopy(0);

  //
  // Set Nice Level
  //
  if(nice(main_config->globalNiceLevel())<0) {
    syslog(LOG_WARNING,"failed to set process priority to %d [%s]",
	   main_config->globalNiceLevel(),strerror(errno));
  }
  else {
    syslog(LOG_DEBUG,"set process priority to %d",
	   main_config->globalNiceLevel());
  }
}


MainObject::~MainObject()
{
}


void MainObject::commandReceivedData(int id,int cmd,const QStringList &args)
{
  QStringList outargs;
  QString filename;
  QString binlog_name="";
  int binlog_pos=0;

  switch((Am::Command)cmd) {
  case Am::StateCommand:
    main_cmd_server->sendCommand(id,Am::StateCommand,StateUpdateArgs());
    break;

  case Am::DisconnectCommand:
    main_cmd_server->closeConnection(id);
    break;

  case Am::GenerateSnapshotCommand:
    filename=MakeSnapshotName();
    if(GenerateMysqlSnapshot(QString(AM_SNAPSHOT_DIR)+"/"+filename)) {
      outargs.push_back(filename);
    }
    else {
      outargs.push_back("-");
    }
    main_cmd_server->sendCommand(id,Am::GenerateSnapshotCommand,outargs);
    break;

  case Am::LoadSnapshotCommand:
    if((args.size()==2)&&
	(RestoreMysqlSnapshot(QString(AM_SNAPSHOT_DIR)+"/"+args[0],
			      &binlog_name,&binlog_pos))) {
      outargs.push_back(args[0]);
      outargs.push_back(binlog_name);
      outargs.push_back(QString().sprintf("%d",binlog_pos));
    }
    else {
      outargs.push_back("-");
      outargs.push_back("-");
      outargs.push_back("0");
    }
    main_cmd_server->sendCommand(id,Am::LoadSnapshotCommand,outargs);
    break;

  case Am::MakeMasterCommand:
    StopSlaves();
    filename=MakeSnapshotName();
    if(GenerateMysqlSnapshot(QString(AM_SNAPSHOT_DIR)+"/"+filename)) {
      outargs.push_back(filename);
      main_state->setDbState(State::StateMaster);
      main_monitor->setThisDbState(State::StateMaster);
      main_state->setCurrentSnapshot(Am::This,filename);
      main_monitor->setThisSnapshotName(filename);
      SendAlert("Database Replication State changed to MASTER on server \""+
		main_config->hostname(Am::This)+"\".");
      syslog(LOG_INFO,"state changed to MASTER");
    }
    else {
      outargs.push_back("-");
    }
    main_cmd_server->sendCommand(id,Am::MakeMasterCommand,outargs);
    break;

  case Am::MakeSlaveCommand:
    if(RestoreMysqlSnapshot(QString(AM_SNAPSHOT_DIR)+"/"+
			    main_state->currentSnapshot(Am::That),
			    &binlog_name,&binlog_pos)) {
      main_state->setDbState(State::StateSlave);
      main_monitor->setThisDbState(State::StateSlave);
      outargs.push_back(main_state->currentSnapshot(Am::That));
      SendAlert("Database Replication State changed to SLAVE on server \""+
		main_config->hostname(Am::This)+"\".");
      syslog(LOG_INFO,"state changed to SLAVE");
    }
    else {
      outargs.push_back("-");
    }
    main_cmd_server->sendCommand(id,Am::MakeSlaveCommand,outargs);
    break;

  case Am::MakeIdleCommand:
    StopSlaves();
    main_state->setDbState(State::StateIdle);
    main_monitor->setThisDbState(State::StateIdle);
    break;

  case Am::SetMetadataCommand:
    if((args.size()==2)&&(SetMysqlMetadata(args[0],args[1].toInt()))) {
      outargs.push_back(args[0]);
      outargs.push_back(args[1]);
      main_state->setDbState(State::StateMaster);
      main_monitor->setThisDbState(State::StateMaster);
    }
    else {
      outargs.push_back("-");
      outargs.push_back("0");
    }
    main_cmd_server->sendCommand(id,Am::SetMetadataCommand,outargs);
    break;

  case Am::StartAudioSlaveCommand:
    if((main_monitor->audioState(Am::This)!=State::StateSlave)&&
       (main_monitor->audioState(Am::That)!=State::StateSlave)) {
      main_state->setAudioState(State::StateSlave);
      main_monitor->setThisAudioState(State::StateSlave);
      ScheduleAudioCopy(0);
      SendAlert("Audio Replication State changed to SLAVE on server \""+
	      main_config->hostname(Am::This)+"\".");
    }
    main_cmd_server->sendCommand(id,Am::StartAudioSlaveCommand);
    break;

  case Am::StopAudioSlaveCommand:
    StopAudioCopy();
    main_state->setAudioState(State::StateIdle);
    main_monitor->setThisAudioState(State::StateIdle);
    main_cmd_server->sendCommand(id,Am::StopAudioSlaveCommand);
    SendAlert("Audio Replication State changed to IDLE on server \""+
	      main_config->hostname(Am::This)+"\".");
    break;
  }
  main_cmd_server->sendCommand(Am::StateCommand,StateUpdateArgs());
}


void MainObject::thatStateChangedData(bool ping,bool running,bool accessible,
				      State::ClusterState db_state,
				      const QString &snapshot,
				      int replication_time,
				      State::ClusterState audio_state,
				      bool audio_status)
{
  main_state->setCurrentSnapshot(Am::That,snapshot);
  switch(main_state->dbState()) {
  case State::StateSlave:
    if(db_state==State::StateIdle) {
      main_monitor->setThisDbState(State::StateIdle);
      main_state->setDbState(State::StateIdle);
      SendAlert("Database Replication State changed to IDLE on server \""+
		main_config->hostname(Am::This)+"\".");
      syslog(LOG_INFO,"DB state changed to IDLE");
    }
    break;

  case State::StateMaster:
    if(db_state==State::StateMaster) {
      main_monitor->setThisDbState(State::StateIdle);
      main_state->setDbState(State::StateIdle);
      SendAlert("Database Replication State changed to IDLE on server \""+
		main_config->hostname(Am::This)+"\".");
      syslog(LOG_INFO,"DB state changed to IDLE");
    }
    break;

  case State::StateIdle:
  case State::StateOffline:
    break;
  }

  
  switch(main_state->audioState()) {
  case State::StateSlave:
    break;

  case State::StateMaster:
  case State::StateIdle:
  case State::StateOffline:
    break;
  }
  main_cmd_server->sendCommand(Am::StateCommand,StateUpdateArgs());
}


void MainObject::checkStatusData()
{
  if(!CheckLocalState()) {
    return;
  }
  main_cmd_server->sendCommand(Am::StateCommand,StateUpdateArgs());
}


void MainObject::replicationTestCompleteData(bool success,int msecs)
{
  if(success) {
    if(main_monitor->mysqlReplicationTime(Am::This)!=msecs) {
      main_monitor->setThisMysqlReplicationTime(msecs);
      main_cmd_server->sendCommand(Am::StateCommand,StateUpdateArgs());
    }
    if(!main_replication_test_state) {
      if(main_monitor->dbState(Am::That)==State::StateSlave) {
	SendAlert("Database replication has RESUMED between servers \""+
		  main_config->hostname(Am::This)+"\" and \""+
		  main_config->hostname(Am::That)+"\".");
	syslog(LOG_INFO,"replication restored for mysql at %s",
	       (const char *)main_config->
	       address(Am::That,Config::PrivateAddress).toString().
	       toAscii());
      }
      main_replication_test_state=true;
    }
  }
  else {
    if(main_monitor->mysqlReplicationTime(Am::This)!=0) {
      main_monitor->setThisMysqlReplicationTime(0);
      main_cmd_server->sendCommand(Am::StateCommand,StateUpdateArgs());
    }
    if(main_replication_test_state) {
      if(main_monitor->dbState(Am::That)==State::StateSlave) {
	SendAlert("Database replication has STOPPED between servers \""+
		  main_config->hostname(Am::This)+"\" and \""+
		  main_config->hostname(Am::That)+"\".");	  
	syslog(LOG_WARNING,"replication failed for mysql at %s",
	       (const char *)main_config->
	       address(Am::That,Config::PrivateAddress).toString().
	       toAscii());
      }
      main_replication_test_state=false;
    }
  }
}


QStringList MainObject::StateUpdateArgs() const
{
  QStringList ret;
  Am::Instance inst[2];

  inst[0]=main_config->instanceA();
  inst[1]=main_config->instanceB();

  for(int i=0;i<Am::LastInstance;i++) {
    ret.push_back(main_config->hostname(inst[i]));
    ret.push_back(QString().sprintf("%d",main_monitor->isReachable(inst[i])));
    ret.push_back(QString().sprintf("%d",main_monitor->mysqlRunning(inst[i])));
    ret.push_back(QString().sprintf("%d",
		  main_monitor->mysqlAccessible(inst[i])));
    ret.push_back(QString().sprintf("%d",main_monitor->dbState(inst[i])));
    ret.push_back(QString().sprintf("%d",
		  main_monitor->mysqlReplicationTime(inst[i])));
    ret.push_back(QString().sprintf("%d",main_monitor->audioState(inst[i])));
    ret.push_back(QString().sprintf("%d",main_monitor->audioStatus(inst[i])));
  }

  return ret;
}


bool MainObject::CheckLocalState()
{
  bool running=false;
  bool accessible=false;
  int testval=rand();

  if(main_repl_test->isActive()) {
    return false;
  }
  if((running=IsMysqlRunning())) {
    if((accessible=IsMysqlAccessible(testval))) {
      main_repl_test->startTest(testval);
    }
  }
  bool ret=(running!=main_monitor->mysqlRunning(Am::This))||
    (accessible!=main_monitor->mysqlAccessible(Am::This));
  if(ret) {
    main_monitor->setThisMysqlState(running,accessible);
  }
  return ret;
}


bool MainObject::IsMysqlRunning() const
{
  int code=system("service mysqld status > /dev/null 2> /dev/null");
  return code==0;
}


bool MainObject::IsMysqlAccessible(int testval)
{
  QString sql;
  QSqlQuery *q;
  bool ret=false;

  if(!OpenMysql(Am::This,Config::PublicAddress)) {
    return ret;
  }
  sql=QString("update ")+main_config->pingTablename(Am::This)+
    QString().sprintf(" set VALUE=%d",testval);
  q=new QSqlQuery(sql,Db());
  delete q;

  sql=QString("select VALUE from ")+main_config->pingTablename(Am::This);
  q=new QSqlQuery(sql,Db());
  if(q->first()) {
    ret=q->value(0).toInt()==testval;
  }
  delete q;

  CloseMysql();

  return ret;
}


bool MainObject::StopSlaves()
{
  QString sql;
  QSqlQuery *q;
  Config::Address addr;

  //
  // Open Mysql
  //
  addr=Config::PublicAddress;
  if(!OpenMysql(Am::This,addr)) {
    addr=Config::PublicAddress;
    if(!OpenMysql(Am::This,addr)) {
      return false;
    }
  }

  sql="stop slave";
  q=new QSqlQuery(sql,Db());
  if(!q->isActive()) {
    syslog(LOG_ERR,"cannot stop slave process in mysql at %s [%s]",
	   (const char *)main_config->address(Am::This,addr).toString().
	   toAscii(),
	   (const char *)q->lastError().text().toAscii());
    delete q;
    CloseMysql();
    return false;
  }
  delete q;
  CloseMysql();

  return true;
}


QSqlDatabase MainObject::Db()
{
  return QSqlDatabase::database("main_db");
}


bool MainObject::OpenMysql(Am::Instance inst,Config::Address addr)
{
  QSqlDatabase db=
    QSqlDatabase::addDatabase(main_config->globalMysqlDriver(),"main_db");
  db.setDatabaseName(main_config->globalMysqlDatabase());
  db.setUserName(main_config->mysqlUsername(inst));
  db.setPassword(main_config->mysqlPassword(inst));
  db.setHostName(main_config->address(inst,addr).toString());
  if(!db.open()) {
    syslog(LOG_ERR,"cannot connect to mysql at %s [%s]",
	   (const char *)main_config->address(inst,addr).toString().toAscii(),
	   (const char *)db.lastError().text().toAscii());
    return false;
  }
  return true;
}


void MainObject::CloseMysql()
{
  QSqlDatabase::removeDatabase("main_db");
}


bool MainObject::PushFile(const QString &srcfile,const QString &desthost,
			  const QString &destfile)
{
  QProcess *proc=new QProcess(this);
  QStringList args;
  bool ret=true;

  args.push_back("-i");
  args.push_back(main_config->secureShellIdentity(Am::This));
  args.push_back(srcfile);
  args.push_back(desthost+":"+destfile);
  proc->start("scp",args);
  proc->waitForFinished(-1);
  if(proc->exitCode()!=0) {
    syslog(LOG_ERR,"file push \"%s\" to \"%s:%s\" failed [%s]",
	   (const char *)srcfile.toAscii(),
	   (const char *)desthost.toAscii(),
	   (const char *)destfile.toAscii(),
	   (const char *)proc->readAllStandardError());    
    ret=false;
  }
  delete proc;
  return ret;
}


QString MainObject::MakeTempDir() const
{
  char dirpath[PATH_MAX];

  strcpy(dirpath,"/tmp/amanXXXXXX");
  if(mkdtemp(dirpath)==NULL) {
    syslog(LOG_ERR,"unable to create temporary directory [%s]",strerror(errno));
    return QString();
  }
  return QString(dirpath);
}


QString MainObject::MakeSnapshotName() const
{
  QDateTime current_datetime=
    QDateTime(QDate::currentDate(),QTime::currentTime());
  return main_config->hostname(Am::This)+"-snapshot-"+
    current_datetime.toString("yyyyMMddhhmmss")+".tar.bz2";
}


void MainObject::SendAlert(const QString &msg)
{
  QProcess *p;
  QString text;
  QStringList args;

  if(main_config->globalAlertAddress().isEmpty()) {
    return;
  }

  //
  // Compose Message
  //
  text="From: Rivendell Server Monitor on "+main_config->hostname(Am::This)+
    " <"+main_config->globalFromAddress()+">\n";
  text+="To: "+main_config->globalAlertAddress()+"\n";
  text+="Subject: "+tr("Rivendell Server Alert")+"\n";
  text+="\n";
  text+=msg;
  text+="\n";
  text+=".\n";

  //
  // Generate Arguments
  //
  args.push_back("-bm");
  args.push_back(main_config->globalAlertAddress());

  //
  // Send Message
  //
  p=new QProcess(this);
  p->start("sendmail",args);
  if(!p->waitForStarted()) {
    syslog(LOG_WARNING,"unable to send mail to \"%s\"",
	   (const char *)main_config->globalAlertAddress().toAscii());
    delete p;
    return;
  }
  p->write(text.toUtf8());
  if(!p->waitForFinished()) {
    syslog(LOG_WARNING,"unable to send mail to \"%s\"",
	   (const char *)main_config->globalAlertAddress().toAscii());
  }

  delete p;
}


int main(int argc,char *argv[])
{
  QCoreApplication a(argc,argv);
  new MainObject();
  return a.exec();
}
