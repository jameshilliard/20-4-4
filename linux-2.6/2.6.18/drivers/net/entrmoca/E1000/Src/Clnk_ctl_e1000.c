/*******************************************************************************
*
* E1000/Src/Clnk_ctl_e1000.c
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

static int do_clnk_ctl_dvr(dc_context_t *dccp, int cmd, struct clnk_io *io);


int clnk_ctl_drv(void *vdkcp, int cmd, struct clnk_io *io)
{
    dc_context_t        *dccp = dk_to_dc( vdkcp ) ;
    Clnk_MBX_Mailbox_t  *pMbx = &dccp->mailbox ;

    switch (cmd)
    {
        case CLNK_CTL_NET_CARRIER_OK:
            break;

        case CLNK_CTL_RESET_DEVICE:
            break;

        case CLNK_CTL_STOP_DEVICE:
            Clnk_Kern_Task_Kill((void *)dccp);
            break;

        case CLNK_CTL_NET_CARRIER_ON:
            break;
        
        case CLNK_CTL_NET_CARRIER_OFF:
            // Turn off mailbox processing
            pMbx->mbxOpen = SYS_FALSE;
            Clnk_Kern_Task_Stop((void *)dccp);
            break;
        
        case CLNK_CTL_SOC_INIT_BUS:
            break; 
        
        case CLNK_CTL_SOC_BOOTED:
        {
            ClnkDef_dataPlaneVars_t *in = (void *)io->in;
            int                     stat ;

            if (io->in_len < sizeof( *in)) 
            {
                return(-SYS_INPUT_OUTPUT_ERROR);
                break;
            }
            if (pMbx->mbxOpen == SYS_TRUE)
            {
                pMbx->mbxOpen = SYS_FALSE;
                Clnk_Kern_Task_Stop((void *)dccp);
            } 

            //mailbox reset initialization 
            dccp->pSwUnsolQueue = in->unsol_msgbuf; 
            dccp->swUnsolQueueSize = SW_UNSOL_HW_QUEUE_SIZE;

            if (dccp->pSwUnsolQueue != SYS_NULL)
            { 
                // set mbxopen
                stat = Clnk_MBX_Initialize( pMbx, dccp, CLNK_MBX_ETHERNET_TYPE );

                if (stat != SYS_SUCCESS)
                {
                    HostOS_PrintLog(L_ERR, "Clink Reset MBX failed, stat=%d.\n", stat);
                    return (stat);
                }


                dccp->mailboxInitialized = SYS_TRUE;

                Clnk_MBX_Control(&dccp->mailbox, CLNK_MBX_CTRL_SET_REPLY_RDY_CB,
                         0, (SYS_UINTPTR)MbxReplyRdyCallback, (SYS_UINTPTR)dccp);
                // set unsol mbox pointer
                Clnk_MBX_Control(&dccp->mailbox, CLNK_MBX_CTRL_SET_SW_UNSOL_Q, 
                         0, dccp->pSwUnsolQueue, dccp->swUnsolQueueSize);
                Clnk_MBX_Control(&dccp->mailbox, CLNK_MBX_CTRL_SET_SW_UNSOL_RDY_CB,
                         0, (SYS_UINTPTR)MbxSwUnsolRdyCallback, (SYS_UINTPTR)dccp);
                
                if(Clnk_Kern_Task_Init((void *)dccp) != 0)
                    HostOS_PrintLog(L_ERR, "MBX open, Start TT task failed!\n");
            }
            else
                HostOS_PrintLog(L_ERR, "ERROR: mailbox unsolicited queue is NULL!!\n");

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
            break;
        }
        case CLNK_CTL_HW_DESC_INIT:
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
            SYS_UINT32 * status = (void*)io->out;
            if((retval = Clnk_ETH_Control_drv(dccp, CLNK_ETH_CTRL_GET_LINK_STATUS,
               0, (SYS_UINTPTR)status, 0)) != SYS_SUCCESS)
            {
                HostOS_PrintLog(L_INFO,"failed CLNK_CTL_GET_LINK_STATUS\n");
                return(retval);
            }
            break;
        }
        case CLNK_CTL_SET_MAC_ADDRESS:
            break;
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
*       dccp      - Pointer to the control context
*       option    - Driver option to control
*       reg       - Register offset or pointer to register offsets
*       val       - Register value or pointer to register values
*       length    - Length of data for bulk data
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
                         SYS_UINTPTR val,  // io block pointer or register value
                         SYS_UINTPTR length)
{
    dc_context_t *dccp = vdccp;
    int status = CLNK_ETH_RET_CODE_SUCCESS;

    switch (option)
    {
        case CLNK_ETH_CTRL_DO_CLNK_CTL:
            status = do_clnk_ctl_dvr(dccp, (SYS_INT32)reg, (struct clnk_io *)val);
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
    //SYS_UINT32 arg0 = io->in_len ? io->in[0] : ((SYS_UINTPTR)io->in);

    /* cmd is processed by the common driver */
    switch(cmd)
    {
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
#if 1
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
#endif

        case CLNK_CTL_GET_ETH_STATS:
        {
            // The Ethernet statistic counters are no meaning here, it is only for compatible purpose
            io->out_len = MIN_VAL(io->out_len, sizeof(ClnkDef_EthStats_t));
            // Make sure all counters are cleared
            HostOS_Memset((void *)io->out, 0, io->out_len);        
            ret = CLNK_ETH_RET_CODE_SUCCESS;
            break;        
         }
#if 0
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
#endif
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

