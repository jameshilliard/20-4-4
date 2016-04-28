###############################################################################
#
# File: Makefile
#
# Copyright 2013 TiVo Inc.  All Rights Reserved.
#
###############################################################################

ISM_DEPTH := ./..
IDIRT = $(OBJDIR)
DEFINE_OBJDIR=1
include $(ISM_DEPTH)/ismdefs

# clean these up too
LDIRT += $(GENERATED_BOM_DIR)

# DirectFB documents say to use Make 3.81
MAKE = $(TOOLROOT)/bin/make-381

# compile one source at a time for better diagnostics
MAKEFLAGS += -j 1

SOURCE_CMD = .

#==============================================================================
#
# directfb build steps:
#
# - Create ../obj/[arch]/ output directory.
# - Run configure
# - Run make
# - Run make install (which will install files in $(OBJDIR)/install
#
#==============================================================================

INSTALLDIR=$(CURDIR)/$(OBJDIR)/install

#TARGETS= \
#	$(INSTALLDIR)/lib/libdirect.so		\
#	$(INSTALLDIR)/lib/libdirectfb.so	\
#	$(INSTALLDIR)/lib/libfusion.so		\
#	$(NULL)

# Arbitrarily pick one binary file to be the nominal target
TARGETS= \
	$(INSTALLDIR)/lib/libdirectfb.so	\
	$(NULL)

include $(COMMONRULES)


#==============================================================================
# DirectFB env setup
#==============================================================================

DIRECTFB_DESTDIR := $(INSTALLDIR)

DIRECTFB_CPPFLAGS :=
DIRECTFB_CFLAGS   :=
DIRECTFB_CXXFLAGS :=

# let direct/conf.h know we have <signal.h>
DIRECTFB_CPPFLAGS += -DHAVE_SIGNAL_H=1

# enable TiVo modifications to opensource
DIRECTFB_CPPFLAGS += -DTIVO

DIRECTFB_CPPFLAGS += -I$(ROOT)/include/tae -I$(ROOT)/include
DIRECTFB_CPPFLAGS += -I$(ROOT)/include/tae/jpeg

# XXX should be CFLAGS and CXXFLAGS?
# or should we just build these from scratch here?
# DIRECTFB_CFLAGS   += $(COPTS)
# DIRECTFB_CXXFLAGS += $(CXXOPTS)
#
# Some of our code violates C++ strict-aliasing rules which can cause
# code generation errors (for example, load before a store).
# Strict aliasing optimization is enabled by default at -O2
DIRECTFB_CFLAGS   += -fno-strict-aliasing
DIRECTFB_CXXFLAGS += -fno-strict-aliasing

DIRECTFB_LDFLAGS := -L$(ROOT)/lib
DIRECTFB_LDFLAGS += -Wl,--no-undefined

ifeq ($(BOOTARCH),x86)
	DIRECTFB_LDFLAGS += -Wl,--dynamic-linker=/usr/tivo/lib/ld-tivo.so.2
endif

# flags for LIBPNG, overriding pkg-config.
# Note: Each must be non-empty.
DIRECTFB_LIBPNG_CFLAGS := -I$(ROOT)/include
DIRECTFB_LIBPNG_LIBS   := -lpng -lz

# flags for X11, overriding pkg-config.
# Note: Each must be non-empty.
DIRECTFB_X11_CFLAGS := -I$(ROOT)/X11R6/include
DIRECTFB_X11_LIBS   := -L$(ROOT)/X11R6/lib -lX11 -lXext

# pass through TiVo flags
TIVO_CFLAGS   := $(CFLAGS)
TIVO_CXXFLAGS := $(CXXFLAGS)
TIVO_LDFLAGS  := $(LDFLAGS)

# -Wconversion is broken in GCC 4.23
# http://gcc.gnu.org/wiki/NewWconversion
TIVO_CFLAGS   := $(subst -Wconversion,,$(TIVO_CFLAGS))
TIVO_CXXFLAGS := $(subst -Wconversion,,$(TIVO_CXXFLAGS))

TIVO_LIBS	:= -ltmk
TIVO_LIBS	+= -lstx2
TIVO_LIBS	+= -lhpkoss
TIVO_LIBS	+= -lhpkinterfaces
ifeq ($(BOOTARCH),x86)
	TIVO_LIBS	+= -lhpkhl
endif
ifeq ($(BOOTARCH),mips)
	TIVO_LIBS	+= -lhpkwrhl
	TIVO_LIBS	+= -lhpkwrll
