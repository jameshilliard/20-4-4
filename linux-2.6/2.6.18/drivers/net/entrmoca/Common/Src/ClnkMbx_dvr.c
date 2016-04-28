/*******************************************************************************
*
* Common/Src/ClnkMbx_dvr.c
*
* Description: Common Mailbox Module
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


// Mailbox version number
#define VER_NUM                  0x00

// Queue Constants
#define CMD_QUEUE_MASK           (CLNK_MBX_CMD_QUEUE_SIZE-1)
#define SWUNSOL_QUEUE_MASK       (CLNK_MBX_SWUNSOL_QUEUE_SIZE-1)

// Transaction ID Constants
#define HOST_ORIG_BIT            0x80UL
#define UNSOL_TYPE_BIT           0x40UL

#if ECFG_CHIP_ZIP1
  #define IS_READ_INTERRUPT(y)     ((y) & \
          (CLNK_REG_PCI_READ_MBX_SEM_SET_BIT | CLNK_REG_PCI_SW_INT_1_BIT))
  #define IS_WRITE_INTERRUPT(y)    ((y) & (CLNK_REG_PCI_WRITE_MBX_SEM_CLR_BIT))
  #define IS_SW_UNSOL_INTERRUPT(x) ((x) & CLNK_REG_PCI_SW_INT_1_BIT)
  #define ACK_SW_UNSOL_INTERRUPT(x) \
          { clnk_reg_write((x)->dc_ctx, CLNK_REG_SLAVE_1_MAP_ADDR, (x)->swUnsolQueueSlaveMap); }
#else
  #define IS_READ_INTERRUPT(y)     1
  #define IS_WRITE_INTERRUPT(y)    1
  #define IS_SW_UNSOL_INTERRUPT(x) 1
  #define ACK_SW_UNSOL_INTERRUPT(x) {}
#endif // ECFG_CHIP_ZIP1

#define UMSG_ELEMENT(x, y)      ((x) + offsetof(Clnk_MBX_SwUnsolQueueEntry_t, y))


/*******************************************************************************
*                             D a t a   T y p e s                              *
********************************************************************************/

/*******************************************************************************
*                             C o n s t a n t s                                *
********************************************************************************/

/*******************************************************************************
*                             G l o b a l   D a t a                            *
********************************************************************************/

/*******************************************************************************
*                       M e t h o d   P r o t o t y p e s                      *
********************************************************************************/

static void Clnk_MBX_Alloc_wqts( Clnk_MBX_Mailbox_t *pMbx );
static int Clnk_MBX_wqt_condition( void *vmp );
static SYS_VOID Clnk_MBX_msg_timer(SYS_ULONG data);
static int Clnk_MBX_RcvMsg(Clnk_MBX_Mailbox_t *pMbx, 
                           Clnk_MBX_Msg_t     *pMsg,
                           SYS_UINT8          transID);
static int Clnk_MBX_CheckReplyRdy(Clnk_MBX_Mailbox_t* pMbx, SYS_UINT8 transID);
static void Clnk_MBX_Read_Hw_Mailbox_Clear_Ready(Clnk_MBX_Mailbox_t* pMbx);
static void Clnk_MBX_Write_Hw_Mailbox_Set_Ready(Clnk_MBX_Mailbox_t* pMbx);
static void Clnk_MBX_Read_Hw_Mailbox(Clnk_MBX_Mailbox_t* pMbx, void* pvMsg);
static void Clnk_MBX_Write_Hw_Mailbox(Clnk_MBX_Mailbox_t* pMbx, void* pvMsg);

#ifdef CLNK_MBX_AUTO_REPLY
static void AutoReplyMsg(Clnk_MBX_Mailbox_t* pMbx);
#endif

/*******************************************************************************
*                      M e t h o d   D e f i n i t i o n s                     *
********************************************************************************/


/*******************************************************************************
*
* Description:
*       Initializes the Mailbox Module.
*
* Inputs:
*       pMbx  - Pointer to the mailbox
*       dcctx - pointer to the control context
*       type  - Type of mailbox (Ethernet or MPEG)
*
* Outputs:
*       SYS_SUCCESS or a nonzero error code
*
* Notes:
*       It is assumed that sufficient time will elapse between the
*       RST_STATE_INIT_CCPU and RST_STATE_INIT_MBX states of the
*       reset state machine to allow the CCPU to initialize the
*       mailbox registers before checking them in this routine
*
*******************************************************************************/
int Clnk_MBX_Initialize(Clnk_MBX_Mailbox_t *pMbx, void *dcctx, SYS_UINT32 type)
{
    SYS_UINT32 reg;
    dc_context_t *dccp = (dc_context_t *)dcctx ;

    //HostOS_PrintLog(L_ERR, "Clnk_MBX_Initialize\n" );
    // Free any wqts
    Clnk_MBX_Free_wqts( pMbx ) ;

    // Initialize mailbox block
    HostOS_Memset(pMbx, 0, sizeof(Clnk_MBX_Mailbox_t));

    pMbx->dc_ctx             = dccp;
    pMbx->type               = type;
    pMbx->cmdCurrTransID     = HOST_ORIG_BIT;

    // Initialize mailbox locks
    pMbx->mbx_lock          = dccp->mbx_cmd_lock_link ; 
    pMbx->swUnsolLock       = dccp->mbx_swun_lock_link ;

    // Initialize register offsets
    if (pMbx->type == CLNK_MBX_ETHERNET_TYPE)
    {
        pMbx->writeMbxCsrOffset  = CLNK_REG_MBX_WRITE_CSR;
        pMbx->readMbxCsrOffset   = CLNK_REG_MBX_READ_CSR;
        pMbx->writeMbxRegOffset  = CLNK_REG_MBX_REG_1;
        pMbx->readMbxRegOffset   = CLNK_REG_MBX_REG_9;
        pMbx->readMbxSize        = MAX_MBX_MSG;
        pMbx->writeMbxSize       = MAX_MBX_MSG;
    }
#if 0
    HostOS_PrintLog(L_INFO, "Initialize register offsets\n");
    HostOS_PrintLog(L_INFO, "unsol  = %x\n",pMbx->pSwUnsolQueue);
    HostOS_PrintLog(L_INFO, "writeMbxCsrOffset = %x\n",CLNK_REG_MBX_WRITE_CSR);
    HostOS_PrintLog(L_INFO, "readMbxCsrOffset = %x\n",CLNK_REG_MBX_READ_CSR);
    HostOS_PrintLog(L_INFO, "writeMbxRegOffset = %x\n",CLNK_REG_MBX_REG_1);
    HostOS_PrintLog(L_INFO, "readMbxRegOffset = %x\n",CLNK_REG_MBX_REG_9);
    HostOS_PrintLog(L_INFO, "readMbxSize  = %x\n",MAX_MBX_MSG);
    HostOS_PrintLog(L_INFO, "writeMbxSize = %x\n",MAX_MBX_MSG);
#endif
    // allocate all wqts, setup timers and wait queues
    Clnk_MBX_Alloc_wqts( pMbx ) ;

    // Confirm that the CCPU has initialized the mailbox registers
    // Register was cleared by host driver before CCPU was released,
    clnk_reg_read(pMbx->dc_ctx, CLNK_REG_MBX_REG_1, &reg);
//    if (0 == reg) 
//        return (~SYS_SUCCESS);
    
    pMbx->mbxOpen = SYS_TRUE;
    
    return (SYS_SUCCESS);
}

