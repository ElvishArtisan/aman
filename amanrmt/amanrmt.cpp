// amanrmt.cpp
//
// amanrmt(8) Monitoring Client.
//
//   (C) Copyright 2012-2022 Fred Gleason <fredg@paravelsystems.com>
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

#include <syslog.h>
#include <sys/types.h>
#include <unistd.h>

#include <curl/curl.h>

#include <QApplication>
#include <QCloseEvent>
#include <QFileDialog>
#include <QFontMetrics>
#include <QMessageBox>
#include <QPainter>
#include <QSignalMapper>
#include <QSqlDatabase>
#include <QSqlQuery>

#include <amsendmail.h>

#include "amanrmt.h"

//
// Icons
//
#include "../icons/aman-22x22.xpm"

MainWidget::MainWidget(QWidget *parent)
  : QWidget(parent)
{
  QString err_msg;

  am_connection_table[0]=0;
  am_connection_table[1]=1;
  am_active_connection=NULL;
  am_exiting=false;

  setWindowTitle(tr("Rivendell Server Manager")+" v"+VERSION);
  setWindowIcon(QPixmap(aman_xpm));
  setMinimumWidth(sizeHint().width());
  setMaximumWidth(sizeHint().width());
  setMinimumHeight(sizeHint().height());
  setMaximumHeight(sizeHint().height());

  //
  // Check for root permissions
  //
  if(geteuid()!=0) {
    QMessageBox::information(this,tr("Server Manager"),
			     tr("This program requires root permissions."));
    exit(0);
  }

  //
  // Initialize CURL
  //
  curl_global_init(CURL_GLOBAL_ALL);

  //
  // Fonts
  //
  QFont title_font(font().family(),font().pointSize(),QFont::Bold);
  QFont label_font(font().family(),font().pointSize()+2,QFont::Bold);

  //
  // Load Configuration
  //
  am_config=new AMConfig("/etc/aman.conf",true);
  if(!am_config->load()) {
    QMessageBox::warning(this,tr("Server Monitor"),
	       tr("Unable to read configuration from \"/etc/aman.conf\"."));
    exit(256);
  }

  //
  // Connect to local DB
  //
  if(!OpenDb(&err_msg)) {
    QMessageBox::critical(this,"AmanRmt - "+tr("Error"),err_msg);
    exit(1);
  }

  //
  // Source Status Connections
  //
  QSignalMapper *connected_mapper=new QSignalMapper(this);
  connect(connected_mapper,SIGNAL(mapped(int)),this,SLOT(connectedData(int)));
  QSignalMapper *disconnected_mapper=new QSignalMapper(this);
  connect(disconnected_mapper,SIGNAL(mapped(int)),
	  this,SLOT(disconnectedData(int)));
  for(int i=0;i<2;i++) {
    am_connection[i]=new AMConnection(this);
    connect(am_connection[i],SIGNAL(connected()),
	    connected_mapper,SLOT(map()));
    connected_mapper->setMapping(am_connection[i],i);
    connect(am_connection[i],SIGNAL(disconnected()),
	    disconnected_mapper,SLOT(map()));
    disconnected_mapper->setMapping(am_connection[i],i);
  }
  for(int i=0;i<2;i++) {
    connect(am_connection[i],SIGNAL(statusChanged(AMStatus *,AMStatus *)),
	    this,SLOT(statusChangedData(AMStatus *,AMStatus *)));
    connect(am_connection[i],SIGNAL(snapshotGenerated(const QString &)),
	    this,SLOT(snapshotGeneratedData(const QString &)));
    connect(am_connection[i],SIGNAL(snapshotLoaded(const QString &)),
	    this,SLOT(snapshotLoadedData(const QString &)));
    connect(am_connection[i],SIGNAL(errorReturned(const QString &)),
	    this,SLOT(showConnectionError(const QString &)));
  }

  //
  // Progress Dialog
  //
  am_progress_dialog=new AMProgressDialog(this);

  //
  // Source Servers
  //
  am_source_label=new QLabel(tr("Source Servers"),this);
  am_source_label->setFont(label_font);
  am_source_label->setAlignment(Qt::AlignCenter);

  for(int i=0;i<2;i++) {
    am_src_system_label[i]=new QLabel(this);
    am_src_system_label[i]->setAlignment(Qt::AlignCenter);
    am_src_system_label[i]->setFont(title_font);

    am_src_hostname_label[i]=new QLabel(tr("Hostname:"),this);
    am_src_hostname_label[i]->setAlignment(Qt::AlignVCenter|Qt::AlignRight);
    am_src_hostname_edit[i]=new QLineEdit(this);
    am_src_hostname_edit[i]->setReadOnly(true);
    am_src_hostname_edit[i]->setText(tr("[unknown]"));

    am_src_db_state_label[i]=new QLabel(tr("DB State:"),this);
    am_src_db_state_label[i]->setAlignment(Qt::AlignVCenter|Qt::AlignRight);
    am_src_db_state_edit[i]=new QLineEdit(this);
    am_src_db_state_edit[i]->setReadOnly(true);
    am_src_db_state_edit[i]->setText(tr("UNKNOWN"));

    am_src_db_replicating_label[i]=new QLabel(tr("DB Replicating"),this);
    am_src_db_replicating_label[i]->setAlignment(Qt::AlignVCenter|Qt::AlignLeft);
    am_src_db_replicating_light[i]=new AMStatusLight(this);

    am_src_audio_state_label[i]=new QLabel(tr("Audio State:"),this);
    am_src_audio_state_label[i]->setAlignment(Qt::AlignVCenter|Qt::AlignRight);
    am_src_audio_state_edit[i]=new QLineEdit(this);
    am_src_audio_state_edit[i]->setReadOnly(true);
    am_src_audio_state_edit[i]->setText(tr("UNKNOWN"));

    am_src_audio_replicating_label[i]=new QLabel(tr("Audio Replicating"),this);
    am_src_audio_replicating_label[i]->setAlignment(Qt::AlignVCenter|Qt::AlignLeft);
    am_src_audio_replicating_light[i]=new AMStatusLight(this);

  }
  am_src_system_label[0]->setText(tr("System A"));
  am_src_system_label[1]->setText(tr("System B"));

  //
  // Destination Server
  //
  am_destination_label=new QLabel(tr("Destination Server"),this);
  am_destination_label->setFont(label_font);
  am_destination_label->setAlignment(Qt::AlignCenter);

  am_dst_db_state_label=new QLabel(tr("DB State:"),this);
  am_dst_db_state_label->setAlignment(Qt::AlignVCenter|Qt::AlignRight);
  am_dst_db_state_edit=new QLineEdit(this);
  am_dst_db_state_edit->setReadOnly(true);
  am_dst_db_state_edit->setText(AMState::stateString(AMState::StateIdle));

  am_dst_db_replicating_label=new QLabel(tr("DB Replicating"),this);
  am_dst_db_replicating_label->setAlignment(Qt::AlignVCenter|Qt::AlignLeft);
  am_dst_db_replicating_light=new AMStatusLight(this);

  am_dst_audio_state_label=new QLabel(tr("Audio State:"),this);
  am_dst_audio_state_label->setAlignment(Qt::AlignVCenter|Qt::AlignRight);
  am_dst_audio_state_edit=new QLineEdit(this);
  am_dst_audio_state_edit->setReadOnly(true);
  am_dst_audio_state_edit->setText(AMState::stateString(AMState::StateIdle));

  am_dst_audio_replicating_label=new QLabel(tr("Audio Replicating"),this);
  am_dst_audio_replicating_label->setAlignment(Qt::AlignVCenter|Qt::AlignLeft);
  am_dst_audio_replicating_light=new AMStatusLight(this);

  am_db_slave_button=new QPushButton(tr("Make Slave"),this);
  am_db_slave_button->setFont(title_font);
  connect(am_db_slave_button,SIGNAL(clicked()),
	  this,SLOT(makeDbSlaveData()));
  am_db_slave_button->setDisabled(true);

  am_db_idle_button=new QPushButton(tr("Make Idle"),this);
  am_db_idle_button->setFont(title_font);
  connect(am_db_idle_button,SIGNAL(clicked()),
	  this,SLOT(makeDbIdleData()));
  am_db_idle_button->setDisabled(true);

  am_audio_slave_button=new QPushButton(tr("Start Slave"),this);
  am_audio_slave_button->setFont(title_font);
  connect(am_audio_slave_button,SIGNAL(clicked()),
	  this,SLOT(startAudioData()));
  am_audio_slave_button->setDisabled(true);

  am_audio_idle_button=new QPushButton(tr("Make Idle"),this);
  am_audio_idle_button->setFont(title_font);
  connect(am_audio_idle_button,SIGNAL(clicked()),
	  this,SLOT(stopAudioData()));
  am_audio_idle_button->setDisabled(true);

  //
  // Start Source Server Connections
  //
  am_connection[0]->
    connectToHost(am_config->address(0,AMConfig::PublicAddress).toString(),
		  AM_CMD_TCP_PORT);
  am_connection[1]->
    connectToHost(am_config->address(1,AMConfig::PublicAddress).toString(),
		  AM_CMD_TCP_PORT);

  EnableSourceFields(0,false);
  EnableSourceFields(1,false);

  //
  // DB Replication Monitoring
  //
  am_check_db_prev_ping=0;
  am_check_db_replication_timer=new QTimer(this);
  connect(am_check_db_replication_timer,SIGNAL(timeout()),
	  this,SLOT(checkDbReplicationData()));
  am_check_db_replication_timer->start(1000);

  //
  // Audio Replication
  //
  am_audio_active=false;
  am_audio_process=NULL;
  am_audio_timer=new QTimer(this);
  am_audio_timer->setSingleShot(true);
  connect(am_audio_timer,SIGNAL(timeout()),this,SLOT(startAudioProcessData()));
}


