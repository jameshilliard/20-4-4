/*******************************************************************************
*
* GPL/PCI/eth_dvr.c
*
* Description: Linux Ethernet Driver
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
*******************************************************************************/

/*******************************************************************************
*                            # I n c l u d e s                                 *
********************************************************************************/

#include "ClnkEth_Spec.h"
#include "pci_gpl_hdr.h"
#include "common_dvr.h"

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 0)
#define MOD_INC_USE_COUNT   do { } while(0)
#define MOD_DEC_USE_COUNT   do { } while(0)
#else
#define IRQ_NONE
#define IRQ_HANDLED
#define IRQ_RETVAL(x)
#endif

#ifndef IRQF_SHARED
#define IRQF_SHARED SA_SHIRQ
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 4, 22) && LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 11)
typedef void irqreturn_t;
#endif

#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,22)
#undef ETHTOOL_GPERMADDR
#undef SET_MODULE_OWNER
#define SET_MODULE_OWNER(dev) do { } while (0)
#endif


/*******************************************************************************
*                             # D e f i n e s                                  *
********************************************************************************/

#define TX_TIMEOUT (4*HZ)     // for watchdog

#define MAX_ETH_MTU_SIZE 0x700

// CONFIGURATION CHOICES (see also HostOS_Spec.h)

// MAX_ETH_RX_BUF_SIZE:
#if defined(CONFIG_ARCH_ENTROPIC_ECB) /* ALTERNATE WAY FOR XSCALE ECB */
  // Allows fewer cache flushes when packet ultimately goes from clink rx to
  // ixp1 tx to ixp1 rx.  May or may not be worth while.
  #define MAX_ETH_RX_BUF_SIZE ((MAX_ETH_MTU_SIZE) + 212)
#else
  // At least +2 due to our Ethernet header misalignment.
  #define MAX_ETH_RX_BUF_SIZE ((MAX_ETH_MTU_SIZE) + 4UL)
#endif

// CLNK_PER_PKT_INTS
#define CLNK_PER_PKT_INTS (CLNK_ETH_PER_PACKET ? CLNK_ETH_INTERRUPT_FLAG : 0)


MODULE_AUTHOR("Clink SW Development Team");
MODULE_DESCRIPTION("Clink ethernet driver");


#if (LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 0))
  static char mac_addr[18];  // command-line optional hw mac_addr override

  MODULE_PARM (mac_addr, "c18");
#else
  static char *mac_addr = "00:00:00:00:00:00";// EntropicMAC 00:09:8b:xx:xx:xx

  module_param (mac_addr, charp, 0);
  MODULE_PARM_DESC (mac_addr, "c18 string");
#endif // LINUX_VERSION_CODE
static SYS_INT32 board_idx = -1;


#if (FEATURE_QOS && ECFG_CHIP_ZIP1)
#define TX_FIFO_RESIZE_PENDING 1
#endif /* FEATURE_QOS && ECFG_CHIP_ZIP1 */


/*******************************************************************************
*                             D a t a   T y p e s                              *
********************************************************************************/



/*******************************************************************************
*                             C o n s t a n t s                                *
********************************************************************************/

    // Works for both (CLNK_VLAN_MODE_HWVLAN || CLNK_VLAN_MODE_SWVLAN)
    // Taken from IEEE 802.1Q document, page 48

#if   (CLNK_ETH_VLAN_ACTUAL == 8)
    static const SYS_UINT8  priority_lut[8] = { 2, 0, 1, 3, 4, 5, 6, 7};
#elif (CLNK_ETH_VLAN_ACTUAL == 7)
    static const SYS_UINT8  priority_lut[8] = { 1, 0, 0, 2, 3, 4, 5, 6};
#elif (CLNK_ETH_VLAN_ACTUAL == 6)
    static const SYS_UINT8  priority_lut[8] = { 1, 0, 0, 2, 3, 4, 5, 5};
#elif (CLNK_ETH_VLAN_ACTUAL == 5)
    static const SYS_UINT8  priority_lut[8] = { 1, 0, 0, 1, 2, 3, 4, 4};
#elif (CLNK_ETH_VLAN_ACTUAL == 4)
    static const SYS_UINT8  priority_lut[8] = { 1, 0, 0, 1, 2, 2, 3, 3};
#elif (CLNK_ETH_VLAN_ACTUAL == 3)
    static const SYS_UINT8  priority_lut[8] = { 0, 0, 0, 0, 1, 1, 2, 2};
#elif (CLNK_ETH_VLAN_ACTUAL == 2)
    static const SYS_UINT8  priority_lut[8] = { 0, 0, 0, 0, 1, 1, 1, 1};
#elif (CLNK_ETH_VLAN_ACTUAL == 1)
    static const SYS_UINT8  priority_lut[8] = { 0, 0, 0, 0, 0, 0, 0, 0};
#endif

#if (CLNK_VLAN_MODE_HWVLAN)

    #if   (CLNK_ETH_VLAN_ACTUAL + FEATURE_QOS == 4)
        // priority queue size percentages in absence of PQoS flows
        static const SYS_UINT16 priority_pct[4] = { 17, 17, 3, 0 };
        // priority queue size percentages in presence of PQoS flows
        static const SYS_UINT16 priority_pct_qos[4] = { 14, 3, 2, 18 };
    #elif (CLNK_ETH_VLAN_ACTUAL == 3)
        // NOTE: Original values allowed up to (100, 27, 27) @ 3 nodes
        // static const SYS_UINT16 priority_pct[3] = { 22, 8, 7 };

        // NOTE: New MoCA requirement in 2.6x release states that autonmous
        // nodes must preserve 65 Mb Video and 10 Mb Control.  Rather than try
        // to modify queue sizes as nodes admit, these numbers assume a worst
        // case of 8 nodes running. Technically this ends up being just 64 Mb.
        // If 64 Mbit is presented, it will get thru.  If higher PRIO 2 rate is
        // presented the actual rate may decrease to eg 61 Mbits.
        // NOTE: A DVR or settop may know enough about the environment that it
        // can use its own settings.
        // Moca/2.6 allows for (64, 64, 10) @ 8 nodes
        static const SYS_UINT16 priority_pct[3] = { 17, 17, 3 };
    #elif (CLNK_ETH_VLAN_ACTUAL == 2)
        static const SYS_UINT16 priority_pct[2] = { 22, 15 };
    #elif (CLNK_ETH_VLAN_ACTUAL == 1)
        static const SYS_UINT16 priority_pct[1] = { 37 };
    #else
        #error "New Version of HWVLAN distribution needs Tuning"
        // Using more than 3 HW Fifo's requires SOC mods as well.
    #endif

#elif (CLNK_VLAN_MODE_SWVLAN)

    static const SYS_UINT16 priority_pct[1] = { 37 };

#else

    #error "Unimplemented VLAN MODE"

#endif


/*******************************************************************************
*                             G l o b a l   D a t a                            *
********************************************************************************/

static const char version[] __devinitdata =
    "Linux Clink Ethernet driver version " DRV_VERSION "\n";

static const struct pci_device_id clnketh_pci_tbl[] __devinitdata =
{
    // Vendor, device, subvendor,  subdevice,  class, mask, drv_data
#if ECFG_CHIP_ZIP1
    {  0x17e6, 0x0010, PCI_ANY_ID, PCI_ANY_ID, 0,     0,    0 },
#elif ECFG_CHIP_ZIP2
    {  0x17e6, 0x0021, PCI_ANY_ID, PCI_ANY_ID, 0,     0,    0 },
#elif ECFG_CHIP_MAVERICKS 
    {  0x17e6, 0x0025, PCI_ANY_ID, PCI_ANY_ID, 0,     0,    0 },
#endif
    { }
};
MODULE_DEVICE_TABLE(pci, clnketh_pci_tbl);


