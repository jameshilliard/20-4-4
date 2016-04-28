/*******************************************************************************
*
* Common/Src/ClnkIo.c
*
* Description: ioctl layer
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

static SYS_INT32 clnkioc_driver_cmd_work( void *dkcp, IfrDataStruct *kifr );
static SYS_INT32 clnkioc_mbox_cmd_request_work( void *dkcp, IfrDataStruct *kifr, int response );
static SYS_INT32 clnkioc_mbox_unsolq_retrieve_work( void *dkcp, IfrDataStruct *kifr );
static SYS_INT32 clnkioc_mem_read_work( void *dkcp, IfrDataStruct *kifr );
#ifndef SONICS_ADDRESS_OF_sniff_buffer_pa
#error "Sniff buffer is not defined"
#else
#if SONICS_ADDRESS_OF_sniff_buffer_pa
static SYS_INT32 clnkioc_sniff_read_work( void *dkcp, IfrDataStruct *kifr );
#endif
#endif
static SYS_INT32 clnkioc_mem_write_work( void *dkcp, IfrDataStruct *kifr );
static SYS_INT32 clnkioc_copy_request_block( void *arg, IfrDataStruct *kifr );
static SYS_INT32 clnkioc_io_block_setup( dc_context_t *dccp, IfrDataStruct *kifr, 
                                         struct clnk_io *uio, struct clnk_io *kio );
static SYS_INT32 clnkioc_io_block_return( void *dkcp, struct clnk_io *uio, 
                                          struct clnk_io *kio, SYS_UINTPTR us_ioblk,
                                          void *odata );


/****************************************************************************
*                      IOCTL Methods                                        *
*****************************************************************************/


/**
 *  Purpose:    IOCTL entry point for SIOCCLINKDRV 
 *
 *  Imports:    dkcp - driver kernel context pointer
 *              arg - request structure from user via kernel
 *
 *  Exports:    Error status
 *              Response data, if any, is returned via ioctl structures
 *
*PUBLIC***************************************************************************/
unsigned long clnkioc_driver_cmd( void *dkcp, void *arg )
{
    dc_context_t    *dccp = dk_to_dc( dkcp ) ;
    SYS_INT32       error = SYS_SUCCESS;
    IfrDataStruct   kifr;

    // copy the user's request block to kernel space
    error = clnkioc_copy_request_block( arg, &kifr ) ; 
    if( !error )
    {

        //HostOS_Lock(dccp->ioctl_lock_link);
        error = HostOS_mutex_acquire_intr( dccp->ioctl_sem_link ) ;
        if( !error ) {

            // do the real IOCTL work    
            error = clnkioc_driver_cmd_work( dkcp, &kifr ) ;

            HostOS_mutex_release( dccp->ioctl_sem_link );
        }
        //HostOS_Unlock(dccp->ioctl_lock_link);

    }

    return( error ) ;
}

