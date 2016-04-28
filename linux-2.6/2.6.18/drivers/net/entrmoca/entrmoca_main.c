/*
 * Copyright (c) 2010, TiVo, Inc.  All Rights Reserved
 *
 * This file is licensed under GNU General Public license.                      
 *                                                                              
 * This file is free software: you can redistribute and/or modify it under the  
 * terms of the GNU General Public License, Version 2, as published by the Free 
 * Software Foundation.                                                         
 *                                                                              
 * This program is distributed in the hope that it will be useful, but AS-IS and
 * WITHOUT ANY WARRANTY; without even the implied warranties of MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE, TITLE, or NONINFRINGEMENT. Redistribution, 
 * except as permitted by the GNU General Public License is prohibited.         
 *                                                                              
 * You should have received a copy of the GNU General Public License, Version 2 
 * along with this file; if not, see <http://www.gnu.org/licenses/>.            
 */

#include <linux/module.h>
#include <linux/netdevice.h>
#include <linux/delay.h>
#include <linux/mii.h>
#include <linux/miscdevice.h>
#include <linux/ethtool.h>
#include <asm/brcmstb/common/brcm-pm.h>
#include "bcmmii.h"
#include "bcmemac.h"
#include "drv_hdr.h"
#include "data_context.h"

#define EMOCA_DRIVER_NAME        "emoca"
#define EMOCA_DRIVER_VER_STRING  "1.0"
#define EMOCA_MAJOR              60

/* brcmint7038 Ethernet driver */
extern void bcmemac_tx_reclaim_timer(unsigned long arg);
extern int bcmemac_init_dev(BcmEnet_devctrl *pDevCtrl);
extern int bcmemac_uninit_dev(BcmEnet_devctrl *pDevCtrl);
extern irqreturn_t bcmemac_net_isr(int irq, void *, struct pt_regs *regs);
extern void bcmemac_link_change_task(BcmEnet_devctrl *pDevCtrl);
extern void bcmemac_set_multicast_list(struct net_device * dev);
extern int bcmemac_set_mac_addr(struct net_device *dev, void *p);
extern int bcmemac_net_open(struct net_device * dev);
extern int bcmemac_net_close(struct net_device * dev);
extern void bcmemac_net_timeout(struct net_device * dev);
extern int bcmemac_net_xmit(struct sk_buff * skb, struct net_device * dev);
extern struct net_device_stats * bcmemac_net_query(struct net_device * dev);
extern int bcmemac_enet_poll(struct net_device * dev, int *budget);
extern void bcmemac_write_mac_address(struct net_device *dev);
extern struct net_device * bcmemac_get_device(void);
extern bool bcmemac_bridge_enabled(void);
extern void ar8328_write(struct net_device *dev, uint32 addr, uint32 data);

extern int force_bcmemac1_up;

/* IOCTL structure */
struct mmi_ioctl_data {
        uint16_t                phy_id;
        uint16_t                reg_num;
        uint16_t                val_in;
        uint16_t                val_out;
};

struct ioctl_stuff
{
    char                        name[16];
    union ioctl_data
    {
        unsigned int            *ptr;
        struct mmi_ioctl_data   mmi;
        unsigned char           MAC[8];
    } dat;
};

/* emoca driver */
extern int emoca_mdio_read(struct net_device *dev, int phy_id, int location);
extern void emoca_mdio_write(struct net_device *dev, int phy_id, int location, int val);

dk_context_t dk;                
dd_context_t dd; 
struct net_device *dev = NULL;
struct mii_if_info emoca_mii;
struct ethtool_ops emoca_ethtool_ops;
static unsigned int mii_phy_id = 1;
static bool bridge_enabled = FALSE;
static bool chr_opened = FALSE;
static DEFINE_MUTEX(emoca_mutex);
#define lock() mutex_lock(&emoca_mutex)
#define unlock() mutex_unlock(&emoca_mutex)

module_param(mii_phy_id, uint, S_IRUGO);

/*
 * Return the PHY ID 
 */
int emoca_get_mii_phy_id(void)
{
        return mii_phy_id;
}

/**
 * Open the mochactl
 */
static int emoca_chr_open(struct inode *inode, struct file * file)
{       
	// printk(KERN_INFO "MoCA char device opened\n");
	chr_opened = TRUE;
	return 0;
}

/**
 * Close the mochactl
 */
static int emoca_chr_close(struct inode *inode, struct file * file)
{
	// printk(KERN_INFO "MoCA char device closed\n");
        chr_opened = FALSE;
        return 0;
}

