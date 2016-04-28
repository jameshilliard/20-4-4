#
# File: Makefile
#
# Copyright (C) 2000, 2002 TiVo, Inc.  All rights reserved.
#
###############################################################################

ISM_DEPTH	:= ./..
include $(ISM_DEPTH)/ismdefs

SFILES		= \
		  	cdaudio.c \
			irix_cdaudio.c \
			beos_cdaudio.c \
			cddb.c \
			cdindex.c \
			coverart.c \
			data.c \
			changer.c \
			base64.c \
			extra_objs.c 

LCWARN	= \
	-Wno-all                	\
        -Wno-cast-qual          	\
        -Wno-conversion         	\
        -Wno-implicit           	\
        -Wno-missing-prototypes 	\
        -Wno-strict-prototypes  	\
        -Wno-undef              	\
        -Wno-missing-declarations       \
        -Wno-error			\
	$(NULL)

LCINCS		= -I./

ifneq ($(TIVO_SERVER),y)
LLDSHLIBS = $(AUTO_LLDSHLIBS)
else
LLDSHLIBS = $(AUTO_LLDSHLIBS) -lc
endif

LIBS		= $(ISM_LIBDIR)/libcdaudio.a
SHLIBS		= $(ISM_LIBDIR)/libcdaudio.so

include $(ISMRULES)

