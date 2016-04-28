/*******************************************************************************
*
* GPL/PCI/eth_dvr.h
*
* Description: Linux Ethernet Driver types
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
*******************************************************************************/

#ifndef _eth_dvr_h_
#define _eth_dvr_h_

#include "common_dvr.h"
#include "hostos_linux.h"

#if ECFG_CHIP_ZIP1
#define RX_RING_SIZE 256
#else
#define RX_RING_SIZE 64
#endif

/*******************************************************************************
*                             D a t a   T y p e s                              *
********************************************************************************/


struct ring_info {
      struct sk_buff *skb;
      dma_addr_t mapping;
};





/*
    This structure/variable is instantiated at probe time by allocation of the
    net_device. It exists at the end of the net_device and is pointed to by the
    priv member of the net_device. 
*/

struct eth_dev {            // AKA  driver data context (dd_context_t) (ddcp) 
    void                    *p_dg_ctx ; // pointer to driver gpl context
    void                    *p_dc_ctx ; // pointer to driver control context
                                   
    struct pci_dev          *pdev;    // pointer to pci_dev
    struct net_device_stats stats; // net device status entries
    struct ring_info        rx_ring[RX_RING_SIZE];
    struct ring_info        tx_ring[TX_MAPPING_SIZE];
    SYS_INT32               tx_ring_robin;
    SYS_UINT8               txFifoLut[CLNK_ETH_VLAN_8021Q]; // Priority Mapping

    Clnk_ETH_Firmware_t     fw;
    hostos_lock_t           tx_lock;        // xmit spin lock

    SYS_UINTPTR             base_ioaddr;
    SYS_UINT32              base_iolen;
    SYS_UINTPTR             base_ioremap;
    SYS_INT32               chip_id;
    SYS_INT32               chip_rev;
    SYS_UINT32              throttle;
    SYS_UINT32              first_time;  // first-time up indicator
                            
    SYS_UINT8               *pDebugInfo;
    SYS_UINT32              DebugInfoLen;
    SYS_UINT32              netif_rx_dropped;
    SYS_UINT32              tx_int_count;
    SYS_UINT32              resetDelay;
    SYS_UINT32              active;
#if (FEATURE_QOS && ECFG_CHIP_ZIP1)
    SYS_UINT32              tfifo_status;
#endif /* (FEATURE_QOS && ECFG_CHIP_ZIP1) */

};


#endif  // _eth_dvr_h_