/**
 *  Purpose:    IOCTL entry point for SIOCCLINKDRV 
 *
 *  Imports:    dkcp - driver kernel context pointer
 *              kifr - kernel ifr pointer
 *
 *  Exports:    Error status
 *              Response data, if any, is returned via ioctl structures
 *
*STATIC***************************************************************************/
static SYS_INT32 clnkioc_driver_cmd_work( void *dkcp, IfrDataStruct *kifr )
{
    dc_context_t *dccp = dk_to_dc( dkcp ) ;
    SYS_UINTPTR  param1, param2, param3;
    SYS_INT32    error = SYS_SUCCESS;
    struct clnk_io uio, kio;
    SYS_UINT32 cmd ;

    param1  = (SYS_UINTPTR)kifr->param1;        // ioctl command
    param2  = (SYS_UINTPTR)kifr->param2;        // ioctl io block
    param3  = (SYS_UINTPTR)kifr->param3;        // version constant
    cmd     = param1;

#if DEBUG_IOCTL_PRIV    
    //HostOS_PrintLog(L_ERR, "ioctl priv %x %x %x %x\n", kifr->cmd, param1, param2, param3);
#endif

    // copy the user's io block to kernel space
    error = clnkioc_io_block_setup( dccp, kifr, &uio, &kio ) ;
    if( !error ) {

        if( CLNK_CTL_FOR_DRV(cmd)) {
            error = clnk_ctl_drv( dkcp, cmd, &kio );
        } else if( CLNK_CTL_FOR_ETH(cmd) ) {
            error = Clnk_ETH_Control_drv( dccp, CLNK_ETH_CTRL_DO_CLNK_CTL, cmd, (SYS_UINTPTR)&kio, 0);

            /* special handling for a few cmds */
            if( !error ) {
                clnk_ctl_postprocess( dkcp, kifr, &kio );
            }
        }

        // send reply back to IOCTL caller
        if( !error ) {
            error = clnkioc_io_block_return( dkcp, &uio, &kio, param2, dccp->clnk_ctl_out ) ;
        }
    }

#if DEBUG_IOCTL_PRIV    
    if( error ) {   
        HostOS_PrintLog(L_ERR, "ioctl priv %x %x %x %x\n", kifr->cmd, param1, param2, param3);
        HostOS_PrintLog(L_ERR, "ioctl err=%d.\n", error );
    }
#endif
    return( error );
}



/**
 *  Purpose:    IOCTL entry point for request-response command
 *              This is a daemon sending a command mailbox request 
 *              and expecting a response.
 *
 *  Imports:    dkcp     - driver kernel context pointer
 *              arg      - request structure from user via kernel
 *              response - 1 for response expected
 *                         0 for no response expected
 *
 *  Exports:    Error status
 *              Response data, if any, is returned via ioctl structures
 *
*PUBLIC***************************************************************************/
unsigned long clnkioc_mbox_cmd_request( void *dkcp, void *arg, int response )
{
    dc_context_t  *dccp = dk_to_dc( dkcp ) ;
    SYS_INT32     error = SYS_SUCCESS;
    IfrDataStruct kifr;

    // copy the user's request block to kernel space
    error = clnkioc_copy_request_block( arg, &kifr ) ; 
    if( !error )
    {

        //HostOS_Lock(dccp->ioctl_lock_link);
        error = HostOS_mutex_acquire_intr( dccp->ioctl_sem_link ) ;
        if( !error ) {

            // do the real IOCTL work    
            error = clnkioc_mbox_cmd_request_work( dkcp, &kifr, response ) ;

            HostOS_mutex_release( dccp->ioctl_sem_link );
        }
        //HostOS_Unlock(dccp->ioctl_lock_link);

    }

    return( error );
}

/**
 *  Purpose:    IOCTL entry point for request-response command
 *
 *  Imports:    dkcp     - driver kernel context pointer
 *              kifr     - kernel ifr pointer
 *              response - 1 for response expected
 *                         0 for no response expected
 * 
 *  Exports:    Error status
 *              Response data, if any, is returned via ioctl structures
 *
*STATIC***************************************************************************/
static SYS_INT32 clnkioc_mbox_cmd_request_work( void *dkcp, IfrDataStruct *kifr, int response )
{
    dc_context_t   *dccp = dk_to_dc( dkcp ) ;
    SYS_UINTPTR    param1, param2, param3;
    SYS_INT32      error = SYS_SUCCESS;
    struct clnk_io uio, kio;
    SYS_UINT32     cmd ;

    param1  = (SYS_UINTPTR)kifr->param1;        // ioctl command
    param2  = (SYS_UINTPTR)kifr->param2;        // ioctl io block
    param3  = (SYS_UINTPTR)kifr->param3;        // version constant
    cmd     = param1;

#if DEBUG_IOCTL_CMDQ    
    HostOS_PrintLog(L_INFO, "ioctl crw %x %x %x %x\n", kifr->cmd, param1, param2, param3);
#endif        

    // copy the user's io block to kernel space
    error = clnkioc_io_block_setup( dccp, kifr, &uio, &kio ) ;
    if( !error ) {

        if( response ) {

            // send MESSAGE - wait for response

            error = clnk_cmd_msg_send_recv(       dccp, kifr->cmd, param1, &kio ) ;
            /* special handling for a few cmds */
            if( !error ) {
                clnk_ctl_postprocess( dkcp, kifr, &kio );
            }

        } else {

            // send MESSAGE

            error = clnk_cmd_msg_send( dccp, kifr->cmd, param1, &kio ) ;

        }

        if( !error ) {
            // send reply back to IOCTL caller
            error = clnkioc_io_block_return( dkcp, &uio, &kio, param2, dccp->clnk_ctl_out ) ;
        }
    }

#if DEBUG_IOCTL_CMDQ    
    if( error ) {   
        HostOS_PrintLog(L_ERR, "ioctl crw %x %x %x %x\n", kifr->cmd, param1, param2, param3);
        HostOS_PrintLog(L_ERR, "ioctl err=%d.\n", error );
    }
#endif
    return( error );
}


