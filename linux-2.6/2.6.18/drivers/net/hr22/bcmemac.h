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
#ifndef _BCMINTENET_H_
#define _BCMINTENET_H_

#include <linux/skbuff.h>
#include <linux/if_ether.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include "linux/kernel.h"	/* For barrier() */
#include "boardparms.h"
#include "intemac_map.h"

#include <asm/brcmstb/common/bcmtypes.h>
#include <asm/brcmstb/common/brcmstb.h>

#include <linux/spinlock.h>
#ifdef USE_BH
// Obsolete in 2.6, will address this if we ever allow background completion.
//#include <linux/tqueue.h>
#endif

#ifdef CONFIG_TIVO
#define CONFIG_BCMINTEMAC_NETLINK
#endif /* CONFIG_TIVO */

#ifdef CONFIG_BCMINTEMAC_NETLINK 
#include <linux/rtnetlink.h>	
#endif	

/*---------------------------------------------------------------------*/
/* specify number of BDs and buffers to use                            */
/*---------------------------------------------------------------------*/
#if defined( CONFIG_MIPS_BCM7401C0 ) || defined( CONFIG_MIPS_BCM7402C0 )  \
    || defined( CONFIG_MIPS_BCM7403A0 ) || defined( CONFIG_MIPS_BCM7400B0 ) \
    || defined( CONFIG_MIPS_BCM7452A0 ) || defined( CONFIG_MIPS_BCM7405A0 )

#define TOTAL_DESC				256		/* total number of Buffer Descriptors */
#define RX_RATIO				1/2		/* ratio of RX descriptors number in total */
#define EXTRA_TX_DESC			24		/* fine adjustment in TX descriptor number */
#else
#define TOTAL_DESC				128		/* total number of Buffer Descriptors */
#define RX_RATIO				3/4		/* ratio of RX descriptors number in total */
#define EXTRA_TX_DESC			24		/* fine adjustment in TX descriptor number */
#endif

#define DESC_MASK	(TOTAL_DESC - 1)

#define NR_RX_BDS               ((TOTAL_DESC*RX_RATIO) - EXTRA_TX_DESC)
#define NR_TX_BDS               (TOTAL_DESC - NR_RX_BDS)

#define ENET_MAX_MTU_SIZE       1536    /* Body(1500) + EH_SIZE(14) + VLANTAG(4) + BRCMTAG(6) + FCS(4) = 1528.  1536 is multiple of 256 bytes */
#define DMA_MAX_BURST_LENGTH    0x40    /* in 32 bit words = 256 bytes  THT per David F, to allow 256B burst */
#define MAX_RX_BUFS             (NR_RX_BDS * 4)

#define ETH_CRC_LEN             4
/* misc. configuration */
#define DMA_FC_THRESH_LO        5
#define DMA_FC_THRESH_HI        (NR_RX_BDS / 2)

#if defined( CONFIG_MIPS_BCM7038 ) || defined( CONFIG_MIPS_BCM7400 ) \
	|| defined( CONFIG_MIPS_BCM7401 ) || defined( CONFIG_MIPS_BCM7402 ) \
	|| defined( CONFIG_MIPS_BCM7402S ) || defined( CONFIG_MIPS_BCM7440 ) \
        || defined( CONFIG_MIPS_BCM7403 ) || defined( CONFIG_MIPS_BCM7452 )

#if defined( CONFIG_MIPS_BCM7401C0 ) || defined( CONFIG_MIPS_BCM7402C0 ) \
    || defined( CONFIG_MIPS_BCM7403A0 ) || defined( CONFIG_MIPS_BCM7400B0 ) \
    || defined( CONFIG_MIPS_BCM7452A0 )
 
#define EMAC_RX_DESC_BASE   	0xb0082800	/* MAC DMA Rx Descriptor word */
#else
#define EMAC_RX_DESC_BASE   	0xb0082000	/* MAC DMA Rx Descriptor word */
#endif

#define ENET_MAC_ADR_BASE		0xb0080000
#define EMAC_TX_DESC_BASE   	(EMAC_RX_DESC_BASE+(8*NR_RX_BDS))			/* MAC DMA Tx Descriptor word */

#define EMAC_DMA_BASE   		0xb0082400
#define DMA_ADR_BASE			EMAC_DMA_BASE

