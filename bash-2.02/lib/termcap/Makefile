
ISM_DEPTH       := ./../../..
include $(ISM_DEPTH)/ismdefs

LCWARN =                            \
        -Wno-strict-prototypes  \
        -Wno-undef              \
        -Wno-parentheses        \
        -Wno-error              \
        $(NULL)
#
# Removed because -Wno-tivo is not supported in gcc 4.2.0
#        -Wno-tivo               \
#

# Generated automatically from Makefile.in by configure.
## -*- text -*- ####################################################
#								   #
# Makefile for termcap replacement libbrary.			   #
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
RM = rm -f
CP = cp
MV = mv

#CFLAGS = -g -O2 -msoft-float
CPPFLAGS = 
LDFLAGS = 

DEFS = -DHAVE_CONFIG_H

INCLUDES = -I. -I../.. -I$(topdir) -I$(topdir)/lib

CCFLAGS = $(CFLAGS) $(DEFS) $(CPPFLAGS) ${INCLUDES}

# Here is a rule for making .o files from .c files that doesn't force
# the type of the machine (like -sun3) into the flags.
.c.o:
	$(CC) -c $(CCFLAGS) $<

SHELL = /bin/sh

SOURCES = termcap.c tparam.c
OBJECTS = termcap.o tparam.o

DOCUMENTATION = termcap.texinfo

THINGS_TO_TAR = $(SOURCES) $(DOCUMENTATION)

##########################################################################

all: libtermcap.a

libtermcap.a:	$(OBJECTS)
	$(RM) -f $@
	$(AR) cr $@ $(OBJECTS)
	-test -n "$(RANLIB)" && $(RANLIB) $@

install:	

clean:
	$(RM) *.o *.a *.log *.cp *.tp *.vr *.fn *.aux *.pg *.toc

mostlyclean distclean maintainer-clean: clean

$(DESTDIR)/libtermcap.a: libtermcap.a
	${INSTALL_DATA} -c -m 644 libtermcap.a $@
	-test -n "$(RANLIB)" && $(RANLIB) -t $@

termcap.o: $(BUILD_DIR)/config.h
tparam.o: $(BUILD_DIR)/config.h
version.o: $(BUILD_DIR)/config.h
