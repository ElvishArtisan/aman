// aman.h
//
// aman(1) Monitoring Client.
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

#ifndef AMAN_H
#define AMAN_H

#define AMAN_USAGE "\n"

#include <QWidget>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>

#include <amconfig.h>
#include <amstatuslight.h>
#include <amconnection.h>
#include <amprogressdialog.h>

class MainWidget : public QWidget
{
 Q_OBJECT;
 public:
  MainWidget(QWidget *parent=0);
  QSize sizeHint() const;

 private slots:
  void connectedData(int inst);
  void disconnectedData(int inst);
  void makeDbMasterData(int inst);
  void makeDbSlaveData(int inst);
  void makeDbIdleData(int inst);
  void startAudioData(int inst);
  void stopAudioData(int inst);
  void statusChangedData(AMStatus *a,AMStatus *b);
  void showConnectionError(const QString &str);

  void snapshotGeneratedData(const QString &name);
  void loadSnapshotData();
  void snapshotLoadedData(const QString &name);

 protected:
  void resizeEvent(QResizeEvent *e);
  void paintEvent(QPaintEvent *e);

 private:
  void UpdateStatus(int sys,AMStatus *s);
  void EnableFields(int inst,bool state);
  QLabel *am_database_label;
  QLabel *am_audio_label;
  QLabel *am_system_label[2];
  QLabel *am_hostname_label[2];
  QLineEdit *am_hostname_edit[2];
  QLabel *am_db_state_label[2];
  QLineEdit *am_db_state_edit[2];
  QLabel *am_service_running_label[2];
  AMStatusLight *am_service_running_light[2];
  QLabel *am_db_running_label[2];
  AMStatusLight *am_db_running_light[2];
  QLabel *am_db_accessible_label[2];
  AMStatusLight *am_db_accessible_light[2];
  QLabel *am_db_replicating_label[2];
  AMStatusLight *am_db_replicating_light[2];
  QPushButton *am_db_master_button[2];
  QPushButton *am_db_slave_button[2];
  QPushButton *am_db_idle_button[2];
  QLabel *am_audio_state_label[2];
  QLineEdit *am_audio_state_edit[2];
  QLabel *am_audio_replicating_label[2];
  AMStatusLight *am_audio_replicating_light[2];
  QPushButton *am_audio_slave_button[2];
  QPushButton *am_audio_idle_button[2];
  AMConnection *am_connection[2];
  int am_connection_table[2];
  AMProgressDialog *am_progress_dialog;
  AMConfig *am_config;
};


#endif  // AMAN_H
