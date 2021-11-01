// amanctl.cpp
//
// amanctl(8) Control Aman from the command line
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

#include <QByteArray>
#include <QCoreApplication>
#include <QStringList>

#include <am.h>

#include <amcmdswitch.h>

#include "amanctl.h"

MainObject::MainObject(QObject *parent)
  : QObject(parent)
{
  ctl_command="GET_STATE";
  ctl_system=-1;

  AMCmdSwitch *cmd=new AMCmdSwitch("amanctl",AMANCTL_USAGE);
  for(unsigned i=0;i<cmd->keys();i++) {
    if(cmd->key(i)=="--command") {
      if((cmd->value(i).toUpper()!="GET_STATE")&&
	 (cmd->value(i).toUpper()!="MAKE_DB_IDLE")&&
	 (cmd->value(i).toUpper()!="MAKE_DB_MASTER")&&
	 (cmd->value(i).toUpper()!="MAKE_DB_SLAVE")&&
	 (cmd->value(i).toUpper()!="MAKE_AUDIO_IDLE")&&
	 (cmd->value(i).toUpper()!="MAKE_AUDIO_SLAVE")){
	fprintf(stderr,"amanctl: unrecognized command \"%s\"\n",
		(const char *)cmd->value(i).toUtf8());
	exit(1);
      }
      ctl_command=cmd->value(i);
      cmd->setProcessed(i,true);
    }
    if(cmd->key(i)=="--system") {
      if((cmd->value(i).toUpper()!="A")&&(cmd->value(i).toUpper()!="B")) {
	fprintf(stderr,"amanctl: invalid value for --system\n");
	exit(1);
      }
      ctl_system=cmd->value(i).toUpper()=="B";
      cmd->setProcessed(i,true);
    }
    if(!cmd->processed(i)) {
      fprintf(stderr,"amanctl: unknown option \"%s\"\n",
	      (const char *)cmd->key(i).toUtf8());
      exit(1);
    }
  }

  ctl_config=new AMConfig(AM_CONF_FILE);
  ctl_config->load();

  ctl_socket=new QTcpSocket(this);
  connect(ctl_socket,SIGNAL(connected()),this,SLOT(connectedData()));
  connect(ctl_socket,SIGNAL(disconnected()),this,SLOT(disconnectedData()));
  connect(ctl_socket,SIGNAL(readyRead()),this,SLOT(readyReadData()));
  connect(ctl_socket,SIGNAL(error(QAbstractSocket::SocketError)),
	  this,SLOT(errorData(QAbstractSocket::SocketError)));
  QHostAddress addr;
  if(ctl_system<0) {
    addr=ctl_config->address(Am::This,AMConfig::PublicAddress);
  }
  else {
    addr=ctl_config->
      address(ctl_config->instance(QString(ctl_system+'a')),AMConfig::PublicAddress);
  }
  ctl_socket->connectToHost(addr,AM_CMD_TCP_PORT);
}


void MainObject::connectedData()
{
  ctl_socket->write("ST!",3);
}


void MainObject::disconnectedData()
{
  fprintf(stderr,"amanctl: server disconnected\n");
  exit(1);
}


void MainObject::readyReadData()
{
  QByteArray data=ctl_socket->readAll();

  for(int i=0;i<data.size();i++) {
    switch(0xff&data[i]) {
    case 13:
    case 10:
      break;

    case '!':
      ProcessResponse(ctl_accum);
      ctl_accum="";
      break;

    default:
      ctl_accum+=data[i];
      break;
    }
  }
}


void MainObject::errorData(QAbstractSocket::SocketError err)
{
  QString err_str=tr("unknown socket error")+QString().sprintf(" [%d]",err);

  fprintf(stderr,"amanctl: %s\n",(const char *)err_str.toUtf8());
  exit(1);
}


