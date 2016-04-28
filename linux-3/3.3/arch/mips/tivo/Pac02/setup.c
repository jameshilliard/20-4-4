/*
 * TiVo initialization and setup code
 *
 * Copyright (C) 2012 TiVo Inc. All Rights Reserved.
 */

#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/tivoconfig.h>
#include <linux/string.h>
#include <asm/bootinfo.h>
#include <asm/setup.h>
#include <linux/module.h>
#include <asm/fw/cfe/cfe_api.h>
#include <asm/fw/cfe/cfe_error.h>

#include "../common/cmdline.h"

// Null-terminated list of command line params which are allowed by release builds, see cmdline.c 
char *platform_whitelist[] __initdata = { "root=", "dsscon=", "brev=", "remaster=", "brcmgenet.macaddr_eth=", "brcmgenet.macaddr_moca=", "bcmvenet.macaddr=", "PROM_TYPE=", "mtdparts=", "ubi.mtd=", "boot_device=", "rootdelay=", NULL };

extern int bmem_setup(char *);

unsigned int boardID;

EXPORT_SYMBOL(boardID);

#define PTR_BCHP_GIO_DATA_LO *((volatile uint32_t *)(0x90406504))

int __init tivo_init(void)
{
    char buffer[60];
    char *value;

    boardID=0;

    // is brev on command line?
    value=cmdline_lookup(arcs_cmdline,"brev=");
    if (!value) /* board rev arg not passed in, so make an assumption here */
    {
      strcpy(buffer,"0x90002"); /* X3 is alloted the default board revision of 0x9002 */
      boardID=simple_strtoul(buffer,NULL,0)<<8;
    }
else
    {
       boardID=simple_strtoul(value, NULL, 0)<<8;
    }

    // in any case, force board ID into range for this platform  
    if (boardID < kTivoConfigBoardIDPaceBase || boardID > kTivoConfigBoardIDPaceMax)
    {
        printk("Missing or invalid BoardID, taking a wild guess...\n");
        boardID = kTivoConfigBoardIDX3;                                   
    }   

   
    printk(KERN_NOTICE "BoardID is 0x%X\n", boardID);

    // tell kernel
    InitTivoConfig( (TivoConfigValue) boardID );

    // and user space
    if (!cmdline_lookup(arcs_cmdline,"boardID=")) 
        cmdline_append(arcs_cmdline, COMMAND_LINE_SIZE, "boardID=0x%X", boardID);

    // set default HpkImpl
    if (!cmdline_lookup(arcs_cmdline,"HpkImpl=")) 
        cmdline_append(arcs_cmdline, COMMAND_LINE_SIZE, "HpkImpl=Pac02");

    // set default boot device
    if (!cmdline_lookup(arcs_cmdline,"boot_device="))
    {
        // use sda if firmware reports BOOT_DEVICE=1, else normally use mmca
        if ((cfe_getenv("BOOT_DEVICE", buffer, sizeof(buffer))==CFE_OK) && (simple_strtoul(buffer,NULL,0) != 0))
            cmdline_append(arcs_cmdline, COMMAND_LINE_SIZE,"boot_device=sda");
        else    
            cmdline_append(arcs_cmdline, COMMAND_LINE_SIZE,"boot_device=mmca");
    }        

    // also set default bmem if not on command line
    if (!cmdline_lookup(arcs_cmdline,"bmem="))
    {
        unsigned long size, addr;
        if(!GetTivoConfig(kTivoConfigBmem0Size, &size) && !GetTivoConfig(kTivoConfigBmem0Address,&addr))
        {
            sprintf(buffer,"%lu@%lu",size,addr);
            bmem_setup(buffer);

            if(!GetTivoConfig(kTivoConfigBmem1Size, &size) && !GetTivoConfig(kTivoConfigBmem1Address, &addr))
            {
                sprintf(buffer,"%lu@%lu",size,addr);
                bmem_setup(buffer);
            }
        }
    }

    // set default sata spread spectrum
    if (!cmdline_lookup(arcs_cmdline, "sata3_ssc"))
        cmdline_append(arcs_cmdline, COMMAND_LINE_SIZE, "sata3_ssc");
    
#ifdef CONFIG_TIVO_DEVEL
    if (cmdline_lookup(arcs_cmdline, "nfs1"))
    {
        cmdline_delete(arcs_cmdline, "nfs1", NULL);
        cmdline_append(arcs_cmdline, COMMAND_LINE_SIZE, "xnfsinit=192.168.1.250:/Pac02/nfsroot sysgen=true rw");
    }

    if ((value = cmdline_lookup(arcs_cmdline, "nfsinit=")) != NULL || 
        (value = cmdline_lookup(arcs_cmdline, "xnfsinit=")) !=NULL)
    {
        // note, leave the [x]nfsinit command intact so it lands in the environment
        cmdline_delete(arcs_cmdline, "init=", "nfsroot=", NULL);
        cmdline_append(arcs_cmdline, COMMAND_LINE_SIZE, "init=/devbin/nfsinit nfsroot=%s", value);
    }
#else
    // disable dsscon by default on release
    if (!cmdline_lookup(arcs_cmdline, "dsscon=")) cmdline_append(arcs_cmdline, COMMAND_LINE_SIZE, "dsscon=false");
#endif

    return 0;
}
