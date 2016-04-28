/*******************************************************************************
*
* CandD/Inc/control_context_candd.h
*
* Description: CandD Driver context definition
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

#ifndef __control_context_candd_h__
#define __control_context_candd_h__

#if 1 // added for MID RF (DEBUG_EPP)
#include "common_dvr.h"

#define EPP_CAP_FLAGS_RAW_DATA  1
#define EPP_CAP_FLAGS_CORR_DATA 2
#endif
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
    void                *ms_cmd_lock_link;      // moca shell command spin lock - referenced in !GPL side

    SYS_UINT32          clnkThreadID;
    SYS_UINT32          clnkThreadStop;

    SYS_UINT8           clnk_ctl_in[   CLNK_CTL_MAX_IN_LEN];
    SYS_UINT8           clnk_ctl_out[  CLNK_CTL_MAX_OUT_LEN];

    SYS_UINT32          at3_base;   // Address translator 3 base address
    ClnkDef_EvmData_t *         evmData;
#if 1 // added for MID RF (DEBUG_EPP)
    ClnkDef_EppData_t           eppData;
    ClnkDef_EppCapCfg_t         eppCapCfg;
#endif
    SYS_UINT32          freqBand;
} ;

typedef struct _control_context dc_context_t;

#endif /* __control_context_candd_h__ */