/**
 *  Purpose:    IOCTL entry point for unsolicited message retrieval
 *
 *  Imports:    dkcp - driver kernel context pointer
 *              arg - request structure from user via kernel
 *
 *  Exports:    Error status
 *              Response data, if any, is returned via ioctl structures
 *
*PUBLIC***************************************************************************/
unsigned long clnkioc_mbox_unsolq_retrieve( void *dkcp, void *arg ) 
{
    dc_context_t  *dccp = dk_to_dc( dkcp ) ;
    SYS_INT32     error = SYS_SUCCESS;
    IfrDataStruct kifr;

    // copy the user's request block to kernel space
    error = clnkioc_copy_request_block( arg, &kifr ) ; 
    if( !error )
    {

        //HostOS_Lock(dccp->ioctl_lock_link);
        error = HostOS_mutex_acquire_intr( dccp->ioctl_sem_link ) ;
        if( !error ) {

            // do the real IOCTL work    
            error = clnkioc_mbox_unsolq_retrieve_work( dkcp, &kifr ) ;

            HostOS_mutex_release( dccp->ioctl_sem_link );
        }
        //HostOS_Unlock(dccp->ioctl_lock_link);

    }

    return( error );
}

/**
 *  Purpose:    IOCTL entry point for sw unsolicited retrieval
 *
 *  Imports:    dkcp - driver kernel context pointer
 *              kifr - kernel ifr pointer
 * 
 *  Exports:    Error status
 *              Response data, if any, is returned via ioctl structures
 *
*STATIC***************************************************************************/
static SYS_INT32 clnkioc_mbox_unsolq_retrieve_work( void *dkcp, IfrDataStruct *kifr )
{
    dc_context_t    *dccp = dk_to_dc( dkcp ) ;
    SYS_UINTPTR     param1, param2, param3;
    SYS_INT32       error = SYS_SUCCESS;
    struct clnk_io  uio, kio ;

    param1  = (SYS_UINTPTR)kifr->param1;        // ioctl command
    param2  = (SYS_UINTPTR)kifr->param2;        // ioctl io block
    param3  = (SYS_UINTPTR)kifr->param3;        // version constant

#if DEBUG_IOCTL_UNSOLQ    
    //HostOS_PrintLog(L_ERR, "ioctl urw %x %x %x %x\n", kifr->cmd, param1, param2, param3);
#endif        

    // copy the user's io block to kernel space
    error = clnkioc_io_block_setup( dccp, kifr, &uio, &kio ) ;
    if( !error ) {

        if( uio.out_len != MAX_UNSOL_MSG )
        {
            _ioctl_dbg( dkcp, "SW UNSOL length");
            error = -SYS_INVALID_ARGUMENT_ERROR;
        } else {

            error = Clnk_MBX_RcvUnsolMsg( &dccp->mailbox, (SYS_UINT32 *)dccp->clnk_ctl_out ) ;
            if( !error ) {
                // send reply back to IOCTL caller
                error = clnkioc_io_block_return( dkcp, &uio, &kio, param2, dccp->clnk_ctl_out ) ;
            }
        }
    }

#if DEBUG_IOCTL_UNSOLQ    
    if( error ) {   
        HostOS_PrintLog(L_ERR, "ioctl urw %x %x %x %x\n", kifr->cmd, param1, param2, param3);
        HostOS_PrintLog(L_ERR, "ioctl err=%d.\n", error );
    }
#endif
    return( error );
}



