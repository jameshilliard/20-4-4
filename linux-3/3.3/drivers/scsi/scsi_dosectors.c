/*
*
* SCSI Raw Disk DIO Interface
*
*/

#include <linux/bio.h>
#include <linux/genhd.h>
#include <linux/errno.h>
#include <linux/blkdev.h>
#include <linux/time.h>

#include <linux/ide-tivo.h>
#include <linux/tivo-tagio.h>
#include <linux/tivo-monotime.h>

#include <asm/uaccess.h>
#include <asm/cacheflush.h>

#include <scsi/scsi.h>
#include <scsi/scsi_cmnd.h>
#include <scsi/scsi_ioctl.h>
#include <scsi/sg.h>
#include "scsi_logging.h"

extern void __generic_unplug_device(struct request_queue *);

#ifdef CONFIG_TIVO
#define MAX_NRSECTORS   4096
#else
#define MAX_NRSECTORS   512
#endif
#define SECTOR_BYTES    512
#define MAX_BYTES       (MAX_NRSECTORS * SECTOR_BYTES)
unsigned int err_count = 10;

static struct blk_plug plug;

static int split_requests(struct block_device *bdev, struct FsIovec *buf,
                int buf_len, unsigned long long start_sect,
                monotonic_t *deadline, int rw);

static int prepare_and_do_scsi_request(struct block_device *bdev,
	unsigned long long sector, unsigned long long nr_sectors,
	struct FsIovec *iov_user, int len, int direction,
	monotonic_t *deadline);


static inline long long now_usec(void) {
        long long msec;
        struct timeval tv;

        do_gettimeofday(&tv);
        msec = (long long)tv.tv_sec * 1000000 + tv.tv_usec;
        return msec; 
}

static void scsi_dosectors_end_io(struct request *rq, int error)
{
	struct completion *waiting = rq->end_io_data;
	complete(waiting);
}

static int scsi_dosectors_io(struct block_device *bdev, struct sg_io_hdr *hdr,
		unsigned long long sector, monotonic_t *deadline)
{
	unsigned long start_time, flags;
	int writing = 0, ret = 0;
	struct request *rq;
	struct gendisk *bd_disk = bdev->bd_disk;
        struct request_queue *q;
	struct bio *bio;
        DECLARE_COMPLETION_ONSTACK(wait);

        q = bd_disk->queue;
	if (!q)
		return -ENXIO;

	if (hdr->cmd_len > BLK_MAX_CDB)
		return -EINVAL;

	if (hdr->dxfer_len > (queue_max_hw_sectors(q) << 9))
		return -EIO;

        BUG_ON(!hdr->dxferp);

	if (hdr->dxfer_len)
		switch (hdr->dxfer_direction) {
                        default:
                                return -EINVAL;
                        case SG_DXFER_TO_DEV:
                                writing = 1;
                                break;
                        case SG_DXFER_TO_FROM_DEV:
                        case SG_DXFER_FROM_DEV:
                                break;
		}

#ifdef CONFIG_TIVO
	rq = blk_get_urgent_request(q, writing ? WRITE : READ, GFP_KERNEL);
#else
	rq = blk_get_request(q, writing ? WRITE : READ, GFP_KERNEL);
#endif
	if (!rq)
		return -ENOMEM;

	memcpy(rq->cmd, hdr->cmdp, hdr->cmd_len);

	/*
	 * fill in request structure
	 */
	rq->cmd_len = hdr->cmd_len;
	rq->cmd_type = REQ_TYPE_FS;

	rq->timeout = msecs_to_jiffies(hdr->timeout);
	if (!rq->timeout)
		rq->timeout = q->sg_timeout;
	if (!rq->timeout)
		rq->timeout = BLK_DEFAULT_SG_TIMEOUT;
	if (rq->timeout < BLK_MIN_SG_TIMEOUT)
		rq->timeout = BLK_MIN_SG_TIMEOUT;

        rq->rq_disk = bd_disk;
        hd_struct_get(bdev->bd_part);
        rq->part = bdev->bd_part;

        ret = blk_rq_map_user_iov(q, rq, NULL, hdr->dxferp, hdr->iovec_count,
                hdr->dxfer_len, GFP_KERNEL);

	if (ret < 0) {
                printk("scsi_dosectors: blk_rq_map_user_iov failed err = %d \n", ret);
		goto out;
        }

        flush_dcache_all();

	bio = rq->bio;
	bio->bi_sector = sector;
	bio->bi_bdev = bdev;

	/*
	 * nasty to do, but this would have been missed because the code path
	 * that initializes rq from bio was invoked before we got a chance to
	 * set bi_sector
	 */
	rq->__sector = sector;

	rq->sense = hdr->sbp;
	rq->sense_len = 0;

    rq->deadline = tivo_monotonic_retrieve(deadline);

	rq->retries = 0;

	start_time = jiffies;

    BUG_ON(!rq->sense);

	rq->end_io_data = &wait;
    rq->end_io = scsi_dosectors_end_io;
    WARN_ON(irqs_disabled());

    spin_lock_irqsave(q->queue_lock, flags);
#ifdef  CONFIG_TIVO_PLOG
    plog_diskdispatch(sector, rq);
#endif
    __elv_add_request(q, rq, ELEVATOR_INSERT_SORT);

    __blk_run_queue(q);

    spin_unlock_irqrestore(q->queue_lock, flags);

    wait_for_completion(&wait);

	rq->end_io_data = NULL;

	hdr->duration = jiffies_to_msecs(jiffies - start_time);

	hdr->status = 0xff & rq->errors;
	hdr->masked_status = status_byte(rq->errors);
	hdr->msg_status = msg_byte(rq->errors);
	hdr->host_status = host_byte(rq->errors);
	hdr->driver_status = driver_byte(rq->errors);
	hdr->info = 0;
	if (hdr->masked_status || hdr->host_status || hdr->driver_status)
		hdr->info |= SG_INFO_CHECK;
	hdr->resid = rq->resid_len;
	hdr->sb_len_wr = 0;

// XXX: isn't resid bytes remaining??? and yet this does seem to get the right value. 
        /*Return number of bytes transfered*/
        ret = hdr->resid;

	if (rq->sense_len && hdr->sbp) {
		int len = min((unsigned int) hdr->mx_sb_len, rq->sense_len);
                hdr->sb_len_wr = len;
	}

	if (blk_rq_unmap_user(bio))
		ret = -EFAULT;

// XXX: why is this needed?
        flush_dcache_all();
	/* may not have succeeded, but output values written to control
	 * structure (struct sg_io_hdr).  */
out:
	blk_put_request(rq);
	return ret;
}

