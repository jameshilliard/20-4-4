#include <linux/autoconf.h>
#include <linux/interrupt.h>
#include <linux/kthread.h>
#include <linux/sched.h>
#include <linux/module.h>
#include <linux/kallsyms.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>

#define THREADNAME_SIZE_BYTES   16
#define THREADNAME_SIZE_WORDS   (THREADNAME_SIZE_BYTES/4)

/* A trace entry contains the thread name, and 2 words for PID and PC */
#define TRACEENTRY_SIZE_WORDS   (THREADNAME_SIZE_WORDS+2)

static struct proc_dir_entry *proc_trace_dir;
static struct proc_dir_entry *trace_proc_entry[2];

static unsigned long trace_buffersize     = 0; /* in words */
static unsigned long trace_maxentries     = 0; /* in words */
static unsigned long trace_size           = 0; /* in entries */
static unsigned long trace_output_size    = 0; /* in entries */
static unsigned long trace_counter        = 0; /* in entries */
static unsigned long trace_entries        = 0; /* in entries */
static unsigned long trace_output_entries = 0; /* in entries */
static unsigned long *pTraceData          = NULL;
 
void store_trace_entry(void)
{
	struct pt_regs *regs;
	struct task_struct *current_thread;
	volatile unsigned long *pWrite;
	unsigned long name[THREADNAME_SIZE_WORDS];
	unsigned long pid;
	unsigned long pc;
	int i;

	if (trace_size != 0)
	{
		pWrite = pTraceData + TRACEENTRY_SIZE_WORDS * trace_counter + 3;

		regs = get_irq_regs();
		if (regs)
		{
			pc = regs->cp0_epc;
		}
		else
		{
			pc = (unsigned long)-1;
		}

		current_thread = get_current();
		if (current_thread)
		{
			memcpy(&name, current_thread->comm, THREADNAME_SIZE_BYTES);
			pid = current_thread->pid;
		}
		else
		{
			strcpy((unsigned char *)&name, "null thread");
			pid = (unsigned long)-1;
		}

		for (i = 0; i < THREADNAME_SIZE_WORDS; i++)
		{
			__raw_writel((unsigned long)name[i], pWrite++);
		}
		__raw_writel((unsigned long)pid,           pWrite++);
		__raw_writel((unsigned long)regs->cp0_epc, pWrite++);

		trace_counter = (trace_counter + 1) % trace_size;
		__raw_writel(trace_counter, pTraceData + 1);
		if (trace_entries < trace_size)
		{
			trace_entries++;
			__raw_writel(trace_entries, pTraceData + 2);
		}
	}
}

static void *trace_seq_start(struct seq_file *file, loff_t *pos)
{
	static unsigned long trace_current_line = 0;

	trace_output_size = pTraceData[0];
	if ((trace_output_size & (unsigned long)0xfff00000) == (unsigned long)0xace00000)
	{
		trace_output_size &= (unsigned long)0x000fffff;
		trace_output_entries = pTraceData[2];
		if ((*pos < trace_output_size) && (*pos < trace_output_entries))
		{
			if (trace_output_entries < trace_output_size)
			{
				trace_current_line = ((unsigned long)*pos) % trace_output_size;
			}
			else
			{
				trace_current_line = (pTraceData[1] + (unsigned long)*pos) % trace_output_size;
			}
			return (void *)&trace_current_line;
		}
	}

	return NULL;
}

static void *trace_seq_next(struct seq_file *file, void *v, loff_t *pos)
{
	unsigned long *tmp_v = (unsigned long *)v;

	if (v != NULL)
	{
		*tmp_v = (*tmp_v + 1) % trace_output_size;
		*pos += 1;
	}

	return ((*pos < trace_output_size) && (*pos < trace_output_entries)) ? tmp_v : NULL;
}

static void trace_seq_stop(struct seq_file *file, void *v)
{
}

