/*
 * Watchdog support for Broadcom bcm7401 device.
 *
 * Derived from the support for the BCM71xxx device, which
 * was derived from a previous BCM7038 watchdog.
 *
 * Adapted from watchdog driver for Intel IXP2000 network processors
 * And James Chapman's  bcm7038 watchdog driver
 *
 * This file is licensed under  the terms of the GNU General Public
 * License version 2. This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#include <linux/autoconf.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/watchdog.h>
#include <linux/init.h>
#include <linux/bitops.h>
#include <linux/interrupt.h>
#include <asm/hardirq.h>
// #include <unistd.h>

#include <asm/io.h>
#include <asm/uaccess.h>
#include <asm/brcmstb/brcmstb.h>

#include <linux/hwmsclock.h>


/* state machine for the watchdog system */
typedef enum
{
   WDT_STATE_UNINITIALISED=0, /* initial state */
   WDT_STATE_DISARMED,        /* inited, but not armed */
   WDT_STATE_ARMED,           /* armed, needs keepalive to prevent reset */
   WDT_STATE_TRIGGERED,       /* watchdog interrupt has fired to warn us */
   WDT_STATE_LAST             /* last known */
} wdt_state_enum;

typedef int wdt_state;

static wdt_state wdt_currentState = WDT_STATE_UNINITIALISED;

static struct timer_list wdt_delayed_timer;

/* #define CONFIG_WATCHDOG_BCM_DEBUG */

#ifdef  CONFIG_WATCHDOG_BCM_DEBUG
#define DPRINTK_WARN(ARGS)       printk ARGS
#define DPRINTK_TRACE(ARGS)      printk ARGS
#define DPRINTK_URGENT(ARGS)     printk ARGS

char * wdt_state_name_table[] =
{
   "UNINITIALISED", /* initial state */
   "DISARMED",        /* inited, but not armed */
   "ARMED",           /* armed, needs keepalive to prevent reset */
   "TRIGGERED",       /* watchdog interrupt has fired to warn us */
};

static char *    wdt_state_name( wdt_state state );

#else

#define DPRINTK_WARN(ARGS)   {while(0) {};}
#define DPRINTK_TRACE(ARGS)  {while(0) {};}
#define DPRINTK_URGENT(ARGS) {while(0) {};}
#endif

#ifdef CONFIG_WATCHDOG_NOWAYOUT
static int nowayout = 1;
#else
static int nowayout = 0;
#endif

/* time to reset . Note 7038 watchdog ISR fires at hearbeat/2 seconds */
static unsigned int heartbeat                   = 30; /* (secs) */

static unsigned long wdt_status;

#define WDT_IN_USE              0
#define WDT_OK_TO_CLOSE         1

static DEFINE_SPINLOCK(wdt_spinlock);

#define wdt_lock(FLAGS)   spin_lock_irqsave(&wdt_spinlock,FLAGS);
#define wdt_unlock(FLAGS) spin_unlock_irqrestore(&wdt_spinlock,FLAGS);

#define WDT_FIRST_INTERESTING_PID 100

#define WDT_TIME_TO_DROP_CORE_AND_SHUTDOWN   20 /* seconds */
#define WDT_TIME_FOR_INIT_TO_SHUTDOWN         5 /* seconds */

/* state machine methods. These drive everything in this driver  */

static int       wdt_setState( wdt_state state );

static wdt_state wdt_getState( void );

/* keepalive method */

static int       wdt_keepalive( void  );

/* external handler registered with ISR in bcm_msclock driver */

extern int wdt_watchdog_interrupt_handler( void * private_data );

/* methods for handling the countdown to reset */

static void wdt_delayed_timer_arm( u32 interval );


static void wdt_delayed_timer_handler(unsigned long ptr);


static void wdt_action_kill_application( void );

static void wdt_action_force_software_reset( void ) ;

static void wdt_action_delay_force_software_reset( void ) ;

/* an external declaration */

extern void ctrl_alt_del(void);

/***************************************************************************
*   Watchdog state machine functions
***************************************************************************/

static void wdt_delayed_timer_arm( u32 interval )
{
   struct timer_list * timer = &wdt_delayed_timer;
   init_timer(timer);
   timer->function = wdt_delayed_timer_handler;
   timer->data     = (unsigned long)0;
   timer->expires  = jiffies + HZ  * interval;
   add_timer(timer);
}


static void wdt_delayed_timer_handler(unsigned long ptr)
{
   wdt_action_force_software_reset() ;
}

