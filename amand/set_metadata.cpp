// set_metadata.cpp
//
// Set MySQL Replication Metadata
//
//   (C) Copyright 2012 Fred Gleason <fredg@paravelsystems.com>
//
//      $Id: set_metadata.cpp,v 1.1 2012/06/22 19:55:27 cvs Exp $
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

#include <QtCore/QProcess>
#include <QtCore/QStringList>
#include <QtSql/QSqlQuery>
#include <QtSql/QSqlError>

#include "profile.h"
#include "amand.h"

bool MainObject::SetMysqlMetadata(const QString &binlog_name,int binlog_pos)
{
  QString sql;
  QSqlQuery *q;
  QStringList args;
  Config::Address addr;

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
  // Set Metadata
  //
  sql="stop slave";
  q=new QSqlQuery(sql,Db());
  delete q;
  sql="reset slave";
  q=new QSqlQuery(sql,Db());
  delete q;
  sql=QString("change master to MASTER_HOST=\"")+
    main_config->address(Am::That,Config::PublicAddress).toString()+"\","+
    "MASTER_USER=\""+main_config->mysqlUsername(Am::This)+"\","+
    "MASTER_PASSWORD=\""+main_config->mysqlPassword(Am::This)+"\","+
    "MASTER_LOG_FILE=\""+binlog_name+"\","+
    "MASTER_LOG_POS="+QString().sprintf("%d",binlog_pos);
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
  delete q;

  CloseMysql();

  return true;
}
