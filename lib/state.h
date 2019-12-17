// state.h
//
// A container class for an Aman State
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

#ifndef STATE_H
#define STATE_H

#include "am.h"
#include "profile.h"

class State
{
 public:
  enum ClusterState {StateOffline=0,StateIdle=1,StateMaster=2,StateSlave=3};
  State();
  ClusterState dbState();
  void setDbState(State::ClusterState state);
  ClusterState audioState();
  void setAudioState(State::ClusterState state);
  QString currentSnapshot(Am::Instance inst);
  void setCurrentSnapshot(Am::Instance inst,const QString &str);
  void purgeSnapshots();
  void clear();
  static QString stateString(ClusterState state);

 private:
  void ReadState();
  void WriteState() const;
  ClusterState db_state;
  ClusterState audio_state;
  QString current_snapshot[Am::LastInstance];
};


#endif  // STATE_H
