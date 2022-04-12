// amprogressdialog.cpp
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

#include "amprogressdialog.h"

AMProgressDialog::AMProgressDialog(QWidget *parent)
  : QDialog(parent)
{
  setModal(false);

  setWindowTitle(tr("Rivendell Server Manager"));

  QFont bold_font=font();
  bold_font.setPixelSize(36);

  d_label=new QLabel(tr("Please wait..."),this);
  d_label->setAlignment(Qt::AlignCenter|Qt::AlignVCenter);
  d_label->setFont(bold_font);  

  hide();
}


QSize AMProgressDialog::sizeHint() const
{
  return QSize(4*d_label->sizeHint().width()/3,
	       3*d_label->sizeHint().height()/2);
}


void AMProgressDialog::resizeEvent(QResizeEvent *e)
{
  d_label->setGeometry(0,0,size().width(),size().height());
}
