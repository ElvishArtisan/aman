// aman.cpp
//
// aman(8) Monitoring Client.
//
//   (C) Copyright 2012-2013 Fred Gleason <fredg@paravelsystems.com>
//
//      $Id: aman.cpp,v 1.11 2013/07/02 22:18:57 cvs Exp $
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

#include <sys/types.h>
#include <unistd.h>

#include <QtCore/QSignalMapper>
#include <QtGui/QApplication>
#include <QtGui/QFileDialog>
#include <QtGui/QMessageBox>
#include <QtGui/QPainter>

#include "aman.h"

//
// Icons
//
#include "../icons/aman-22x22.xpm"

MainWidget::MainWidget(QWidget *parent)
  : QWidget(parent)
{
  am_connection_table[0]=0;
  am_connection_table[1]=1;

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
  // Fonts
  //
  QFont title_font(font().family(),font().pointSize(),QFont::Bold);
  QFont label_font(font().family(),font().pointSize()+2,QFont::Bold);

  //
  // Load Configuration
  //
  am_config=new Config("/etc/aman.conf");
  if(!am_config->load()) {
    QMessageBox::warning(this,tr("Server Monitor"),
	       tr("Unable to read configuration from \"/etc/aman.conf\"."));
    exit(256);
  }

  //
  // Local Status Connection
  //
  QSignalMapper *connected_mapper=new QSignalMapper(this);
  connect(connected_mapper,SIGNAL(mapped(int)),this,SLOT(connectedData(int)));
  QSignalMapper *disconnected_mapper=new QSignalMapper(this);
  connect(disconnected_mapper,SIGNAL(mapped(int)),
	  this,SLOT(disconnectedData(int)));
  for(int i=0;i<2;i++) {
    am_connection[i]=new Connection(this);
    connect(am_connection[i],SIGNAL(connected()),
	    connected_mapper,SLOT(map()));
    connected_mapper->setMapping(am_connection[i],i);
    connect(am_connection[i],SIGNAL(disconnected()),
	    disconnected_mapper,SLOT(map()));
    disconnected_mapper->setMapping(am_connection[i],i);
  }
  connect(am_connection[0],SIGNAL(statusChanged(Status *,Status *)),
	  this,SLOT(statusChangedData(Status *,Status *)));
  connect(am_connection[0],SIGNAL(snapshotGenerated(const QString &)),
	  this,SLOT(snapshotGeneratedData(const QString &)));
  connect(am_connection[0],SIGNAL(snapshotLoaded(const QString &)),
	  this,SLOT(snapshotLoadedData(const QString &)));

  //
  // Progress Dialog
  //
  am_progress_dialog=new QProgressDialog(tr("Processing..."),"",-1,-1,this);
  am_progress_dialog->setWindowTitle(tr("Server Manager"));
  am_progress_dialog->setWindowModality(Qt::WindowModal);
  am_progress_dialog->setMinimumDuration(2000);
  am_progress_dialog->setCancelButton(NULL);

  //
  // Controls
  //
  am_database_label=new QLabel(tr("Database"),this);
  am_database_label->setFont(label_font);
  am_database_label->setAlignment(Qt::AlignCenter);

  am_audio_label=new QLabel(tr("Audio Store"),this);
  am_audio_label->setFont(label_font);
  am_audio_label->setAlignment(Qt::AlignCenter);

  QSignalMapper *db_master_mapper=new QSignalMapper(this);
  connect(db_master_mapper,SIGNAL(mapped(int)),
	  this,SLOT(makeDbMasterData(int)));
  QSignalMapper *db_slave_mapper=new QSignalMapper(this);
  connect(db_slave_mapper,SIGNAL(mapped(int)),this,SLOT(makeDbSlaveData(int)));
  QSignalMapper *db_idle_mapper=new QSignalMapper(this);
  connect(db_idle_mapper,SIGNAL(mapped(int)),this,SLOT(makeDbIdleData(int)));
  QSignalMapper *audio_slave_mapper=new QSignalMapper(this);
  connect(audio_slave_mapper,SIGNAL(mapped(int)),
	  this,SLOT(startAudioData(int)));
  QSignalMapper *audio_idle_mapper=new QSignalMapper(this);
  connect(audio_idle_mapper,SIGNAL(mapped(int)),this,SLOT(stopAudioData(int)));
  for(int i=0;i<2;i++) {
    am_system_label[i]=new QLabel(this);
    am_system_label[i]->setAlignment(Qt::AlignCenter);
    am_system_label[i]->setFont(title_font);

    am_hostname_label[i]=new QLabel(tr("Hostname:"),this);
    am_hostname_label[i]->setAlignment(Qt::AlignVCenter|Qt::AlignRight);
    am_hostname_edit[i]=new QLineEdit(this);
    am_hostname_edit[i]->setReadOnly(true);
    am_hostname_edit[i]->setText(tr("[unknown]"));

    am_db_state_label[i]=new QLabel(tr("State:"),this);
    am_db_state_label[i]->setAlignment(Qt::AlignVCenter|Qt::AlignRight);
    am_db_state_edit[i]=new QLineEdit(this);
    am_db_state_edit[i]->setReadOnly(true);
    am_db_state_edit[i]->setText(tr("UNKNOWN"));

    am_service_running_label[i]=new QLabel(tr("Service Running"),this);
    am_service_running_label[i]->setAlignment(Qt::AlignVCenter|Qt::AlignLeft);
    am_service_running_light[i]=new StatusLight(this);

    am_db_running_label[i]=new QLabel(tr("DB Server Running"),this);
    am_db_running_label[i]->setAlignment(Qt::AlignVCenter|Qt::AlignLeft);
    am_db_running_light[i]=new StatusLight(this);

    am_db_accessible_label[i]=new QLabel(tr("DB Accessible"),this);
    am_db_accessible_label[i]->setAlignment(Qt::AlignVCenter|Qt::AlignLeft);
    am_db_accessible_light[i]=new StatusLight(this);

    am_db_replicating_label[i]=new QLabel(tr("DB Replicating"),this);
    am_db_replicating_label[i]->setAlignment(Qt::AlignVCenter|Qt::AlignLeft);
    am_db_replicating_light[i]=new StatusLight(this);

    am_db_master_button[i]=new QPushButton(tr("Make Master"),this);
    am_db_master_button[i]->setFont(title_font);
    connect(am_db_master_button[i],SIGNAL(clicked()),
	    db_master_mapper,SLOT(map()));
    db_master_mapper->setMapping(am_db_master_button[i],i);

    am_db_slave_button[i]=new QPushButton(tr("Make Slave"),this);
    am_db_slave_button[i]->setFont(title_font);
    connect(am_db_slave_button[i],SIGNAL(clicked()),
	    db_slave_mapper,SLOT(map()));
    db_slave_mapper->setMapping(am_db_slave_button[i],i);

    am_db_idle_button[i]=new QPushButton(tr("Make Idle"),this);
    am_db_idle_button[i]->setFont(title_font);
    connect(am_db_idle_button[i],SIGNAL(clicked()),
	    db_idle_mapper,SLOT(map()));
    db_idle_mapper->setMapping(am_db_idle_button[i],i);

    am_audio_state_label[i]=new QLabel(tr("State:"),this);
    am_audio_state_label[i]->setAlignment(Qt::AlignVCenter|Qt::AlignRight);
    am_audio_state_edit[i]=new QLineEdit(this);
    am_audio_state_edit[i]->setReadOnly(true);
    am_audio_state_edit[i]->setText(tr("UNKNOWN"));

    am_audio_replicating_label[i]=new QLabel(tr("Audio Replicating"),this);
    am_audio_replicating_label[i]->setAlignment(Qt::AlignVCenter|Qt::AlignLeft);
    am_audio_replicating_light[i]=new StatusLight(this);

    am_audio_slave_button[i]=new QPushButton(tr("Start Slave"),this);
    am_audio_slave_button[i]->setFont(title_font);
    connect(am_audio_slave_button[i],SIGNAL(clicked()),
	    audio_slave_mapper,SLOT(map()));
    audio_slave_mapper->setMapping(am_audio_slave_button[i],i);

    am_audio_idle_button[i]=new QPushButton(tr("Stop Slave"),this);
    am_audio_idle_button[i]->setFont(title_font);
    connect(am_audio_idle_button[i],SIGNAL(clicked()),
	    audio_idle_mapper,SLOT(map()));
    audio_idle_mapper->setMapping(am_audio_idle_button[i],i);
  }
  am_system_label[0]->setText(tr("System A"));
  am_system_label[1]->setText(tr("System B"));

  am_connection[0]->connectToHost("localhost",AM_CMD_TCP_PORT);
  am_connection[1]->connectToHost(am_config->
		  address(Am::That,Config::PrivateAddress).toString(),
		  AM_CMD_TCP_PORT);

  EnableFields(0,false);
  EnableFields(1,false);
}


