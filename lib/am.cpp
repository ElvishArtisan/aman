// am.cpp
//
// amand(8) System-Wide Defines.
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

#include <QtCore/QObject>

#include "am.h"

QString Am::instanceText(Am::Instance inst)
{
  QString ret=QObject::tr("Unknown instance");

  switch(inst) {
  case Am::This:
    ret=QObject::tr("This");
    break;

  case Am::That:
    ret=QObject::tr("That");
    break;

  case Am::LastInstance:
    break;
  }

  return ret;
}


bool Am::emailAddressIsValid(const QString &addr)
{
  QStringList f0=addr.split("@",QString::KeepEmptyParts);

  if(f0.size()!=2) {
    return false;
  }
  QStringList f1=f0.last().split(".");
  if(f1.size()<2) {
    return false;
  }
  return true;
}
