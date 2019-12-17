// sqltorture.cpp
//
// Stree-test a MySQL server with write quiries.
//
//   (C) Copyright 2017-2019 Fred Gleason <fredg@paravelsystems.com>
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

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>

#include <QtCore/QCoreApplication>
#include <QtSql/QSqlError>
#include <QtSql/QSqlQuery>

#include <amcmdswitch.h>

#include "sqltorture.h"

bool global_exiting=false;

void SigHandler(int signo)
{
  switch(signo) {
  case SIGTERM:
  case SIGINT:
    global_exiting=true;
    break;
  }
}


MainObject::MainObject(QObject *parent)
  : QObject(parent)
{
  QString mysql_hostname="localhost";
  QString mysql_username="rduser";
  QString mysql_password="letmein";
  QString mysql_database="Rivendell";
  QString mysql_tablename="SQLTORTURE";

  //
  // Read Command Options
  //
  AMCmdSwitch *cmd=
    new AMCmdSwitch(qApp->argc(),qApp->argv(),"sqltorture",SQLTORTURE_USAGE);
  for(unsigned i=0;i<cmd->keys();i++) {
    if(cmd->key(i)=="--mysql-hostname") {
      mysql_hostname=cmd->value(i);
      cmd->setProcessed(i,true);
    }
    if(cmd->key(i)=="--mysql-username") {
      mysql_username=cmd->value(i);
      cmd->setProcessed(i,true);
    }
    if(cmd->key(i)=="--mysql-password") {
      mysql_password=cmd->value(i);
      cmd->setProcessed(i,true);
    }
    if(cmd->key(i)=="--mysql-database") {
      mysql_database=cmd->value(i);
      cmd->setProcessed(i,true);
    }
    if(cmd->key(i)=="--mysql-tablename") {
      mysql_tablename=cmd->value(i);
      cmd->setProcessed(i,true);
    }
    if(!cmd->processed(i)) {
      fprintf(stderr,"sqltoruture: unknown option \"%s\"\n",
	      (const char *)cmd->key(i).toUtf8());
      exit(256);
    }
  }
  delete cmd;

  //
  // Open Database
  //
  QSqlDatabase db=QSqlDatabase::addDatabase("QMYSQL3");
  db.setDatabaseName(mysql_database);
  db.setUserName(mysql_username);
  db.setPassword(mysql_password);
  db.setHostName(mysql_hostname);
  if(!db.open()) {
    fprintf(stderr,"sqltoruture: cannot connect to mysql at %s [%s]",
	    (const char *)mysql_hostname.toUtf8(),
	    (const char *)db.lastError().text().toUtf8());
    exit(256);
  }

  //
  // Create table
  //
  QString sql=QString("create table if not exists ")+
    mysql_tablename+"("+
    "ID int primary key auto_increment,"+
    "VALUE int)";
  QSqlQuery *q=new QSqlQuery(sql);
  if(!q->isActive()) {
    fprintf(stderr,"sqltorture: unable to create SQL table [%s]\n",
	    (const char *)db.lastError().text().toUtf8());
    exit(256);
  }

  //
  // Main Loop
  //
  signal(SIGTERM,SigHandler);
  signal(SIGINT,SigHandler);
  while(!global_exiting) {
    sql=QString("insert into ")+mysql_tablename+" set "+
      QString().sprintf("VALUE=%d",rand());
    q=new QSqlQuery(sql);
    delete q;
    sql=QString("delete from ")+mysql_tablename;
    q=new QSqlQuery(sql);
    delete q;
  }

  //
  // Drop Table
  //
  sql=QString("drop table ")+mysql_tablename;
  q=new QSqlQuery(sql);
  delete q;

  exit(0);
}


int main(int argc,char *argv[])
{
  QCoreApplication a(argc,argv);
  new MainObject();

  return a.exec();
}