QSize MainWidget::sizeHint() const
{
  return QSize(400,500);
}


void MainWidget::connectedData(int inst)
{
  EnableFields(inst,true);
}


void MainWidget::disconnectedData(int inst)
{
  EnableFields(inst,false);
}


void MainWidget::makeDbMasterData(int inst)
{
  am_connection[am_connection_table[inst]]->makeDbMaster();
  am_progress_dialog->setValue(0);
}


void MainWidget::makeDbSlaveData(int inst)
{
  am_connection[am_connection_table[inst]]->makeDbSlave();
  am_progress_dialog->setValue(0);
}


void MainWidget::makeDbIdleData(int inst)
{
  am_connection[am_connection_table[inst]]->makeDbIdle();
  am_progress_dialog->setValue(0);
}


void MainWidget::startAudioData(int inst)
{
  am_connection[am_connection_table[inst]]->startAudioSlave();
}


void MainWidget::stopAudioData(int inst)
{
  am_connection[am_connection_table[inst]]->stopAudioSlave();
}


void MainWidget::statusChangedData(Status *a,Status *b)
{
  am_progress_dialog->reset();

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
  UpdateStatus(0,a);
  am_db_replicating_light[0]->setStatus(b->dbReplicationTime()>0);
  am_db_replicating_light[1]->setStatus(a->dbReplicationTime()>0);
  switch(a->dbState()) {
  case State::StateOffline:
    am_db_master_button[0]->setEnabled(false);
    am_db_slave_button[0]->setEnabled(false);
    am_db_idle_button[0]->setEnabled(false);
    break;

  case State::StateIdle:
    am_db_master_button[0]->setEnabled(true);
    am_db_slave_button[0]->setEnabled(b->dbState()==State::StateMaster);
    am_db_idle_button[0]->setEnabled(false);
    break;

  case State::StateMaster:
    am_db_master_button[0]->setEnabled(false);
    am_db_slave_button[0]->setEnabled(false);
    am_db_idle_button[0]->setEnabled(true);
    break;

  case State::StateSlave:
    am_db_master_button[0]->setEnabled(false);
    am_db_slave_button[0]->setEnabled(false);
    am_db_idle_button[0]->setEnabled(true);
    break;
  }
  am_audio_replicating_light[0]->setEnabled(a->audioState()==State::StateSlave);
  am_audio_replicating_light[0]->setStatus(a->audioStatus());
  am_audio_replicating_light[1]->setEnabled(b->audioState()==State::StateSlave);
  am_audio_replicating_light[1]->setStatus(b->audioStatus());
  switch(a->audioState()) {
  case State::StateSlave:
    am_audio_state_edit[0]->setText(State::stateString(State::StateSlave));
    am_audio_slave_button[0]->setEnabled(false);
    am_audio_idle_button[0]->setEnabled(true);
    break;

  case State::StateIdle:
    if(b->audioState()==State::StateSlave) {
      am_audio_state_edit[0]->setText(State::stateString(State::StateMaster));
    }
    else {
      am_audio_state_edit[0]->setText(State::stateString(State::StateIdle));
    }
    am_audio_slave_button[0]->setEnabled(b->audioState()==State::StateIdle);
    am_audio_idle_button[0]->setEnabled(false);
    break;

  case State::StateMaster:
  case State::StateOffline:
    am_audio_state_edit[0]->setText(State::stateString(a->audioState()));
    am_audio_slave_button[0]->setEnabled(false);
    am_audio_idle_button[0]->setEnabled(false);
    break;
  }

  //
  // Update System B Status
  //
  UpdateStatus(1,b);
  switch(b->dbState()) {
  case State::StateOffline:
    am_db_master_button[1]->setEnabled(false);
    am_db_slave_button[1]->setEnabled(false);
    am_db_idle_button[1]->setEnabled(false);
    break;

  case State::StateIdle:
    am_db_master_button[1]->setEnabled(true);
    am_db_slave_button[1]->setEnabled(a->dbState()==State::StateMaster);
    am_db_idle_button[1]->setEnabled(false);
    break;

  case State::StateMaster:
    am_db_master_button[1]->setEnabled(false);
    am_db_slave_button[1]->setEnabled(false);
    am_db_idle_button[1]->setEnabled(true);
    break;

  case State::StateSlave:
    am_db_master_button[1]->setEnabled(false);
    am_db_slave_button[1]->setEnabled(false);
    am_db_idle_button[1]->setEnabled(true);
    break;
  }
  switch(b->audioState()) {
  case State::StateSlave:
    am_audio_state_edit[1]->setText(State::stateString(State::StateSlave));
    am_audio_slave_button[1]->setEnabled(false);
    am_audio_idle_button[1]->setEnabled(true);
    break;

  case State::StateIdle:
    if(b->audioState()==State::StateSlave) {
      am_audio_state_edit[1]->setText(State::stateString(State::StateMaster));
    }
    else {
      am_audio_state_edit[1]->setText(State::stateString(State::StateIdle));
    }
    am_audio_slave_button[1]->setEnabled(a->audioState()==State::StateIdle);
    am_audio_idle_button[1]->setEnabled(false);
    break;

  case State::StateMaster:
  case State::StateOffline:
    am_audio_state_edit[1]->setText(State::stateString(a->audioState()));
    am_audio_slave_button[1]->setEnabled(false);
    am_audio_idle_button[1]->setEnabled(false);
    break;
  }
}


