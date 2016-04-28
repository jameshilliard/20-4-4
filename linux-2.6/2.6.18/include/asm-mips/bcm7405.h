/*
 * Register defs for Broadcom BCM7405 host controller
 *
 * Copyright (C) 2006 TiVo, Inc. All Rights Reserved.
 * Portions (C) 2008 Koherence, LLC.
 */
#ifndef __ASM_BCM7405_H
#define __ASM_BCM7405_H

#include <asm/addrspace.h>
///////////////////////////////////////////////////////////////////////////////
//
// Interrupts
//
///////////////////////////////////////////////////////////////////////////////

/*
 * MIPS Hardware interrupts 
 */
#define CPU_IRQ_SW1     0  /* CPU CP0_STATUS.IP[0] - Software interrupt      */
#define CPU_IRQ_SW2     1  /* CPU CP0_STATUS.IP[1] - Software interrupt      */
#define CPU_IRQ_EINT0   2  /* CPU CP0_STATUS.IP[2] - External interrupt      */
#define CPU_IRQ_EINT1   3  /* CPU CP0_STATUS.IP[3] - External interrupt      */
#define CPU_IRQ_EINT2   4  /* CPU CP0_STATUS.IP[4] - External interrupt      */
#define CPU_IRQ_EINT3   5  /* CPU CP0_STATUS.IP[5] - External interrupt      */
#define CPU_IRQ_EINT4   6  /* CPU CP0_STATUS.IP[6] - External interrupt      */
#define CPU_IRQ_TMR     7  /* CPU CP0_STATUS.IP[7] - Cycle Timer interrupt   */

/*
 * The following block of logical interrupts (HIF_CPU_INTR1) are routed to 
 * CPU_IRQ_INT0 and are grouped together to correspond with HW registers
 * of HIF_CPU_INTR1_INTR_W0_xxx group, where xxx is {STATUS, SET, CLEAR}
 *
 * Bit [i] in W0 regs is corresponging to logical irq number 8+i here.
 */
#define BCM_IRQ_HIF_CPU_INTR             8  // 00    HIF_CPU_INTR       
#define BCM_IRQ_XPT_STATUS_CPU_INTR      9  // 01    XPT_STATUS_CPU_INTR
#define BCM_IRQ_XPT_OVFL_CPU_INTR       10  // 02    XPT_OVFL_CPU_INTR  
#define BCM_IRQ_XPT_MSG_CPU_INTR        11  // 03    XPT_MSG_CPU_INTR   
#define BCM_IRQ_W0_PRIVATE_4            12  // 04    private            
#define BCM_IRQ_W0_PRIVATE_5            13  // 05    private            
#define BCM_IRQ_AIO_CPU_INTR            14  // 06    AIO_CPU_INTR       
#define BCM_IRQ_GFX_CPU_INTR            15  // 07    GFX_CPU_INTR       
#define BCM_IRQ_HDMI_CPU_INTR           16  // 08    HDMI_CPU_INTR      
#define BCM_IRQ_RPTD_CPU_INTR           17  // 09    RPTD_CPU_INTR      
#define BCM_IRQ_VEC_CPU_INTR            18  // 10    VEC_CPU_INTR       
#define BCM_IRQ_BVNB_CPU_INTR           19  // 11    BVNB_CPU_INTR      
#define BCM_IRQ_BVNF_CPU_INTR_0         20  // 12    BVNF_CPU_INTR_0    
#define BCM_IRQ_BVNF_CPU_INTR_1         21  // 13    BVNF_CPU_INTR_1    
#define BCM_IRQ_BVNF_CPU_INTR_2         22  // 14    BVNF_CPU_INTR_2    
#define BCM_IRQ_ENET_CPU_INTR           23  // 15    ENET_CPU_INTR      
#define BCM_IRQ_RFM_CPU_INTR            24  // 16    RFM_CPU_INTR       
#define BCM_IRQ_UPG_TMR_CPU_INTR        25  // 17    UPG_TMR_CPU_INTR   
#define BCM_IRQ_UPG_CPU_INTR            26  // 18    UPG_CPU_INTR       
#define BCM_IRQ_UPG_BSC_CPU_INTR        27  // 19    UPG_BSC_CPU_INTR   
#define BCM_IRQ_UPG_SPI_CPU_INTR        28  // 20    UPG_SPI_CPU_INTR   
#define BCM_IRQ_UPG_UART0_CPU_INTR      29  // 21    UPG_UART0_CPU_INTR 
#define BCM_IRQ_UPG_SC_CPU_INTR         30  // 22    UPG_SC_CPU_INTR    
#define BCM_IRQ_SUN_CPU_INTR            31  // 23    SUN_CPU_INTR       
#define BCM_IRQ_UHF1_CPU_INTR           32  // 24    UHF1_CPU_INTR      
#define BCM_IRQ_W0_RESERVED_25          33  // 25    Reserved           
#define BCM_IRQ_USB_CPU_INTR            34  // 26    USB_CPU_INTR       
#define BCM_IRQ_MC_CPU_INTR             35  // 27    MC_CPU_INTR        
#define BCM_IRQ_BVNF_CPU_INTR_3         36  // 28    BVNF_CPU_INTR_3    
#define BCM_IRQ_SATA_PCIB_CPU_INTR      37  // 29    SATA_PCIB_CPU_INTR 
#define BCM_IRQ_AVD0_CPU_INTR_0         38  // 30    AVD0_CPU_INTR_0    
#define BCM_IRQ_XPT_RAV_CPU_INTR        39  // 31    XPT_RAV_CPU_INTR   

