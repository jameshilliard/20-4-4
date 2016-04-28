/*******************************************************************************
*
* Src/ClnkBus68K_dvr.c
*
* Description: Zip1b 68K support
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
// this file only be include in ClinkEth.c
// so no include file here

/*******************************************************************************
*                             # D e f i n e s                                  *
********************************************************************************/

#define DMA_CHAPERONE    0  // Motorola define


/*******************************************************************************
*                             D a t a   T y p e s                              *
********************************************************************************/

/*******************************************************************************
*                             C o n s t a n t s                                *
********************************************************************************/

/* None */

/*******************************************************************************
*                             G l o b a l   D a t a                            *
********************************************************************************/

/* None */
/*******************************************************************************
*                       M e t h o d   P r o t o t y p e s                      *
********************************************************************************/


/*******************************************************************************
*                      M e t h o d   D e f i n i t i o n s                     *
********************************************************************************/


/*
@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
@                                                                              @
@                        P u b l i c  M e t h o d s                            @
@                                                                              @
@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
*/


/*******************************************************************************
*
* Public method:       socInitBus()
*
********************************************************************************
*
* Description:
*       Initializes the bus.
*
* Inputs:
*       void*  Pointer to the context
*
* Outputs:
*       None
*
* Notes:
*       None
*
*
********************************************************************************/
void socInitBus(dc_context_t *dccp)
{
    void socInitBusEnd(dc_context_t *dccp); 
    void activateEntropicReset(void); 

    // Reset the SOC
    activateEntropicReset();
    
    dccp->p68kRxPending = SYS_NULL; // keep receive host descriptor in DMA, prevent reentry
    dccp->p68kTxPending = SYS_NULL; // keep transmit host descriptor in DMA, prevent reentry
    dccp->Int68kStatus = 0;     // keep the interrupt for 68k
    dccp->tx68kCurListNum = 0;  // keep the track of used txList for 68k DMA
    dccp->rx68kCurListNum = 0;  // keep the track of used rxList for 68K DMA

    socInitBusEnd(dccp); 
}

void socInitBusEnd(dc_context_t *dccp)
{
    // Reset the SOC
    SYS_UINT32 temp;
    clnk_reg_write_nl(dccp, CLNK_REG_SLAVE_1_MAP_ADDR, 0x00400001);
    clnk_reg_write_nl(dccp, CLNK_REG_SLAVE_2_MAP_ADDR, 0x00100001);
    //Set PCI interrupt sign to test PIN
    clnk_reg_read_nl(dccp, 0x3704c,&temp);
    clnk_reg_write_nl(dccp, 0x3704c, (temp & ~(0x1f<<5))|(0x8<<5));
    clnk_reg_write_nl(dccp, 0x30400,0x44);
    clnk_reg_write_nl(dccp,TX_HOST_DESCRIPTOR_NXT, 0xffffffff); //TX descriptor Next Descriptor Pointer
    clnk_reg_write_nl(dccp,TX_HOST_DESCRIPTOR_FRG, 0x0);
    clnk_reg_write_nl(dccp,TX_HOST_DESCRIPTOR_CTL, 0x0);        //TX descriptor Status/Control
    clnk_reg_write_nl(dccp,TX_HOST_DESCRIPTOR_PTR, TX_FIFO);    //TX descriptor Fragment 1 Pointer
    clnk_reg_write_nl(dccp,TX_HOST_DESCRIPTOR_LEN, 0x0);


    clnk_reg_write_nl(dccp,RX_HOST_DESCRIPTOR_NXT, RX_HOST_DESCRIPTOR_NXT);
    clnk_reg_write_nl(dccp,RX_HOST_DESCRIPTOR_CTL, 0x3);        //TX descriptor Status/Control
    clnk_reg_write_nl(dccp,RX_HOST_DESCRIPTOR_FRG, 0x10800);
    clnk_reg_write_nl(dccp,RX_HOST_DESCRIPTOR_PTR, RX_FIFO);    //TX descriptor Fragment 1 Pointer
    clnk_reg_write_nl(dccp,RX_HOST_DESCRIPTOR_LEN, 0x800);
    // Initiate RX
    clnk_reg_write_nl(dccp,RCVL1HDESCADDRREG, RX_HOST_DESCRIPTOR_NXT);
    clnk_reg_write_nl(dccp,RCVL1FRAMELTHREG, 0x00000100);
    clnk_reg_write_nl(dccp,RCVTHREADPTRREG, 0x00000000);
    clnk_reg_write_nl(dccp,RCVL1STCTLREG, 0x00000001);
    clnk_reg_write_nl(dccp,RCVHSTCTLREG, 0x00000200);
    clnk_reg_write_nl(dccp,RCVSTCTLREG, 0x00000000);
}

