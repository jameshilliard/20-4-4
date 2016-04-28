/*******************************************************************************
*
* Common/Inc/ClnkMbx_dvr.h
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

#ifndef __ClnkMbx_dvr_h__
#define __ClnkMbx_dvr_h__

/*******************************************************************************
*                             # i n c l u d e s                                *
********************************************************************************/

#include "inctypes_dvr.h"
#include "common_dvr.h"

/*******************************************************************************
*                             # d e f i n e s                                  *
********************************************************************************/

// Common Mailbox Module Configuration
//#define CLNK_MBX_AUTO_REPLY  // Auto reply (for debugging only)

#define MAX_UNSOL_MSG       8
#define MAX_MBX_MSG         100

// Mailbox Type Constants
#define CLNK_MBX_ETHERNET_TYPE  0
#define CLNK_MBX_MPEG_TYPE      1

// Mailbox Polling Rate
#define MBX_POLL_TIMEOUT_IN_US    (20*1000)

// Mailbox Queue Constants
// queue sizes are in HostOS_Spec.h

// Mailbox Command Code Definitions
#define CLNK_MBX_CMD_CODE_OFFSET      0
#define CLNK_MBX_CMD_CODE_MASK        0xff
#define CLNK_MBX_CMD_TRANS_ID_OFFSET  8
#define CLNK_MBX_CMD_TRANS_ID_MASK    0xff
#define CLNK_MBX_CMD_VER_NUM_OFFSET   16
#define CLNK_MBX_CMD_VER_NUM_MASK     0xf
#define CLNK_MBX_CMD_PORT_OFFSET      20
#define CLNK_MBX_CMD_PORT_MASK        0xf
#define CLNK_MBX_CMD_LEN_OFFSET       24
#define CLNK_MBX_CMD_LEN_MASK         0xff

#define CLNK_MBX_SET_CMD(x)       (((x) & CLNK_MBX_CMD_CODE_MASK)     << \
                                   CLNK_MBX_CMD_CODE_OFFSET)
#define CLNK_MBX_SET_TRANS_ID(x)  (((x) & CLNK_MBX_CMD_TRANS_ID_MASK) << \
                                   CLNK_MBX_CMD_TRANS_ID_OFFSET)
#define CLNK_MBX_SET_VER_NUM(x)   (((x) & CLNK_MBX_CMD_VER_NUM_MASK)  << \
                                   CLNK_MBX_CMD_VER_NUM_OFFSET)
#define CLNK_MBX_SET_PORT(x)      (((x) & CLNK_MBX_CMD_PORT_MASK)     << \
                                   CLNK_MBX_CMD_PORT_OFFSET)
#define CLNK_MBX_SET_LEN(x)       (((x) & CLNK_MBX_CMD_LEN_MASK)      << \
                                   CLNK_MBX_CMD_LEN_OFFSET)

#define CLNK_MBX_GET_CMD(x)       (((x) >> CLNK_MBX_CMD_CODE_OFFSET) &   \
                                   CLNK_MBX_CMD_CODE_MASK)
#define CLNK_MBX_GET_CMD_LEN(x)   (((x) >> CLNK_MBX_CMD_LEN_OFFSET) &    \
                                   CLNK_MBX_CMD_LEN_MASK)


// Mailbox Reply Status Definitions
#define CLNK_MBX_REPLY_TRANS_ID_OFFSET  0
#define CLNK_MBX_REPLY_TRANS_ID_MASK    0xff
#define CLNK_MBX_REPLY_STATUS_OFFSET    8
#define CLNK_MBX_REPLY_STATUS_MASK      0xff
#define CLNK_MBX_REPLY_ARG1_OFFSET      16
#define CLNK_MBX_REPLY_ARG1_MASK        0xff
#define CLNK_MBX_REPLY_LEN_OFFSET       24
#define CLNK_MBX_REPLY_LEN_MASK         0xff