/*
 * The following block of logical interrupts (HIF_CPU_INTR2) are routed to
 * BCM_IRQ_HIF_CPU_INTR of CPU_IRQ_INT0 and are grouped together to correspond
 * with HW registers of HIF_CPU_INTR1_INTR_W1_xxx group, where xxx is 
 * {STATUS, SET, CLEAR}
 *
 * Bit [i] in W1 regs is corresponging to logical irq number 8+32+i here.
 */
#define BCM_IRQ_PCI_INTA_0_CPU_INTR     40  // 00    PCI_INTA_0_CPU_INTR
#define BCM_IRQ_PCI_INTA_1_CPU_INTR     41  // 01    PCI_INTA_1_CPU_INTR
#define BCM_IRQ_PCI_INTA_2_CPU_INTR     42  // 02    PCI_INTA_2_CPU_INTR
#define BCM_IRQ_W1_RESERVED_3           43  // 03    Reserved
#define BCM_IRQ_EXT_IRQ_0_CPU_INTR      44  // 04    EXT_IRQ_0_CPU_INTR
#define BCM_IRQ_EXT_IRQ_1_CPU_INTR      45  // 05    EXT_IRQ_1_CPU_INTR
#define BCM_IRQ_EXT_IRQ_2_CPU_INTR      46  // 06    EXT_IRQ_2_CPU_INTR
#define BCM_IRQ_EXT_IRQ_3_CPU_INTR      47  // 07    EXT_IRQ_3_CPU_INTR
#define BCM_IRQ_EXT_IRQ_4_CPU_INTR      48  // 08    EXT_IRQ_4_CPU_INTR
#define BCM_IRQ_PCI_SATA_CPU_INTR       49  // 09    PCI_SATA_CPU_INTR
#define BCM_IRQ_EXT_IRQ_5_CPU_INTR      50  // 10    EXT_IRQ_5_CPU_INTR
#define BCM_IRQ_EXT_IRQ_6_CPU_INTR      51  // 11    EXT_IRQ_6_CPU_INTR
#define BCM_IRQ_EXT_IRQ_7_CPU_INTR      52  // 12    EXT_IRQ_7_CPU_INTR
#define BCM_IRQ_EXT_IRQ_8_CPU_INTR      53  // 13    EXT_IRQ_8_CPU_INTR
#define BCM_IRQ_EXT_IRQ_9_CPU_INTR      54  // 14    EXT_IRQ_9_CPU_INTR
#define BCM_IRQ_EXT_IRQ_10_CPU_INTR     55  // 15    EXT_IRQ_10_CPU_INTR
#define BCM_IRQ_EXT_IRQ_11_CPU_INTR     56  // 16    EXT_IRQ_11_CPU_INTR
#define BCM_IRQ_EXT_IRQ_12_CPU_INTR     57  // 17    EXT_IRQ_12_CPU_INTR
#define BCM_IRQ_EXT_IRQ_13_CPU_INTR     58  // 18    EXT_IRQ_13_CPU_INTR
#define BCM_IRQ_EXT_IRQ_14_CPU_INTR     59  // 19    EXT_IRQ_14_CPU_INTR
#define BCM_IRQ_W1_RESERVED_20          60  // 20    Reserved
#define BCM_IRQ_W1_RESERVED_21          61  // 21    Reserved
#define BCM_IRQ_W1_RESERVED_22          62  // 22    Reserved
#define BCM_IRQ_W1_RESERVED_23          63  // 23    Reserved
#define BCM_IRQ_W1_RESERVED_24          64  // 24    Reserved
#define BCM_IRQ_W1_RESERVED_25          65  // 25    Reserved
#define BCM_IRQ_XPT_PCR_CPU_INTR        66  // 26    XPT_PCR_CPU_INTR
#define BCM_IRQ_XPT_FE_CPU_INTR         67  // 27    XPT_FE_CPU_INTR
#define BCM_IRQ_XPT_MSG_STAT_CPU_INTR   68  // 28    XPT_MSG_STAT_CPU_INTR
#define BCM_IRQ_USB_EHCI_CPU_INTR       69  // 29    USB_EHCI_CPU_INTR
#define BCM_IRQ_USB_OHCI_0_CPU_INTR     70  // 30    USB_OHCI_0_CPU_INTR
#define BCM_IRQ_USB_OHCI_1_CPU_INTR     71  // 31    USB_OHCI_1_CPU_INTR

//
// FIXME: The rest of software irq values is NOT yet fixed for 7401.
//

