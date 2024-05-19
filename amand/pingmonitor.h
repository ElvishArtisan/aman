// pingmonitor.h
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

#ifndef PINGMONITOR_H
#define PINGMONITOR_H

#include <QObject>
#include <QTimer>
#include <QSignalMapper>
#include <QTimer>
#include <QUdpSocket>

#include <amconfig.h>
#include <amstate.h>

#include "am.h"
#include "amprofile.h"

class PingMonitor : public QObject
{
 Q_OBJECT;
 public:
  PingMonitor(AMConfig *config,QObject *parent=0);
  ~PingMonitor();
  bool start();
  bool isReachable(Am::Instance inst) const;
  bool mysqlRunning(Am::Instance inst) const;
  bool mysqlAccessible(Am::Instance inst) const;
  AMState::ClusterState dbState(Am::Instance inst) const;
  AMState::ClusterState audioState(Am::Instance inst) const;
  bool audioStatus(Am::Instance inst);
  QString snapshotName(Am::Instance inst) const;
  int mysqlReplicationTime(Am::Instance inst) const;
  bool audioReplicationState(Am::Instance inst) const;
  void setThisMysqlState(bool running,bool accessible);
  void setThisDbState(AMState::ClusterState state);
  void setThisAudioState(AMState::ClusterState state);
  void setThisAudioStatus(bool status);
  void setThisSnapshotName(const QString &str);
  void setThisMysqlReplicationTime(int msecs);
  void setThisAudioReplicationState(bool state);

 signals:
  void thatStateChanged(bool ping,bool running,bool accessible,
			AMState::ClusterState db_state,
			const QString &snapshot_name,
			int replication_time,
			AMState::ClusterState audio_state,
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
  AMState::ClusterState ping_db_state[Am::LastInstance];
  AMState::ClusterState ping_audio_state[Am::LastInstance];
  bool ping_audio_status[Am::LastInstance];
  QString ping_snapshot_name[Am::LastInstance];
  int ping_mysql_replication_time[Am::LastInstance];
  bool ping_reachable[2];
  QUdpSocket *ping_sockets[2];
  QTimer *ping_send_timer;
  QTimer *ping_watchdog_timer[2];
  AMConfig *ping_config;
};


#endif  // PINGMONITOR_H