/*******************************************************************************
*
* Public method:       socEnableInterrupt
*
********************************************************************************
*
* Description:
*       Enable SOC interrupt
*
* Notes:
*       None
*
*
********************************************************************************/
void socEnableInterrupt(dc_context_t* dccp)
{
    void EntropicIrqEnable(void);
    SYS_UINT32     regVal;
            
    EntropicIrqEnable();

    // Enable 68k bus interrupt for TX and RX DMA done
    clnk_reg_read_nl(dccp, CLNK_REG_68K_INT_MASK, &regVal);
    regVal &= ~(CLNK_REG_INTERRUPT_FRAM_RECEIVED |
                CLNK_REG_INTERRUPT_FRAM_RECEIVE_COMPLETE  |
                CLNK_REG_INTERRUPT_FRAM_TRANSMITED |
                CLNK_REG_INTERRUPT_FRAM_DATA_ERROR );
    clnk_reg_write_nl(dccp, CLNK_REG_68K_INT_MASK, regVal);

    // Enable mailbox interrupts
    Clnk_MBX_Control(&dccp->mailbox, CLNK_MBX_CTRL_ENABLE_INTERRUPT, 0, 0, 0);

    // Enable PCI interrupt for Ethernet interface
    clnk_reg_read_nl(dccp, CLNK_REG_PCI_INT_ENABLE, &regVal);
    regVal |= (CLNK_REG_PCI_MASTER_INT_ENABLE_BIT |
              // CLNK_REG_PCI_ETH_IF_INT_ENABLE_BIT | //- not necessary
              CLNK_REG_PCI_INT_ENABLE_BIT);
    clnk_reg_write_nl(dccp, CLNK_REG_PCI_INT_ENABLE, regVal);

    if (DMA_CHAPERONE)
    {
        // Enable NDIS interrupt for TX and RX DMA done
        clnk_reg_read_nl(dccp, CLNK_REG_PCI_INT_MASK, &regVal);

        regVal |= CLNK_REG_PCI_SW_INT_2_BIT;  /* RX interrupt */
        clnk_reg_write_nl(dccp, CLNK_REG_PCI_INT_MASK, regVal);
    }
}

/*******************************************************************************
*
* Public method:       socDisableInterrupt
*
********************************************************************************
*
* Description:
*       Disable SOC interrupt
*
* Notes:
*       None
*
*
********************************************************************************/
void socDisableInterrupt(dc_context_t* dccp)
{
    void EntropicIrqDisable(void); 
    void EntropicIrqClear(void); 
    SYS_UINT32 regVal;

    // Disable NDIS interrupt for TX and RX DMA done
    clnk_reg_read_nl(dccp, CLNK_REG_PCI_INT_MASK, &regVal);
    regVal &= ~(CLNK_REG_PCI_TX_PRI1_DMA_DONE_BIT |
                CLNK_REG_PCI_TX_PRI2_DMA_DONE_BIT |
                CLNK_REG_PCI_TX_PRI3_DMA_DONE_BIT |
                CLNK_REG_PCI_TX_PRI4_DMA_DONE_BIT |
                CLNK_REG_PCI_TX_PRI5_DMA_DONE_BIT |
                CLNK_REG_PCI_TX_PRI6_DMA_DONE_BIT |
                CLNK_REG_PCI_TX_PRI7_DMA_DONE_BIT |
                CLNK_REG_PCI_TX_PRI8_DMA_DONE_BIT |
                CLNK_REG_PCI_RX_DMA_DONE_BIT      |
                CLNK_REG_PCI_SW_INT_2_BIT         |
                CLNK_REG_PCI_TMR_2_ROLLOVER_BIT   |
                CLNK_REG_PCI_TX_DMA_HALTED_BIT    |
                CLNK_REG_PCI_RX_DMA_HALTED_BIT);
    clnk_reg_write_nl(dccp, CLNK_REG_PCI_INT_MASK, regVal);

    // Disable mailbox interrupts
    Clnk_MBX_Control(&dccp->mailbox, CLNK_MBX_CTRL_DISABLE_INTERRUPT, 0, 0, 0);
 
    // Disable PCI interrupt for Ethernet interface
    clnk_reg_read_nl(dccp, CLNK_REG_PCI_INT_ENABLE, &regVal);
    regVal &= ~(CLNK_REG_PCI_ETH_IF_INT_ENABLE_BIT |
                        CLNK_REG_PCI_INT_ENABLE_BIT);
    clnk_reg_write_nl(dccp, CLNK_REG_PCI_INT_ENABLE, regVal);

    // Disable 68k bus interrupt for TX and RX DMA done
    clnk_reg_read_nl(dccp, CLNK_REG_68K_INT_MASK, &regVal);
    regVal |= (CLNK_REG_INTERRUPT_FRAM_RECEIVED |
               CLNK_REG_INTERRUPT_FRAM_RECEIVE_COMPLETE  |
               CLNK_REG_INTERRUPT_FRAM_TRANSMITED |
               CLNK_REG_INTERRUPT_FRAM_DATA_ERROR );
    clnk_reg_write_nl(dccp, CLNK_REG_68K_INT_MASK, regVal);
    
    // Be sure they are really stopped
    clnk_reg_read_nl(dccp, CLNK_REG_PCI_INT_ENABLE, &regVal);
    clnk_reg_read_nl(dccp, CLNK_REG_68K_INT_MASK, &regVal);

    EntropicIrqDisable();
    EntropicIrqClear();
}