/*
 * The following block of logical interrupts are routed to 
 * BCM_IRQ_UPG_CPU_INTR with IRQ0_ENABLE register for 
 * enabling/disabling and are grouped together to correspond 
 * with register interfaces
 *
 */
#define BCM_IRQ_KEYBRD1_CPU_INTR          72
#define BCM_IRQ_KEYPAD1_CPU_INTR          73
#define BCM_IRQ_IRBLASTER_CPU_INTR        74

/*FIXME:Remove the CONFIG_MIPS_BRCM974XX*/
#ifdef CONFIG_MIPS_BRCM974XX

#define BCM_IRQ_UARTB_CPU_INTR            75
#define BCM_IRQ_UARTA_CPU_INTR            76

#else /*CONFIG_MIPS_BRCM974XX*/

#define BCM_IRQ_UARTA_CPU_INTR            75
#define BCM_IRQ_UARTB_CPU_INTR            76

#endif /*CONFIG_MIPS_BRCM974XX*/

#define BCM_IRQ_KEYBRD2_CPU_INTR          77
#define BCM_IRQ_GIO_CPU_INTR              78
#define BCM_IRQ_ICAP_CPU_INTR             79
#define BCM_IRQ_KEYBRD3_CPU_INTR          80
#define BCM_IRQ_UARTC_CPU_INTR            81

/* the next seven IRQs are reserved by the h/w */
//#define BCM_IRQ0_RSVD_INTR_9              81
#define BCM_IRQ0_RSVD_INTR_10             82
#define BCM_IRQ0_RSVD_INTR_11             83
#define BCM_IRQ0_RSVD_INTR_12             84
#define BCM_IRQ0_RSVD_INTR_13             85
#define BCM_IRQ0_RSVD_INTR_14             86
#define BCM_IRQ0_RSVD_INTR_15             87

#define BCM_IRQ_UARTA_SEC_CPU_INTR        88

/* the next three IRQs are reserved by the h/w */
#define BCM_IRQ0_RSVD_INTR_17             89
#define BCM_IRQ0_RSVD_INTR_18             90
#define BCM_IRQ0_RSVD_INTR_19             91
#define BCM_IRQ_SPI_MASTER_CPU_INTR       92
/* the next three IRQs are reserved by the h/w */
#define BCM_IRQ0_RSVD_INTR_21             93
#define BCM_IRQ0_RSVD_INTR_22             94
#define BCM_IRQ0_RSVD_INTR_23             95
#define BCM_IRQ_BSC_MASTERA_CPU_INTR      96
#define BCM_IRQ_BSC_MASTERB_CPU_INTR      97
#define BCM_IRQ_BSC_MASTERC_CPU_INTR      98
#define BCM_IRQ_BSC_MASTERD_CPU_INTR      99
#define BCM_IRQ0_RSVD_INTR_28             100 
#define BCM_IRQ0_RSVD_INTR_29             101
#define BCM_IRQ0_RSVD_INTR_30             102
#define BCM_IRQ0_RSVD_INTR_31             103

/*
 * The following block of logical interrupts are level 2 interrups
 * and are grouped together to correspond with register interfaces
 * HIF_INTR2_CPU_xx and HIF_INTR2_PCI_xx
 */
#define BCM_IRQ_HIF_RGR_BRIDGE_INTR       104
#define BCM_IRQ_PCI_RG_BRIDGE_INTR        105
#define BCM_IRQ_EBI_RG_BRIDGE_INTR        106
#define BCM_IRQ_PCI_ADR_PERR_INTR         107
#define BCM_IRQ_PCI_SERR_DET_INTR         108
#define BCM_IRQ_PCI_REC_MSTR_ABORT_INTR   109
#define BCM_IRQ_PCI_REC_TAR_ABORT_INTR    110
#define BCM_IRQ_PCI_REC_DATA_PERR_INTR    111
#define BCM_IRQ_PCI_SRC_DATA_PERR_INTR    112
#define BCM_IRQ_PCI_DMA_ERROR_INTR        113
#define BCM_IRQ_PCI_DMA_DONE_INTR         114
#define BCM_IRQ_PCI_RBUS_MSTR_ERR_INTR    115
#define BCM_HIF_INTR2_RSVD_INTR_12        116
#define BCM_IRQ_MEM_DMA_INTR              117
#define BCM_IRQ_EBI_DMA_RX_INTR           118
#define BCM_IRQ_EBI_DMA_TX_INTR           119
#define BCM_HIF_INTR2_RSVD_INTR_16        120 
#define BCM_HIF_INTR2_RSVD_INTR_17        121
#define BCM_HIF_INTR2_RSVD_INTR_18        122 
#define BCM_HIF_INTR2_RSVD_INTR_19        123 
#define BCM_HIF_INTR2_RSVD_INTR_20        124 
#define BCM_HIF_INTR2_RSVD_INTR_21        125 
#define BCM_HIF_INTR2_RSVD_INTR_22        126
#define BCM_HIF_INTR2_RSVD_INTR_23        127 
#define BCM_HIF_INTR2_RSVD_INTR_24        128 
#define BCM_HIF_INTR2_RSVD_INTR_25        129 
#define BCM_HIF_INTR2_RSVD_INTR_26        130 
#define BCM_HIF_INTR2_RSVD_INTR_27        131 
#define BCM_HIF_INTR2_RSVD_INTR_28        132 
#define BCM_HIF_INTR2_RSVD_INTR_29        133 
#define BCM_HIF_INTR2_RSVD_INTR_30        134 
#define BCM_HIF_INTR2_RSVD_INTR_31        135 

