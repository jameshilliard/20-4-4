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
#include <asm/tivo/seals.h>
#include <asm/brcmstb/common/cfe_call.h>

#include "../common/cmdline.h"

// Null-terminated list of command line params which are allowed by release builds, see cmdline.c 
char *platform_whitelist[] __initdata = { "root=", "dsscon=", NULL };

extern unsigned int cfe_seal;
extern unsigned int firmargs;
int __init tivo_init(void)
{
    TivoConfigValue brev;
    char *value;

    brev = kTivoConfigBoardIDMojave;

#ifdef CONFIG_TIVO_DEVEL
    value = cmdline_lookup(arcs_cmdline, "brev=");
    if (value) 
        brev = simple_strtoul(value, NULL, 0) << 8;
#endif        

    InitTivoConfig(brev);
    SetTivoConfig(kTivoConfigBoardID, brev);

    if (cfe_seal == CFE_SEAL)
    {
        uart_puts("Booted with CFE\n");
        cmdline_append(arcs_cmdline, CL_SIZE, "PROM_TYPE=CFE");
    } else if (cfe_seal == BSISEAL)
    {
        uart_puts("Booted with BSI\n");
        cmdline_append(arcs_cmdline, CL_SIZE, "PROM_TYPE=BSI mem=32M$16M root=/dev/mtdblock6");
    } else
    {
#if 0
        // Disable use of boot parameters stored in NOR flash to prevent
        // security hole.
        int slen;
        slen = strlen((char *)firmargs) > (CL_SIZE / 2) ? (CL_SIZE / 2) : strlen((char *)firmargs);
        strncpy(arcs_cmdline, (char *)firmargs, slen);
        arcs_cmdline[slen + 1] = 0;
#endif

        cfe_seal = BSLSEAL;
        uart_puts("Booted with BSL\n");
        cmdline_append(arcs_cmdline, CL_SIZE, "PROM_TYPE=BSL root=/dev/mtdblock6");
    }
    // Use /dev/ttyS0 as the default kernel console if none specified
    if (!cmdline_lookup(arcs_cmdline, "console="))
        cmdline_append(arcs_cmdline, CL_SIZE, "console=ttyS0");

    if (!cmdline_lookup(arcs_cmdline, "HpkImpl="))
        cmdline_append(arcs_cmdline, CL_SIZE, "HpkImpl=Gen06D");

#if !defined (CONFIG_TIVO_DEVEL)
    if (!cmdline_lookup(arcs_cmdline, "dsscon="))
        cmdline_append(arcs_cmdline, CL_SIZE, "dsscon=true sdsscon=true");
#endif

#ifdef CONFIG_TIVO_DEVEL
    if (cmdline_lookup(arcs_cmdline, "nfs1"))
    {
        cmdline_delete(arcs_cmdline, "nfs1", NULL);
        cmdline_append(arcs_cmdline, CL_SIZE, "xnfsinit=192.168.1.250:/Gen06D/nfsroot sysgen=true rw");
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
