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
#include <linux/module.h>

#include "../common/cmdline.h"

// Null-terminated list of command line params which are allowed by release builds, see cmdline.c 
char *platform_whitelist[] __initdata = { "root=", "dsscon=", "brev=", "runfactorydiag=", NULL };

// defined in cfe_call.c
int tivo_get_cfe_env_variable(void *name_ptr, int name_length, void *val_ptr, int val_length);

unsigned int boardID;
EXPORT_SYMBOL(boardID);

int __init tivo_init(void)
{
    char buf[64];
    char *value;
    unsigned int cpu;

    boardID = 0;

    // first check for brev on the command line
    if ((value = cmdline_lookup(arcs_cmdline, "brev=")))
        boardID = simple_strtoul(value, NULL, 0) << 8;

    // if not, try the BREV variable in the flash  
    if (!boardID && !tivo_get_cfe_env_variable("BREV", 4, buf, sizeof(buf)))
        boardID = simple_strtoul(buf, NULL, 0) << 8;

    // otherwise use default
    if (!boardID) boardID = kTivoConfigBoardIDKontiki;

    InitTivoConfig((TivoConfigValue) boardID);
    SetTivoConfig(kTivoConfigBoardID, (TivoConfigValue) boardID);

    // Virtual hardware strapping resistors!
    cpu = *(unsigned int *)(0xb0404000);                        // SUN_TOP_CTRL_PROD_REVISION
    if ((cpu >> 16) != 0x7405) boardID |= 1;                    // flag non-7405s

    // tell user space about the boardID
    cmdline_append(arcs_cmdline, CL_SIZE, "boardID=0x%X", boardID);

    // Use /dev/ttyS0 as the default kernel console if none specified
    if (!cmdline_lookup(arcs_cmdline, "console="))
        cmdline_append(arcs_cmdline, CL_SIZE, "console=ttyS0");

    if (!cmdline_lookup(arcs_cmdline, "HpkImpl="))
        cmdline_append(arcs_cmdline, CL_SIZE, "HpkImpl=Gen07T");

#ifdef CONFIG_TIVO_DEVEL
    if (cmdline_lookup(arcs_cmdline, "nfs1"))
    {
        cmdline_delete(arcs_cmdline, "nfs1", NULL);
        // Gen07T uses Gen07 bootstrap
        cmdline_append(arcs_cmdline, CL_SIZE, "xnfsinit=192.168.1.250:/Gen07/nfsroot sysgen=true rw");
    }

    if ((value = cmdline_lookup(arcs_cmdline, "nfsinit=")) != NULL ||
        (value = cmdline_lookup(arcs_cmdline, "xnfsinit=")) != NULL)
    {
        // note, leave the [x]nfsinit command intact so it lands in the environment
        cmdline_delete(arcs_cmdline, "init=", "nfsroot=", NULL);
        cmdline_append(arcs_cmdline, CL_SIZE, "init=/devbin/nfsinit nfsroot=%s", value);
    }
#endif

    return 0;
}