int scsi_dosectors(struct block_device *bdev, void __user *arg)
{
        struct {
                tivotag tagtuple;
                int present;
        } tagarray[TIVOTAGIO_NUMTAGS];
        tivotag tagtuple;
        char *argPtr;
        int nTags;
        struct FsIovec *iov_user;
        int user_len, nbytes = 0;
        int rw;
        sector_t start_sect, nr_sects;
        unsigned long long sector, nr_sectors; 
        monotonic_t deadline;

	start_sect = get_start_sect(bdev);

	if(bdev == bdev->bd_contains) {
		/* read/write on whole /dev/sda or /dev/sdb */
		nr_sects = get_capacity(bdev->bd_disk);
	} else {
		/* read/write on /dev/sda[1-15] or /dev/sdb[1-15] */
		struct hd_struct *part = bdev->bd_part;

		if (part->nr_sects == 0) {
			return -ENXIO;
		}

		nr_sects = part->nr_sects;
	}

        memset(tagarray, 0, sizeof tagarray);
        argPtr = (char *) arg;
        nTags = 0;

        /* Process the tags in the array.  Check for out-of-range tag codes and
        * duplicates. */
        do {
                if (copy_from_user(&tagtuple, argPtr, sizeof tagtuple)) {
                        return -EFAULT;
                }
                if (tagtuple.tag < TIVOTAGIO_END || tagtuple.tag >= TIVOTAGIO_NUMTAGS) {
                        return -EINVAL;
                }
                if (tagarray[tagtuple.tag].present++) {
                        return -EINVAL;
                }
                tagarray[tagtuple.tag].tagtuple = tagtuple;
                argPtr += sizeof tagtuple;
                nTags ++;
        } while (tagtuple.tag != TIVOTAGIO_END);

        /* Check for required tags and acceptable values */

        if (!tagarray[TIVOTAGIO_CMD].present ||
        !tagarray[TIVOTAGIO_IOVEC].present ||
        !tagarray[TIVOTAGIO_SECTOR].present ||
        !tagarray[TIVOTAGIO_NRSECTORS].present ||
        (tagarray[TIVOTAGIO_CMD].tagtuple.val != READ &&
        tagarray[TIVOTAGIO_CMD].tagtuple.val != WRITE)) {
                return -EINVAL;
        }

        rw = tagarray[TIVOTAGIO_CMD].tagtuple.val;
        
        /* Offset for the beginning of this partition */
        sector = tagarray[TIVOTAGIO_SECTOR].tagtuple.val;
        nr_sectors = tagarray[TIVOTAGIO_NRSECTORS].tagtuple.val;

        /* Check to see if its off the end of this partition */
	if (sector > nr_sects || sector + nr_sectors > nr_sects) {
                printk(KERN_WARNING "scsi_dosectors: Access beyond end of "
                                "drive %llu %llu %llu %llu\n",sector,start_sect,nr_sectors,nr_sects);
                return -EINVAL; 
        }

        user_len = tagarray[TIVOTAGIO_IOVEC].tagtuple.size/sizeof(struct FsIovec);
        iov_user = (struct FsIovec *) (unsigned long int)
                        tagarray[TIVOTAGIO_IOVEC].tagtuple.val;
        sector += start_sect;

        /*
         * support non-monotonic (gettimeofday-based) deadlines for backwards
	 * compatibility, by converting the time diff to a monotonic time
         */
        if (tagarray[TIVOTAGIO_DEADLINE].present &&
                        tagarray[TIVOTAGIO_DEADLINE].tagtuple.val != 0) {
                long long delta_usec;
                delta_usec = tagarray[TIVOTAGIO_DEADLINE].tagtuple.val - now_usec();
                tivo_monotonic_get_current(&deadline);
                tivo_monotonic_add_usecs(&deadline, delta_usec);
        } else {
                /*
                 * yes, this uses 0 to mean no deadline, which leaves a small possibility
                 * every thousand years or so of system run time that a real deadline
                 * will be ignored.  Live with it.
                 */
                tivo_monotonic_store(&deadline,
                                tagarray[TIVOTAGIO_DEADLINE_MONOTONIC].tagtuple.val);
        }

        if (!access_ok(VERIFY_READ, iov_user, user_len)) {
                printk( "Fail check iovec %08x\n", (unsigned int)iov_user );
                return -EFAULT;
        }

        if(nr_sectors <= MAX_NRSECTORS)
        {
                /*No need to split the request, send it directly */
                /* prepare a real request */
                return prepare_and_do_scsi_request(bdev, sector, nr_sectors,
				iov_user, user_len, rw, &deadline);
        }
        else
        {
                nbytes = split_requests(bdev, iov_user, user_len , sector, &deadline, rw);

                return nbytes;
        }
}

