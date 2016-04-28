/*******************************************************************************
*
* Common/Src/ctx_abs.c
*
* Description: context abstraction
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
*                                N O T E                                       *
********************************************************************************/



/*******************************************************************************
*                            # I n c l u d e s                                 *
********************************************************************************/

#include "drv_hdr.h"

/*
    The parameter  CONTEXT_DEBUG   must be defined to enable context
    linkage checking. With it defined you'll get error checking and
    messages about null context linkage pointers.
*/
#define CONTEXT_DEBUG       0   // define 1 for context linkage checking



/**
*  Purpose:    Converts a driver control context pointer to
*              a driver gpl context pointer.
*
*  Imports:    dccp - pointer to a device control context
*
*  Exports:    pointer to the driver gpl context
*
*PUBLIC**************************/
void *dc_to_dg( void *dccp )
{
    void *dgcp ;

#if CONTEXT_DEBUG
    if( dccp ) {
        dgcp = ((dc_context_t *)dccp)->p_dg_ctx;
    } else {
        HostOS_PrintLog(L_INFO, "Error - context linkage dc_to_dg\n" );
        dgcp = 0 ;
    }
#else
    dgcp = ((dc_context_t *)dccp)->p_dg_ctx ;
#endif

    return( dgcp ) ;
}


/**
*  Purpose: Converts a driver control context pointer to
*           a driver kernel context pointer.
*
*           This is the only way to get back to the dk!           
*
*  Imports: dccp - pointer to a device control context
*
*  Exports: pointer to the driver kernel context
*
*PUBLIC**************************/
void *dc_to_dk( void *dccp )
{
    void *dkcp ;

#if CONTEXT_DEBUG
    if( dccp ) {
        dkcp = ((dc_context_t *)dccp)->p_dk_ctx;
    } else {
        HostOS_PrintLog(L_INFO, "Error - context linkage dc_to_dk\n" );
        dkcp = 0 ;
    }
#else
    dkcp = ((dc_context_t *)dccp)->p_dk_ctx ;
#endif

    return( dkcp ) ;
}