/**
 * emoca_ethtool_get_drvinfo: Do ETHTOOL_GDRVINFO ioctl
 * @dev: MoCA interface 
 * @drv_info: Ethernet driver name & version number
 *
 * Implements the "ETHTOOL_GDRVINFO" ethtool ioctl operation.
 * Returns driver name and version number.
 */
static void emoca_ethtool_get_drvinfo(struct net_device *dev, struct ethtool_drvinfo *drv_info)
{
        (void)dev;

        /*
         * The Entropic user space software will only place nice with one of a small
         * number of known drivers. Masquerade as "CandD_dvr" driver in order to keep
         * Entropic daemon happy.
         */
        strncpy(drv_info->driver, "CandD_dvr", sizeof(drv_info->driver)-1);
        strncpy(drv_info->version, EMOCA_DRIVER_VER_STRING, sizeof(drv_info->version)-1);
}

/**
 * emoca_ethtool_get_settings: Do ETHTOOL_GSET ioctl
 * @dev: MoCA interface 
 * @cmd: Ethernet MAC/PHY settings
 *
 * Implements the "ETHTOOL_GSET" ethtool ioctl operation.
 * Retrieves Ethernet MAC/PHY settings
 */
static int emoca_ethtool_get_settings(struct net_device *dev, struct ethtool_cmd *cmd)
{
        int bmcr;
        int phy_id;
        BcmEnet_devctrl *dev_ctrl = netdev_priv(dev);

        BUG_ON(dev_ctrl == NULL);

        if (!bridge_enabled)
            phy_id = dev_ctrl->EnetInfo.ucPhyAddress;
        else
            phy_id = mii_phy_id;
    
        cmd->supported =
                SUPPORTED_10baseT_Half   |
                SUPPORTED_10baseT_Full   |
                SUPPORTED_100baseT_Half  |
                SUPPORTED_100baseT_Full  |
                SUPPORTED_1000baseT_Half |
                SUPPORTED_1000baseT_Full |
                SUPPORTED_MII            |
                SUPPORTED_Pause;

        cmd->phy_address = phy_id;
        cmd->port = PORT_MII;
        cmd->transceiver = XCVR_EXTERNAL;       

        /* Entropic EN2510 does not support auto-negotiation */
        cmd->advertising = 0;
        cmd->autoneg = AUTONEG_DISABLE;

        bmcr = emoca_mdio_read(dev, phy_id, MII_BMCR);
        cmd->duplex = bmcr & BMCR_FULLDPLX ? DUPLEX_FULL : DUPLEX_HALF;

        /* BMCR_SPEED1000 | BMCR_SPEED100 | Speed (mbps)
         * =============================================
         *       0        |       0       |       10
         *       0        |       1       |      100
         *       1        |       0       |     1000
         *       1        |       1       |   Reserved
         */
        switch (bmcr & (BMCR_SPEED100|BMCR_SPEED1000))
        {
        case 0:
                cmd->speed = SPEED_10;
                break;
                        
        case BMCR_SPEED100:
                cmd->speed = SPEED_100;
                break;

        case BMCR_SPEED1000:
                cmd->speed = SPEED_1000;
                break;

        case BMCR_SPEED100 | BMCR_SPEED1000:
                printk(KERN_ERR "%s: Invalid EN2510 MII interface speed!\n", __FUNCTION__);
                cmd->speed = SPEED_10;
                break;
        }

        // printk(KERN_INFO "%s: emoca ioctl %s\n", __FUNCTION__, "emoca_ethtool_get_settings");

        return 0;
}

/**
 * emoca_ethtool_set_settings Do ETHTOOL_SSET ioctl
 * @dev: MoCA interface 
 * @cmd: Ethernet MAC/PHY settings
 *
 * Implements the "ETHTOOL_SSET" ethtool ioctl operation.
 * Configures Ethernet MAC/PHY settings
 */
