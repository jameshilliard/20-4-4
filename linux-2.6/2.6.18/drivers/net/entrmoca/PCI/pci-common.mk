
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
##############################################
# PCI eth driver makefile for a non operating system build
##############################################

##############################################
# Common definitions for all builds of PCI.  At this time, these 
#  lists are not used in operating system builds.
export ENTROPIC_PCI_C_FILES =                \
  Src/ClnkCam_dvr.c                          \
  Src/Clnk_ctl_pci.c                         \
  Src/ClnkBus_iface_pci.c                    \
  Src/ClnkEth_dvr.c                          \
  Src/crc_dvr.c                              \
  ../Common/Src/ClnkIo.c                     \
  ../Common/Src/ctx_setup.c                  \
  ../Common/Src/ClnkIo_common.c              \
  ../Common/Src/ClnkMbx_dvr.c                \
  ../Common/Src/ctx_abs.c                    \
  ../Common/Src/util_dvr.c                   \

export ENTROPIC_PCI_H_FILES =                \

export ENTROPIC_PCI_INC_DIRS =               \
 Inc                                         \
 ../Common/Inc                               \

export ENTROPIC_PCI_EXTRA_CFLAGS =           \
  -DPCI_DRVR_SUPPORT                         \

# adjust for chip
ifeq ($(ENTROPIC_CHIP),ZIP1)
ENTROPIC_PCI_C_FILES      += Src/hw_z1_pci_dvr.c
#ENTROPIC_PCI_C_FILES      += Src/ClnkBusPCI_dvr.c
else
ENTROPIC_PCI_C_FILES      += Src/hw_z2_pci_dvr.c
#ENTROPIC_PCI_C_FILES      += Src/ClnkBusPCI2_dvr.c
endif


# FOR NOW this is only used in the configuration
#   below.  Later that may change.

include pci-osnone.mk
