/*******************************************************************************
*
* PCI/Src/hw_z2_pci_dvr.c
*
* Description: Provides hardware-specific functions for Zip2 in PCI mode
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

#include "drv_hdr.h"


#if MAX_TX_PRIORITY > 4
// The upper limit of 4 is hard-coded into: dc_context_t struct, mb_return struct
#error Zip2 supports at most 4 priorities.
#endif

// Host Rx/Tx entry queue magic numbers
#define RX_EQ_MAGIC    0xcafebeef
#define TX_EQ_MAGIC    0xfeedbeef

// Host line buffer entry status exchanged between host and SoC
#define BUFF_DESC_STATE_FREE   0
#define BUFF_DESC_STATE_GOOD   1
#define BUFF_DESC_STATE_BAD    2

#ifndef SONICS_ADDRESS_OF_sniff_buffer_pa
#error "Sniff buffer is not defined"
#else
#if SONICS_ADDRESS_OF_sniff_buffer_pa

unsigned int *sniff_buffer_pa;		// physical address
unsigned int *sniff_buffer;		// Virtuial address

#define HOST_RING_SIZE 0x400000		// Entire size of ring in bytes
#define HOST_ENTRY_SIZE 0x100		// Each entry size in bytes
#endif
#endif

int clnk_hw_init(dc_context_t *dccp, struct clnk_hw_params *hparams)
{
    void *ddcp = dc_to_dd( dccp ) ;
    void *pMem, *pa;
    int pri;
    SYS_UINT32 i;

    dccp->rxMemSize = sizeof(struct pkt_entry) * (hparams->numRxHostDesc);
    pMem = (SYS_UINT8*)HostOS_Alloc(dccp->rxMemSize);
    if (pMem == SYS_NULL)
    {
        Clnk_ETH_Terminate_drv(dccp);
        return (CLNK_ETH_RET_CODE_MEM_ALLOC_ERR);
    }
    dccp->pRxMemBlock = pMem;

    dccp->txMemSize = sizeof(struct pkt_entry)
                        * (hparams->numTxHostDesc) * MAX_TX_PRIORITY;
    pMem = (SYS_UINT8*)HostOS_Alloc(dccp->txMemSize);
    if (pMem == SYS_NULL)
    {
        Clnk_ETH_Terminate_drv(dccp);
        return (CLNK_ETH_RET_CODE_MEM_ALLOC_ERR);
    }
    dccp->pTxMemBlock = pMem;

    dccp->txNumHostDesc       = hparams->numTxHostDesc;
    dccp->rxNumHostDesc       = hparams->numRxHostDesc;

    dccp->rx_entry_queue = HostOS_AllocDmaMem(ddcp,
            dccp->rxNumHostDesc * sizeof(struct ptr_list), &pa);
    if (! dccp->rx_entry_queue)
    {
        Clnk_ETH_Terminate_drv(dccp);
        return (CLNK_ETH_RET_CODE_MEM_ALLOC_ERR);
    }

#ifndef SONICS_ADDRESS_OF_sniff_buffer_pa
#error "Sniff buffer is not defined"
#else
#if SONICS_ADDRESS_OF_sniff_buffer_pa

    // #########################################################################
    // Allocate the buffer to be used to store the descriptors logged from the
    // SoC.
    // #########################################################################
    sniff_buffer = HostOS_AllocDmaMem(ddcp, HOST_RING_SIZE , (void **) &sniff_buffer_pa);
    if (! sniff_buffer)
    {
        Clnk_ETH_Terminate_drv(dccp);
        return (CLNK_ETH_RET_CODE_MEM_ALLOC_ERR);
    }
    HostOS_PrintLog(L_INFO, "Sniff buffer at physical memory %08x and virtual address %08x\n",sniff_buffer_pa, sniff_buffer);
#endif
#endif

    /* PCI/physical addresses fit in 32 bits */
    dccp->rx_entry_queue_pa = (SYS_UINT32)(SYS_UINTPTR)pa;
    HostOS_Memset(dccp->rx_entry_queue, 0,
                  dccp->rxNumHostDesc * sizeof(struct ptr_list));

    /*
     * Set up magic number for rx ptr_list entry DMA verfication from the SoC
     */
    for (i = 0; i < dccp->rxNumHostDesc; i++)
        SET_HOST_DESC(&(((struct ptr_list *)(dccp->rx_entry_queue))[i]),
                      magic, RX_EQ_MAGIC);

    for (pri = 0; pri < MAX_TX_PRIORITY; pri++)
    {
        dccp->tx_entry_queue[pri] = HostOS_AllocDmaMem(ddcp,
                dccp->txNumHostDesc * sizeof(struct ptr_list), &pa);
        if (! dccp->tx_entry_queue[pri])
        {
            Clnk_ETH_Terminate_drv(dccp);
            return (CLNK_ETH_RET_CODE_MEM_ALLOC_ERR);
        }
        dccp->tx_entry_queue_pa[pri] = (SYS_UINT32)(SYS_UINTPTR)pa;
        HostOS_Memset(dccp->tx_entry_queue[pri], 0,
                      dccp->txNumHostDesc * sizeof(struct ptr_list));

        /*
         * Set up magic number for tx ptr_list entry DMA verfication from the
         * SoC
         */
        for (i = 0; i < dccp->txNumHostDesc; i++)
            SET_HOST_DESC(&(((struct ptr_list *)(dccp->tx_entry_queue[pri]))[i]),
                          magic, TX_EQ_MAGIC);
    }

    return(SYS_SUCCESS);
}