void MainWidget::snapshotGeneratedData(const QString &name)
{
  QMessageBox::information(this,tr("Server Maanger - Snapshot Generated"),
			   tr("Generated snapshot")+":\n \""+name+"\".");
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


void MainWidget::resizeEvent(QResizeEvent *e)
{
  am_database_label->setGeometry(10,89,size().width()-20,20);
  am_audio_label->setGeometry(10,335,size().width()-20,20);

  for(int i=0;i<2;i++) {
    am_system_label[i]->
      setGeometry(10+i*size().width()/2,15,size().width()/2-20,20);

    am_hostname_label[i]->
      setGeometry(10+i*size().width()/2,37,80,20);
    am_hostname_edit[i]->
      setGeometry(95+i*size().width()/2,37,size().width()/2-105,20);

    am_service_running_light[i]->
      setGeometry(20+i*size().width()/2,59,20,20);
    am_service_running_label[i]->
      setGeometry(45+i*size().width()/2,59,150,20);

    am_db_state_label[i]->
      setGeometry(10+i*size().width()/2,111,80,20);
    am_db_state_edit[i]->
      setGeometry(95+i*size().width()/2,111,size().width()/2-105,20);

    am_db_running_light[i]->
      setGeometry(20+i*size().width()/2,133,20,20);
    am_db_running_label[i]->
      setGeometry(45+i*size().width()/2,133,150,20);

    am_db_accessible_light[i]->
      setGeometry(20+i*size().width()/2,155,20,20);
    am_db_accessible_label[i]->
      setGeometry(45+i*size().width()/2,155,150,20);

    am_db_replicating_light[i]->
      setGeometry(20+i*size().width()/2,177,20,20);
    am_db_replicating_label[i]->
      setGeometry(45+i*size().width()/2,177,150,20);

    am_db_master_button[i]->setGeometry(20+i*size().width()/2,202,170,35);
    am_db_slave_button[i]->setGeometry(20+i*size().width()/2,242,170,35);
    am_db_idle_button[i]->setGeometry(20+i*size().width()/2,282,170,35);

    am_audio_state_label[i]->
      setGeometry(10+i*size().width()/2,365,80,20);
    am_audio_state_edit[i]->
      setGeometry(95+i*size().width()/2,365,size().width()/2-105,20);

    am_audio_replicating_light[i]->
      setGeometry(20+i*size().width()/2,387,20,20);
    am_audio_replicating_label[i]->
      setGeometry(45+i*size().width()/2,387,150,20);

    am_audio_slave_button[i]->setGeometry(20+i*size().width()/2,415,170,35);
    am_audio_idle_button[i]->setGeometry(20+i*size().width()/2,455,170,35);
  }
}


void MainWidget::paintEvent(QPaintEvent *e)
{
  QPainter *p=new QPainter(this);

  p->setPen(Qt::black);
  p->setBrush(Qt::black);

  //
  // Database
  //
  /*
  p->drawLine(5,77,150,77);
  p->drawLine(size().width()-150,77,size().width()-5,77);
  p->drawLine(size().width()-5,77,size().width()-5,320);
  p->drawLine(5,77,5,320);
  p->drawLine(5,320,size().width()-5,320);
  */
  p->drawLine(5,99,150,99);
  p->drawLine(size().width()-150,99,size().width()-5,99);
  p->drawLine(size().width()-5,99,size().width()-5,320);
  p->drawLine(5,99,5,320);
  p->drawLine(5,320,size().width()-5,320);

  //
  // Audio Store
  //
  p->drawLine(5,345,135,345);
  p->drawLine(size().width()-135,345,size().width()-5,345);

  p->drawLine(size().width()-5,345,size().width()-5,495);
  p->drawLine(5,345,5,495);
  p->drawLine(5,495,size().width()-5,495);

  delete p;
}


void MainWidget::UpdateStatus(int sys,Status *s)
{
  am_hostname_edit[sys]->setText(s->hostname());
  am_db_state_edit[sys]->setText(State::stateString(s->dbState()));
  am_service_running_light[sys]->setStatus(s->serviceRunning());
  am_db_running_light[sys]->setStatus(s->dbRunning());
  am_db_accessible_light[sys]->setStatus(s->dbAccessible());
  am_db_replicating_label[sys]->setEnabled(s->dbState()==State::StateSlave);
  am_db_replicating_light[sys]->setEnabled(s->dbState()==State::StateSlave);
  am_audio_replicating_label[sys]->
    setEnabled(s->audioState()==State::StateSlave);
  am_audio_replicating_light[sys]->
    setEnabled(s->audioState()==State::StateSlave);
}


void MainWidget::EnableFields(int inst,bool state)
{
  if(inst==0) {
    for(int i=0;i<2;i++) {
      am_hostname_label[i]->setEnabled(state);
      am_hostname_edit[i]->setEnabled(state);
      am_service_running_label[i]->setEnabled(state);
      am_service_running_light[i]->setEnabled(state);
      am_db_running_label[i]->setEnabled(state);
      am_db_running_light[i]->setEnabled(state);
      am_db_accessible_label[i]->setEnabled(state);
      am_db_accessible_light[i]->setEnabled(state);
      am_db_replicating_label[i]->setEnabled(state);
      am_db_replicating_light[i]->setEnabled(state);
    }
  }
}


int main(int argc,char *argv[])
{
  QApplication a(argc,argv);
  MainWidget *m=new MainWidget();
  m->show();
  return a.exec();
}
