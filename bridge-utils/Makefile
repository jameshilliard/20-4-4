#
# File: Makefile
#
# Copyright 2009-2010 TiVo Inc. All Rights Reserved.
#
###############################################################################

#
# ** Taken and modified from opensource/cmdutils/e2fsprogs-*
#
# Builds bridge-utils for use on a TCD
#
# The default steps are
#
#	- Create ../obj/[arch]/ output directory.
#	- Run configure
#	- Run make
#	- Run make install (which will install files in $(OBJDIR)/install)
#

ISM_DEPTH=./..
LDIRT = $(OBJDIR)
include $(ISM_DEPTH)/ismdefs

# override GDIRT to avoid errors on 'make clean'
GDIRT=
GTDIRT=
ODIRT=

# Support multiple architectures

ifeq (mips,$(BOOTARCH))
CONFIGURE_SUFFIX=--host=$(CANONICAL_SYSTEM_NAME) --with-cc=$(CC)
endif

LOCAL_CONFIG_PACKAGE_DIR=$(CURDIR)/bridge-utils-1.4
INSTALLDIR=$(CURDIR)/$(OBJDIR)/install

TARGETS=$(INSTALLDIR)/bin/brctl

DEFINE_OBJDIR=1
include $(COMMONRULES)

$(OBJDIR)/config.status:
	mkdir -p $(OBJDIR); \
	cd $(LOCAL_CONFIG_PACKAGE_DIR); \
	export PATH=$$TOOLROOT/bin:$$PATH; \
	touch $(LOCAL_CONFIG_PACKAGE_DIR)/configure ;\
	ac_cv_func_malloc_0_nonnull=yes $(LOCAL_CONFIG_PACKAGE_DIR)/configure \
		--prefix=$(INSTALLDIR) ${CONFIGURE_SUFFIX}

$(INSTALLDIR)/bin/brctl: $(OBJDIR)/config.status
	mkdir -p $(INSTALLDIR)
	PATH=$$TOOLROOT/bin:$$PATH; \
	cd $(LOCAL_CONFIG_PACKAGE_DIR); \
	make; \
	make install