int clnk_hw_term(dc_context_t *dccp)
{
    void *ddcp = dc_to_dd( dccp ) ;
    int pri;

    if (dccp->pRxMemBlock)
    {
        HostOS_Free(dccp->pRxMemBlock, dccp->rxMemSize);
        dccp->pRxMemBlock = SYS_NULL;
    }
    if (dccp->pTxMemBlock)
    {
        HostOS_Free(dccp->pTxMemBlock, dccp->txMemSize);
        dccp->pTxMemBlock = SYS_NULL;
    }
    if (dccp->rx_entry_queue)
    {
        HostOS_FreeDmaMem(ddcp,
                          dccp->rxNumHostDesc * sizeof(struct ptr_list),
                          dccp->rx_entry_queue,
                          (void *)(SYS_UINTPTR)dccp->rx_entry_queue_pa);
        dccp->rx_entry_queue = SYS_NULL;
    }
    for (pri = 0; pri < MAX_TX_PRIORITY; pri++)
    {
        if (dccp->tx_entry_queue[pri])
        {
            HostOS_FreeDmaMem(ddcp,
                              dccp->txNumHostDesc * sizeof(struct ptr_list),
                              dccp->tx_entry_queue[pri],
                              (void *)(SYS_UINTPTR)dccp->tx_entry_queue_pa[pri]);
            dccp->tx_entry_queue[pri] = SYS_NULL;
        }
    }
#ifndef SONICS_ADDRESS_OF_sniff_buffer_pa
#error "Sniff buffer is not defined"
#else
#if SONICS_ADDRESS_OF_sniff_buffer_pa
        // #####################################################################
        // Clean up by deleting the hold on the memory used for the sniff buffer
        // #####################################################################
	if (sniff_buffer)
		HostOS_FreeDmaMem(ddcp,HOST_RING_SIZE,sniff_buffer,sniff_buffer_pa);
#endif
#endif

    return(SYS_SUCCESS);
}

int clnk_hw_open(dc_context_t *dccp)
{
    SYS_UINT32 val;
    struct pkt_entry *pEntry;

    if (! dccp->socBooted)
        return(SYS_SUCCESS);

    /* enable periodic timer for CAM */
    clnk_reg_read(dccp, CSC_SYS_TMR_LO, &val);
    clnk_reg_write(dccp, CSC_TMR_TIMEOUT_0, val + (50*1000*1000 / CAM_TMR_JIFFIES));
    clnk_reg_write(dccp, CSC_TMR_MASK_0, 0xfffffff1);

    pEntry = COMMON_LIST_HEAD(struct pkt_entry, &dccp->rxList[0]);
    clnk_reg_write(dccp, DIC_D_RX_DST_IDX, pEntry->status_idx);
    clnk_reg_write(dccp, DIC_D_RX_DST_PTR, pEntry->status_idx * sizeof(struct ptr_list));

    /* enable TX/RX */
    dccp->txIsOpen = dccp->rxIsOpen = SYS_TRUE;
    clnk_reg_write(dccp, DEV_SHARED(rx_active), 1);

    return (CLNK_ETH_RET_CODE_SUCCESS);
}

int clnk_hw_close(dc_context_t *dccp)
{
    dccp->txIsOpen = dccp->rxIsOpen = SYS_FALSE;
    clnk_reg_write(dccp, DEV_SHARED(rx_active), 0);
    HostOS_Sleep(10);   /* wait for any remaining DMAs to complete */
    return (CLNK_ETH_RET_CODE_SUCCESS);
}

int clnk_hw_tx_detach(dc_context_t* dccp, SYS_UINT32* pPacketID)
{
    int i;

    // Check if transmit DMA hardware is enabled
    if (dccp->txIsOpen)
    {
        return (-SYS_INPUT_OUTPUT_ERROR);
    }

    for (i = 0; i < MAX_TX_PRIORITY; i++)
    {
        struct pkt_entry *pEntry;

        if (COMMON_LIST_HEAD(void, &dccp->txQueuedPkts[i]))
        {
            pEntry = (struct pkt_entry *) COMMON_LIST_REM_HEAD(&dccp->txQueuedPkts[i]);
            *pPacketID = pEntry->packetID;
            COMMON_LIST_ADD_TAIL(&dccp->txPriFreeList[i], pEntry);
            return (SYS_SUCCESS);
        }
    }

    return (-SYS_INPUT_OUTPUT_ERROR);
}

