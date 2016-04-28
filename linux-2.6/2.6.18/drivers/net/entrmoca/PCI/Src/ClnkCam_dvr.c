/*******************************************************************************
*
* PCI/Src/ClnkCam_dvr.c
*
* Description: Implements the new host-side CAM management
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
*                             # i n c l u d e s                                *
********************************************************************************/
#include "drv_hdr.h"

#define BRDHI       0xFFFFFFFF
#define BRDLO       0xFFFF0000
#define NO_INDEX    0x88   /* Not a real CAM entry >= 0x80 */

#define UNCLAIMED_MAX 4

#define IS_MULTI(hi)        ((hi) & 0x01000000)

#define CAM_TIME(T)         (1 + ((T) / (1*1000*1000 / CAM_TMR_JIFFIES)))
#define CAM_TIME_MBX        CAM_TIME(    10*1000)
#define CAM_TIME_FLOOD      CAM_TIME(   100*1000)
#define CAM_TIME_CAROUSEL   CAM_TIME(2*1000*1000)



static CamEntry_t   *cam_add(ListHeader_t *pList, CamEntry_t *pEntry);
static void          cam_delay(CamContext_t *pCam);
static CamEntry_t   *cam_lookup(ListHeader_t *pList, SYS_UINT32 hi, SYS_UINT32 lo);
static CamEntry_t   *cam_lookupHead(ListHeader_t *pList, SYS_UINT32 hi, SYS_UINT32 lo);
static CamEntry_t   *cam_head(ListHeader_t *pList, CamEntry_t *pEntry);
static CamEntry_t   *cam_print(char*, CamEntry_t* pEntry);
static CamContext_t *cam_program0(char*, CamContext_t *pCam, CamEntry_t *pEntry);
static CamContext_t *cam_program(char*, CamContext_t *pCam, CamEntry_t *pEntry);
static void          cam_publish(CamContext_t *pCam, CamEntry_t *pEntry, int carousel);
static CamEntry_t   *cam_switch(CamEntry_t *pEntry1, CamEntry_t *pEntry2);
static CamEntry_t   *cam_tail(ListHeader_t *pList, CamEntry_t *pEntry);
static CamEntry_t   *cam_update0(CamEntry_t *pEntry, CamType_t camType,
                                 SYS_UINT32 hi, SYS_UINT32 lo,
                                 SYS_UINT32 ndx, SYS_UINT32 node);
static CamEntry_t   *cam_update(CamEntry_t *pEntry, CamType_t camType,
                                SYS_UINT32 hi, SYS_UINT32 lo);
static CamEntry_t   *cam_zap(CamEntry_t *pEntry, SYS_UINT32 ndx);



void Cam_Init(CamContext_t *pCam, void *pMbox, void *dcctx, SYS_BOOLEAN softCam)
{
    ListHeader_t * pL;
    CamEntry_t * pE;
    int j;

    // Setup any extra administrative variables
    pCam->dc_ctx  = dcctx;
    pCam->pMbx    = pMbox;
    pCam->softCam = softCam;

    // Setup any extra time stamp variables
    pCam->camSrcRR = 0;
    pCam->timeMbx = 0; // CAM_TIME_MBX;
    pCam->timeFlood = CAM_TIME_FLOOD;
    pCam->timeCarousel = CAM_TIME_CAROUSEL;

    // Clear out state for publishing source addresses
    HostOS_Memset(&pCam->ethCmd, 0, sizeof(pCam->ethCmd));
    pCam->ethCnt = 1; // empty - legal values are 1, 3, and 5

    // ... Setup Lists
    pL = &pCam->UcastL; // UNICAST: 72 elements of UcastL, plus CAM
    COMMON_LIST_INIT(pL);
    for (j = 0; pE = &(pCam->UcastA[j]), j < NUM_UCAST; j++)
        cam_delay(cam_program0(SYS_NULL, pCam, cam_add(pL, cam_zap(pE, j))));

    pL = &pCam->McastL; // MULTICAST: 24 elements of McastL, plus CAM
    COMMON_LIST_INIT(pL);
    for (j = 0; pE = &(pCam->McastA[j]), j < NUM_MCAST; j++)
        cam_delay(cam_program0(SYS_NULL, pCam, cam_add(pL, cam_zap(pE, j+NUM_UCAST))));

#if FEATURE_QOS
    pL = &pCam->FcastL; // FLOWCAST: 32 elements of FcastL, plus CAM
    COMMON_LIST_INIT(pL);
    for (j = 0; pE = &(pCam->FcastA[j]), j < NUM_FCAST; j++)
        cam_delay(cam_program0(SYS_NULL, pCam, cam_add(pL, cam_zap(pE, j+NUM_UCAST+NUM_MCAST))));
#if ECFG_CHIP_ZIP1
    ((dc_context_t *)(pCam->dc_ctx))->qos_flow_count = 0;
#endif /* ECFG_CHIP_ZIP1 */
    pCam->ingress_routing_conflicts = 0;
#endif /* FEATURE_QOS */

    pL = &pCam->XcastL; // eXtra UNICAST: 128 elements of XcastL
    COMMON_LIST_INIT(pL);
    for (j = 0; pE = &(pCam->XcastA[j]), j < NUM_XCAST; j++)
        cam_add(pL, cam_zap(pE, NO_INDEX));

    pL = &pCam->ScastL; // SOURCE: 64 elements of ScastL
    COMMON_LIST_INIT(pL);
    for (j = 0; pE = &(pCam->ScastA[j]), j < 64; j++)
        cam_add(pL, cam_zap(pE, NO_INDEX));
}

