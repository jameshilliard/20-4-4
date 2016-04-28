###############################################################################
#
# Copyright 2013 TiVo Inc. All Rights Reserved.
#
###############################################################################

ISM_DEPTH := .
include $(ISM_DEPTH)/ismdefs

SOURCE_DIR = mxml-2.7
SOURCE_TAR = $(SOURCE_DIR).tar.gz

ifeq (mxml-2.7,$(SOURCE_DIR))
	PATCH ?= /usr/bin/patch
	PATCH_FILES = patch/mxml-2.7.patch
endif

L_CONFIGURE_ENV = \
	CFLAGS="-I$(CWD)/$(TARGET_DIR) -I$(CWD)/$(SOURCE_DIR)" \
	PKG_CONFIG_BIN="$(TOOLROOT)/bin/pkg-config" \
	$(NULL)

L_CONFIGURE_OPTS = \
	--host=$(CANONICAL_SYSTEM_NAME) \
	--enable-shared \
	--enable-threads \
	--srcdir=$(CWD)/$(SOURCE_DIR) \
	$(NULL)

default: build-opensource

include $(ISMRULES)

