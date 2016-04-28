/*
 * TiVo initialization and setup code
 *
 * Copyright (C) 2011-2012 TiVo Inc. All Rights Reserved.
 */

#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/string.h>
#include <asm/setup.h>

// dsscon is turned on by default so early kernel output always appears,
// release kernel will turn it off later if not specified otherwise.
int dsscon=1;  

static int __init do_dsscon(char *s)
{
    dsscon=(*s != 'f' && *s != '0'); // on unless explicitly off
    return 1;
}

__setup("dsscon=", do_dsscon);
