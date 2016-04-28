/*******************************************************************************
*
* Common/Inc/hw_z2_dvr.h
*
* Description: Header file for general Zip2 support (all datapaths)
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

#ifndef _HW_Z2_H_
#define _HW_Z2_H_

#include "inctypes_dvr.h"
#include "common_dvr.h"

/* Fragment Gather Support:
 *
 * For stock Linux implementations, 1 FRAG is necessary and sufficient. 
 * - Standard rule of thumb is driver must support IP chksum (NETIF_F_IP_CSUM)
 *   before it makes sense to enable scatter/gather (NETIF_F_SG) in the
 *   dev->features flag mask.  Linux always builds a single buffer WHEN it has
 *   to do to its own IP Checksum (defeating NETIF_F_SG) !!
 * - Due to various restrictions (see custom), general support of _SG would
 *   require backstop code to coalesce when restrictions are violated.  See
 *   drivers/net/typhoon.c for some good examples.
 *
 * For custom implementations, Gathered Fragments can be supported. Details ...
 * - Limit of 4 Fragments exists with current CCPU/TC code
 * -- See CLNK_HW_MAX_FRAGMENTS compile define below.
 * -- Common driver also limited to 4. (See CLNK_ETH_MAX_FRAGMENTS. Modifiable)
 * -- Has been well tested up to 4 Fragments.
 * -- Configuring more than you need wastes some host driver space
 * -- Configuring more than you need increases ptr_list DMA size
 * - Restrictions below this point are not validated by code!
 * -- Minimum Fragment must be at least 32 bytes
 * -- External Fragment data can be on any byte host address boundary.
 * -- Internal Fragment data has to start with 4 byte aligned.
 * -- All fragments must be multiple of 4 in length, except final.
 * - See SIMULATE_FRAGMENTS define in eth.c for example code.
 * -- Includes tests which set successive frag sizes to 32, 64, 128 ...
 */
#define CLNK_HW_MAX_FRAGMENTS   1
#if (CLNK_HW_MAX_FRAGMENTS > 4)
#warning "Current 1.5 (SOC,TC) Xmit Support limited to 4 Fragments"
#endif
struct _frag_desc
{
    SYS_UINT32              ptr;
    SYS_UINT32              len;
};
struct ptr_list
{
    SYS_UINT32              n_frags;
    struct _frag_desc       fragments[CLNK_HW_MAX_FRAGMENTS];
    SYS_UINT32              magic;
};

#define EHI_START       0x0c100000
#define EHI_END         0x0c1003fc

#define AT1_BASE        0x0c000000
#define AT2_BASE        0x0c080000
#define AT3_BASE        0x0c100400

#define HOST_INTR       (1 << 1)        /* interrupt bit for tx/rx/mbx */
#define TMR_INTR        (1 << 24)       /* interrupt bit for zip2 timer 0 */

#if ! defined(CLNK_ETH_ENDIAN_SWAP)
#define SET_HOST_DESC(desc, param, val)           \
            (desc)->param = (val)
#else
#define SET_HOST_DESC(desc, param, val)           \
            (desc)->param = HOST_OS_ENDIAN_SWAP(val)

#endif /* ! defined(CLNK_ETH_ENDIAN_SWAP) */

#define RX_STATUS_PTR(ctx, idx) ((ctx)->rx_linebuf + sizeof(struct linebuf) + ((idx) << 2))

#define TX_STATUS_PTR(ctx, pri, idx) ((ctx)->tx_linebuf[pri] + sizeof(struct linebuf) + ((idx) << 2))


/*
 * CLNK_REG_*() and soc*Mem() take full Sonics addresses on Zip2,
 * and handle the translation themselves for buses on which it is necessary
 */
#define SETUP_ATRANS(pContext, addr) do { } while(0)

/*
 * ZIP2 REGISTERS
 */

#define CLNK_ETH_INTERRUPT_FLAG  0x01

// PCI device/vendor ID
#define CLNK_ETH_PCI_DEVICE_ID  0x002117e6

// Unused macro (Zip1b only)
#define SEM_INIT_DONE_BITS() do { } while(0)

#define CLNK_REG_ETH_MAC_ADDR_HIGH              DEV_MAC_ADDRESS0H
#define CLNK_REG_ETH_MAC_ADDR_LOW               DEV_MAC_ADDRESS0L

#define EHI_BASE                    0x0c100000
#define CSC_BASE                    0x0c100400
#define CCPU_BASE                   0x0c100800
#define DIC_CSR_BASE                0x0c100c00
#define ETH_CSR_BASE                0x0c103000
#define GPHY_BASE                   0x0c103800
#define DIC_BASE                    0x0c104000
#define CPC_BASE                    0x0c108000
#define RFIC_BASE                   0x0c109000
#define FEC_BASE                    0x0c10a000
#define CPC_TC_BASE                 0x0c10c000
#define PHY_RX_BASE                 0x0c110000
#define PHY_TX_BASE                 0x0c118000
#define PCIE_CSR_BASE               0x0c102000
#define HOST_INTR_TO_INTX           (1 << 10)
#define PCIE_CSC_CORE_CTL           (PCIE_CSR_BASE + 0xc00)

