/*
 **
 ** Mechanism for testing that threads are getting scheduling in an
 ** given intravel of time.
 ** This code uses broadcom hardware watchdog timer which is available
 ** on brcm 7401 & brcm 7405(mojave & neutron).
 ** 
 ** Copyright TiVo (C) 2009
 **
 **/

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/types.h>
#include <linux/ioport.h>
#include <linux/timer.h>
#include <linux/miscdevice.h>
#include <linux/watchdog.h>
#include <linux/fs.h>
#include <linux/reboot.h>
#include <linux/init.h>
#include <linux/jiffies.h>
#include <linux/interrupt.h>
#include <linux/proc_fs.h>

#include <asm/io.h>
#include <asm/uaccess.h>
#include <asm/system.h>
#include <asm/atomic.h>

#if defined(CONFIG_TIVO_MOJAVE)
#include <asm/brcmstb/brcm97401c0/bcmtimer.h>
#include <asm/brcmstb/brcm97401c0/bchp_hif_cpu_intr1.h>
#elif defined(CONFIG_TIVO_NEUTRON)
#include <asm/brcmstb/brcm97405a0/bcmtimer.h>
#include <asm/brcmstb/brcm97405a0/bchp_hif_cpu_intr1.h>
#endif

#undef CONFIG_PROC_FS

#define BRCM_TIMER_START_ADDR  0x104007c0
#define BRCM_TIMER_END_ADDR    0x104007fc
#define BCM_LINUX_TMR_IRQ ( 1 + BCHP_HIF_CPU_INTR1_INTR_W0_STATUS_UPG_TMR_CPU_INTR_SHIFT )

/* 
 * maximum timout 159 seconds, considering 27 MHz clock
 *  = (159 * 27000000) = 0xFFE1FB40 
 */
#define WATCHDOG_TIMEOUT 0xFFE1FB40
#define MAX_INTR_COUNT   3       /* reboot if thread not scheduled in (159/2)*3 = 238.5 seconds*/

enum Timer_Reg_Offset {
        TIMER_TIMER_IS_REG,
        TIMER_TIMER_IE0_REG,
        TIMER_TIMER0_CTRL_REG,
        TIMER_TIMER1_CTRL_REG,
        TIMER_TIMER2_CTRL_REG,
        TIMER_TIMER3_CTRL_REG,
        TIMER_TIMER0_STAT_REG,
        TIMER_TIMER1_STAT_REG,
        TIMER_TIMER2_STAT_REG,
        TIMER_TIMER3_STAT_REG,
        TIMER_WDTIMEOUT_REG,
        TIMER_WDCMD_REG,
        TIMER_WDCHIPRST_CNT_REG,
        TIMER_WDCRS_REG,
        TIMER_TIMER_IE1_REG,
        TIMER_WDCTRL_REG
};

/* Thread specific data */
#define	WD_NTEST_THREAD		3		// Number of test threads.
/*
 * See the file sw/tmk/Linux/lib/tmk/linux-thread.C to understand these
 * priority numbers.
 */
#define WD_TEST_THREAD0_PRI			9		// For RT_PIPELINE threads
#define	WD_TEST_THREAD0_CHECKTIME		10		// In Seconds

#define WD_TEST_THREAD1_PRI			4		// For RT_VIEWER threads
#define	WD_TEST_THREAD1_CHECKTIME		20		// In Seconds

#define WD_TEST_THREAD2_PRI			0		// For RT_STANDARD threads
#define	WD_TEST_THREAD2_CHECKTIME		30		// In Seconds

typedef struct watchdog_thd_data
{
	short			pri_num;	// Thread priority to check
	atomic_t		intr_cnt;	// intr Count
	unsigned long	        ping_time;	// In seconds

}watchdog_thd_data_t;

static watchdog_thd_data_t watchdog_thread_pri[] = { 
		{
                        .pri_num   =    WD_TEST_THREAD0_PRI, 
                        .intr_cnt  = 	ATOMIC_INIT(0),
                        .ping_time = 	WD_TEST_THREAD0_CHECKTIME,
                }, 
                {
                        .pri_num   =	WD_TEST_THREAD1_PRI, 
                        .intr_cnt  =	ATOMIC_INIT(0),
                        .ping_time =	WD_TEST_THREAD1_CHECKTIME,
                }, 
                {
                        .pri_num   =	WD_TEST_THREAD2_PRI, 
                        .intr_cnt  =	ATOMIC_INIT(0),
                        .ping_time =	WD_TEST_THREAD2_CHECKTIME,
		}
};

static __u32 __iomem *timer_base;
static u32 size = BRCM_TIMER_END_ADDR - BRCM_TIMER_START_ADDR + 1;
static spinlock_t wdt_spinlock;
static int failed = 0;
extern void set_rt_priority(struct task_struct *p, int policy, int prio);

#ifdef CONFIG_PROC_FS
static struct proc_dir_entry *proc_wdt;

