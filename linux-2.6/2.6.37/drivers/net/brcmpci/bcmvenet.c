/****************************************************************************
*
*  Copyright (c) 2011-2012 Broadcom Corporation
*
*  This program is the proprietary software of Broadcom Corporation and/or
*  its licensors, and may only be used, duplicated, modified or distributed
*  pursuant to the terms and conditions of a separate, written license
*  agreement executed between you and Broadcom (an "Authorized License").
*  Except as set forth in an Authorized License, Broadcom grants no license
*  (express or implied), right to use, or waiver of any kind with respect to
*  the Software, and Broadcom expressly reserves all rights in and to the
*  Software and all intellectual property rights therein.  IF YOU HAVE NO
*  AUTHORIZED LICENSE, THEN YOU HAVE NO RIGHT TO USE THIS SOFTWARE IN ANY WAY,
*  AND SHOULD IMMEDIATELY NOTIFY BROADCOM AND DISCONTINUE ALL USE OF THE
*  SOFTWARE.
*
*  Except as expressly set forth in the Authorized License,
*
*  1.     This program, including its structure, sequence and organization,
*  constitutes the valuable trade secrets of Broadcom, and you shall use all
*  reasonable efforts to protect the confidentiality thereof, and to use this
*  information only in connection with your use of Broadcom integrated circuit
*  products.
*
*  2.     TO THE MAXIMUM EXTENT PERMITTED BY LAW, THE SOFTWARE IS PROVIDED
*  "AS IS" AND WITH ALL FAULTS AND BROADCOM MAKES NO PROMISES, REPRESENTATIONS
*  OR WARRANTIES, EITHER EXPRESS, IMPLIED, STATUTORY, OR OTHERWISE, WITH
*  RESPECT TO THE SOFTWARE.  BROADCOM SPECIFICALLY DISCLAIMS ANY AND ALL
*  IMPLIED WARRANTIES OF TITLE, MERCHANTABILITY, NONINFRINGEMENT, FITNESS FOR
*  A PARTICULAR PURPOSE, LACK OF VIRUSES, ACCURACY OR COMPLETENESS, QUIET
*  ENJOYMENT, QUIET POSSESSION OR CORRESPONDENCE TO DESCRIPTION. YOU ASSUME
*  THE ENTIRE RISK ARISING OUT OF USE OR PERFORMANCE OF THE SOFTWARE.
*
*  3.     TO THE MAXIMUM EXTENT PERMITTED BY LAW, IN NO EVENT SHALL BROADCOM
*  OR ITS LICENSORS BE LIABLE FOR (i) CONSEQUENTIAL, INCIDENTAL, SPECIAL,
*  INDIRECT, OR EXEMPLARY DAMAGES WHATSOEVER ARISING OUT OF OR IN ANY WAY
*  RELATING TO YOUR USE OF OR INABILITY TO USE THE SOFTWARE EVEN IF BROADCOM
*  HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES; OR (ii) ANY AMOUNT IN
*  EXCESS OF THE AMOUNT ACTUALLY PAID FOR THE SOFTWARE ITSELF OR U.S. $1,
*  WHICHEVER IS GREATER. THESE LIMITATIONS SHALL APPLY NOTWITHSTANDING ANY
*  FAILURE OF ESSENTIAL PURPOSE OF ANY LIMITED REMEDY.
*
****************************************************************************
*
*  Filename: bcmvenet.c
*
****************************************************************************
* Description: This is the network interface driver for the virtual Ethernet
*              interface between 7xxx and 3383.
*
* Updates    : 04-13-2011  wfeng.  Created.
*
****************************************************************************/
#define CARDNAME    "bcmpcieeth"
#define VERSION     "1.13"
#define VER_STR     "v" VERSION " " __DATE__ " " __TIME__

#if defined(CONFIG_MODVERSIONS) && ! defined(MODVERSIONS)
#include <config/modversions.h>
#define MODVERSIONS
#endif

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/types.h>
#include <linux/fcntl.h>
#include <linux/interrupt.h>
#include <linux/string.h>
#include <linux/init.h>
#include <linux/errno.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/dma-mapping.h>

#include <linux/mii.h>
#include <linux/ethtool.h>
#include <linux/netdevice.h>
#include <linux/inetdevice.h>
#include <linux/etherdevice.h>
#include <linux/skbuff.h>
#include <linux/in.h>
#include <linux/proc_fs.h>
#include <linux/list.h>
#include <linux/pci.h>
#include <linux/aer.h>
#include <linux/version.h>

#include <asm/mipsregs.h>
#include <asm/cacheflush.h>
#include <asm/brcmstb/brcmstb.h>
#include <asm/brcmstb/brcmapi.h>
#include <linux/if_ether.h>
#include <linux/spinlock.h>

#include "bcmvenet.h"

#define ENET_MAX_MTU_SIZE 1536    /* Body(1500) + EH_SIZE(14) + VLANTAG(4) + BRCMTAG(6) + FCS(4) = 1528.  1536 is multiple of 256 bytes */
#define uint32 u32

#define BCM3383DEVICE 0x3383
#define BCM3383DEVICETEXT "3383"
#define BCM3383VENDOR 0x14e4

//#define DUMP_TRACE

#if defined(DUMP_TRACE)
#define TRACE(x)        printk x
#else
#define TRACE(x)
#endif

#define TIMEOUT_RESET           (HZ / 1)
/*
#define TIMEOUT_RESET           (HZ / 10)
#define TIMEOUT_RESET           (HZ * 2)
*/
#define WATCHDOG_INTERVAL       (HZ * 1)

#define L0_RX_DONE_IRQ      (1 << 2)
#define L0_TX_DONE_IRQ      (1 << 0)
#define MBOX0_IRQ           (1 << 16)
#define MBOX1_IRQ           (1 << 17)
#define MBOX2_IRQ           (1 << 18)
#define MBOX3_IRQ           (1 << 19)
#define VENDOR_CFG_MBOX0    0x198
#define VENDOR_CFG_MBOX1    0x19c
#define VENDOR_CFG_MBOX2    0x1a0
#define VENDOR_CFG_MBOX3    0x1a4
#define VENDOR_SPECIFIC_REG1 0x188
#define VENDOR_SPECIFIC_REG2 0x18c
#define VENDOR_SPECIFIC_REG3 0x190
#define VENDOR_SPECIFIC_REG4 0x194


#define VENET_RX_BUF_SIZE 2048
#define MAX_VENET_RX_BUF 1024
#define MAX_VENET_TX_BUF 1024

#define VENET_CACHE_LINE_SIZE 32
#define FPM_TOKEN_MASK        0x7ffff000
#define FPM_TOKEN_VALID_MASK  0x80000000
#define FPM_TOKEN_INDEX_SHIFT 12
#define FPM_TOKEN_SIZE 0x800

#define BCHP_PCIE_RC_CFG_PCIE_DEVICE_STATUS_CONTROL 0x004100b4 /* Device status control */

#define skb_dataref(x)   (&skb_shinfo(x)->dataref)

typedef struct {
	uint32 TxFirstDescLAddrList0;
	uint32 TxFirstDescUAddrList0;
	uint32 TxFirstDescLAddrList1;
	uint32 TxFirstDescUAddrList1;
	uint32 TxSwDescListCtrlSts;
	uint32 TxWakeCtrl;
	uint32 TxErrorStatus;
	uint32 TxList0CurDescLAddr;
	uint32 TxList0CurDescUAddr;
	uint32 TxList0CurByteCnt;
	uint32 TxList1CurDescLAddr;
	uint32 TxList1CurDescUAddr;
	uint32 TxList1CurByteCnt;
	uint32 RxFirstDescLAddrList0;
	uint32 RxFirstDescUAddrList0;
	uint32 RxFirstDescLAddrList1;
	uint32 RxFirstDescUAddrList1;
	uint32 RxSwDescListCtrlSts;
	uint32 RxWakeCtrl;
	uint32 RxErrorStatus;
	uint32 RxList0CurDescLAddr;
	uint32 RxList0CurDescUAddr;
	uint32 RxList0CurByteCnt;
	uint32 RxList1CurDescLAddr;
	uint32 RxList1CurDescUAddr;
	uint32 RxList1CurByteCnt;
	uint32 DmaDebugOptionsReg;
	uint32 ReadChannelErrorStatus;
}  PcieDmaReg;

typedef struct {
	uint32                              IntrStatus;
	uint32                              IntrMaskStatus;
	uint32                              IntrMaskSet;
	uint32                              IntrMaskClear;
}  PcieIntr1Registers;

#define PCIE_UBUS_PCIE_INTR (1<<7)
#define PCIE_INTR_PCIE_INTR (1<<5)

typedef struct {
	uint32                              CpuStatus;
	uint32                              CpuSet;
	uint32                              CpuClear;
	uint32                              CpuMaskStatus;
	uint32                              CpuMaskSet;
	uint32                              CpuMaskClear;
	uint32                              PciStatus;
	uint32                              PciSet;
	uint32                              PciClear;
	uint32                              PciMaskStatus;
	uint32                              PciMaskSet;
	uint32                              PciMaskClear;
}  PcieIntr2Registers;

typedef struct s_pcieBufDesc {
	uint32 localAddr;
	uint32 pcieLowerAddr;
	uint32 pcieUpperAddr;
	uint32 control0;
	uint32 control1;
	uint32 nextLowerAddr;
	uint32 nextUpperAddr;
	uint32 reserved;
} pcieBufDesc;

typedef struct s_venet_cb {
	struct sk_buff *skb;
	dma_addr_t       pa;
} VENET_CB;

typedef struct {
	uint32 tx_first_bd;
	uint32 tx_new_bd;
	uint32 tx_prev_new_bd;
	uint32 num_of_tx_bds;
	uint32 rx_first_bd;
	uint32 rx_new_bd;
	uint32 rx_prev_new_bd;
	uint32 num_of_rx_bds;
} pcieIpcInfo;