QSize MainWidget::sizeHint() const
{
  return QSize(400,500);
}


void MainWidget::connectedData(int inst)
{
  EnableSourceFields(inst,true);
}


void MainWidget::disconnectedData(int inst)
{
  EnableSourceFields(inst,false);
}


void MainWidget::makeDbSlaveData()
{
  //
  // Get Master Connection
  //
  int master_id=GetMasterServerId();
  if(master_id<0) {
    QMessageBox::warning(this,tr("Server Manager - Error"),
			 tr("No master server available!"));
    am_active_connection=NULL;
    return;
  }
  am_active_connection=am_connection[master_id];

  //
  // Get Snapshot
  //
  am_active_connection->generateSnapshot();

  am_progress_dialog->show();
}


void MainWidget::makeDbIdleData()
{
  QString sql;
  QSqlQuery *q=NULL;

  //
  // Stop Replication
  //
  sql="stop slave";
  q=new QSqlQuery(sql);
  delete q;

  sql="reset slave";
  q=new QSqlQuery(sql);
  delete q;

  am_db_slave_button->setEnabled(true);
  am_db_idle_button->setDisabled(true);
}


void MainWidget::startAudioData()
{
  am_audio_timer->start(1000);
  am_audio_active=true;
}


void MainWidget::stopAudioData()
{
  am_audio_active=false;
  if(am_audio_process!=NULL) {
    am_audio_process->terminate();
  }
}


