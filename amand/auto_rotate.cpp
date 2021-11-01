// auto_rotate.cpp
//
// Autorotation Routines for amand(8).
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
#include <errno.h>

#include <QDir>
#include <QSqlQuery>
#include <QVariant>

#include "amand.h"

void MainObject::autoRotateData()
{
  AutoRotate();
}


void MainObject::ScheduleAutoRotation()
{
  int msecs=0;

  if(main_config->globalAutoRotateBinlogs()) {
    QDateTime now=QDateTime(QDate::currentDate(),QTime::currentTime());
    if(now.time()<main_config->globalAutoRotateTime()) {
      msecs=now.time().msecsTo(main_config->globalAutoRotateTime());
    }
    else {
      msecs=now.time().msecsTo(QTime(23,59,59))+1000+
	QTime().msecsTo(main_config->globalAutoRotateTime());
    }
    main_auto_rotate_timer->start(msecs);
    syslog(LOG_DEBUG,"next auto rotation scheduled for %s",
	   now.addSecs(msecs/1000).toString("hh:mm:ss").toUtf8().constData());
  }
}


void MainObject::AutoRotate()
{
  //
  // Generate Snapshot
  //
  QString snapshot=MakeSnapshotName();

  if(GenerateMysqlSnapshot(QString(AM_SNAPSHOT_DIR)+"/"+snapshot)) {
    main_state->setCurrentSnapshot(Am::This,snapshot);
    main_monitor->setThisSnapshotName(snapshot);
  }

  //
  // Purge Snapshots
  //
  main_state->purgeSnapshots();

  //
  // Purge Bin Logs
  //
  if(main_config->globalAutoPurgeBinlogs()) {
    PurgeBinlogs();
  }

  ScheduleAutoRotation();
}


void MainObject::PurgeBinlogs()
{
  QString sql;
  QSqlQuery *q;
  QString log1;
  QString log2;
  QStringList f1;
  QStringList f2;
  bool ok1=false;
  bool ok2=false;
  unsigned v1=0;
  unsigned v2=0;

  if((main_monitor->dbState(Am::This)==AMState::StateMaster)&&
     (main_monitor->dbState(Am::That)==AMState::StateSlave)) {
    if(OpenMysql(Am::This,AMConfig::PrivateAddress)) {
      sql="show master status";
      q=new QSqlQuery(sql,Db());
      if(q->first()) {
	log1=q->value(0).toString();
	delete q;
	CloseMysql();
	if(OpenMysql(Am::That,AMConfig::PublicAddress)) {
	  sql="show slave status";
	  q=new QSqlQuery(sql,Db());
	  if(q->first()) {
	    log2=q->value(5).toString();   // Field: 'Master_Log_File'
	  }
	  CloseMysql();
	  f1=log1.split(".");
	  f2=log2.split(".");
	  if((f1.size()==2)&&(f2.size()==2)) {
	    v1=f1[1].toUInt(&ok1);
	    v2=f2[1].toUInt(&ok2);
	    if(ok1&&ok2) {
	      if(v1<=v2) {
		DeleteBinlogSequence(f1[0],v1);
	      }
	      else {
		DeleteBinlogSequence(f2[0],v2);
	      }
	    }
	  }
	}
      }
      else {
	delete q;
	CloseMysql();
      }
    }
  }
  if((main_monitor->dbState(Am::This)==AMState::StateSlave)&&
     (main_monitor->dbState(Am::That)==AMState::StateMaster)) {
    if(OpenMysql(Am::This,AMConfig::PrivateAddress)) {
      sql="show master status";
      q=new QSqlQuery(sql,Db());
      if(q->first()) {
	log1=q->value(0).toString();      // Field: 'File'
	delete q;
	CloseMysql();
	f1=log1.split(".");
	if(f1.size()==2) {
	  v1=f1[1].toUInt(&ok1);
	  if(ok1) {
	    DeleteBinlogSequence(f1[0],v1-1);
	  }
	}
	if(OpenMysql(Am::This,AMConfig::PrivateAddress)) {
	  sql="show slave status";
	  q=new QSqlQuery(sql,Db());
	  if(q->first()) {
	    log1=q->value(7).toString();   // Field: 'Relay_Log_File'
	  }
	  delete q;
	  CloseMysql();
	  f1=log1.split(".");
	  if(f1.size()) {
	    v1=f1[1].toUInt(&ok1);
	    if(ok1) {
	      DeleteBinlogSequence(f1[0],v1-2);
	    }
	  }
	}
      }
      else {
	delete q;
	CloseMysql();
      }
    }
  }
}


void MainObject::DeleteBinlogSequence(const QString &basename,
				      unsigned last)
{
  printf("delete up to %s.%06u\n",(const char *)basename.toUtf8(),last);

  if(!main_config->archiveDirectory(Am::This).isEmpty()) {
    FILE *f=NULL;
    QString indexfile=
      main_config->mysqlDataDirectory(Am::This)+"/"+basename+".index";
    if((f=fopen(indexfile.toUtf8(),"r"))==NULL) {
      syslog(LOG_WARNING,"unable to open MySQL binlog index at \"%s\"",
	     (const char *)indexfile.toUtf8());
    }
    else {
      QString lastfile=basename+QString().sprintf(".%06u",last);
      QStringList lines=QTextStream(f,QIODevice::ReadOnly).
	readAll().split("\n",QString::SkipEmptyParts);
      for(int i=0;i<lines.size();i++) {
	QStringList f0=lines.at(i).split("/",QString::SkipEmptyParts);
	QString logfile=f0.at(f0.size()-1);
	if(logfile!=basename+QString().sprintf(".%06u",last)) {
	  QString err_msg;
	  QString src_file=
	    main_config->mysqlDataDirectory(Am::This)+"/"+logfile;
	  QString dst_file=main_config->archiveDirectory(Am::This)+"/"+logfile;
	  if(AMConfig::copyFile(src_file,dst_file,&err_msg)) {
	    syslog(LOG_DEBUG,"archived binlog to \"%s\"",
		   (const char *)dst_file.toUtf8());
	  }
	  else {
	    syslog(LOG_WARNING,"unable to archive binlog to \"%s\" [%s]",
		   (const char *)dst_file.toUtf8(),
		   (const char *)err_msg.toUtf8());
	  }
	}
      }
    }
  }

  QString sql;
  QSqlQuery *q;

  if(OpenMysql(Am::This,AMConfig::PublicAddress)) {
    sql=QString("purge binary logs to ")+
      "\""+basename+QString().sprintf(".%06u\"",last);
    q=new QSqlQuery(sql,Db());
    delete q;
    CloseMysql();
  }
}