/* This method called when watchdog interrupt fires at heartbeat secs */

extern int wdt_watchdog_interrupt_handler( void * private_data )
{
#ifdef KERNEL_CHANGES_FOR_ABL
    /* For the Advanced Bootloader only, we want a hard reset after 2 hours. */

    /*Increment a persistent variable everytime the watchdog interrupt occurs. 
    This is set up to once per minute. If 2 hours (120 minutes) have elapsed, 
    force a machine reboot.  */
    static u32 watchdogCounter = 0; 
    watchdogCounter++; 

    if (watchdogCounter >= 240)
    {
    	machine_restart(0); 
    }
    else
    {
    	wdt_setState(WDT_STATE_ARMED);
    }
    return 0; 

    /*Reset the machine. This works, but it seems an exception handler is still enabled. 
    We get a lot of debug including CPU status dump, etc. I hunted down the 
    machine_restart() call which is much cleaner. */
    //	__asm__ __volatile__ ("j 0");
#endif


    if (wdt_getState() == WDT_STATE_ARMED)
    {
       wdt_setState(WDT_STATE_TRIGGERED);
    }
    else
    {
       DPRINTK_URGENT((KERN_CRIT "Watchdog : interrupt handler unexpectedly fired\n"));
    }
    return 0;
}

#ifdef CONFIG_WATCHDOG_BCM_DEBUG
static char * wdt_state_name( wdt_state state)
{
  if ((state >= 0) && (state < WDT_STATE_LAST))
  {
    return wdt_state_name_table[state];
  }
  else
  {
    return "ILLEGAL/CORRUPT";
  }
}
#endif

/* do appropriate action for this state */
static int  wdt_state_entry_action(wdt_state newState)
{
   int rc = 0;
   DPRINTK_WARN((KERN_CRIT "Watchdog : WARNING : wdog entering state %s\n",
                wdt_state_name(newState)));

   switch(newState)
   {
      case WDT_STATE_DISARMED:
      {
         bcm_msclock_hw_watchdog_disarm();
         break;
      }
      case WDT_STATE_UNINITIALISED:
      {
         bcm_msclock_hw_watchdog_disarm();
         break;
      }
      case WDT_STATE_ARMED:
      {
         /* set it up to interrupt at heartbeat seconds */
         bcm_msclock_hw_watchdog_arm(2*heartbeat*1000,
                                     wdt_watchdog_interrupt_handler);

         break;
      }
      case WDT_STATE_TRIGGERED:
      {
         if (1)
         {
            u32 secs_to_reset = ( WDT_TIME_FOR_INIT_TO_SHUTDOWN + 
                                  WDT_TIME_TO_DROP_CORE_AND_SHUTDOWN ) ;

            DPRINTK_WARN((KERN_CRIT "Watchdog : WARNING : SYNCING %s\n",
                wdt_state_name(newState)));

            bcm_msclock_hw_watchdog_arm(secs_to_reset*1000, NULL ) ;

            wdt_action_delay_force_software_reset() ;
            wdt_action_kill_application();
         }
         else
         {
            /* this method doesn't work because init runs at normal piority,
               and the problem is usually a thread running at higher priority
            */
            u32 secs_to_reset = WDT_TIME_FOR_INIT_TO_SHUTDOWN ;

            bcm_msclock_hw_watchdog_arm(secs_to_reset*1000, NULL ) ;

            wdt_action_force_software_reset() ;
         }
            
         break;
      }
      default:
         break;
   }
   return rc;
}

static void  wdt_state_exit_action(wdt_state oldState)
{
   DPRINTK_WARN((KERN_CRIT "Watchdog : WARNING : wdog leaving state %s\n",
                wdt_state_name(oldState)));
   /* LEAVING STATE X */
   switch(oldState)
   {
      case WDT_STATE_DISARMED:
      {
         break;
      }
      case WDT_STATE_UNINITIALISED:
      {
         /* on driver init, turn it all off explicitly */
         bcm_msclock_hw_watchdog_disarm();
         break;
      }
      case WDT_STATE_ARMED:
      {
         break;
      }
      case WDT_STATE_TRIGGERED:
      {
         /* Once entered triggered, never exits */
         break;
      }
      default:
         break;
   }
}

