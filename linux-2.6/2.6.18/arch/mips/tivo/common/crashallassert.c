/*
 * crashallassrt: Walks all the tasks' stacks, both kernel and user,
 * inorder to capture the back traces
 *
 * Copyright 2013 TiVo Inc. All Rights Reserved.
 */

#include <linux/string.h>
#include <asm/uaccess.h>
#include <asm/param.h>
#include <asm/user.h>
#include <linux/sched.h>
#include <linux/threads.h>
#include <linux/kthread.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/tivo-crashallassert.h>
#include <linux/reboot.h>
#include <asm/uaccess.h>
#include <linux/syscalls.h>
#include <asm-mips/delay.h>
#include <linux/plog.h>

#if defined(CONFIG_TIVO) && defined(CONFIG_BLK_DEV_SD)
#include <linux/tivo-reset.h>
#endif

#ifdef CONFIG_TIVO_NEUTRON
#include <linux/tivoconfig.h>
#endif

extern void emergency_sync(void);
extern void show_trace_task_bt(struct task_struct *tsk, int dispconsole);
extern void do_emergency_sync(void);
extern void _machine_restart(char *command);

extern struct list_head modules;

extern int stop_runnables(void);
void restore_runnables(void);
static ssize_t pid_read_maps (struct task_struct *, char *);
void dump_module_addrs(void);

static int debugconsole = 1;
static int in_tivocrashallassert = 0;
static struct task_struct *crashall_thread;

#if defined(CONFIG_TIVO_KONTIKI)
 #define DEBUG_LEVEL KERN_ERR
#elif defined(CONFIG_TIVO_NEUTRON)
#define DEBUG_LEVEL KERN_ERR
#elif defined(CONFIG_TIVO_MOJAVE)
#define DEBUG_LEVEL 
#endif

static ssize_t pid_read_maps (struct task_struct *task, char *tmp)
{
	struct mm_struct *mm;
	struct vm_area_struct * map;
	long retval;

	task_lock(task);
	mm = task->mm;
	if (mm)
		atomic_inc(&mm->mm_users);
	task_unlock(task);

	retval = 0;
	if (!mm)
		return retval;

	down_read(&mm->mmap_sem);
	map = mm->mmap;

	while (map) {
		char *line;

                if ( (map->vm_file == NULL) || 
                        !((map->vm_flags & VM_READ) && 
                        (map->vm_flags & VM_EXEC))) { 

                        map = map->vm_next;
                        continue;
                }
                line = d_path(map->vm_file->f_dentry,
                              map->vm_file->f_vfsmnt,
                              tmp, PAGE_SIZE);
                printk(DEBUG_LEVEL "read 0x%08lx 0x%08lx %s\n",
                       map->vm_start,map->vm_end,line);
                map = map->vm_next;
        }
	up_read(&mm->mmap_sem);
	mmput(mm);

	return retval;
}

#define MAX_STOP_RUN_ATTEMPTS   (100)

int in_crashallassert(void) {
    return in_tivocrashallassert;
}
EXPORT_SYMBOL(in_crashallassert);

#define MAX_STOP_RUN_ATTEMPTS   (100)