static int trace_seq_show(struct seq_file *file, void *v)
{
	unsigned long trace_current_line;
	unsigned char thread_name[THREADNAME_SIZE_BYTES+1];
	unsigned int pid;
	unsigned long pc;
	unsigned char function_name[KSYM_SYMBOL_LEN+1];

	if (v != NULL)
	{
		trace_current_line = *((unsigned long *)v);

		if (pTraceData != NULL)
		{
			memset(thread_name, 0, sizeof(thread_name));
			memcpy(thread_name, &pTraceData[TRACEENTRY_SIZE_WORDS*trace_current_line+3], THREADNAME_SIZE_BYTES);

			pid = pTraceData[TRACEENTRY_SIZE_WORDS*trace_current_line+THREADNAME_SIZE_WORDS+3];

			memset(function_name, 0, sizeof(function_name));
			pc  = pTraceData[TRACEENTRY_SIZE_WORDS*trace_current_line+THREADNAME_SIZE_WORDS+4];
			if ( ((pc & 0xf0000000) == 0x80000000) || ((pc & 0xf0000000) == 0xe0000000) )
			{
				/* kernelspace */
				if (sprint_symbol(function_name, pc) == 0)
				{
					/* did not find symbol */
					sprintf(function_name, "0x%08lx", pc);
				}
			}
			else
			{
				/* userspace, no way to find symbol name yet */
				sprintf(function_name, "0x%08lx", pc);
			}

			seq_printf(file, "%s(%d) %s\n", thread_name, pid, function_name);
		}
	}

	return 0;
}

static int trace_reset(struct file *file, const char *buffer, unsigned long count, void *data)
{
	int i;
	unsigned long value = 0;

	trace_counter = 0;
	trace_entries = 0;

	for (i = 0; i < count; i++)
	{
		if ((buffer[i] >= '0') && (buffer[i] <= '9'))
		{
			value = (value * 10) + (buffer[i] - '0');
		}
	}

	/* Limit trace entries to fit into buffer */
	if (value > trace_maxentries)
	{
		value = trace_maxentries;
	}
	trace_size = value;

	if (trace_size)
	{
		/* Number of minutes depends on sample rate set in bcm_msclock.c - currently once every 10 ms */
		printk("%s: starting trace, %lu samples (%lu minutes)\n", __FUNCTION__, trace_size, trace_size / (60*100));
	}
	else
	{
		printk("%s: stopping trace\n", __FUNCTION__);
	}

	/* trace_size == 0 means disable trace from now on */
	/* don't mess up a trace that's already in memory  */
	if (trace_size != 0)
	{
		pTraceData[0] = trace_size | (unsigned long)0xace00000;
	}

	return count;
}

static struct seq_operations trace_seq_ops = {
	.start = trace_seq_start,
	.next  = trace_seq_next,
	.stop  = trace_seq_stop,
	.show  = trace_seq_show,
};

static int trace_proc_open(struct inode *inode, struct file *file)
{
	return seq_open(file, &trace_seq_ops);
}

static struct file_operations trace_proc_ops = {
	.open    = trace_proc_open,
	.read    = seq_read,
	.llseek  = seq_lseek,
	.release = seq_release,
};

void tracebuffer_setup(unsigned long *buffer, unsigned long size)
{
	printk("%s - %lu bytes @ 0x%08lx\n", __FUNCTION__, size, buffer);

	pTraceData = buffer;
	trace_buffersize = size/4; /* in words */
	trace_maxentries = (trace_buffersize-3)/TRACEENTRY_SIZE_WORDS; /* first 3 words are for admin */

	proc_trace_dir = create_proc_entry( "trace", S_IFDIR, NULL );
	if (proc_trace_dir)
	{
		trace_proc_entry[0] = create_proc_entry("reset", 0644, proc_trace_dir);
		if (trace_proc_entry[0])
		{
			trace_proc_entry[0]->read_proc  = NULL;
			trace_proc_entry[0]->write_proc = trace_reset;
			trace_proc_entry[0]->data       = NULL;
		}

		trace_proc_entry[1] = create_proc_entry("output", 0644, proc_trace_dir);
		if (trace_proc_entry[1])
		{
			trace_proc_entry[1]->proc_fops = &trace_proc_ops;
		}
	}
	else
	{
		printk(KERN_CRIT "%s: could not create /proc/trace\n", __FUNCTION__);
	}
}
EXPORT_SYMBOL(tracebuffer_setup);