/* hardware-specific operations */

#if ECFG_CHIP_ZIP1

void
Cam_Terminate(CamContext_t *pCam)
{
    clnk_reg_write(pCam->dc_ctx, CLNK_REG_CAM_ADDR_INDEX, 0); // disable all cams
    pCam->dc_ctx = SYS_NULL;
}

static CamContext_t *
cam_program0(char *msg, CamContext_t *pCam, CamEntry_t *pEntry)
{
    // Program entry contents in correlated CAM slot/ndx
    cam_print(msg, pEntry);

    // Program regular HW CAM
    clnk_reg_write(pCam->dc_ctx, CLNK_REG_CAM_LOAD_DATA_HIGH, pEntry->camHi);
    clnk_reg_write(pCam->dc_ctx, CLNK_REG_CAM_LOAD_DATA_LOW,
                       (pEntry->camLo & 0xffff0000) |
                       ((pEntry->camNode & 0x3f) << 8) | // bits 8-13
                   CLNK_REG_CAM_LO_VALID_BIT |  // bit 14
                   CLNK_REG_CAM_LO_XXXXX_BIT);  // bit 0

    clnk_reg_write(pCam->dc_ctx, CLNK_REG_CAM_ADDR_INDEX, pEntry->camNdx |
                                                  CLNK_REG_CAM_MASTER_DMA_ENABLE |
                                                  CLNK_REG_CAM_FILL_FLAG);
    return pCam; // see also cam_delay()
}

static void
cam_delay(CamContext_t *pCam)
{
    // In case you need to wait for back to back cam operations
    SYS_UINT32 dwaddle;
    clnk_reg_read(pCam->dc_ctx, CLNK_REG_CAM_ADDR_INDEX, &dwaddle);
    (void)dwaddle;  // Quash a compiler warning.
}

#else /* ECFG_CHIP_ZIP2/ECFG_CHIP_MAVERICKS */

/* HW CAM access is stubbed out on Zip2 */

void
Cam_Terminate(CamContext_t *pCam)
{
    return;
}

static CamContext_t *
cam_program0(char *msg, CamContext_t *pCam, CamEntry_t *pEntry)
{
    cam_print(msg, pEntry);
    return(pCam);
}

static void
cam_delay(CamContext_t *pCam)
{
    return;
}

#endif /* ECFG_CHIP_ZIP1 */

void
Cam_AddNode(CamContext_t *pCam, SYS_UINT32 hi, SYS_UINT32 lo, SYS_UINT32 nodeId)
{
    if (pCam->softCam)
    {
        // Setup actual hardCam for softCam
        CamEntry_t entry;
        HostOS_PrintLog(L_INFO, "CAM - %s:%02x:%02d %08x%04x\n", "AddNode",
                        nodeId, nodeId,  hi, lo >> 16);
        cam_update0(&entry, CAM_DEST, hi, lo, nodeId, nodeId);
        cam_program0("PrgmADACC", pCam, &entry);
    }
    Cam_RcvCarousel(pCam, hi, lo, nodeId);
}