int tivocrashallassert(unsigned long arg){
        struct task_struct *g, *t; 
        struct task_struct *syslog = NULL;
        char *tbuf;
        struct k_arg_list k_arg_list;
        struct k_arg *k_args,*k_argsp;
        unsigned int time_stamp = 0xdeadbeef;
        int err;
        int count;
        int tmkdbg=0;
        unsigned long flush_count = 0;

        if ( arg ) {
                err = copy_from_user((void *)&k_arg_list,
                            (void *)arg,sizeof(struct k_arg_list));
                if ( err )
                        return -EFAULT;

                k_args = (struct k_arg *)
                        kmalloc(sizeof(struct k_arg)*k_arg_list.count,GFP_KERNEL);
                if ( k_args == NULL )
                        return -ENOMEM;

                err = copy_from_user((void *)k_args,
                                    (void *)k_arg_list.kargs,
                                    sizeof(struct k_arg)*k_arg_list.count);
                if (err){
                        kfree(k_args);
                        return -EFAULT;
                }

                for ( count=0,k_argsp=k_args; count < k_arg_list.count; k_argsp++,count++){
                        switch(k_argsp->tag){
                            case TAG_TIME:
                                time_stamp = k_argsp->ptr;
                            break;
                            case TAG_TMKDBG:
                                if ( k_argsp->ptr ) 
                                    tmkdbg = 1;
                                else
                                    tmkdbg = 0; 
                            break; 
                            default:
                                printk( DEBUG_LEVEL "Invalid tag/ptr/len\n");
                        }
                }

                kfree(k_args);
        }

        tbuf = (char*)__get_free_page(GFP_KERNEL);
        if (!tbuf)
                return -ENOMEM;

        if ( in_crashallassert() )
                return -EINVAL;

        in_tivocrashallassert = 1;
        if (panic_blink) panic_blink(1);

        printk( DEBUG_LEVEL "bt_all_thread 0x%08x start\n",time_stamp);
        printk( DEBUG_LEVEL "global\n");
        /* 
	 *Prevent allmost all runnable threads from getting scheduled on a CPU.
	 *However, we would be waiting for the thread currently running on a different CPU 
	 *to relinquish the CPU before deactivating it.
         */ 
	for (count = 0; count < MAX_STOP_RUN_ATTEMPTS; count++) {
		read_lock_irq(&tasklist_lock);
		err = stop_runnables();
		read_unlock_irq(&tasklist_lock);
		if(!err) {
			break;
		} else {
			set_current_state(TASK_INTERRUPTIBLE);
			schedule_timeout(1); /* jiffies */
		}
        }

	if(count == MAX_STOP_RUN_ATTEMPTS) {
		printk(DEBUG_LEVEL "Failed to stop pid=%d\n",err);
	}
        dump_module_addrs();

        read_lock_irq(&tasklist_lock);
        do_each_thread(g, t) {
                if( !(++flush_count % 5) ) {
                        read_unlock_irq(&tasklist_lock);
                        set_current_state(TASK_INTERRUPTIBLE);
                        schedule_timeout(100);
                        read_lock_irq(&tasklist_lock);
                }   

                if (t == g) {
                        if(*(t->cmdline)){
                                printk(DEBUG_LEVEL "threadgroup %d name \"%s\"\n",
                                           g->tgid, t->cmdline);
                                if (strstr(t->cmdline, "syslogd") != NULL)
                                        syslog = t;
                        }else {
                                printk(DEBUG_LEVEL "threadgroup %d name \"%s\"\n",
                                           g->tgid, t->comm);
                                if (strstr(t->comm, "syslogd") != NULL)
                                        syslog = t;
                        }
                        /*Dump memory map of a valid threadgroup*/
                        pid_read_maps(g, tbuf);		
                }
                /* cmdline contains "<threadgroupname>:<space> <threadname>" in this format
                 * Back tace will not parse the ':', this logic extract and print the thread name from the cmdline 
                 */	
                if(*(t->cmdline)) {
                        unsigned char *pdata = NULL;
                        /*
                         * This logic is to extract the thread name from the cmdline after the ':<space> '
                         */ 
                        pdata = strchr(t->cmdline, ':');
                        if(pdata != NULL){
                                pdata += 2;
                                printk(DEBUG_LEVEL "pid %d threadgroup %d name \"%s\"\n",
                                        t->pid, t->tgid, pdata);
                        }else{
                                printk(DEBUG_LEVEL "pid %d threadgroup %d name \"%s\"\n",
                                          t->pid, t->tgid, t->cmdline);
                        }
                }else {
                        printk(DEBUG_LEVEL "pid %d threadgroup %d name \"%s\"\n",
                                   t->pid, t->tgid, t->comm);
                }
                /*Dump back traces*/
                show_trace_task_bt(t, debugconsole);
        
        } while_each_thread(g, t); 
        read_unlock_irq(&tasklist_lock);


        printk ( DEBUG_LEVEL "bt_all_thread 0x%08x end\n",time_stamp);

        free_page((unsigned long)tbuf);

        /*
         * Flush all disk buffers.
         */ 
        emergency_sync();

        /* 
         * Wait for some time (30 sec) before rebooting to ensure that crashall
         * is logged in the log files.
         */
        for (count=0; count<30; count++) {
	    unsigned long long now;

	    now = sched_clock();
	    set_current_state(TASK_UNINTERRUPTIBLE);
	    schedule_timeout(HZ);
	    if (syslog && syslog->last_ran < now)
		break;
        }
        
        /*
         * Done dumping back traces, reboot the TCD.
         */ 
#ifdef CONFIG_TIVO_NEUTRON
        /* On platforms with iSSD drive (ie, Lego), do graceful shutdown of the
         * drive before rebooting. */
        if (boardID & kTivoConfigGen07Strap_HasISSD) {
            named_device_shutdown("sd");
        }
#endif
        machine_restart(NULL);
        
        in_tivocrashallassert = 0;
    return 0;
}
EXPORT_SYMBOL(tivocrashallassert);

