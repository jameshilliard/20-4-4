/*
 *
 * Copyright (c) 2002-2005 Broadcom Corporation
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 *
 *File Name  : bcmmii.c
 *
 *Description: Broadcom PHY/GPHY/Ethernet Switch Configuration
 *Revision:	09/25/2008, L.Sun created.
*/

#include "bcmgenet.h"
#include "bcmgenet_map.h"
#include "bcmmii.h"

#include <linux/types.h>
#include <linux/delay.h>
#include <linux/wait.h>
#include <linux/mii.h>
#include <linux/ethtool.h>
#include <linux/bitops.h>
#include <linux/netdevice.h>
#include <linux/platform_device.h>
#include <linux/brcmstb/brcmstb.h>

#ifdef CONFIG_TIVO
#include <linux/module.h>

// Break out the guts of mii read/write so we can call it from multiple places
// Also we don't use the waitq for some reason, just spin inline
// return -1 if failure
static int mii_read_guts(volatile struct uniMacRegs *umac, int phy_id, int location)
{
	unsigned long timeout;
	
        umac->mdio_cmd = (MDIO_RD | (phy_id << MDIO_PMD_SHIFT) | (location << MDIO_REG_SHIFT));
	// start MDIO transaction
	umac->mdio_cmd |= MDIO_START_BUSY;
        // spin for 10ms
        timeout = jiffies + HZ/100;
	while ((umac->mdio_cmd & MDIO_START_BUSY) && time_after(timeout, jiffies));
        if (umac->mdio_cmd & (MDIO_READ_FAIL|MDIO_START_BUSY)) return -1;
	return umac->mdio_cmd & 0xffff;
}            

// return non-zero if write failed
static int mii_write_guts(volatile struct uniMacRegs *umac, int phy_id, int location, int val)
{
	unsigned long timeout;
	
        umac->mdio_cmd = (MDIO_WR | (phy_id << MDIO_PMD_SHIFT) | (location << MDIO_REG_SHIFT) | (0xffff & val));
	umac->mdio_cmd |= MDIO_START_BUSY;
        // spin for 10mS
	timeout = jiffies + HZ/100;
	while ((umac->mdio_cmd & MDIO_START_BUSY) && time_after(timeout, jiffies));
	if (umac->mdio_cmd & MDIO_START_BUSY) return -1;
        return 0;
}
#endif

/* read a value from the MII */
int bcmgenet_mii_read(struct net_device *dev, int phy_id, int location)
{
	int ret;
	struct BcmEnet_devctrl *pDevCtrl = netdev_priv(dev);
	volatile struct uniMacRegs *umac = pDevCtrl->umac;

	if (phy_id == BRCM_PHY_ID_NONE) {
		switch (location) {
		case MII_BMCR:
#ifdef CONFIG_TIVO_PAC01
			return pDevCtrl->phyType == BRCM_PHY_TYPE_EXT_MII ?
				BMCR_FULLDPLX | BMCR_SPEED100 :
				BMCR_FULLDPLX | BMCR_SPEED1000;
#else
			return (pDevCtrl->phySpeed == SPEED_1000) ?
				BMCR_FULLDPLX | BMCR_SPEED1000 :
				BMCR_FULLDPLX | BMCR_SPEED100;
#endif
		case MII_BMSR:
#ifdef CONFIG_TIVO_PAC01
			/* In Pace XG1 HW external switch does not support the MDIO and hence
				kernel needs to be explicitly fed the switch Link Status */
			return (BMSR_LSTATUS & pDevCtrl->switchlinkstat);
#else
			return BMSR_LSTATUS;
#endif
		default:
			return 0;
		}
	}

#ifndef CONFIG_TIVO        
	mutex_lock(&pDevCtrl->mdio_mutex);
	umac->mdio_cmd = (MDIO_RD | (phy_id << MDIO_PMD_SHIFT) |
			(location << MDIO_REG_SHIFT));
	/* Start MDIO transaction*/
	umac->mdio_cmd |= MDIO_START_BUSY;
	wait_event_timeout(pDevCtrl->wq, !(umac->mdio_cmd & MDIO_START_BUSY),
			HZ/100);
	ret = umac->mdio_cmd;
	mutex_unlock(&pDevCtrl->mdio_mutex);
	if (ret & MDIO_READ_FAIL) {
		TRACE(("MDIO read failure\n"));
		ret = 0;
	}
	return ret & 0xffff;
#else
	mutex_lock(&pDevCtrl->mdio_mutex);
        ret = mii_read_guts(umac, phy_id, location);
	mutex_unlock(&pDevCtrl->mdio_mutex);
        if (ret >= 0) return ret;
        printk("bcmgenet_mii_read failed\n");
        return 0;
#endif        
}