#define BCM_IRQ_SYSTIMER_INTR             136  // not used -- depricated

#define BCM_IRQ_3250_CPU_INTR             137
#define BCM_IRQ_7041_CPU_INTR             138

//138-210 reserved for future use (GIO, etc)

// Note: if you increase this, also increase NR_IRQS in <asm/irq.h>
#define BCM_NR_IRQS                       139

#ifndef _LANGUAGE_ASSEMBLY
#ifdef __KERNEL__
#include <linux/types.h>

#define __preg8		(volatile u8*)
#define __preg16	(volatile u16*)
#define __preg32	(volatile u32*)
#else /* !__KERNEL__ */
#define __preg8		(volatile unsigned char*)
#define __preg16	(volatile unsigned short*)
#define __preg32	(volatile unsigned int*)
#endif /* __KERNEL__ */
#else /* _LANGUAGE_ASSEMBLY */
#define __preg8
#define __preg16
#define __preg32
#endif /* !_LANGUAGE_ASSEMBLY */

#define BCM_SDRAM_PHYS		        0x00000000
#define BCM_SDRAM_BASE 		        (KSEG1 + BCM_SDRAM_PHYS)
#define BCM_SDRAM_SIZE		        0x08000000
#define BCM_REG_PHYS		        0x10000000
#define BCM_REG_BASE 		        (KSEG1 + BCM_REG_PHYS)

#define	BCM_LBUS_PHYS		        0x1c000000

#define	__PHYSREG(offset)	     (BCM_REG_PHYS + (offset))
#define __SYSREG(offset)	     __preg32(BCM_REG_BASE + 0 + (offset))

#define BCM_DEV_ID		     __SYSREG(0x200)		/* Device ID Register (R) */

#define BCM_INTR_WIN0_STATUS	     __SYSREG(0x1400)	/* Interrupt Status Register (R) */
#define BCM_INTR_WIN1_STATUS	     __SYSREG(0x1404)	/* Interrupt Status Register (R) */
#define BCM_INTR_WIN0_MASK_STATUS    __SYSREG(0x1408)	/* Interrupt Enable Register (W) */
#define BCM_INTR_WIN1_MASK_STATUS    __SYSREG(0x140c)	/* Interrupt Enable Register (W) */
#define BCM_INTR_WIN0_MASK_SET	     __SYSREG(0x1410)	/* Interrupt Enable Register (W) */
#define BCM_INTR_WIN1_MASK_SET	     __SYSREG(0x1414)	/* Interrupt Enable Register (W) */
#define BCM_INTR_WIN0_MASK_CLEAR     __SYSREG(0x1418)	/* Interrupt Clear Register (W) */
#define BCM_INTR_WIN1_MASK_CLEAR     __SYSREG(0x141c)	/* Interrupt Clear Register (W) */
#define BCM_INTR2_CPU_STATUS         __SYSREG(0x1000)
#define BCM_INTR2_CPU_SET            __SYSREG(0x1004)
#define BCM_INTR2_CPU_CLEAR          __SYSREG(0x1008)
#define BCM_INTR2_CPU_MASK_STATUS    __SYSREG(0x100C)
#define BCM_INTR2_CPU_MASK_SET       __SYSREG(0x1010)
#define BCM_INTR2_CPU_MASK_CLEAR     __SYSREG(0x1014)
#define BCM_INTR2_PCI_STATUS         __SYSREG(0x1018)
#define BCM_INTR2_PCI_SET            __SYSREG(0x101C)
#define BCM_INTR2_PCI_CLEAR          __SYSREG(0x1020)
#define BCM_INTR2_PCI_MASK_STATUS    __SYSREG(0x1024)
#define BCM_INTR2_PCI_MASK_SET       __SYSREG(0x1028)
#define BCM_INTR2_PCI_MASK_CLEAR     __SYSREG(0x102C)
#define BCM_INTR2_USB_CPU_STATUS     __SYSREG(0x4c0000)
#define BCM_INTR2_USB_CPU_SET        __SYSREG(0x4c0004)
#define BCM_INTR2_USB_CPU_CLEAR      __SYSREG(0x4c0008)
#define BCM_INTR2_USB_CPU_MASK_STATUS __SYSREG(0x4c000C)
#define BCM_INTR2_USB_CPU_MASK_SET   __SYSREG(0x4c0010)
#define BCM_INTR2_USB_CPU_MASK_CLEAR __SYSREG(0x4c0014)
#define BCM_INTR_IRQ0_ENABLE         __SYSREG(0x400780)
#define BCM_INTR_IRQ0_STATUS         __SYSREG(0x400784)
#define BCM_INTR_IRQ1_ENABLE         __SYSREG(0x400788)
#define BCM_INTR_IRQ1_STATUS         __SYSREG(0x40078C)


