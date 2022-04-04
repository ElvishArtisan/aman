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

#include <sys/types.h>
#include <unistd.h>

#include <QApplication>
#include <QFileDialog>
#include <QFontMetrics>
#include <QMessageBox>
#include <QPainter>
#include <QSignalMapper>

#include "amanrmt.h"

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
  am_config=new AMConfig("/etc/aman.conf",true);
  if(!am_config->load()) {
    QMessageBox::warning(this,tr("Server Monitor"),
	       tr("Unable to read configuration from \"/etc/aman.conf\"."));
    exit(256);
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
  connect(am_connection[0],SIGNAL(statusChanged(AMStatus *,AMStatus *)),
	  this,SLOT(statusChangedData(AMStatus *,AMStatus *)));
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
  am_dst_db_state_edit->setText(tr("UNKNOWN"));

  am_dst_db_replicating_label=new QLabel(tr("DB Replicating"),this);
  am_dst_db_replicating_label->setAlignment(Qt::AlignVCenter|Qt::AlignLeft);
  am_dst_db_replicating_light=new AMStatusLight(this);

  am_dst_audio_state_label=new QLabel(tr("Audio State:"),this);
  am_dst_audio_state_label->setAlignment(Qt::AlignVCenter|Qt::AlignRight);
  am_dst_audio_state_edit=new QLineEdit(this);
  am_dst_audio_state_edit->setReadOnly(true);
  am_dst_audio_state_edit->setText(tr("UNKNOWN"));

  am_dst_audio_replicating_label=new QLabel(tr("Audio Replicating"),this);
  am_dst_audio_replicating_label->setAlignment(Qt::AlignVCenter|Qt::AlignLeft);
  am_dst_audio_replicating_light=new AMStatusLight(this);

  for(int i=0;i<2;i++) {
    am_db_slave_button[i]=new QPushButton(tr("Make Slave"),this);
    am_db_slave_button[i]->setFont(title_font);
    //    connect(am_db_slave_button[i],SIGNAL(clicked()),
    //	    db_slave_mapper,SLOT(map()));
    am_db_slave_button[i]->setDisabled(true);

    am_audio_slave_button[i]=new QPushButton(tr("Start Slave"),this);
    am_audio_slave_button[i]->setFont(title_font);
    //    connect(am_audio_slave_button[i],SIGNAL(clicked()),
    //	    audio_slave_mapper,SLOT(map()));
    am_audio_slave_button[i]->setDisabled(true);
  }
  am_db_idle_button=new QPushButton(tr("Make Idle"),this);
  am_db_idle_button->setFont(title_font);
  //    connect(am_db_idle_button,SIGNAL(clicked()),
  //	    db_idle_mapper,SLOT(map()));
  am_db_idle_button->setDisabled(true);

  am_audio_idle_button=new QPushButton(tr("Make Idle"),this);
  am_audio_idle_button->setFont(title_font);
  //    connect(am_audio_idle_button,SIGNAL(clicked()),
  //	    audio_idle_mapper,SLOT(map()));
  am_audio_idle_button->setDisabled(true);

  //
  // Start Source Server Connections
  //
  am_connection[0]->
    connectToHost(am_config->address(0,AMConfig::PrivateAddress).toString(),
		  AM_CMD_TCP_PORT);
  am_connection[1]->
    connectToHost(am_config->address(1,AMConfig::PrivateAddress).toString(),
		  AM_CMD_TCP_PORT);

  EnableSourceFields(0,false);
  EnableSourceFields(1,false);
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


void MainWidget::makeDbSlaveData(int inst)
{
  //  am_connection[am_connection_table[inst]]->makeDbSlave();
  //  am_progress_dialog->setValue(0);
}


void MainWidget::makeDbIdleData(int inst)
{
  //  am_connection[am_connection_table[inst]]->makeDbIdle();
  //  am_progress_dialog->setValue(0);
}


void MainWidget::startAudioData(int inst)
{
  //  am_connection[am_connection_table[inst]]->startAudioSlave();
}


void MainWidget::stopAudioData(int inst)
{
  //  am_connection[am_connection_table[inst]]->stopAudioSlave();
}


void MainWidget::statusChangedData(AMStatus *a,AMStatus *b)
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
  UpdateSourceStatus(0,a);
  am_src_db_replicating_light[0]->setStatus(b->dbReplicationTime()>0);
  am_src_db_replicating_light[1]->setStatus(a->dbReplicationTime()>0);
  am_src_audio_replicating_light[0]->setEnabled(a->audioState()==AMState::StateSlave);
  am_src_audio_replicating_light[0]->setStatus(a->audioStatus());
  am_src_audio_replicating_light[1]->setEnabled(b->audioState()==AMState::StateSlave);
  am_src_audio_replicating_light[1]->setStatus(b->audioStatus());
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
  for(int i=0;i<2;i++) {
    am_db_slave_button[i]->setGeometry(20+i*size().width()/2,254,170,35);
  }
  am_db_idle_button->setGeometry(85,294,240,35);

  am_dst_audio_state_label->setGeometry(10,344,size().width()/2-15,20);
  am_dst_audio_state_edit->
    setGeometry(size().width()/2,344,size().width()/2-105,20);
  am_dst_audio_replicating_light->setGeometry(size().width()/2-70,366,20,20);
  am_dst_audio_replicating_label->setGeometry(size().width()/2-45,366,150,20);
  for(int i=0;i<2;i++) {
    am_audio_slave_button[i]->setGeometry(20+i*size().width()/2,388,170,35);
  }
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


int main(int argc,char *argv[])
{
  QApplication a(argc,argv);
  MainWidget *m=new MainWidget();
  m->show();
  return a.exec();
}
