/*******************************************************************************
*
* PCI/Src/Clnk_ctl_pci.c
*
* Description: 
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


static int do_clnk_ctl_dvr(dc_context_t *dccp, int cmd, struct clnk_io *io);


int clnk_ctl_drv(void *vdkcp, int cmd, struct clnk_io *io)
{
    dc_context_t *dccp = dk_to_dc( vdkcp ) ;

    switch (cmd)
    {
        case CLNK_CTL_NET_CARRIER_OK:
        {
            SYS_UINT32 * out = (void *)io->out;
            if (HostOS_netif_carrier_ok(vdkcp))
                *out = 1;
            else
                *out = 0;

            break;
        }

        case CLNK_CTL_RESET_DEVICE:
            HostOS_PrintLog(L_ALERT, "Clink Device Forced reset\n");
            HostOS_close(vdkcp);
            HostOS_PrintLog(L_ALERT, "Clink Link Down\n");

            // Disable interrupts
            Clnk_ETH_Control_drv(dccp, CLNK_ETH_CTRL_DISABLE_INTERRUPT, 0, 0, 0);

#if ECFG_CHIP_ZIP1
            clnk_hw_reg_init(dccp);
#endif // ECFG_CHIP_ZIP1

            // call ClnkReset.c's reset init
            clnkrst_init_drv(dccp);     // this is needed for subsequent resets...

            // Initialize the transparent bridiging module
            Cam_Init(&dccp->cam, &dccp->mailbox, dccp,
                     (dccp->fw.swConfig & CLNK_DEF_SW_CONFIG_SOFTCAM_BIT) != 0);
            dccp->bridgeInitialized = SYS_TRUE;

#if ECFG_CHIP_ZIP1
            clnk_hw_setup_timers(dccp); // only used for Zip1
#endif // ECFG_CHIP_ZIP1

            break;

        case CLNK_CTL_STOP_DEVICE:
            if (Clnk_ETH_Stop(dccp) != SYS_SUCCESS)
            {
                HostOS_PrintLog(L_ERR, "Halt failed\n");
                return(-SYS_INPUT_OUTPUT_ERROR);
            }
            break;

        case CLNK_CTL_NET_CARRIER_ON:
#if ECFG_CHIP_ZIP1
            clnk_hw_set_timer_spd(dccp, 1);
#endif // ECFG_CHIP_ZIP1
            HostOS_netif_carrier_on(vdkcp);
            HostOS_PrintLog(L_ALERT, "Clink Link Up\n");
            Cam_LookupSrc(&dccp->cam, dccp->fw.macAddrHigh, dccp->fw.macAddrLow);
            break;
        
        case CLNK_CTL_NET_CARRIER_OFF:
            HostOS_netif_carrier_off(vdkcp);
#if ECFG_CHIP_ZIP1
            clnk_hw_set_timer_spd(dccp, 0);
#endif
            break;
        
        case CLNK_CTL_SOC_INIT_BUS:
            socInitBus(dccp);
            break; 

        case CLNK_CTL_SET_DATA_PLANE_VARS:
        {
            ClnkDef_dataPlaneVars_t *in = (void *)io->in;
  
            if (io->in_len < sizeof( *in)) 
            {
                return(-SYS_INPUT_OUTPUT_ERROR);
                break;
            }

            dccp->fw.swConfig         = in->swConfig;
            dccp->fw.PQoSClassifyMode = in->pqosClassifyMode;
            HostOS_PrintLog(L_ALERT, "Clink Driver PQoS Classification Mode Changed [%d]\n", dccp->fw.PQoSClassifyMode);
            break;
        }
        
        case CLNK_CTL_SOC_BOOTED:
        {
            ClnkDef_dataPlaneVars_t *in = (void *)io->in;
            int                     stat ;

            if (io->in_len < sizeof( *in)) 
            {
                return(-SYS_INPUT_OUTPUT_ERROR);
                break;
            }
            dccp->fw.swConfig = in->swConfig;  // there may be other clink conf 
                                               // file variables that driver needs
            dccp->socBooted = SYS_TRUE;

            /*
             * Save the PQoS Classification Mode state in the device
             * driver struct so it can be used by the transmitter
             * to determine whether we are in MoCA 1.1 or Pre-UPnP
             * mode.
             */
            dccp->fw.PQoSClassifyMode = in->pqosClassifyMode;
            HostOS_PrintLog(L_ALERT, "Clink PQoS Classification Mode [%d]\n", 
                            dccp->fw.PQoSClassifyMode);

            // Initialize the mailbox module upon reset
            // SOC must be booted and interrupts enabled
            stat = Clnk_MBX_Initialize( &dccp->mailbox, dccp, CLNK_MBX_ETHERNET_TYPE );

            if (stat != SYS_SUCCESS)
            {
                HostOS_PrintLog(L_ERR, "Clink Reset MBX failed, stat=%d.\n", stat);
                return (stat);
            }
            dccp->mailboxInitialized = SYS_TRUE;
            Clnk_MBX_Control(&dccp->mailbox, CLNK_MBX_CTRL_SET_REPLY_RDY_CB,
                            0, (SYS_UINTPTR)MbxReplyRdyCallback, (SYS_UINTPTR)dccp);
            Clnk_MBX_Control(&dccp->mailbox, CLNK_MBX_CTRL_SET_SW_UNSOL_Q, 
                            0, in->pSwUnsolQueue, in->swUnsolQueueSize);
            Clnk_MBX_Control(&dccp->mailbox, CLNK_MBX_CTRL_SET_SW_UNSOL_RDY_CB,
                            0, (SYS_UINTPTR)MbxSwUnsolRdyCallback, (SYS_UINTPTR)dccp);

            // enable interrupts
            Clnk_ETH_Control_drv(dccp, CLNK_ETH_CTRL_ENABLE_INTERRUPT, 0, 0, 0);
            HostOS_open(vdkcp);
            break;
        }
        case CLNK_CTL_TC_DIC_INIT:
        {
            struct mb_return *mb = (void *)io->in;  
            if (io->in_len < sizeof(*mb) )
            {
                return(-SYS_INPUT_OUTPUT_ERROR);
                break;
            }
            //mailbox reset initialization  
            dccp->pSwUnsolQueue= mb->unsol_msgbuf; 

#if !ECFG_CHIP_ZIP1
            //tc_dic initialization
            clnk_tc_dic_init_pci_drv(dccp, mb);
#endif
            break;
        }
        case CLNK_CTL_HW_DESC_INIT:
            clnk_hw_desc_init(dccp); 
            break;
        case CLNK_CTL_GET_SOC_STATUS:
        {   
            int retval;
            SYS_UINT32 * status = (void*)io->out;
            if((retval = Clnk_ETH_Control_drv(dccp, CLNK_ETH_CTRL_GET_SOC_STATUS,
               0, (SYS_UINTPTR)status, 0)) != SYS_SUCCESS)
            {
                HostOS_PrintLog(L_INFO,"failed CLNK_CTL_GET_SOC_STATUS, stat=%d.\n", *status);
                return(retval);
            }
            break;
        }
        case CLNK_CTL_GET_LINK_STATUS:
        {   
            int retval;
            SYS_UINT32 *status = (void*)io->out;
            if((retval = Clnk_ETH_Control_drv(dccp, CLNK_ETH_CTRL_GET_LINK_STATUS,
               0, (SYS_UINTPTR)status, 0)) != SYS_SUCCESS)
            {
                HostOS_PrintLog(L_INFO,"failed CLNK_CTL_GET_LINK_STATUS\n");
                return(retval);
            }
            break;
        }
        case CLNK_CTL_SET_MAC_ADDRESS:
        {   
            int retval;
            ClnkDef_BridgeEntry_t *mac_addr = (void*)io->in;
            SYS_UINT32 mac_hi, mac_lo;
            mac_hi = mac_addr->macAddrHigh;
            mac_lo = mac_addr->macAddrLow;

            if (io->in_len < sizeof( *mac_addr)) 
            {
                return(-SYS_INPUT_OUTPUT_ERROR);
                break;
            }
            if((retval = Clnk_ETH_Control_drv(dccp, CLNK_ETH_CTRL_SET_MAC_ADDR,
                (SYS_UINTPTR)mac_addr->macAddrHigh,
                (SYS_UINTPTR)mac_addr->macAddrLow, 0)) != SYS_SUCCESS)
            {
                HostOS_PrintLog(L_INFO,"failed CLNK_CTL_SET_MAC_ADDR\n");
                return(retval);
            }
            HostOS_set_mac_address(vdkcp, mac_hi, mac_lo);

            break;
        }
        default:
            return(-SYS_INVALID_ARGUMENT_ERROR);
    }
    return(SYS_SUCCESS);
}

