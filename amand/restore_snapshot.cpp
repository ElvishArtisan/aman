// restore_snapshot.cpp
//
// Restore a MySQL Snapshot
//
//   (C) Copyright 2012 Fred Gleason <fredg@paravelsystems.com>
//
//      $Id: restore_snapshot.cpp,v 1.2 2013/06/15 00:23:36 cvs Exp $
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

#include <QtCore/QProcess>
#include <QtCore/QStringList>
#include <QtCore/QVariant>
#include <QtSql/QSqlQuery>
#include <QtSql/QSqlError>

#include "profile.h"
#include "amand.h"

bool MainObject::RestoreMysqlSnapshot(const QString &filename,QString *binlog,
				      int *pos)
{
  QString sql;
  QSqlQuery *q;
  QStringList args;
  Config::Address addr;
  Profile *p=NULL;

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
  args.push_back(filename);
  proc->start("tar",args);
  proc->waitForFinished(-1);
  if(proc->exitCode()!=0) {
    syslog(LOG_ERR,"unpacking snapshot failed [%s]",
	   (const char *)proc->readAllStandardError());
    delete proc;
    return false;
  }
  delete proc;

  //
  // Open Mysql
  //
  addr=Config::PublicAddress;
  if(!OpenMysql(Am::This,addr)) {
    addr=Config::PublicAddress;
    if(!OpenMysql(Am::This,addr)) {
      return false;
    }
  }

  //
  // Stop Replication
  //
  sql="stop slave";
  q=new QSqlQuery(sql,Db());
  delete q;
  sql="reset slave";
  q=new QSqlQuery(sql,Db());
  delete q;
  CloseMysql();

  //
  // Stop MySQL Service
  //
  proc=new QProcess(this);
  args.clear();
  args.push_back(main_config->globalMysqlServiceName());
  args.push_back("stop");
  proc->start("service",args);
  proc->waitForFinished(-1);
  if(proc->exitCode()!=0) {
    syslog(LOG_ERR,"%s(8) shutdown failed [%s]",
	   (const char *)main_config->globalMysqlServiceName().toUtf8(),
	   (const char *)proc->readAllStandardError());
    delete proc;
    return false;
  }
  delete proc;

  //
  // Delete Old Database
  //
  QDir dir(main_config->mysqlDataDirectory(Am::This)+"/"+
	   main_config->globalMysqlDatabase());
  QStringList files=dir.entryList();
  for(int i=0;i<files.size();i++) {
    unlink(files[i].toAscii());
  }
  rmdir((main_config->mysqlDataDirectory(Am::This)+"/"+
	 main_config->globalMysqlDatabase()).toAscii());
  
  //
  // Install New Database
  //
  proc=new QProcess(this);
  args.clear();
  args.push_back("-C");
  args.push_back(main_config->mysqlDataDirectory(Am::This));
  args.push_back("-xf");
  args.push_back(tempdir+"/sql.tar");
  proc->start("tar",args);
  proc->waitForFinished(-1);
  if(proc->exitCode()!=0) {
    syslog(LOG_ERR,"database table restore failed [%s]",
	   (const char *)proc->readAllStandardError());
    delete proc;
    return false;
  }
  delete proc;

  //
  // Restart MySQL Service
  //
  proc=new QProcess(this);
  args.clear();
  args.push_back(main_config->globalMysqlServiceName());
  args.push_back("start");
  proc->start("service",args);
  proc->waitForFinished(-1);
  if(proc->exitCode()!=0) {
    syslog(LOG_ERR,"%s(8) restart failed [%s]",
	   (const char *)main_config->globalMysqlServiceName().toUtf8(),
	   (const char *)proc->readAllStandardError());
    delete proc;
    return false;
  }
  delete proc;
  if(!OpenMysql(Am::This,addr)) {
    return false;
  }
  sql="reset master";
  q=new QSqlQuery(sql,Db());
  delete q;

  //
  // Open Metadata Record
  //
  p=new Profile();
  if(!p->setSource(tempdir+"/metadata.ini")) {
    syslog(LOG_ERR,"unable to open metadata in snapshot");
    CloseMysql();
    return false;
  }

  //
  // Restart Replication
  //
  sql="stop slave";
  q=new QSqlQuery(sql,Db());
  delete q;
  sql="reset slave";
  q=new QSqlQuery(sql,Db());
  delete q;
  sql=QString("change master to MASTER_HOST=\"")+
    main_config->address(Am::That,Config::PrivateAddress).toString()+"\","+
    "MASTER_USER=\""+main_config->mysqlUsername(Am::This)+"\","+
    "MASTER_PASSWORD=\""+main_config->mysqlPassword(Am::This)+"\","+
    "MASTER_LOG_FILE=\""+p->stringValue("Master","BinlogFilename")+"\","+
    "MASTER_LOG_POS="+
    QString().sprintf("%d",p->intValue("Master","BinlogPosition"));
  q=new QSqlQuery(sql,Db());
  if(!q->isActive()) {
    syslog(LOG_ERR,"cannot configure replication source in mysql at %s [%s]",
	   (const char *)main_config->address(Am::This,addr).toString().
	   toAscii(),
	   (const char *)q->lastError().text().toAscii());
    delete q;
    CloseMysql();
    return false;
  }
  delete q;
  sql="start slave";
  q=new QSqlQuery(sql,Db());
  if(!q->isActive()) {
    syslog(LOG_ERR,"starting replication slave failed in mysql at %s [%s]",
	   (const char *)main_config->address(Am::This,addr).toString().
	   toAscii(),
	   (const char *)q->lastError().text().toAscii());
    delete q;
    CloseMysql();
    return false;
  }
  delete q;

  //
  // Get New Metadata
  //
  sql="show master status";
  q=new QSqlQuery(sql,Db());
  if(q->first()) {
    *binlog=q->value(0).toString();
    *pos=q->value(1).toInt();
  }
  delete q;
  CloseMysql();

  //
  // Clean Up
  //
  unlink((tempdir+"/sql.tar").toAscii());
  unlink((tempdir+"/metadata.ini").toAscii());
  rmdir(tempdir.toAscii());
  delete p;

  return true;
}
