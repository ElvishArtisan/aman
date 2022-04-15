// amconfig.h
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

#ifndef AMCONFIG_H
#define AMCONFIG_H

#include <vector>

#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtCore/QDateTime>
#include <QtNetwork/QHostAddress>

#include "am.h"
#include "amprofile.h"

class AMConfig
{
 public:
  enum Address {PublicAddress=0,PrivateAddress=1,LastAddress=2};
  AMConfig(QString filename,bool is_rmt);
  QString globalMysqlDatabase() const;
  QString globalMysqlDriver() const;
  int globalMysqlReplicationTimeout() const;
  bool globalMirrorDeleteAudio() const;
  bool globalAutoRotateBinlogs() const;
  bool globalAutoPurgeBinlogs() const;
  QTime globalAutoRotateTime() const;
  QString globalAlertAddress() const;
  QString globalFromAddress() const;
  int globalNiceLevel() const;
  QString globalMysqlServiceName() const;
  QString hostname(Am::Instance inst) const;
  QString hostname(int n) const;
  QString sitename(Am::Instance inst) const;
  QString sitename(int n) const;
  QString mysqlUsername(Am::Instance inst) const;
  QString mysqlUsername(int n) const;
  QString mysqlPassword(Am::Instance inst) const;
  QString mysqlPassword(int n) const;
  QString mysqlDataDirectory(Am::Instance inst) const;
  QString mysqlDataDirectory(int n) const;
  QString archiveDirectory(Am::Instance inst) const;
  QString archiveDirectory(int n) const;
  QHostAddress address(Am::Instance inst,Address addr) const;
  QHostAddress address(int n,Address addr) const;
  QString pingTablename(Am::Instance inst) const;
  QString pingTablename(int n) const;
  QString secureShellIdentity(Am::Instance inst) const;
  QString secureShellIdentity(int n) const;
  Am::Instance instanceA() const;
  Am::Instance instanceB() const;
  Am::Instance instance(const QString &letter) const;
  bool load();
  void clear();
  static bool copyFile(const QString &srcfile,const QString &dstfile,
		       QString *err_msg);

 private:
  void LoadHost(AMProfile *p,const QString &section);
  bool Validate(AMProfile *p) const;
  QString conf_filename;
  QString conf_global_mysql_database;
  QString conf_global_mysql_driver;
  int conf_global_mysql_replication_timeout;
  bool conf_global_mirror_delete_audio;
  QString conf_global_alert_address;
  QString conf_global_from_address;
  bool conf_global_auto_rotate_binlogs;
  bool conf_global_auto_purge_binlogs;
  QTime conf_global_auto_rotate_time;
  int conf_global_nice_level;
  QString conf_global_mysql_service_name;
  QString conf_hostname[Am::LastInstance];
  QStringList conf_hostnames;
  QString conf_sitename[Am::LastInstance];
  QStringList conf_sitenames;
  QString conf_mysql_username[Am::LastInstance];
  QStringList conf_mysql_usernames;
  QString conf_mysql_password[Am::LastInstance];
  QStringList conf_mysql_passwords;
  QString conf_mysql_data_directory[Am::LastInstance];
  QStringList conf_mysql_data_directories;
  QString conf_archive_directory[Am::LastInstance];
  QStringList conf_archive_directories;
  QHostAddress conf_address[Am::LastInstance][AMConfig::LastAddress];
  QList<QList<QHostAddress> > conf_addresses;
  QString conf_ping_tablename[Am::LastInstance];
  QStringList conf_ping_tablenames;
  QString conf_secure_shell_identity[Am::LastInstance];
  QStringList conf_secure_shell_identities;
  Am::Instance conf_instance_table[Am::LastInstance];
  bool conf_is_remote;
};


#endif  // AMCONFIG_H
