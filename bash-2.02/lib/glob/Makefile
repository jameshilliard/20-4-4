
ISM_DEPTH       := ./../../..
include $(ISM_DEPTH)/ismdefs

LCWARN =                        \
        -Wno-strict-prototypes  \
        -Wno-undef              \
        -Wno-parentheses        \
        -Wno-error		\
	$(NULL)
#
# Removed because -Wno-tivo is not supported in gcc 4.2.0
#        -Wno-tivo               \
#

# Generated automatically from Makefile.in by configure.
## -*- text -*- ####################################################
#								   #
# Makefile for the GNU Glob Library.				   #
#								   #
####################################################################

srcdir = .
VPATH = .:.
topdir = ../..
BUILD_DIR = $(ISM_TOPDIR)/bash-2.02

INSTALL = /usr/bin/install -c
INSTALL_PROGRAM = ${INSTALL}
INSTALL_DATA = ${INSTALL} -m 644

#CC = /usr/local/gcc-ppc/bin/gcc
#RANLIB = /usr/local/gcc-ppc/bin/ranlib
#AR = /usr/local/gcc-ppc/bin/ar
RM = rm
CP = cp
MV = mv

#CFLAGS = -g -O2 -msoft-float
LOCAL_CFLAGS = 
CPPFLAGS = 
LDFLAGS =  -rdynamic

DEFS = -DHAVE_CONFIG_H
LOCAL_DEFS = -DSHELL

INCLUDES = -I. -I../.. -I$(topdir) -I$(topdir)/lib

CCFLAGS = $(DEFS) $(LOCAL_DEFS) $(CPPFLAGS) ${INCLUDES} $(LOCAL_CFLAGS) $(CFLAGS)

# Here is a rule for making .o files from .c files that doesn't force
# the type of the machine (like -sun3) into the flags.
.c.o:
	$(CC) -c $(CCFLAGS) $<

# The name of the library target.
LIBRARY_NAME = libglob.a

# The C code source files for this library.
CSOURCES = $(srcdir)/glob.c $(srcdir)/fnmatch.c

# The header files for this library.
HSOURCES = $(srcdir)/fnmatch.h

OBJECTS = glob.o fnmatch.o

# The texinfo files which document this library.
DOCSOURCE = doc/glob.texi
DOCOBJECT = doc/glob.dvi
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

what-tar:
	@for file in $(THINGS_TO_TAR); do \
	  echo $(selfdir)$$file; \
	done

documentation: force
	-(cd doc; $(MAKE) $(MFLAGS))
force:

# The rule for 'includes' is written funny so that the if statement
# always returns TRUE unless there really was an error installing the
# include files.
install:

clean:
	rm -f $(OBJECTS) $(OBJECTS:.o=.d) $(LIBRARY_NAME)
	-(cd doc && $(MAKE) $(MFLAGS) $@ )

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

fnmatch.o: fnmatch.h
fnmatch.o: $(BUILD_DIR)/config.h

glob.o: $(BUILD_DIR)/config.h
glob.o: $(topdir)/bashtypes.h $(topdir)/ansi_stdlib.h $(topdir)/bashansi.h
glob.o: $(topdir)/posixstat.h $(topdir)/memalloc.h
glob.o: fnmatch.h

# Rules for deficient makes, like SunOS and Solaris
fnmatch.o: fnmatch.c
glob.o: glob.c
