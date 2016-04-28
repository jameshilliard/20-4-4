/*******************************************************************************
*
* Common/Src/ClnkMbx_ttask.c
*
* Description: mailbox timer task support 
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
*                            # I n c l u d e s                                 *
********************************************************************************/

#include "drv_hdr.h"

/*******************************************************************************
*                             # D e f i n e s                                  *
********************************************************************************/





/*******************************************************************************
*            S t a t i c   M e t h o d   P r o t o t y p e s                   *
********************************************************************************/

static void clnketh_tt_task_sched( dc_context_t *dccp, void *task );





/****************************************************************************
*           
*   Purpose:    TT timer expiration function.
*
*   Imports:    data - timer's opaque data, driver control context
*
*   Exports:    
*
*PUBLIC**********************************************************************/
SYS_VOID clnketh_tt_timer(SYS_ULONG data)
{
    dc_context_t *dccp = (dc_context_t *)data;    
//    Clnk_MBX_Mailbox_t *pMbx = &dccp->mailbox; 
#if 0 
    if( Clnk_MBX_Read_Hw_Mailbox_Check_Ready(  &dccp->mailbox ) )
        HostOS_PrintLog(L_INFO, "In clnketh_tt_timer - Pass\n");
    else
        HostOS_PrintLog(L_INFO, "In clnketh_tt_timer - Fail\n");

#endif
#if 0

    // detect command message HW Q ready - rx
  //  HostOS_PrintLog(L_INFO, "Check Ready -rx\n");
    if( Clnk_MBX_Read_Hw_Mailbox_Check_Ready(  &dccp->mailbox ) )
    {
      //  HostOS_PrintLog(L_INFO, "In Check Ready -rx\n");
        clnketh_tt_task_sched( dccp, dccp->tt_cmtask_link ) ;
    }

    // detect command message HW Q ready - tx
   //     HostOS_PrintLog(L_INFO, "Check Ready -tx\n");
    if( Clnk_MBX_Write_Hw_Mailbox_Check_Ready( &dccp->mailbox ) ) 
    {
        if( !Clnk_MBX_Send_Mailbox_Check_Empty( &dccp->mailbox ) )
        {
      //  HostOS_PrintLog(L_INFO, "In Check Ready -tx\n");
            clnketh_tt_task_sched( dccp, dccp->tt_cmtask_link ) ;
        }
    }

    // detect unsolicited message ready - rx
    //    HostOS_PrintLog(L_INFO, "Check Ready -unsol\n");
    if( Clnk_MBX_Unsol_Hw_Mailbox_Check_Ready( &dccp->mailbox ) )
    {
     //   HostOS_PrintLog(L_INFO, "In Check Ready -unsol\n");
        clnketh_tt_task_sched( dccp, dccp->tt_umtask_link ) ;
    }
#endif
#if 0
    // detect command message HW Q ready - rx
//    HostOS_PrintLog(L_INFO, "Check Ready -rx\n");
    if( Clnk_MBX_Read_Hw_Mailbox_Check_Ready(  &dccp->mailbox ) )
    {
    // pass rx message
    Clnk_MBX_Read_ISR( &dccp->mailbox ) ;
    // pass tx message
    Clnk_MBX_Write_ISR( &dccp->mailbox ) ;
    }

    // detect command message HW Q ready - tx
 //       HostOS_PrintLog(L_INFO, "Check Ready -tx\n");
    if( Clnk_MBX_Write_Hw_Mailbox_Check_Ready( &dccp->mailbox ) ) 
    {
        if( !Clnk_MBX_Send_Mailbox_Check_Empty( &dccp->mailbox ) )
        {
  //      HostOS_PrintLog(L_INFO, "In Check Ready -tx\n");
    // pass rx message
    Clnk_MBX_Read_ISR( &dccp->mailbox ) ;
    // pass tx message
    Clnk_MBX_Write_ISR( &dccp->mailbox ) ;

        }
    }
#endif
    Clnk_MBX_Read_ISR( &dccp->mailbox ) ;
    Clnk_MBX_Write_ISR( &dccp->mailbox ) ;
#if 1
//    HostOS_Lock(pMbx->swUnsolLock);
    // detect unsolicited message ready - rx
//        HostOS_PrintLog(L_INFO, "Check Ready -unsol\n");
    if( Clnk_MBX_Unsol_Hw_Mailbox_Check_Ready( &dccp->mailbox ) )
    {
//        HostOS_PrintLog(L_INFO, "In Check Ready -unsol\n");
        Clnk_MBX_Unsol_ISR( &dccp->mailbox ) ;
    }
#endif
 //  HostOS_Unlock(pMbx->swUnsolLock);

    // go again
    if( !dccp->tt_stopping ) {
        HostOS_timer_mod( dccp->tt_timer_link, HostOS_timer_expire_ticks(TT_TASK_TIMEOUT) );
    }
}


/****************************************************************************
*           
*   Purpose:    TT command message handling thread.
*
*               Called as a tasklet.
*
*   Imports:    data - context data
*
*   Exports:    
*
*PUBLIC**********************************************************************/
SYS_VOID clnketh_tt_cmtask(SYS_ULONG data)
{
    dc_context_t *dccp = (dc_context_t *)data;    

//HostOS_PrintLog(L_INFO, "In clnketh_tt_cmtask() \n" );
    // pass rx message
    Clnk_MBX_Read_ISR( &dccp->mailbox ) ;

    // pass tx message
    Clnk_MBX_Write_ISR( &dccp->mailbox ) ;

#if 1
    // possibly re-schedule
    if( Clnk_MBX_Read_Hw_Mailbox_Check_Ready(  &dccp->mailbox ) )
    {
        clnketh_tt_task_sched( dccp, dccp->tt_cmtask_link ) ;
    }

    // possibly re-schedule
    if( !Clnk_MBX_Send_Mailbox_Check_Empty( &dccp->mailbox ) )
    {
        clnketh_tt_task_sched( dccp, dccp->tt_cmtask_link ) ;
    }
#endif
}

/****************************************************************************
*           
*   Purpose:    TT unsolicited message handling thread.
*
*               Called as a tasklet.
*
*   Imports:    data - context data
*
*   Exports:    
*
*PUBLIC**********************************************************************/
SYS_VOID clnketh_tt_umtask(SYS_ULONG data)
{
    dc_context_t *dccp = (dc_context_t *)data;    

    Clnk_MBX_Unsol_ISR( &dccp->mailbox ) ;

    // no re-schedule as Handle UnsolInterrupt already sucks up multiple messages 
}

/****************************************************************************
*           
*   Purpose:    Possibly reschedules a TT tasklet.
*
*   Imports:    dccp - control context
*               task - pointer to task structure
*
*   Exports:    
*
*STATIC**********************************************************************/
static void clnketh_tt_task_sched( dc_context_t *dccp, void *task )
{

    // possibly re-schedule
    if( !dccp->tt_stopping )
    {
        HostOS_task_schedule( task ) ;
    }

}










