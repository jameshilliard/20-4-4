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
 */

/**************************************************************************
 * File Name  : board_setup.c
 *
 * Description: This file contains the board configuration info
 *              
 * 
 * Updates    : 07/10/2007  Created.
 ***************************************************************************/

#include "board_setup.h"
#include "bcmemac.h"

static int port_phy_t[MAX_PHY_PORT] = {2, 0, 1, 0 };

/* Initialise the Ethernet Switch control registers */

int init_ethsw_pinmux(BcmEnet_devctrl * pDevCtrl);

/* Gets board dependent phy port configuration */

int get_phy_port_config(int phy_port[], int num_of_ports);


/* 
 * init_ethsw_pinmux: Initializes the Ethernet Switch control registers
 */

int init_ethsw_pinmux(BcmEnet_devctrl * pDevCtrl)
{
    volatile uint32 *pin_mux_ctrl;
    uint32 data;

    TRACE(("bcmemacenet: init_ethsw_pinmux\n"));

#ifdef ETH_SWITCH_HR21_T
    pin_mux_ctrl = (volatile uint32 *)BCM_PHYS_TO_K1(BCHP_PHYSICAL_OFFSET+BCHP_SUN_TOP_CTRL_PIN_MUX_CTRL_1);
   *pin_mux_ctrl &= ~(uint32)(LED_LD_4_MSK |
                           LED_LS_1_MSK |
                           LED_LS_2_MSK |
                           LED_LS_3_MSK |
                           LED_LS_4_MSK |
                           VI0_656_1_MSK );
   *pin_mux_ctrl |= (uint32)(ALT_MII_TX_EN |
                           ALT_MII_TXD_03 |
                           ALT_MII_TXD_02 |
                           ALT_MII_TXD_01 |
                           ALT_MII_TXD_00 |
                           ALT_MII_RXD_03 );
    pin_mux_ctrl = (volatile uint32 *)BCM_PHYS_TO_K1(BCHP_PHYSICAL_OFFSET+BCHP_SUN_TOP_CTRL_PIN_MUX_CTRL_2);
   *pin_mux_ctrl &= ~(uint32)(VI0_656_2_MSK |
                           VI0_656_4_MSK |
                           VI0_656_7_MSK |
                           VI0_656_CLK_MSK );
   *pin_mux_ctrl |= (uint32)(ALT_MII_RXD_02 |
                           ALT_MII_MDIO |
                           ALT_MII_RXD_01 |
                           ALT_MII_RX_CLK );
    pin_mux_ctrl = (volatile uint32 *)BCM_PHYS_TO_K1(BCHP_PHYSICAL_OFFSET+BCHP_SUN_TOP_CTRL_PIN_MUX_CTRL_3);
   *pin_mux_ctrl &= ~(uint32)I2S_DATA_MSK;
   *pin_mux_ctrl |= (uint32)ALT_MII_RX_ERR;
    pin_mux_ctrl = (volatile uint32 *)BCM_PHYS_TO_K1(BCHP_PHYSICAL_OFFSET+BCHP_SUN_TOP_CTRL_PIN_MUX_CTRL_5);
   *pin_mux_ctrl &= ~(uint32)IR_IN1_MSK;
   *pin_mux_ctrl |= (uint32)ALT_MII_RXD_00;
    pin_mux_ctrl = (volatile uint32 *)BCM_PHYS_TO_K1(BCHP_PHYSICAL_OFFSET+BCHP_SUN_TOP_CTRL_PIN_MUX_CTRL_7);
   *pin_mux_ctrl &= ~(uint32)(GPIO_15_MSK |
                              GPIO_14_MSK |
                              GPIO_12_MSK );
   *pin_mux_ctrl |= (uint32)(ALT_MII_RX_EN |
                             ALT_MII_CRS |
                             ALT_MII_TX_CLK);
    pin_mux_ctrl = (volatile uint32 *)BCM_PHYS_TO_K1(BCHP_PHYSICAL_OFFSET+BCHP_SUN_TOP_CTRL_PIN_MUX_CTRL_8);
   *pin_mux_ctrl &= ~(uint32)GPIO_20_MSK;
   *pin_mux_ctrl |= (uint32)MII_MDC;

    pin_mux_ctrl = (volatile uint32 *)BCM_PHYS_TO_K1(BCHP_PHYSICAL_OFFSET+BCHP_SUN_TOP_CTRL_PIN_MUX_CTRL_11);
   *pin_mux_ctrl &= ~(uint32)GPIO_49_MSK;
   *pin_mux_ctrl |= (uint32)ETHERNET_PWR;
        
   // Turn ETHERNET_PWR* off (active low)
   data = *(volatile uint32 *)BCM_PHYS_TO_K1(BCHP_PHYSICAL_OFFSET+BCHP_GIO_IODIR_HI);
   data &= (uint32)(~GPIO49_OFFSET); // Force gpio49 to output direction (low)
   *(volatile uint32 *)BCM_PHYS_TO_K1(BCHP_PHYSICAL_OFFSET+BCHP_GIO_IODIR_HI) = data;

   data = *(volatile uint32 *)BCM_PHYS_TO_K1(BCHP_PHYSICAL_OFFSET+BCHP_GIO_DATA_HI);
   data |= (uint32)GPIO49_OFFSET; // set bit to 1
   *(volatile uint32 *)BCM_PHYS_TO_K1(BCHP_PHYSICAL_OFFSET+BCHP_GIO_DATA_HI) = data;

#ifdef	ETH_SWITCH_HR21_S	// for Samsung board only
        pin_mux_ctrl = (volatile uint32 *)BCM_PHYS_TO_K1(BCHP_PHYSICAL_OFFSET+BCHP_SUN_TOP_CTRL_PIN_MUX_CTRL_10);
        data = *pin_mux_ctrl;
        data &= GPIO_45; // mux setting for gpio45. clear bit 29, 28, 27
        *pin_mux_ctrl = data;

        // reset the Ethernet Switch
        data = *(volatile uint32 *)BCM_PHYS_TO_K1(BCHP_PHYSICAL_OFFSET+BCHP_GIO_IODIR_HI);
        data &= (uint32)(~GPIO45_OFFSET); // Force gpio45 to output direction (forcing bit 13 to 0)
        *(volatile uint32 *)BCM_PHYS_TO_K1(BCHP_PHYSICAL_OFFSET+BCHP_GIO_IODIR_HI) = data;

        data = *(volatile uint32 *)BCM_PHYS_TO_K1(BCHP_PHYSICAL_OFFSET+BCHP_GIO_DATA_HI);
        // pulse once on gpio45 to reset 5325
        data &= (uint32)(~GPIO45_OFFSET); // set bit 13 to 0
        *(volatile uint32 *)BCM_PHYS_TO_K1(BCHP_PHYSICAL_OFFSET+BCHP_GIO_DATA_HI) = data;
        udelay(1000);
        data |= (uint32)GPIO45_OFFSET; // set bit 13 to 1
        *(volatile uint32 *)BCM_PHYS_TO_K1(BCHP_PHYSICAL_OFFSET+BCHP_GIO_DATA_HI) = data;
#endif

#else
    pin_mux_ctrl = (volatile uint32 *)BCM_PHYS_TO_K1(BCHP_PHYSICAL_OFFSET + BCHP_SUN_TOP_CTRL_PIN_MUX_CTRL_1);
    *pin_mux_ctrl |= (uint32) (ALT_MII_TX_EN |
                               ALT_MII_TXD_03 |
                               ALT_MII_TXD_02 |
                               ALT_MII_TXD_01 |
                               ALT_MII_TXD_00 |
                               ALT_MII_RXD_03);
    pin_mux_ctrl = (volatile uint32 *)BCM_PHYS_TO_K1(BCHP_PHYSICAL_OFFSET + BCHP_SUN_TOP_CTRL_PIN_MUX_CTRL_2);
    *pin_mux_ctrl |= (uint32) (ALT_MII_RXD_02 |
                               ALT_MII_MDIO |
                               ALT_MII_RXD_01 |
                               ALT_MII_RX_CLK);
    pin_mux_ctrl = (volatile uint32 *)BCM_PHYS_TO_K1(BCHP_PHYSICAL_OFFSET + BCHP_SUN_TOP_CTRL_PIN_MUX_CTRL_3);
    *pin_mux_ctrl |= (uint32) ALT_MII_RX_ERR;
    pin_mux_ctrl = (volatile uint32 *)BCM_PHYS_TO_K1(BCHP_PHYSICAL_OFFSET + BCHP_SUN_TOP_CTRL_PIN_MUX_CTRL_5);
    *pin_mux_ctrl |= (uint32) ALT_MII_RXD_00;
    pin_mux_ctrl = (volatile uint32 *)BCM_PHYS_TO_K1(BCHP_PHYSICAL_OFFSET + BCHP_SUN_TOP_CTRL_PIN_MUX_CTRL_7);
    *pin_mux_ctrl |= (uint32) (ALT_MII_RX_EN | ALT_MII_CRS | ALT_MII_COL | ALT_MII_TX_CLK);
    pin_mux_ctrl = (volatile uint32 *)BCM_PHYS_TO_K1(BCHP_PHYSICAL_OFFSET + BCHP_SUN_TOP_CTRL_PIN_MUX_CTRL_8);
    *pin_mux_ctrl |= (uint32) GPIO_20;
#endif

    return 0;

}


/* 
 *  get_phy_port_config: Gets board dependent phy port configuration
 */

int get_phy_port_config(int phy_port[], int num_of_ports)
{
    int i;

    for (i = 0; i < num_of_ports; i++)
    {
        phy_port[i] = port_phy_t[i];
    }

    return 0;
}