static int emoca_ethtool_set_settings(struct net_device *dev, struct ethtool_cmd *cmd)
{
        int bmcr;
        int phy_id;
        unsigned long flags;
        BcmEnet_devctrl *dev_ctrl = netdev_priv(dev);
        int duplex = cmd->duplex;
        int speed = cmd->speed;
        
        BUG_ON(dev_ctrl == NULL);

        if (!bridge_enabled)
            phy_id = dev_ctrl->EnetInfo.ucPhyAddress;
        else
            phy_id = mii_phy_id;

        if (cmd->phy_address != phy_id) return -EINVAL;

        /* Fixed settings */
        if (cmd->port != PORT_MII              ||
            cmd->transceiver != XCVR_EXTERNAL  ||
            cmd->autoneg != AUTONEG_DISABLE) {
                return -EINVAL;
        }

        if (!(duplex == DUPLEX_HALF || duplex == DUPLEX_FULL))  {
                return -EINVAL;
        }
        
        /* EN2510 is capable of 1000 Mbit operation but the BCM7413
         * MII interface is not
         */
        if (!(speed == SPEED_10 || speed == SPEED_100)) {
                return -EINVAL;
        }

        bmcr = emoca_mdio_read(dev, phy_id, MII_BMCR);

        /* BMCR_SPEED1000 | BMCR_SPEED100 | Speed (mbps)
         * =============================================
         *       0        |       0       |       10
         *       0        |       1       |      100
         *       1        |       0       |     1000
         *       1        |       1       |   Reserved
         */
        
        bmcr &= ~(BMCR_SPEED100|BMCR_SPEED1000);
        
        if (speed == SPEED_100) {
                bmcr |= BMCR_SPEED100;
        }
        else if (speed == SPEED_1000) {
                bmcr |= BMCR_SPEED1000;
        }
        
        emoca_mdio_write(dev, phy_id, MII_BMCR, bmcr);

        spin_lock_irqsave(&dev_ctrl->lock, flags);

        if (duplex == DUPLEX_FULL) {
                bmcr |= BMCR_FULLDPLX;
                dev_ctrl->emac->txControl |= EMAC_FD; 
        }
        else {
                bmcr &= ~BMCR_FULLDPLX;
                dev_ctrl->emac->txControl &= ~EMAC_FD; 
        }

        spin_unlock_irqrestore(&dev_ctrl->lock, flags);

        // printk(KERN_INFO "%s: emoca ioctl %s\n", __FUNCTION__, "emoca_ethtool_set_settings");

        return 0;
}

/**
 * emoca_ethtool_get_pauseparam: Do ETHTOOL_GPAUSEPARAM ioctl
 * @dev: MoCA interface 
 * @params: Ethernet MAC/PHY flow control settings
 *
 * Implements the "ETHTOOL_GPAUSEPARAM" ethtool ioctl operation.
 * Retrieves Ethernet MAC/PHY flow control settings
 */
void emoca_ethtool_get_pauseparam(struct net_device *dev, struct ethtool_pauseparam* params)
{
        unsigned long flags;
        BcmEnet_devctrl *dev_ctrl = netdev_priv(dev);
        BUG_ON(dev_ctrl == NULL);

        /* Entropic EN2510 does not support auto-negotiation */
        params->autoneg = AUTONEG_DISABLE;

        spin_lock_irqsave(&dev_ctrl->lock, flags);
        
        if (dev_ctrl->emac->rxControl & EMAC_FC_EN) {
                params->rx_pause = params->tx_pause = 1;
        }
        else {
                params->rx_pause = params->tx_pause = 0;
        }

        spin_unlock_irqrestore(&dev_ctrl->lock, flags);

        // printk(KERN_INFO "%s: emoca ioctl %s\n", __FUNCTION__, "emoca_ethtool_get_pauseparam");
}

/**
 * emoca_ethtool_get_pauseparam: Do ETHTOOL_SPAUSEPARAM ioctl
 * @dev: MoCA interface 
 * @params: Ethernet MAC/PHY flow control settings
 *
 * Implements the "ETHTOOL_SPAUSEPARAM" ethtool ioctl operation.
 * Sets Ethernet MAC/PHY flow control settings
 */
int  emoca_ethtool_set_pauseparam(struct net_device *dev, struct ethtool_pauseparam* params)
{
        unsigned long flags;
        BcmEnet_devctrl *dev_ctrl = netdev_priv(dev);
        BUG_ON(dev_ctrl == NULL);

        /* Entropic EN2510 does not support auto-negotiation */
        if (params->autoneg != AUTONEG_DISABLE) return -EINVAL;

        spin_lock_irqsave(&dev_ctrl->lock, flags);

        /*
         * BCM7413 EMAC does not support asymmetric flow control.
         * If either RX pause or TX pause is requested enable
         * flow control
         */
        if (params->rx_pause || params->tx_pause) {
                dev_ctrl->emac->rxControl |= EMAC_FC_EN;  
        }
        else {
                dev_ctrl->emac->rxControl &= ~EMAC_FC_EN;               
        }

        spin_unlock_irqrestore(&dev_ctrl->lock, flags);

        // printk(KERN_INFO "%s: emoca ioctl %s\n", __FUNCTION__, "emoca_ethtool_set_pauseparam");

        return 0;
}

