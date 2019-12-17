// amconfig.cpp
//
// A container class for an Aman Configuration
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

#include <errno.h>
#include <syslog.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <QtCore/QStringList>

#include "amconfig.h"

AMConfig::AMConfig(QString filename)
{
  clear();
  conf_filename=filename;
}


QString AMConfig::globalMysqlDatabase() const
{
  return conf_global_mysql_database;
}


QString AMConfig::globalMysqlDriver() const
{
  return conf_global_mysql_driver;
}


int AMConfig::globalMysqlReplicationTimeout() const
{
  return conf_global_mysql_replication_timeout;
}


bool AMConfig::globalMirrorDeleteAudio() const
{
  return conf_global_mirror_delete_audio;
}


bool AMConfig::globalAutoRotateBinlogs() const
{
  return conf_global_auto_rotate_binlogs;
}


bool AMConfig::globalAutoPurgeBinlogs() const
{
  return conf_global_auto_purge_binlogs;
}


QTime AMConfig::globalAutoRotateTime() const
{
  return conf_global_auto_rotate_time;
}


QString AMConfig::globalAlertAddress() const
{
  return conf_global_alert_address;
}


QString AMConfig::globalFromAddress() const
{
  return conf_global_from_address;
}


int AMConfig::globalNiceLevel() const
{
  return conf_global_nice_level;
}


QString AMConfig::globalMysqlServiceName() const
{
  return conf_global_mysql_service_name;
}


QString AMConfig::hostname(Am::Instance inst) const
{
  return conf_hostname[inst];
}


QString AMConfig::mysqlUsername(Am::Instance inst) const
{
  return conf_mysql_username[inst];
}


QString AMConfig::mysqlPassword(Am::Instance inst) const
{
  return conf_mysql_password[inst];
}


QString AMConfig::mysqlDataDirectory(Am::Instance inst) const
{
  return conf_mysql_data_directory[inst];
}


QString AMConfig::archiveDirectory(Am::Instance inst) const
{
  return conf_archive_directory[inst];
}


QHostAddress AMConfig::address(Am::Instance inst,AMConfig::Address addr) const
{
  return conf_address[inst][addr];
}


QString AMConfig::pingTablename(Am::Instance inst) const
{
  return conf_ping_tablename[inst];
}


QString AMConfig::secureShellIdentity(Am::Instance inst) const
{
  return conf_secure_shell_identity[inst];
}


Am::Instance AMConfig::instanceA() const
{
  return conf_instance_table[0];
}


Am::Instance AMConfig::instanceB() const
{
  return conf_instance_table[1];
}


Am::Instance AMConfig::instance(const QString &letter) const
{
  if(letter.toLower()=="a") {
    return instanceA();
  }
  if(letter.toLower()=="b") {
    return instanceB();
  }
  return Am::LastInstance;
}


bool AMConfig::load()
{
  char hostname[PATH_MAX];
  AMProfile *p=new AMProfile();
  if(!p->setSource(conf_filename)) {
    syslog(LOG_ERR,"missing configuration file");
    return false;
  }

  gethostname(hostname,PATH_MAX);
  conf_global_mysql_database=
    p->stringValue("Global","MysqlDatabase","Rivendell");
  conf_global_mysql_driver=
    p->stringValue("Global","MysqlDriver","QMYSQL3");
  conf_global_mysql_replication_timeout=
    p->intValue("Global","MysqlReplicationTimeout",
		AM_DEFAULT_REPLICATION_TICK_TIMEOUT)/
    AM_REPLICATION_TICK_INTERVAL;
  conf_global_mirror_delete_audio=
    p->boolValue("Global","MirrorDeleteAudio","QMYSQL3");
  conf_global_auto_rotate_binlogs=
    p->boolValue("Global","AutoRotateBinlogs",false);
  conf_global_auto_purge_binlogs=
    p->boolValue("Global","AutoPurgeBinlogs",false);
  conf_global_auto_rotate_time=
    p->timeValue("Global","AutoRotateTime",conf_global_auto_rotate_time);
  conf_global_alert_address=p->stringValue("Global","AlertAddress");
  conf_global_from_address=
    p->stringValue("Global","FromAddress",conf_global_from_address);
  conf_global_nice_level=
    p->intValue("Global","NiceLevel",10);
  conf_global_mysql_service_name=
    p->stringValue("Global","MysqlServiceName","mysqld");
  LoadHost(p,"SystemA");
  LoadHost(p,"SystemB");

  bool ret=Validate(p);
  delete p;
  return ret;
}