int clnk_hw_rx_detach(dc_context_t* dccp, SYS_UINT32* pPacketID)
{
    if (dccp->rxIsOpen)
    {
        return (-SYS_INPUT_OUTPUT_ERROR);
    }

    if (COMMON_LIST_HEAD(void, &dccp->rxList[0]))
    {
        struct pkt_entry * pEntry;
        pEntry = (struct pkt_entry *) COMMON_LIST_REM_HEAD(&dccp->rxList[0]);
        *pPacketID = pEntry->packetID;
        COMMON_LIST_ADD_TAIL(&dccp->rxFreeList[0], pEntry);
        return (SYS_SUCCESS);
    }

    return (-SYS_INPUT_OUTPUT_ERROR);
}

//#define RX_DEBUG

void clnk_hw_rx_intr(dc_context_t *dccp)
{
    struct pkt_entry *pEntry;
    SYS_UINT32     val;
    SYS_UINT32     listNums[MAX_RX_PACKETS_PER_INT];
    SYS_UINT32     packetIDs[MAX_RX_PACKETS_PER_INT];
    SYS_UINT16     lengths[MAX_RX_PACKETS_PER_INT];
    SYS_UINT16     errCodes[MAX_RX_PACKETS_PER_INT];
    SYS_UINT32     numIDs = 0;

    // Check if receive DMA hardware is enabled
    if (!dccp->socBooted || !dccp->rxIsOpen)
    {
        return;
    }

    while (numIDs < MAX_RX_PACKETS_PER_INT)
    {
        SYS_UINT32 status_ptr, status;

        pEntry = COMMON_LIST_HEAD(struct pkt_entry, &dccp->rxList[0]);
        if (! pEntry)
            break;

        status_ptr = RX_STATUS_PTR(dccp, pEntry->status_idx);

        /* check RX host BD entry status */
        clnk_reg_read(dccp, status_ptr, &val);
        status = (val & 0x000f);

#ifdef RX_DEBUG
        HostOS_PrintLog(L_INFO, "Read status @%x w/ value=%x,\n",
                         status_ptr, val);
#endif /* RX_DEBUG */
        if ((status == BUFF_DESC_STATE_GOOD) || (status == BUFF_DESC_STATE_BAD))
        {
#ifdef RX_DEBUG
            SYS_UINT8  *src = pEntry->skb_data;
            SYS_UINT32 i;
#endif /* RX_DEBUG */
            SYS_UINT32 len;

            /* Length field in the upper 16 bits of the entry status field */
            len = (val >> 16);

            if (len > MAX_TAGGED_ETH_PKT_LEN)
            {
                HostOS_PrintLog(L_ERR, "RX packet (len=%u) too big!!!\n"
                                 "packet idx = %u!!!\n",
                                 len, pEntry->status_idx);
                len = MAX_TAGGED_ETH_PKT_LEN;
                status = BUFF_DESC_STATE_BAD;
                dccp->stats.rxLengthErrs++;
            }
            if (len < 32)
            {
                HostOS_PrintLog(L_ERR, "RX packet (len=%u) too small!!!\n"
                                 "packet idx = %u!!!\n",
                                 len, pEntry->status_idx);
                len = 32;
                status = BUFF_DESC_STATE_BAD;
                dccp->stats.rxLengthErrs++;
            }

            if (val & 0x00000010)      /* Ethernet FCS included */
            {
                len -= 4;              /* Ethernet FCS stripped off */
#if (CLNK_RX_ETH_FCS_ENABLED)
                /*
                ** NOTE: The following sample code provides Ethernet FCS
                ** verification (typically done in hw) for potential improvement
                ** in aggregated packet error rate.
                **
                ** If packet status is bad, then compute CRC for the Ethernet
                ** payload and verify it against the trailing FCS. In case of a
                ** match, mark the packet as good.
                **/
                if (status == BUFF_DESC_STATE_BAD)
                {
                    SYS_UINT8  *src = pEntry->skb_data;
                    SYS_UINT32 crc32 = compute_crc32(src, len, 0);
                    if (crc32 == *(SYS_UINT32 *)(src + len))
                    {
                        status = BUFF_DESC_STATE_GOOD;
                    }
                    else
                    {
                        status = BUFF_DESC_STATE_BAD;
                        dccp->stats.rxCrc32Errs++;
                    }
                    HostOS_PrintLog(L_INFO, "rx len=%u, crc=0x%08x, "
                                    "orig crc=0x%08x\n", len, crc32,
                                    *(SYS_UINT32 *)(src + len));
                }
#endif /* CLNK_RX_ETH_FCS_ENABLED */
            }

#ifdef RX_DEBUG
            if (status == BUFF_DESC_STATE_GOOD)
            {
                HostOS_PrintLog(L_INFO, "Good packet received, len=%u, "
                                 "skb_data=%lx, packet idx = %u!!!\n",
                                 len, pEntry->skb_data, pEntry->status_idx);
            }
            else
            {
                HostOS_PrintLog(L_ERR, "Bad packet received, len=%u, "
                                 "skb_data=%lx, packet idx = %u!!!\n",
                                 len, pEntry->skb_data, pEntry->status_idx);
            }
            for(i = 0; i < 64; i+=8)
            {
                HostOS_PrintLog(L_INFO, "%02x %02x %02x %02x "
                             "%02x %02x %02x %02x\n",
                             (SYS_UINT32)src[i], (SYS_UINT32)src[i+1],
                             (SYS_UINT32)src[i+2], (SYS_UINT32)src[i+3],
                             (SYS_UINT32)src[i+4], (SYS_UINT32)src[i+5],
                             (SYS_UINT32)src[i+6], (SYS_UINT32)src[i+7]);
            }
#endif /* RX_DEBUG */

            if (status == BUFF_DESC_STATE_GOOD)
            {
                dccp->stats.rxPacketsGood++;
                errCodes[numIDs] = 0;
            }
            else
            {
                dccp->stats.rxPacketErrs++;
                errCodes[numIDs] = 1;
            }

            dccp->stats.rxPackets++;
            dccp->stats.rxBytes += len;

            listNums[numIDs] = 0;
            packetIDs[numIDs] = pEntry->packetID;
            lengths[numIDs] = len;
            numIDs++;

            /* free the entry */
            //clnk_reg_write(dccp, status_ptr, BUFF_DESC_STATE_FREE);

            (void)COMMON_LIST_REM_HEAD(&dccp->rxList[0]);
            COMMON_LIST_ADD_TAIL(&dccp->rxFreeList[0], pEntry);
        }
        else
        {
            break;
        }
    }

    if ((numIDs != 0) && (dccp->rcvPacketCallback != SYS_NULL))
    {
        dccp->rcvPacketCallback(dccp->p_dk_ctx, listNums,
                                    packetIDs, lengths, errCodes, numIDs);
    }

    return;
}

