/*******************************************************************************
*
* Common/Inc/drv_ctl_opts.h
*
* Description: driver (io)control options
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


#ifndef __drv_ctl_opts_h__
#define __drv_ctl_opts_h__

#include "common_dvr.h"

#ifndef FEATURE_QOS
#define FEATURE_QOS     0
#endif




// Driver Control Options
typedef enum
{
    CLNK_ETH_CTRL_GET_REG,
    CLNK_ETH_CTRL_SET_REG,
    CLNK_ETH_CTRL_GET_MEM,
    CLNK_ETH_CTRL_SET_MEM,
    CLNK_ETH_CTRL_GET_TRACE,
    CLNK_ETH_CTRL_GET_BRIDGE_SRC_TABLE, // COMBINE THESE 4 !!!!
    CLNK_ETH_CTRL_GET_BRIDGE_DST_TABLE,
    CLNK_ETH_CTRL_GET_BRIDGE_BCAST_TABLE,
    CLNK_ETH_CTRL_GET_BRIDGE_MCAST_TABLE,
    CLNK_ETH_CTRL_GET_EPP_DATA,
    CLNK_ETH_CTRL_GET_EVM_DATA,
#if FEATURE_FEIC_PWR_CAL
    CLNK_ETH_CTRL_GET_FEIC_STATUS,
#endif
    CLNK_ETH_CTRL_GET_MY_NODE_INFO,
    CLNK_ETH_CTRL_GET_NET_NODE_INFO,
    CLNK_ETH_CTRL_GET_PHY_DATA,
    CLNK_ETH_CTRL_GET_RX_ERR_DATA,
    CLNK_ETH_CTRL_GET_TX_FIFO_SIZE,
    CLNK_ETH_CTRL_GET_RX_FIFO_SIZE,
    CLNK_ETH_CTRL_GET_LINK_STATUS,
    CLNK_ETH_CTRL_GET_MAC_ADDR,
    CLNK_ETH_CTRL_GET_MULTICAST_TABLE,
    CLNK_ETH_CTRL_GET_NUM_TX_HOST_DESC,
    CLNK_ETH_CTRL_GET_SOC_STATUS,  /* OBSOLETE/DO_NOT_REUSE */  
    CLNK_ETH_CTRL_GET_STATS,
    CLNK_ETH_CTRL_GET_SW_REV_NUM,
    CLNK_ETH_CTRL_SET_CM_RATIO,
    CLNK_ETH_CTRL_SET_EPP_DATA,    /* OBSOLETE/DO_NOT_REUSE */
    CLNK_ETH_CTRL_SET_TX_FIFO_SIZE,
    CLNK_ETH_CTRL_SET_RX_FIFO_SIZE,
    CLNK_ETH_CTRL_SET_FIRMWARE,
    CLNK_ETH_CTRL_SET_MAC_ADDR,
    CLNK_ETH_CTRL_SET_PHY_MARGIN,
    CLNK_ETH_CTRL_SET_PRIVACY_MODE, /* OBSOLETE/DO_NOT_REUSE */
    CLNK_ETH_CTRL_SHA_PRIVACY_KEY,
    CLNK_ETH_CTRL_SET_PRIVACY_KEY,
    CLNK_ETH_CTRL_SET_SW_CONFIG,
    CLNK_ETH_CTRL_SET_TX_POWER,
    CLNK_ETH_CTRL_TEST_PORT,
    CLNK_ETH_CTRL_RESET,
    CLNK_ETH_CTRL_ENABLE_INTERRUPT,
    CLNK_ETH_CTRL_DISABLE_INTERRUPT,
    CLNK_ETH_CTRL_RD_CLR_INTERRUPT,
    CLNK_ETH_CTRL_DETACH_SEND_PACKET,
    CLNK_ETH_CTRL_DETACH_RCV_PACKET,
    CLNK_ETH_CTRL_JOIN_MULTICAST,
    CLNK_ETH_CTRL_LEAVE_MULTICAST,
    CLNK_ETH_CTRL_SET_SEND_CALLBACK,
    CLNK_ETH_CTRL_SET_RCV_CALLBACK,
    CLNK_ETH_CTRL_SET_UNSOL_CALLBACK,
    CLNK_ETH_CTRL_GET_RFIC_TUNING_DATA,
    CLNK_ETH_CTRL_SET_RFIC_TUNING_DATA, /* OBSOLETE/DO_NOT_REUSE */
    CLNK_ETH_CTRL_SET_AGC_GAIN_TABLE,   /* OBSOLETE/DO_NOT_REUSE */
    CLNK_ETH_CTRL_SET_CHANNEL_MASK,
    CLNK_ETH_CTRL_GET_RANDOM_BITS,  /* OBSOLETE/DO_NOT_REUSE */
   // CLNK_SOFTCAM
    CLNK_ETH_CTRL_GET_SOFT_CAMDATA,

    CLNK_ETH_CTRL_SET_LOF,
    CLNK_ETH_CTRL_GET_LOF,
    CLNK_ETH_CTRL_SET_BIAS,
    CLNK_ETH_CTRL_FS_UPDATE,  /* OBSOLETE */
    CLNK_ETH_CTRL_SET_CHANNEL_PLAN,
    CLNK_ETH_CTRL_SET_TABOO_INFO,
    CLNK_ETH_CTRL_SET_SCAN_MASK,

    CLNK_ETH_CTRL_SET_DISTANCE_MODE,
    CLNK_ETH_CTRL_SET_POWERCTL_PHYRATE,

    CLNK_ETH_CTRL_CLINK_ACCEPT_SMALL_ROUTED_MESSAGE,
    CLNK_ETH_CTRL_DEFINE_CLM_INIT_SETTINGS,

    /* OBSOLETE/DO_NOT_REUSE */
    CLNK_ETH_CTRL_PRIVACY_SEND,
    CLNK_ETH_CTRL_PRIVACY_SET_KEY,
    CLNK_ETH_CTRL_PRIVACY_UPDATE,

    /* OBSOLETE/DO_NOT_REUSE */
    CLNK_ETH_CTRL_GET_PRIVACY_EVENT,
    CLNK_ETH_CTRL_GET_PRIVACY_STAT,
    CLNK_ETH_CTRL_GET_PRIVACY_KEY,
    CLNK_ETH_CTRL_GET_PRIVACY_DATA,
    CLNK_ETH_CTRL_GET_PRIVACY_RANDBIT,

    CLNK_ETH_CTRL_NODEMANAGE_ADD ,
    CLNK_ETH_CTRL_NODEMANAGE_DELETE ,
    CLNK_ETH_CTRL_NODEMANAGE_GET,

    CLNK_ETH_CTRL_SET_TIMER_SPEED,

    CLNK_ETH_CTRL_GET_PRIV_INFO,
    CLNK_ETH_CTRL_GET_PRIV_STATS,
    CLNK_ETH_CTRL_GET_PRIV_NODE_INFO,

    CLNK_ETH_CTRL_DO_CLNK_CTL,
    CLNK_ETH_CTRL_SET_BEACON_POWER_LEVEL, // added for DIP feature.

#if FEATURE_QOS
    CLNK_ETH_CTRL_CREATE_FLOW,
    CLNK_ETH_CTRL_UPDATE_FLOW,
    CLNK_ETH_CTRL_DELETE_FLOW,
    CLNK_ETH_CTRL_QUERY_FLOW,
    CLNK_ETH_CTRL_LIST_FLOWS,
    CLNK_ETH_CTRL_GET_EVENT_COUNTS,
    CLNK_ETH_CTRL_QUERY_IF_CAPS,
    CLNK_ETH_CTRL_QUERY_NODES,
    CLNK_ETH_CTRL_QUERY_PATH_INFO,
    CLNK_ETH_CTRL_EXPIRE_FLOW,
#if ECFG_CHIP_ZIP1
    CLNK_ETH_CTRL_SET_TFIFO_RESIZE_CALLBACK,
#endif /* ECFG_CHIP_ZIP1 */
#endif

    CLNK_ETH_CTRL_GET_PEER_RATES,
    CLNK_ETH_CTRL_STOP,

    CLNK_ETH_CTRL_MAX  // This must always be last
}
CLNK_ETH_CTRL_OPTIONS;





#endif // __drv_ctl_opts_h__