#define CLNK_MBX_GET_TRANS_ID(x)  (((x) >> CLNK_MBX_REPLY_TRANS_ID_OFFSET) & \
                                   CLNK_MBX_REPLY_TRANS_ID_MASK)
#define CLNK_MBX_GET_STATUS(x)    (((x) >> CLNK_MBX_REPLY_STATUS_OFFSET)   & \
                                   CLNK_MBX_REPLY_STATUS_MASK)
#define CLNK_MBX_GET_ARG1(x)      (((x) >> CLNK_MBX_REPLY_ARG1_OFFSET)     & \
                                   CLNK_MBX_REPLY_ARG1_MASK)
#define CLNK_MBX_GET_REPLY_LEN(x) (((x) >> CLNK_MBX_REPLY_LEN_OFFSET)      & \
                                   CLNK_MBX_REPLY_LEN_MASK)

// CLNK Kernel Thread Status Definition
#define CLNK_TASK_RUNNING       0
#define CLNK_TASK_STOPPED       1
#define CLNK_TASK_SLEEP         2

/*******************************************************************************
*                       G l o b a l   D a t a   T y p e s                      *
********************************************************************************/

// Mailbox Status Codes
typedef enum
{
    CLNK_MBX_STATUS_SUCCESS = SYS_SUCCESS,

    CLNK_MBX_STATUS_MAX  // This must always be last
}
CLNK_MBX_STATUS;

// Mailbox Control Options
typedef enum
{
    CLNK_MBX_CTRL_ENABLE_INTERRUPT,
    CLNK_MBX_CTRL_DISABLE_INTERRUPT,
    CLNK_MBX_CTRL_CLEAR_INTERRUPT,
    CLNK_MBX_CTRL_SET_SW_UNSOL_Q,
    CLNK_MBX_CTRL_SET_REPLY_RDY_CB,
    CLNK_MBX_CTRL_SET_UNSOL_RDY_CB,
    CLNK_MBX_CTRL_SET_SW_UNSOL_RDY_CB,

    CLNK_MBX_CTRL_MAX  // This must always be last
}
CLNK_MBX_CTRL_OPTIONS;