/*******************************************************************************
*
* Description:
*       Frees all Mailbox message wqts.
*
* Inputs:
*       pMbx            - Pointer to the mailbox
*
* Outputs:
*
*******************************************************************************/
void Clnk_MBX_Free_wqts( Clnk_MBX_Mailbox_t *pMbx )
{
    int                         msg ;
    Clnk_MBX_CmdQueueEntry_t    *mp ;

    // for all messages
    for( msg = 0 ; msg < CLNK_MBX_CMD_QUEUE_SIZE ; msg++ )
    {
        mp = &pMbx->cmdQueue[msg] ;
        if( mp->msg_wqt )
        {
            HostOS_wqt_timer_del( mp->msg_wqt ) ;

            HostOS_wqt_free( mp->msg_wqt );
            mp->msg_wqt = 0 ;
        }
    }
}

/*******************************************************************************
*
* Description:
*       Terminates the Common Mailbox Module.
*
* Inputs:
*       Clnk_MBX_Mailbox_t*  Pointer to the mailbox
*
* Outputs:
*       None
*
*******************************************************************************/
void Clnk_MBX_Terminate(Clnk_MBX_Mailbox_t* pMbx)
{
    // Terminate mailbox lock
    HostOS_TermLock(pMbx->mbx_lock);
}

/*******************************************************************************
*
* Description:
*       Controls the operation of the Common Mailbox Module.
*
* Inputs:
*       Clnk_MBX_Mailbox_t*  Pointer to the mailbox
*       int                  Driver option to control
*       SYS_UINT32           Register offset or pointer to register offsets
*       SYS_UINT32           Register value or pointer to register values
*       SYS_UINT32           Length of data for bulk data
*
* Outputs:
*       SYS_SUCCESS or a nonzero error code
*
* Notes:
*       Driver option                     Parameters
*       -------------                     ----------
*       CLNK_MBX_CTRL_ENABLE_INTERRUPT    reg    - Not used
*                                         val    - Not used
*                                         length - Not used
*       CLNK_MBX_CTRL_DISABLE_INTERRUPT   reg    - Not used
*                                         val    - Not used
*                                         length - Not used
*       CLNK_MBX_CTRL_CLEAR_INTERRUPT     reg    - Not used
*                                         val    - Not used
*                                         length - Not used
*       CLNK_MBX_CTRL_SET_SW_UNSOL_Q      reg    - Not used
*                                         val    - Pointer to emb mem
*                                         length - Number of entries
*       CLNK_MBX_CTRL_SET_REPLY_RDY_CB    reg    - Not used
*                                         val    - Function pointer
*                                         length - Function parameter
*       CLNK_MBX_CTRL_SET_UNSOL_RDY_CB    reg    - Not used
*                                         val    - Function pointer
*                                         length - Function parameter
*       CLNK_MBX_CTRL_SET_SW_UNSOL_RDY_CB reg    - Not used
*                                         val    - Function pointer
*                                         length - Function parameter
*
*
*******************************************************************************/
int Clnk_MBX_Control(Clnk_MBX_Mailbox_t* pMbx, 
                     int option, 
                     SYS_UINTPTR reg,
                     SYS_UINTPTR val, 
                     SYS_UINTPTR length)
{
    int status = SYS_SUCCESS;

    switch (option)
    {
        case CLNK_MBX_CTRL_SET_SW_UNSOL_Q:
            // Initialize the software unsolicited queue
        #if ECFG_CHIP_ZIP1
            pMbx->swUnsolQueueSlaveMap = (val & 0x0ffc0000) |
                                         CLNK_REG_ADDR_TRANS_ENABLE_BIT;
            pMbx->pSwUnsolQueue        =  val & 0x0003ffff;
        #else
            pMbx->pSwUnsolQueue        = val;
        #endif 
            pMbx->swUnsolQueueSize     = length;
            pMbx->swUnsolReadIndex     = 0;
            break;

        case CLNK_MBX_CTRL_SET_REPLY_RDY_CB:
            // Set the reply ready callback function   MbxReplyRdyCallback
            pMbx->replyRdyCallback = (Clnk_MBX_RdyCallback)val;
            pMbx->replyParam       = (void *)length;
            break;

        case CLNK_MBX_CTRL_SET_SW_UNSOL_RDY_CB:
            // Set the unsolicited ready callback function   MbxSwUnsolRdyCallback
            pMbx->swUnsolRdyCallback = (Clnk_MBX_SwUnsolRdyCallback)val;
            pMbx->swUnsolParam       = (void *)length; // context at this point
            break;

        default:
            status = -SYS_BAD_MSG_TYPE_ERROR;
            break;
    }
    return (status);
}

/*******************************************************************************
*
* Description:
*       Handles the interrupt for the Common Mailbox Module.
*
* Inputs:
*       Clnk_MBX_Mailbox_t*  Pointer to the mailbox
*       SYS_UINT32           PCI interrupt status register
*
* Outputs:
*       None
*
* Notes:
*       None
*
*
*******************************************************************************/
void Clnk_MBX_HandleInterrupt(Clnk_MBX_Mailbox_t* pMbx, SYS_UINT32 pciIntStatus)
{

    // Check if hardware mailbox has command response
    if (IS_READ_INTERRUPT(pciIntStatus))
    {
        Clnk_MBX_Read_ISR(pMbx);
    }

    // Check if hardware mailbox has unsolicited msg
    if ( IS_SW_UNSOL_INTERRUPT(pciIntStatus) )
    {
        Clnk_MBX_Unsol_ISR(pMbx);
    }

    // Check if hardware mailbox can take a command
    if (IS_WRITE_INTERRUPT(pciIntStatus))
    {
        Clnk_MBX_Write_ISR(pMbx);
    }
}

