/*
 * Generic remote debug code
 *
 * Copyright (C) 2001, 2004 TiVo Inc. All Rights Reserved.
 */

#include <linux/ptrace.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/wait.h>
#include <linux/init.h>
#include <linux/plog.h>
#include <linux/sysrq.h>
#include <linux/smp_lock.h>
#include <linux/timer.h>
#include <linux/proc_fs.h>
#include <asm/uaccess.h>
#include <linux/reboot.h>
#ifdef CONFIG_KDB
#include <linux/config.h>
#include <linux/kdb.h>
#endif

#include <linux/netdevice.h>
#include <linux/inetdevice.h>

#ifdef CONFIG_TIVO_CONSDBG

extern void emergency_sync(void);
static int setting_pid;
static int debug_pid;
extern void show_state(void);
extern void show_mem(void);
extern void show_trace_task(struct task_struct *);
extern struct pt_regs * show_trace_task_regs(struct task_struct *, int);

static void print_task_header(struct task_struct *tsk)
{
	printk("PID %d (%s):\n", tsk->pid, tsk->comm);
}

enum dtrace_state {
	DTRACE_NONE,
	DTRACE_BLOCK,
	DTRACE_CANCEL,
	DTRACE_BACKTRACE,
	DTRACE_BACKTRACE_ALL,
};

static enum dtrace_state dt_state = DTRACE_NONE;
static struct task_struct *blocked_thread;

static int set_dtrace(struct task_struct *tsk, enum dtrace_state state)
{
	if (sigismember(&tsk->blocked, SIGSTOP)) {
		return 1;
	}
	dt_state = state;
	tsk->ptrace |= PT_DTRACE;
	send_sig(SIGSTOP, tsk, 1);

	return 0;
}

//
// NOTE: this bit runs at process context, with ints enabled
// so be careful to maintain coherency with do_async_dbg()
//
void do_debug_dtrace(void)
{
	current->ptrace &= ~PT_DTRACE;

	switch (dt_state) {
		case DTRACE_BLOCK:
			current->state = TASK_STOPPED;
			barrier();
			dt_state = DTRACE_NONE;
			schedule();
			return;
		case DTRACE_BACKTRACE:
			show_trace_task(current);
			break;
		case DTRACE_BACKTRACE_ALL:
		{
			struct task_struct *tsk = current;

			show_trace_task(tsk);

			// Note that we walk the task list without any
			// locks between user processes, so it's possible
			// that tasks will get added or removed while we're
			// in the middle.  That's pretty much the way it is...
			while (tsk != &init_task) {
				local_irq_disable();
				//tsk = tsk->next_task;
                                tsk = next_task(tsk);
				print_task_header(tsk);
				show_trace_task(tsk);
				if (tsk->mm && tsk->pid != 1 &&
				   set_dtrace(tsk, DTRACE_BACKTRACE_ALL) == 0) {
					local_irq_enable();
					return;
				}
				local_irq_enable();
			}
			break;
		}
		case DTRACE_CANCEL:
		default:
			break;
	}
	barrier();
	dt_state = DTRACE_NONE;
}

static struct task_struct * get_debug_task(int pid)
{
	struct task_struct *tsk;

	if (pid == 0) {
		tsk = current;
	} else {
		tsk = find_task_by_pid(pid);
		if (!tsk) {
			printk(KERN_WARNING "No process with PID %d\n", pid);
		}
	}
	return tsk;
}

static void do_block_process(int pid)
{
	struct task_struct *tsk;

	// If we're still busy trying to do something to a (any)
	// thread, don't confuse things by trying to do more...
	if (dt_state != DTRACE_NONE && dt_state != DTRACE_BLOCK) {
		return;
	}

	if (blocked_thread) {
		if (dt_state == DTRACE_BLOCK) {
			// If we're waiting on a pending block, there's
			// not much we can do now, since we've already
			// sent the signal.  Just mark the pending request
			// such that we ignore it.
			dt_state = DTRACE_CANCEL;
		} else {
			wake_up_process(blocked_thread);
			dt_state = DTRACE_NONE;
		}
		printk("PID %d unblocked\n", blocked_thread->pid);
		blocked_thread = NULL;
	} else {
		tsk = get_debug_task(pid);
		if (!tsk) {
			return;
		}
		if (!tsk->pid) {
			printk(KERN_WARNING "Not blocking idle...\n");
			return;
		}
		if (tsk == current) {
			tsk->state = TASK_STOPPED;
                        set_tsk_need_resched(tsk);
		} else {
			if (set_dtrace(tsk, DTRACE_BLOCK) != 0) {
				printk("Can't block PID %d\n", tsk->pid);
				return;
			}
		}
		blocked_thread = tsk;
		printk("PID %d blocked\n", tsk->pid);
	}
}

extern int dsscon;

