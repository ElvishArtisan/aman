// replicationtest.cpp
//
// Test MySQL Replication
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

#include <syslog.h>

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QVariant>

#include "replicationtest.h"

ReplicationTest::ReplicationTest(AMConfig::Address addr,AMConfig *config,
				 QObject *parent)
  : QObject(parent)
{
  repl_addr=addr;
  repl_config=config;
  repl_value=0;
  repl_ticks=0;

  //
  // Timer
  //
  repl_timer=new QTimer(this);
  repl_timer->setSingleShot(true);
  connect(repl_timer,SIGNAL(timeout()),this,SLOT(tryResultData()));
}


ReplicationTest::~ReplicationTest()
{
  delete repl_timer;
}


bool ReplicationTest::isActive() const
{
  return repl_timer->isActive();
}


bool ReplicationTest::startTest(int val)
{
  if(isActive()) {
    return false;
  }
  repl_value=val;
  repl_ticks=0;

  //
  // Open Database
  //
  QSqlDatabase db=
    QSqlDatabase::addDatabase(repl_config->globalMysqlDriver(),"repl_db");
  db.setDatabaseName(repl_config->globalMysqlDatabase());
  db.setUserName(repl_config->mysqlUsername(Am::That));
  db.setPassword(repl_config->mysqlPassword(Am::That));
  db.setHostName(repl_config->address(Am::That,repl_addr).toString());
  if(!db.open()) {
    syslog(LOG_ERR,"cannot connect to mysql at %s [%s]",
	   (const char *)repl_config->address(Am::That,repl_addr).toString().
	   toAscii(),
	   (const char *)db.lastError().text().toAscii());
    emit testComplete(false,0);
    return true;
  }

  repl_timer->start(AM_REPLICATION_TICK_INTERVAL);

  return true;
}


void ReplicationTest::tryResultData()
{
  QString sql;
  QSqlQuery *q;

  repl_ticks++;

  //
  // Read Value
  //
  sql=QString("select VALUE from ")+repl_config->pingTablename(Am::This);
  q=new QSqlQuery(sql,QSqlDatabase::database("repl_db"));
  if(!q->first()) {
    syslog(LOG_ERR,"cannot select from mysql at %s [%s]",
	   (const char *)repl_config->address(Am::That,repl_addr).toString().
	   toAscii(),
	   (const char *)q->lastError().text().toAscii());
    delete q;
    ReportResult(false,0);
    return;
  }
  if(q->value(0).toInt()==repl_value) {
    delete q;
    ReportResult(true,repl_ticks);
    return;
  }
  delete q;

  //
  // Check for Timeout
  //
  if(repl_ticks>=repl_config->globalMysqlReplicationTimeout()) {
    ReportResult(false,0);
    return;
  }

  //
  // Restart for Another Attempt
  //
  repl_timer->start(AM_REPLICATION_TICK_INTERVAL);
}


void ReplicationTest::ReportResult(bool success,int ticks)
{
  QSqlDatabase::removeDatabase("repl_db");
  emit testComplete(success,ticks*AM_REPLICATION_TICK_INTERVAL);
}