/*******************************************************************************
*
* Description:
*           Sends a mailbox message.
*
* Inputs:   pMbx     - Pointer to the mailbox
*           pMsg     - Pointer to the message to send
*           pTransID - Pointer to place to return the transaction ID
*           len      - length of message in longs
*
* Exports:  0           - success, message queued/sent
*           -SYS_INVALID_ADDRESS_ERROR     - mailbox not open
*           -SYS_INVALID_ARGUMENT_ERROR    - invalid request, length
*           -SYS_OUT_OF_SPACE_ERROR        - queue full, message dropped
*
*******************************************************************************/
int Clnk_MBX_SendMsg(Clnk_MBX_Mailbox_t *pMbx, 
                     Clnk_MBX_Msg_t     *pMsg,
                     SYS_UINT8          *pTransID, 
                     SYS_UINT32          len)
{
    SYS_UINT32 index;

    if (!pMbx->mbxOpen) 
        return(-SYS_INVALID_ADDRESS_ERROR);

    if((len < 1) || (len > MAX_MBX_MSG))
    {
        HostOS_PrintLog(L_ERR, "warning: invalid mbx request: %d words\n", len);
        return(-SYS_INVALID_ARGUMENT_ERROR);
    }

    // Lock the mailbox
    HostOS_Lock(pMbx->mbx_lock);

    /*  
        Check if mailbox is (too) full.  
        Note that we always leave two entries empty.
        This is the only place Send messages are queued so it's safe to say
        that this is the only place where tail may attempt to overrun head.
    */
    if (((pMbx->cmdTailIndex+3) & CMD_QUEUE_MASK) == pMbx->cmdHeadIndex)
    {
        // mailbox full - log queuing error
        HostOS_Unlock(pMbx->mbx_lock);
        return (-SYS_OUT_OF_SPACE_ERROR);
    }

    // Save the transaction ID for the command
    *pTransID = pMbx->cmdCurrTransID;    // push the transID back to the caller
    index = pMbx->cmdTailIndex;
    pMbx->cmdQueue[index].transID    = *pTransID;
    pMbx->cmdQueue[index].isReplyRdy = SYS_FALSE;
    pMsg->msg.cmdCode |=                                        // byte 0 = cmd
                         CLNK_MBX_SET_TRANS_ID(*pTransID) |     // byte 1
                         CLNK_MBX_SET_VER_NUM(VER_NUM)    |     // byte 2 lo nybl
                         CLNK_MBX_SET_PORT(pMbx->type)    |     // byte 2 hi nybl
                         CLNK_MBX_SET_LEN(len);                 // byte 3  (num longs)

    // Copy message to into mail box buffer
    HostOS_Memcpy(&pMbx->cmdQueue[index].sendMsg, pMsg, len * sizeof(SYS_UINT32));

    // Check if Q empty and hardware mailbox is in use
    if ((pMbx->cmdTailIndex == pMbx->cmdHeadIndex) &&
        Clnk_MBX_Write_Hw_Mailbox_Check_Ready(pMbx))
    {
        // Write message to hardware mailbox
        Clnk_MBX_Write_Hw_Mailbox(pMbx, pMsg);

        // Effectively Q the msg. There might be a response
        // Since the Q is empty move the head along, too, so head will stay at or ahead of tail
        pMbx->cmdHeadIndex = (pMbx->cmdHeadIndex+1) & CMD_QUEUE_MASK;
        pMbx->cmdTailIndex = (pMbx->cmdTailIndex+1) & CMD_QUEUE_MASK;
    }
    else
    {
        //HostOS_PrintLog(L_NOTICE, "SendMsg to Q  tid=%02xx\n", *pTransID);

        // Add entry to tail of command queue
        pMbx->cmdTailIndex = (pMbx->cmdTailIndex+1) & CMD_QUEUE_MASK;
    }

    // Generate new transaction ID (make sure host originated bit is set)
    pMbx->cmdCurrTransID++;
    pMbx->cmdCurrTransID |= HOST_ORIG_BIT;

    // Unlock the mailbox
    HostOS_Unlock(pMbx->mbx_lock);

#ifdef CLNK_MBX_AUTO_REPLY
    // Automatically generate reply
    AutoReplyMsg(pMbx);
#endif
    return (SYS_SUCCESS);
}

/*******************************************************************************
*
* Purpose:  Receives a sw unsolicited mailbox message.
*           This reads messages that have not been internally
*           consumed from the sw unsolicited Q.
*           There may not be any such messages.
*           There may have been a Q overrun.
*
* Imports:  pMbx    - Pointer to the mailbox
*           pbuf    - Pointer to a kernel buffer to get the message
*
* Exports:  0           - success, no more messages
*           -SYS_DIR_NOT_EMPTY_ERROR  - success, more messages
*           -SYS_BAD_MSG_TYPE_ERROR   - no message, try again later
*           -SYS_INPUT_OUTPUT_ERROR   - queue overrun, oldest message returned
*                                       and the overrun error is cleared
*           -SYS_INVALID_ADDRESS_ERROR- mailbox not open
*                                       sw unsolicited Q not setup
*                                       other
*
*******************************************************************************/
int Clnk_MBX_RcvUnsolMsg(Clnk_MBX_Mailbox_t *pMbx, 
                         SYS_UINT32         *pbuf)
{                   
    SYS_UINT32      index;
    int             error = 0 ;

    if( !pMbx->mbxOpen)
    {
        error = -SYS_INVALID_ADDRESS_ERROR ;
    }

    // Check if software unsolicited queue has been created
    if( !error && pMbx->pSwUnsolQueue == 0)
    {
        error = -SYS_INVALID_ADDRESS_ERROR ;
    }

    if( !error )
    {
        if( pMbx->swUnsolCount ) // we have message     
        {
            HostOS_Lock(pMbx->swUnsolLock);

            // Copy message to receive buffer
            index = pMbx->swUnsolHeadIndex;
            HostOS_Memcpy(pbuf, 
                          &pMbx->swUnsolQueue[index].msg.msg.maxMsg.msg, 
                          MAX_UNSOL_MSG * sizeof(SYS_UINT32));

            pMbx->swUnsolHeadIndex = (pMbx->swUnsolHeadIndex+1) & SWUNSOL_QUEUE_MASK;
            pMbx->swUnsolCount-- ;          // uncount an item taken

            if( pMbx->swUnsolCount ) // we have more
            {
                error = -SYS_DIR_NOT_EMPTY_ERROR ;
            }
            if( pMbx->swUnsolOverrun ) // we have overrun 
            {
                error = -SYS_INPUT_OUTPUT_ERROR ;
                pMbx->swUnsolOverrun = 0 ;   // and clear it
            }

            HostOS_Unlock(pMbx->swUnsolLock);
        }
        else
        {
            error = -SYS_BAD_MSG_TYPE_ERROR ;
        }
    }
    return( error );
}

