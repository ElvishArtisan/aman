// replicationtest.h
//
// Test MySQL Replication
//
//   (C) Copyright 2012 Fred Gleason <fredg@paravelsystems.com>
//
//      $Id: replicationtest.h,v 1.2 2013/06/24 19:51:06 cvs Exp $
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

#ifndef REPLICATIONTEST_H
#define REPLICATIONTEST_H

#include <QtCore/QObject>
#include <QtCore/QTimer>

#include <config.h>

class ReplicationTest : public QObject
{
 Q_OBJECT;
 public:
  ReplicationTest(Config::Address addr,Config *config,QObject *parent=0);
  ~ReplicationTest();
  bool isActive() const;

 public slots:
  bool startTest(int val);

 signals:
  void testComplete(bool success,int msecs);

 private slots:
  void tryResultData();

 private:
  void ReportResult(bool success,int ticks);
  QTimer *repl_timer;
  int repl_value;
  int repl_ticks;
  Config *repl_config;
  Config::Address repl_addr;
};


#endif  // REPLICATIONTEST_H
