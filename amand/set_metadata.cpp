// set_metadata.cpp
//
// Set MySQL Replication Metadata
//
//   (C) Copyright 2012-2021 Fred Gleason <fredg@paravelsystems.com>
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

#include <QProcess>
#include <QStringList>
#include <QSqlQuery>
#include <QSqlError>

#include "amprofile.h"
#include "amand.h"

bool MainObject::SetMysqlMetadata(const QString &binlog_name,int binlog_pos)
{
  QString sql;
  QSqlQuery *q;
  QStringList args;
  AMConfig::Address addr;

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

  //
  // Set Metadata
  //
  sql="stop slave";
  q=new QSqlQuery(sql,Db());
  delete q;
  sql="reset slave";
  q=new QSqlQuery(sql,Db());
  delete q;
  sql=QString("change master to MASTER_HOST=\"")+
    main_config->address(Am::That,AMConfig::PublicAddress).toString()+"\","+
    "MASTER_USER=\""+main_config->mysqlUsername(Am::This)+"\","+
    "MASTER_PASSWORD=\""+main_config->mysqlPassword(Am::This)+"\","+
    "MASTER_LOG_FILE=\""+binlog_name+"\","+
    "MASTER_LOG_POS="+QString().sprintf("%d",binlog_pos);
  q=new QSqlQuery(sql,Db());
  if(!q->isActive()) {
    syslog(LOG_ERR,"cannot configure replication source in mysql at %s [%s]",
	   main_config->address(Am::This,addr).toString().toUtf8().constData(),
	   q->lastError().text().toUtf8().constData());
    delete q;
    CloseMysql();
    return false;
  }
  delete q;
  sql="start slave";
  q=new QSqlQuery(sql,Db());
  delete q;

  CloseMysql();

  return true;
}