/*
* device context
*/
typedef struct BcmVEnet_devctrl {
	struct net_device *ndev;            /* ptr to net_device */
	struct pci_dev *pdev;               /* ptr to net_device */
	spinlock_t      lock;               /* Serializing lock */
	spinlock_t      bh_lock;            /* Serializing lock */

	struct napi_struct napi;
	unsigned int rbase0;                /* SUB register start address. */
	unsigned int mbase2;                /* SUB register start address. */
	unsigned int mbase4;                /* SUB register start address. */
	unsigned int base_offset;
	volatile PcieDmaReg *dmactl;        /* SUB DMA register block base address */
	volatile PcieIntr1Registers *int_lvl1; /* SUB DMA register block base address */
	volatile PcieIntr2Registers *int_lvl2; /* SUB DMA register block base address */
	volatile unsigned long *inMbox;
	volatile unsigned long *outMbox;
	volatile unsigned long *altOutMbox;

	pcieBufDesc *tx_first_bd;
	pcieBufDesc *tx_last_bd;
	pcieBufDesc *tx_new_bd;
	pcieBufDesc *tx_used_bd;
	pcieBufDesc *tx_prev_new_bd;
	pcieBufDesc *tx_refill_bd;
	volatile unsigned int *tx_consumed_bd;

	pcieBufDesc *rx_first_bd;
	pcieBufDesc *rx_last_bd;
	pcieBufDesc *rx_new_bd;
	pcieBufDesc *rx_refill_bd;
	volatile unsigned int *rx_consumed_bd;

	pcieBufDesc *rx_prev_new_bd;

	unsigned int tx_used_bd_cnt;
	unsigned int tx_buf_num;
	unsigned int rx_buf_num;

	VENET_CB rx_cb[MAX_VENET_RX_BUF];
	unsigned int *tx_buf[MAX_VENET_TX_BUF];

	int irq;                            /* BNM HOST IRQ */
	int irq_rx;                         /* BNM HOST IRQ */
	int	irq_stat;                       /* Software copy of irq status, for botom half processing */
	struct tasklet_struct mtask;        /* Task to process mbox messages */
	volatile int link;                  /* Link status */
	int state;                          /* Up or down */
	struct timer_list rtimer;           /* Timer to clean up */

	unsigned int rx_cnt;                /* Packets successfully received */
	unsigned int rx_err_cnt;            /* Packets dropped because no skb is available */
	unsigned int rx_free_cnt;           /* Packets released back to BNM */
	unsigned int rx_free_err_cnt;       /* Packets can not be released back to BNM */
	unsigned int tx_cnt;                /* Packets successfully transmitted */
	unsigned int tx_err_cnt;            /* Packets not transmitted because no BNM buffer is available */
    unsigned int hostbootMail;
	unsigned int ipc_info_pa;
	int pcie_cfg_cap;
	
	/* PACE additions */ 
	int BnmResetDetected;
	MBOX_ResetList MboxResetList;
	/* Pace Additions end */
} BcmVEnet_devctrl;

#define PCIE_32BIT_ADDRESS_SPACE
//#define DEDICATED_RX_INTERRUPT
// --------------------------------------------------------------------------
//      External, indirect entry points.
// --------------------------------------------------------------------------
static int bcmvenet_ioctl(struct net_device *dev, struct ifreq *rq, int cmd);
// --------------------------------------------------------------------------
//      Called for "ifconfig ethX up" & "down"
// --------------------------------------------------------------------------
static int bcmvenet_open(struct net_device *dev);
static int bcmvenet_close(struct net_device *dev);
// --------------------------------------------------------------------------
//      Watchdog timeout
// --------------------------------------------------------------------------
static void bcmvenet_timeout(struct net_device *dev);
// --------------------------------------------------------------------------
//      Packet transmission.
// --------------------------------------------------------------------------
static int bcmvenet_xmit(struct sk_buff *skb, struct net_device *dev);
// --------------------------------------------------------------------------
//      Set address filtering mode
// --------------------------------------------------------------------------
static void bcmvenet_set_multicast_list(struct net_device *dev);
// --------------------------------------------------------------------------
//      Set the hardware MAC address.
// --------------------------------------------------------------------------
static int bcmvenet_set_mac_addr(struct net_device *dev, void *p);

// --------------------------------------------------------------------------
//      Interrupt routine, for all interrupts except ring buffer interrupts
// --------------------------------------------------------------------------
static irqreturn_t bcmvenet_isr(int irq, void *dev_id);

#if defined(DEDICATED_RX_INTERRUPT)
// --------------------------------------------------------------------------
//      Interrupt routine, for rx data interrupts only
// --------------------------------------------------------------------------
static irqreturn_t bcmvenet_rx_isr(int irq, void *dev_id);
#endif
// --------------------------------------------------------------------------
//      dev->poll() method
// --------------------------------------------------------------------------
static int bcmvenet_poll(struct napi_struct *napi, int budget);
// --------------------------------------------------------------------------
//      Process recived packet for descriptor based DMA
// --------------------------------------------------------------------------
static unsigned int bcmvenet_rx(void *ptr, unsigned int budget);

// --------------------------------------------------------------------------
//      Internal routines
// --------------------------------------------------------------------------
/* Allocate and initialize tx/rx buffer descriptor pools */
static int bcmvenet_init_dev(BcmVEnet_devctrl *pDevCtrl);
static void bcmvenet_uninit_dev(BcmVEnet_devctrl *pDevCtrl);

//#define DUMP_DATA
#if defined(DUMP_DATA)
/* Display hex base data */
static void dumpHexData(char *typ, unsigned char *head, int len);
/* dumpMem32 dump out the number of 32 bit hex data  */
static void dumpMem32(unsigned int *pMemAddr, int iNumWords);
#endif

static int bcmvenet_do_init_dev(BcmVEnet_devctrl *pDevCtrl);
static void mbox_tasklet(unsigned long arg);
/*static void bcmvenet_rtimer_handler(unsigned long arg);*/
static char bcmvenet_drv_name[] = "BCM" BCM3383DEVICETEXT;
static char ecm_Ready=0;

/* Pace Additions */
static BcmVEnet_devctrl *pDevContext = NULL;

static u8 bcmemac_cmdline_macaddr[ETH_ALEN] ;

static void MBOX_ResetListInit( MBOX_ResetList *l )
{
   int i;
   unsigned long flags;
   MBOX_ResetEntry *resetListEntry = 0;

   spin_lock_init( &l->mySpinLock );
    
   /* take the sema, just in case while we init the rest */
   spin_lock_irqsave( &l->mySpinLock, flags );
 
   INIT_LIST_HEAD( &l->usedList );
   INIT_LIST_HEAD( &l->freeList );

   /* chain all entries onto the freelist */
   for ( i = 0; i < MBOX_RESET_L_MAX; i++ )
   {
      resetListEntry = &l->resetEntries[i];
      /* WARNING - list head must be at start of struct */
      list_add((struct list_head *) resetListEntry, &l->freeList );
   }

   /* Unlock the critical section code PDI queue head */
   spin_unlock_irqrestore( &l->mySpinLock, flags );
}

static void MBOX_ResetListInsert( MBOX_ResetList *l, MBOX_RESET_FUNC func, void *user_data )
{ 
   unsigned long flags;
 
   /* Lock the critical section code PDI queue head */
   spin_lock_irqsave( &l->mySpinLock, flags );

   if ( !list_empty( &l->freeList ) )
   {
      struct list_head *freeListEntry = l->freeList.next;
      struct list_head *usedListEntry = &(l->usedList);
      MBOX_ResetEntry  *newResetEntry = 0;

      list_del( freeListEntry ); /* Remove from freelist */

      newResetEntry = list_entry( freeListEntry, MBOX_ResetEntry, entry );

      newResetEntry->mbox_reset_func = func;
      newResetEntry->user_data       = user_data;

      list_add_tail( freeListEntry, usedListEntry ); /* add to used list */
   }

   /* Unlock the critical section code PDI queue head */
   spin_unlock_irqrestore( &l->mySpinLock, flags );
}

static void MBOX_ResetListRemove( MBOX_ResetList *l, MBOX_RESET_FUNC func )
{
   unsigned long flags;
 
   /* Lock the critical section code PDI queue head */
   spin_lock_irqsave( &l->mySpinLock, flags );
 
   if ( !list_empty( &l->usedList ) )
   {
      struct list_head *usedListEntry;
      struct list_head *next;
      struct list_head *freeListEntry = l->freeList.next;

      list_for_each_safe( usedListEntry, next, &l->usedList )
      {
         MBOX_ResetEntry *resetEntry = list_entry( usedListEntry, MBOX_ResetEntry, entry );

         if ( resetEntry->mbox_reset_func == func )
         {
            resetEntry->mbox_reset_func = NULL;
            resetEntry->user_data       = NULL;
            list_del( usedListEntry );
            list_add_tail( freeListEntry, usedListEntry );
         }
      }
   }

   /* Unlock the critical section code PDI queue head */
   spin_unlock_irqrestore( &l->mySpinLock, flags );
}

void MBOX_InsertResetListener( MBOX_RESET_FUNC func, void *user_data )
{
   if ( pDevContext )
   {
      MBOX_ResetListInsert( &pDevContext->MboxResetList, func, user_data );
   }
}
EXPORT_SYMBOL(MBOX_InsertResetListener);

void MBOX_RemoveResetListener( MBOX_RESET_FUNC func )
{
   if ( pDevContext )
   {
      MBOX_ResetListRemove( &pDevContext->MboxResetList, func );
   }
}
EXPORT_SYMBOL(MBOX_RemoveResetListener);