/**
 * emoca_open: Start MoCA interface
 * @dev: MoCA interface 
 *
 * Called in response to "ifconfig moca0 up"
 */
int emoca_open(struct net_device *dev)
{
        return (bcmemac_net_open(dev));
}

/**
 * emoca_stop: Stop MoCA interface
 * @dev: MoCA interface 
 *
 * Called in response to "ifconfig moca0 down"
 */
int emoca_stop(struct net_device *dev)  
{
        return (bcmemac_net_close(dev));
}

/**
 * emoca_eth_tx: Transmit Ethernet frame
 * @skb: socket buffer containing Ethernet frame
 * @dev: MoCA interface 
 */
int emoca_eth_tx(struct sk_buff *skb, struct net_device *dev)
{
        return (bcmemac_net_xmit(skb, dev));
}

/**
 * emoca_tx_timeout: Kick transmitter
 * @dev: MoCA interface 
 *
 * Called by kernel when packet transmission stalls
 */
void emoca_tx_timeout(struct net_device *dev)
{
        bcmemac_net_timeout(dev);
}

/**
 * emoca_get_stats: Get Ethernet stats
 * @dev: MoCA interface 
 *
 * Returns Ethernet stats for MoCA interface
 */
struct net_device_stats* emoca_get_stats(struct net_device *dev)
{
        return (bcmemac_net_query(dev));
}

/**
 * emoca_poll: Poll for received packets
 * @dev: MoCA interface 
 * @quota: Max number of RX packets to process
 *
 * Polls for received packets when packet RX interrupt
 * is disabled
 */
int emoca_poll(struct net_device *dev, int *quota)
{
        return (bcmemac_enet_poll(dev, quota));
}

/**
 * emoca_ioctl: Do ioctl operations
 * @dev: MoCA interface 
 * @ifr: ioctl command data
 * @cmd: ioctl command
 *
 * Handles ioctl command issued to MoCA driver
 */
int emoca_ioctl(struct net_device *dev, struct ifreq *ifr, int cmd)
{
        int ret = 0;
        
        switch (cmd)
        {

        case SIOCGMIIPHY:
        case SIOCGMIIREG:
        case SIOCSMIIREG:
                /* Uses generic routines provided by kernel to read and
                 * write MII registers and retrive MII PHY address
                 */
                // printk(KERN_INFO "%s: emoca ioctl %s\n", __FUNCTION__, "SIOCGMIIPHY");
                ret = generic_mii_ioctl(&emoca_mii, if_mii(ifr), cmd, NULL);
                break;
         
        case SIOCHDRCMD:
        {
                /* Used by Entropic daemon to control reset and powerdown inputs. Control
                 * of these signals was pushed up from the driver to the daemon, so this 
                 * IOCTL is a NOP. 
                 */
                ret = 0;
                break;
        }
        
        case SIOCCLINKDRV:
                /* Control plane commands for the driver */
                // printk(KERN_INFO "%s: emoca ioctl %s\n", __FUNCTION__, "SIOCCLINKDRV");
                lock();
                ret = clnkioc_driver_cmd( &dk, ifr->ifr_data ) ;
                unlock();
                break ;
        
        case SIOCGCLINKMEM:
                /* Reads registers/memory in c.LINK address space */
                // printk(KERN_INFO "%s: emoca ioctl %s\n", __FUNCTION__, "SIOCGCLINKMEM");
                lock();
                ret = clnkioc_mem_read( &dk, ifr->ifr_data ) ;
                unlock();
                break ;
        
        case SIOCSCLINKMEM:
                /* Sets registers/memory in c.LINK address space */
                // printk(KERN_INFO "%s: emoca ioctl %s\n", __FUNCTION__, "SIOCSCLINKMEM");
                lock();
                ret = clnkioc_mem_write( &dk, ifr->ifr_data ) ;
                unlock();
                break ;
        
        case SIOCGCLNKCMD:
                /* mbox cmmds: request with response */
                // printk(KERN_INFO "%s: emoca ioctl %s\n", __FUNCTION__, "SIOCGCLNKCMD");
                lock();
                ret = clnkioc_mbox_cmd_request( &dk, ifr->ifr_data, 1 ) ;
                unlock();
                break ;
        
        case SIOCSCLNKCMD:
                /* mbox cmmds: request with no response */
                // printk(KERN_INFO "%s: emoca ioctl %s\n", __FUNCTION__, "SIOCSCLNKCMD");
                lock();
                ret = clnkioc_mbox_cmd_request( &dk, ifr->ifr_data, 0 ) ;
                unlock();
                break ;
        
        case SIOCLNKDRV:
                /* mbox cmmds: retrieve unsol messages */
                // printk(KERN_INFO "%s: emoca ioctl %s\n", __FUNCTION__, "SIOCLNKDRV");
                lock();
                ret = clnkioc_mbox_unsolq_retrieve( &dk, ifr->ifr_data ) ;
                unlock();
                break ;
    
        default:
                ret = -EOPNOTSUPP;
                break;
        }
        
        return ret;
}

