/* 
 * This is to write out to the disk in the case that we have
 * essentially a non-functional TiVo.  The idea here is we're
 * going to write out ot the swap partition some log of what
 * happened.  The two cases to get us here generally are
 * a kernel panic, or a watch dog fire.
 *
 * We are going to write out to swap, on boot up, we'll scan
 * the swap before turning it on to see our signature.  If
 * the signature is present, then we'll extract the data
 * and put it in our normal logs.
 *
 */
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/blkdev.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/device.h>
#include <linux/types.h>
#include <linux/major.h>
#include <linux/root_dev.h>

#include <scsi/scsi_device.h>
#include <scsi/scsi_host.h>
#include <linux/libata.h>
#include <linux/ata_deadsystem.h>

#define SWAPFS_MAJOR	SCSI_DISK0_MAJOR
#define SWAPFS_MINOR	8

#define OK_STAT(stat,good,bad)  (((stat)&((good)|(bad)))==(good))

extern struct scsi_disk *scsi_disk_get(struct gendisk *disk);
extern void scsi_disk_put(struct scsi_disk *sdkp);
extern struct ata_device *
ata_scsi_find_dev(struct ata_port *ap, const struct scsi_device *scsidev);
extern struct scsi_device *scsi_dev_get(struct scsi_disk *sdkp);

static int initdrive=1;

static inline int pio_busy_wait(struct ata_port *ap, unsigned int max)
{
        do {
                udelay(1);
        } while ((ata_chk_status(ap) & ATA_BUSY) && (--max > 0));

        if(max == 0){
                return -ENODEV;
        }
        else{
                return 0;
        }
}

static inline int pio_wait(struct ata_port *ap)
{
        u8 status;
	ata_busy_wait(ap, ATA_BUSY | ATA_DRQ, 100000);

        status = ata_chk_status(ap);
	if (!OK_STAT(status, ATA_DRDY, (ATA_BUSY | ATA_DF | ATA_DRQ | ATA_ERR))) {
		unsigned long l = ap->ioaddr.status_addr;
			printk("ATA Dead System: abnormal status 0x%X on port 0x%lX\n",
				status, l);
                        return -ENODEV;
	}

	return 0;
}

void inline lba48_address_write(struct ata_port *ap, unsigned long sector) {
        struct ata_taskfile tf;
        struct ata_device *dev = ap->link.device;
        
        ata_tf_init(dev, &tf);
        
        tf.flags |= ATA_TFLAG_ISADDR | ATA_TFLAG_DEVICE | ATA_TFLAG_LBA48;
        tf.nsect = 1;
        tf.hob_nsect = (1 >> 8) & 0xff;
        tf.hob_lbah = ((u64)sector >> 40) & 0xff;
        tf.hob_lbam = ((u64)sector >> 32) & 0xff;
        tf.hob_lbal = (sector >> 24) & 0xff;
        tf.lbah = (sector >> 16) & 0xff;
        tf.lbam = (sector >> 8) & 0xff;
        tf.lbal = sector & 0xff;
        tf.device = 1 << 6;

        ap->ops->tf_load(ap, &tf);
	writeb(ATA_CMD_PIO_WRITE_EXT , (void __iomem *)ap->ioaddr.command_addr);	
}

void inline lba28_address_write(struct ata_port *ap, unsigned long sector) {
        struct ata_taskfile tf;
        struct ata_device *dev = ap->link.device;
        
        ata_tf_init(dev, &tf);
        
        tf.flags |= ATA_TFLAG_ISADDR | ATA_TFLAG_DEVICE;
        tf.nsect = 1;
        tf.lbah = (sector >> 16) & 0xff;
        tf.lbam = (sector >> 8) & 0xff;
        tf.lbal = sector & 0xff;
        tf.device = ((sector>>24) & 0xf) | 1 << 6;

        ap->ops->tf_load(ap, &tf);
	writeb(ATA_CMD_PIO_WRITE, (void __iomem *)ap->ioaddr.command_addr);	
}