// Command Codes
typedef enum
{
    // Ethernet commands
    CLNK_MBX_ETH_RESET_CMD                        = 0x00,   /* Alias: ETH_MB_RESET,                             */
    CLNK_MBX_ETH_ALLOC_TX_FIFO_CMD                = 0x01,   /* Alias: ETH_MB_ALLOCATE_TX_FIFO,                  */
    CLNK_MBX_ETH_ALLOC_RX_FIFO_CMD                = 0x02,   /* Alias: ETH_MB_ALLOCATE_RX_FIFO,                  */
    CLNK_MBX_ETH_JOIN_MCAST_CMD                   = 0x03,   /* Alias: ETH_MB_JOIN_MULTICAST,                    */
    CLNK_MBX_ETH_LEAVE_MCAST_CMD                  = 0x04,   /* Alias: ETH_MB_LEAVE_MULTICAST,                   */
    CLNK_MBX_ETH_REGISTER_MCAST_CMD               = 0x05,   /* Alias: ETH_MB_REGISTER_MULTICAST,                */
    CLNK_MBX_ETH_GET_STATS_CMD                    = 0x06,   /* Alias: ETH_MB_QUERY_RCV_STATS,                   */
    CLNK_MBX_ETH_SET_RCV_MODE_CMD                 = 0x07,   /* Alias: ETH_MB_SET_RCV_MODE,                      */
    CLNK_MBX_ETH_GET_RCV_MODE_CMD                 = 0x08,   /* Alias: ETH_MB_GET_RCV_MODE,                      */
    CLNK_MBX_ETH_PUB_UCAST_CMD                    = 0x09,   /* Alias: ETH_MB_PUBLISH_UNICAST_ADDR,              */
    CLNK_MBX_ETH_DEREGISTER_MCAST_CMD             = 0x0a,   /* Alias: ETH_MB_DEREGISTER_MULTICAST,              */
    CLNK_MBX_ETH_DATA_BUF_CMD                     = 0x0b,   /* Alias: ETH_MB_DATA_BUF_CMD,                      */
    CLNK_MBX_ETH_GET_STATUS_CMD                   = 0x0c,   /* Alias: ETH_MB_GET_STATUS,                        */
    CLNK_MBX_ETH_SET_SW_CONFIG_CMD                = 0x0d,   /* Alias: ETH_MB_SET_SW_CONFIG,                     */
    CLNK_MBX_ETH_ECHO_PROFILE_PROBE_RESPONSE      = 0x0e,   /* Alias: ETH_MB_ECHO_PROFILE_PROBE_RESPONSE,       */
    CLNK_MBX_ETH_GET_CMD                          = 0x0f,   /* Alias: ETH_MB_GET_CMD,                           */
    CLNK_MBX_ETH_SET_CMD                          = 0x10,   /* Alias: ETH_MB_SET_CMD,                           */
    CLNK_MBX_ETH_CLINK_ACCEPT_SMALL_ROUTED_MESSAGE = 0x11,  /* Alias: ETH_MB_CLINK_ACCEPT_SMALL_ROUTED_MESSAGE, */
    CLNK_MBX_ETH_SET_BEACON_POWER_LEVEL_CMD       = 0x12,   /* Alias: ETH_MB_SET_BEACON_POWER_LEVEL_CMD,        */
    CLNK_MBX_ETH_QOS_ASYNC_REQ_BLOB               = 0x13,   /* Alias: ETH_MB_QOS_ASYNC_REQ_BLOB,                */
    CLNK_MBX_ETH_QFM_RESP_BLOB                    = 0x14,   /* Alias: ETH_MB_QFM_RESP_BLOB,                     */
    CLNK_MBX_ETH_SET_MIXED_MODE_ACTIVE            = 0x15,   /* Obsolete */
    CLNK_MBX_ETH_GET_MIXED_MODE_ACTIVE            = 0x16,   /* Obsolete */
    CLNK_MBX_ETH_QFM_CAM_DONE                     = 0x17,   /* Alias: ETH_MB_QFM_CAM_DONE,                      */
    // MPEG commands
    CLNK_MBX_MPEG_CREATE_INPUT_CHANNEL_CMD        = 0x20,
    CLNK_MBX_MPEG_JOIN_CHANNEL_CMD                = 0x21,
    CLNK_MBX_MPEG_JOIN_AS_PASSIVE_LISTENER_CMD    = 0x22,
    CLNK_MBX_MPEG_SUBSCRIPTION_REQUEST_CMD        = 0x23,
    CLNK_MBX_MPEG_MODIFY_INPUT_CHANNEL_CMD        = 0x24,
    CLNK_MBX_MPEG_MODIFY_OUTPUT_CHANNEL_CMD       = 0x25,
    CLNK_MBX_MPEG_LEAVE_INPUT_CHANNEL_CMD         = 0x26,
    CLNK_MBX_MPEG_LEAVE_OUTPUT_CHANNEL_CMD        = 0x27,
    CLNK_MBX_MPEG_CREATE_OUTPUT_CHANNEL_CMD       = 0x28,
    CLNK_MBX_MPEG_RESERVED_CMD_FROM               = 0x29,
    CLNK_MBX_MPEG_RESERVED_CMD_TO                 = 0x2F,
    CLNK_MBX_MPEG_READ_TRANSMIT_STATS_CMD         = 0x30,
    CLNK_MBX_MPEG_READ_RECEIVE_STATS_CMD          = 0x31,
    CLNK_MBX_MPEG_SUBSCRIPTION_REPORT_REQUEST_CMD = 0x32,

    CLNK_MBX_MAX_CMD  // This must always be last
}
CLNK_MBX_CMD_CODES;