#ifdef CONFIG_TIVO
EXPORT_SYMBOL(bcmgenet_mii_read);
#endif

/* write a value to the MII */
void bcmgenet_mii_write(struct net_device *dev, int phy_id,
			int location, int val)
{
	struct BcmEnet_devctrl *pDevCtrl = netdev_priv(dev);
	volatile struct uniMacRegs *umac = pDevCtrl->umac;

	if (phy_id == BRCM_PHY_ID_NONE)
		return;
#ifndef CONFIG_TIVO
	mutex_lock(&pDevCtrl->mdio_mutex);
	umac->mdio_cmd = (MDIO_WR | (phy_id << MDIO_PMD_SHIFT) |
			(location << MDIO_REG_SHIFT) | (0xffff & val));
	umac->mdio_cmd |= MDIO_START_BUSY;
	wait_event_timeout(pDevCtrl->wq, !(umac->mdio_cmd & MDIO_START_BUSY),
			HZ/100);
	if (umac->mdio_cmd & MDIO_START_BUSY)
		printk("mii_write: timeout!\n");
	mutex_unlock(&pDevCtrl->mdio_mutex);
#else
	mutex_lock(&pDevCtrl->mdio_mutex);
        if (mii_write_guts(umac, phy_id, location, val))
		printk("bcmgenet_mii_write failed\n");
	mutex_unlock(&pDevCtrl->mdio_mutex);
#endif        
}

/* mii register read/modify/write helper function */
static int bcmgenet_mii_set_clr_bits(struct net_device *dev, int location,
		     int set_mask, int clr_mask)
{
	struct BcmEnet_devctrl *pDevCtrl = netdev_priv(dev);
	int phy_id = pDevCtrl->phyAddr;
	int v;

	v = bcmgenet_mii_read(dev, phy_id, location);
	v &= ~clr_mask;
	v |= set_mask;
	bcmgenet_mii_write(dev, phy_id, location, v);
	return v;
}

#ifdef CONFIG_TIVO
EXPORT_SYMBOL(bcmgenet_mii_write);
#endif

/* probe for an external PHY via MDIO; return PHY address */
int bcmgenet_mii_probe(struct net_device *dev, void *p)
{
	struct BcmEnet_devctrl *pDevCtrl = netdev_priv(dev);
	int i;
	struct bcmemac_platform_data *cfg = p;

	if (cfg->phy_type != BRCM_PHY_TYPE_EXT_MII &&
	    cfg->phy_type != BRCM_PHY_TYPE_EXT_RVMII) {
		/*
		 * Enable RGMII to interface external PHY, disable
		 * internal 10/100 MII.
		 */
		GENET_RGMII_OOB_CTRL(pDevCtrl) |= RGMII_MODE_EN;
	}
	/* Power down EPHY */
	if (pDevCtrl->ext)
		pDevCtrl->ext->ext_pwr_mgmt |= (EXT_PWR_DOWN_PHY |
			EXT_PWR_DOWN_DLL | EXT_PWR_DOWN_BIAS);

	for (i = 31; i >= 0; i--) {
		if (bcmgenet_mii_read(dev, i, MII_BMSR) != 0) {
			pDevCtrl->phyAddr = i;
			if (i == 1)
				continue;
			return 0;
		}
		TRACE(("I=%d\n", i));
	}
	return -ENODEV;
}

/*
 * setup netdev link state when PHY link status change and
 * update UMAC and RGMII block when link up
 */