void
Cam_DelNode(CamContext_t *pCam, int nodeId)
{
    // Purge node from tables when node drops out of network
    int j; CamEntry_t * pE;

    if (pCam->softCam)
    {
        // actually clear hardCam for softCam
        CamEntry_t entry;
        cam_program0("PrgmDLACC", pCam, cam_zap(&entry, nodeId));
    }
    // Clear entry out of Active Unicast CAM
    for (j = 0; pE = &(pCam->UcastA[j]), j < NUM_UCAST; j++)
        if (pE->camNode == nodeId)
            cam_delay(cam_program("PrgmDLNOD", pCam, cam_tail(&pCam->UcastL, cam_zap(pE, j))));

    // Clear entry out of eXtra Unicast List
    for (j = 0; pE = &(pCam->XcastA[j]), j < NUM_XCAST; j++)
        if (pE->camNode == nodeId)
            cam_tail(&pCam->XcastL, cam_zap(pE, NO_INDEX));
}

void
Cam_LookupSrc(CamContext_t *pCam, SYS_UINT32 hi, SYS_UINT32 lo)
{   // Similar to Clnk_ETH_Bridge_PublishSrcAddr()
    CamEntry_t *pE;
    if (!IS_MULTI(hi))
    {
        if (!cam_lookupHead(&pCam->ScastL, hi, lo))
        {
            // Make a new SRC entry
            pE = cam_head(&pCam->ScastL, SYS_NULL);
            cam_publish(pCam, cam_print("NewSource", cam_update(pE, CAM_SRC, hi, lo)), 0);
            // Make sure its not in DEST or BDEST tables
            if ((pE = cam_lookup(&pCam->UcastL, hi, lo))) // while
                cam_delay(cam_program("PrgmZAPD1", pCam, cam_tail(&pCam->UcastL, cam_zap(pE, pE->camNdx))));
            if ((pE = cam_lookup(&pCam->XcastL, hi, lo))) // else, while
                cam_print("SrcZAPDX2", cam_tail(&pCam->XcastL, cam_zap(pE, NO_INDEX)));
#if FEATURE_QOS
            // Make sure its not in Fcast List
            if ((pE = cam_lookup(&pCam->FcastL, hi, lo)))
            {
                HostOS_PrintLog(L_ALERT, " CAM - Ucast Flow %08x%04x (%d) is entering zombie state\n", hi, lo>>16, pE->camNode);
                cam_delay(cam_program("PrgmZAPF", pCam, cam_tail(&pCam->FcastL, cam_zap(pE, pE->camNdx))));
                pCam->ingress_routing_conflicts++;
                // Update ingress routing conflict counter in the SoC
                clnk_reg_write(pCam->dc_ctx, 
                               DEV_SHARED(ingr_routing_conflict_ctr),
                               pCam->ingress_routing_conflicts);
            }
#endif /* FEATURE_QOS */
        }
    }
}

