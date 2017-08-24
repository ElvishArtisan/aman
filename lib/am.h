// am.h
//
// Aman System-Wide Defines.
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

#ifndef AM_H
#define AM_H

#define AM_PING_UDP_PORT 7456
#define AM_CMD_TCP_PORT 4342

/*
 * Configuration File
 */
#define AM_CONF_FILE "/etc/aman.conf"

/*
 * Location of the MySQL Bin Logs
 */
#define AM_MYSQL_DATADIR "/var/lib/mysql"

/*
 * mS Between Host-To-Host Pings
 */
#define AM_PING_INTERVAL 1000

/*
 * Host-To-Host Ping Timeout in mS
 */
#define AM_PING_TIMEOUT 10000

/*
 * mS Between MySQL Health Checks
 */
#define AM_MYSQL_MONITOR_INTERVAL 1000

/*
 * mS Between Replication Read Checks
 */
#define AM_REPLICATION_TICK_INTERVAL 10

/*
 * Timeout After This Many Ticks
 */
#define AM_DEFAULT_REPLICATION_TICK_TIMEOUT 60000

/*
 * Location for Cluster State File
 */
#define AM_STATE_FILE "/var/aman/state"
#define AM_TEMP_STATE_FILE "/var/aman/state.temp"

/*
 * Location for MySQL DB Snapshots
 */
#define AM_SNAPSHOT_DIR "/var/aman/snapshots"

/*
 * Default Secure Shell Identity File
 */
#define AM_IDENTITY_FILE "/var/aman/keys/id_dsa"

/*
 * Snapshot File Extension
 */
#define AM_SNAPSHOT_EXT "tar.bz2"

/*
 * Pause Time Between rsync(1) Runs
 */
#define AM_RSYNC_PAUSE_INTERVAL 30000
#define AM_RSYNC_ERROR_PAUSE_INTERVAL 900000

#include <QtCore/QString>

class Am
{
 public:
  enum Instance {This=0,That=1,LastInstance=2};
  enum Command {DisconnectCommand=0,StateCommand=1,GenerateSnapshotCommand=2,
		LoadSnapshotCommand=3,SetMetadataCommand=4,MakeMasterCommand=5,
		MakeSlaveCommand=6,MakeIdleCommand=7,StartAudioSlaveCommand=8,
		StopAudioSlaveCommand=9,PurgeBinLogsCommand=10};
  static QString instanceText(Am::Instance inst);
};


#endif  // AM_H
