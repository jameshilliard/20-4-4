#
# File: Makefile
#
# Copyright 2012 TiVo Inc. All Rights Reserved.
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
	aes.c	\
	audioconvert.c	\
	avstring.c	\
	base64.c	\
	cpu.c	\
	crc.c	\
	dict.c	\
	error.c	\
	eval.c	\
	fifo.c	\
	imgutils.c	\
	intfloat_readwrite.c	\
	log.c	\
	lfg.c	\
	mathematics.c	\
	mem.c	\
	md5.c	\
	opt.c	\
	parseutils.c	\
	pixdesc.c	\
	random_seed.c	\
	rational.c	\
	samplefmt.c	\
	sha.c	\
	utils.c	\
	$(NULL)



CFLAGS += -O2 -DPACKAGE=\"libavutil\" -DVERSION=\"0.10.3\"


#LIBS :=  $(ISM_LIBDIR)/$(LIBNAME).a

LLDSHLIBS=$(AUTO_LLDSHLIBS)
SHLIBS=$(ISM_LIBDIR)/$(LIBNAME).so

default:

include $(COMMONRULES)