static int prepare_and_do_scsi_request(struct block_device *bdev,
	unsigned long long sector, unsigned long long nr_sectors,
	struct FsIovec *iov_user, int len, int direction,
	monotonic_t *deadline)
{
	struct sg_io_hdr sghdr;
	struct sg_io_hdr *io_hdr = &sghdr;
        unsigned char rdCmd[BLK_MAX_CDB];
        unsigned char senseBuff[SCSI_SENSE_BUFFERSIZE];

        memset(io_hdr, 0, sizeof(struct sg_io_hdr));

        /*Build read/write command*/
        memset(rdCmd, 0, BLK_MAX_CDB);

        rdCmd[0] = (unsigned char)(direction == READ ? READ_16 : WRITE_16);
        rdCmd[2] = (unsigned char)((sector >> 56) & 0xff);
        rdCmd[3] = (unsigned char)((sector >> 48) & 0xff);
        rdCmd[4] = (unsigned char)((sector >> 40) & 0xff);
        rdCmd[5] = (unsigned char)((sector >> 32) & 0xff);
        rdCmd[6] = (unsigned char)((sector >> 24) & 0xff);
        rdCmd[7] = (unsigned char)((sector >> 16) & 0xff);
        rdCmd[8] = (unsigned char)((sector >> 8) & 0xff);
        rdCmd[9] = (unsigned char)(sector & 0xff);
        rdCmd[10] = (unsigned char)((nr_sectors >> 24) & 0xff);
        rdCmd[11] = (unsigned char)((nr_sectors >> 16) & 0xff);
        rdCmd[12] = (unsigned char)((nr_sectors >> 8) & 0xff);
        rdCmd[13] = (unsigned char)(nr_sectors & 0xff);

        io_hdr->interface_id = 'S';
        io_hdr->cmd_len = 16;
        io_hdr->cmdp = rdCmd;
        if ( direction == READ )
                io_hdr->dxfer_direction = SG_DXFER_FROM_DEV;
        else
                io_hdr->dxfer_direction = SG_DXFER_TO_DEV;
        io_hdr->dxfer_len = nr_sectors << 9; /* in bytes */
        io_hdr->dxferp = iov_user;
        io_hdr->iovec_count = len;
        io_hdr->mx_sb_len = SCSI_SENSE_BUFFERSIZE;
        io_hdr->sbp = senseBuff;

	return scsi_dosectors_io(bdev, io_hdr, sector, deadline);
}