int pio_write_buffer(struct ata_device *dev, struct ata_port *ap, unsigned long sector,
		     const u8* buf, unsigned int size, u32 *csum) {
	int ret;
	int writtensects=0;
	int i;
	u32 checksum = 0, carry = 0;
	const u16 *id = dev->id;
	unsigned long flags;

	spin_lock_irqsave(ap->lock, flags);
	
	while(size > 0) {
		/* Written according to ATA - 6 specification */
		int cnt=0;
		if (ata_id_has_lba48(id)) {
			/* LBA 48 addressing */
			lba48_address_write(ap, sector);
		} else if (ata_id_has_lba(id)) {
			/* LBA 28 addressing */
			lba28_address_write(ap, sector);
		} else {
			printk("ATA Dead System doesn't do CHS\n");
			spin_unlock_irqrestore(ap->lock, flags);
			return -ENODEV;
		}

		udelay(1);

		if((ret = pio_busy_wait(ap, 100000)) < 0) {
			spin_unlock_irqrestore(ap->lock, flags);			
			return ret;
		}

		for(i=(512/2);i;i--) { 
			u16 data;
			/* Unaligned access unwise, makes for funky code */
			if(size >= 2) {
				data = *buf++;
				data |= (*buf++)<<8;
				size -= 2;
			} else if (size == 1){
				data = *buf++;
				size = 0;
			} else {
				data = 0;
			}
			
			writew(data, (void __iomem *)ap->ioaddr.data_addr);
			checksum += carry;
			checksum += data;
			carry = (data > checksum);
			cnt++;
		}
		mdelay(5);

		if((ret = pio_wait(ap)) < 0) {
			spin_unlock_irqrestore(ap->lock, flags);						
			return ret;
		}

		sector++;
		writtensects++;
	}

	spin_unlock_irqrestore(ap->lock, flags);			
	
	checksum += carry;
	
	if(csum != NULL) {
		*csum += checksum;
		if(checksum > *csum)
			*csum = (*csum) + 1;
	}
	
	return writtensects;
}

static int pio_reset(struct ata_port *ap) {
	unsigned long flags;
	
	int retries=10;

	spin_lock_irqsave(ap->lock, flags);
	
	while(--retries) {
		/* Delays are ATAPI-6 spec times 5 */
		writeb(ATA_NIEN | ATA_SRST, (void __iomem *)ap->ioaddr.ctl_addr);		
		udelay(60);
		writeb(ATA_NIEN, (void __iomem *)ap->ioaddr.ctl_addr);				

		mdelay(50);
                
		if(pio_busy_wait(ap, 250000) == 0) { /* Up to 250 ms */
                        if(OK_STAT(ata_chk_status(ap), 0, (ATA_DRQ | ATA_ERR))) {
				break;
			}
		}
	}

	if(retries == 0) {
		spin_unlock_irqrestore(ap->lock, flags);			
		printk("ATA Dead System: drive won't reset, giving up\n");
		return -ENODEV;
	}

        ap->ops->dev_select(ap, 0);
	
	udelay(1);
	
	if(pio_wait(ap) < 0) { // Device ready?
		spin_unlock_irqrestore(ap->lock, flags);			
		return -ENODEV;
	}
	/*
	 * According to the spec, that's all we *have* to do
	 * since we already have the info about the drive that
	 * the kernel queried out.  If this gets buggy on some
	 * drives, we might try adding an IDENTIFY command (0xec
	 * I think)
	 */
	spin_unlock_irqrestore(ap->lock, flags);			

	return 0;
}

/* Returns number of sectors written */
int pio_write_buffer_to_swap(const u8* buf, unsigned int size, int secoffset,
	u32 *csum) {
	struct block_device *bdev = NULL;
	struct gendisk *gp = NULL;
	struct scsi_disk *sdkp = NULL;
	struct scsi_device *sdev = NULL;
	struct ata_port *ap = NULL;
	struct ata_device *dev = NULL;
        int written_sect;
	
	unsigned long tsector;

        if( (bdev = bdget(MKDEV(SWAPFS_MAJOR, SWAPFS_MINOR))) != NULL )
        { 
                if( (gp = bdev->bd_disk) != NULL){
                        if ((sdkp = scsi_disk_get(gp)) == NULL){
                                return -ENXIO;
                        }
                }
                else{
                        return -ENODEV;
                }
        }
        else{
                return -ENODEV;
        }

        if((sdev = scsi_dev_get(sdkp)) == NULL)
        {
                return -ENXIO;
        }

        ap = ata_shost_to_port(sdev->host);
	dev = ata_scsi_find_dev(ap, sdev);	  	   

        if( ap == NULL || dev == NULL ){
                return -ENXIO;
        }

	/*
	 * We adjust secoffset to write to the last 1MB of the device,
	 * this is because we don't want to rebuild swap mostly, and
	 * because we can use "lspanic" to grab the last tivo panic.
	 *
	 * We will track which one we've logged by bit-inverting
	 * the checksum.
	 *
	 */

	/* Partition must be at least 2 MB */
	if(gp->part[SWAPFS_MINOR - 1]->nr_sects < (2* 1024 * 1024 / 512)){
		return -ENODEV;
        }

	/* Offset by 1MB from end of partition */
	secoffset += gp->part[SWAPFS_MINOR - 1]->nr_sects - (1 * 1024 * 1024 /512);

	/* Make sure partition is sane size-wise */

	if(gp->part[SWAPFS_MINOR - 1]->nr_sects < secoffset){
		return -ENODEV;
        }
	if(gp->part[SWAPFS_MINOR - 1]->nr_sects - secoffset < ((size + 511)/512)) {
		return -ENODEV;
	}
	
	if(initdrive) {
                /* Issue software reset, disable ide interrupts */
		if(pio_reset(ap) < 0) { 
			return -ENODEV;
		}
	
                initdrive=0;
	}

	tsector = gp->part[SWAPFS_MINOR - 1]->start_sect + secoffset;

        written_sect = pio_write_buffer(dev, ap, tsector, buf, size, csum);

	scsi_disk_put(sdkp);

	return written_sect;
}

