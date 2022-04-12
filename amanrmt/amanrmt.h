// amanrmt.h
//
// amanrmt(1) Monitoring Client.
//
//   (C) Copyright 2012-2022 Fred Gleason <fredg@paravelsystems.com>
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

#ifndef AMANRMT_H
#define AMANRMT_H

#define AMANRMT_USAGE "\n"

#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QProcess>
#include <QPushButton>
#include <QTimer>
#include <QWidget>

#include <amconfig.h>
#include <amconnection.h>
#include <amprogressdialog.h>
#include <amstatuslight.h>

class MainWidget : public QWidget
{
 Q_OBJECT;
 public:
  MainWidget(QWidget *parent=0);
  QSize sizeHint() const;

 private slots:
  void connectedData(int inst);
  void disconnectedData(int inst);
  void makeDbSlaveData();
  void makeDbIdleData();
  void startAudioData();
  void stopAudioData();
  void statusChangedData(AMStatus *a,AMStatus *b);
  void snapshotGeneratedData(const QString &name);
  void loadSnapshotData();
  void snapshotLoadedData(const QString &name);
  void checkDbReplicationData();
  void startAudioProcessData();
  void audioProcessFinishedData(int exit_code,QProcess::ExitStatus status);
  void showConnectionError(const QString &str);

 protected:
  void closeEvent(QCloseEvent *e);
  void resizeEvent(QResizeEvent *e);
  void paintEvent(QPaintEvent *e);

 private:
  void UpdateSourceStatus(int sys,AMStatus *s);
  void EnableSourceFields(int inst,bool state);
  void DrawFrame(QPainter *p,const QRect &rect,QLabel *label);
  bool RestoreMysqlSnapshot(const QString &filename,QString *binlog,int *pos,
			    int src_sys,QString *err_msg);
  int GetMasterServerId() const;
  bool OpenDb(QString *err_msg);
  void CloseDb();
  QString MakeTempDir();
  QLabel *am_source_label;
  QLabel *am_src_system_label[2];
  QLabel *am_src_hostname_label[2];
  QLineEdit *am_src_hostname_edit[2];
  QLabel *am_src_db_state_label[2];
  QLineEdit *am_src_db_state_edit[2];
  QLabel *am_src_db_replicating_label[2];
  AMStatusLight *am_src_db_replicating_light[2];
  QLabel *am_src_audio_state_label[2];
  QLineEdit *am_src_audio_state_edit[2];
  QLabel *am_src_audio_replicating_label[2];
  AMStatusLight *am_src_audio_replicating_light[2];
  QLabel *am_destination_label;
  QLabel *am_dst_db_state_label;
  QLineEdit *am_dst_db_state_edit;
  QLabel *am_dst_db_replicating_label;
  AMStatusLight *am_dst_db_replicating_light;
  QLabel *am_dst_audio_state_label;
  QLineEdit *am_dst_audio_state_edit;
  QLabel *am_dst_audio_replicating_label;
  AMStatusLight *am_dst_audio_replicating_light;
  QPushButton *am_db_slave_button;
  QPushButton *am_db_idle_button;
  QPushButton *am_audio_slave_button;
  QPushButton *am_audio_idle_button;
  AMConnection *am_connection[2];
  AMConnection *am_active_connection;
  int am_connection_table[2];
  AMProgressDialog *am_progress_dialog;
  AMConfig *am_config;
  AMState *am_state;
  QTimer *am_check_db_replication_timer;
  unsigned am_check_db_prev_ping;
  bool am_audio_active;
  QProcess *am_audio_process;
  QTimer *am_audio_timer;
  bool am_exiting;
};


#endif  // AMAN_H
