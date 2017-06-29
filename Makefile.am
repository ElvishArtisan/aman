## automake.am
##
## Top level automake.am for Aman
##
## Use automake to process this into a Makefile.in
##
##   (C) Copyright 2012,2017 Fred Gleason <fredg@paravelsystems.com>
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

if DOCBOOK_AM
  DOCBOOK_AM_OPT = docs
endif

SUBDIRS = lib\
          aman\
          amanctl\
          amand\
          conf\
          $(DOCBOOK_AM_OPT) icons\
          scripts\
          tests\
          xdg

ACLOCAL_AMFLAGS = -I m4

maintainer-clean-local:	
	rm -rf m4

install-exec-local:	
	mkdir -p $(DESTDIR)/etc/init.d
	cp rc.amand $(DESTDIR)/etc/init.d/amand

uninstall-local:
	rm -f $(DESTDIR)/etc/init.d/amand

rpm:	dist
	cp aman-$(VERSION).tar.gz $(HOME)/rpmbuild/SOURCES/
	$(RPMBUILD) -ba --target $(ARCH)-$(VENDOR)-linux aman.spec
	mv $(HOME)/rpmbuild/RPMS/$(ARCH)/aman-*.rpm $(top_srcdir)/
	mv $(HOME)/rpmbuild/SRPMS/aman-*.src.rpm $(top_srcdir)/
	rm $(HOME)/rpmbuild/SOURCES/aman-$(VERSION).tar.gz
	rm -rf $(HOME)/rpmbuild/BUILD/aman-$(VERSION)

EXTRA_DIST = aman.spec.in\
             autogen.sh\
             get_distro.sh\
             get_target.sh\
             PACKAGE_VERSION\
             rc.amand.in

CLEANFILES = *~\
             *.deb\
             *.mo\
             *.rpm\
             *.tar.gz\
             *.tgz\
             *.zip

DISTCLEANFILES = aman.spec\
                 config.guess\
                 config.sub\
                 ltmain.sh\
                 rc.aman

MAINTAINERCLEANFILES = *~\
                       *.tar.gz\
                       aclocal.m4\
                       config.guess\
                       config.status\
                       config.sub\
                       configure\
                       depcomp\
                       install-sh\
                       libtool\
                       ltmain.sh\
                       Makefile.in\
                       missing\
                       mkinstalldirs
