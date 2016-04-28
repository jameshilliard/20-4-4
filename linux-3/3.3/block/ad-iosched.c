/*
 *  Anticipatory Deadline i/o scheduler.
 *  Derived from Deadline i/o scheduler.
 *  Proactively checks if any I/O request is going to miss the deadline.
 *  If so serves that request immediately out of order.
 */
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/blkdev.h>
#include <linux/elevator.h>
#include <linux/bio.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/compiler.h>
#include <linux/rbtree.h>
#include <linux/tivo-monotime.h>
#include <linux/ide-tivo.h>
#include <scsi/scsi.h>

#ifdef CONFIG_TIVO_PLOG
#include <linux/plog.h>
#endif

static int logging = 0;
static int seq = 0;

#define DEBUG_AD_SCHED
#ifdef DEBUG_AD_SCHED
#define DPRINTK(fmt, args...)           if ( logging ) printk(fmt, ## args);      
#else
#define DPRINTK(fmt, args...)
#endif

/*
 * run time data
 */
struct ad_data {

        /*
         * requests are present on both sort_list and fifo_list
         */
        struct list_head fifo_list;
        /*
         * settings that change how the i/o scheduler behaves
	 */
        int seek; /* Disk Avg seek time in usec */
        int rotation;/* Disk Avg Rotation time in usec */
        int transfer; /* Worst DTR in usec */
        int slice; /* time to make one elevator sweep in usec */
        int inter_track_allowance; /* avg adjacent track repair allowance in usec */
        int inter_sector_allowance; /* avg adjacent sector repair allowance in usec */
        int nr_requests;
        int debt;          /* current debt level in usec */
        int peak_debt;     /* highest-seen debt level in usec */
        unsigned long last_run;  /* time in jiffies at which "debt" was last calculated; */
        spinlock_t ad_fifo_lock;
};

/* Deadline estimation helper functions */
static inline int ad_estimate( struct ad_data *dd,
                    /*unsigned long current_sector,*/
                    struct request *req) 
{
        int result;

        result = dd->seek + /* avg seek time */
                dd->rotation + /* avg rotation time */ 
		(dd->transfer * blk_rq_sectors(req))  /* transfer time */;
                //DPRINTK("%s:%d seek %d rot %d xfer %d sec %d res %d\n",
                //dd->transfer, req->nr_sectors, result);
        return result;
}

/*
 * add to fifo
 */
static void
ad_add_request(struct request_queue *q, struct request *rq)
{
        struct ad_data *dd = q->elevator->elevator_data;
        struct list_head *list;

#ifdef CONFIG_TIVO_PLOG
        plog_add_disk_request_to_queue(rq);
#endif

	spin_lock(&dd->ad_fifo_lock);

        /* Requests with deadline value = 0 */
        if ( !rq->deadline ){
                rq->deadline = INFINITE_DEADLINE;
                list_add_tail(&rq->queuelist, &dd->fifo_list);
        }
        else{ /* media requests with valid deadline value */
                list_for_each(list, &dd->fifo_list) {
                        struct request *trq;
                        trq = list_entry(list, struct request, queuelist);
                        if (rq->deadline < trq->deadline)
                                break;
                }
                __list_add(&rq->queuelist, list->prev, list);
        }
        rq->start_time = seq++;
        dd->nr_requests++;
	spin_unlock(&dd->ad_fifo_lock);
}

static void ad_dispatch_sort(struct request *rq, struct list_head *sort_list)
{
        struct list_head *list;

        list_for_each(list, sort_list) {
                struct request *trq;
                trq = list_entry(list, struct request, queuelist);
                if (blk_rq_pos(rq) < blk_rq_pos(trq))
                        break;
        }
        __list_add(&rq->queuelist, list->prev, list);

}

/*
 * ad_dispatch_requests selects the best request according to
 * read/write expire etc
 */
