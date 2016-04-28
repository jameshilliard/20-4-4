/*******************************************************************************
*
* Common/Src/ClnkIo_common.c
*
* Description: ioctl common functions
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




/**
* 
*   Purpose:    Sends and receives a command message.
*
*   Imports:    dccp    - driver control context pointer
*               rq_cmd  - request block command
*               rq_scmd - request block subcommand
*               iob     - io block pointer
*                         Note: If io->in_len is 0, io->in is treated as a 
*                               single argument
*
*   Exports:    Error status
*
*PUBLIC***************************************************************************/
int clnk_cmd_msg_send_recv( dc_context_t *dccp,    // control context
                            SYS_UINT32 rq_cmd,     // request command
                            SYS_UINT32 rq_scmd,    // subcommand
                            struct clnk_io *iob )  // io block pointer
{
    int             error ;
    Clnk_MBX_Msg_t  mbxMsg;
    int             len;
    SYS_INT32       out_len ;

    /* create data_buf mailbox request */
    mbxMsg.msg.ethCmd.cmd      = CLNK_MBX_SET_CMD(rq_cmd);          /* wd 0 */
    mbxMsg.msg.ethCmd.param[0] = rq_scmd & ~CLNK_CMD_DST_MASK;      /* wd 1 */
    mbxMsg.msg.ethCmd.param[1] = 0;                                 /* wd 2 */

    if( iob->in_len )
    {
        /* send a blob of data down to the SoC */
        int i;
        
        len = MIN_VAL(MAX_MBX_MSG - 3, iob->in_len >> 2);  // get number of longs
        for(i = 0; i < len; i++) {
            mbxMsg.msg.ethCmd.param[2 + i] = iob->in[i];            /* wd 3+ */
        }
    } else {
        mbxMsg.msg.ethCmd.param[2] = (SYS_UINTPTR)iob->in;          /* wd 3 */
        len = 1;
    }

#if ECFG_FLAVOR_VALIDATION==1
    if (cmd == CLNK_CTL_VAL_TRIGGER_MBOX_EVENT)
    {
        clnkEth_valMboxHostCounts.triggeredSendRcvs++;
    }
#endif

    // send message - wait for and receive response

    error = Clnk_MBX_SendRcvMsg( &dccp->mailbox, &mbxMsg, &mbxMsg,
                                    3 + len, MBX_POLL_TIMEOUT_IN_US);
//HostOS_PrintLog(L_INFO, "Clnk_MBX_SendRcvMsg, subcmd = %x, error = %d\n", rq_scmd, error);
    /*
    * status is the status of the mailbox transaction itself
    *
    * CLNK_MBX_GET_STATUS() reports the replyStatus from
    *   ETH_ProcDataBufCmd() handler on the SoC (if the mailbox
    *   transaction didn't fail)
    */
    if( !error &&
        (CLNK_MBX_GET_STATUS(mbxMsg.msg.ethReply.status) == SYS_SUCCESS))
    {
        // extract out length
        out_len = (CLNK_MBX_GET_REPLY_LEN(mbxMsg.msg.ethReply.status) << 2) - 4;

        // adjust out length for reality
        iob->out_len = MIN_VAL(iob->out_len, out_len);
        if( iob->out_len )
        {
            // copy response to 'out' buffer
            HostOS_Memcpy( iob->out, &mbxMsg.msg.ethReply.param[0], out_len);
        }
    }

    return( error );
}