static void clnk_hw_tx_prime(dc_context_t *dccp, int priority)
{
    while (COMMON_LIST_HEAD(void, &dccp->txQueuedPkts[priority]))
    {
        SYS_UINT32 addr = dccp->pd_queue[priority] + (dccp->pd_entry_widx[priority] << 3);
        SYS_UINT32 val;
        struct pkt_entry *pEntry;

        clnk_reg_read(dccp, addr, &val);
        if (val & 1)
        {
            return; // No PD's ...
        }

        if ((++dccp->pd_entry_widx[priority]) == dccp->pd_entries)
            dccp->pd_entry_widx[priority] = 0;

        pEntry = (struct pkt_entry *) COMMON_LIST_REM_HEAD(&dccp->txQueuedPkts[priority]);

        /* waste some time */
        dccp->stats.txPackets++;
        dccp->stats.txPacketsGood++;
        dccp->stats.txBytes += pEntry->packetLEN;

        /* write PD */
        clnk_reg_write(dccp, addr + 4, pEntry->status_idx << 16 | pEntry->status_idx << 8);
        clnk_reg_write(dccp, addr, (pEntry->packetLEN << 16) | (pEntry->nodeID << 4) | (CLNK_TX_ETH_FCS_ENABLED << 3) | 1);
    }

    return;
}

void clnk_hw_tx_intr(dc_context_t *dccp)
{
    int pri;

    // Check if transmit DMA hardware is enabled
    if (!dccp->socBooted || !dccp->txIsOpen)
    {
        return;
    }

    for (pri = MAX_TX_PRIORITY - 1; pri >= 0; pri--)
    {
        SYS_UINT32 wIdx;
        SYS_UINT32 descIdx;
        SYS_UINT32 packetIDs[MAX_TX_PACKETS_PER_INT];
        SYS_UINT32 numIDs = 0;
        struct pkt_entry *pEntry;
        
        /* Read current release queue tail pointer from linebuffer descriptor in shared memory */
        clnk_reg_read(dccp, dccp->tx_linebuf[pri] +
                      offsetof(struct linebuf, entry_windex),
                      &wIdx);

        while (dccp->tx_release_rindex[pri] != wIdx)
        {
            /* Read next descriptor index from head of release queue  */
            clnk_reg_read(dccp, dccp->tx_release_queue[pri] + dccp->tx_release_rindex[pri] * 4, &descIdx);
            dccp->tx_release_rindex[pri] = ++dccp->tx_release_rindex[pri] % dccp->tx_release_q_size[pri];
            pEntry = dccp->tx_pkt_descs[pri] + descIdx; 

            COMMON_LIST_ADD_TAIL(&dccp->txPriFreeList[pri], pEntry);
            packetIDs[numIDs] = pEntry->packetID;

            if ((++numIDs) == MAX_TX_PACKETS_PER_INT)
                break;
        }
        if (numIDs)
        {
            clnk_hw_tx_prime(dccp, pri);
            if (dccp->sendPacketCallback)
                dccp->sendPacketCallback(dccp->p_dk_ctx, packetIDs, numIDs);
        }
    }

    return;
}

