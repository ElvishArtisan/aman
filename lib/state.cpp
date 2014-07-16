// state.cpp
//
// A container class for an Aman State
//
//   (C) Copyright 2012 Fred Gleason <fredg@paravelsystems.com>
//
//      $Id: state.cpp,v 1.6 2013/11/19 00:14:40 cvs Exp $
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

#include "state.h"

State::State()
{
  clear();
  if(!QFile::exists(AM_STATE_FILE)) {
    WriteState();
  }
}


State::ClusterState State::dbState()
{
  ReadState();
  return db_state;
}


void State::setDbState(State::ClusterState state)
{
  db_state=state;
  WriteState();
}


State::ClusterState State::audioState()
{
  ReadState();
  return audio_state;
}


void State::setAudioState(State::ClusterState state)
{
  audio_state=state;
  WriteState();
}


QString State::currentSnapshot(Am::Instance inst)
{
  ReadState();
  return current_snapshot[inst];
}


void State::setCurrentSnapshot(Am::Instance inst,const QString &str)
{
  if(!str.isEmpty()) {
    current_snapshot[inst]=str;
    WriteState();
  }
}


void State::purgeSnapshots()
{
  ReadState();
  QDir *dir=new QDir(AM_SNAPSHOT_DIR,QString("*.")+AM_SNAPSHOT_EXT);
  QStringList files=dir->entryList();
  for(int i=0;i<files.size();i++) {
    if((files[i]!=current_snapshot[Am::This])&&
       (files[i]!=current_snapshot[Am::That])) {
      dir->remove(files[i]);
      syslog(LOG_INFO,"purged snapshot \"%s\"",
	     (const char *)files[i].toAscii());
    }
  }

  delete dir;
}


void State::clear()
{
  db_state=State::StateIdle;
  audio_state=State::StateIdle;
  for(int i=0;i<Am::LastInstance;i++) {
    current_snapshot[i]="";
  }
}


QString State::stateString(ClusterState state)
{
  QString ret=QObject::tr("OFFLINE");

  switch(state) {
  case State::StateOffline:
    ret=QObject::tr("OFFLINE");
    break;

  case State::StateIdle:
    ret=QObject::tr("IDLE");
    break;

  case State::StateMaster:
    ret=QObject::tr("MASTER");
    break;

  case State::StateSlave:
    ret=QObject::tr("SLAVE");
    break;
  }

  return ret;
}


void State::ReadState()
{
  Profile *p=new Profile();

  if(!p->setSource(AM_STATE_FILE)) {
    db_state=State::StateIdle;
    audio_state=State::StateIdle;
    syslog(LOG_CRIT,"unable to read state file");
    return;
  }
  db_state=
    (State::ClusterState)p->intValue("State","Database",State::StateIdle);
  audio_state=
    (State::ClusterState)p->intValue("State","Audio",State::StateIdle);
  current_snapshot[Am::This]=p->stringValue("State","ThisCurrentSnapshot");
  current_snapshot[Am::That]=p->stringValue("State","ThatCurrentSnapshot");

  delete p;
}


void State::WriteState() const
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
	  (const char *)current_snapshot[Am::This].toAscii());
  fprintf(f,"ThatCurrentSnapshot=%s\n",
	  (const char *)current_snapshot[Am::That].toAscii());
  fclose(f);

  if(rename(AM_TEMP_STATE_FILE,AM_STATE_FILE)!=0) {
    syslog(LOG_CRIT,"unable to write state file [%s]",strerror(errno));
  }
}
