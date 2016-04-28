# Copyright 2014 TiVo Inc. All Rights Reserved.

ISM_DEPTH := .
include $(ISM_DEPTH)/ismdefs

# The name of the distribution tarball (.gz, .zip, or .bz2)
SOURCE_TAR = xz-5.0.5.tar.gz

# The name of the directory contained in the tarball, that will be created when it's unpacked
SOURCE_DIR = xz-5.0.5

# Options for tarball's 'configure', must at least define --host
CONFIGURE_OPTS = --host=$(CANONICAL_SYSTEM_NAME) --disable-shared

# Where files will be built
TARGET_DIR = obj/$(TIVO_SYSTEM)

# Find patches automatically
PATCH ?= /usr/bin/patch
PATCH_FILES = $(wildcard patches/*.patch)

default : build-opensource

include $(ISMRULES)

DIRT += obj
