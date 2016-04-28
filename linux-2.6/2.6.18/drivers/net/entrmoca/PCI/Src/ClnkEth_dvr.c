/*******************************************************************************
*
* PCI/Src/ClnkEth_dvr.c
*
* Description: Common Ethernet Module
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

#if (MONITOR_SWAPS)
SYS_UINT32 rrrClink, wwwClink, rrrSwap, wwwSwap;
#endif

// Skip reset constants
#define NOT_START                 0xffffffff
#define MAX_RESET_NUM             5
#define RESET_CHECK_INTERVAL      15
#if (NETWORK_TYPE_ACCESS)
#define RESET_SKIP_INTERVAL       60
#else
#define RESET_SKIP_INTERVAL       0
#endif

// Firmware Section Type Constants
#define SECTION_TYPE_EMBEDDED_SW  0
#define SECTION_TYPE_TC_MICROCODE 1
#define SECTION_TYPE_MACPHY_REG   2
#define SECTION_TYPE_COMMENT      4
#define SECTION_TYPE_VERSION      5

// Link Down Constants
#if (NETWORK_TYPE_ACCESS)
#define MAX_LINK_DOWN_COUNT       0
#else
#define MAX_LINK_DOWN_COUNT       0
#endif

/*******************************************************************************
*                             U t i l i t y                                    *
********************************************************************************/

/*******************************************************************************
*                             D a t a   T y p e s                              *
********************************************************************************/

/*******************************************************************************
*                             C o n s t a n t s                                *
********************************************************************************/

/* None */

/*******************************************************************************
*                             G l o b a l   D a t a                            *
********************************************************************************/

#if ECFG_FLAVOR_VALIDATION==1
eclair_ValMboxHostCounts_t clnkEth_valMboxHostCounts;
#endif

/*******************************************************************************
*                       M e t h o d   P r o t o t y p e s                      *
********************************************************************************/

static int StopDevice(dc_context_t *dccp);

/*******************************************************************************
*                      BUS HAL   D e f i n i t i o n s                         *
********************************************************************************/

#if ECFG_CHIP_ZIP1
#if CLNK_BUS_TYPE == DATAPATH_PCI
#include "ClnkBusPCI_dvr.c"
#else
#include "ClnkBus68K_dvr.c"
#endif
#else  
#include "ClnkBusPCI2_dvr.c"
#endif /* ECFG_CHIP_ZIP1 */

/*******************************************************************************
*                      M e t h o d   D e f i n i t i o n s                     *
********************************************************************************/


/*
@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
@                                                                              @
@                        P u b l i c  M e t h o d s                            @
@                                                                              @
@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
*/

/*******************************************************************************
*
* Public method:    Clnk_ETH_Initalize()
*
* Description:      Initializes the Common Ethernet Module.
*
* Inputs:
*       vcp            - driver control context pointer
*       fw_setup       - Eth hw stuff
*                        (Number of receive host descriptors)
*       hparams        - box of stuff
*
* Outputs:
*       CLNK_ETH_RET_CODE_SUCCESS
*
*******************************************************************************/
int Clnk_ETH_Initialize_drv(void                  *vcp, 
                            Clnk_ETH_Firmware_t   *fw_setup,
                            struct clnk_hw_params *hparams )
{
    dc_context_t *dccp = vcp;

#if ECFG_FLAVOR_VALIDATION==1
    INCTYPES_SAFE_VAR_ZERO(clnkEth_valMboxHostCounts);
#endif

    // Initialize context
    dccp->stats.socResetCount = (SYS_UINT32)-1;
    dccp->fw                  = *fw_setup;

    // Allocate and initialize host tx/rx lists and descriptors
    clnk_hw_init(dccp, hparams);

    // init the rxList and rxFreeList before clnketh_open is called
    clnk_hw_desc_init(dccp);

    {
        SYS_UINT32 macAddrHigh = fw_setup->macAddrHigh;
        SYS_UINT32 macAddrLow  = fw_setup->macAddrLow;

        if ((macAddrHigh == 0) && (macAddrLow == 0))
        {
            clnk_reg_read(dccp, CLNK_REG_ETH_MAC_ADDR_HIGH, &macAddrHigh);
            clnk_reg_read(dccp, CLNK_REG_ETH_MAC_ADDR_LOW,  &macAddrLow);
            fw_setup->macAddrHigh = macAddrHigh; // Side Effect !?
            fw_setup->macAddrLow  = macAddrLow; // Side Effect !?
        }
    }
 
#if (NETWORK_TYPE_ACCESS)
    // Any Access mode subsystems
    CLNK_Access_Init(&(dccp->accessBlk));
#endif

    HostOS_PrintLog(L_DEBUG, "Clnk_ETH_Initialize_drv: FW options init done\n");
    HostOS_PrintLog(L_DEBUG, "    scanMask    = %08x\n", dccp->fw.scanMask); 
    HostOS_PrintLog(L_DEBUG, "    productMask = %08x\n", dccp->fw.productMask);
    HostOS_PrintLog(L_DEBUG, "    channelMask = %08x\n", dccp->fw.channelMask);
    HostOS_PrintLog(L_DEBUG, "    channelPlan = %d\n",   dccp->fw.channelPlan);
    HostOS_PrintLog(L_DEBUG, "    bias        = %d\n",   dccp->fw.bias);       
    HostOS_PrintLog(L_DEBUG, "    lof         = %d\n",   dccp->fw.lof);
    HostOS_PrintLog(L_DEBUG, "    cmRatio     = %03x\n", dccp->fw.cmRatio);
    HostOS_PrintLog(L_DEBUG, "    tabooInfo   = %08x\n", dccp->fw.tabooInfo);

    return (CLNK_ETH_RET_CODE_SUCCESS);
}

