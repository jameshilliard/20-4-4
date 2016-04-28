/*******************************************************************************
*
* PCI/Inc/control_context_pci.h
*
* Description: PCI Driver context definition
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

#ifndef __control_context_pci_h__
#define __control_context_pci_h__

/*******************************************************************************
*                             # i n c l u d e s                                *
********************************************************************************/
#include "common_dvr.h"

// Control plane Context Structure
struct _control_context
{
    void                *p_dg_ctx ;     // pointer to driver gpl context
    void                *p_dk_ctx ;     // pointer to driver kernel context

    SYS_UINTPTR         baseAddr;
    Clnk_MBX_Mailbox_t  mailbox;
    SYS_UINT8           mailboxInitialized;

    SYS_UINT32          pSwUnsolQueue;      // Software unsolicited queue pointer
    SYS_UINT32          swUnsolQueueSize;

    void                *at_lock_link;          // spinlock for address translation
    void                *ioctl_sem_link;
    void                *mbx_cmd_lock_link;     // mailbox cmd spin lock - referenced in !GPL side
    void                *mbx_swun_lock_link;    // mailbox sw unsol spin lock - referenced in !GPL side

    SYS_UINT8           clnk_ctl_in[CLNK_CTL_MAX_IN_LEN];
    SYS_UINT8           clnk_ctl_out[CLNK_CTL_MAX_OUT_LEN];

    SYS_UINT32          at3_base;   // Address translator 3 base address



    SYS_UINT32                  pciIntStatus;
    SYS_UINT32                  rxHaltStatus;
    SYS_UINT32                  txHaltStatus;
    Clnk_ETH_SendPacketCallback sendPacketCallback;
    Clnk_ETH_RcvPacketCallback  rcvPacketCallback;
    ClnkDef_EthStats_t          stats;
    SYS_UINT32                  txDroppedErrs;

    // Configuration variables
    Clnk_ETH_Firmware_t         fw;

    // SOC status variables
    SYS_UINT32                  socStatus;
    SYS_UINT32                  socStatusEmbedded;
    SYS_UINT8                   intr_enabled;
    SYS_UINT8                   socBooted;
    SYS_UINT8                   timer_intr;
    SYS_UINT8                   socStatusInProgress;
    SYS_UINT8                   socStatusLastTransID;
    SYS_UINT32                  socStatusTimeoutCnt;
    SYS_UINT32                  socStatusLinkDownCnt;
    SYS_UINT32                  socStatusTxHaltedCnt;
    SYS_UINT32                  socStatusLastTxCnt;
    SYS_UINT32                  socStatusLastTxAsyncPendingCnt;

    // Receive lists
    SYS_UINT8*                  pRxMemBlock;
    SYS_UINT32                  rxMemSize;
    void*                       pRxDmaMemBlock;
    SYS_UINT32                  rxDmaMemSize;
    SYS_UINT32                  rxNumHostDesc;
    SYS_UINT8                   rxIsOpen;
    ListHeader_t                rxList[MAX_RX_LISTS];
    SYS_UINT32                  rxListMinLen[MAX_RX_LISTS];
    ListHeader_t                rxFreeList[MAX_RX_LISTS];
    ListHeader_t                rxGenFreeList;
    RxListEntry_t*              pRxQueuedPkts[MAX_RX_LISTS];

