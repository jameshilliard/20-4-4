#
# File: Makefile
#
# Copyright 2012 TiVo Inc. All Rights Reserved.
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
	options.c \
	rgb2rgb.c \
	swscale.c \
	swscale_unscaled.c \
	utils.c	\
	yuv2rgb.c \
	$(NULL)


CFLAGS += -O2 -DPACKAGE=\"libswscale\" -DVERSION=\"0.10.3\"


#LIBS :=  $(ISM_LIBDIR)/$(LIBNAME).a

LDSHLIBS=-lpthread
LLDSHLIBS=$(AUTO_LLDSHLIBS)
SHLIBS=$(ISM_LIBDIR)/$(LIBNAME).so

default:

include $(COMMONRULES)
