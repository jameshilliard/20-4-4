###############################################################################
#
# Copyright 2014 TiVo Inc. All Rights Reserved.
#
###############################################################################

ISM_DEPTH := .
include $(ISM_DEPTH)/ismdefs

SOURCE_TAR = gst-plugins-good-0.10.31.tar.gz
SOURCE_DIR = gst-plugins-good-0.10.31

ifneq ($(findstring 0.10.31,$(SOURCE_TAR)),)
	PATCH ?= /usr/bin/patch
	PATCH_FILES = patch/gst-plugins-good-0.10.31.patch
endif

L_CONFIGURE_ENV = \
	PKG_CONFIG_BIN="$(TOOLROOT)/bin/pkg-config"
	
ifeq ($(FLAVOR),release)
    L_CONFIGURE_OPTS = \
	--disable-gst-debug \
	--disable-debug \
	--disable-debugutils \
	$(NULL)
endif

L_CONFIGURE_OPTS += \
	LDFLAGS="-Wl,--rpath=$(ROOT)/lib" \
	--host=$(CANONICAL_SYSTEM_NAME) \
	--disable-x \
	--disable-xvideo \
	$(NULL)

default : build-opensource

include $(ISMRULES)
