// streamcmdserver.cpp
//
// Parse commands on connection-oriented protocols.
//
//   (C) Copyright 2012,2017 Fred Gleason <fredg@paravelsystems.com>
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

#include <vector>

#include "am.h"
#include "streamcmdserver.h"

StreamCmdServer::StreamCmdServer(const std::map<int,QString> &cmd_table,
				 const std::map<int,int> &upper_table,
				 const std::map<int,int> &lower_table,
				 QTcpServer *server,QObject *parent)
  : QObject(parent)
{
  //
  // Tables
  //
  cmd_cmd_table=cmd_table;
  cmd_upper_table=upper_table;
  cmd_lower_table=lower_table;

  //
  // TCP Server
  //
  cmd_server=server;
  connect(cmd_server,SIGNAL(newConnection()),this,SLOT(newConnectionData()));

  //
  // Read Mapper
  //
  cmd_read_mapper=new QSignalMapper(this);
  connect(cmd_read_mapper,SIGNAL(mapped(int)),this,SLOT(readyReadData(int)));

  //
  // Garbage Timer
  //
  cmd_garbage_timer=new QTimer(this);
  connect(cmd_garbage_timer,SIGNAL(timeout()),this,SLOT(collectGarbageData()));
  cmd_garbage_timer->start(10000);
}


StreamCmdServer::~StreamCmdServer()
{
  delete cmd_garbage_timer;
  delete cmd_read_mapper;
  delete cmd_server;
}


void StreamCmdServer::sendCommand(int id,int cmd,const QStringList &args)
{
  QString str=cmd_cmd_table[cmd];
  for(int i=0;i<args.size();i++) {
    str+=QString(" ")+args[i];
  }
  str+="!";
  cmd_sockets[id]->write(str.toUtf8());
}


void StreamCmdServer::sendCommand(int cmd,const QStringList &args)
{
  for(std::map<int,QTcpSocket *>::iterator it=cmd_sockets.begin();
      it!=cmd_sockets.end();it++) {
    if(it->second!=NULL) {
      if(it->second->state()==QAbstractSocket::ConnectedState) {
	sendCommand(it->first,cmd,args);
      }
      else {
	closeConnection(it->first);
      }
    }
  }
}


void StreamCmdServer::closeConnection(int id)
{
  cmd_sockets[id]->deleteLater();
  cmd_sockets[id]=NULL;
  cmd_recv_buffers[id]="";
}


void StreamCmdServer::newConnectionData()
{
  QTcpSocket *conn=cmd_server->nextPendingConnection();
  cmd_sockets[conn->socketDescriptor()]=conn;
  cmd_recv_buffers[conn->socketDescriptor()]="";
  cmd_read_mapper->setMapping(conn,conn->socketDescriptor());
  connect(conn,SIGNAL(readyRead()),cmd_read_mapper,SLOT(map()));
}


void StreamCmdServer::readyReadData(int id)
{
  int n=-1;
  char data[1500];

  n=cmd_sockets[id]->read(data,1500);
  for(int i=0;i<n;i++) {
    if(data[i]=='!') {
      ProcessCommand(id);
      cmd_recv_buffers[id]="";
    }
    else {
      if((data[i]!=10)&&(data[i]!=13)) {
	cmd_recv_buffers[id]+=data[i];
      }
    }
  }
}


void StreamCmdServer::collectGarbageData()
{
  std::vector<int> ids;

  for(std::map<int,QTcpSocket *>::iterator it=cmd_sockets.begin();
      it!=cmd_sockets.end();it++) {
    if(it->second==NULL) {
      ids.push_back(it->first);
      cmd_sockets.erase(it);
    }
  }
  for(std::map<int,QString>::iterator it=cmd_recv_buffers.begin();
      it!=cmd_recv_buffers.end();it++) {
    for(unsigned i=0;i<ids.size();i++) {
      if(it->first==ids[i]) {
	cmd_recv_buffers.erase(it);
      }
    }
  }
}


void StreamCmdServer::ProcessCommand(int id)
{
  QStringList cmds=cmd_recv_buffers[id].split(" ");
  for(unsigned i=0;i<cmd_cmd_table.size();i++) {
    if(cmd_cmd_table[i]==cmds[0]) {
      if(((cmd_upper_table[i]<0)||((cmds.size()-1)<=cmd_upper_table[i]))&&
	 ((cmd_lower_table[i]<0)||((cmds.size()-1)>=cmd_lower_table[i]))) {
	QStringList args;
	for(int j=1;j<cmds.size();j++) {
	  args.push_back(cmds[j]);
	}
	emit commandReceived(id,i,args);
      }
    }
  }
}