#define EHI_VERSION                 (EHI_BASE + 0x00)
#define DEV_MAC_ADDRESS0L           (EHI_BASE + 0xd0)
#define DEV_MAC_ADDRESS0H           (EHI_BASE + 0xd4)

#define CLNK_REG_CPU_COLD_RESET_BIT (1 << 0)
#define CLNK_REG_CPU_RESET          (CCPU_BASE + 0x04)
#define CLNK_SYS_RST_CONTROL        (CCPU_BASE + 0x7c)

#define EHI_INTR_OUT_MASK           (EHI_BASE + 0x24)
#define CSC_INTR_BUS                (CSC_BASE + 0xc)

#define EHI_TIMEOUT_CTL             (EHI_BASE + 0x38)
#define EHI_ERR_ADDR_EXT            (EHI_BASE + 0xf8)
#define EHI_ERR_ADDR_INT            (EHI_BASE + 0xfc)
#define EHI_ERR_AT                  (EHI_BASE + 0xec)
#define EHI_ERR_DET                 (EHI_BASE + 0x1c)

#define EHI_MISC_CTL                (EHI_BASE + 0x90)
#define EHI_AT1_CTL                 (EHI_BASE + 0x50)
#define EHI_AT1_ADDR                (EHI_BASE + 0x54)
#define EHI_AT1_MASK                (EHI_BASE + 0x58)
#define EHI_AT2_CTL                 (EHI_BASE + 0x60)
#define EHI_AT2_ADDR                (EHI_BASE + 0x64)
#define EHI_AT2_MASK                (EHI_BASE + 0x68)
#define EHI_AT3_CTL                 (EHI_BASE + 0x70)
#define EHI_AT3_ADDR                (EHI_BASE + 0x74)
#define EHI_AT3_MASK                (EHI_BASE + 0x78)

#define IMI_AT0_CTL                 (EHI_BASE + 0xa0)
#define IMI_AT0_ADDR                (EHI_BASE + 0xa4)
#define IMS_CTL_0                   (EHI_BASE + 0xc0)
#define IMS_CTL_1                   (EHI_BASE + 0xc4)
#define IMS_CTL_2                   (EHI_BASE + 0xc8)
#define IMI_ARB_CTL_0               (EHI_BASE + 0xe0)
#define IMI_ARB_CTL_1               (EHI_BASE + 0xe4)
#define IMI_ARB_CTL_2               (EHI_BASE + 0xe8)
#if ECFG_CHIP_MAVERICKS 
#define CPC_TC_INST_MEM             (CPC_TC_BASE + 0x0)
#define CPC_TC_INST_MEM_SZ          0x3000
#define CPC_TC_DATA_MEM_SZ          0x2000
#define CPC_TC_DATA_MEM             (CPC_TC_BASE + CPC_TC_INST_MEM_SZ)
#endif

#define DIC_TC_CTL_0                (DIC_CSR_BASE + 0x50)
#define DIC_MISC_CTL_0              (DIC_CSR_BASE + 0x40)
#define DIC_ATRANS0_CTL0            (DIC_CSR_BASE + 0x70)
#define DIC_ATRANS1_CTL0            (DIC_CSR_BASE + 0x78)
#define DIC_TC_NOTIFY_SET           (DIC_CSR_BASE + 0x5c)

#define DIC_TC_INST_MEM             (DIC_BASE + 0x00)
#if ECFG_CHIP_ZIP2
#define DIC_TC_INST_MEM_SZ          0x2000
#define DIC_TC_DATA_MEM_SZ          0x1000
#elif ECFG_CHIP_MAVERICKS
#define DIC_TC_INST_MEM_SZ          0x2800
#define DIC_TC_DATA_MEM_SZ          0x1800
#endif
/*For ZIP1 there are no TCs*/
#define DIC_TC_DATA_MEM             (DIC_BASE + DIC_TC_INST_MEM_SZ)