static int wdt_setState( wdt_state state )
{
   int rc;
   unsigned long flags = 0;

   if (!in_interrupt())
   {
     wdt_lock(flags);
   }
   /* leave old state */
   wdt_state_exit_action(wdt_getState());

   /* and enter new */
   wdt_currentState = state;
   rc = wdt_state_entry_action(state);


   if (!in_interrupt())
   {
      wdt_unlock(flags);
   }
   return rc;
}


/* determine current flags */
static  wdt_state wdt_getState( )
{
  return wdt_currentState;
}

/* keepalive call. This cancels the watchdog reset for another N seconds
   but leaves the watchdog armed
*/
static int wdt_keepalive( void )
{
    bcm_msclock_hw_watchdog_keepalive();
    return 0;
}

/***************************************************************************
*   ACTION FUNCTIONS (REBOOT, SYNC, APP KILL ETC )
***************************************************************************/


/*
 * Simple selection loop. We chose the process with the highest
 * number of 'points'. We expect the caller will lock the tasklist.
  */

static int wdt_process_badness(struct task_struct *p,unsigned long uptime)
{
   int points;

   int cpu_time, run_time;
   /*
      * CPU time is in tens of seconds and run time is in thousands
      * of seconds. There is no particular reason for this other than
      * that it turned out to work very well in practice.
   */
   
   cpu_time = (cputime_to_jiffies(p->utime) + cputime_to_jiffies(p->stime));

   /* compute wall clock time */
   if (uptime >= p->start_time.tv_sec)
	run_time = (uptime - p->start_time.tv_sec) ;
   else
	run_time = 0;
    
   /* at the moment , we pick the process with biggest cpu time */
   points = cpu_time;

   DPRINTK_TRACE((KERN_DEBUG "wdt task %d (%s) got %d points, cpu_time = %u,run_time %u \n",
                  p->pid, p->comm, points,cpu_time,run_time));
   return points;
}

static struct task_struct * wdt_select_stuck_process(void)
{
        /* This code DERIVED FROM 2.6 kernel oom_kill.c 
           
   */
	unsigned long maxpoints = 0;
	struct task_struct *g, *p;
	struct task_struct *chosen = NULL;
	struct timespec uptime;
	do_posix_clock_monotonic_gettime(&uptime);

	do_each_thread(g, p)
		/* skip low down tasks (init, biods, etc  */
		if (p->pid > WDT_FIRST_INTERESTING_PID) 
                {
                    int points = wdt_process_badness(p,uptime.tv_sec);
                    if (points > maxpoints) 
                    {
            chosen = p;
            maxpoints = points;
         }
      }
	while_each_thread(g, p);
   return chosen;
}


static void wdt_action_kill_application()
{
   struct task_struct * stuck_process = 0;

   DPRINTK_URGENT((KERN_CRIT "Watchdog : Attempting to kill app\n"));

   read_lock(&tasklist_lock);

   stuck_process = wdt_select_stuck_process();
   /* first non daemon process */
   if (stuck_process && (stuck_process->pid > WDT_FIRST_INTERESTING_PID))
   {
       DPRINTK_URGENT((KERN_CRIT  "Watchdog : killing pid %u [ %s]  \n",
                      stuck_process->pid,
                      stuck_process->comm));
       force_sig(SIGBUS,stuck_process);
   }

   read_unlock(&tasklist_lock);

   DPRINTK_URGENT((KERN_CRIT  "Watchdog : app kill done  \n"));
}

/***************************************************************************/

static void wdt_action_force_software_reset( void )
{
    DPRINTK_URGENT((KERN_CRIT "Watchdog : hardware reset imminent. "
                    "Attempting software reboot\n"));

    ctrl_alt_del() ;

    DPRINTK_URGENT((KERN_CRIT "Watchdog : hardware reset imminent . Goodbye\n"));
}

static void wdt_action_delay_force_software_reset( void )
{
   wdt_delayed_timer_arm(WDT_TIME_TO_DROP_CORE_AND_SHUTDOWN);
}

/***************************************************************************
*   Linux device driver interface functions
***************************************************************************/

static int
wdt_open(struct inode *inode, struct file *file)
{
   switch(MINOR(inode->i_rdev)) 
   {
   case WATCHDOG_MINOR:
      if (test_and_set_bit(WDT_IN_USE, &wdt_status))
         return -EBUSY;
      
      clear_bit(WDT_OK_TO_CLOSE, &wdt_status);
      
      wdt_setState(WDT_STATE_ARMED);
      break;
      
   default:
      return -ENODEV;
   }
   
   return 0;
}

