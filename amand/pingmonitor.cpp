// pingmonitor.cpp
//
// Monitor the Opposite Aman Instance
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

#include <stdint.h>
#include <syslog.h>

#include <QStringList>

#include "pingmonitor.h"

PingMonitor::PingMonitor(AMConfig *config,QObject *parent)
  : QObject(parent)
{
  ping_config=config;
  for(int i=0;i<Am::LastInstance;i++) {
    ping_mysql_running[i]=false;
    ping_mysql_accessible[i]=false;
    ping_db_state[i]=AMState::StateOffline;
    ping_audio_state[i]=AMState::StateIdle;
    ping_snapshot_name[i]="";
    ping_mysql_replication_time[i]=0;
    ping_audio_state[i]=AMState::StateIdle;
    ping_audio_status[i]=false;
  }
  ping_reachable[0]=true;
  ping_reachable[1]=true;

  //
  // Sockets
  //
  ping_ready_mapper=new QSignalMapper(this);
  connect(ping_ready_mapper,SIGNAL(mapped(int)),
	  this,SLOT(socketReadyData(int)));
  for(int i=0;i<2;i++) {
    ping_sockets[i]=new QUdpSocket(this);
    ping_ready_mapper->setMapping(ping_sockets[i],i);
    connect(ping_sockets[i],SIGNAL(readyRead()),ping_ready_mapper,SLOT(map()));
  }

  //
  // Timers
  //
  ping_send_timer=new QTimer(this);
  connect(ping_send_timer,SIGNAL(timeout()),this,SLOT(socketSendData()));

  ping_watchdog_mapper=new QSignalMapper(this);
  connect(ping_watchdog_mapper,SIGNAL(mapped(int)),
	  this,SLOT(socketTimeoutData(int)));
  for(int i=0;i<2;i++) {
    ping_watchdog_timer[i]=new QTimer(this);
    ping_watchdog_timer[i]->setSingleShot(true);
    ping_watchdog_mapper->setMapping(ping_watchdog_timer[i],i);
    connect(ping_watchdog_timer[i],SIGNAL(timeout()),
	    ping_watchdog_mapper,SLOT(map()));
  }
}


PingMonitor::~PingMonitor()
{
  for(int i=0;i<2;i++) {
    delete ping_watchdog_timer[i];
    delete ping_sockets[i];
  }
  delete ping_send_timer;
  delete ping_ready_mapper;
  delete ping_watchdog_mapper;
  delete ping_watchdog_mapper;
  delete ping_ready_mapper;
}


bool PingMonitor::start()
{
  bool ret=true;

  //
  // Bind Sockets
  //
  if(!ping_sockets[0]->
     bind(ping_config->address(Am::This,AMConfig::PublicAddress),
	  AM_PING_UDP_PORT)) {
    syslog(LOG_ERR,"unable to bind %s:%d",
	   ping_config->address(Am::This,AMConfig::PublicAddress).
	   toString().toUtf8().constData(),AM_PING_UDP_PORT);
    ret=false;
  }
  if(!ping_sockets[1]->
     bind(ping_config->address(Am::This,AMConfig::PrivateAddress),
	  AM_PING_UDP_PORT)) {
    syslog(LOG_ERR,"unable to bind %s:%d",
	   ping_config->address(Am::This,AMConfig::PrivateAddress).
	   toString().toUtf8().constData(),AM_PING_UDP_PORT);
    ret=false;
  }

  //
  // Start Timers
  //
  ping_send_timer->start(AM_PING_INTERVAL);
  for(int i=0;i<2;i++) {
    ping_watchdog_timer[i]->start(AM_PING_TIMEOUT);
  }

  return ret;
}


bool PingMonitor::isReachable(Am::Instance inst) const
{
  if(inst==Am::That) {
    return ping_reachable[0]||ping_reachable[1];
  }
  return true;
}


bool PingMonitor::mysqlRunning(Am::Instance inst) const
{
  return ping_mysql_running[inst];
}


bool PingMonitor::mysqlAccessible(Am::Instance inst) const
{
  return ping_mysql_accessible[inst];
}


AMState::ClusterState PingMonitor::dbState(Am::Instance inst) const
{
  AMState::ClusterState ret=AMState::StateOffline;
  if(isReachable(inst)&&mysqlRunning(inst)&&mysqlAccessible(inst)) {
    return ping_db_state[inst];
  }
  return ret;
}


AMState::ClusterState PingMonitor::audioState(Am::Instance inst) const
{
  return ping_audio_state[inst];
}


bool PingMonitor::audioStatus(Am::Instance inst)
{
  return ping_audio_status[inst];
}


QString PingMonitor::snapshotName(Am::Instance inst) const
{
  return ping_snapshot_name[inst];
}


int PingMonitor::mysqlReplicationTime(Am::Instance inst) const
{
  return ping_mysql_replication_time[inst];
}


