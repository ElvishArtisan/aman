// amanctl.h
//
// amanctl(8) Control Aman from the command line
//
//   (C) Copyright 2017 Fred Gleason <fredg@paravelsystems.com>
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

#ifndef AMANCTL_H
#define AMANCTL_H

#define AMANCTL_USAGE "\n"

#include <QtCore/QObject>
#include <QtNetwork/QTcpSocket>

#include <config.h>

class MainObject : public QObject
{
 Q_OBJECT;
 public:
  enum DbState {DbOffline=0,DbIdle=1,DbMaster=2,DbSlave=3};
  enum AudioState {AudioIDle=1,AudioSlave=3};
  MainObject(QObject *parent=0);

 private slots:
  void connectedData();
  void disconnectedData();
  void readyReadData();
  void errorData(QAbstractSocket::SocketError err);

 private:
  void ProcessResponse(const QString &str);
  void PrintServerState(const QStringList &msg,int arg_offset) const;
  QString DbStateString(int repl_state) const;
  QString AudioStateString(int repl_state) const;
  QTcpSocket *ctl_socket;
  QString ctl_command;
  int ctl_system;
  QString ctl_accum;
  DbState ctl_db_states[2];
  AudioState ctl_audio_states[2];
  Config *ctl_config;
};


#endif  // AMANCTL_H