static ssize_t
wdt_write(struct file *file, const char *data, size_t len, loff_t *ppos)
{
   if (len) 
   {
      if (!nowayout) {
         size_t i;
         
         clear_bit(WDT_OK_TO_CLOSE, &wdt_status);
         
         for (i = 0; i != len; i++) 
         {
            char c;
            
            if (get_user(c, data + i))
               return -EFAULT;
            if (c == 'V')
               set_bit(WDT_OK_TO_CLOSE, &wdt_status);
         }
      }
      wdt_keepalive();
   }
   
   return len;
}


static struct watchdog_info ident = {
	.options	= WDIOF_MAGICCLOSE | WDIOF_SETTIMEOUT |
				WDIOF_KEEPALIVEPING,
	.identity	= "BCM71XX Watchdog",
};

static int
wdt_ioctl(struct inode *inode, struct file *file, unsigned int cmd,
	  unsigned long arg)
{
	int ret = -ENOIOCTLCMD;
	int time;

	switch (cmd) {
	case WDIOC_GETSUPPORT:
		ret = copy_to_user((struct watchdog_info *)arg, &ident,
				   sizeof(ident)) ? -EFAULT : 0;
		break;

	case WDIOC_GETSTATUS:
		ret = put_user(0, (int *)arg);
		break;

	case WDIOC_GETBOOTSTATUS:
		ret = put_user(0, (int *)arg);
		break;

	case WDIOC_SETTIMEOUT:
		ret = get_user(time, (int *)arg);
		if (ret)
			break;

		if (time <= 0 || time > 120) {
			ret = -EINVAL;
			break;
		}
		heartbeat = time;
		wdt_keepalive();
		/* Fall through */

	case WDIOC_GETTIMEOUT:
		ret = put_user(heartbeat, (int *)arg);
		break;

	case WDIOC_KEEPALIVE:
		wdt_keepalive();
		ret = 0;
		break;
	}

	return ret;
}

static int
wdt_release(struct inode *inode, struct file *file)
{
	switch(MINOR(inode->i_rdev)) {
	case WATCHDOG_MINOR:
		if (test_bit(WDT_OK_TO_CLOSE, &wdt_status))
		{
	           wdt_setState( WDT_STATE_DISARMED );
		}
		else
		{
			printk(KERN_CRIT "WATCHDOG: Device closed unexpectedly - "
			       "timer will not stop\n");
		}

		clear_bit(WDT_IN_USE, &wdt_status);
		clear_bit(WDT_OK_TO_CLOSE, &wdt_status);
		break;

	default:
		return -ENODEV;
	}

	return 0;
}


static struct file_operations wdt_fops =
{
	.owner		= THIS_MODULE,
	.llseek		= no_llseek,
	.write		= wdt_write,
	.ioctl		= wdt_ioctl,
	.open		= wdt_open,
	.release	= wdt_release,
};

static struct miscdevice wdt_miscdev =
{
	.minor		= WATCHDOG_MINOR,
	.name		= "watchdog",
	.fops		= &wdt_fops,
};

static int __init wdt_init(void)
{
	wdt_setState( WDT_STATE_DISARMED );
	return misc_register(&wdt_miscdev);
}

static void __exit wdt_exit(void)
{
	wdt_setState( WDT_STATE_UNINITIALISED );
	misc_deregister(&wdt_miscdev);
}


#ifndef MODULE

static int __init wdt_setup(char *str)
{
	int ints[4];

	str = get_options (str, ARRAY_SIZE(ints), ints);

	if (ints[0] > 0) {
		heartbeat = ints[1];
		if(ints[0] > 1)
			nowayout = ints[2];
	}

	return 1;
}

__setup("wdt=", wdt_setup);

#endif /* !MODULE */

module_init(wdt_init);
module_exit(wdt_exit);

MODULE_AUTHOR("Dave Morris"); /* using the work of others */
MODULE_DESCRIPTION("BCM7401 Watchdog");

module_param(heartbeat, int, 0);
MODULE_PARM_DESC(heartbeat, "Watchdog heartbeat in seconds (default 30s)");

module_param(nowayout, int, 0);
MODULE_PARM_DESC(nowayout, "Watchdog cannot be stopped once started");

MODULE_LICENSE("GPL");

/* EXPORT_NO_SYMBOLS;   -- ONLY WORKS ON OLD pre 2.6 KERNELS */

/* allow watchdog ISR to signal TRIGGERED state */

EXPORT_SYMBOL(wdt_watchdog_interrupt_handler);
