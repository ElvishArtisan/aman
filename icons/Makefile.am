## automake.am
##
## icons/automake.am for Aman
##
## (C) Copyright 2012-2013 Fred Gleason <fredg@salemradiolabs.com>
##
##      $Id: Makefile.am,v 1.1 2013/06/17 19:14:24 cvs Exp $
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

install-exec-am:	
	mkdir -p $(DESTDIR)@prefix@/share/icons/hicolor/16x16/apps
	cp aman-16x16.png $(DESTDIR)@prefix@/share/icons/hicolor/16x16/apps/aman.png
	mkdir -p $(DESTDIR)@prefix@/share/icons/hicolor/22x22/apps
	cp aman-22x22.png $(DESTDIR)@prefix@/share/icons/hicolor/22x22/apps/aman.png
	mkdir -p $(DESTDIR)@prefix@/share/icons/hicolor/32x32/apps
	cp aman-32x32.png $(DESTDIR)@prefix@/share/icons/hicolor/32x32/apps/aman.png
	mkdir -p $(DESTDIR)@prefix@/share/icons/hicolor/48x48/apps
	cp aman-48x48.png $(DESTDIR)@prefix@/share/icons/hicolor/48x48/apps/aman.png
	mkdir -p $(DESTDIR)@prefix@/share/icons/hicolor/64x64/apps

uninstall:	
	rm -f $(DESTDIR)@prefix@/share/icons/hicolor/16x16/apps/aman.png
	rm -f $(DESTDIR)@prefix@/share/icons/hicolor/22x22/apps/aman.png
	rm -f $(DESTDIR)@prefix@/share/icons/hicolor/32x32/apps/aman.png
	rm -f $(DESTDIR)@prefix@/share/icons/hicolor/48x48/apps/aman.png

EXTRA_DIST = aman-16x16.png\
             aman-16x16.xpm\
             aman-22x22.png\
             aman-22x22.xpm\
             aman-32x32.png\
             aman-32x32.xpm\
             aman-48x48.png\
             aman-48x48.xpm

CLEANFILES = *~

MAINTAINERCLEANFILES = *~\
                       aclocal.m4\
                       configure\
                       Makefile.in