#if defined( CONFIG_MIPS_BCM7401) || defined( CONFIG_MIPS_BCM7402 ) \
    || defined( CONFIG_MIPS_BCM7402S ) || defined( CONFIG_MIPS_BCM7403 ) \
    || defined( CONFIG_MIPS_BCM7452 )

#define CONFIG_BCMINTEMAC_KTHREAD
#endif

#elif defined CONFIG_MIPS_BCM7318
#include <asm/brcmstb/brcm97318/bcm7318.h>

#define ENET_MAC_ADR_BASE 		ENET_ADR_BASE				/* 0xfffd4000 */
#define EMAC_RX_DESC_BASE   	(ENET_ADR_BASE+0x2000)		/* 0xfffd6000  MAC DMA Rx Descriptor word */
#define EMAC_TX_DESC_BASE   	(EMAC_RX_DESC_BASE+(8*NR_RX_BDS))	/* MAC DMA Tx Descriptor word */
#define EMAC_DMA_BASE           (EMAC_RX_DESC_BASE+0x400)	/* 0xfffd6400 */
#define DMA_ADR_BASE			EMAC_DMA_BASE

#else
  #error "Unsupported platform for IntEMAC\n"
#endif



#define CACHE_TO_NONCACHE(x)	KSEG1ADDR(x)

#define ERROR(x)        printk x
#ifndef ASSERT
#define ASSERT(x)       if (x); else ERROR(("assert: "__FILE__" line %d\n", __LINE__)); 
#endif

//#define DUMP_TRACE
//#define DUMP_DATA
#define IUDMA_INIT_WORKAROUND // for 7038 A0 since IUDMA endine does not get reset properly. Should not need it for 7038B0.

#if defined(DUMP_TRACE)
#define TRACE(x)        printk x
#else
#define TRACE(x)
#endif

typedef struct ethsw_info_t {
    int cid;                            /* Current chip ID */
    int page;                           /* Current page */
    int type;                           /* Ethernet Switch type */
} ethsw_info_t;

#pragma pack(1)
typedef struct Enet_Tx_CB {
    struct sk_buff      *skb;
    uint32              nrBds;          /* # of bds req'd (incl 1 for this header) */
    volatile DmaDesc    *lastBdAddr;
    struct Enet_Tx_CB   *next;          /* ptr to next header in free list */
} Enet_Tx_CB;

typedef struct Enet_Rx_Buff {
    unsigned char       dAddr[ETH_ALEN];/* destination hardware address */
    unsigned char       sAddr[ETH_ALEN];/* source hardware address */
    uint16              type;           /* Ethernet type/length */
    char                buffer[1500];   /* data */
    uint32              crc;            /* normal checksum (FCS1) */
    uint16              pad1;           /* rsvd padding */
    uint32              pad2[4];        /* Must be mult of EMAC_DMA_MAX_BURST_LENGTH */
} Enet_Rx_Buff;

typedef struct {
    unsigned char da[6];
    unsigned char sa[6];
    uint16 brcm_type;
    uint32 brcm_tag;
} BcmEnet_hdr;
#pragma pack()

typedef struct PM_Addr {
    bool                valid;          /* 1 indicates the corresponding address is valid */
    unsigned int        ref;            /* reference count */
    unsigned char       dAddr[ETH_ALEN];/* perfect match register's destination address */
    char                unused[2];      /* pad */
} PM_Addr;					 
#define MAX_PMADDR      8               /* # of perfect match address */

#define MAX_NUM_OF_VPORTS   8
#define BRCM_TAG_LEN        6
#define BRCM_TYPE           0x8874
#define BRCM_TAG_UNICAST    0x00000000
#define BRCM_TAG_MULTICAST  0x20000000
#define BRCM_TAG_EGRESS     0x40000000
#define BRCM_TAG_INGRESS    0x60000000
#define MAX_PHY_PORT        4
#define PHY_PORT            16 
#define VPORT_MASK          0xffff
 
/*
 * device context
 */ 
