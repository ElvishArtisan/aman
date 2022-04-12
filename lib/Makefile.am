## automake.am
##
## Automake.am for callcommander/libp
##
## by Fred Gleason <fredg@paravelsystems.com>
##
## (C) Copyright 2012-2022 Fred Gleason <fredg@paravelsystems.com>
##
##   This program is free software; you can redistribute it and/or modify
##   it under the terms of the GNU General Public License version 2 as
##   published by the Free Software Foundation.
##
##   This program is distributed in the hope that it will be useful,
##   but WITHOUT ANY WARRANTY; without even the implied warranty of
##   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
##   GNU General Public License for more details.
##
##   You should have received a copy of the GNU General Public
##   License along with this program; if not, write to the Free Software
##   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
##
##
## Use automake to process this into a Makefile.in

AM_CPPFLAGS = -Wall -std=c++11 -fPIC @QT5_GUI_CFLAGS@ @LIBCURL_CFLAGS@
MOC = @QT_MOC@

# The dependency for qt's Meta Object Compiler (moc)
moc_%.cpp:	%.h
	$(MOC) $< -o $@

clean-local:	
	rm -rf debug release

#instdir = @LOCAL_PREFIX@/lib

lib_LTLIBRARIES = libaman.la
dist_libaman_la_SOURCES = amconnection.cpp amconnection.h\
                          am.cpp am.h\
                          amcmdswitch.cpp amcmdswitch.h\
                          amconfig.cpp amconfig.h\
                          amprofile.cpp amprofile.h\
                          amprogressdialog.cpp amprogressdialog.h\
                          amstate.cpp amstate.h\
                          amstatuslight.cpp amstatuslight.h

nodist_libaman_la_SOURCES = moc_amconnection.cpp\
                            moc_amprogressdialog.cpp\
                            moc_amstatuslight.cpp

libaman_la_LDFLAGS = -release $(VERSION)

EXTRA_DIST = 

CLEANFILES = *~\
             moc_*

DISTCLEANFILES = *~\
                 moc_* 

MAINTAINERCLEANFILES = *~\
                       Makefile.in\
                       moc_*
