// Copyright 2012,2013 TiVo Inc. All Rights Reserved.

#include <linux/module.h>
#include <linux/sched.h>
#include <linux/fs.h>
#include <linux/proc_fs.h>
#include <linux/jiffies.h>
#include <asm/processor.h>
#include <linux/plog.h>
#include <linux/timer.h>


#define NAME "watchdog"

#define WARNING(fmt,args...) printk(KERN_WARNING NAME ": " fmt, ## args)
#define ERROR(fmt,args...) printk(KERN_ERR NAME ": " fmt, ## args)

#define WDHWMAX 159
#define AON_CTRL_UNCLEARED_SCRATCH *((volatile u32 *)(0x90408068))

extern int in_crashallassert(void);
extern int kernel_crashall(const char *);

extern unsigned char ucIsWarm;

extern struct atomic_notifier_head panic_notifier_list;
static int panic_event(struct notifier_block *, unsigned long val, void *);

static struct notifier_block panic_block = {
    panic_event,
    NULL,
    INT_MAX /* try to do it first */
};

// Watchdog types
#define WD_RESET	(0)
#define WD_NMI		(0x1)
#define WD_BOTH		(0x2)

// poll time in seconds
static int wdpoll=1;

// hardware watchdog timeout in seconds, must not exceed WDHWMAX
#ifdef CONFIG_TIVO_DEVEL
static int wdhwto=9;   
static int wdtype=-1;
#else
static int wdhwto=18;   
static int wdtype=WD_RESET;
#endif

// reboot due to watchdog timeout
static int  wdRebootReason=0;

static int wdnocrashall=0;

#ifdef MODULE
module_param(wdpoll, int, 0);
module_param(wdhwto, int, 0);
module_param(wdnocrashall, int, 0);
module_param(wdtype, int, 0);
#else
static int __init wdpoll_setup(char *s)
{
    wdpoll=simple_strtoul(s,NULL,0);
    return 1;
}    
__setup("wdpoll=", wdpoll_setup);

static int __init wdhwto_setup(char *s)
{
    wdhwto=simple_strtoul(s,NULL,0);
    return 1;
}    
__setup("wdhwto=", wdhwto_setup);

static int __init wdnocrashall_setup(char *s)
{
    wdnocrashall=simple_strtoul(s,NULL,0);
    return 1;
}    
__setup("wdnocrashall=", wdnocrashall_setup);

static int __init wdtype_setup(char *s)
{
    wdtype=simple_strtoul(s,NULL,0);
    return 1;
}    
__setup("wdtype=", wdtype_setup);

#endif

// interesting watchdog registers 
#define WDTIMEOUT              *((volatile u32 *)(0x904067e8))
#define WDCMD                  *((volatile u32 *)(0x904067ec))
#define WDCTRL                 *((volatile u32 *)(0x904067fc))
#define WDCNT		*((volatile u32 *)(0x904067f0))

// SUN_TOP_CTRL_RESET_HISTORY/WDCRS is NOT USED in chips that have a sys_aon always-on power island module.
// They use the reset history feature in sys_aon.
#define AON_CTRL_RESET_CTRL    *((volatile u32 *)(0x90408000))
#define AON_CTRL_RESET_HISTORY *((volatile u32 *)(0x9040806c))

#define AON_CTRL_UNCLEARED_SCRATCH *((volatile u32 *)(0x90408068))

#define H_POR_RESET     (1<<0)
#define H_MAIN_RESET    (1<<1)
#define H_TAP_RESET     (1<<2)
#define H_FP_RESET      (1<<3)
#define H_S3_RESET      (1<<4)
#define H_CARD_RESET    (1<<5)
#define H_WD_RESET      (1<<6)
#define H_PCIE_RESET    (1<<7)
#define H_SW_RESET      (1<<8)
#define H_LOW_V_RESET   (1<<11)
#define H_HI_V_RESET    (1<<12)
#define H_HI_TEMP_RESET (1<<13)
#define H_JTAG_RESET    (1<<14)

static DEFINE_SPINLOCK(lock);
unsigned long expire=0;

static int panic_event(struct notifier_block *this, unsigned long event, void *ptr)
{
    AON_CTRL_UNCLEARED_SCRATCH = 1;
    return NOTIFY_DONE;
}

// set if watchdog is running
static int keep_scratching=1;
static struct timer_list watchdog_timer;

static void reconf_wdog_interval(int timeout, int type)
{
    if ( timeout > 0  && timeout <= WDHWMAX ){
        WDCMD=0xee00; WDCMD=0x00ee;	// ensure stopped
	WDTIMEOUT = timeout*27000000;   // set hardware timeout
	WDCNT = -1;			// reset pulse length
	WDCTRL = type;                  // chip reset mode
        WDCMD=0xff00; WDCMD=0x00ff;     // start watchdog
    }
}

// this is a timeout
static void dodog(unsigned long x)
{

    if (in_crashallassert()) {
	    reconf_wdog_interval(WDHWMAX, WD_RESET);
	    return;
    }

    // always keep hardware happy
    WDCMD=0xff00; WDCMD=0x00ff;

    plog_generic_event0(PLOG_TYPE_HW_WDOG_KEEPALIVE);

    // check if soft timeout has expired
    spin_lock(&lock);
    x = expire;
    spin_unlock(&lock);
    if (x && time_after(jiffies, x) && keep_scratching) {
	    if ( wdnocrashall ) {
		    ERROR("SOFT TIMEOUT OCCURRED\n");
		    keep_scratching = 0;
	    } else {
		    ERROR("SOFT TIMEOUT OCCURRED"
			" - system will reboot\n");
		    reconf_wdog_interval(WDHWMAX, WD_RESET);
		    _plog_enable(0);
            AON_CTRL_UNCLEARED_SCRATCH=3;
		    kernel_crashall("Watchdog timeout");
		    return;
	    }
    }
    // check again soon
    mod_timer(&watchdog_timer, jiffies + wdpoll*HZ);

}    

