// amconnection.h
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

#ifndef AMCONNECTION_H
#define AMCONNECTION_H

#include <stdint.h>

#include <QTimer>
#include <QObject>
#include <QTcpSocket>

#include <curl/curl.h>

#include <amstate.h>

#include "am.h"

#define AMCONNECTION_RECONNECT_INTERVAL 5000

class AMStatus
{
 public:
  AMStatus();
  QString hostname() const;
  void setHostname(const QString &str);
  bool serviceRunning() const;
  void setServiceRunning(bool state);
  bool dbRunning() const;
  void setDbRunning(bool state);
  bool dbAccessible() const;
  void setDbAccessible(bool state);
  AMState::ClusterState dbState() const;
  void setDbState(AMState::ClusterState state);
  int dbReplicationTime() const;
  void setDbReplicationTime(int msecs);
  AMState::ClusterState audioState() const;
  void setAudioState(AMState::ClusterState state);
  bool audioStatus() const;
  void setAudioStatus(bool status);
  bool isLocal() const;
  void setLocal(bool state);

 private:
  QString stat_hostname;
  bool stat_service_running;
  bool stat_db_running;
  bool stat_db_accessible;
  AMState::ClusterState stat_db_state;
  int stat_db_replication_time;
  AMState::ClusterState stat_audio_state;
  bool stat_audio_status;
  bool stat_is_local;
};


class AMConnection : public QObject
{
 Q_OBJECT;
 public:
  AMConnection(QObject *parent=0);
  ~AMConnection();
  void connectToHost(const QString &hostname,uint16_t port);
  bool isConnected() const;
  AMStatus *status(int sys);

 public slots:
  void generateSnapshot();
  bool downloadSnapshot(const QString &name,const QString &ssh_id,
			QString *err_msg);
  void loadSnapshot(const QString &name);
  void makeDbMaster();
  void makeDbSlave();
  void makeDbIdle();
  void startAudioSlave();
  void stopAudioSlave();

 signals:
  void statusChanged(AMStatus *a,AMStatus *b);
  void snapshotGenerated(const QString &name);
  void snapshotLoaded(const QString &name);
  void connected();
  void disconnected();
  void errorReturned(const QString &str);

 private slots:
  void connectedData();
  void disconnectedData();
  void errorData(QAbstractSocket::SocketError err);
  void readyReadData();
  void watchdogData();

 private:
  void ProcessMessage();
  AMStatus *conn_status[2];
  QString conn_buffer;
  QTcpSocket *conn_socket;
  QTimer *conn_watchdog_timer;
  QString conn_hostname;
  QString conn_localname;
  uint16_t conn_port;
  char conn_curl_errorbuffer[CURL_ERROR_SIZE];
};


#endif  // AMCONNECTION_H
