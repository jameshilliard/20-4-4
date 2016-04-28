/**
 * Tivo specific /proc/sys support
 */
 
#include <linux/init.h>
#include <linux/sysctl.h>
#include <linux/proc_fs.h>
#include <linux/security.h>

#include <asm/uaccess.h>

/**
 * /proc interface which could be used to poke another CPU core
 * for dumping its current backtrace on the system concole.
 */
static struct proc_dir_entry *proc_cpupoke_entry = NULL;
#define       PROC_NAME_CPUPOKE "cpupoke"
extern void backtrace_current(void *unused);

/**
 * The proc structure which indicates whether TCD had a warm reboot 
 * or cold reboot (power plugged)
 * */
static struct proc_dir_entry *proc_IsWarm = NULL;
#define       PROC_NAME_IS_WARM "IsWarm"
#define       TCD_WARM_REBOOT     1
unsigned char ucIsWarm = TCD_WARM_REBOOT;

extern int dsscon;

static ssize_t write_dsscon(struct file *file, const char *buf, unsigned long count, void *data)
{
    char b[80]; // big enough to hold a number

    if (count)
    {
        memset(b,0,sizeof(b));
        copy_from_user(b, buf, count >= sizeof(b) ? sizeof(b) - 1 : count);
        dsscon = simple_strtol(b, NULL, 0);
    }

    return count;
}

static int read_dsscon(char *buf, char **start, off_t offset, int count, int *eof, void *data)
{
    int n = 0;

    if (!offset) n = sprintf(buf,"%d\n", dsscon);
    buf[n]=0;
    *eof=1;
    return n;
}

static int proc_IsWarm_Read(char *page, char **buffer_location, off_t offset, 
                        int buffer_length, int *eof, void *data)
{
    int iLen=0;
    if(!offset)
        iLen = sprintf(page,"%x\n",ucIsWarm);
    *eof = 1;
    return iLen;
}

static int proc_cpupoke(char  *page, 
                        char **buffer_location, 
                        off_t  offset, 
                        int    buffer_length, 
                        int   *eof, 
                        void  *data)
{
    (void)page; 
    (void)buffer_location;
    (void)offset;
    (void)buffer_length;
    (void)eof;
    (void)data;

    printk("CPU %d is poking other CPUs for backtrace...\n", smp_processor_id());

    // Producing backtrace on other CPUs, not waiting for completion
    smp_call_function(backtrace_current,NULL,0);

    return 0;
}

int __init tivo_proc_sys_init(void)
{
    struct proc_dir_entry *proc_sys_root;

    proc_sys_root = create_proc_entry("dsscon", 0644, NULL);
    proc_sys_root->read_proc  = read_dsscon;
    proc_sys_root->write_proc = write_dsscon;
    
    /*
     * Create a proc entry IsWarm
     */
    proc_IsWarm = create_proc_read_entry(PROC_NAME_IS_WARM,0444,NULL,proc_IsWarm_Read,NULL);
    if (NULL == proc_IsWarm)
    {
        remove_proc_entry(PROC_NAME_IS_WARM,NULL); 
        printk("Error: Could not create /proc/%s\n",PROC_NAME_IS_WARM);
    }

    /**
     * Create a proc entry cpupoke
     */
    proc_cpupoke_entry = create_proc_read_entry(PROC_NAME_CPUPOKE,
                                                0444,
                                                NULL,
                                                proc_cpupoke,
                                                NULL);
    if (NULL == proc_cpupoke)
    {
        remove_proc_entry(PROC_NAME_CPUPOKE, NULL); 
        printk("Error: Could not create /proc/%s\n",PROC_NAME_CPUPOKE);
    }

    return 0;
}
