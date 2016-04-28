#
# File: Makefile
#
# Copyright 2013 TiVo Inc. All Rights Reserved.
#
###############################################################################

ISM_DEPTH	:= ./../..
include $(ISM_DEPTH)/ismdefs

LIBNAME = libavcodec

PACKAGE = $(LIBNAME)

LCDEFS = -DHAVE_AV_CONFIG_H
LCINCS = -I. -I../. 
ICWARN = -w  

SFILES := \
	aac_ac3_parser.c \
	aacadtsdec.c	\
	aac_parser.c	\
	allcodecs.c	\
	audioconvert.c	\
	avpacket.c	\
	bitstream.c	\
	dsputil.c	\
	dvdata.c	\
	faanidct.c	\
	imgconvert.c	\
	jrevdct.c	\
	mpeg4audio.c	\
	mpegaudiodata.c	\
	options.c	\
	parser.c	\
	pthread.c	\
	raw.c	\
	rawdec.c	\
	simple_idct.c	\
	utils.c \
	$(NULL)


CFLAGS += -O2 -DPACKAGE=\"libavcodec\" -DVERSION=\"0.10.3\"


#LIBS :=  $(ISM_LIBDIR)/$(LIBNAME).a

LLDSHLIBS=$(AUTO_LLDSHLIBS)
SHLIBS=$(ISM_LIBDIR)/$(LIBNAME).so

default:

include $(COMMONRULES)