/**
 *  Purpose:    IOCTL entry point for reading clink memory/registers
 *
 *  Imports:    dkcp - driver kernel context pointer
 *              arg  - request structure from user via kernel
 *
 *  Exports:    Error status
 *              Response data, if any, is returned via ioctl structures
 *
*PUBLIC***************************************************************************/
unsigned long clnkioc_mem_read( void *dkcp, void *arg )
{
    dc_context_t    *dccp = dk_to_dc( dkcp ) ;
    SYS_INT32       error = SYS_SUCCESS;
    IfrDataStruct   kifr;

    // copy the user's request block to kernel space
    error = clnkioc_copy_request_block( arg, &kifr ) ; 
    if( !error )
    {

        //HostOS_Lock(dccp->ioctl_lock_link);
        error = HostOS_mutex_acquire_intr( dccp->ioctl_sem_link ) ;
        if( !error ) {

            // do the real IOCTL work    
            error = clnkioc_mem_read_work( dkcp, &kifr ) ;

            HostOS_mutex_release( dccp->ioctl_sem_link );
        }
        //HostOS_Unlock(dccp->ioctl_lock_link);

    }

    return( error );
}

#ifndef SONICS_ADDRESS_OF_sniff_buffer_pa
#error "Sniff buffer is not defined"
#else
#if SONICS_ADDRESS_OF_sniff_buffer_pa

// ########################################################################
// ##              clnkioc_sniff_read - read sniffer buffer              ##
// ########################################################################
// ## Description:                                                       ##
// ##   Handle the ioctl call that request to read the sniff buffer. The ##
// ##   sniff buffer is located in the host memory, and is allocated by  ##
// ##   the driver. The SoC then fills it with descriptor information as ##
// ##   it is logged. This function reads the buffer and passes it to    ##
// ##   the user space memroy.                                           ##
// ##____________________________________________________________________##
// ## Input:                                                             ##
// ##    dkcp = pointer to get the context from                          ##
// ##    arg = the argument block                                        ##
// ##____________________________________________________________________##
// ## Globals Used:                                                      ##
// ##                                                                    ##
// ##____________________________________________________________________##
// ## Return:                                                            ##
// ##                                                                    ##
// ##____________________________________________________________________##
// ## Side effects:                                                      ##
// ##                                                                    ##
// ########################################################################
unsigned long clnkioc_sniff_read( void *dkcp, void *arg )
{
    dc_context_t    *dccp = dk_to_dc( dkcp ) ;
    SYS_INT32       error = SYS_SUCCESS;
    IfrDataStruct   kifr;

    // copy the user's request block to kernel space
    error = clnkioc_copy_request_block( arg, &kifr ) ; 
    if( !error )
    {

        //HostOS_Lock(dccp->ioctl_lock_link);
        error = HostOS_mutex_acquire_intr( dccp->ioctl_sem_link ) ;
        if( !error ) {

            // do the real IOCTL work    
            error = clnkioc_sniff_read_work( dkcp, &kifr ) ;

            HostOS_mutex_release( dccp->ioctl_sem_link );
        }
        //HostOS_Unlock(dccp->ioctl_lock_link);

    }

    return( error );
}

extern unsigned int *sniff_buffer;		// Virtual address
extern unsigned int *sniff_buffer_pa;		// Physical address

int one_shot;					// Used for debugging only

