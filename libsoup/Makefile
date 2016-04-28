###############################################################################
#
# Copyright 2014 TiVo Inc. All Rights Reserved.
#
###############################################################################

ISM_DEPTH := .
include $(ISM_DEPTH)/ismdefs

SOURCE_TAR = libsoup-2.26.3.tar.gz
SOURCE_DIR = libsoup-2.26.3

L_CONFIGURE_ENV = \
	PKG_CONFIG_BIN="$(TOOLROOT)/bin/pkg-config"

ifeq ($(FLAVOR),release)
    L_CONFIGURE_OPTS = \
	--enable-debug=no \
	$(NULL)
endif

L_CONFIGURE_OPTS += \
	--disable-glibtest \
	--host=$(CANONICAL_SYSTEM_NAME) \
	--without-gnome \
	$(NULL)

default : build-opensource

include $(ISMRULES)