void bcmgenet_mii_setup(struct net_device *dev)
{
	struct BcmEnet_devctrl *pDevCtrl = netdev_priv(dev);
	struct ethtool_cmd ecmd;
	volatile struct uniMacRegs *umac = pDevCtrl->umac;
	int cur_link, prev_link;
	unsigned int val, cmd_bits;

	if (pDevCtrl->phyType == BRCM_PHY_TYPE_MOCA)
		return;

	cur_link = mii_link_ok(&pDevCtrl->mii);
	prev_link = netif_carrier_ok(pDevCtrl->dev);
	if (cur_link && !prev_link) {
		mii_ethtool_gset(&pDevCtrl->mii, &ecmd);
		/*
		 * program UMAC and RGMII block based on established link
		 * speed, pause, and duplex.
		 * the speed set in umac->cmd tell RGMII block which clock
		 * 25MHz(100Mbps)/125MHz(1Gbps) to use for transmit.
		 * receive clock is provided by PHY.
		 */
		GENET_RGMII_OOB_CTRL(pDevCtrl) &= ~OOB_DISABLE;
		GENET_RGMII_OOB_CTRL(pDevCtrl) |= RGMII_LINK;

		/* speed */
		if (ecmd.speed == SPEED_1000)
			cmd_bits = UMAC_SPEED_1000;
		else if (ecmd.speed == SPEED_100)
			cmd_bits = UMAC_SPEED_100;
		else
			cmd_bits = UMAC_SPEED_10;
		cmd_bits <<= CMD_SPEED_SHIFT;

		/* duplex */
		if (ecmd.duplex != DUPLEX_FULL)
			cmd_bits |= CMD_HD_EN;

		/* pause capability */
		if (pDevCtrl->phyType == BRCM_PHY_TYPE_INT ||
		    pDevCtrl->phyType == BRCM_PHY_TYPE_EXT_MII ||
		    pDevCtrl->phyType == BRCM_PHY_TYPE_EXT_RVMII) {
			val = bcmgenet_mii_read(
				dev, pDevCtrl->phyAddr, MII_LPA);
			if (!(val & LPA_PAUSE_CAP)) {
				cmd_bits |= CMD_RX_PAUSE_IGNORE;
				cmd_bits |= CMD_TX_PAUSE_IGNORE;
			}
		} else if (pDevCtrl->extPhy) { /* RGMII only */
			val = bcmgenet_mii_read(dev,
				pDevCtrl->phyAddr, MII_BRCM_AUX_STAT_SUM);
			if (!(val & MII_BRCM_AUX_GPHY_RX_PAUSE))
				cmd_bits |= CMD_RX_PAUSE_IGNORE;
			if (!(val & MII_BRCM_AUX_GPHY_TX_PAUSE))
				cmd_bits |= CMD_TX_PAUSE_IGNORE;
		}

		umac->cmd &= ~((CMD_SPEED_MASK << CMD_SPEED_SHIFT) |
			       CMD_HD_EN |
			       CMD_RX_PAUSE_IGNORE | CMD_TX_PAUSE_IGNORE);
		umac->cmd |= cmd_bits;

		netif_carrier_on(pDevCtrl->dev);
		netdev_info(dev, "link up, %d Mbps, %s duplex\n", ecmd.speed,
			ecmd.duplex == DUPLEX_FULL ? "full" : "half");
	} else if (!cur_link && prev_link) {
#ifdef CONFIG_PACE_BCMGENET_CABLE_DETECT
		/* Switch off the interface only if the cable detection interrupt flags have been enabled */
		if (pDevCtrl->cable_detect_interrupts_enabled == 1)
		{
#endif /* #ifdef CONFIG_PACE_BCMGENET_CABLE_DETECT */
		netif_carrier_off(pDevCtrl->dev);
		netdev_info(dev, "link down\n");
#ifdef CONFIG_PACE_BCMGENET_CABLE_DETECT
		}
#endif /* #ifdef CONFIG_PACE_BCMGENET_CABLE_DETECT */
	}
}

void bcmgenet_ephy_workaround(struct net_device *dev)
{
	struct BcmEnet_devctrl *pDevCtrl = netdev_priv(dev);
	int phy_id = pDevCtrl->phyAddr;

	/* set shadow mode 2 */
	bcmgenet_mii_set_clr_bits(dev, 0x1f, 0x0004, 0x0004);

	/*
	 * Workaround for SWLINUX-2281: explicitly reset IDDQ_CLKBIAS
	 * in the Shadow 2 regset, due to power sequencing issues.
	 */
	/* set iddq_clkbias */
	bcmgenet_mii_write(dev, phy_id, 0x14, 0x0F00);
	udelay(10);
	/* reset iddq_clkbias */
	bcmgenet_mii_write(dev, phy_id, 0x14, 0x0C00);

	/*
	 * Workaround for SWLINUX-2056: fix timing issue between the ephy
	 * digital and the ephy analog blocks.  This clock inversion will
	 * inherently fix any setup and hold issue.
	 */
	bcmgenet_mii_write(dev, phy_id, 0x13, 0x7555);

	/* reset shadow mode 2 */
	bcmgenet_mii_set_clr_bits(dev, 0x1f, 0x0004, 0);
}

