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
#ifdef CONFIG_TIVO
#include <linux/string.h>
#else
//#include <linux/string.h>
extern int strncmp(char *, char *, int);
extern int strlen(char *);
#endif
/*******************************************************************************
*                             # D e f i n e s                                  *
********************************************************************************/





/*******************************************************************************
*            S t a t i c   M e t h o d   P r o t o t y p e s                   *
********************************************************************************/

static SYS_INT32 clnkioc_driver_cmd_work( void *dkcp, IfrDataStruct *kifr );
#ifdef CONFIG_TIVO
/* For some reason these were disabled in their drop. */
static SYS_INT32 clnkioc_mbox_cmd_request_work( void *dkcp, IfrDataStruct *kifr, int response );
static SYS_INT32 clnkioc_mbox_unsolq_retrieve_work( void *dkcp, IfrDataStruct *kifr );
#else
//static SYS_INT32 clnkioc_mbox_cmd_request_work( void *dkcp, IfrDataStruct *kifr, int response );
//static SYS_INT32 clnkioc_mbox_unsolq_retrieve_work( void *dkcp, IfrDataStruct *kifr );
#endif
static SYS_INT32 clnkioc_mem_read_work( void *dkcp, IfrDataStruct *kifr );
static SYS_INT32 clnkioc_mem_write_work( void *dkcp, IfrDataStruct *kifr );
static SYS_INT32 clnkioc_copy_request_block( void *arg, IfrDataStruct *kifr );
static SYS_INT32 clnkioc_io_block_setup( dc_context_t *dccp, IfrDataStruct *kifr,
        struct clnk_io *uio, struct clnk_io *kio );
static SYS_INT32 clnkioc_io_block_return( void *dkcp, struct clnk_io *uio,
        struct clnk_io *kio, SYS_UINTPTR us_ioblk,
        void *odata );
static SYS_INT32 clnkioc_moca_shell_io_block_setup( dc_context_t *dccp, IfrDataStruct *kifr,
        struct clnk_io *uio, struct clnk_io *kio );
static SYS_INT32 clnkioc_moca_shell_io_block_return( void *dkcp, struct clnk_io *uio,
        struct clnk_io *kio, SYS_UINTPTR us_ioblk,
        void *odata );
static SYS_INT32 clnkioc_moca_shell_cmd_work( void *dkcp, IfrDataStruct *kifr );


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

#if 1
/* XXXX CONFIG_TIVO -- This section was disabled in the balboa drop*/

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
#endif


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

unsigned long clnkioc_moca_shell_cmd( void *dkcp, void *arg )
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
            error = clnkioc_moca_shell_cmd_work( dkcp, &kifr ) ;

            HostOS_mutex_release( dccp->ioctl_sem_link );
        }
        //HostOS_Unlock(dccp->ioctl_lock_link);

    }

    return( error );
}

SYS_UINT32 htonl(SYS_UINT32 x)
{
    SYS_UINT32 y = x;
    char *p = (char *)&y;

    return ((p[0]) << 24 | (p[1]) << 16 | (p[2]) << 8 | (p[3]));
}

