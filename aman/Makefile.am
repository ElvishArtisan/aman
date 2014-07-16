## automake.am
##
## Makefile for the aman(1) client.
##
## (C) Copyright 2012 Fred Gleason <fredg@paravelsystems.com>
##
##      $Id: Makefile.am,v 1.2 2012/12/19 00:09:11 cvs Exp $
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

AM_CPPFLAGS = -Wall -I$(top_srcdir)/lib -DPREFIX=\"$(prefix)\"
LIBS = -L$(top_srcdir)/lib

# The dependency for qt's Meta Object Compiler (moc)
moc_%.cpp:	%.h
	@MOC@ $< -o $@


bin_PROGRAMS = aman

dist_aman_SOURCES = aman.cpp aman.h\
                    connection.cpp connection.h\
                    statuslight.cpp statuslight.h

nodist_aman_SOURCES = moc_aman.cpp\
                      moc_connection.cpp\
                      moc_statuslight.cpp

aman_LDADD = @LIB_ALIBS@
aman_LDFLAGS = @LIB_AFLAGS@

CLEANFILES = *~\
             moc_*

DISTCLEANFILES = *~\
                 moc_*

MAINTAINERCLEANFILES = *~\
                       Makefile.in