#define __TIMRREG(offset)	        __preg32(BCM_REG_BASE + 0x4007c0 + (offset))
#define BCM_TIMER_INTR_STATUS        __TIMRREG(0x0)
#define BCM_TIMER_INT0_ENABLE        __TIMRREG(0x4)
#define BCM_TIMER0_CNTL              __TIMRREG(0x08)
#define BCM_TIMER1_CNTL              __TIMRREG(0x0C)
#define BCM_TIMER2_CNTL              __TIMRREG(0x10)
#define BCM_TIMER3_CNTL              __TIMRREG(0x14)
#define BCM_TIMER0_STAT              __TIMRREG(0x18)
#define BCM_TIMER1_STAT              __TIMRREG(0x1C)
#define BCM_TIMER2_STAT              __TIMRREG(0x20)
#define BCM_TIMER3_STAT              __TIMRREG(0x24)

///////////////////////////////////////////////////////////////////////////////
//
// PCI definitions
//
///////////////////////////////////////////////////////////////////////////////
#define BCM_PCIIO_PHYS		        0x10520000
#define BCM_PCIIO_BASE 		        (KSEG1+BCM_PCIIO_PHYS)
#define BCM_PCIIO_SIZE		        0x00010000

#define BCM_PCIMEM_PHYS		        0xd0000000
#define BCM_PCIMEM_SIZE		        0x10000000
#define BCM_PCICFG_PHYS		        (BCM_REG_PHYS + 0x200)
#define BCM_PCICFG_BASE 	        (KSEG1 + BCM_PCICFG_PHYS)

#define BCM_PCI_BUS_NUM		        __SYSREG(0x104)		/* PCI Bus Number Register (R/W) */
#define BCM_PCI_DMA_CTRL	        __SYSREG(0x304)		/* PCI DMA Controller Register (R/W) */
#define BCM_PCI_DMA_CURRENT_DESC    __SYSREG(0x310) 	/* PCI DMA Current Descriptor Register (R/W) */
#define BCM_PCI_DMA_DESC_WRD0       __SYSREG(0x314)		/* PCI DMA Current Descriptor Word 0 Register (R/W) */
#define BCM_PCI_DMA_DESC_WRD1       __SYSREG(0x318)		/* PCI DMA Current Descriptor Word 1 Register (R/W) */
#define BCM_PCI_DMA_DESC_WRD2       __SYSREG(0x31c)		/* PCI DMA Current Descriptor Word 2 Register (R/W) */
#define BCM_PCI_DMA_CUR_MEM_BYTE_CNT __SYSREG(0x320)	/* PCI DMA Current Memory Byte Count Register (R/W) */

/* from PCI spec, Maybe we can put this in some include file. */
#define PCI_ENABLE                 0x80000000
#define PCI_IDSEL(x)		       (((x)&0x1f)<<11)
#define PCI_FNCSEL(x)		       (((x)&0x7)<<8)
#define PCI_IO_ENABLE              0x00000001
#define PCI_MEM_ENABLE             0x00000002
#define PCI_BUS_MASTER             0x00000004

#define OFFSET_16MBYTES            0x1000000
#define OFFSET_32MBYTES            0x2000000
#define TLBLO_OFFSET_32MBYTES      (0x2000000 >> 6)

//
//PCI controller 1
//
#define CPU2PCI_CPU_PHYS_MEM_WIN_BASE  0xd0000000
#define CPU2PCI_CPU_PHYS_MEM_WIN1_BASE 0xd8000000
#define CPU2PCI_CPU_PHYS_MEM_WIN2_BASE 0xe0000000
#define CPU2PCI_CPU_PHYS_MEM_WIN3_BASE 0xe8000000

#define CPU2PCI_CPU_PHYS_IO_WIN0_BASE  0x00000002
#define CPU2PCI_CPU_PHYS_IO_WIN1_BASE  0x00200002
#define CPU2PCI_CPU_PHYS_IO_WIN2_BASE  0x00400002

#define MIPS_PCI_XCFG_INDEX            0xf0600004
#define MIPS_PCI_XCFG_DATA             0xf0600008

#define __CFGREG8(offset)	        __preg8(BCM_PCICFG_BASE + (offset))
#define __CFGREG32(offset)    	    __preg32(BCM_PCICFG_BASE + (offset))