static void MBOX_ResetListProcess( MBOX_ResetList *l, int resetting )
{
   unsigned long flags;

   /* Lock the critical section code PDI queue head */
   spin_lock_irqsave( &l->mySpinLock, flags );

   if ( !list_empty( &l->usedList ) )
   { 
      MBOX_ResetEntry *resetEntry;

      list_for_each_entry( resetEntry, &l->usedList, entry )
      {
         if ( resetEntry->mbox_reset_func )
         {
            resetEntry->mbox_reset_func( resetting, resetEntry->user_data );
         }
      }
   }

   /* Unlock the critical section code PDI queue head */
   spin_unlock_irqrestore( &l->mySpinLock, flags );
}
/* Pace Additions end */

// --------------------------------------------------------------------------
//      Miscellaneous variables
// --------------------------------------------------------------------------
/*static struct kmem_cache * skb_head_cache __read_mostly;*/

#if defined(DUMP_DATA)
/*
 * dumpHexData dump out the hex base binary data
 */
static void dumpHexData(char *typ, unsigned char *head, int len) {
	int i;
	unsigned char *curPtr = head;

	printk("%s (%d):", typ, len);

	for (i = 0; i < len; ++i) {
		if ((i & 3) == 0) {
			printk(" ");
		}
		printk("%02x", *curPtr++);
	}
	printk("\n");
}

/*
 * dumpMem32 dump out the number of 32 bit hex data
 */
static void dumpMem32(unsigned int *pMemAddr, int iNumWords) {
	int i = 0;
	static char buffer[80];

	sprintf(buffer, "%08X: ", (unsigned int)pMemAddr);
	printk(buffer);
	while (iNumWords) {
		sprintf(buffer, "%08X ", (unsigned int)*pMemAddr++);
		printk(buffer);
		iNumWords--;
		i++;
		if ((i % 4) == 0 && iNumWords) {
			sprintf(buffer, "\n%08X: ", (unsigned int)pMemAddr);
			printk(buffer);
		}
	}
	printk("\n");
}
#endif

void mb_send(unsigned int msg, unsigned int data, void *arg) {
	BcmVEnet_devctrl *pDevCtrl = (BcmVEnet_devctrl *)arg;
	unsigned int val;

	val = msg & MBOX_MSG_MASK;
	val |= (data & (~MBOX_MSG_MASK));
	*(pDevCtrl->altOutMbox) = val;
	pDevCtrl->int_lvl2->CpuSet |= MBOX3_IRQ;
}

/***********************************************************************/
/*   mbox_tasklet() -                                              */
/*                                                                     */
/*   Process MailBox receive interrupt                             */
/*                                                                     */
/***********************************************************************/
static void mbox_tasklet(unsigned long arg) {
	BcmVEnet_devctrl *pDevCtrl = (BcmVEnet_devctrl *)arg;

    unsigned int mailbox = *(pDevCtrl->inMbox);

	/* Read the mbox message */
	switch (mailbox & MBOX_MSG_MASK) {
	case MBOX_NULL:
        pDevCtrl->hostbootMail = mailbox;
        break;

	case MBOX_REBOOTING:
		if (pDevCtrl->link) {
			pDevCtrl->link = 0;
            printk(KERN_INFO CARDNAME ": ECM rebooting!\n");
            // First tell the EP DMA engine to stop
            pDevCtrl->dmactl->TxSwDescListCtrlSts &= ~0x1;
            pDevCtrl->dmactl->RxSwDescListCtrlSts &= ~0x1;
            // Ok, turn off our end
			netif_carrier_off(pDevCtrl->ndev);
			/* Stop the network engine */
			napi_disable(&pDevCtrl->napi);
			netif_stop_queue(pDevCtrl->ndev);
		}
		break;

	case MBOX_DMA_READY:
        /* Pace Changes start */
        /* Ignore any DMA Ready after it has been seen once, See below for coments */
        if(0 == ecm_Ready) 
        {
		printk(KERN_INFO CARDNAME ": ECM ready!\n");
		if (!pDevCtrl->link) {
			pDevCtrl->ipc_info_pa = mailbox & (~MBOX_MSG_MASK);
			TRACE(("%s: ipc_info_pa = %08x\n", __FUNCTION__, pDevCtrl->ipc_info_pa));
			bcmvenet_do_init_dev(pDevCtrl);
		}
            ecm_Ready = 1;
        }
		break;

        /* For TiVo builds, the eCM sends the MBOX_DMA_READY several times for parallel boot
         * This is since the kernel is booted of the Disk & accounting for Disk latencies
         * The bcmvenet driver may not have been setup in time to receive the Ready
         * Additionally MBOX_XFER_BLOCK is not expected anytime post DMA Ready and
         * This is used to catch the uncontrolled eCM resets */
    case MBOX_XFER_BLOCK:
        pDevCtrl->hostbootMail = mailbox;
        if(ecm_Ready) /* catch an unhandled eCM reset */
        {
          MBOX_ResetListProcess( &pDevCtrl->MboxResetList, 1 ); 
        }
          /* Pace Changes end */
		break;

	default:
		printk(KERN_ERR CARDNAME ": Unknown mailbox command (%08x)!\n", mailbox);
		break;
	}

	/* Enable the interrupt at L2 */
	pDevCtrl->int_lvl2->PciMaskClear |=  MBOX0_IRQ;

}

/* --------------------------------------------------------------------------
    Name: bcmvenet_open
 Purpose: Open and Initialize the SUB DMA on the chip
-------------------------------------------------------------------------- */
static int bcmvenet_open(struct net_device *dev) {
	BcmVEnet_devctrl *pDevCtrl = netdev_priv(dev);

	TRACE(("%s: bcmvenet_open\n", dev->name));
	/* PACE Additions */
	pDevCtrl->BnmResetDetected = 0;
	pDevContext = pDevCtrl;
	/* Pace Additions end */
	
	/* Enable RX/TX DMA */
	pDevCtrl->state = 1;
	if (pDevCtrl->link) {
		pci_write_config_dword(pDevCtrl->pdev, VENDOR_SPECIFIC_REG2, pDevCtrl->link);
		/* Start the network engine */
		netif_start_queue(dev);
		napi_enable(&pDevCtrl->napi);

        pDevCtrl->int_lvl2->PciClear |= (L0_RX_DONE_IRQ  | MBOX0_IRQ);
        pDevCtrl->int_lvl2->PciMaskClear |= (L0_RX_DONE_IRQ  | MBOX0_IRQ);
        netif_carrier_on(pDevCtrl->ndev);
	}

	return 0;
}


/* --------------------------------------------------------------------------
    Name: bcmvenet_close
    Purpose: Stop communicating with the outside world
    Note: Caused by 'ifconfig ethX down'
-------------------------------------------------------------------------- */
static int bcmvenet_close(struct net_device *dev) {
	BcmVEnet_devctrl *pDevCtrl = netdev_priv(dev);

	TRACE(("%s: bcmvenet_close\n", dev->name));

    if (pDevCtrl->link) {
        netif_carrier_off(pDevCtrl->ndev);
	/* Disable mbox and data interrupts */
	pDevCtrl->int_lvl2->PciMaskSet |= (L0_RX_DONE_IRQ  | MBOX0_IRQ);
	pDevCtrl->int_lvl2->PciClear |= (L0_RX_DONE_IRQ  | MBOX0_IRQ);

	/* Stop the network engine */
	pDevCtrl->link = 0;
	napi_disable(&pDevCtrl->napi);
	netif_stop_queue(dev);
    }

	return 0;
}

/* --------------------------------------------------------------------------
    Name: bcmvenet_net_timeout
 Purpose:
-------------------------------------------------------------------------- */
static void bcmvenet_timeout(struct net_device *dev) {
	BUG_ON(dev == NULL);

	TRACE(("%s: bcmvenet_timeout\n", dev->name));

	dev->trans_start = jiffies;

	dev->stats.tx_errors++;
	netif_wake_queue(dev);
}

/* --------------------------------------------------------------------------
 Name: bcmvenet_set_multicast_list
 Purpose: Set the multicast mode, ie. promiscuous or multicast
-------------------------------------------------------------------------- */
static void bcmvenet_set_multicast_list(struct net_device *dev) {
//    TRACE(("%s: bcmvenet_set_multicast_list: %08X\n", dev->name, dev->flags));
}
/*
 * Set the hardware MAC address.
 */
static int bcmvenet_set_mac_addr(struct net_device *dev, void *p) {
	struct sockaddr *addr = p;

	if (netif_running(dev)) {
		return -EBUSY;
	}

	memcpy(dev->dev_addr, addr->sa_data, dev->addr_len);

	return 0;
}

