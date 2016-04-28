/*******************************************************************************
*
* E1000/Inc/mii_common.h
*
* Description: E1000 Bus interface layer
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

#ifndef __mii_common_h__
#define __mii_common_h__


/*
 * EN2210 MII PHY registers
 */

#define MMI_CSR_BASE              (0x0)
#define MMI_REG_CTL               (MMI_CSR_BASE + 0x0)
#define MMI_REG_STATUS            (MMI_CSR_BASE + 0x1)
#define MMI_REG_EXT_STATUS        (MMI_CSR_BASE + 0x0F)
#define MMI_REG_ENH_CTL           (MMI_CSR_BASE + 0x1A)
#define MMI_REG_ENH_ADDR_MODE     (MMI_CSR_BASE + 0x1B)
#define MMI_REG_ENH_ADDR_PORT_HI  (MMI_CSR_BASE + 0x1C)
#define MMI_REG_ENH_ADDR_PORT_LO  (MMI_CSR_BASE + 0x1D)
#define MMI_REG_ENH_DATA_PORT_HI  (MMI_CSR_BASE + 0x1E)
#define MMI_REG_ENH_DATA_PORT_LO  (MMI_CSR_BASE + 0x1F)

#define   GPHY_ENH_CTL__GPHY_MODE_EN       (0x80)    // bit 07
#define   GPHY_ENH_CTL__GMAC_ISOLATE       (0x40)    // bit 06
#define   GPHY_ENH_CTL__LINK_UP            (0x20)    // bit 05
#define   GPHY_ENH_CTL__GPHY_ADDR_MASK     (0x1F)    // bit (4..0)
#define ETH_GPHY_REG_ENH_ADDR_MODE        (ETH_GPHY_CSR_BASE+0x6C)
#define   GPHY_ENH_ADDR_MODE__START_WRITE (0x01)
#define   GPHY_ENH_ADDR_MODE__START_READ  (0x02)
#define   GPHY_ENH_ADDR_MODE__AUTO_INCR   (0x04)
#define   GPHY_ENH_ADDR_MODE__BUSY        (0x08)
#define   GPHY_ENH_ADDR_MODE__ERROR       (0x10)


#endif /* ! __mii_common_h__ */
