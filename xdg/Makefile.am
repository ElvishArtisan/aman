## automake.am
##
## xdg/automake.am for Aman
##
##   (C) Copyright 2012-2019 Fred Gleason <fredg@paravelsystems.com>
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
## Use automake to process this into a Makefile.in

install-exec-local:	
	mkdir -p $(DESTDIR)@prefix@/share/applications
	cp aman-aman.desktop $(DESTDIR)@prefix@/share/applications/
	mkdir -p $(DESTDIR)/etc/pam.d
	cp aman-root-pam $(DESTDIR)/etc/pam.d/aman-root
	mkdir -p $(DESTDIR)/etc/security/console.apps
	cp aman-root-consolehelper $(DESTDIR)/etc/security/console.apps/aman-root
	rm -f $(DESTDIR)@prefix@/bin/aman-root
	ln -s /usr/bin/consolehelper $(DESTDIR)@prefix@/bin/aman-root

uninstall-local:	
	rm -f $(DESTDIR)@prefix@/share/applications/aman-aman.desktop
	rm -f $(DESTDIR)/etc/pam.d/aman-root
	rm -f $(DESTDIR)/etc/security/console.apps/aman-root
	rm -f $(DESTDIR)@prefix@/sbin/aman-root

EXTRA_DIST = aman-aman.desktop\
             aman-root-consolehelper\
             aman-root-pam

CLEANFILES = *~

MAINTAINERCLEANFILES = *~\
                       Makefile.in
