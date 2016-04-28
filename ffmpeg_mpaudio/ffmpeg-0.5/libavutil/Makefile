#
# File: Makefile
#
# Copyright 2009 TiVo Inc.  All Rights Reserved.
#
###############################################################################

ISM_DEPTH	:= ./../..
include $(ISM_DEPTH)/ismdefs

LIBNAME = libavutil

PACKAGE = $(LIBNAME)
 
LCDEFS = -DHAVE_AV_CONFIG_H
LCINCS = -I. -I../. 
ICWARN = -w  

SFILES := \
	adler32.c	\
   	base64.c	\
  	fifo.c	\
  	log.c	\
	md5.c	\
     	rational.c	\
  	tree.c	\
	aes.c	\
       	crc.c	\
     	lfg.c	\
   	lzo.c	\
	mem.c	\
     	rc4.c	\
       	utils.c	\
	avstring.c	\
  	des.c	\
     	lls.c	\
   	mathematics.c	\
  	random.c	\
  	sha1.c	\
	$(NULL)



CFLAGS += -O2 -DPACKAGE=\"libavutil\" -DVERSION=\"0.5\"


LIBS :=  $(ISM_LIBDIR)/$(LIBNAME).a

SHLIBS=$(ISM_LIBDIR)/$(LIBNAME).so

default:

include $(COMMONRULES)