// Note we won't get here if dsscon < 0. If dsscon is zero, printk doesn't
// write to console, so block any purely informational stuff.  However do allow
// various status dumps to work, they will still be logged.
void _do_async_dbg(struct pt_regs *regs, char c){
	struct task_struct *tsk;
	struct pt_regs *tsk_regs;
        switch (c) {
                case 'H':
                        if (dsscon < 1) break;
                        printk("KMON v0.4\n"
                               "Help "
                               "Reboot "
                               "regS "
                               "Tasks "
                               "Memory "
                               "Backtrace "
                               "tracEall "
                               "\n"
                               "un/blocK "
                               "us/setPid[0-9...] "
                               "sYnc "
                               "paNic "
                               "Ipadrs "
                               "\n");
                        break;
                case 'R':
                        emergency_sync();
                        printk("Rebooting ... ");
                        machine_restart(NULL);
                        break;
                case 'S':
                        if (dsscon < 1) break;
                        tsk = get_debug_task(debug_pid);
                        if (!tsk) {
                               break; 
                        }
                        print_task_header(tsk);
                        tsk_regs = show_trace_task_regs(tsk, 1);
                        if (tsk == current) {
                                tsk_regs = regs;
                        }
                        if (tsk_regs) {
                                show_regs(regs);
                        }
                        break;
                case 'T':
                        show_state();
                        break;
                case 'M':
                        show_mem();
                        break;
                case 'B':
                        if (dsscon < 1) break;
                        tsk = get_debug_task(debug_pid);
                        // don't bt in the middle of another bt...
                        if (!tsk || dt_state == DTRACE_BACKTRACE) {
                               break; 
                        }
                        print_task_header(tsk);
                        show_trace_task(tsk);
                        if (tsk != current && tsk->mm &&
                            dt_state == DTRACE_NONE) {
                                set_dtrace(tsk, DTRACE_BACKTRACE);
                        }
                        break;
                case 'E':
                        if (dt_state == DTRACE_NONE) {
                                for_each_process(tsk) {
                                        print_task_header(tsk);
                                        show_trace_task(tsk);
                                        if (tsk->mm && tsk->pid == 1 &&
                                            set_dtrace(tsk, DTRACE_BACKTRACE_ALL) == 0) {
                                                break;
                                        }
                                }
                        } else if (dt_state == DTRACE_BACKTRACE_ALL) {
                                dt_state = DTRACE_CANCEL;
                                printk("Trace cancelled.\n");
                        } else {
                                printk("Trace busy.\n");
                        }
                        break;
                case 'K':
                        if (dsscon < 1) break;
                        do_block_process(debug_pid);
                        break;
                case '\n':
                case '\r':
                        if (setting_pid == 0) {
                                printk("\n");
                                break;
                        }
                        // fall thru
                case 'P':
                        if (dsscon < 1) break;
                        if (setting_pid) {
                                if (debug_pid == 0) {
                                        printk("unset\n");
                                } else {
                                        printk(" set\n");
                                }
                                setting_pid = 0;
                        } else {
                                debug_pid = 0;
                                printk("PID: ");
                                setting_pid = 1;
                        }
                        break;
                case '0' ... '9':
                        if (setting_pid) {
                                printk("%c", c);
                                debug_pid = debug_pid * 10 + c - '0';
                        }
                        break;
                case 'Y':
                        printk("Emergency Sync\n");
                        emergency_sync();
                        break;
                case 'N':
                        /* We're in an interrupt, so this is a fun
                           way to make the kernel panic */

                        printk("Inducing Kernel Panic (How fun!)\n");
                        *((unsigned char*)0)=0;
                        printk("BUG: panic returned!\n");
                        break;
                case 'I':
                {
                    if (dsscon < 1) break;
                    // show network interfaces
                    struct net_device *dev;
                    struct in_ifaddr *ifa;
                    read_lock(&dev_base_lock);
                    for (dev = dev_base; dev; dev = dev->next)
                    {
                        if (dev->flags & IFF_LOOPBACK || !netif_running(dev) || !dev->ip_ptr) continue;
                        for(ifa = ((struct in_device *)(dev->ip_ptr))->ifa_list; ifa; ifa = ifa->ifa_next)
                        {
                            uint32_t ip = ntohl(ifa->ifa_address);
                            printk("%s => %d.%d.%d.%d\n", ifa->ifa_label, 
                                      (ip >> 24) & 0xff, (ip >> 16) & 0xff, (ip >> 8) & 0xff, ip & 0xff);
                        }    
                    }
                    read_unlock(&dev_base_lock);
                }
        }
}

void do_async_dbg(struct pt_regs *regs)
{
        printk("Console debugging is not enabled\n");
}

#else
void _do_async_dbg(struct pt_regs *regs)
{
        printk("Console debugging is not enabled\n");
}
void do_async_dbg(struct pt_regs *regs)
{
        _do_async_dbg(struct pt_regs *regs);
}
#endif