/*******************************************************************************
*
* Description:
*       Sends and receives a mailbox message.
*
* Inputs:   pMbx        - Pointer to the mailbox
*           pSendMsg    - Pointer to the message to send
*           pRcvMsg     - Pointer to the buffer for receiving message
*           len         - length of send message
*           timeoutInUs - Time to wait for reply in ms
*
* Exports:  0           - success, message queued/sent
*           -SYS_INVALID_ADDRESS_ERROR - mailbox not open
*           -SYS_INPUT_OUTPUT_ERROR    - I/O error
*           -SYS_TIMEOUT_ERROR         - timeout waiting for response
*           ?           - other codes from called functions
*
*******************************************************************************/
int Clnk_MBX_SendRcvMsg(Clnk_MBX_Mailbox_t  *pMbx, 
                        Clnk_MBX_Msg_t      *pSendMsg,
                        Clnk_MBX_Msg_t      *pRcvMsg, 
                        SYS_UINT32          len,
                        SYS_UINT32          timeoutInUs)
{
    SYS_UINT8                   transID;
    int                         status;
    Clnk_MBX_CmdQueueEntry_t    *mp ;

    if (!pMbx->mbxOpen)
    {
        return (-SYS_INVALID_ADDRESS_ERROR);
    }

    // Send message    (get transaction ID)
    status = Clnk_MBX_SendMsg(pMbx, pSendMsg, &transID, len);
    //HostOS_PrintLog(L_INFO, "Clnk_MBX_SendRcvMsg send status=%d , transid=%x.\n", status,transID );
    if (status != SYS_SUCCESS)
    {
        HostOS_PrintLog(L_ERR, "Unable to send msg!!\n");
        return (status);
    }

    // setup to wait
    HostOS_Lock(pMbx->mbx_lock);

    if( !Clnk_MBX_CheckReplyRdy(pMbx, transID) )
    {
        // msg not ready - start wait
        mp = &pMbx->cmdQueue[transID & CMD_QUEUE_MASK] ;
        mp->isReplyTimedOut = 0 ;
        HostOS_wqt_timer_set_timeout( mp->msg_wqt, HostOS_timer_expire_seconds(2));
        HostOS_wqt_timer_add( mp->msg_wqt);

        HostOS_Unlock(pMbx->mbx_lock);

        HostOS_wqt_waitq_wait_event_intr( mp->msg_wqt,
                                          Clnk_MBX_wqt_condition,
                                          mp );

        // done waiting
        //HostOS_PrintLog(L_INFO, "We had to sleep!!\n");
        HostOS_wqt_timer_del( mp->msg_wqt );
    }
    else
    {
        // msg ready already
        //HostOS_PrintLog(L_INFO, "Msg read already - did not have to sleep\n");
        HostOS_Unlock(pMbx->mbx_lock);
    }

    // msg should be ready now
    if( Clnk_MBX_CheckReplyRdy(pMbx, transID) )
    {
        // Get the reply
        status = Clnk_MBX_RcvMsg(pMbx, pRcvMsg, transID);
    }
    else
    {
        status = -SYS_TIMEOUT_ERROR ;  // timeout - no message
    }

    return (status);
}


// define U_SHOW_TRAFFIC to see unsol traffic
#define xU_SHOW_TRAFFIC
// define Q_SHOW to see the unsol Q
#define xQ_SHOW
// define Q_RESYNC to try to resync the unsol Q for when the SOC overruns it
#define xQ_RESYNC

#ifdef Q_SHOW
extern int strcpy(char *, const char *) ;
extern int strcmp(const char *, const char *) ;
extern unsigned long volatile jiffies;
#endif

/*******************************************************************************
*
* Description:
*       Handles the read interrupt.
*
* Inputs:
*       pMbx         - Pointer to the mailbox
*
* Outputs:
*       None
*
*******************************************************************************/
void Clnk_MBX_Read_ISR( Clnk_MBX_Mailbox_t *pMbx )
{
    SYS_UINT32          offset, regVal;
    SYS_UINT8           transID, index ;

    if (Clnk_MBX_Read_Hw_Mailbox_Check_Ready(pMbx))
    {
        // Get the transaction ID
        clnk_reg_read(pMbx->dc_ctx, pMbx->readMbxRegOffset, &offset);
#if ECFG_CHIP_ZIP1
        offset &= 0x3ffff;
#endif
        clnk_reg_read(pMbx->dc_ctx, offset, &regVal);
        transID = regVal & 0xff;

//HostOS_PrintLog(L_INFO, "Clnk_MBX_Read_ISR transid=%d \n", transID );
        if (transID & HOST_ORIG_BIT)
        {
            index = transID & CMD_QUEUE_MASK;

            // Lock the mailbox
            HostOS_Lock(pMbx->mbx_lock);

            // Check if transaction ID is valid
            if (pMbx->cmdQueue[index].transID != transID)
            {
                // Toss message if transaction ID is not valid
                Clnk_MBX_Read_Hw_Mailbox(pMbx, SYS_NULL);

                // Unlock the mailbox
                HostOS_Unlock(pMbx->mbx_lock);

                return;
            }

            // Read message from hardware mailbox
            Clnk_MBX_Read_Hw_Mailbox(pMbx, &pMbx->cmdQueue[index].rcvMsg);

            // Mark reply as received
            pMbx->cmdQueue[index].isReplyRdy = SYS_TRUE;
//HostOS_PrintLog(L_INFO, "In ClnkMBX_READ_ISR  replyRdy = true\n");
            // Call reply ready callback function
            if (pMbx->replyRdyCallback != SYS_NULL)
            {           // MbxReplyRdyCallback
                pMbx->replyRdyCallback(pMbx->replyParam,
                                       &pMbx->cmdQueue[index].rcvMsg);
            }

            // Unlock the mailbox
            HostOS_Unlock(pMbx->mbx_lock);
            HostOS_wqt_waitq_wakeup_intr( pMbx->cmdQueue[index].msg_wqt );

        }
        else
        {
            Clnk_MBX_Read_Hw_Mailbox(pMbx, SYS_NULL);       // discard this type of unsolicited msg
        }
    }
}

/*******************************************************************************
*
* Description:
*       Handles the write interrupt.
*
* Inputs:
*       pMbx - Pointer to the mailbox
*
* Outputs:
*       None
*
*******************************************************************************/
void Clnk_MBX_Write_ISR( Clnk_MBX_Mailbox_t *pMbx )
{

    if (Clnk_MBX_Write_Hw_Mailbox_Check_Ready(pMbx))
    {
        // Lock the mailbox
        HostOS_Lock(pMbx->mbx_lock);

        // Check if command queue is emtpy
        if (pMbx->cmdHeadIndex == pMbx->cmdTailIndex)
        {
            HostOS_Unlock(pMbx->mbx_lock);
            return;
        }

        // Write message to hardware mailbox
        Clnk_MBX_Write_Hw_Mailbox(pMbx, &pMbx->cmdQueue[pMbx->cmdHeadIndex].sendMsg);

        // Remove entry from head of command queue
        pMbx->cmdHeadIndex = (pMbx->cmdHeadIndex+1) & CMD_QUEUE_MASK;

        // Unlock the mailbox
        HostOS_Unlock(pMbx->mbx_lock);
    }
}