typedef struct BcmEnet_devctrl {
    struct net_device *dev;             /* ptr to net_device */
    spinlock_t      lock;               /* Serializing lock */
    struct timer_list timer;            /* used by Tx reclaim */
    int             linkstatus_polltimer;
    int             linkstatus_phyport;
    int             linkstatus_holder;
    atomic_t        devInUsed;          /* device in used */

    struct net_device_stats stats;      /* statistics used by the kernel */

    struct sk_buff  *skb_pool[MAX_RX_BUFS]; /* rx buffer pool */
    int             nextskb;            /* next free skb in skb_pool */ 
    atomic_t        rxDmaRefill;        /* rxDmaRefill == 1 needs refill rxDma */
	
    // THT: Argument to test_and_set_bit must be declared as such
    volatile unsigned long rxbuf_assign_busy;  /* assign_rx_buffers busy */

    volatile EmacRegisters *emac;       /* EMAC register base address */

    /* transmit variables */
    volatile DmaRegs *dmaRegs;
    volatile DmaChannel *txDma;         /* location of transmit DMA register set */

    Enet_Tx_CB      *txCbPtrHead;       /* points to EnetTxCB struct head */
    Enet_Tx_CB      *txCbPtrTail;       /* points to EnetTxCB struct tail */

    Enet_Tx_CB      *txCbQHead;         /* points to EnetTxCB struct queue head */
    Enet_Tx_CB      *txCbQTail;         /* points to EnetTxCB struct queue tail */
    Enet_Tx_CB      *txCbs;             /* memory locaation of tx control block pool */

    volatile DmaDesc *txBds;            /* Memory location of tx Dma BD ring */
    volatile DmaDesc *txLastBdPtr;      /* ptr to last allocated Tx BD */
    volatile DmaDesc *txFirstBdPtr;     /* ptr to first allocated Tx BD */
    volatile DmaDesc *txNextBdPtr;      /* ptr to next Tx BD to transmit with */

    int             nrTxBds;            /* number of transmit bds */
    int             txFreeBds;          /* # of free transmit bds */

    /* receive variables */
    volatile DmaChannel *rxDma;         /* location of receive DMA register set */
    volatile DmaDesc *rxBds;            /* Memory location of rx bd ring */
    volatile DmaDesc *rxBdAssignPtr;    /* ptr to next rx bd to become full */
    volatile DmaDesc *rxBdReadPtr;      /* ptr to next rx bd to be processed */
    volatile DmaDesc *rxLastBdPtr;      /* ptr to last allocated rx bd */
    volatile DmaDesc *rxFirstBdPtr;     /* ptr to first allocated rx bd */

    int             nrRxBds;            /* number of receive bds */
    int             rxBufLen;           /* size of rx buffers for DMA */

    uint16          chipId;             /* chip's id */
    uint16          chipRev;            /* step */
    int             rxIrq;              /* rx dma irq */
    int             phyAddr;            /* 1 - external MII, 0 - internal MII (default after reset) */
    PM_Addr         pmAddr[MAX_PMADDR]; /* perfect match address */
#ifdef USE_BH
    struct tq_struct task;              /* task queue */
#endif
#ifdef CONFIG_BCMINTEMAC_KTHREAD
	int cond;
	int time_to_gohome;
	pid_t pid;
	wait_queue_head_t wq;
	struct completion compl;
    int counter_bh;
    int counter_th;
    atomic_t nevents;           /* number of events to deal with */
    int catchup;                /* number of 'missed events' */
#endif
    bool            bIPHdrOptimize;
    int             linkState;
#ifdef CONFIG_BCMINTEMAC_NETLINK 	
	struct work_struct link_change_task;
#endif	
    ethsw_info_t    ethSwitch;          /* external switch */
    ETHERNET_MAC_INFO EnetInfo;
    int             phy_port[MAX_PHY_PORT];
} BcmEnet_devctrl;

// BD macros
#define IncTxBDptr(x, s) if (x == ((BcmEnet_devctrl *)s)->txLastbdPtr) \
                             x = ((BcmEnet_devctrl *)s)->txFirstbdPtr; \
                      else x++

#define IncRxBDptr(x, s) if (x == ((BcmEnet_devctrl *)s)->rxLastBdPtr) \
                             x = ((BcmEnet_devctrl *)s)->rxFirstBdPtr; \
                      else x++

/* Big Endian */
#define EMAC_SWAP16(x) (x)
#define EMAC_SWAP32(x) (x)

#endif /* _BCMINTENET_H_ */
