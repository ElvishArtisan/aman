// amprogressdialog.h
//
// Simple progress dialog
//
//   (C) Copyright 2022 Fred Gleason <fredg@paravelsystems.com>
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

#ifndef AMPROGRESSDIALOG_H
#define AMPROGRESSDIALOG_H

#include <QDialog>
#include <QLabel>

class AMProgressDialog : public QDialog
{
 Q_OBJECT;
 public:
  AMProgressDialog(QWidget *parent=0);
  QSize sizeHint() const;

 protected:
  void resizeEvent(QResizeEvent *e);

 private:
  QLabel *d_label;
};


#endif  // AMSTATUSLIGHT_H
