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
/* Power management in this driver is borken */
#undef CONFIG_BRCM_PM
#ifdef CONFIG_BRCM_PM
#include <asm/brcmstb/common/brcm-pm.h>
#endif
#include "bcmmii.h"
#include "bcmgenet.h"
#include "drv_hdr.h"
#include "data_context.h"

#include <linux/tivoconfig.h>

#define EMOCA_DRIVER_NAME        "emoca"
#define EMOCA_DRIVER_VER_STRING  "1.0"
#define EMOCA_MAJOR              60

#ifdef MOCA_DEBUG
#define DBPRINTF(x)	printk x
#else
#define DBPRINTF(x)
#endif

/* bcmgenet Ethernet driver */
extern void bcmgenet_tx_reclaim_timer(unsigned long arg);
extern int bcmgenet_init_dev(struct BcmEnet_devctrl *pDevCtrl);
extern int bcmgenet_uninit_dev(struct BcmEnet_devctrl *pDevCtrl);
extern irqreturn_t bcmgenet_net_isr(int irq, void *, struct pt_regs *regs);
extern void bcmgenet_link_change_task(struct BcmEnet_devctrl *pDevCtrl);
extern void bcmgenet_set_multicast_list(struct net_device * dev);
extern int bcmgenet_set_mac_addr(struct net_device *dev, void *p);
extern int bcmgenet_net_open(struct net_device * dev);
extern int bcmgenet_net_close(struct net_device * dev);
extern void bcmgenet_net_timeout(struct net_device * dev);
extern int bcmgenet_net_xmit(struct sk_buff * skb, struct net_device * dev);
extern struct net_device_stats * bcmgenet_net_query(struct net_device * dev);
extern int bcmgenet_enet_poll(struct net_device * dev, int *budget);
extern void bcmgenet_write_mac_address(struct net_device *dev);
extern struct net_device * bcmemac_get_device(void);
extern bool bcmgenet_bridge_enabled(void);
extern void athsw_write_reg(struct net_device *dev, uint32_t addr, uint32_t data);
extern unsigned long clnkioc_moca_shell_cmd( void *dkcp, void *arg );
extern int ethtool_ioctl(struct ifreq *ifr);


ssize_t memory_write(   struct file *filp,
                        const char *buf,
                        size_t count,
                        loff_t *f_pos);

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
static unsigned int mii_phy_id = 7;
static bool chr_opened = 0;
static DEFINE_MUTEX(emoca_mutex);
#define lock() mutex_lock(&emoca_mutex)
#define unlock() mutex_unlock(&emoca_mutex)

#ifdef MODULE
module_param(mii_phy_id, uint, S_IRUGO);
#else
static int __init phy_id_setup(char *s)
{
	mii_phy_id = simple_strtoul(s,NULL,0);
	return 1;
}
__setup("mocaphy=", phy_id_setup);
#endif


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
	DBPRINTF((KERN_DEBUG "MoCA char device opened\n"));
	chr_opened = 1;
	return 0;
}

/**
 * Close the mochactl
 */
static int emoca_chr_close(struct inode *inode, struct file * file)
{
	DBPRINTF((KERN_DEBUG "MoCA char device closed\n"));
        chr_opened = 0;
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
                DBPRINTF((KERN_DEBUG "%s: emoca ioctl %s\n", __FUNCTION__, "SIOCGMIIPHY"));
                ret = generic_mii_ioctl(&emoca_mii, if_mii(ifr), cmd, NULL);
                break;
         
        case SIOCHDRCMD:
        {
#if 0
                /* Used by Entropic daemon to control reset and powerdown inputs. Control
                 * of these signals was pushed up from the driver to the daemon, so this 
                 * IOCTL is a NOP. 
                 */
		volatile int *regs = (volatile int *)0x90406500; // ODEN reg
		int flag = ifr->ifr_ifru.ifru_ivalue;

		/* XXX We should use the GPIO driver for this. */
#define GIO_SET(x) { regs[1] = ~(1<<(x)) & regs[1]; \
			regs[2] = ~(1<<(x)) & regs[2]; }
#define GIO_CLR(x) { regs[1] = ~(1<<(x)) & regs[1]; \
			regs[2] = ~(1<<(x)) & regs[2]; }

                ret = 0;
		switch (flag) {
		case 1:
			/* Hold chip in reset */
			GIO_SET(4);
			break;
		case 0:
			/* Clear chip reset */
			GIO_CLR(4);
			break;
		case 4:
			/* Hold PLL reset */
		case 5:
			/* Clear PLL reset */
			/* XXX Don't have no steenkin PLL. */
			break;
		default:
			/* Unsupported operations */
			ret = -EIO;
			break;
		}
#else
		ret = -EIO;