/*******************************************************************************
*                             L O C A L   D a t a                              *
********************************************************************************/
static const SYS_UINT32 eth_minrxlen[] = { MAX_ETH_MTU_SIZE, 0, 0 };



/*******************************************************************************
*                       M e t h o d   P r o t o t y p e s                      *
********************************************************************************/

static int __devinit clnketh_probe_one(struct pci_dev *pdev, const struct pci_device_id *ent);
static SYS_VOID __devexit clnketh_remove_one(struct pci_dev *pdev);
static SYS_VOID clnketh_pci_config(struct pci_dev *pdev, struct net_device *dev);
static SYS_VOID clnketh_config_mac_address(struct net_device *dev, struct pci_dev *pdev);
static SYS_INT32 clnketh_enqueue_rx_buffs(struct net_device *dev);
static SYS_INT32 clnketh_start_xmit(struct sk_buff *skb, struct net_device *dev);
static SYS_VOID clnketh_tx_timeout(struct net_device *dev);
static SYS_VOID clnketh_xmit_callback(struct net_device *dev, SYS_UINT32 packetIDs[], SYS_UINT32 ids);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 19)
static irqreturn_t clnketh_isr(SYS_INT32 irq, SYS_VOID *dev_instance );
#else
static irqreturn_t clnketh_isr(SYS_INT32 irq, SYS_VOID *dev_instance, struct pt_regs *regs);
#endif
static SYS_VOID clnketh_rx_callback(struct net_device *dev, SYS_UINT32 listnums[], SYS_UINT32 packetIDs[],
                      SYS_UINT16 len[], SYS_UINT16 rxstatus[], SYS_INT32 bufs);
static struct net_device_stats *clnketh_get_stats(struct net_device *dev);
static SYS_VOID clnketh_set_rx_mode(struct net_device *dev);
static SYS_INT32 __init clnketh_init(SYS_VOID);
static SYS_VOID __exit clnketh_cleanup(SYS_VOID);
#if (FEATURE_QOS && ECFG_CHIP_ZIP1)
static SYS_VOID clnketh_tfifo_resize_callback(struct net_device *dev);
#endif /* FEATURE_QOS && ECFG_CHIP_ZIP1 */


/*******************************************************************************
*
* Description:
*       Initializes an instance of clink ethernet driver.
*
* Inputs:
*       struct pci_dev*        Pointer to the PCI device
*       struct pci_device_id*  Pointer to the PCI device ID
*
* Outputs:
*       0 if successful, <0 (Linux error code) if error
*
* Notes:
*       None
*
*
*STATIC*******************************************************************************/
static int __devinit clnketh_probe_one(struct pci_dev *pdev, const struct pci_device_id *ent)
{
    struct net_device       *dev;   // AKA dk_context_t *dkcp
    struct eth_dev          *tp;    // AKA dd_context_t *ddcp
    dd_context_t            *ddcp;
    dg_context_t            *dgcp;
    void                    *dccp;  // AKA dc_context_t *
    SYS_INT32               irq;
    unsigned char           my_mac_addr[6];
    struct clnk_hw_params   hparams;
    int                     err ; 

    board_idx++; // figure out better way to print device name/number
    HostOS_PrintLog(L_DEBUG, "Clink-Ethernet board #%d.\n", board_idx);
    // HostOS_PrintLog(L_DEBUG, "Page size %d\n", (int)PAGE_SIZE);

    // Enable the PCI device
    if (pci_enable_device(pdev))
    {
        HostOS_PrintLog(L_ERR, "Cannot enable clink board #%d, aborting\n",
                board_idx);
        return -ENODEV;
    }

    // allocate an aligned and zeroed private driver structure
    // the structure is 
    //    net_device with eth_dev tacked on the end
    //    net_device->priv points at the eth_dev
    dev = alloc_etherdev(sizeof(struct eth_dev));
    if (!dev)
    {
        HostOS_PrintLog(L_ERR, "ether device alloc failed, aborting\n");
        return -ENOMEM;
    }
    // Initialize the owner field in net device
    SET_MODULE_OWNER(dev);

    if (pci_request_regions (pdev, DRV_NAME))
    {
        HostOS_PrintLog(L_ERR, " failed request regions\n");
        goto err_out_free_netdev;
    }
    // Store net device in PCI device structure
    pci_set_drvdata(pdev, dev);      // pci_dev -> driver_data = net_device

/**************************Purpose************************************
Open it for debug purpose
*********************************************************************/          
#if 1 
    {   // Helpful code to print out PCI CONFIG space for debug
        int reg;
        for (reg = 0; reg < 68; reg += 4)
        {   unsigned int dword;
            pci_read_config_dword(pdev, reg, &dword);
            HostOS_PrintLog(L_INFO, " ... PCI_CONFIG[%d] = 0x%x\n", reg, dword);
        }
    }
#endif

    // init the driver data context

    tp = dev->priv;         // from net_device -> priv
    
    HostOS_lock_init(&tp->tx_lock);

    tp->base_ioaddr = pci_resource_start (pdev, 0);
    tp->base_iolen  = pci_resource_len (pdev, 0);
    tp->base_ioremap = (long)ioremap(tp->base_ioaddr, tp->base_iolen);
    irq              = pdev->irq;
    tp->pdev         = pdev;    // save the pci_dev pointer ???
    dev->base_addr   = tp->base_ioaddr;
    dev->irq         = irq;

    HostOS_PrintLog(L_DEBUG, "mem0: 0x%lx len:%d\n", (SYS_UINTPTR)tp->base_ioaddr, (SYS_UINT32)tp->base_iolen);
    HostOS_PrintLog(L_DEBUG, "remap mem0 addr: 0x%lx len:%d\n", (SYS_UINTPTR)tp->base_ioremap, (SYS_UINT32)tp->base_iolen);

    // create dg and dc contexts

    ddcp = tp ;
    err = Clnk_init_dev( &ddcp->p_dg_ctx, ddcp, dev, (unsigned long)tp->base_ioremap ) ;
    if( err ) {
        HostOS_PrintLog(L_ERR, "%s: Clnk_init_dev err=%d.\n", __FUNCTION__ , err );
        goto err_out_free_alloc;  
    }
    dccp = dd_to_dc( ddcp ) ;



    // initialize default handlers
    ether_setup(dev);

    // Initialize net device for Ethernet
    dev->open               = clnketh_open;
    dev->stop               = clnketh_close;
    dev->hard_start_xmit    = clnketh_start_xmit;
    dev->do_ioctl           = clnketh_ioctl;
    dev->get_stats          = clnketh_get_stats;
    dev->set_multicast_list = clnketh_set_rx_mode;
    dev->tx_timeout         = clnketh_tx_timeout;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 19)    
    clnk_set_ethtool_var(dev);
#endif	
    dev->watchdog_timeo     = TX_TIMEOUT;
    dev->tx_queue_len       = 20; // TX_ADVERTIZE_SIZE;

    HostOS_Memset(my_mac_addr, 0, 6);