    // Transmit lists
    SYS_UINT8*                  pTxMemBlock;
    SYS_UINT32                  txMemSize;
    void*                       pTxDmaMemBlock;
    SYS_UINT32                  txDmaMemSize;
    SYS_UINT32                  txNumHostDesc;
    SYS_UINT8                   txIsOpen;
    ListHeader_t                txPriFreeList[MAX_TX_PRIORITY];
    ListHeader_t                txQueuedPkts[MAX_TX_PRIORITY];
    SYS_UINT8                   txNumFreeList[MAX_TX_PRIORITY];
    SYS_UINT32                  txTotalPercentage; // sum of fw.txFifoSize[ ]
    SYS_UINT32                  txAttempts;
    SYS_UINT32                  txCompletions;
    SYS_UINT32                  txPriSendCnt[MAX_TX_PRIORITY];
    SYS_UINT32                  txPriDropCnt[MAX_TX_PRIORITY];
    SYS_UINT32                  txPriUFloodCnt[MAX_TX_PRIORITY];
    SYS_UINT32                  txQMask[(MAX_TX_PRIORITY+31)/32];

#if (NETWORK_TYPE_ACCESS)
    ClnkAccess_t                access;
#endif


#ifdef CLNK_ETH_PHY_DATA_LOG
    // EVM data variables
    ClnkDef_EvmData_t *         evmData;
#endif

#if defined(CLNK_ETH_BRIDGE) 
    // Bridge variables
    CamContext_t                cam;
    SYS_UINT8                   bridgeInitialized;
#endif

#ifdef CLNK_ETH_ECHO_PROFILE
    // Echo profile variables
    SYS_UINT32                  eppDataValid;
    ClnkDef_EppData_t           eppData;
#endif

#ifdef CLNK_ETH_MMU
    void*                       pRxDmaMemBlockPa;
    void*                       pTxDmaMemBlockPa;
#endif

#ifdef CLNK_ETH_PROFILE
    // Profiling variables
    SYS_UINT32                  profileSendPacketTime;
    SYS_UINT32                  profileHandleRxTime;
    SYS_UINT32                  profileHandleTxTime;
#endif

#if (FEATURE_FREQ_SCAN)
    SYS_BOOLEAN                 fs_restart;
    SYS_BOOLEAN                 fs_reset;
#endif /* FEATURE_FREQ_SCAN */

#if (FEATURE_QOS)
    SYS_UINT32                  qos_flows_per_dst[MAX_NUM_NODES];
#if ECFG_CHIP_ZIP1
    SYS_UINT32                  qos_flow_count;
    // Tx fifo resize status
    #define TX_FIFO_RESIZE_STS_DONE        0   // TX Fifos are all resized
    #define TX_FIFO_RESIZE_STS_THROTTLED   1   // Netstack is throttled
    #define TX_FIFO_RESIZE_STS_DRAINED     2   // TX Fifos are all drained
    SYS_UINT32                  txFifoResizeStatus;
    SYS_UINT32                  txFifoResizeTransId;
    Clnk_ETH_TFifoReszCallback  tFifoResizeCallback;
#endif /* ECFG_CHIP_ZIP1 */
#endif

  
#if (NETWORK_TYPE_ACCESS)
    accessBlk_t                 accessBlk; //access module specific data struct
#endif

#if CLNK_BUS_TYPE == DATAPATH_FLEXBUS
    RxHostDesc_t*               p68kRxPending;   // keep receive host descriptor in DMA, prevent reentry
    TxHostDesc_t*               p68kTxPending;   // keep transmit host descriptor in DMA, prevent reentry
    SYS_UINT32                  Int68kStatus;    // keep the interrupt for 68k
    SYS_UINT32                  tx68kCurListNum; // keep the track of used txList for 68k DMA
    SYS_UINT32                  rx68kCurListNum; // keep the track of used rxList for 68K DMA
//  SYS_UINT8                   inInt68KProc;    // keep the track of function called inside of interrupt   
#endif

#if !ECFG_CHIP_ZIP1
    // host buffer descriptors
    SYS_UINT32                  rx_linebuf;
    SYS_UINT32                  tx_linebuf[MAX_TX_PRIORITY];
    // host entry queues (packet buffers) - virtual and physical addresses
    SYS_UINT32                  *rx_entry_queue;
    SYS_UINT32                  *tx_entry_queue[MAX_TX_PRIORITY];
    SYS_UINT32                  rx_entry_queue_pa;
    SYS_UINT32                  tx_entry_queue_pa[MAX_TX_PRIORITY];
    // host descriptor lists
    struct pkt_entry*           tx_pkt_descs[MAX_TX_PRIORITY];
    // entry queue sizes
    SYS_UINT32                  rx_pkt_size;
    SYS_UINT32                  tx_pkt_size;
    // protocol descriptor ring, size, and index of current entry
    SYS_UINT32                  pd_queue[4];
    SYS_UINT32                  pd_entries;
    SYS_UINT32                  pd_entry_widx[MAX_TX_PRIORITY];
    // Release queues
    SYS_UINT32 tx_release_queue[MAX_TX_PRIORITY];
    SYS_UINT32 tx_release_rindex[MAX_TX_PRIORITY];
    SYS_UINT32 tx_release_q_size[MAX_TX_PRIORITY];

#endif

} ;

typedef struct _control_context dc_context_t;

#endif /* __control_context_pci_h__ */
