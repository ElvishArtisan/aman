// connection.cpp
//
// Client connection to an Aman monitor instance.
//
//   (C) Copyright 2012-2019 Fred Gleason <fredg@paravelsystems.com>
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

#include <unistd.h>

#include <QStringList>

#include "connection.h"

Status::Status()
{
  stat_hostname="";
  stat_service_running=false;
  stat_db_running=false;
  stat_db_accessible=false;
  stat_db_state=AMState::StateIdle;
  stat_audio_state=AMState::StateIdle;
  stat_audio_status=false;
  stat_db_replication_time=0;
  stat_is_local=false;
}


QString Status::hostname() const
{
  return stat_hostname;
}


void Status::setHostname(const QString &str)
{
  stat_hostname=str;
}


bool Status::serviceRunning() const
{
  return stat_service_running;
}


void Status::setServiceRunning(bool state)
{
  stat_service_running=state;
}


bool Status::dbRunning() const
{
  return stat_db_running;
}


void Status::setDbRunning(bool state)
{
  stat_db_running=state;
}


bool Status::dbAccessible() const
{
  return stat_db_accessible;
}


void Status::setDbAccessible(bool state)
{
  stat_db_accessible=state;
}


AMState::ClusterState Status::dbState() const
{
  return stat_db_state;
}


void Status::setDbState(AMState::ClusterState state)
{
  stat_db_state=state;
}


int Status::dbReplicationTime() const
{
  return stat_db_replication_time;
}


void Status::setDbReplicationTime(int msecs)
{
  stat_db_replication_time=msecs;
}


AMState::ClusterState Status::audioState() const
{
  return stat_audio_state;
}


void Status::setAudioState(AMState::ClusterState state)
{
  stat_audio_state=state;
}


bool Status::audioStatus() const
{
  return stat_audio_status;
}


void Status::setAudioStatus(bool status)
{
  stat_audio_status=status;
}


bool Status::isLocal() const
{
  return stat_is_local;
}


void Status::setLocal(bool state)
{
  stat_is_local=state;
}


Connection::Connection(QObject *parent)
  : QObject(parent)
{
  conn_hostname="";
  conn_port=0;
  char sname[256];

  //
  // Status
  //
  for(unsigned i=0;i<2;i++) {
    conn_status[i]=new Status();
  }

  //
  // TCP Socket
  //
  conn_socket=new QTcpSocket(this);
  connect(conn_socket,SIGNAL(connected()),this,SLOT(connectedData()));
  connect(conn_socket,SIGNAL(disconnected()),this,SLOT(disconnectedData()));
  connect(conn_socket,SIGNAL(readyRead()),this,SLOT(readyReadData()));
  connect(conn_socket,SIGNAL(error(QAbstractSocket::SocketError)),
	  this,SLOT(errorData(QAbstractSocket::SocketError)));

  //
  // Watchdog Timer
  //
  conn_watchdog_timer=new QTimer(this);
  conn_watchdog_timer->setSingleShot(true);
  connect(conn_watchdog_timer,SIGNAL(timeout()),this,SLOT(watchdogData()));

  //
  // Local System Name
  //
  gethostname(sname,255);
  QStringList list=QString(sname).split(".");  // Strip domain name parts
  conn_localname=list[0];
}


Connection::~Connection()
{
  delete conn_watchdog_timer;
  delete conn_socket;
  for(unsigned i=0;i<2;i++) {
    delete conn_status[i];
  }
}


void Connection::connectToHost(const QString &hostname,uint16_t port)
{
  conn_hostname=hostname;
  conn_port=port;
  conn_socket->connectToHost(hostname,port);
}


Status *Connection::status(int sys)
{
  return conn_status[sys];
}


void Connection::generateSnapshot()
{
  conn_socket->write("GS!",3);
}


void Connection::loadSnapshot(const QString &name)
{
  QString msg="LS "+name+"!";
  conn_socket->write(msg.toUtf8());
}


void Connection::makeDbMaster()
{
  conn_socket->write("MM!");
}


void Connection::makeDbSlave()
{
  conn_socket->write("MS!");
}


void Connection::makeDbIdle()
{
  conn_socket->write("MI!");
}


void Connection::startAudioSlave()
{
  conn_socket->write("AS!");
}


void Connection::stopAudioSlave()
{
  conn_socket->write("AI!");
}


void Connection::connectedData()
{
  conn_buffer="";
  conn_socket->write("ST!",3);
  emit connected();
}


void Connection::disconnectedData()
{
  conn_watchdog_timer->start(CONNECTION_RECONNECT_INTERVAL);
  emit disconnected();
}


void Connection::errorData(QAbstractSocket::SocketError err)
{
  switch(err) {
  case QAbstractSocket::ConnectionRefusedError:
  case QAbstractSocket::SocketTimeoutError:
    conn_watchdog_timer->stop();
    conn_watchdog_timer->start(CONNECTION_RECONNECT_INTERVAL);
    break;

  default:
    break;
  }
}


void Connection::readyReadData()
{
  int n=0;
  char data[1500];

  while((n=conn_socket->read(data,1500))>0) {
    for(int i=0;i<n;i++) {
      if(data[i]=='!') {
	ProcessMessage();
	conn_buffer="";
      }
      else {
	conn_buffer+=data[i];
      }
    }
  }
}


void Connection::watchdogData()
{
  conn_socket->connectToHost(conn_hostname,conn_port);
}


void Connection::ProcessMessage()
{
  QStringList fields=conn_buffer.split(" ");

  if(fields[0]=="ST") {
    if(fields.size()==17) {
      for(int i=0;i<2;i++) {
	conn_status[i]->setLocal(conn_localname==fields[1+i*8]);
	conn_status[i]->setHostname(fields[1+i*8]);
	conn_status[i]->setServiceRunning(fields[2+i*8]=="1");
	conn_status[i]->setDbRunning(fields[3+i*8]=="1");
	conn_status[i]->setDbAccessible(fields[4+i*8]=="1");
	conn_status[i]->setDbState((AMState::ClusterState)fields[5+i*8].toInt());
	conn_status[i]->setDbReplicationTime(fields[6+i*8].toInt());
	conn_status[i]->
	  setAudioState((AMState::ClusterState)fields[7+i*8].toInt());
	conn_status[i]->
	  setAudioStatus(fields[8+i*8].toInt());
      }
    }
    emit statusChanged(conn_status[0],conn_status[1]);
    return;
  }

  if(fields[0]=="GS") {
    if(fields.size()==2) {
      emit snapshotGenerated(fields[1]);
    }
    return;
  }

  if(fields[0]=="LS") {
    if(fields.size()==3) {
      emit snapshotLoaded(fields[1]);
    }
    return;
  }
  /*
  fprintf(stderr,"unknown/malformated control message received [%s]\n",
	  conn_buffer.toUtf8().constData());
  */
}
