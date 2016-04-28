/*
 * tivo specific /proc/sys support
 */
#include <linux/init.h>
#include <linux/sysctl.h>
#include <linux/proc_fs.h>
#include <linux/security.h>

#include <asm/uaccess.h>

/*
 * The new proc structure which indicates whether TCD had a warm reboot 
 * or cold reboot (power plugged)
 * */
struct proc_dir_entry *proc_IsWarm = NULL;
#define PROC_NAME_IS_WARM "IsWarm"
#define     TCD_WARM_REBOOT     1
unsigned char ucIsWarm=TCD_WARM_REBOOT;

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

int __init tivo_proc_sys_init(void)
{
	struct proc_dir_entry *proc_sys_root;

	proc_sys_root = create_proc_entry("dsscon", 0644, NULL);
	proc_sys_root->read_proc=read_dsscon;
	proc_sys_root->write_proc=write_dsscon;
    

#if defined(CONFIG_TIVO_DEVEL) || defined(CONFIG_TIVO_VMTCD)
	create_proc_entry("devkernel", 0444, NULL);
#endif

    /*
     * Create a proc entry IsWarm
     */
    proc_IsWarm = create_proc_read_entry(PROC_NAME_IS_WARM,0444,NULL,proc_IsWarm_Read,NULL);
    if(NULL==proc_IsWarm)
    {
        remove_proc_entry(PROC_NAME_IS_WARM,NULL); 
        printk("Error: Could not initialize /proc/%s\n",PROC_NAME_IS_WARM);
    }

	return 0;
}
