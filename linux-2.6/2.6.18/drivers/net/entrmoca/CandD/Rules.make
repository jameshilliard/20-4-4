
# This file is licensed under GNU General Public license.                      
#                                                                              
# This file is free software: you can redistribute and/or modify it under the  
# terms of the GNU General Public License, Version 2, as published by the Free 
# Software Foundation.                                                         
#                                                                             
# This program is distributed in the hope that it will be useful, but AS-IS and
# WITHOUT ANY WARRANTY; without even the implied warranties of MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE, TITLE, or NONINFRINGEMENT. Redistribution, 
# except as permitted by the GNU General Public License is prohibited.         
#                                                                              
# You should have received a copy of the GNU General Public License, Version 2 
# along with this file; if not, see <http://www.gnu.org/licenses/>.            
#
# -*-makefile-*-
#
# This file is part of the sample code for the book "Linux Device Drivers",
# second edition. It is meant to be generic and is designed to be recycled
# by other drivers. The comments should be clear enough.
# It partly comes from Linux Makefile, and needs GNU make. <rubini@linux.it>

# TOPDIR is declared by the Makefile including this file.
ifndef TOPDIR
	TOPDIR = .
endif

# Set the CROSS COMPILE prefix and directory for source
CROSS_COMPILE=y

ifeq ($(CROSS_COMPILE),y)
	KERNELDIR = /exports/coyote/usr/src/linux
	X_COMPILE=xscale_be-
else
	KERNELDIR = /usr/src/linux
	X_COMPILE=
endif


# The headers are taken from the kernel
	INCLUDEDIR = $(KERNELDIR)/include

# We need the configuration file, for CONFIG_SMP and possibly other stuff
# (especiall for RISC platforms, where CFLAGS depends on the exact
# processor being used).
#ifeq ($(KERNELDIR)/.config,$(wildcard $(KERNELDIR))/.config)
#	include $(KERNELDIR)/.config
#else
#	MESSAGE := $(shell echo "WARNING: no .config file in $(KERNELDIR)")
#endif

# ARCH can be speficed on the comdline or env. too, and defaults to this arch
# Unfortunately, we can't easily extract if from kernel configuration
# (well, we could look athe asm- symlink... don't know if worth the effort)
ifndef ARCH
  ARCH := $(shell uname -m | sed -e s/i.86/i386/ -e s/sun4u/sparc64/ \
		-e s/arm.*/arm/ -e s/sa110/arm/)
endif



# This is useful if cross-compiling. Taken from kernel Makefile (CC changed)
AS      =$(X_COMPILE)as
LD      =$(X_COMPILE)ld
CC      =$(X_COMPILE)gcc
CPP     =$(CC) -E
AR      =$(X_COMPILE)ar
NM      =$(X_COMPILE)nm
STRIP   =$(X_COMPILE)strip
OBJCOPY =$(X_COMPILE)objcopy
OBJDUMP =$(X_COMPILE)objdump

# The platform-specific Makefiles include portability nightmares.
# Some platforms, though, don't have one, so check for existence first
ARCHMAKEFILE = $(TOPDIR)/Makefile.$(ARCH)
ifeq ($(ARCHMAKEFILE),$(wildcard $(ARCHMAKEFILE)))
  include $(ARCHMAKEFILE)
endif

# CFLAGS: all assignments to CFLAGS are inclremental, so you can specify
# the initial flags on the command line or environment, if needed.
CFLAGS +=  -DCLINK_LINUX -Wall -D__KERNEL__ -DMODULE -I$(INCLUDEDIR)

ifdef CONFIG_SMP
	CFLAGS += -D__SMP__ -DSMP
endif

# Prepend modversions.h if we're running with versioning.
ifdef CONFIG_MODVERSIONS
	CFLAGS += -DMODVERSIONS -include $(KERNELDIR)/include/linux/modversions.h
endif

#Install dir
VERSIONFILE = $(INCLUDEDIR)/linux/version.h
VERSION     = $(shell awk -F\" '/REL/ {print $$2}' $(VERSIONFILE))
INSTALLDIR = /lib/modules/$(VERSION)/misc
