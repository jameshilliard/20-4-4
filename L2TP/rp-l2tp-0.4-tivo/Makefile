#
# File: Makefile
#
# Copyright (c) 2009 TiVo Inc.
#
###############################################################################

ISM_DEPTH       := ..
include $(ISM_DEPTH)/ismdefs

LCXXOPTS = -O2
LCXXWARN = \
	-Wno-cast-qual            \
	-Wno-conversion           \
	-Wno-implicit             \
	-Wno-undef                \
	-Wno-sign-compare         \
	-Wno-unused               \
	-Wno-error                \
	$(NULL)
#
# Removed because -Wno-tivo is not supported by gcc 4.2.0
#	-Wno-tivo                 \
#

SFILES_LLDOBJS = auth.C debug.C dgram.C md5.C network.C session.C tunnel.C utils.C event.C hash.C external.C

PROGRAMS = l2tp_lac

include $(ISMRULES)