// ########################################################################
// ##              sniff_read - actual read and copy of data             ##
// ########################################################################
// ## Description:                                                       ##
// ##   This function actually copies the data from the sniff buffer to  ##
// ##   the target memory.                                               ##
// ##____________________________________________________________________##
// ## Input:                                                             ##
// ##   dccp = context pointer                                           ##
// ##   addr = offset into the sniff buffer (offset in bytes)            ##
// ##   tgt = target pointer                                             ##
// ##   len = length in bytes                                            ##
// ##____________________________________________________________________##
// ## Globals Used:                                                      ##
// ##                                                                    ##
// ##____________________________________________________________________##
// ## Return:                                                            ##
// ##   0                                                                ##
// ##____________________________________________________________________##
// ## Side effects:                                                      ##
// ##                                                                    ##
// ########################################################################

int sniff_read(dc_context_t *dccp, SYS_UINT32 addr, SYS_UINT32 *tgt, int len)
	{
	int offset;
        //----------------------------------------------------------------------
        // Special case - address request of -1 will return the physical
        // address of the circular queue. This is needed so that we can plug in
        // the address of the address of the circular queue into the SoC at the
        // desired point at startup. This is a sort of bypass to passing the
        // necessary "configuration" data to the SoC.
        //----------------------------------------------------------------------
if (++one_shot < 17) { HostOS_PrintLog(L_INFO, "sniff_read(%08x,xx,%d)\n", addr,len); } else one_shot--;
	if (addr == -4)
		{
		*tgt++ = (SYS_UINT32) sniff_buffer_pa;
		return(0);
		}
        //----------------------------------------------------------------------
        // Convert theaddress ot a word offset from a byte offset
        //----------------------------------------------------------------------
	addr /= sizeof(unsigned int);		
        //----------------------------------------------------------------------
        // Copy the data across to the target buffer specified. This will copy
        // the bytes from the host circicular queue buffer to the requested
        // buffer that will eventually end up in user space.
        //----------------------------------------------------------------------
	for (offset=0;len>0;offset++, len -= sizeof(SYS_UINT32))
		tgt[offset] = sniff_buffer[addr + offset];
        //----------------------------------------------------------------------
        // Finally - identify the buffer as being consumed by zeroing out the
        // value of the first word. This word is used as both a sequence number
        // and an "ownership" flag. When it is set to a 1 by the SoC it is
        // expected that we will read it here and set it back to zero.
        //----------------------------------------------------------------------
	if (*tgt)
		sniff_buffer[addr] = 0;
	return(0);
	}

// ########################################################################
// ##         clnkioc_sniff_read_work - actual read of the memory        ##
// ########################################################################
// ## Description:                                                       ##
// ##    This function calls the actual function to perform the work of  ##
// ##    moving the data to the target buffer.                           ##
// ##____________________________________________________________________##
// ## Input:                                                             ##
// ##    dkcp = context pointer                                          ##
// ##    kifr = io structure that contains infomation about how to get   ##
// ##           the parameters                                           ##
// ##____________________________________________________________________##
// ## Globals Used:                                                      ##
// ##                                                                    ##
// ##____________________________________________________________________##
// ## Return:                                                            ##
// ##   error code - 0 = all is OK, else not good                        ##
// ##____________________________________________________________________##
// ## Side effects:                                                      ##
// ##                                                                    ##
// ########################################################################
static SYS_INT32 clnkioc_sniff_read_work( void *dkcp, IfrDataStruct *kifr )
{
    dc_context_t *dccp = dk_to_dc( dkcp ) ;
    SYS_UINTPTR  param1, param2, param3;
    SYS_INT32    error = SYS_SUCCESS;
    struct clnk_io uio, kio;
    SYS_UINT32   addr ;

    param1  = (SYS_UINTPTR)kifr->param1;        // ioctl command
    param2  = (SYS_UINTPTR)kifr->param2;        // ioctl io block
    param3  = (SYS_UINTPTR)kifr->param3;        // version constant


    // copy the user's io block to kernel space
    error = clnkioc_io_block_setup( dccp, kifr, &uio, &kio ) ;
    if( !error ) {

        // check stuff
        if( (uio.out_len == 0) ) {
            _ioctl_dbg( dkcp, "clnkioc_sniff_read_work 0 out length");
            error = -SYS_INVALID_ARGUMENT_ERROR;
        } else {

            addr = kio.in_len ? kio.in[0] : ((SYS_UINTPTR)kio.in);

            // the READ

            sniff_read( dccp, addr, (SYS_UINT32 *)kio.out, kio.out_len ) ;

            // send reply back to IOCTL caller
            error = clnkioc_io_block_return( dkcp, &uio, &kio, param2, dccp->clnk_ctl_out ) ;
        }
    }

    return( error );
}

