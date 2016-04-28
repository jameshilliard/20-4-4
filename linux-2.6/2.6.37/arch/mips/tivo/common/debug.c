/*
 * Generic remote debug code
 *
 * Copyright (C) 2001, 2004, 2013 TiVo Inc. All Rights Reserved.
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
#include <asm/processor.h>
#include <linux/reboot.h>
#include <linux/nsproxy.h>
#ifdef CONFIG_KDB
#include <linux/config.h>
#include <linux/kdb.h>
#endif

#if 0
extern void breakpoint(void);
extern int pollDebugChar(void);
extern unsigned char getDebugChar(void);
extern void show_state(void);
extern void show_mem(void);
extern void show_trace_task(struct task_struct *);
extern struct pt_regs * show_trace_task_regs(struct task_struct *, int);
extern int panic_timeout;

#ifdef CONFIG_KTOP
extern void ktop_start(int, int);
static void ktopdebug_callback(void *ignored);

static struct tq_struct ktopdebug_callback_tq = {
	routine: ktopdebug_callback,
};
#endif

#ifdef CONFIG_TIVO_PLOG
static int plog_max = 0;
static int plog_flags = 0xffffffff;

static int __init debug_plog_setup(char *str)
{
	int opt;
	char *cstr = str;

	if (get_option(&cstr, &opt)) {
		plog_flags = opt;
		if (get_option(&cstr, &opt)) {
			plog_max = opt;
		}
	}

	return 1;
}

__setup("plog=", debug_plog_setup);
#endif

#ifdef CONFIG_MODULES
// This was mostly swiped from get_ksyms_list() in kernel/module.c
//
static LIST_HEAD(modules);
static inline void show_modsyms(int show_all)
{
	struct module *mod;

	list_for_each_entry(mod, &modules, list) {
		unsigned int i;
		struct module_symbol *sym;

		if (mod->state != MODULE_STATE_LIVE )
			continue;

		for (i = mod->num_syms-1, sym = mod->syms; i > 0; --i, ++sym) {
			// If not show_all, only show syms that look like
			// they show the base offset of the module sections
			if (show_all || (strstr(sym->name, "__insmod") &&
			                 strstr(sym->name, "_S."))) {
				printk("%08lx %s", sym->value, sym->name);
				if (mod->name[0]) {
					printk("\t[%s]", mod->name);
				}
				printk("\n");
			}
		}
	}
}
#endif

#include <linux/fastbufs.h>
static void print_task_header(struct task_struct *tsk)
{
    thread_fbuf *exec_cntxt = (thread_fbuf *) tsk->exec_cntxt;
	printk("PID %d (%s):\n", tsk->pid, (exec_cntxt ? 
                            ( exec_cntxt->cntxt? 
                             exec_cntxt->cntxt->name:tsk->comm):
                             tsk->comm));
}

enum dtrace_state {
	DTRACE_NONE,
	DTRACE_BLOCK,
	DTRACE_CANCEL,
	DTRACE_BACKTRACE,
	DTRACE_BACKTRACE_ALL,
	DTRACE_DEBUG,
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
		case DTRACE_DEBUG:
			breakpoint();
			break;
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
			tsk->need_resched = 1;
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

enum swallow_states { SAW_NOTHING, SAW_ESC, SAW_BRACKET };
static enum swallow_states swallow_state = SAW_NOTHING;
static unsigned long lastchar_time;
static int debug_pid;
static int setting_pid;

/* Warn user about potential ills of trying to write out a core file */
static int user_warned=0;
static void warn_user(void) {
	printk("Doing this will probably destroy your /devbin.\n"
	       "If you're sure you know what you're doing, Press 'G' again\n");
	user_warned = 1;
}

#include <linux/paniclog.h>
#include <linux/reboot.h>
#include <linux/ide-deadsystem.h>

static void write_core_panic(void) {
	unsigned int flags;
	int swritten = 0;
	printk("Don't say I didn't warn you....\n");
	printk("Writing core first....\n");
	save_and_cli(flags);
	
	swritten = pio_write_corefile(0);
	printk("Core written to offset 0 of alternate application device\n");
	printk("Sectors written %d\n", swritten);
	do_panic_log_nocli();
	machine_restart("");
	panic("This should never happen!\n");
	BUG();
}

