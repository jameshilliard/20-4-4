// Copyright 2012,2013 TiVo Inc. All Rights Reserved.

#include <linux/module.h>
#include <linux/sched.h>
#include <linux/fs.h>
#include <linux/proc_fs.h>
#include <linux/jiffies.h>
#include <asm/processor.h>
#include <linux/plog.h>
#include <linux/kdebug.h>
#include <linux/kthread.h>
#include <linux/cpu.h>
#include <linux/cpumask.h>
#include <linux/delay.h>
#include <linux/kernel_stat.h>
#include <linux/tick.h>

#define NAME "watchdog"

#define WARNING(fmt,args...) printk(KERN_WARNING NAME ": " fmt, ## args)
#define ERROR(fmt,args...) printk(KERN_ERR NAME ": " fmt, ## args)

// hardware watchdog min and max seconds
#define WDHWMAX 159
#define WDHWMIN 10

extern int in_crashallassert(void);
extern int kernel_crashall(const char *);

// set to 0 for cold boot, 1 for warm boot, 2 for kernel panic
extern unsigned char ucIsWarm;

// set if reboot due to watchdog timeout, reported via /proc/watchdog
static int wdRebootReason = 0;

// watchdog types
#define RESET           0                                       // watchdog forces hardware reset
#define NMI             1                                       // watchdog forces NMI
#define BOTH            2                                       // watchdog generates an NMI, then much later a reset
#define AUTO            3                                       // select based on dsscon

#ifdef CONFIG_TIVO_DEVEL
// dev kernel is configurable
static unsigned int wdhwto = 30;                                // hardware timeout, seconds
static unsigned int wdswto = 120;                               // software timeout, seconds
static unsigned int wdtype = AUTO;                              // watchdog type, maybe based on dsscon
extern int dsscon;
static int wdpoll = 1;                                          // 0=disable watchdog (does not affect poll time)
static int wdnocrashall = 0;                                    // 0=crash on sw timeout

static int __init wdpoll_setup(char *s)
{
    wdpoll = simple_strtoul(s, NULL, 0);
    return 1;
}

__setup("nwdpoll=", wdpoll_setup);

static int __init wdswto_setup(char *s) {
    wdswto = simple_strtoul(s, NULL, 0);
    return 1;
}

__setup("nwdswto=", wdswto_setup);

static int __init wdhwto_setup(char *s)
{
    wdhwto = simple_strtoul(s, NULL, 0);
    return 1;
}

__setup("nwdhwto=", wdhwto_setup);

static int __init wdnocrashall_setup(char *s)
{
    wdnocrashall = simple_strtoul(s, NULL, 0);
    return 1;
}

__setup("nwdnocrashall=", wdnocrashall_setup);

static int __init wdtype_setup(char *s)
{
    wdtype = simple_strtoul(s, NULL, 0);
    return 1;
}

__setup("nwdtype=", wdtype_setup);

#else
// release kernel is not configurable
#define wdhwto 30
#define wdswto 120
#define wdtype RESET
#endif

// how often to complain about soft watchdog, seconds
#define COMPLAIN 10

// interesting watchdog registers
#define WDTIMEOUT                   *((volatile u32 *)(0x904066e8))
#define WDCMD                       *((volatile u32 *)(0x904066ec))
#define WDCTRL                      *((volatile u32 *)(0x904066fc))
#define WDCNT                       *((volatile u32 *)(0x904066f0))

// reason for reboot
#define AON_CTRL_UNCLEARED_SCRATCH  *((volatile u32 *)(0x90408068)) // this will be set to non-zero by kernel panic
#define AON_CTRL_RESET_CTRL         *((volatile u32 *)(0x90408000))
#define AON_CTRL_RESET_HISTORY      *((volatile u32 *)(0x9040806c))

// interesting codes for scratch register
#define WAS_NMI 0xC0D1F1ED          // reboot caused by nmi
#define WAS_PANIC 0xAC1D1F1C        // reboot caused by panic

// interesting history bits
#define H_POR_RESET     (1<<0)
#define H_WD_RESET      (1<<6)

// handle for software watchdog timer
static struct timer_list watchdog_timer;

// seconds until software watchdog expires
static unsigned int soft_timeout;

// bits per cpu
static uint32_t fleas = 0;

// bits per running flea thread
static uint32_t biting = 0;

// spin lock for the above
static DEFINE_SPINLOCK(lock);

// set if either cpu known to be blocked
static int blocked = 0;

#ifdef CONFIG_TIVO_DEVEL
static uint32_t masked = 0;                                     // for debug, set via echo ~~N > /proc/watchdog
#endif

