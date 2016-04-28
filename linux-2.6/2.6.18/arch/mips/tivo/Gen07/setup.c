/*
 * TiVo initialization and setup code
 *
 * Copyright 2012 TiVo Inc. All Rights Reserved.
 */
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/tivoconfig.h>
#include <linux/string.h>
#include <asm/bootinfo.h>
#include <linux/module.h>

#include "../common/cmdline.h"

// Null-terminated list of command line params which are allowed by release builds, see cmdline.c 
char * platform_whitelist[] __initdata = { "root=", "dsscon=", "brev=", "runfactorydiag=", NULL };

#define SUN_TOP_CTRL_PROD_REVISION  *(volatile unsigned int *)(0xb0404000)
#define SUN_TOP_OTP_OPTION_STATUS   *(volatile unsigned int *)(0xb040402c)
#define GIO_DATA_EXT                *(volatile unsigned int *)(0xb0400744)
#define BCHP_BMIPS4380_L2_CONFIG    *((volatile unsigned int *)0xb1f0000c)

// defined in cfe_call.c
int tivo_get_cfe_env_variable(void *name_ptr, int name_length, void *val_ptr, int val_length);

unsigned int boardID;
EXPORT_SYMBOL(boardID);

int __init tivo_init(void)
{
    char buf[64];
    char *value;

    boardID = 0;

    // first check for brev on the command line
    if ((value = cmdline_lookup(arcs_cmdline, "brev=")))
        boardID = simple_strtoul(value, NULL, 0) << 8;

    // if not, try the BREV variable in the flash  
    if (!boardID && !tivo_get_cfe_env_variable("BREV", 4, buf, sizeof(buf)))
        boardID = simple_strtoul(buf, NULL, 0) << 8;

    // Keep board ID in range.
    if (boardID < kTivoConfigBoardIDGen07Base || boardID > kTivoConfigBoardIDGen07Max) 
    {
        printk("boardID = 0x%x --> 0x%x\n", boardID, kTivoConfigBoardIDNeutron);
        boardID = kTivoConfigBoardIDNeutron;
    }

    InitTivoConfig((TivoConfigValue) boardID);

    // Add virtual hardware strapping resistors
    if ((boardID >= kTivoConfigBoardIDLegoBase && boardID <= kTivoConfigBoardIDLegoMax) ||
        (boardID >= kTivoConfigBoardIDHeliumBase && boardID <= kTivoConfigBoardIDHeliumMax) || 
        (boardID >= kTivoConfigBoardIDLeoBase && boardID <= kTivoConfigBoardIDLeoMax))
        boardID |= kTivoConfigGen07Strap_HasMoCA;               // if platform has a MoCA modem

    if (boardID >= kTivoConfigBoardIDLegoBase && boardID <= kTivoConfigBoardIDLegoMax)
        boardID |= kTivoConfigGen07Strap_HasISSD;               // platform has iSSD drive

    if ((SUN_TOP_CTRL_PROD_REVISION >> 16) != 0x7405)           // if not a 7405 processor    
        boardID |= kTivoConfigGen07Strap_Not7405;

    if (SUN_TOP_OTP_OPTION_STATUS & (1 << 14))                  // if SATA hardware is not present (7414)
        boardID |= kTivoConfigGen07Strap_NoSATA;

    if (GIO_DATA_EXT & 0x80000000)                              // GPIO indicates if board has AR8328 switch on EMAC1 (Helium) 
        boardID |= kTivoConfigGen07Strap_IsBridge;

    // tell kernel
    SetTivoConfig(kTivoConfigBoardID, (TivoConfigValue) boardID);

    // and user space 
    cmdline_append(arcs_cmdline, CL_SIZE, "boardID=0x%X", boardID);

    // Use /dev/ttyS0 as the default kernel console if none specified
    if (!cmdline_lookup(arcs_cmdline, "console="))
        cmdline_append(arcs_cmdline, CL_SIZE, "console=ttyS0");

    if (!cmdline_lookup(arcs_cmdline, "HpkImpl="))
        cmdline_append(arcs_cmdline, CL_SIZE, "HpkImpl=Gen07");

    if (boardID & kTivoConfigGen07Strap_Not7405)
    {
        printk("******DISABLING CBG (Copyback Gathering) in the L2 control register*******\n");
        BCHP_BMIPS4380_L2_CONFIG &= 0x70ffffff;
        printk("******verify CBG register value=%08x *******\n", BCHP_BMIPS4380_L2_CONFIG);
    }

#ifdef CONFIG_TIVO_DEVEL
    if (cmdline_lookup(arcs_cmdline, "nfs1"))
    {
        cmdline_delete(arcs_cmdline, "nfs1", NULL);
        cmdline_append(arcs_cmdline, CL_SIZE, "xnfsinit=192.168.1.250:/Gen07/nfsroot sysgen=true rw");
    }

    if ((value = cmdline_lookup(arcs_cmdline, "nfsinit=")) != NULL || 
        (value = cmdline_lookup(arcs_cmdline, "xnfsinit=")) !=NULL)
    {
        // note, leave the [x]nfsinit command intact so it lands in the environment
        cmdline_delete(arcs_cmdline, "init=", "nfsroot=", NULL);
        cmdline_append(arcs_cmdline, CL_SIZE, "init=/devbin/nfsinit nfsroot=%s", value);
    }
#endif

    return 0;
}