#if !defined(MAC_ADDR_IN_PROM)
    {
        unsigned char myaddr[6];
        int i;
        for (i=0; i < 6; i++) {
            char c1 = mac_addr[i*3];
            char c2 = mac_addr[i*3+1];
            if (i != 5 && mac_addr[i*3+2] != ':') break;
            if (!isxdigit(c1) || !isxdigit(c2)) break;
            c1 = isdigit(c1) ? c1 - '0' : tolower(c1) - 'a' + 10;
            c2 = isdigit(c2) ? c2 - '0' : tolower(c2) - 'a' + 10;
            myaddr[i] = (unsigned char) c1 * 16 + (unsigned char) c2;
            // HostOS_PrintLog(L_INFO, "%2.2x:", myaddr[i]); // debug temporary hack
        }   // HostOS_PrintLog(L_INFO, "\n"); // debug temporary hack
        if (i == 6)
            HostOS_Memcpy(my_mac_addr, myaddr, 6);
    }
#endif


    pci_set_master(pdev);

    // Register Linux net device
    // Note: clnketh_get_stats (and possibly others) is called from inside register_netdev
    if (register_netdev(dev)) {
        goto err_out_free_umap;
    }

    tp->resetDelay        = 0;
    // this is used in data path init
    memcpy(tp->txFifoLut, priority_lut, sizeof(priority_lut));
#if ECFG_CHIP_ZIP1
    memcpy(tp->fw.txFifoPct, priority_pct, sizeof(priority_pct));
    memcpy(tp->fw.txFifoSize, priority_pct, sizeof(priority_pct));
#if FEATURE_QOS
    memcpy(tp->fw.txFifoSizeQos, priority_pct_qos, sizeof(priority_pct_qos));
#endif /* FEATURE_QOS */
    memset(tp->fw.txFifoChunks, 0, sizeof(tp->fw.txFifoChunks));
    tp->fw.rxFifoSize = MAX_RX_FIFO_SIZE;
#endif /* ECFG_CHIP_ZIP1 */
    // initializing the mac address from mac_addr
    tp->fw.macAddrHigh = (my_mac_addr[0] << 24) | (my_mac_addr[1] << 16) |
                         (my_mac_addr[2] << 8)  | (my_mac_addr[3]);
    tp->fw.macAddrLow  = (my_mac_addr[4] << 24) | (my_mac_addr[5] << 16);
    tp->fw.pFirmware = SYS_NULL ;





    // init the driver control context

    hparams.numRxHostDesc = RX_RING_SIZE;
#if ECFG_CHIP_ZIP1
    hparams.numTxHostDesc = TX_MAPPING_SIZE;
    hparams.minRxLens = (SYS_UINT32 *)eth_minrxlen;
#else
    hparams.numTxHostDesc = (TX_MAPPING_SIZE / MAX_TX_PRIORITY) - 1;
#endif
    if (Clnk_ETH_Initialize_drv( dccp, 
                             (Clnk_ETH_Firmware_t *)&(ddcp->fw),
                             &hparams) != SYS_SUCCESS)
    {
        HostOS_PrintLog(L_ERR, DRV_NAME " %d: clnk init failed\n", board_idx);
        goto err_out_free_alloc;
    }


// note: The MAC address is really stored in shared memory and not the following
#if !ECFG_CHIP_ZIP1
    {
        SYS_UINT32 hi, lo;

        Clnk_ETH_Control_drv(dccp, CLNK_ETH_CTRL_GET_MAC_ADDR, (SYS_UINTPTR)&hi, (SYS_UINTPTR)&lo, 0);
        // 00:01:00:02:00:03 (:00:04) means eeprom is not present or is empty
        if ((hi == 0x00010002) && (lo == 0x00030004))
        {
            Clnk_ETH_Control_drv(dccp, CLNK_ETH_CTRL_SET_MAC_ADDR, 0, 0, 0);
            HostOS_PrintLog(L_ERR, "Please specify mac_addr module parameter\n");
        } else {
            HostOS_PrintLog(L_DEBUG, "Mac Address: %02x:%02x:%02x:%02x:%02x:%02x\n",
                (hi >> 24) & 0xff, (hi >> 16) & 0xff, (hi >> 8) & 0xff, hi & 0xff,
                (lo >> 24) & 0xff, (lo >> 16) & 0xff);
        }
    }
#endif // !ECFG_CHIP_ZIP1

    // fetch the actual mac address
    clnketh_config_mac_address(dev, pdev);

    // Install the send and receive callback functions
    Clnk_ETH_Control_drv(dccp, CLNK_ETH_CTRL_SET_SEND_CALLBACK,
                     0, (SYS_UINTPTR)clnketh_xmit_callback, 0);
    Clnk_ETH_Control_drv(dccp, CLNK_ETH_CTRL_SET_RCV_CALLBACK,
                     0, (SYS_UINTPTR)clnketh_rx_callback, 0);
#if (FEATURE_QOS && ECFG_CHIP_ZIP1)
    // Install the tx fifo resize callback function
    Clnk_ETH_Control_drv(dccp, CLNK_ETH_CTRL_SET_TFIFO_RESIZE_CALLBACK,
                         0, (SYS_UINTPTR)clnketh_tfifo_resize_callback, 0);
#endif /* FEATURE_QOS && ECFG_CHIP_ZIP1 */


    clnketh_pci_config(pdev, dev);

    // Register interrupt handler
    err = request_irq(dev->irq, clnketh_isr, IRQF_SHARED, dev->name, dev);
    if(err)
    {
        HostOS_PrintLog(L_ERR, "Unable to allocate PCI interrupt Error: %d\n",err);
        goto err_out_free_alloc;
    }
    // Clear and Enable interrupts
    Clnk_ETH_Control_drv(dccp, CLNK_ETH_CTRL_ENABLE_INTERRUPT, 0, 0, 0);

    tp->netif_rx_dropped = 0;
    tp->tx_int_count     = 0;

    return (0);



err_out_free_alloc:
    tp = dev->priv;         // from net_device -> priv
    ddcp = tp ;
    dgcp = dd_to_dg( ddcp ) ;
    if( dgcp ) {
        dccp = dd_to_dc( ddcp ) ;
        if( dccp ) {
            kfree(dccp );
        }
        kfree( dgcp );
    }

err_out_free_umap:
    HostOS_PrintLog(L_ERR, "failed to get a region #%d.\n", board_idx);
    // Unregister Linux net device
    unregister_netdev(dev);
    // Free the net device
    pci_disable_device(pdev);
    iounmap((SYS_VOID *)tp->base_ioremap);
    pci_release_regions(pdev);

err_out_free_netdev:
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 0)
	// dev is not the allocation address anymore, can't use kfree() directly.
	free_netdev(dev);
#else
    kfree(dev);
#endif
    pci_set_drvdata (pdev, NULL);
    return -ENODEV;
}


/*******************************************************************************
*
* Description:
*       Remove or uninstall the driver
*
* Inputs:
*       struct pci_dev*  Pointer to the PCI device
*
* Outputs:
*       None
*
* Notes:
*       None
*
*
*STATIC*******************************************************************************/
static SYS_VOID __devexit clnketh_remove_one(struct pci_dev *pdev)
{
    struct net_device *dev = pci_get_drvdata(pdev);     // get our driver_data
    dd_context_t *ddcp = dk_to_dd( dev ) ;
    void         *dccp = dd_to_dc( ddcp ) ;   // AKA dc_context_t *

    // Unregister interrupt handler
    free_irq(dev->irq, dev);
    HostOS_PrintLog(L_NOTICE, "Removed Clink-Ethernet board #%d.\n", board_idx);
    // Get the net device from the PCI device structure
    if (!dev)
        return;

    // Terminate the Common Ethernet Module
    Clnk_ETH_Terminate_drv(dccp);

    // Unregister Linux net device
    unregister_netdev(dev);
    // Free the net device
    pci_disable_device(pdev);
    iounmap((SYS_VOID *)ddcp->base_ioremap);
    pci_release_regions (pdev);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 0)
	// dev is not the allocation address anymore, can't use kfree() directly.
    free_netdev(dev);
