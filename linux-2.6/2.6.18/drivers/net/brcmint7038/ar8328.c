// Copyright 2010 by Tivo Inc, All Rights Reserved

#include <linux/types.h>
#include <linux/delay.h>
#include <linux/mii.h>
#include <linux/stddef.h>
#include <linux/ctype.h>
#include <linux/string.h>
#include "intemac_map.h"
#include "bcmemac.h"
#include "bcmmii.h"
#include "ar8328.h"

#define GIO_IODIR_LO *(volatile uint32 *)(0xb0400708)
#define GIO_DATA_LO *(volatile uint32 *)(0xb0400704)

// DVT testmode
// 0  : normal
// 1-4: 1000BaseT test mode 1-4
// 5  : 10BaseT loopback
// 6  : 100BaseT loopback
// 7  : 1000BaseT loopback
static int testmode=0;
static int __init do_testmode(char *s)
{
    testmode = *s & 7; // assume it's "0" thru "7"
    return 1;
}    
__setup("artestmode=", do_testmode);

// access extended 32-bit switch registers
// first set "page" from addr bits 18-9 written to phy 24 register 0
// addr bits 8-6 map to phy 16-23, and addr bits 5-2 map to phy register 0-30 
// addr bits 1-0 are unused (always zero)
// low order register bit selects high or low half of 32-bit data
void ar8328_write(struct net_device *dev, uint32 addr, uint32 data)
{
    uint32 xphy = 0x10 | ((addr >> 6) & 7), 
    xreg = (addr >> 1) & 0x1E;
    mii_write(dev, 0x18, 0, addr >> 9);
    mii_write(dev, xphy, xreg, data & 0xFFFF);
    mii_write(dev, xphy, xreg|1, data >> 16);
}
EXPORT_SYMBOL(ar8328_write);        

uint32 ar8328_read(struct net_device *dev, uint32 addr)
{
    uint32 xphy = 0x10 | ((addr >> 6) & 7),
    xreg = (addr >> 1) & 0x1E;

    mii_write(dev, 0x18, 0, addr >> 9);
    return mii_read(dev, xphy, xreg) | mii_read(dev, xphy, xreg|1) << 16;
}
EXPORT_SYMBOL(ar8328_read);        
                       
// initialize ar8328                       
void ar8328_config(struct net_device *dev)
{
    uint32 n;

    // force bridge through reset via gpio 20
    GIO_IODIR_LO &= ~(1<<20); 
    GIO_DATA_LO &= ~(1<<20); 
    mdelay(10);
    GIO_DATA_LO |= (1<<20);  
    mdelay(10);

    n = ar8328_read(dev,0) & 0xffff;   // clear eeprom load bit
    ar8328_write(dev,0,n);             // put it back
    printk("Initializing ar8328 version %lX\n", n);
    mdelay(10);
 
    if (!testmode)
    {     
        // configure a0 silicon phys per the faq
        for (n = 0; n < 5; n++)
        {
            mii_write(dev, n, 0x1d, 0x3d);
            mii_write(dev, n, 0x1e, 0x48a0);
        }    
        
        ar8328_write(dev, 0x0624, 0x007f7f7f);     // allow broadcast, multicast, unknown unicast to all ports
        
        // mac0 goes to the cpu
        ar8328_write(dev, 0x0004, 0x00000004);     // mac mode (external clock)
        ar8328_write(dev, 0x007c, 0x0000007d);     // full duplex, flow control, rx and txmac, 100M (TMII)
    } else if (testmode < 5)  
    {
        printk("Enabling ar8328 1000BaseT test mode %d\n", testmode);
        // per the faq
        ar8328_write(dev, 0x0010, 0x20241320);
        mii_write(dev, AR8328_ETH_PHY, 0x1d, 0x0b);
        mii_write(dev, AR8328_ETH_PHY, 0x1e, 0x05);
        mii_write(dev, AR8328_ETH_PHY, 0x00, 0x8140); 
        mii_write(dev, AR8328_ETH_PHY, 0x09, 0x0200|(testmode<<13));
    } else
    {
        printk("Enabling ar8328 %dBaseT loopback\n",(testmode==5)?10:(testmode==6)?100:1000);
        // per the faq
        mii_write(dev, AR8328_ETH_PHY, 0x1d, 0x0b);
        mii_write(dev, AR8328_ETH_PHY, 0x1e, 0x3c40);
        mii_write(dev, AR8328_ETH_PHY, 0x1d, 0x11);
        mii_write(dev, AR8328_ETH_PHY, 0x1e, 0x7553);
        mii_write(dev, AR8328_ETH_PHY, 0x00, (testmode==5)?0x8100:(testmode==7)?0xa100:0x8140);
    }    
}