int clnk_hw_tx_pkt(dc_context_t *dccp, Clnk_ETH_FragList_t* pFragList,
                   SYS_UINT32 priority, SYS_UINT32 packetID, SYS_UINT32 flags)
{
    struct pkt_entry *pEntry;
    struct ptr_list  *plist;
    SYS_UINT32       frag, nFrags;
    SYS_INT32        nodeid;
    void             *ptrVa = pFragList->fragments[0].ptrVa;

    // Check if transmit DMA hardware is enabled
    if (!dccp->txIsOpen)
        return (CLNK_ETH_RET_CODE_NOT_OPEN_ERR);

    // Check packet length
    if ((IS_CLNK_ETH_PKT_TAGGED(ptrVa) && (pFragList->totalLen > 
            MAX_TAGGED_ETH_PKT_LEN - (CLNK_TX_ETH_FCS_ENABLED ? 0 : 4))) ||
        (!IS_CLNK_ETH_PKT_TAGGED(ptrVa) && (pFragList->totalLen > 
            MAX_UNTAGD_ETH_PKT_LEN - (CLNK_TX_ETH_FCS_ENABLED ? 0 : 4))))
    {
        HostOS_PrintLog(L_ERR, "TX packet (len=%u)too big!!!\n", 
                        pFragList->totalLen );
        dccp->txDroppedErrs++;
        dccp->txPriUFloodCnt[priority]++;
        return (CLNK_ETH_RET_CODE_PKT_LEN_ERR);
    }

#ifdef CLNK_ETH_BRIDGE
    Cam_LookupSrc(&dccp->cam,
                  CLNK_ETH_GET_SRC_MAC_HI(ptrVa),
                  CLNK_ETH_GET_SRC_MAC_LO(ptrVa));
#endif

    nodeid = Cam_LookupDst(&dccp->cam,
                           CLNK_ETH_GET_DST_MAC_HI(ptrVa),
                           CLNK_ETH_GET_DST_MAC_LO(ptrVa));

    if (nodeid == -1)
    {   // Drop a packet destined to unclaimed device most of the time
        // HostOS_PrintLog(L_INFO, "NEW CAM - FLOOD DROP\n");
        dccp->txDroppedErrs++;
        dccp->txPriUFloodCnt[priority]++;
        return (CLNK_ETH_RET_CODE_UCAST_FLOOD_ERR);
    }

    // A better way to achieve aggregation across all priorities might be
    // to zero out the contents of the priority lookup table when the
    // CLNK_DEF_SW_CONFIG_AGGREGATION_METHOD_BIT bit is set
    if (dccp->fw.swConfig & CLNK_DEF_SW_CONFIG_AGGREGATION_METHOD_BIT)
        priority = 0;

#if FEATURE_QOS
    else if (dccp->fw.PQoSClassifyMode == CLNK_PQOS_MODE_PUPQ_CLASSIFY) 
    {
        // For Pre-UPnP PQoS mode, flow destination packets do not require
        // VLAN tags to be promoted to PQoS priority. Also, any packet to a
        // unicast destination node to which there is flow(s) present from
        // this ingress node gets PQoS proirity.
        if ((nodeid & 0x100) || (dccp->qos_flows_per_dst[nodeid] && 
            (nodeid != CLNKMAC_BCAST_ADDR)))
        {
            priority = 3;
        }
    }
    else if (nodeid & 0x100)
    {
        // For MoCA 1.1 PQoS mode, promote flow destination packets to 
        // priority 3 if the packets were originally assigned middle MoCA
        // priority (i.e., VLAN priority 4 or 5).
        if (priority == 1)
        {
            priority = 3;
        }
        // Otherwise, turn multicast flow destination into MoCA broadcast.
        else if (CLNK_ETH_GET_DST_MAC_HI(ptrVa) & 0x01000000)
        {
            nodeid = CLNKMAC_BCAST_ADDR;
        }
    }
#endif //FEATURE_QOS

    pEntry = (struct pkt_entry *) COMMON_LIST_REM_HEAD(&dccp->txPriFreeList[priority]);
    if (pEntry == SYS_NULL)
    {
        // HostOS_PrintLog(L_DBG, "warning: no tx host desc\n");
        dccp->stats.txFifoFullErrs++;
        dccp->txPriDropCnt[priority]++; // Extra Monitor Info
        return (CLNK_ETH_RET_CODE_NO_HOST_DESC_ERR);
    }

    nFrags = pFragList->numFragments;

    if (nFrags == 0 || nFrags > CLNK_HW_MAX_FRAGMENTS)
    {
        return (CLNK_ETH_RET_CODE_NO_HOST_DESC_ERR); // need new errcode
    }

    pEntry->packetLEN = pFragList->totalLen;
    pEntry->packetID = packetID;
    pEntry->nodeID = nodeid;

    plist = (void *)pEntry->plist;

    SET_HOST_DESC(plist, n_frags, nFrags);
    for (frag = 0; frag < nFrags; frag++)
    {   // Consider adding further error checks. (alignment, etc)
        SET_HOST_DESC(plist, fragments[frag].ptr, pFragList->fragments[frag].ptr);
        SET_HOST_DESC(plist, fragments[frag].len, pFragList->fragments[frag].len);
    }

    COMMON_LIST_ADD_TAIL(&dccp->txQueuedPkts[priority], pEntry);

    clnk_hw_tx_prime(dccp, priority);

    return (CLNK_ETH_RET_CODE_SUCCESS);
}

