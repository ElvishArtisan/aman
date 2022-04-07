// restore_snapshot.cpp
//
// Restore a MySQL Snapshot
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

#include <syslog.h>
#include <unistd.h>

#include <QDir>
#include <QProcess>
#include <QStringList>
#include <QVariant>
#include <QSqlQuery>
#include <QSqlError>

#include "amanrmt.h"

bool MainWidget::RestoreMysqlSnapshot(const QString &filename,QString *binlog,
				      int *pos,int src_sys,QString *err_msg)
{
  QString sql;
  QSqlQuery *q;
  QStringList args;
  AMProfile *p=NULL;

  *binlog="-";
  *pos=0;

  //
  // Generate Working Directory
  //
  QString tempdir=MakeTempDir();
  if(tempdir.isNull()) {
    return false;
  }

  //
  // Unpack Archive
  //
  QProcess *proc=new QProcess(this);
  args.clear();
  args.push_back("-C");
  args.push_back(tempdir);
  args.push_back("-jxf");
  args.push_back(QString(AM_SNAPSHOT_DIR)+"/"+filename);
  proc->start("tar",args);
  proc->waitForFinished(-1);
  if(proc->exitCode()!=0) {
    *err_msg=QString::asprintf("unpacking snapshot failed [%s]",
			       proc->readAllStandardError().constData());
    delete proc;
    return false;
  }
  delete proc;

  //
  // Stop Replication
  //
  sql="stop slave";
  q=new QSqlQuery(sql);
  delete q;
  sql="reset slave";
  q=new QSqlQuery(sql);
  delete q;
  CloseDb();

  //
  // Stop MySQL Service
  //
  proc=new QProcess(this);
  args.clear();
  args.push_back(am_config->globalMysqlServiceName());
  args.push_back("stop");
  proc->start("service",args);
  proc->waitForFinished(-1);
  if(proc->exitCode()!=0) {
    *err_msg=QString::asprintf("%s(8) shutdown failed [%s]",
			       am_config->globalMysqlServiceName().toUtf8().constData(),
			       proc->readAllStandardError().constData());
    delete proc;
    return false;
  }
  delete proc;

  //
  // Delete Old Database
  //
  QDir dir(QString(AM_MYSQL_DATADIR)+"/"+am_config->globalMysqlDatabase());
  QStringList files=dir.entryList();
  for(int i=0;i<files.size();i++) {
    unlink(files[i].toUtf8());
  }
  rmdir((QString(AM_MYSQL_DATADIR)+"/"+
	 am_config->globalMysqlDatabase()).toUtf8());
  
  //
  // Install New Database
  //
  proc=new QProcess(this);
  args.clear();
  args.push_back("-C");
  args.push_back(AM_MYSQL_DATADIR);
  args.push_back("-xf");
  args.push_back(tempdir+"/sql.tar");
  proc->start("tar",args);
  proc->waitForFinished(-1);
  if(proc->exitCode()!=0) {
    *err_msg=QString::asprintf("database table restore failed [%s]",
			       proc->readAllStandardError().constData());
    delete proc;
    return false;
  }
  delete proc;

  //
  // Restart MySQL Service
  //
  proc=new QProcess(this);
  args.clear();
  args.push_back(am_config->globalMysqlServiceName());
  args.push_back("start");
  proc->start("service",args);
  proc->waitForFinished(-1);
  if(proc->exitCode()!=0) {
    *err_msg=QString::asprintf("%s(8) restart failed [%s]",
			       am_config->globalMysqlServiceName().toUtf8().constData(),
			       proc->readAllStandardError().constData());
    delete proc;
    return false;
  }
  delete proc;

  if(!OpenDb(err_msg)) {
    return false;
  }

  sql="reset master";
  q=new QSqlQuery(sql);
  delete q;

  //
  // Open Metadata Record
  //
  p=new AMProfile();
  if(!p->setSource(tempdir+"/metadata.ini")) {
    *err_msg=QString::asprintf("unable to open metadata in snapshot");
    return false;
  }

  //
  // Restart Replication
  //
  sql="stop slave";
  q=new QSqlQuery(sql);
  delete q;
  sql="reset slave";
  q=new QSqlQuery(sql);
  delete q;
  sql=QString("change master to MASTER_HOST=\"")+
    am_config->address(src_sys,AMConfig::PublicAddress).toString()+"\","+
    "MASTER_USER=\""+am_config->mysqlUsername(src_sys)+"\","+
    "MASTER_PASSWORD=\""+am_config->mysqlPassword(src_sys)+"\","+
    "MASTER_LOG_FILE=\""+p->stringValue("Master","BinlogFilename")+"\","+
    "MASTER_LOG_POS="+
    QString().sprintf("%d",p->intValue("Master","BinlogPosition"));
  q=new QSqlQuery(sql);
  if(!q->isActive()) {
    *err_msg=QString::asprintf("cannot configure replication source [%s]",
			       q->lastError().text().toUtf8().constData());
    delete q;
    return false;
  }
  delete q;
  sql="start slave";
  q=new QSqlQuery(sql);
  if(!q->isActive()) {
    *err_msg=QString::asprintf("starting replication slave failed [%s]",
			       q->lastError().text().toUtf8().constData());
    delete q;
    return false;
  }
  delete q;

  //
  // Get New Metadata
  //
  sql="show master status";
  q=new QSqlQuery(sql);
  if(q->first()) {
    *binlog=q->value(0).toString();
    *pos=q->value(1).toInt();
  }
  delete q;

  //
  // Clean Up
  //
  unlink((tempdir+"/sql.tar").toUtf8());
  unlink((tempdir+"/metadata.ini").toUtf8());
  rmdir(tempdir.toUtf8());
  delete p;

  return true;
}


bool MainWidget::OpenDb(QString *err_msg)
{
  AMProfile *p=new AMProfile();
  p->setSource("/etc/amanrmt.conf");

  QSqlDatabase db=
    QSqlDatabase::addDatabase(p->stringValue("MySQL","Driver","QMYSQL3"));
  if(!db.isValid()) {
    *err_msg=QObject::tr("Couldn't initialize MySql driver!");
    return false;
  }
  db.setHostName(p->stringValue("MySQL","Hostname","localhost"));
  db.setDatabaseName(p->stringValue("MySQL","Database","Rivendell"));
  db.setUserName(p->stringValue("MySQL","Loginname","amanrmt"));
  db.setPassword(p->stringValue("MySQL","Password"));
  if(!db.open()) {
    *err_msg=QObject::tr("Couldn't open MySQL connection!");
    db.removeDatabase(p->stringValue("MySQL","Database","Rivendell"));
    db.close();
    return false;
  }
  delete p;

  return true;
}


void MainWidget::CloseDb()
{
  QSqlDatabase::removeDatabase("main_db");
}