#define CORE_START (0x80000000)
#define CORE_SIZE  (32 * 1024 * 1024)

int pio_write_corefile(int offset) {
	/*
	 * The original idea here is to write to the "alternate
	 * application partition".  This has some problems though,
	 * spec. with developer boxes.  For now, we're going to
	 * assume this won't be called from a dev build.  (This is so
	 * broken it bothers me, we could write it to swap -32M -1M)
	 *
	 * The second broken thing here is we don't account for
	 * actual size of the memory of the box in question.  We
	 * assume it's 32Meg.  This is somewhat easily overcome by
	 * just checking the config variables.  However, I'm loathe
	 * to do that at this point since having variable size input
	 * just risks adding a new bug to the system.  (Something I don't
	 * want to do at this point in the release cycle)
	 *
	 * This is a *very basic* implementation of what we want to do.
	 * It starts at 0x80000000, goes for 32MB writing out to alternate
	 * application at either "offset 0", or "offset 1".  Offset 0 is
	 * at the beginning of the partition.  Offset 1 is 32MB into the
	 * partition.  (ah, now you see why I don't want to deal with
	 * variable sizes)
	 *
	 * Cache flushing is not required as we're doing it PIO, which just
	 * pulls from cache.  Of course, we're taking the kernel centric
	 * as opposed to hardware centric view of memory.  This is reasonable
	 * because really, the core is just a bunch of kernel memory
	 * structures for the most part.
	 * 
	 */
	struct block_device *bdev;
	struct gendisk *gp;
	struct scsi_disk *sdkp;
	struct scsi_device *sdev;
	struct ata_port *ap;
	struct ata_device *dev;
	
	int altpartition;
	unsigned long secoffset = (CORE_SIZE / 512) * offset;
	u32 csum; /* Currently unused */
	static int core_written = 0;

	/* Only 0 or 1 are valid offset values */
	if(!(offset == 0 || offset == 1))
		return -EINVAL;

	/* If the root is sda4, then altap is sda7 and vice-versa */
	if(ROOT_DEV == MKDEV(SCSI_DISK0_MAJOR, 4))
		altpartition = 7;
	else if(ROOT_DEV == MKDEV(SCSI_DISK0_MAJOR, 7))
		altpartition = 4;
	else
		return -ENODEV;

        if( (bdev = bdget(MKDEV(SCSI_DISK0_MAJOR, 0))) != NULL )
        { 
                if( (gp = bdev->bd_disk) != NULL){
                        if ((sdkp = scsi_disk_get(gp)) == NULL){
                                return -ENXIO;
                        }
                }
                else{
                        return -ENODEV;
                }
        }
        else{
                return -ENODEV;
        }

        if((sdev = scsi_dev_get(sdkp)) == NULL)
        {
                return -ENXIO;
        }

        ap = ata_shost_to_port(sdev->host);
	dev = ata_scsi_find_dev(ap, sdev);	  	   

        if( ap == NULL || dev == NULL ){
                return -ENXIO;
        }
        
	if(core_written != 0) {
		printk("Core already written\n");
		return -EINVAL;
	}
	
	core_written = 1;
	
	/* Partition must be at least twice a CORE */
	if(gp->part[altpartition - 1]->nr_sects < ((2 * CORE_SIZE) / 512))
		return -ENODEV;
	
	if(initdrive) {
                /* Issue software reset, disable ide interrupts */
		if(pio_reset(ap) < 0) { 
			return -ENODEV;
		}
		initdrive=0;
	}

	secoffset += gp->part[altpartition - 1]->start_sect;
	
	return pio_write_buffer(dev, ap, secoffset, (const u8*)CORE_START,
		       CORE_SIZE, &csum);
}
