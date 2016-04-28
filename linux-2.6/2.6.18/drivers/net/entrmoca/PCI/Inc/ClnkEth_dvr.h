/*******************************************************************************
*
* PCI/Inc/ClnkEth_dvr.h
*
* Description: Common Ethernet Module
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

#ifndef __CLNK_ETH_DVR_H__
#define __CLNK_ETH_DVR_H__

/*******************************************************************************
*                             # i n c l u d e s                                *
********************************************************************************/
#include "inctypes_dvr.h"
#include "ClnkMbx_dvr.h"
#include "common_dvr.h"
#if defined(CLNK_ETH_BRIDGE) 
#include "ClnkCam_dvr.h"
#endif

#if ECFG_CHIP_ZIP1
#include "hw_z1_dvr.h"
#else  
#include "hw_z2_dvr.h"
#endif

#include "drv_ctl_opts.h"

/*******************************************************************************
*                             # d e f i n e s                                  *
********************************************************************************/

// Common Ethernet Module Configuration
#define CLNK_ETH_LINK_STATUS  // Check link status (for debugging only)
#define CLNK_ETH_DEBUG        // Debug code (for debugging only)
#define CLNK_ETH_PHY_DATA_LOG // Phy data logging (for debugging only)
#define CLNK_ETH_BRIDGE_BCAST // Bridge broadcast (for debugging only)
#define CLNK_ETH_ECHO_PROFILE // Echo profile (for debugging only)

// Maximum Number of Fragments
#define CLNK_ETH_MAX_FRAGMENTS  8

// Link status
#define CLNK_ETH_LINK_DOWN  0
#define CLNK_ETH_LINK_UP    1

// Number of SW and HW Queues, etc. depends on Mesh vs Access ...

#define TMR_2_SLOWJIFFIES            (11)

// The following compile-time options are used to enable/disable Ethernet
// FCS generation and verification in the host c.LINK driver for PCI mode. 
// Enabling these options will affect throughput performances (specially tx)
// depending on host cpu processing capabilities.
#define CLNK_RX_ETH_FCS_ENABLED    0   // 1:enable, 0:disable
#define CLNK_TX_ETH_FCS_ENABLED    0   // 1:enable, 0:disable

#define MAX_TAGGED_ETH_PKT_LEN     1522   // VLAN tagged Ethernet packet w/ FCS
#define MAX_UNTAGD_ETH_PKT_LEN     1518   // Untagged Ethernet packet w/ FCS

// Transmit and Receive FIFO sizes
#define _CLUSTER_BYTES(N)         ((N) * 128)
#ifdef CLNK_ETH_ZIP_10 /* ZIP_10: 511 Clusters, Max of 255 per queue */
  // ZIP_10 had 511 clusters but only 255 per queue. So why this ??
  #define MAX_RX_FIFO_SIZE          10240
  #define MAX_TX_FIFO_SIZE          13824
#else
  #ifdef CLNK_ETH_FPGA_HW
    #define MAX_RX_FIFO_SIZE          (10240/2)
    #define MAX_TX_FIFO_SIZE          (13824/2)
  #else /* ZIP_1B and ZIP_1C: 511 Cluster, 5 Rcv Pkt, 37+ Xmit Pkt */
    #define MAX_RX_FIFO_SIZE          (_CLUSTER_BYTES(5*12))
    #define MAX_TX_FIFO_SIZE          (_CLUSTER_BYTES(511) - MAX_RX_FIFO_SIZE)
  #endif
#endif

// Transmit DMA Hardware Constants
#define MAX_TX_FIFOS_SOC          8
#define MAX_TX_FRAGMENTS          8
#define MAX_TX_PACKETS_PER_INT    64

// Receive DMA Hardware Constants
#define MAX_RX_LISTS              3
#define MAX_RX_FRAGMENTS          3
#define MAX_RX_PACKETS_PER_INT    64

#ifdef __GNUC__
#define __UNUSED        __attribute__((unused))
#else
#define __UNUSED        /* */
#endif

