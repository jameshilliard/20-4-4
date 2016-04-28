/*
 * TiVo initialization and setup code
 *
 * Copyright (C) 2011 TiVo Inc. All Rights Reserved.
 */

#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/string.h>
#include <asm/setup.h>

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

// Used to verify that setup params are disallowed, ie we panic then they aren't.
static int __init check_disallow(char *s) { (void)s; panic("check_disallow"); }
__setup("check_disallow", check_disallow);

