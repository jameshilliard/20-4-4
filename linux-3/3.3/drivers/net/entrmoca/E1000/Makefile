
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
# Build whatever should be built for the E1000 module
##############################################################################
#

# Files needed for this driver located from the perspective of this directory.
export ENTROPIC_E1000_C_FILES =               \
  Src/Clnk_ctl_e1000.c                        \
  Src/ClnkBus_iface_e1000.c                   \
  ../Common/Src/ClnkIo.c                      \
  ../Common/Src/ctx_setup.c                   \
  ../Common/Src/ClnkMbx_dvr.c                 \
  ../Common/Src/ClnkMbx_call.c                \
  ../Common/Src/ClnkIo_common.c               \
  ../Common/Src/ctx_abs.c                     \
  ../Common/Src/util_dvr.c                    \

export ENTROPIC_E1000_H_FILES =               \
  Inc/E1000_proto.h                           \
  Inc/HostOS_Spec_e1000.h                     \
  Inc/mii_common.h                            \
  ../Common/Inc/Clnk_ctl_dvr.h                \
  ../Common/Inc/clnkiodefs.h                  \
  ../Common/Inc/ClnkMbx_dvr.h                 \
  ../Common/Inc/com_abs_proto.h               \
  ../Common/Inc/common_dvr.h                  \
  ../Common/Inc/Common_proto.h                \
  ../Common/Inc/data_context.h                \
  ../Common/Inc/driverversion.h               \
  ../Common/Inc/drv_ctl_opts.h                \
  ../Common/Inc/hw_z2_dvr.h                   \
  ../Common/Inc/inctypes_dvr.h                \

export ENTROPIC_E1000_INC_DIRS =              \
  Inc                                         \
  ../Common/Inc                               \

export ENTROPIC_E1000_EXTRA_CFLAGS =          \
  -DE1000_DRVR_SUPPORT                        \
  -DZIP_2=1                                   \
  -DCLNK_HOST_SEM

.PHONY: default clean
default clean:
	@echo "Makefile::$@: Starting with target=$(ENTROPIC_TARGET_RULE)... "
	test -z "`echo $(ENTROPIC_OS) | grep LINUX_`"  ||  \
	    $(MAKE) -f e1000-oslinux.mk $@
	test -z "`echo $(ENTROPIC_OS) | grep NONE`"    ||  \
	    $(MAKE) -f e1000-osnone.mk  $@
	@echo "Makefile::$@: Done. "


