#
# File: Makefile
#
# Copyright 2009 TiVo Inc.  All Rights Reserved.
#
###############################################################################

ISM_DEPTH	:= ./../..
include $(ISM_DEPTH)/ismdefs

LIBNAME = libavdevice

PACKAGE = $(LIBNAME)
 
LCDEFS = -DHAVE_AV_CONFIG_H
LCINCS = -I. -I../. 
ICWARN = -w  

SFILES := \
	alldevices.c	\
	$(NULL)



CFLAGS += -O2 -DPACKAGE=\"libavdevice\" -DVERSION=\"0.5\"


LIBS :=  $(ISM_LIBDIR)/$(LIBNAME).a

#LDSHLIBS=-lc_threadsafe
SHLIBS=$(ISM_LIBDIR)/$(LIBNAME).so

default:

include $(COMMONRULES)
