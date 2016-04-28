###############################################################################
#
# Copyright 2013 TiVo Inc. All Rights Reserved.
#
###############################################################################

ISM_DEPTH := .
include $(ISM_DEPTH)/ismdefs

SOURCE_TAR = glib-2.30.2.tar.bz2
SOURCE_DIR = glib-2.30.2

ifneq ($(findstring 2.30.2,$(SOURCE_TAR)),)
	PATCH ?= /usr/bin/patch
	PATCH_FILES = patch/glib-2.30.2.patch
endif

ifneq ($(findstring mipsel,$(TIVO_SYSTEM)),)
	CONFIG_CACHE_IN = $(ISM_DEPTH)/config.cache.mipsel
else
ifneq ($(findstring mips,$(TIVO_SYSTEM)),)
	CONFIG_CACHE_IN = $(ISM_DEPTH)/config.cache.mips
else
ifneq ($(findstring x86,$(TIVO_SYSTEM)),)
	CONFIG_CACHE_IN = $(ISM_DEPTH)/config.cache.x86
else
	$(error "Unknown TIVO_SYSTEM $(TIVO_SYSTEM)")
endif
endif
endif

CONFIG_CACHE = $(CWD)/config.cache.$(TIVO_SYSTEM)
IDIRT += $(CONFIG_CACHE) $(CWD)/glib-compile-schemas

L_CONFIGURE_ENV = \
	PATH="$(CWD):$$PATH" \
	PKG_CONFIG_BIN="$(TOOLROOT)/bin/pkg-config" 

L_CONFIGURE_OPTS = \
	--enable-debug=no \
	--enable-included-printf=no \
	--cache-file=$(CONFIG_CACHE) \
	--host=$(CANONICAL_SYSTEM_NAME) \
	$(NULL)

ifeq ($(BOOTARCH),x86)
	L_CONFIGURE_OPTS += CFLAGS="-lpthread"
endif

default : config-cache build-opensource
	rm -rf $(CWD)/glib-compile-schemas

config-cache : fake-glib-compile-schemas
	if [ -e "$(CONFIG_CACHE)" ] ; \
	then \
	    rm -rf $(CONFIG_CACHE); \
	fi; \
	cp -f $(CONFIG_CACHE_IN) $(CONFIG_CACHE)

fake-glib-compile-schemas:
ifeq ($(BOOTARCH),mips)
	rm -rf $(CWD)/glib-compile-schemas
	touch $(CWD)/glib-compile-schemas
	chmod +x $(CWD)/glib-compile-schemas
endif

include $(ISMRULES)