int clnk_hw_rx_pkt(dc_context_t *dccp, Clnk_ETH_FragList_t* pFragList,
                   SYS_UINT32 listNum, SYS_UINT32 packetID, SYS_UINT32 flags)
{
    struct pkt_entry *pEntry;
    struct ptr_list *plist;
    SYS_UINT32 status_ptr;

    pEntry = COMMON_LIST_HEAD(struct pkt_entry, &dccp->rxFreeList[0]);
    if (pEntry == SYS_NULL)
    {
        return (CLNK_ETH_RET_CODE_NO_HOST_DESC_ERR);
    }
    (void)COMMON_LIST_REM_HEAD(&dccp->rxFreeList[0]);

    pEntry->skb_data = (void *)pFragList->fragments[0].ptrVa;
    pEntry->packetID = packetID;

    plist = (void *)pEntry->plist;

    SET_HOST_DESC(plist, n_frags, 1);
    SET_HOST_DESC(plist, fragments[0].ptr, pFragList->fragments[0].ptr);
    SET_HOST_DESC(plist, fragments[0].len, pFragList->fragments[0].len);

    /* free the entry */
    status_ptr = RX_STATUS_PTR(dccp, pEntry->status_idx);
    clnk_reg_write(dccp, status_ptr, BUFF_DESC_STATE_FREE);

    COMMON_LIST_ADD_TAIL(&dccp->rxList[0], pEntry);

    return (CLNK_ETH_RET_CODE_SUCCESS);
}

void clnk_hw_misc_intr(dc_context_t *dccp, SYS_UINT32 *statusVal)
{
    SYS_UINT32 val;

    if (dccp->timer_intr)
    {
        dccp->timer_intr = 0;
        clnk_reg_read(dccp, CSC_SYS_TMR_LO, &val);
        clnk_reg_write(dccp, CSC_TMR_TIMEOUT_0, val + (50*1000*1000 / CAM_TMR_JIFFIES));
        Cam_Bump(&dccp->cam);
    }
    return;
}