#else
    kfree(dev);
#endif
    pci_set_drvdata (pdev, NULL);

}

/*
*STATIC*******************************************************************************/
static SYS_VOID clnketh_pci_config(struct pci_dev *pdev, struct net_device *dev)
{
    unsigned short status_word;
    // SYS_UINT8 cache;
    // pci_read_config_byte(pdev, PCI_CACHE_LINE_SIZE, &cache);
    // HostOS_PrintLog(L_DEBUG, ":%s cache line %d \n", __FUNCTION__, cache);

    // Clear Initial PCI status word.
    pci_read_config_word(pdev, PCI_STATUS, &status_word); // Register 6
    pci_write_config_word(pdev, PCI_STATUS, status_word); // Clear any error bits

    // NOTE:
    // Some Linux's set this to 128 which is too high with multiple cards
    // Some Linux's leave this at 0; which can be very bad
    // Should be at least 16 (matches our burst_size register)
    // Should be a multiple of 8.  Suggested values ... 32, 40, 48 ...
    // Value becomes much more important when multiple PCI devices are connected
    pci_write_config_byte(pdev, PCI_LATENCY_TIMER, 48); // Register 0x0d

    // NOTE:
    // Some vendors with multiple PCI cards found it useful to increase the number
    // of retries of the bus before causing a pci_abort within the SOC. The 0x40
    // register is an extension of the normal PCI config space.
    //
    // lo byte = max PCI clocks the internal master waits for TRDY (default 0x80)
    // hi byte = max retries the internal master will perform (default 0x80)
    pci_write_config_word(pdev, 0x40, 0xff00);
}


// fetch the mac address from the Clink Ethernet driver
/*
*STATIC*******************************************************************************/
static SYS_VOID clnketh_config_mac_address(struct net_device *dev, struct pci_dev *pdev)
{
    dd_context_t *ddcp = dk_to_dd( dev ) ;
    void         *dccp = dd_to_dc( ddcp ) ;
    SYS_UINT32 mac_hi, mac_lo;

    (SYS_VOID)Clnk_ETH_Control_drv(dccp,  CLNK_ETH_CTRL_GET_MAC_ADDR,
                                (SYS_UINTPTR)&mac_hi,
                                (SYS_UINTPTR)&mac_lo, 0);

    // shift out the hi and low words to be endian-agnostic
    dev->dev_addr[0] = (SYS_UCHAR) ((mac_hi >> 24) & 0xff);
    dev->dev_addr[1] = (SYS_UCHAR) ((mac_hi >> 16) & 0xff);
    dev->dev_addr[2] = (SYS_UCHAR) ((mac_hi >>  8) & 0xff);
    dev->dev_addr[3] = (SYS_UCHAR) ((mac_hi      ) & 0xff);
    dev->dev_addr[4] = (SYS_UCHAR) ((mac_lo >> 24) & 0xff);
    dev->dev_addr[5] = (SYS_UCHAR) ((mac_lo >> 16) & 0xff);

#if 0 // Debug
    { SYS_INT32 i;
        for (i = 0; i < 6; i++)
            HostOS_PrintLog(L_INFO, "%2.2x:", dev->dev_addr[i]);
        HostOS_PrintLog(L_INFO, "\n");        // this is WRONG - must sprint to buffer then printk the buffer
    }
#endif
}

/*******************************************************************************
*
* Description:
*
* Inputs:
*
* Outputs:
*
* Notes:
*       None
*
*
*STATIC*******************************************************************************/
static SYS_INT32 clnketh_enqueue_rx_buffs(struct net_device *dev)
{
    dd_context_t *ddcp = dk_to_dd( dev ) ;
    void         *dccp = dd_to_dc( ddcp ) ;
    SYS_UINT32 list, buf;
    Clnk_ETH_FragList_t fragList, *fg = &fragList;
    struct sk_buff *skb;
    dma_addr_t mapping;
    SYS_INT32 packetID;

    packetID = 0;
    list = 0;
    for (buf=0;buf<RX_RING_SIZE;buf++)
    {
        if (!(skb = dev_alloc_skb(MAX_ETH_RX_BUF_SIZE)))
             return (-ENOMEM);

        ddcp->rx_ring[packetID].skb = skb;
        skb->dev = dev;

        mapping = pci_map_single(ddcp->pdev, skb->data, MAX_ETH_RX_BUF_SIZE, PCI_DMA_FROMDEVICE);
        ddcp->rx_ring[packetID].mapping = mapping;
#if ECFG_CHIP_ZIP1
#define SLIP_SLIDE 6
        fg->fragments[0].ptr   = mapping;
        fg->fragments[0].ptrVa = (SYS_VOID *) (skb->data);
        fg->fragments[0].len   = SLIP_SLIDE;
        fg->fragments[1].ptr   = mapping + SLIP_SLIDE + 2;
        fg->fragments[1].ptrVa = (SYS_VOID *) (skb->data + SLIP_SLIDE + 2);
        fg->fragments[1].len   = MAX_ETH_MTU_SIZE - SLIP_SLIDE;
        fg->totalLen           = MAX_ETH_MTU_SIZE;
        fg->numFragments       = 2;
#else
        fg->fragments[0].ptr   = mapping + 2UL;
        fg->fragments[0].ptrVa = (SYS_VOID *) (skb->data + 2UL);
        fg->fragments[0].len   = MAX_ETH_MTU_SIZE;
        fg->totalLen           = MAX_ETH_MTU_SIZE;
        fg->numFragments       = 1;
#endif  /* ECFG_CHIP_ZIP1 */

        if (Clnk_ETH_RcvPacket(dccp, fg, list, (SYS_UINT32)packetID, CLNK_PER_PKT_INTS) != SYS_SUCCESS)
             return (-ENOMEM);
        
        packetID++;
    }
    return 0;
}

