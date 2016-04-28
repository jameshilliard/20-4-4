#
# File: Makefile
#
# Copyright (c) 2009 TiVo Inc.
#
###############################################################################

ISM_DEPTH       := ./..
include $(ISM_DEPTH)/ismdefs

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

# Removed because this option is not supported by the Blackjack compiler.
#	-Wno-tivo                 \
#

SFILES_LLDOBJS := buffer.c io.c lib.c
PROGRAMS := lzmadec

include $(ISMRULES)
