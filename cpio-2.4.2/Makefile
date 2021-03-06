# Generated automatically from Makefile.in by configure.
# Makefile for GNU cpio.
# Copyright (C) 1990, 1991, 1993 Free Software Foundation, Inc.

# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2, or (at your option)
# any later version.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

#### Start of system configuration section. ####

ISM_DEPTH       := ./..
include $(ISM_DEPTH)/ismdefs

default checkin: all

test:

srcdir = .

# If you use gcc, you should either run the fixincludes script that
# comes with it or else use gcc with the -traditional option.  Otherwise
# ioctl calls will be compiled incorrectly on some systems.
#CC = gcc

# If you don't have a BSD or GNU install program, use cp.
INSTALL = /usr/bin/install -c
INSTALL_PROGRAM = ${INSTALL}
INSTALL_DATA = ${INSTALL} -m 644

# Things you might add to DEFS:
# -DSTDC_HEADERS	If you have ANSI C headers and libraries.
# -DHAVE_UNISTD_H	If you have unistd.h.
# -DHAVE_STRING_H	If you don't have ANSI C headers but have string.h.
# -DHAVE_UTIME_H	If you have utime.h.
# -DMAJOR_IN_MKDEV	If major, minor, makedev defined in sys/mkdev.h.
# -DMAJOR_IN_SYSMACROS	If major, minor, makedev defined in sys/sysmacros.h.
# -DHAVE_FCNTL_H	If not POSIX.1 but you have fcntl.h.
# -DRETSIGTYPE=int	If your signal handlers return int, not void.
# -DHAVE_LCHOWN		If you have lchown system call.
# -DHAVE_STRERROR	If you have strerror function.
# -DHAVE_VPRINTF	If you have vprintf function.
# -DHAVE_DOPRNT		If you have _doprnt function (but lack vprintf).
# -DHAVE_SYS_MTIO_H	If you have sys/mtio.h (magtape ioctls).
# -DHAVE_SYS_GENTAPE_H	If you have sys/gentape.h (ISC magtape ioctls).
# -DHAVE_NETDB_H	To use rexec for remote tape operations
#			instead of forking rsh or remsh.
# -DNO_REMOTE		If you have neither a remote shell nor rexec.
# -DSYMLINK_USES_UMASK	If your symlink system call uses the value of
#			umask to set the protection of symbolic links.
# -DHPUX_CDF		If you are on an HP/UX system and want support
#			for context dependent files.

DEFS =  -DRETSIGTYPE=void -DMAJOR_IN_SYSMACROS=1 -DHAVE_SYS_MTIO_H=1 -DSTDC_HEADERS=1 -DHAVE_UNISTD_H=1 -DHAVE_STRING_H=1 -DHAVE_FCNTL_H=1 -DHAVE_UTIME_H=1 -DHAVE_STRERROR=1 -DHAVE_VPRINTF=1 -DHAVE_ALLOCA_H=1 -DHAVE_ALLOCA=1 -DHAVE_DIRENT_H=1 -DNO_REMOTE
# LIBS :=  -lnsl
# Set this to rtapelib.o unless you defined NO_REMOTE, in which case
# make it empty.
# RTAPELIB = rtapelib.o

CFLAGS += -fsigned-char
# LDFLAGS = -static
LCWARN =				\
	-Wno-conversion			\
	-Wno-missing-prototypes		\
	-Wno-strict-prototypes		\
	-Wno-undef			\
	-Wno-missing-declarations	\
	-Wno-sign-compare		\
	-Wno-parentheses 		\
	-Wno-error			\
	$(NULL)
#
# Removed because -Wno-tivo is not supported in gcc 4.2.0
#	-Wno-tivo			\
#

prefix = $(TIVO_ROOT)
exec_prefix = ${prefix}

# Prefix for each installed program, normally empty or `g'.
binprefix = 
# Prefix for each installed man page, normally empty or `g'.
manprefix = 


# Where to install the cpio and mt executables.
bindir = ${exec_prefix}/bin

# Where to put the rmt executable.
libexecdir = ${exec_prefix}/libexec

# Where to put the info 
infodir = ${prefix}/info

# Where to put the Unix-style manual pages.
mandir = ${prefix}/man/man1
# Extension (not including `.') for the Unix-style manual page filenames.
manext = 1

#### End of system configuration section. ####

SHELL = /bin/sh

MAKEINFO = makeinfo
TEXI2DVI = texi2dvi

TEXFILES = *.aux *.cp *.cps *.dvi *.fn *.fns *.ky *.log *.pg *.toc *.tp *.vr

INFOS = cpio.info*
INFO_DEPS = cpio.info
DVIS = cpio.dvi