/*******************************************************************************
*
* Description:
*       Handles the unsolicited message (read) interrupt.
*
* Inputs:
*       pMbx         - Pointer to the mailbox
*
* Outputs:
*       None
*
*******************************************************************************/
void Clnk_MBX_Unsol_ISR( Clnk_MBX_Mailbox_t* pMbx )
{
    SYS_UINT32          tid;
    SYS_UINT8           index;
    SYS_UINT32          pSwUnsolMsg;
    SYS_UINT32          ownership, i, *up, *qp, type ;
    int                 consumed ;
    Clnk_MBX_Msg_t      mbuf;
#ifdef Q_SHOW
    int                 hit ;
    SYS_UINT32          pSwUnsolMsgX;
    static char         ox[SW_UNSOL_HW_QUEUE_SIZE+1] ;
    static SYS_UINT8    dexx ;
#endif
    // Check if software unsolicited queue has been created
    if (pMbx->pSwUnsolQueue == 0)
    {
        return;
    }

    // Process the unsolicited messages

    ACK_SW_UNSOL_INTERRUPT(pMbx);
#if 1
// for loop with clnk_reg_read caused hang
    for ( ; ; )
    {
        // get the current index
        index = pMbx->swUnsolReadIndex;

        // Check ownership bit
        pSwUnsolMsg = pMbx->pSwUnsolQueue + index * sizeof(Clnk_MBX_SwUnsolQueueEntry_t);
        clnk_reg_read(pMbx->dc_ctx, UMSG_ELEMENT(pSwUnsolMsg, ownership), &ownership);
        if (!ownership)
        {
            break ;
        }
#if 1
        // we have a message, process it

        // Generate transaction ID (make sure host originated bit is clear)
        pMbx->swUnsolCurrTransID = index;
        pMbx->swUnsolCurrTransID &= ~HOST_ORIG_BIT;
        pMbx->swUnsolCurrTransID |= UNSOL_TYPE_BIT;
        tid = pMbx->swUnsolCurrTransID;

        clnk_reg_write(pMbx->dc_ctx, UMSG_ELEMENT(pSwUnsolMsg, transID), tid);

        // process the message

        // copy from hw Q to local msg buffer
        up   = &mbuf.msg.maxMsg.msg[0] ; 
#if 1
        for (i = 0; i < MAX_UNSOL_MSG; i++, up++)
        {
            clnk_reg_read(pMbx->dc_ctx,
                            UMSG_ELEMENT(pSwUnsolMsg, msg[0]) + (i << 2),
                            up);
        }
#endif
        up   = &mbuf.msg.maxMsg.msg[0] ; 
        type = *up ;

        consumed = 0 ;
#if 1
        if (pMbx->swUnsolRdyCallback != SYS_NULL)      // if there's a callback, call it
        {               // MbxSwUnsolRdyCallback
            consumed = pMbx->swUnsolRdyCallback(pMbx->swUnsolParam, &mbuf);
#ifdef U_SHOW_TRAFFIC
            if( consumed )
            {
                HostOS_PrintLog(L_NOTICE, "sw unsol msg, type=%d. consumed=%d tid=%xx\n",
                                type, consumed, tid );
            }
#endif
        }
#endif
#if 1
        if( !consumed )
        {
            // queue msg in sw Q

            HostOS_Lock(pMbx->swUnsolLock);

            // copy from buffer to sw Q
            qp = &pMbx->swUnsolQueue[pMbx->swUnsolTailIndex].msg.msg.maxMsg.msg[0] ; 
            for (i = 0; i < MAX_UNSOL_MSG; i++)
            {
                *qp++ = *up++ ; 
            }
            up = &mbuf.msg.maxMsg.msg[0] ; 

            if( pMbx->swUnsolCount == SW_UNSOL_HW_QUEUE_SIZE )
            {  // if max count pMbx->swUnsolOverrun = 1 ;      // flag an error
                // move head to oldest
                pMbx->swUnsolHeadIndex = (pMbx->swUnsolHeadIndex+1) & SWUNSOL_QUEUE_MASK;
            }
            else
            {
                pMbx->swUnsolCount++ ;          // count an item added
            }
            // move tail off newest
            pMbx->swUnsolTailIndex = (pMbx->swUnsolTailIndex+1) & SWUNSOL_QUEUE_MASK;

            HostOS_Unlock(pMbx->swUnsolLock);
#ifdef U_SHOW_TRAFFIC
            HostOS_PrintLog(L_NOTICE, "sw unsol msg Qd, type=%d. cnt=%d. tid=%xx\n",
                            *up, pMbx->swUnsolCount, tid );
#endif
        }

#endif
        // Clear ownership
        clnk_reg_write(pMbx->dc_ctx, UMSG_ELEMENT(pSwUnsolMsg, ownership), 0);

        // Increment HW's read index
        pMbx->swUnsolReadIndex = (pMbx->swUnsolReadIndex+1) % pMbx->swUnsolQueueSize;
#endif
    }
#endif
}

/*******************************************************************************
*
* Description:
*       Checks if the hardware mailbox is available to read.
*
* Inputs:
*       pMbx - Pointer to the mailbox
*
* Outputs:
*       1 if mailbox is available to read, 0 otherwise
*
*******************************************************************************/
int Clnk_MBX_Read_Hw_Mailbox_Check_Ready(Clnk_MBX_Mailbox_t* pMbx)
{
    SYS_UINT32 regVal;

    // Check if mailbox available to read
    clnk_reg_read(pMbx->dc_ctx, pMbx->readMbxCsrOffset, &regVal);
//HostOS_PrintLog(L_INFO, "HostCheckMailboxAvailToRead: readMbxCsrOffset = %x, regVal = %x \n",
//                pMbx->readMbxCsrOffset, regVal);

    return ((regVal & CLNK_REG_MBX_SEMAPHORE_BIT) != 0);
}

/*******************************************************************************
*
* Description:
*       Checks if the hardware mailbox is available to write.
*
* Inputs:
*       pMbx - Pointer to the mailbox
*
* Outputs:
*       1 if mailbox is available to write, 0 otherwise
*
*******************************************************************************/
int Clnk_MBX_Write_Hw_Mailbox_Check_Ready(Clnk_MBX_Mailbox_t* pMbx)
{
    SYS_UINT32 regVal;

    // Check if mailbox available to write
    clnk_reg_read(pMbx->dc_ctx, pMbx->writeMbxCsrOffset, &regVal);

    return ((regVal & CLNK_REG_MBX_SEMAPHORE_BIT) == 0);
}

/*******************************************************************************
*
* Description:
*       Checks if the write cmd message mailbox is empty.
*
* Inputs:
*       pMbx - Pointer to the mailbox
*
* Outputs:
*       1 = empty, 0 = !empty
*
*******************************************************************************/
int Clnk_MBX_Send_Mailbox_Check_Empty(Clnk_MBX_Mailbox_t* pMbx)
{
    int mt ;

    // Lock the mailbox
    HostOS_Lock(pMbx->mbx_lock);

    // Check if command queue is emtpy
    mt = (pMbx->cmdHeadIndex == pMbx->cmdTailIndex) ;
    
    // Unlock the mailbox
    HostOS_Unlock(pMbx->mbx_lock);

    return( mt ) ;
}