/*******************************************************************************
*
* Public method:       socClearInterrupt
*
********************************************************************************
*
* Description:
*       clear SOC interrupt
*
* Notes:
*       None
*
*
********************************************************************************/
void socSet68kStatus(dc_context_t *dccp, SYS_UINT32 regVal)
{
    dccp->Int68kStatus |= regVal;
}

void socSetPciStatus(dc_context_t *dccp, SYS_UINT32 regVal)
{
    dccp->pciIntStatus = regVal;
}

void socClearInterrupt(dc_context_t *dccp)
{
    SYS_UINT32 regVal;

    clnk_reg_read_nl(dccp, CLNK_REG_68K_INT_STATUS, &regVal);
    dccp->Int68kStatus |= regVal;
    // acknowledge 68K interrupt sources
    if (regVal)
    {
        clnk_reg_write_nl(dccp, CLNK_REG_68K_INT_REG, regVal);
        clnk_reg_write_nl(dccp, CLNK_REG_68K_INT_REG, 0);

        clnk_reg_read_nl(dccp, CLNK_REG_68K_INT_STATUS, &regVal);
        if (regVal)
            HostOS_PrintLog(L_DEBUG, "can't clear 68k INT!\n\r");
    }

    /* get PCI interrupt status */
    clnk_reg_read_nl(dccp, CLNK_REG_PCI_INT_REG, &regVal);

    // Clear PCI interrupt
    // Always check MBX, to avoid race condition on pciIntStatus between
    // interrupt level and task level (slight overhead in MBX process).
    dccp->pciIntStatus = regVal |
                         (CLNK_REG_PCI_WRITE_MBX_SEM_CLR_BIT |  /* mailbox */
                         CLNK_REG_PCI_READ_MBX_SEM_SET_BIT |
                         CLNK_REG_PCI_SW_INT_1_BIT);

    clnk_reg_write_nl(dccp, CLNK_REG_PCI_INT_REG, regVal);

    // Read back value so calling routine knows this is complete
    clnk_reg_read_nl(dccp, CLNK_REG_PCI_INT_REG, &regVal);
}

/*******************************************************************************
*
* Public method:       socRestartRxDma()
*
********************************************************************************
*
* Description:
*       Restart an idle Rx dma queue 
*
* Notes:
*       NDIS dma engine not used.
*
********************************************************************************/
void socRestartRxDma(dc_context_t *dccp, SYS_UINT32 listNum)
{
}

/*******************************************************************************
*
* Public method:       socFixHaltRxDma()
*
********************************************************************************
*
* Description:
*       Fix an Hung Rx dma queue - example: after pci_abort
*
* Notes:
*       NDIS dma engine not used.
*
********************************************************************************/
void socFixHaltRxDma(dc_context_t *dccp, SYS_UINT32 listNum)
{
    dccp->stats.rxListHaltErr[listNum]++;
}