void AMConfig::clear()
{
  conf_global_mysql_database="";
  conf_global_mysql_driver="";
  conf_global_mysql_replication_timeout=AM_DEFAULT_REPLICATION_TICK_TIMEOUT/
    AM_REPLICATION_TICK_INTERVAL;
  conf_global_mirror_delete_audio=false;
  conf_global_alert_address="";
  conf_global_from_address="noreply@example.com";
  conf_global_auto_rotate_binlogs=false;
  conf_global_auto_purge_binlogs=false;
  conf_global_auto_rotate_time=QTime(3,32,0);
  conf_global_nice_level=10;
  conf_global_mysql_service_name="mysqld";
  for(int i=0;i<2;i++) {
    conf_hostname[i]="";
    conf_mysql_username[i]="";
    conf_mysql_password[i]="";
    conf_archive_directory[i]="";
    conf_mysql_data_directory[i]="";
    conf_ping_tablename[i]="";
    conf_secure_shell_identity[i]="";
    for(int j=0;j<AMConfig::LastAddress;j++) {
      conf_address[i][j]=QHostAddress();
    }
  }
  conf_instance_table[0]=Am::This;
  conf_instance_table[1]=Am::That;
}


bool AMConfig::copyFile(const QString &srcfile,const QString &dstfile,
		      QString *err_msg)
{
  FILE *src=NULL;
  FILE *dst=NULL;
  struct stat st;
  char *data=NULL;
  size_t n;

  if(stat(srcfile.toUtf8(),&st)!=0) {
    *err_msg=strerror(errno);
    return false;
  }
  if((src=fopen(srcfile.toUtf8(),"r"))==NULL) {
    *err_msg=strerror(errno);
    return false;
  }
  if((dst=fopen(dstfile.toUtf8(),"w"))==NULL) {
    *err_msg=strerror(errno);
    fclose(src);
    return false;
  }
  data=new char[st.st_blksize];
  while((n=fread(data,1,st.st_blksize,src))>0) {
    fwrite(data,1,n,dst);
  }
  delete data;
  fclose(dst);
  fclose(src);
  *err_msg="ok";

  return true;
}


void AMConfig::LoadHost(AMProfile *p,const QString &section)
{
  char fullname[PATH_MAX];
  int host=1;

  gethostname(fullname,PATH_MAX);
  QString hostname=QString(fullname).split(".")[0];
  if(p->stringValue(section,"Hostname")==hostname) {
    host=0;
    if(section=="SystemA") {
      conf_instance_table[0]=Am::This;
      conf_instance_table[1]=Am::That;
    }
    else {
      conf_instance_table[0]=Am::That;
      conf_instance_table[1]=Am::This;
    }
  }
  conf_hostname[host]=p->stringValue(section,"Hostname");
  conf_mysql_username[host]=p->stringValue(section,"MysqlUsername","repl");
  conf_mysql_password[host]=p->stringValue(section,"MysqlPassword","repl");
  conf_mysql_data_directory[host]=
    p->stringValue(section,"MysqlDataDirectory","/var/lib/mysql");
  conf_archive_directory[host]=p->stringValue(section,"ArchiveDirectory");
  conf_address[host][AMConfig::PublicAddress].
    setAddress(p->stringValue(section,"PublicAddress"));
  conf_address[host][AMConfig::PrivateAddress].
    setAddress(p->stringValue(section,"PrivateAddress"));
  conf_ping_tablename[host]=p->stringValue(section,"PingTablename",
			  QString("AMAN_")+section.toUpper()+"_PINGS");
  conf_secure_shell_identity[host]=
    p->stringValue(section,"SecureShellIdentity",AM_IDENTITY_FILE);
}


bool AMConfig::Validate(AMProfile *p) const
{
  bool ret=true;

  if(conf_hostname[Am::This].isEmpty()) {
    syslog(LOG_ERR,"we are neither SystemA nor SystemB");
    ret=false;
  }
  else {
    if(conf_hostname[Am::This]==p->stringValue("SystemA","Hostname")) {
      syslog(LOG_DEBUG,"we are SystemA");
    }
    else {
      syslog(LOG_DEBUG,"we are SystemB");
    }
  }
  if(conf_hostname[0]==conf_hostname[1]) {
    syslog(LOG_ERR,"SystemA and SystemB hostnames cannot match");
    ret=false;
  }
  if(conf_address[0][AMConfig::PublicAddress]==
     conf_address[1][AMConfig::PublicAddress]) {
    syslog(LOG_ERR,"SystemA and SystemB public addresses cannot match");
    ret=false;
  }
  if(conf_address[0][AMConfig::PrivateAddress]==
     conf_address[1][AMConfig::PrivateAddress]) {
    syslog(LOG_ERR,"SystemA and SystemB private addresses cannot match");
    ret=false;
  }
  if(conf_ping_tablename[0]==conf_ping_tablename[1]) {
    syslog(LOG_ERR,"SystemA and SystemB table names cannot match");
    ret=false;
  }

  return ret;
}
