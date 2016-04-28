/*******************************************************************************
*
* Common/Src/ctx_setup.c
*
* Description: context setup support
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
*            S t a t i c   M e t h o d   P r o t o t y p e s                   *
********************************************************************************/

static int Clnk_alloc_contexts(void **ddcp_dgcp, void *ddcp );
static void Clnk_init_control_context( dc_context_t *dccp, void *dkcp, unsigned long dev_base );



/*******************************************************************************
*
* Description:
*       Initializes the context structures for a device.
*       Called from the probe function for each device.
*
*       Allocate our context
*       Put our context in the adapter context.
*       Put the adapter context pointer in our context.
*    
* Inputs:
*       ddcp_dgcp   - Pointer into the driver data context to the
*                     place to save our gpl context.
*       ddcp        - Pointer to the adapter context.
*                     This will be saved in our context.        
*
* Outputs:
*       0 or error code
*
*STATIC***************************************************************************/
static int Clnk_alloc_contexts(void **ddcp_dgcp, void *ddcp )
{
    void         *dgcp ;
    dc_context_t *dccp ;

    // Allocate the contexts

    dgcp = ctx_alloc_dg_context();
    if( !dgcp )
    {
        return (-SYS_OUT_OF_MEMORY_ERROR );
    }

    dccp = (dc_context_t *)ctx_alloc_dc_context();
    if( !dccp )
    {
        ctx_free_dg_context( dgcp ) ;
        return (-SYS_OUT_OF_MEMORY_ERROR );
    }

    // linkage init
    *ddcp_dgcp     = dgcp;   // link dd to dg
    dccp->p_dg_ctx = dgcp;   // link dc to dg
    ctx_link_dg_context( dgcp, ddcp, dccp ) ;

    return(SYS_SUCCESS);
}

/*******************************************************************************
*
* Description:
*       Initializes the control context.
*
* Inputs:
*       dccp        - Pointer to the control context.
*       dkcp        - Pointer to the kernel context.
*       dev_base    - Device base address
*
* Outputs:
*       0 = SYS_SUCCESS
*
*STATIC***************************************************************************/
static void Clnk_init_control_context( dc_context_t *dccp, void *dkcp, unsigned long dev_base )
{
    unsigned int *linkarray[4], **vp ;

    dccp->p_dk_ctx = dkcp ;  // reverse link to the kernel context
    dccp->baseAddr = dev_base; 

    vp = &linkarray[0] ;
    ctx_linkage_lister( dkcp, vp, 4 ) ;

    // the order of this list must match that in ctx_linkage_lister
    dccp->at_lock_link                  = (void *)*vp++ ;
    dccp->ioctl_sem_link                = (void *)*vp++ ;
    dccp->mbx_cmd_lock_link             = (void *)*vp++ ;
    dccp->mbx_swun_lock_link            = (void *)*vp++ ;
}

/*******************************************************************************
*
* Description:
*       Initializes the clink for a device.
*       Called from the probe function for each device.
*
*       Allocate our contexts
*       Put our context in the adapter context.
*       Put the adapter context pointer in our context.
*       Point the control context at the gpl context.
*    
* Inputs:
*       ddcp_dgcp   - Pointer into the driver data context to the
*                     place to save our gpl context.
*       ddcp        - Pointer to the adapter context.
*                     This will be saved in our context.        
*       dkcp        - Pointer to the kernel context.
*                     This will be saved in the control context.        
*       dev_base    - Device base address
*
* Outputs:
*       0 = SYS_SUCCESS
*       -SYS_OUT_OF_MEMORY_ERROR
*
*PUBLIC***************************************************************************/
int Clnk_init_dev(void **ddcp_dgcp, void *ddcp, void *dkcp, unsigned long dev_base )
{
    void         *dgcp ;
    dc_context_t *dccp ;
    int err ;

    // Allocate the contexts

    err = Clnk_alloc_contexts( ddcp_dgcp, ddcp );
    if( !err )
    {
        dgcp = dd_to_dg( ddcp ) ;
        dccp = dg_to_dc( dgcp ) ;

        // Initialize OS context

        Clnk_init_os_context( dgcp ) ; 

        // Initialize control context

        Clnk_init_control_context( dccp, dkcp, dev_base ) ;
    }

    return(err);
}

/*******************************************************************************
*
* Description:
*       De-Initializes the Mailbox Module for a device.
*       Called from the remove function for each device.
*
*       The thinking here, based on assumptions about what the
*       kernel does during remove, is to assume that ioctls
*       are stopped already and that there is no need to lock
*       them out.
*
*       Stops the Timer Task.
*       Offs the mailboxes.
*       De-Allocates everything.
*    
* Inputs:
*       vdgcp - Pointer into the gpl context 
*
* Outputs:
*       0 = SYS_SUCCESS
*       -SYS_OUT_OF_MEMORY_ERROR
*
*PUBLIC***************************************************************************/
void Clnk_exit_dev( void *vdgcp )
{
    dc_context_t *dccp ;

    if( vdgcp ) {
        dccp = dg_to_dc( vdgcp ) ;

        // Free any wqts
        Clnk_MBX_Free_wqts( &dccp->mailbox ) ;

        ctx_free_dc_context( dccp ) ; 
        ctx_free_dg_context( vdgcp ) ;
    }
}

/*******************************************************************************
*           
* Purpose:  Allocates and clears a DC
*    
* Inputs:
*
* Outputs:  the context
*
*PUBLIC***************************************************************************/
void *ctx_alloc_dc_context( void )
{
    dc_context_t *dccp ;

    dccp = (dc_context_t *)HostOS_Alloc(sizeof(dc_context_t));
    if( dccp )
    {
        HostOS_Memset(dccp, 0, sizeof(dc_context_t));
    }

    return(dccp);
}

/*******************************************************************************
*           
* Purpose:  Frees a DC
*    
* Inputs:   dccp - the DC context to free
*
* Outputs:  
*
*PUBLIC***************************************************************************/
void ctx_free_dc_context( void *dccp )
{

    HostOS_Free( dccp, sizeof(dc_context_t) ) ;
}




