// generate_snapshot.cpp
//
// Generate a MySQL Snapshot
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

#include <errno.h>
#include <syslog.h>
#include <unistd.h>

#include <QProcess>
#include <QVariant>
#include <QSqlQuery>
#include <QSqlError>

#include "amand.h"

bool MainObject::GenerateMysqlSnapshot(const QString &filename)
{
  QString sql;
  QSqlQuery *q;
  AMConfig::Address addr;
  bool ret=true;
  FILE *f=NULL;

  //
  // Generate Working Directory
  //
  QString tempdir=MakeTempDir();
  if(tempdir.isNull()) {
    return false;
  }

  //
  // Open Mysql
  //
  addr=AMConfig::PublicAddress;
  if(!OpenMysql(Am::This,addr)) {
    addr=AMConfig::PublicAddress;
    if(!OpenMysql(Am::This,addr)) {
      return false;
    }
  }

  if(!CheckTableEngines("MyISAM")) {
    syslog(LOG_WARNING,"unable to generate snapshot");
    CloseMysql();
    return false;
  }

  //
  // Start a New Binlog
  //
  sql="flush logs";
  q=new QSqlQuery(sql,Db());
  if(!q->isActive()) {
    syslog(LOG_ERR,"cannot flush logs in mysql at %s [%s]",
	   main_config->address(Am::This,addr).toString().toUtf8().constData(),
	   q->lastError().text().toUtf8().constData());
    delete q;
    CloseMysql();
    return false;
  }
  delete q;

  //
  // Lock Tables
  //
  QTime start=QTime::currentTime();
  sql="flush tables with read lock";
  q=new QSqlQuery(sql,Db());
  if(!q->isActive()) {
    syslog(LOG_ERR,"cannot lock tables in mysql at %s [%s]",
	   main_config->address(Am::This,addr).toString().toUtf8().constData(),
	   q->lastError().text().toUtf8().constData());
    delete q;
    CloseMysql();
    return false;
  }
  delete q;

  //
  // Copy Database
  //
  QProcess *proc=new QProcess(this);
  QStringList args;
  args.push_back("-C");
  args.push_back(main_config->mysqlDataDirectory(Am::This));
  args.push_back("-cf");
  args.push_back(tempdir+"/sql.tar");
  args.push_back(main_config->globalMysqlDatabase());
  proc->start("tar",args);
  proc->waitForFinished(-1);
  if(proc->exitCode()!=0) {
    syslog(LOG_ERR,"mysql snapshot copy failed [%s]",
	   (const char *)proc->readAllStandardError());
    unlink(filename.toUtf8());
    ret=false;
  }
  delete proc;

  //
  // Generate Binlog Pointer
  //
  sql="show master status";
  q=new QSqlQuery(sql,Db());
  if(!q->first()) {
    syslog(LOG_ERR,"unable to get master status in mysql at %s [%s]",
	   main_config->address(Am::This,addr).toString().toUtf8().constData(),
	   q->lastError().text().toUtf8().constData());
    delete q;
    sql="unlock tables";
    q=new QSqlQuery(sql,Db());
    delete q;
    CloseMysql();
    return false;
  }
  if((f=fopen((tempdir+"/metadata.ini").toUtf8(),"w"))==NULL) {
    syslog(LOG_ERR,"unable to create temporary file at %s",
	   (tempdir+"/metadata.ini").toUtf8().constData());
    sql="unlock tables";
    q=new QSqlQuery(sql,Db());
    delete q;
    CloseMysql();
    return false;
  }
  fprintf(f,"[Master]\n");
  fprintf(f,"MysqlDbname=%s\n",
	  main_config->globalMysqlDatabase().toUtf8().constData());
  fprintf(f,"BinlogFilename=%s\n",
	  q->value(0).toString().toUtf8().constData());
  fprintf(f,"BinlogPosition=%u\n",q->value(1).toUInt());
  fclose(f);
  delete q;

  //
  // Unlock Database
  //
  sql="unlock tables";
  q=new QSqlQuery(sql,Db());
  delete q;
  QTime finish=QTime::currentTime();
  CloseMysql();

  //
  // Generate Archive
  //
  proc=new QProcess(this);
  args.clear();
  args.push_back("-C");
  args.push_back(tempdir);
  args.push_back("-jcf");
  args.push_back(filename);
  args.push_back("sql.tar");
  args.push_back("metadata.ini");
  proc->start("tar",args);
  proc->waitForFinished(-1);
  if(proc->exitCode()!=0) {
    syslog(LOG_ERR,"mysql snapshot archive creation failed [%s]",
	   proc->readAllStandardError().constData());
    unlink(filename.toUtf8());
    ret=false;
  }
  delete proc;
  syslog(LOG_INFO,
	 "generated MySQL snapshot in \"%s\", db was locked for %6.2lf sec",
	 filename.toUtf8().constData(),(double)start.msecsTo(finish)/1000.0);

  //
  // Push Snapshot to Remote Systems
  //
  if(!PushFile(filename,main_config->address(Am::That,AMConfig::PublicAddress).
	       toString(),filename)) {
    if(!PushFile(filename,main_config->address(Am::That,AMConfig::PrivateAddress).
		 toString(),filename)) {
      syslog(LOG_ERR,"unable to push snapshot to \"%s\"",
	     main_config->hostname(Am::That).toUtf8().constData());
    }
  }

  //
  // Archive Snapshot
  //
  if(!main_config->archiveDirectory(Am::This).isEmpty()) {
    QStringList f0=filename.split("/",QString::SkipEmptyParts);
    QString dst_filename=
      main_config->archiveDirectory(Am::This)+"/"+f0.at(f0.size()-1);
    QString err_msg;
    if(!AMConfig::copyFile(filename,dst_filename,&err_msg)) {
      syslog(LOG_WARNING,"unable to archive DB snapshot to \"%s\" [%s]",
	     (const char *)dst_filename.toUtf8(),
	     (const char *)err_msg.toUtf8());
    }
  }

  //
  // Clean Up
  //
  unlink((tempdir+"/sql.tar").toUtf8());
  unlink((tempdir+"/metadata.ini").toUtf8());
  rmdir(tempdir.toUtf8());

  return ret;
}