endif
TIVO_LIBS	+= -lgfxcompositor


#==============================================================================
# DirectFB config setup
#==============================================================================

DIRECTFB_CONFIG :=

DIRECTFB_CONFIG += --prefix=

DIRECTFB_CONFIG += --build=i686-TiVo-linux-gnu
DIRECTFB_CONFIG += --host=$(CANONICAL_SYSTEM_NAME)

# configure dev vs. release
# XXX need to investigate these
ifeq ($(FLAVOR),debug)
	DIRECTFB_CONFIG += --enable-debug
	DIRECTFB_CONFIG += --enable-debug-support
else
	DIRECTFB_CONFIG += --disable-debug
	DIRECTFB_CONFIG += --disable-debug-support
endif

# build tests
ifndef NO_TESTS
	DIRECTFB_CONFIG += --with-tests
endif

# disable unsupported misc options
DIRECTFB_CONFIG += --disable-mmx
DIRECTFB_CONFIG += --disable-sse
# XXX do we want network?
#DIRECTFB_CONFIG += --disable-network
DIRECTFB_CONFIG += --disable-sysfs

# disable unsupported system modules
DIRECTFB_CONFIG += --disable-fbdev
DIRECTFB_CONFIG += --disable-devmem
DIRECTFB_CONFIG += --disable-osx
DIRECTFB_CONFIG += --disable-sdl
DIRECTFB_CONFIG += --disable-vnc

ifeq ($(BOOTARCH),x86)
# enable X11 driver build
# Set DFB_USE_X11=1 at runtime to use it.
	DIRECTFB_CONFIG += --enable-x11
else
	DIRECTFB_CONFIG += --disable-x11
endif

# enable TiVo Gfx system module
DIRECTFB_CONFIG += --enable-tivogfx

# enable freetype only on non-x86 builds
ifeq ($(BOOTARCH),x86)
	DIRECTFB_CONFIG += --disable-freetype
else
	DIRECTFB_CONFIG += --enable-freetype
endif

FREETYPE_CFLAGS += -I$(ROOT)/include/freetype2 -I$(ROOT)/include
FREETYPE_LIBS   += -I$(ROOT)/lib -lfreetype -lz

# disable unsupported video provider modules
DIRECTFB_CONFIG += --disable-video4linux

# build only the desired graphics drivers
# (note that tivogfx graphics driver is included with the system module)
DIRECTFB_CONFIG += --with-gfxdrivers=none

# build only the desired input drivers
# (note that tivogfx input driver is included with the system module)
DIRECTFB_CONFIG += --with-inputdrivers=none

# build developer tools (dfbdump, dfbinfo, et al)
DIRECTFB_CONFIG += --with-tools


#==============================================================================
# TARGET: set-dfb-env.sh
#
# Common setup for environment variables used by DirectFB make.
#==============================================================================

SET_DFB_ENV_SCRIPT=$(CURDIR)/$(OBJDIR)/set-dfb-env.sh
SET_DFB_ENV_SCRIPT_TEMP=$(CURDIR)/$(OBJDIR)/set-dfb-env.sh.tmp

.PHONY: make-dfb-env-script

make-dfb-env-script: FORCE
	@set -e; \
	echo "*******************************************************************"; \
	echo "* Making $@"; \
	echo "*******************************************************************"; \
	mkdir -p $(OBJDIR); \
	{ \
		echo "export DESTDIR=\"$(DIRECTFB_DESTDIR)\""; \
		echo "export CPPFLAGS=\"$(DIRECTFB_CPPFLAGS)\""; \
		echo "export CFLAGS=\"$(DIRECTFB_CFLAGS)\""; \
		echo "export CXXFLAGS=\"$(DIRECTFB_CXXFLAGS)\""; \
		echo "export LDFLAGS=\"$(DIRECTFB_LDFLAGS)\""; \
		echo "export LIBPNG_CFLAGS=\"$(DIRECTFB_LIBPNG_CFLAGS)\""; \
		echo "export LIBPNG_LIBS=\"$(DIRECTFB_LIBPNG_LIBS)\""; \
		echo "export X11_CFLAGS=\"$(DIRECTFB_X11_CFLAGS)\""; \
		echo "export X11_LIBS=\"$(DIRECTFB_X11_LIBS)\""; \
		echo "export TIVO_CFLAGS=\"$(TIVO_CFLAGS)\""; \
		echo "export TIVO_CXXFLAGS=\"$(TIVO_CXXFLAGS)\""; \
		echo "export TIVO_LDFLAGS=\"$(TIVO_LDFLAGS)\""; \
		echo "export TIVO_LIBS=\"$(TIVO_LIBS)\""; \
		echo "export FREETYPE_CFLAGS=\"$(FREETYPE_CFLAGS)\""; \
		echo "export FREETYPE_LIBS=\"$(FREETYPE_LIBS)\""; \
		} > $(SET_DFB_ENV_SCRIPT_TEMP); \
	if [ ! -f $(SET_DFB_ENV_SCRIPT) ]; then \
		echo "set-dfb-env.sh didn't exist, creating it."; \
		mv $(SET_DFB_ENV_SCRIPT_TEMP) $(SET_DFB_ENV_SCRIPT); \
	else \
		if cmp --silent $(SET_DFB_ENV_SCRIPT_TEMP) $(SET_DFB_ENV_SCRIPT); then \
			echo "set-dfb-env.sh didn't change."; \
			rm -f $(SET_DFB_ENV_SCRIPT_TEMP); \
		else \
			echo "set-dfb-env.sh changed, updating it."; \
			mv -f $(SET_DFB_ENV_SCRIPT_TEMP) $(SET_DFB_ENV_SCRIPT); \
		fi \
	fi

