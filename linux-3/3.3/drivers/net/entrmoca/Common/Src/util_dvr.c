/*******************************************************************************
*
* Common/Src/util_dvr.c
*
* Description: Ethernet Driver Utility Functions
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

/*******************************************************************************
*                            # I n c l u d e s                                 *
********************************************************************************/

#include "drv_hdr.h"

/*******************************************************************************
*                            P R O T O T Y P E S                               *
********************************************************************************/

#if defined(PCI_DRVR_SUPPORT)
static volatile SYS_UINT32 *setup_atrans( dc_context_t *dccp, SYS_UINT32 addr);
#endif


#if defined(PCI_DRVR_SUPPORT)
/**
*   Purpose:    Messes with the addresses
*
*   Imports:    dccp - pointer to the driver control context
*               addr - address of the register/location
*                      this will be translated
*               val  - pointer to place to put the data
*
*   Exports:    
*
*STATIC*******************************************************************************/
static volatile SYS_UINT32 *setup_atrans(dc_context_t *dccp, SYS_UINT32 addr)
{
    SYS_UINTPTR base = dccp->baseAddr;
    volatile SYS_UINT32 *dst;
    SYS_UINT32 addr_hi = (addr & 0x0fff0000);
    SYS_UINT32 addr_lo = (addr & 0x0000ffff);

#if ECFG_CHIP_ZIP1
    if(((addr & 0xffff0000) == 0x30000) || (addr_hi == 0x100000))
    {
        /* this is in the CSR block (Sonics 0x0010xxxx), so use SLV2MAR */
        dst = (volatile SYS_UINT32 *)(base + (addr_lo | 0x30000));
    } else {
        /*
         * not in the CSR block - configure and use SLV1MAR
         * note: bits 16 and 17 should never be set at the same time
         * it is unclear whether bit 17 will ever be set by the host driver
         */
        //SETUP_ATRANS(ctx, &addr);
        dst = (volatile SYS_UINT32 *)(base + CLNK_REG_SLAVE_1_MAP_ADDR);
        HostOS_Write_Word((addr & 0x0ffc0000) | CLNK_REG_ADDR_TRANS_ENABLE_BIT,
                          (SYS_UINT32 *)dst);
        dst = (volatile SYS_UINT32 *)(base + (addr & 0x3ffff));
    }
#else  /* ECFG_CHIP_ZIP2/ECFG_CHIP_MAVERICKS */
    if ((addr >= EHI_START) && (addr <= EHI_END))
    {
        dst = (volatile SYS_UINT32 *)(base + 0x00000 + addr_lo);
    } else if (addr_hi == AT1_BASE) {
        dst = (volatile SYS_UINT32 *)(base + 0x10000 + addr_lo);
    } else if (addr_hi == AT2_BASE) {
        dst = (volatile SYS_UINT32 *)(base + 0x20000 + addr_lo);
    } else {
        if (addr_hi != dccp->at3_base)
        {
            dst = (volatile SYS_UINT32 *)(base + 0x00074);

            clnk_bus_write( dccp, (SYS_UINTPTR)dst, addr_hi);

            dccp->at3_base = addr_hi;
        }
        dst = (volatile SYS_UINT32 *)(base + 0x30000 + addr_lo);
    }
#endif /* ECFG_CHIP_ZIP1 */

    return(dst);
}
#endif

/**
*   Purpose:    Reads a clink bus register/location
*               The register/location might be on the PCI bus
*               or it might be accessed via MDIO
*
*               Takes the Address Translation lock
*
*   Imports:    vcp  - pointer to the driver control context
*               addr - address of the register/location
*                      this will be translated
*               val  - pointer to place to put the data
*
*   Exports:    
*
*PUBLIC*******************************************************************************/
void clnk_reg_read(void *vcp, SYS_UINT32 addr, SYS_UINT32 *val)
{
    dc_context_t *dccp = (dc_context_t *)vcp ;
#if defined(PCI_DRVR_SUPPORT)
    volatile SYS_UINT32 *src;
#endif
    HostOS_Lock(dccp->at_lock_link);
//HostOS_PrintLog(L_INFO, "In clnk_reg_read: address=%x \n",addr);
#if defined(PCI_DRVR_SUPPORT)
    src = setup_atrans(dccp, addr);
    clnk_bus_read( dccp, (SYS_UINTPTR)src, val);
#elif defined(E1000_DRVR_SUPPORT) || defined(CANDD_DRVR_SUPPORT) || defined(CONFIG_TIVO)
    clnk_bus_read( dccp, (SYS_UINTPTR)addr, val);
#endif
//HostOS_PrintLog(L_INFO, "Done clnk_reg_read: val=%x \n",*val);

    HostOS_Unlock(dccp->at_lock_link);
}

