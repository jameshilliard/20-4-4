# Generated automatically from Makefile.in by configure.
#
# Simple makefile for the sample loadable builtins
#
# This includes some boilerplate definitions added by configure, but will
# still need hand-editing
#
# Include some boilerplate Gnu makefile definitions.
prefix = /usr/local

exec_prefix = ${prefix}
bindir = ${exec_prefix}/bin
libdir = ${exec_prefix}/lib
infodir = ${prefix}/info
includedir = ${prefix}/include

topdir = ../..
BUILD_DIR = /home/sstone/sandbox/devtools/bash-2.02
srcdir = .
VPATH = .:.


CC = gcc
RM = rm -f

SHELL = /bin/sh

# SunOS 4
#PICFLAG = -pic
# Some versions of gcc, esp. on NetBSD and FreeBSD
PICFLAG = -fpic
# Linux -- could also be -fpic
#PICFLAG = -fPIC
# SunOS 5
#PICFLAG = -K pic
# SVR4, SVR4.2, Irix
#PICFLAG = -K PIC
# BSD/OS 2.1, BSD/OS 3.x
#PICFLAG =
# AIX 4.2
#PICFLAG = -K

# SunOS 4, BSD/OS 2.1, BSD/OS 3.x, SVR4.2, SVR4, Linux, AIX 4.2, etc.
LD = ld
# SunOS 5, Linux
#LD = ${CC}

# SunOS 4
#LDOPT = -assert pure-text
# OSF/1, Digital UNIX
#LDOPT = -shared -soname $@ -expect_unresolved '*'
# SunOS 5 using sun cc
#LDOPT = -dy -z text -G -i -h $@
# SunOS 5 using gcc with Sun ld
#LDOPT = -shared -Wl,-dy -Wl,-G -Wl,-i                      
# SVR4, SVR4.2
#LDOPT = -dy -z text -G -h $@
# NetBSD, FreeBSD -- might also need -r
LDOPT = -x -Bshareable
# Linux
#LDOPT = -shared
# BSD/OS 2.1, BSD/OS 3.x
#LDOPT = -r
# AIX 4.2
#LDOPT = -bdynamic -bnoentry -bexpall -G

# other libraries to link the shared object against
# BSD/OS 2.1
#LDLIBS := -lc_s.2.1.0
# BSD/OS 3.0, BSD/OS 3.1
#LDLIBS := -lc_s.3.0.0


INC = -I. -I.. -I$(topdir) -I$(topdir)/lib -I$(topdir)/builtins \
      -I$(BUILD_DIR) -I$(BUILD_DIR)/lib -I$(BUILD_DIR)/builtins

.c.o:
	$(CC) $(PICFLAG) $(CFLAGS) $(INC) -c -o $@ $<


ALLPROG = print truefalse sleep pushd finfo logname basename dirname \
	  tty pathchk tee head rmdir sprintf
OTHERPROG = necho getconf hello cat

all:	$(ALLPROG)
others:	$(OTHERPROG)

everything: all others

sprintf: sprintf.o
	$(LD) $(LDOPT) -o $@ sprintf.o $(LDLIBS)

print:	print.o
	$(LD) $(LDOPT) -o $@ print.o $(LDLIBS)

necho:	necho.o
	$(LD) $(LDOPT) -o $@ necho.o $(LDLIBS)

getconf: getconf.o
	$(LD) $(LDOPT) -o $@ getconf.o $(LDLIBS)

hello:	hello.o
	$(LD) $(LDOPT) -o $@ hello.o $(LDLIBS)

truefalse: truefalse.o
	$(LD) $(LDOPT) -o $@ truefalse.o $(LDLIBS)

sleep:	sleep.o
	$(LD) $(LDOPT) -o $@ sleep.o $(LDLIBS)

pushd:	pushd.o
	$(LD) $(LDOPT) -o $@ pushd.o $(LDLIBS)

finfo:	finfo.o
	$(LD) $(LDOPT) -o $@ finfo.o $(LDLIBS)

cat:	cat.o
	$(LD) $(LDOPT) -o $@ cat.o $(LDLIBS)

logname:	logname.o
	$(LD) $(LDOPT) -o $@ logname.o $(LDLIBS)

basename:	basename.o
	$(LD) $(LDOPT) -o $@ basename.o $(LDLIBS)

dirname:	dirname.o
	$(LD) $(LDOPT) -o $@ dirname.o $(LDLIBS)

tty:	tty.o
	$(LD) $(LDOPT) -o $@ tty.o $(LDLIBS)

pathchk:	pathchk.o
	$(LD) $(LDOPT) -o $@ pathchk.o $(LDLIBS)

tee:	tee.o
	$(LD) $(LDOPT) -o $@ tee.o $(LDLIBS)

rmdir:	rmdir.o
	$(LD) $(LDOPT) -o $@ rmdir.o $(LDLIBS)

head:	head.o
	$(LD) $(LDOPT) -o $@ head.o $(LDLIBS)

clean:
	$(RM) $(ALLPROG) $(OTHERPROG) *.o

mostlyclean:	clean

distclean maintainer-clean: clean
	$(RM) Makefile

print.o: print.c
truefalse.o: truefalse.c
sleep.o: sleep.c
pushd.o: pushd.c
finfo.o: finfo.c
logname.o: logname.c
basename.o: basename.c
dirname.o: dirname.c
tty.o: tty.c
pathchk.o: pathchk.c
tee.o: tee.c
head.o: head.c
rmdir.o: rmdir.c
sprintf.o: sprintf.c
necho.o: necho.c
getconf.o: getconf.c
hello.o: hello.c
cat.o: cat.c

