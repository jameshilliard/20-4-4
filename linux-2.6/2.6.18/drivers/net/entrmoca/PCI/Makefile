
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
# PCI eth driver makefile
#
#

# Comment/uncomment the following line to enable/disable debugging
#DEBUG = y


ifndef DVR_CWD
    # Pick up full path to cwd because kernel build (2.6.x) doesn't do relative paths.
    # It also calls this makefile a couple of times so grab the cwd at the very
    # beginning and save it in the environment.
    export DVR_CWD := $(CURDIR)
    #$(warning ~~~~~~~~~~~ top DVR_CWD=$(DVR_CWD))
endif

ifeq (m,$(CONFIG_NET_CLINK))
ifeq ($(SNAPGEAR_KERN_VER), 26)
DVR_INCLUDES  = -I $(DVR_CWD)/drivers/clink/Inc \
                -I $(DVR_CWD)/drivers/clink/../GPL/PCI \
                -I $(DVR_CWD)/drivers/clink/../Common/Inc \
                -I $(DVR_CWD)/drivers/clink/../GPL/Common \
                -I $(TOPDIR)/include/linux/
else
DVR_INCLUDES  = -I $(DVR_CWD)/Inc \
                -I $(DVR_CWD)/../GPL/PCI \
                -I $(DVR_CWD)/../Common/Inc \
                -I $(DVR_CWD)/../GPL/Common \
                -I $(TOPDIR)/include/linux/
endif
else
DVR_INCLUDES  = -I $(DVR_CWD)/Inc \
                -I $(DVR_CWD)/../GPL/PCI \
                -I $(DVR_CWD)/../Common/Inc \
                -I $(DVR_CWD)/../GPL/Common \
                -I $(TOPDIR)/include/linux/
endif

olist := Src/Clnk_ctl_pci.o \
         Src/ClnkCam_dvr.o \
         Src/ClnkEth_dvr.o \
         Src/ClnkBus_iface_pci.o \
         Src/crc_dvr.o \
         ../Common/Src/ClnkIo.o \
         ../Common/Src/ctx_setup.o \
         ../Common/Src/ClnkIo_common.o \
         ../Common/Src/ClnkMbx_dvr.o \
         ../Common/Src/ctx_abs.o \
         ../Common/Src/util_dvr.o

ifeq ($(USE_68K_BUS),1)
olist += ../Common/Src/hostos_stubs.o 
else
olist += ../GPL/PCI/eth_dvr.o \
         ../GPL/PCI/eth_ioctl_dvr.o \
         ../GPL/Common/gpl_ctx_setup.o \
         ../GPL/Common/gpl_ctx_abs.o \
         ../GPL/Common/hostos_linux.o
endif


ifeq (m,$(CONFIG_NET_CLINK))

# Go with the flow in SnapGear;
ifeq ($(SNAPGEAR_KERN_VER), 26)
  # clnkdvr components
  clnkdvr-objs := $(olist)

  ifeq ($(ENTROPIC_CHIP),ZIP1)
    clnkdvr-objs += Src/hw_z1_pci_dvr.o
    ifeq ($(USE_68K_BUS),1)
      EXTRA_CFLAGS += -DUSE_68K_BUS
    endif  
  else
    clnkdvr-objs += Src/hw_z2_pci_dvr.o
  endif
  
  # final LKM objects
  obj-$(CONFIG_NET_CLINK) += clnkdvr.o
  
  ifeq ($(DEBUG),y)
    DEBFLAGS = -O -g # "-O" is needed to expand inlines
  else
    DEBFLAGS = -O2
  endif
    
  CFLAGS += $(DEBFLAGS)
  CFLAGS += $(DVR_INCLUDES)
  
  ifeq ($(ACCESS),1)
  CFLAGS += -DNETWORK_TYPE_ACCESS=1
  endif
  
  CFLAGS += -DPCI_DRVR_SUPPORT

else #2.4 version

export-objs :=
obj-y       :=
obj-n       :=
obj-m       :=
obj-        :=
O_TARGET    := clnkdvr.o
obj-y       += $(olist)

obj-$(CONFIG_NET_CLINK) += clnkdvr.o

ifeq ($(ENTROPIC_CHIP),ZIP1)
  obj-y += Src/hw_z1_pci_dvr.o
  ifeq ($(USE_68K_BUS),1)
    EXTRA_CFLAGS += -DUSE_68K_BUS
  endif
else
  #obj-y += hw_z2.o hw_z2_pci.o
  obj-y += Src/hw_z2_pci_dvr.o
endif

include $(TOPDIR)/Rules.make

ifeq ($(DEBUG),y)
  DEBFLAGS = -O -g # "-O" is needed to expand inlines
else
  DEBFLAGS = -O2
endif

EXTRA_CFLAGS_nostdinc += $(DEBFLAGS)
EXTRA_CFLAGS_nostdinc += $(DVR_INCLUDES) 

ifeq ($(ACCESS),1)
EXTRA_CFLAGS_nostdinc += -DNETWORK_TYPE_ACCESS=1
endif

EXTRA_CFLAGS_nostdinc += -DPCI_DRVR_SUPPORT

endif # SNAPGEAR_KERN_VER

else # original



ifeq ($(KBUILD_EXTMOD),)

KSRC ?= /usr/src/linux


kern:   clean
	$(MAKE) modules -C $(KSRC) KBUILD_EXTMOD=`pwd`

clean:
	-$(MAKE) clean -C $(KSRC) KBUILD_EXTMOD=`pwd`
	-rm -f $(DVR_CWD)/../GPL/PCI/*.o
	-rm -f $(DVR_CWD)/Src/*.o
	-rm -f $(DVR_CWD)/../Common/Src/*.o
	-rm -f $(DVR_CWD)/../GPL/Common/*.o



else # KBUILD_EXTMOD



CWD := $(KBUILD_EXTMOD)

EXTRA_CFLAGS += $(DVR_INCLUDES) 
EXTRA_CFLAGS += -DPCI_DRVR_SUPPORT

# clnkdvr components
clnkdvr-objs := $(olist)

ifeq ($(ENTROPIC_CHIP),ZIP1)
    clnkdvr-objs += Src/hw_z1_pci_dvr.o
    ifeq ($(USE_68K_BUS),1)
        EXTRA_CFLAGS += -DUSE_68K_BUS
    endif
else
    clnkdvr-objs += Src/hw_z2_pci_dvr.o
endif


# final LKM objects
obj-m       = clnkdvr.o



endif # KBUILD_EXTMOD



endif # CONFIG_NET_CLINK