// set hardware timeout and watchdog type
static void sethwto(int timeout, int type)
{
    WDCMD = 0xee00;
    WDCMD = 0x00ee;                                             // ensure stopped
    if (type == BOTH) timeout *= 2;                             // generate NMI on time
    if (timeout > WDHWMAX) timeout = WDHWMAX;                   // keep in range
    WDTIMEOUT = timeout * 27000000;                             // set hardware timeout
    WDCNT = 0x3ffffff;                                          // for NMIs set this to the max value.
    WDCTRL = type;                                              // chip reset mode (0=reset, 1=nmi)
    WDCMD = 0xff00;
    WDCMD = 0x00ff;                                             // start watchdog
}

// show current watchdog and cpu status
static void status(void)
{
    int i, this=get_cpu();
    WARNING("fleas %s, biting=%X/%X, timeout in %d seconds\n", blocked?"BLOCKED":"ok", biting, fleas, soft_timeout);
    // report each cpu, status char '<' if current, ':' if normal, '?' if blocked, '!' if blocked and yet current(!)
    for (i=0; (1<<i) < fleas; i++) if (fleas && (1<<i))
    {
        WARNING("%d%c irqs=%lu us=%llu sy=%llu id=%llu io=%llu th=%llu bh=%llu\n",
            i,
            (biting & (1<<i))?((i==this)?'<':':'):((i==this)?'!':'?'),
            kstat_cpu(i).irqs_sum,
            // times are in jiffies
            kcpustat_cpu(i).cpustat[CPUTIME_USER],
            kcpustat_cpu(i).cpustat[CPUTIME_SYSTEM],
#ifdef CONFIG_NO_HZ
            usecs_to_cputime64(get_cpu_idle_time_us(i,NULL)),
            usecs_to_cputime64(get_cpu_iowait_time_us(i,NULL)),
#else
            kcpustat_cpu(cpu).cpustat[CPUTIME_IDLE]),
            kcpustat_cpu(cpu).cpustat[CPUTIME_IOWAIT]),
#endif
            kcpustat_cpu(i).cpustat[CPUTIME_IRQ],
            kcpustat_cpu(i).cpustat[CPUTIME_SOFTIRQ]
        );
    }
}

// This is the software watchdog, runs in timer bottom-half. Always tickles the
// hardware watchdog, crashes if flea threads are blocked for longer than
// wdswto seconds.
static void watchdog(unsigned long x)
{
    if (in_crashallassert())
    {
        sethwto(WDHWMAX/2, wdtype);                             // system will reboot later
        return;                                                 // don't reschedule
    }

    // First, tickle hardware watchdog and log it
    WDCMD = 0xff00;
    WDCMD = 0x00ff;
#ifdef CONFIG_TIVO_PLOG
    plog_generic_event0(PLOG_TYPE_HW_WDOG_KEEPALIVE);
#endif
    // if soft timeout is active
    if (soft_timeout >= 0)
    {
        // if all fleas are biting, restart soft_timeout
        spin_lock(&lock);
        if (fleas == biting)
        {
            if (blocked) blocked = -1; // see below
            soft_timeout = wdswto;
            biting = 0;
        }
        spin_unlock(&lock);

        // maybe report unblock
        if (blocked<0)
        {
            blocked=0;
            status();
        }

        // periodically check if blocked and complain
        if (soft_timeout < wdswto && !((wdswto - soft_timeout) % COMPLAIN))
        {
            blocked=1;
            status();
            biting=0;  // all must run again to restore
        }

        // downcount to zero
        if (!soft_timeout)
        {
#ifdef CONFIG_TIVO_DEVEL
            if (wdnocrashall)
            {
                ERROR("SOFT WATCHDOG TIMEOUT OCCURRED - crash is disabled\n");
            } else
#endif
            {
                ERROR("SOFT WATCHDOG TIMEOUT OCCURRED\n");
                sethwto(WDHWMAX/2, wdtype);
#ifdef CONFIG_TIVO_PLOG
                _plog_enable(0);
#endif
                kernel_crashall("soft watchdog");
                return;                                         // don't reschedule
            }
        }

        soft_timeout--;
    }

    // run every second
    mod_timer(&watchdog_timer, jiffies + HZ);
}

// flea process runs for each cpu
static int flea(int cpu)
{
    WARNING("started flea/%d\n", cpu);
    cpu = 1 << cpu;

    while (1)
    {
        msleep(500);                            // every 500 mS
#ifdef CONFIG_TIVO_DEVEL
        if (masked & cpu) continue;             // unless mimicing cpu blockage
#endif
        spin_lock_bh(&lock);
        biting |= cpu;                          // just note cpu is alive
        spin_unlock_bh(&lock);
    }

    // never returns, but make gcc happy
    return 0;
}

// Return interesting watchdog info
static int proc_read_wd(char *buf, char **start, off_t offset, int count, int *eof, void *data)
{
    int n = 0;

    if (!offset)
    {
        // wdRebootReason must be the second token on this line
        n = sprintf(buf, "%d %d %X/%X\n", soft_timeout, wdRebootReason, biting, fleas);
    }
    buf[n] = 0;
    *eof = 1;
    return n;
}

