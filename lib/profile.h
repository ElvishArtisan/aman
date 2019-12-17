// profile.h
//
// Class for reading INI configuration files.
//
// (C) Copyright 2019 Fred Gleason <fredg@paravelsystems.com>
//
//   This program is free software; you can redistribute it and/or modify
//   it under the terms of the GNU Library General Public License 
//   version 2 as published by the Free Software Foundation.
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

#ifndef PROFILE_H
#define PROFILE_H

#include <vector>

#include <QtCore/QString>
#include <QtCore/QTime>

class ProfileLine
{
 public:
  ProfileLine();
  QString tag() const;
  void setTag(QString tag);
  QString value() const;
  void setValue(QString value);
  void clear();

 private:
  QString line_tag;
  QString line_value;
};


class ProfileSection
{
 public:
  ProfileSection();
  QString name() const;
  void setName(QString name);
  bool getValue(QString tag,QString *value) const;
  void addValue(QString tag,QString value);
  void clear();

 private:
  QString section_name;
  std::vector<ProfileLine> section_line;
};


class Profile
{
 public:
  Profile();
  QString source() const;
  bool setSource(QString filename);
  bool setSource(std::vector<QString> *values);
  QString stringValue(QString section,QString tag,
		      QString default_value="",bool *ok=0) const;
  int intValue(QString section,QString tag,
	       int default_value=0,bool *ok=0) const;
  int hexValue(QString section,QString tag,
	       int default_value=0,bool *ok=0) const;
  float floatValue(QString section,QString tag,
		   float default_value=0.0,bool *ok=0) const;
  double doubleValue(QString section,QString tag,
		    double default_value=0.0,bool *ok=0) const;
  bool boolValue(QString section,QString tag,
		 bool default_value=false,bool *ok=0) const;
  QTime timeValue(QString section,QString tag,QTime default_value=QTime(),
		  bool *ok=0);
  void clear();

 private:
  QString profile_source;
  std::vector<ProfileSection> profile_section;
};


#endif  // PROFILE_H
