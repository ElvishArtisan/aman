## automake.am
##
## conf/ automake.am for Aman
##
## (C) Copyright 2012,2017 Fred Gleason <fredg@paravelsystems.com>
##
##      $Id: Makefile.am,v 1.7 2013/07/02 21:27:28 cvs Exp $
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

install-exec-local:	
	mkdir -p $(DESTDIR)/etc/security/limits.d
	cp aman-mysql-limits.conf $(DESTDIR)/etc/security/limits.d/

uninstall-local:	
	rm -f $(DESTDIR)/etc/security/limits.d/aman-mysql-limits.conf

EXTRA_DIST = aman.conf-sample\
             aman.cnf-SystemA-rhel7-sample\
             aman.cnf-SystemB-rhel7-sample\
             aman-mysql-limits.conf\
             my.cnf-SystemA-rhel6-sample\
             my.cnf-SystemB-rhel6-sample\
             rsync.xinetd-sample\
             rsyncd.conf-sample

CLEANFILES = *~
DISTCLEANFILES = 
MAINTAINERCLEANFILES = *~ Makefile.in configure aclocal.m4
