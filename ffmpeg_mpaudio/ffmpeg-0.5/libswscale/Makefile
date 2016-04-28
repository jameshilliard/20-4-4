#
# File: Makefile
#
# Copyright 2009 TiVo Inc.  All Rights Reserved.
#
###############################################################################

ISM_DEPTH	:= ./../..
include $(ISM_DEPTH)/ismdefs

LIBNAME = libswscale

PACKAGE = $(LIBNAME)
 
LCDEFS = -DHAVE_AV_CONFIG_H
LCINCS = -I. -I../. 
ICWARN = -w  

SFILES := \
	rgb2rgb.c \
	swscale_avoption.c \
  	yuv2rgb.c \
	$(NULL)



CFLAGS += -O2 -DPACKAGE=\"libswscale\" -DVERSION=\"0.5\"


LIBS :=  $(ISM_LIBDIR)/$(LIBNAME).a

#LDSHLIBS=-lc_threadsafe
SHLIBS=$(ISM_LIBDIR)/$(LIBNAME).so


default:

include $(COMMONRULES)
