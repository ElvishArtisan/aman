## automake.am
##
## Makefile for the amand(8) daemon.
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

AM_CPPFLAGS = -Wall -I$(top_srcdir)/lib -DPREFIX=\"$(prefix)\" -std=c++11 -fPIC @QT5_GUI_CFLAGS@
LIBS = -L$(top_srcdir)/lib
MOC = @QT_MOC@

# The dependency for qt's Meta Object Compiler (moc)
moc_%.cpp:	%.h
	$(MOC) $< -o $@


noinst_PROGRAMS = sqltorture

dist_sqltorture_SOURCES = sqltorture.cpp sqltorture.h

nodist_sqltorture_SOURCES = moc_sqltorture.cpp

sqltorture_LDADD = @LIB_ALIBS@ @QT5_GUI_LIBS@ @LIBCURL_LIBS@
sqltorture_LDFLAGS = @LIB_AFLAGS@


CLEANFILES = *~\
             moc_*\
             *.obj\
             *.idb\
             *.pdb\
             *ilk

MAINTAINERCLEANFILES = *~\
                       Makefile.in
