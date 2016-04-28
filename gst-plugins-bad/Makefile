###############################################################################
#
# Copyright 2014 TiVo Inc. All Rights Reserved.
#
###############################################################################

ISM_DEPTH := .
include $(ISM_DEPTH)/ismdefs

SOURCE_TAR = gst-plugins-bad-0.10.23.tar.gz
SOURCE_DIR = gst-plugins-bad-0.10.23

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
	--disable-gsettings \
	$(NULL)

default : build-opensource

include $(ISMRULES)
