/*******************************************************************************
*
* Common/Src/ClnkMbx_call.c
*
* Description: mailbox common functions
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
*PUBLIC***************************************************************************/
int MbxSwUnsolRdyCallback(void *vcp, Clnk_MBX_Msg_t *pMsg)
{
    int consumed = 1 ;              // message consumed flag
    dc_context_t *dccp = vcp;
    switch (pMsg->msg.maxMsg.msg[0])
    {
        case CLNK_MBX_UNSOL_MSG_ADMISSION_STATUS:
        case CLNK_MBX_UNSOL_MSG_BEACON_STATUS:
        case CLNK_MBX_UNSOL_MSG_RESET:
            consumed = 0 ;
            break;

#if defined(CLNK_ETH_BRIDGE) 
        case CLNK_MBX_UNSOL_MSG_UCAST_PUB:
            break;
#endif
#if defined(CLNK_ETH_BRIDGE) 
        case CLNK_MBX_UNSOL_MSG_UCAST_UNPUB:
            break;
#endif
        case CLNK_MBX_UNSOL_MSG_NODE_ADDED:
            break;
        case CLNK_MBX_UNSOL_MSG_NODE_DELETED:
            break; 
//#ifdef CLNK_ETH_PHY_DATA_LOG
        case CLNK_MBX_UNSOL_MSG_EVM_DATA_READY:
            //HostOS_PrintLog(L_INFO, "FSUPDATE: IMO Node = %d\n", pMsg->msg.maxMsg.msg[1]);
               
            if (dccp->evmData)
            {
                int i;

                dccp->evmData->valid = SYS_FALSE;                   // Don't let others read the data while it s updated
                dccp->evmData->NodeId = pMsg->msg.maxMsg.msg[1];    // Get the data 
                for (i=0;i<256;i++)
                {    
                    dccp->evmData->Data[i]=0;
                }
                dccp->evmData->valid = SYS_TRUE;      
            }            
            consumed = 0 ;
            break;
//#endif
#if FEATURE_FEIC_PWR_CAL
        // for MII
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
#if (NETWORK_TYPE_HIRF_MESH || NETWORK_TYPE_MIDHIRF_MESH)
#define FS_FREQ_START   800000000
#define FS_FREQ_STEP    25000000
#elif NETWORK_TYPE_MIDRF_MESH
#define FS_FREQ_START   400000000
#define FS_FREQ_STEP    (25000000/2)
#endif
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
#if (NETWORK_TYPE_HIRF_MESH || NETWORK_TYPE_MIDHIRF_MESH)
                    HostOS_PrintLog(L_INFO, "M0FeicAtt2: %2d  M0Feic_Att1: %2d  M0TxCalDecrN: %2d  M0TxdCGainSel: %2d \n",
                           feicstatus_data->intermediateDetectorStatus.m0FeicAtt2,
#elif NETWORK_TYPE_MIDRF_MESH
                    HostOS_PrintLog(L_INFO, "M0FeicAtt2: N/A  M0Feic_Att1: %2d  M0TxCalDecrN: %2d  M0TxdCGainSel: %2d \n",
#endif
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
#if (NETWORK_TYPE_HIRF_MESH || NETWORK_TYPE_MIDHIRF_MESH)
                    HostOS_PrintLog(L_INFO, "M1FeicAtt2: %2d  M1Feic_Att1: %2d  M1TxCalDecrN: %2d  M1TxdCGainSel: %2d \n",
                           feicstatus_data->intermediateDetectorStatus.m1FeicAtt2,
#elif NETWORK_TYPE_MIDRF_MESH
                    HostOS_PrintLog(L_INFO, "M1FeicAtt2: N/A  M1Feic_Att1: %2d  M1TxCalDecrN: %2d  M1TxdCGainSel: %2d \n",
#endif
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
#if (NETWORK_TYPE_HIRF_MESH || NETWORK_TYPE_MIDHIRF_MESH)
                    HostOS_PrintLog(L_INFO, "M2FeicAtt2: %2d  M2Feic_Att1: %2d  M2TxCalDecrN: %2d  M2TxdCGainSel: %2d \n",
                           feicstatus_data->intermediateDetectorStatus.m2FeicAtt2,
#elif NETWORK_TYPE_MIDRF_MESH
                    HostOS_PrintLog(L_INFO, "M2FeicAtt2: N/A  M2Feic_Att1: %2d  M2TxCalDecrN: %2d  M2TxdCGainSel: %2d \n",
#else
#error It needs to define one NETWORK TYPE!!
#endif
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
#if (NETWORK_TYPE_HIRF_MESH || NETWORK_TYPE_MIDHIRF_MESH)
                    HostOS_PrintLog(L_INFO, "M4FeicAtt2: %2d  M4Feic_Att1: %2d  M4TxCalDecrN: %2d  M4TxdCGainSel: %2d \n",
                           feicstatus_data->intermediateDetectorStatus.m4FeicAtt2,
#elif NETWORK_TYPE_MIDRF_MESH
                    HostOS_PrintLog(L_INFO, "M4FeicAtt2: N/A  M4Feic_Att1: %2d  M4TxCalDecrN: %2d  M4TxdCGainSel: %2d \n",
#endif
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
#if (NETWORK_TYPE_HIRF_MESH || NETWORK_TYPE_MIDHIRF_MESH)
                HostOS_PrintLog(L_INFO, "FeicAtt2: %d      Att1: %d    TxCalDecrN: %d   TxdCGainSel: %2d\n",
                            feicstatus_data->finalConfigStatus.feicAtt2,
#elif NETWORK_TYPE_MIDRF_MESH
                HostOS_PrintLog(L_INFO, "FeicAtt2: N/A      Att1: %d    TxCalDecrN: %d   TxdCGainSel: %2d\n",
#endif
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
// #if CLNK_ETH_ECHO_PROFILE 
#if 1 // modified for MID RF (DEBUG_EPP)
         #define NUMBER_OF_RX_ECHO_PROBE_SYMBOLS 3
         #define EP_RX_PROBE_SIZE ((256 + NUMBER_OF_RX_ECHO_PROBE_SYMBOLS*(256 + 64)) * 4)
         #define EPP_RX_CORR_SIZE 256
        case CLNK_MBX_UNSOL_MSG_ECHO_PROFILE_PROBE:
            consumed = 0 ;
	{
            int  sample;
            
            dccp->eppData.nodeId = pMsg->msg.maxMsg.msg[2] & 0x0000FFFF;
            dccp->eppData.probeId = pMsg->msg.maxMsg.msg[2] >> 16;
            dccp->eppData.cpLen = pMsg->msg.maxMsg.msg[6] & 0x0000FFFF;
            dccp->eppData.sysBias = pMsg->msg.maxMsg.msg[6] >> 16;
            dccp->eppData.status = pMsg->msg.maxMsg.msg[7];

            if (pMsg->msg.maxMsg.msg[1] & EPP_CAP_FLAGS_RAW_DATA)
            {
                clnk_blk_read(dccp, pMsg->msg.maxMsg.msg[3],
                           dccp->eppData.rawData, EP_RX_PROBE_SIZE);
                for (sample = 0; sample < 128; sample += 127)
                {
                    HostOS_PrintLog(L_DEBUG,
                            "ProbeId:%-5u  "
                            "Sample:%-3u  "
                            "Status:%-2u  "
                            "EPPraw:%u\n",
                            (SYS_UINT32)dccp->eppData.probeId,
                            (SYS_UINT32)sample,
                            (SYS_UINT32)dccp->eppData.status,
                            (SYS_UINT32)dccp->eppData.rawData[sample]);
                }
            }

            if (pMsg->msg.maxMsg.msg[1] & EPP_CAP_FLAGS_CORR_DATA)
            {
                clnk_blk_read(dccp, pMsg->msg.maxMsg.msg[4],
                           (SYS_UINT32 *)dccp->eppData.corrData,
                           EPP_RX_CORR_SIZE);
                clnk_blk_read(dccp, pMsg->msg.maxMsg.msg[5],
                           (SYS_UINT32 *)dccp->eppData.corrEnergy,
                           EPP_RX_CORR_SIZE);
                for (sample = 0; sample < 128; sample +=127)
                {
                    HostOS_PrintLog(L_DEBUG,
                            "ProbeId:%-5u  "
                            "Sample:%-3u  "
                            "Status:%-2u  "
                            "CP=%-2u  "
                            "SB:%-2u  "
                            "EPPcorr:%-3u  "
                            "EPPenergy:%-3u\n",
                            (SYS_UINT32)dccp->eppData.probeId,
                            (SYS_UINT32)sample,
                            (SYS_UINT32)dccp->eppData.status,
                            (SYS_UINT32)dccp->eppData.cpLen,
                            (SYS_UINT32)dccp->eppData.sysBias,
                            (SYS_UINT32)dccp->eppData.corrData[sample],
                            (SYS_UINT32)dccp->eppData.corrEnergy[sample]);
                }
            }

            dccp->eppData.valid |= pMsg->msg.maxMsg.msg[1];
            //    return;
        }

            break;
#endif
#if (NETWORK_TYPE_ACCESS)
        case CLNK_MBX_UNSOL_MSG_ACCESS_CHK_MAC:
            consumed = 0 ;
            break;
#endif
        case CLNK_MBX_UNSOL_MSG_TABOO_INFO:
            HostOS_PrintLog(L_INFO, "FSUPDATE: Taboo Mask = 0x%06x, Offset = %d\n",
                            pMsg->msg.maxMsg.msg[1] >> 8,
                            pMsg->msg.maxMsg.msg[1] & 0x0ff);
            consumed = 0 ;
            break;
        case CLNK_MBX_UNSOL_MSG_FSUPDATE:
#if 0 
            HostOS_PrintLog(L_INFO, 
                            "FSUPDATE: Pass = %2d, Tuned Freq = %4d.%1d MHz (%d)\n",
                            pMsg->msg.maxMsg.msg[1] >> 16,
                            400 +  ((pMsg->msg.maxMsg.msg[1] & 0x00ff) * 125/10), (((pMsg->msg.maxMsg.msg[1] & 0x00ff)% 2) ? 5: 0),
                            (pMsg->msg.maxMsg.msg[1] >> 8) & 0x00ff);   
#else
#define FS_FREQ_START(band) (band <= FREQ_BAND_D ? 800 : (band == FREQ_BAND_E ? 500 : 675 /*band f*/))
            if (dccp->freqBand == FREQ_BAND_TRIKA)
            {
                int trika_subband = (pMsg->msg.maxMsg.msg[1] >> 6) & 0x3; // band D or E
                /*HostOS_PrintLog(L_INFO, "subband = %d\n", trika_subband);
                HostOS_PrintLog(L_INFO, "start freq = %d\n", FS_FREQ_START(trika_subband));
                HostOS_PrintLog(L_INFO, "idx = %d\n", pMsg->msg.maxMsg.msg[1] & 0x003f);*/
                HostOS_PrintLog(L_INFO,
                            "FSUPDATE: Pass = %2d, Tuned Freq = %4d MHz (%d)\n",
                            pMsg->msg.maxMsg.msg[1] >> 16,
                            FS_FREQ_START(trika_subband) +  ((pMsg->msg.maxMsg.msg[1] & 0x003f) * 25),
                            (pMsg->msg.maxMsg.msg[1] >> 8) & 0x00ff);                

            }
            else
            {
                HostOS_PrintLog(L_INFO,
                            "FSUPDATE: Pass = %2d, Tuned Freq = %4d MHz (%d)\n",
                            pMsg->msg.maxMsg.msg[1] >> 16,
                            FS_FREQ_START(dccp->freqBand) +  ((pMsg->msg.maxMsg.msg[1] & 0x003f) * 25),
                            (pMsg->msg.maxMsg.msg[1] >> 8) & 0x00ff);
            }
#endif
            consumed = 0 ;
            break;

        case CLNK_MBX_UNSOL_MSG_ADD_CAM_FLOW_ENTRY:
            break;

        case CLNK_MBX_UNSOL_MSG_DELETE_CAM_FLOW_ENTRIES:
            break;

#if ECFG_FLAVOR_VALIDATION==1
        case CLNK_MBX_UNSOL_MSG_VAL_ISOC_EVENT:
            consumed = 0 ;
            break ;
#endif
        default:
            consumed = 0 ;
            break;
    }

    return( consumed ) ;
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
 * \param[in] pvContext void pointer to our dc_context_t
 * \param[in] pMsg Struct containing newly received message
 *
*PUBLIC*****************************************************************/
void MbxReplyRdyCallback(void *vcp, Clnk_MBX_Msg_t* pMsg)
{
#if 000
    dc_context_t *dccp = vcp;
    SYS_UINT8  transID;

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
#endif
}



