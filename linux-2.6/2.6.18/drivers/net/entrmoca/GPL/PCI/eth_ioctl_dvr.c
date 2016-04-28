/*******************************************************************************
*
* GPL/PCI/eth_ioctl_dvr.c
*
* Description: Linux Ethernet Driver -- IOCTL section
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
*******************************************************************************/

/*******************************************************************************
*                            # I n c l u d e s                                 *
********************************************************************************/

#include "pci_gpl_hdr.h"


/*******************************************************************************
*                             # D e f i n e s                                  *
********************************************************************************/

/*******************************************************************************
*                             D a t a   T y p e s                              *
********************************************************************************/

/*******************************************************************************
*                             C o n s t a n t s                                *
********************************************************************************/

/*******************************************************************************
*                             G l o b a l   D a t a                            *
********************************************************************************/
extern int debug_mode ;   // command-line flag to enable debug mode

/*******************************************************************************
*                       M e t h o d   P r o t o t y p e s                      *
********************************************************************************/
extern int Clnk_MBX_RcvUnsolMsg(Clnk_MBX_Mailbox_t *pMbx, 
                                SYS_UINT32         *pbuf);

/*******************************************************************************
*                       S t a t i c   P r o t o t y p e s                      *
********************************************************************************/
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 19)

static SYS_INT32 netdev_ethtool_ioctl(struct net_device *dev, SYS_VOID *useraddr);

#else
static void clnk_ethtool_get_drvinfo(struct net_device *netdev, struct ethtool_drvinfo *drvinfo);

#endif

/*******************************************************************************
********************************************************************************/

/**
*  Purpose:    IOCTL entry point for network driver
*
*  Imports:    dev - device structure pointer
*              ifr - request structure from user via kernel
*                    See the ifr_data member.
*              cmd - IOCTL code
*
*  Exports:    Error status
*              Response data, if any, is returned via ioctl structures
*
*PUBLIC***************************************************************************/
SYS_INT32 clnketh_ioctl(struct net_device *dev, 
                        struct ifreq *ifr, 
                        SYS_INT32 cmd)
{
    SYS_INT32    status = -EINVAL;
    dk_context_t *dkcp  = (dk_context_t *)dev ;
    void         *arg   = ifr->ifr_data ;

    if( !capable(CAP_SYS_ADMIN) ) {  // kernel check & set permissions
        status = -EPERM;
    } else {

        switch( cmd ) 
        {
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 19)
            case SIOCETHTOOL :
                status = netdev_ethtool_ioctl(dev, (SYS_VOID *)ifr->ifr_data);
                break ;
#endif
            case SIOCHDRCMD :
                break;
            case SIOCCLINKDRV :     // Control plane commands for the driver
                status = clnkioc_driver_cmd( dkcp, arg ) ;
                break ; 
            case SIOCGCLINKMEM :    // Reads registers/memory in c.LINK address space
                status = clnkioc_mem_read( dkcp, arg ) ;
                break ; 
#ifndef SONICS_ADDRESS_OF_sniff_buffer_pa
#error "Sniff buffer is not defined"
#else
#if SONICS_ADDRESS_OF_sniff_buffer_pa
            // #################################################################
            // Handle the case where the application (in this case the deamon)
            // wants to read the sniff buffer that holds the descriptors that
            // have been logged by the SoC.
            // #################################################################
            case SIOCGSNIFFMEM :    // Reads sniffer buffer directly in the driver
                status = clnkioc_sniff_read( dkcp, arg ) ;
                break ; 
#endif
#endif
            case SIOCSCLINKMEM :    // Sets registers/memory in c.LINK address space
                status = clnkioc_mem_write( dkcp, arg ) ;
                break ; 
            case SIOCGCLNKCMD :     // mbox cmmds: request with response
                status = clnkioc_mbox_cmd_request( dkcp, arg, 1 ) ;
                break ;
            case SIOCSCLNKCMD :     // mbox cmmds: request with no response
                status = clnkioc_mbox_cmd_request( dkcp, arg, 0 ) ;
                break ;
            case SIOCLNKDRV :       // mbox cmmds: retrieve unsol messages
                status = clnkioc_mbox_unsolq_retrieve( dkcp, arg ) ;
                break ;
            default:
                return -EOPNOTSUPP;
        }
    }
    return(status);
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 19)

/*
*STATIC***************************************************************************/
static SYS_INT32 netdev_ethtool_ioctl(struct net_device *dev, SYS_VOID *useraddr)
{
    u32 ethcmd;

    if (HostOS_copy_from_user(&ethcmd, useraddr, sizeof(ethcmd)))
        return -EFAULT;

    switch (ethcmd)
    {
        case ETHTOOL_GDRVINFO:
        {
            struct ethtool_drvinfo info = {ETHTOOL_GDRVINFO};
            HostOS_Memcpy(info.driver, DRV_NAME, sizeof(DRV_NAME));
            HostOS_Memcpy(info.version, DRV_VERSION, sizeof(DRV_VERSION));
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 0)
            /* slot_name does not exist on 2.6 */
            info.bus_info[0] = 0;
#else
            {
                dd_context_t *ddcp = dk_to_dd( dev ) ;
                HostOS_Memcpy(info.bus_info, ddcp->pdev->slot_name, sizeof(ddcp->pdev->slot_name));
            }
#endif
            if (HostOS_copy_to_user(useraddr, &info, sizeof(info)))
                return -EFAULT;
            return 0;
        }
    } /* end switch */
    return -EOPNOTSUPP;
}

#else

static void clnk_ethtool_get_drvinfo(struct net_device *netdev,
                              struct ethtool_drvinfo *drvinfo)
{
	strncpy(drvinfo->driver,  DRV_NAME, 32);
	strncpy(drvinfo->version, DRV_VERSION, 32);
	strcpy(drvinfo->fw_version, "N/A");
	drvinfo->bus_info[0] = 0;
}

static struct ethtool_ops clnk_ethtool_var = {
	.get_settings           = 0,
	.set_settings           = 0,
	.get_drvinfo            = clnk_ethtool_get_drvinfo,
	.get_regs_len           = 0,
	.get_regs               = 0,
	.get_wol                = 0,
	.set_wol                = 0,
	.get_msglevel           = 0,
	.set_msglevel           = 0,
	.nway_reset             = 0,
	.get_link               = ethtool_op_get_link,
	.get_eeprom_len         = 0,
	.get_eeprom             = 0,
	.set_eeprom             = 0,
	.get_ringparam          = 0,
	.set_ringparam          = 0,
	.get_pauseparam         = 0,
	.set_pauseparam         = 0,
	.get_rx_csum            = 0,
	.set_rx_csum            = 0,
	.get_tx_csum            = 0,
	.set_tx_csum            = 0,
	.get_sg                 = ethtool_op_get_sg,
	.set_sg                 = ethtool_op_set_sg,
#ifdef NETIF_F_TSO
	.get_tso                = ethtool_op_get_tso,
	.set_tso                = 0,
#endif
	.self_test_count        = 0,
	.self_test              = 0,
	.get_strings            = 0,
	.phys_id                = 0,
	.get_stats_count        = 0,
	.get_ethtool_stats      = 0,
	.get_coalesce           = 0,
	.set_coalesce           = 0,
};
/*
*PUBLIC***************************************************************************/

void clnk_set_ethtool_var(struct net_device *netdev)
{
	SET_ETHTOOL_OPS(netdev, &clnk_ethtool_var);
}

#endif