void PingMonitor::setThisMysqlState(bool running,bool accessible)
{
  ping_mysql_running[Am::This]=running;
  ping_mysql_accessible[Am::This]=accessible;
}


void PingMonitor::setThisDbState(AMState::ClusterState state)
{
  ping_db_state[Am::This]=state;
}


void PingMonitor::setThisAudioState(AMState::ClusterState state)
{
  ping_audio_state[Am::This]=state;
}


void PingMonitor::setThisAudioStatus(bool status)
{
  ping_audio_status[Am::This]=status;
}


void PingMonitor::setThisSnapshotName(const QString &str)
{
  ping_snapshot_name[Am::This]=str;
}


void PingMonitor::setThisMysqlReplicationTime(int msecs)
{
  ping_mysql_replication_time[Am::This]=msecs;
}


void PingMonitor::socketReadyData(int id)
{
  int n;
  char data[1500];
  QStringList fields;
  bool running;
  bool accessible;
  AMState::ClusterState db_state;
  QString snapshot;
  bool replication_time;
  AMState::ClusterState audio_state;
  bool audio_status;

  while((n=ping_sockets[id]->readDatagram(data,1500))>0) {
    data[n]=0;
    fields=QString(data).split(" ");
    if(fields[0]=="ping") {
      ping_reachable[id]=true;
      if(fields.size()==8) {
	running=fields[1].toInt();
	accessible=fields[2].toInt();
	db_state=(AMState::ClusterState)fields[3].toInt();
	snapshot=fields[4];
	replication_time=fields[5].toInt();
	audio_state=(AMState::ClusterState)fields[6].toInt();
	audio_status=fields[7].toInt();
	if((running!=ping_mysql_running[Am::That])||
	   (accessible!=ping_mysql_accessible[Am::That])||
	   (db_state!=ping_db_state[Am::That])||
	   (replication_time!=ping_mysql_replication_time[Am::That])||
	   (audio_state!=ping_audio_state[Am::That])||
	   (audio_status!=ping_audio_status[Am::That])||
	   ((snapshot!="-")&&(snapshot!=ping_snapshot_name[Am::That]))) {
	  ping_mysql_running[Am::That]=running;
	  ping_mysql_accessible[Am::That]=accessible;
	  ping_db_state[Am::That]=db_state;
	  if(snapshot!="-") {
	    ping_snapshot_name[Am::That]=snapshot;
	  }
	  else {
	    ping_snapshot_name[Am::That]="";
	  }
	  ping_mysql_replication_time[Am::That]=replication_time;
	  ping_audio_state[Am::That]=audio_state;
	  ping_audio_status[Am::That]=audio_status;
	  emit thatStateChanged(ping_reachable[0]||ping_reachable[1],
				running,accessible,db_state,
				ping_snapshot_name[Am::That],
				replication_time,audio_state,audio_status);
	}
      }
      ping_sockets[id]->writeDatagram("pong",4,
			   ping_config->address(Am::That,(AMConfig::Address)id),
				     AM_PING_UDP_PORT);
    }
    if(fields[0]=="pong") {
      ping_watchdog_timer[id]->stop();
      ping_watchdog_timer[id]->start(AM_PING_TIMEOUT);
    }
  }
}


void PingMonitor::socketSendData()
{
  QString snapshot="-";

  if(!ping_snapshot_name[Am::This].isEmpty()) {
    snapshot=ping_snapshot_name[Am::This];
  }

  QString cmd=QString().sprintf("ping %d %d %d %s %d %d %d",
				ping_mysql_running[Am::This],
				ping_mysql_accessible[Am::This],
				ping_db_state[Am::This],
				snapshot.toUtf8().constData(),
				ping_mysql_replication_time[Am::This],
				ping_audio_state[Am::This],
				ping_audio_status[Am::This]);
  for(int i=0;i<AMConfig::LastAddress;i++) {
    ping_sockets[i]->
      writeDatagram(cmd.toUtf8(),
		    ping_config->address(Am::That,(AMConfig::Address)i),
		    AM_PING_UDP_PORT);
  }
}


void PingMonitor::socketTimeoutData(int id)
{
  if(ping_reachable[id]) {
    ping_reachable[id]=false;
    if(ping_reachable[0]==ping_reachable[1]) {
      ping_mysql_running[Am::That]=false;
      ping_mysql_accessible[Am::That]=false;
      ping_mysql_replication_time[Am::That]=0;
      emit thatStateChanged(ping_reachable[0]||ping_reachable[1],
			    ping_mysql_running[Am::That],
			    ping_mysql_accessible[Am::That],
			    ping_db_state[Am::That],
			    ping_snapshot_name[Am::That],
			    ping_mysql_replication_time[Am::That],
			    ping_audio_state[Am::That],
			    ping_audio_status[Am::That]);
    }
  }
}


int PingMonitor::OtherAddr(int addr)
{
  int ret=0;

  if(addr==0) {
    ret=1;
  }

  return ret;
}