static int split_requests(struct block_device *bdev, struct FsIovec *iovec,
			int iovec_count, unsigned long long start_sect,
			monotonic_t *deadline, int rw)
{
    struct FsIovec *curr_iovec;
    unsigned int cb = 0;
    unsigned int byte_count, count, nr_iovec;
    int bytes_transferred, bytes, split_iovec;
    sector_t target_sect, n_sect;
    struct sg_iovec iov_table;

    byte_count = count = 0;
    bytes_transferred = bytes = split_iovec = 0;

    target_sect = start_sect;

    while (count < iovec_count)
    {
            curr_iovec = iovec;
            nr_iovec = 0;
            n_sect = 0;
            byte_count = 0;

            for (; count < iovec_count ; count++) {
                    if (__get_user(cb, &iovec->cb)) {
                            printk( "Fail read iovec %08x\n", (unsigned int)iovec );
                            return -EFAULT;
                    }

                    if((byte_count + cb) > MAX_BYTES){

                        if(byte_count > 0) {
                            break;
                        }
                        else if(byte_count == 0 && cb > MAX_BYTES){
                                /* A single iovec of size > MAX_NRSECTORS */

                                int length;
                                unsigned int *base;
                                void *pb;
                                split_iovec = 1;

                                if (__get_user(pb, &iovec->pb)) {
                                        printk( "Fail read iovec %08x\n", (unsigned int)iovec );
                                        return -EFAULT;
                                }

                                base = (unsigned int *) pb;
                                length = cb;

                                while(length > 0){
                                        iov_table.iov_base = base;

                                        if(length >= MAX_BYTES) {
                                                iov_table.iov_len = MAX_BYTES; 
                                        }
                                        else {
                                                iov_table.iov_len = length;
                                        }

                                        base += MAX_BYTES/4;
                                        length -= MAX_BYTES;

                                        if(iov_table.iov_len % 512){
                                                printk("scsi_dosector: iovec bytes not aligned on sector boundary\n");
                                                if(!(err_count--))
                                                BUG();
                                        }

                                        n_sect = iov_table.iov_len / SECTOR_BYTES;

					bytes = prepare_and_do_scsi_request(bdev, target_sect,
						n_sect, (struct FsIovec *)&iov_table, 1, rw,
						deadline);

                                        if (bytes < 0) {
                                                goto out;
                                        }
                                        else {
                                                bytes_transferred += bytes;
                                        }

                                        target_sect += n_sect;
                                }
                                        iovec++;
                                        continue;
                        }
                    }
                    byte_count += cb;
                    iovec++;
                    nr_iovec++; 
            }

            if(split_iovec){
                    split_iovec = 0;
                    continue;
            }

            if((byte_count % SECTOR_BYTES)){
                    printk("scsi_dosector: iovec bytes not aligned on sector boundary\n");
                    if(!(err_count--))
                    BUG();
            }

            n_sect = byte_count / SECTOR_BYTES;

	    bytes = prepare_and_do_scsi_request(bdev, target_sect, n_sect,
			    curr_iovec, nr_iovec, rw, deadline);

            if (bytes < 0) {
                    break;
            }
            else {
                    bytes_transferred += bytes;
            }

            target_sect += n_sect;
    }

out:
    return bytes_transferred;
}

