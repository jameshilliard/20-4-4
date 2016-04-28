
/*
 * linux/arch/mips/tivo/tivo-swwatchdog.c
 *
 * Mechanism for testing user level threads are getting scheduling in an
 * given intravel of time.
 *
 * Copyright TiVo (C) 2006
 * Written by Krishna Yeddanapudi (kyeddanapudi@tivo.com)
 *
 */

#define __KERNEL_SYSCALLS__

#include <linux/config.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/init.h>
#include <linux/unistd.h>
#include <linux/signal.h>
#include <linux/completion.h>


#define	SW_WATCHDOG_CHECKTIME	30	// In seconds

#define	SWWD_NTEST_THREAD		3		// Number of test threads.

/*
 * See the file sw/tmk/Linux/lib/tmk/linux-thread.C to understand these
 * priority numbers.
 */
#define SWWD_TEST_THREAD0_PRI			9		// For RT_PIPELINE threads
#define	SWWD_TEST_THREAD0_CHECKTIME		60		// In Seconds
#define SWWD_TEST_THREAD0_FAILCNT		4		// Maximum number of failures 
											// before Crashing

#define SWWD_TEST_THREAD1_PRI			4		// For RT_VIEWER threads
#define	SWWD_TEST_THREAD1_CHECKTIME		120		// In Seconds
#define SWWD_TEST_THREAD1_FAILCNT		4		// Maximum number of failures 
											// before Crashing

#define SWWD_TEST_THREAD2_PRI			0		// For RT_STANDARD threads
#define	SWWD_TEST_THREAD2_CHECKTIME		240		// In Seconds
#define SWWD_TEST_THREAD2_FAILCNT		4		// Maximum number of failures 
											// before Crashing

/***************************************************************************
 * IMPORTANT: Nothing below this lines need to be changed normally.
 */

// Assumed atomic operations, even they are not atomic not much harm will
// be done. Hate to use locks here.
#define	SWWD_ASSUMED_ATOMIC(_x)		_x



static int	sw_watchdog_threads_initialized = 0;

typedef struct sw_watchdog_thd_data
{
	short			pri_num;	// Thread priority to check
	short			fail_cnt;	// Failure Count
	unsigned long	cum_fail_cnt;	// Cumulative failure count
	unsigned long	chk_time;	// In seconds

}
sw_watchdog_thd_data_t;


static unsigned long	jiffes_per_checktime = (SW_WATCHDOG_CHECKTIME * HZ);

static sw_watchdog_thd_data_t	sw_watchdog_thread_pri[] = { 
		{
				SWWD_TEST_THREAD0_PRI, 
				SWWD_TEST_THREAD0_FAILCNT, 
				0,
				SWWD_TEST_THREAD0_CHECKTIME
		}, 
		{
				SWWD_TEST_THREAD1_PRI, 
				SWWD_TEST_THREAD1_FAILCNT, 
				0,
				SWWD_TEST_THREAD1_CHECKTIME
		}, 
		{
				SWWD_TEST_THREAD2_PRI, 
				SWWD_TEST_THREAD2_FAILCNT, 
				0,
				SWWD_TEST_THREAD2_CHECKTIME
		}
};

static int	nsw_watchdog_thd = SWWD_NTEST_THREAD;

static short sw_watchdog_thread_counter[SWWD_NTEST_THREAD] = {0}; 
		// Incremented in the thread and reset in the timer ISR.

static short sw_watchdog_timer_counter[SWWD_NTEST_THREAD] = {0}; 
		// Incremented in the Timer ISR and reset in timer ISR.

static short sw_watchdog_thread_fail_count[SWWD_NTEST_THREAD] = {0};
		// Number of times the thread failed to respond.
		// Reset inside the thread and incremented in the timer ISR.


/*
 * This routine gets called at an intravel of 'nj' jiffies from the timer ISR.
 * This value is also passed as an input paramer.
 */
