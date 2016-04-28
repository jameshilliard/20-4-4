/*
 * Copyright (C) 2009 Broadcom Corporation
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
 */

#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/smp.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/bitops.h>
#include <linux/bmoca.h>
#include <linux/mtd/partitions.h>
#include <linux/brcmstb/brcmstb.h>

/* board features */
#ifdef CONFIG_PACESTB
/* We want this set, BCM passes this in from CFE */
int brcm_docsis_platform = 1;
#else
int brcm_docsis_platform;
#endif
char brcm_cfe_boardname[CFE_STRING_SIZE];

/* MTD partition layout */
unsigned long brcm_mtd_rootfs_start;
unsigned long brcm_mtd_rootfs_len;
unsigned long brcm_mtd_kernel_start;
unsigned long brcm_mtd_kernel_len;
unsigned long brcm_mtd_ocap_start;
unsigned long brcm_mtd_ocap_len;
unsigned long brcm_mtd_flash_size_mb;
char brcm_mtd_flash_type[CFE_STRING_SIZE];

/* MoCA 3450 I2C port */
unsigned long brcm_moca_i2c_base;

#ifndef CONFIG_TIVO_GEN08
/* Default MoCA RF band (can be overridden in board_pinmux_setup) */
#ifdef CONFIG_BRCM_HAS_MOCA_MIDRF 
unsigned long brcm_moca_rf_band = MOCA_BAND_MIDRF;
#else
unsigned long brcm_moca_rf_band = MOCA_BAND_HIGHRF;
#endif
#else
// force HIGHRF on Leo
unsigned long brcm_moca_rf_band = MOCA_BAND_HIGHRF;
#endif

/***********************************************************************
 * PIN_MUX setup
 *
 * NOTE: This code assumes that the bootloader already set up the pinmux
 * for the primary UART (almost always UARTA) and EBI.
 *
 * This code is free to modify genet_pdata[].  For example, to set up an
 * external RGMII PHY on GENET_0, set up your pinmux / pad voltages /
 * pulldowns then add:
 *
 *   genet_pdata[0].phy_type = BRCM_PHY_TYPE_EXT_RGMII;
 *   genet_pdata[0].phy_id = BRCM_PHY_ID_AUTO;
 ***********************************************************************/