#endif
#endif

/**
 *  Purpose:    IOCTL entry point for reading clink memory/registers
 *
 *  Imports:    dkcp - driver kernel context pointer
 *              kifr - kernel ifr pointer
 * 
 *  Exports:    Error status
 *              Response data, if any, is returned via ioctl structures
 *
*STATIC***************************************************************************/
static SYS_INT32 clnkioc_mem_read_work( void *dkcp, IfrDataStruct *kifr )
{
    dc_context_t *dccp = dk_to_dc( dkcp ) ;
    SYS_UINTPTR  param1, param2, param3;
    SYS_INT32    error = SYS_SUCCESS;
    struct clnk_io uio, kio;
    SYS_UINT32   addr ;

    param1  = (SYS_UINTPTR)kifr->param1;        // ioctl command
    param2  = (SYS_UINTPTR)kifr->param2;        // ioctl io block
    param3  = (SYS_UINTPTR)kifr->param3;        // version constant

#if DEBUG_IOCTL_MEM    
    //HostOS_PrintLog(L_ERR, "ioctl mrw %x %x %x %x\n", kifr->cmd, param1, param2, param3);
#endif        

    // copy the user's io block to kernel space
    error = clnkioc_io_block_setup( dccp, kifr, &uio, &kio ) ;
    if( !error ) {

        // check stuff
        if( (uio.out_len == 0) ) {
            _ioctl_dbg( dkcp, "DRV_CLNK_CTL 0 out length");
            error = -SYS_INVALID_ARGUMENT_ERROR;
        } else {

            addr = kio.in_len ? kio.in[0] : ((SYS_UINTPTR)kio.in);

            // the READ

            clnk_blk_read( dccp, addr, (SYS_UINT32 *)kio.out, kio.out_len ) ;

            // send reply back to IOCTL caller
            error = clnkioc_io_block_return( dkcp, &uio, &kio, param2, dccp->clnk_ctl_out ) ;
        }
    }

#if DEBUG_IOCTL_MEM 
    if( error ) {   
        HostOS_PrintLog(L_ERR, "ioctl mrw %x %x %x %x\n", kifr->cmd, param1, param2, param3);
        HostOS_PrintLog(L_ERR, "ioctl err=%d.\n", error );
    }
#endif
    return( error );
}


/**
 *  Purpose:    IOCTL entry point for writing clink memory/registers
 *
 *  Imports:    dkcp - driver kernel context pointer
 *              arg  - request structure from user via kernel
 *
 *  Exports:    Error status
 *              Response data, if any, is returned via ioctl structures
 *
*PUBLIC***************************************************************************/
unsigned long clnkioc_mem_write( void *dkcp, void *arg )
{
    dc_context_t    *dccp = dk_to_dc( dkcp ) ;
    SYS_INT32       error = SYS_SUCCESS;
    IfrDataStruct   kifr;

    // copy the user's request block to kernel space
    error = clnkioc_copy_request_block( arg, &kifr ) ; 
    if( !error )
    {

        //HostOS_Lock(dccp->ioctl_lock_link);
        error = HostOS_mutex_acquire_intr( dccp->ioctl_sem_link ) ;
        if( !error ) {

            // do the real IOCTL work    
            error = clnkioc_mem_write_work( dkcp, &kifr ) ;

            HostOS_mutex_release( dccp->ioctl_sem_link );
        }
        //HostOS_Unlock(dccp->ioctl_lock_link);

    }

    return( error );
}