int
Cam_LookupDst(CamContext_t *pCam, SYS_UINT32 hi, SYS_UINT32 lo)
{
    // Returns -1 if should drop the packet. Otherwise the nodeID.
    if (IS_MULTI(hi))
    {
        // MultiCast (including broadcast!)

        // NOTE: Choose between efficiency or having record of upto 48 multicast addresses
        // if (!pCam->softcam && !cam_lookupHead(&pCam->McastL, hi, lo))

#if FEATURE_QOS
        CamEntry_t *pEntry;
        if ((pEntry = cam_lookupHead(&pCam->FcastL, hi, lo)))
        {
            // Bump Flow if necessary
            //cam_flowBump(pCam, pEntry);
            return pEntry->camNode + 0x100;
        }
#endif /* FEATURE_QOS */

        if (!cam_lookupHead(&pCam->McastL, hi, lo))
        {
            // Need new Multicast Address
            cam_program("PrgmMCAST", pCam,
                cam_update(cam_head(&pCam->McastL, SYS_NULL), CAM_MCAST, hi, lo));
        }
        return(CLNKMAC_BCAST_ADDR);
    }
    else
    {
        // UniCast
        CamEntry_t *pEntry, *pXEntry;
        ListHeader_t *pUcastL = &pCam->UcastL;

#if FEATURE_QOS
        // Check if a PQoS flow is associated with this unicast DA
        if ((pEntry = cam_lookupHead(&pCam->FcastL, hi, lo)))
        {
            // Bump Flow if necessary
            //cam_flowBump(pCam, pEntry);
            return pEntry->camNode + 0x100;
        }
#endif /* FEATURE_QOS */

        if (!(pEntry = cam_lookupHead(pUcastL, hi, lo)))
        {
            // Not Found, look in Inactive Queue
            ListHeader_t *pXcastL = &(pCam->XcastL);
            if (!(pXEntry = cam_lookup(pXcastL, hi, lo)))
            {
                // Create New Entry at front of Backup List
                // NOTE: when it becomes DEST, it will be purged from ScastL
                pXEntry = cam_head(pXcastL, SYS_NULL);
                cam_update(pXEntry, CAM_BDEST, hi, lo);
                cam_print("NewBDEST", pXEntry);
            }
            // Switch between lists; then reprogram hw.
            pEntry = cam_head(pUcastL, SYS_NULL);
            cam_program("PrgmDESTL", pCam, cam_switch(pEntry, pXEntry));
            cam_tail(pXcastL, pXEntry);
            pEntry->camPkts = UNCLAIMED_MAX;
        }
        if (pEntry->camType == CAM_BDEST)
        {
            // Be careful with Stale/Unclaimed entries
            if (pEntry->camPkts == 0)
                return -1;  // Drop
            pEntry->camPkts--;
        }
        return pEntry->camNode;
    }
}

void
Cam_RcvCarousel(CamContext_t *pCam, SYS_UINT32 hi, SYS_UINT32 lo, SYS_UINT32 nodeId)
{
    CamEntry_t *pEntry;

#if FEATURE_QOS
    // Check Fcast List
    if ((pEntry = cam_lookup(&pCam->FcastL, hi, lo)))
    {
        // Already Active CAM, It must be a DEST with the same nodeId.
        if (pEntry->camType == CAM_BDEST || pEntry->camNode != nodeId)
        {
            // Flow dest has changed - need to provide a PQoS error event here
            HostOS_PrintLog(L_ALERT, " CAM - RCV publish  %08x%04x (%d), "
                            "but found dest mismatch w/ Flow CAM entry (%d)\n", 
                            hi, lo>>16, nodeId, pEntry->camNode);
            cam_delay(cam_program("PrgmZAPF", pCam, cam_tail(&pCam->FcastL, cam_zap(pEntry, pEntry->camNdx))));
            pCam->ingress_routing_conflicts++;
            clnk_reg_write(pCam->dc_ctx, DEV_SHARED(ingr_routing_conflict_ctr),
                           pCam->ingress_routing_conflicts);
            //pEntry->camType = CAM_DEST;
            //pEntry->camNode = nodeId;
            //cam_program("PrgmRCSL1", pCam, pEntry);
        } else {
            pEntry = SYS_NULL;
        }
    } 
    else
#endif /* FEATURE_QOS */
    // Check Active Ucast List
    if ((pEntry = cam_lookup(&pCam->UcastL, hi, lo)))
    {
        // Already Active CAM, It must be a DEST or BDEST.  MRU on xmits only.
        if (pEntry->camType == CAM_BDEST || pEntry->camNode != nodeId)
        {
            // Promote from BDEST to DEST
            pEntry->camType = CAM_DEST;
            pEntry->camNode = nodeId;
            cam_program("PrgmRCSL1", pCam, pEntry);
        } else {
            pEntry = SYS_NULL;
        }
    }
    else if ((pEntry = cam_lookupHead(&pCam->XcastL, hi, lo)))
    {
        // Already Backup, Might have changed.  MRU on carousel
        if (pEntry->camType == CAM_BDEST || pEntry->camNode != nodeId)
        {
            // Promote from BDEST to DEST
            pEntry->camType = CAM_DEST;
            pEntry->camNode = nodeId;
            cam_print("RcvCrsel2", pEntry);
        } else {
            pEntry = SYS_NULL;
        }
    }
    else
    {
        // Not found, create in Backup
        pEntry = cam_head(&pCam->XcastL, SYS_NULL);
        cam_update(pEntry, CAM_DEST, hi, lo);
        pEntry->camNode = nodeId;
        cam_print("RcvCrsel3", pEntry);
    }

    if (pEntry)
    {
        // Make sure its not in Src Table
        Cam_RcvUnpublish(pCam, hi, lo, nodeId);
        // For ACCESS NC, consider sending UNPUBLISH now when a node switch is really
        // occurring instead of duplicating each carousel in the SOC ?
    }
}