#define DIC_D_STATUS                (DIC_TC_DATA_MEM + 0x00)
#define DIC_D_DESC_TX_PTR           (DIC_TC_DATA_MEM + 0x04)
#define DIC_D_DESC_RX_PTR           (DIC_TC_DATA_MEM + 0x08)
#define DIC_D_MISC_CFG              (DIC_TC_DATA_MEM + 0x0c)
#define DIC_D_PROC_STATE            (DIC_TC_DATA_MEM + 0x10)
#define DIC_D_LINEBUF_SOC_RX        (DIC_TC_DATA_MEM + 0x20)
#define DIC_D_MIN_PKT_SIZE          (DIC_TC_DATA_MEM + 0x38)
#define DIC_D_LINEBUF_HOST_RX       (DIC_TC_DATA_MEM + 0x24)
#define DIC_D_LINEBUF0_SOC_TX       (DIC_TC_DATA_MEM + 0x2c)
#define DIC_D_ETH_SPU_LEN_W_FCS_MAX (DIC_TC_DATA_MEM + 0x30)
#define DIC_D_AGGR_HDR_SPU_CNT_MAX  (DIC_TC_DATA_MEM + 0x34)
#define DIC_D_RX_DST_PTR            (DIC_TC_DATA_MEM + 0x68)
#define DIC_D_RX_DST_IDX            (DIC_TC_DATA_MEM + 0x6c)
#define DIC_D_TX_DMA_CTL            (DIC_TC_DATA_MEM + 0x78)
#define DIC_D_RX_DMA_CTL            (DIC_TC_DATA_MEM + 0x7c)


#define DIC_CFG_USAGE_MODE_PCI      0x00000000
#define DIC_CFG_USAGE_MODE_MII      0x00000001
#define DIC_CFG_BAD_PKT_FLTR_OFF    0x00000000
#define DIC_CFG_BAD_PKT_FLTR_ON     0x00000010
#define DIC_CFG_SPU_FCS_ENABLED     0x00000020
#define DIC_CFG_SPU_FCS_DISABLED    0x00000000
#define DIC_CFG_AGGR_DST_ONLY       0x00000040
#define DIC_CFG_AGGR_DST_AND_PRI    0x00000000
#define DIC_CFG_TX_BUF_PROF_WAN     0x00000080
#define DIC_CFG_TX_BUF_PROF_LAN     0x00000000
#define DIC_CFG_MII_SPEED_MASK      0x000FFF00
#define DIC_CFG_PQOS_MODE_MASK      0x00F00000
#define DIC_CFG_PQOS_MODE_MOCA_11   0x00000000
#define DIC_CFG_PQOS_MODE_PUPQ      0x00100000

#define DIC_TX_DMA_CTL_WORD         0x00048000
#define DIC_RX_DMA_CTL_WORD         0x00048100

#define CSC_CLK_CTL1                (CSC_BASE + 0xe4)

#define CSC_SYS_TMR_LO              (CSC_BASE + 0x70)
#define CSC_TMR_TIMEOUT_0           (CSC_BASE + 0x80)
#define CSC_TMR_MASK_0              (CSC_BASE + 0x84)

#define GPHY_MAC_CONFIG             (GPHY_BASE + 0x00)
#define GPHY_FILTER                 (GPHY_BASE + 0x04)
#define GPHY_FLOW                   (GPHY_BASE + 0x18)

#define GPHY_FLOW_MAC_LO	    (GPHY_BASE + 0x40)		// Low 2 bytes of mac address as (xx xx MM ZZ) where MM is lowest byte
#define GPHY_FLOW_MAC_HI	    (GPHY_BASE + 0x44)		// High 4 bytes of mac address in reverse order 
                                                                //             (AA BB CC DD) where DD is highest byte.
#define GPHY_BMCR                   (GPHY_BASE + 0x100)
#define GPHY_ENH_CTRL               (GPHY_BASE + 0x168)
#define GPHY_BUS_MODE               (GPHY_BASE + 0x200)
#define GPHY_RX_DESCR_PTR           (GPHY_BASE + 0x20c)
#define GPHY_TX_DESCR_PTR           (GPHY_BASE + 0x210)
#define GPHY_OP_MODE                (GPHY_BASE + 0x218)

#define CLNK_REG_MBX_REG_1          DEV_SHARED(mailbox_reg[0])
#define CLNK_REG_MBX_REG_9          DEV_SHARED(mailbox_reg[8])
#define CLNK_REG_MBX_REG_16         DEV_SHARED(mailbox_reg[15])
#define CLNK_REG_MBX_READ_CSR       DEV_SHARED(read_csr_reg)
#define CLNK_REG_MBX_WRITE_CSR      DEV_SHARED(write_csr_reg)
#define CLNK_REG_MBX_SEMAPHORE_BIT  0x80000000UL

#define CLNK_REG_DEBUG_0            DEV_SHARED(debug_reg[0])
#define CLNK_REG_DEBUG_1            DEV_SHARED(debug_reg[1])

#define CLNK_REG_MBX_FIRST          CLNK_REG_MBX_REG_1
#define CLNK_REG_MBX_LAST           CLNK_REG_MBX_REG_16

#endif /* ! _HW_Z2_H_ */
