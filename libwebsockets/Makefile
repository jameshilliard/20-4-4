#
# File: Makefile
#
# Copyright 2012 TiVo Inc. All Rights Reserved.
#
###############################################################################

ISM_DEPTH       := .
include $(ISM_DEPTH)/ismdefs

# enable explicitly, as it has no DIRS definition
ISM_ENABLED := 1

VERSION := 2012-06-19
SRCDIR := $(ISM_TOPDIR)/libwebsockets-$(VERSION)

# ARGH!  This subsystem has not been converted to buildism!
TARGET_DIR := $(ISM_TOPDIR)/obj/$(TIVO_SYSTEM)
INSTALL_DIR := $(TARGET_DIR)/install
ISM_LIBDIR = $(TARGET_DIR)/lib
ISM_PLUGINLIBDIR := $(TARGET_DIR)/plugins

LIBWEBSOCKETS_SO := $(ISM_LIBDIR)/libwebsockets.so

# Manipulate NMTARGETS manually, but clear it from TARGETS
# so that we can use the commonrules to build the
# libwebsockets.nm file
NMTARGETS_SO := $(ISM_LIBDIR)/libwebsockets.nm
TARGETS =

# turn off warnings as errors, as it causes configure to fail
WERROR :=

# turn off all TiVo warnings, as the libwebsockets test programs turn
# on -Werror, but have some code that emits warnings if the TiVo warnings
# are enabled, breaking the build.
GCWARN :=

# Specific libwebsockets configuration optional features.  
# Run 'make show-options'for a full list.
LIBWEBSOCKET_CONFIG_OPTS := \
	--disable-static \
	--enable-openssl \
	--enable-libcrypto \
	$(NULL)

default: $(LIBWEBSOCKETS_SO) $(NMTARGETS_SO)

include $(ISMRULES)

# Re-run configure if the current Makefile or configure script changes,
# or if it has never been run before.  Put output in the $(TARGET_DIR),
# not in the source tree.
$(TARGET_DIR)/config.stamp: Makefile $(SRCDIR)/configure
	mkdir -p $(TARGET_DIR)
	cd $(TARGET_DIR) && \
	/usr/bin/env CC="$(CC)" CXX="$(CXX)" AR="$(AR)" RANLIB="$(RANLIB)" \
	LD="$(LD)" CFLAGS="$(CFLAGS)" LDFLAGS="$(LDSHFLAGS)" \
	PKG_CONFIG="$(PKG_CONFIG)" PKG_CONFIG_LIBDIR="$(ROOT)/lib/pkgconfig" \
	$(SRCDIR)/configure --prefix=$(INSTALL_DIR) --srcdir=$(SRCDIR) --host=$(CANONICAL_SYSTEM_NAME) $(LIBWEBSOCKET_CONFIG_OPTS)
	touch $@

# Build the pre-configured project from the $(TARGET_DIR) directory,
# not the source tree.
.PHONY: do-libwebsockets-build
do-libwebsockets-build: $(TARGET_DIR)/config.stamp
	$(MAKE) -C $(TARGET_DIR)
	$(MAKE) -C $(TARGET_DIR) install

# Move the websockets shared library to ISM_LIBDIR so that
# the NMTARGETS rules can act on it
$(LIBWEBSOCKETS_SO): do-libwebsockets-build
	@cp $(INSTALL_DIR)/lib/libwebsockets.so $@

# Test drive the configuration of opensource project
test-config: $(TARGET_DIR)/config.stamp

# Print out the configure options
show-options:
	cd $(SRCDIR); ./configure --help