#define BCM_PCI_VENDOR_SELF	        __CFGREG32(0x0)
#define BCM_PCI_CMD_SELF        	__CFGREG32(0x4)
#define BCM_PCI_CLASS_CODE_REVID_SELF	__CFGREG32(0x8)
#define BCM_PCI_LAT_SELF        	__CFGREG32(0xc)
#define BCM_PCI_IO_BASE	        	__CFGREG32(0x10)	/* PCI BAR0 */
#define BCM_PCI_GISB_BASE        	__CFGREG32(0x10)	/* PCI BAR0 */
#define BCM_PCI_MEM_W0_BASE	        __CFGREG32(0x14)	/* PCI BAR1 */
#define BCM_PCI_MEM_W1_BASE	        __CFGREG32(0x18)	/* PCI BAR2 */
#define BCM_PCI_MEM_W2_BASE	        __CFGREG32(0x1c)	/* PCI BAR3 */
#define BCM_PCI_MEM_REMAP_W0        __CFGREG32(0x50)
#define BCM_PCI_MEM_REMAP_W1        __CFGREG32(0x54)
#define BCM_PCI_MEM_REMAP_W2        __CFGREG32(0x58)
#define BCM_PCI_MEM_REMAP_W3	    __CFGREG32(0x5c)
#define BCM_PCI_IO_REMAP_W0        	__CFGREG32(0x60)
#define BCM_PCI_IO_REMAP_W1        	__CFGREG32(0x64)
#define BCM_PCI_IO_REMAP_W2	        __CFGREG32(0x68)
#define BCM_PCI_SDRAM_ENDIAN_CTRL   __CFGREG32(0x6c)

#define BCM_PCI_MR_SWAP	            0x02	/* bit1: byte swap */
#define BCM_PCI_MR_ARB		        0x04	/* bit2: external arbiter enable */

/* PCI config space data (R/W) */
#define BCM_PCI_CF8		            __preg32(MIPS_PCI_XCFG_INDEX)	
/* PCI config space address (R?/W) */
#define BCM_PCI_CFC		            __preg32(MIPS_PCI_XCFG_DATA)	

/* Since the following is not defined in any of our header files. */
#define PCI_7401_PHYS_GISB_WIN_BASE 0x10000000
#define PCI_7401_PHYS_MEM_WIN_BASE  0x00000000

/*SATA IDs*/
#define BCM7401_SATA_VID	        0x02421166
#define TIVO_VIXS_PCI_ID	        0x21001745
#define NXP7164_PCI_ID                  0x71641131

#define PCI_DEVICE_ID_SATA		    0x00
#define PCI_DEVICE_ID_NXP7164       0x0d
#define PCI_DEVICE_ID_VIXS          0x0e

/*PCI Extension slot*/
#define PCI_DEVICE_ID_EXT           0x0f

//
//PCI controller 2 (SATA)
//
#define PCI_SATA_BRIDGE_BASE		    __SYSREG(0x500200)
#define __SATABRGDREG32(offset)		    __preg32(PCI_SATA_BRIDGE_BASE+ (offset))

#define	PCI_BRIDGE_PCI_CTRL		            __SATABRGDREG32(0x00)
#define	PCI_BRIDGE_SATA_CFG_INDEX	        __SATABRGDREG32(0x01)
#define	PCI_BRIDGE_SATA_CFG_DATA	        __SATABRGDREG32(0x02)
#define PCI_BRIDGE_SLV_MEM_WIN_BASE	        __SATABRGDREG32(0x03)
#define PCI_BRIDGE_CPU_TO_SATA_MEM_WIN_BASE __SATABRGDREG32(0x04)
#define PCI_BRIDGE_CPU_TO_SATA_IO_WIN_BASE  __SATABRGDREG32(0x05)

#define CPU2PCI_PCI_SATA_PHYS_MEM_WIN0_BASE __PHYSREG(0x510000)
#define CPU2PCI_PCI_SATA_PHYS_MEM_WIN0_SIZE 0x10000

#define SATA_MEM_BASE			            (BCM_REG_BASE+0x510000)
#define SATA_MEM_SIZE			            0x10000

#define SATA_MMIO_BASE                      SATA_MEM_BASE
#define SATA_MMIO_SIZE                      SATA_MEM_SIZE 

#define SATA_CFG_BASE			            (BCM_REG_BASE+0x520000)
#define __SATACFGREG8(offset)               __preg8(SATA_CFG_BASE+ (offset))

#define CPU2PCI_PCI_SATA_PHYS_IO_WIN0_BASE  __PHYSREG(0x520000)	

#define SATA_IO_BASE			            SATA_CFG_BASE
#define SATA_IO_SIZE			            0x10000

#define	SATA_CFG_VENDOR_ID		        __SATACFGREG8(0x00)
#define	SATA_CFG_DEVICE_ID		        __SATACFGREG8(0x02)
#define SATA_CFG_PCI_COMMAND		    __SATACFGREG8(0x04)
#define	SATA_CFG_PCI_STATUS		        __SATACFGREG8(0x06)
#define	SATA_CFG_REV_ID_CLASS_CODE	    __SATACFGREG8(0x08)
#define SATA_CFG_CACHELINE_SIZE		    __SATACFGREG8(0x0c)
#define SATA_CFG_MASTER_LATENCY_TIMER   __SATACFGREG8(0x0d)
#define SATA_CFG_HEADER_TYPE		    __SATACFGREG8(0x0e)
#define SATA_CFG_BIST			        __SATACFGREG8(0x0f)
#define SATA_CFG_PCS0			        __SATACFGREG8(0x10)
#define SATA_CFG_PCS1			        __SATACFGREG8(0x14)
#define SATA_CFG_SCS0			        __SATACFGREG8(0x18)
#define SATA_CFG_SCS1			        __SATACFGREG8(0x1c)
#define SATA_CFG_BUS_MASTER_BASE_ADDR	__SATACFGREG8(0x20)
#define SATA_CFG_MMIO_BASE_ADDR		    __SATACFGREG8(0x24)
#define	SATA_CFG_SUB_VENDOR_ID		    __SATACFGREG8(0x2c)
#define	SATA_CFG_SUB_DEVICE_ID		    __SATACFGREG8(0x2e)
#define SATA_CFG_INTERRUPT_LINE		    __SATACFGREG8(0x3c)
#define SATA_CFG_INTERRUPT_PIN		    __SATACFGREG8(0x3d)
#define SATA_CFG_MINIMUM_GRANT		    __SATACFGREG8(0x3e)
#define SATA_CFG_MINIMUM_LATENCY	    __SATACFGREG8(0x3f)

