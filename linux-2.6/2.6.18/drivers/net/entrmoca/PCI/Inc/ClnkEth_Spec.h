/*******************************************************************************
*
* PCI/Inc/ClnkEth_Spec.h
*
* Description: Clink Ethernet Specifications
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

#ifndef __ClnkEth_Spec_h__
#define __ClnkEth_Spec_h__

#include "entropic-config.h"

/*
 * USER CONFIGURABLES
 */

// Common Ethernet Module Configuration
#define CLNK_ETH_MMU          // CPU has MMU
#define CLNK_ETH_BRIDGE       // Transparent bridging

#if \
    ECFG_BOARD_PC_DVT2_PCI_ZIP2==1        || \
    ECFG_BOARD_PC_PCI_ZIP2==1             || \
    ECFG_BOARD_PC_PCI_ZIP1==1             || \
    ECFG_BOARD_PC_PCIE_MAVERICKS==1        || \
    0
#elif \
    ECFG_BOARD_PC_DVT_MII_ZIP2==1         || \
    ECFG_BOARD_PC_DVT_TMII_ZIP2==1        || \
    ECFG_BOARD_PC_DVT_GMII_ZIP2==1        || \
    ECFG_BOARD_COLDFIRE_DVT_FLEX_ZIP2==1  || \
    ECFG_BOARD_ECB_PCI_ZIP2==1            || \
    ECFG_BOARD_ECB_ROW==1                 || \
    0
#define CLNK_ETH_ENDIAN_SWAP  // Endian swap chip accesses.  Undefine for LE.
#else
#error Unknown board.
#endif

//bus used for data path
#define DATAPATH_HOSTLESS           0
#define DATAPATH_PCI                1
#define DATAPATH_FLEXBUS            2
#define DATAPATH_MII                3

// Optional bus type override (from CFLAGS)
#if defined(USE_MII_BUS)
#define CLNK_BUS_TYPE           DATAPATH_MII
#elif defined(USE_68K_BUS)
#define CLNK_BUS_TYPE           DATAPATH_FLEXBUS
#else /* PCI is default */
#define CLNK_BUS_TYPE           DATAPATH_PCI
#endif /* defined(CLNK_BUS_TYPE) */


// HostOS_ control -- define to have console print
#define HOST_OS_ENABLE_PRINTLOG
#define HOST_OS_PRINTLOG_THRESHOLD   L_INFO

// Controls whether Per Packet (=1) or Timer (=0) based ISR's
#define CLNK_ETH_PER_PACKET  1

// Debugging possibilities
#define MONITOR_SWAPS 0

// Optional ctrl type override (from CFLAGS)
#if defined(USE_FLEX_CTRL)
#define CLNK_CTRL_PATH          CTRLPATH_FLEXBUS    // 1
#else /* 0 is default */
#define CLNK_CTRL_PATH          0
#endif




#endif /* __ClnkEth_Spec_h__ */


