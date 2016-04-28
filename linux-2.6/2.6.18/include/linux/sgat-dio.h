#ifndef _SGAT_DIO_H
#define _SGAT_DIO_H
#ifdef __KERNEL__
#include <linux/uio.h> /* for struct iovec */
#else 
#include <sys/uio.h> /* for struct iovec */
#endif /* __KERNEL__ */

///////////////////////////////////////////////////////////////////////////////
//
//Data Definitions
//
//////////////////////////////////////////////////////////////////////////////

typedef struct sgat_dio_hdr
{
    int                 dxfer_dir;
    unsigned short      xferId;
    int                 status;
    int                 timeout;
    unsigned short      iovec_count;
    struct iovec        *iovec;
} sgat_dio_hdr_t;

#ifdef __KERNEL__
#ifndef DIO_DPRINTK
//#define DIO_DPRINTK(x...) printk("%s:",__FUNCTION__); printk(x)
#define DIO_DPRINTK(x...)
#endif
struct iobuf 
{
	int		nr_pages;	/* Pages actually referenced */
	int		array_len;	/* Space in the allocated lists */
	int		length;		/* Number of valid bytes of data */
	struct page **  maplist;
};

typedef struct sgat_dio_req 
{
        sgat_dio_hdr_t          *u_io_hdrp;  /* user io hdr*/
        sgat_dio_hdr_t          *k_io_hdrp;  /* kernel io hdr*/
        unsigned short          k_useg;      /* number of physically contigous segements*/    
        char                    in_use;      /* flag indicating if the dio request is in use*/   
        struct iobuf           *kiobp;      /* iobuf */    
        void                    *sclist;     /* scatter list holding addreses and lengths*/
        int                     sc_elems;    /* number of elements in scatter list*/ 
        int                     iobuf_inuse;/* flag indicating if iobuf is still mapped*/ 
} sgat_dio_req;

struct sgat_dio_fops{
        /*
         * General purpose memory allocation routine. Should be able to handle
         * any device specific DIO memory allocation requests eg., allocating
         * memory chunk from a given slab with device specific alignments.
         */ 
        void *(*sgat_dio_malloc)(int rqSz);
        /*
         * General purpose DIO free routine
         *
         */ 
        void (*sgat_dio_free)(void * buff);
        /*
         * Expands or Allocates memory for DIO scatter list of any given type 
         *
         */ 
        void (*expand_sclist)(struct sgat_dio_fops *fops,sgat_dio_req *dio_req, int sc_elems);
        /*
         * 
         * add_dio_chunk adds an element to DIO scatter list taking kernel virtual address
         * and length of a mapped memory segment as input parameters.
         * add_dio_chunk should be able to handle the device specific DIO (or DMA) requirements 
         * (eg., most of the DMA engines need physical addresses in the list along with length as a 
         * scatter list element).
         */ 
        void * (*add_dio_chunk)(struct sgat_dio_fops *fops, sgat_dio_req * dio_req, 
                                u_long kaddress, int length,void *sclp);
        /*
         * end_dio_req would terminate the DIO scatter list (for eg., by witing a pattern
         * at the end of the scatter-gather list)  
         *
         */ 
        int (*end_dio_req)(struct sgat_dio_fops *fops, sgat_dio_req *dio_req);
        /*
         * sgat_dio_done completes the DIO transaction by waking up any sleeping process or
         * copying status to the user space etc.
         *
         */ 
        void (*sgat_dio_done)( struct sgat_dio_fops *fops, sgat_dio_req *dio_req);
};


///////////////////////////////////////////////////////////////////////////////
//
//API
//
//////////////////////////////////////////////////////////////////////////////
/*
 * sgat_dio_alloc_req allocates DIO request from a given memory area
 * meeting any device specific requirements.
 * sgat_dio_malloc in fops knows which memory area to use.
 */ 
sgat_dio_req *sgat_dio_alloc_req(struct sgat_dio_fops *fops);

/* 
 * sgat_dio_init_req allocates memory for all pointers in dio_req and initializes
 * them accordingly.
 */ 
int sgat_dio_init_req(struct sgat_dio_fops *fops,
                        sgat_dio_req *dio_req,
                        sgat_dio_hdr_t *io_hdrp);

/* Builds scatter-gather DIO list*/
int sgat_dio_build_sg_list(struct sgat_dio_fops *fops,
                           sgat_dio_req *dio_req, int flushDcache);

/* 
 * sgat_dio_free_req frees DIO request
 */ 
void sgat_dio_free_req(struct sgat_dio_fops *fops, struct sgat_dio_req *dio_req);

/*
 *sgat_dio_exit_req free all pointers in DIO req and clears all the status flags
 */ 
void sgat_dio_exit_req(struct sgat_dio_fops *fops, struct sgat_dio_req *dio_req,
                        int flushDcache);

/*
 * Normally one iobuf will be consumed per one iovec entry and allocating iobuf 
 * per iovec is proven to be a heavy weight exercise for various reasons 
 * (frequent icache flushes, allocating excess amount of memory etc).
 * sgat_dio_map_user_iobuf is a kind of extension to map_user_iobuf while utilizing the
 * iobuf efficiently by minimizing cache flushes and allocating memory just on need basis. 
 * All mapped iovec elements' addresses are held by just one 
 * iobuf instead of one iobuf per iovec (user address) entry.
 */ 
int sgat_dio_map_user_iobuf(int rw, struct iobuf *iobuf, 
                             struct iovec *iovec, int iovec_count,int flushDcache);
/*
 * pins down page in the memory
 */
struct page *lock_down_page(struct mm_struct *mm, struct vm_area_struct *vma,
                            unsigned long addr, int write);
/*
 *unlocks pages
 */
void unlock_pages(struct page **page_list);

/*
 * Initializes fops with (following) default DIO operations.
 * All these functions can be overloaded or re-implemented according to the devices or 
 * driver specific requirements.
 * sgat_dio_malloc: Allocates requested number of bytes chunk from GFP_KERNEL area.
 * sgat_dio_free: Frees memory allocated by sgat_dio_malloc.
 * expand_sclist: Assumes struct scatterlist as the type of scatter list for holding DIO 
 *                mapped kernel virtual addresses and corresponding lengths. 
 * add_dio_chunk: Assumes struct scatterlist as the type of scatter list for holding DIO
 *                mapped kernel virtual addresses and corresponding lengths.                       
 * end_dio_req: Doesnt write any termination pattern at the end of the scatter list.
 * sgat_dio_done: Doesnt copy any status to user space or neither does any critical activity.
 */ 
int sgat_dio_get_default_fops(struct sgat_dio_fops *fops);

#endif /* __KERNEL__ */

#endif /*_SGAT_DIO_H*/
