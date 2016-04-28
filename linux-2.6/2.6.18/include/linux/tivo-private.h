#ifndef _LINUX_TIVO_PRIVATE_H
#define _LINUX_TIVO_PRIVATE_H

#include <linux/ide-tivo.h>
#include <linux/tivo-scramble.h>
#include <linux/tivo-monotime.h>

#define IDETIVO_PROFILE 

#define SECTOR_BYTES 512
#define MAX_NRSECTORS 256

#define IDE_TIVO_INFO		(&HWIF(drive)->tivo)
#define IDE_TIVO_DRIVE_INFO     (&drive->tivo)

extern const int idetivo_version_supported;

/*
 * Special request format for TiVo volumes
 */

enum idetivo_request_status { IDETIVO_FREE, IDETIVO_DONE, IDETIVO_BUSY,
			      IDETIVO_ERROR, IDETIVO_INPROGRESS }; 

struct idetivo_request {
    enum idetivo_request_status status; /* { free, done, busy } */
    unsigned int max_nrsectors; /* man number of sector per IDE transfer */
    unsigned int num_chunks; /* number of max_nrsectors sector chunks */
    unsigned int partition_flags; /* flags from partition being transferred */

    unsigned int max_size; /* size of page_list + driver private area */
    struct page **page_list;
    void *drv_priv;

    struct ide_drive_s *drive;
    int drive_status;   /* Drive status upon completion */
    int progress; /* total progress in bytes */
    long streamId; /* ATA-7 stream ID */
    unsigned long partition_start; /* start sector of partition */

    monotonic_t deadline;
    struct request *req; /* standard blk request associated with this one */
    struct idetivo_request *next; /* next in free or deadline-ordered list */
    struct idetivo_request *prev; /* previous in list */
    struct io_scramble scramble; /* scrambler keying */

#ifdef IDETIVO_PROFILE
    unsigned long iostart; /* time in jiffies of I/O start */
    int num_interrupts; 
#endif
#ifdef CONFIG_TGC_IDE_WAR
	int num_pages;
#endif
};

#define DISK_PERFORMANCE_SIZE 64
#define DISK_PERF_LOOKUP(perf, sector) (&((perf)[((sector) >> 18)]))
#define REQUEST_POOL_SIZE 32

struct disk_performance {
    int seek;
    int transfer; 
};

enum media_state {
   STATE_UNKNOWN, STATE_KNOWN, STATE_MEDIA, STATE_NOMEDIA, STATE_FLUSHED
};

/*
 * It is the resposibility of the low-level IDE driver to fill in the
 * version field of the TiVo hwif info, indicating that it provides
 * support for processing TiVo requests.  This version requires that
 * the low-level driver:
 *  - implement DMA for all drives that are issued TiVo requests
 *  - implement ll_dma_ops for that DMA interface, and set sgops appropriately
 *  - properly handle TiVo requests for DMA request processing and interrupt
 *    handling, including restart of requests > MAX_NRSECTORS sectors
 *  - update head position in TiVo drive info after every DMA operation,
 *    whether a TiVo request or normal request
 */
#define IDETIVO_DRIVER_VERSION 0x00020002

enum drive_family {
   FAMILY_UNKNOWN, FAMILY_QUANTUM, FAMILY_STREAMWEAVER1, FAMILY_UNSUPPORTED
};

/*
 * Low-level DMA preprocessing operations and other TiVo extensions
 * for the IDE controller
 */

struct ll_dma_ops {
    unsigned int (*size_priv)(int dma_chunks, int block_chunks);
    void (*init_drvpriv)(struct idetivo_request *trq, int reading);
    int (*add_dmachunk)(struct idetivo_request *trq, unsigned long k_addr,
                        unsigned long u_addr, int size, int reading);
    void (*set_cmd_hook)(void (*handler)(struct ide_drive_s *, unsigned char *));
#ifdef CONFIG_TGC_IDE_WAR
    void (*fini_drvpriv)(struct idetivo_request *trq);
    int  (*add_blkchunk)(struct idetivo_request *trq, int chunks, int sectors);
    int  (*end_blkchunk)(struct idetivo_request *trq, int chunks, int sectors);
#else
    void (*add_blkchunk)(struct idetivo_request *trq, int chunks, int sectors);
    void (*end_blkchunk)(struct idetivo_request *trq, int chunks, int sectors);
#endif
};

/*
 * Private structure for managing TiVo I/O requests.  This portion
 * is on a per-hwif basis.
 */

typedef struct idetivo_hwif_info {

    unsigned long int version;
   
    struct idetivo_request *request_head_rt; /* head of all deadline ordered
                                                lists */
    struct ll_dma_ops *sgops;

#ifdef IDETIVO_PROFILE
    /* This is just for performance monitoring... */
    int total_requests;
    unsigned long long total_jiffies;
    unsigned long long total_bytes;
    int elev;
    int elev_rt;
    int rt_only; 
#endif

} idetivo_hwif_info;

/*
 * Create per-drive private-state information, in addition to the
 * per-hardware-interface information in idetivo_hwif_info.
 */
  
typedef struct idetivo_drive_info {
    enum drive_family family;

    /* 
     * State-switching must be done on a per-drive basis - state and
     * config array in idetivo_driver_info fields are no longer used.
     */
    enum media_state state;
    char config[512];

    int back_to_back_writes;
    int back_to_back_limit;

    unsigned long head_pos;

} idetivo_drive_info;

#endif