static int ad_dispatch_requests(struct request_queue *q, int force)
{
	struct ad_data *dd = q->elevator->elevator_data;
	struct list_head sort_list;
	struct request *rq, *next_rq;
	int time_left;
	int deadline;
	int est;
	int dispatched = 0;
	int paced = 0;
	unsigned long jNow;
	long jElapsed, payback;

	if (list_empty(&dd->fifo_list)) {
		return 0;
	}

	time_left = dd->slice - dd->inter_track_allowance;
    
	INIT_LIST_HEAD(&sort_list);
	spin_lock(&dd->ad_fifo_lock);

	/*
	  Our previous run left us with some amount of "time debt"
	  (the time for all commands that had been issued, to
	  reasonably be expected to be completed, including the
	  controller->platter time for writeback).  Figure out how
	  long it has been since we last came through here and reduce
	  the debt accordingly.
	*/

	jNow = jiffies;
	jElapsed = jNow - dd->last_run;

	payback = jElapsed * (1000000 / HZ);

	if (payback > dd->debt) {
	    dd->debt = 0;
	} else {
	    dd->debt -= payback;
	}
	
	DPRINTK("*******Batch Start, now %lu, last %lu, payback %ld, debt %d ********\n", jNow, dd->last_run, payback, dd->debt);

	dd->last_run = jNow;

	/*
	* find and add next batch of requests
	*/

	rq = rq_entry_fifo(dd->fifo_list.next);


	while (rq && (time_left > 0) ) {

	    	if (list_is_last(&rq->queuelist, &dd->fifo_list)) {
			next_rq = NULL;
		} else {
			next_rq = (struct request *) rq->queuelist.next;
		}

		DPRINTK(" Examine: request 0x%p, next in queue %p, next_rq %p\n", rq, rq->queuelist.next, next_rq);

		if ( rq->deadline == INFINITE_DEADLINE )
			deadline = 0;
		else
			deadline = (rq->deadline / ((1000 * tivo_counts_per_256usec)>>8)); 
			

		if ( deadline && ( deadline < time_left )) {
			time_left = deadline;
		}

		est = ad_estimate(dd, rq);
		if ( est > time_left && !deadline) {
		    DPRINTK("  Beyond time limit, bail\n");
		    break;
		}

		time_left -= est;
		dd->debt += est;

		/*
		 * take it off fifo
		 */
		rq_fifo_clear(rq);

		DPRINTK(" Adding: %lu: sector = 0x%llx, nr_sectors = 0x%x, deadline = 0x%llx, est = %d\n", rq->start_time, 
			blk_rq_pos(rq), blk_rq_sectors(rq), rq->deadline, est);

		/*
		 * Add to elevator sort list.  This does sort by elevator order, and
		 * so can place a longer-deadline request prior to a shorter-deadline request.
		 * We may need to revisit the est/time_left calculations up above, to make
		 * sure that this can't cause missed deadlines (I suspect it can... dplatt)
		 */
		ad_dispatch_sort(rq, &sort_list);

		dd->nr_requests--; 

		rq = next_rq;
	}

/*        DPRINTK("\n"); */
	while (!list_empty(&sort_list) ) {
		rq = rq_entry_fifo(sort_list.next);
		rq_fifo_clear(rq);
		/*
		 * move the entry to dispatch queue
		 */
		DPRINTK(" Dispatching: %lu: sector = 0x%llx, nr_sectors = 0x%x, deadline = 0x%llx\n", rq->start_time,
				blk_rq_pos(rq), blk_rq_sectors(rq), rq->deadline);
		elv_dispatch_add_tail(q, rq);
		dispatched ++;
	}

	if (dd->peak_debt < dd->debt) {
	    dd->peak_debt = dd->debt;
	}

	DPRINTK("*******Batch End, dispatched %d%s, debt %u ********\n", dispatched, paced ? " (pacing)" : "", dd->debt);

	spin_unlock(&dd->ad_fifo_lock);

	return (dispatched > 0);
}

static void ad_exit_queue(struct elevator_queue *e)
{
	struct ad_data *dd = e->elevator_data;

	BUG_ON(!list_empty(&dd->fifo_list));

	kfree(dd);
}

/*
 * initialize elevator private data (ad_data).
 */
static void *ad_init_queue(struct request_queue *q)
{
	struct ad_data *dd;

	dd = kmalloc(sizeof(*dd), GFP_KERNEL);
	if (!dd)
		return NULL;

	INIT_LIST_HEAD(&dd->fifo_list);

	/* Worst case seek + rotation and data transfer times from actual data.*/
	dd->seek = 19000; /* 19ms worst case seek (no rotation) */
	dd->rotation = 11111; /* 5400rpm = 90rps = 11111us/rev */
	dd->transfer = (1000000 * 512 /*sector size*/)/(96*1024*1024); /* worst case per sector DTR -- 96MBytes per sec*/
	dd->slice = 350000; /* 350 msec */
	dd->inter_track_allowance = 50000; /* 50 msec */
	dd->inter_sector_allowance = 10000; /* 10 msec */
	dd->nr_requests = 0; 
	dd->debt = 0;  
	dd->peak_debt = 0;
	dd->last_run = jiffies; /* start time from now */
	dd->ad_fifo_lock = __SPIN_LOCK_UNLOCKED(dd->ad_fifo_lock);

	return dd;
}

