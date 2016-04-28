#
# File: Makefile
#
# Copyright (c) 2009 TiVo Inc.
#
###############################################################################

ISM_DEPTH       := ./../..
include $(ISM_DEPTH)/ismdefs

# Generated automatically from Makefile.in by configure.
# $Id: Makefile.in,v 1.2 2002/09/30 19:45:00 dskoll Exp $
#
# Makefile for event-handling library
#
# Copyright 2002 Roaring Penguin Software Inc.
#
# This software may be distributed according to the terms of the GNU
# General Public License, version 2 or (at your option) any later version.
# LIC: GPL

LCDEFS =
LCINCS = -I..
LCWARN = \
	-Wno-cast-qual            \
	-Wno-conversion           \
	-Wno-implicit             \
	-Wno-missing-prototypes   \
	-Wno-strict-prototypes    \
	-Wno-undef                \
	-Wno-missing-declarations \
	-Wno-sign-compare         \
	-Wno-unused               \
	-Wno-error                \
	$(NULL)
#
# Removed because -Wno-tivo is not supported by gcc 4.2.0
#	-Wno-tivo                 \
#

SFILES = event.c event_tcp.c hash.c event_sig.c
LIBS = $(ISM_LIBDIR)/libevent.a

include $(ISMRULES)