void MainObject::ProcessResponse(const QString &str)
{
  QStringList msg=str.split(" ");

  if(msg.at(0)=="ST") {   // Status Update
    if(msg.size()!=17) {
      fprintf(stderr,"amanctl: malformed response [cmd: ST]\n");
      exit(1);
    }
    if(ctl_command=="GET_STATE") {
      printf("<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\n");
      if(ctl_system!=1) {
	printf("<amanStatus>\n");
	printf("  <systemA>\n");
	PrintServerState(msg,1);
	printf("  </systemA>\n");
      }
      if(ctl_system!=0) {
	printf("  <systemB>\n");
	PrintServerState(msg,9);
	printf("  </systemB>\n");
	printf("</amanStatus>\n");
      }
      exit(0);
    }
    if(msg.at(2).toInt()==0) {
      fprintf(stderr,"amanctl: SystemA [%s] service not running\n",
	      (const char *)msg.at(1).toUtf8());
      exit(1);
    }
    if(msg.at(10).toInt()==0) {
      fprintf(stderr,"amanctl: SystemB [%s] service not running\n",
	      (const char *)msg.at(9).toUtf8());
      exit(1);
    }
    if(msg.at(3).toInt()==0) {
      fprintf(stderr,"amanctl: SystemA [%s] DB server not running\n",
	      (const char *)msg.at(1).toUtf8());
      exit(1);
    }
    if(msg.at(11).toInt()==0) {
      fprintf(stderr,"amanctl: SystemB [%s] DB server not running\n",
	      (const char *)msg.at(9).toUtf8());
      exit(1);
    }
    if(msg.at(4).toInt()==0) {
      fprintf(stderr,"amanctl: SystemA [%s] DB is not accessible\n",
	      (const char *)msg.at(1).toUtf8());
      exit(1);
    }
    if(msg.at(12).toInt()==0) {
      fprintf(stderr,"amanctl: SystemB [%s] DB is not accessible\n",
	      (const char *)msg.at(9).toUtf8());
      exit(1);
    }
    ctl_db_states[0]=(MainObject::DbState)msg.at(5).toInt();
    ctl_db_states[1]=(MainObject::DbState)msg.at(13).toInt();
    ctl_audio_states[0]=(MainObject::AudioState)msg.at(7).toInt();
    ctl_audio_states[1]=(MainObject::AudioState)msg.at(15).toInt();

    //
    // Audio Commands
    //
    if(ctl_command=="MAKE_AUDIO_IDLE") {
      if(ctl_system<0) {
	fprintf(stderr,"amanctl: no system specified\n");
	exit(1);
      }
      ctl_socket->write("AI!",3);
      qApp->processEvents();
      exit(0);
    }

    if(ctl_command=="MAKE_AUDIO_SLAVE") {
      if(ctl_system<0) {
	fprintf(stderr,"amanctl: no system specified\n");
	exit(1);
      }
      if(ctl_audio_states[!ctl_system]==MainObject::AudioSlave) {
	fprintf(stderr,"amanctl: other system not in AUDIO_IDLE state\n");
	exit(1);
      }
      ctl_socket->write("AS!",3);
      qApp->processEvents();
      exit(0);
    }

    //
    // Database Commands
    //
    if(ctl_command=="MAKE_DB_IDLE") {
      if(ctl_system<0) {
	fprintf(stderr,"amanctl: no system specified\n");
	exit(1);
      }
      if((ctl_db_states[ctl_system]==MainObject::DbMaster)&&
	 (ctl_db_states[!ctl_system]!=MainObject::DbIdle)) {
	fprintf(stderr,"amanctl: other system not in IDLE state\n");
	exit(1);
      }
      ctl_socket->write("MI!",3);
      qApp->processEvents();
      exit(0);
    }

    if(ctl_command=="MAKE_DB_MASTER") {
      if(ctl_system<0) {
	fprintf(stderr,"amanctl: no system specified\n");
	exit(1);
      }
      if(ctl_db_states[!ctl_system]==MainObject::DbMaster) {
	fprintf(stderr,"amanctl: other system is in MASTER state\n");
	exit(1);
      }
      ctl_socket->write("MM!",3);
      qApp->processEvents();
      exit(0);
    }

    if(ctl_command=="MAKE_DB_SLAVE") {
      if(ctl_system<0) {
	fprintf(stderr,"amanctl: no system specified\n");
	exit(1);
      }
      if(ctl_db_states[!ctl_system]!=MainObject::DbMaster) {
	fprintf(stderr,"amanctl: other system not in MASTER state\n");
	exit(1);
      }
      ctl_socket->write("MS!",3);
      qApp->processEvents();
      exit(0);
    }
  }
  fprintf(stderr,"amanctl: unhandled protocol response [%s]\n",
	  (const char *)str.toUtf8());
  exit(1);
}


void MainObject::PrintServerState(const QStringList &msg,int arg_offset) const
{
  printf("    <hostname>%s</hostname>\n",
	 (const char *)msg.at(arg_offset).toUtf8());
  printf("    <ping>%s</ping>\n",(const char *)msg.at(1+arg_offset).toUtf8());
  printf("    <dbRunning>%s</dbRunning>\n",
	 (const char *)msg.at(2+arg_offset).toUtf8());
  printf("    <dbAccessible>%s</dbAccessible>\n",
	 (const char *)msg.at(3+arg_offset).toUtf8());
  printf("    <dbReplicationState>%s</dbReplicationState>\n",
	 (const char *)DbStateString(msg.at(4+arg_offset).toInt()).toUtf8());
  printf("    <dbReplicationTime>%d</dbReplicationTime>\n",
	 msg.at(5+arg_offset).toInt());
  printf("    <audioReplicationState>%s</audioReplicationState>\n",
	 (const char *)AudioStateString(msg.at(6+arg_offset).toInt()).toUtf8());
  printf("    <audioReplicationActive>%d</audioReplicationActive>\n",
	 msg.at(7+arg_offset).toInt());
}


QString MainObject::DbStateString(int repl_state) const
{
  QString ret="UNKNOWN";

  switch(repl_state) {
  case 0:
    ret="OFFLINE";
    break;

  case 1:
    ret="IDLE";
    break;

  case 2:
    ret="MASTER";
    break;

  case 3:
    ret="SLAVE";
    break;
  }

  return ret;
}


QString MainObject::AudioStateString(int repl_state) const
{
  QString ret="UNKNOWN";

  switch(repl_state) {
  case 1:
    ret="IDLE";
    break;

  case 3:
    ret="SLAVE";
    break;
  }

  return ret;
}


int main(int argc,char *argv[])
{
  QCoreApplication a(argc,argv);
  new MainObject();

  return a.exec();
}