/*******************************************************************************
*
* Public method:       socProgramRxDma()
*
********************************************************************************
*
* Description:
*       start rx dma 
*
* Notes:
*       None
*
*
********************************************************************************/
void socProgramRxDma(dc_context_t *dccp, SYS_UINT32 listNum, RxListEntry_t* pEntry)
{
    // Initiate RX
    clnk_reg_write_nl(dccp, RCVL1HDESCADDRREG, RX_HOST_DESCRIPTOR_NXT);
    clnk_reg_write_nl(dccp, RCVL1FRAMELTHREG, 0x00000100);
    clnk_reg_write_nl(dccp, RCVTHREADPTRREG, 0x00000000);
    clnk_reg_write_nl(dccp, RCVL1STCTLREG, 0x00000001);
    clnk_reg_write_nl(dccp, RCVHSTCTLREG, 0x00000200);
    clnk_reg_write_nl(dccp, RCVSTCTLREG, 0x00000000);
}

/*******************************************************************************
*
* Public method:       socFixHaltTxDma()
*
********************************************************************************
*
* Description:
*       Attempt to fix PCI TX DMA Q which is reported to be halted by error.
*
* Notes:
*       NDIS engine not used - SOC can't get bus faults.
*
********************************************************************************/
void socFixHaltTxDma(dc_context_t *dccp, SYS_UINT32 fifoNum)
{
    dccp->stats.txFifoHaltErr[fifoNum]++;
}

/*******************************************************************************
*
* Public method:       socProgramTxDma()
*
********************************************************************************
*
* Description:
*       Restart an idle tx dma queue 
*
* Notes:
*       None
*
*
********************************************************************************/
void socProgramTxDma(dc_context_t *dccp, SYS_UINT32 fifoNum)
{
    SYS_UINT32 i,nCount,nLength,txWord;
    ListHeader_t  *pHwFifo = &dccp->txList[fifoNum]; 
    TxListEntry_t *pFirstEntry = COMMON_LIST_HEAD(TxListEntry_t, pHwFifo);
    TxHostDesc_t *pTxHostDesc;
    char *txWp, *fragp;

    if(pFirstEntry == SYS_NULL)
        return;
    pTxHostDesc = pFirstEntry->pTxHostDesc;

    // Check if the previous TX done - if not save the current TX! 
    if(dccp->p68kTxPending)
    {
        HostOS_PrintLog(L_DEBUG, "\n\r\n\rXMIT TxPending\n\r\n\r");
        return;
    }
    dccp->p68kTxPending=pTxHostDesc;

    /* get fragment count and frame length */
    nCount =  pTxHostDesc->fragmentLen>>16;
    if (nCount != 1) HostOS_PrintLog(L_DEBUG, "Tx fragmentLen !=1 (%d)\n\r", nCount);
    nLength = (pTxHostDesc->fragmentLen) & 0xfffc;
    if(nLength < ((pTxHostDesc->fragmentLen) & 0xffff))
       nLength+=4;
    // copy the packet size from software TXD to 68k interface TXD location
    clnk_reg_write_nl(dccp,CLNK_REG_68K_INT_MASK,0);
    clnk_reg_write_nl(dccp,TX_HOST_DESCRIPTOR_NXT, 0xffffffff);
    clnk_reg_write_nl(dccp,TX_HOST_DESCRIPTOR_CTL, 0x00000003);
    clnk_reg_write_nl(dccp,TX_HOST_DESCRIPTOR_FRG, ((pTxHostDesc->fragmentLen) & 0xffff)|0x10000);
    clnk_reg_write_nl(dccp,TX_HOST_DESCRIPTOR_LEN, nLength);

    clnk_reg_write_nl(dccp,0x30304,0x0c080400);
    clnk_reg_write_nl(dccp,0x30100,0x00041000);
    clnk_reg_write_nl(dccp,0x30104,0x11);
    clnk_reg_write_nl(dccp,0x3020c,nLength);
    clnk_reg_write_nl(dccp,0x30200,0x0);
    clnk_reg_write_nl(dccp,0x30204,0x2);

    //PRINTOUT HostOS_PrintLog(L_DEBUG, "XMIT %d\n\r", nLength);
    //send to 68k bus - use txWord for alignment
    fragp = (char *)(pTxHostDesc->fragments[0].ptr);
    txWp = (char *)&txWord;
    for(i=0; i<(nLength>>2); i++) 
    {
        *txWp = *fragp++;
        *(txWp + 1) = *fragp++;
        *(txWp + 2) = *fragp++;
        *(txWp + 3) = *fragp++;
        clnk_reg_write_nl(dccp, TX_FIFO, txWord);
    //PRINTOUT HostOS_PrintLog(L_DEBUG, "%08x ", txWord);
    }
    //PRINTOUT HostOS_PrintLog(L_DEBUG, "\n\r");
    return;
}