/*******************************************************************************
*
* Description:
*       Brings up the Ethernet interface.
*
* Inputs:
*       struct net_device * Pointer to the net device
*
* Outputs:
*       0 if successful, <0 (Linux error code) if error
*
* Notes:
*       None
*
*
*PUBLIC*******************************************************************************/
SYS_INT32 clnketh_open(struct net_device *dev)
{
    dd_context_t *ddcp = dk_to_dd( dev ) ;
    void         *dccp = dd_to_dc( ddcp ) ;
    SYS_INT32 index;
    if(!Clnk_ETH_IsSoCBooted(dccp))
    {
        HostOS_PrintLog(L_ERR, "SoC is not booted, cannot open Clink port now!\n");
        return (0);
    }
    HostOS_Lock_Irqsave(&ddcp->tx_lock);

    // clear TX/RX ring buffers
    for (index = 0; index < TX_MAPPING_SIZE; index++)
    {
        ddcp->tx_ring[index].skb = NULL;
    }
    ddcp->tx_ring_robin = 0;
    for (index = 0; index < RX_RING_SIZE; index++)
    {
        ddcp->rx_ring[index].skb = NULL;
    }

    // add RX buffers
    if (clnketh_enqueue_rx_buffs(dev) < 0)
    {
        HostOS_PrintLog(L_ERR, "failed to allocate RX buffs.\n");
        HostOS_Unlock_Irqrestore(&ddcp->tx_lock);
        return (-1);
    }

#if (FEATURE_QOS && ECFG_CHIP_ZIP1)
    // Initialize host OS context for c.LINK tx fifo resizing.
    ddcp->tfifo_status = 0;
#endif /* FEATURE_QOS && ECFG_CHIP_ZIP1 */

    // start the TX/RX DMA engines
    if (Clnk_ETH_Open(dccp) != SYS_SUCCESS)
    {
        HostOS_PrintLog(L_ERR, "common ethernet open failed.\n");
        HostOS_Unlock_Irqrestore(&ddcp->tx_lock);
        return (-1);
    }

    // Start Linux net interface
    netif_carrier_off(dev);
    netif_stop_queue(dev);

    ddcp->first_time = SYS_TRUE;
    ddcp->throttle = 0;

    HostOS_Unlock_Irqrestore(&ddcp->tx_lock);

    // Increment module use count
    MOD_INC_USE_COUNT;

    return (0);
}

/*******************************************************************************
*
* Description:
*       Shuts down the Ethernet interface.
*
* Inputs:
*       struct net_device*  Pointer to the net device
*
* Outputs:
*       0 if successful, <0 (Linux error code) if error
*
* Notes:
*       None
*
*
*PUBLIC*******************************************************************************/
SYS_INT32 clnketh_close(struct net_device *dev)
{
    dd_context_t *ddcp = dk_to_dd( dev ) ;
    void         *dccp = dd_to_dc( ddcp ) ;
    SYS_INT32 status, detach_count;
    SYS_UINT32 packetID;
    SYS_UINT32 index;

    HostOS_Lock_Irqsave(&ddcp->tx_lock);

    // Stop Linux net interface
    netif_stop_queue(dev);

    // Shut down the Ethernet interface
    if (Clnk_ETH_Close(dccp) != SYS_SUCCESS)
    {
        HostOS_PrintLog(L_ERR, "common close failed\n");
        //HostOS_Unlock_Irqrestore(&tp->tx_lock);
        // return (-1);
    }

    packetID = 0;
    // Free outstanding RX buffers
    while (packetID < RX_RING_SIZE)
    {
        packetID++; //- whacky loop
    }

    // Free outstanding RX buffers
    detach_count = 0;
    while (1)
    {
        status = Clnk_ETH_Control_drv(dccp, CLNK_ETH_CTRL_DETACH_RCV_PACKET,
                                  0, (SYS_UINTPTR)&packetID, 0);
        if (status != SYS_SUCCESS)
        {
            break;
        }
        if (ddcp->rx_ring[packetID].skb == NULL)
        {
            HostOS_PrintLog(L_ERR, "rx detach failed! null skb packet id:%d\n",
                    packetID);
        }
        else
        {
            pci_unmap_single(ddcp->pdev, ddcp->rx_ring[packetID].mapping,
                             MAX_ETH_MTU_SIZE, PCI_DMA_FROMDEVICE);
            dev_kfree_skb(ddcp->rx_ring[packetID].skb);
            ddcp->rx_ring[packetID].skb = NULL;
            detach_count++;
        }
    }

    // Free outstanding TX buffers
    detach_count = 0;
    while (1)
    {
        status = Clnk_ETH_Control_drv(dccp, CLNK_ETH_CTRL_DETACH_SEND_PACKET,
                                  0, (SYS_UINTPTR)&packetID, 0);
        if (status != SYS_SUCCESS)
        {
            break;
        }
        index = packetID & 0xffff;
        if (index >= TX_MAPPING_SIZE)
        {
            HostOS_PrintLog(L_ERR, "tx detach failed! packet id:0x%x\n", packetID);
        }
        else if ( ddcp->tx_ring[index].skb == NULL)
        {
            HostOS_PrintLog(L_ERR, "tx detach failed! null skb packet id:0x%x\n",
                    packetID);
        }
        else
        {
            pci_unmap_single(ddcp->pdev, ddcp->tx_ring[index].mapping,
                             MAX_ETH_MTU_SIZE, PCI_DMA_TODEVICE);
            dev_kfree_skb(ddcp->tx_ring[index].skb);
            ddcp->tx_ring[index].skb = NULL;
            detach_count++;
        }
    }

    HostOS_Unlock_Irqrestore(&ddcp->tx_lock);

    // Decrement module use count
    MOD_DEC_USE_COUNT;

    return (0);
}

/*******************************************************************************
*
* Description:
*       Sends a packet over the Ethernet interface.
*
* Inputs:
*       struct sk_buff      * Pointer to the socket buffer to send
*       struct net_device   * Pointer to the net device
*
* Outputs:
*       0 if successful, <0 (Linux error code) if error
*
* Notes:
*       None
*
*
*STATIC*******************************************************************************/

