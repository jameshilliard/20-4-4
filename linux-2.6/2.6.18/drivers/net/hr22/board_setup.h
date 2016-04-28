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


#define	ETH_SWITCH_HR21_T	// jipeng - testing only

#ifdef ETH_SWITCH_HR21_T
#define BCHP_PHYSICAL_OFFSET                    0x10000000
#define BCHP_SUN_TOP_CTRL_PIN_MUX_CTRL_1        0x00404098 /* Pin mux control register 1 */
#define BCHP_SUN_TOP_CTRL_PIN_MUX_CTRL_2        0x0040409c /* Pin mux control register 2 */
#define BCHP_SUN_TOP_CTRL_PIN_MUX_CTRL_3        0x004040a0 /* Pin mux control register 3 */
#define BCHP_SUN_TOP_CTRL_PIN_MUX_CTRL_5        0x004040a8 /* Pin mux control register 5 */
#define BCHP_SUN_TOP_CTRL_PIN_MUX_CTRL_7        0x004040b0 /* Pin mux control register 7 */
#define BCHP_SUN_TOP_CTRL_PIN_MUX_CTRL_8        0x004040b4 /* Pin mux control register 8 */
#define BCHP_SUN_TOP_CTRL_PIN_MUX_CTRL_10       0x004040bc /* Pin mux control register 10 */
#define BCHP_SUN_TOP_CTRL_PIN_MUX_CTRL_11       0x004040c0 /* Pin mux control register 11 */
#define BCHP_GIO_DATA_HI                        0x00400724 /* GENERAL PURPOSE I/O DATA [63:32] */
#define BCHP_GIO_IODIR_HI                       0x00400728 /* GENERAL PURPOSE I/O DIRECTION [63:32] */

/* Pin mux control register 1 */
#define ALT_MII_TX_EN	(0x2<<6)
#define ALT_MII_TX_ERR	(0x2<<9)
#define ALT_MII_MDC		(0x2<<12)
#define ALT_MII_TXD_03	(0x2<<18)
#define ALT_MII_TXD_02	(0x2<<20)
#define ALT_MII_TXD_01	(0x2<<22)
#define ALT_MII_TXD_00	(0x2<<24)
#define ALT_MII_RXD_03	(0x2<<29)

#define LED_LD_4_MSK    (0x7<<6)
#define LED_LD_5_MSK    (0x7<<9)
#define LED_LD_6_MSK    (0x3<<12)
#define LED_LS_1_MSK    (0x3<<18)
#define LED_LS_2_MSK    (0x3<<20)
#define LED_LS_3_MSK    (0x3<<22)
#define LED_LS_4_MSK    (0x3<<24)
#define VI0_656_1_MSK   (0x7<<29)

/* Pin mux control register 2 */
#define ALT_MII_RXD_02	(0x2<<0)
#define ALT_MII_MDIO	(0x2<<6)
#define ALT_MII_RXD_01	(0x2<<15)
#define ALT_MII_RX_CLK	(0x2<<18)

#define VI0_656_2_MSK   (0x7<<0)
#define VI0_656_4_MSK   (0x7<<6)
#define VI0_656_7_MSK   (0x7<<15)
#define VI0_656_CLK_MSK (0x3<<18)

/* Pin mux control register 3 */
#define ALT_MII_RX_ERR	(0x3<<8)

#define I2S_DATA_MSK    (0x7<<8)

/* Pin mux control register 5 */
#define ALT_MII_RXD_00	(0x4<<28)

#define IR_IN1_MSK      (0x7<<28)

/* Pin mux control register 7*/
#define ALT_MII_RX_EN	(0x3<<27)
#define ALT_MII_CRS		(0x4<<24)
#define ALT_MII_COL		(0x3<<21)
#define ALT_MII_TX_CLK	(0x3<<18)

#define GPIO_15_MSK     (0x7<<27)
#define GPIO_14_MSK     (0x7<<24)
#define GPIO_13_MSK     (0x7<<21)
#define GPIO_12_MSK     (0x7<<18)

/* Pin mux control register 8 */
#define MII_MDC         (0x2<<12)

#define GPIO_20_MSK     (0x7<<12)

/* Pin mux control register 10 */
#define GPIO_45         (0xC7FFFFFF)

#define GPIO45_OFFSET   (0x00002000)

/* Pin mux control register 11 */
#define ETHERNET_PWR    (0x0<<5)

#define GPIO_49_MSK     (0x7<<5)

#define GPIO49_OFFSET   (0x00020000)

#else

#define BCHP_PHYSICAL_OFFSET                    0x10000000
#define BCHP_SUN_TOP_CTRL_PIN_MUX_CTRL_1        0x00404098      /* Pin mux control register 1 */
#define BCHP_SUN_TOP_CTRL_PIN_MUX_CTRL_2        0x0040409c      /* Pin mux control register 2 */
#define BCHP_SUN_TOP_CTRL_PIN_MUX_CTRL_3        0x004040a0      /* Pin mux control register 3 */
#define BCHP_SUN_TOP_CTRL_PIN_MUX_CTRL_5        0x004040a8      /* Pin mux control register 5 */
#define BCHP_SUN_TOP_CTRL_PIN_MUX_CTRL_7        0x004040b0      /* Pin mux control register 7 */
#define BCHP_SUN_TOP_CTRL_PIN_MUX_CTRL_8        0x004040b4      /* Pin mux control register 8 */
#define BCHP_SUN_TOP_CTRL_PIN_MUX_CTRL_10       0x004040bc      /* Pin mux control register 10 */

/* Pin mux control register 1 */
#define ALT_MII_TX_EN	(0x2<<6)
#define ALT_MII_TX_ERR	(0x2<<9)
#define ALT_MII_MDC		(0x2<<12)
#define ALT_MII_TXD_03	(0x2<<18)
#define ALT_MII_TXD_02	(0x2<<20)
#define ALT_MII_TXD_01	(0x2<<22)
#define ALT_MII_TXD_00	(0x2<<24)
#define ALT_MII_RXD_03	(0x2<<29)

/* Pin mux control register 2 */
#define ALT_MII_RXD_02	(0x2<<0)
#define ALT_MII_MDIO	(0x2<<6)
#define ALT_MII_RXD_01	(0x2<<15)
#define ALT_MII_RX_CLK	(0x2<<18)

/* Pin mux control register 3 */
#define ALT_MII_RX_ERR	(0x3<<8)


/* Pin mux control register 5 */
#define ALT_MII_RXD_00	(0x4<<28)

/* Pin mux control register 7 */
#define ALT_MII_RX_EN	(0x3<<27)
#define ALT_MII_CRS		(0x4<<24)
#define ALT_MII_COL		(0x3<<21)
#define ALT_MII_TX_CLK	(0x3<<18)
/* Pin mux control register 8 */
#define GPIO_20         (0x2<<12)

/* Pin mux control register 10 */
#define GPIO_45         (0xC7FFFFFF)

#define GPIO45_OFFSET   (0x00002000)

#endif