#ifdef CONFIG_TIVO_DEVEL
// Handle debug commands, the old "==X" input is ignored.
static int proc_write_wd(struct file *file, const char *buffer, unsigned long count, void *data)
{
    char b[80];
    if (count)
    {
        copy_from_user(b, buffer, count >= sizeof(b) ? sizeof(b) - 1 : count);
        b[sizeof(b) - 1] = 0;

        if (!strncmp(b, "~~g", 3))                              // 'g' as in grrr
        {
            WARNING("hard timeout forced\n");
            del_timer_sync(&watchdog_timer);
        } else if (!strncmp(b, "~~", 2))
        {
            // bits in mask prevent associated flea bites
            masked = simple_strtoul(b + 2, NULL, 0);
            WARNING("masking %d\n", masked);
        } else
        {
            status();
        }
    }
    return count;
}
#endif

static int __init init(void)
{
    struct proc_dir_entry *proc;
    int cpu;

    // do ucIsWarm first
    WARNING("reset history=0x%X, scratch=0x%X\n", AON_CTRL_RESET_HISTORY, AON_CTRL_UNCLEARED_SCRATCH);

    if (AON_CTRL_RESET_HISTORY & 1)
    {
        WARNING("system was cold booted\n");
        ucIsWarm = 0;
    } else if (AON_CTRL_RESET_HISTORY & 0x40)
    {
        WARNING("NOTICE - previous reboot was triggered by watchdog\n");
        wdRebootReason = 1;
        ucIsWarm = 1; // warm boot by definition
    } else if (AON_CTRL_UNCLEARED_SCRATCH == WAS_NMI)
    {
        WARNING("NOTICE - previous reboot was triggered by watchdog, via NMI\n");
        wdRebootReason = 1;
        ucIsWarm = 1;
    } else if (AON_CTRL_UNCLEARED_SCRATCH == WAS_PANIC)
    {
        WARNING("system was rebooted by kernel panic\n");
        ucIsWarm = 2;
    } else
    {
        WARNING("system was warm booted\n");
        ucIsWarm = 1;
    }

    // reset boot context
    AON_CTRL_UNCLEARED_SCRATCH = 0;
    AON_CTRL_RESET_CTRL |= 1;

    if (!(proc = create_proc_entry(NAME, 0644, NULL)))
    {
        ERROR("couldn't create /proc/%s\n", NAME);
        // try to run anyway
    } else
    {
        // enable watchdog status
        proc->read_proc = proc_read_wd;

#ifdef CONFIG_TIVO_DEVEL
        // enable debug commands
        proc->write_proc = proc_write_wd;
#endif
    }

#ifdef CONFIG_TIVO_DEVEL
    if (!wdpoll)
    {
        WARNING("disabled\n");
        return 0;
    }

    // maybe enable NMI when dsscon=true
    // (otherwise there's no point, since it can't log)
    if (wdtype >= AUTO) wdtype = (dsscon > 0) ? BOTH : RESET;

    // sanity check
    if (wdhwto < WDHWMIN) wdhwto=WDHWMIN;
    if (wdhwto > WDHWMAX) wdhwto=WDHWMAX;
    if (wdtype == BOTH && wdhwto > WDHWMAX/2) wdhwto=WDHWMAX/2; // keep NMI in range
    if (wdswto < 2*wdhwto) wdswto=2*wdhwto;                 // soft watchdog must be at least 2X the hardware

    // always route NMI to both strands
    write_c0_brcm_mode(read_c0_brcm_mode() | 0x3);

#endif

    WARNING("hard timeout=%d, soft timeout=%d, type=%d\n", wdhwto, wdswto, wdtype);

    // start the hardware watchdog
    sethwto(wdhwto, wdtype);

    // start the software watchdog
    soft_timeout = wdswto;
    setup_timer(&watchdog_timer, watchdog, 0);
    watchdog(0);

    // start a flea thread on each CPU at baseline priority
    for_each_possible_cpu(cpu)
    {
        struct task_struct *p = kthread_create_on_node((int (*)(void *))flea, (void *)cpu, cpu_to_node(cpu), "flea/%d", cpu);

        if (!IS_ERR(p))
        {
            kthread_bind(p, cpu);
            wake_up_process(p);
        } else
        {
            ERROR("failed to start flea/%d, sorry for your impending crash...\n", cpu);
        }

        // remember running fleas
        fleas |= 1 << cpu;

    }
    return 0;
}

// remember that reboot was caused by nmi
void note_nmi(void)
{
    AON_CTRL_UNCLEARED_SCRATCH = WAS_NMI;
}

// remember that reboot was caused by panic
void note_panic(void)
{
    if (!AON_CTRL_UNCLEARED_SCRATCH) AON_CTRL_UNCLEARED_SCRATCH = WAS_PANIC;
}

module_init(init);