// Unsolicited message types
typedef enum
{
    CLNK_MBX_UNSOL_MSG_UCAST_PUB                    = 0,     /* Alias: UNSOL_MSG_UCAST_PUB_TYPE               */
    CLNK_MBX_UNSOL_MSG_EVM_DATA_READY               = 1,     /* Alias: UNSOL_MSG_EVM_DATA_READY_TYPE          */
    CLNK_MBX_UNSOL_MSG_ECHO_PROFILE_PROBE           = 2,     /* Alias: UNSOL_MSG_ECHO_PROFILE_PROBE           */
    CLNK_MBX_UNSOL_MSG_ADMISSION_STATUS             = 3,     /* Alias: UNSOL_MSG_ADMISSION_STATUS             */
    CLNK_MBX_UNSOL_MSG_BEACON_STATUS                = 4,     /* Alias: UNSOL_MSG_BEACON_STATUS                */
    CLNK_MBX_UNSOL_MSG_RESET                        = 5,     /* Alias: UNSOL_MSG_MAC_RESET                    */
    CLNK_MBX_UNSOL_MSG_TABOO_INFO                   = 6,     /* Alias: UNSOL_MSG_TABOO_INFO                   */
    CLNK_MBX_UNSOL_MSG_ACCESS_CHK_MAC               = 7,     /* Alias: UNSOL_MSG_CHK_MAC_ADDRESS              */
    CLNK_MBX_UNSOL_MSG_NODE_ADDED                   = 8,     /* Alias: UNSOL_MSG_NODE_ADDED                   */
    CLNK_MBX_UNSOL_MSG_NODE_DELETED                 = 9,     /* Alias: UNSOL_MSG_NODE_DELETED                 */
    CLNK_MBX_UNSOL_MSG_UCAST_UNPUB                  = 10,    /* Alias: UNSOL_MSG_UCAST_UNPUB_TYPE             */
    CLNK_MBX_UNSOL_MSG_ROUTE_SMALL_MESSAGE_TO_HOST  = 11,    /* Alias: UNSOL_MSG_ROUTE_SMALL_MESSAGE_TO_HOST  */
    CLNK_MBX_UNSOL_MSG_LOG_SMALL_MESSAGE_TO_HOST    = 12,    /* Alias: UNSOL_MSG_LOG_SMALL_MESSAGE_TO_HOST    */
    CLNK_MBX_UNSOL_MSG_LOG_MESSAGE_INDIRECT_TO_HOST = 13,    /* Alias: UNSOL_MSG_LOG_MESSAGE_INDIRECT_TO_HOST */
    CLNK_MBX_UNSOL_MSG_FSUPDATE                     = 14,    /* Alias: UNSOL_MSG_FSUPDATE                     */
    CLNK_MBX_UNSOL_MSG_ADD_CAM_FLOW_ENTRY           = 20,    /* Alias: UNSOL_MSG_ADD_CAM_ENTRY                */
    CLNK_MBX_UNSOL_MSG_DELETE_CAM_FLOW_ENTRIES      = 21,    /* Alias: UNSOL_MSG_DELETE_CAM_FLOW_ENTRIES      */

#if ECFG_FLAVOR_VALIDATION==1
    CLNK_MBX_UNSOL_MSG_VAL_ISOC_EVENT               = 22,    /* Alias: UNSOL_MSG_VAL_ISOC_EVENT               */
#endif
#if FEATURE_FEIC_PWR_CAL
    CLNK_MBX_UNSOL_MSG_FEIC_STATUS_READY            = 23,    /* Alias: UNSOL_MSG_FEIC_STATUS_READY_TYPE          */
#endif

    CLNK_MBX_MAX_UNSOL_MSG_TYPE // This must always be last
}
CLNK_MBX_UNSOL_MSG_TYPES;