/*******************************************************************************
*
* Public method:       interruptTxPreProcess
*
********************************************************************************
*
* Description:
*       bus process before do tx interrupt 
*
* Notes:
*       No need for PCI
*
*
********************************************************************************/
void socInterruptTxPreProcess(dc_context_t* dccp)
{
    // handle the bus specific call
    TxHostDesc_t *pTxHostDesc;
    SYS_UINT32 regVal;

    //DISABLE  68K TX interrupt
    clnk_reg_write_nl(dccp, CLNK_REG_68K_INT_MASK, CLNK_REG_INTERRUPT_FRAM_TRANSMITED);
    regVal = dccp->Int68kStatus;
    dccp->Int68kStatus = 0;
    //ENABLE 68K TX interrupt!
    clnk_reg_write_nl(dccp, CLNK_REG_68K_INT_MASK, 0);

    if (regVal & CLNK_REG_INTERRUPT_FRAM_TRANSMITED)
    {
        dccp->Int68kStatus &=~CLNK_REG_INTERRUPT_FRAM_TRANSMITED; 
        pTxHostDesc=dccp->p68kTxPending;
        if (pTxHostDesc == SYS_NULL)
            return;
        dccp->p68kTxPending = SYS_NULL;
        clnk_reg_read_nl(dccp,TX_HOST_DESCRIPTOR_CTL, &(pTxHostDesc->statusCtrl));
        //check if to send more
    }
}

#define ET_BUF_SIZE (64 * 1024)
#define KV_TO_K0(a) ((int) (a) | 0x20000000)

extern char          etBuf[ET_BUF_SIZE];
extern unsigned int  etRdInx;
extern unsigned int  etWrInx;

/*******************************************************************************
*
* Public method:       interruptRxPreProcess()
*
********************************************************************************
*
* Description:
*       Read packet(s) from RX FIFO after Rx interrupt 
*
* Notes:
*       68K Bus interface
*
*
********************************************************************************/
void socInterruptRxPreProcess(dc_context_t* dccp)
{
    RxHostDesc_t  *pRxHostDesc;
    RxListEntry_t *pCurEntry;
    SYS_UINT32     nCount,i,ctrl,pktLength,pollfifo;
    SYS_UINT32    *rxBuff;

    SYS_UINT32    *pa32;
  
    /* check for no packet in DMA buffer */
    if (etRdInx == etWrInx)
      return;
    
    /* check for DMA buffer wrap flag */
    pa32 = (SYS_UINT32*) KV_TO_K0(&etBuf[etRdInx]);
    if (*pa32 == 0xffffffff)
    {
      etRdInx = 0;
    
      /* Recheck for no packet after the wrap */
      if (etRdInx == etWrInx)
        return;
        
      pa32 = (SYS_UINT32*) KV_TO_K0(&etBuf[0]);
    }
    pollfifo  = *pa32++;  // RX descriptor status/control
    pktLength = *pa32++;  // dst+len
    ctrl      = *pa32++;  // reserved
    pktLength &= 0xffff;

    pCurEntry = COMMON_LIST_HEAD(RxListEntry_t, &dccp->rxList[dccp->rx68kCurListNum]);
    pRxHostDesc = pCurEntry->pRxHostDesc;
    pRxHostDesc->statusCtrl = ctrl;
    nCount = pRxHostDesc->fragmentLen >> 16;
    if (nCount != 1) HostOS_PrintLog(L_DEBUG, "Rx fragmentLen !=1 (%d)\n\r", nCount);
    rxBuff = (SYS_UINT32 *)pRxHostDesc->fragments[0].ptr;

    nCount = (pktLength + 3) >> 2;  /* in number of 32-bit words */
    for (i = 0; i < nCount; i++) 
      *rxBuff++ = *pa32++;

    etRdInx += (nCount + 3) << 2;
    
    pRxHostDesc->fragmentLen = (pRxHostDesc->fragmentLen & 0xffff0000) | pktLength;
    pRxHostDesc->fragments[0].len = pktLength;

    dccp->rx68kCurListNum++;
    dccp->rx68kCurListNum %= MAX_RX_LISTS;
}

SYS_BOOLEAN32 ValidRxList(dc_context_t* dccp)
{
    SYS_BOOLEAN32 bReturn = SYS_FALSE;
    if (dccp->rxList[dccp->rx68kCurListNum].pHead != 0)
    {
        bReturn = SYS_TRUE;
    }
    return bReturn;
}