LDIRT += $(SET_DFB_ENV_SCRIPT)


#==============================================================================
# TARGET: debug-flags
#
# For examining the TiVo common definitions.
#==============================================================================

.PHONY: debug-flags

debug-flags: FORCE
	@set -e; \
	echo "=========================================="; \
	echo "COPTS=$(COPTS)"; \
	echo "CXXOPTS=$(CXXOPTS)"; \
	echo "CFLAGS=$(CFLAGS)"; \
	echo "CXXFLAGS=$(CXXFLAGS)"; \
	echo "=========================================="


#==============================================================================
# TARGET: debug-configure
#==============================================================================

.PHONY: debug-configure

debug-configure: make-dfb-env-script FORCE
	set -ex; \
	echo "*******************************************************************"; \
	echo "* Making $@"; \
	echo "*******************************************************************"; \
	export CONFIG_SHELL="sh -x"; \
	mkdir -p $(OBJDIR); \
	cd $(OBJDIR); \
	export PATH=$$TOOLROOT/bin:$$PATH; \
	$(SOURCE_CMD) $(SET_DFB_ENV_SCRIPT); \
	sh -x $(CURDIR)/configure $(DIRECTFB_CONFIG)


#==============================================================================
# TARGET: configure
#==============================================================================

.PHONY: configure

configure: make-dfb-env-script $(OBJDIR)/config.status

$(OBJDIR)/config.status: $(SET_DFB_ENV_SCRIPT)
	@set -e; \
	echo "*******************************************************************"; \
	echo "* Making $@"; \
	echo "*******************************************************************"; \
	mkdir -p $(OBJDIR); \
	cd $(OBJDIR); \
	export PATH=$$TOOLROOT/bin:$$PATH; \
	$(SOURCE_CMD) $(SET_DFB_ENV_SCRIPT); \
	sh $(CURDIR)/configure $(DIRECTFB_CONFIG)


#==============================================================================
# TARGET: build
#==============================================================================

.PHONY: build

build: make-dfb-env-script configure
	@set -e; \
	echo "*******************************************************************"; \
	echo "* Making $@"; \
	echo "*******************************************************************"; \
	cd $(OBJDIR); \
	export PATH=$$TOOLROOT/bin:$$PATH; \
	$(SOURCE_CMD) $(SET_DFB_ENV_SCRIPT); \
	$(MAKE)


#==============================================================================
# TARGET: install
#==============================================================================

.PHONY: install

install: make-dfb-env-script build
	@set -e; \
	echo "*******************************************************************"; \
	echo "* Making $@"; \
	echo "*******************************************************************"; \
	cd $(OBJDIR); \
	export PATH=$$TOOLROOT/bin:$$PATH; \
	$(SOURCE_CMD) $(SET_DFB_ENV_SCRIPT); \
	$(MAKE) install


#==============================================================================
# TARGET: $(TARGETS)
#==============================================================================

$(TARGETS): install


#==============================================================================
# TARGET: clean
#==============================================================================

#.PHONY: clean
#
#clean:
#	cd $(OBJDIR); \
#	export PATH=$$TOOLROOT/bin:$$PATH; \
#	$(MAKE) mrproper