void dump_module_addrs(void)
{
        int fd;
        struct module *mod;
        char mod_path[256];
        char mod_name[MODULE_NAME_LEN];
        mm_segment_t old_fs;

        old_fs = get_fs();
        set_fs(KERNEL_DS);

        /*Kernel Loadable Module information*/
        list_for_each_entry(mod, &modules, list) {

                strcpy(mod_name,mod->name);

#ifdef CONFIG_TIVO_MOJAVE                
                if(!strcmp(mod->name,"bcm7401")){
                        strcat(mod_name, "_HR22");
                }
#endif                
                strcat(mod_name,".ko");
                strcpy(mod_path,"/platform/lib/modules/");
                strcat(mod_path,mod_name);

                fd = sys_open((char __user *)mod_path, 0, 0);

                if(fd < 0) {
                        strcpy(mod_path,"/lib/modules/");
                        strcat(mod_path,mod_name);
                        fd = sys_open((char __user *)mod_path, 0, 0);
                }

                if(fd >= 0){               
                        sys_close(fd);
                        printk(DEBUG_LEVEL "read 0x%p %s\n", mod->module_core , mod_path);
                }
        }

        set_fs(old_fs);
}

#ifdef CONFIG_TICK_KILLER
int tick_killer_start(int ticks) {
	if(ticks >= 0) {
		if(tick_killer_ticks == ticks) {
			printk("TickKiller called with %d, no effect\n", ticks);
			return 0;
		}
		if(ticks == TICK_KILLER_DISABLED) {
			printk("TickKiller disabled\n");
		} else {
			printk("TickKiller will kill the TCD in %d ticks\n",
					ticks);
		}
		tick_killer_ticks = ticks;
	} else {
		return -EINVAL;
	}
	return 0;
}
#endif


static int _crashall_thread(void *x) {
	(void) x;

	_plog_enable(0);
	if (!in_crashallassert())
		tivocrashallassert(0);

	return 0;
}

static struct timer_list crashall_timeout;
static void crashall_check(unsigned long x)
{
	if (!in_crashallassert())
		panic("Crashall did not start!\n", x);
}


// Fire off the thread to start a crashall.
int kernel_crashall(const char *msg) {
	int i;

	printk("Crashall: %s\n", msg);
	// Start a 10s deadman timer
	mod_timer(&crashall_timeout, jiffies + 10*HZ);
	wake_up_process(crashall_thread);

	return 0;
}
EXPORT_SYMBOL(kernel_crashall);


// Create a kernel thread we can use to crashall, but don't start it.
static int __init init_crashall(void)
{
	struct task_struct *p;

	p = kthread_create(_crashall_thread, 0, "crashall");
	if (IS_ERR(p)) {
		printk("Could not create crashall thread!\n");
		return -ENOMEM;
	}
	p->policy = SCHED_FIFO;
	p->rt_priority = MAX_RT_PRIO-1;
 
	crashall_thread = p;

	setup_timer(&crashall_timeout, crashall_check, 0);

	return 0;
}
subsys_initcall(init_crashall);
