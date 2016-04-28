// Copyright 2011 TiVo Inc. All Rights Reserved.

#include <linux/module.h>
#include <linux/fs.h>
#include <linux/proc_fs.h>
#include <linux/jiffies.h>
#include <asm/processor.h>
#include <linux/plog.h>
#include <linux/timer.h>


#define NAME "watchdog"

#define WARN(fmt,args...) printk(KERN_WARNING NAME ": " fmt, ## args)
#define ERROR(fmt,args...) printk(KERN_ERR NAME ": " fmt, ## args)

#define WDHWMAX 159

extern int in_crashallassert(void);
extern int kernel_crashall(const char *);

// poll time in seconds
static int wdpoll=3;

// hardware watchdog timeout in seconds, must not exceed WDHWMAX
static int wdhwto=18;   

// reboot due to watchdog timeout
static int  wdRebootReason=0;

static int wdnocrashall=0;

#ifdef MODULE
module_param(wdpoll, int, 0);
module_param(wdhwto, int, 0);
module_param(wdnocrashall, int, 0);
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
#endif

// interesting watchdog registers 
#define WDTIMEOUT       *((volatile u32 *)(0xb04007e8))
#define WDCMD           *((volatile u32 *)(0xb04007ec))
#define WDCRS           *((volatile u32 *)(0xb04007f4))
#define WDCTRL          *((volatile u32 *)(0xb04007fc))

static DEFINE_SPINLOCK(lock);
unsigned long expire=0;

// set if watchdog is running
static int keep_scratching=1;
static struct timer_list watchdog_timer;

static void reconf_wdog_interval(int timeout)
{
    if ( timeout > 0  && timeout <= WDHWMAX ){
        WDCMD=0xee00; WDCMD=0x00ee;     // ensure stopped
        WDTIMEOUT = timeout*27000000;    // set hardware timeout
        WDCTRL = 0;                     // chip reset mode
        WDCMD=0xff00; WDCMD=0x00ff;     // start watchdog
    }
}

// this is a timeout
static void dodog(unsigned long x)
{

    if (in_crashallassert()) {
	    reconf_wdog_interval(WDHWMAX);
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
		    reconf_wdog_interval(WDHWMAX);
		    _plog_enable(0);
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
            WARN("hard timeout forced\n");
	    del_timer_sync(&watchdog_timer);
        }
        else if (b[0]=='=' && b[1]=='=')
        {
            int n = (unsigned int)simple_strtoul(b+2, NULL, 0);
            if (!expire && n) WARN("enabling soft timeout\n");
            else if (expire && !n) WARN("disabling soft timeout\n");
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

    if (!wdpoll) 
    {
        WARN("disabled due to wdpoll=0\n");
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

    if (!(proc=create_proc_entry(NAME,0666,NULL)))
    {
        ERROR("couldn't create /proc/%s\n", NAME);
        return -ENODEV;
    }
    
    proc->owner=THIS_MODULE;
    proc->read_proc=proc_read_wd;
    proc->write_proc=proc_write_wd;

    // configure hardware watchdog
    WARN("hardware timeout is %d seconds, poll interval is %d seconds\n", wdhwto, wdpoll);
    if (WDCRS)
    {
        WARN("NOTICE - previous reboot was triggered by watchdog\n");
        wdRebootReason=1;
    }
    WDCRS=1;
    reconf_wdog_interval(wdhwto);
    setup_timer(&watchdog_timer, dodog, 0);

    // Fire it up by calling it once.  It should reschedule itself now on.
    dodog(0);
    WARN("Watchdog started\n");

    return 0;
}


module_init(init);

#ifdef MODULE
// rmmod driver
static void exit(void)
{
    del_timer_sync(&watchdog_timer);
    WARN("Watchdog exiting\n");
    remove_proc_entry(NAME, NULL);
    WDCMD=0xee00; WDCMD=0x00ee;     // stop hardware watchdog
}

module_exit(exit);
#endif