static SYS_INT32 clnketh_start_xmit(struct sk_buff *skb, struct net_device *dev)
{
    dd_context_t *ddcp = dk_to_dd( dev ) ;
    void         *dccp = dd_to_dc( ddcp ) ;
    Clnk_ETH_FragList_t fraglist;
    dma_addr_t mapping;
    SYS_INT32 priority, index;
    int ret_val;

    // Get raw VLAN Priority (p < 8), and map to internal Priority (p < 4)
    priority = CLNK_ETH_GET_VLAN_PRIORITY_FROM_BUF (skb->data);
#if FEATURE_QOS
    if (!priority) 
        priority = skb->priority; // eg setsockopt(s, SOL_SOCKET, SO_PRIORITY, ...
#endif
    priority = ddcp->txFifoLut[ priority < CLNK_ETH_VLAN_8021Q ? priority : 0 ];

    /*
    ** NOTE: The following sample code is used to generate Ethernet FCS
    ** (typically done in hw) and attach it at the end of the packet payload
    ** with the assumption that there is at least 4 bytes of additional space
    ** available beyond skb->len. Enabling this code will affect tx throughput
    ** performance.
    **/
#if (CLNK_TX_ETH_FCS_ENABLED)
    {
        SYS_UINT8  *src = (SYS_UINT8 *)skb->data;
        SYS_UINT32 crc32 = compute_crc32(src, skb->len, 0);
        //HostOS_PrintLog(L_INFO, "tx len=%u, crc=0x%08x, orig crc=0x%08x\n",
        //                skb->len, crc32, *(SYS_UINT32 *)(src + skb->len));
        *(SYS_UINT32 *)(src + skb->len) = crc32;
        skb->len += 4;                 /* Ethernet FCS included here */
    }
#endif /* CLNK_TX_ETH_FCS_ENABLED */

#define SIMULATE_FRAGMENTS 0   /* TEST CODE: NORMALLY ZERO */
#define _HUNK              32  /* size of 1st frag, double each successor */

    /* NOTE:  Cache Clean Operation - Potentially Costly
    ** This can cost up to 15 usecs on a machine with a write back cache, but
    ** probably only 1 or 2 usec on a machine with write thru cache.  It is
    ** pluasible to relocate the actual flush operation by using PCI_DMA_NONE
    ** here and adding a call to consistent_sync() queueing the pkt to HW.
    ** (A) Doing operation here is good because it happens before we block ISR.
    ** Also simpler.  Doesn't matter for Write Thru caches, etc.
    ** (B) Doing it later is good because some of the write back buffers may
    ** trickle out to memory during idle bus cycles.
    */

    mapping = pci_map_single(ddcp->pdev, skb->data, skb->len, PCI_DMA_TODEVICE);

    fraglist.totalLen = skb->len;

#if SIMULATE_FRAGMENTS
    {
        SYS_UINT32 hunk = _HUNK; // 32: 32,64,128,256
        SYS_UINT8* vaddr = skb->data;
        SYS_UINT32 len = skb->len;
        Clnk_ETH_Fragment_t *fragptr = &(fraglist.fragments[0]);

        fraglist.numFragments = 0;
        for ( ; len > 0 && fraglist.numFragments < CLNK_HW_MAX_FRAGMENTS; 
                fragptr++, hunk <<= 1)
        {
            SYS_UINT32 fraglen;

            fraglist.numFragments++;
            if (fraglist.numFragments >= CLNK_HW_MAX_FRAGMENTS)
                fraglen = len;
            else
                fraglen = (len > hunk) ? hunk : len;
            fragptr->ptr = mapping;  mapping += fraglen;
            fragptr->ptrVa = vaddr;  vaddr += fraglen;
            fragptr->len = fraglen;  len -= fraglen;
        }
    #if (_SHUFFLE)
        // Swap Ptr's for 3rd and 4th frag
        fragptr = &(fraglist.fragments[2]);
        fragptr->ptr += (_HUNK*8);
        fragptr->ptrVa += (_HUNK*8);
        fragptr = &(fraglist.fragments[3]);
        fragptr->ptr -= (_HUNK*4);
        fragptr->ptrVa -= (_HUNK*4);
    #endif
    }
#else
    // Normal Linux Code with single fragment
    fraglist.numFragments = 1;
    fraglist.fragments[0].ptr = mapping;
    fraglist.fragments[0].ptrVa = (SYS_VOID *)(skb->data);
    fraglist.fragments[0].len = skb->len;
#endif

    HostOS_Lock_Irqsave(&ddcp->tx_lock);   /* ---SPIN LOCK STARTS HERE--- */

    // Find available mapping, beware of fullness.
    index = ddcp->tx_ring_robin;
    while (ddcp->tx_ring[index].skb) {
        if (++index >= TX_MAPPING_SIZE)
            index = 0;
    }
    ddcp->tx_ring_robin = index;

    ddcp->tx_ring[index].skb = skb;
    ddcp->tx_ring[index].mapping = mapping;

    // queue full for the HW, then tell the IP stack to give some head room
    if ((ret_val = Clnk_ETH_SendPacket(dccp, &fraglist,
                            priority, (priority << 16) | index,
                            CLNK_PER_PKT_INTS)) != SYS_SUCCESS)
    {
#if (MAX_TX_PRIORITY > 1)
        // Other VLAN queues may still work, so don't stop the interface queue
        switch (ret_val)
        {
            case CLNK_ETH_RET_CODE_NO_HOST_DESC_ERR:
            case CLNK_ETH_RET_CODE_UCAST_FLOOD_ERR:
            case CLNK_ETH_RET_CODE_PKT_LEN_ERR:
                break;

            default: // Unless the error is serious
                netif_stop_queue(dev); // throttle the stack
                break;
        }
#else   /* NO VLAN */
        netif_stop_queue(dev); // throttle the interface stack
#endif

        // can't transmit, free the buffer
        pci_unmap_single(ddcp->pdev, mapping, skb->len, PCI_DMA_TODEVICE);

        ddcp->tx_ring[index].skb = NULL;
        dev_kfree_skb_irq(skb);
    }
    else
    {
        ddcp->tx_int_count++;
    }

#if (MAX_TX_PRIORITY > 1)
#else  /* NO VLAN */
    {   // if tx resources exhausted, throttle the interface
        SYS_UINT32 numTxHostDesc;
        Clnk_ETH_Control_drv(dccp, CLNK_ETH_CTRL_GET_NUM_TX_HOST_DESC,
                         priority, (SYS_UINTPTR)&numTxHostDesc, 0);
        if (numTxHostDesc == 0)
        {
            ddcp->throttle++;
            netif_stop_queue(dev); // throttle the interface stack
        }
    }
#endif
    HostOS_Unlock_Irqrestore(&ddcp->tx_lock);
    dev->trans_start = jiffies;

    return 0;
}

/*
*STATIC*******************************************************************************/
static SYS_VOID clnketh_tx_timeout(struct net_device *dev)
{
}

/*
*STATIC*******************************************************************************/
static SYS_VOID clnketh_xmit_callback(struct net_device *dev, SYS_UINT32 packetIDs[], SYS_UINT32 ids)
{
    dd_context_t *ddcp = dk_to_dd( dev ) ;
    SYS_INT32 packetID;
    SYS_INT32 index;
    SYS_INT32 xmitted = 0;
    dma_addr_t mapping;
    struct sk_buff *skb;

    while (xmitted < ids)
    {
        packetID = packetIDs[xmitted];
        index = packetID & 0xffff;
        if (index >= TX_MAPPING_SIZE)
        {
            HostOS_PrintLog(L_ERR, "invalid xmitted packet id:0x%x\n", packetID);
        }
        else if ((skb = (struct sk_buff *)ddcp->tx_ring[index].skb) == NULL)
        {
            HostOS_PrintLog(L_ERR, "invalid tx skb for packet id:%d\n", packetID);
        }
        else
        {
            mapping = (dma_addr_t)ddcp->tx_ring[index].mapping;
            pci_unmap_single(ddcp->pdev, mapping, skb->len, PCI_DMA_TODEVICE);
            dev_kfree_skb_irq(skb);
            ddcp->tx_ring[index].skb = NULL;
        }
        xmitted++;
    }
    if (xmitted
#if (FEATURE_QOS && ECFG_CHIP_ZIP1)
        && !(ddcp->tfifo_status & TX_FIFO_RESIZE_PENDING)
#endif /* FEATURE_QOS && ECFG_CHIP_ZIP1 */
       )
    {
        netif_wake_queue(dev);
    }
}

#if (FEATURE_QOS && ECFG_CHIP_ZIP1)
static struct timeval resize_tm = {0}; // for profiling c.LINK if throttling

/**
 * \brief Callback for TX fifo resize trigger or completion notification.
 *
 * This function is invoked by the c.LINK Common Driver to trigger TX fifo
 * resizing or notify completion of it as a result of QoS state change (start
 * or stop). The responsibility of this callback is to apply or release packet
 * throttling of the c.LINK net if.
 *
 * \param[in] dev - Pointer to the net device.
 *
 * \retval None.
 */
static SYS_VOID clnketh_tfifo_resize_callback(struct net_device *dev)
{
    struct eth_dev *tp = dev->priv;
    struct timeval tv;

    if (tp->tfifo_status & TX_FIFO_RESIZE_PENDING)
    {
        do_gettimeofday (&tv);
        HostOS_PrintLog(L_DEBUG,
               "***** c.LINK net if throttled at time %u.%06u secs\n"
               "      c.LINK net if awakened  at time %u.%06u secs\n",
               (SYS_UINT32)resize_tm.tv_sec, (SYS_UINT32)resize_tm.tv_usec,
               (SYS_UINT32)tv.tv_sec, (SYS_UINT32)tv.tv_usec);
        netif_wake_queue(dev);
        tp->tfifo_status &=  ~TX_FIFO_RESIZE_PENDING;
    }
    else
    {
        netif_stop_queue(dev);
        tp->throttle++;
        tp->tfifo_status |= TX_FIFO_RESIZE_PENDING;
        do_gettimeofday (&tv);
        resize_tm = tv;
    }
}
#endif /* (FEATURE_QOS && ECFG_CHIP_ZIP1) */


