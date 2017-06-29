## automake.am
##
## Automake.am for callcommander/libp
##
## by Fred Gleason <fredg@paravelsystems.com>
##
## (C) Copyright 2012,2017 Fred Gleason <fredg@paravelsystems.com>
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

AM_CPPFLAGS = -Wall @QT4_CLI_CFLAGS@
MOC = @MOC@

# The dependency for qt's Meta Object Compiler (moc)
moc_%.cpp:	%.h
	$(MOC) $< -o $@

clean-local:	
	rm -rf debug release

#instdir = @LOCAL_PREFIX@/lib

lib_LTLIBRARIES = libaman.la
dist_libaman_la_SOURCES = am.cpp am.h\
                          cmdswitch.cpp cmdswitch.h\
                          config.cpp config.h\
                          profile.cpp profile.h\
                          state.cpp state.h

nodist_libaman_la_SOURCES = 

libaman_la_LDFLAGS = -release $(VERSION)

EXTRA_DIST = 

CLEANFILES = *~\
             moc_*

DISTCLEANFILES = *~\
                 moc_* 

MAINTAINERCLEANFILES = *~\
                       Makefile.in\
                       moc_*