void MainWidget::statusChangedData(AMStatus *a,AMStatus *b)
{
  am_progress_dialog->hide();

  //
  // Update Connection Table
  //
  char sname[1024];
  gethostname(sname,1024);
  QStringList names=QString(sname).split(".");
  if(names[0].toLower()==b->hostname().toLower()) {
    am_connection_table[0]=1;
    am_connection_table[1]=0;
  }

  //
  // Update System A Status
  //
  UpdateSourceStatus(0,a);
  am_src_db_replicating_light[0]->setStatus(b->dbReplicationTime()>0);
  am_src_db_replicating_light[1]->setStatus(a->dbReplicationTime()>0);
  am_src_audio_replicating_light[0]->setEnabled(a->audioState()==AMState::StateSlave);
  am_src_audio_replicating_light[1]->setEnabled(b->audioState()==AMState::StateSlave);
  switch(a->audioState()) {
  case AMState::StateSlave:
    am_src_audio_state_edit[0]->setText(AMState::stateString(AMState::StateSlave));
    break;

  case AMState::StateIdle:
    if(b->audioState()==AMState::StateSlave) {
      am_src_audio_state_edit[0]->setText(AMState::stateString(AMState::StateMaster));
    }
    else {
      am_src_audio_state_edit[0]->setText(AMState::stateString(AMState::StateIdle));
    }
    break;

  case AMState::StateMaster:
  case AMState::StateOffline:
    am_src_audio_state_edit[0]->setText(AMState::stateString(a->audioState()));
    break;
  }

  //
  // Update System B Status
  //
  UpdateSourceStatus(1,b);
  switch(b->audioState()) {
  case AMState::StateSlave:
    am_src_audio_state_edit[1]->setText(AMState::stateString(AMState::StateSlave));
    break;

  case AMState::StateIdle:
    if(b->audioState()==AMState::StateSlave) {
      am_src_audio_state_edit[1]->setText(AMState::stateString(AMState::StateMaster));
    }
    else {
      am_src_audio_state_edit[1]->setText(AMState::stateString(AMState::StateIdle));
    }
    break;

  case AMState::StateMaster:
  case AMState::StateOffline:
    am_src_audio_state_edit[1]->setText(AMState::stateString(a->audioState()));
    break;
  }

  //
  // Update Local State Indicators
  //
  am_db_slave_button->setEnabled(((a->dbState()==AMState::StateMaster)||
				  (b->dbState()==AMState::StateMaster)));
  am_db_idle_button->setEnabled(a->dbState()==AMState::StateSlave);
}