/* /proc/watchdog just for debugging active timers */
static int wdt_read_proc (char *page, char **start, off_t off, int count,
                int *eof, void *data_unused)
{
        int len,i;
        off_t   begin = 0;
        len = 0;

        spin_lock(&wdt_spinlock);

        for(i = 0; i < 16; i++)
        {
                len += sprintf(page + len, "REG %2d =  0x%08x\n", 
                                i, readl(timer_base + i));
        }

        if (len+begin > off+count)
                goto done;
        if (len+begin < off) {
                begin += len;
                len = 0;
        }

        *eof = 1;

done:
        spin_lock(&wdt_spinlock);
        if (off >= len+begin)
                return 0;
        *start = page + (off-begin);
        return ((count < begin+len-off) ? count : begin+len-off);
}
#endif /* CONFIG_PROC_FS */

/*
 *	wdt_startup:
 *
 *	Start the watchdog driver.
 */
static int wdt_startup(void)
{
        unsigned long flags;
        volatile u32 temp;

        spin_lock_irqsave(&wdt_spinlock, flags);

        /* maximum timout 159 seconds, considering 27 MHz clock */        
        writel(WATCHDOG_TIMEOUT , timer_base + TIMER_WDTIMEOUT_REG);

        /* configure watchdog in interrupt mode */
        temp = readl(timer_base + TIMER_WDCTRL_REG);        
        writel(temp | 1 , timer_base + TIMER_WDCTRL_REG);   

        /* Enable WDT interrupt */
        temp = readl(timer_base + TIMER_TIMER_IE0_REG);        
        writel(BCHP_TIMER_TIMER_IS_WDINT_MASK | temp, timer_base + TIMER_TIMER_IE0_REG);

        /* stop watchdog for a while */ 
        writel(0xEE00, timer_base + TIMER_WDCMD_REG);
        writel(0x00EE, timer_base + TIMER_WDCMD_REG);

        /* Reload watchdog counter from timout register, start watchdog*/
        writel(0xFF00, timer_base + TIMER_WDCMD_REG);
        writel(0x00FF, timer_base + TIMER_WDCMD_REG);

        spin_unlock_irqrestore(&wdt_spinlock, flags);

        printk(KERN_INFO "Watchdog timer is now enabled.\n");
        return 0;
}

/*
 *	wdt_turnoff:
 *
 *	Stop the watchdog counter.
 */

static int wdt_turnoff(void)
{
        unsigned long flags;
        volatile u32 temp;

        spin_lock_irqsave(&wdt_spinlock, flags);

        writel(0xEE00, timer_base + TIMER_WDCMD_REG);
        writel(0x00EE, timer_base + TIMER_WDCMD_REG);

        temp = readl(timer_base + TIMER_TIMER_IE0_REG);
        /* Mask WDT interrupt */
        writel(temp & (~BCHP_TIMER_TIMER_IS_WDINT_MASK), timer_base + TIMER_TIMER_IE0_REG);

        spin_unlock_irqrestore(&wdt_spinlock, flags);

        printk(KERN_INFO "Watchdog timer is now disabled.\n");
        return 0;
}

/*
 *	wdt_interrupt:
 *	@irq:		Interrupt number
 *	@dev_id:	Unused as we don't allow multiple devices.
 *	@regs:		Unused.
 *
 *	Handle an interrupt from the watchdog. These are raised when the 
 *	watchdog counter reaches half of timeout value(currently 79.5 sec).
 */

static irqreturn_t wdt_interrupt(int irq, void *dev_id, struct pt_regs *regs)
{
        volatile u32 temp;
        struct task_struct *tsk;
        int index;

        /* 
         * watchdog interrupt bit in TIMER_TIMER_IS_REG
         * should be set once the watchdog counter reaches half of
         * the timeout value. Then we should restart or reload watchdog 
         * counter.
         * So, Mask watchdog interrupt for a while to prevent spurious interrupts.
         */

        if(!(readl(timer_base + TIMER_TIMER_IS_REG) & BCHP_TIMER_TIMER_IS_WDINT_MASK))
        {
                return IRQ_NONE;   /* Interrupt belongs to brcm Timers[0-3] not us*/
        }

        /* Mask WDT interrupt */
        temp = readl(timer_base + TIMER_TIMER_IE0_REG);
        writel(temp & (~BCHP_TIMER_TIMER_IS_WDINT_MASK), timer_base + TIMER_TIMER_IE0_REG);

        /* stop WDT for a moment */
        writel(0xEE00, timer_base + TIMER_WDCMD_REG);
        writel(0x00EE, timer_base + TIMER_WDCMD_REG);

        /* check that all threads are running */
        for (index = 0; index < WD_NTEST_THREAD; ++index)
        {
                if(atomic_read(&watchdog_thread_pri[index].intr_cnt) >= MAX_INTR_COUNT){
                        failed = 1;
                        break;
                }
        }

        /* thread not getting scheduled , reboot */
        if(failed){
                local_irq_disable();

                for_each_process(tsk) {
                        printk("pid %5d, ra = 0x%08lx, state = %ld, %s\n",
                                        tsk->pid, thread_saved_pc(tsk), tsk->state, tsk->comm);
                }

                printk(KERN_CRIT "##### Watchdog Fired: Initiating system reboot. #####\n");
                machine_restart(NULL);
        }
       
        /* threads are running ... OK  */ 
        for (index = 0; index < WD_NTEST_THREAD; ++index)
        {
                atomic_inc(&watchdog_thread_pri[index].intr_cnt);
        }

        /* Enable WDT interrupt */
        temp = readl(timer_base + TIMER_TIMER_IE0_REG);        
        writel(BCHP_TIMER_TIMER_IS_WDINT_MASK | temp, timer_base + TIMER_TIMER_IE0_REG);

        /* Reload watchdog counter from timout register, start watchdog*/
        writel(0xFF00, timer_base + TIMER_WDCMD_REG);
        writel(0x00FF, timer_base + TIMER_WDCMD_REG);

        return IRQ_HANDLED;
}

