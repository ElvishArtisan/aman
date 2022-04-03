## automake.am
##
## Makefile for the amand(8) daemon.
##
## (C) Copyright 2012-2021 Fred Gleason <fredg@paravelsystems.com>
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

AM_CPPFLAGS = -Wall -I$(top_srcdir)/lib -std=c++11 -fPIC -DPREFIX=\"$(prefix)\" @QT5_GUI_CFLAGS@
LIBS = -L$(top_srcdir)/lib
MOC = @QT_MOC@

# The dependency for qt's Meta Object Compiler (moc)
moc_%.cpp:	%.h
	$(MOC) $< -o $@


sbin_PROGRAMS = amand

dist_amand_SOURCES = amand.cpp amand.h\
                     audiostore.cpp\
                     auto_rotate.cpp\
                     generate_snapshot.cpp\
                     pingmonitor.cpp pingmonitor.h\
                     replicationtest.cpp replicationtest.h\
                     restore_snapshot.cpp\
                     set_metadata.cpp\
                     streamcmdserver.cpp streamcmdserver.h

nodist_amand_SOURCES = moc_amand.cpp\
                       moc_pingmonitor.cpp\
                       moc_replicationtest.cpp\
                       moc_streamcmdserver.cpp

amand_LDADD = @LIB_ALIBS@ @QT5_GUI_LIBS@
amand_LDFLAGS = @LIB_AFLAGS@

CLEANFILES = *~\
             moc_*\
             *.obj\
             *.idb\
             *.pdb\
             *ilk

MAINTAINERCLEANFILES = *~\
                       Makefile.in
