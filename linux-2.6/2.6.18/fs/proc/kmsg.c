/*
 *  linux/fs/proc/kmsg.c
 *
 *  Copyright (C) 1992  by Linus Torvalds
 *
 */

#include <linux/types.h>
#include <linux/errno.h>
#include <linux/time.h>
#include <linux/kernel.h>
#include <linux/poll.h>
#include <linux/fs.h>

#include <asm/uaccess.h>
#include <asm/io.h>

extern wait_queue_head_t log_wait;

extern int do_syslog(int type, char __user *bug, int count);

static int kmsg_open(struct inode * inode, struct file * file)
{
	return do_syslog(1,NULL,0);
}

static int kmsg_release(struct inode * inode, struct file * file)
{
	(void) do_syslog(0,NULL,0);
	return 0;
}

static ssize_t kmsg_read(struct file *file, char __user *buf,
			 size_t count, loff_t *ppos)
{
	if ((file->f_flags & O_NONBLOCK) && !do_syslog(9, NULL, 0))
		return -EAGAIN;
	return do_syslog(2, buf, count);
}

static unsigned int kmsg_poll(struct file *file, poll_table *wait)
{
	poll_wait(file, &log_wait, wait);
	if (do_syslog(9, NULL, 0))
		return POLLIN | POLLRDNORM;
	return 0;
}

#ifdef CONFIG_TIVO
static ssize_t kmsg_write(struct file *file, const char __user *buf, size_t count, loff_t *ppos)
{
    char buffer[128]; int n, x;
    
    if (!capable(CAP_SYS_RESOURCE)) return -EPERM;
    n=sizeof(buffer)-1;
    if (n > count) n=count; 
    copy_from_user(buffer, buf, n);
    buffer[++n]=0;
    for (x=0; buffer[x] >= ' ' && buffer[x] <= '~'; x++);
    buffer[x]=0;
    if (*buffer) printk(KERN_NOTICE "kmsg: %s\n", buffer); 
    return count;
}    
#endif    

const struct file_operations proc_kmsg_operations = {
	.read		= kmsg_read,
	.poll		= kmsg_poll,
	.open		= kmsg_open,
	.release	= kmsg_release,
#ifdef CONFIG_TIVO
    .write = kmsg_write
#endif
};
