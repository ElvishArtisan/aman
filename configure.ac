dnl configure.in
dnl
dnl Autoconf configuration for Aman.
dnl
dnl Use autoconf to process this into a configure script
dnl
dnl   (C) Copyright 2012-2022 Fred Gleason <fredg@paravelsystems.com>
dnl
dnl   This program is free software; you can redistribute it and/or modify
dnl   it under the terms of the GNU General Public License version 2 as
dnl   published by the Free Software Foundation.
dnl
dnl   This program is distributed in the hope that it will be useful,
dnl   but WITHOUT ANY WARRANTY; without even the implied warranty of
dnl   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
dnl   GNU General Public License for more details.
dnl
dnl   You should have received a copy of the GNU General Public
dnl   License along with this program; if not, write to the Free Software
dnl   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
dnl

AC_INIT(aman,m4_esyscmd(cat PACKAGE_VERSION))
AM_INIT_AUTOMAKE
AC_SUBST(RPM_RELEASE,1)
m4_ifdef([AM_SILENT_RULES],[AM_SILENT_RULES([yes])])
AC_CONFIG_MACRO_DIR([m4])

#
# Some Fully Resolved Install Paths
# (for the packaging system and friends)
#
if test ${prefix} = "NONE" ; then
  AC_SUBST(LOCAL_PREFIX,/usr/local)
else
  AC_SUBST(LOCAL_PREFIX,${prefix})
fi

#
# Basic Compiler Checks
#
AC_PROG_CXX
AC_PROG_LIBTOOL
AC_LANG(C++)

# to avoid rpath usage :
# http://wiki.debian.net/index.cgi?RpathIssue
case ${host} in
  *-pc-linux-gnu)
    AC_MSG_RESULT([Fixing libtool for -rpath problems.])
    sed < libtool > libtool-2 \
    's/^hardcode_libdir_flag_spec.*$/hardcode_libdir_flag_spec=" -D__LIBTOOL_IS_A_FOOL__ "/'
    mv libtool-2 libtool
    chmod 755 libtool
  ;;
esac

AC_ARG_ENABLE(docbook,[  --disable-docbook       disable building of documentation],[DOCBOOK_DISABLED=yes],[])

#
# Check for Qt
#
PKG_CHECK_MODULES(QT5_GUI,Qt5Core Qt5Widgets Qt5Gui Qt5Network Qt5Sql,,[AC_MSG_ERROR([*** Qt5 not found ***])])
AC_CHECK_PROG(MOC_NAME,moc-qt5,[moc-qt5],[moc])
AC_SUBST(QT_MOC,$MOC_NAME)

AC_CHECK_PROG(LUPDATE_NAME,lupdate-qt5,[lupdate-qt5],[lupdate])
AC_SUBST(QT_LUPDATE,$LUPDATE_NAME)

AC_CHECK_PROG(LRELEASE_NAME,lrelease-qt5,[lrelease-qt5],[lrelease])
AC_SUBST(QT_LRELEASE,$LRELEASE_NAME)

#
# Check for OpenSSL
#
PKG_CHECK_MODULES(LIBCURL,libcurl,[],[AC_MSG_ERROR([*** LibCURL not found ***])])

#
# Build DocBook Items?
#
if test -z $DOCBOOK_DISABLED ; then
  USING_DOCBOOK=yes
fi
AM_CONDITIONAL([DOCBOOK_AM], [test "$USING_DOCBOOK" = yes])

#
# Determine the target architecture
#
AR_GCC_TARGET()
AC_SUBST(VENDOR,$ar_gcc_distro)
AC_SUBST(ARCH,$ar_gcc_arch)

#
# Determine Distro
#
AR_GET_DISTRO()
AC_SUBST(DISTRO,$ar_gcc_distro)
if test $ar_gcc_distro = suse ; then
  AC_SUBST(QT5_MYSQL_PKG,"libqt4-sql-mysql")
  AC_SUBST(MYSQL_PKG,"mysql")
  AC_SUBST(DIST_VER,"")
  AC_SUBST(DEBUG_RPM,"no")