/**
 * IOCTL for mochactl
 */
static int emoca_chr_ioctl(struct inode *inode, struct file *file,
               unsigned int cmd, unsigned long arg)
{
        struct ifreq    *ifr   = (struct ifreq *)arg;
        struct ioctl_stuff wk;
        u32 ethcmd;
        struct ethtool_drvinfo drv_info;
        
        /*
        if (!chr_opened)
          return 1; */

        switch (cmd)
        {
        case SIOCETHTOOL:

                // printk(KERN_INFO "%s: emoca ioctl %s\n", __FUNCTION__, "SIOCETHTOOL");
                copy_from_user((char *)&wk, (char *) ifr, sizeof(struct ioctl_stuff)); 
                copy_from_user(&ethcmd, wk.dat.ptr, sizeof(ethcmd));
                
                if (ethcmd == ETHTOOL_GDRVINFO)
                {
                        emoca_ethtool_get_drvinfo(dev, &drv_info);
                        copy_to_user(wk.dat.ptr, &drv_info, sizeof(drv_info));
                        copy_to_user(ifr, &wk, sizeof(struct ioctl_stuff));
                }
                // implement the ETHTOOL for non-bridge mode
                // for bridge mode, it is set default when bridge is init
                else if (ethcmd == ETHTOOL_GSET && !bridge_enabled)
                {
                        struct ethtool_cmd et_settings;
                        
                        lock();          
                        emoca_ethtool_get_settings(dev, &et_settings);
                        unlock();                       
                        copy_to_user(wk.dat.ptr, &et_settings, sizeof(et_settings)); 
                        copy_to_user(ifr, &wk, sizeof(struct ioctl_stuff));
                }
                else if (ethcmd == ETHTOOL_SSET && !bridge_enabled)
                {
                        struct ethtool_cmd et_settings;
    
                        copy_from_user(&et_settings, wk.dat.ptr, sizeof(et_settings));
                        lock();                       
                        emoca_ethtool_set_settings(dev, &et_settings); 
						unlock();
                }
                else if (ethcmd == ETHTOOL_GPAUSEPARAM && !bridge_enabled)
                {
                        struct ethtool_pauseparam epause;

                        memset(&epause, 0, sizeof(epause));

                        lock();                 
                        emoca_ethtool_get_pauseparam(dev, &epause);
                        unlock();                       
                        copy_to_user(wk.dat.ptr, &epause, sizeof(epause)); 
                        copy_to_user(ifr, &wk, sizeof(struct ioctl_stuff));
                }
                else if (ethcmd == ETHTOOL_SPAUSEPARAM && !bridge_enabled)
                {
                        struct ethtool_pauseparam epause;

                        copy_from_user(&epause, wk.dat.ptr, sizeof(epause));

                        lock();                 
                        emoca_ethtool_set_pauseparam(dev, &epause);
                        unlock();                       
                }
                break;

        case SIOCGIFHWADDR:

                // printk(KERN_INFO "%s: emoca ioctl %s\n", __FUNCTION__, "SIOCGIFHWADDR");
                copy_from_user((char *)&wk, (char *) ifr, sizeof(struct ioctl_stuff));
                memcpy(&wk.dat.MAC[2], dev->dev_addr, 6);
                copy_to_user(ifr, &wk, sizeof(struct ioctl_stuff));
                break;
                
        case SIOCLINKCHANGE: /* Used by the daemon to signal to the driver that the MoCA link status
                              * has changed (i.e. EN2510 has joined/left a MoCA network)
                              */
                
                /* If an Ethernet switch is present (i.e. if this is a Helium) the MAC within the
                 * switch to which the EN2510 is connected must be disabled unless the EN2510 is booted.
                 * When the EN2510 is powered down the MII clock inputs to the switch MAC are not being
                 * driven; if the MAC is enabled in this state it queues up all broadcast and multicast
                 * packets forwarded from other switch ports and eventually exhausts all of the packet
                 * buffers within the switch.
                 */
                if (bridge_enabled) {

                        int link_status = 0;

                        if (get_user(link_status, (int __user *)ifr->ifr_data)) {
                                return -EFAULT;
                        }

                        if (link_status) {   /* Link up, EN2510 has joined a MoCA network   */

                                lock();
                                force_bcmemac1_up = 1; // tell kernel
                                /* Port 6 Pad Mode Control Register: set Mac6_mac_mii_en    */
                                /* Port 6 Status Register: full duplex, RX flow control,    */
                                /*                         RX and TX MAC enable, 100M (MII) */
                                ar8328_write(dev, 0x000c, 4);         
                                ar8328_write(dev, 0x0094, 0x0000006D);  
                                unlock();                        
                        }
                        else {               /* Link down, EN2510 has left a MoCA network   */

                                lock();
                                force_bcmemac1_up = 0; // tell kernel
                                /* Port 6 Pad Mode Control Register: clear Mac6_mac_mii_en  */
                                /* Port 6 Status Register: clear RX and TX MAC enable       */
                                ar8328_write(dev, 0x000c, 0);         
                                ar8328_write(dev, 0x0094, 0x00000061);  
                                unlock();
                        }
                }
                break;

        default:
                return (emoca_ioctl(dev, ifr, cmd));
        }

        return 0;
}

