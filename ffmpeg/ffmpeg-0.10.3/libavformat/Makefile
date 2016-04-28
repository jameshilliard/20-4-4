#
# File: Makefile
#
# Copyright 2012 TiVo Inc. All Rights Reserved.
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
	aacdec.c	\
	allformats.c	\
	applehttp.c	\
	applehttpproto.c	\
	audiointerleave.c	\
	avio.c	\
	aviobuf.c	\
	crypto.c	\
	dv.c	\
	file.c	\
	http.c	\
	httpauth.c	\
	id3v1.c	\
	id3v2.c	\
	isom.c	\
	metadata.c	\
	mpeg.c	\
	mpegts.c	\
	network.c	\
	options.c	\
	os_support.c	\
	rawdec.c	\
	riff.c	\
	rtp.c	\
	rtpproto.c	\
	seek.c	\
	tcp.c	\
	tls.c	\
	udp.c	\
	utils.c	\
	$(NULL)


CFLAGS += -O2 -DPACKAGE=\"libavformat\" -DVERSION=\"0.10.3\"


#LIBS :=  $(ISM_LIBDIR)/$(LIBNAME).a

LDSHLIBS=-lpthread
LLDSHLIBS=$(AUTO_LLDSHLIBS)
SHLIBS=$(ISM_LIBDIR)/$(LIBNAME).so

default:

include $(COMMONRULES)
