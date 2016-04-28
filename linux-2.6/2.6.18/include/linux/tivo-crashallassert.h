/*
 * tivo-crashallassrt.h
 * Copyright (C) 2005,2006 TiVo Inc. All Rights Reserved.
 */

#ifndef __TIVO_CRASH_ALL_ASSERT_H__
#define __TIVO_CRASH_ALL_ASSERT_H__

/* Exported to user section: */

/*
 * Tags
 */ 
#define TAG_CMD         0
#define TAG_TIME        1
#define TAG_EXEC_CTX    2
#define TAG_TMKDBG      3

struct k_arg {
        short tag;
        unsigned long ptr;
        short len;
};
struct k_arg_list {
        struct k_arg *kargs;
        short count;
};

/* Kernel only section : */
#ifdef __KERNEL__

int tivocrashallassert(unsigned long);

#define TICK_KILLER_DISABLED 0
int tick_killer_start(int);
extern int tick_killer_ticks;

#endif

#endif