void
Cam_RcvUnpublish(CamContext_t *pCam, SYS_UINT32 hi, SYS_UINT32 lo, SYS_UINT32 nodeId)
{
    CamEntry_t *pEntry = cam_lookup(&pCam->ScastL, hi, lo);
    if (pEntry)
    {
        // New or Modified, purge from SRC
        // HostOS_PrintLog(L_INFO, " NEW CAM - RCV Unpublish  %08x%04x (%d)\n", hi, lo>>16, nodeId);
        cam_tail(&pCam->ScastL, cam_zap(pEntry, NO_INDEX));
    }
}

void
Cam_Bump(CamContext_t *pCam)
{
    if (pCam->dc_ctx == SYS_NULL)
    {
        // Make sure we are initialized
        return;
    }

    if (pCam->timeMbx && --pCam->timeMbx == 0)
    {   // Suppress or allow Source updates every eg 10 milliseconds
        cam_publish(pCam, SYS_NULL, 0);
        return;
    }

    if ( pCam->timeMbx == 0 && --pCam->timeCarousel == 0)
    {
        // Every 2 Seconds, send out a few more CAM_SRC addresses
        // NOTE: consider expedited carousel just after AddNode.
        cam_publish(pCam, SYS_NULL, 1);
        pCam->timeCarousel = CAM_TIME_CAROUSEL;
        return;
    }

    // Every 100 milliseconds, increment Flood packets
    if (!pCam->softCam && --pCam->timeFlood == 0)
    {
        // For any BDEST in UcastL, increment permissable Flood packet count
        CamEntry_t *pE = COMMON_LIST_HEAD(CamEntry_t, &pCam->UcastL);
        for ( ; pE && pE->camType != CAM_NONE; pE = COMMON_LIST_NEXT(CamEntry_t, pE))
            if (pE->camType == CAM_BDEST && pE->camPkts < UNCLAIMED_MAX)
                pE->camPkts++;
        pCam->timeFlood = CAM_TIME_FLOOD;
    }
}

int
Cam_Get(CamContext_t *pCam, ClnkDef_BridgeEntry_t *pBridge, int type, int count)
{
    // For 'clnkstat -b'
    CamEntry_t * pEntry;
    SYS_UINT8 cam_type;
    int numRet = 0, numTry = 0, numArray;
    switch (type)
    {
        case 0: pEntry = &(pCam->ScastA[0]);
                numArray = NUM_SCAST;
                cam_type = CAM_SRC;
                break;
        default: // 1 or 2
                pEntry = &(pCam->UcastA[0]);
                numArray = NUM_UCAST + NUM_XCAST;
                cam_type = type == 1 ? CAM_DEST : CAM_BDEST;
                break;
        case 3: pEntry = &(pCam->McastA[0]);
                numArray = NUM_MCAST;
                cam_type = CAM_MCAST;
                break;

#if FEATURE_QOS
        case 4: pEntry = &(pCam->FcastA[0]);
                numArray = NUM_FCAST;
                cam_type = CAM_FLOW;
                break;
#endif /* FEATURE_QOS */

    }
    for ( ; numRet < count && numTry < numArray; numTry++, pEntry++)
    {
        if (cam_type == pEntry->camType)
        {
            pBridge->macAddrHigh = pEntry->camHi;
            pBridge->macAddrLow  = pEntry->camLo;
            if ((cam_type == CAM_DEST) || (cam_type == CAM_FLOW))
            {
                // Additional Info
                pBridge->macAddrLow += pEntry->camNode;
                if (numTry >= NUM_UCAST)
                    pBridge->macAddrLow += 100;
            }
            pBridge++;
            numRet++;
        }
    }
    return numRet;
}