// Maximum Message Structure
typedef struct
{
    SYS_UINT32 msg[MAX_MBX_MSG];
}
Clnk_MBX_MaxMsg_t;

// Ethernet Command Structure
typedef struct
{
    SYS_UINT32 cmd;
    SYS_UINT32 param[7];
}
Clnk_MBX_EthCmd_t;

// Ethernet Reply Structure
typedef struct
{
    SYS_UINT32 status;
    SYS_UINT32 param[7];
}
Clnk_MBX_EthReply_t;

// MPEG Command Structure
typedef struct
{
    SYS_UINT32  cmd;
    SYS_UINT32  param[7];
}
Clnk_MBX_MpegCmd_t;

// MPEG Reply Structure
typedef struct
{
    SYS_UINT32 status;
    SYS_UINT32 param;
}
Clnk_MBX_MpegReply_t;

// Generic Mailbox Message Structure
typedef struct
{
    union
    {
        Clnk_MBX_MaxMsg_t           maxMsg;
        SYS_UINT32                  cmdCode;
        SYS_UINT32                  replyStatus;

        // Ethernet mailbox command
        Clnk_MBX_EthCmd_t           ethCmd;

        // Ethernet mailbox reply
        Clnk_MBX_EthReply_t         ethReply;

        // MPEG mailbox command
        Clnk_MBX_MpegCmd_t          mpegCmd;

        // MPEG mailbox reply
        Clnk_MBX_MpegReply_t        mpegReply;
    }
    msg;
}
Clnk_MBX_Msg_t;

// Reply and Unsolicited Ready Callback Function
typedef void (*Clnk_MBX_RdyCallback)(void* pvParam, Clnk_MBX_Msg_t* pMsg);
typedef int (*Clnk_MBX_SwUnsolRdyCallback)(void* pvParam, Clnk_MBX_Msg_t* pMsg);

// Command Queue Entry Structure
typedef struct
{
    Clnk_MBX_Msg_t          sendMsg;
    Clnk_MBX_Msg_t          rcvMsg;
    SYS_UINT8               transID;
    volatile int            isReplyRdy;
    volatile int            isReplyTimedOut;
    void                    *msg_wqt ;
 //   struct hostos_timer     msg_timer;
 //   wait_queue_head_t       msg_wq;
} Clnk_MBX_CmdQueueEntry_t;

// Unsolicited Queue Entry Structure
typedef struct
{
    Clnk_MBX_Msg_t msg;
    SYS_UINT8      transID;
}
Clnk_MBX_UnsolQueueEntry_t;

// Sw Unsolicited Queue Entry Structure
typedef struct
{
    Clnk_MBX_Msg_t msg;
}
Clnk_MBX_SwUnsolQItem_t;

// Software unsolicited Queue Entry Structure
typedef struct
{
    SYS_UINT32     ownership;
    SYS_UINT32     transID;
    SYS_UINT32     msg[MAX_UNSOL_MSG];
}
Clnk_MBX_SwUnsolQueueEntry_t;