void MainWidget::snapshotGeneratedData(const QString &name)
{
  QString err_msg;
  QString binlog;
  int pos=0;

  if(!am_active_connection->
     downloadSnapshot(name,AM_AMANRMT_IDENTITY_FILE,&err_msg)) {
    QMessageBox::warning(this,tr("Server Manager - Error"),
			 tr("Error generating snapshot")+": "+err_msg);
    return;
  }
  if(!RestoreMysqlSnapshot(name,&binlog,&pos,GetMasterServerId(),&err_msg)) {
    QMessageBox::warning(this,tr("Server Manager - Error"),
			 tr("Error restoring snapshot")+": "+name+"\n"+
			 "["+err_msg+"]");
    return;
  }
  unlink((QString(AM_SNAPSHOT_DIR)+"/"+name).toUtf8());
  am_db_slave_button->setDisabled(true);
  am_db_idle_button->setEnabled(true);
}


void MainWidget::loadSnapshotData()
{
  QString snapshot=QFileDialog::getOpenFileName(this,
		tr("Server Manager - Get Snapshot"),AM_SNAPSHOT_DIR,
		QString("*.")+AM_SNAPSHOT_EXT);
  if(snapshot.isEmpty()) {
    return;
  }
  QStringList path=snapshot.split("/");
  am_connection[0]->loadSnapshot(path[path.size()-1]);
}


void MainWidget::snapshotLoadedData(const QString &name)
{
  QMessageBox::information(this,tr("Server Maanger - Snapshot Loaded"),
			   tr("Loaded snapshot")+":\n \""+name+"\".");
}


