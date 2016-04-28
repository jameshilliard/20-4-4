/*
 * Copyright (c) 2010, TiVo, Inc.  All Rights Reserved
 *
 * Portions of this file are copyrighted by Entropic Communications:
 * 
 *                        Entropic Communications, Inc.
 *                         Copyright (c) 2001-2008
 *                          All rights reserved.
 *
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

#include "e1000_gpl_hdr.h"
// The previous header rudely #defines bool
// It defines it to a char. The following header typedef's bool as unsigned long
// From what I can tell in a cursory glance at this code, it doesn't need any functions that
// would use the bool type in a signature anyway, so simply undefing to avoid compiler complaint
#undef bool
#include "bcmemac.h"

/* brcmint7038 driver functions */
extern int bcmemac_mii_read(struct net_device *dev, int phy_id, int location);
extern void bcmemac_mii_write(struct net_device *dev, int phy_id, int location, int val);

extern int emoca_get_mii_phy_id(void);

/**
 * emoca_mdio_read: Read MII PHY register via MMI interface
 * @dev: MoCA interface 
 * @phy_id: EN2510 MII PHY address
 * @location: MII PHY register number
 */ 
int emoca_mdio_read(struct net_device *dev, int phy_id, int location)
{
    return(bcmemac_mii_read(dev, phy_id, location));
}

/**
 * emoca_mdio_write: Write MII PHY register via MMI interface
 * @dev: MoCA interface 
 * @phy_id: EN2510 MII PHY address
 * @location: MII PHY register number
 * @val: register value
 */
void emoca_mdio_write(struct net_device *dev, int phy_id, int location, int val)
{
    bcmemac_mii_write(dev, phy_id, location, val);
}

/*
 * All functions below this point are based on source code provide by Entropic
 * Communications. See os/drivers_k26/entropic/host/Clink/Drv/GPL/E1000/mii_mdio.c
 */
 
/*
*STATIC*******************************************************************************/
static int clnk_busy_check(struct net_device *dev)
{
    int addr_mode;
    int stat ;
    // BcmEnet_devctrl *dev_ctrl = netdev_priv(dev);
    int phy_id = emoca_get_mii_phy_id();
    
    //HostOS_PrintLog(L_INFO, "%s: ehw=%08x\n", __FUNCTION__ , (unsigned int)ehw );
    
    stat = 0 ;
    do
    {
        addr_mode = emoca_mdio_read(dev, phy_id, MMI_REG_ENH_ADDR_MODE);
        
        if(addr_mode < 0)
        {
            HostOS_PrintLog(L_ERR, "MMI_REG_ENH_ADDR_MODE lookup failed\n");
            stat = -1 ;
            break ;
        }
        if(addr_mode & GPHY_ENH_ADDR_MODE__ERROR)
        {
            HostOS_PrintLog(L_ERR, "MMI reported ADDR_MODE_ERROR\n");
            stat = -1 ;
            break ;
        }
    } while( addr_mode & GPHY_ENH_ADDR_MODE__BUSY );

    //HostOS_PrintLog(L_INFO, "%s: stat=%08x\n", __FUNCTION__ , (unsigned int)stat );
    return( stat ) ;
}

/*
*   Purpose:    Writes a clink bus register/location
*               The register/location is accessed via MDIO
*
*   Imports:    vctx - pointer to the driver control context
*               addr - address of the register/location
*                      this will be translated
*               data - the data to write
*
*   Exports:    -1 on busy, 0 otherwise
*
*PUBLIC*******************************************************************************/
int clnk_write( void *vctx, SYS_UINT32 addr, SYS_UINT32 data)
{
    struct net_device *dev = ((dk_context_t *)dc_to_dk(vctx))->dev;
    // BcmEnet_devctrl *dev_ctrl = netdev_priv(dev);
    int phy_id = emoca_get_mii_phy_id();

    if( clnk_busy_check(dev) < 0 ) {
        return(-1);
    }

    emoca_mdio_write(dev, phy_id, MMI_REG_ENH_ADDR_PORT_HI, addr >> 16) ;
    emoca_mdio_write(dev, phy_id, MMI_REG_ENH_ADDR_PORT_LO, addr & 0xffff) ;
    emoca_mdio_write(dev, phy_id, MMI_REG_ENH_DATA_PORT_HI, data >> 16) ;
    emoca_mdio_write(dev, phy_id, MMI_REG_ENH_DATA_PORT_LO, data & 0xffff) ;
    emoca_mdio_write(dev, phy_id, MMI_REG_ENH_ADDR_MODE, GPHY_ENH_ADDR_MODE__START_WRITE) ;

    return(0);
}

