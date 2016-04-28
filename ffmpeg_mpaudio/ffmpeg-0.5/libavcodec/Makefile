#
# File: Makefile
#
# Copyright 2009 TiVo Inc.  All Rights Reserved.
#
###############################################################################

ISM_DEPTH := ./../..
include $(ISM_DEPTH)/ismdefs

LIBNAME = libavcodec

PACKAGE = $(LIBNAME)

LCDEFS = -DHAVE_AV_CONFIG_H 
LCINCS = -I. -I../. 
ICWARN = -w  

SFILES := \
	allcodecs.c \
	audioconvert.c \
	bitstream.c \
	bitstream_filter.c \
	dsputil.c \
	eval.c \
	faanidct.c \
	jrevdct.c \
	opt.c \
	options.c \
	parser.c \
	raw.c \
	resample.c \
	resample2.c \
	simple_idct.c \
	utils.c \
	mpegaudio.c \
	mpegaudiodec.c \
	mpegaudiodecheader.c \
	mpegaudiodata.c \
	mp3_header_decompress_bsf.c \
	mpegaudio_parser.c \
	imgresample.c \
	imgconvert.c \
	$(NULL)


CFLAGS += -O2 -DPACKAGE=\"libavcodec\" -DVERSION=\"0.5\"


LIBS :=  $(ISM_LIBDIR)/$(LIBNAME).a

#LLDSHLIBS=-lc_threadsafe
SHLIBS=$(ISM_LIBDIR)/$(LIBNAME).so

default:

include $(COMMONRULES)