/*******************************************************************************
*
* Description:
*       Checks if the hardware unsolicited mailbox is available to read.
*
* Inputs:
*       pMbx - Pointer to the mailbox
*
* Outputs:
*       1 if mailbox is available to read, 0 otherwise
*
*******************************************************************************/
int Clnk_MBX_Unsol_Hw_Mailbox_Check_Ready(Clnk_MBX_Mailbox_t* pMbx)
{
    SYS_UINT8           index;
    SYS_UINT32          pSwUnsolMsg;
    SYS_UINT32          ownership ;

    ownership = 0 ;

    // get the current index
    index = pMbx->swUnsolReadIndex;

    // Check ownership bit
    pSwUnsolMsg = pMbx->pSwUnsolQueue + index * sizeof(Clnk_MBX_SwUnsolQueueEntry_t);
//    HostOS_PrintLog(L_INFO, "In Clnk_MBX_Unsol_Hw_Mailbox_Check_Ready: pMbx->pSwUnsolQueue=%x, index=%d, sizeofSwUnsolEntry=%d\n",
//                    pMbx->pSwUnsolQueue, index , sizeof(Clnk_MBX_SwUnsolQueueEntry_t));
    clnk_reg_read(pMbx->dc_ctx, UMSG_ELEMENT(pSwUnsolMsg, ownership), &ownership);
//    clnk_reg_read(pMbx->dc_ctx, 0x8c00b204, &ownership);
if (ownership == 1)
    return(1);
else 
    return(0);
}


/*******************************************************************************
*
* Description:
*       Allocates all Mailbox message wqts.
*       Sets up the timer and wq.
*
* Inputs:
*       pMbx            - Pointer to the mailbox
*
* Outputs:
*
*******************************************************************************/
static void Clnk_MBX_Alloc_wqts( Clnk_MBX_Mailbox_t *pMbx )
{
    int                         msg ;
    Clnk_MBX_CmdQueueEntry_t    *mp ;

    // for all messages
    for( msg = 0 ; msg < CLNK_MBX_CMD_QUEUE_SIZE ; msg++ )
    {
        mp = &pMbx->cmdQueue[msg] ;
        if( !mp->msg_wqt ) {
            mp->msg_wqt = HostOS_wqt_alloc( );
            if( mp->msg_wqt )
            {
                HostOS_wqt_waitq_init( mp->msg_wqt );
                HostOS_wqt_timer_init( mp->msg_wqt );
                HostOS_wqt_timer_setup( mp->msg_wqt , 
                                        Clnk_MBX_msg_timer, 
                                        (SYS_UINTPTR)mp ) ;
            }
            else
            {
                HostOS_PrintLog(L_ERR, "Cannot allocate wqt.\n" );
                break ;
            }
        }
    }
}

/*******************************************************************************
*
* Description:
*           Wait q condition expression check.
*
* Inputs:   vmp - Pointer to a mailbox message
*
* Outputs:  true/false
*
*******************************************************************************/
static int Clnk_MBX_wqt_condition( void *vmp )
{
    Clnk_MBX_CmdQueueEntry_t    *mp ;
    int cond ;

    mp = (Clnk_MBX_CmdQueueEntry_t *)vmp ;

    cond = (mp->isReplyTimedOut || mp->isReplyRdy) ;
//HostOS_PrintLog(L_NOTICE, "<wqt> cond=%d. to=%d. rdy=%d.\n",
//                cond, mp->isReplyTimedOut, mp->isReplyRdy );
/*
if (cond)
    HostOS_PrintLog(L_INFO, "reply = %x\n", mp->isReplyRdy);
*/
    return( cond ) ;
}

/*******************************************************************************
*
* Description:
*       msg timer expiration function.
*
* Inputs:
*       data    - Pointer to the mailbox message with timeout
*
* Outputs:
*
*******************************************************************************/
static SYS_VOID Clnk_MBX_msg_timer(SYS_ULONG data)
{
    Clnk_MBX_CmdQueueEntry_t *mp ;

    mp = (Clnk_MBX_CmdQueueEntry_t *)data ;
    mp->isReplyTimedOut = 1 ;
    HostOS_wqt_timer_del( mp->msg_wqt );
    HostOS_wqt_waitq_wakeup_intr( mp->msg_wqt );
//HostOS_PrintLog(L_NOTICE, "<msgtimer> expired\n" );
}

/*******************************************************************************
*
* Description:
*       Receives a mailbox message.
*
* Inputs:   pMbx    - Pointer to the mailbox
*           pMsg    - Pointer to the buffer for receiving message
*           transID - Transaction ID of message to receive
*
* Exports:  0           - success, message queued/sent
*           -SYS_INVALID_ADDRESS_ERROR  - mailbox not open
*           -SYS_INPUT_OUTPUT_ERROR     - I/O error
*           -SYS_TIMEOUT_ERROR          - timeout waiting for response
*           ?           - other codes from called functions
*
*******************************************************************************/
static int Clnk_MBX_RcvMsg(Clnk_MBX_Mailbox_t *pMbx, 
                    Clnk_MBX_Msg_t     *pMsg,
                    SYS_UINT8          transID)
{
    SYS_UINT32      index;

    if (!pMbx->mbxOpen) 
        return(-SYS_INVALID_ADDRESS_ERROR);

    if (transID & HOST_ORIG_BIT)
    {
        //HostOS_PrintLog(L_NOTICE, "Clnk_MBX_RcvMsg recv tid=%02xx\n", transID);
        // Lock the mailbox
        HostOS_Lock(pMbx->mbx_lock);

        // Check if the transaction ID is still valid
        index = transID & CMD_QUEUE_MASK;
        if (pMbx->cmdQueue[index].transID != transID)
        {
            HostOS_Unlock(pMbx->mbx_lock);
            return (-SYS_INPUT_OUTPUT_ERROR);
        }

        // Check if reply has been received
        if (!pMbx->cmdQueue[index].isReplyRdy)
        {
            HostOS_Unlock(pMbx->mbx_lock);
            return (-SYS_TIMEOUT_ERROR);
        }

        //HostOS_PrintLog(L_NOTICE, "Clnk_MBX_RcvMsg tid=%02xx\n", transID );

        // Copy message to receive buffer
        HostOS_Memcpy(pMsg, &pMbx->cmdQueue[index].rcvMsg, sizeof(*pMsg));

        // Unlock the mailbox
        HostOS_Unlock(pMbx->mbx_lock);
//HostOS_PrintLog(L_NOTICE, "<rm>\n" );
    }
    else
    {
        return (-SYS_INPUT_OUTPUT_ERROR);
    }

    return (SYS_SUCCESS);
}

/*******************************************************************************
*
* Description:
*       Checks if a reply has been received.
*
* Inputs:
*       pMbx    - Pointer to the mailbox
*       transID - Transaction ID of message to check
*
* Outputs:
*       1 if a reply has been received, 0 otherwise
*
* Notes:
*       None
*
*******************************************************************************/
static int Clnk_MBX_CheckReplyRdy(Clnk_MBX_Mailbox_t* pMbx, SYS_UINT8 transID)
{
    SYS_UINT32  index;
    int         rdy ;

    rdy = 123456 ;

    // Check if transaction ID is valid
    if ((transID & HOST_ORIG_BIT) == 0)
    {
        return (0);
    }

    // we're just reading, no lock

    index = transID & CMD_QUEUE_MASK;
    if (pMbx->cmdQueue[index].transID != transID)
    {
        return (0);
    }

    //HostOS_PrintLog(L_NOTICE, "x\n" );
    rdy = (volatile int)(pMbx->cmdQueue[index].isReplyRdy) ;
    //HostOS_PrintLog(L_NOTICE, "x%d\n", rdy );

    // Check if reply has been received
    if( rdy )
    {
        //HostOS_PrintLog(L_INFO, "reply has been received \n");
        return (1);
    }
        //HostOS_PrintLog(L_INFO, "reply has not been received \n");
    return (0);
}