/*******************************************************************************
*
* Description:
*       Controls the operation of the Common Ethernet Module.
*
* Inputs:
*       dccp   - Pointer to the control context
*       option - Driver option to control
*       reg    - Register offset or pointer to register offsets
*       val    - Register value or pointer to register values
*       length - Length of data for bulk data
*
* Outputs:
*       CLNK_ETH_RET_CODE_SUCCESS
*       CLNK_ETH_RET_CODE_GEN_ERR
*       CLNK_ETH_RET_CODE_RESET_ERR
*       CLNK_ETH_RET_CODE_LINK_DOWN_ERR
*
*******************************************************************************/
int Clnk_ETH_Control_drv(void *vdccp, 
                        int option, 
                        SYS_UINTPTR reg, 
                        SYS_UINTPTR val,   // io block pointer or register value
                        SYS_UINTPTR length)
{
    dc_context_t *dccp = vdccp;
    int status = CLNK_ETH_RET_CODE_SUCCESS;

    switch (option)
    {
        case CLNK_ETH_CTRL_DO_CLNK_CTL:
            status = do_clnk_ctl_dvr(dccp, (SYS_INT32)reg, (struct clnk_io *)val);
            break;

        case CLNK_ETH_CTRL_ENABLE_INTERRUPT: // Clear and then Enable
            dccp->intr_enabled = SYS_TRUE;
            socClearInterrupt(dccp);
            socEnableInterrupt(dccp);
            break;

        case CLNK_ETH_CTRL_DISABLE_INTERRUPT:
            socDisableInterrupt(dccp);
            dccp->intr_enabled = SYS_FALSE;
            break;

        case CLNK_ETH_CTRL_RD_CLR_INTERRUPT:
            dccp->pciIntStatus = 0;
            if (dccp->intr_enabled)
                socClearInterrupt(dccp);
            break;

        case CLNK_ETH_CTRL_DETACH_SEND_PACKET:
            // Detach send packet
            if (clnk_hw_tx_detach(dccp, (SYS_UINT32 *)val) != SYS_SUCCESS)
            {
                status = CLNK_ETH_RET_CODE_GEN_ERR;
            }
            break;

        case CLNK_ETH_CTRL_DETACH_RCV_PACKET:
            // Detach receive packet
            if (clnk_hw_rx_detach(dccp, (SYS_UINT32 *)val) != SYS_SUCCESS)
            {
                status = CLNK_ETH_RET_CODE_GEN_ERR;
            }
            break;

        case CLNK_ETH_CTRL_SET_SEND_CALLBACK:
            // Set the send packet callback function
            dccp->sendPacketCallback = (Clnk_ETH_SendPacketCallback)val;
            break;

        case CLNK_ETH_CTRL_SET_RCV_CALLBACK:
            // Set the receive packet callback function
            dccp->rcvPacketCallback = (Clnk_ETH_RcvPacketCallback)val;
            break;

#if (FEATURE_QOS && ECFG_CHIP_ZIP1)
        case CLNK_ETH_CTRL_SET_TFIFO_RESIZE_CALLBACK:
            // Set the tx fifo resize callback function
            dccp->tFifoResizeCallback= (Clnk_ETH_TFifoReszCallback)val;
            break;
#endif

       case CLNK_ETH_CTRL_GET_MAC_ADDR:
            *(SYS_UINT32 *)reg = dccp->fw.macAddrHigh;
            *(SYS_UINT32 *)val = dccp->fw.macAddrLow;
            break;

       case CLNK_ETH_CTRL_SET_MAC_ADDR:
#if !ECFG_CHIP_ZIP1
            dccp->fw.macAddrHigh = (SYS_UINT32)reg;
            dccp->fw.macAddrLow  = (SYS_UINT32)val;
            clnk_reg_write(dccp, DEV_SHARED(mac_addr_hi), dccp->fw.macAddrHigh);
            clnk_reg_write(dccp, DEV_SHARED(mac_addr_lo), dccp->fw.macAddrLow);
            HostOS_PrintLog(L_INFO, "Set mac_addr_hi=%08x, mac_addr_lo=%08x\n",
                            dccp->fw.macAddrHigh, dccp->fw.macAddrLow);
#endif /* !ECFG_CHIP_ZIP1 */
            break;
 
        case CLNK_ETH_CTRL_GET_STATS:
#if ECFG_CHIP_ZIP1
            // Update the Ethernet statistics
            clnk_hw_get_eth_stats(dccp, val);
#else  /* ECFG_CHIP_ZIP2/ECFG_CHIP_MAVERICKS */
            dccp->stats.txDroppedErrs = dccp->txDroppedErrs;
#endif /* ECFG_CHIP_ZIP1 */

            // Copy Ethernet statistics
            *(ClnkDef_EthStats_t *)reg = dccp->stats;
            if (val)
            {
                // Clear Ethernet statistics
                HostOS_Memset(&dccp->stats, 0, sizeof(ClnkDef_EthStats_t));
                dccp->txDroppedErrs = 0;
            }
            break;

        default:
            status = CLNK_ETH_RET_CODE_GEN_ERR;
            break;
    }
    return (status);
}