/*
 *	wdt_open:
 *	@inode: inode of device
 *	@file: file handle to device
 */

static int wdt_open(struct inode * inode, struct file * file)
{
        return 0;
}

/*
 *	wdt_close:
 *	@inode: inode to board
 *	@file: file handle to board
 */
static int wdt_close(struct inode * inode, struct file * file)
{
        return 0;
}


static const struct file_operations wdt_fops = {
        .owner		= THIS_MODULE,
        .open		= wdt_open,
        .release	= wdt_close,
};

static struct miscdevice wdt_miscdev = {
        .minor	= WATCHDOG_MINOR,
        .name	= "watchdog",
        .fops	= &wdt_fops,
};

/* this is called when watchdog threads get scheduled */
static int watchdog_thread_entry(void *index_p){
	int	index = *(int *)index_p;

        set_rt_priority(current, SCHED_FIFO, watchdog_thread_pri[index].pri_num);

        while(1){
                atomic_set(&watchdog_thread_pri[index].intr_cnt, 0);

                set_current_state(TASK_UNINTERRUPTIBLE);
                schedule_timeout(watchdog_thread_pri[index].ping_time * HZ);
        }
        return 0;
}

/* create WD_NTEST_THREAD threads for monitoring system state */
static void watchdog_threads_start(void)
{
	static int thd_param[WD_NTEST_THREAD] = {0, 1, 2};
        int index;

        for (index = 0; index < WD_NTEST_THREAD; ++index)
        {
                thd_param[index] = index;
                kernel_thread( watchdog_thread_entry, &thd_param[index],
                                CLONE_FS | CLONE_FILES );
        }
}

static int __init brcm_wdt_init(void)
{
        int rc = -EBUSY;

        spin_lock_init(&wdt_spinlock);

        if(!request_mem_region(BRCM_TIMER_START_ADDR , size, "watchdog"))
        {
                printk(KERN_INFO "%s: Failed to get memory region for broadcom watchdog\n", __FUNCTION__);
                goto err_out_region2;
        }

        timer_base = ioremap((unsigned long)BRCM_TIMER_START_ADDR, size);
        if (!timer_base) {
                printk(KERN_ERR "%s: Unable to remap memory\n", __FUNCTION__);
                rc = -ENOMEM;
                goto err_out_region2;
        }

        rc = misc_register(&wdt_miscdev);
        if (rc) {
                printk(KERN_ERR "%s: cannot register miscdev on minor=%d (err=%d)\n",
                                __FUNCTION__, WATCHDOG_MINOR, rc);
                goto err_out_ioremap;
        }

        /*
         * Broadcom Timer[0-3] share the same IRQ line with watchdog timer.
         * So, installing a shared ISR. 
         */
       
        rc = request_irq(BCM_LINUX_TMR_IRQ, wdt_interrupt, IRQF_DISABLED | IRQF_SHARED, "watchdog", &wdt_miscdev); 
        if(rc)
        {
                printk(KERN_ERR "%s: IRQ %d is not free, err = %d\n", __FUNCTION__, BCM_LINUX_TMR_IRQ, rc);
                goto err_out_ioremap;
        }

#ifdef CONFIG_PROC_FS
        if ((proc_wdt = create_proc_entry( "watchdog", 0, NULL )))
                proc_wdt->read_proc = wdt_read_proc;
#endif   

        watchdog_threads_start();

        wdt_startup();

err_out_ioremap:
        iounmap(timer_base);
err_out_region2:
        return rc;
}

static void __exit brcm_wdt_exit(void)
{
        wdt_turnoff();

        /* Deregister */
        misc_deregister(&wdt_miscdev);
        release_mem_region(BRCM_TIMER_START_ADDR, size);
        iounmap(timer_base);
        free_irq(BCM_LINUX_TMR_IRQ, &wdt_miscdev);

#ifdef CONFIG_PROC_FS
        if (proc_wdt)
                remove_proc_entry( "watchdog", NULL);
#endif        

}
module_init(brcm_wdt_init);
module_exit(brcm_wdt_exit);

MODULE_DESCRIPTION("Driver for watchdog timer in broadcom 74xx Processor");
MODULE_ALIAS_MISCDEV(WATCHDOG_MINOR);