/*******************************************************************************
*
* Description:
*       Clears the read mailbox ready bit.
*
* Inputs:
*       pMbx - Pointer to the mailbox
*
* Outputs:
*
*******************************************************************************/
static void Clnk_MBX_Read_Hw_Mailbox_Clear_Ready(Clnk_MBX_Mailbox_t* pMbx)
{
    SYS_UINT32 val;

    clnk_reg_read(pMbx->dc_ctx, pMbx->readMbxCsrOffset, &val);
    val &= ~CLNK_REG_MBX_SEMAPHORE_BIT;
    clnk_reg_write(pMbx->dc_ctx, pMbx->readMbxCsrOffset, val);
}

/*******************************************************************************
*
* Description:
*       Sets the write hardware mailbox ready bit.
*
* Inputs:
*       pMbx - Pointer to the mailbox
*
* Outputs:
*
*******************************************************************************/
static void Clnk_MBX_Write_Hw_Mailbox_Set_Ready(Clnk_MBX_Mailbox_t* pMbx)
{
    SYS_UINT32 val;

    clnk_reg_read(pMbx->dc_ctx, pMbx->writeMbxCsrOffset, &val);
    val |= CLNK_REG_MBX_SEMAPHORE_BIT;
    clnk_reg_write(pMbx->dc_ctx, pMbx->writeMbxCsrOffset, val);
}

/*******************************************************************************
*
* Description:
*       Reads a message from the hardware mailbox.
*
* Inputs:
*       Clnk_MBX_Mailbox_t*  Pointer to the mailbox
*       void*                Pointer to the buffer for reading message
*
* Outputs:
*       None
*
*******************************************************************************/
static void Clnk_MBX_Read_Hw_Mailbox(Clnk_MBX_Mailbox_t* pMbx, void* pvMsg)
{
    //SYS_UINT32  val;
    SYS_UINT32* pMsg = (SYS_UINT32 *)pvMsg;
    SYS_UINT32  len;

    // Read message from mailbox
    if (pMsg != SYS_NULL)
    {
        SYS_UINT32 reg, lastReg;

        // Read location of mailbox message
        clnk_reg_read(pMbx->dc_ctx, pMbx->readMbxRegOffset, &reg);
#if ECFG_CHIP_ZIP1
        reg &= 0x0003ffff;
#endif

        // Read first word
        clnk_reg_read(pMbx->dc_ctx, reg, pMsg);
        len = CLNK_MBX_GET_REPLY_LEN(*pMsg);
        reg += 4;
        pMsg++;
        
        if((len < 1) || (len > MAX_MBX_MSG))
        {
            HostOS_PrintLog(L_ERR, "warning: invalid mbx reply: %d words\n", len);
            HostOS_Memset(pMsg, 0, sizeof(*pMsg));
            return;
        }

        // Read subsequent words, if necessary
        lastReg = reg + ((len-1) * 4);
        for ( ; reg < lastReg; reg += 4, pMsg++)
        {
            clnk_reg_read(pMbx->dc_ctx, reg, pMsg);
        }

#if 0
        SYS_UINT32 reg = pMbx->readMbxRegOffset;
        SYS_UINT32 lastReg = reg + (pMbx->readMbxSize * 4);
        for ( ; reg < lastReg; reg += 4, pMsg++)
        {
            clnk_reg_read(pMbx->dc_ctx, reg, pMsg);
        }
#endif
    }

    Clnk_MBX_Read_Hw_Mailbox_Clear_Ready(pMbx);
}

/*******************************************************************************
*
* Description:
*       Writes a message to the hardware mailbox.
*
* Inputs:
*       Clnk_MBX_Mailbox_t*  Pointer to the mailbox
*       void*                Pointer to the message to write
*
* Outputs:
*       None
*
* Notes:
*       None
*
*
*******************************************************************************/
static void Clnk_MBX_Write_Hw_Mailbox(Clnk_MBX_Mailbox_t* pMbx, void* pvMsg)
{
    //SYS_UINT32  val;
    SYS_UINT32* pMsg = (SYS_UINT32 *)pvMsg;
    SYS_UINT32  len;
    SYS_UINT32  reg, lastReg;

    // Read location of mailbox message
//HostOS_PrintLog(L_INFO, "In Clnk_MBX_Write_Hw_Mailbox: writeMbxRegOffset=%x\n",pMbx->writeMbxRegOffset);
    clnk_reg_read(pMbx->dc_ctx, pMbx->writeMbxRegOffset, &reg);
#if ECFG_CHIP_ZIP1
    reg &= 0x0003ffff;
#endif

    // Write message to mailbox
    len = CLNK_MBX_GET_CMD_LEN(*pMsg);
    lastReg = reg + (len * 4);
    for ( ; reg < lastReg; reg += 4, pMsg++)
    {
//HostOS_PrintLog(L_INFO, "C_MBX_Wr_Hw_Mbx a=%08x data=%08x\n", reg, *pMsg );
        clnk_reg_write(pMbx->dc_ctx, reg, *pMsg);
    }

#if 0
    SYS_UINT32* pMsg = (SYS_UINT32 *)pvMsg;
    SYS_UINT32  reg = pMbx->writeMbxRegOffset;
    SYS_UINT32  lastReg = reg + (8 * 4);
    for ( ; reg < lastReg; reg += 4, pMsg++)
    {
        clnk_reg_write(pMbx->dc_ctx, reg, *pMsg);
    }
#endif

    Clnk_MBX_Write_Hw_Mailbox_Set_Ready(pMbx);
}


#ifdef CLNK_MBX_AUTO_REPLY
static 
void AutoReplyMsg(Clnk_MBX_Mailbox_t* pMbx)
{
    SYS_UINT32 regVal;

    // Check the write semaphore bit
    for ( ; ; )
    {
        clnk_reg_read(pMbx->dc_ctx, CLNK_REG_MBX_WRITE_CSR, &regVal);
        if (regVal & CLNK_REG_MBX_SEMAPHORE_BIT)
        {
            break;
        }
    }

    // Loop back message
    clnk_reg_read(pMbx->dc_ctx,  CLNK_REG_MBX_REG_1,  &regVal);
    regVal >>= 8;
    regVal &= 0xff;
    clnk_reg_write(pMbx->dc_ctx, CLNK_REG_MBX_REG_9,  regVal);
    clnk_reg_write(pMbx->dc_ctx, CLNK_REG_MBX_WRITE_CSR, 0);

    // Set the read semaphore bit
    clnk_reg_write(pMbx->dc_ctx, CLNK_REG_MBX_READ_CSR, CLNK_REG_MBX_SEMAPHORE_BIT);
}
#endif