#if FEATURE_QOS
void qos_cam_program(CamContext_t *cam_p, SYS_UINT32 slot, SYS_UINT32 hi,
                     SYS_UINT32 lo, SYS_UINT32 node_id)
{
    CamEntry_t *ent_p;
    dc_context_t *dccp = cam_p->dc_ctx;

    // Increment ingress flow count to the destination node.
    if (node_id < MAX_NUM_NODES)
    {
        dccp->qos_flows_per_dst[node_id]++;
        HostOS_PrintLog(L_DEBUG, "CFLOW: flow count for DN%lu = %lu\n",
                        node_id, dccp->qos_flows_per_dst[node_id]);
    }

#if ECFG_CHIP_ZIP1
    // If this is the first ingress flow for this node, tnen initiate tx fifo
    // resizing for PQoS mode
    if (!dccp->qos_flow_count)
    {
        // Call host OS callback for throttling the c.LINK net i/f
        dccp->tFifoResizeCallback(dccp->p_dk_ctx);
        dccp->txFifoResizeStatus = TX_FIFO_RESIZE_STS_THROTTLED;
        HostOS_PrintLog(L_DEBUG, "Tx fifo resize for PQoS\n");
    }
    dccp->qos_flow_count++;
#endif /* ECFG_CHIP_ZIP1 */

    if (IS_MULTI(hi))
    {
        // Need to get rid of any normal MCAST entry with the same flow ID.
        ent_p = cam_lookup(&cam_p->McastL, hi, lo);
        if (ent_p  != SYS_NULL)
            cam_delay(cam_program("PrgmMCAST", cam_p, cam_tail(&cam_p->McastL,
                      cam_zap(ent_p, ent_p->camNdx))));
    }
    else
    {
        // Need to get rid of any normal UCAST entry with the same ucast DA.
        ent_p = cam_lookup(&cam_p->UcastL, hi, lo);
        if (ent_p  != SYS_NULL)
            cam_delay(cam_program("PrgmUCAST", cam_p, cam_tail(&cam_p->UcastL,
                      cam_zap(ent_p, ent_p->camNdx))));

        // Need to get rid of any normal XCAST entry with the same ucast DA.
        ent_p = cam_lookup(&cam_p->XcastL, hi, lo);
        if (ent_p  != SYS_NULL)
            cam_tail(&cam_p->XcastL, cam_zap(ent_p, ent_p->camNdx));

        // Need to get rid of any normal SCAST entry with the same ucast DA.
        ent_p = cam_lookup(&cam_p->ScastL, hi, lo);
        if (ent_p != SYS_NULL)
            Cam_RcvUnpublish(cam_p, hi, lo, node_id);
    }

    // Now program the CAM entry for the QoS flow.
    ent_p = &cam_p->FcastA[slot]; 
    COMMON_LIST_MOVE_TO_HEAD(&cam_p->FcastL, ent_p);
    cam_update(ent_p, CAM_FLOW, hi, lo);
    ent_p->camNode = node_id;
    cam_delay(cam_program("PrgmCFLOW", cam_p, ent_p));
}

void qos_cam_delete(CamContext_t *cam_p, SYS_UINT32 mask)
{
    unsigned int slot;
    CamEntry_t *ent_p;

    for (slot = 0; slot < QOS_MAX_FLOWS; slot++) {
        if (mask & (1 << slot))
        {
            dc_context_t *dccp = cam_p->dc_ctx;

            ent_p = &cam_p->FcastA[slot]; 

            // Decrement ingress flow count to the destination node.
            if ((ent_p->camNode < MAX_NUM_NODES) && 
                dccp->qos_flows_per_dst[ent_p->camNode])
            {
                dccp->qos_flows_per_dst[ent_p->camNode]--;
                HostOS_PrintLog(L_DEBUG, "DFLOW: flow count for DN%lu = %lu\n",
                                ent_p->camNode,
                                dccp->qos_flows_per_dst[ent_p->camNode]);
            }

#if ECFG_CHIP_ZIP1
            if (dccp->qos_flow_count)
            {
                dccp->qos_flow_count--;
                // If this is the last ingress flow for this node, then initiate
                // tx fifo resizing for non-PQoS mode
                if (!dccp->qos_flow_count)
                {
                    // Call host OS callback for throttling the c.LINK net i/f
                    dccp->tFifoResizeCallback(dccp->p_dk_ctx);
                    dccp->txFifoResizeStatus = TX_FIFO_RESIZE_STS_THROTTLED;
                    HostOS_PrintLog(L_DEBUG, "Tx fifo resize for non-PQoS\n");
                }
            }
#endif /* ECFG_CHIP_ZIP1 */

            cam_delay(cam_program("PrgmDFLOW", cam_p, cam_tail(&cam_p->FcastL, 
                      cam_zap(ent_p, slot + CAM_FBASE))));
        }
    }
}
#endif /* FEATURE_QOS */

