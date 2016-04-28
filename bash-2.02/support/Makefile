# Generated automatically from Makefile.in by configure.
#
# Simple Makefile for the support programs.
#
# documentation support: man2html
# testing support: printenv recho zecho
#
# bashbug lives here but is created by the top-level makefile
#
# Currently only man2html is built
#

#
# Boilerplate
#
topdir = ..
srcdir = .
VPATH = .:.
BUILD_DIR = /home/sstone/sandbox/devtools/bash-2.02

RM	= rm -f
SHELL	= /bin/sh
CC = gcc

#
# Compiler options:
#
PROFILE_FLAGS = 
CFLAGS = -g -O2
LOCAL_CFLAGS = 
CPPFLAGS = 
DEFS = -DHAVE_CONFIG_H
LOCAL_DEFS = -DSHELL
LDFLAGS =  $(LOCAL_LDFLAGS) $(CFLAGS)
LOCAL_LDFLAGS = -rdynamic
LIBS := -ldl 

INCLUDES = -I${BUILD_DIR} -I${topdir}

#
CCFLAGS = ${PROFILE_FLAGS} $(DEFS) $(LOCAL_DEFS) $(SYSTEM_FLAGS) $(CPPFLAGS) \
	  ${INCLUDES} $(LOCAL_CFLAGS) $(CFLAGS)

SRC1 =	man2html.c
OBJ1 =	man2html.o

.c.o:
	$(RM) $@
	$(CC) -c $(CCFLAGS) $<

all: man2html

man2html: $(OBJ1)
	$(CC) $(CCFLAGS) $(OBJ1) -o $@ ${LIBS}

clean:
	rm man2html

man2html.o: man2html.c
