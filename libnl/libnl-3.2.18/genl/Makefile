#
# File: Makefile
#
# Copyright 2013 TiVo Inc.  All Rights Reserved.
#
###############################################################################

ISM_DEPTH	:= ./../..
include $(ISM_DEPTH)/ismdefs

LIBNAME = libnl-genl-3

PACKAGE = $(LIBNAME)
 
LCINCS = -I. -I../include \
	-D_GNU_SOURCE \
	-DSYSCONFDIR=\"/etc/libnl\"

ICWARN = -Wall

SFILES := \
	ctrl.c		\
	family.c	\
	genl.c		\
	mngt.c		\
	$(NULL)


SHLIBS=$(ISM_LIBDIR)/$(LIBNAME).so

default:

include $(COMMONRULES)
