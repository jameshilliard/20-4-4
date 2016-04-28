###############################################################################
#
# Copyright 2012 TiVo Inc. All Rights Reserved.
#
###############################################################################

ISM_DEPTH := .
include $(ISM_DEPTH)/ismdefs

SOURCE_DIR = fuse-2.9.1
SOURCE_TAR = fuse-2.9.1.tar.gz

PATCH ?= /usr/bin/patch
PATCH_FILES = $(wildcard patch/*.patch)

TARGET_DIR = obj/$(TIVO_SYSTEM)

CONFIGURE_OPTS = --host=$(CANONICAL_SYSTEM_NAME) --disable-example

L_CONFIGURE_ENV = LDFLAGS="-L$(ROOT)/lib"

default : build-opensource

include $(ISMRULES)