#endif
                break;
        }
        
        case SIOCCLINKDRV:
                /* Control plane commands for the driver */
                DBPRINTF((KERN_DEBUG "%s: emoca ioctl %s\n", __FUNCTION__, "SIOCCLINKDRV"));
                lock();
                ret = clnkioc_driver_cmd( &dk, ifr->ifr_data ) ;
                unlock();
                break ;
        
        case SIOCGCLINKMEM:
                /* Reads registers/memory in c.LINK address space */
                DBPRINTF((KERN_DEBUG "%s: emoca ioctl %s\n", __FUNCTION__, "SIOCGCLINKMEM"));
                lock();
                ret = clnkioc_mem_read( &dk, ifr->ifr_data ) ;
                unlock();
                break ;
        
        case SIOCSCLINKMEM:
                /* Sets registers/memory in c.LINK address space */
                DBPRINTF((KERN_DEBUG "%s: emoca ioctl %s\n", __FUNCTION__, "SIOCSCLINKMEM"));
                lock();
                ret = clnkioc_mem_write( &dk, ifr->ifr_data ) ;
                unlock();
                break ;


		/*
		 * The following ioctl()s are disabled in the balboa CandD driver:
		 */
#if 0
        case SIOCGCLNKCMD:
                /* mbox cmmds: request with response */
                DBPRINTF((KERN_DEBUG "%s: emoca ioctl %s\n", __FUNCTION__, "SIOCGCLNKCMD"));
                lock();
                ret = clnkioc_mbox_cmd_request( &dk, ifr->ifr_data, 1 ) ;
                unlock();
                break ;
        
        case SIOCSCLNKCMD:
                /* mbox cmmds: request with no response */
                DBPRINTF((KERN_DEBUG "%s: emoca ioctl %s\n", __FUNCTION__, "SIOCSCLNKCMD"));
                lock();
                ret = clnkioc_mbox_cmd_request( &dk, ifr->ifr_data, 0 ) ;
                unlock();
                break ;
        
        case SIOCLNKDRV:
                /* mbox cmmds: retrieve unsol messages */
                DBPRINTF((KERN_DEBUG "%s: emoca ioctl %s\n", __FUNCTION__, "SIOCLNKDRV"));
                lock();
                ret = clnkioc_mbox_unsolq_retrieve( &dk, ifr->ifr_data ) ;
                unlock();
                break ;
#endif
        case SIOCMSCMD:         // moca shell commands
                DBPRINTF((KERN_DEBUG "%s: emoca ioctl %s\n", __FUNCTION__, "SIOCMSCMD"));
                lock();
		ret = clnkioc_moca_shell_cmd( &dk, ifr->ifr_data ) ;
		unlock();
            break ;

        default:
                printk(KERN_DEBUG "%s: emoca ioctl unsupported %x\n", 
		    __FUNCTION__, cmd);
                ret = -EOPNOTSUPP;
                break;
        }
        
        return ret;
}

/**
 * IOCTL for mochactl
 */
extern unsigned int boardID;

