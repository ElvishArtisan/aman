// sqltorture.h
//
// Stree-test a MySQL server with write quiries.
//
//   (C) Copyright 2017 Fred Gleason <fredg@paravelsystems.com>
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

#ifndef SQLTORTURE_H
#define SQLTORTURE_H

#define SQLTORTURE_USAGE "--mysql-hostname=<hostname> --mysql-username=<username> --mysql-password=<passwd> --mysql-database=<dbname>  --mysql-tablename=<tablename>\n"
#define SQLTORTURE_PID_FILE "/var/run/sqltorture.pid"

#include <QtCore/QObject>
#include <QtSql/QSqlDatabase>

class MainObject : public QObject
{
 Q_OBJECT;
 public:
  MainObject(QObject *parent=0);
};


#endif  // SQLTORTURE_H