else
  AC_SUBST(QT5_MYSQL_PKG,"qt5-qtbase-mysql")
  DISTRO_MAJOR_VER=`echo $ar_distro_version | awk 'split($1,fields,".") {print fields[[1]]}'`
  if test $DISTRO_MAJOR_VER -eq 6 ; then
    AC_SUBST(MYSQL_PKG,"mysql-server")
    AC_SUBST(DIST_VER,".el$DISTRO_MAJOR_VER")
    AC_SUBST(DEBUG_RPM,"yes")
    AC_SUBST(XINETD_PKG,"")
  else
    if test $DISTRO_MAJOR_VER -ge 7 ; then
      AC_SUBST(MYSQL_PKG,"mariadb-server")
      AC_SUBST(DIST_VER,".el$DISTRO_MAJOR_VER")
      AC_SUBST(DEBUG_RPM,"yes")
      AC_SUBST(XINETD_PKG,"")
    else
      AC_SUBST(MYSQL_PKG,"mysql-server")
      AC_SUBST(DIST_VER,"")
      AC_SUBST(DEBUG_RPM,"no")
      AC_SUBST(XINETD_PKG,"xinetd")
    fi
  fi
fi

#
# Linux vs. OS X Setup
#
if test $ar_gcc_os = "darwin10" ; then
  AC_SUBST(LIB_ALIBS,"-laman")
  AC_SUBST(LIB_AFLAGS,"-framework QtCore -framework QtGui -framework QtNetwork -framework QtSql")
  AC_SUBST(INSTALL_INIT,"install-init-osx.sh")
  AC_DEFINE(OSX)
else
#  AC_SUBST(LIB_ALIBS,"-laman -lQtCore -lQtGui -lQtNetwork -lQtSql")
  AC_SUBST(LIB_ALIBS,"-laman")
  AC_SUBST(LIB_AFLAGS,"")
  AC_SUBST(INSTALL_INIT,"install-init-linux.sh")
  AC_DEFINE(LINUX)
fi

#
# Debug Build Configuration
#
if test -n $STRIP ; then
  AC_SUBST(STRIP_DEBUG,yes)
else
  AC_SUBST(STRIP_DEBUG,"")
fi

#
# Configure RPM Build
#
AC_CHECK_PROG(RPMBUILD_FOUND,rpmbuild,[yes],[])
if test -z $RPMBUILD_FOUND ; then
  AC_SUBST(RPMBUILD,rpm)
else
  AC_SUBST(RPMBUILD,rpmbuild)
fi
if test -d /usr/src/redhat ; then
  AC_SUBST(RPM_ROOT,/usr/src/redhat)
  AC_SUBST(VENDOR,redhat)
else
  AC_SUBST(RPM_ROOT,/usr/src/packages)
  AC_SUBST(VENDOR,suse)
fi 

#
# Generated File Warnings
#
AC_SUBST_FILE(GENERATED_C_FILE_WARNING)
GENERATED_C_FILE_WARNING=$srcdir/helpers/c_file_warning

AC_SUBST_FILE(GENERATED_SCRIPT_FILE_WARNING)
GENERATED_SCRIPT_FILE_WARNING=$srcdir/helpers/script_file_warning

AC_SUBST_FILE(GENERATED_XML_FILE_WARNING)
GENERATED_XML_FILE_WARNING=$srcdir/helpers/xml_file_warning

AC_CONFIG_FILES([aman.spec \
    aman/Makefile \
    amanctl/Makefile \
    amand/Makefile \
    amanrmt/Makefile \
    conf/Makefile \
    debian/source/Makefile \
    debian/Makefile \
    docs/Makefile \
    helpers/Makefile \
    icons/Makefile \
    lib/Makefile \
    scripts/Makefile \
    systemd/Makefile \
    tests/Makefile \
    xdg/Makefile \
    Makefile \
    build_debs.sh\
])
AC_OUTPUT()

chmod 755 build_debs.sh

AC_MSG_NOTICE()
AC_MSG_NOTICE(Now enter 'make' to build the software.)
AC_MSG_NOTICE()
