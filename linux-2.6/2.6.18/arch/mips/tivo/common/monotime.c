/*
 * Common code for monotonic time on MIPS-based TiVo systems
 *
 * Copyright (C) 2002-2005 TiVo Inc. All Rights Reserved.
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/param.h>
#include <linux/proc_fs.h>
#include <linux/interrupt.h>
#include <linux/tivo-monotime.h>

#include <asm/mipsregs.h>
#include <asm/system.h>

static volatile unsigned long tivo_timer_wrap_count = 0;
static volatile unsigned long tivo_last_counter_count = 0;
struct timer_list tivo_monotonic_timer;
static volatile unsigned long tivo_timer_entry = 0;
static volatile unsigned long tivo_timer_exit = 0;

static void tivo_monotonic_timer_init(void);
static inline unsigned long get_timer0_count(void);

void tivo_monotonic_timer_handler(unsigned long arg)
{
        unsigned long flags;
        u32 now;

        local_irq_save(flags);  // keep calls to tivo_monotonic_get_current IRQ-safe by making this atomic

        now = get_timer0_count();

        tivo_timer_entry++;

        if (now < tivo_last_counter_count) {
            tivo_timer_wrap_count++;
        }

        tivo_last_counter_count = now;

        tivo_timer_exit++;

        local_irq_restore(flags);

        tivo_monotonic_timer.expires = jiffies + HZ;
        add_timer(&tivo_monotonic_timer);
}

/*
 * tivo_time_getcount implements a guaranteed monotonic time counter,
 * using the brcm Timer0 and an interrupt-driven wrap counter
 */
void tivo_time_getcount(struct pt_regs *regs)
{
	unsigned int shi, slo, lo;
        unsigned long t1, t2;

	do {
                t1 = tivo_timer_entry;
		shi = tivo_timer_wrap_count;
		slo = tivo_last_counter_count;
                t2 = tivo_timer_exit;
		lo = get_timer0_count();
	} while (t1 != t2);

        /*
         * Account for the possibility that the counter has wrapped since the
         * last interrupt. It is unlikely that if would have wrapped more than
         * once (and that would be very bad in any case). 
         *
         * Brcm Timer provides 30 bit count. So, here we will get 62 bit monotonic 
         * counter. This will be good for 5416 years as Timer0 is operating on 27 MHz.
         */

        if (lo < slo) shi++;
	regs->regs[2] = (shi >> 2);	                // high order bits -> v0
	regs->regs[3] = (((shi & 3) << 30) | lo);	// low order bits  -> v1
}

/*
 * same monotonic count, for kernel functions
 */
void tivo_monotonic_get_current(monotonic_t *t)
{
	unsigned int shi, slo, lo;
        unsigned long t1, t2;

	do {
                t1 = tivo_timer_entry;
		shi = tivo_timer_wrap_count;
		slo = tivo_last_counter_count;
                t2 = tivo_timer_exit;
		lo = get_timer0_count();
	} while (t1 != t2);

        /*
         * Account for the possibility that the counter has wrapped since the
         * last interrupt. It is unlikely that if would have wrapped more than
         * once (and that would be very bad in any case). 
         */

        if (lo < slo) shi++;
	t->count = ((u64)shi << 30) | lo;
}
EXPORT_SYMBOL(tivo_monotonic_get_current);


unsigned long tivo_counts_per_256usec;
EXPORT_SYMBOL(tivo_counts_per_256usec);

/*
 * Here we calculate number of counts per 256 usec in brcm timer0.
 * There's no work to do, since we already know the timer runs at
 * 27MHz.
 */
#define BCM_REG_PHYS 0x10000000
#define BCM_REG_BASE (KSEG1 + BCM_REG_PHYS)

#define BCM7405_TIMER0_CNTL ((volatile unsigned long *)(BCM_REG_BASE + 0x4007C8))
#define BCM7405_TIMER0_STAT ((volatile unsigned long *)(BCM_REG_BASE + 0x4007D8))

static void tivo_time_init(void)
{
        tivo_counts_per_256usec=27<<8;
}


static void tivo_monotonic_timer_init(void)
{
        unsigned long otimercntl;

        /* Use BCM7405_TIMER0 as monotonic timer */

        otimercntl = *BCM7405_TIMER0_CNTL;
        if (otimercntl & 0x80000000) {
                printk("Can't use BCM7405_TIMER0 as monotonic timer, BCM7405_TIMER0_CNTL: %.8lx\n",
                        otimercntl);
        }

        /* Enable timer 0, set max timeout_val to 0x3fffffff */
        *BCM7405_TIMER0_CNTL = 0x80000000 | 0x3FFFFFFF;
}

static inline unsigned long get_timer0_count(void)
{
        unsigned long count;

        /* get current count from BCM7405_TIMER0_CNTL register */
        count = *BCM7405_TIMER0_STAT & 0x3FFFFFFF;

        return count;
}
int tivo_monotonic_initialized = 0;
int tivo_monotonic_init(void)
{
        tivo_monotonic_timer_init();
        tivo_time_init();
        init_timer (&tivo_monotonic_timer);
        tivo_monotonic_timer.function = tivo_monotonic_timer_handler;
        tivo_monotonic_timer.expires = jiffies + HZ;
        add_timer(&tivo_monotonic_timer);

        tivo_monotonic_initialized = 1; 

	return 0;
}

EXPORT_SYMBOL(tivo_monotonic_init);
