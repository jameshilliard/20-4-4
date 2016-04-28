/*******************************************************************************
*
* PCI/Inc/ClnkCam_dvr.h
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

#ifndef _CLNKCAM_H_
#define _CLNKCAM_H_

#include "common_dvr.h"
#include "ClnkMbx_dvr.h"

typedef enum { CAM_NONE, CAM_SRC, CAM_DEST, CAM_BDEST, CAM_MCAST, CAM_FLOW } CamType_t;

/* Overview:
* All Cam Entries and Lists are statically allocated. 
** No Mallocs.  Lists are initialized from 4 arrays of Cam Entries.
** The UcastA and McastA entries are one to one with HW CAM entries for life.
** CamEntries move to head or tail, but never between lists.
* List searches terminated by first CAM_NONE entry. 
* Notes about specific lists
** ScastL holds only CAM_SOURCE entries.
** McastL holds 48 'MRU' CAM_MCAST entries, including FF:FF:FF:FF:FF:FF
** UcastL holds 80 'MRU' CAM_DEST or CAM_BDEST entries
*** XcastL is an extension of UcastL, but is not in CAM
*** New receive carousel entries go in XcastL, until needed in UcastL
*** Hence XcastL is 'MRU' by carousel
*/

typedef struct
{
    ListEntry_t    ListEntry;
    SYS_UINT8      camType;    // CamType_t as byte
    SYS_UINT8      camNdx;     // HW CAM index
    SYS_UINT8      camNode;
    SYS_UINT8      camPkts;    // MultiCast Flood count
    SYS_UINT32     camLo;      // Mac (Lo precedes Hi as cache speedup) 
    SYS_UINT32     camHi;      // Mac
} CamEntry_t;

#if (NETWORK_TYPE_ACCESS) || !(FEATURE_QOS)
#define NUM_UCAST  80          /* U+M == 128 (hw cam limit) */
#define NUM_MCAST  48          /* U+M == 128 (hw cam limit) */
#else
#define NUM_UCAST  72          /* U+F+M == 128 (hw cam limit) */
#define NUM_MCAST  24          /* U+F+M == 128 (hw cam limit) */
#define NUM_FCAST  32          /* U+F+M == 128 (hw cam limit) */
#endif
#define NUM_XCAST  128         /* size optional */
#define NUM_SCAST  64          /* size optional, but 64 for MoCA */

#if FEATURE_QOS
#define CAM_FBASE    (NUM_UCAST + NUM_MCAST)
#endif /* FEATURE_QOS */

typedef struct
{   // Administrative Stuff
    void           *dc_ctx;
    void           *pMbx;
    SYS_BOOLEAN    softCam;

    // Time constants and throttling info
    SYS_INT32      camSrcRR;
    SYS_INT32      timeMbx;
    SYS_INT32      timeFlood;
    SYS_INT32      timeCarousel;

    // For saving and sending publish messages
    Clnk_MBX_EthCmd_t ethCmd;
    SYS_INT32      ethCnt;

    // Various Cam Queues
    ListHeader_t   UcastL;     // Unicast List: CAM_DEST or CAM_BDEST (Active CAM entries)
    ListHeader_t   McastL;     // Muliticast List: CAM_MCAST (Active CAM entries)
    ListHeader_t   XcastL;     // Xtra Unicast List: (InActive entries)
    ListHeader_t   ScastL;     // Source List: CAM_SOURCE (Inactive)
    CamEntry_t     UcastA[NUM_UCAST]; // Cam_Get assumes that XcastA follows UcastA
    CamEntry_t     XcastA[NUM_XCAST];
    CamEntry_t     McastA[NUM_MCAST];
    CamEntry_t     ScastA[NUM_SCAST];

#if FEATURE_QOS
    CamEntry_t     FcastA[NUM_FCAST];
    ListHeader_t   FcastL;     // QOS Flow Multicast List: CAM_MCAST (Active CAM entries)
    SYS_UINT32     ingress_routing_conflicts;
#endif

} CamContext_t;

/*
* External routines in Camel_Case()
* Internal static routines in lower_case()
*/
extern void Cam_Init(CamContext_t *pCam, void *pMbx, void *dcctx, SYS_BOOLEAN softCam);
extern void Cam_Terminate(CamContext_t *pCam);
extern void Cam_AddNode(CamContext_t *pCam, SYS_UINT32 hi, SYS_UINT32 lo, SYS_UINT32 nodeId);
extern void Cam_DelNode(CamContext_t *pCam, int nodeId);
extern void Cam_LookupSrc(CamContext_t *pCam, SYS_UINT32 hi, SYS_UINT32 lo);
extern int  Cam_LookupDst(CamContext_t *pCam, SYS_UINT32 hi, SYS_UINT32 lo);
extern void Cam_RcvCarousel(CamContext_t *pCam, SYS_UINT32 hi, SYS_UINT32 lo, SYS_UINT32 nodeId);
extern void Cam_RcvUnpublish(CamContext_t *pCam, SYS_UINT32 hi, SYS_UINT32 lo, SYS_UINT32 nodeId);
extern void Cam_Bump(CamContext_t *pCam);
extern int  Cam_Get(CamContext_t *pCam, ClnkDef_BridgeEntry_t *pBridge, int type, int count);

#if FEATURE_QOS
extern void qos_cam_program(CamContext_t *, SYS_UINT32, SYS_UINT32, SYS_UINT32, SYS_UINT32);
extern void qos_cam_delete(CamContext_t *, SYS_UINT32);
#endif

#endif /* ! _CLNKCAM_H_ */