static void
sw_watchdog_thread_checktime(unsigned long nj)
{
	int	i;
	unsigned long	nsec = nj / HZ;		// jiffies value in seconds.

	for (i = 0; i < nsw_watchdog_thd; ++i)
	{
		sw_watchdog_timer_counter[i] += nsec;

		if (sw_watchdog_thread_pri[i].chk_time <= 
				sw_watchdog_timer_counter[i])
		{
			unsigned long flags;

			sw_watchdog_timer_counter[i] = 0; // reset the timer cnt

			if (sw_watchdog_thread_counter[i] > 0)
			{
				SWWD_ASSUMED_ATOMIC(sw_watchdog_thread_counter[i] = 0);
				continue;	// Thread is okay.
			}

			if (sw_watchdog_thread_fail_count[i]
					< sw_watchdog_thread_pri[i].fail_cnt)
			{
				SWWD_ASSUMED_ATOMIC(sw_watchdog_thread_counter[i] = 0);
				SWWD_ASSUMED_ATOMIC(sw_watchdog_thread_fail_count[i]++);
				continue;	// Thread is not okay, but we can wait for sometime.
			}

			// The thread failed completely.
			// Crassh assert and reboot the system.
			save_and_cli(flags);	
			printk("Error: Software Watchdog Failed!\n");
			printk("\tPriority of the Failed Thread = %d\n",
								sw_watchdog_thread_pri[i].pri_num);	
			printk("\tNumber of Failures = %d\n",
							sw_watchdog_thread_fail_count[i]);	
			printk("\tCumulative Failures = %ld\n",
						 (sw_watchdog_thread_pri[i].cum_fail_cnt 
						  + sw_watchdog_thread_fail_count[i]));
			printk("\tChecking Time Intravel = %ld Seconds\n",
							 sw_watchdog_thread_pri[i].chk_time);
			printk("Other Information: jiffies = %ld\n", jiffies);
			do_panic_log_nocli();
			machine_restart("");
			panic("Software Watchdog Failed!\n");
			BUG();
		}
	}
}


/*
 * This routine getts called from the do_timer routine.
 */
void
sw_watchdog_timer_isr(void)
{
		static	unsigned long	local_jiffies = 0;

		if (sw_watchdog_threads_initialized == 0)
		{
				return;
		}

		if (((jiffies - local_jiffies) >= jiffes_per_checktime) &&
			((jiffies - local_jiffies) < (2*jiffes_per_checktime)))
		{
			local_jiffies = jiffies;

			sw_watchdog_thread_checktime(jiffes_per_checktime);

			if ((local_jiffies + jiffes_per_checktime) < jiffies)
			{
					// It means next time when we do the check 
					// jiffies will wrap around.
					// Hence change the local_jiffies
				local_jiffies =	jiffes_per_checktime - (~0UL - jiffies);
			}
		}
}

static int
sw_watchdog_thread_entry(void *index_p)
{
	int	index = *(int *)index_p;

	current->policy = SCHED_FIFO;
	//current->nice = 0;
	current->rt_priority = sw_watchdog_thread_pri[index].pri_num;

	while (1)
	{
		if (sw_watchdog_thread_fail_count[index] > 0)
		{
			sw_watchdog_thread_pri[index].cum_fail_cnt 
					+= (long) sw_watchdog_thread_fail_count[index];
			// Spit out a warning as this thread is getting
			// delayed while scheduling.

				// Reset the fail counter
				SWWD_ASSUMED_ATOMIC(sw_watchdog_thread_fail_count[index] = 0);
		}
		SWWD_ASSUMED_ATOMIC(sw_watchdog_thread_counter[index]++);

	    set_current_state(TASK_UNINTERRUPTIBLE);
		// Nothingelse to do just sleep now.
		schedule_timeout(((long) 
					sw_watchdog_thread_pri[index].chk_time / 2) * HZ);
	}
}

void sw_watchdog_threads_hold(void){
	sw_watchdog_threads_initialized = 0;
}

void sw_watchdog_threads_release(void){
	sw_watchdog_threads_initialized = 1;
}

// Called at the time of initialization
void
sw_watchdog_threads_start(void)
{
	static int thd_param[SWWD_NTEST_THREAD] = {0, 1, 2};
	int	i;

	for (i = 0; i <  nsw_watchdog_thd; ++i)
	{
			thd_param[i] = i;
			kernel_thread( sw_watchdog_thread_entry, &thd_param[i],
							CLONE_FS | CLONE_FILES );
	}
	sw_watchdog_threads_initialized = 1;
	printk("Software Watchdog Initialized\n");
}


static int __init sw_watchdog_threads_init(void) 
{
	sw_watchdog_threads_start();
	return 0;
}

module_init(sw_watchdog_threads_init);
