#
# File: Makefile
#
# Copyright (c) 2009 TiVo Inc.
#
###############################################################################

ISM_DEPTH       := ./..
include $(ISM_DEPTH)/ismdefs

#***********************************************************************
#
# Makefile
#
# Makefile for L2TP code.
#
# Copyright (C) 2002 Roaring Penguin Software Inc.
#
# This software may be distributed under the terms of the GNU General
# Public License, Version 2, or (at your option) any later version.
#
# LIC: GPL
#
# $Id: Makefile.in,v 1.4 2004/05/16 04:12:48 dskoll Exp $
#***********************************************************************

ifeq ($(BUILD_TARGET),libevent)
DIRS = libevent
endif

ifeq ($(BUILD_TARGET),handlers)
DIRS = handlers
endif

ifeq ($(BUILD_TARGET),l2tpd)
LCDEFS = -DVERSION=\"0.4\" -DPREFIX=\"\"
LCINCS = -I.. -I./libevent/
LLDLIBS = $(ISM_LIBDIR)/libevent.a -ldl
LCWARN = \
	-Wno-cast-qual            \
	-Wno-conversion           \
	-Wno-implicit             \
	-Wno-missing-prototypes   \
	-Wno-strict-prototypes    \
	-Wno-undef                \
	-Wno-tivo                 \
	-Wno-missing-declarations \
	-Wno-sign-compare         \
	-Wno-unused               \
	-Wno-error                \
	$(NULL)
#
# Removed because -Wno-tivo is not supported by gcc 4.2.0
#	-Wno-tivo                 \
#
SFILES_LLDOBJS = auth.c debug.c dgram.c md5.c network.c options.c peer.c session.c tunnel.c utils.c
PROGRAMS = l2tpd
endif

ifndef BUILD_TARGET
OTHERPROGS = recursedefault
endif

include $(ISMRULES)

.PHONY: recursedefault recurseclobber
recursedefault:
	$(MAKE) default BUILD_TARGET=libevent
	$(MAKE) default BUILD_TARGET=l2tpd
	$(MAKE) default BUILD_TARGET=handlers

ifndef BUILD_TARGET
clobber: recurseclobber
endif

recurseclobber:
	$(MAKE) clobber BUILD_TARGET=libevent
	$(MAKE) clobber BUILD_TARGET=l2tpd
	$(MAKE) clobber BUILD_TARGET=handlers