/*
*P UBLIC*******************************************************************************/
int clnk_write_burst( void *vctx, SYS_UINT32 addr, SYS_UINT32 *data, unsigned int size, int inc )
{
    struct net_device *dev = ((dk_context_t *)dc_to_dk(vctx))->dev;
    // BcmEnet_devctrl *dev_ctrl = netdev_priv(dev);
    int phy_id = emoca_get_mii_phy_id();
    unsigned int i;

    if(clnk_busy_check(dev) < 0) {
        return(-1);
    }

    emoca_mdio_write(dev, phy_id, MMI_REG_ENH_ADDR_PORT_HI, addr >> 16) ;
    emoca_mdio_write(dev, phy_id, MMI_REG_ENH_ADDR_PORT_LO, addr & 0xffff) ;
    emoca_mdio_write(dev, phy_id, MMI_REG_ENH_ADDR_PORT_LO, addr & 0xffff) ;
    
    for(i = 0; i < size; i += sizeof(*data))
    {
        int auto_inc;

        /*
         * FPGA AUTO_INCR is pre-increment
         * SoC AUTO_INCR is post-increment
         */
        if((i == 0) && (!inc))
            auto_inc = 0;
        else
            auto_inc = GPHY_ENH_ADDR_MODE__AUTO_INCR;

        emoca_mdio_write(dev, phy_id, MMI_REG_ENH_DATA_PORT_HI, (*data) >> 16) ;
        emoca_mdio_write(dev, phy_id, MMI_REG_ENH_DATA_PORT_LO, (*data) & 0xffff) ;
        emoca_mdio_write(dev, phy_id, MMI_REG_ENH_ADDR_MODE,
                   GPHY_ENH_ADDR_MODE__START_WRITE |
                   auto_inc) ;
        
        data++;
    }

    emoca_mdio_write(dev, phy_id, MMI_REG_ENH_ADDR_MODE, 0);

    return(0);
}

/*
*   Purpose:    Reads a clink bus register/location
*               The register/location is accessed via MDIO
*
*   Imports:    vctx - pointer to the driver control context
*               addr - address of the register/location
*                      this will be translated
*               val  - pointer to place to put the data
*
*   Exports:    -1 on busy, 0 otherwise
*
*PUBLIC*******************************************************************************/
int clnk_read( void *vctx, SYS_UINT32 addr, SYS_UINT32 *data)
{
    struct net_device *dev = ((dk_context_t *)dc_to_dk(vctx))->dev;
    // BcmEnet_devctrl *dev_ctrl = netdev_priv(dev);
    int phy_id = emoca_get_mii_phy_id();
    SYS_INT32       out;
    SYS_UINT32      ret;

    //HostOS_PrintLog(L_INFO, "In clnk_read function\n");
    //HostOS_PrintLog(L_INFO, "%s: vctx=%08x\n", __FUNCTION__ , vctx );
    //HostOS_PrintLog(L_INFO, "%s: data=%08x\n", __FUNCTION__ , data );
    //HostOS_PrintLog(L_INFO, "%s: ehw=%08x\n", __FUNCTION__ , ehw );
    if(clnk_busy_check(dev) < 0) {
        //HostOS_PrintLog(L_ERR, "%s: return -1\n", __FUNCTION__ );
        return(-1);
    }

    emoca_mdio_write(dev, phy_id, MMI_REG_ENH_ADDR_PORT_HI, addr >> 16) ;
    emoca_mdio_write(dev, phy_id, MMI_REG_ENH_ADDR_PORT_LO, addr & 0xffff) ;
    emoca_mdio_write(dev, phy_id, MMI_REG_ENH_ADDR_MODE, GPHY_ENH_ADDR_MODE__START_READ) ;

    if(clnk_busy_check(dev) < 0) {
        return(-1);
    }

    out = emoca_mdio_read(dev, phy_id, MMI_REG_ENH_DATA_PORT_HI);
    ret = out << 16;

    out = emoca_mdio_read(dev, phy_id, MMI_REG_ENH_DATA_PORT_LO);
    *data = ret | (out & 0xffff);

//HostOS_PrintLog(L_INFO, "%s: return 0\n", __FUNCTION__ );
//HostOS_PrintLog(L_INFO, "OUT clnk_read function\n");
    return(0);
}

/*
*P UBLIC*******************************************************************************/
int clnk_read_burst( void *vctx, SYS_UINT32 addr, SYS_UINT32 *data, unsigned int size, int inc)
{
    struct net_device *dev = ((dk_context_t *)dc_to_dk(vctx))->dev;
    // BcmEnet_devctrl *dev_ctrl = netdev_priv(dev);
    int phy_id = emoca_get_mii_phy_id();
    SYS_INT32       out;
    SYS_UINT32      ret;
    unsigned int    i;

    if(clnk_busy_check(dev) < 0) {
        return(-1);
    }

    emoca_mdio_write(dev, phy_id, MMI_REG_ENH_ADDR_PORT_HI, addr >> 16)    ;
    emoca_mdio_write(dev, phy_id, MMI_REG_ENH_ADDR_PORT_LO, addr & 0xffff) ;
    emoca_mdio_write(dev, phy_id, MMI_REG_ENH_ADDR_PORT_LO, addr & 0xffff) ;

    for(i = 0; i < size; i += sizeof(*data))
    {
        int auto_inc;

        if((i == 0) && (!inc))
            auto_inc = 0;
        else
            auto_inc = GPHY_ENH_ADDR_MODE__AUTO_INCR;

        emoca_mdio_write(dev, phy_id, MMI_REG_ENH_ADDR_MODE,
                   GPHY_ENH_ADDR_MODE__START_READ | auto_inc) ;

        if(clnk_busy_check(dev) < 0) {
            return(-1);
        }

        out = emoca_mdio_read(dev, phy_id, MMI_REG_ENH_DATA_PORT_HI);
        ret = out << 16;

        out = emoca_mdio_read(dev, phy_id, MMI_REG_ENH_DATA_PORT_LO);
        *data = ret | (out & 0xffff);
        
        data++;
    }

    emoca_mdio_write(dev, phy_id, MMI_REG_ENH_ADDR_MODE, 0) ;
    
    return(0);
}
