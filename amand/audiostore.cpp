// audiostore.cpp
//
// Audio Store routines for Aman
//
//   (C) Copyright 2012-2013 Fred Gleason <fredg@paravelsystems.com>
//
//      $Id: audiostore.cpp,v 1.6 2013/07/09 15:22:28 cvs Exp $
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
#include <sys/types.h>
#include <signal.h>

#include <QtCore/QProcess>
#include <QtCore/QStringList>

#include "amand.h"

void MainObject::rsyncFinishedData(int exitcode,QProcess::ExitStatus status)
{
  if(status==QProcess::CrashExit) {
    syslog(LOG_WARNING,"rsync(1) process crashed");
    ScheduleAudioCopy(AM_RSYNC_ERROR_PAUSE_INTERVAL);
    return;
  }

  switch(main_audio_process->exitCode()) {
  case 0:   // Normal Exit
  case 20:  // Stopped due to signal
  case 24:  // Partial transfer due to vanished source files
    main_audio_holdoff_timer->start(AM_RSYNC_PAUSE_INTERVAL);
    break;

  case 23:  // Partial transfer due to error
    syslog(LOG_NOTICE,"rsync(1) process failed to transfer all files");
    main_audio_holdoff_timer->start(AM_RSYNC_PAUSE_INTERVAL);
    break;
 
  default:
    syslog(LOG_WARNING,"rsync(1) process returned an error, exit code %d",
	   main_audio_process->exitCode());
    SetAudioStatus(false);
    main_audio_holdoff_timer->start(AM_RSYNC_ERROR_PAUSE_INTERVAL);
    return;
  }
}


void MainObject::rsyncErrorData(QProcess::ProcessError err)
{
  QString str=tr("rsync(1) process returned unknown error");
  switch(err) {
  case QProcess::FailedToStart:
    str=tr("rsync(1) process failed to start");
    break;

  case QProcess::Crashed:
    str=tr("rsync(1) process crashed");
    break;

  case QProcess::Timedout:
    str=tr("rsync(1) process timed out");
    break;

  case QProcess::WriteError:
    str=tr("rsync(1) process returned write error");
    break;

  case QProcess::ReadError:
    str=tr("rsync(1) process returned read error");
    break;

  case QProcess::UnknownError:
    break;
  }
  syslog(LOG_WARNING,str.toAscii());
}

void MainObject::startAudioCopy()
{
  QStringList args;
  QProcess *p=new QProcess(this);

  if(main_state->audioState()!=State::StateSlave) {
    return;
  }

  //
  // Check that the master is alive and ready
  //
  args.push_back(main_config->address(Am::That,Config::PrivateAddress).
		 toString()+"::rivendell/repl.chk");
  p->start("rsync",args);
  p->waitForFinished(30000);
  switch(p->exitCode()) {
  case 0:
    SetAudioStatus(true);
    break;

  case 23:   // Check file not found
  case 30:   // Data timeout
  case 35:   // Connection timeout
    syslog(LOG_NOTICE,"master audio store not available, deferring sync");
    delete p;
    SetAudioStatus(false);
    ScheduleAudioCopy(AM_RSYNC_ERROR_PAUSE_INTERVAL);
    return;
    
  default:
    syslog(LOG_WARNING,"rsync(1) process returned an error, exit code %d",
	   p->exitCode());
    delete p;
    SetAudioStatus(false);
    ScheduleAudioCopy(AM_RSYNC_ERROR_PAUSE_INTERVAL);
    return;
  }

  //
  // Start the main file copy
  //
  args.clear();
  args.push_back("-a");
  if(main_config->globalMirrorDeleteAudio()) {
    args.push_back("--delete");
  }
  args.push_back(main_config->address(Am::That,Config::PrivateAddress).
		 toString()+"::rivendell/*.wav");
  args.push_back("/var/snd/");
  main_audio_process->start("rsync",args);
}


void MainObject::ScheduleAudioCopy(int msecs)
{
  if(main_state->audioState()==State::StateSlave) {
    main_audio_holdoff_timer->start(msecs);
  }
}


void MainObject::StopAudioCopy()
{
  if(main_audio_process->state()!=QProcess::NotRunning) {
    kill(main_audio_process->pid(),SIGTERM);
    main_monitor->setThisAudioStatus(false);
  }
}


void MainObject::SetAudioStatus(bool status)
{
  if(main_monitor->audioStatus(Am::This)!=status) {
    if(status) {
      SendAlert("Audio replication has RESUMED between servers \""+
		main_config->hostname(Am::This)+"\" and \""+
		main_config->hostname(Am::That)+"\".");
    }
    else {
      SendAlert("Audio replication has STOPPED between servers \""+
		main_config->hostname(Am::This)+"\" and \""+
		main_config->hostname(Am::That)+"\".");
    }
    main_monitor->setThisAudioStatus(status);
    main_cmd_server->sendCommand(Am::StateCommand,StateUpdateArgs());
  }
}