#define SATA_PCS0_OFS                   0x200
#define SATA_PCS1_OFS                   0x240
#define SATA_SCS0_OFS                   0x280
#define SATA_SCS1_OFS                   0x2c0
#define SATA_CFG_BUS_MASTER_OFS		    0x300

///////////////////////////////////////////////////////////////////////////////
//
// SUN TOP Control Registers
//
///////////////////////////////////////////////////////////////////////////////
#define BCM_SUN_TOP_CTRL_BASE		    __SYSREG(0x404000)
#define __SUNTOPREG32(offset)		    __preg32(BCM_SUN_TOP_CTRL_BASE+ (offset))

#define BCM_SUN_TOP_CTRL_PROD_REVISION  __SUNTOPREG32(0x00)
#define BCM_SUN_TOP_CTRL_RESET_CTRL	    __SUNTOPREG32(0x02)
#define	BCM_SUN_TOP_CTRL_SW_RESET	    __SUNTOPREG32(0x05)
#define	BCM_SUN_TOP_CTRL_STRAP_VALUE	__SUNTOPREG32(0x07)

#define	BCM_SUN_TOP_CTRL_SEMA_0		    __SUNTOPREG32(0x0c)
#define	BCM_SUN_TOP_CTRL_SEMA_1		    __SUNTOPREG32(0x0d)
#define	BCM_SUN_TOP_CTRL_SEMA_2		    __SUNTOPREG32(0x0e)
#define	BCM_SUN_TOP_CTRL_SEMA_3		    __SUNTOPREG32(0x0f)
#define	BCM_SUN_TOP_CTRL_SEMA_4		    __SUNTOPREG32(0x10)
#define	BCM_SUN_TOP_CTRL_SEMA_5		    __SUNTOPREG32(0x11)
#define	BCM_SUN_TOP_CTRL_SEMA_6		    __SUNTOPREG32(0x12)
#define	BCM_SUN_TOP_CTRL_SEMA_7		    __SUNTOPREG32(0x13)
#define	BCM_SUN_TOP_CTRL_SEMA_8		    __SUNTOPREG32(0x14)
#define	BCM_SUN_TOP_CTRL_SEMA_9		    __SUNTOPREG32(0x15)
#define	BCM_SUN_TOP_CTRL_SEMA_10	    __SUNTOPREG32(0x16)
#define	BCM_SUN_TOP_CTRL_SEMA_11	    __SUNTOPREG32(0x17)
#define	BCM_SUN_TOP_CTRL_SEMA_12	    __SUNTOPREG32(0x18)
#define	BCM_SUN_TOP_CTRL_SEMA_13	    __SUNTOPREG32(0x19)
#define	BCM_SUN_TOP_CTRL_SEMA_14	    __SUNTOPREG32(0x1a)
#define	BCM_SUN_TOP_CTRL_SEMA_15	    __SUNTOPREG32(0x1b)

#define BCM_SUN_TOP_CTRL_GEN_WATCHDOG_0 __SUNTOPREG32(0x1c)
#define BCM_SUN_TOP_CTRL_GEN_WATCHDOG_1 __SUNTOPREG32(0x1d)
#define	BCM_SUN_TOP_CTRL_SATA_CTRL	    __SUNTOPREG32(0x1e)
#define BCM_SUN_TOP_CTRL_DAA_CTRL       __SUNTOPREG32(0x1f)
#define	BCM_SUN_TOP_CTRL_SPARE_CTRL	    __SUNTOPREG32(0x21)

