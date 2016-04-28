/*
 *  Reset helper routines 
 *
 * Copyright (C) 2009-2012 TiVo Inc. All Rights Reserved.
 */
#include <linux/config.h>
#include <linux/mm.h>
#include <linux/sysrq.h>
#include <linux/smp_lock.h>
#include <linux/proc_fs.h>
#include <linux/kdev_t.h>

static int is_local_disk(dev_t dev)	    /* Guess if the device is a local hard drive */
{
	unsigned int major = MAJOR(dev);
//FIXME
#if 0
	switch (major) {
	case IDE0_MAJOR:
	case IDE1_MAJOR:
	case IDE2_MAJOR:
	case IDE3_MAJOR:
		return 1;
	default:
		return 0;
	}
#endif   
        return 1;
}

//FIXME
static void go_sync(struct super_block *sb)
{
	//printk(KERN_INFO "Syncing device %s ... ",
	  //     kdevname(sb->s_dev));

	//fsync_bdev(sb->s_dev);			    /* Sync only */
	printk("OK\n");
}

volatile int emergency_sync_scheduled;

void do_emergency_sync(void)
{
	struct super_block *sb;

	lock_kernel();
	emergency_sync_scheduled = 0;

	for (sb = sb_entry(super_blocks.next);
	     sb != sb_entry(&super_blocks); 
	     sb = sb_entry(sb->s_list.next))
		if (is_local_disk(sb->s_dev))
			go_sync(sb);

	for (sb = sb_entry(super_blocks.next);
	     sb != sb_entry(&super_blocks); 
	     sb = sb_entry(sb->s_list.next))
		if (!is_local_disk(sb->s_dev) && MAJOR(sb->s_dev))
			go_sync(sb);

	unlock_kernel();
	printk(KERN_INFO "Done.\n");
}
