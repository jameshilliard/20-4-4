/*******************************************************************************
*
* PCI/Src/ClnkBusPCI2_dvr.c
*
* Description: Zip2 PCI support
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


void socInitBus(dc_context_t *dccp)
{
    SYS_UINT32 regVal;
    void *ddcp = dc_to_dd( dccp ) ;

    // Reset the SOC
    HostOS_ReadPciConfig( ddcp, 0x48, &regVal);
    regVal |= 0x00000002;
    HostOS_WritePciConfig(ddcp, 0x48, regVal);

    HostOS_ReadPciConfig( ddcp, 0x48, &regVal);
    regVal &= (~0x00000002);
    HostOS_WritePciConfig(ddcp, 0x48, regVal);

    HostOS_ReadPciConfig( ddcp, 0x48, &regVal);

}

void socEnableInterrupt(dc_context_t *dccp)
{
#if ECFG_CHIP_MAVERICKS    
    SYS_UINT32 val;
#endif
    //HostOS_PrintLog(L_NOTICE, "socEnableInterrupt\n" );
    clnk_reg_write(dccp, EHI_INTR_OUT_MASK, HOST_INTR | TMR_INTR);
#if ECFG_CHIP_MAVERICKS    
    //Configure the PCIE code, setup the interrupt mode.
    clnk_reg_read( dccp, PCIE_CSC_CORE_CTL, &val);
    clnk_reg_write(dccp, PCIE_CSC_CORE_CTL, val | HOST_INTR_TO_INTX);    
#endif    
}

void socDisableInterrupt(dc_context_t *dccp)
{
    //HostOS_PrintLog(L_NOTICE, "socDisableInterrupt\n" );
    clnk_reg_write(dccp, EHI_INTR_OUT_MASK, 0);
}

void socClearInterrupt(dc_context_t *dccp)
{
    SYS_UINT32 val;

    clnk_reg_read(dccp, CSC_INTR_BUS, &val);
    if(val & TMR_INTR)
        dccp->timer_intr = 1;
    clnk_reg_write(dccp, CSC_INTR_BUS, val & (HOST_INTR | TMR_INTR));
#if (defined(CONFIG_ARCH_IXP425) || defined(CONFIG_ARCH_IXP4XX))
    // Read back value so calling routine knows previous write is complete.
    // (Only known to be necessary on ECB2 where the CPU interrupt cant be
    //  cleared until after the Chip withdraws the interrupt signal).
    clnk_reg_read(dccp, CSC_INTR_BUS, &val);
#endif
}

void socRestartRxDma(dc_context_t *dccp, SYS_UINT32 listNum)
{
}

void socFixHaltRxDma(dc_context_t *dccp, SYS_UINT32 listNum)
{
}

void socProgramRxDma(dc_context_t *dccp, SYS_UINT32 listNum, RxListEntry_t* pEntry)
{
}

void socFixHaltTxDma(dc_context_t *dccp, SYS_UINT32 fifoNum)
{
}

void socProgramTxDma(dc_context_t *dccp, SYS_UINT32 fifoNum)
{
}

void socInterruptTxPreProcess(dc_context_t *dccp)
{
}

void socInterruptRxPreProcess(dc_context_t *dccp)
{
}