/*
*   Purpose:    Reads a block of register address space into a buffer.
*
*               The block is read as longs.
*
*               Takes the Address Translation lock
*
*   Imports:    dccp            - control context
*               sourceClinkAddr - beginning register address
*               destHostAddr    - beginning memory buffer address
*               length          - byte count, minimum is 4 and must be modulo 4
*
*   Exports:    none
*
*PUBLIC*****************************************************************/
void clnk_blk_read( dc_context_t *dccp, 
                    SYS_UINT32 sourceClinkAddr, 
                    SYS_UINT32 *destHostAddr, 
                    SYS_UINT32 length)
{

#if defined(E1000_DRVR_SUPPORT) || defined(CANDD_DRVR_SUPPORT) || defined(CONFIG_TIVO)
    HostOS_Lock(dccp->at_lock_link);

    /*
     * FPGA AUTO_INCR is pre-increment
     * SoC AUTO_INCR is post-increment, here is SOC
     */
    clnk_read_burst( dccp, sourceClinkAddr, destHostAddr, length, 1); //1 for SOC, 0 for FPGA

    HostOS_Unlock(dccp->at_lock_link);
#else

    for( ; length ; length -= 4, sourceClinkAddr += 4, destHostAddr++ ) {
        clnk_reg_read(dccp, sourceClinkAddr, destHostAddr);
    }

#endif

}

/*
*   Purpose:    Writes a buffer to a block of register address space.
*
*               The block is written as longs.
*
*               Takes the Address Translation lock
*
*   Imports:    dccp           - control context
*               destClnkAddr   - beginning register address
*               sourceHostAddr - beginning memory buffer address
*               length         - byte count, minimum is 4 and must be modulo 4
*
*   Exports:    none
*
*PUBLIC*****************************************************************/
void clnk_blk_write(dc_context_t  *dccp, 
                    SYS_UINT32 destClnkAddr, 
                    SYS_UINT32 *sourceHostAddr, 
                    SYS_UINT32 length)
{

#if defined(E1000_DRVR_SUPPORT) || defined(CANDD_DRVR_SUPPORT) || defined(CONFIG_TIVO)
    HostOS_Lock(dccp->at_lock_link);

    /*
     * FPGA AUTO_INCR is pre-increment
     * SoC AUTO_INCR is post-increment, here is SOC
     */
    clnk_write_burst( dccp, destClnkAddr, sourceHostAddr, length, 1); //for SOC

    HostOS_Unlock(dccp->at_lock_link);
#else

    for( ; length ; length -= 4, destClnkAddr += 4, sourceHostAddr++ ) {
        clnk_reg_write(dccp, destClnkAddr, *sourceHostAddr);
    }
#endif
}

/**
*   Purpose:    Sends command message 
*
*   Imports:    dccp    - driver control context pointer
*               rq_cmd  - request block command
*               rq_scmd - request block subcommand
*               iob     - io block pointer
* 
*   Exports:    Error status
*
*PUBLIC***************************************************************************/
int clnk_cmd_msg_send( dc_context_t *dccp,    // control context
                       SYS_UINT32 rq_cmd,     // request command
                       SYS_UINT32 rq_scmd,    // subcommand
                       struct clnk_io *iob )  // io block pointer
{
    SYS_INT32           error = SYS_SUCCESS;
    Clnk_MBX_Msg_t      mbxMsg;
    int                 len, i;
    SYS_UINT8           tid ;

    /* create data_buf mailbox request */
    mbxMsg.msg.ethCmd.cmd      = CLNK_MBX_SET_CMD(rq_cmd);                  /* wd 0 */
    mbxMsg.msg.ethCmd.param[0] = rq_scmd & CLNK_CMD_BYTE_MASK;              /* wd 1 */             
    mbxMsg.msg.ethCmd.param[1] = 0;                                         /* wd 2 */

    if( iob->in_len )   // buffer to send
    {
        len = MIN_VAL(MAX_MBX_MSG - 3, iob->in_len >> 2);
        for( i = 0 ; i < len ; i++ ) {
            mbxMsg.msg.ethCmd.param[2 + i] = iob->in[i];                    /* wd 3+ */
        }
    } else {            // single long to send
        mbxMsg.msg.ethCmd.param[2] = (SYS_UINTPTR)iob->in ;                 /* wd 3 */
        len = 1;        // len is in longs
    }

    error = Clnk_MBX_SendMsg( &dccp->mailbox, &mbxMsg, &tid, 3 + len );

    return( error );
}