/**
 * emoca_set_multicast_list
 * @dev: MoCA interface 
 *
 * Called by kernel when interface flags change
 */
void emoca_set_multicast_list(struct net_device *dev)
{
        bcmemac_set_multicast_list(dev);
}

/**
 * emoca_set_mac_address: Set Ethernet MAC address
 * @dev: MoCA interface 
 * @addr: Ethernet MAC address
 */
int emoca_set_mac_address(struct net_device *dev, void *addr)
{
        return (bcmemac_set_mac_addr(dev, addr));
}

/* Access functions for EMOCHA char driver */
static const struct file_operations emoca_chr_fops = {
        .owner  = THIS_MODULE,
        .open   = emoca_chr_open,
        .release = emoca_chr_close,
        .ioctl = emoca_chr_ioctl, 
};

/* mknod /dev/mocactl c 10 1 */
static struct miscdevice emoca_miscdev = {
        .minor = 1,
        .name = "mocactl",
        .fops = &emoca_chr_fops,
};

/**
 * emoca_init: Initialize emoca module
 */
static int __init emoca_init(void)
{
        struct BcmEnet_devctrl *dev_ctrl;
        int bmcr;
        int ret = 0;

        if (bcmemac_bridge_enabled())
        {
            /* if bridge is there, MoCA is connected to PHY ID 8 */
               bridge_enabled = TRUE;
               mii_phy_id = 8;
        } 
        else
        {
               bridge_enabled = FALSE;
               mii_phy_id = 1;
        }

        /* Register char driver for IOCTL */
        ret = misc_register(&emoca_miscdev);

        if (ret)
        {
              printk(KERN_ERR "%s: Can not register misc device\n", __FUNCTION__);
              goto out1;
        }

        if (!bridge_enabled)
        {
#ifdef CONFIG_BRCM_PM
               /* Register with Broadcom Power Management Module  */
               brcm_pm_enet_add();
#endif
               /* Allocate struct net_device and struct BrcmEnet_devctrl */
               dev = alloc_netdev(sizeof(struct BcmEnet_devctrl), "moca%d", ether_setup);

               if (!dev) {
                 ret = -EFAULT;
                 goto out1;
               }
        
               dev->base_addr = ENET_MAC_1_ADR_BASE;

               /* brcmint7038 Ethernet driver uses device private memory
                * accessed by 'netdev_priv' to store its context data
                */
               dev_ctrl = (BcmEnet_devctrl *)netdev_priv(dev);
               dev_ctrl->dev = dev;
               dev_ctrl->devnum = 1;
               dev_ctrl->linkstatus_phyport = -1;
               dev_ctrl->bIPHdrOptimize = 1;

               /* Masquerading as an Ethernet switch with a single port
                * will prevent the brcmint7038 Ethernet driver from trying
                * to start auto-negotation using the Entropic EN2510 PHY
                */
               dev_ctrl->EnetInfo.ucPhyType = BP_ENET_EXTERNAL_SWITCH;
               dev_ctrl->EnetInfo.ucPhyAddress = mii_phy_id;
               dev_ctrl->EnetInfo.numSwitchPorts = 1;
               dev_ctrl->EnetInfo.usManagementSwitch = BP_NOT_DEFINED;
               dev_ctrl->EnetInfo.usConfigType = BP_ENET_CONFIG_MDIO;
               dev_ctrl->EnetInfo.usReverseMii = BP_NOT_DEFINED;

               /* Initialize brcmint7038 spinlock */
               spin_lock_init(&dev_ctrl->lock);
        
               atomic_set(&dev_ctrl->devInUsed, 0);

               /* bcmemac_tx_reclaim_timer is used to reclaim buffers after
                * Ethernet packet transmission is complete. It accesses the
                * EMAC registers from software interrupt context; the spinlock
                * above is used to protect accesses to the EMAC registers
                */
              init_timer(&dev_ctrl->timer);
              dev_ctrl->timer.data = (unsigned long)dev_ctrl;
              dev_ctrl->timer.function = bcmemac_tx_reclaim_timer;

              /* Initialize link_change_task workqueue */
              INIT_WORK(&dev_ctrl->link_change_task, (void (*)(void *))bcmemac_link_change_task, dev_ctrl);

              /* Use bcmemac_init_dev function to initialize struct BrcmEnet_devctrl */
              if (bcmemac_init_dev(dev_ctrl)) {
                ret = -EFAULT;
                goto out2;
              }
        }
        else
        {
              /* For bridge case, net device come from eth0 */
              dev = bcmemac_get_device();
              if (!dev) {
                ret = -EFAULT;
                goto out1;
              }
              dev_ctrl = (BcmEnet_devctrl *)netdev_priv(dev);
        }

        /* Retrieve MAC address from PROM */
        if (!bridge_enabled) 
        {
                extern int gNumHwAddrs;
                extern unsigned char* gHwAddrs[];
                
                if (gNumHwAddrs > 1) {
                        memcpy(dev->dev_addr, gHwAddrs[1], ETH_ALEN);
                }
                else {
                        printk(KERN_ERR "%s: MoCA MAC address undefined\n", __FUNCTION__);
                        /* Use Ethernet MAC address, modify one octet */
                        memcpy(dev->dev_addr, gHwAddrs[0], ETH_ALEN);
                        dev->dev_addr[4] += 1;
                }
                /* Write MAC address to EMAC1 perfect match registers */
                bcmemac_write_mac_address(dev);
        
                ret = request_irq(dev_ctrl->rxIrq,
                          bcmemac_net_isr,
                          SA_INTERRUPT|SA_SHIRQ|IRQF_SAMPLE_RANDOM,
                          dev->name,
                          dev_ctrl);

               if (ret) {
                  goto out3;
               }

               dev_ctrl->dmaRegs->enet_iudma_r5k_irq_msk |= 0x2;
        }

        /* Allocate and initialize Entropic context structs  */
        dk.dev = dev;
        dk.priv = &dd;
        
        if (Clnk_init_dev(&dd.p_dg_ctx, &dd, &dk, 0)) {
                ret = -ENOMEM;
                goto out4;
        }

        /* For use with generic MII ioctl operations in drivers/net/mii.c */
        emoca_mii.phy_id = mii_phy_id;
        emoca_mii.phy_id_mask = 0x1F;
        emoca_mii.reg_num_mask = 0x1F;
        emoca_mii.full_duplex = 1;
        emoca_mii.force_media = 1;
        emoca_mii.supports_gmii = 0;
        emoca_mii.dev = dev;
        emoca_mii.mdio_read = emoca_mdio_read;
        emoca_mii.mdio_write = emoca_mdio_write;

        if (!bridge_enabled)
        {
                /* Enable ETHTOOL ioctl commands */
                memset(&emoca_ethtool_ops, 0, sizeof(emoca_ethtool_ops));
                emoca_ethtool_ops.get_drvinfo = emoca_ethtool_get_drvinfo;
                emoca_ethtool_ops.get_settings = emoca_ethtool_get_settings;
                emoca_ethtool_ops.set_settings = emoca_ethtool_set_settings;
                emoca_ethtool_ops.get_pauseparam = emoca_ethtool_get_pauseparam;
                emoca_ethtool_ops.set_pauseparam = emoca_ethtool_set_pauseparam;
                SET_ETHTOOL_OPS(dev, &emoca_ethtool_ops);
        }

        if (!bridge_enabled)
        {

                 bmcr = emoca_mdio_read(dev, mii_phy_id, MII_BMCR);

                 dev_ctrl->emac->config |= EMAC_DISABLE ;
                 while(dev_ctrl->emac->config & EMAC_DISABLE);

                 /* Configure EMAC duplex setting to match EN2510 PHY */
                if (bmcr & BMCR_FULLDPLX) {
                    dev_ctrl->emac->txControl |= EMAC_FD; 
                }
                else {
                    dev_ctrl->emac->txControl &= ~EMAC_FD;
                }

                /* The seven LSBs of the EMAC1 MII Status/Control register
                 * control a clock divider that sets the MDC clock frequency
                 * where:
                 *
                 * MDC freq. = System clock freq. / (2 * Status/Control[6:0])
                 *
                 * The system clock is 108 MHz (BCM7413 B0 Programmer’s Register
                 * Reference Guide p. 201) and the max MDC clock frequency
                 * supported by the EN2510 is 25 MHz (EN2510 Datasheet, section
                 * 4.4.8). The MDC clock is set to 1.75 MHz below to minimize the
                 * radiated emissions generated by the MDC clock trace.
                 */
                 dev_ctrl->emac->mdioFreq = EMAC_MII_PRE_EN | 0x1F; 
                 dev_ctrl->emac->config |= EMAC_ENABLE;  
        }
        else
        {        
                /* 
                 dev_ctrl->emac->config |= EMAC_DISABLE ;
                 while(dev_ctrl->emac->config & EMAC_DISABLE);

                 dev_ctrl->emac->mdioFreq = EMAC_MII_PRE_EN | 0x01F; 
                 dev_ctrl->emac->txControl |= EMAC_FD; 
                 dev_ctrl->emac->config |= EMAC_ENABLE; */
        }
	
        /* Override relevant entries in struct net dev */
        if (!bridge_enabled)
        {
                dev->irq                    = dev_ctrl->rxIrq;
   	        dev->open                   = emoca_open;
   	        dev->stop                   = emoca_stop;
   	        dev->hard_start_xmit        = emoca_eth_tx;
   	        dev->tx_timeout             = emoca_tx_timeout;
   	        dev->watchdog_timeo         = 2*HZ;
   	        dev->get_stats              = emoca_get_stats;
	        dev->set_mac_address        = emoca_set_mac_address;
   	        dev->set_multicast_list     = emoca_set_multicast_list;
	        dev->do_ioctl               = emoca_ioctl;
                dev->poll                   = emoca_poll;
                dev->weight                 = 64;

	        /* Register network interface with kernel */
	        ret = register_netdev(dev);
        }

	if (!ret) 
        {
                printk(KERN_INFO "%s: emoca_init OK\n", __FUNCTION__);
                return 0;
        }
	
	/* Free Entropic context structs */
	Clnk_exit_dev(dd.p_dg_ctx);
out4:
        if (!bridge_enabled)
        {
	        free_irq(dev_ctrl->rxIrq, dev_ctrl);
	        dev_ctrl->dmaRegs->enet_iudma_r5k_irq_msk &= ~0x2;
        }
out3:	
        if (!bridge_enabled)
        {
	        /* Use bcmemac_uninitdev to free memory allocated by brcmint7038 driver */
	        bcmemac_uninit_dev(dev_ctrl);	
        }
out2:
        if (!bridge_enabled)
        {
	        /* Free memory allocated for struct net_device and struct BrcmEnet_devctrl */
	        free_netdev(dev);
        }
out1:
	/* Un-register with Broadcom Power Management Module */
        if (!bridge_enabled)
        {
#ifdef CONFIG_BRCM_PM
	        brcm_pm_enet_remove(); 
#endif
        }
	return ret;
}
module_init(emoca_init);


/**
 * emoca_init: emoca module cleanup
 *
 * Note: netdev_unregister and netdev_free
 *       are handled by bcmemac_uninit_dev
 */
static void __exit emoca_exit(void)
{
	struct BcmEnet_devctrl *dev_ctrl;
	BUG_ON(dev == NULL);

        if (!bridge_enabled)
        {
	        dev_ctrl = netdev_priv(dev);
	        bcmemac_uninit_dev(dev_ctrl);		
        }
	Clnk_exit_dev(dd.p_dg_ctx);

        if (!bridge_enabled)
        {
#ifdef CONFIG_BRCM_PM
	        brcm_pm_enet_remove();
#endif
        }

        misc_deregister(&emoca_miscdev);
}
module_exit(emoca_exit);