/**
 *  Purpose:    IOCTL entry point for writing clink memory/registers
 *
 *  Imports:    dkcp - driver kernel context pointer
 *              kifr - kernel ifr pointer
 * 
 *  Exports:    Error status
 *              Response data, if any, is returned via ioctl structures
 *
*STATIC***************************************************************************/
static SYS_INT32 clnkioc_mem_write_work( void *dkcp, IfrDataStruct *kifr )
{
    dc_context_t *dccp = dk_to_dc( dkcp ) ;
    SYS_UINTPTR  param1, param2, param3;
    SYS_INT32    error = SYS_SUCCESS;
    struct clnk_io uio, kio;
    SYS_UINT32   addr, *data, len ;

    param1  = (SYS_UINTPTR)kifr->param1;        // ioctl command
    param2  = (SYS_UINTPTR)kifr->param2;        // ioctl io block
    param3  = (SYS_UINTPTR)kifr->param3;        // version constant

#if DEBUG_IOCTL_MEM    
    //HostOS_PrintLog(L_ERR, "ioctl mww %x %x %x %x\n", kifr->cmd, param1, param2, param3);
#endif        

    // copy the user's io block to kernel space
    error = clnkioc_io_block_setup( dccp, kifr, &uio, &kio ) ;
    if( !error ) {

        addr = kio.in[0];               // first  in long is the address
        data = &kio.in[1];              // second in long is the first data long
        len  = kio.in_len - sizeof(SYS_UINT32);  // len is the block length less the address long
        if( len < sizeof(SYS_UINT32) )
        {
            // kio.in_len includes the address (4 bytes) and the data
            /* minimum write is 1 word */
            _ioctl_dbg( dkcp, "DRV_CLNK_CTL min write");
            error = -SYS_INVALID_ADDRESS_ERROR;
        } else {

            // the WRITE
            clnk_blk_write(dccp, addr, (SYS_UINT32 *)data, len);

            // send reply back to IOCTL caller
            error = clnkioc_io_block_return( dkcp, &uio, &kio, param2, dccp->clnk_ctl_out ) ;
        }
    }

#if DEBUG_IOCTL_MEM 
    if( error ) {   
        HostOS_PrintLog(L_ERR, "ioctl mww %x %x %x %x\n", kifr->cmd, param1, param2, param3);
        HostOS_PrintLog(L_ERR, "ioctl err=%d.\n", error );
    }
#endif
    return( error );
}

/**
 *  Purpose:    Copies the IOCTL request block to kernel space
 *              Maybe performs some checks.
 *
 *  Imports:    arg  - request structure from user via kernel
 *              kifr - kernel space block to recieve IOCTL request block       
 *
 *  Exports:    Error status
 *
*STATIC***************************************************************************/
static SYS_INT32 clnkioc_copy_request_block( void *arg, IfrDataStruct *kifr )
{
    SYS_INT32    error = SYS_SUCCESS;
    // copy the user's request block to kernel space
    if( HostOS_copy_from_user( (void *)kifr, arg, sizeof(IfrDataStruct)) ) {
        HostOS_PrintLog(L_INFO, "fault: %p\n", arg );
        error = -SYS_INVALID_ADDRESS_ERROR ;
    } else {
		// cursory checks
        if( (SYS_INT32)kifr->param3 != CLNK_CTL_VERSION ) {
            HostOS_PrintLog(L_INFO, "fault: version not %d\n", CLNK_CTL_VERSION );
            error = -SYS_PERMISSION_ERROR ;
        }
    }

    return( error );
}

/**
 *  Purpose:    Setup io block for IOCTL 
 *
 *              This is called after clnkioc_copy_request_block()
 *
 *              The io block is copied from user space to kernel space
 *              into the uio. The uio is then used to setup the kio.
 *
 *  Imports:    dccp - driver control context pointer
 *              kifr - kernel ifr pointer
 *              uio  - io block copied from user space
 *              kio  - kernel io block
 *
 *  Exports:    Error status
 *
*STATIC***************************************************************************/
static SYS_INT32 clnkioc_io_block_setup( dc_context_t *dccp, IfrDataStruct *kifr, 
                                         struct clnk_io *uio, struct clnk_io *kio )
{
    SYS_UINTPTR  param1, param2 ;
    SYS_INT32    error = SYS_SUCCESS;
    SYS_UINT32   in_len, max_in, max_out;