void MainWidget::checkDbReplicationData()
{
  QString sql;
  QSqlQuery *q=NULL;
  int master_id=-1;

  //
  // DB State Check
  //
  // FIXME: "information_schema" becomes "performance_schema" in MySQL 5.7
  //        and later. See:
  //  https://stackoverflow.com/questions/7009021/how-to-know-mysql-replication-status-using-a-select-query
  //
  sql=QString("select variable_value FROM information_schema.global_status ")+
	      "where variable_name='SLAVE_RUNNING'";
  q=new QSqlQuery(sql);
  if(q->first()) {
    if(q->value(0).toString().toUpper()=="ON") {
      SetClusterState(am_dst_db_state_edit,
		      tr("DB replication state changed to SLAVE"),
		      AMState::StateSlave);
      am_db_slave_button->setDisabled(true);
      am_db_idle_button->setEnabled(true);
    }
    else {
      SetClusterState(am_dst_db_state_edit,
		      tr("DB replication state changed to IDLE"),
		      AMState::StateIdle);
      am_db_slave_button->setEnabled(true);
      am_db_idle_button->setDisabled(true);
    }
  }
  else {
    SetClusterState(am_dst_db_state_edit,
		    tr("DB replication state changed to OFFLINE"),
		    AMState::StateOffline);
    am_db_slave_button->setDisabled(true);
    am_db_idle_button->setDisabled(true);
  }
  delete q;

  //
  // DB Confidence Monitor Check
  //
  if((master_id=GetMasterServerId())>=0) {
    sql=QString("select ")+
      "`VALUE` "+  // 00
      "from `"+am_config->pingTablename(master_id)+"`";
    q=new QSqlQuery(sql);
    if(q->next()) {
      if(q->value(0).toUInt()!=am_check_db_prev_ping) {
	SetLightStatus(am_dst_db_replicating_light,tr("DB replication RESUMED"),
		       true);
      }
      else {
	SetLightStatus(am_dst_db_replicating_light,tr("DB replication STOPPED"),
		       false);
      }
      am_check_db_prev_ping=q->value(0).toUInt();
    }
    else {
      am_check_db_prev_ping=0;
      SetLightStatus(am_dst_db_replicating_light,tr("DB replication STOPPED"),
		     false);
    }
    delete q;
  }
  else {
    am_check_db_prev_ping=0;
    SetLightStatus(am_dst_db_replicating_light,tr("DB replication STOPPED"),
		   false);
  }

  //
  // Audio State Check
  //
  if(am_audio_active) {
    SetClusterState(am_dst_audio_state_edit,
		    tr("Audio replication state changed to SLAVE"),
		    AMState::StateSlave);
  }
  else {
    SetClusterState(am_dst_audio_state_edit,
		    tr("Audio replication state changed to IDLE"),
		    AMState::StateIdle);
    SetLightStatus(am_dst_audio_replicating_light,
		   tr("Audio replication STOPPED"),false);
  }
  am_audio_slave_button->setDisabled(am_audio_active);
  am_audio_idle_button->setEnabled(am_audio_active);
}


void MainWidget::startAudioProcessData()
{
  QStringList args;
  int master_id=GetMasterServerId();

  if(master_id<0) {
    am_audio_timer->start(AM_RSYNC_PAUSE_INTERVAL);
    return;
  }

  //
  // Check that the master is alive and ready
  //
  args.clear();
  args.push_back(am_config->address(master_id,AMConfig::PublicAddress).
		 toString()+"::rivendell/repl.chk");
  am_audio_process=new QProcess(this);
  am_audio_process->start("rsync",args);
  am_audio_process->waitForFinished(30000);
  switch(am_audio_process->exitCode()) {
  case 0:
    SetLightStatus(am_dst_audio_replicating_light,
		   tr("Audio replication RESUMED"),true);
    break;

  case 23:  // Check file not found, deferring sync
  case 30:  // Rsync service timed out, deferring sync
  case 35:  // Connection timeout, deferring sync
    delete am_audio_process;
    am_audio_process=NULL;
    SetLightStatus(am_dst_audio_replicating_light,
		   tr("Audio replication STOPPED"),true);
    am_audio_timer->start(AM_RSYNC_PAUSE_INTERVAL);
    return;

  default:
    QMessageBox::warning(this,tr("Server Manager - Error"),
			 tr("rsync(1) process returned an error, exit code")+
			 QString::asprintf(" %d",am_audio_process->exitCode()));
    delete am_audio_process;
    am_audio_process=NULL;
    am_audio_active=false;
    return;
  }
  delete am_audio_process;
  am_audio_process=NULL;

  //
  // Start the main file copy
  //
  args.clear();
  args.push_back("-a");
  args.push_back("--delete");
  args.push_back(am_config->address(master_id,AMConfig::PublicAddress).
		 toString()+"::rivendell/");
  args.push_back("/var/snd/");
  am_audio_process=new QProcess(this);
  connect(am_audio_process,SIGNAL(finished(int,QProcess::ExitStatus)),
	  this,SLOT(audioProcessFinishedData(int,QProcess::ExitStatus)));
  am_audio_process->start("rsync",args);
}