// Mailbox Structure
typedef struct
{
    void                          *dc_ctx; // driver control ctx
    SYS_UINT32                    type;
    SYS_UINT32                    mbxOpen;

    // Callback functions
    Clnk_MBX_RdyCallback          replyRdyCallback;
    void                          *replyParam;
    Clnk_MBX_SwUnsolRdyCallback   swUnsolRdyCallback;
    void                          *swUnsolParam;

    // Command queue
    Clnk_MBX_CmdQueueEntry_t      cmdQueue[CLNK_MBX_CMD_QUEUE_SIZE];
    SYS_UINT8                     cmdHeadIndex;   // dequeue point
    SYS_UINT8                     cmdTailIndex;   // enqueue point
    SYS_UINT8                     cmdCurrTransID;
    void                          *mbx_lock;      // referenced here from GPL side

    // Software unsolicited queue
    SYS_UINT32                    swUnsolQueueSlaveMap;
    SYS_UINT32                    pSwUnsolQueue; // ptr to array of Clnk_MBX_SwUnsolQueueEntry_t in pci space
    SYS_UINT32                    swUnsolQueueSize;
    SYS_UINT8                     swUnsolReadIndex;
    SYS_UINT8                     swUnsolCurrTransID;
    Clnk_MBX_SwUnsolQItem_t       swUnsolQueue[CLNK_MBX_SWUNSOL_QUEUE_SIZE];
    SYS_UINT8                     swUnsolHeadIndex;
    SYS_UINT8                     swUnsolTailIndex;
    SYS_UINT8                     swUnsolCount;
    SYS_UINT8                     swUnsolOverrun;
    void                          *swUnsolLock;     // referenced here from GPL side

    // Register offsets and bit masks
    SYS_UINT32                    writeMbxCsrOffset;
    SYS_UINT32                    readMbxCsrOffset;
    SYS_UINT32                    writeMbxRegOffset;
    SYS_UINT32                    readMbxRegOffset;
    SYS_UINT32                    writeMbxSize;
    SYS_UINT32                    readMbxSize;
}
Clnk_MBX_Mailbox_t;


int Clnk_MBX_Initialize(Clnk_MBX_Mailbox_t *pMbx, void *dcctx, SYS_UINT32 type);
void Clnk_MBX_Free_wqts( Clnk_MBX_Mailbox_t *pMbx );
void Clnk_MBX_Terminate(Clnk_MBX_Mailbox_t* pMbx);
int Clnk_MBX_Control(Clnk_MBX_Mailbox_t* pMbx, 
                     int option, 
                     SYS_UINTPTR reg,
                     SYS_UINTPTR val, 
                     SYS_UINTPTR length);
void Clnk_MBX_HandleInterrupt(Clnk_MBX_Mailbox_t* pMbx, SYS_UINT32 pciIntStatus);
int Clnk_MBX_SendMsg(Clnk_MBX_Mailbox_t *pMbx, 
                     Clnk_MBX_Msg_t     *pMsg,
                     SYS_UINT8          *pTransID, 
                     SYS_UINT32          len);
int Clnk_MBX_RcvUnsolMsg(Clnk_MBX_Mailbox_t *pMbx, 
                         SYS_UINT32         *pbuf);
                   
int Clnk_MBX_SendRcvMsg(Clnk_MBX_Mailbox_t  *pMbx, 
                        Clnk_MBX_Msg_t      *pSendMsg,
                        Clnk_MBX_Msg_t      *pRcvMsg, 
                        SYS_UINT32          len,
                        SYS_UINT32          timeoutInUs);
void Clnk_MBX_Read_ISR( Clnk_MBX_Mailbox_t *pMbx );
void Clnk_MBX_Write_ISR( Clnk_MBX_Mailbox_t *pMbx );
void Clnk_MBX_Unsol_ISR( Clnk_MBX_Mailbox_t* pMbx );
int Clnk_MBX_Read_Hw_Mailbox_Check_Ready(Clnk_MBX_Mailbox_t* pMbx);
int Clnk_MBX_Write_Hw_Mailbox_Check_Ready(Clnk_MBX_Mailbox_t* pMbx);
int Clnk_MBX_Send_Mailbox_Check_Empty(Clnk_MBX_Mailbox_t* pMbx);
int Clnk_MBX_Unsol_Hw_Mailbox_Check_Ready(Clnk_MBX_Mailbox_t* pMbx);
#if defined(E1000_DRVR_SUPPORT) || defined(CANDD_DRVR_SUPPORT) || defined(CONFIG_TIVO)
int Clnk_Kern_Task_Init(void *data);
int Clnk_Kern_Task_Stop(void *data);
int Clnk_Kern_Task_Kill(void *data);
#endif

#endif /* __ClnkMbx_dvr_h__ */