    param1  = (SYS_UINTPTR)kifr->param1;        // ioctl command
    param2  = (SYS_UINTPTR)kifr->param2;        // ioctl io block pointer

    // copy the user's io block to kernel space
    if( HostOS_copy_from_user( uio, (void *)param2, sizeof(struct clnk_io)) ) {
        _ioctl_dbg( dkcp, "DRV_CLNK_CTL copy _from_user (uio)");
        error = -SYS_INVALID_ADDRESS_ERROR;
    } else {

        in_len = uio->in_len;

        max_in  = CLNK_CTL_MAX_IN_LEN;
        max_out = CLNK_CTL_MAX_OUT_LEN;

        if( (in_len > max_in)        || 
            (in_len & 3)             || 
            (uio->out_len > max_out) ||
            (uio->out_len & 3)          ) {
            _ioctl_dbg( dkcp, "DRV_CLNK_CTL lengths");
            error = -SYS_INVALID_ARGUMENT_ERROR;
        } else {
            if( in_len ) {   // 'in' to the driver
                // copy data block to fixed buffer
                if( HostOS_copy_from_user( dccp->clnk_ctl_in, uio->in, uio->in_len) )
                {
                    _ioctl_dbg( dkcp, "DRV_CLNK_CTL copy _from_user (data in)");
                    error = -SYS_INVALID_ADDRESS_ERROR;
                } else {
                    // set the kio 'in' pointer
                    kio->in     = (SYS_UINT32 *)( dccp->clnk_ctl_in );
                    kio->in_len = in_len;
                }
            } else {
                /* if in_len is 0, uio.in is an integer single argument */
                kio->in     = uio->in;
                kio->in_len = 0;
            }
            // set the kio 'out' pointer
            kio->out     = (SYS_UINT32 *)( dccp->clnk_ctl_out );
            kio->out_len = uio->out_len;
            // Note that some IOCTLs require out_len non-zero - we don't check for that.
        }
    }

    return( error ) ;
}

/**
 *  Purpose:    Reply back to the IOCTL caller by copying the io block
 *              and data, if any, back to user space.
 *
 *              This is called at the end of the IOCTL operation
 *
 *  Imports:    dkcp     - driver kernel context pointer
 *              uio      - user io block pointer, in kernel space
 *              kio      - kernel io block pointer
 *              us_ioblk - user space pointer to user io block
 *              odata    - out data, in kernel space
 *
 *  Exports:    Error status
 *
*STATIC***************************************************************************/
static SYS_INT32 clnkioc_io_block_return( void *dkcp, struct clnk_io *uio, 
                                          struct clnk_io *kio, SYS_UINTPTR us_ioblk,
                                          void *odata )
{
    SYS_INT32    error = SYS_SUCCESS;

    /* update out length if shorter now */
    if( kio->out_len < uio->out_len ) {

        uio->out_len = kio->out_len;     // set new length

        // copy length to user space - along with the rest of the io block
        if( HostOS_copy_to_user( (void *)us_ioblk, (void *)uio, sizeof(struct clnk_io)) )
        {
            _ioctl_dbg( dkcp, "DRV_CLNK_CTL copy _to_user (uio)");
            error = -SYS_INVALID_ADDRESS_ERROR;
        }
    }

    if( uio->out_len &&       // if supposed to return data
        HostOS_copy_to_user( uio->out, odata, uio->out_len) ) {
        _ioctl_dbg( dkcp, "DRV_CLNK_CTL copy _to_user (out data)");
        error = -SYS_INVALID_ADDRESS_ERROR;
    }

    return( error );
}

