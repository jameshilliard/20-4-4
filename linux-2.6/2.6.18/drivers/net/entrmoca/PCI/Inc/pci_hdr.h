/*******************************************************************************
*
* PCI/Inc/pci_hdr.h
*
* Description: PCI driver main compilation control
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

#ifndef __pci_hdr_h__
#define __pci_hdr_h__

/*******************************************************************************
*                            # I n c l u d e s                                 *
********************************************************************************/

#include "driverversion.h"
#include "ClnkEth_Spec.h"
#include "HostOS_Spec_pci.h"
#include "inctypes_dvr.h"

#include "common_dvr.h"
#include "Clnk_ctl_dvr.h"
#include "ClnkBus_iface.h"
#include "ClnkMbx_dvr.h"

#if ECFG_CHIP_ZIP1
#include "hw_z1_dvr.h"
#else
#include "hw_z2_dvr.h"
#endif

#include "ClnkCore_dvr.h"
#include "ClnkEth_dvr.h"

#include "control_context.h"
#include "drv_ctl_opts.h"
#include "clnkiodefs.h"
#include "ClnkEth_proto_dvr.h"

#include "debug.h"

/*******************************************************************************
*                            P R O T O T Y P E S                               *
********************************************************************************/

#include "com_abs_proto.h"
#include "HostOS_proto.h"

#include "PCI_proto.h"
#include "Common_proto.h"


#endif // __pci_hdr_h__