static long emoca_chr_ioctl(struct file *file,
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

                DBPRINTF((KERN_DEBUG "%s: emoca ioctl %s\n", __FUNCTION__, "SIOCETHTOOL"));
                copy_from_user((char *)&wk, (char *) ifr, sizeof(struct ioctl_stuff)); 
                copy_from_user(&ethcmd, wk.dat.ptr, sizeof(ethcmd));
                
                if (ethcmd == ETHTOOL_GDRVINFO)
                {
                        emoca_ethtool_get_drvinfo(dev, &drv_info);
                        copy_to_user(wk.dat.ptr, &drv_info, sizeof(drv_info));
                        copy_to_user(ifr, &wk, sizeof(struct ioctl_stuff));
			break;
                }
#if 0
                // implement the ETHTOOL for non-bridge mode
                // for bridge mode, it is set default when bridge is init
		return ethtool_ioctl(ifr);
#endif
		return -EIO;
		break;

        case SIOCGIFHWADDR:

                DBPRINTF((KERN_DEBUG "%s: emoca ioctl %s\n", __FUNCTION__, "SIOCGIFHWADDR"));
                copy_from_user((char *)&wk, (char *) ifr, sizeof(struct ioctl_stuff));
                memcpy(&wk.dat.MAC[2], dev->dev_addr, 6);
                copy_to_user(ifr, &wk, sizeof(struct ioctl_stuff));
                break;
                
        case SIOCLINKCHANGE: /* Used by the daemon to signal to the driver that the MoCA link status
                              * has changed (i.e. EN2510 has joined/left a MoCA network)
                              */
                
                DBPRINTF((KERN_DEBUG "%s: emoca ioctl %s\n", __FUNCTION__, "SIOCLINKCHANGE"));
                /* If an Ethernet switch is present (i.e. if this is a Helium) the MAC within the
                 * switch to which the EN2510 is connected must be disabled unless the EN2510 is booted.
                 * When the EN2510 is powered down the MII clock inputs to the switch MAC are not being
                 * driven; if the MAC is enabled in this state it queues up all broadcast and multicast
                 * packets forwarded from other switch ports and eventually exhausts all of the packet
                 * buffers within the switch.
                 */
		{

                        int link_status = 0;

                        if (get_user(link_status, (int __user *)ifr->ifr_data)) {
                                return -EFAULT;
                        }
                        if (link_status) {   /* Link up, EN2510 has joined a MoCA network   */

                                lock();
                                if( (boardID & 0x00FFFF00) == kTivoConfigBoardIDTitanRevC)
                                {
                                    //force_bcmgenet1_up = 1; // tell kernel
                                    /* Port 6 Pad Mode Control Register: set Mac6_mac_RGMii_en    */
                                    /* Port 6 Status Register: full duplex, RX flow control,    */
                                    /* RX and TX MAC enable, 100M (MII) */
                                    printk("Configuring moca to Port 6\n");
                                    athsw_write_reg(dev, 0x000c, 0x07600000);
                                    athsw_write_reg(dev, 0x0094, 0x0000007E);
                                 }
                                else //kTivoConfigBoardIDArgonXL
                                {
                                    printk("Configuring moca to Port 0\n");
                                    athsw_write_reg(dev, 0x0004, 0x07a00000);
                                    athsw_write_reg(dev, 0x007C, 0x0000007E);

                                }
                                unlock();                        
                        }
                        else {               /* Link down, EN2510 has left a MoCA network   */

                                lock();
                                /* Port 6 Pad Mode Control Register: clear Mac6_mac_RGMII_en  */
                                /* Port 6 Status Register: clear RX and TX MAC enable       */
                                if( (boardID & 0x00FFFF00) == kTivoConfigBoardIDTitanRevC)
                                {
                                    athsw_write_reg(dev, 0x000c, 0x01000000);
                                    athsw_write_reg(dev, 0x0094, 0x00000072);
                                }
                                else
                                {
                                    athsw_write_reg(dev, 0x0004, 0x01000000);
                                    athsw_write_reg(dev, 0x007C, 0x00000072);
                                }
                                unlock();
                        }
                }
                break;

        default:
//		printk(KERN_DEBUG "%s: emoca ioctl %x\n", __FUNCTION__, cmd);
                return (emoca_ioctl(dev, ifr, cmd));
        }

        return 0;
}

/* Access functions for EMOCHA char driver */
static const struct file_operations emoca_chr_fops = {
        .owner  = THIS_MODULE,
        .open   = emoca_chr_open,
        .release = emoca_chr_close,
        .unlocked_ioctl = emoca_chr_ioctl, 
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
static int __init init(void)
{
	struct BcmEnet_devctrl *dev_ctrl;
        int ret = 0;

	DBPRINTF(("In emoca_init\n"));

        /* Register char driver for IOCTL */
        ret = misc_register(&emoca_miscdev);

        if (ret)
        {
              printk(KERN_ERR "%s: Can not register misc device\n", __FUNCTION__);
	      goto out;
        }

	/* For bridge case, net device come from eth0 */
	dev = bcmemac_get_device();
	if (!dev) {
		ret = -EFAULT;
		goto out;
	}
	dev_ctrl = (struct BcmEnet_devctrl *)netdev_priv(dev);

        /* Allocate and initialize Entropic context structs  */
        dk.dev = dev;
        dk.priv = &dd;
        
        if (Clnk_init_dev(&dd.p_dg_ctx, &dd, &dk, 0)) {
                ret = -ENOMEM;
		goto out;
        }

        /* For use with generic MII ioctl operations in drivers/net/mii.c */
	emoca_mii = dev_ctrl->mii;

        emoca_mii.phy_id = mii_phy_id;

	if (!ret) 
        {
                printk(KERN_DEBUG "%s: emoca_init OK\n", __FUNCTION__);
                return 0;
        }

	/* Free Entropic context structs */
	Clnk_exit_dev(dd.p_dg_ctx);

out:
	DBPRINTF(("emoca_init: ret = %x\n", ret));

	return ret;
}
module_init(init);


/**
 * emoca_init: emoca module cleanup
 *
 * Note: netdev_unregister and netdev_free
 *       are handled by bcmgenet_uninit_dev
 */
static void __exit exit(void)
{
	BUG_ON(dev == NULL);

	Clnk_exit_dev(dd.p_dg_ctx);
        misc_deregister(&emoca_miscdev);
}
module_exit(exit);
