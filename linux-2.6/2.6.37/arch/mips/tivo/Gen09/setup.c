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
char *platform_whitelist[] __initdata = { "root=", "dsscon=", "brev=", "runfactorydiag=", NULL };

extern int bmem_setup(char *);

unsigned int boardID;
EXPORT_SYMBOL(boardID);

int __init tivo_init(void)
{
    char buffer[60];
    char *value;

    boardID=0;
    
    // is brev on command line?
    value=cmdline_lookup(arcs_cmdline,"brev=");
    if (value) boardID=simple_strtoul(value, NULL, 0)<<8;

    // if not, try to get BREV variable from flash  
    if (!boardID && cfe_getenv("BREV", buffer, sizeof(buffer))==CFE_OK) 
        boardID=simple_strtoul(buffer,NULL,0)<<8;

    // in any case, force board ID into range for this platform  
    if (boardID < kTivoConfigBoardIDGen09Base || boardID > kTivoConfigBoardIDGen09Max)
        boardID = kTivoConfigBoardIDGen09Base;

    printk(KERN_NOTICE "BoardID is 0x%X\n", boardID);

    // tell kernel
    InitTivoConfig( (TivoConfigValue) boardID );

    // Add virtual hardware strapping resistors
    if (cfe_getenv("DISABLE_BRIDGE", buffer, sizeof(buffer))!=CFE_OK) 
        boardID |= kTivoConfigGen09Strap_IsBridge;

    // and user space
    if (!cmdline_lookup(arcs_cmdline,"boardID")) 
        cmdline_append(arcs_cmdline, COMMAND_LINE_SIZE, "boardID=0x%X", boardID);

    if (!cmdline_lookup(arcs_cmdline,"HpkImpl")) 
        cmdline_append(arcs_cmdline, COMMAND_LINE_SIZE, "HpkImpl=Gen09");

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
    
#ifdef CONFIG_TIVO_DEVEL
    if (cmdline_lookup(arcs_cmdline, "nfs1"))
    {
        cmdline_delete(arcs_cmdline, "nfs1", NULL);
        cmdline_append(arcs_cmdline, COMMAND_LINE_SIZE, "xnfsinit=192.168.1.250:/Gen09/nfsroot sysgen=true rw");
    }

    if ((value = cmdline_lookup(arcs_cmdline, "nfsinit=")) != NULL || 
        (value = cmdline_lookup(arcs_cmdline, "xnfsinit=")) !=NULL)
    {
        // note, leave the [x]nfsinit command intact so it lands in the environment
        cmdline_delete(arcs_cmdline, "init=", "nfsroot=", NULL);
        cmdline_append(arcs_cmdline, COMMAND_LINE_SIZE, "init=/devbin/nfsinit nfsroot=%s", value);
    }
#endif

    return 0;
}
