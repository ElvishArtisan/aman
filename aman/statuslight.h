// statuslight.h
//
// Status Light Widget
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

#ifndef STATUSLIGHT_H
#define STATUSLIGHT_H

#define AMAN_USAGE "\n"

#include <QEvent>
#include <QWidget>

class StatusLight : public QWidget
{
 Q_OBJECT;
 public:
  StatusLight(QWidget *parent=0);
  QSize sizeHint() const;
  bool status() const;
  void setStatus(bool state);

 protected:
  void changeEvent(QEvent *e);
  void resizeEvent(QResizeEvent *e);
  void paintEvent(QPaintEvent *e);

 private:
  bool status_status;
  bool status_enabled;
};


#endif  // STATUSLIGHT_H
