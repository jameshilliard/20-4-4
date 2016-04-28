
ISM_DEPTH       := ./../../..
include $(ISM_DEPTH)/ismdefs

# Generated automatically from Makefile.in by configure.
## -*- text -*- ####################################################
#								   #
# Makefile for the GNU Tilde Library.				   #
#								   #
####################################################################

srcdir = .
VPATH = .:.
topdir = ../..
BUILD_DIR = $(ISM_TOPDIR)/bash-2.02

INSTALL = /usr/bin/install -c
INSTALL_PROGRAM = ${INSTALL}
INSTALL_DATA = ${INSTALL} -m 644

CC = gcc
RANLIB = ranlib
AR = ar
RM = rm
CP = cp
MV = mv

CFLAGS = -g -O2
LOCAL_CFLAGS = 
CPPFLAGS = 
LDFLAGS =  -rdynamic

DEFS = -DHAVE_CONFIG_H
LOCAL_DEFS = -DSHELL

INCLUDES = -I. -I../.. -I$(topdir) -I$(topdir)/lib

CCFLAGS = $(DEFS) $(LOCAL_DEFS) $(CPPFLAGS) ${INCLUDES} $(LOCAL_CFLAGS) $(CFLAGS)

.c.o:
	$(CC) -c $(CCFLAGS) $<

# The name of the library target.
LIBRARY_NAME = libtilde.a

# The C code source files for this library.
CSOURCES = $(srcdir)/tilde.c

# The header files for this library.
HSOURCES = $(srcdir)/tilde.h

OBJECTS = tilde.o

# The texinfo files which document this library.
DOCSOURCE = doc/tilde.texi
DOCOBJECT = doc/tilde.dvi
DOCSUPPORT = doc/Makefile
DOCUMENTATION = $(DOCSOURCE) $(DOCOBJECT) $(DOCSUPPORT)

SUPPORT = Makefile ChangeLog $(DOCSUPPORT)

SOURCES  = $(CSOURCES) $(HSOURCES) $(DOCSOURCE)

THINGS_TO_TAR = $(SOURCES) $(SUPPORT)

######################################################################

all: $(LIBRARY_NAME)

$(LIBRARY_NAME): $(OBJECTS)
	$(RM) -f $@
	$(AR) cr $@ $(OBJECTS)
	-test -n "$(RANLIB)" && $(RANLIB) $@

documentation: force
	-(cd doc; $(MAKE) $(MFLAGS))

force:

# The rule for 'includes' is written funny so that the if statement
# always returns TRUE unless there really was an error installing the
# include files.
install:
	$(INSTALL_DATA) -c -m 644 $(LIBRARY_NAME) $(bindir)/$(LIBRARY_NAME)
	-test -n "$(RANLIB)" && $(RANLIB) -t $(bindir)/$(LIBRARY_NAME)

clean:
	$(RM) -f $(OBJECTS) $(LIBRARY_NAME)
	-( cd doc && $(MAKE) $(MFLAGS) $@ )

realclean distclean maintainer-clean: clean
	-( cd doc && $(MAKE) $(MFLAGS) $@ )
	$(RM) -f Makefile

mostlyclean: clean
	-( cd doc && $(MAKE) $(MFLAGS) $@ )

######################################################################
#								     #
#  Dependencies for the object files which make up this library.     #
#								     #
######################################################################

tilde.o: tilde.h $(topdir)/ansi_stdlib.h
tilde.o: $(BUILD_DIR)/config.h

# Rules for deficient makes, like SunOS and Solaris
tilde.o: tilde.c