void clnk_ctl_postprocess(void *vcp, IfrDataStruct *kifr, struct clnk_io *kio)
{
    SYS_UINT32 cmd = kifr->cmd;
    SYS_UINT32 subcmd = (SYS_UINT32)kifr->param1;

    switch (cmd)
    {
        case CLNK_MBX_ETH_DATA_BUF_CMD:
        {
            switch (subcmd)
            {
                case CLNK_CTL_GET_MY_NODE_INFO:
                {
                    ClnkDef_MyNodeInfo_t *out = (void *)kio->out;
                    SYS_UINT32  rev1=0, rev2=0, rev3=0, rev4=0;
                    
                    HostOS_Sscanf(DRV_VERSION, "%d.%d.%d.%d", 
                                  &rev1, &rev2, &rev3, &rev4);
                    out->SwRevNum = ((rev1 & 0xff) << 24) | 
                                    ((rev2 & 0xff) << 16) |
                                    ((rev3 & 0xff) << 8)  | 
                                    ((rev4 & 0xff) );
                    break;
                }
            }
            break;
        }

#if ECFG_CHIP_ZIP1
        case CLNK_MBX_ETH_GET_STATUS_CMD:
        {
            dc_context_t *dccp = dk_to_dc( vcp ) ;

            /*
             * The response data contains the current SoC status in the 1st 
             * LWord and the current TX Async pending count in the 2nd LWord.
             * Only the finalized SoC status is returned for this iocl command.
             */
            if (kio->out[0] == CLNK_DEF_SOC_STATUS_SUCCESS)
            {
                clnk_hw_soc_status(dccp, kio->out);
            }
            //HostOS_PrintLog(L_INFO, "SoC status=%u, Tx async pending=%u\n", 
            //                kio->out[0], kio->out[1]);
            break;
        }
#endif // ECFG_CHIP_ZIP1
    }
}