void bcmgenet_mii_reset(struct net_device *dev)
{
	struct BcmEnet_devctrl *pDevCtrl = netdev_priv(dev);

	bcmgenet_mii_write(dev, pDevCtrl->phyAddr, MII_BMCR, BMCR_RESET);
	udelay(1);
	/* enable 64 clock MDIO */
	bcmgenet_mii_write(dev, pDevCtrl->phyAddr, 0x1d, 0x1000);
	bcmgenet_mii_read(dev, pDevCtrl->phyAddr, 0x1d);
	bcmgenet_ephy_workaround(dev);
}

int bcmgenet_mii_init(struct net_device *dev)
{
	struct BcmEnet_devctrl *pDevCtrl = netdev_priv(dev);
	volatile struct uniMacRegs *umac;
	u32 id_mode_dis = 0;
	char *phy_name;

#ifdef CONFIG_PACE_BCMGENET_CABLE_DETECT
	int bmcr;
#endif /* #ifdef CONFIG_PACE_BCMGENET_CABLE_DETECT */

	umac = pDevCtrl->umac;
	pDevCtrl->mii.phy_id = pDevCtrl->phyAddr;
	pDevCtrl->mii.phy_id_mask = 0x1f;
	pDevCtrl->mii.reg_num_mask = 0x1f;
	pDevCtrl->mii.dev = dev;
	pDevCtrl->mii.mdio_read = bcmgenet_mii_read;
	pDevCtrl->mii.mdio_write = bcmgenet_mii_write;
	pDevCtrl->mii.supports_gmii = 0;

	switch (pDevCtrl->phyType) {

	case BRCM_PHY_TYPE_INT:
		phy_name = "internal PHY";
		pDevCtrl->sys->sys_port_ctrl = PORT_MODE_INT_EPHY;
#ifdef CONFIG_PACE_BCMGENET_CABLE_DETECT
		/* enable APD only if the cable detection interrupt flags have been enabled */
		if (pDevCtrl->cable_detect_interrupts_enabled == 1)
		{
			pDevCtrl->ext->ext_pwr_mgmt |= EXT_PWR_DN_EN_LD;
		}
		else
		{
			pDevCtrl->ext->ext_pwr_mgmt &= ~(EXT_PWR_DN_EN_LD);
		}
#else
		/* enable APD */
		pDevCtrl->ext->ext_pwr_mgmt |= EXT_PWR_DN_EN_LD;
#endif /* #ifdef CONFIG_PACE_BCMGENET_CABLE_DETECT */

		bcmgenet_mii_reset(dev);
#ifdef CONFIG_PACE_BCMGENET_CABLE_DETECT
		/* The PHY always comes back up with auto negotiation back on after a reset,
		   so we need to turn it off again if speed, duplex etc. were previously specified
		   manually (signified by mii.force_media being non-zero) */
		bmcr = pDevCtrl->mii.mdio_read(pDevCtrl->dev, pDevCtrl->phyAddr, MII_BMCR);

		if (((bmcr & BMCR_ANENABLE) != 0) && (pDevCtrl->mii.force_media != 0))
		{
			/* Turn off auto-negotiation again */
			bmcr &= ~(BMCR_ANENABLE);
			pDevCtrl->mii.mdio_write(pDevCtrl->dev, pDevCtrl->phyAddr, MII_BMCR, bmcr);
		}
#endif /* #ifdef CONFIG_PACE_BCMGENET_CABLE_DETECT */

		break;
	case BRCM_PHY_TYPE_EXT_MII:
		phy_name = "external MII";
		pDevCtrl->sys->sys_port_ctrl = PORT_MODE_EXT_EPHY;
		break;
	case BRCM_PHY_TYPE_EXT_RVMII:
		phy_name = "external RvMII";
		if (pDevCtrl->phySpeed == SPEED_100)
			pDevCtrl->sys->sys_port_ctrl = PORT_MODE_EXT_RVMII_25;
		else
			pDevCtrl->sys->sys_port_ctrl = PORT_MODE_EXT_RVMII_50;
		break;
	case BRCM_PHY_TYPE_EXT_RGMII_NO_ID:
		/*
		 * RGMII_NO_ID: TXC transitions at the same time as TXD
		 *              (requires PCB or receiver-side delay)
		 * RGMII:       Add 2ns delay on TXC (90 degree shift)
		 *
		 * ID is implicitly disabled for 100Mbps (RG)MII operation.
		 */
		id_mode_dis = BIT(16);
		/* fall through */
	case BRCM_PHY_TYPE_EXT_RGMII:
		phy_name = "external RGMII";
#if defined(CONFIG_BCM7425) && defined(CONFIG_PACESTB)
		/* The 7425 RGMII interface is connected to the Realtek Ethernet switch on D364.
		   Ensure 'id_mode_dis' is zero. This shifts the RGMII TX clock by 90 degrees,
		   effectively delaying it by ~2ns to match what the Realtek switch expects to receive */
		GENET_RGMII_OOB_CTRL(pDevCtrl) |= RGMII_MODE_EN;
		GENET_RGMII_OOB_CTRL(pDevCtrl) &= ~(id_mode_dis);
#else
		GENET_RGMII_OOB_CTRL(pDevCtrl) |= RGMII_MODE_EN | id_mode_dis;
#endif /* #if defined(CONFIG_BCM7425) ... */
		pDevCtrl->sys->sys_port_ctrl = PORT_MODE_EXT_GPHY;
		/*
		 * setup mii based on configure speed and RGMII txclk is set in
		 * umac->cmd, mii_setup() after link established.
		 */
		if (pDevCtrl->phySpeed == SPEED_1000) {
			pDevCtrl->mii.supports_gmii = 1;
		} else if (pDevCtrl->phySpeed == SPEED_100) {
			/* disable 1000BASE-T full, half-duplex capability */
			bcmgenet_mii_set_clr_bits(dev, MII_CTRL1000, 0,
				(ADVERTISE_1000FULL|ADVERTISE_1000HALF));
			/* restart autoneg */
			bcmgenet_mii_set_clr_bits(dev, MII_BMCR, BMCR_ANRESTART,
				BMCR_ANRESTART);
		}
		break;
	case BRCM_PHY_TYPE_MOCA:
		phy_name = "MoCA";
		/* setup speed in umac->cmd for RGMII txclk set to 125MHz */
		umac->cmd = umac->cmd | (UMAC_SPEED_1000 << CMD_SPEED_SHIFT);
		pDevCtrl->mii.force_media = 1;
		pDevCtrl->sys->sys_port_ctrl = PORT_MODE_INT_GPHY |
			LED_ACT_SOURCE_MAC;
		break;
	default:
		pr_err("unknown phy_type: %d\n", pDevCtrl->phyType);
		return -1;
	}

	pr_info("configuring instance #%d for %s\n", pDevCtrl->pdev->id,
		phy_name);

	return 0;
}