/*
 * sysfs parts below
 */

static ssize_t
ad_var_show(int var, char *page)
{
	return sprintf(page, "%d\n", var);
}

static ssize_t
ad_var_store(int *var, const char *page, size_t count)
{
	char *p = (char *) page;

	*var = simple_strtol(p, &p, 10);
	return count;
}

#define SHOW_FUNCTION(__FUNC, __VAR)				        \
static ssize_t __FUNC(struct elevator_queue *e, char *page)		\
{									\
	struct ad_data *dd = e->elevator_data;			        \
	int __data = __VAR;						\
	return ad_var_show(__data, (page));			        \
}
SHOW_FUNCTION(ad_seek_show, dd->seek);
SHOW_FUNCTION(ad_rotation_show, dd->rotation);
SHOW_FUNCTION(ad_transfer_show, dd->transfer);
SHOW_FUNCTION(ad_slice_show, dd->slice);
SHOW_FUNCTION(ad_inter_track_allowance_show, dd->inter_track_allowance);
SHOW_FUNCTION(ad_inter_sector_allowance_show, dd->inter_sector_allowance);
SHOW_FUNCTION(ad_logging_show, logging);
SHOW_FUNCTION(ad_debt_show, dd->debt);
SHOW_FUNCTION(ad_peak_debt_show, dd->peak_debt);
SHOW_FUNCTION(ad_last_run_show, dd->last_run);
#undef SHOW_FUNCTION

#define STORE_FUNCTION(__FUNC, __PTR)			                \
static ssize_t __FUNC(struct elevator_queue *e, const char *page, size_t count)	\
{									\
	struct ad_data *dd = e->elevator_data;			        \
	int __data;							\
	int ret = ad_var_store(&__data, (page), count);		        \
	*(__PTR) = __data;					        \
	return ret;							\
}
STORE_FUNCTION(ad_seek_store, &dd->seek);
STORE_FUNCTION(ad_rotation_store, &dd->rotation);
STORE_FUNCTION(ad_transfer_store, &dd->transfer);
STORE_FUNCTION(ad_slice_store, &dd->slice);
STORE_FUNCTION(ad_inter_track_allowance_store, &dd->inter_track_allowance);
STORE_FUNCTION(ad_inter_sector_allowance_store, &dd->inter_sector_allowance);
STORE_FUNCTION(ad_logging_store, &logging);
STORE_FUNCTION(ad_debt_store, &dd->debt);
STORE_FUNCTION(ad_peak_debt_store, &dd->peak_debt);
STORE_FUNCTION(ad_last_run_store, &dd->last_run);
#undef STORE_FUNCTION

#define DD_ATTR(name) \
	__ATTR(name, S_IRUGO|S_IWUSR, ad_##name##_show, \
				      ad_##name##_store)

static struct elv_fs_entry ad_attrs[] = {
	DD_ATTR(seek),
	DD_ATTR(rotation),
	DD_ATTR(transfer),
	DD_ATTR(slice),
	DD_ATTR(inter_track_allowance),
	DD_ATTR(inter_sector_allowance),
	DD_ATTR(logging),
	DD_ATTR(debt),
	DD_ATTR(peak_debt),
	DD_ATTR(last_run),
	__ATTR_NULL
};

static struct elevator_type iosched_deadline = {
	.ops = {
		.elevator_dispatch_fn =		ad_dispatch_requests,
		.elevator_add_req_fn =		ad_add_request,
		.elevator_init_fn =		ad_init_queue,
		.elevator_exit_fn =		ad_exit_queue,
	},

	.elevator_attrs = ad_attrs,
	.elevator_name = "ad",
	.elevator_owner = THIS_MODULE,
};

static int __init ad_init(void)
{
	elv_register(&iosched_deadline);

	return 0;
}

static void __exit ad_exit(void)
{
	elv_unregister(&iosched_deadline);
}

module_init(ad_init);
module_exit(ad_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("anticipatory deadline IO scheduler");