// do_async_dbg will eat all serial input on the debug port, as long as
// it is first in the interrupt chain.  This does make the debug port
// useless for any other input not launched from here, but it should
// coexist with output drivers (at least until the debugger is invoked...)
//
// NOTE: must call this function with interrupts disabled
//
void do_async_dbg(struct pt_regs *regs)
{
	unsigned char c;
	int poll, spin = 0;
	struct task_struct *tsk;
	struct pt_regs *tsk_regs;
	unsigned long time_diff;

	while ((poll = pollDebugChar()) || spin) {
		if (!poll) {
			continue;
		}
		c = getDebugChar();

		// hack to ignore A, B, C, D if part of an arrow key
		// escape sequence
		time_diff = jiffies - lastchar_time;
		lastchar_time = jiffies;
		switch (swallow_state) {
			case SAW_NOTHING:
				if (c == 0x1b) {
					swallow_state = SAW_ESC;
					continue;
				}
				break;
			case SAW_ESC:
				if (time_diff < HZ && c == '[') {
					swallow_state = SAW_BRACKET;
					continue;
				}
				break;
			case SAW_BRACKET:
				if (time_diff < HZ) {
					swallow_state = SAW_NOTHING;
					continue;
				}
				break;
		}
		swallow_state = SAW_NOTHING;

		switch (c) {
#ifdef CONFIG_KDB
			case 0x01:
				kdb(KDB_REASON_KEYBOARD, 0, regs);
				break;
#endif
			case 'H':
				printk("KMON v0.3\n"
				       "Help "
				       "Cpu_usage "
				       "Debug "
				       "Reboot "
				       "regS "
				       "Tasks "
				       "Memory "
				       "Backtrace "
				       "tracEall "
				       "\n"
#ifdef CONFIG_MODULES
				       "Allsyms "
				       "Offsetsyms "
#endif
#ifdef CONFIG_TIVO_PLOG
				       "pLog "
#endif
				       "un/blocK "
				       "un/Wedge "
				       "us/setPid[0-9...] "
				       "sYnc "
				       "paNic "
				       "Gencore "
				       "\n");
				break;
			case 'C':
#ifdef CONFIG_KTOP
				schedule_task(&ktopdebug_callback_tq);
#endif
				break;
			case 'G':
				if(user_warned == 0) {
					warn_user();
				} else {
					write_core_panic();
				}
				break;
			case 'D':
#ifdef CONFIG_KDB
				printk("KDB enabled Kernel; " 
					"Remote Debugging Disable; "
					"'make KDB=n' for remote debugging kernel\n");
#else
				tsk = get_debug_task(debug_pid);
				if (!tsk || tsk == current || dt_state != DTRACE_NONE ||
				    set_dtrace(tsk, DTRACE_DEBUG) != 0) {
					breakpoint();
				} else {
					spin = 0;
				}
#endif /* CONFIG_KDB */
				break;
			case 'R':
#if 1
				printk("Rebooting ... ");
				machine_restart(NULL);
#else
				/* Don't think we still need this case,
				 * keep around until Feb '06, del after if
				 * nobody complained.  snay 10/20/05
				 */

				// don't dump to debugger if we explicitly
				// reboot.  Press 'D' to bring up debugger
				// instead of 'R'
				if (panic_timeout < 0) {
					panic_timeout = 1;
				}
				die("Reboot", regs);
#endif
				break;
			case 'S':
				tsk = get_debug_task(debug_pid);
				if (!tsk) {
					continue;
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
				tsk = get_debug_task(debug_pid);
				// don't bt in the middle of another bt...
				if (!tsk || dt_state == DTRACE_BACKTRACE) {
					continue;
				}
				print_task_header(tsk);
				show_trace_task(tsk);
				if (tsk != current && !spin && tsk->mm &&
				    dt_state == DTRACE_NONE) {
					set_dtrace(tsk, DTRACE_BACKTRACE);
				}
				break;
			case 'E':
				if (!spin && dt_state == DTRACE_NONE) {
					for_each_process(tsk) {
                                                print_task_header();
                                                show_trace_task(p);
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
#ifdef CONFIG_MODULES
			case 'A':
			case 'O':
				show_modsyms(c == 'A');
				break;
#endif
#ifdef CONFIG_TIVO_PLOG
			case 'L':
			{
				int max, flags;

				if (debug_pid) {
					max = -debug_pid;
					flags = 0;
				} else {
					max = plog_max;
					flags = plog_flags;
				}
				plog_dump(max, flags);
				break;
			}
#endif
			case 'K':
				do_block_process(debug_pid);
				break;
			case 'W':
				spin = !spin;
				printk(spin ? "Wedged\n" : "Unwedged\n");
				break;
			case '\n':
			case '\r':
				if (setting_pid == 0) {
					printk("\n");
					break;
				}
				// otherwise fall through...
			case 'P':
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
				emergency_sync_scheduled = EMERG_SYNC;
				wakeup_bdflush();
				break;
			case 'N':
				/* We're in an interrupt, so this is a fun
				   way to make the kernel panic */

				printk("Inducing Kernel Panic (How fun!)\n");
				*((unsigned char*)0)=0;
				printk("BUG: panic returned!\n");
		}
	}
}

void debug(char *s, struct pt_regs *regs) {
	printk("Kernel debug halt requested: %s\n", s);
	breakpoint();
}

#ifdef CONFIG_KTOP
#define TOP_RUN_SECONDS 30

void ktopdebug_callback(void* ignored) {
        ktop_start(TOP_RUN_SECONDS, 1);
}
#endif
#endif

#ifdef CONFIG_TIVO_CONSDBG

extern void emergency_sync(void);
static int setting_pid;
static int debug_pid;
static int spin = 0;
extern void show_state(void);
extern void show_mem(void);
extern void show_trace_task(struct task_struct *);
extern struct pt_regs * show_trace_task_regs(struct task_struct *, int);

#include <linux/fastbufs.h>
static void print_task_header(struct task_struct *tsk)
{
    thread_fbuf *exec_cntxt = (thread_fbuf *) tsk->exec_cntxt;
	printk("CPU %d PID %d (%s):\n", 
           smp_processor_id(),
           tsk->pid, 
           (exec_cntxt ? 
                        ( exec_cntxt->cntxt? 
                         exec_cntxt->cntxt->name:tsk->comm):
                         tsk->comm));
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
		tsk = find_task_by_pid_ns(pid, current->nsproxy->pid_ns);
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

void _do_async_dbg(char c){
	struct task_struct *tsk;
	struct pt_regs *tsk_regs;
        if (dsscon >= 0) 
        switch (c) {
                case 'H':
                        if (!dsscon) break;
                        printk("KMON v0.3\n"
                               "Help "
                               "Reboot "
                               "regS "
                               "Tasks "
                               "Memory "
                               "Backtrace "
                               "tracEall "
                               "\n"
                               "un/blocK "
                               "un/Wedge "
                               "us/setPid[0-9...] "
                               "sYnc "
                               "paNic "
                               "\n");
                        break;
                case 'R':
                        /* 
                        *  This tries to wake up pdflush. The M/C might reset 
                        *  before pdflush wakes up.
                        *  defined in fs/buffer.c
                        */
                        emergency_sync();
                        printk("Rebooting ... ");
                        machine_restart(NULL);
                        break;
                case 'S':
                    {
                        struct pt_regs *regs = task_pt_regs(current);
                        if (!dsscon) break;
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
                    }
                        break;
                case 'T':
                        show_state();
                        break;
                case 'M':
                        show_mem();
                        break;
                case 'B':
                        if (!dsscon) break;
                        tsk = get_debug_task(debug_pid);
                        // don't bt in the middle of another bt...
                        if (!tsk || dt_state == DTRACE_BACKTRACE) {
                               break; 
                        }
                        print_task_header(tsk);
                        show_trace_task(tsk);
                        if (tsk != current && !spin && tsk->mm &&
                            dt_state == DTRACE_NONE) {
                                set_dtrace(tsk, DTRACE_BACKTRACE);
                        }
                        break;
                case 'E':
                        if (!spin && dt_state == DTRACE_NONE) {
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
                        if (!dsscon) break;
                        do_block_process(debug_pid);
                        break;
                case 'W':
                        if (!dsscon) break;
                        spin = !spin;
                        printk(spin ? "Wedged\n" : "Unwedged\n");
                        break;
                case '\n':
                case '\r':
                        if (setting_pid == 0) {
                                printk("\n");
                                break;
                        }
                        // fall thru
                case 'P':
                        if (!dsscon) break;
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