//BRJ temp to test sending of mailbox message and recieving response via interrupt
#ifdef SEND_MBX_MSG_TEST
int Clnk_MBX_SendTestMsg( void* pvMailBox )
{
    ClnkDef_MyNodeInfo_t NodeInfo;
    Clnk_MBX_Mailbox_t* pMbx;
    SYS_UINT32  u32Val;
    pMbx = ( Clnk_MBX_Mailbox_t* )pvMailBox;

    clnk_reg_read(pMbx->dc_ctx, CLNK_REG_MBX_WRITE_CSR, &u32Val);
    if( 0 == ( CLNK_REG_MBX_SEMAPHORE_BIT & u32Val))
    {
        // mailbox is available to write to
        // get interrupt status for debugging
#if !ECFG_CHIP_ZIP1
        clnk_reg_read(pMbx->dc_ctx, CLNK_MBX_INTERRUPT_REG, &u32Val);
#endif // !ECFG_CHIP_ZIP1
        // set command
        NodeInfo.ClearStats = 0;
        u32Val = CLNK_ETH_CTRL_GET_MY_NODE_INFO;
        clnk_reg_write(pMbx->dc_ctx, CLNK_REG_MBX_REG_1, u32Val);
        // set params 
        u32Val = CLNK_MBX_SET_CMD( CLNK_MBX_ETH_DATA_BUF_CMD ); 
        clnk_reg_write(pMbx->dc_ctx, CLNK_REG_MBX_REG_2, u32Val);
    
        u32Val = sizeof(ClnkDef_MyNodeInfo_t);
        clnk_reg_write(pMbx->dc_ctx, CLNK_REG_MBX_REG_3, u32Val);
    
        u32Val = NodeInfo.ClearStats;
        clnk_reg_write(pMbx->dc_ctx, CLNK_REG_MBX_REG_4, u32Val);
    
        // tell clink we wrote a message
        u32Val = CLNK_REG_MBX_SEMAPHORE_BIT;
        clnk_reg_write(pMbx->dc_ctx, CLNK_REG_MBX_WRITE_CSR, u32Val);
    }
    return (SYS_SUCCESS);
}
#endif // SEND_MBX_MSG_TEST


#if defined(E1000_DRVR_SUPPORT) || defined(CANDD_DRVR_SUPPORT) || defined(CONFIG_TIVO)
/*******************************************************************************
*
* Description:
*       Clnk kernel thread main loop function. it processes mbox messages etc.
*
* Inputs:
*       data - Pointer to driver control context
*
* Outputs:
*       None
*
*******************************************************************************/
void Clnk_Kthread_Mainloop(void *data)
{
    dc_context_t *dccp = (dc_context_t *)data;    
  
    while(1)
    {
        if(dccp->clnkThreadStop == CLNK_TASK_RUNNING)
        {
            //PCI mode, CAM processing
            //Clnk_ETH_Cam_Proc(dccp);
        
            //MBOX processing
            Clnk_MBX_Read_ISR( &dccp->mailbox ) ;
            Clnk_MBX_Write_ISR( &dccp->mailbox ) ;
            //Unsol msg processing
            if(Clnk_MBX_Unsol_Hw_Mailbox_Check_Ready( &dccp->mailbox ))
            {
               Clnk_MBX_Unsol_ISR( &dccp->mailbox ) ;
            }
        }
        else
            dccp->clnkThreadStop = CLNK_TASK_SLEEP;

        /* Sleep */
        HostOS_msleep_interruptible(TT_TASK_SLEEP); 

        if(HostOS_signal_pending(SYS_NULL) != 0)
            break;
    }
}

/*******************************************************************************
*
* Description:
*       Clnk kernel thread initialization.
*
* Inputs:
*       data - Pointer to driver control context
*
* Outputs:
*       OK: 0, Failed: -1
*
*******************************************************************************/
int Clnk_Kern_Task_Init(void *data)
{
    dc_context_t *dccp = (dc_context_t *)data;
    int ret;
    char name[20] = "kclinkd";

    //HostOS_PrintLog(L_INFO, "Clnk_Kern_Task_Init(): Entry dccp 0x%x \n", data);

    if(dccp->clnkThreadID != 0)
    {   
        if(dccp->clnkThreadStop == CLNK_TASK_STOPPED || dccp->clnkThreadStop == CLNK_TASK_SLEEP)
            dccp->clnkThreadStop = CLNK_TASK_RUNNING; //Running
        return 0;
    }

    ret = HostOS_thread_start(&dccp->clnkThreadID, (char *)name, (SYS_VDFCVD_PTR)Clnk_Kthread_Mainloop, data);
    if(ret < 0)
    {
        HostOS_PrintLog(L_ERR, "Failed to start kclinkd thread! \n");
        dccp->clnkThreadID = 0;
        return -1;
    }
    //else
    //    HostOS_PrintLog(L_INFO, "Start kclinkd thread %d\n", dccp->clnkThreadID);

    dccp->clnkThreadStop = CLNK_TASK_RUNNING;
    return 0;
}

/*******************************************************************************
*
* Description:
*       Stop Clnk kernel thread.
*
* Inputs:
*       data - Pointer to driver control context
*
* Outputs:
*       OK: 0, Failed: -1
*
*******************************************************************************/
int Clnk_Kern_Task_Stop(void *data)
{
    dc_context_t *dccp = (dc_context_t *)data;
    int loopCnt = 0;

    //HostOS_PrintLog(L_INFO, "Clnk_Kern_Task_Stop(): Entry \n");

    if(dccp->clnkThreadID != 0)
    {
        dccp->clnkThreadStop = CLNK_TASK_STOPPED;

        // Wait thread to stop,
        while(1)
        {
            if(dccp->clnkThreadStop == CLNK_TASK_SLEEP || loopCnt > 10)
                break;

            //HostOS_PrintLog(L_INFO, "Clnk_Kern_Task_Stop(): Delay %d \n", loopCnt);
            HostOS_msleep_interruptible(1); // 1ms
            loopCnt++;
        }
    }
    return 0;
}

/*******************************************************************************
*
* Description:
*       Kill Clnk kernel thread.
*
* Inputs:
*       data - Pointer to driver control context
*
* Outputs:
*       OK: 0, Failed: -1
*
*******************************************************************************/
int Clnk_Kern_Task_Kill(void *data)
{
    dc_context_t *dccp = (dc_context_t *)data;
    int ret = 0;
    
    //HostOS_PrintLog(L_INFO, "Clnk_Kern_Task_Kill(): Entry \n");

    ret = HostOS_thread_stop(dccp->clnkThreadID);

    if(ret != 0)
    {
        HostOS_PrintLog(L_ERR, "Unable to stop kclnkd thread! (thread ID %d)\n", dccp->clnkThreadID);
        return -1;
    }

    dccp->clnkThreadID = 0;
    dccp->clnkThreadStop = CLNK_TASK_STOPPED;
    return 0;
}
#endif
