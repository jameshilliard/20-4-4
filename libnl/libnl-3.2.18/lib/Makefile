#
# File: Makefile
#
# Copyright 2013 TiVo Inc.  All Rights Reserved.
#
###############################################################################

ISM_DEPTH	:= ./../..
include $(ISM_DEPTH)/ismdefs

LIBNAME = libnl-3

PACKAGE = $(LIBNAME)
 
#LCDEFS = -DHAVE_AV_CONFIG_H
LCINCS = -I. -I../include \
	-D_GNU_SOURCE \
	-DSYSCONFDIR=\"/etc/libnl\"

ICWARN = -Wall

SFILES := \
	addr.c		\
	attr.c		\
	cache.c		\
	cache_mngr.c	\
	cache_mngt.c	\
	data.c		\
	error.c		\
	handlers.c	\
	msg.c		\
	nl.c		\
	object.c	\
	socket.c	\
	utils.c		\
	version.c	\
	hash.c 		\
	hashtable.c	\
	$(NULL)


SHLIBS=$(ISM_LIBDIR)/$(LIBNAME).so

default:

include $(COMMONRULES)
