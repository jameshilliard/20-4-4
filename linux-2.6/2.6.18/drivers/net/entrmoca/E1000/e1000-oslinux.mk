
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

##############################################################################
# Build the E1000 module in a LINUX way
##############################################################################
#

ENTROPIC_E1000_C_FILES +=                     \
  ../GPL/Common/hostos_linux.c                \
  ../GPL/Common/gpl_ctx_abs.c                 \
  ../GPL/Common/gpl_ctx_setup.c               \
  ../GPL/E1000/mii_mdio.c                     \

ENTROPIC_E1000_H_FILES +=                     \
  ../GPL/Common/common_gpl_proto.h            \
  ../GPL/Common/gpl_context.h                 \
  ../GPL/Common/hostos.h                      \

ENTROPIC_E1000_INC_DIRS +=                    \
  ../GPL/Common                               \
  ../GPL/E1000                                \

INITIAL_FLAGS = \
  -DDRIVER_E1000 \
  -DDRIVER_NAME=e1000 \
  -DDRIVER_NAME_CAPS=E1000 \
  -DLINUX \
  -D__KERNEL__ \
  -DMODULE \
  -O2 \
  -pipe \
  -Wall \


MORE_FLAGS = $(patsubst %, -I $(CURDIR)/%, $(ENTROPIC_E1000_INC_DIRS))

.PHONY: default clean
default clean:
	echo "Makefile::$@: Starting... "
	$(MAKE) -C ../GPL/E1000 $@ \
	    KSRC=$(ENTROPIC_LINUX_KSRC) \
	    K_VERSION=$(ENTROPIC_LINUX_K_VERSION) \
	    ENTROPIC_C_FILES="$(patsubst %,  ../../E1000/%, $(ENTROPIC_E1000_C_FILES))"  \
	    ENTROPIC_H_FILES="$(patsubst %,  ../../E1000/%, $(ENTROPIC_E1000_H_FILES))"  \
	    EXTRA_CFLAGS="$(INITIAL_FLAGS) $(ENTROPIC_E1000_EXTRA_CFLAGS) $(MORE_FLAGS)"  
	@echo "Makefile::$@: Done. "