void MainWidget::audioProcessFinishedData(int exit_code,
					  QProcess::ExitStatus status)
{
  if(am_exiting) {
    exit(0);
  }
  if(status==QProcess::CrashExit) {
    QMessageBox::warning(this,tr("Server Manager - Error"),
			 tr("rsync(1) crashed!"));
    delete am_audio_process;
    am_audio_process->deleteLater();
    am_audio_process=NULL;
    am_audio_active=false;
    return;
  }

  switch(am_audio_process->exitCode()) {
  case 0:   // Normal Exit
  case 20:  // Stopped due to signal
  case 23:  // Partial transfer due to error
  case 24:  // Partial transfer due to vanished source files
    if(am_audio_active) {
      am_audio_timer->start(AM_RSYNC_PAUSE_INTERVAL);
    }
    break;

  default:
    QMessageBox::warning(this,tr("Server Manager - Error"),
			 tr("rsync(1) process returned an error, exit code")+
			 QString::asprintf(" %d",am_audio_process->exitCode()));
    am_audio_active=false;
    break;
  }
  am_audio_process->deleteLater();
  am_audio_process=NULL;
}


void MainWidget::showConnectionError(const QString &str)
{
  am_progress_dialog->hide();

  QMessageBox::warning(this,tr("Server Manager")+" - "+tr("Error"),
		       str+"\n\n"+
		       tr("See syslog for details."));
}


void MainWidget::closeEvent(QCloseEvent *e)
{
  if(am_audio_active) {
    if(QMessageBox::question(this,tr("Server Manager - Closing"),
			     tr("Audio replication will be halted if this program is exited.")+"\n"+
			     tr("Exit program?"),
			     QMessageBox::Yes,QMessageBox::No)==QMessageBox::Yes) {
      if((am_audio_process!=NULL)&&
	 am_audio_process->state()==QProcess::Running) {
	am_exiting=true;
	am_audio_process->terminate();
      }
      else {
	e->accept();
      }      
    }
    else {
      e->ignore();
    }
  }
  else {
    e->accept();
  }
}


void MainWidget::resizeEvent(QResizeEvent *e)
{
  //
  // Source Servers
  //
  am_source_label->setGeometry(10,2,size().width()-20,20);
  for(int i=0;i<2;i++) {
    am_src_system_label[i]->
      setGeometry(10+i*size().width()/2,20,size().width()/2-20,20);

    am_src_hostname_label[i]->setGeometry(10+i*size().width()/2,42,80,20);
    am_src_hostname_edit[i]->
      setGeometry(95+i*size().width()/2,42,size().width()/2-105,20);

    am_src_db_state_label[i]->setGeometry(10+i*size().width()/2,74,80,20);
    am_src_db_state_edit[i]->
      setGeometry(95+i*size().width()/2,74,size().width()/2-105,20);
    am_src_db_replicating_light[i]->setGeometry(20+i*size().width()/2,96,20,20);
    am_src_db_replicating_label[i]->
      setGeometry(45+i*size().width()/2,96,150,20);

    am_src_audio_state_label[i]->setGeometry(10+i*size().width()/2,123,80,20);
    am_src_audio_state_edit[i]->
      setGeometry(95+i*size().width()/2,123,size().width()/2-105,20);

    am_src_audio_replicating_light[i]->
      setGeometry(20+i*size().width()/2,145,20,20);
    am_src_audio_replicating_label[i]->
      setGeometry(45+i*size().width()/2,145,150,20);
  }

  //
  // Destination Server
  //
  am_destination_label->setGeometry(10,180,size().width()-20,20);
  am_dst_db_state_label->setGeometry(10,210,size().width()/2-15,20);
  am_dst_db_state_edit->
    setGeometry(size().width()/2,210,size().width()/2-105,20);
  am_dst_db_replicating_light->setGeometry(size().width()/2-70,232,20,20);
  am_dst_db_replicating_label->setGeometry(size().width()/2-45,232,150,20);
  am_db_slave_button->setGeometry(85,254,240,35);
  am_db_idle_button->setGeometry(85,294,240,35);

  am_dst_audio_state_label->setGeometry(10,344,size().width()/2-15,20);
  am_dst_audio_state_edit->
    setGeometry(size().width()/2,344,size().width()/2-105,20);
  am_dst_audio_replicating_light->setGeometry(size().width()/2-70,366,20,20);
  am_dst_audio_replicating_label->setGeometry(size().width()/2-45,366,150,20);
  am_audio_slave_button->setGeometry(85,388,240,35);
  am_audio_idle_button->setGeometry(85,428,240,35);
}