/*
*S TATIC*******************************************************************************/
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 19)
static irqreturn_t clnketh_isr(SYS_INT32 irq, SYS_VOID *dev_instance )
#else
static irqreturn_t clnketh_isr(SYS_INT32 irq, SYS_VOID *dev_instance, struct pt_regs *regs)
#endif
{
    dd_context_t *ddcp = dk_to_dd( dev_instance ) ;
    void         *dccp = dd_to_dc( ddcp ) ;

    HostOS_Lock_Irqsave(&ddcp->tx_lock);

#ifdef ISR_DEBUG
    HostOS_PrintLog(L_INFO, "got an isr\n");  // very noisy to turn this on
#endif

    // acknowledge all interrupt sources
    Clnk_ETH_Control_drv(dccp, CLNK_ETH_CTRL_RD_CLR_INTERRUPT,0,0,0);

#if (defined(CONFIG_ARCH_IXP425) || defined(CONFIG_ARCH_IXP4XX))
    // On ARM/XSCALE cpus, the CPU's actual interrupt can't be cleared until
    // after the source was removed, so try to clear it again.   This avoids
    // the so called "double interrupt" problem.  We tried GPIO_6, but it
    // seems only GPIO_11 is relevant.
    gpio_line_isr_clear(11/*IXP425_GPIO_PIN_11*/);
#endif

    // call RX interrupt routine
    Clnk_ETH_HandleRxInterrupt(dccp);
    Clnk_ETH_HandleTxInterrupt(dccp);
    Clnk_ETH_HandleMiscInterrupt(dccp);

    HostOS_Unlock_Irqrestore(&ddcp->tx_lock);

    return IRQ_HANDLED;
}




/*
*STATIC*******************************************************************************/
static SYS_VOID clnketh_rx_callback(struct net_device *dev, SYS_UINT32 listnums[], SYS_UINT32 packetIDs[],
                      SYS_UINT16 len[], SYS_UINT16 rxstatus[], SYS_INT32 bufs)
{
    dd_context_t *ddcp = dk_to_dd( dev ) ;
    void         *dccp = dd_to_dc( ddcp ) ;
    SYS_INT32 pid, pkt_len, rxed = 0;
    SYS_UINT32 status;
    dma_addr_t mapping;
    struct sk_buff *skb, *copy_skb;
    Clnk_ETH_FragList_t fg, *fgp = &fg;
#ifdef RX_DEBUG
    SYS_UCHAR *p;
    SYS_UINT32 reg;
    SYS_INT32 i;
#endif
    SYS_UINT32 netif_rx_status;

#if defined(RX_DEBUG) && ECFG_CHIP_ZIP1
    reg = (ddcp->base_ioremap + 0x30000 + 0x31c);
    HostOS_PrintLog(L_INFO, "addr:0x%x data:0x%x\n", reg, HostOS_Read_Word(reg));
    reg = (ddcp->base_ioremap + 0x30000 + 0x320);
    HostOS_PrintLog(L_INFO, "addr:0x%x data:0x%x\n", reg, HostOS_Read_Word(reg));
    reg = (ddcp->base_ioremap + 0x30000 + 0x324);
    HostOS_PrintLog(L_INFO, "addr:0x%x data:0x%x\n", reg, HostOS_Read_Word(reg));

    HostOS_PrintLog(L_INFO, "rx callback nrx:%d\n", bufs);
#endif

    while (rxed < bufs)
    {
        pid = packetIDs[rxed];
        if (pid >= RX_RING_SIZE)
        {
            HostOS_PrintLog(L_WARNING, "%s invalid packet id %d\n", dev->name, pid);
            rxed++;
            continue;
        }
        if (ddcp->rx_ring[pid].skb == NULL)
        {
            HostOS_PrintLog(L_WARNING, "%s invalid rx skb for packet id %d\n", dev->name, pid);
            rxed++;
            continue;
        }
        skb = (struct sk_buff *)ddcp->rx_ring[pid].skb;
        pkt_len = len[rxed];
        status = rxstatus[rxed];

#ifdef RX_DEBUG
        HostOS_PrintLog(L_INFO, "rx len:%d status:%x rxed:%x bufs:%x\n", pkt_len, status, rxed, bufs);
#endif

        // unlock memory; may improve eg checksum speed
        pci_unmap_single (ddcp->pdev, (dma_addr_t)ddcp->rx_ring[pid].mapping, MAX_ETH_RX_BUF_SIZE, PCI_DMA_FROMDEVICE);

        if (!status)
        {
            if (pkt_len > MAX_ETH_MTU_SIZE)
            {
                HostOS_PrintLog(L_WARNING, "%s X large packet size %d(%#x)\n", dev->name, pkt_len, pkt_len);
                pkt_len = MAX_ETH_MTU_SIZE;
            }

            // NOTE: MAX_ETH_RX_BUF_SIZE or MAX_ETH_MTU_SIZE+2
            if ((copy_skb = dev_alloc_skb(MAX_ETH_RX_BUF_SIZE)) != NULL)
            {
#if ECFG_CHIP_ZIP1
                memmove(skb->data+2,skb->data,SLIP_SLIDE);
                skb_reserve(skb,2UL);                
#else
                // Ethernet header alignment at 2B boundary
                skb_reserve(skb,2UL);
#endif /* ECFG_CHIP_ZIP1 */
                skb_put(skb,pkt_len);
                skb->protocol = eth_type_trans(skb, dev);

                netif_rx_status = netif_rx(skb);
                if (netif_rx_status == NET_RX_DROP)
                {
                    ddcp->netif_rx_dropped++;
                }
                dev->last_rx = jiffies;

                skb = copy_skb;
                skb->dev = dev;
            }
            else
            {   // Re-hang this SKB instead of passing up stack, count as dropped
                ddcp->netif_rx_dropped++;
            }
        } /* end if !status */

        ddcp->rx_ring[pid].skb = skb;

        // NOTE: MAX_ETH_RX_BUF_SIZE or MAX_ETH_MTU_SIZE+2
        mapping = pci_map_single(ddcp->pdev, skb->data, MAX_ETH_RX_BUF_SIZE, PCI_DMA_FROMDEVICE);
        // NOTE: Not strictly necessary at this time to store rx_ring[pid].mapping
        ddcp->rx_ring[pid].mapping = mapping;
#if ECFG_CHIP_ZIP1
        fgp->fragments[0].ptr   = mapping;
        fgp->fragments[0].ptrVa = (SYS_VOID *) (skb->data);
        fgp->fragments[0].len   = SLIP_SLIDE;
        fgp->fragments[1].ptr   = mapping + SLIP_SLIDE + 2;
        fgp->fragments[1].ptrVa = (SYS_VOID *) (skb->data + SLIP_SLIDE + 2);
        fgp->fragments[1].len   = MAX_ETH_MTU_SIZE - SLIP_SLIDE;
        fgp->numFragments = (2);
        fgp->totalLen = (MAX_ETH_MTU_SIZE);        
#else
        fgp->fragments[0].ptr   = mapping + 2UL;
        fgp->fragments[0].ptrVa = (SYS_VOID *) (skb->data + 2UL);
        fgp->fragments[0].len   = MAX_ETH_MTU_SIZE;
        fgp->numFragments = (1);
        fgp->totalLen = MAX_ETH_MTU_SIZE;
#endif  /* ECFG_CHIP_ZIP1 */

        status = Clnk_ETH_RcvPacket(dccp, fgp, listnums[rxed], pid, CLNK_PER_PKT_INTS);
        if (status != SYS_SUCCESS)
        {
            HostOS_PrintLog(L_WARNING, "%s attach rx failed packet id:%d\n", dev->name, pid);
            break;
        }
        rxed++;
    } /*end while */
}