/********************************************************************
* Function Name: clnkrst_init_drv
*
* Description:  Initializes driver context associated with the cLink device
*
* Parameters:   vcp - pointer to device context structure
*
* Returns:      None
*   
*********************************************************************/
void clnkrst_init_drv(void *vcp)
{
    dc_context_t *dccp = vcp;
    SYS_UINT32 socResetCount;
    SYS_UINT32 socResetHistory;

    // Maintain variables across reset
    socResetCount   = dccp->stats.socResetCount;
    socResetHistory = dccp->stats.socResetHistory;

    // Clear context variables
    dccp->pciIntStatus          = 0;
    dccp->rxHaltStatus          = 0;
    dccp->txHaltStatus          = 0;
    HostOS_Memset(&dccp->stats, 0, sizeof(ClnkDef_EthStats_t));
    if (!dccp->fs_reset)
        socResetCount++;
    dccp->stats.socResetCount   = socResetCount;
    dccp->stats.socResetHistory = socResetHistory;
    dccp->socStatus             = CLNK_DEF_SOC_STATUS_SUCCESS;
    dccp->socStatusEmbedded     = 0;
    dccp->socStatusInProgress   = 0;
    dccp->socStatusLastTransID  = 0;
    dccp->socStatusTimeoutCnt   = 0;
    dccp->socStatusLinkDownCnt  = 0;
    dccp->socStatusTxHaltedCnt  = 0;
    dccp->socStatusLastTxCnt    = 0;
    dccp->txAttempts            = 0;
    dccp->txCompletions         = 0;
    dccp->pSwUnsolQueue         = 0;
    dccp->swUnsolQueueSize      = SW_UNSOL_HW_QUEUE_SIZE;
    dccp->eppDataValid          = SYS_FALSE;
    if (dccp->evmData) {
        HostOS_Free(dccp->evmData, sizeof(ClnkDef_EvmData_t));
        dccp->evmData           = (ClnkDef_EvmData_t *) SYS_NULL;
    }

    // Initialize CPU semaphores
    SEM_INIT_DONE_BITS();
}

/*******************************************************************************
*
* Public method:        Clnk_ETH_Terminate_drv()
*
********************************************************************************
*
* Description:
*       Terminates the Common Ethernet Module.
*
* Inputs:
*       vcp*  Pointer to the context
*
* Outputs:
*       None
*
* Notes:
*       None
*
*
*******************************************************************************/
void Clnk_ETH_Terminate_drv(void* vcp)
{
    dc_context_t *dccp = vcp;

    if (dccp == SYS_NULL)
    {
        return;
    }

    // Initialize hardware registers to reset device
    StopDevice(dccp);

    clnk_hw_term(dccp);

#if defined(CLNK_ETH_BRIDGE) 
    // Terminte the transparent bridging module, if required
    if (dccp->bridgeInitialized)
    {
        Cam_Terminate(&dccp->cam);
    }
#endif

    // Terminate the mailbox module, if required
    if (dccp->mailboxInitialized)
    {
        Clnk_MBX_Terminate(&dccp->mailbox);
    }

#if (NETWORK_TYPE_ACCESS)
    CLNK_Access_Terminate(&(dccp->accessBlk));
#endif

    // Free any EVM or EPP probe allocations
    if (dccp->evmData) HostOS_Free(dccp->evmData, sizeof(ClnkDef_EvmData_t));

    // Free memory for context
    HostOS_Free(dccp, sizeof(dc_context_t));
}

/*******************************************************************************
*
* Public method:        Clnk_ETH_Stop()
*
********************************************************************************
*
* Description:
*       Gracefully shuts down the c.LINK device and puts it in a low power mode.
*
* Inputs:
*       vcp*  Pointer to the context
*
* Outputs:
*       CLNK_ETH_RET_CODE_SUCCESS
*       CLNK_ETH_RET_CODE_RESET_ERR
*
*
*******************************************************************************/
int Clnk_ETH_Stop(void* vcp)
{
    dc_context_t *dccp = vcp;

    if (StopDevice(dccp) != SYS_SUCCESS)
    {
        return (CLNK_ETH_RET_CODE_RESET_ERR);
    }

    return (CLNK_ETH_RET_CODE_SUCCESS);
}

/*******************************************************************************
*
* Public method:        Clnk_ETH_Open()
*
********************************************************************************
*
* Description:
*       Brings up the Common Ethernet Module.
*
* Inputs:
*       vcp*  Pointer to the context
*
* Outputs:
*       CLNK_ETH_RET_CODE_SUCCESS
*
* Notes:
*       None
*
*
*******************************************************************************/

int Clnk_ETH_Open(void* vcp)
{
    dc_context_t *dccp = vcp;

    return(clnk_hw_open(dccp));
}

/*******************************************************************************
*
* Public method:        Clnk_ETH_Close()
*
********************************************************************************
*
* Description:
*       Shuts down the Common Ethernet Module.
*
* Inputs:
*       dccp*  Pointer to the context
*
* Outputs:
*       CLNK_ETH_RET_CODE_SUCCESS
*
* Notes:
*       None
*
*
*******************************************************************************/

int Clnk_ETH_Close(void* vcp)
{
    dc_context_t *dccp = vcp;

    return(clnk_hw_close(dccp));
}

/*******************************************************************************
*
* Public method:        Clnk_ETH_HandleRxInterrupt()
*
********************************************************************************
*
* Description:
*       Handles the receive interrupt for the common Ethernet Module.
*
* Inputs:
*       vcp*  Pointer to the context
*
* Outputs:
*       None
*
* Notes:
*       When handling interrupts for the Common Ethernet Module, all
*       interrupt handlers in this module must be called and in the
*       following order:
*           1) Clnk _ETH_Control(CLNK_ETH_CTRL_RD_CLR_INTERRUPT)
*           2) Clnk _ETH_HandleRxInterrupt()
*           3) Clnk _ETH_HandleTxInterrupt()
*           4) Clnk _ETH_HandleMiscInterrupt()
*
*
*******************************************************************************/

void Clnk_ETH_HandleRxInterrupt(void* vcp)
{
    dc_context_t *dccp = vcp;

    clnk_hw_rx_intr(dccp);
}

/*******************************************************************************
*
* Public method:        Clnk_ETH_HandleTxInterrupt()
*
********************************************************************************
*
* Description:
*       Handles the transmit interrupt for the Common Ethernet Module.
*
* Inputs:
*       vcp*  Pointer to the context
*
* Outputs:
*       None
*
* Notes:
*       When handling interrupts for the Common Ethernet Module, all
*       interrupt handlers in this module must be called and in the
*       following order:
*           1) Clnk _ETH_Control(CLNK_ETH_CTRL_RD_CLR_INTERRUPT)
*           2) Clnk _ETH_HandleRxInterrupt()
*           3) Clnk _ETH_HandleTxInterrupt()
*           4) Clnk _ETH_HandleMiscInterrupt()
*
*
*******************************************************************************/

void Clnk_ETH_HandleTxInterrupt(void* vcp)
{
    dc_context_t *dccp = vcp;

    clnk_hw_tx_intr(dccp);
}