// reset timeouts
// loop count for polling for CCPU up at 25 usec rate
#define CCPU_RESET_LIMIT    600 // CCPU should come up in about 2.675 msec (107 polls)
// loop count for polling for TC to respond after a reset with a polling rate of 1 usec
#define TC_RESET_LIMIT      10  // TC should come up in about 1 usec

/*******************************************************************************
*                       G l o b a l   D a t a   T y p e s                      *
********************************************************************************/




// Fragment Structure
typedef struct
{
    SYS_UINT32 ptr;
    SYS_UINT32 len;

#ifdef CLNK_ETH_MMU
    void*      ptrVa;
#endif
}
Clnk_ETH_Fragment_t;

// Fragment List Structure
typedef struct
{
    SYS_UINT32          totalLen;
    SYS_UINT32          numFragments;
    Clnk_ETH_Fragment_t fragments[CLNK_ETH_MAX_FRAGMENTS];
}
Clnk_ETH_FragList_t;



typedef void
(*Clnk_ETH_SendPacketCallback)(void* dk_ctx, SYS_UINT32 packetIDs[],
                               SYS_UINT32 numIDs);

typedef void
(*Clnk_ETH_RcvPacketCallback)(void* dk_ctx, SYS_UINT32 listNums[],
                              SYS_UINT32 packetIDs[], SYS_UINT16 lengths[],
                              SYS_UINT16 errCodes[], SYS_UINT32 numIDs);

#if (FEATURE_QOS && ECFG_CHIP_ZIP1)
typedef void
(*Clnk_ETH_TFifoReszCallback)(void* dk_ctx);
#endif /* FEATURE_QOS && ECFG_CHIP_ZIP1 */


// Fragment Structure
typedef struct
{
    SYS_UINT32 ptr;
    SYS_UINT32 len;
}
Fragment_t;

// Receive Host Descriptor Structure
// This structure is used to control the receive DMA hardware.
typedef struct RxHostDesc_t
{
    SYS_UINT32 pNext;                       // Pointer to next descriptor
    SYS_UINT32 statusCtrl;                  // Status/Control
    SYS_UINT32 fragmentLen;                 // Number of fragments
    Fragment_t fragments[MAX_RX_FRAGMENTS]; // Fragments
}
RxHostDesc_t;

// Transmit Host Descriptor Structure
// This structure is used to control the transmit DMA hardware.
typedef struct TxHostDesc_t
{
    SYS_UINT32 pNext;                       // Pointer to next descriptor
    SYS_UINT32 statusCtrl;                  // Status/Control
    SYS_UINT32 fragmentLen;                 // Number of fragments
    Fragment_t fragments[MAX_TX_FRAGMENTS]; // Fragments
}
TxHostDesc_t;

// Receive List Entry Structure
// This defines the structure for an entry in the receive lists.  The
// receive lists include the currently receiving list and the receive
// free list.
typedef struct
{
    ListEntry_t   listEntry;
    RxHostDesc_t* pRxHostDesc;   // Pointer to receive host descriptor
    SYS_UINT32    packetID;      // Packet ID

#ifdef CLNK_ETH_MMU
    SYS_UINT32    rxHostDescPa;  // Physical address of host descriptor
    void*         fragmentVa;    // Virtual address of first fragment
#endif
}
RxListEntry_t;

// Transmit List Entry Structure
// This defines the structure for an entry in the transmit lists.  The
// tranmsit lists include the currently transmitting list and the transmit
// free lists.
typedef struct
{
    ListEntry_t   listEntry;
    TxHostDesc_t* pTxHostDesc;   // Pointer to transmit host descriptor
    SYS_UINT32    packetID;      // Packet ID

#ifdef CLNK_ETH_MMU
    SYS_UINT32    txHostDescPa;  // Physical address of host descriptor
    void*         fragmentVa;    // Virtual address of first fragment
#endif
}
TxListEntry_t;