#define PINMUX(reg, field, val) do { \
	BDEV_WR(BCHP_SUN_TOP_CTRL_PIN_MUX_CTRL_##reg, \
		(BDEV_RD(BCHP_SUN_TOP_CTRL_PIN_MUX_CTRL_##reg) & \
		 ~BCHP_SUN_TOP_CTRL_PIN_MUX_CTRL_##reg##_##field##_MASK) | \
		((val) << \
		 BCHP_SUN_TOP_CTRL_PIN_MUX_CTRL_##reg##_##field##_SHIFT)); \
	} while (0)

#define AON_PINMUX(reg, field, val) do { \
	BDEV_WR(BCHP_AON_PIN_CTRL_PIN_MUX_CTRL_##reg, \
		(BDEV_RD(BCHP_AON_PIN_CTRL_PIN_MUX_CTRL_##reg) & \
		 ~BCHP_AON_PIN_CTRL_PIN_MUX_CTRL_##reg##_##field##_MASK) | \
		((val) << \
		 BCHP_AON_PIN_CTRL_PIN_MUX_CTRL_##reg##_##field##_SHIFT)); \
	} while (0)

#define PADCTRL(reg, field, val) do { \
	BDEV_WR(BCHP_SUN_TOP_CTRL_PIN_MUX_PAD_CTRL_##reg, \
		(BDEV_RD(BCHP_SUN_TOP_CTRL_PIN_MUX_PAD_CTRL_##reg) & \
		 ~BCHP_SUN_TOP_CTRL_PIN_MUX_PAD_CTRL_##reg##_##field##_MASK) | \
		((val) << \
		 BCHP_SUN_TOP_CTRL_PIN_MUX_PAD_CTRL_##reg##_##field##_SHIFT)); \
	} while (0)

#define AON_PADCTRL(reg, field, val) do { \
	BDEV_WR(BCHP_AON_PIN_CTRL_PIN_MUX_PAD_CTRL_##reg, \
		(BDEV_RD(BCHP_AON_PIN_CTRL_PIN_MUX_PAD_CTRL_##reg) & \
		 ~BCHP_AON_PIN_CTRL_PIN_MUX_PAD_CTRL_##reg##_##field##_MASK) | \
		((val) << \
		 BCHP_AON_PIN_CTRL_PIN_MUX_PAD_CTRL_##reg##_##field##_SHIFT)); \
	} while (0)


#define SDIO_CFG_REG(x, y)	(BCHP_SDIO_##x##_CFG_REG_START + \
				 (BCHP_SDIO_0_CFG_##y - \
				  BCHP_SDIO_0_CFG_REG_START))

#if !defined(CONFIG_TIVO_GEN10)
void board_pinmux_setup(void)
{
#if !defined(CONFIG_BRCM_IKOS)
#if defined(CONFIG_BCM7231)

	PINMUX(11, gpio_94, 1);		/* UARTB TX */
	PINMUX(11, gpio_95, 1);		/* UARTB RX */

	if (BRCM_PROD_ID() == 0x7230) {
		/* 7230 is not the same ballout as 7231 */
		AON_PINMUX(0, aon_gpio_04, 6);	/* SDIO */
		AON_PINMUX(0, aon_gpio_05, 6);
		AON_PINMUX(1, aon_gpio_12, 5);
		AON_PINMUX(1, aon_gpio_13, 5);
		AON_PINMUX(2, aon_gpio_14, 5);
		AON_PINMUX(2, aon_gpio_15, 6);
		AON_PINMUX(2, aon_gpio_16, 6);
		AON_PINMUX(2, aon_gpio_17, 6);
		AON_PINMUX(2, aon_gpio_18, 6);
		AON_PINMUX(2, aon_gpio_19, 6);

		/* disable GPIO pulldowns */
		BDEV_WR_F_RB(AON_PIN_CTRL_PIN_MUX_PAD_CTRL_0,
			aon_gpio_04_pad_ctrl, 0);
		BDEV_WR_F_RB(AON_PIN_CTRL_PIN_MUX_PAD_CTRL_0,
			aon_gpio_05_pad_ctrl, 0);
		BDEV_WR_F_RB(AON_PIN_CTRL_PIN_MUX_PAD_CTRL_1,
			aon_gpio_12_pad_ctrl, 0);
		BDEV_WR_F_RB(AON_PIN_CTRL_PIN_MUX_PAD_CTRL_1,
			aon_gpio_13_pad_ctrl, 0);
		BDEV_WR_F_RB(AON_PIN_CTRL_PIN_MUX_PAD_CTRL_1,
			aon_gpio_14_pad_ctrl, 0);
		BDEV_WR_F_RB(AON_PIN_CTRL_PIN_MUX_PAD_CTRL_1,
			aon_gpio_15_pad_ctrl, 0);
		BDEV_WR_F_RB(AON_PIN_CTRL_PIN_MUX_PAD_CTRL_1,
			aon_gpio_16_pad_ctrl, 0);
		BDEV_WR_F_RB(AON_PIN_CTRL_PIN_MUX_PAD_CTRL_1,
			aon_gpio_17_pad_ctrl, 0);
		BDEV_WR_F_RB(AON_PIN_CTRL_PIN_MUX_PAD_CTRL_1,
			aon_gpio_18_pad_ctrl, 0);
		BDEV_WR_F_RB(AON_PIN_CTRL_PIN_MUX_PAD_CTRL_1,
			aon_gpio_19_pad_ctrl, 0);
	} else {
		/* set RGMII lines to 2.5V */
		BDEV_WR_F(SUN_TOP_CTRL_GENERAL_CTRL_NO_SCAN_0,
			rgmii_0_pad_mode, 1);

		PINMUX(15, gpio_132, 1);	/* RGMII RX */
		PINMUX(15, gpio_133, 1);
		PINMUX(15, gpio_134, 1);
		PINMUX(16, gpio_139, 1);
		PINMUX(16, gpio_140, 1);
		PINMUX(16, gpio_141, 1);
		PINMUX(16, gpio_142, 1);

		PINMUX(16, gpio_138, 1);	/* RGMII TX */
		PINMUX(17, gpio_143, 1);
		PINMUX(17, gpio_144, 1);
		PINMUX(17, gpio_145, 1);
		PINMUX(17, gpio_146, 1);
		PINMUX(17, gpio_147, 1);

		PINMUX(17, gpio_149, 1);	/* RGMII MDIO */
		PINMUX(16, gpio_137, 1);

		/* no pulldown on RGMII lines */
		PADCTRL(8, gpio_132_pad_ctrl, 0);
		PADCTRL(8, gpio_133_pad_ctrl, 0);
		PADCTRL(8, gpio_134_pad_ctrl, 0);
		PADCTRL(8, gpio_137_pad_ctrl, 0);
		PADCTRL(8, gpio_138_pad_ctrl, 0);
		PADCTRL(9, gpio_139_pad_ctrl, 0);
		PADCTRL(9, gpio_140_pad_ctrl, 0);
		PADCTRL(9, gpio_141_pad_ctrl, 0);
		PADCTRL(9, gpio_142_pad_ctrl, 0);
		PADCTRL(9, gpio_143_pad_ctrl, 0);
		PADCTRL(9, gpio_144_pad_ctrl, 0);
		PADCTRL(9, gpio_145_pad_ctrl, 0);
		PADCTRL(9, gpio_146_pad_ctrl, 0);
		PADCTRL(9, gpio_147_pad_ctrl, 0);
		PADCTRL(9, gpio_149_pad_ctrl, 0);

		PINMUX(14, gpio_122, 1);	/* SDIO */
		PINMUX(14, gpio_123, 1);
		PINMUX(14, gpio_124, 1);
		PINMUX(14, gpio_125, 1);
		PINMUX(14, gpio_126, 1);
		PINMUX(15, gpio_127, 1);
		PINMUX(15, gpio_128, 1);
		PINMUX(15, gpio_129, 1);
		PINMUX(15, gpio_130, 1);
		PINMUX(15, gpio_131, 1);

		/* no pulldown on SDIO lines */
		PADCTRL(7, gpio_122_pad_ctrl, 0);
		PADCTRL(7, gpio_123_pad_ctrl, 0);
		PADCTRL(8, gpio_124_pad_ctrl, 0);
		PADCTRL(8, gpio_125_pad_ctrl, 0);
		PADCTRL(8, gpio_126_pad_ctrl, 0);
		PADCTRL(8, gpio_127_pad_ctrl, 0);
		PADCTRL(8, gpio_128_pad_ctrl, 0);
		PADCTRL(8, gpio_129_pad_ctrl, 0);
		PADCTRL(8, gpio_130_pad_ctrl, 0);
		PADCTRL(8, gpio_131_pad_ctrl, 0);
	}

#elif defined(CONFIG_BCM7344)

	PINMUX(15, gpio_79, 1);		/* MoCA link */
	PINMUX(15, gpio_80, 1);		/* MoCA activity */

	PINMUX(17, uart_rxdb, 0);	/* UARTB RX */
	PINMUX(17, uart_txdb, 0);	/* UARTB TX */

	PINMUX(0, gpio_68, 3);		/* SDIO */
	PINMUX(0, gpio_69, 3);
	PINMUX(0, gpio_70, 3);
	PINMUX(0, gpio_71, 2);
	PINMUX(0, gpio_72, 2);
	PINMUX(0, gpio_73, 2);
	PINMUX(0, gpio_74, 2);
	PINMUX(0, gpio_75, 2);
	PINMUX(1, gpio_76, 2);
	PINMUX(1, gpio_77, 2);

	/* enable pullup on SDIO lines */
	PADCTRL(0, gpio_68_pad_ctrl, 2);
	PADCTRL(0, gpio_69_pad_ctrl, 2);
	PADCTRL(0, gpio_70_pad_ctrl, 2);
	PADCTRL(0, gpio_71_pad_ctrl, 2);
	PADCTRL(0, gpio_72_pad_ctrl, 2);
	PADCTRL(0, gpio_73_pad_ctrl, 2);
	PADCTRL(0, gpio_74_pad_ctrl, 2);
	PADCTRL(0, gpio_75_pad_ctrl, 2);
	PADCTRL(0, gpio_76_pad_ctrl, 2);
	PADCTRL(0, gpio_77_pad_ctrl, 2);

	AON_PINMUX(0, aon_gpio_00, 3);	/* UARTC RX (NC) */
	AON_PINMUX(0, aon_gpio_01, 3);	/* UARTC TX (NC) */

	PINMUX(16, sgpio_04, 1);	/* MoCA I2C */
	PINMUX(16, sgpio_05, 1);
	brcm_moca_i2c_base = BPHYSADDR(BCHP_BSCD_REG_START);

	/* 7344 is normally MidRF, but some 7418 boards are HighRF */
	if (strstarts(brcm_cfe_boardname, "BCM97418SAT") ||
	    strstarts(brcm_cfe_boardname, "BCM97418SFF_RVU"))
		brcm_moca_rf_band = MOCA_BAND_MIDRF;
	else if (strstarts(brcm_cfe_boardname, "BCM97418"))
		brcm_moca_rf_band = MOCA_BAND_HIGHRF;

#elif defined(CONFIG_BCM7346)

	PINMUX(15, gpio_068, 2);	/* MoCA link */
	PINMUX(16, gpio_069, 1);	/* MoCA activity */

	PINMUX(9, gpio_017, 1);		/* UARTB TX */
	PINMUX(9, gpio_018, 1);		/* UARTB RX */
	PINMUX(10, gpio_021, 1);	/* UARTC TX */
	PINMUX(10, gpio_022, 1);	/* UARTC RX */

	PINMUX(16, sgpio_02, 1);	/* MoCA I2C on BSCB */
	PINMUX(16, sgpio_03, 1);
	brcm_moca_i2c_base = BPHYSADDR(BCHP_BSCB_REG_START);

	/*
	 * To enable SDIO_LED (activity LED) on the BCM97346 reference boards:
	 * install R1127, remove R1120, uncomment this line, and don't use MoCA
	 */
	/* PINMUX(16, sgpio_02, 3); */

	PINMUX(17, vo_656_5, 2);	/* SDIO_PRES */
	PINMUX(17, vo_656_4, 1);	/* SDIO_PWR0 */
	PINMUX(17, vo_656_3, 2);	/* SDIO_DAT3 */
	PINMUX(17, vo_656_2, 2);	/* SDIO_DAT1 */
	PINMUX(17, vo_656_1, 2);	/* SDIO_DAT1 */
	PINMUX(17, vo_656_0, 2);	/* SDIO_DAT0 */

	PINMUX(18, vo_656_clk, 1);	/* SDIO_CLK */
	PINMUX(18, vo_656_7, 1);	/* SDIO_CMD */
	PINMUX(18, vo_656_6, 2);	/* SDIO_WPROT */

#elif defined(CONFIG_BCM7358) || defined(CONFIG_BCM7552) || \
	defined(CONFIG_BCM7360)

	PINMUX(11, gpio_89, 1);		/* UARTB TX */
	PINMUX(11, gpio_90, 1);		/* UARTB RX */
	PINMUX(11, gpio_91, 1);		/* UARTC TX */
	PINMUX(11, gpio_92, 1);		/* UARTC RX */

	AON_PINMUX(1, aon_gpio_08, 6);	/* SDIO */
	AON_PINMUX(1, aon_gpio_12, 5);
	AON_PINMUX(1, aon_gpio_13, 5);
	AON_PINMUX(2, aon_gpio_14, 4);
	AON_PINMUX(2, aon_gpio_15, 5);
	AON_PINMUX(2, aon_gpio_16, 5);
	AON_PINMUX(2, aon_gpio_17, 5);
	AON_PINMUX(2, aon_gpio_18, 5);
	AON_PINMUX(2, aon_gpio_19, 5);
	AON_PINMUX(2, aon_gpio_20, 5);

	/* enable SDIO pullups */
	AON_PADCTRL(0, aon_gpio_08_pad_ctrl, 2);
	AON_PADCTRL(1, aon_gpio_12_pad_ctrl, 2);
	AON_PADCTRL(1, aon_gpio_13_pad_ctrl, 2);
	AON_PADCTRL(1, aon_gpio_14_pad_ctrl, 2);
	AON_PADCTRL(1, aon_gpio_15_pad_ctrl, 2);
	AON_PADCTRL(1, aon_gpio_16_pad_ctrl, 2);
	AON_PADCTRL(1, aon_gpio_17_pad_ctrl, 2);
	AON_PADCTRL(1, aon_gpio_18_pad_ctrl, 2);
	AON_PADCTRL(1, aon_gpio_19_pad_ctrl, 2);
	AON_PADCTRL(1, aon_gpio_20_pad_ctrl, 2);

#elif defined(CONFIG_BCM7425)

	/* Bootloader indicates the availability of SDIO_0 in SCRATCH reg */
	if ((BDEV_RD(SDIO_CFG_REG(0, SCRATCH)) & 0x01) == 0) {
		PINMUX(14, gpio_072, 2);
		PINMUX(14, gpio_073, 2);
		PINMUX(14, gpio_074, 2);
		PINMUX(14, gpio_075, 2);
		PINMUX(15, gpio_076, 2);
		PINMUX(15, gpio_077, 2);
		PINMUX(15, gpio_078, 2);
		PINMUX(15, gpio_079, 2);
		PINMUX(15, gpio_080, 2);
		PINMUX(15, gpio_081, 2);

		/* enable internal pullups */
		BDEV_WR_F_RB(SUN_TOP_CTRL_PIN_MUX_PAD_CTRL_9,
			gpio_072_pad_ctrl, 2);
		BDEV_WR_F_RB(SUN_TOP_CTRL_PIN_MUX_PAD_CTRL_10,
			gpio_073_pad_ctrl, 2);
		BDEV_WR_F_RB(SUN_TOP_CTRL_PIN_MUX_PAD_CTRL_10,
			gpio_074_pad_ctrl, 2);
		BDEV_WR_F_RB(SUN_TOP_CTRL_PIN_MUX_PAD_CTRL_10,
			gpio_075_pad_ctrl, 2);
		BDEV_WR_F_RB(SUN_TOP_CTRL_PIN_MUX_PAD_CTRL_10,
			gpio_076_pad_ctrl, 2);
		BDEV_WR_F_RB(SUN_TOP_CTRL_PIN_MUX_PAD_CTRL_10,
			gpio_077_pad_ctrl, 2);
		BDEV_WR_F_RB(SUN_TOP_CTRL_PIN_MUX_PAD_CTRL_10,
			gpio_078_pad_ctrl, 2);
		BDEV_WR_F_RB(SUN_TOP_CTRL_PIN_MUX_PAD_CTRL_10,
			gpio_079_pad_ctrl, 2);
		BDEV_WR_F_RB(SUN_TOP_CTRL_PIN_MUX_PAD_CTRL_10,
			gpio_080_pad_ctrl, 2);
		BDEV_WR_F_RB(SUN_TOP_CTRL_PIN_MUX_PAD_CTRL_10,
			gpio_081_pad_ctrl, 2);

		/* always use 3.3V (SDIO0_LOW_V_SEL_N=1) */
		/* V00 boards or A0 parts */
		BDEV_UNSET(BCHP_GIO_AON_IODIR_LO, 1 << 4);
		BDEV_SET(BCHP_GIO_AON_DATA_LO, 1 << 4);
		/* V10 boards with B parts use SDIO0_VOLT signal */
		AON_PINMUX(2, gpio_100, 5);
	}

	PINMUX(18, sgpio_00, 1);	/* MoCA I2C */
	PINMUX(19, sgpio_01, 1);
#if defined(CONFIG_BCM7425B0)
	brcm_moca_i2c_base = BPHYSADDR(BCHP_BSCC_REG_START);
#else
	brcm_moca_i2c_base = BPHYSADDR(BCHP_BSCA_REG_START);
#endif

#ifdef CONFIG_PACESTB
       /* Set RGMII lines to 2.5V to match the Realtek Ethernet switch configuration
 	      (DVDDIO_RG pins 9 & 70 should be hooked up to the 2V5 supply) */
        BDEV_WR_F(SUN_TOP_CTRL_GENERAL_CTRL_NO_SCAN_0, rgmii_0_pad_sel, 1);

        /* Configure the GPIO pin muxes for RGMII operation */
        PINMUX(5, gpio_002, 1);    /* RGMII RX */
        PINMUX(5, gpio_003, 1);
        PINMUX(6, gpio_004, 1);
        PINMUX(6, gpio_005, 1);
        PINMUX(6, gpio_006, 1);
        PINMUX(6, gpio_007, 1);

        PINMUX(6, gpio_008, 1);    /* RGMII TX */
        PINMUX(6, gpio_009, 1);
        PINMUX(6, gpio_010, 1);
        PINMUX(6, gpio_011, 1);
        PINMUX(7, gpio_012, 1);
        PINMUX(7, gpio_013, 1);

        /* The Realtek Ethernet switch doesn't use the MDIO interface - it uses I2C instead,
         *                so configure these pins as general GPIOs */
        PINMUX(7, gpio_014, 0);   /* ENET MDIO */
        PINMUX(7, gpio_015, 0);
#endif /* #ifdef CONFIG_PACESTB */

#elif defined(CONFIG_BCM7435)

	PINMUX(16, gpio_088, 1);        /* MoCA LEDs */
	PINMUX(16, gpio_087, 1);

	PINMUX(18, sgpio_00, 1);        /* MoCA I2C */
	PINMUX(19, sgpio_01, 1);
	brcm_moca_i2c_base = BPHYSADDR(BCHP_BSCC_REG_START);
	/* Bootloader indicates the availability of SDIO_0 in SCRATCH reg */
	if ((BDEV_RD(SDIO_CFG_REG(0, SCRATCH)) & 0x01) == 0) {
		PINMUX(14, gpio_072, 2);
		PINMUX(14, gpio_073, 2);
		PINMUX(14, gpio_074, 2);
		PINMUX(14, gpio_075, 2);
		PINMUX(15, gpio_076, 2);
		PINMUX(15, gpio_077, 2);
		PINMUX(15, gpio_078, 2);
		PINMUX(15, gpio_079, 2);
		PINMUX(15, gpio_080, 2);
		PINMUX(15, gpio_081, 2);

		/* enable internal pullups */
		BDEV_WR_F_RB(SUN_TOP_CTRL_PIN_MUX_PAD_CTRL_10,
			     gpio_072_pad_ctrl, 2);
		BDEV_WR_F_RB(SUN_TOP_CTRL_PIN_MUX_PAD_CTRL_10,
			     gpio_073_pad_ctrl, 2);
		BDEV_WR_F_RB(SUN_TOP_CTRL_PIN_MUX_PAD_CTRL_10,
			     gpio_074_pad_ctrl, 2);
		BDEV_WR_F_RB(SUN_TOP_CTRL_PIN_MUX_PAD_CTRL_10,
			     gpio_075_pad_ctrl, 2);
		BDEV_WR_F_RB(SUN_TOP_CTRL_PIN_MUX_PAD_CTRL_10,
			     gpio_076_pad_ctrl, 2);
		BDEV_WR_F_RB(SUN_TOP_CTRL_PIN_MUX_PAD_CTRL_10,
			     gpio_077_pad_ctrl, 2);
		BDEV_WR_F_RB(SUN_TOP_CTRL_PIN_MUX_PAD_CTRL_10,
			     gpio_078_pad_ctrl, 2);
		BDEV_WR_F_RB(SUN_TOP_CTRL_PIN_MUX_PAD_CTRL_10,
			     gpio_079_pad_ctrl, 2);
		BDEV_WR_F_RB(SUN_TOP_CTRL_PIN_MUX_PAD_CTRL_10,
			     gpio_080_pad_ctrl, 2);
		BDEV_WR_F_RB(SUN_TOP_CTRL_PIN_MUX_PAD_CTRL_10,
			     gpio_081_pad_ctrl, 2);

		/* SDIO0_VOLT signal */
		AON_PINMUX(2, gpio_100, 5);
	}

#elif defined(CONFIG_BCM7429)
#ifdef CONFIG_PACESTB
        /* JBH Turn of EBI bus muxes */
        PINMUX(2, gpio_019, BCHP_SUN_TOP_CTRL_PIN_MUX_CTRL_2_gpio_019_GPIO_019);
        PADCTRL(1, gpio_019_pad_ctrl, 1);
        PINMUX(2, gpio_022, BCHP_SUN_TOP_CTRL_PIN_MUX_CTRL_2_gpio_022_GPIO_022);
        PADCTRL(2, gpio_022_pad_ctrl, 1);
        PINMUX(15, gpio_132, BCHP_SUN_TOP_CTRL_PIN_MUX_CTRL_15_gpio_132_GPIO_132);
        PINMUX(16, gpio_134, BCHP_SUN_TOP_CTRL_PIN_MUX_CTRL_16_gpio_134_GPIO_134);
        PINMUX(16, gpio_136, BCHP_SUN_TOP_CTRL_PIN_MUX_CTRL_16_gpio_136_GPIO_136);
        PINMUX(16, gpio_140, BCHP_SUN_TOP_CTRL_PIN_MUX_CTRL_16_gpio_140_GPIO_140);
        PINMUX(17, gpio_141, BCHP_SUN_TOP_CTRL_PIN_MUX_CTRL_17_gpio_141_GPIO_141);
        PINMUX(17, gpio_142, BCHP_SUN_TOP_CTRL_PIN_MUX_CTRL_17_gpio_142_GPIO_142);
        PINMUX(17, gpio_143, BCHP_SUN_TOP_CTRL_PIN_MUX_CTRL_17_gpio_143_GPIO_143);
        PINMUX(17, gpio_144, BCHP_SUN_TOP_CTRL_PIN_MUX_CTRL_17_gpio_144_GPIO_144);
        PINMUX(17, gpio_145, BCHP_SUN_TOP_CTRL_PIN_MUX_CTRL_17_gpio_145_GPIO_145);
        PINMUX(17, gpio_146, BCHP_SUN_TOP_CTRL_PIN_MUX_CTRL_17_gpio_146_GPIO_146);


        /* JBH Set UARTs up as per D520 design */
	AON_PINMUX(1, aon_gpio_12, 7);	/* MoCA LEDs */
	AON_PINMUX(2, aon_gpio_14, 8);  /* JBH: Changed from UART_TX0 aon_gpio_14 */
        
        /* JBH AON GPIO 18 is reset GPIO */
        AON_PINMUX(2, aon_gpio_18, 0);  

        /* JBH Set FP Leds + PWR button */
        AON_PINMUX(0, aon_gpio_04, BCHP_AON_PIN_CTRL_PIN_MUX_CTRL_0_aon_gpio_04_LED_LD0); 
        AON_PINMUX(0, aon_gpio_05, BCHP_AON_PIN_CTRL_PIN_MUX_CTRL_0_aon_gpio_05_LED_LD1); 
        AON_PINMUX(1, aon_gpio_06, BCHP_AON_PIN_CTRL_PIN_MUX_CTRL_1_aon_gpio_06_LED_LD2); 
        AON_PINMUX(1, aon_gpio_07, BCHP_AON_PIN_CTRL_PIN_MUX_CTRL_1_aon_gpio_07_LED_LD3); 
        AON_PINMUX(1, aon_gpio_08, BCHP_AON_PIN_CTRL_PIN_MUX_CTRL_1_aon_gpio_08_LED_LD4); 
        AON_PINMUX(1, aon_gpio_13, BCHP_AON_PIN_CTRL_PIN_MUX_CTRL_1_aon_gpio_13_LED_LS1);
        AON_PINMUX(1, aon_gpio_10, BCHP_AON_PIN_CTRL_PIN_MUX_CTRL_1_aon_gpio_10_LED_LD6);

        /* TJ set power button to GPIO */
        AON_PINMUX(1, aon_gpio_09, BCHP_AON_PIN_CTRL_PIN_MUX_CTRL_1_aon_gpio_09_AON_GPIO_09);        

        /* Set USB pwr pin muxes */ 
        PINMUX(13, gpio_106, BCHP_SUN_TOP_CTRL_PIN_MUX_CTRL_13_gpio_106_USB0_PWRON);
        PINMUX(13, gpio_107, BCHP_SUN_TOP_CTRL_PIN_MUX_CTRL_13_gpio_107_USB1_PWRON);
        PINMUX(13, gpio_110, BCHP_SUN_TOP_CTRL_PIN_MUX_CTRL_13_gpio_110_USB0_PWRFLT);
        PINMUX(13, gpio_111, BCHP_SUN_TOP_CTRL_PIN_MUX_CTRL_13_gpio_111_USB1_PWRFLT);
        
        /* LED enable */
        AON_PINMUX(2, aon_gpio_20, BCHP_AON_PIN_CTRL_PIN_MUX_CTRL_2_aon_gpio_20_AON_GPIO_20);
        BDEV_UNSET(BCHP_GIO_AON_IODIR_LO, 1 << 20);
        BDEV_SET(BCHP_GIO_AON_DATA_LO, 1 << 20);

        /* Disable CLK27_OUT and VCXOA 27MHz clocks */
	BDEV_SET(BCHP_CLKGEN_PAD_CLOCK_DISABLE, 1 << BCHP_CLKGEN_PAD_CLOCK_DISABLE_DISABLE_PAD_OUTPUT_CLK27_CLOCK_SHIFT);
	BDEV_SET(BCHP_CLKGEN_PAD_CLOCK_DISABLE, 1 << BCHP_CLKGEN_PAD_CLOCK_DISABLE_DISABLE_PAD_OUTPUT_VCXO27_CLOCK_SHIFT);
	
        PINMUX( 8, gpio_065, 4);	/* UARTB TX */
	PINMUX( 8, gpio_066, 4);	/* UARTB RX */
	PINMUX( 8, gpio_067, 4);	/* UARTC TX */
	PINMUX( 8, gpio_068, 4);	/* UARTC RX */

	AON_PINMUX(1, aon_gpio_11, 7);	/* MoCA LEDs */
	AON_PINMUX(0, aon_gpio_03, 7);  /* JBH: Changed from UART_TX0 aon_gpio_14 */

	PINMUX(18, sgpio_00, 1);	/* MoCA I2C */
	PINMUX(18, sgpio_01, 1);
	brcm_moca_i2c_base = BPHYSADDR(BCHP_BSCD_REG_START);

        /* JBH: Set up SPDIF pin mux */
        AON_PINMUX(0, aon_gpio_00, BCHP_AON_PIN_CTRL_PIN_MUX_CTRL_0_aon_gpio_00_AUD_SPDIF);

        /* JBH: Set ENET0 link and activity pin mux */
        AON_PINMUX(0, aon_gpio_01, BCHP_AON_PIN_CTRL_PIN_MUX_CTRL_0_aon_gpio_01_ENET0_LINK);
        PINMUX(14, gpio_114, BCHP_SUN_TOP_CTRL_PIN_MUX_CTRL_14_gpio_114_ENET0_ACTIVITY);

        /* JBH: Set up GP500 SPI pin mux */
        PINMUX( 1, gpio_015, BCHP_SUN_TOP_CTRL_PIN_MUX_CTRL_1_gpio_015_GPIO_015);  /* GP500 reset */
        PINMUX(16, gpio_138, BCHP_SUN_TOP_CTRL_PIN_MUX_CTRL_16_gpio_138_SPI_M_MISO);
        PINMUX(16, gpio_139, BCHP_SUN_TOP_CTRL_PIN_MUX_CTRL_16_gpio_139_GPIO_139); /* Set SPI_M_SS0B to GPIO as per GP request */
        PINMUX(17, gpio_147, BCHP_SUN_TOP_CTRL_PIN_MUX_CTRL_17_gpio_147_SPI_M_MOSI);
        PINMUX(18, gpio_149, BCHP_SUN_TOP_CTRL_PIN_MUX_CTRL_18_gpio_149_SPI_M_SCK);
		
        /* JBH: Rear SD pad ctrl internal pull-ups*/
        PADCTRL(2, gpio_023_pad_ctrl, 2);
        PADCTRL(2, gpio_024_pad_ctrl, 0); /* SD1_LED - Pull none */
        PADCTRL(2, gpio_033_pad_ctrl, 2);
        PADCTRL(2, gpio_034_pad_ctrl, 2);
        PADCTRL(2, gpio_035_pad_ctrl, 2);
        PADCTRL(3, gpio_036_pad_ctrl, 2);
        PADCTRL(3, gpio_037_pad_ctrl, 2);
        PADCTRL(3, gpio_038_pad_ctrl, 2);
        PADCTRL(3, gpio_039_pad_ctrl, 2);
        PADCTRL(3, gpio_040_pad_ctrl, 2);

        /* JBH: Rear SD pin mux */
        PINMUX( 3, gpio_024, BCHP_SUN_TOP_CTRL_PIN_MUX_CTRL_3_gpio_024_SD1_PWR0);
        PINMUX( 4, gpio_033, BCHP_SUN_TOP_CTRL_PIN_MUX_CTRL_4_gpio_033_SD1_DAT1);
        PINMUX( 4, gpio_034, BCHP_SUN_TOP_CTRL_PIN_MUX_CTRL_4_gpio_034_SD1_DAT2);
        PINMUX( 4, gpio_035, BCHP_SUN_TOP_CTRL_PIN_MUX_CTRL_4_gpio_035_SD1_DAT3);
        PINMUX( 4, gpio_036, BCHP_SUN_TOP_CTRL_PIN_MUX_CTRL_4_gpio_036_SD1_WPROT);
        PINMUX( 4, gpio_037, BCHP_SUN_TOP_CTRL_PIN_MUX_CTRL_4_gpio_037_SD1_PRES);
        PINMUX( 4, gpio_038, BCHP_SUN_TOP_CTRL_PIN_MUX_CTRL_4_gpio_038_SD1_CMD);
        PINMUX( 4, gpio_039, BCHP_SUN_TOP_CTRL_PIN_MUX_CTRL_4_gpio_039_SD1_CLK);
        PINMUX( 5, gpio_040, BCHP_SUN_TOP_CTRL_PIN_MUX_CTRL_5_gpio_040_SD1_DAT0);

        /* JBH: Rear SD card power enable - retained for CAD-A compatibility*/
        PINMUX(2, gpio_023, BCHP_SUN_TOP_CTRL_PIN_MUX_CTRL_2_gpio_023_GPIO_023);
        BDEV_UNSET(BCHP_GIO_IODIR_LO, 1 << 23);
        BDEV_SET(BCHP_GIO_DATA_LO, 1 << 23);
#if 0
        /* Below requires CAD-A board strapped for SD card SD1_PW1 and SD0_PRES */  
        BDEV_SET(BCHP_GIO_IODIR_LO, 1 << 23); /* Test purposes - make this an input */
        BDEV_SET(BCHP_GIO_IODIR_LO, 1 << 15); /* Test purposes - make this an input */
#endif
	
        /* JBH Set EMMC_PIN_SEL of EMMC_PIN_CTRL to '2' to enable SDIO_1 */
        BDEV_WR_F_RB(HIF_TOP_CTRL_EMMC_PIN_CTRL, EMMC_PIN_SEL, 2);

        
        /* JBH: Set up I2C bus pin mux */
	AON_PINMUX(3, aon_sgpio_02, BCHP_AON_PIN_CTRL_PIN_MUX_CTRL_3_aon_sgpio_02_AON_BSC_S1_SCL); 
	AON_PINMUX(3, aon_sgpio_03, BCHP_AON_PIN_CTRL_PIN_MUX_CTRL_3_aon_sgpio_03_AON_BSC_S1_SDA); 
	AON_PINMUX(3, aon_sgpio_04, BCHP_AON_PIN_CTRL_PIN_MUX_CTRL_3_aon_sgpio_04_AON_BSC_M2_SCL); 
	AON_PINMUX(3, aon_sgpio_05, BCHP_AON_PIN_CTRL_PIN_MUX_CTRL_3_aon_sgpio_05_AON_BSC_M2_SDA); 

	if (1) {     /* We know we have one.. */
#else
	PINMUX(11, gpio_094, 1);	/* UARTB TX */
	PINMUX(11, gpio_095, 1);	/* UARTB RX */
	PINMUX(12, gpio_096, 1);	/* UARTC TX */
	PINMUX(12, gpio_097, 1);	/* UARTC RX */

	AON_PINMUX(1, aon_gpio_11, 7);	/* MoCA LEDs */
	AON_PINMUX(2, aon_gpio_14, 5);

	PINMUX(18, sgpio_00, 1);	/* MoCA I2C */
	PINMUX(18, sgpio_01, 1);
	brcm_moca_i2c_base = BPHYSADDR(BCHP_BSCD_REG_START);

	/* Bootloader indicates the availability of SDIO_0 in SCRATCH reg */
	if ((BDEV_RD(SDIO_CFG_REG(0, SCRATCH)) & 0x01) == 0) {
#endif /* CONFIG_PACESTB */
		/*
		 * 7241 uses GPIO_092 for UART_TX0 instead of SDIO0_VCTL so
		 * leave it alone. We don't need SDIO0_VCTL because the board
		 * is 3.3V only and doesn't use it.
		 */
		if (BRCM_PROD_ID() != 0x7241)
			PINMUX(11, gpio_092, 5);
		PINMUX(14, gpio_122, 1);
		PINMUX(14, gpio_123, 1);
#ifdef CONFIG_PACESTB
		PINMUX(14, gpio_124, 0);                    /* For D890 set SD card power high always to also power the touch button */
                BDEV_UNSET(BCHP_GIO_IODIR_EXT2, 1);         /* Bit 0 = GPIO_124 */
                BDEV_SET(BCHP_GIO_DATA_EXT2, 1);
#else
		PINMUX(14, gpio_124, 1);
#endif
		PINMUX(15, gpio_125, 1);
		PINMUX(15, gpio_126, 1);
		PINMUX(15, gpio_127, 1);
		PINMUX(15, gpio_128, 1);
		PINMUX(15, gpio_129, 1);
		PINMUX(15, gpio_131, 1);
		/*
		 * 7428a0 board uses GPIO_93 for SDIO0_PRES
		 * 7429a0 board uses GPIO_130 for SDIO0_PRES
		 */
		if (BRCM_PROD_ID() == 0x7428) {
			PINMUX(11, gpio_093, 5);
			BDEV_WR_F_RB(SUN_TOP_CTRL_PIN_MUX_PAD_CTRL_6,
						 gpio_093_pad_ctrl, 0);
		} else
			PINMUX(15, gpio_130, 1);

		/* enable internal pullups */
		if (BRCM_PROD_ID() != 0x7241)
			BDEV_WR_F_RB(SUN_TOP_CTRL_PIN_MUX_PAD_CTRL_6,
						 gpio_092_pad_ctrl, 2);
		BDEV_WR_F_RB(SUN_TOP_CTRL_PIN_MUX_PAD_CTRL_8,
			gpio_122_pad_ctrl, 2);
		BDEV_WR_F_RB(SUN_TOP_CTRL_PIN_MUX_PAD_CTRL_8,
			gpio_123_pad_ctrl, 2);
		BDEV_WR_F_RB(SUN_TOP_CTRL_PIN_MUX_PAD_CTRL_8,
			gpio_124_pad_ctrl, 2);
		BDEV_WR_F_RB(SUN_TOP_CTRL_PIN_MUX_PAD_CTRL_8,
			gpio_125_pad_ctrl, 2);
		BDEV_WR_F_RB(SUN_TOP_CTRL_PIN_MUX_PAD_CTRL_8,
			gpio_126_pad_ctrl, 2);
		BDEV_WR_F_RB(SUN_TOP_CTRL_PIN_MUX_PAD_CTRL_8,
			gpio_127_pad_ctrl, 2);
		BDEV_WR_F_RB(SUN_TOP_CTRL_PIN_MUX_PAD_CTRL_8,
			gpio_128_pad_ctrl, 2);
		BDEV_WR_F_RB(SUN_TOP_CTRL_PIN_MUX_PAD_CTRL_8,
			gpio_129_pad_ctrl, 2);
		BDEV_WR_F_RB(SUN_TOP_CTRL_PIN_MUX_PAD_CTRL_9,
			gpio_130_pad_ctrl, 2);
		BDEV_WR_F_RB(SUN_TOP_CTRL_PIN_MUX_PAD_CTRL_9,
			gpio_131_pad_ctrl, 2);
	}
#ifndef CONFIG_PACESTB 
	/* Bootloader indicates the availability of SDIO_1 in SCRATCH reg */
	if ((BDEV_RD(SDIO_CFG_REG(1, SCRATCH)) & 0x01) == 0)
		/* default SDIO1 to eMMC */
		BDEV_SET(BCHP_HIF_TOP_CTRL_EMMC_PIN_CTRL, 0x00000001);
#endif

#elif defined(CONFIG_BCM7563)

	/* NOTE: UARTB and UARTC are disabled by default */

	AON_PINMUX(0, aon_gpio_01, 1);		/* ENET link */
	PINMUX(4, gpio_105, 1);			/* ENET activity */

	/* SDIO pinmux + pullups */
	AON_PINMUX(1, aon_gpio_08, 6);
	AON_PINMUX(1, aon_gpio_12, 5);
	AON_PINMUX(1, aon_gpio_13, 5);
	AON_PINMUX(2, aon_gpio_14, 5);
	AON_PINMUX(2, aon_gpio_15, 5);
	AON_PINMUX(2, aon_gpio_16, 5);
	AON_PINMUX(2, aon_gpio_17, 5);
	AON_PINMUX(2, aon_gpio_18, 5);
	AON_PINMUX(2, aon_gpio_19, 5);
	AON_PINMUX(2, aon_gpio_20, 5);

	AON_PADCTRL(0, aon_gpio_08_pad_ctrl, 2);
	AON_PADCTRL(1, aon_gpio_12_pad_ctrl, 2);
	AON_PADCTRL(1, aon_gpio_13_pad_ctrl, 2);
	AON_PADCTRL(1, aon_gpio_14_pad_ctrl, 2);
	AON_PADCTRL(1, aon_gpio_15_pad_ctrl, 2);
	AON_PADCTRL(1, aon_gpio_16_pad_ctrl, 2);
	AON_PADCTRL(1, aon_gpio_17_pad_ctrl, 2);
	AON_PADCTRL(1, aon_gpio_18_pad_ctrl, 2);
	AON_PADCTRL(1, aon_gpio_19_pad_ctrl, 2);
	AON_PADCTRL(1, aon_gpio_20_pad_ctrl, 2);

#elif defined(CONFIG_BCM7584)

	if (BRCM_PROD_ID() == 0x7584) {
		/* external BCM3349 on GENET_1 using reverse MII */
		PINMUX(0, gpio_00, 2);
		PINMUX(0, gpio_01, 2);
		PINMUX(0, gpio_02, 2);
		PINMUX(1, gpio_03, 2);
		PINMUX(1, gpio_04, 2);
		PINMUX(1, gpio_05, 2);
		PINMUX(1, gpio_06, 2);
		PINMUX(1, gpio_07, 2);
		PINMUX(1, gpio_08, 2);
		PINMUX(1, gpio_09, 2);
		PINMUX(1, gpio_10, 2);
		PINMUX(2, gpio_11, 2);
		PINMUX(2, gpio_12, 2);
		PINMUX(2, gpio_13, 2);
		PINMUX(2, gpio_14, 2);
		PINMUX(13, gpio_110, 1);

		/* no MDIO in RvMII mode; avoid driving these pins */
		PINMUX(15, gpio_123, 0);
		PINMUX(15, gpio_124, 0);

		/* no pulldowns on MII */
		PADCTRL(0, gpio_00_pad_ctrl, 0);
		PADCTRL(0, gpio_01_pad_ctrl, 0);
		PADCTRL(0, gpio_02_pad_ctrl, 0);
		PADCTRL(0, gpio_03_pad_ctrl, 0);
		PADCTRL(0, gpio_04_pad_ctrl, 0);
		PADCTRL(0, gpio_05_pad_ctrl, 0);
		PADCTRL(0, gpio_06_pad_ctrl, 0);
		PADCTRL(0, gpio_07_pad_ctrl, 0);
		PADCTRL(0, gpio_08_pad_ctrl, 0);
		PADCTRL(1, gpio_09_pad_ctrl, 0);
		PADCTRL(1, gpio_10_pad_ctrl, 0);
		PADCTRL(1, gpio_11_pad_ctrl, 0);
		PADCTRL(1, gpio_12_pad_ctrl, 0);
		PADCTRL(1, gpio_13_pad_ctrl, 0);
		PADCTRL(1, gpio_14_pad_ctrl, 0);
		PADCTRL(7, gpio_110_pad_ctrl, 0);
		PADCTRL(8, gpio_123_pad_ctrl, 0);
		PADCTRL(8, gpio_124_pad_ctrl, 0);

		genet_pdata[1].phy_type = BRCM_PHY_TYPE_EXT_RVMII;
		genet_pdata[1].phy_id = BRCM_PHY_ID_NONE;
		genet_pdata[1].phy_speed = 100;

		/* SD card mux + pullups */
		PINMUX(14, gpio_112, 3);
		PINMUX(14, gpio_113, 3);
		PINMUX(14, gpio_114, 3);
		PINMUX(14, gpio_115, 3);
		PINMUX(14, gpio_116, 3);
		PINMUX(14, gpio_117, 3);
		PINMUX(14, gpio_118, 3);
		PINMUX(15, gpio_119, 3);
		PINMUX(15, gpio_120, 3);
		PINMUX(15, gpio_121, 3);
		PINMUX(15, gpio_122, 3);

		PADCTRL(7, gpio_112_pad_ctrl, 2);
		PADCTRL(7, gpio_113_pad_ctrl, 2);
		PADCTRL(7, gpio_114_pad_ctrl, 2);
		PADCTRL(7, gpio_115_pad_ctrl, 2);
		PADCTRL(7, gpio_116_pad_ctrl, 2);
		PADCTRL(7, gpio_117_pad_ctrl, 2);
		PADCTRL(8, gpio_118_pad_ctrl, 2);
		PADCTRL(8, gpio_119_pad_ctrl, 2);
		PADCTRL(8, gpio_120_pad_ctrl, 2);
		PADCTRL(8, gpio_121_pad_ctrl, 2);
		PADCTRL(8, gpio_122_pad_ctrl, 2);
	} else {
		/* 7583 ballout - SD card on alt pins, GENET_1 disabled */
		AON_PINMUX(1, aon_gpio_12, 5);
		AON_PINMUX(1, aon_gpio_13, 5);
		AON_PINMUX(2, aon_gpio_14, 4);
		AON_PINMUX(2, aon_gpio_15, 5);
		AON_PINMUX(2, aon_gpio_16, 5);
		AON_PINMUX(2, aon_gpio_17, 5);
		AON_PINMUX(2, aon_gpio_18, 5);
		AON_PINMUX(2, aon_gpio_19, 5);
		AON_PINMUX(2, aon_gpio_20, 5);

		AON_PADCTRL(1, aon_gpio_12_pad_ctrl, 2);
		AON_PADCTRL(1, aon_gpio_13_pad_ctrl, 2);
		AON_PADCTRL(1, aon_gpio_14_pad_ctrl, 2);
		AON_PADCTRL(1, aon_gpio_15_pad_ctrl, 2);
		AON_PADCTRL(1, aon_gpio_16_pad_ctrl, 2);
		AON_PADCTRL(1, aon_gpio_17_pad_ctrl, 2);
		AON_PADCTRL(1, aon_gpio_18_pad_ctrl, 2);
		AON_PADCTRL(1, aon_gpio_19_pad_ctrl, 2);
		AON_PADCTRL(1, aon_gpio_20_pad_ctrl, 2);
	}

	PINMUX(11, gpio_89, 1);		/* UARTB TX */
	PINMUX(11, gpio_90, 1);		/* UARTB RX */

	AON_PINMUX(1, aon_gpio_07, 4);	/* UARTC RX */
	AON_PINMUX(1, aon_gpio_09, 4);	/* UARTC TX */

	AON_PINMUX(0, aon_gpio_01, 1);	/* ENET link */
	PINMUX(13, gpio_105, 1);	/* ENET activity */

#endif /* chip type */
#endif /* !defined(CONFIG_BRCM_IKOS) */
}
#else
#include <linux/tivoconfig.h>
extern uint32_t boardID;
void board_pinmux_setup(void)
{
    // if we are bridged
    if (boardID & kTivoConfigGen10Strap_IsBridge)
    {
        printk("Fixing bridge...\n");

        // reset MAC
        BDEV_SET(BCHP_SUN_TOP_CTRL_SW_INIT_0_SET, BCHP_SUN_TOP_CTRL_SW_INIT_0_SET_genet0_sw_init_MASK);

        // put bridge into reset via gpio_003
        BDEV_UNSET(BCHP_GIO_IODIR_LO, 1 << 3);              // make it an output
        BDEV_UNSET(BCHP_GIO_DATA_LO, 1 << 3);               // take it low 

        // gpio_132 is RGMII RX, but happens also to connect ar8328 802.3az
        // strap option. We want to force it low before taking then switch out of reset.
        PINMUX(15, gpio_132, 0);                            // make it a gpio
        BDEV_UNSET(BCHP_GIO_IODIR_EXT2, 1 << (132-124) );   // make it an output
        BDEV_UNSET(BCHP_GIO_DATA_EXT2, 1 << (132-124) );    // take it low

        // take switch out of reset
        BDEV_SET(BCHP_GIO_DATA_LO, 1 << 3);                 // take it high
        udelay(10000);                                      // allow 10 mS
       
        // restore gpio_132 to RGMII RX
        PINMUX(15, gpio_132, 1); 

        // also enable 2.5V RGMII operation
        BDEV_WR_F(SUN_TOP_CTRL_GENERAL_CTRL_NO_SCAN_0, rgmii_0_pad_sel, 1);
        
        // last, re-enable MAC
        BDEV_SET(BCHP_SUN_TOP_CTRL_SW_INIT_0_CLEAR, BCHP_SUN_TOP_CTRL_SW_INIT_0_SET_genet0_sw_init_MASK);
    }    
    
    // enable uarts
    AON_PINMUX(1, aon_gpio_09, 8);  // uart 0 tx
    AON_PINMUX(1, aon_gpio_07, 8);  // uart 0 rx 
    PINMUX(11, gpio_094, 1);        // uart 1 tx
    PINMUX(11, gpio_095, 1);        // uart 1 rx
    PINMUX(12, gpio_096, 1);        // uart 2 tx
    PINMUX(12, gpio_097, 1);        // uart 2 rx

    // MMC uses gpios 25-31 (data), 43 (clock), 44 (cmd)
    // first make sure they are all pullups
    PADCTRL(2, gpio_025_pad_ctrl, 2);
    PADCTRL(2, gpio_026_pad_ctrl, 2);
    PADCTRL(2, gpio_027_pad_ctrl, 2);
    PADCTRL(2, gpio_028_pad_ctrl, 2);
    PADCTRL(2, gpio_029_pad_ctrl, 2);
    PADCTRL(2, gpio_030_pad_ctrl, 2);
    PADCTRL(2, gpio_031_pad_ctrl, 2);
    PADCTRL(3, gpio_043_pad_ctrl, 2);
    PADCTRL(3, gpio_044_pad_ctrl, 2);

    // then enable the EBI 
    PINMUX(3, gpio_025, 1);
    PINMUX(3, gpio_026, 1);
    PINMUX(3, gpio_027, 1);
    PINMUX(3, gpio_028, 1);
    PINMUX(3, gpio_029, 1);
    PINMUX(3, gpio_030, 1);
    PINMUX(3, gpio_031, 1);
    PINMUX(4, gpio_032, 1);
    PINMUX(5, gpio_043, 1);
    PINMUX(5, gpio_044, 1);

    // set the magic EMMC bit
    BDEV_WR(BCHP_HIF_TOP_CTRL_EMMC_PIN_CTRL, 1);

    // mux USB PWRON and PWRFLT pins
    PINMUX(13, gpio_106, 1);
    PINMUX(13, gpio_107, 1);
    PINMUX(13, gpio_108, 1);
    PINMUX(13, gpio_109, 1);
    PINMUX(13, gpio_110, 1);
    PINMUX(13, gpio_111, 1);
    PINMUX(13, gpio_112, 1);
    PINMUX(14, gpio_113, 1);

    // set pull up on the PWRFLT pins
    PADCTRL(7, gpio_110_pad_ctrl, 2);
    PADCTRL(8, gpio_111_pad_ctrl, 2);
    PADCTRL(8, gpio_112_pad_ctrl, 2);
    PADCTRL(8, gpio_113_pad_ctrl, 2);

}
#endif

/***********************************************************************
 * FLASH configuration
 ***********************************************************************/

#if defined(CONFIG_BRCM_FIXED_MTD_PARTITIONS)

static struct mtd_partition fixed_partition_map[] = {
	{
		.name = "entire_device",
		.size = MTDPART_SIZ_FULL,
		.offset = 0x00000000
	},
};

/*
 * Use the partition map defined at compile time
 */
int __init board_get_partition_map(struct mtd_partition **p)
{
	*p = fixed_partition_map;
	return ARRAY_SIZE(fixed_partition_map);
}

#else /* defined(CONFIG_BRCM_FIXED_MTD_PARTITIONS) */

/*
 * Construct the partition map for the primary flash device, based on
 * CFE environment variables that were read from prom.c
 *
 * Note that this does NOT have all of the partitions that CFE recognizes
 * (e.g. macadr, nvram).  It only has the rootfs, entire_device, and
 * optionally the kernel image partition.
 */
int __init board_get_partition_map(struct mtd_partition **p)
{
	struct mtd_partition *ret;
	int nr_parts;

#ifdef CONFIG_PACESTB
	/* This is the sort of partitioning we want on PACE boards */
	/* Below is the setup for Delphi, this will get overridden */
	/* by the mtdparts kernel command line parameter           */
	nr_parts = 4;
	ret = kzalloc(sizeof(struct mtd_partition) * nr_parts, GFP_KERNEL);
	if (! ret)
		return -ENOMEM;

	ret[0].offset = 0;
	ret[0].size = 0x1000000;
	ret[0].name = "kernel";

	ret[1].offset = 0x1000000;
	ret[1].size = 0xef80000;
	ret[1].name = "ubi_nand";

	ret[2].offset = 0xff80000;   /* 4 blocks of 128K each */
	ret[2].size = 0x0080000;
	ret[2].name = "bbt";

	ret[3].offset = 0;
	ret[3].size = 0x10000000;
	ret[3].name = "entire_device";
#else
	if (brcm_mtd_rootfs_len == 0)
		return -ENODEV;

	nr_parts = 2;
	if (brcm_mtd_kernel_len != 0)
		nr_parts++;

	ret = kzalloc(nr_parts * sizeof(struct mtd_partition), GFP_KERNEL);
	if (!ret)
		return -ENOMEM;

	ret[0].offset = brcm_mtd_rootfs_start;
	ret[0].size = brcm_mtd_rootfs_len;
	ret[0].name = "rootfs";

	ret[1].offset = 0;
	ret[1].size = MTDPART_SIZ_FULL;
	ret[1].name = "entire_device";

	if (brcm_mtd_kernel_len != 0) {
		ret[2].offset = brcm_mtd_kernel_start;
		ret[2].size = brcm_mtd_kernel_len;
		ret[2].name = "kernel";
	}
#endif /* CONFIG_PACESTB */

	*p = ret;
	return nr_parts;
}
#endif /* defined(CONFIG_BRCM_FIXED_MTD_PARTITIONS) */

void brcm_get_ocap_info(struct brcm_ocap_info *info)
{
	info->ocap_part_start = brcm_mtd_ocap_start;
	info->ocap_part_len = brcm_mtd_ocap_len;
	info->docsis_platform = brcm_docsis_platform;
}
EXPORT_SYMBOL(brcm_get_ocap_info);