/*******************************************************************************
*
* Public method:        Clnk_ETH_HandleMiscInterrupt()
*
********************************************************************************
*
* Description:
*       Handles the miscellaneous interrupts for the Common Ethernet Module.
*
* Inputs:
*       vcp*  Pointer to the context
*
* Outputs:
*       None
*
* Notes:
*       When handling interrupts for the Common Ethernet Module, all
*       interrupt handlers in this module must be called and in the
*       following order:
*           1) Clnk _ETH_Control(CLNK_ETH_CTRL_RD_CLR_INTERRUPT)
*           2) Clnk _ETH_HandleRxInterrupt()
*           3) Clnk _ETH_HandleTxInterrupt()
*           4) Clnk _ETH_HandleMiscInterrupt()
*
*
*******************************************************************************/

void Clnk_ETH_HandleMiscInterrupt(void* vcp)
{
    dc_context_t *dccp = vcp;
    SYS_UINT32 statusVal;

    if(! (dccp->intr_enabled && dccp->socBooted))
        return;

    statusVal = 0 ;

    clnk_hw_misc_intr(dccp, &statusVal);

    // Handle mailbox interrupt
    Clnk_MBX_HandleInterrupt(&dccp->mailbox, statusVal);
}

/*******************************************************************************
*
* Public method:        Clnk_ETH_SendPacket()
*
********************************************************************************
*
* Description:
*       Sends a packet over the C.Link network.
*
* Inputs:
*       vcp*                  Pointer to the context
*       Clnk_ETH_FragList_t*  List of fragments for the packet
*       SYS_UINT32            Priority of the packet
*       SYS_UINT32            ID of the packet
*       SYS_UINT32            Flags for the packet (currently unused)
*
* Outputs:
*       CLNK_ETH_RET_CODE_SUCCESS
*       CLNK_ETH_RET_CODE_NOT_OPEN_ERR
*       CLNK_ETH_RET_CODE_LINK_DOWN_ERR
*       CLNK_ETH_RET_CODE_NO_HOST_DESC_ERR
*       CLNK_ETH_RET_CODE_PKT_LEN_ERR
*
* Notes:
*       None
*
*
*******************************************************************************/

int Clnk_ETH_SendPacket(void* vcp, Clnk_ETH_FragList_t* pFragList,
                        SYS_UINT32 priority, SYS_UINT32 packetID, 
                        SYS_UINT32 flags)
{
    dc_context_t *dccp = vcp;
#ifdef CLNK_ETH_LINK_STATUS
    SYS_UINT32    linkStatus;

    // NOTE: possibly unnecessary since netif_carrier_on() or off()
    // Check for valid link state
    GET_LINK_STATUS_REG(dccp, &linkStatus);
    if (!IS_LINK_STATUS_UP(linkStatus))
    {
        return (CLNK_ETH_RET_CODE_LINK_DOWN_ERR);
    }
#endif
    return(clnk_hw_tx_pkt(dccp, pFragList, priority, packetID, flags));
}

/*******************************************************************************
*
* Public method:        Clnk_ETH_RcvPacket()
*
********************************************************************************
*
* Description:
*       Receives a packet form the C.Link network.
*
* Inputs:
*       vcp*                  Pointer to the context
*       Clnk_ETH_FragList_t*  List of fragments for the packet
*       SYS_UINT32            List number for the packet
*       SYS_UINT32            ID of the packet
*       SYS_UINT32            Flags for the packet
*
* Outputs:
*       CLNK_ETH_RET_CODE_SUCCESS
*       CLNK_ETH_RET_CODE_GEN_ERR
*       CLNK_ETH_RET_CODE_NO_HOST_DESC_ERR
*
* Notes:
*       None
*
*
*******************************************************************************/

int Clnk_ETH_RcvPacket(void* vcp, Clnk_ETH_FragList_t* pFragList,
                       SYS_UINT32 listNum, SYS_UINT32 packetID, SYS_UINT32 flags)
{
    dc_context_t *dccp = vcp;

    return(clnk_hw_rx_pkt(dccp, pFragList, listNum, packetID, flags));
}

/*******************************************************************************
*
* Public method:        Clnk_ETH_IsSoCBooted()
*
********************************************************************************
*
* Description:
*       Brings up the Common Ethernet Module.
*
* Inputs:
*       vcp*  Pointer to the context
*
* Outputs:
*       0 means SoC is not booted
*       1 means Soc is booted
* Notes:
*       None
*
*
*******************************************************************************/

int Clnk_ETH_IsSoCBooted(void* vcp)
{
    dc_context_t *dccp = vcp;
#if CLNK_BUS_TYPE == DATAPATH_PCI
    return(dccp->socBooted);
#else
    return SYS_FALSE;
#endif
}


/**
 * \brief Callback function for HOST_ORIG mailbox messages
 *
 * This function is called from the ClnkMbx read interrupt when a mailbox
 * reply to a HOST_ORIG message is received.
 *
 * The only message we actually process in this callback is the
 * ETH_MB_GET_STATUS / CLNK_MBX_ETH_GET_STATUS_CMD response.
 * socStatusEmbedded is used later in Get SocStatus().
 *
 * \param[in] dccp void pointer to our dc_context_t
 * \param[in] pMsg Struct containing newly received message
 *
 ******************************************************************************/
void MbxReplyRdyCallback(void* vcp, Clnk_MBX_Msg_t* pMsg)
{
    dc_context_t *dccp = vcp;
    SYS_UINT8 transID;

    transID = (SYS_UINT8)CLNK_MBX_GET_TRANS_ID(pMsg->msg.ethReply.status);
    if (transID == dccp->socStatusLastTransID)
    {
        /*
         * This reply comes from ETH_MB_GET_STATUS in the embedded.
         * If there was a reset condition, it copied the reset reason into
         * param[0].  Otherwise, it wrote 0.
         */
        if (pMsg->msg.ethReply.param[0] != 0)
        {
            dccp->socStatusEmbedded =
                CLNK_DEF_SOC_STATUS_EMBEDDED_FAILURE |
                ((pMsg->msg.ethReply.param[0] & 0xffff) << 16);
        }
        dccp->socStatusInProgress  = SYS_FALSE;
        dccp->socStatusLastTransID = 0;
    }

#if ECFG_CHIP_ZIP1
    if (transID == dccp->txFifoResizeTransId)
    {
        dccp->txFifoResizeTransId = -1UL;  // Invalidate transaction ID
        //HostOS_PrintLog(L_DEBUG, "MbxRdyReplyCallback: tfifo resz reply=%08x\n",
        //                pMsg->msg.maxMsg.msg[0]);

        // Invoke host OS callback for unthrottling of c.LINK net i/f
        if (dccp->txFifoResizeStatus == TX_FIFO_RESIZE_STS_DRAINED)
        {
            dccp->txFifoResizeStatus = TX_FIFO_RESIZE_STS_DONE;
            dccp->tFifoResizeCallback(dccp->p_dk_ctx);
        }
    }
#endif /* ECFG_CHIP_ZIP1 */
}