/**
*   Purpose:    Writes a clink bus register/location
*               The register/location might be on the PCI bus
*               or it might be accessed via MDIO
*
*               Takes the Address Translation lock
*
*   Imports:    vcp  - pointer to the driver control context
*               addr - address of the register/location
*                      this will be translated
*               val  - the data to write
*
*   Exports:    
*
*PUBLIC*******************************************************************************/
void clnk_reg_write(void *vcp, SYS_UINT32 addr, SYS_UINT32 val)
{
    dc_context_t *dccp = (dc_context_t *)vcp ;
#if defined(PCI_DRVR_SUPPORT)
    volatile SYS_UINT32 *dst;
#endif

    HostOS_Lock(dccp->at_lock_link);
#if defined(PCI_DRVR_SUPPORT)
    dst = setup_atrans(dccp, addr);
    clnk_bus_write( dccp, (SYS_UINTPTR)dst, val);
#elif defined(E1000_DRVR_SUPPORT) || defined(CANDD_DRVR_SUPPORT) || defined(CONFIG_TIVO)
    clnk_bus_write( dccp, (SYS_UINTPTR)addr, val);
#endif
    HostOS_Unlock(dccp->at_lock_link);
}

/**
*
*   Purpose:    Writes a clink bus register/location
*                 WITHOUT LOCKING
*               This is in support of a block write operation
*               and the lock is outside the write loop.
*
*               The register/location might be on the PCI bus
*               or it might be accessed via MDIO
*
*               Take the Address Translation lock before calling
*
*   Imports:    vcp  - pointer to the driver control context
*               addr - address of the register/location
*                      this will be translated
*               val  - the data to write
*
*   Exports:    
*
*PUBLIC*******************************************************************************/
void clnk_reg_write_nl(void *vcp, SYS_UINT32 addr, SYS_UINT32 val)
{
    dc_context_t *dccp = (dc_context_t *)vcp ;

#if defined(PCI_DRVR_SUPPORT)
    volatile SYS_UINT32 *dst;
    dst = setup_atrans(dccp, addr);
    clnk_bus_write( dccp, (SYS_UINTPTR)dst, val);
#elif defined(E1000_DRVR_SUPPORT) || defined(CANDD_DRVR_SUPPORT) || defined(CONFIG_TIVO)
    clnk_bus_write( dccp, (SYS_UINTPTR)addr, val);
#endif

}

/**
*   Purpose:    Reads a clink bus register/location
*                 WITHOUT LOCKING
*               The register/location might be on the PCI bus
*               or it might be accessed via MDIO
*
*               Take the Address Translation lock before calling
*
*   Imports:    vcp  - pointer to the driver control context
*               addr - address of the register/location
*                      this will be translated
*               val  - pointer to place to put the data
*
*   Exports:    
*
*PUBLIC*******************************************************************************/
void clnk_reg_read_nl(void *vcp, SYS_UINT32 addr, SYS_UINT32 *val)
{
    dc_context_t *dccp = (dc_context_t *)vcp ;
#if defined(PCI_DRVR_SUPPORT)
    volatile SYS_UINT32 *src;
    src = setup_atrans(dccp, addr);
    clnk_bus_read( dccp, (SYS_UINTPTR)src, val);
#elif defined(E1000_DRVR_SUPPORT) || defined(CANDD_DRVR_SUPPORT) || defined(CONFIG_TIVO)
    clnk_bus_read( dccp, (SYS_UINTPTR)addr, val);
#endif
}