/* PRIVATE INTERFACES */

static CamEntry_t *
cam_add(ListHeader_t *pList, CamEntry_t *pEntry)
{
    // For initialization only
    COMMON_LIST_ADD_TAIL(pList, pEntry);
    return pEntry;
}

static CamEntry_t *
cam_head(ListHeader_t *pList, CamEntry_t *pEntry)
{
    // Move entry to head of list.  If SYS_NULL, use LRU at end of queue
    if (!pEntry && !(pEntry = COMMON_LIST_TAIL(CamEntry_t, pList)))
        return SYS_NULL; // OUCH

    COMMON_LIST_MOVE_TO_HEAD(pList, pEntry);
    return pEntry;
}

static CamEntry_t *
cam_lookup(ListHeader_t * pList, SYS_UINT32 hi, SYS_UINT32 lo)
{
    // Lookup: By macaddr. Stop at first empty (CAM_NONE) element
    CamEntry_t *pE;
    for (pE = COMMON_LIST_HEAD(CamEntry_t, pList); pE; pE = COMMON_LIST_NEXT(CamEntry_t, pE))
    {
        // Keep searches short, but be careful when creating SYS_NULL entries
        if (pE->camType == CAM_NONE)
            return SYS_NULL;

        if (pE->camLo == lo && pE->camHi == hi)
            break;
    }
    return pE; // possibly SYS_NULL
}

static CamEntry_t *
cam_lookupHead(ListHeader_t * pList, SYS_UINT32 hi, SYS_UINT32 lo)
{
    // Lookup: If successful, move to head of list
    CamEntry_t *pEntry = cam_lookup(pList, hi, lo);
    if (pEntry)
        (void) cam_head(pList, pEntry);
    return pEntry;
}

static CamEntry_t *
cam_print(char *msg, CamEntry_t* pEntry)
{
    // Debugging, change '0 && msg' to 'msg' to enable print
    if (0 && msg)
        HostOS_PrintLog(L_INFO, "CAM - %s:%02x:%02d %08x%04x\n", msg, pEntry->camNdx,
            pEntry->camNode,  pEntry->camHi, pEntry->camLo >> 16);
    return pEntry;
}

static CamContext_t *
cam_program(char *msg, CamContext_t *pCam, CamEntry_t *pEntry)
{
    // Wrapper to program Cam - ignored when softCam is true
    return pCam->softCam ? pCam : cam_program0(msg, pCam, pEntry);
}

static void
cam_publish(CamContext_t * pCam, CamEntry_t *pEntry, int carousel)
{
    // Notify other Nodes of upto 3 CAM_SOURCE entries via mailbox

    // ethCnt can be 1 (empty),  3 (1 entry), 5 (2 entry), or 7 (full)
    if (pEntry && pCam->ethCnt < 7)
    {
        // Send Specific entry (and avoid in next loop)
        pCam->ethCmd.param[pCam->ethCnt++] = pEntry->camHi;
        pCam->ethCmd.param[pCam->ethCnt++] = pEntry->camLo;
    }

    if (pCam->timeMbx == 0)
    {
        // OK to send a new Msg at this time
        if (carousel)
        {
            // Try to fill up the packet
            CamEntry_t *pE;
            int rr1 = (unsigned) pCam->camSrcRR < NUM_SCAST ? pCam->camSrcRR : NUM_SCAST;
            int rr = rr1;
            while ( pCam->ethCnt < 7 )
            {
                // Find Up to 3 Entries from Src Array
                pE = &(pCam->ScastA[rr]);
                if (pE->camType == CAM_SRC && pE != pEntry)
                {
                    pCam->ethCmd.param[pCam->ethCnt++] = pE->camHi;
                    pCam->ethCmd.param[pCam->ethCnt++] = pE->camLo;
                }
                if (++rr == NUM_SCAST) rr = 0;
                if (rr == rr1) break;
            }
            pCam->camSrcRR = rr;
        }
        if (pCam->ethCnt > 1)
        {
            // Send the packet now
            SYS_UINT8 transID;
            pCam->ethCmd.cmd = CLNK_MBX_SET_CMD(CLNK_MBX_ETH_PUB_UCAST_CMD);
            pCam->ethCmd.param[0] = CAM_SRC; // = CLNK_BRIDGE_SRC_TYPE = 1
            Clnk_MBX_SendMsg(pCam->pMbx, (Clnk_MBX_Msg_t*) &pCam->ethCmd, &transID, 8);
            pCam->timeMbx = CAM_TIME_MBX; // suppress future packets for ~10msec.
            HostOS_Memset(&pCam->ethCmd, 0, sizeof(pCam->ethCmd));
            pCam->ethCnt = 1;
        }
    }
}