/**
 * Callback function for unsolicited mailbox messages
 *
 * This function is called from the ClnkMbx read interrupt when an unsolicited
 * message appears on the SW unsol queue.
 *
 * Unsolicited messages poked into the hardware mailbox CSRs are unsupported
 * and should never be produced by the embedded.
 * 
 * vcp       - void pointer to control context
 * pMsg      - Struct containing newly received message in DRIVER SPACE
 *             Some of the cases in this function modify this message
 *
 * Returns 1 for message consumed. 0 for message NOT consumed.
 *
 ******************************************************************************/
int MbxSwUnsolRdyCallback(void* vcp, Clnk_MBX_Msg_t* pMsg)
{
    dc_context_t *dccp = vcp;
    int consumed = 1 ;              // message consumed flag

    switch( pMsg->msg.maxMsg.msg[0] )
    {
        case CLNK_MBX_UNSOL_MSG_ADMISSION_STATUS:
        case CLNK_MBX_UNSOL_MSG_BEACON_STATUS:
            consumed = 0 ;
            break;
        case CLNK_MBX_UNSOL_MSG_RESET:
            consumed = 0 ;
            break;

#if defined(CLNK_ETH_BRIDGE) 
        case CLNK_MBX_UNSOL_MSG_UCAST_PUB:
            Cam_RcvCarousel(&dccp->cam,
                            pMsg->msg.maxMsg.msg[1], // hi
                            pMsg->msg.maxMsg.msg[2], // lo
                            pMsg->msg.maxMsg.msg[3]); // node
            break;
#endif

#if defined(CLNK_ETH_BRIDGE) 
        case CLNK_MBX_UNSOL_MSG_UCAST_UNPUB:
            Cam_RcvUnpublish(&dccp->cam,
                            pMsg->msg.maxMsg.msg[1], // hi
                            pMsg->msg.maxMsg.msg[2], // lo
                            pMsg->msg.maxMsg.msg[3]); // node
            break;
#endif

        case CLNK_MBX_UNSOL_MSG_NODE_ADDED:
            {
                SYS_UINT32 macHigh = pMsg->msg.maxMsg.msg[1];
                SYS_UINT32 macLow  = pMsg->msg.maxMsg.msg[2];
                SYS_UINT32 nodeId  = pMsg->msg.maxMsg.msg[3];
                Cam_AddNode(&dccp->cam, macHigh, macLow, nodeId);

                // Setup SW Queue with Guaranteed and Peak Token Buffers.
                clnk_hw_bw_setup(dccp, nodeId, macHigh, macLow);
            }
            break;

        case CLNK_MBX_UNSOL_MSG_NODE_DELETED:
            {
                SYS_UINT32 nodeId = pMsg->msg.maxMsg.msg[1];
                Cam_DelNode(&dccp->cam, nodeId);
                clnk_hw_bw_teardown(dccp, nodeId);
            }
            break; 

#ifdef CLNK_ETH_PHY_DATA_LOG
        case CLNK_MBX_UNSOL_MSG_EVM_DATA_READY:
            if (dccp->evmData)
            {   // Should skip this if User Copy in Progress.
                dccp->evmData->valid = SYS_FALSE;
                dccp->evmData->NodeId = pMsg->msg.maxMsg.msg[1];
                clnk_blk_read(dccp, pMsg->msg.maxMsg.msg[2], dccp->evmData->Data, 1024);
                dccp->evmData->valid = SYS_TRUE;
            }
//            consumed = 0 ;
            break;
#endif

#if FEATURE_FEIC_PWR_CAL
        // for PCI
        case CLNK_MBX_UNSOL_MSG_FEIC_STATUS_READY:
        {
            int i, j, lshift;
            SYS_UINT32 dbg_mask;
            SYS_INT32 feic_profileID;
            SYS_INT32 feicdata_array[sizeof(FeicStatusData_t)/4]; //= 95 words (normal)
            FeicStatusData_t * feicstatus_data;
            SYS_UCHAR temp_array[4];
            SYS_UINT32 pfeicdata_src;

            clnk_reg_read(dccp, DEV_SHARED(dbgMask), &dbg_mask);
            clnk_reg_read(dccp, DEV_SHARED(feicProfileId), &feic_profileID);

            //Code below packs feic status data from SoC to host driver in a portable fashion,
            //such that we avoid any endianess issues.
            pfeicdata_src = pMsg->msg.maxMsg.msg[2];
            for (i = 0; i < sizeof(feicdata_array)/4; i++) 
            {
                clnk_reg_read(dccp, pfeicdata_src, (SYS_UINT32 *)temp_array); //copy one word from SoC
                for (j = 0; j < 4; j++) 
                {
                    lshift = 24 - 8 * (j - (j/4)*4);
                    feicdata_array[i] &= ~(0xff << lshift); //clear byte location
                    feicdata_array[i] |= ((temp_array[j] & 0xff) << lshift); //store value in location
                }
                pfeicdata_src += 4;
            }

            feicstatus_data = (FeicStatusData_t*)feicdata_array;
            
            // Display FEIC status (Internal usage only)
            if ((dbg_mask & 0x1) && (feic_profileID > 0))
            {
                // Display Target Power Status Variables
#define FS_FREQ_START   800000000
#define FS_FREQ_STEP    25000000
#define IDX_TO_FREQ(i)  (FS_FREQ_START + (i) * FS_FREQ_STEP)
                HostOS_PrintLog(L_INFO, "TempAdc: 0x%02x Freq: %4d MHz  TgtLsbBias: %3d  TgtDeltaAdc(+bias): 0x%02x  TgtPwr: %2d.%1d dBm \n\n",
                            feicstatus_data->targetPwrStatus.tempAdc,
                            IDX_TO_FREQ(feicstatus_data->targetPwrStatus.freqIndex) / 1000000,
                            feicstatus_data->targetPwrStatus.targetLsbBias,
                            feicstatus_data->targetPwrStatus.tLsbPlusTargetLsbBias,
                            (SYS_INT8)(feicstatus_data->targetPwrStatus.targetPwrDbmT10 / 10),
                            (feicstatus_data->targetPwrStatus.targetPwrDbmT10 < 0 ? 
                               (feicstatus_data->targetPwrStatus.targetPwrDbmT10 * -1) % 10:
                                feicstatus_data->targetPwrStatus.targetPwrDbmT10 % 10));
                    
                // Display Intermediate Detector Status Variables
                //Iteration 0 status variables (M0)
                if ((*(SYS_UINT32 *)&feicstatus_data->intermediateDetectorStatus.m0TxOn[0] != 0) ||
                    (*(SYS_UINT32 *)&feicstatus_data->intermediateDetectorStatus.m0TxOn[NUM_ADC_MEASUREMENTS-4] != 0))
                {
                    HostOS_PrintLog(L_INFO, "M0FeicAtt2: %2d  M0Feic_Att1: %2d  M0TxCalDecrN: %2d  M0TxdCGainSel: %2d \n",
                           feicstatus_data->intermediateDetectorStatus.m0FeicAtt2,
                           feicstatus_data->intermediateDetectorStatus.m0FeicAtt1,
                           feicstatus_data->intermediateDetectorStatus.m0RficTxCal,
                           feicstatus_data->intermediateDetectorStatus.m0TxdCGainSel);
                    for (i = 0; i < NUM_ADC_MEASUREMENTS; i += 16)
                    {
                        HostOS_PrintLog(L_INFO, "%s: %2x %2x %2x %2x %2x %2x %2x %2x %2x %2x %2x %2x %2x %2x %2x %2x\n",
                                        i == 0 ? "M0TxOn" : "      ",
                                        feicstatus_data->intermediateDetectorStatus.m0TxOn[i+0],
                                        feicstatus_data->intermediateDetectorStatus.m0TxOn[i+1],
                                        feicstatus_data->intermediateDetectorStatus.m0TxOn[i+2],
                                        feicstatus_data->intermediateDetectorStatus.m0TxOn[i+3],
                                        feicstatus_data->intermediateDetectorStatus.m0TxOn[i+4],
                                        feicstatus_data->intermediateDetectorStatus.m0TxOn[i+5],
                                        feicstatus_data->intermediateDetectorStatus.m0TxOn[i+6],
                                        feicstatus_data->intermediateDetectorStatus.m0TxOn[i+7],
                                        feicstatus_data->intermediateDetectorStatus.m0TxOn[i+8],
                                        feicstatus_data->intermediateDetectorStatus.m0TxOn[i+9],
                                        feicstatus_data->intermediateDetectorStatus.m0TxOn[i+10],
                                        feicstatus_data->intermediateDetectorStatus.m0TxOn[i+11],
                                        feicstatus_data->intermediateDetectorStatus.m0TxOn[i+12],
                                        feicstatus_data->intermediateDetectorStatus.m0TxOn[i+13],
                                        feicstatus_data->intermediateDetectorStatus.m0TxOn[i+14],
                                        feicstatus_data->intermediateDetectorStatus.m0TxOn[i+15]);
                    }      
                    HostOS_PrintLog(L_INFO, "M0AdcOn: %2d    M0AdcOff: %2d   M0DeltaAdcBelowMin:  %2d \n\n",
                           feicstatus_data->intermediateDetectorStatus.m0TxOnAvg,
                           feicstatus_data->intermediateDetectorStatus.m0TxOffAvg,
                           feicstatus_data->intermediateDetectorStatus.m0DeltaAdcBelowMin);
                }

                //Iteration 1 status variables (M1)
                if ((*(SYS_UINT32 *)&feicstatus_data->intermediateDetectorStatus.m1TxOn[0] != 0) ||
                    (*(SYS_UINT32 *)&feicstatus_data->intermediateDetectorStatus.m1TxOn[NUM_ADC_MEASUREMENTS-4] != 0))
                {
                    HostOS_PrintLog(L_INFO, "M1FeicAtt2: %2d  M1Feic_Att1: %2d  M1TxCalDecrN: %2d  M1TxdCGainSel: %2d \n",
                           feicstatus_data->intermediateDetectorStatus.m1FeicAtt2,
                           feicstatus_data->intermediateDetectorStatus.m1FeicAtt1,
                           feicstatus_data->intermediateDetectorStatus.m1RficTxCal,
                           feicstatus_data->intermediateDetectorStatus.m1TxdCGainSel);
                    for (i = 0; i < NUM_ADC_MEASUREMENTS; i += 16)
                    {
                        HostOS_PrintLog(L_INFO, "%s: %2x %2x %2x %2x %2x %2x %2x %2x %2x %2x %2x %2x %2x %2x %2x %2x\n",
                                        i == 0 ? "M1TxOn" : "      ",
                                        feicstatus_data->intermediateDetectorStatus.m1TxOn[i+0],
                                        feicstatus_data->intermediateDetectorStatus.m1TxOn[i+1],
                                        feicstatus_data->intermediateDetectorStatus.m1TxOn[i+2],
                                        feicstatus_data->intermediateDetectorStatus.m1TxOn[i+3],
                                        feicstatus_data->intermediateDetectorStatus.m1TxOn[i+4],
                                        feicstatus_data->intermediateDetectorStatus.m1TxOn[i+5],
                                        feicstatus_data->intermediateDetectorStatus.m1TxOn[i+6],
                                        feicstatus_data->intermediateDetectorStatus.m1TxOn[i+7],
                                        feicstatus_data->intermediateDetectorStatus.m1TxOn[i+8],
                                        feicstatus_data->intermediateDetectorStatus.m1TxOn[i+9],
                                        feicstatus_data->intermediateDetectorStatus.m1TxOn[i+10],
                                        feicstatus_data->intermediateDetectorStatus.m1TxOn[i+11],
                                        feicstatus_data->intermediateDetectorStatus.m1TxOn[i+12],
                                        feicstatus_data->intermediateDetectorStatus.m1TxOn[i+13],
                                        feicstatus_data->intermediateDetectorStatus.m1TxOn[i+14],
                                        feicstatus_data->intermediateDetectorStatus.m1TxOn[i+15]);
                    }      
                    HostOS_PrintLog(L_INFO, "M1AdcOn: %2d \n\n",
                           feicstatus_data->intermediateDetectorStatus.m1TxOnAvg);
                }

                //Iteration 2 status variables (M2)
                if ((*(SYS_UINT32 *)&feicstatus_data->intermediateDetectorStatus.m2TxOn[0] != 0) ||
                    (*(SYS_UINT32 *)&feicstatus_data->intermediateDetectorStatus.m2TxOn[NUM_ADC_MEASUREMENTS-4] != 0))
                {
                    HostOS_PrintLog(L_INFO, "M2FeicAtt2: %2d  M2Feic_Att1: %2d  M2TxCalDecrN: %2d  M2TxdCGainSel: %2d \n",
                           feicstatus_data->intermediateDetectorStatus.m2FeicAtt2,
                           feicstatus_data->intermediateDetectorStatus.m2FeicAtt1,
                           feicstatus_data->intermediateDetectorStatus.m2RficTxCal,
                           feicstatus_data->intermediateDetectorStatus.m2TxdCGainSel);
                    for (i = 0; i < NUM_ADC_MEASUREMENTS; i += 16)
                    {
                        HostOS_PrintLog(L_INFO, "%s: %2x %2x %2x %2x %2x %2x %2x %2x %2x %2x %2x %2x %2x %2x %2x %2x\n",
                                        i == 0 ? "M2TxOn" : "      ",
                                        feicstatus_data->intermediateDetectorStatus.m2TxOn[i+0],
                                        feicstatus_data->intermediateDetectorStatus.m2TxOn[i+1],
                                        feicstatus_data->intermediateDetectorStatus.m2TxOn[i+2],
                                        feicstatus_data->intermediateDetectorStatus.m2TxOn[i+3],
                                        feicstatus_data->intermediateDetectorStatus.m2TxOn[i+4],
                                        feicstatus_data->intermediateDetectorStatus.m2TxOn[i+5],
                                        feicstatus_data->intermediateDetectorStatus.m2TxOn[i+6],
                                        feicstatus_data->intermediateDetectorStatus.m2TxOn[i+7],
                                        feicstatus_data->intermediateDetectorStatus.m2TxOn[i+8],
                                        feicstatus_data->intermediateDetectorStatus.m2TxOn[i+9],
                                        feicstatus_data->intermediateDetectorStatus.m2TxOn[i+10],
                                        feicstatus_data->intermediateDetectorStatus.m2TxOn[i+11],
                                        feicstatus_data->intermediateDetectorStatus.m2TxOn[i+12],
                                        feicstatus_data->intermediateDetectorStatus.m2TxOn[i+13],
                                        feicstatus_data->intermediateDetectorStatus.m2TxOn[i+14],
                                        feicstatus_data->intermediateDetectorStatus.m2TxOn[i+15]);
                    }      
                    if ((*(SYS_UINT32 *)&feicstatus_data->intermediateDetectorStatus.m1TxOn[0] != 0) ||
                        (*(SYS_UINT32 *)&feicstatus_data->intermediateDetectorStatus.m1TxOn[NUM_ADC_MEASUREMENTS-4] != 0))
                    {
                        // display correct value for M1Error_0.25dB
                        HostOS_PrintLog(L_INFO, "M2AdcOn: %2d   M2DcOffset: %2d   M0Error_0.25dB: %2d  M1Error_0.25dB: %2d  M0M1ErrorSmall: %2d\n\n",
                               feicstatus_data->intermediateDetectorStatus.m2TxOnAvg,
                               feicstatus_data->intermediateDetectorStatus.m2DcOffset,
                               feicstatus_data->intermediateDetectorStatus.m0ErrorQtrDb,
                               feicstatus_data->intermediateDetectorStatus.m1ErrorQtrDb,
                               feicstatus_data->intermediateDetectorStatus.m0m1ErrorSmall);
                    }
                    else
                    {
                        // display N/A for M1Error_0.25dB
                        HostOS_PrintLog(L_INFO, "M2AdcOn: %2d   M2DcOffset: %2d   M0Error_0.25dB: %2d  M1Error_0.25dB: N/A  M0M1ErrorSmall: %2d\n\n",
                               feicstatus_data->intermediateDetectorStatus.m2TxOnAvg,
                               feicstatus_data->intermediateDetectorStatus.m2DcOffset,
                               feicstatus_data->intermediateDetectorStatus.m0ErrorQtrDb,
                               feicstatus_data->intermediateDetectorStatus.m0m1ErrorSmall);
                    }

                }

                //Iteration 3 status variables (M3)
                if ((*(SYS_UINT32 *)&feicstatus_data->intermediateDetectorStatus.m3TxOn[0] != 0) ||
                    (*(SYS_UINT32 *)&feicstatus_data->intermediateDetectorStatus.m3TxOn[NUM_ADC_MEASUREMENTS-4] != 0))
                {
                    HostOS_PrintLog(L_INFO, "M3FeicAtt2: %2d  M3Feic_Att1: %2d  M3TxCalDecrN: %2d  M3TxdCGainSel: %2d \n",
                           feicstatus_data->intermediateDetectorStatus.m3FeicAtt2,
                           feicstatus_data->intermediateDetectorStatus.m3FeicAtt1,
                           feicstatus_data->intermediateDetectorStatus.m3RficTxCal,
                           feicstatus_data->intermediateDetectorStatus.m3TxdCGainSel);
                    for (i = 0; i < NUM_ADC_MEASUREMENTS; i += 16)
                    {
                        HostOS_PrintLog(L_INFO, "%s: %2x %2x %2x %2x %2x %2x %2x %2x %2x %2x %2x %2x %2x %2x %2x %2x\n",
                                        i == 0 ? "M3TxOn" : "      ",
                                        feicstatus_data->intermediateDetectorStatus.m3TxOn[i+0],
                                        feicstatus_data->intermediateDetectorStatus.m3TxOn[i+1],
                                        feicstatus_data->intermediateDetectorStatus.m3TxOn[i+2],
                                        feicstatus_data->intermediateDetectorStatus.m3TxOn[i+3],
                                        feicstatus_data->intermediateDetectorStatus.m3TxOn[i+4],
                                        feicstatus_data->intermediateDetectorStatus.m3TxOn[i+5],
                                        feicstatus_data->intermediateDetectorStatus.m3TxOn[i+6],
                                        feicstatus_data->intermediateDetectorStatus.m3TxOn[i+7],
                                        feicstatus_data->intermediateDetectorStatus.m3TxOn[i+8],
                                        feicstatus_data->intermediateDetectorStatus.m3TxOn[i+9],
                                        feicstatus_data->intermediateDetectorStatus.m3TxOn[i+10],
                                        feicstatus_data->intermediateDetectorStatus.m3TxOn[i+11],
                                        feicstatus_data->intermediateDetectorStatus.m3TxOn[i+12],
                                        feicstatus_data->intermediateDetectorStatus.m3TxOn[i+13],
                                        feicstatus_data->intermediateDetectorStatus.m3TxOn[i+14],
                                        feicstatus_data->intermediateDetectorStatus.m3TxOn[i+15]);
                    }      
                    HostOS_PrintLog(L_INFO, "M3AdcOn: %2d    M3Error_0.25dB: %2d    M3ErrorSmall: %2d\n\n",
                           feicstatus_data->intermediateDetectorStatus.m3TxOnAvg,
                           feicstatus_data->intermediateDetectorStatus.m3ErrorQtrDb,
                           feicstatus_data->intermediateDetectorStatus.m3ErrorSmall);
                }

                //Iteration 4 status variables (M4)
                if ((*(SYS_UINT32 *)&feicstatus_data->intermediateDetectorStatus.m4TxOn[0] != 0) ||
                    (*(SYS_UINT32 *)&feicstatus_data->intermediateDetectorStatus.m4TxOn[NUM_ADC_MEASUREMENTS-4] != 0))
                {
                    HostOS_PrintLog(L_INFO, "M4FeicAtt2: %2d  M4Feic_Att1: %2d  M4TxCalDecrN: %2d  M4TxdCGainSel: %2d \n",
                           feicstatus_data->intermediateDetectorStatus.m4FeicAtt2,
                           feicstatus_data->intermediateDetectorStatus.m4FeicAtt1,
                           feicstatus_data->intermediateDetectorStatus.m4RficTxCal,
                           feicstatus_data->intermediateDetectorStatus.m4TxdCGainSel);
                    for (i = 0; i < NUM_ADC_MEASUREMENTS; i += 16)
                    {
                        HostOS_PrintLog(L_INFO, "%s: %2x %2x %2x %2x %2x %2x %2x %2x %2x %2x %2x %2x %2x %2x %2x %2x\n",
                                        i == 0 ? "M4TxOn" : "      ",
                                        feicstatus_data->intermediateDetectorStatus.m4TxOn[i+0],
                                        feicstatus_data->intermediateDetectorStatus.m4TxOn[i+1],
                                        feicstatus_data->intermediateDetectorStatus.m4TxOn[i+2],
                                        feicstatus_data->intermediateDetectorStatus.m4TxOn[i+3],
                                        feicstatus_data->intermediateDetectorStatus.m4TxOn[i+4],
                                        feicstatus_data->intermediateDetectorStatus.m4TxOn[i+5],
                                        feicstatus_data->intermediateDetectorStatus.m4TxOn[i+6],
                                        feicstatus_data->intermediateDetectorStatus.m4TxOn[i+7],
                                        feicstatus_data->intermediateDetectorStatus.m4TxOn[i+8],
                                        feicstatus_data->intermediateDetectorStatus.m4TxOn[i+9],
                                        feicstatus_data->intermediateDetectorStatus.m4TxOn[i+10],
                                        feicstatus_data->intermediateDetectorStatus.m4TxOn[i+11],
                                        feicstatus_data->intermediateDetectorStatus.m4TxOn[i+12],
                                        feicstatus_data->intermediateDetectorStatus.m4TxOn[i+13],
                                        feicstatus_data->intermediateDetectorStatus.m4TxOn[i+14],
                                        feicstatus_data->intermediateDetectorStatus.m4TxOn[i+15]);
                    }      
                    HostOS_PrintLog(L_INFO, "M4AdcOn: %2d    M4Error_0.25dB: %2d\n\n",
                           feicstatus_data->intermediateDetectorStatus.m4TxOnAvg,
                           feicstatus_data->intermediateDetectorStatus.m4ErrorQtrDb);
                }
            }
            
            if (dbg_mask & 0x1)
            {
                // Display Final Configuration Status Variables
                HostOS_PrintLog(L_INFO, "FeicProfileID: %d ProfFound:%s  CalDisabled: %s   CalBypassed: %s\n",
                            feicstatus_data->finalConfigStatus.feicProfileId,
                            (feicstatus_data->finalConfigStatus.feicFileFound ? "Yes" : "No"),
                            (feicstatus_data->finalConfigStatus.CalDisabled ? "Yes" : "No"),
                            (feicstatus_data->finalConfigStatus.CalBypassed ? "Yes" : "No"));
                HostOS_PrintLog(L_INFO, "FeicAtt2: %d      Att1: %d    TxCalDecrN: %d   TxdCGainSel: %2d\n",
                            feicstatus_data->finalConfigStatus.feicAtt2,
                            feicstatus_data->finalConfigStatus.feicAtt1,
                            feicstatus_data->finalConfigStatus.rficTxCal,
                            feicstatus_data->finalConfigStatus.txdCGainSel);
                if (!feicstatus_data->finalConfigStatus.CalDisabled && !feicstatus_data->finalConfigStatus.CalBypassed)
                {
                    HostOS_PrintLog(L_INFO, "TempClass: %s    TgtLsbBias: %3d   FeicPwrEst: %2d.%1d dBm  CalErr: %s\n\n",
                            (feicstatus_data->finalConfigStatus.tempClass ? "COM" : "IND"),
                            feicstatus_data->finalConfigStatus.targetLsbBias,       //signed
                            (SYS_INT8)(feicstatus_data->finalConfigStatus.pwrEstimateT10 / 10),
                            (feicstatus_data->finalConfigStatus.pwrEstimateT10 < 0 ? 
                               (feicstatus_data->finalConfigStatus.pwrEstimateT10 * -1) % 10:
                                feicstatus_data->finalConfigStatus.pwrEstimateT10 % 10),
                            (feicstatus_data->finalConfigStatus.calErr ? "Yes" : "No"));       
                }
                else
                {
                    HostOS_PrintLog(L_INFO, "TempClass: N/A   TgtLsbBias: N/A   FeicPwrEst: N/A          CalErr: N/A\n\n");       
                }
            }
            break;
        }
#endif

#ifdef CLNK_ETH_ECHO_PROFILE
         #define NUMBER_OF_RX_ECHO_PROBE_SYMBOLS 3
         #define EP_RX_PROBE_SIZE ((256 + NUMBER_OF_RX_ECHO_PROBE_SYMBOLS*(256 + 64)) * 4)
        case CLNK_MBX_UNSOL_MSG_ECHO_PROFILE_PROBE:
            dccp->eppData.NodeId = pMsg->msg.maxMsg.msg[1];
            clnk_blk_read(dccp, pMsg->msg.maxMsg.msg[2],
                             (SYS_UINT32 *)dccp->eppData.Data,
                             EP_RX_PROBE_SIZE);
            dccp->eppDataValid = SYS_TRUE;
//            consumed = 0 ;
            break;
#endif

#if (NETWORK_TYPE_ACCESS)
        case CLNK_MBX_UNSOL_MSG_ACCESS_CHK_MAC:
            {
            SYS_UINT8  transID;
            SYS_UINT32 status;
            SYS_UINT32 macHigh = pMsg->msg.maxMsg.msg[2];
            SYS_UINT32 macLow  = pMsg->msg.maxMsg.msg[3];
            status = Clnk_Access_FindMacBW(&dccp->accessBlk,
                                           macHigh, macLow, SYS_NULL, SYS_NULL);
       //   HostOS_PrintLog(L_INFO, "Check node %08x:%04x status %d\n", macHigh, macLow>>16, status);
            pMsg->msg.ethCmd.cmd  = CLNK_MBX_SET_CMD(CLNK_MBX_ETH_SET_CMD);
            pMsg->msg.ethCmd.param[2] = (status == SYS_SUCCESS);
            pMsg->msg.ethCmd.param[1] = pMsg->msg.maxMsg.msg[1];
            pMsg->msg.ethCmd.param[0] = (IO_NODEMANAGE) | 
                                        (IOSET_NODEMANAGE_CHK_MAC<<8),
            Clnk_MBX_SendMsg(&dccp->mailbox, pMsg, &transID, 4);
            }
//            consumed = 0 ;
            break;
#endif
        case CLNK_MBX_UNSOL_MSG_TABOO_INFO:
            HostOS_PrintLog(L_INFO, "FSUPDATE: Taboo Mask = 0x%06x, Offset = %d\n",
                            pMsg->msg.maxMsg.msg[1] >> 8,
                            pMsg->msg.maxMsg.msg[1] & 0x0ff);
 //           consumed = 0 ;
            break;

        case CLNK_MBX_UNSOL_MSG_FSUPDATE:
#if 0
            HostOS_PrintLog(L_INFO, 
                            "FSUPDATE: Pass = %2d, Tuned Freq = %4d MHz (%d)\n",
                            pMsg->msg.maxMsg.msg[1] >> 16,
                            800 +  ((pMsg->msg.maxMsg.msg[1] & 0x00ff) * 25),
                            (pMsg->msg.maxMsg.msg[1] >> 8) & 0x00ff);   
#endif
            /*
             * Sustain the link down count so that frequency scanning can
             * complete.
             */
            if (dccp->socStatusLinkDownCnt)
                dccp->socStatusLinkDownCnt--;
//            consumed = 0 ;
            break;

        case CLNK_MBX_UNSOL_MSG_ADD_CAM_FLOW_ENTRY:
            {
                SYS_UINT32 slot, node_id, flow_hi, flow_lo, cookie;
                SYS_UINT32 status;
                SYS_UINT8  transID;
                SYS_UINT32 packet_da_hi, packet_da_lo;
        
                flow_hi      = pMsg->msg.maxMsg.msg[1];
                flow_lo      = pMsg->msg.maxMsg.msg[2];
                slot         = pMsg->msg.maxMsg.msg[3] >> 16;
                node_id      = pMsg->msg.maxMsg.msg[3] & 0xffff;
                cookie       = pMsg->msg.maxMsg.msg[4];
                packet_da_hi = pMsg->msg.maxMsg.msg[5];
                packet_da_lo = pMsg->msg.maxMsg.msg[6];
                //HostOS_PrintLog(L_INFO, "slot %x node_id %x cookie %x flow_hi %x flow_lo %x\n", slot, node_id, cookie, flow_hi, flow_lo);
#if FEATURE_QOS
                qos_cam_program(&dccp->cam, slot, packet_da_hi, packet_da_lo, node_id);
#endif
                pMsg->msg.ethCmd.cmd  = CLNK_MBX_SET_CMD(CLNK_MBX_ETH_QFM_CAM_DONE);
                pMsg->msg.ethCmd.param[0] = cookie;
                status = Clnk_MBX_SendMsg(&dccp->mailbox, pMsg, &transID, 2);
            }
            break;

        case CLNK_MBX_UNSOL_MSG_DELETE_CAM_FLOW_ENTRIES:
            {
                SYS_UINT32 mask, cookie;
                SYS_UINT32 status;
                SYS_UINT8      transID;

                mask   = pMsg->msg.maxMsg.msg[1];
                cookie = pMsg->msg.maxMsg.msg[2];
                //HostOS_PrintLog(L_INFO, "mask %x cookie %x\n", mask, cookie);
#if FEATURE_QOS
                qos_cam_delete(&dccp->cam, mask);
#endif
                pMsg->msg.ethCmd.cmd  = CLNK_MBX_SET_CMD(CLNK_MBX_ETH_QFM_CAM_DONE);
                pMsg->msg.ethCmd.param[0] = cookie;
                status = Clnk_MBX_SendMsg(&dccp->mailbox, pMsg, &transID, 2);
            }
            break;

#if ECFG_FLAVOR_VALIDATION==1
        case CLNK_MBX_UNSOL_MSG_VAL_ISOC_EVENT:
        {
            clnkEth_valMboxHostCounts.triggeredIsocEvents++;
        }
//            consumed = 0 ;
            break ;
#endif

        default:
            consumed = 0 ;
            break;
    }

    return( consumed ) ;
}

/*
$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
$                                                                              $
$                         P r i v a t e  M e t h o d s                         $
$                                                                              $
$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
*/

/********************************************************************
* Function Name: StopDevice
* Parameters:
*   dccp - pointer to clnk context
* Description: 
*   Performs a controlled stop of a cLink device to ensure that no data
*   or interrupts are sent to the host from the SoC.
* Returns:
*   SYS_SUCCESS or a nonzero error code
* Notes:
*
*********************************************************************/
static int StopDevice(dc_context_t *dccp)
{
    void *dkcp = dc_to_dk( dccp );

    // perform graceful shutdown if possible
    HostOS_PrintLog(L_INFO, "c.LINK device stopping\n");
    HostOS_close(dkcp);

    // Disable interrupts
    Clnk_ETH_Control_drv(dccp, CLNK_ETH_CTRL_DISABLE_INTERRUPT, 0, 0, 0);

    return SYS_SUCCESS;
}

/* End of File: ClnkEth.c */