struct pkt_entry
{
    ListEntry_t             listEntry;
    struct ptr_list         *plist;         /* ptr list in entry queue */
    void                    *skb_data;      /* VA of pkt data */
    int                     status_idx;     /* index into entry queue */
    SYS_UINT16              nodeID;         /* node from CAM lookup */
    SYS_UINT16              packetLEN;      /* length for statistics */
    SYS_UINT32              packetID;       /* packet ID (for eth.c) */
};





// Endian Swap Macro (used for non-packet data which is read/written by the
// cLINK SOC NDIS engine / DIC TC from Host DRAM; includes Rx and Tx Host
// Descriptors, and pointer mode linebuffer entries, a.k.a. scatter/gather
// lists).  See CLNK_ETH_ENDIAN_SWAP define.
// See also the MISC_CONTROL register (0x310) bit (1<<2), on Zip1.
// (In contrast, host access to SOC memory swapping via clnk_reg_read/WRITE)

#define HOST_OS_ENDIAN_SWAP(val)  ((((val) & 0xff000000) >> 24) | \
                                  (((val) & 0x00ff0000) >> 8)   | \
                                  (((val) & 0x0000ff00) << 8)   | \
                                  (((val) & 0x000000ff) << 24))

// Link State Macros
// Note that set can only be used when the embedded is not running.
// Currently this is only used before downloading firmware.
#define GET_LINK_STATUS_REG(ctx,status)                                  \
            clnk_reg_read(ctx, CLNK_REG_MBX_READ_CSR, status);  \
            *(SYS_UINT32 *)(status) &= 0xffff
#define SET_LINK_STATUS_REG(ctx,status)                                  \
            clnk_reg_write(ctx, CLNK_REG_MBX_READ_CSR, status & 0xffff)
#define GET_RESET_REASON(ctx,status)                                     \
            clnk_reg_read(ctx, CLNK_REG_DEBUG_0, status);                \
            *(SYS_UINT32 *)(status) &= 0xffff
#define SET_RESET_REASON(ctx,status)                                     \
            clnk_reg_write(ctx, CLNK_REG_DEBUG_0, status);               \

#define IS_LINK_STATUS_UP(status)    ((status & 0xff) == CLNK_ETH_LINK_UP)
#define IS_LINK_STATUS_DOWN(status)  ((status & 0xff) == CLNK_ETH_LINK_DOWN)
#define MY_LINK_STATE(status)        ((status >> 8) & 0xff)


int Clnk_ETH_Initialize_drv(void                  *vcp, 
                            Clnk_ETH_Firmware_t   *fw_setup,
                            struct clnk_hw_params *hparams );
void clnkrst_init_drv(void *vcp);
void Clnk_ETH_Terminate_drv(void* vcp);
int Clnk_ETH_Stop(void* vcp);
int Clnk_ETH_Open(void* vcp);
int Clnk_ETH_Close(void* vcp);
void Clnk_ETH_HandleRxInterrupt(void* vcp);
void Clnk_ETH_HandleTxInterrupt(void* vcp);
void Clnk_ETH_HandleMiscInterrupt(void* vcp);
int Clnk_ETH_SendPacket(void* vcp, Clnk_ETH_FragList_t* pFragList,
                        SYS_UINT32 priority, SYS_UINT32 packetID, SYS_UINT32 flags);
int Clnk_ETH_RcvPacket(void* vcp, Clnk_ETH_FragList_t* pFragList,
                       SYS_UINT32 listNum, SYS_UINT32 packetID, SYS_UINT32 flags);
void MbxReplyRdyCallback(void* vcp, Clnk_MBX_Msg_t* pMsg);
int MbxSwUnsolRdyCallback(void* vcp, Clnk_MBX_Msg_t* pMsg);
int Clnk_ETH_IsSoCBooted(void* vcp);

#endif /* __CLNK_ETH_DVR_H__ */


/* End of File: ClnkEth_dvr.h */

