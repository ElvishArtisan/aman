// amstate.cpp
//
// A container class for an Aman State
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

#include <stdio.h>
#include <syslog.h>
#include <errno.h>

#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QObject>

#include "amstate.h"
#include "amprofile.h"

AMState::AMState()
{
  clear();
  if(!QFile::exists(AM_STATE_FILE)) {
    WriteState();
  }
}


AMState::ClusterState AMState::dbState()
{
  ReadState();
  return db_state;
}


void AMState::setDbState(AMState::ClusterState state)
{
  db_state=state;
  WriteState();
}


AMState::ClusterState AMState::audioState()
{
  ReadState();
  return audio_state;
}


void AMState::setAudioState(AMState::ClusterState state)
{
  audio_state=state;
  WriteState();
}


QString AMState::currentSnapshot(Am::Instance inst)
{
  ReadState();
  return current_snapshot[inst];
}


void AMState::setCurrentSnapshot(Am::Instance inst,const QString &str)
{
  if(!str.isEmpty()) {
    current_snapshot[inst]=str;
    WriteState();
  }
}


void AMState::purgeSnapshots()
{
  ReadState();
  QDir *dir=new QDir(AM_SNAPSHOT_DIR,QString("*.")+AM_SNAPSHOT_EXT);
  QStringList files=dir->entryList();
  for(int i=0;i<files.size();i++) {
    if((files[i]!=current_snapshot[Am::This])&&
       (files[i]!=current_snapshot[Am::That])) {
      dir->remove(files[i]);
      syslog(LOG_INFO,"purged snapshot \"%s\"",
	     files[i].toUtf8().constData());
    }
  }

  delete dir;
}


void AMState::clear()
{
  db_state=AMState::StateIdle;
  audio_state=AMState::StateIdle;
  for(int i=0;i<Am::LastInstance;i++) {
    current_snapshot[i]="";
  }
}


QString AMState::stateString(ClusterState state)
{
  QString ret=QObject::tr("OFFLINE");

  switch(state) {
  case AMState::StateOffline:
    ret=QObject::tr("OFFLINE");
    break;

  case AMState::StateIdle:
    ret=QObject::tr("IDLE");
    break;

  case AMState::StateMaster:
    ret=QObject::tr("MASTER");
    break;

  case AMState::StateSlave:
    ret=QObject::tr("SLAVE");
    break;
  }

  return ret;
}


void AMState::ReadState()
{
  AMProfile *p=new AMProfile();

  if(!p->setSource(AM_STATE_FILE)) {
    db_state=AMState::StateIdle;
    audio_state=AMState::StateIdle;
    syslog(LOG_CRIT,"unable to read state file");
    return;
  }
  db_state=
    (AMState::ClusterState)p->intValue("State","Database",AMState::StateIdle);
  audio_state=
    (AMState::ClusterState)p->intValue("State","Audio",AMState::StateIdle);
  current_snapshot[Am::This]=p->stringValue("State","ThisCurrentSnapshot");
  current_snapshot[Am::That]=p->stringValue("State","ThatCurrentSnapshot");

  delete p;
}


void AMState::WriteState() const
{
  FILE *f=NULL;

  if((f=fopen(AM_TEMP_STATE_FILE,"w"))==NULL) {
    syslog(LOG_CRIT,"unable to write state file [%s]",strerror(errno));
    return;
  }
  fprintf(f,"[State]\n");
  fprintf(f,"Database=%d\n",db_state);
  fprintf(f,"Audio=%d\n",audio_state);
  fprintf(f,"ThisCurrentSnapshot=%s\n",
	  current_snapshot[Am::This].toUtf8().constData());
  fprintf(f,"ThatCurrentSnapshot=%s\n",
	  current_snapshot[Am::That].toUtf8().constData());
  fclose(f);

  if(rename(AM_TEMP_STATE_FILE,AM_STATE_FILE)!=0) {
    syslog(LOG_CRIT,"unable to write state file [%s]",strerror(errno));
  }
}
