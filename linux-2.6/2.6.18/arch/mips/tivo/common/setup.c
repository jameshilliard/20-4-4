/*
 * TiVo initialization and setup code
 *
 * Copyright (C) 2012 TiVo Inc. All Rights Reserved.
 */

#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/string.h>

// handle dsscon parameter
#ifdef CONFIG_TIVO_DEVEL
int dsscon=1;
#else
int dsscon=0;
#endif

static int __init do_dsscon(char *s)
{
    dsscon=(*s != 'f' && *s != '0'); // on unless explicitly off
    return 1;
}

__setup("dsscon=", do_dsscon);

#ifdef CONFIG_TIVO_CONSDBG
int nodebug = 0;

static int __init setup_nodebug(char *s)
{
    // If "nodebug" argument is passed to the kernel
    // remote debugging via serial port is disabled
    nodebug = 1;
    return 1;
}

__setup("nodebug", setup_nodebug);

#else
int nodebug = 1;
#endif
