#
# File: Makefile
#
# Copyright 2010 TiVo, Inc.  All rights reserved.
#
###############################################################################

ISM_DEPTH       := ./..
include $(ISM_DEPTH)/ismdefs

LCWARN =                   \
        -Wno-cast-qual            \
        -Wno-conversion           \
        -Wno-implicit             \
        -Wno-missing-prototypes   \
        -Wno-strict-prototypes    \
        -Wno-undef                \
        -Wno-missing-declarations \
        -Wno-sign-compare 	  \
        -Wno-unused		  \
        -Wno-error		  \
	$(NULL)
#
# Removed because -Wno-tivo is not supported in gcc 4.2.0
#        -Wno-tivo                 \
#

CPPMDOPT =

SFILES_LLDOBJS := iwlib.c

PROGRAMS := iwconfig iwlist iwpriv iwspy iwgetid iwevent ifrename

include $(ISMRULES)

