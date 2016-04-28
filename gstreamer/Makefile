###############################################################################
#
# Copyright 2014 TiVo Inc. All Rights Reserved.
#
###############################################################################

ISM_DEPTH := .
include $(ISM_DEPTH)/ismdefs

SOURCE_TAR = gstreamer-0.10.36.tar.gz
SOURCE_DIR = gstreamer-0.10.36

ifneq ($(findstring 0.10.36,$(SOURCE_TAR)),)
	PATCH ?= /usr/bin/patch
	PATCH_FILES = patch/gstreamer-0.10.36.patch
endif

# The -rpath-link is to hack around the fact that libtool puts /lib into lib
# search path on x86 builds at higher priority than the system libs
L_CONFIGURE_ENV = \
	LDFLAGS="$(LDFLAGS) -Wl,-rpath-link,$(TOOLROOT)/$(CANONICAL_SYSTEM_NAME)/sys-root/lib" \
	PKG_CONFIG_BIN="$(TOOLROOT)/bin/pkg-config"

ifeq ($(FLAVOR),release)
    L_CONFIGURE_OPTS = \
	--disable-gst-debug \
	--disable-debug \
	$(NULL)
endif

L_CONFIGURE_OPTS += \
	--host=$(CANONICAL_SYSTEM_NAME) \
	$(NULL)

default : build-opensource

include $(ISMRULES)