// read time left until watchdog expires, in seconds
static int proc_read_wd(char *buf, char **start, off_t offset, int count, int *eof, void *data)
{
    int n = 0;

    if (!offset) 
    {
        if (!expire) 
            n = sprintf(buf, "disabled   %d\n",wdRebootReason);
        else 
            n = sprintf(buf,"%ld   %d\n", (signed long)(expire-jiffies)/HZ, wdRebootReason);
    }
    buf[n]=0;
    *eof=1;
    return n;
}

// set watcdog, in seconds (in form "==N")
static int proc_write_wd(struct file *file, const char *buffer, unsigned long count, void *data)
{
    char b[80]; 
    
    if (count >= 3)
    {
        copy_from_user(b, buffer, count >= sizeof(b) ? sizeof(b) - 1 : count);
	b[sizeof(b)-1] = 0;
        
        // force a hard timeout to occur
        if (!strncmp(b,"==grrr",6))
        {
            WARNING("hard timeout forced\n");
	    del_timer_sync(&watchdog_timer);
        }
        else if (b[0]=='=' && b[1]=='=')
        {
            int n = (unsigned int)simple_strtoul(b+2, NULL, 0);
            if (!expire && n) WARNING("enabling soft timeout\n");
            else if (expire && !n) WARNING("disabling soft timeout\n");
            spin_lock_bh(&lock);
            if (!n) expire=0;
            else
            {
                plog_generic_event0(PLOG_TYPE_SW_WDOG_KEEPALIVE);
                expire=jiffies+(n*HZ); // convert to jiffies
                if (!expire) expire=1;
            }    
            spin_unlock_bh(&lock);    
        } 
    }    
    return count;
}

struct proc_dir_entry *proc;

static int __init init(void)
{
    int reset_history;
    extern int dsscon;

    if (!wdpoll) 
    {
        WARNING("disabled due to wdpoll=0\n");
        return 0;
    }    

    if (wdhwto <= wdpoll) 
    {
        ERROR("wdhwto must exceed wdpoll\n");
        return -EINVAL;
    }    

    if (wdhwto > WDHWMAX)
    {
        ERROR("wdhwto must not exceed %d\n",WDHWMAX);
        return -EINVAL;
    }

    if (wdtype == -1) {
	    wdtype = WD_NMI;
	    if (dsscon <= 0) wdtype = WD_RESET;
    }

    if (!(proc=create_proc_entry(NAME,0666,NULL)))
    {
        ERROR("couldn't create /proc/%s\n", NAME);
        return -ENODEV;
    }

     /* Register a call for panic or crash all conditions. */
        atomic_notifier_chain_register(&panic_notifier_list,
                            &panic_block);
   
    proc->read_proc=proc_read_wd;
    proc->write_proc=proc_write_wd;

    // Route NMI to both strands
    write_c0_brcm_mode(read_c0_brcm_mode() | 0x3);

    // configure hardware watchdog
    WARNING("hardware timeout is %d seconds, poll interval is %d seconds\n", wdhwto, wdpoll);
    // XXX If we had an NMI then this bit won't be set.
    reset_history = AON_CTRL_RESET_HISTORY;
    if (reset_history & H_WD_RESET)
    {
        WARNING("NOTICE - previous reboot was triggered by watchdog\n");
        wdRebootReason=1;
    }
    // If any interesting resets occured we probably want to know.
    if (reset_history & (H_LOW_V_RESET|H_HI_V_RESET|H_HI_TEMP_RESET))
    {
        WARNING("NOTICE - Reset history %x\n", reset_history);
    }

    if(0x1 & reset_history)
    {
        printk("Detected cold reboot (power pulled). Other reasons could include booting from an older kernel. Value=0x%x\n", reset_history );
       ucIsWarm = 0;
    }
    else
    {
        if(0==AON_CTRL_UNCLEARED_SCRATCH)
        {
            printk("Detected warm reboot AON value is 0x%x\n", reset_history);
            ucIsWarm = 1;
        }
        else if(3==AON_CTRL_UNCLEARED_SCRATCH)
        {
            printk("Detected warm reboot (software watchdog timeout )  AON value is 0x%x\n", reset_history);
            ucIsWarm = 3;
        }
        else if (reset_history & H_WD_RESET)
        {
            printk("Detected warm reboot (hardware watchdog timeout )  AON value is 0x%x\n", reset_history);
            ucIsWarm = 3;
        }
        else
        {
            printk("Detected warm reboot (Kernel panic) AON value is 0x%x\n", reset_history);
            ucIsWarm = 2;
        }
    }
    //Clear the scratch register
    AON_CTRL_UNCLEARED_SCRATCH =0;
    AON_CTRL_RESET_CTRL |= 0x1;
    reconf_wdog_interval(wdhwto, wdtype);
    setup_timer(&watchdog_timer, dodog, 0);

    // Fire it up by calling it once.  It should reschedule itself now on.
    dodog(0);
    WARNING("Watchdog started\n");
    return 0;
}


module_init(init);

#ifdef MODULE
// rmmod driver
static void exit(void)
{
    del_timer_sync(&watchdog_timer);
    WARNING("Watchdog exiting\n");
    remove_proc_entry(NAME, NULL);
    WDCMD=0xee00; WDCMD=0x00ee;     // stop hardware watchdog
}

module_exit(exit);
#endif