/********************************************************************
* Function Name: clnk_hw_desc_init
* Parameters:
*   dccp - pointer to clink context
* Description:
*   Allocates and initializes the PCI hardware descriptors
* Returns:
*   None
* Notes:
*   This function is called from the reset state machine
*
*   The total time for initializing 256 Tx descriptors for 1 priority is
*   approx 100us for a total time of 400us. When combined with RX desc
*   this may cause too long a delay (450us) on some systems for the reset
*   RST_STATE_INIT_DESCRIPTORS state. In which case this function
*   would need to implement substate re-entrancy. The easiest place to
*   insert this is to make each Tx priority queue a substate.
*********************************************************************/
void clnk_hw_desc_init(dc_context_t *dccp)
{
    int i, pri;
    SYS_UINT8 *pMem, *eq;
    struct pkt_entry *pEntry;

    COMMON_LIST_INIT(&dccp->rxList[0]);
    COMMON_LIST_INIT(&dccp->rxFreeList[0]);

    eq = (SYS_UINT8 *)dccp->rx_entry_queue;
    pMem = dccp->pRxMemBlock;
    HostOS_Memset(pMem, 0, sizeof(*pEntry) * dccp->rxNumHostDesc);
    for (i = 0; i < dccp->rxNumHostDesc; i++)
    {
        pEntry = (struct pkt_entry *)pMem;
        //HostOS_Memset(pEntry, 0, sizeof(*pEntry));

        pEntry->status_idx = i;         /* entry queue index */
        pEntry->plist = (void *)eq;     /* ptr to pkt buffer in entry queue */

        COMMON_LIST_ADD_TAIL(&dccp->rxFreeList[0], pEntry);
        pMem += sizeof(*pEntry);

        eq += sizeof(struct ptr_list);
    }

    pMem = dccp->pTxMemBlock;
    HostOS_Memset(pMem, 0, sizeof(*pEntry) * dccp->txNumHostDesc * MAX_TX_PRIORITY);

    for (pri = 0; pri < MAX_TX_PRIORITY; pri++)
    {
        COMMON_LIST_INIT(&dccp->txQueuedPkts[pri]);
        COMMON_LIST_INIT(&dccp->txPriFreeList[pri]);

        dccp->tx_pkt_descs[pri] = (struct pkt_entry *)pMem;      

        eq = (SYS_UINT8 *)dccp->tx_entry_queue[pri];

        for (i = 0; i < dccp->txNumHostDesc; i++)
        {   // simplify ...
            pEntry = (struct pkt_entry *)pMem;
            // HostOS_Memset(pEntry, 0, sizeof(*pEntry));

            pEntry->status_idx = i;
            pEntry->plist = (void *)eq;

            COMMON_LIST_ADD_TAIL(&dccp->txPriFreeList[pri], pEntry);
            pMem += sizeof(*pEntry);

            eq += sizeof(struct ptr_list);
        }
    }
    return;
}

/********************************************************************
* Function Name: clnk_tc_dic_init_pci_drv
* Parameters:
*   dccp - pointer to device context structure
* Description: 
*   Initialize context and descriptors used by the DIC TC
* Returns:
*   None
* Notes: This function is called from clink daemon through ioctl
*   
*********************************************************************/
void clnk_tc_dic_init_pci_drv(dc_context_t *dccp, struct mb_return *mb)
{
    SYS_UINT32 val;

    // Set DIC parameters passed from SoC
    clnk_reg_write(dccp, DIC_D_DESC_TX_PTR, mb->tx_did);
    clnk_reg_write(dccp, DIC_D_DESC_RX_PTR, mb->rx_did);
    clnk_reg_write(dccp, DIC_D_LINEBUF0_SOC_TX, mb->linebuf0_soc_tx);
    clnk_reg_write(dccp, DIC_D_LINEBUF_SOC_RX, mb->linebuf_soc_rx);

    // Set maximum Ethernet frame size & maximum number of Ethernet frames that may be
    // aggregated into a single c.LINK packet
    clnk_reg_write(dccp, DIC_D_ETH_SPU_LEN_W_FCS_MAX, 1522);
    clnk_reg_write(dccp, DIC_D_AGGR_HDR_SPU_CNT_MAX, 6);
 
    /* read packet sizes from the SoC */
    clnk_reg_read(dccp, mb->linebuf0_soc_tx, &val);
    dccp->tx_pkt_size = val & 0xffff;
    clnk_reg_read(dccp, mb->linebuf_soc_rx, &val);
    dccp->rx_pkt_size = val & 0xffff;
    /* protocol descriptor ring */
    dccp->pd_queue[0] = mb->pd_queue_0;
    dccp->pd_queue[1] = mb->pd_queue_1;
    dccp->pd_queue[2] = mb->pd_queue_2;
    dccp->pd_queue[3] = mb->pd_queue_3;
    dccp->pd_entries = mb->pd_entries;

    for(val = 0; val < MAX_TX_PRIORITY; val++)
        dccp->pd_entry_widx[val] = 0;

    clnk_setup_host_bd(dccp, mb->extra_pkt_mem);
    clnk_reg_write(dccp, DIC_D_LINEBUF_HOST_RX, dccp->rx_linebuf);
    clnk_reg_write(dccp, DEV_SHARED(linebuf_host_rx), dccp->rx_linebuf);
    for(val = 0; val < MAX_TX_PRIORITY; val++)
        clnk_reg_write(dccp, DEV_SHARED(linebuf_host_tx[val]), dccp->tx_linebuf[val]);
    clnk_reg_write(dccp, DIC_D_TX_DMA_CTL, DIC_TX_DMA_CTL_WORD);
    clnk_reg_write(dccp, DIC_D_RX_DMA_CTL, DIC_RX_DMA_CTL_WORD);

    // Configure DIC address translators
    clnk_reg_write(dccp, DIC_ATRANS0_CTL0, 0x00060701);
    clnk_reg_write(dccp, DIC_ATRANS1_CTL0, 0x00020101);

    // Release DIC TC from reset
    clnk_reg_write(dccp, DIC_TC_CTL_0, 1);
}