#if defined(CONFIG_TIVO_GEN10)
// Access extended 32-bit switch registers
// First set "page" from addr bits 18-9 written to phy 24 register 0
// addr bits 8-6 map to phy 16-23, and addr bits 5-2 map to phy register 0-30
// addr bits 1-0 are unused (always zero)
// low order register bit selects high or low half of 32-bit data
// Note these MII operations MUST be atomic, we take the mdio spinlock 
// Atheros switch will be configured when MoCA is connected, so need to export this function
void athsw_write_reg(struct net_device *dev, int addr, int data)
{
    struct BcmEnet_devctrl *pDevCtrl = netdev_priv(dev);
    int xphy = 0x10 | ((addr >> 6) & 7);
    int xreg = (addr >> 1) & 0x1E;

    mutex_lock(&pDevCtrl->mdio_mutex);
    if (mii_write_guts(pDevCtrl->umac, 0x18, 0, addr >> 9) || 
        mii_write_guts(pDevCtrl->umac, xphy, xreg, data & 0xFFFF) || 
        mii_write_guts(pDevCtrl->umac, xphy, xreg|1, data >> 16))
        printk("athsw_write_reg failed\n");
    mutex_unlock(&pDevCtrl->mdio_mutex);
}
EXPORT_SYMBOL(athsw_write_reg);

int athsw_read_reg(struct net_device *dev, int addr)
{	
    struct BcmEnet_devctrl *pDevCtrl = netdev_priv(dev);
    int xphy = 0x10 | ((addr >> 6) & 7),
    xreg = (addr >> 1) & 0x1E;
    int lo, hi;

    mutex_lock(&pDevCtrl->mdio_mutex);
    if (mii_write_guts(pDevCtrl->umac, 0x18, 0, addr >> 9) ||
       ((lo = mii_read_guts(pDevCtrl->umac, xphy, xreg)) < 0) ||
       ((hi = mii_read_guts(pDevCtrl->umac, xphy, xreg|1)) < 0))
    {     
        printk("athsw_read_reg failed\n");     
        lo=hi=0;
    }    
    mutex_unlock(&pDevCtrl->mdio_mutex);
    return lo | hi << 16; 
}
#endif

