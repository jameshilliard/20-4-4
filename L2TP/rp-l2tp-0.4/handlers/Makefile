#
# File: Makefile
#
# Copyright (c) 2009 TiVo Inc.
#
###############################################################################

ISM_DEPTH       := ./../..
include $(ISM_DEPTH)/ismdefs

# $Id: Makefile.in,v 1.2 2002/09/30 19:45:00 dskoll Exp $
# Makefile for LNS handlers
# LIC: GPL

LCDEFS =
LCINCS = -I.. -I../libevent -I../../libevent
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

ifeq ($(BUILD_SUBTARGET),sync-pppd)
SFILES = pty.c sync-pppd.c
SHLIBS = $(ISM_LIBDIR)/sync-pppd.so
endif

ifeq ($(BUILD_SUBTARGET),cmd)
SFILES = cmd.c dstring.c
SHLIBS = $(ISM_LIBDIR)/cmd.so
endif

ifeq ($(BUILD_SUBTARGET),l2tp-control)
PROGRAMS = l2tp-control
endif

ifndef BUILD_SUBTARGET
OTHERPROGS = recursedefault
endif

include $(ISMRULES)

.PHONY: recursedefault recurseclobber
recursedefault:
	$(MAKE) default BUILD_SUBTARGET=sync-pppd
	$(MAKE) default BUILD_SUBTARGET=cmd
	$(MAKE) default BUILD_SUBTARGET=l2tp-control

ifndef BUILD_SUBTARGET
clobber: recurseclobber
endif

recurseclobber:
	$(MAKE) clobber BUILD_SUBTARGET=sync-pppd
	$(MAKE) clobber BUILD_SUBTARGET=cmd
	$(MAKE) clobber BUILD_SUBTARGET=l2tp-control

