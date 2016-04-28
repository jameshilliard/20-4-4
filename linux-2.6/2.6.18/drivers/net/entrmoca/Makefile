#
#
# Copyright (c) 2011 TiVo Inc. All rights reserved. 
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License version 2 as
# published by the Free Software Foundation.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
#
#
# File: drivers/net/emoca/Makefile
#
# Makefile for the TiVo EN2510 MoCA driver
#

ifeq ($(CONFIG_TIVO_NEUTRON),y)
obj-m += entrmoca.o
endif

entrmoca-objs := entrmoca_main.o entrmoca_mmi.o Common/Src/ClnkIo.o Common/Src/ClnkIo_common.o \
                 Common/Src/ClnkMbx_dvr.o Common/Src/util_dvr.o Common/Src/ctx_abs.o \
                 Common/Src/ctx_setup.o Common/Src/ClnkMbx_call.o E1000/Src/Clnk_ctl_e1000.o \
                 E1000/Src/ClnkBus_iface_e1000.o GPL/Common/gpl_ctx_abs.o \
                 GPL/Common/gpl_ctx_setup.o GPL/Common/hostos_linux.o 

EXTRA_CFLAGS += -I. -Idrivers/net/entrmoca/GPL/Common -Idrivers/net/entrmoca/Common/Inc \
                -Idrivers/net/entrmoca/E1000/Inc -Idrivers/net/entrmoca/GPL/E1000 \
                -Idrivers/net/brcmint7038 -DCLNK_HOST_SEM



