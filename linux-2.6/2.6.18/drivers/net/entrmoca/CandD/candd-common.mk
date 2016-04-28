
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

##############################################
# PCI eth driver makefile for a non operating system build
##############################################

##############################################
# Common definitions for all builds of PCI.  At this time, these 
#  lists are not used in operating system builds.
export ENTROPIC_CANDD_C_FILES =              \
  Src/Clnk_ctl_candd.c                       \
  Src/ClnkBus_iface_candd.c                  \
  ../Common/Src/ClnkIo.c                     \
  ../Common/Src/ctx_setup.c                  \
  ../Common/Src/ClnkIo_common.c              \
  ../Common/Src/ClnkMbx_dvr.c                \
  ../Common/Src/ClnkMbx_call.c               \
  ../Common/Src/ClnkMbx_ttask.c              \
  ../Common/Src/ctx_abs.c                    \
  ../Common/Src/util_dvr.c                   \

export ENTROPIC_CANDD_H_FILES =              \

export ENTROPIC_CANDD_INC_DIRS =             \
 Inc                                         \
 ../Common/Inc                               \

export ENTROPIC_CANDD_EXTRA_CFLAGS =         \
  -DCANDD_DRVR_SUPPORT                       \


include candd-osnone.mk
