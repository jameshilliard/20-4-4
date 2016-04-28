/*******************************************************************************
*
* PCI/Inc/ClnkEth_proto_dvr.h
*
* Description: Clink Ethernet Module Ethernet prototypes
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

#ifndef __ClnkEth_proto_dvr_h__
#define __ClnkEth_proto_dvr_h__

/*******************************************************************************
*                             # i n c l u d e s                                *
********************************************************************************/
#include "common_dvr.h"

/*******************************************************************************
*                            I n t e r f a c e s                               *
********************************************************************************/

/*
 * Hardware-independent interface
 *
 * hw_z1, hw_z2_mii, hw_z2_pci, etc. implement hardware-specific functionality
 * for Zip1b, Zip2 in MII mode, Zip2 in PCI mode, etc.
 *
 * Non-static function names are prefaced with "clnk_" to prevent namespace
 * collisions with other kernel drivers.
 *
 * The clnk_hw_params struct is defined in hw_*.h .
 */

#if ECFG_CHIP_ZIP1
int clnk_hw_reg_init(dc_context_t *dccp);
#endif // ECFG_CHIP_ZIP1
int  clnk_hw_init(dc_context_t *dccp, struct clnk_hw_params *hparams);
int  clnk_hw_term(dc_context_t *dccp);
int  clnk_hw_open(dc_context_t *dccp);
int  clnk_hw_close(dc_context_t *dccp);
int  clnk_hw_tx_detach(dc_context_t *dccp, SYS_UINT32 *pPacketID);
int  clnk_hw_rx_detach(dc_context_t *dccp, SYS_UINT32 *pPacketID);
void clnk_hw_rx_intr(dc_context_t *dccp);
void clnk_hw_tx_intr(dc_context_t *dccp);
void clnk_hw_misc_intr(dc_context_t *dccp, SYS_UINT32 *statusVal);
int  clnk_hw_tx_pkt(dc_context_t *dccp, Clnk_ETH_FragList_t *pFragList,
                    SYS_UINT32 priority, SYS_UINT32 packetID, SYS_UINT32 flags);
int  clnk_hw_rx_pkt(dc_context_t *dccp, Clnk_ETH_FragList_t *pFragList,
                    SYS_UINT32 listNum, SYS_UINT32 packetID, SYS_UINT32 flags);
void clnk_hw_setup_timers(dc_context_t *dccp);
void clnk_hw_desc_init(dc_context_t *dccp);
void clnk_hw_get_eth_stats(dc_context_t *dccp, SYS_UINT32 clearStats);
void clnk_hw_bw_teardown(dc_context_t *dccp, SYS_INT32 nodeId);
void clnk_hw_bw_setup(dc_context_t *dccp, SYS_INT32 nodeId,
                      SYS_UINT32 addrHi, SYS_UINT32 addrLo);
void clnk_hw_set_timer_spd(dc_context_t *dccp, SYS_UINT32 spd);
void clnk_hw_soc_status(dc_context_t *dccp, SYS_UINT32 *status);

void clnk_setup_host_bd(dc_context_t *dccp, SYS_UINT32 base);
void clnk_tc_dic_init_pci_drv(dc_context_t *dccp, struct mb_return *mb);

/*
 * Bus-independent interface
 * 
 * ClnkBusPCI, ClnkBus68K, ClnkBusMII implement bus initialization,
 * word range Sonics read/write, etc.
 */

void socInitBus(dc_context_t *dccp);
void socEnableInterrupt(dc_context_t *dccp);
void socDisableInterrupt(dc_context_t *dccp);
void socClearInterrupt(dc_context_t *dccp);
void socRestartRxDma(dc_context_t *dccp, SYS_UINT32 listNum);
void socFixHaltRxDma(dc_context_t *dccp, SYS_UINT32 listNum);
void socProgramRxDma(dc_context_t *dccp, SYS_UINT32 listNum, RxListEntry_t* pEntry);
void socFixHaltTxDma(dc_context_t *dccp, SYS_UINT32 fifoNum);
void socProgramTxDma(dc_context_t *dccp, SYS_UINT32 fifoNum);
void socInterruptTxPreProcess(dc_context_t *dccp);
void socInterruptRxPreProcess(dc_context_t *dccp);

#endif /* __ClnkEth_proto_dvr_h__ */