/* --------------------------------------------------------------------------
 Name: bcmvenet_xmit
 Purpose: Send ethernet traffic
-------------------------------------------------------------------------- */
static int bcmvenet_xmit(struct sk_buff *skb, struct net_device *dev) {
	BcmVEnet_devctrl *pDevCtrl = netdev_priv(dev);
	/*unsigned char *pBuf;*/
	unsigned long flags;
	dma_addr_t dma_addr;
	unsigned int index;

	if (skb == NULL) {
		printk("null skb\n");
		return 0;
	}

    if (!netif_carrier_ok(pDevCtrl->ndev)) {
		printk("link down\n");
		dev_kfree_skb(skb);
		dev->stats.tx_errors++;
		dev->stats.tx_dropped++;
		return 0;
	}

#if defined(DUMP_DATA)
	TRACE(("bcmvenet_xmit: len %d", skb->len));
	dumpHexData("xmit", skb->data, skb->len);
#endif

	/*
	 * Obtain exclusive access to transmitter.  This is necessary because
	 * we might have more than one stack transmitting at once.
	 */
	spin_lock_irqsave(&pDevCtrl->lock, flags);

	/* Reclaim some BDs first */
	while (1) {
		if ((((unsigned int)pDevCtrl->tx_used_bd == (pDevCtrl->mbase2 + *pDevCtrl->tx_consumed_bd))
			 && (pDevCtrl->tx_used_bd_cnt != pDevCtrl->tx_buf_num))
			|| (pDevCtrl->tx_used_bd_cnt == 0)) {
			break;
		}
		/* Decrement the used bd counter */
		pDevCtrl->tx_used_bd_cnt--;
		if ((unsigned int)pDevCtrl->tx_used_bd == (unsigned int)pDevCtrl->tx_last_bd) {
			pDevCtrl->tx_used_bd = pDevCtrl->tx_first_bd;
		} else {
			pDevCtrl->tx_used_bd++;
		}
	}

	if (pDevCtrl->tx_used_bd_cnt == pDevCtrl->tx_buf_num) {
		dev_kfree_skb(skb);
		dev->stats.tx_errors++;
		dev->stats.tx_dropped++;
		/* TODO: schedule a timer */
		spin_unlock_irqrestore(&pDevCtrl->lock, flags);
		return 0;
	}

	index = (pDevCtrl->tx_new_bd - pDevCtrl->tx_first_bd) / sizeof(pcieBufDesc);

	/* copy the data to an aligned buffer */
	skb_copy_from_linear_data(skb, (void *)pDevCtrl->tx_buf[index], skb->len);

	/* Flush the cache */
	dma_addr = pci_map_single(pDevCtrl->pdev, (void *)pDevCtrl->tx_buf[index], skb->len, DMA_TO_DEVICE);

	/* Program the BD */
	pDevCtrl->tx_new_bd->pcieLowerAddr = dma_addr;
#if !defined(PCIE_32BIT_ADDRESS_SPACE)
	pDevCtrl->tx_new_bd->pcieUpperAddr = 0x00000001;
#endif
	pDevCtrl->tx_new_bd->control0 = (0x80000000 | (skb->len + 4));
	pDevCtrl->tx_new_bd->control1 |= 0xc0000002; // byte swapping

	/* Queue the descriptor */
	if (pDevCtrl->tx_prev_new_bd == pDevCtrl->tx_new_bd) {

		/* Set up DMA control: single list, descriptor in local memory */
		pDevCtrl->dmactl->TxSwDescListCtrlSts = 0x00000300;

		/* Queue the descriptor */
		pDevCtrl->dmactl->TxFirstDescLAddrList0 = ((((unsigned int)pDevCtrl->tx_first_bd - pDevCtrl->mbase2) & 0x1FFFFFFF) | 0x1);

		/* Kick the DMA */
		pDevCtrl->dmactl->TxSwDescListCtrlSts |= 0x1;
	} else {
		/* Clear the last record bit in previous BD */
		pDevCtrl->tx_prev_new_bd->control1 &= (~0x80000000);

		// Wake the DMA
		pDevCtrl->dmactl->TxWakeCtrl |= 1;
	}

	/* Increment the BD ptr */
	pDevCtrl->tx_prev_new_bd = pDevCtrl->tx_new_bd;
	if ((unsigned int)pDevCtrl->tx_new_bd >= (unsigned int)pDevCtrl->tx_last_bd) {
		pDevCtrl->tx_new_bd = pDevCtrl->tx_first_bd;
	} else {
		pDevCtrl->tx_new_bd++;
	}

	/* Counter */
	pDevCtrl->tx_cnt++;

	/* Increment the used bd counter */
	pDevCtrl->tx_used_bd_cnt++;

	/* Update stats */
	dev->stats.tx_bytes += skb->len;
	dev->stats.tx_packets++;
	dev->trans_start = jiffies;

	spin_unlock_irqrestore(&pDevCtrl->lock, flags);

	/* Free the network buffer now */
	dev_kfree_skb(skb);

	return 0;
}

static int assign_rx_buffers(BcmVEnet_devctrl *pDevCtrl) {
	int rc = 0;
	struct sk_buff *skb;
	unsigned int index;

	spin_lock_bh(&pDevCtrl->bh_lock);

	while (1) {

		/* Get the index */
		index = ((unsigned int)pDevCtrl->rx_refill_bd - (unsigned int)pDevCtrl->rx_first_bd) / sizeof(pcieBufDesc);

		if (pDevCtrl->rx_cb[index].skb != NULL) {
			break;
		}

		/* Get a buffer */
		skb = netdev_alloc_skb(pDevCtrl->ndev, ENET_MAX_MTU_SIZE);
		if (skb == NULL) {
			break;
		}

		/* Fill up the control block */
		pDevCtrl->rx_cb[index].skb = skb;
		pDevCtrl->rx_cb[index].pa = pci_map_single(pDevCtrl->pdev, skb->data, ENET_MAX_MTU_SIZE, DMA_FROM_DEVICE);
		/* Assign the buffer to the BD */
		pDevCtrl->rx_refill_bd->pcieLowerAddr = pDevCtrl->rx_cb[index].pa;
#if !defined(PCIE_32BIT_ADDRESS_SPACE)
		pDevCtrl->rx_refill_bd->pcieUpperAddr = 0x00000001;
#endif

		/* Increment the refill ptr */
		if (pDevCtrl->rx_refill_bd == pDevCtrl->rx_last_bd) {
			pDevCtrl->rx_refill_bd = pDevCtrl->rx_first_bd;
		} else {
			pDevCtrl->rx_refill_bd++;
		}
	}

	spin_unlock_bh(&pDevCtrl->bh_lock);

	return rc;
}


/* NAPI polling method*/
static int bcmvenet_poll(struct napi_struct *napi, int budget) {
	BcmVEnet_devctrl *pDevCtrl = container_of(napi, struct BcmVEnet_devctrl, napi);
	unsigned int work_done;

    if (!netif_carrier_ok(pDevCtrl->ndev)) {
		napi_complete(napi);
		return 0;
	}

	work_done = bcmvenet_rx(pDevCtrl, budget);
	assign_rx_buffers(pDevCtrl);
	if (work_done < budget) {
		napi_complete(napi);

		/* Enable data interrupt */
		pDevCtrl->int_lvl2->PciMaskClear |=  L0_RX_DONE_IRQ;
	}
	return work_done;
}

#if defined(DEDICATED_RX_INTERRUPT)
/*
 * bcmvenet_rx_isr: BNM rx data interrupt handler
 */
static irqreturn_t bcmvenet_rx_isr(int irq, void *dev_id) {
	BcmVEnet_devctrl *pDevCtrl = dev_id;

	/* Disable the interrupt at L2 */
	pDevCtrl->bnmctl->hostIrqMask2 &= ~(BD_THRESH_IRQ | BD_TIMEOUT_IRQ);
	pDevCtrl->bnmctl->hostIrqStatus2 = (BD_THRESH_IRQ | BD_TIMEOUT_IRQ);
	napi_schedule(&pDevCtrl->napi);

	return IRQ_HANDLED;
}
#endif

/*
 * bcmvenet_isr: BNM interrupt handler
 */
static irqreturn_t bcmvenet_isr(int irq, void *dev_id) {
	BcmVEnet_devctrl *pDevCtrl = dev_id;
	uint32 events;

	events = pDevCtrl->int_lvl2->PciStatus;

	//TRACE(("%s : = %08x\n", __FUNCTION__, events));
	if (events & L0_RX_DONE_IRQ) {
		/* Disable interrupt at L2 */
		pDevCtrl->int_lvl2->PciMaskSet |=  L0_RX_DONE_IRQ;
		/* Clear the interrupt */
		pDevCtrl->int_lvl2->PciClear |=  L0_RX_DONE_IRQ;
		/* Schedule the NAPI function */
		napi_schedule(&pDevCtrl->napi);
	}

	if (events & MBOX0_IRQ) {
		/* Disable the interrupt at L2 */
		pDevCtrl->int_lvl2->PciMaskSet |=  MBOX0_IRQ;
		/* Clear the interrupt */
		pDevCtrl->int_lvl2->PciClear |=  MBOX0_IRQ;

		/* Schedule the task */
		tasklet_schedule(&pDevCtrl->mtask);
	}

	return IRQ_HANDLED;
}

/*
 *  bcmvenet_rx - called from NAPI polling method.
 */
