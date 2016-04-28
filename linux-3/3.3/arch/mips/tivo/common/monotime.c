/*
 * Common code for monotonic time on MIPS-based TiVo systems
 *
 * Copyright (C) 2002-2012 TiVo Inc. All Rights Reserved.
 */

#include <linux/module.h>
#include <linux/sched.h>
#include <linux/param.h>
#include <linux/ktime.h>
#include <linux/tivo-monotime.h>

#include <asm/mipsregs.h>
#include <asm/system.h>


unsigned long tivo_counts_per_256usec = 256000;
EXPORT_SYMBOL(tivo_counts_per_256usec);

/*
 * tivo_time_getcount implements a guaranteed monotonic time counter,
 * 
 */
void tivo_time_getcount(struct pt_regs *regs)
{
    u64 count;

    count = ktime_to_ns(ktime_get());
#ifdef CONFIG_CPU_BIG_ENDIAN
    regs->regs[2] = (unsigned long)(count >> 32);	     // high order bits -> v0
    regs->regs[3] = (unsigned long)(count & 0xffffffff); // low order bits  -> v1
#else
    regs->regs[2] = (unsigned long)(count & 0xffffffff); // low order bits -> v0
    regs->regs[3] = (unsigned long)(count >> 32);        // high order bits -> v1
#endif
}

/*
 * same monotonic count, for kernel functions
 */
void tivo_monotonic_get_current(monotonic_t *t)
{
    t->count = ktime_to_ns(ktime_get());
}
EXPORT_SYMBOL(tivo_monotonic_get_current);

