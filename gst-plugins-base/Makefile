###############################################################################
#
# Copyright 2014 TiVo Inc. All Rights Reserved.
#
###############################################################################

ISM_DEPTH := .
include $(ISM_DEPTH)/ismdefs

SOURCE_TAR = gst-plugins-base-0.10.36.tar.gz
SOURCE_DIR = gst-plugins-base-0.10.36

ifneq ($(findstring 0.10.36,$(SOURCE_TAR)),)
	PATCH ?= /usr/bin/patch
	PATCH_FILES = patch/gst-plugins-base-0.10.36.patch
endif

L_CONFIGURE_ENV = \
	PKG_CONFIG_BIN="$(TOOLROOT)/bin/pkg-config"

ifeq ($(FLAVOR),release)
    L_CONFIGURE_OPTS = \
	--disable-gst-debug \
	--disable-debug \
	$(NULL)
endif

L_CONFIGURE_OPTS += \
	LDFLAGS="-Wl,--rpath=$(ROOT)/lib" \
	--host=$(CANONICAL_SYSTEM_NAME) \
	--disable-gio \
	--disable-audioresample \
	--disable-examples \
	--disable-ivorbis \
	--disable-ogg \
	--disable-vorbis \
	--disable-x \
	--disable-xvideo \
	$(NULL)

default : build-opensource

include $(ISMRULES)
