
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
# CandD module makefile
#
# Makefile for mini driver to drive Clock and Data lines       ##
#



ifndef DVR_CWD
    export DVR_CWD := $(CURDIR)
    #$(warning ~~~~~~~~~~~ top DVR_CWD=$(DVR_CWD))
endif

ifeq ($(SNAPGEAR_KERN_VER), 26)
DVR_INCLUDES  = -I $(DVR_CWD)/drivers/clink/Inc \
                -I $(DVR_CWD)/drivers/clink/../GPL/CandD \
                -I $(DVR_CWD)/drivers/clink/../Common/Inc \
                -I $(DVR_CWD)/drivers/clink/../GPL/Common \
                -I $(TOPDIR)/include/linux/
else
DRV_INCLUDES  = -I $(DVR_CWD)/Inc \
                -I $(DVR_CWD)/../GPL/CandD \
                -I $(DVR_CWD)/../Common/Inc \
                -I $(DVR_CWD)/../GPL/Common
endif

olist := Src/Clnk_ctl_candd.o \
         Src/ClnkBus_iface_candd.o \
         ../Common/Src/ClnkIo.o \
         ../Common/Src/ctx_setup.o \
         ../Common/Src/ClnkMbx_dvr.o \
         ../Common/Src/ClnkMbx_call.o \
         ../Common/Src/ClnkIo_common.o \
         ../Common/Src/ctx_abs.o \
         ../Common/Src/util_dvr.o \
         ../GPL/CandD/CandD.o \
         ../GPL/CandD/mdio.o \
         ../GPL/Common/gpl_ctx_setup.o \
         ../GPL/Common/gpl_ctx_abs.o \
         ../GPL/Common/hostos_linux.o


EXTRA_CFLAGS +=  $(DRV_INCLUDES)
EXTRA_CFLAGS += -DCANDD_DRVR_SUPPORT
EXTRA_CFLAGS += -DCLNK_HOST_SEM

ifeq ($(SNAPGEAR_KERN_VER), 26)

# clnkdvr components
  CandDdvr-objs := $(olist)

  # final LKM objects
  obj-$(CONFIG_NET_CLINK) += CandDdvr.o
  
  ifeq ($(DEBUG),y)
    DEBFLAGS = -O -g # "-O" is needed to expand inlines
  else
    DEBFLAGS = -O2
  endif
  
  CFLAGS += $(DEBFLAGS)
  CFLAGS += $(DVR_INCLUDES)
  
else # KERN 24
export-objs :=
obj-y       :=
obj-n       :=
obj-m       :=
obj-        :=
O_TARGET    := CandDdvr.ko
obj-y       += $(olist)
 
obj-$(CONFIG_NET_CLINK) += CandDdvr.ko

include $(TOPDIR)/Rules.make
endif # KERN 26

EXTRA_CFLAGS_nostdinc += $(DEBFLAGS)
##EXTRA_CFLAGS_nostdinc += -I../../inc -I.


KSRC ?= /usr/src/linux

# V=1 for verbose showing of commands
kern:   clean
	$(MAKE) modules -C $(KSRC) KBUILD_EXTMOD=`pwd` V=1

clean:
	@echo "CLEAN $(DVR_CWD)"
	-$(MAKE) clean -C $(KSRC) KBUILD_EXTMOD=`pwd`
	rm -f ../GPL/CandD/*.o 
	rm -f Src/*.o 
	rm -f ../Common/Src/*.o
	rm -f ../GPL/Common/*.o 
	rm -f *.ko






