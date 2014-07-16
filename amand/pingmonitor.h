// pingmonitor.h
//
// Monitor the Opposite Aman Instance
//
//   (C) Copyright 2012 Fred Gleason <fredg@paravelsystems.com>
//
//      $Id: pingmonitor.h,v 1.8 2013/06/24 19:51:06 cvs Exp $
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

#ifndef PINGMONITOR_H
#define PINGMONITOR_H

#include <QtCore/QObject>
#include <QtCore/QTimer>
#include <QtCore/QSignalMapper>
#include <QtCore/QTimer>
#include <QtNetwork/QUdpSocket>

#include <config.h>
#include <state.h>

#include "am.h"
#include "profile.h"

class PingMonitor : public QObject
{
 Q_OBJECT;
 public:
  PingMonitor(Config *config,QObject *parent=0);
  ~PingMonitor();
  bool start();
  bool isReachable(Am::Instance inst) const;
  bool mysqlRunning(Am::Instance inst) const;
  bool mysqlAccessible(Am::Instance inst) const;
  State::ClusterState dbState(Am::Instance inst) const;
  State::ClusterState audioState(Am::Instance inst) const;
  bool audioStatus(Am::Instance inst);
  QString snapshotName(Am::Instance inst) const;
  int mysqlReplicationTime(Am::Instance inst) const;
  bool audioReplicationState(Am::Instance inst) const;
  void setThisMysqlState(bool running,bool accessible);
  void setThisDbState(State::ClusterState state);
  void setThisAudioState(State::ClusterState state);
  void setThisAudioStatus(bool status);
  void setThisSnapshotName(const QString &str);
  void setThisMysqlReplicationTime(int msecs);
  void setThisAudioReplicationState(bool state);

 signals:
  void thatStateChanged(bool ping,bool running,bool accessible,
			State::ClusterState db_state,
			const QString &snapshot_name,
			int replication_time,
			State::ClusterState audio_state,
			bool audio_status);

 private slots:
  void socketReadyData(int id);
  void socketSendData();
  void socketTimeoutData(int id);

 private:
  int OtherAddr(int addr);
  QSignalMapper *ping_ready_mapper;
  QSignalMapper *ping_watchdog_mapper;
  bool ping_mysql_running[Am::LastInstance];
  bool ping_mysql_accessible[Am::LastInstance];
  State::ClusterState ping_db_state[Am::LastInstance];
  State::ClusterState ping_audio_state[Am::LastInstance];
  bool ping_audio_status[Am::LastInstance];
  QString ping_snapshot_name[Am::LastInstance];
  int ping_mysql_replication_time[Am::LastInstance];
  bool ping_reachable[2];
  QUdpSocket *ping_sockets[2];
  QTimer *ping_send_timer;
  QTimer *ping_watchdog_timer[2];
  Config *ping_config;
};


#endif  // PINGMONITOR_H