void MainWidget::paintEvent(QPaintEvent *e)
{
  QPainter *p=new QPainter(this);

  p->setPen(Qt::black);
  p->setBrush(Qt::black);

  //
  // Source Servers
  //
  DrawFrame(p,QRect(5,11,size().width()-5,160),am_source_label);

  //
  // Destination Server
  //
  DrawFrame(p,QRect(5,190,size().width()-5,300),am_destination_label);

  delete p;
}


void MainWidget::UpdateSourceStatus(int sys,AMStatus *s)
{
  am_src_hostname_edit[sys]->setText(s->hostname());
  am_src_db_state_edit[sys]->setText(AMState::stateString(s->dbState()));
  am_src_db_replicating_label[sys]->setEnabled(s->dbState()==AMState::StateSlave);
  am_src_db_replicating_light[sys]->setEnabled(s->dbState()==AMState::StateSlave);
  am_src_audio_replicating_label[sys]->
    setEnabled(s->audioState()==AMState::StateSlave);
  am_src_audio_replicating_light[sys]->
    setEnabled(s->audioState()==AMState::StateSlave);
}


void MainWidget::EnableSourceFields(int inst,bool state)
{
  if(inst==0) {
    for(int i=0;i<2;i++) {
      am_src_hostname_label[i]->setEnabled(state);
      am_src_hostname_edit[i]->setEnabled(state);
      am_src_db_replicating_label[i]->setEnabled(state);
      am_src_db_replicating_light[i]->setEnabled(state);
    }
  }
}


void MainWidget::DrawFrame(QPainter *p,const QRect &rect,QLabel *label)
{
  QFontMetrics *fm=new QFontMetrics(label->font());
  int topl=(rect.width()-fm->width(label->text()))/2;

  p->drawLine(rect.x(),rect.y(),topl,rect.y());
  p->drawLine(rect.x()+rect.width()-topl,rect.y(),
	      rect.width(),rect.y());

  p->drawLine(rect.width(),rect.y(),rect.width(),rect.y()+rect.height());
  p->drawLine(rect.x(),rect.y(),rect.x(),rect.y()+rect.height());
  p->drawLine(rect.x(),rect.y()+rect.height(),rect.width(),rect.y()+rect.height());

  delete fm;
}


int MainWidget::GetMasterServerId() const
{
  int sys=-1;

  for(int i=0;i<2;i++) {
    if(am_connection[i]->isConnected()) {
      if(am_connection[i]->status(i)->dbState()==AMState::StateMaster) {
	sys=i;
	break;
      }
    }
  }
  return sys;
}


QString MainWidget::MakeTempDir()
{
  char dirpath[PATH_MAX];

  strcpy(dirpath,"/tmp/amanrmtXXXXXX");
  if(mkdtemp(dirpath)==NULL) {
    QMessageBox::critical(this,"AmanRmt - "+tr("Error"),
			  tr("Unable to create temporary directory")+
			  " ["+strerror(errno)+"].");
    exit(1);
  }
  return QString(dirpath);
}


void MainWidget::SendAlert(const QString &msg) const
{
  QString err_msg;

  if(am_config->globalAlertAddress().isEmpty()) {
    return;
  }
  AMProfile *p=new AMProfile();
  p->setSource("/etc/amanrmt.conf");
  QString site_name=p->stringValue("Global","SiteName",tr("Satellite Site"));
  delete p;

  if(!AMSendMail(&err_msg,tr("Rivendell Server Alert")+" ["+site_name+"]",msg,
		 am_config->globalFromAddress(),
		 am_config->globalAlertAddress())) {
    syslog(LOG_WARNING,"mail send failed [%s]",err_msg.toUtf8().constData());
  }
}


void MainWidget::SetLightStatus(AMStatusLight *light,const QString &alert_msg,
				bool status)
{
  if(status!=light->status()) {
    light->setStatus(status);
    SendAlert(alert_msg);
  }
}


void MainWidget::SetClusterState(QLineEdit *edit,const QString &alert_msg,
				 AMState::ClusterState state)
{
  QString str=AMState::stateString(state);

  if(edit->text()!=str) {
    edit->setText(str);
    SendAlert(alert_msg);
  }
}


int main(int argc,char *argv[])
{
  QApplication a(argc,argv);
  MainWidget *m=new MainWidget();
  m->show();
  return a.exec();
}