#define	BCM_SUN_TOP_CTRL_PIN_MUX_CRTL_0	    __SUNTOPREG32(0x25)
#define	BCM_SUN_TOP_CTRL_PIN_MUX_CRTL_1	    __SUNTOPREG32(0x26)
#define	BCM_SUN_TOP_CTRL_PIN_MUX_CRTL_2	    __SUNTOPREG32(0x27)
#define	BCM_SUN_TOP_CTRL_PIN_MUX_CRTL_3	    __SUNTOPREG32(0x28)
#define	BCM_SUN_TOP_CTRL_PIN_MUX_CRTL_4	    __SUNTOPREG32(0x29)
#define	BCM_SUN_TOP_CTRL_PIN_MUX_CRTL_5	    __SUNTOPREG32(0x2a)
#define	BCM_SUN_TOP_CTRL_PIN_MUX_CRTL_6	    __SUNTOPREG32(0x2b)
#define	BCM_SUN_TOP_CTRL_PIN_MUX_CRTL_7	    __SUNTOPREG32(0x2c)
#define	BCM_SUN_TOP_CTRL_PIN_MUX_CRTL_8	    __SUNTOPREG32(0x2d)
#define	BCM_SUN_TOP_CTRL_PIN_MUX_CRTL_9	    __SUNTOPREG32(0x2e)
#define	BCM_SUN_TOP_CTRL_PIN_MUX_CRTL_10    __SUNTOPREG32(0x2f)
#define	BCM_SUN_TOP_CTRL_PIN_MUX_CRTL_11    __SUNTOPREG32(0x30)

#define BCM_SUN_TOP_CTRL_RESET_PCI      0x2
#define BCM_SUN_TOP_CTRL_ENABLE_ALL     0x0

///////////////////////////////////////////////////////////////////////////////
//
// UART
//
///////////////////////////////////////////////////////////////////////////////

#define XTALFREQ                27000000
/*FIXME: Remove CONFIG_MIPS_BRCM974XX*/
#ifdef CONFIG_MIPS_BRCM974XX

#define BCM_UARTA_BASE          (BCM_REG_BASE + 0x400180)
#define BCM_UARTB_BASE          (BCM_REG_BASE + 0x4001A0)

#else /*CONFIG_MIPS_BRCM974XX*/

#define BCM_UARTB_BASE          (BCM_REG_BASE + 0x400180)
#define BCM_UARTA_BASE          (BCM_REG_BASE + 0x4001A0)

#endif /*CONFIG_MIPS_BRCM974XX*/

#define BCM_UART_RCVSTAT        0x00
#define BCM_UART_RCVDATA        0x01
#define BCM_UART_CONTROL        0x03
#define BCM_UART_BAUDHI         0x04
#define BCM_UART_BAUDLO         0x05
#define BCM_UART_XMTSTAT        0x06
#define BCM_UART_XMTDATA        0x07

#define RXEN                    0x2
#define TXEN                    0x4
#define BITM8                   0x10
#define TXDRE                   0x1
#define RXRDA                   0x4
#define RXOVFERR                0x8
#define RXFRAMERR               0x10
#define RXPARERR                0x20

///////////////////////////////////////////////////////////////////////////////
//
// IDE
//
///////////////////////////////////////////////////////////////////////////////
#define BCM_IDE_BASE_7xxx       0xF0000000

#define __IDEREG8(offset)       __preg8(BCM_IDE_BASE_7xxx + offset)
#define __IDEREG16(offset)      __preg16(BCM_IDE_BASE_7xxx + offset)
#define __IDEREG32(offset)      __preg32(BCM_IDE_BASE_7xxx + offset)

#define BCM_IDE_CMD             __IDEREG32(0x004)
#define BCM_IDE_PROG_IF         __IDEREG32(0x008)
#define BCM_IDE_PRI_CS0_ADDR    __IDEREG32(0x010)
#define BCM_IDE_PRI_CS1_ADDR    __IDEREG32(0x014)
#define BCM_IDE_SEC_CS0_ADDR    __IDEREG32(0x018)
#define BCM_IDE_SEC_CS1_ADDR    __IDEREG32(0x01c)
#define BCM_IDE_BMIDE_ADDR      __IDEREG32(0x020)
#define BCM_IDE_PIO_TIMING      __IDEREG32(0x040)
#define BCM_IDE_DMA_TIMING      __IDEREG32(0x044)
#define BCM_IDE_PIO_CONTROL     __IDEREG32(0x048)
#define BCM_IDE_UDMA_CONTROL    __IDEREG32(0x054)
#define BCM_IDE_UDMA_MODE       __IDEREG32(0x058)
#define BCM_IDE_IF_CONTROL      __IDEREG32(0x064)
#define BCM_IDE_MBIST_CONTROL   __IDEREG32(0x068)
#define BCM_IDE_INT_MASK        __IDEREG32(0x080)
#define BCM_IDE_INT_STATUS      __IDEREG32(0x084)
#define BCM_IDE_PRI_DATA        __IDEREG16(0x200)
#define BCM_IDE_PRI_CMD_STAT    __IDEREG8(0x242)
#define BCM_IDE_PRI_BM_CMD_STAT __IDEREG32(0x300)
#define BCM_IDE_PRI_BM_DESC_TAB __IDEREG32(0x304)
#define BCM_IDE_SEC_BM_CMD_STAT __IDEREG32(0x308)
#define BCM_IDE_SEC_BM_DESC_TAB __IDEREG32(0x30c)


///////////////////////////////////////////////////////////////////////////////
//
// CACHES
//
///////////////////////////////////////////////////////////////////////////////
#define ICACHE_SIZE                     0x8000
#define ICACHE_LINESIZE                 0x0010
#define DCACHE_SIZE                     0x8000
#define DCACHE_LINESIZE                 0x0010

#endif /* __ASM_BCM7401_H */