/********************************************************************
* Function Name: clnk_setup_host_bd
* Parameters:
*   dccp - pointer to device context structure
*   base - offset of start of host buffer descriptors
* Description: 
*   Initialize host buffer descriptors
* Returns:
*   None
* Notes:
*   Only used for PCI
*********************************************************************/
void clnk_setup_host_bd(dc_context_t *dccp, SYS_UINT32 base)
{
    struct linebuf bd;
    int i, pri;

    /* init RX buffer descriptor */

//  HostOS_PrintLog(L_INFO, "Host Rx LB desc start = %08x\n", base);
    HostOS_Memset(&bd, 0, sizeof(bd));
    WR_LINEBUF_PDU_SIZE(&bd, dccp->rx_pkt_size);
    WR_LINEBUF_ENTRY_SIZE(&bd, sizeof(struct ptr_list));
    WR_LINEBUF_PTR_MODE(&bd, 1);
    bd.n_entries = dccp->rxNumHostDesc;
    bd.buffer_len = dccp->rxNumHostDesc * sizeof(struct ptr_list);
    bd.buffer_base = dccp->rx_entry_queue_pa;
//  HostOS_PrintLog(L_INFO, "Host Rx LB entry q start = %08x\n", bd.buffer_base);

    dccp->rx_linebuf = base;

    clnk_blk_write(dccp, base, (void *)&bd, sizeof(bd));
    base += sizeof(bd);

//    HostOS_PrintLog(L_INFO, "Host Rx LB entries = %d\n", 
//                     dccp->rxNumHostDesc);

    /* clear entry status words */
    for (i = 0; i < dccp->rxNumHostDesc; i++, base += 4)
    {
        clnk_reg_write(dccp, base, 0);
    }
//  HostOS_PrintLog(L_INFO, "Host Rx LB end = %08x\n", base);

    /* init TX buffer descriptor(s) */

    for(pri = 0; pri < MAX_TX_PRIORITY; pri++)
    {
//      HostOS_PrintLog(L_INFO, "Host Pr%d Tx LB desc start = %08x\n", pri, base);
        HostOS_Memset(&bd, 0, sizeof(bd));
        WR_LINEBUF_PDU_SIZE(&bd, dccp->tx_pkt_size);
        WR_LINEBUF_ENTRY_SIZE(&bd, sizeof(struct ptr_list));
        WR_LINEBUF_PTR_MODE(&bd, 1);
        bd.n_entries = dccp->txNumHostDesc;
        bd.buffer_len = dccp->txNumHostDesc * sizeof(struct ptr_list);
        bd.buffer_base = dccp->tx_entry_queue_pa[pri];
        /* Use write pointer field to store base address of release queue */
        bd.entry_wptr = base + sizeof(bd) + bd.n_entries * 4;
//      HostOS_PrintLog(L_INFO, "Host Pr%d Tx LB entry q start = %08x\n", pri, bd.buffer_base);
//      HostOS_PrintLog(L_INFO, "Host Pr%d Tx LB release queue start = %08x\n", pri, bd.entry_wptr);
        
        dccp->tx_linebuf[pri] = base;

        clnk_blk_write(dccp, base, (void *)&bd, sizeof(bd));
        base += sizeof(bd);

//      HostOS_PrintLog(L_INFO, "Host Pr%d Tx LB entries = %d\n", pri, dccp->txNumHostDesc);
        /* clear entry status words */
        for (i = 0; i < dccp->txNumHostDesc; i++, base += 4)
        {
            clnk_reg_write(dccp, base, 0xFFFF);
        }

        /* Store release queue base address & size, initialize read pointer */ 
        dccp->tx_release_queue[pri] = base;
        dccp->tx_release_q_size[pri] = dccp->txNumHostDesc + 1;
        dccp->tx_release_rindex[pri] = 0;
        
        for (i = 0; i < dccp->tx_release_q_size[pri]; i++, base += 4)
        {
            clnk_reg_write(dccp, base, 0);
        }
//      HostOS_PrintLog(L_INFO, "Host Pr%d Tx LB end = %08x\n", pri, base);
    }

//  HostOS_PrintLog(L_INFO, "Host BD end = %08x\n", base);
}

void clnk_hw_bw_teardown(dc_context_t *dccp, SYS_INT32 nodeId)
{
    return;
}

void clnk_hw_bw_setup(dc_context_t *dccp, SYS_INT32 nodeId,
                SYS_UINT32 addrHi, SYS_UINT32 addrLo)
{
    return;
}

void clnk_hw_set_timer_spd(dc_context_t *dccp, SYS_UINT32 spd)
{
    return;
}
