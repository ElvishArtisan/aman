// amconnection.cpp
//
// Client connection to an Aman monitor instance.
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

#include <unistd.h>

#include <QHostAddress>
#include <QStringList>
#include <QUrl>

#include "amconnection.h"

AMStatus::AMStatus()
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


QString AMStatus::hostname() const
{
  return stat_hostname;
}


void AMStatus::setHostname(const QString &str)
{
  stat_hostname=str;
}


bool AMStatus::serviceRunning() const
{
  return stat_service_running;
}


void AMStatus::setServiceRunning(bool state)
{
  stat_service_running=state;
}


bool AMStatus::dbRunning() const
{
  return stat_db_running;
}


void AMStatus::setDbRunning(bool state)
{
  stat_db_running=state;
}


bool AMStatus::dbAccessible() const
{
  return stat_db_accessible;
}


void AMStatus::setDbAccessible(bool state)
{
  stat_db_accessible=state;
}


AMState::ClusterState AMStatus::dbState() const
{
  return stat_db_state;
}


void AMStatus::setDbState(AMState::ClusterState state)
{
  stat_db_state=state;
}


int AMStatus::dbReplicationTime() const
{
  return stat_db_replication_time;
}


void AMStatus::setDbReplicationTime(int msecs)
{
  stat_db_replication_time=msecs;
}


AMState::ClusterState AMStatus::audioState() const
{
  return stat_audio_state;
}


void AMStatus::setAudioState(AMState::ClusterState state)
{
  stat_audio_state=state;
}


bool AMStatus::audioStatus() const
{
  return stat_audio_status;
}


void AMStatus::setAudioStatus(bool status)
{
  stat_audio_status=status;
}


bool AMStatus::isLocal() const
{
  return stat_is_local;
}


void AMStatus::setLocal(bool state)
{
  stat_is_local=state;
}




size_t __AMConnection_WriteCallback(char *ptr, size_t size, size_t nmemb, 
				    void *userdata)
{
  fwrite(ptr,size,nmemb,(FILE *)userdata);

  return size*nmemb;
}

AMConnection::AMConnection(QObject *parent)
  : QObject(parent)
{
  conn_hostname="";
  conn_port=0;
  char sname[256];

  //
  // AMStatus
  //
  for(unsigned i=0;i<2;i++) {
    conn_status[i]=new AMStatus();
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


AMConnection::~AMConnection()
{
  delete conn_watchdog_timer;
  delete conn_socket;
  for(unsigned i=0;i<2;i++) {
    delete conn_status[i];
  }
}


void AMConnection::connectToHost(const QString &hostname,uint16_t port)
{
  conn_hostname=hostname;
  conn_port=port;
  conn_socket->connectToHost(hostname,port);
}


bool AMConnection::isConnected() const
{
  return conn_socket->state()==QAbstractSocket::ConnectedState;
}


AMStatus *AMConnection::status(int sys)
{
  return conn_status[sys];
}


void AMConnection::generateSnapshot()
{
  printf("snapshot addr: %s\n",
	 conn_socket->peerAddress().toString().toUtf8().constData());
  conn_socket->write("GS!",3);
}


bool AMConnection::downloadSnapshot(const QString &name,const QString &ssh_id,
				    QString *err_msg)
{
  CURL *curl=NULL;
  FILE *f=NULL;
  long resp_code=0;
  QString filepath=QString(AM_SNAPSHOT_DIR)+"/"+name;
  QUrl url(QString("sftp://")+conn_hostname+filepath);

  if((f=fopen(filepath.toUtf8(),"w"))==NULL) {
    *err_msg=tr("Unable to download snapshot")+" ["+strerror(errno)+"]";
    return false;
  }

  if((curl=curl_easy_init())==NULL) {
    *err_msg=tr("Unable to initialize the cURL subsystem!");
    unlink(filepath.toUtf8());
    return false;
  }
  curl_easy_setopt(curl,CURLOPT_VERBOSE,1);
  curl_easy_setopt(curl,CURLOPT_USERNAME,"root");
  curl_easy_setopt(curl,CURLOPT_SSH_PRIVATE_KEYFILE,
		   ssh_id.toUtf8().constData());
  curl_easy_setopt(curl,CURLOPT_ERRORBUFFER,conn_curl_errorbuffer);

  curl_easy_setopt(curl,CURLOPT_WRITEFUNCTION,__AMConnection_WriteCallback);
  curl_easy_setopt(curl,CURLOPT_WRITEDATA,(void *)f);
  curl_easy_setopt(curl,CURLOPT_URL,url.toEncoded().constData());
  CURLcode code=curl_easy_perform(curl);
  if(code==CURLE_OK) {
    curl_easy_getinfo(curl,CURLINFO_RESPONSE_CODE,&resp_code);
    if(((resp_code<200)||(resp_code>=300))&&(resp_code!=0)) {
      *err_msg=
	QString::asprintf("Snapshot download failed, returned code %lu",
			  resp_code);
      fclose(f);
      unlink(filepath.toUtf8());
      curl_easy_cleanup(curl);
      return false;
    }
  }
  else {
    *err_msg=tr("Snapshot download failed")+" ["+conn_curl_errorbuffer+"]";
    fclose(f);
    unlink(filepath.toUtf8());
    curl_easy_cleanup(curl);
    return false;
  }

  fclose(f);
  curl_easy_cleanup(curl);
  *err_msg=tr("OK");
  return true;
}


void AMConnection::loadSnapshot(const QString &name)
{
  QString msg="LS "+name+"!";
  conn_socket->write(msg.toUtf8());
}


void AMConnection::makeDbMaster()
{
  conn_socket->write("MM!");
}


void AMConnection::makeDbSlave()
{
  conn_socket->write("MS!");
}


void AMConnection::makeDbIdle()
{
  conn_socket->write("MI!");
}


void AMConnection::startAudioSlave()
{
  conn_socket->write("AS!");
}


void AMConnection::stopAudioSlave()
{
  conn_socket->write("AI!");
}


void AMConnection::connectedData()
{
  conn_buffer="";
  conn_socket->write("ST!",3);
  emit connected();
}


void AMConnection::disconnectedData()
{
  conn_watchdog_timer->start(AMCONNECTION_RECONNECT_INTERVAL);
  emit disconnected();
}


void AMConnection::errorData(QAbstractSocket::SocketError err)
{
  switch(err) {
  case QAbstractSocket::ConnectionRefusedError:
  case QAbstractSocket::SocketTimeoutError:
    conn_watchdog_timer->stop();
    conn_watchdog_timer->start(AMCONNECTION_RECONNECT_INTERVAL);
    break;

  default:
    break;
  }
}


void AMConnection::readyReadData()
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


void AMConnection::watchdogData()
{
  conn_socket->connectToHost(conn_hostname,conn_port);
}


void AMConnection::ProcessMessage()
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