static SYS_INT32 clnkioc_moca_shell_cmd_work( void *dkcp, IfrDataStruct *kifr )
{
#define MOCA_SHELL_MIN_CMD_LEN 4
    int i;
    dc_context_t *dccp = dk_to_dc( dkcp ) ;
    SYS_UINTPTR  param1, param2, param3;
    SYS_INT32    error = SYS_SUCCESS;
    struct clnk_io uio, kio;
    SYS_UINT32   shift, addr, *data, len, owner_flag, session, cnt;
    SYS_UINT8    *wp;
//    SYS_UINT32   *pout;

    param1  = (SYS_UINTPTR)kifr->param1;        // ioctl command
    param2  = (SYS_UINTPTR)kifr->param2;        // ioctl io block
    param3  = (SYS_UINTPTR)kifr->param3;        // version constant

    // copy the user's io block to kernel space
    error = clnkioc_moca_shell_io_block_setup( dccp, kifr, &uio, &kio ) ;

    if( !error ) {

        clnk_reg_read(dccp, DEV_SHARED(ms_cmd_buf_addr), &addr);
        data = &kio.in[0];
        len  = kio.in_len;
        if( len < MOCA_SHELL_MIN_CMD_LEN )
        {
            _ioctl_dbg( dkcp, "DRV_CLNK_CTL min write");
            error = -SYS_INVALID_ARGUMENT_ERROR;
        } else {

            // WRITE MoCA Shell command to SoC
            //HostOS_PrintLog(L_ERR, "request to write command: %s\n", data);
            clnk_reg_write(dccp, DEV_SHARED(ms_session_req), MS_SESSION_REQ_MDIO);
            cnt = 0;
            while (1)
            {
                clnk_reg_read(dccp, DEV_SHARED(ms_session_req), &session);
                if (MS_SESSION_RESP_MDIO == session)
                {
                    break;
                }

                if (cnt++ > 1000)
                {
#if 0
                    strcpy(dccp->clnk_ctl_out, "\r\nerror\r\n");
                    uio.out_len = strlen(dccp->clnk_ctl_out) + 1;
                    goto out;
#else
                    return -SYS_TIMEOUT_ERROR;
#endif
                }

                HostOS_msleep_interruptible(10);
            }

            //HostOS_PrintLog(L_ERR, "start to write command\n");
            addr &= 0x7fffffff;

            len = (len + 3) & ~3;
            for (i = 0; i < len/4; i++)
            {
                data[i] = htonl(data[i]);
            }
            //HostOS_PrintLog(L_ERR, "write command: %s\n", (char *)data);
            clnk_blk_write(dccp, addr, (SYS_UINT32 *)data, len);
            clnk_reg_write(dccp, DEV_SHARED(ms_session_req), MS_SESSION_RESP_ACK_MDIO);


            // READ MoCA Shell result from SoC
            //HostOS_PrintLog(L_ERR, "wait for response\n");
            uio.out_len = 0;
            wp = (SYS_UINT8 *)dccp->clnk_ctl_out;
            cnt = 0;
            while (1)
            {
                clnk_reg_read( dccp, DEV_SHARED(ms_resp_owner_flag), &owner_flag);
                if (OWNER_HOST == MOCA_SHELL_RESP_OWNER(owner_flag))
                {
                    cnt = 0;
                    len   = MOCA_SHELL_RESP_LEN(owner_flag);
                    shift = len & 0x3;
                    len   = (len + 3) & ~3;

                    clnk_reg_read( dccp, DEV_SHARED(ms_resp_buf_addr), &addr);
                    addr &= 0x7fffffff;

                    clnk_blk_read( dccp, addr, (SYS_UINT32 *)wp, len ) ;
                    //HostOS_PrintLog(L_ERR, "len = %d, addr = %#x, clnk_blk_read: %s\n", owner_flag >> 16 , addr, wp);

                    wp          += len;
                    uio.out_len += len;
                    if (shift)
                    {
                        wp          += shift;
                        wp          -= 4;
                        uio.out_len += shift;
                        uio.out_len -= 4;
                    }

                    clnk_reg_write( dccp, DEV_SHARED(ms_resp_owner_flag), OWNER_SOC);

                    if (!MOCA_SHELL_RESP_MORE(owner_flag))  // no more data, MoCA shell send error code leave this field 0
                        break;
                }

                if (cnt++ > 1000)
                {
#if 0
                    strcpy(dccp->clnk_ctl_out, "\r\nerror\r\n");
                    uio.out_len = strlen(dccp->clnk_ctl_out) + 1;
                    break;
#else
                    return -SYS_TIMEOUT_ERROR;
#endif
                }

                HostOS_msleep_interruptible(10);
            }

#if 0
            uio.out_len = (uio.out_len + 3) & ~3;
            pout = (SYS_UINT32 *)uio.out;
            for (i = 0; i < uio.out_len/4; i++)
            {
                pout[i] = htonl(pout[i]);
            }
#endif
            /* zhu add 11/03/2011/ for BZ11168:the driver version is always 0.0.0.0 
            0x10009 is the virtual address of MY_NODE_INFO_VADDR
            */
            if(strncmp((char *)uio.in, "read 0x10009", strlen("read 0x10009")) == 0)
            {
                ClnkDef_MyNodeInfo_t *out = (void *)kio.out;
                SYS_UINT32  rev1=0, rev2=0, rev3=0, rev4=0;
                    
                HostOS_Sscanf(DRV_VERSION, "%d.%d.%d.%d", 
                              &rev1, &rev2, &rev3, &rev4);
                out->SwRevNum = ((rev1 & 0xff) << 24) | 
                                ((rev2 & 0xff) << 16) |
                                ((rev3 & 0xff) << 8)  | 
                                ((rev4 & 0xff) );
            }
//out:
            // copy to user space
            error = clnkioc_moca_shell_io_block_return( dkcp, &uio, &kio, param2, dccp->clnk_ctl_out ) ;
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

static SYS_INT32 clnkioc_moca_shell_io_block_setup( dc_context_t *dccp, IfrDataStruct *kifr,
        struct clnk_io *uio, struct clnk_io *kio )
{
    SYS_INT32    error = SYS_SUCCESS;
    SYS_UINT32   in_len;
    SYS_UINTPTR  param2;

    param2  = (SYS_UINTPTR)kifr->param2;        // ioctl io block

    if( HostOS_copy_from_user( uio, (void *)param2, sizeof(struct clnk_io)) ) {
        _ioctl_dbg( dkcp, "DRV_CLNK_CTL copy _from_user (uio)");
        error = -SYS_INVALID_ADDRESS_ERROR;
    } else {

        //HostOS_PrintLog(L_ERR, "uio->in_len = %d, uio->in: %s\n", uio->in_len, uio->in);
        uio->in_len  = (uio->in_len + 3) & ~3;
        uio->out_len = (uio->out_len + 3) & ~3;
        in_len = uio->in_len;

        if( (in_len > CLNK_CTL_MAX_IN_LEN) ||
                (uio->out_len > CLNK_CTL_MAX_OUT_LEN) ) {
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
                /* if in_len is 0, empty moca shell commands? */
                _ioctl_dbg( dkcp, "DRV_CLNK_CTL empty moca shell command (data in)");
                error = -SYS_INVALID_ARGUMENT_ERROR;
            }
            // set the kio 'out' pointer
            kio->out     = (SYS_UINT32 *)( dccp->clnk_ctl_out );
            kio->out_len = uio->out_len;
            // Note that some IOCTLs require out_len non-zero - we don't check for that.
        }
    }

    return error;
}

static SYS_INT32 clnkioc_moca_shell_io_block_return( void *dkcp, struct clnk_io *uio,
        struct clnk_io *kio, SYS_UINTPTR us_ioblk,
        void *odata )
{
    SYS_INT32    error = SYS_SUCCESS;

    if( HostOS_copy_to_user( (void *)us_ioblk, (void *)uio, sizeof(struct clnk_io)) )
    {
        _ioctl_dbg( dkcp, "DRV_CLNK_CTL copy _to_user (uio)");
        error = -SYS_INVALID_ADDRESS_ERROR;
    }

    if(HostOS_copy_to_user( uio->out, odata, uio->out_len) ) {
        _ioctl_dbg( dkcp, "DRV_CLNK_CTL copy _to_user (out data)");
        error = -SYS_INVALID_ADDRESS_ERROR;
    }

    return error;
}