/****************************************************************************
*           
*   Purpose:    network device statistics gatherer.
*
*   Imports:    dev - net device to get stats from
*
*   Exports:    pointer to net_device_stats block
*
*STATIC*******************************************************************************/
static struct net_device_stats *clnketh_get_stats(struct net_device *dev)
{
    dd_context_t *ddcp = dk_to_dd( dev ) ;
    void         *dccp = dd_to_dc( ddcp ) ;
    ClnkDef_EthStats_t eth_stats;

    // get stats from context
    if (dccp)
    {
        Clnk_ETH_Control_drv(dccp, CLNK_ETH_CTRL_GET_STATS, (SYS_UINTPTR)&eth_stats, 0, 0);
    }

    // then, update driver stats structure
    ddcp->stats.rx_packets = eth_stats.rxPackets;       /* total packets received */
    ddcp->stats.tx_packets = eth_stats.txPackets;       /* total packets transmitted */
    ddcp->stats.rx_bytes   = eth_stats.rxBytes;         /* total bytes received */
    ddcp->stats.tx_bytes   = eth_stats.txBytes;         /* total bytes transmitted */
    ddcp->stats.rx_errors  = eth_stats.rxPacketErrs;
                           //+ eth_stats.rxCrc32Errs;     /* bad packets received */
    ddcp->stats.tx_errors  = eth_stats.txPacketErrs;    /* packet transmit problems */
    //ddcp->stats.rx_dropped = eth_stats.rxDroppedErrs; /* no space in linux buffers */
    ddcp->stats.rx_dropped = ddcp->netif_rx_dropped;    /* no space in linux buffers */
    ddcp->stats.tx_dropped = eth_stats.txDroppedErrs;
    ddcp->stats.multicast  = eth_stats.rxMulticastPackets; /* multicast packets received */
    ddcp->stats.rx_length_errors  = eth_stats.rxLengthErrs;
    ddcp->stats.rx_over_errors    = 0;
    ddcp->stats.rx_crc_errors     = eth_stats.rxCrc32Errs;  /* recved pkt with crc error */
    ddcp->stats.rx_frame_errors   = eth_stats.rxFrameHeaderErrs; /* recv'd frame alignment error */
    ddcp->stats.rx_fifo_errors    = eth_stats.rxFifoFullErrs;    /* recv'r fifo overrun */
    ddcp->stats.tx_aborted_errors = eth_stats.txCrc32Errs;
    ddcp->stats.tx_fifo_errors    = eth_stats.txFifoFullErrs;//ddcp->throttle;

    return &ddcp->stats;
}

/****************************************************************************
*           
*   Purpose:    network rx mode setting function.
*
*   Imports:    dev - net device to set the mode of
*
*   Exports:    
*
*STATIC*******************************************************************************/
static SYS_VOID clnketh_set_rx_mode(struct net_device *dev)
{
#if 1
    /* JOIN_MULTICAST mailbox is not implemented, so dont bother sending it */
    dev = dev;
#else
    struct dev_mc_list *mclist = dev->mc_list;
    SYS_INT32 mc_count = 0;

#if 0 /* debug */
    HostOS_PrintLog(L_NOTICE, "Ethernet multicast flags:%x mc:%d\n",
                    dev->flags, dev->mc_count);
    while (mclist && mc_count < dev->mc_count)
    {
        HostOS_PrintLog(L_INFO, "%s failed to filter mc addr:"
               " %2.2x:%2.2x:%2.2x:%2.2x:%2.2x:%2.2x\n", dev->name,
                mclist->dmi_addr[0], mclist->dmi_addr[1],
                mclist->dmi_addr[2], mclist->dmi_addr[3],
                mclist->dmi_addr[4], mclist->dmi_addr[5]);
        mclist = mclist->next, mc_count++;
    } 
    mc_count = 0; 
    mclist   = dev->mc_list;
#endif

    if (dev->flags & IFF_PROMISC)
    {
        // HostOS_PrintLog(L_INFO, "%s promiscuous mode not supported\n", dev->name);
    }
    else if (dev->flags & IFF_ALLMULTI)
    {
        HostOS_PrintLog(L_INFO, "%s no support to receive all multicast addresses\n", dev->name);
    }
    else if (dev->mc_count > 64)
    {
        HostOS_PrintLog(L_INFO, "%s reached maximum number of multicast addresses\n", dev->name);
    }
    else
    {
        while (mclist && mc_count < dev->mc_count)
        {
#if 0
            dd_context_t *ddcp = dk_to_dd( dev ) ;
            void         *dccp = dd_to_dc( ddcp ) ;
            SYS_INT32 status;

            // Currently a NOP
            status = Clnk_ETH_Control_drv(dccp, CLNK_ETH_CTRL_JOIN_MULTICAST,
                                      0, (SYS_UINTPTR)&mclist->dmi_addr, 0);
            if (status != SYS_SUCCESS)
            {
                HostOS_PrintLog(L_INFO, "%s failed to filter mc addr:"
                        " %2.2x:%2.2x:%2.2x:%2.2x:%2.2x:%2.2x\n", dev->name,
                        mclist->dmi_addr[0], mclist->dmi_addr[1],
                        mclist->dmi_addr[2], mclist->dmi_addr[3],
                        mclist->dmi_addr[4], mclist->dmi_addr[5]);
            }
#endif
            mclist = mclist->next, mc_count++;
        }
    }
#endif
}


// NOTE: appears to be "const" but linux routines dont declare as such.
static struct pci_driver clnketh_driver =
{
    name:     DRV_NAME,
    id_table: clnketh_pci_tbl,
    probe:    clnketh_probe_one,
    remove:   __devexit_p(clnketh_remove_one)
};

/**
 * pci_module_init() obsoleted & replaced by
 * pci_register_driver() for kernel versions > 2.6.22
 */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 22)
#define pci_module_init pci_register_driver
#endif

/****************************************************************************
*           
*   Purpose:    Driver init entry point.
*
*   Imports:    
*
*   Exports:    0  - registration was successful
*               !0 - registration failed
*
*STATIC*******************************************************************************/
static SYS_INT32 __init clnketh_init(SYS_VOID)
{
    int rc ;

    HostOS_PrintLog(L_INFO, "%s", version);

    // probe for and init the PCI module
    rc = pci_module_init(&clnketh_driver) ;
    HostOS_PrintLog(L_INFO, "pci_module_init returns %d\n", rc );
    return( rc );
}

/****************************************************************************
*           
*   Purpose:    Driver exit entry point.
*
*   Imports:    
*
*   Exports:    
*
*STATIC*******************************************************************************/
static SYS_VOID __exit clnketh_cleanup(SYS_VOID)
{
    // Cleanup PCI module
    pci_unregister_driver(&clnketh_driver);
}


module_init(clnketh_init);
module_exit(clnketh_cleanup);
MODULE_LICENSE("GPL");

