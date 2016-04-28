/*
 *  Reset helper routines 
 *
 * Copyright (C) 2009 TiVo Inc. All Rights Reserved.
 */
#include <linux/config.h>
#include <linux/mm.h>
#include <linux/sysrq.h>
#include <linux/smp_lock.h>
#include <linux/proc_fs.h>
#include <linux/kdev_t.h>
#include <linux/major.h>
#include <linux/tivo-reset.h>

volatile int emergency_sync_scheduled;

extern struct scsi_disk *scsi_disk_get(struct gendisk *disk);
extern struct scsi_device *scsi_dev_get(struct scsi_disk *sdkp);
extern void ata_sync_platter(struct scsi_device* sd);

static int is_local_disk(dev_t dev)
{
	unsigned int major = MAJOR(dev);
	/* kernel 2.6 we have use SATA disk */		
	switch(major) {
	case SCSI_DISK0_MAJOR:
	case SCSI_DISK1_MAJOR:
	case SCSI_DISK2_MAJOR:
	case SCSI_DISK3_MAJOR:
	case SCSI_DISK4_MAJOR:
	case SCSI_DISK5_MAJOR:
	case SCSI_DISK6_MAJOR:
	case SCSI_DISK7_MAJOR:
        	return 1;
	default:
		return 0;
	}
}

struct scsi_device *sb_to_scsidev(struct super_block *sb) {
	int rc = 0;
	struct block_device *bdev = sb->s_bdev;
	struct gendisk *gp = NULL;
	struct scsi_disk *sdkp = NULL;
	struct scsi_device *scsidev = NULL;

	if(bdev)
	{
		if( (gp = bdev->bd_disk) != NULL)
		{
			if ((sdkp = scsi_disk_get(gp)) == NULL)
				rc = ENXIO;
		} else {
			rc = ENODEV;
		}
	} else {
		rc = ENODEV;
	}
	if (rc)
		goto out;

	if((scsidev = scsi_dev_get(sdkp)) == NULL)
                rc = ENXIO;
out: 
	if (rc != 0) 
		printk("sb_to_scsidev failed! gendisk:%p scsi_disk:%p err:%d\n"
			, gp, sdkp, rc);

	return scsidev;

}

void disk_emergency_sync(void)
{
	struct super_block *sb;
	struct scsi_dev *sdev;

	/* host side sync... */
	emergency_sync();

	/* ...and now the disk side sync */
	lock_kernel();
	emergency_sync_scheduled = 0;

	for (sb = sb_entry(super_blocks.next);
	     sb != sb_entry(&super_blocks); 
	     sb = sb_entry(sb->s_list.next)) {
		if (is_local_disk(sb->s_dev)){
			if ((sdev = sb_to_scsidev(sb)) != NULL)
				ata_sync_platter(sdev);
		}
	}

	unlock_kernel();
	printk(KERN_INFO "disk_emergency_sync done!\n");
}
