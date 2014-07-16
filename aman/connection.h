// connection.h
//
// Client connection to an Aman monitor instance.
//
//   (C) Copyright 2012 Fred Gleason <fredg@paravelsystems.com>
//
//      $Id: connection.h,v 1.5 2013/06/18 23:09:54 cvs Exp $
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

#ifndef CONNECTION_H
#define CONNECTION_H

#include <stdint.h>

#include <QtCore/QTimer>
#include <QtCore/QObject>
#include <QtNetwork/QTcpSocket>

#include <state.h>

#include "am.h"

#define CONNECTION_RECONNECT_INTERVAL 5000

class Status
{
 public:
  Status();
  QString hostname() const;
  void setHostname(const QString &str);
  bool serviceRunning() const;
  void setServiceRunning(bool state);
  bool dbRunning() const;
  void setDbRunning(bool state);
  bool dbAccessible() const;
  void setDbAccessible(bool state);
  State::ClusterState dbState() const;
  void setDbState(State::ClusterState state);
  int dbReplicationTime() const;
  void setDbReplicationTime(int msecs);
  State::ClusterState audioState() const;
  void setAudioState(State::ClusterState state);
  bool audioStatus() const;
  void setAudioStatus(bool status);
  bool isLocal() const;
  void setLocal(bool state);

 private:
  QString stat_hostname;
  bool stat_service_running;
  bool stat_db_running;
  bool stat_db_accessible;
  State::ClusterState stat_db_state;
  int stat_db_replication_time;
  State::ClusterState stat_audio_state;
  bool stat_audio_status;
  bool stat_is_local;
};


class Connection : public QObject
{
 Q_OBJECT;
 public:
  Connection(QObject *parent=0);
  ~Connection();
  void connectToHost(const QString &hostname,uint16_t port);
  Status *status(int sys);

 public slots:
  void generateSnapshot();
  void loadSnapshot(const QString &name);
  void makeDbMaster();
  void makeDbSlave();
  void makeDbIdle();
  void startAudioSlave();
  void stopAudioSlave();

 signals:
  void statusChanged(Status *a,Status *b);
  void snapshotGenerated(const QString &name);
  void snapshotLoaded(const QString &name);
  void connected();
  void disconnected();

 private slots:
  void connectedData();
  void disconnectedData();
  void errorData(QAbstractSocket::SocketError err);
  void readyReadData();
  void watchdogData();

 private:
  void ProcessMessage();
  Status *conn_status[2];
  QString conn_buffer;
  QTcpSocket *conn_socket;
  QTimer *conn_watchdog_timer;
  QString conn_hostname;
  QString conn_localname;
  uint16_t conn_port;
};


#endif  // CONNECTION_H