/**
* 
*   Purpose:    Handle clnk_ctl operations originating from within the driver
*               or from an ioctl.
*
*               io->in_len words are used as arguments to the function call.  
*               Special case:
*                 if io->in_len is 0, io->in is treated as a single argument 
*                 rather than a pointer to an argument list.
*
*               Up to io->out_len words will be copied as the result of the call.
*
*   Imports:    dccp - ClnkEth context struct
*               cmd  - CLNK_CTL_* command
*               io   - Inputs and outputs for the CLNK_CTL command
*
*   Exports:    CLNK_ETH_RET_CODE_SUCCESS Success.
*               CLNK_ETH_RET_CODE_GEN_ERR Failure.
*
*******************************************************************************/
static int do_clnk_ctl_dvr(dc_context_t *dccp, int cmd, struct clnk_io *io)
{
    int ret = CLNK_ETH_RET_CODE_GEN_ERR ;
    SYS_UINT32 arg0 = io->in_len ? io->in[0] : ((SYS_UINTPTR)io->in);

    /* cmd is processed by the common driver */
    switch(cmd)
    {
        case CLNK_CTL_GET_ETH_STATS:
        {
            SYS_UINT32 clr_stats = arg0;

#if ECFG_CHIP_ZIP1
            clnk_hw_get_eth_stats(dccp, clr_stats);
#endif

            // Update ethernet stats; clear if requested
            io->out_len = MIN_VAL(io->out_len, sizeof(ClnkDef_EthStats_t));
            HostOS_Memcpy((void *)io->out, (void *)&dccp->stats, io->out_len);

            if(clr_stats)
            {
                HostOS_Memset(&dccp->stats, 0, sizeof(ClnkDef_EthStats_t));
                dccp->txDroppedErrs = 0;
            }

            ret = CLNK_ETH_RET_CODE_SUCCESS;
            break;
        }
#if defined(CLNK_ETH_BRIDGE) 
        case CLNK_CTL_GET_BRIDGE_TABLE:
        {
            ClnkDef_BridgeTable_t *tbl = (void *)io->out;
            SYS_UINT32 entries;

            if(io->out_len < sizeof(*tbl))
            {
                ret = CLNK_ETH_RET_CODE_GEN_ERR;
                break;
            }

            entries = Cam_Get(&dccp->cam, &tbl->ent[0], arg0, BRIDGE_ENTRIES);
            tbl->num_entries = entries;
            ret = CLNK_ETH_RET_CODE_SUCCESS;
            break;
        }
#endif
        case CLNK_CTL_GET_EVM_DATA:
        {
            ClnkDef_EvmData_t *out = (void *)io->out;

            if (io->out_len < sizeof(*out))
            {   // Bad Specification
                ret = CLNK_ETH_RET_CODE_GEN_ERR;
            }
            else if (!dccp->evmData)
            {   // First time, alloc space and return
                dccp->evmData = (ClnkDef_EvmData_t *)
                    HostOS_Alloc(sizeof(ClnkDef_EvmData_t));
                if (dccp->evmData)
                {
                    dccp->evmData->valid = 0;
                    dccp->evmData->NodeId = 0;
                }
                ret = CLNK_ETH_RET_CODE_GEN_ERR;
            }
            else if (!dccp->evmData->valid)
            {   // Waiting for New Data
                ret = CLNK_ETH_RET_CODE_GEN_ERR;
            }
            else
            {   // Actually got new data
                HostOS_Memcpy((void *)out, dccp->evmData, sizeof(*out));
                dccp->evmData->valid = 0;
                ret = CLNK_ETH_RET_CODE_SUCCESS;
            }
            break;
        }

        case CLNK_CTL_GET_EPP_DATA:
        {
            ClnkDef_EppData_t *out = (void *)io->out;

            if ((!dccp->eppDataValid) || (io->out_len < sizeof(*out)))
            {
                ret = CLNK_ETH_RET_CODE_GEN_ERR;
                break;
            }
            HostOS_Memcpy((void *)out, &dccp->eppData, sizeof(*out));
            dccp->eppDataValid = SYS_FALSE;
            out->valid = 1;
            ret = CLNK_ETH_RET_CODE_SUCCESS;
            break;
        }

        case CLNK_CTL_SET_ETH_FIFO_SIZE:
        {
#if ECFG_CHIP_ZIP1
            ClnkDef_TxFifoCfg_t *in = (void *)io->in;
            int i;

            if(io->in_len < sizeof(*in))
            {
                ret = CLNK_ETH_RET_CODE_GEN_ERR;
                break;
            }

            for(i = 0; i < NUM_TX_PRIO; i++)
                dccp->fw.txFifoSize[i] = in->tx_prio[i];
#endif /* ECFG_CHIP_ZIP1 */

            ret = CLNK_ETH_RET_CODE_SUCCESS;
            break;
        }
#if ECFG_FLAVOR_VALIDATION==1
        case CLNK_CTL_VAL_GET_MBOX_HOST_COUNTS:
        {
            eclair_ValMboxHostCounts_t *out = (void*)io->out;

            INCTYPES_SAFE_PTR_COPY(out, &clnkEth_valMboxHostCounts);

            ret = CLNK_ETH_RET_CODE_SUCCESS;
        }
#endif
    }

    return(ret);
}

