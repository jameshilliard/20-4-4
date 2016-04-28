
ISM_DEPTH       := ./../../..
include $(ISM_DEPTH)/ismdefs

LCWARN =                    \
        -Wno-strict-prototypes  \
        -Wno-undef              \
        -Wno-parentheses        \
        -Wno-error              \
        $(NULL)
#
# Removed because -Wno-tivo is not supported in gcc 4.2.0
#        -Wno-tivo          	\
#

# Generated automatically from Makefile.in by configure.
# Skeleton Makefile for the GNU malloc code
#

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

PROFILE_FLAGS = 

#CFLAGS = -g -O2 -msoft-float
LOCAL_CFLAGS = 
CPPFLAGS = 
LDFLAGS = 

DEFS = -DHAVE_CONFIG_H
LOCAL_DEFS = -DSHELL

INCLUDES = -I. -I../.. -I$(topdir) -I$(topdir)/lib

CCFLAGS = ${PROFILE_FLAGS} ${INCLUDES} $(DEFS) $(LOCAL_DEFS) $(LOCAL_CFLAGS) \
	  $(CFLAGS) $(MALLOC_CFLAGS) $(CPPFLAGS)

.c.o:
	$(CC) $(CCFLAGS) -c $<

.s.o:
	$(CC) $(CCFLAGS) -c $<

MALLOC_SOURCE = malloc.c
GMALLOC_SOURCE = gmalloc.c
NMALLOC_SOURCE = nmalloc.c
STUB_SOURCE = stub.c

ALLOCA_SOURCE = alloca.c
ALLOCA_OBJECT = alloca.o

MALLOC_SRC = malloc.c
MALLOC = @MALLOC@
ALLOCA = 

MALLOC_OBJS = malloc.o $(ALLOCA) stub.o
GMALLOC_OBJS = gmalloc.o $(ALLOCA) stub.o
NMALLOC_OBJS = nmalloc.o $(ALLOCA) stub.o
NMALLOC2_OBJS = nmalloc2.o $(ALLOCA) stub.o
NGMALLOC_OBJS = ngmalloc.o $(ALLOCA) stub.o
STUB_OBJS = $(ALLOCA) stub.o

.PHONY:		malloc gmalloc stubmalloc nmalloc ngmalloc nmalloc2

all:		malloc

malloc: ${MALLOC_OBJS}
	${RM} libmalloc.a
	${AR} cr libmalloc.a ${MALLOC_OBJS}
	-test -n "$(RANLIB)" && $(RANLIB) libmalloc.a

nmalloc: ${NMALLOC_OBJS}
	${RM} libmalloc.a
	${AR} cr libmalloc.a ${NMALLOC_OBJS}
	-test -n "$(RANLIB)" && $(RANLIB) libmalloc.a

nmalloc2: ${NMALLOC2_OBJS}
	${RM} libmalloc.a
	${AR} cr libmalloc.a ${NMALLOC2_OBJS}
	-test -n "$(RANLIB)" && $(RANLIB) libmalloc.a

gmalloc: ${GMALLOC_OBJS}
	${RM} libmalloc.a
	${AR} cr libmalloc.a ${GMALLOC_OBJS}
	-test -n "$(RANLIB)" && $(RANLIB) libmalloc.a

ngmalloc: ${NGMALLOC_OBJS}
	${RM} libmalloc.a
	${AR} cr libmalloc.a ${NGMALLOC_OBJS}
	-test -n "$(RANLIB)" && $(RANLIB) libmalloc.a

stubmalloc: ${STUB_OBJS}
	${RM} libmalloc.a
	${AR} cr libmalloc.a ${STUB_OBJS}
	-test -n "$(RANLIB)" && $(RANLIB) libmalloc.a

alloca.o:	$(srcdir)/$(ALLOCA_SOURCE)
	$(CC) $(CCFLAGS) -c $(srcdir)/$(ALLOCA_SOURCE)
	@- if test "$(ALLOCA_OBJECT)" != alloca.o ; then \
		mv $(ALLOCA_OBJECT) alloca.o >/dev/null 2>&1 ; \
	fi

mostlyclean clean:
	$(RM) *.o *.d libmalloc.a

distclean realclean maintainer-clean: clean
	$(RM) Makefile

alloca.o: $(BUILD_DIR)/config.h
malloc.o: $(BUILD_DIR)/config.h $(topdir)/bashtypes.h getpagesize.h
nmalloc.o: $(BUILD_DIR)/config.h $(topdir)/bashtypes.h getpagesize.h
nmalloc2.o: $(BUILD_DIR)/config.h $(topdir)/bashtypes.h getpagesize.h
xmalloc.o: $(BUILD_DIR)/config.h $(topdir)/ansi_stdlib.h
gmalloc.o: $(BUILD_DIR)/config.h

# Rules for deficient makes, like SunOS and Solaris
stub.o: stub.c
malloc.o: malloc.c
gmalloc.o: gmalloc.c