static CamEntry_t *
cam_switch(CamEntry_t *pE1, CamEntry_t *pE2)
{
    // Switch contents (but not pointers or camNdx) of two cam entries.
#define SWITCHEROO(xx,f) xx = pE1->f; pE1->f = pE2->f; pE2->f = xx
    SYS_UINT8   x8;
    SYS_UINT32 x32;
    SWITCHEROO(x32, camHi);
    SWITCHEROO(x32, camLo);
    SWITCHEROO( x8, camType);
    SWITCHEROO( x8, camNode);
    SWITCHEROO( x8, camPkts);
    return pE1;
}

static CamEntry_t *
cam_tail(ListHeader_t *pList, CamEntry_t *pEntry)
{
    // Conditionally move unused entry to tail
    if (pEntry->camType == CAM_NONE)
    {
        // Need a LIST_MOVE_TO_TAIL(pList, pEntry) function
        COMMON_LIST_REM_NODE(pList, pEntry);
        COMMON_LIST_ADD_TAIL(pList, pEntry);
    }
    return pEntry;
}

static CamEntry_t *
cam_update0(CamEntry_t *pEntry, CamType_t camType,
                SYS_UINT32 hi, SYS_UINT32 lo, SYS_UINT32 ndx, SYS_UINT32 node)
{
    // structure initialize
    pEntry->camType = camType;
    pEntry->camNdx  = ndx;
    pEntry->camNode = node;
    pEntry->camPkts = 0;
    pEntry->camHi   = hi;
    pEntry->camLo   = lo;
    return pEntry;
}

static CamEntry_t *
cam_update(CamEntry_t *pEntry, CamType_t camType, SYS_UINT32 hi, SYS_UINT32 lo)
{
    // Update entry. Leave camNdx alone.  Zap camNode.
    pEntry->camType = camType;
    pEntry->camNode = CLNKMAC_BCAST_ADDR;
    pEntry->camPkts = 0;
    pEntry->camHi   = hi;
    pEntry->camLo   = lo;
    return pEntry;
}

static CamEntry_t *
cam_zap(CamEntry_t *pEntry, SYS_UINT32 ndx)
{
    // initialize entry as broadcast, assign to specific cam slot or NO_INDEX
    // NOTE: One interesting idea is to save (ndx ^ 0xC) or somesuch as a
    // clever way to get some of the multicasts to lookup quicker in HW CAM.
    pEntry->camType = CAM_NONE;
    pEntry->camNode = CLNKMAC_BCAST_ADDR;
    pEntry->camPkts = 0;
    pEntry->camHi   = BRDHI;
    pEntry->camLo   = BRDLO;
    pEntry->camNdx  = ndx;
    return pEntry;
}

#if 0
static void print_cam_list(ListHeader_t *pL)
{
    ListEntry_t* p;

    HostOS_PrintLog(L_INFO, "head %p tail %p num elements %d\n", pL->pHead, pL->pTail, pL->numElements);
    for (p = pL->pHead; p; p = p->pNext)
        HostOS_PrintLog(L_INFO, "element %p prev %p next %p\n", p, p->pPrev, p->pNext);
}
#endif

/* */
