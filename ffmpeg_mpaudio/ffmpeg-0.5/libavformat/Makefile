#
# File: Makefile
#
# Copyright 2009 TiVo Inc.  All Rights Reserved.
#
###############################################################################

ISM_DEPTH	:= ./../..
include $(ISM_DEPTH)/ismdefs

LIBNAME = libavformat

PACKAGE = $(LIBNAME)
 
LCDEFS = -DHAVE_AV_CONFIG_H
LCINCS = -I. -I../. 
ICWARN = -w  

SFILES := \
	allformats.c	\
  	avio.c	\
    	id3v2.c	\
     	metadata_compat.c	\
  	mpeg.c	\
     	os_support.c	\
	aviobuf.c	\
     	cutils.c	\
  	metadata.c	\
  	mp3.c	\
        options.c	\
  	sdp.c	\
	file.c	\
	utils.c \
	riff.c	\
	$(NULL)


CFLAGS += -O2 -DPACKAGE=\"libavformat\" -DVERSION=\"0.5\"


LIBS :=  $(ISM_LIBDIR)/$(LIBNAME).a

#LDSHLIBS=-lc_threadsafe
SHLIBS=$(ISM_LIBDIR)/$(LIBNAME).so

default:

include $(COMMONRULES)
