/*******************************************************************************
*
* Common/Src/misc_driver_stubs.c
*
* Description: Host OS abstraction support 
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

#include <stddef.h>
#include "drv_hdr.h"

/** Standalone link needs a main.  No useful binary is produced but at least
 *  we know that all the referenced symbols linked.  More proof that the 
 *  package is good.
 */
int main(int argc, char **argv)
{
    return 0;
}


/**
*  Purpose:    Converts a driver kernel context pointer to
*              a driver data context pointer.
*
*  Imports:    dkcp - pointer to a device kernel context
*
*  Exports:    pointer to the driver data context
*
*PUBLIC**************************/
void *dk_to_dd( void *dkcp )
{
    return NULL;
}

/**
*  Purpose:    Converts a driver data context pointer to
*              a driver gpl context pointer.
*
*  Imports:    ddcp - pointer to a device data context
*
*  Exports:    pointer to the driver gpl context
*
*PUBLIC**************************/
void *dd_to_dg( void *ddcp )
{
    return NULL;
}

/**
*  Purpose:    Converts a driver gpl context pointer to
*              a driver control context pointer.
*
*  Imports:    dgcp - pointer to a device gpl context
*
*  Exports:    pointer to the driver control context
*
*PUBLIC**************************/
void *dg_to_dc( void *dgcp )
{
    return NULL;
}

/**
*  Purpose:    Converts a driver gpl context pointer to
*              a driver data context pointer.
*
*  Imports:    dgcp - pointer to a device gpl context
*
*  Exports:    pointer to the driver data context
*
*PUBLIC**************************/
void *dg_to_dd( void *dgcp )
{
    return NULL;
}



////////////////////////////////////////////////////////////
// === combo functions
////////////////////////////////////////////////////////////

/**
 *  Purpose:    Converts a driver kernel context pointer to
 *              a driver control context pointer.
 *
 *  Imports:    dkcp - pointer to a device kernel context
 *
 *  Exports:    pointer to the driver control context
 *
*PUBLIC**************************/
void *dk_to_dc( void *dkcp )
{
    return NULL;
}

/**
 *  Purpose:    Converts a driver data context pointer to
 *              a driver control context pointer.
 *
 *  Imports:    dkcp - pointer to a device kernel context
 *
 *  Exports:    pointer to the driver control context
 *
*PUBLIC**************************/
void *dd_to_dc( void *ddcp )
{
    return NULL;
}

/**
 *  Purpose:    Converts a driver kernel context pointer to
 *              a driver gpl context pointer.
 *
 *  Imports:    dkcp - pointer to a device kernel context
 *
 *  Exports:    pointer to the driver gpl context
 *
*PUBLIC**************************/
void *dk_to_dg( void *dkcp )
{
    return NULL;
}

/**
 *  Purpose:    Converts a driver control context pointer to
 *              a driver data context pointer.
 *
 *  Imports:    dccp - pointer to a device control context
 *
 *  Exports:    pointer to the driver data context
 *
*PUBLIC**************************/
void *dc_to_dd( void *dccp )
{
    return NULL;
}


/*******************************************************************************
*           
* Purpose:  Allocates and clears a DG
*    
* Inputs:
*
* Outputs:  the context
*
*PUBLIC***************************************************************************/
void *ctx_alloc_dg_context( void )
{
    return NULL;
}

/*******************************************************************************
*           
* Purpose:  Frees a DG
*    
* Inputs:   dgcp - the DG context to free
*
* Outputs:  
*
*PUBLIC***************************************************************************/
void ctx_free_dg_context( void *dgcp )
{
    return;
}

/*******************************************************************************
*           
* Purpose:  links a DG somewhere
*    
* Inputs:   vdgcp - void pointer to DG context 
*           ddcp  - void pointer to DD context
*           dccp  - void pointer to DC context
*
* Outputs:  
*
*PUBLIC***************************************************************************/
void ctx_link_dg_context( void *vdgcp, void *ddcp, void *dccp )
{
    return;
}

/*******************************************************************************
*
* Description:
*       Initializes the OS context.
*
* Inputs:
*       vdgcp        - void Pointer to the OS context.
*
* Outputs:
*       0 = SYS_SUCCESS
*
*PUBLIC***************************************************************************/
void Clnk_init_os_context( void *vdgcp )
{
    return;
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
*PUBLIC***************************************************************************/
void ctx_linkage_lister( void *dkcp, unsigned int **vp, int len )
{
    return;
}



#if defined(E1000_DRVR_SUPPORT) || defined(CANDD_DRVR_SUPPORT)

int clnk_write( void *vctx, SYS_UINT32 addr, SYS_UINT32 data)
{
    return 0;
}

int clnk_read( void *vctx, SYS_UINT32 addr, SYS_UINT32 *data)
{
    return 0;
}

int clnk_write_burst( void *vctx, SYS_UINT32 addr, SYS_UINT32 *data, unsigned int size, int inc )
{
    return 0;
}

int clnk_read_burst( void *vctx, SYS_UINT32 addr, SYS_UINT32 *data, unsigned int size, int inc)
{
    return 0;
}

#endif

