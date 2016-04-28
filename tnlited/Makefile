#
# File: Makefile
#
# Copyright 2010 TiVo Inc.  All rights reserved.
#
###############################################################################

ISM_DEPTH	:= ./..
include $(ISM_DEPTH)/ismdefs

#SFILES_LLDOBJS = inet_ntoa.c

LCWARN =			\
	-Wno-strict-prototypes 	\
	-Wno-parentheses 	\
	-Wno-undef              \
	-Wno-error		\
	$(NULL) 
# Removed because -Wno-tivo is not supported in GCC 4.2.0
#        -Wno-tivo               \
#

CPPMDOPT =

#LCOPTS = -fsigned-char -DBAD_LAUNCHER_PTY
LCOPTS = -fsigned-char

PROGRAMS := tnlited

default:

include $(ISMRULES)

# install:
# 	ln -s -f `/bin/pwd`/tnlited $(INSTALL_STD_SBIN)

