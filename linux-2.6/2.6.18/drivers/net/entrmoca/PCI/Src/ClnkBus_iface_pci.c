/*******************************************************************************
*
* PCI/Src/ClnkBus_iface_pci.c
*
* Description: PCI Bus interface layer
*
*******************************************************************************/

/*******************************************************************************
*                        Entropic Communications, Inc.
*                         Copyright (c) 2001-2008
*                          All rights reserved.
*******************************************************************************/

/*******************************************************************************
* This file is licensed under GNU General Public license.                      *
*                                                                              *
* This file is free software: you can redistribute and/or modify it under the  *
* terms of the GNU General Public License, Version 2, as published by the Free *
* Software Foundation.                                                         *
*                                                                              *
* This program is distributed in the hope that it will be useful, but AS-IS and*
* WITHOUT ANY WARRANTY; without even the implied warranties of MERCHANTABILITY,*
* FITNESS FOR A PARTICULAR PURPOSE, TITLE, or NONINFRINGEMENT. Redistribution, *
* except as permitted by the GNU General Public License is prohibited.         *
*                                                                              * 
* You should have received a copy of the GNU General Public License, Version 2 *
* along with this file; if not, see <http://www.gnu.org/licenses/>.            *
********************************************************************************/

/*******************************************************************************
*                            # I n c l u d e s                                 *
********************************************************************************/

#include "drv_hdr.h"


void clnk_bus_read( void *vctx, SYS_UINTPTR addr, SYS_UINT32 *data)
{
    *data = HostOS_Read_Word((SYS_UINT32 *)addr);
}

void clnk_bus_write( void *vctx, SYS_UINTPTR addr, SYS_UINT32 data)
{
    HostOS_Write_Word(data, (SYS_UINT32 *)addr);
}

