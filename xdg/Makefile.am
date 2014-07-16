## automake.am
##
## xdg/automake.am for Aman
##
##   (C) Copyright 2012-2013 Fred Gleason <fredg@paravelsystems.com>
##
##      $Id: Makefile.am,v 1.2 2013/06/25 16:23:17 cvs Exp $
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
	mkdir -p @prefix@/share/applications
	cp aman-aman.desktop @prefix@/share/applications/
	cp aman-root-pam /etc/pam.d/aman-root
	cp aman-root-consolehelper /etc/security/console.apps/aman-root
	rm -f $(prefix)/bin/aman-root
	ln -s /usr/bin/consolehelper $(prefix)/bin/aman-root

uninstall-local:	
	rm -f /usr/share/applications/aman-aman.desktop
	rm -f /etc/pam.d/aman-root
	rm -f /etc/security/console.apps/aman-root
	rm -f $(prefix)/sbin/aman-root

EXTRA_DIST = aman-aman.desktop\
             aman-root-consolehelper\
             aman-root-pam

CLEANFILES = *~

MAINTAINERCLEANFILES = *~\
                       Makefile.in