static unsigned int bcmvenet_rx(void *ptr, unsigned int budget) {
	struct sk_buff *skb;
	int len;
	//unsigned char *pBuf;
	unsigned int index;
	pcieBufDesc *cur_bd;
	unsigned int cnt = 0;
	BcmVEnet_devctrl *pDevCtrl = ptr;
	struct net_device *ndev = pDevCtrl->ndev;

	/* Set the initial condition so current BD is read from H/W */
	cur_bd = pDevCtrl->rx_new_bd;
	while ((cnt < budget)) {
        if (!netif_carrier_ok(pDevCtrl->ndev)) {
			break;
		}
		/* Get the index */
		index = ((unsigned int)pDevCtrl->rx_new_bd - (unsigned int)pDevCtrl->rx_first_bd) / sizeof(pcieBufDesc);

		/* Get the control block */
		skb = pDevCtrl->rx_cb[index].skb;
		if (skb == NULL) {
			printk("null skb\n");
			break;
		}

		/* Read the current BD pointer */
		if (pDevCtrl->rx_new_bd  == cur_bd) {
			cur_bd = (pcieBufDesc *)(pDevCtrl->mbase2 + (pDevCtrl->dmactl->RxList0CurDescLAddr & 0xffffffe0));
		}

		/* Check again with updated current BD ptr */
		if (pDevCtrl->rx_new_bd  == cur_bd) {
			/* Check DMA satus */
			if ((pDevCtrl->dmactl->TxSwDescListCtrlSts & 0x30) == 0x10) {
				/* DMA is busy, so I bail */
				printk("dma busy\n");
				break;
			}
		} else {
			if (pDevCtrl->rx_prev_new_bd  == cur_bd) {
				/* New pointer had moved, but DMA had not, bail */
				if ((cur_bd->control0 & 0x01FFFFFF) == 0) {
					break;
				}
			}
		}

		/* Get the length */
		len = (int)(pDevCtrl->rx_new_bd->control0 & 0x01FFFFFF);
		if (len >= 4) {
			len -= 4;
		}
#if defined(DUMP_DATA)
		dumpHexData("recv", skb->data, len);
#endif
		if (len < 64) {
			len = 64;
		}

		/* Zero the length */
		pDevCtrl->rx_new_bd->control0 &= 0xFE000000;

		/* Zero out the entry */
		pDevCtrl->rx_cb[index].skb = NULL;

		if (pDevCtrl->rx_refill_bd == NULL) {
			pDevCtrl->rx_refill_bd = pDevCtrl->rx_first_bd;
		}

		/* Remember the last position */
		pDevCtrl->rx_prev_new_bd = pDevCtrl->rx_new_bd ;

		/* Increment the new pointer */
		if (pDevCtrl->rx_new_bd == pDevCtrl->rx_last_bd) {
			pDevCtrl->rx_new_bd = pDevCtrl->rx_first_bd;
		} else {
			pDevCtrl->rx_new_bd++;
		}

		/* Update the consumed index */
		*pDevCtrl->rx_consumed_bd = ((u32)pDevCtrl->rx_new_bd - (u32)pDevCtrl->mbase2);

        if (netif_carrier_ok(pDevCtrl->ndev)) {
		/* Set packet length */
		skb_put(skb, len);
		/* Device, protocol and housekeeping */
		skb->dev = pDevCtrl->ndev;
		skb->protocol = eth_type_trans(skb, pDevCtrl->ndev);
		ndev->stats.rx_packets++;
		ndev->stats.rx_bytes += len;

		/* Give it to kernel */
		netif_receive_skb(skb);

		pDevCtrl->rx_cnt++;
        } else {
            dev_kfree_skb(skb);
        }

		cnt++;
	}

	return cnt;
}

/* PACE Additions */
static int pci0_proc_read(char *page, char **start, off_t off, int cnt, int *eof, void *data)
{
    BcmVEnet_devctrl *pDevCtrl = (BcmVEnet_devctrl *)data;

	int len = 0;


	if (pDevCtrl->BnmResetDetected)
 	{
		len += sprintf(&page[len], "PciResetDetected\n");
	}
	else if (pDevCtrl->link)
 	{
		len += sprintf(&page[len], "PciHostDmaReady\n");
	}
	else
	{
		len += sprintf(&page[len], "PciNotReady\n");
	}

	return len;

}
/* PACE Additions end */

static int bcmvenet_proc_get_stat(char *page, char **start, off_t off, int cnt, int *eof, void *data) {
	int r;
	BcmVEnet_devctrl *pDevCtrl = (BcmVEnet_devctrl *)data;
#if 0
	int i;
	volatile pcieBufDesc *p;
#endif

	r = sprintf(page, "Total queue length should be %d\n", pDevCtrl->rx_buf_num);

    r += sprintf(page + r, "rbase0 = %08x\n", (u32)pDevCtrl->rbase0);
	r += sprintf(page + r, "mbase2 = %08x\n", (u32)pDevCtrl->mbase2);
	r += sprintf(page + r, "tx_consumed_bd_ptr= %08x\n", (u32)pDevCtrl->tx_consumed_bd);
	r += sprintf(page + r, "tx_consumed_bd= %08x\n", *pDevCtrl->tx_consumed_bd);
	r += sprintf(page + r, "\n<===RX BD DUMP===>\n");
	r += sprintf(page + r, "First = %08x\n", (u32)pDevCtrl->rx_first_bd);
	r += sprintf(page + r, "Last = %08x\n", (u32)pDevCtrl->rx_last_bd);
	r += sprintf(page + r, "New = %08x\n", (u32)pDevCtrl->rx_new_bd);
	r += sprintf(page + r, "Prev New = %08x\n", (u32)pDevCtrl->rx_prev_new_bd);
	r += sprintf(page + r, "Refill = %08x\n", (u32)pDevCtrl->rx_refill_bd);
	r += sprintf(page + r, "\n[BD Ptr(index)] buf_ptr  dst_ptrL dst_ptrH control0 control1 next_low next_hgh reserved\n");
	r += sprintf(page + r, "=======================================================================================\n");
#if 0
	p = pDevCtrl->rx_first_bd;
	for (i = 0; i < pDevCtrl->rx_buf_num; i++) {
		r += sprintf(page + r, "[%08x(%03d)] %08x %08x %08x %08x %08x %08x %08x %08x\n",
					 (u32)p, i,
					 p->localAddr, p->pcieLowerAddr,
					 p->pcieUpperAddr, p->control0,
					 p->control1, p->nextLowerAddr,
					 p->nextUpperAddr, p->reserved);
		p++;
	}
	r += sprintf(page + r, "\n");
#endif
	r += sprintf(page + r, "<===TX BD DUMP===>\n");
	r += sprintf(page + r, "First = %08x\n", (u32)pDevCtrl->tx_first_bd);
	r += sprintf(page + r, "Last = %08x\n", (u32)pDevCtrl->tx_last_bd);
	r += sprintf(page + r, "New = %08x\n", (u32)pDevCtrl->tx_new_bd);
	r += sprintf(page + r, "Used = %08x\n", (u32)pDevCtrl->tx_used_bd);
	r += sprintf(page + r, "Prev New = %08x\n", (u32)pDevCtrl->tx_prev_new_bd);
	r += sprintf(page + r, "tx_used_bd_cnt = %d\n", (u32)pDevCtrl->tx_used_bd_cnt);

	r += sprintf(page + r, "\n[BD Ptr(index)] buf_ptr  src_ptrL src_ptrH control0 control1 next_low next_hgh reserved\n");
	r += sprintf(page + r, "=======================================================================================\n");
#if 0
	p = pDevCtrl->tx_first_bd;
	for (i = 0; i < pDevCtrl->tx_buf_num; i++) {
		r += sprintf(page + r, "[%08x(%03d)] %08x %08x %08x %08x %08x %08x %08x %08x\n",
					 (u32)p, i,
					 p->localAddr, p->pcieLowerAddr,
					 p->pcieUpperAddr, p->control0,
					 p->control1, p->nextLowerAddr,
					 p->nextUpperAddr, p->reserved);
		p++;
	}
	r += sprintf(page + r, "\n");
#endif
	r += sprintf(page + r, "\n");

	*eof = 1;
	return r;
}

int bcmvenet_del_proc_files(struct net_device *dev) {
	remove_proc_entry("driver/bcmvenet/stat", NULL);
	remove_proc_entry("driver/bcmvenet", NULL);
	return 0;
}

int bcmvenet_add_proc_files(struct net_device *dev) {
	BcmVEnet_devctrl *p = netdev_priv(dev);

	proc_mkdir("driver/bcmvenet", NULL);
	create_proc_read_entry("driver/bcmvenet/stat", 0, NULL, bcmvenet_proc_get_stat, p);

	/* PACE Additions */
	create_proc_read_entry("pci0_status", 0, NULL, pci0_proc_read, p);
	/* PACE Additions end */

	return 0;
}

static int bcmvenet_do_init_dev(BcmVEnet_devctrl *pDevCtrl) {
	/*unsigned int ipc_info_pa;*/
	unsigned int tx_bd_pa;
	unsigned int rx_bd_pa;
	unsigned int tx_buf_num;
	unsigned int rx_buf_num;
	pcieBufDesc *p;
	pcieIpcInfo *ptr;
	int i;
	TRACE(("%s\n", __FUNCTION__));

	if (!pDevCtrl->ipc_info_pa) {
		return -1;
	} else {
		ptr = (pcieIpcInfo *)(pDevCtrl->mbase2 + pDevCtrl->ipc_info_pa);
		TRACE(("%s: mem_base = %08x\n", __FUNCTION__, pDevCtrl->mbase2));
		if (!ptr->tx_first_bd || !ptr->num_of_tx_bds || !ptr->rx_first_bd || !ptr->num_of_rx_bds) {
			return -1;
		}
		tx_bd_pa = ptr->rx_first_bd;
		rx_bd_pa = ptr->tx_first_bd;
		rx_buf_num = ptr->num_of_rx_bds;
		tx_buf_num = ptr->num_of_tx_bds;
		if ((tx_buf_num > MAX_VENET_TX_BUF) || (rx_buf_num > MAX_VENET_RX_BUF)) {
			printk(KERN_ERR "bcmvenet: too many TX/RX buffer descriptors!\n");
			return -1;
		}
	}

	pDevCtrl->rx_buf_num = rx_buf_num;
	pDevCtrl->tx_buf_num = tx_buf_num;

	/* The buffer descriptor is in EP's memory space */
	pDevCtrl->tx_first_bd = (pcieBufDesc *)(pDevCtrl->mbase2 + tx_bd_pa);
	pDevCtrl->tx_last_bd = pDevCtrl->tx_first_bd + pDevCtrl->tx_buf_num - 1;
	pDevCtrl->tx_new_bd = (pcieBufDesc *)(pDevCtrl->mbase2 + ptr->rx_new_bd);
	pDevCtrl->tx_prev_new_bd = (pcieBufDesc *)(pDevCtrl->mbase2 + ptr->rx_prev_new_bd);
	pDevCtrl->tx_used_bd = pDevCtrl->tx_new_bd;

	p = pDevCtrl->tx_first_bd;
	TRACE(("[%08x] %08x %08x %08x %08x %08x %08x %08x %08x\n",
		   (u32)p, p->localAddr, p->pcieLowerAddr,
		   p->pcieUpperAddr, p->control0,
		   p->control1, p->nextLowerAddr,
		   p->nextUpperAddr, p->reserved));

	pDevCtrl->rx_first_bd = (pcieBufDesc *)(pDevCtrl->mbase2 + rx_bd_pa);
	pDevCtrl->rx_last_bd = pDevCtrl->rx_first_bd + pDevCtrl->rx_buf_num - 1;
	pDevCtrl->rx_new_bd = (pcieBufDesc *)(pDevCtrl->mbase2 + ptr->tx_new_bd);
	pDevCtrl->rx_prev_new_bd = (pcieBufDesc *)(pDevCtrl->mbase2 + ptr->tx_prev_new_bd);
	pDevCtrl->rx_refill_bd = pDevCtrl->rx_new_bd;

	/* Clear the control block */
	/*memset((void *)pDevCtrl->rx_cb, 0, sizeof(VENET_CB)*pDevCtrl->rx_buf_num); */
	for (i = 0; i < MAX_VENET_RX_BUF; i++) {
		if (pDevCtrl->rx_cb[i].skb) {
			dev_kfree_skb(pDevCtrl->rx_cb[i].skb);
			pDevCtrl->rx_cb[i].skb = NULL;
		}
	}

	TRACE(("%s: int_lvl1=0x%08x\n", __FUNCTION__, (unsigned int)pDevCtrl->int_lvl1));
	TRACE(("%s: int_lvl2=0x%08x\n", __FUNCTION__, (unsigned int)pDevCtrl->int_lvl2));

	TRACE(("%s: tx_first_bd=0x%08x\n", __FUNCTION__, (unsigned int)pDevCtrl->tx_first_bd));
	TRACE(("%s: tx_last_bd=0x%08x\n", __FUNCTION__, (unsigned int)pDevCtrl->tx_last_bd));
	TRACE(("%s: tx_new_bd=0x%08x\n", __FUNCTION__, (unsigned int)pDevCtrl->tx_new_bd));
	TRACE(("%s: tx_prev_new_bd=0x%08x\n", __FUNCTION__, (unsigned int)pDevCtrl->tx_prev_new_bd));
	TRACE(("%s: tx_used_bd=0x%08x\n", __FUNCTION__, (unsigned int)pDevCtrl->tx_used_bd));

	TRACE(("%s: rx_first_bd=0x%08x\n", __FUNCTION__, (unsigned int)pDevCtrl->rx_first_bd));
	TRACE(("%s: rx_last_bd=0x%08x\n", __FUNCTION__, (unsigned int)pDevCtrl->rx_last_bd));
	TRACE(("%s: rx_new_bd=0x%08x\n", __FUNCTION__, (unsigned int)pDevCtrl->rx_new_bd));
	TRACE(("%s: rx_prev_new_bd=0x%08x\n", __FUNCTION__, (unsigned int)pDevCtrl->rx_prev_new_bd));
	TRACE(("%s: rx_refill_bd=0x%08x\n", __FUNCTION__, (unsigned int)pDevCtrl->rx_refill_bd));

	/* Assign the RX ring with buffers */
	assign_rx_buffers(pDevCtrl);

	/* Zero the refill pointer */
	pDevCtrl->rx_refill_bd = NULL;

	/* Update the consumed index */
	*pDevCtrl->rx_consumed_bd = ((u32)pDevCtrl->rx_new_bd - (u32)pDevCtrl->mbase2);

	/* Mark link status */
	pDevCtrl->link = 1;

	/* Pace Additions */
	pDevCtrl->BnmResetDetected = 0;
	/* Pace Additions end */

	/* Write the mailbox */
	if (pDevCtrl->state) {
		pci_write_config_dword(pDevCtrl->pdev, VENDOR_SPECIFIC_REG2, pDevCtrl->link);

		/* Start the network engine */
		netif_start_queue(pDevCtrl->ndev);
		napi_enable(&pDevCtrl->napi);
	}

	/* Now we can take outgoing packets */
	netif_carrier_on(pDevCtrl->ndev);

	/* Enable mbox and data interrupts */
	//pDevCtrl->int_lvl1->IntrMaskClear |= PCIE_INTR_PCIE_INTR;
	pDevCtrl->int_lvl2->PciMaskClear |= L0_RX_DONE_IRQ;

	TRACE(("%s: Link is up, 1Gbps Full Duplex\n", pDevCtrl->ndev->name));
	return 0;
}

/*
 * bcmvenet_init_dev: initialize device
 */
static int bcmvenet_init_dev(BcmVEnet_devctrl *pDevCtrl) {
	int i;

	pDevCtrl->link = 0;

	/* Pace Additions */
	pDevCtrl->BnmResetDetected = 0;

	MBOX_ResetListInit( &pDevCtrl->MboxResetList );
	/* Pace Additions end */

	/* Call the handler */
	/*bcmvenet_do_init_dev(pDevCtrl); */

	/* Add proc file system */
	bcmvenet_add_proc_files(pDevCtrl->ndev);

	/* register block locations */
	TRACE(("%s: dmactl=0x%08x\n", __FUNCTION__, (unsigned int)pDevCtrl->dmactl));

	/* init tx buffer */
	for (i = 0; i < MAX_VENET_TX_BUF; i++) {
		if (pDevCtrl->tx_buf[i] == NULL) {
			pDevCtrl->tx_buf[i] = kmalloc(VENET_RX_BUF_SIZE, GFP_KERNEL);
			BUG_ON(pDevCtrl->tx_buf[i] == NULL);
		}
	}

	/* if we reach this point, we've init'ed successfully */
	return 0;
}

/* Uninitialize tx/rx buffer descriptor pools */
static void bcmvenet_uninit_dev(BcmVEnet_devctrl *pDevCtrl) {
	int i;

	TRACE(("%s\n", __FUNCTION__));

	/* Tell EP about it */
	pDevCtrl->link = 0;
	pci_write_config_dword(pDevCtrl->pdev, VENDOR_SPECIFIC_REG2, pDevCtrl->link);

	/* Sleep 100 ms for DMA to complete */
	msleep(100);

	/* Remove the proc file system entry */
	bcmvenet_del_proc_files(pDevCtrl->ndev);

	/* Free the RX SKB's */
	for (i = 0; i < MAX_VENET_RX_BUF; i++) {
		if (pDevCtrl->rx_cb[i].skb) {
			dev_kfree_skb(pDevCtrl->rx_cb[i].skb);
		}
	}

	/* Free the TX buffer */
	for (i = 0; i < MAX_VENET_TX_BUF; i++) {
		if (pDevCtrl->tx_buf[i]) {
			kfree(pDevCtrl->tx_buf[i]);
		}
	}
}

/*
 * ethtool function - get driver info.
 */
static void bcmvenet_get_drvinfo(struct net_device *dev, struct ethtool_drvinfo *info) {
	strncpy(info->driver, CARDNAME, sizeof(info->driver));
	strncpy(info->version, VER_STR, sizeof(info->version));
}

/*
 * standard ethtool support functions.
 */
static struct ethtool_ops bcmvenet_ethtool_ops = {
	.get_drvinfo		= bcmvenet_get_drvinfo,
	.get_link			= ethtool_op_get_link,
};

static inline void copy_words(void *to, void *from, unsigned len, int swap) {
	volatile uint32 *f, *t;
    len = (len + 3) / 4;
	f = from;
	t = to;
    f += len;
    t += len;
	if (swap) {
		while (len--) {
            uint32 v = *(--f);
			*(--t) = htonl(v);
		}
	} else {
		while (len--) {
			*(--t) = *(--f);
		}
	}
}


/*
 * ioctl handle special commands that are not present in ethtool.
 */
static int bcmvenet_ioctl(struct net_device *dev, struct ifreq *rq, int cmd) {
	BcmVEnet_devctrl *pDevCtrl = netdev_priv(dev);
	BcmVEnet_ioctlparms *ioctlparms;
	unsigned int from;
	unsigned int to;

	/* we can add sub-command in ifr_data if we need to in the future */
	switch (cmd) {
	case SIOCGETMBOX: {
		unsigned int offset = 0;
		if (pDevCtrl->hostbootMail) {
			offset = pDevCtrl->hostbootMail;
			pDevCtrl->hostbootMail = 0;
		}
		copy_words(rq->ifr_data, (void *)&offset, sizeof(offset), 0);
	break;
	}

	case SIOCGETWINDOW:
		ioctlparms = (void *)rq->ifr_data;
		if (ioctlparms->from >= 0x10000000) {
			from = ioctlparms->from + pDevCtrl->base_offset;
		} else {
			from = ioctlparms->from + pDevCtrl->mbase2;
		}
		copy_words((void *)ioctlparms->to, (void *)from, ioctlparms->length, 0);
		break;

	case SIOCPUTWINDOW: {
		ioctlparms = (void *)rq->ifr_data;
		if (ioctlparms->to >= 0x10000000) {
			to = ioctlparms->to + pDevCtrl->base_offset;
		} else {
			to = ioctlparms->to + pDevCtrl->mbase2;
		}
		if (ioctlparms->length > (64 * 1024)) {
			ioctlparms->length = 64 * 1024;
		}
		copy_words((void *)to, (void *)ioctlparms->from, ioctlparms->length, 1);
	break;
	}

	default:
		return -1;
	}
	return 0;
}
static const struct net_device_ops bcmvenet_netdev_ops = {
	.ndo_open = bcmvenet_open,
	.ndo_stop = bcmvenet_close,
	.ndo_start_xmit = bcmvenet_xmit,
	.ndo_tx_timeout = bcmvenet_timeout,
	.ndo_set_multicast_list = bcmvenet_set_multicast_list,
	.ndo_set_mac_address = bcmvenet_set_mac_addr,
	.ndo_do_ioctl = bcmvenet_ioctl,
};

/* PACE Additions */

/* parse MAC address from kernel command line */
static int __init bcmemac_parse_mac_addr(const char *val, struct kernel_param *kp)
{
    int x, i ;

    for (i=0; i<ETH_ALEN; i++) {      
        if ( sscanf(val, "%02x", &x ) != 1) {
            break ;
        }
        bcmemac_cmdline_macaddr[i] = x ;
        val += 2 ;
        if (*val == ':' || *val == '-') {
            val++ ;
        }
    }

    if (i != ETH_ALEN ) {
        printk("Failed to parse MAC address\n");
        memset( bcmemac_cmdline_macaddr, 0, sizeof bcmemac_cmdline_macaddr ) ;
    }

    printk("bcmemac_parse_mac_addr: %x:%x:%x:%x:%x:%x\n",
           bcmemac_cmdline_macaddr[0], bcmemac_cmdline_macaddr[1], bcmemac_cmdline_macaddr[2],
           bcmemac_cmdline_macaddr[3], bcmemac_cmdline_macaddr[4], bcmemac_cmdline_macaddr[5]);

    return 0 ;
}

/* link up module parameter to the parse function */
module_param_call(macaddr, bcmemac_parse_mac_addr, NULL, NULL, 000 ) ;
MODULE_PARM_DESC(macaddr, "MAC address for PCI ethernet "
                 "example bcmvenet.macaddr=00:c0:34:54:22:10 or,"
                 "when using insmod, insmod macadd=00:c0:34:54:22:10 bcmvenet");

/* PACE Additions end */

static int __devinit bcmvenet_drv_probe(struct pci_dev *pdev, const struct pci_device_id *pid) {
	int err = 0;
    int is3383bx;
	resource_size_t bar_base, bar_len;
	resource_size_t bar0_base, bar0_len;
	BcmVEnet_devctrl *pDevCtrl;
	struct net_device *ndev;

	TRACE(("%s...\n", __FUNCTION__));
	/* Enable memory region */
	err = pci_enable_device_mem(pdev);
	if (err) {
		printk(KERN_ERR "bcmvenet: can't enable device memory\n");
		return err;
	}

	/* Set DMA mask ?? */

	/* Request a memory region */
	err = pci_request_selected_regions_exclusive(pdev,
			pci_select_bars(pdev, IORESOURCE_MEM),
			bcmvenet_drv_name);
	if (err) {
		printk(KERN_ERR "bcmvenet: failure requesting device memory\n");
		goto err_request_selected_region;
	}

#if defined(CONFIG_PCIEAER)
	/* Enable error reporting */
	pci_enable_pcie_error_reporting(pdev);
#endif

	/* Set to master */
	pci_set_master(pdev);

	/* Allocate driver data structure */
	/* PACE CHANGE: use different device name instead of bcm0 */
	ndev = alloc_netdev_mq(sizeof(*pDevCtrl), CONFIG_PACE_PCIDEV_NAME, ether_setup, 1);
	if (ndev == NULL) {
		printk(KERN_ERR "bcmvenet: can't allocate net device\n");
		err = -ENOMEM;
		goto err_alloc_netdev;
	}

	SET_NETDEV_DEV(ndev, &pdev->dev);

	/* Get a pointer to my contrl info */
	pDevCtrl = (BcmVEnet_devctrl *)netdev_priv(ndev);

	/* Map BAR 0 for register access */
	bar0_base = pci_resource_start(pdev, 0);
	bar0_len = pci_resource_len(pdev, 0);
	pDevCtrl->rbase0 = (unsigned int)ioremap(bar0_base, bar0_len);
	if (!pDevCtrl->rbase0) {
		printk(KERN_ERR "bcmvenet: failure mapping BAR 0\n");
		goto err_bar0_ioremap;
	}

	bar_base = pci_resource_start(pdev, 2);
	bar_len = pci_resource_len(pdev, 2);
	pDevCtrl->mbase2 = (unsigned int)ioremap(bar_base, bar_len);
	if (!pDevCtrl->mbase2) {
		printk(KERN_ERR "bcmvenet: failure mapping BAR 2\n");
		goto err_bar2_ioremap;
	}

	TRACE(("%s : BAR2 (%08x@%08x->%08x)\n", __FUNCTION__, (unsigned int)bar_len, (unsigned int)bar_base, (unsigned int)pDevCtrl->mbase2));

	/* Test register mapping */
	TRACE(("%s : %s REVID = %08x\n", __FUNCTION__, bcmvenet_drv_name, *(volatile unsigned int *)(pDevCtrl->rbase0 + 0x04e00000)));
	TRACE(("%s : %s PCIE REVID = %08x\n", __FUNCTION__, bcmvenet_drv_name, *(volatile unsigned int *)(pDevCtrl->rbase0 + 0x02a0406c)));
	TRACE(("%s : %s DDR (0x00000000) = %08x\n", __FUNCTION__, bcmvenet_drv_name, *(volatile unsigned int *)(pDevCtrl->mbase2 + 0x00000000)));
	TRACE(("%s : %s DDR (0x033de9e0) = %08x\n", __FUNCTION__, bcmvenet_drv_name, *(volatile unsigned int *)(pDevCtrl->mbase2 + 0x033de9e0)));
	TRACE(("%s : %s DDR (0x07f00000) = %08x\n", __FUNCTION__, bcmvenet_drv_name, *(volatile unsigned int *)(pDevCtrl->mbase2 + 0x07f00000)));

	bar_base = pci_resource_start(pdev, 4);
	bar_len = pci_resource_len(pdev, 4);
	pDevCtrl->mbase4 = (unsigned int)ioremap(bar_base, bar_len);
	if (!pDevCtrl->mbase4) {
		is3383bx = 0;
		pDevCtrl->int_lvl1 = (PcieIntr1Registers *)(pDevCtrl->rbase0 + 0x02a08200);
		pDevCtrl->int_lvl2 = (PcieIntr2Registers *)(pDevCtrl->rbase0 + 0x02a04300);
		pDevCtrl->inMbox = (unsigned long *)(pDevCtrl->rbase0 + 0x02a02198);
		pDevCtrl->outMbox = (unsigned long *)(pDevCtrl->rbase0 + 0x02a0219c);
		pDevCtrl->altOutMbox = (unsigned long *)(pDevCtrl->rbase0 + 0x02a021a4);
		pDevCtrl->tx_consumed_bd = (unsigned int *)(pDevCtrl->rbase0 + 0x02a02190);
		pDevCtrl->rx_consumed_bd = (volatile unsigned int *)(pDevCtrl->rbase0 + 0x02a02194);
		pDevCtrl->dmactl = (PcieDmaReg *)(pDevCtrl->rbase0 + 0x02a04400);
		pDevCtrl->base_offset = pDevCtrl->rbase0 - 0x10000000;
	} else {
		is3383bx = 1;
		pDevCtrl->int_lvl1 = (PcieIntr1Registers *)(pDevCtrl->rbase0 + 0x8200);
		pDevCtrl->int_lvl2 = (PcieIntr2Registers *)(pDevCtrl->rbase0 + 0x4300);
		pDevCtrl->inMbox = (unsigned long *)(pDevCtrl->rbase0 + 0x2198);
		pDevCtrl->outMbox = (unsigned long *)(pDevCtrl->rbase0 + 0x219c);
		pDevCtrl->altOutMbox = (unsigned long *)(pDevCtrl->rbase0 + 0x21a4);
		pDevCtrl->tx_consumed_bd = (unsigned int *)(pDevCtrl->rbase0 + 0x2190);
		pDevCtrl->rx_consumed_bd = (volatile unsigned int *)(pDevCtrl->rbase0 + 0x2194);
		pDevCtrl->dmactl = (PcieDmaReg *)(pDevCtrl->rbase0 + 0x4400);
		pDevCtrl->base_offset = pDevCtrl->mbase4 - 0x13050000;
	}
	printk(KERN_INFO CARDNAME ": Initializing for %s%cx endpoint\n", bcmvenet_drv_name, is3383bx ? 'B' : 'A');

	/* Set up the int control register */

	TRACE(("%s : BAR0 (%08x@%08x->%08x)\n", __FUNCTION__, (unsigned int)bar0_len, (unsigned int)bar0_base, (unsigned int)pDevCtrl->rbase0));

	pDevCtrl->pdev = pdev;

	pci_set_drvdata(pdev, ndev);

#if 0   /* PACE Change - removed this, MAC address comes from kernel command line */
	brcm_alloc_macaddr((u8 *)ndev->dev_addr);
#else
	if ( ( bcmemac_cmdline_macaddr[0] == 0 &&
	       bcmemac_cmdline_macaddr[1] == 0 &&
	       bcmemac_cmdline_macaddr[2] == 0 &&
	       bcmemac_cmdline_macaddr[3] == 0 &&
	       bcmemac_cmdline_macaddr[4] == 0 &&
	       bcmemac_cmdline_macaddr[5] == 0 ) ||
	     ( bcmemac_cmdline_macaddr[0] == 0xff &&
	       bcmemac_cmdline_macaddr[1] == 0xff &&
	       bcmemac_cmdline_macaddr[2] == 0xff &&
	       bcmemac_cmdline_macaddr[3] == 0xff &&
	       bcmemac_cmdline_macaddr[4] == 0xff &&
	       bcmemac_cmdline_macaddr[5] == 0xff ) )
	{
		printk("%s - invalid MAC address, using default value\n", __FUNCTION__);
		ndev->dev_addr[0] = 0x00;
		ndev->dev_addr[1] = 0xc0;
		ndev->dev_addr[2] = 0xa8;
		ndev->dev_addr[3] = 0x74;
		ndev->dev_addr[4] = 0x3b;
		ndev->dev_addr[5] = 0x52;
	}
	else
	{
	    memcpy(ndev->dev_addr,  bcmemac_cmdline_macaddr, ETH_ALEN);
	}
#endif
	/* PACE Change end */

    ndev->irq = pdev->irq;
	ndev->watchdog_timeo         = 2 * HZ;
	SET_ETHTOOL_OPS(ndev, &bcmvenet_ethtool_ops);
	ndev->netdev_ops = &bcmvenet_netdev_ops;
	netif_napi_add(ndev, &pDevCtrl->napi, bcmvenet_poll, 64);

	pDevCtrl->ndev = ndev;
	pDevCtrl->irq = pdev->irq;

	/*
	strncpy(netdev->name, pci_name(pdev), sizeof(ndev->name) - 1);
	*/

	ndev->mem_start = bar0_base;
	ndev->mem_end = bar0_base + bar0_len;

	/* Initialize the locks */
	spin_lock_init(&pDevCtrl->lock);
	spin_lock_init(&pDevCtrl->bh_lock);

	TRACE(("%s: irq=%d\n", __FUNCTION__, (unsigned int)pdev->irq));

	/* Init the tasklet */
	pDevCtrl->mtask.next = NULL;
	pDevCtrl->mtask.state = 0;
	atomic_set(&pDevCtrl->mtask.count, 0);
	pDevCtrl->mtask.func = mbox_tasklet;
	pDevCtrl->mtask.data = (unsigned long)pDevCtrl;

	/* Init registers, Tx/Rx buffers */
	if (bcmvenet_init_dev(pDevCtrl) < 0) {
		printk(KERN_ERR "can't initialize device\n");
		goto err_init_dev;
	}

	pDevCtrl->hostbootMail = 0;

	/* Request an interrupt */
	if (request_irq(pDevCtrl->pdev->irq, bcmvenet_isr, (IRQF_SHARED), ndev->name, pDevCtrl) < 0) {
		printk(KERN_ERR "can't request IRQ %d\n", pDevCtrl->pdev->irq);
		goto err_request_irq;
	}

	netif_carrier_off(pDevCtrl->ndev);

	/* Get the PCIE capability pointer */
	if ((pDevCtrl->pcie_cfg_cap = pci_find_capability(pdev, PCI_CAP_ID_EXP)) == 0) {
		printk(KERN_ERR "can't get PCIE capability position!\n");
		goto err_capability;
	} else {
		u16 val;
		u32 val32;

		/* Set the MPS and MRRS to 256B */
		err = pci_read_config_word(pdev, pDevCtrl->pcie_cfg_cap + PCI_EXP_DEVCTL, &val);
		if (err) {
			printk(KERN_ERR "can't read device control status!\n");
			goto err_capability;
		}

		/*
		val &= ~(PCI_EXP_DEVCTL_PAYLOAD | PCI_EXP_DEVCTL_READRQ);
		val |= 0x1020;
		*/
		val &= ~(PCI_EXP_DEVCTL_PAYLOAD);
		val |= 0x20;
		pci_write_config_word(pdev, pDevCtrl->pcie_cfg_cap + PCI_EXP_DEVCTL, val);

		val32 = BDEV_RD(BCHP_PCIE_RC_CFG_PCIE_DEVICE_STATUS_CONTROL);
		val32 &= 0xFFFFFF1F;
		val32 |= (1 << 5);
		BDEV_WR(BCHP_PCIE_RC_CFG_PCIE_DEVICE_STATUS_CONTROL, val32);
	}


	/* Register an Ethernet device with kernel */
	if ((err = register_netdev(ndev)) != 0) {
		printk(KERN_ERR "can't register netdev\n");
		goto err_register;
	}

	/* Enable interrupts */
	pDevCtrl->int_lvl1->IntrMaskClear |= PCIE_INTR_PCIE_INTR;
	pDevCtrl->int_lvl2->PciMaskClear |= MBOX0_IRQ;

	return(0);

err_register:
	free_irq(pDevCtrl->pdev->irq, pDevCtrl);
err_capability:
err_request_irq:
	bcmvenet_uninit_dev(pDevCtrl);
err_init_dev:
	if (pDevCtrl->mbase4) {
		iounmap((u8 __iomem *)pDevCtrl->mbase4);
	}
	iounmap((u8 __iomem *)pDevCtrl->mbase2);
err_bar2_ioremap:
	iounmap((u8 __iomem *)pDevCtrl->rbase0);
err_bar0_ioremap:
	free_netdev(ndev);
err_alloc_netdev:
	pci_disable_device(pdev);
	pci_release_selected_regions(pdev,
								 pci_select_bars(pdev, IORESOURCE_MEM));
err_request_selected_region:
	return(err);
}

/**
 * This routine will be called by PCI subsystem to release a PCI device.
 * For example, before the driver is removed from memory
 **/
static void __devexit bcmvenet_drv_remove(struct pci_dev *pdev) {
	struct net_device *ndev = pci_get_drvdata(pdev);
	BcmVEnet_devctrl *pDevCtrl = netdev_priv(ndev);

	/* Disable mbox and data interrupts */
	pDevCtrl->int_lvl2->PciMaskSet |= (L0_RX_DONE_IRQ  | MBOX0_IRQ);
	pDevCtrl->int_lvl2->PciClear |= (L0_RX_DONE_IRQ  | MBOX0_IRQ);

	/* Un-initialize everything H/W related */
	bcmvenet_uninit_dev(pDevCtrl);

	/* Unregister the network device */
	unregister_netdev(pDevCtrl->ndev);

	/* Free the interrupt */
	free_irq(pDevCtrl->pdev->irq, pDevCtrl);

	/* Send READY notification */
	mb_send(MBOX_REBOOTING, 0, pDevCtrl);

#if !( LINUX_VERSION_CODE < KERNEL_VERSION(2,6,32) )
	/*
	if (pci_dev_run_wake(pdev))
		pm_runtime_get_noresume(&pdev->dev);
	    */
#endif

	/* Free the memory region */
	pci_release_selected_regions(pdev, pci_select_bars(pdev, IORESOURCE_MEM));

	/* Unmap the register and memory space */
	if (pDevCtrl->mbase4) {
		iounmap((u8 __iomem *)pDevCtrl->mbase4);
	}
	iounmap((u8 __iomem *)pDevCtrl->mbase2);
	iounmap((u8 __iomem *)pDevCtrl->rbase0);

	/* Free the data structure */
	free_netdev(pDevCtrl->ndev);

	/* Disable error reporting */
	pci_disable_pcie_error_reporting(pdev);

	/* Now disable the device */
	/*pci_disable_device(pdev);*/

	return;
}

static pci_ers_result_t bcmvenet_drv_error_detected(struct pci_dev *pdev, pci_channel_state_t state) {
	struct net_device *ndev = pci_get_drvdata(pdev);
	/*
	BcmVEnet_devctrl *pDevCtrl = netdev_priv(ndev);
	*/

	if (netif_running(ndev)) {
		/* TODO: stop the traffic ??? */
	}
	pci_disable_device(pdev);
	return PCI_ERS_RESULT_NEED_RESET;
}

static pci_ers_result_t bcmvenet_drv_slot_reset(struct pci_dev *pdev) {
	return PCI_ERS_RESULT_RECOVERED;
}

static void bcmvenet_drv_error_resume(struct pci_dev *pdev) {
}

/* Error recovery */
static struct pci_error_handlers bcmvenet_drv_err_handler = {
	.error_detected = bcmvenet_drv_error_detected,
	.slot_reset = bcmvenet_drv_slot_reset,
	.resume = bcmvenet_drv_error_resume,
};

static DEFINE_PCI_DEVICE_TABLE(bcmvenet_drv_tbl) = {
	{ BCM3383VENDOR, BCM3383DEVICE, PCI_ANY_ID, PCI_ANY_ID, 0, 0, 0},
	{ }	/* Null termination */
};
MODULE_DEVICE_TABLE(pci, bcmvenet_drv_tbl);

/* I am a PCI driver */
static struct pci_driver bcmvenet_driver = {
	.name     = bcmvenet_drv_name,
	.id_table = bcmvenet_drv_tbl,
	.probe    = bcmvenet_drv_probe,
	.remove   = __devexit_p(bcmvenet_drv_remove),
#ifdef CONFIG_PM_OPS
#if ( LINUX_VERSION_CODE < KERNEL_VERSION(2,6,34) )
	.suspend  = bcmvenet_drv_suspend,
	.resume   = bcmvenet_drv_resume,
#else
	/*.driver.pm = &bcmvenet_drv_pm_ops,*/
#endif /* ( LINUX_VERSION_CODE < KERNEL_VERSION(2,6,34) ) */
#endif
	.err_handler = &bcmvenet_drv_err_handler
};

static int __init bcmvenet_module_init(void) {
    int err = 0;
	TRACE(("bcmvenet_module_init() ...\n"));
    /* print the ChipID and module version info */

    printk(KERN_INFO CARDNAME ": Broadcom BCM3383 Virtual Ethernet Driver " VER_STR "\n");

    if (pci_get_device(BCM3383VENDOR, BCM3383DEVICE, NULL)) {
        if ((err = pci_register_driver(&bcmvenet_driver))) {
            printk(KERN_ERR CARDNAME ": Error registering driver\n");
        }
    } else {
        printk(KERN_ERR CARDNAME ": Can't find %04x ECM device\n", BCM3383DEVICE);
        err = -1;
    }
    return err;
}
module_init(bcmvenet_module_init);

static void __exit bcmvenet_module_exit(void) {
	TRACE(("bcmvenet_module_exit() ...\n"));
	pci_unregister_driver(&bcmvenet_driver);
}
module_exit(bcmvenet_module_exit);

MODULE_AUTHOR("Broadcom Corporation");
MODULE_DESCRIPTION("BCM" BCM3383DEVICETEXT " PCIe Virtual Network Driver");
MODULE_LICENSE("Proprietary");
MODULE_VERSION(VERSION);