SRCS = copyin.c copyout.c copypass.c defer.c dstring.c global.c \
main.c tar.c util.c error.c getopt.c getopt1.c filemode.c version.c \
rtapelib.c dirname.c idcache.c makepath.c xmalloc.c stripslash.c \
userspec.c xstrdup.c bcopy.c fnmatch.c mkdir.c strdup.c
OBJS = copyin.o copyout.o copypass.o defer.o dstring.o global.o \
main.o tar.o util.o error.o getopt.o getopt1.o filemode.o version.o \
$(RTAPELIB) dirname.o idcache.o makepath.o xmalloc.o stripslash.o \
userspec.o xstrdup.o    
# mt source files not shared with cpio.
MT_SRCS = mt.c argmatch.c
MT_OBJS = mt.o argmatch.o error.o getopt.o getopt1.o \
xmalloc.o version.o $(RTAPELIB) 
HDRS = cpio.h cpiohdr.h tar.h tarhdr.h defer.h dstring.h extern.h filetypes.h \
system.h fnmatch.h getopt.h rmt.h safe-stat.h
DISTFILES = $(SRCS) $(HDRS) COPYING COPYING.LIB ChangeLog Makefile.in \
README NEWS INSTALL cpio.1 mt.1 makefile.pc makefile.os2 cpio.def \
configure configure.in mkinstalldirs install-sh $(MT_SRCS) rmt.c tcexparg.c \
alloca.c cpio.texi cpio.info texinfo.tex

all:	cpio # mt rmt

.c.o:
	$(CC) -c $(CPPFLAGS) $(DEFS) -I$(srcdir) $(CFLAGS) $<

#install:: installdirs all $(srcdir)/cpio.1 $(srcdir)/mt.1
#	$(INSTALL_PROGRAM) cpio $(bindir)/$(binprefix)cpio
#	test ! -f mt || $(INSTALL_PROGRAM) mt $(bindir)/$(binprefix)mt
#	-test ! -f rmt || $(INSTALL_PROGRAM) rmt $(libexecdir)/rmt
#	-$(INSTALL_DATA) $(srcdir)/cpio.1 $(mandir)/$(manprefix)cpio.$(manext)
#	-test ! -f mt || $(INSTALL_DATA) $(srcdir)/mt.1 $(mandir)/$(manprefix)mt.$(manext)
#
#installdirs:
#	$(srcdir)/mkinstalldirs $(bindir) $(libexecdir) $(mandir) $(infodir)
#
#uninstall::
#	cd $(bindir); rm -f $(binprefix)cpio $(binprefix)mt
#	-rm -f $(libexecdir)/rmt
#	cd $(mandir); rm -f $(manprefix)cpio.$(manext) $(manprefix)mt.$(manext)

check:
	@echo No tests are supplied.

cpio:	$(OBJS)
	$(CCF) -o $@ $(OBJS) $(LIBS) $(LDFLAGS)

install: all
	@mkdir -p $(INSTALL_STD_BIN)
	ln -s -f `/bin/pwd`/cpio $(INSTALL_STD_BIN)

copyin.o: cpio.h cpiohdr.h defer.h dstring.h extern.h filetypes.h fnmatch.h rmt.h system.h
copyout.o: cpio.h cpiohdr.h defer.h dstring.h extern.h filetypes.h rmt.h system.h
copypass.o: cpio.h cpiohdr.h dstring.h extern.h filetypes.h system.h
defer.o: cpiohdr.h extern.h system.h
dstring.o: dstring.h
global.o: cpio.h cpiohdr.h dstring.h extern.h system.h
main.o: cpio.h cpiohdr.h dstring.h extern.h filetypes.h rmt.h
tar.o: filetypes.h cpio.h cpiohdr.h dstring.h extern.h rmt.h tar.h tarhdr.h system.h
util.o: extern.h rmt.h system.h
mt.o: rmt.h

rmt:	rmt.o
	$(CC) $(LDFLAGS) -o $@ rmt.o $(LIBS)

mt:	$(MT_OBJS)
	$(CC) $(LDFLAGS) -o $@ $(MT_OBJS) $(LIBS)

# Makefile: Makefile.in config.status
# 	$(SHELL) config.status
# config.status: configure
# 	$(SHELL) config.status --recheck
# configure: configure.in
# 	cd $(srcdir); autoconf

TAGS:	$(SRCS)
	etags $(SRCS)

.SUFFIXES: .texi .info .dvi

#.texi.info:
#	$(MAKEINFO) -I$(srcdir) $<
#
#.texi.dvi:
#	TEXINPUTS=$(srcdir):$$TEXINPUTS $(TEXI2DVI) $<
#
#info:: $(INFO_DEPS)
#
#dvi:: $(DVIS)
#
#install:: install-info
#
#install-info: $(INFO_DEPS)
#	cd $(srcdir); for file in *.info*; do \
#	  $(INSTALL_DATA) $$file $(infodir)/$$file; \
#	done
#
#uninstall:: uninstall-info
#
#uninstall-info:
#	cd $(srcdir); for file in *.info*; do \
#	  rm -f $(infodir)/$$file; \
#	done

clean clobber:
	rm -f cpio rmt mt *.o *.d core

mostlyclean: clean

distclean: clean
	rm -f Makefile config.status config.log cpio.info

maintainer-clean: distclean
	@echo "This command is intended only for maintainers to use;"
	@echo "rebuilding the deleted files may require special tools."
	rm -f TAGS

dist: $(DISTFILES)
	echo cpio-`sed -e '/version_string/!d' -e 's/[^0-9.]*\([0-9.]*\).*/\1/' -e q version.c` > .fname
	rm -rf `cat .fname`
	mkdir `cat .fname`
	-ln $(DISTFILES) `cat .fname`
	for file in $(DISTFILES); do \
	  test -r `cat .fname`/$$file || cp -p $$file `cat .fname`; \
	done
	tar chzf `cat .fname`.tar.gz `cat .fname`
	rm -rf `cat .fname` .fname

# Prevent GNU make v3 from overflowing arg limit on SysV.
.NOEXPORT:
