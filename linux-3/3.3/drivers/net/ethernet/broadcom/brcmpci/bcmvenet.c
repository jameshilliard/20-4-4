/****************************************************************************
*
*  Copyright (c) 2011-2013 Broadcom Corporation
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
#define VERSION     "2.1"
#define VER_STR     "v" VERSION " " __DATE__ " " __TIME__

#if defined(CONFIG_MODVERSIONS) && ! defined(MODVERSIONS)
#include <config/modversions.h>
#define MODVERSIONS
#endif

#include <linux/kernel.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/proc_fs.h>
#include <linux/pci.h>
#include <linux/aer.h>
#include <linux/version.h>
#if ( LINUX_VERSION_CODE > KERNEL_VERSION(3,3,0) )
#include <linux/brcmstb/brcmstb.h>
#include <linux/brcmstb/brcmapi.h>
#include <linux/interrupt.h>
#include <linux/module.h>
#else
#include <asm/brcmstb/brcmstb.h>
#include <asm/brcmstb/brcmapi.h>
#endif
#include <linux/spinlock.h>

#include "bcmvenet.h"

#define ENET_MAX_MTU_SIZE 1536    /* Body(1500) + EH_SIZE(14) + VLANTAG(4) + BRCMTAG(6) + FCS(4) = 1528.  1536 is multiple of 256 bytes */
#define uint32 u32

#define BCM3383DEVICE 0x3383
#define BCM3384DEVICE 0x3384
#define BCM33843DEVICE 0x3843
#define BCMVENDOR 0x14e4

#define MAX_NET_DEVICES 3

//#define DUMP_TRACE

#if defined(DUMP_TRACE)
#define TRACE(x)        printk x
#else
#define TRACE(x)
#endif

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
#ifdef CONFIG_PACESTB
#define MAX_VENET_RX_BUF 1024
#define MAX_VENET_TX_BUF 1024
#else
#define MAX_VENET_RX_BUF 256
#define MAX_VENET_TX_BUF 256
#endif

#define VENET_CACHE_LINE_SIZE 32
#define FPM_TOKEN_MASK        0x7ffff000
#define FPM_TOKEN_VALID_MASK  0x80000000
#define FPM_TOKEN_INDEX_SHIFT 12
#define FPM_TOKEN_SIZE 0x800

#define BCHP_PCIE_RC_CFG_PCIE_DEVICE_STATUS_CONTROL 0x004100b4 /* Device status control */

#define skb_dataref(x)   (&skb_shinfo(x)->dataref)

// Number of virtual devices supported, defaults to 1 for standard opreration
static unsigned short devs = 1;
module_param(devs, ushort, S_IRUGO);

typedef struct
{
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

typedef struct
{
    uint32 IntrStatus;
    uint32 IntrMaskStatus;
    uint32 IntrMaskSet;
    uint32 IntrMaskClear;
}  PcieIntr1Registers;

#define PCIE_UBUS_PCIE_INTR (1<<7)
#define PCIE_INTR_PCIE_INTR (1<<5)

typedef struct
{
    uint32 CpuStatus;
    uint32 CpuSet;
    uint32 CpuClear;
    uint32 CpuMaskStatus;
    uint32 CpuMaskSet;
    uint32 CpuMaskClear;
    uint32 PciStatus;
    uint32 PciSet;
    uint32 PciClear;
    uint32 PciMaskStatus;
    uint32 PciMaskSet;
    uint32 PciMaskClear;
}  PcieIntr2Registers;

typedef struct s_pcieBufDesc
{
    uint32 localAddr;
    uint32 pcieLowAddr;
    uint32 pcieHighAddr;
    uint32 control0;
    uint32 control1; // bits 15:8 used as tag
    uint32 nextLowAddr;
    uint32 nextHighAddr;
    uint32 reserved;
} pcieBufDesc;

typedef struct s_venet_cb
{
    struct sk_buff *skb;
    dma_addr_t      pa;
} VENET_RX_CB;

typedef struct
{
    uint32 tx_first_bd;
    uint32 tx_new_bd;
    uint32 tx_prev_new_bd;
    uint32 num_of_tx_bds;
    uint32 rx_first_bd;
    uint32 rx_new_bd;
    uint32 rx_prev_new_bd;
    uint32 num_of_rx_bds;
} pcieIpcInfo;

static struct pci_dev *pcidev;				/* ptr to the physical device */

static spinlock_t tx_buf_lock;				/* Serializing tx_buf_lock */
static spinlock_t rx_buf_lock;				/* Serializing tx_buf_lock */

static struct napi_struct napi;
static unsigned int rbase0;                /* SUB register start address. */
static unsigned int mbase2;                /* SUB register start address. */
static unsigned int mbase4;                /* SUB register start address. */
static unsigned int base_offset;
static volatile PcieDmaReg *dmactl;        /* SUB DMA register block base address */
static volatile PcieIntr1Registers *int_lvl1; /* SUB DMA register block base address */
static volatile PcieIntr2Registers *int_lvl2; /* SUB DMA register block base address */
static volatile unsigned long *inMbox;
static volatile unsigned long *altOutMbox;

static pcieBufDesc *tx_first_bd;
static pcieBufDesc *tx_last_bd;
static pcieBufDesc *tx_new_bd;
static pcieBufDesc *tx_used_bd;
static pcieBufDesc *tx_prev_new_bd;
static volatile unsigned int *tx_consumed_bd;

static pcieBufDesc *rx_first_bd;
static pcieBufDesc *rx_last_bd;
static pcieBufDesc *rx_new_bd;
static pcieBufDesc *rx_refill_bd;
static volatile unsigned int *rx_consumed_bd;

static pcieBufDesc *rx_prev_new_bd;

static unsigned int tx_used_bd_cnt;
static unsigned int xmit_buf_num;
static unsigned int recv_buf_num;

static VENET_RX_CB rx_cb[MAX_VENET_RX_BUF];
static unsigned int *tx_buf[MAX_VENET_TX_BUF];

static struct tasklet_struct mtask;        /* Task to process mbox messages */
static int dmaReady;              /* dma ready status */

static unsigned int rx_cnt;                /* Packets successfully received */
static unsigned int tx_cnt;                /* Packets successfully transmitted */
static unsigned int hostbootMail;
static unsigned int ipc_info_pa;
static int pcie_cfg_cap;

#ifdef CONFIG_PACESTB
static int BnmResetDetected;
static MBOX_ResetList MboxResetList;
#endif

static bool is3383 = false;

typedef struct BcmVEnet_devctrl
{
    unsigned int devIndex;					/* ptr to net_device */
} BcmVEnet_devctrl;

static struct net_device *ndevs[MAX_NET_DEVICES];

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

// --------------------------------------------------------------------------
//      dev->poll() method
// --------------------------------------------------------------------------
static int bcmvenet_poll(struct napi_struct *napi, int budget);
// --------------------------------------------------------------------------
//      Process recived packet for descriptor based DMA
// --------------------------------------------------------------------------
#ifdef CONFIG_PACESTB
static unsigned int bcmvenet_rx(unsigned int budget, int *dma_busy);
#else
static unsigned int bcmvenet_rx(unsigned int budget);
#endif

// --------------------------------------------------------------------------
//      Internal routines
// --------------------------------------------------------------------------
/* Allocate and initialize tx/rx buffer descriptor pools */
static int bcmvenet_init_dev(void);
static void bcmvenet_uninit_dev(void);

//#define DUMP_DATA
#if defined(DUMP_DATA)
/* Display hex base data */
static void dumpHexData(char *typ, unsigned char *head, int len);
/* dumpMem32 dump out the number of 32 bit hex data  */
static void dumpMem32(unsigned int *pMemAddr, int iNumWords);
#endif

static int bcmvenet_do_init_dev(void);
static void mbox_tasklet(unsigned long arg);
/*static void bcmvenet_rtimer_handler(unsigned long arg);*/
static char bcmvenet_drv_name[8];

#ifdef CONFIG_PACESTB
static char ecm_Ready=0;

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
   MBOX_ResetListInsert( &MboxResetList, func, user_data );
}
EXPORT_SYMBOL(MBOX_InsertResetListener);

void MBOX_RemoveResetListener( MBOX_RESET_FUNC func )
{
   MBOX_ResetListRemove( &MboxResetList, func );
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

#endif /* CONFIG_PACESTB */

// --------------------------------------------------------------------------
//      Miscellaneous variables
// --------------------------------------------------------------------------
/*static struct kmem_cache * skb_head_cache __read_mostly;*/

#if defined(DUMP_DATA)
/*
 * dumpHexData dump out the hex base binary data
 */
static void dumpHexData(char *typ, unsigned char *head, int len)
{
    int i;
    unsigned char *curPtr = head;

    printk("%s (%d):", typ, len);

    for (i = 0; i < len; ++i)
    {
        if ((i & 3) == 0)
        {
            printk(" ");
        }
        printk("%02x", *curPtr++);
    }
    printk("\n");
}

/*
 * dumpMem32 dump out the number of 32 bit hex data
 */
static void dumpMem32(unsigned int *pMemAddr, int iNumWords)
{
    int i = 0;
    static char buffer[80];

    sprintf(buffer, "%08X: ", (unsigned int)pMemAddr);
    printk(buffer);
    while (iNumWords)
    {
        sprintf(buffer, "%08X ", (unsigned int)*pMemAddr++);
        printk(buffer);
        iNumWords--;
        i++;
        if ((i % 4) == 0 && iNumWords)
        {
            sprintf(buffer, "\n%08X: ", (unsigned int)pMemAddr);
            printk(buffer);
        }
    }
    printk("\n");
}
#endif

void mb_send(unsigned int msg, unsigned int data, void *arg)
{

    unsigned int val;

    val = msg & MBOX_MSG_MASK;
    val |= (data & (~MBOX_MSG_MASK));
    *(altOutMbox) = val;
    int_lvl2->CpuSet |= MBOX3_IRQ;
}

/***********************************************************************/
/*   mbox_tasklet() -                                              */
/*                                                                     */
/*   Process MailBox receive interrupt                             */
/*                                                                     */
/***********************************************************************/
static void mbox_tasklet(unsigned long arg)
{

    unsigned int mailbox = *inMbox;

    /* Read the mbox message */
    switch (mailbox & MBOX_MSG_MASK)
    {
    case MBOX_NULL:
#ifndef CONFIG_PACESTB
    case MBOX_XFER_BLOCK:
#endif
        hostbootMail = mailbox;
        break;

    case MBOX_REBOOTING:
        if (dmaReady)
        {
            int ndIx;
            dmaReady = 0;
#ifdef CONFIG_PACESTB            
            ecm_Ready = 0;
#endif            
            printk(KERN_INFO CARDNAME ": ECM rebooting!\n");
            // First tell the EP DMA engine to stop
            dmactl->TxSwDescListCtrlSts &= ~0x1;
            dmactl->RxSwDescListCtrlSts &= ~0x1;
            // Ok, turn off our end
            for (ndIx = 0; ndIx < devs; ndIx++)
            {
                if (netif_carrier_ok(ndevs[ndIx]))
                {
                    netif_carrier_off(ndevs[ndIx]);
                    printk("%s: Link is down\n", ndevs[ndIx]->name);
                }
                if (!netif_queue_stopped(ndevs[ndIx]))
                {
                    netif_stop_queue(ndevs[ndIx]);
                }
            }
            napi_disable(&napi);
        }
        break;

    case MBOX_DMA_READY:
#ifdef CONFIG_PACESTB
        /* Pace Changes start */
        /* Ignore any DMA Ready after it has been seen once, See below for coments */
        if(0 == ecm_Ready) 
        {
#endif
        if (!dmaReady)
        {
            printk(KERN_INFO CARDNAME ": ECM ready!\n");
            ipc_info_pa = mailbox & (~MBOX_MSG_MASK);
            TRACE(("%s: ipc_info_pa = %08x\n", __FUNCTION__, ipc_info_pa));
            bcmvenet_do_init_dev();
        }
#ifdef CONFIG_PACESTB
        MBOX_ResetListProcess( &MboxResetList, BnmResetDetected == 1 ? 1 : 0 );
#endif
#ifdef CONFIG_PACESTB
            ecm_Ready = 1;
        }
#endif
        break;

#ifdef CONFIG_PACESTB
        /* For TiVo builds, the eCM sends the MBOX_DMA_READY several times for parallel boot
         * This is since the kernel is booted of the Disk & accounting for Disk latencies
         * The bcmvenet driver may not have been setup in time to receive the Ready
         * Additionally MBOX_XFER_BLOCK is not expected anytime post DMA Ready and
         * This is used to catch the uncontrolled eCM resets */
    case MBOX_XFER_BLOCK:
        hostbootMail = mailbox;
        if(ecm_Ready) /* catch an unhandled eCM reset */
        {
          MBOX_ResetListProcess( &MboxResetList, 1 ); 
        }
          /* Pace Changes end */
          break;
#endif
    
    default:
        printk(KERN_ERR CARDNAME ": Unknown mailbox command (%08x)!\n", mailbox);
        break;
    }

    /* Enable mailbox interrupt at L2 */
    int_lvl2->PciMaskClear |=  MBOX0_IRQ;

}

/* --------------------------------------------------------------------------
    Name: bcmvenet_open
 Purpose: Open and Initialize the SUB DMA on the chip
-------------------------------------------------------------------------- */
static int bcmvenet_open(struct net_device *dev)
{

    TRACE(("%s: bcmvenet_open\n", dev->name));
#ifdef CONFIG_PACESTB
    BnmResetDetected = 0;
#endif

    // startup the interface
    if (dmaReady)
    {
        if (netif_queue_stopped(dev))
        {
            netif_start_queue(dev);
        }
        if (!netif_carrier_ok(dev))
        {
            netif_carrier_on(dev);
            printk("%s: Link is up, 1Gbps Full Duplex\n", dev->name);
        }
    }
    return 0;
}


/* --------------------------------------------------------------------------
    Name: bcmvenet_close
    Purpose: Stop communicating with the outside world
    Note: Caused by 'ifconfig ethX down'
-------------------------------------------------------------------------- */
static int bcmvenet_close(struct net_device *dev)
{

    TRACE(("%s: bcmvenet_close\n", dev->name));

    // Stop the interface
    if (dmaReady)
    {
        if (!netif_queue_stopped(dev))
        {
            netif_stop_queue(dev);
        }
        if (netif_carrier_ok(dev))
        {
            netif_carrier_off(dev);
            printk("%s: Link is down\n", dev->name);
        }
    }

    return 0;
}

/* --------------------------------------------------------------------------
    Name: bcmvenet_net_timeout
 Purpose:
-------------------------------------------------------------------------- */
static void bcmvenet_timeout(struct net_device *dev)
{

    TRACE(("%s: bcmvenet_timeout\n", dev->name));

    dev->trans_start = jiffies;
    dev->stats.tx_errors++;
    netif_wake_queue(dev);
}

/* --------------------------------------------------------------------------
 Name: bcmvenet_set_multicast_list
 Purpose: Set the multicast mode, ie. promiscuous or multicast
-------------------------------------------------------------------------- */
static void bcmvenet_set_multicast_list(struct net_device *dev)
{
    //TRACE(("%s: bcmvenet_set_multicast_list: %08X\n", dev->name, dev->flags));
}

/*
 * Set the hardware MAC address.
 */
static int bcmvenet_set_mac_addr(struct net_device *dev, void *p)
{

    struct sockaddr *addr = p;

    if (netif_running(dev))
    {
        return -EBUSY;
    }
    memcpy(dev->dev_addr, addr->sa_data, dev->addr_len);

    return 0;
}

/* --------------------------------------------------------------------------
 Name: bcmvenet_xmit
 Purpose: Send ethernet traffic
-------------------------------------------------------------------------- */
static int bcmvenet_xmit(struct sk_buff *skb, struct net_device *dev)
{

    unsigned long flags;
    dma_addr_t dma_addr;
    unsigned int index;

    TRACE(("bcmvenet_xmit\n"));

#if defined(DUMP_DATA)
    TRACE(("bcmvenet_xmit: len %d", skb->len));
    dumpHexData("xmit", skb->data, skb->len);
#endif

    /*
     * Obtain exclusive access to transmitter.  This is necessary because
     * we might have more than one stack transmitting at once.
     */
    spin_lock_irqsave(&tx_buf_lock, flags);

    /* Reclaim some BDs first */
    while (1)
    {
        if ((((unsigned int)tx_used_bd == (mbase2 + *tx_consumed_bd))
             && (tx_used_bd_cnt != xmit_buf_num))
            || (tx_used_bd_cnt == 0))
        {
            break;
        }
        /* Decrement the used bd counter */
        tx_used_bd_cnt--;
        if ((unsigned int)tx_used_bd == (unsigned int)tx_last_bd)
        {
            tx_used_bd = tx_first_bd;
        }
        else
        {
            tx_used_bd++;
        }
    }

    if (tx_used_bd_cnt == xmit_buf_num)
    {
        dev_kfree_skb(skb);
        dev->stats.tx_errors++;
        dev->stats.tx_dropped++;
        spin_unlock_irqrestore(&tx_buf_lock, flags);
        printk(KERN_ERR CARDNAME ": no bds, tx used count %d, tx buf num %d\n", tx_used_bd_cnt, xmit_buf_num);
        return NETDEV_TX_BUSY;
    }

    index = tx_new_bd - tx_first_bd;

    /* copy the data to an aligned buffer */
    skb_copy_from_linear_data(skb, (void *)tx_buf[index], skb->len);

    /* Flush the cache */
    dma_addr = pci_map_single(pcidev, (void *)tx_buf[index], skb->len, DMA_TO_DEVICE);

    /* Program the BD */
    tx_new_bd->pcieLowAddr = dma_addr;
    tx_new_bd->control0 = (0x80000000 | (skb->len + 4));
    TRACE(("bcmvenet_xmit: dev id %d\n", ((BcmVEnet_devctrl *)netdev_priv(dev))->devIndex));

    tx_new_bd->control1 =
        (tx_new_bd->control1 & 0x3fff00fc) |
        0xc0000002 | // byte swapping
        ((((BcmVEnet_devctrl *)netdev_priv(dev))->devIndex + 1) << 8); // virtual device tag

    /* Queue the descriptor */
    if (tx_prev_new_bd == tx_new_bd)
    {

        /* Set up DMA control: single list, descriptor in local memory */
        dmactl->TxSwDescListCtrlSts = 0x00000300;

        /* Queue the descriptor */
        dmactl->TxFirstDescLAddrList0 = ((((unsigned int)tx_first_bd - mbase2) & 0x1FFFFFFF) | 0x1);

        /* Kick the DMA */
        dmactl->TxSwDescListCtrlSts |= 0x1;
    }
    else
    {
        /* Clear the last record bit in previous BD */
        tx_prev_new_bd->control1 &= (~0x80000000);

        // Wake the DMA
        dmactl->TxWakeCtrl |= 1;
    }

    /* Increment the BD ptr */
    tx_prev_new_bd = tx_new_bd;
    if ((unsigned int)tx_new_bd >= (unsigned int)tx_last_bd)
    {
        tx_new_bd = tx_first_bd;
    }
    else
    {
        tx_new_bd++;
    }

    /* Counter */
    tx_cnt++;

    /* Increment the used bd counter */
    tx_used_bd_cnt++;

    /* Update stats */
    dev->stats.tx_bytes += skb->len;
    dev->stats.tx_packets++;
    dev->trans_start = jiffies;

    spin_unlock_irqrestore(&tx_buf_lock, flags);

    /* Free the network buffer now */
    dev_kfree_skb(skb);

    return NETDEV_TX_OK;
}

static void assign_rx_buffers(void)
{

    struct sk_buff *skb;
    unsigned int index;

    spin_lock_bh(&rx_buf_lock);

    while (1)
    {

        /* Get the index */
        index = rx_refill_bd - rx_first_bd;

        if (rx_cb[index].skb != NULL)
        {
            break;
        }

        /* Get a buffer */
        skb = netdev_alloc_skb(ndevs[0], ENET_MAX_MTU_SIZE);
        if (skb == NULL)
        {
            break;
        }

        /* Fill up the control block */
        rx_cb[index].skb = skb;
        rx_cb[index].pa = pci_map_single(pcidev, skb->data, ENET_MAX_MTU_SIZE, DMA_FROM_DEVICE);
        /* Assign the buffer to the BD */
        rx_refill_bd->pcieLowAddr = rx_cb[index].pa;

        /* Increment the refill ptr */
        if (rx_refill_bd == rx_last_bd)
        {
            rx_refill_bd = rx_first_bd;
        }
        else
        {
            rx_refill_bd++;
        }
    }

    spin_unlock_bh(&rx_buf_lock);
}


/* NAPI polling method*/
static int bcmvenet_poll(struct napi_struct *napi, int budget)
{

    int ndIx;
#ifdef CONFIG_PACESTB
    int dma_busy = 0;
#endif

    // Check that we've got carrier on at least one virtual device
    for (ndIx = 0; ndIx < devs; ndIx++)
    {
        if (netif_carrier_ok(ndevs[ndIx]))
        {
#ifdef CONFIG_PACESTB
            unsigned int work_done = bcmvenet_rx(budget, &dma_busy);
            assign_rx_buffers();
            if ((work_done < budget) && !dma_busy)
#else
            unsigned int work_done = bcmvenet_rx(budget);
            assign_rx_buffers();
            if (work_done < budget)
#endif
            {
                napi_complete(napi);
                /* Enable data interrupt */
                int_lvl2->PciMaskClear |=  L0_RX_DONE_IRQ;
            }
            return work_done;
        }
    }
    napi_complete(napi);
    return 0;
}

/*
 * bcmvenet_isr: BNM interrupt handler
 */
static irqreturn_t bcmvenet_isr(int irq, void *dev_id)
{

    uint32 events;

    events = int_lvl2->PciStatus;

    //TRACE(("%s : = %08x\n", __FUNCTION__, events));
    if (events & L0_RX_DONE_IRQ)
    {
        /* Disable data interrupt at L2 */
        int_lvl2->PciMaskSet |=  L0_RX_DONE_IRQ;
        /* Clear the interrupt */
        int_lvl2->PciClear |=  L0_RX_DONE_IRQ;
        /* Schedule the NAPI function */
        napi_schedule(&napi);
    }

    if (events & MBOX0_IRQ)
    {
        /* Disable mailbox interrupt at L2 */
        int_lvl2->PciMaskSet |=  MBOX0_IRQ;
        /* Clear the interrupt */
        int_lvl2->PciClear |=  MBOX0_IRQ;
        /* Schedule the task */
        tasklet_schedule(&mtask);
    }

    return IRQ_HANDLED;
}

/*
 *  bcmvenet_rx - called from NAPI polling method.
 */
#ifdef CONFIG_PACESTB
static unsigned int bcmvenet_rx(unsigned int budget, int *dma_busy)
#else
static unsigned int bcmvenet_rx(unsigned int budget)
#endif
{

    struct sk_buff *skb;
    int len;
    unsigned int tag;
    unsigned int index;
    pcieBufDesc *cur_bd;
    unsigned int cnt = 0;

#ifdef CONFIG_PACESTB
    *dma_busy = 0;
#endif

    /* Set the initial condition so current BD is read from H/W */
    cur_bd = rx_new_bd;
    while (cnt < budget)
    {
        /* Get the index */
        index = rx_new_bd - rx_first_bd;

        /* Get the control block */
        skb = rx_cb[index].skb;
        if (skb == NULL)
        {
            printk(KERN_ERR CARDNAME ": null skb\n");
            break;
        }

        /* Read the current BD pointer */
        if (rx_new_bd  == cur_bd)
        {
            cur_bd = (pcieBufDesc *)(mbase2 + (dmactl->RxList0CurDescLAddr & 0xffffffe0));
        }

        /* Check again with updated current BD ptr */
        if (rx_new_bd  == cur_bd)
        {
            /* Check DMA satus */
            if ((dmactl->TxSwDescListCtrlSts & 0x30) == 0x10)
            {
                /* DMA is busy, so I bail */
#ifdef CONFIG_PACESTB
                *dma_busy = 1;
#else
                //printk(KERN_ERR CARDNAME ": dma busy\n");
#endif
                break;
            }
        }
        else
        {
            if (rx_prev_new_bd  == cur_bd)
            {
                /* New pointer had moved, but DMA had not, bail */
                if ((cur_bd->control0 & 0x01FFFFFF) == 0)
                {
                    break;
                }
            }
        }

        /* Get the length */
        len = (int)(rx_new_bd->control0 & 0x01FFFFFF);
        if (len >= 4)
        {
            len -= 4;
        }

        /*
         * If tag is zero, then ecm is using a non-tagging driver.
         * Otherwise, substract one from the tag.
         */
        tag = (unsigned int)((rx_new_bd->control1 & 0x0000ff00) >> 8);
        if (tag)
        {
            tag--;
        }
#if defined(DUMP_DATA)
        dumpHexData("recv", skb->data, len);
#endif
        if (len < 64)
        {
            len = 64;
        }

        /* Zero the length */
        rx_new_bd->control0 &= 0xFE000000;

        /* Zero out the entry */
        rx_cb[index].skb = NULL;

        if (rx_refill_bd == NULL)
        {
            rx_refill_bd = rx_first_bd;
        }

        /* Remember the last position */
        rx_prev_new_bd = rx_new_bd ;

        /* Increment the new pointer */
        if (rx_new_bd == rx_last_bd)
        {
            rx_new_bd = rx_first_bd;
        }
        else
        {
            rx_new_bd++;
        }

        /* Update the consumed index */
        *rx_consumed_bd = ((u32)rx_new_bd - (u32)mbase2);
#ifdef CONFIG_PACESTB
        // DMA from device or some buffers will have wrong/junk data leading to
        // slow CDL downloads due to timeoutds/retries
        pci_map_single(pcidev, skb->data, ENET_MAX_MTU_SIZE, DMA_FROM_DEVICE);
#endif
        if ((tag < devs) && netif_carrier_ok(ndevs[tag]))
        {
            /* Set packet length */
            skb_put(skb, len);
            /* Device, protocol and housekeeping */
            skb->dev = ndevs[tag];
            skb->protocol = eth_type_trans(skb, ndevs[tag]);
            ndevs[tag]->stats.rx_packets++;
            ndevs[tag]->stats.rx_bytes += len;

            /* Give it to kernel */
            netif_receive_skb(skb);

            rx_cnt++;
        }
        else
        {
            printk(KERN_ERR CARDNAME ": invalid tag or link down\n");
            dev_kfree_skb(skb);
        }

        cnt++;
    }

    return cnt;
}

#ifdef CONFIG_PACESTB

void pci0_get_status(unsigned int *status)
{
    *status = 0; /* pciNotInitialised */
    if (BnmResetDetected)
    {
        *status = 1;  /* PciResetDetected */
    }
    else if (dmaReady)
    {
        *status = 2;  /* PciHostDmaReady  */
    }
    else
    {
        *status = 3;  /* PciNotReady  */
    }
}
EXPORT_SYMBOL(pci0_get_status);

static int pci0_proc_read(char *page, char **start, off_t off, int cnt, int *eof, void *data)
{
	int len = 0;

	if (BnmResetDetected)
 	{
		len += sprintf(&page[len], "PciResetDetected\n");
	}
	else if (dmaReady)
 	{
		len += sprintf(&page[len], "PciHostDmaReady\n");
	}
	else
	{
		len += sprintf(&page[len], "PciNotReady\n");
	}

	return len;
}

#endif /* CONFIG_PACESTB */

static int bcmvenet_proc_get_stat(char *page, char **start, off_t off, int cnt, int *eof, void *data)
{

    int r = sprintf(page, "Total queue length should be %d\n", recv_buf_num);
    r += sprintf(page + r, "rbase0 = %08x\n", (u32)rbase0);
    r += sprintf(page + r, "mbase2 = %08x\n", (u32)mbase2);
    r += sprintf(page + r, "tx_consumed_bd_ptr= %08x\n", (u32)tx_consumed_bd);
    r += sprintf(page + r, "tx_consumed_bd= %08x\n", *tx_consumed_bd);
    r += sprintf(page + r, "\n<===RX BD DUMP===>\n");
    r += sprintf(page + r, "First = %08x\n", (u32)rx_first_bd);
    r += sprintf(page + r, "Last = %08x\n", (u32)rx_last_bd);
    r += sprintf(page + r, "New = %08x\n", (u32)rx_new_bd);
    r += sprintf(page + r, "Prev New = %08x\n", (u32)rx_prev_new_bd);
    r += sprintf(page + r, "Refill = %08x\n", (u32)rx_refill_bd);
    r += sprintf(page + r, "\n[BD Ptr(index)] buf_ptr  dst_ptrL dst_ptrH control0 control1 next_low next_hgh reserved\n");
    r += sprintf(page + r, "=======================================================================================\n");
    r += sprintf(page + r, "<===TX BD DUMP===>\n");
    r += sprintf(page + r, "First = %08x\n", (u32)tx_first_bd);
    r += sprintf(page + r, "Last = %08x\n", (u32)tx_last_bd);
    r += sprintf(page + r, "New = %08x\n", (u32)tx_new_bd);
    r += sprintf(page + r, "Used = %08x\n", (u32)tx_used_bd);
    r += sprintf(page + r, "Prev New = %08x\n", (u32)tx_prev_new_bd);
    r += sprintf(page + r, "tx_used_bd_cnt = %d\n", (u32)tx_used_bd_cnt);
    r += sprintf(page + r, "\n[BD Ptr(index)] buf_ptr  src_ptrL src_ptrH control0 control1 next_low next_hgh reserved\n");
    r += sprintf(page + r, "=======================================================================================\n");
    r += sprintf(page + r, "\n");
    *eof = 1;
    return r;
}

void bcmvenet_del_proc_files(void)
{
    remove_proc_entry("driver/bcmvenet/stat", NULL);
    remove_proc_entry("driver/bcmvenet", NULL);
}

void bcmvenet_add_proc_files(void)
{
    proc_mkdir("driver/bcmvenet", NULL);
    create_proc_read_entry("driver/bcmvenet/stat", 0, NULL, bcmvenet_proc_get_stat, NULL);
#ifdef CONFIG_PACESTB
    create_proc_read_entry("pci0_status", 0, NULL, pci0_proc_read, NULL);
#endif
}

static int bcmvenet_do_init_dev()
{

    unsigned int tx_bd_pa;
    unsigned int rx_bd_pa;
    unsigned int tx_buf_num;
    unsigned int rx_buf_num;
    pcieBufDesc *p;
    pcieIpcInfo *ptr;
    int i;
    int ndIx;

    TRACE(("%s\n", __FUNCTION__));

    if (!ipc_info_pa)
    {
        return -1;
    }
    else
    {
        ptr = (pcieIpcInfo *)(mbase2 + ipc_info_pa);
        TRACE(("%s: mem_base = %08x\n", __FUNCTION__, mbase2));
        if (!ptr->tx_first_bd || !ptr->num_of_tx_bds || !ptr->rx_first_bd || !ptr->num_of_rx_bds)
        {
            return -1;
        }
        tx_bd_pa = ptr->rx_first_bd;
        rx_bd_pa = ptr->tx_first_bd;
        rx_buf_num = ptr->num_of_rx_bds;
        tx_buf_num = ptr->num_of_tx_bds;
        if ((tx_buf_num > MAX_VENET_TX_BUF) || (rx_buf_num > MAX_VENET_RX_BUF))
        {
            printk(KERN_ERR CARDNAME ": too many TX/RX buffer descriptors!\n");
            return -1;
        }
    }

    recv_buf_num = rx_buf_num;
    xmit_buf_num = tx_buf_num;

    /* The buffer descriptor is in EP's memory space */
    tx_first_bd = (pcieBufDesc *)(mbase2 + tx_bd_pa);
    tx_last_bd = tx_first_bd + tx_buf_num - 1;
    tx_new_bd = (pcieBufDesc *)(mbase2 + ptr->rx_new_bd);
    tx_prev_new_bd = (pcieBufDesc *)(mbase2 + ptr->rx_prev_new_bd);
    tx_used_bd = tx_new_bd;

    p = tx_first_bd;
    TRACE(("[%08x] %08x %08x %08x %08x %08x %08x %08x %08x\n",
           (u32)p, p->localAddr, p->pcieLowAddr,
           p->pcieHighAddr, p->control0,
           p->control1, p->nextLowAddr,
           p->nextHighAddr, p->reserved));

    rx_first_bd = (pcieBufDesc *)(mbase2 + rx_bd_pa);
    rx_last_bd = rx_first_bd + rx_buf_num - 1;
    rx_new_bd = (pcieBufDesc *)(mbase2 + ptr->tx_new_bd);
    rx_prev_new_bd = (pcieBufDesc *)(mbase2 + ptr->tx_prev_new_bd);
    rx_refill_bd = rx_new_bd;

    /* Clear the control block */
    for (i = 0; i < MAX_VENET_RX_BUF; i++)
    {
        if (rx_cb[i].skb)
        {
            dev_kfree_skb(rx_cb[i].skb);
            rx_cb[i].skb = NULL;
        }
    }

    TRACE(("%s: int_lvl1=0x%08x\n", __FUNCTION__, (unsigned int)int_lvl1));
    TRACE(("%s: int_lvl2=0x%08x\n", __FUNCTION__, (unsigned int)int_lvl2));

    TRACE(("%s: tx_first_bd=0x%08x\n", __FUNCTION__, (unsigned int)tx_first_bd));
    TRACE(("%s: tx_last_bd=0x%08x\n", __FUNCTION__, (unsigned int)tx_last_bd));
    TRACE(("%s: tx_new_bd=0x%08x\n", __FUNCTION__, (unsigned int)tx_new_bd));
    TRACE(("%s: tx_prev_new_bd=0x%08x\n", __FUNCTION__, (unsigned int)tx_prev_new_bd));
    TRACE(("%s: tx_used_bd=0x%08x\n", __FUNCTION__, (unsigned int)tx_used_bd));

    TRACE(("%s: rx_first_bd=0x%08x\n", __FUNCTION__, (unsigned int)rx_first_bd));
    TRACE(("%s: rx_last_bd=0x%08x\n", __FUNCTION__, (unsigned int)rx_last_bd));
    TRACE(("%s: rx_new_bd=0x%08x\n", __FUNCTION__, (unsigned int)rx_new_bd));
    TRACE(("%s: rx_prev_new_bd=0x%08x\n", __FUNCTION__, (unsigned int)rx_prev_new_bd));
    TRACE(("%s: rx_refill_bd=0x%08x\n", __FUNCTION__, (unsigned int)rx_refill_bd));

    /* Assign the RX ring with buffers */
    assign_rx_buffers();

    /* Zero the refill pointer */
    rx_refill_bd = NULL;

    /* Update the consumed index */
    *rx_consumed_bd = ((u32)rx_new_bd - (u32)mbase2);

    /* Mark link status */
    dmaReady = 1;

#ifdef CONFIG_PACESTB
    BnmResetDetected = 0;
#endif

    /* Write the mailbox */
    pci_write_config_dword(pcidev, VENDOR_SPECIFIC_REG2, dmaReady);

    int_lvl2->PciClear |= (L0_RX_DONE_IRQ  | MBOX0_IRQ);
    int_lvl2->PciMaskClear |= (L0_RX_DONE_IRQ  | MBOX0_IRQ);

    /* Start the network engine */
    for (ndIx = 0; ndIx < devs; ndIx++)
    {
        if (netif_queue_stopped(ndevs[ndIx]))
        {
            netif_start_queue(ndevs[ndIx]);
        }
        if (!netif_carrier_ok(ndevs[ndIx]))
        {
            netif_carrier_on(ndevs[ndIx]);
            printk("%s: Link is up, 1Gbps Full Duplex\n", ndevs[ndIx]->name);
        }
    }
    napi_enable(&napi);

    int_lvl2->PciMaskClear |= L0_RX_DONE_IRQ;

    return 0;
}

/*
 * bcmvenet_init_dev: initialize device
 */
static int bcmvenet_init_dev(void)
{

    int i;

    dmaReady = 0;

#ifdef CONFIG_PACESTB
    BnmResetDetected = 0;
    MBOX_ResetListInit( &MboxResetList );
#endif

    /* Add proc file system */
    bcmvenet_add_proc_files();

    /* register block locations */
    TRACE(("%s: dmactl=0x%08x\n", __FUNCTION__, (unsigned int)dmactl));

    /* init tx buffer */
    for (i = 0; i < MAX_VENET_TX_BUF; i++)
    {
        if (tx_buf[i] == NULL)
        {
            tx_buf[i] = kmalloc(VENET_RX_BUF_SIZE, GFP_KERNEL);
            BUG_ON(tx_buf[i] == NULL);
        }
    }

    /* if we reach this point, we've init'ed successfully */
    return 0;
}

/* Uninitialize tx/rx buffer descriptor pools */
static void bcmvenet_uninit_dev()
{
    int i;

    TRACE(("%s\n", __FUNCTION__));

    /* Tell EP about it */
    dmaReady = 0;
    pci_write_config_dword(pcidev, VENDOR_SPECIFIC_REG2, dmaReady);

    /* Sleep 100 ms for DMA to complete */
    msleep(100);

    /* Remove the proc file system entry */
    bcmvenet_del_proc_files();

    /* Free the RX SKB's */
    for (i = 0; i < MAX_VENET_RX_BUF; i++)
    {
        if (rx_cb[i].skb)
        {
            dev_kfree_skb(rx_cb[i].skb);
        }
    }

    /* Free the TX buffer */
    for (i = 0; i < MAX_VENET_TX_BUF; i++)
    {
        if (tx_buf[i])
        {
            kfree(tx_buf[i]);
        }
    }
}

/*
 * ethtool function - get driver info.
 */
static void bcmvenet_get_drvinfo(struct net_device *dev, struct ethtool_drvinfo *info)
{
    strncpy(info->driver, CARDNAME, sizeof(info->driver));
    strncpy(info->version, VER_STR, sizeof(info->version));
}

/*
 * standard ethtool support functions.
 */
static struct ethtool_ops bcmvenet_ethtool_ops =
{
    .get_drvinfo		= bcmvenet_get_drvinfo,
    .get_link			= ethtool_op_get_link,
};

static void copy_words_reverse(void *to, void *from, unsigned len, int swap)
{

    volatile uint32 *f, *t;

    len = (len + 3) / 4;
    f = from;
    t = to;
    f += len;
    t += len;
    if (swap)
    {
        while (len--)
        {
            uint32 v = *(--f);
            *(--t) = htonl(v);
        }
    }
    else
    {
        while (len--)
        {
            *(--t) = *(--f);
        }
    }
}


/*
 * ioctl handle special commands that are not present in ethtool.
 */
static int bcmvenet_ioctl(struct net_device *dev, struct ifreq *rq, int cmd)
{

    BcmVEnet_ioctlparms *ioctlparms;
    unsigned int from;
    unsigned int to;

    /* we can add sub-command in ifr_data if we need to in the future */
    switch (cmd)
    {
    case SIOCGETMBOX:
    {
        unsigned int offset = 0;
        if (hostbootMail)
        {
            offset = hostbootMail;
            hostbootMail = 0;
        }
        copy_words_reverse(rq->ifr_data, (void *)&offset, sizeof(offset), 0);
        break;
    }

    case SIOCGETWINDOW:
        ioctlparms = (void *)rq->ifr_data;
        if (ioctlparms->from >= 0x10000000)
        {
            from = ioctlparms->from + base_offset;
        }
        else
        {
            from = ioctlparms->from + mbase2;
        }
        copy_words_reverse((void *)ioctlparms->to, (void *)from, ioctlparms->length, 0);
        break;

    case SIOCPUTWINDOW:
    {
        ioctlparms = (void *)rq->ifr_data;
        if (ioctlparms->to >= 0x10000000)
        {
            to = ioctlparms->to + base_offset;
        }
        else
        {
            to = ioctlparms->to + mbase2;
        }
        if (ioctlparms->length > (64 * 1024))
        {
            ioctlparms->length = 64 * 1024;
        }
        copy_words_reverse((void *)to, (void *)ioctlparms->from, ioctlparms->length, 1);
        break;
    }

    default:
        return -EOPNOTSUPP;
    }
    return 0;
}

static const struct net_device_ops bcmvenet_netdev_ops =
{
    .ndo_open = bcmvenet_open,
    .ndo_stop = bcmvenet_close,
    .ndo_start_xmit = bcmvenet_xmit,
    .ndo_tx_timeout = bcmvenet_timeout,
#if ( LINUX_VERSION_CODE > KERNEL_VERSION(3,3,0) )
    .ndo_set_rx_mode = bcmvenet_set_multicast_list,
#else
    .ndo_set_multicast_list = bcmvenet_set_multicast_list,
#endif
    .ndo_set_mac_address = bcmvenet_set_mac_addr,
    .ndo_do_ioctl = bcmvenet_ioctl,
};

#ifdef CONFIG_PACESTB

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

#endif /* CONFIG_PACESTB */

static int __devinit bcmvenet_drv_probe(struct pci_dev *pdev, const struct pci_device_id *pid)
{

    int err = 0, is3383bx, ndIx;
    resource_size_t bar_base, bar_len, bar0_base, bar0_len;
#ifdef CONFIG_PACESTB
    char bcmx[5] = CONFIG_PACE_PCIDEV_NAME;
#else
    char bcmx[5] = {'b', 'c', 'm', '0', 0};
#endif

    TRACE(("%s...\n", __FUNCTION__));
    /* Enable memory region */
    err = pci_enable_device_mem(pdev);
    if (err)
    {
        printk(KERN_ERR CARDNAME ": can't enable device memory\n");
        return err;
    }
    pcidev = pdev;

    /* Request a memory region */
    err = pci_request_selected_regions_exclusive(pdev,
            pci_select_bars(pdev, IORESOURCE_MEM),
            bcmvenet_drv_name);
    if (err)
    {
        printk(KERN_ERR CARDNAME ": failure requesting device memory\n");
        goto err_request_selected_region;
    }

#if defined(CONFIG_PCIEAER)
    /* Enable error reporting */
    pci_enable_pcie_error_reporting(pdev);
#endif

    /* Set to master */
    pci_set_master(pdev);

    /* Allocate driver data structure */
    for (ndIx = 0; ndIx < devs; ndIx++)
    {
        ndevs[ndIx] = NULL;
    }
    for (ndIx = 0; ndIx < devs; ndIx++)
    {
        bcmx[3] = '0' + ndIx;
        TRACE(("Allocating netdev %s\n", bcmx));
        ndevs[ndIx] = alloc_netdev_mq(sizeof(BcmVEnet_devctrl), bcmx, ether_setup, 1);
        if (ndevs[ndIx] == NULL)
        {
            printk(KERN_ERR CARDNAME ": can't allocate net device\n");
            err = -ENOMEM;
            goto err_alloc_netdev;
        }
        SET_NETDEV_DEV(ndevs[ndIx], &pdev->dev);
        /* Get a pointer to my contrl info and store the virtual device index */
        ((BcmVEnet_devctrl *)netdev_priv(ndevs[ndIx]))->devIndex = ndIx;
    }

    /* Map BAR 0 for register access */
    bar0_base = pci_resource_start(pdev, 0);
    bar0_len = pci_resource_len(pdev, 0);
    rbase0 = (unsigned int)ioremap(bar0_base, bar0_len);
    if (!rbase0)
    {
        printk(KERN_ERR CARDNAME ": failure mapping BAR 0\n");
        goto err_bar0_ioremap;
    }

    bar_base = pci_resource_start(pdev, 2);
    bar_len = pci_resource_len(pdev, 2);
    mbase2 = (unsigned int)ioremap(bar_base, bar_len);
    if (!mbase2)
    {
        printk(KERN_ERR CARDNAME ": failure mapping BAR 2\n");
        goto err_bar2_ioremap;
    }

    TRACE(("%s : BAR2 (%08x@%08x->%08x)\n", __FUNCTION__, (unsigned int)bar_len, (unsigned int)bar_base, (unsigned int)mbase2));

    /* Test register mapping */
    TRACE(("%s : %s REVID = %08x\n", __FUNCTION__, bcmvenet_drv_name, *(volatile unsigned int *)(rbase0 + 0x04e00000)));
    TRACE(("%s : %s PCIE REVID = %08x\n", __FUNCTION__, bcmvenet_drv_name, *(volatile unsigned int *)(rbase0 + 0x02a0406c)));
    TRACE(("%s : %s DDR (0x00000000) = %08x\n", __FUNCTION__, bcmvenet_drv_name, *(volatile unsigned int *)(mbase2 + 0x00000000)));
    TRACE(("%s : %s DDR (0x033de9e0) = %08x\n", __FUNCTION__, bcmvenet_drv_name, *(volatile unsigned int *)(mbase2 + 0x033de9e0)));
    TRACE(("%s : %s DDR (0x07f00000) = %08x\n", __FUNCTION__, bcmvenet_drv_name, *(volatile unsigned int *)(mbase2 + 0x07f00000)));

    bar_base = pci_resource_start(pdev, 4);
    bar_len = pci_resource_len(pdev, 4);
    mbase4 = (unsigned int)ioremap(bar_base, bar_len);
    if (!mbase4 & is3383)
    {
        is3383bx = 0;
        int_lvl1 = (PcieIntr1Registers *)(rbase0 + 0x02a08200);
        int_lvl2 = (PcieIntr2Registers *)(rbase0 + 0x02a04300);
        inMbox = (unsigned long *)(rbase0 + 0x02a02198);
        altOutMbox = (unsigned long *)(rbase0 + 0x02a021a4);
        tx_consumed_bd = (unsigned int *)(rbase0 + 0x02a02190);
        rx_consumed_bd = (volatile unsigned int *)(rbase0 + 0x02a02194);
        dmactl = (PcieDmaReg *)(rbase0 + 0x02a04400);
        base_offset = rbase0 - 0x10000000;
    }
    else
    {
        is3383bx = 1;
        int_lvl1 = (PcieIntr1Registers *)(rbase0 + 0x8200);
        int_lvl2 = (PcieIntr2Registers *)(rbase0 + 0x4300);
        inMbox = (unsigned long *)(rbase0 + 0x2198);
        altOutMbox = (unsigned long *)(rbase0 + 0x21a4);
        tx_consumed_bd = (unsigned int *)(rbase0 + 0x2190);
        rx_consumed_bd = (volatile unsigned int *)(rbase0 + 0x2194);
        dmactl = (PcieDmaReg *)(rbase0 + 0x4400);
        base_offset = mbase4 - 0x13050000;
    }
    if (is3383)
    {
        printk(KERN_INFO CARDNAME ": Initializing for %s%cx endpoint\n", bcmvenet_drv_name, is3383bx ? 'B' : 'A');
    }
    else
    {
        printk(KERN_INFO CARDNAME ": Initializing for BCM3384(3) endpoint\n");
    }

    /* Set up the int control register */

    TRACE(("%s : BAR0 (%08x@%08x->%08x)\n", __FUNCTION__, (unsigned int)bar0_len, (unsigned int)bar0_base, (unsigned int)rbase0));

    for (ndIx = 0; ndIx < devs; ndIx++)
    {
        pci_set_drvdata(pdev, ndevs[ndIx]);
#ifdef CONFIG_PACESTB
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
            ndevs[ndIx]->dev_addr[0] = 0x00;
            ndevs[ndIx]->dev_addr[1] = 0xc0;
            ndevs[ndIx]->dev_addr[2] = 0xa8;
            ndevs[ndIx]->dev_addr[3] = 0x74;
            ndevs[ndIx]->dev_addr[4] = 0x3b;
            ndevs[ndIx]->dev_addr[5] = 0x52;
        }
        else
        {
            memcpy(ndevs[ndIx]->dev_addr,  bcmemac_cmdline_macaddr, ETH_ALEN);
        }
#else
        brcm_alloc_macaddr((u8 *)ndevs[ndIx]->dev_addr);
#endif
        SET_ETHTOOL_OPS(ndevs[ndIx], &bcmvenet_ethtool_ops);
        ndevs[ndIx]->watchdog_timeo = 2 * HZ;
        ndevs[ndIx]->netdev_ops = &bcmvenet_netdev_ops;
    }

    // We only tell NAPI about first virtual device since we all virtual devices rely on
    // the same physical device and we don't want multiple instances of poling.
#ifdef CONFIG_PACESTB
    netif_napi_add(ndevs[0], &napi, bcmvenet_poll, 1024);
#else
    netif_napi_add(ndevs[0], &napi, bcmvenet_poll, 64);
#endif

    /* Initialize the locks */
    spin_lock_init(&tx_buf_lock);
    spin_lock_init(&rx_buf_lock);

    TRACE(("%s: irq=%d\n", __FUNCTION__, (unsigned int)pdev->irq));

    /* Init the tasklet */
    mtask.next = NULL;
    mtask.state = 0;
    atomic_set(&mtask.count, 0);
    mtask.func = mbox_tasklet;
    mtask.data = 0;

    /* Init registers, Tx/Rx buffers */
    if (bcmvenet_init_dev() < 0)
    {
        printk(KERN_ERR CARDNAME ": can't initialize device\n");
        goto err_init_dev;
    }

    hostbootMail = 0;

    /* Request an interrupt */
    if (request_irq(pdev->irq, bcmvenet_isr, (IRQF_SHARED), ndevs[0]->name, ndevs) < 0)
    {
        printk(KERN_ERR CARDNAME ": can't request IRQ %d\n", pdev->irq);
        goto err_request_irq;
    }

    // Make sure carriers of off
    for (ndIx = 0; ndIx < devs; ndIx++)
    {
        if (netif_carrier_ok(ndevs[ndIx]))
        {
            netif_carrier_off(ndevs[ndIx]);
            printk("%s: Link is down\n", ndevs[ndIx]->name);
        }
    }

    /* Get the PCIE capability pointer */
    if ((pcie_cfg_cap = pci_find_capability(pdev, PCI_CAP_ID_EXP)) == 0)
    {
        printk(KERN_ERR CARDNAME ": can't get PCIE capability position!\n");
        goto err_capability;
    }
    else
    {
        u16 val;
        u32 val32;

        /* Set the MPS and MRRS to 256B */
        err = pci_read_config_word(pdev, pcie_cfg_cap + PCI_EXP_DEVCTL, &val);
        if (err)
        {
            printk(KERN_ERR CARDNAME ": can't read device control status!\n");
            goto err_capability;
        }

        val &= ~(PCI_EXP_DEVCTL_PAYLOAD);
        val |= 0x20;
        pci_write_config_word(pdev, pcie_cfg_cap + PCI_EXP_DEVCTL, val);

        val32 = BDEV_RD(BCHP_PCIE_RC_CFG_PCIE_DEVICE_STATUS_CONTROL);
        val32 &= 0xFFFFFF1F;
        val32 |= (1 << 5);
        BDEV_WR(BCHP_PCIE_RC_CFG_PCIE_DEVICE_STATUS_CONTROL, val32);
    }

    /* Register an Ethernet device with kernel */
    for (ndIx = 0; ndIx < devs; ndIx++)
    {
        if ((err = register_netdev(ndevs[ndIx])) != 0)
        {
            printk(KERN_ERR CARDNAME ": can't register netdev %s\n", ndevs[ndIx]->name);
            goto err_register;
        }
    }

    /* Enable mailbox interrupts */
    int_lvl1->IntrMaskClear |= PCIE_INTR_PCIE_INTR;
    int_lvl2->PciMaskClear |= MBOX0_IRQ;

    return(0);

err_register:
    free_irq(pdev->irq, ndevs);
err_capability:
err_request_irq:
    bcmvenet_uninit_dev();
err_init_dev:
    if (mbase4)
    {
        iounmap((u8 __iomem *)mbase4);
    }
    iounmap((u8 __iomem *)mbase2);
err_bar2_ioremap:
    iounmap((u8 __iomem *)rbase0);
err_bar0_ioremap:
    for (ndIx = 0; ndIx < devs; ndIx++)
    {
        if (ndevs[ndIx])
        {
            free_netdev(ndevs[ndIx]);
        }
    }
err_alloc_netdev:
    pci_disable_device(pdev);
    pci_release_selected_regions(pdev, pci_select_bars(pdev, IORESOURCE_MEM));
err_request_selected_region:
    return(err);
}

/**
 * This routine will be called by PCI subsystem to release a PCI device.
 * For example, before the driver is removed from memory
 **/
static void __devexit bcmvenet_drv_remove(struct pci_dev *pdev)
{
    int ndIx;

    /* Disable mbox and data interrupts */
    printk("Disabling mailbox and pci interrupt\n");
    int_lvl2->PciMaskSet |= (L0_RX_DONE_IRQ  | MBOX0_IRQ);
    int_lvl2->PciClear |= (L0_RX_DONE_IRQ  | MBOX0_IRQ);

    /* Un-initialize everything H/W related */
    bcmvenet_uninit_dev();

    /* Unregister the network device */
    for (ndIx = 0; ndIx < devs; ndIx++)
    {
        unregister_netdev(ndevs[ndIx]);
    }

    /* Free the interrupt */
    free_irq(pdev->irq, ndevs);

    /* Send READY notification */
    mb_send(MBOX_REBOOTING, 0, ndevs);

    /* Free the memory region */
    pci_release_selected_regions(pdev, pci_select_bars(pdev, IORESOURCE_MEM));

    /* Unmap the register and memory space */
    if (mbase4)
    {
        iounmap((u8 __iomem *)mbase4);
    }
    iounmap((u8 __iomem *)mbase2);
    iounmap((u8 __iomem *)rbase0);

    /* Free the data structure */
    for (ndIx = 0; ndIx < devs; ndIx++)
    {
        free_netdev(ndevs[ndIx]);
    }

    /* Disable error reporting */
    pci_disable_pcie_error_reporting(pdev);

    return;
}

static pci_ers_result_t bcmvenet_drv_error_detected(struct pci_dev *pdev, pci_channel_state_t state)
{

    int ndIx;

    printk(KERN_ERR CARDNAME ": fatal PCIe device error\n");
    for (ndIx = 0; ndIx < devs; ndIx++)
    {
        if (netif_running(ndevs[ndIx]))
        {
            if (netif_carrier_ok(ndevs[ndIx]))
            {
                netif_carrier_off(ndevs[ndIx]);
                printk("%s: Link is down\n", ndevs[ndIx]->name);
            }
        }
    }
    return PCI_ERS_RESULT_DISCONNECT;
}

/* Error recovery */
static struct pci_error_handlers bcmvenet_drv_err_handler =
{
    .error_detected = bcmvenet_drv_error_detected,
};

static DEFINE_PCI_DEVICE_TABLE(bcmvenet_drv_tbl) =
{
    { BCMVENDOR, BCM3383DEVICE, PCI_ANY_ID, PCI_ANY_ID, 0, 0, 0},
    { BCMVENDOR, BCM3384DEVICE, PCI_ANY_ID, PCI_ANY_ID, 0, 0, 0},
    { BCMVENDOR, BCM33843DEVICE, PCI_ANY_ID, PCI_ANY_ID, 0, 0, 0},
    { }	/* Null termination */
};

MODULE_DEVICE_TABLE(pci, bcmvenet_drv_tbl);

/* I am a PCI driver */
static struct pci_driver bcmvenet_driver =
{
    .name     = bcmvenet_drv_name,
    .id_table = bcmvenet_drv_tbl,
    .probe    = bcmvenet_drv_probe,
    .remove   = bcmvenet_drv_remove,
#if defined(CONFIG_PM_OPS) && (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,34))
    .suspend  = bcmvenet_drv_suspend,
    .resume   = bcmvenet_drv_resume,
#endif
    .err_handler = &bcmvenet_drv_err_handler
};

static int __init bcmvenet_module_init(void)
{

    int err = 0;

    TRACE(("bcmvenet_module_init() ...\n"));
    /* print the ChipID and module version info */
    printk(KERN_INFO CARDNAME ": Broadcom BCM3383/BCM3384 Virtual Ethernet Driver " VER_STR "\n");

    bcmvenet_drv_name[0] = 0;
    if (pci_get_device(BCMVENDOR, BCM3383DEVICE, NULL))
    {
        is3383 = true;
        strcpy(bcmvenet_drv_name, "BCM3383");
    }
    else
    {
        if (pci_get_device(BCMVENDOR, BCM3384DEVICE, NULL))
        {
            strcpy(bcmvenet_drv_name, "BCM3384");
        }
        else
        {
            if (pci_get_device(BCMVENDOR, BCM33843DEVICE, NULL))
            {
                strcpy(bcmvenet_drv_name, "BCM3843");
            }
            else
            {
                printk(KERN_ERR CARDNAME ": Can't find %04x or %04x or %04x ECM device\n", BCM3383DEVICE, BCM3384DEVICE, BCM33843DEVICE);
                err = -1;
            }
        }
    }
    if (bcmvenet_drv_name[0])
    {
        if ((err = pci_register_driver(&bcmvenet_driver)))
        {
            printk(KERN_ERR CARDNAME ": Error registering driver\n");
        }
    }
    return err;
}

module_init(bcmvenet_module_init);

static void __exit bcmvenet_module_exit(void)
{
    TRACE(("bcmvenet_module_exit() ...\n"));
    pci_unregister_driver(&bcmvenet_driver);
}

module_exit(bcmvenet_module_exit);

MODULE_AUTHOR("Broadcom Corporation");
MODULE_DESCRIPTION("BCM3383/3384(3) PCIe Virtual Network Driver");
MODULE_LICENSE("Proprietary");
MODULE_VERSION(VERSION);
