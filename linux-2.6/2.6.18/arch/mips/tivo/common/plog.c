/**
 * File: plog.c
 * 
 * "Process log" for 2.6 SMP kernel.
 * 
 * Implementation notes:
 * 
 * 1. PLOG_26_XXX is used to commented out not yet ported to 2.6 kernel code.
 * 
 * 2. A spinlock is used to insure multithreading safety.
 * 
 * 3. Pairs of {spin_lock_irqsave() | spin_unlock_irqrestore()} are used even
 *    from ioctl handling code, where {spin_lock_irq() | spin_unlock_irq()}
 *    will be fine as well. Code simplicity, may be?
 * 
 * 4. The plog event header has been changed from 2.4 version.
 * 
 * 5. New plog_syscall_start() and plog_syscall_end() have replaced original
 *    plog_sc_enter() and plog_sc_leave() respectively.
 * 
 * Copyright 2012 TiVo Inc. All Rights Reserved.
 */

#include <linux/sched.h>
#include <linux/unistd.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/proc_fs.h>
#include <linux/plog.h>
#include <linux/interrupt.h>
#include <linux/notifier.h>
#include <linux/tivoconfig.h>
#include <linux/blkdev.h>
#include <linux/tivo-private.h>
#include <linux/tivo-monotime.h>
#include <linux/poll.h>
#include <linux/spinlock.h> 

#include <asm/uaccess.h>
#include <asm/io.h>
#include <asm/semaphore.h>

#include <asm/r4kcache.h>


/*
 * Since our wonderful MIPS cache routines only do writeback invalidate
 * operations, I'll just add this routine that writes back only the desired
 * cache lines.  
 *
 * Most plog entries will be one or two cache lines long so we'll do this in a
 * simple loop.  If we would be doing larger sizes a lot, I would unroll the
 * assembly language cache ops into an instruction array and set up the loop
 * counters so we could jump into the middle of the unrolled loop to cover the
 * desired range.  But that's a bit complicated.
 *
 * The zephyr (BMIPS5000) has an inclusive secondary cache, so we operate on
 * that.  The older core has what can best be described as an inclusive
 * primary cache, so we should use L1 cache operations.
 */

void plog_cache_writeback(unsigned long start, long size);

/* From 7418-5HDM101-RDS pg 5-144: HitWbSc is 0b11011 */
#define HitWbSc	0x1b

void 
plog_cache_writeback(unsigned long start, long size)
{
	long linesz, linemsk;
	unsigned long end;

#if defined(CONFIG_BMIPS5000)
	linesz = cpu_scache_line_size();
#else
	linesz = cpu_dcache_line_size();
#endif

	/*
	 * Set up the loop counters so the address is cache line aligned (do
	 * we really need to do that?) and the length is a multiple of the
	 * cache line size.
	 */

	linemsk = linesz - 1;
	start &= ~linemsk;
	size += linesz;
	end = start + size;

	while (start < end) {
#if defined(CONFIG_BMIPS5000)
		cache_op(HitWbSc, start);
#else
		cache_op(Hit_Writeback_D, start);
#endif
		start += linesz;
	}
	__sync();
}


extern int mem_get_contigmem_region(unsigned int   region, 
                                    unsigned long *start,
                                    unsigned long *size);

// On SMP we need a spinlock to make this multithread safe.
static spinlock_t s_lock = SPIN_LOCK_UNLOCKED;

/**
 * Plog Event Structure:
 *
 * Every plog event begins with a 32-bit word that contains
 * information about the cpu that generated the event, the unique plog
 * event type ID, and the amount of valid data within the plog event.
 * This event is stored slightly differently in the internal plog ring
 * buffer than the way it is returned to user space readers.
 * Internally, the second byte is used to store an XOR'd checksum of
 * the other three bytes.  This checksum is used when the plog event
 * stream synchronization is lost to more accurately reacquire
 * synchronization.  User space code will always see this byte as all
 * zero bits.
 * 
 * Word 0:
 *     _____________
 *    / cpu (1-bit)
 *   |  _____________________________ _______________________________
 *   | | plog type ID (15-bits)      | intrnl chksum | payload size  |
 *   | |                             | (8-bits)      | (8-bits)      |
 *   |3|3|2|2|2|2|2|2|2|2|2|2|1|1|1|1|1|1|1|1|1|1| | | | | | | | | | |
 *   |1|0|9|8|7|6|5|4|3|2|1|0|9|8|7|6|5|4|3|2|1|0|9|8|7|6|5|4|3|2|1|0|
 * 
 * The second 32-bit word contains a timestamp for the plog event.
 * The timestamp is generated from the system clock and the low
 * 32-bits are used.  When interpreting the event stream, these
 * timestamps need to be converted using knowledge of the system clock
 * rate.  (Currently, the 7405 system clock runs at 27Mhz.)
 *  
 * Word 1:
 *    _______________________________________________________________
 *   | system clock timestamp (32-bits)                              |
 *   |                                                               |
 *   |3|3|2|2|2|2|2|2|2|2|2|2|1|1|1|1|1|1|1|1|1|1| | | | | | | | | | |
 *   |1|0|9|8|7|6|5|4|3|2|1|0|9|8|7|6|5|4|3|2|1|0|9|8|7|6|5|4|3|2|1|0|
 * 
 * Word 2-N:
 * 
 *   Event data payload.
 * 
 * NOTE: Plog data events are always padded out to the nearest four
 * byte size.  The valid data size, however, is the sum of the plog
 * header (eight bytes) and event data payload.  That is, the valid
 * data size field may contain a non-multiple of four data value if
 * the data payload was itself not a multiple of four.
 * 
 * When advancing through the stream of plog events, YOU SHOULD ALWAYS
 * ADVANCE BY MULTIPLES OF FOUR BYTES.  This means that readers need
 * to pad out the sizes that they read from each plog event to ensure
 * that they advance properly.  This is a bit annoying, but prevents
 * loss of information when non-multiple of four data payloads are
 * added to events.
 */

// This define controls whether plog is allocated
// as a static array or as a contigmem region.
//
//#define USE_STATIC_PLOG

#ifndef USE_STATIC_PLOG

static int *s_processLog = NULL;
static unsigned long s_processLogSizeBytes = 0;

#define PLOG_LENGTH s_processLogSizeBytes

/**
 * Leave plog disabled until module initialization time.
 * 
 * If you need to debug system bootup with plog, define
 * USE_STATIC_PLOG
 */

static int s_plog_disable = 1;
#else // USE_STATIC_PLOG

static int s_processLog[32768];
#define PLOG_LENGTH sizeof(s_processLog)
static int s_plog_disable = 0;

#endif // !USE_STATIC_PLOG

#define MAX_EVENT   8192

// A 1 in this mask means that the event should not be logged.
static int s_ptypeMask[MAX_EVENT >> 5];

// The size of a "safety zone" between the write pointer and the
// global read pointer.  This zone is to mostly insure that when
// plog_read() tries to read the buffer and re-enables interrupts that
// the place in the buffer that it starts reading from doesn't
// immediately get overwritten by things getting logged.
//
#define PLOG_SAFETY_ZONE       1024

#define PLOG_HEADER_SIZE       8
#define PLOG_CONTIGMEM_REGION  0
#define PLOG_MAX_EVENTSIZE              PLOG_HEADER_SIZE+PLOG_MAX_DATASIZE
#define PLOG_SMALLEST_EVENT_IN_BYTES    PLOG_HEADER_SIZE
#define PLOG_SMALLEST_RATIONAL_READ     PLOG_MAX_EVENTSIZE

// Compute the 'checksum byte' of a plog header by XORing together the
// other three bytes (0xff000000, 0x00ff0000, and 0x0000000ff).  This
// checksum is stored in the otherwise unused second byte of the
// header.  Computing and checking this byte makes the capability more
// robust in the face of lost plog event stream synchronization.
#define PLOG_CHECKSUM(cpu,type,size)   (((cpu << 7) | (type >> 8)) \
                                        ^ (type & 0xff) \
                                        ^ (size & 0xff))

// Compute a four-byte plog event header from the given CPU flag,
// event type, and event size.
#define PLOG_EVENT(cpu,type,size)      ((cpu << 31) \
                                        | (type << 16) \
                                        | (PLOG_CHECKSUM(cpu,type,size) << 8) \
                                        | (size & 0xff))

// Given an internal plog header (including the checksum), generate a
// "user" event header by stripping the checksum byte.
#define PLOG_USER_HEADER(hdr)          ((hdr) & 0xffff00ff)

// Special small event type headers.  NOOP (type 0x00/0) is added at
// the wrap point of the buffer when the next event is too large to
// fit and UNDERRUN (type 0x7f/32767) is added (simulated) in the
// buffer of specific readers when they underrun.
#define PLOG_EVENT_NOOP             PLOG_EVENT(0,0x0000,0x04) // no timestamp!
#define PLOG_EVENT_UNDERRUN         PLOG_EVENT(0,0x7fff,0x08) // incl. timestamp

// Special type (for type safety) for all monotonically increasing
// pointers.  (Wrapping is supported in helper routines below.)
typedef unsigned int monotonic_ptr;

// Struct created for each attached reader.
typedef struct plog_reader {
    monotonic_ptr  read_ptr;
    int            temp_buf[1023];    /* make the struct 4k exactly */
} plog_reader;

/* Global read/write pointers are in bytes and continuously increment */
static volatile monotonic_ptr s_plog_wp = 0;
static volatile monotonic_ptr s_plog_rp = 0;

static volatile unsigned int s_plog_lastChunk = 0;

static wait_queue_head_t plog_wait;

static int plog_open(struct inode * inode, struct file * file)
{
    unsigned int f_flags = file->f_flags & O_ACCMODE;

    if (f_flags == O_RDONLY || f_flags == O_RDWR)
    {
        // Allocate the reader struct...
        plog_reader *s = kmalloc(sizeof(plog_reader), GFP_USER);
        memset(s, 0, sizeof(plog_reader));
        file->private_data = s;

        //printk("[PLOG]: Reader attaching...\n");
    }

    return 0;
}

static int plog_release(struct inode * inode, struct file * file)
{
    unsigned int f_flags = file->f_flags & O_ACCMODE;

    if (f_flags == O_RDONLY || f_flags == O_RDWR)
    {
        // Free the reader struct...
        plog_reader *s = (plog_reader*) file->private_data;
        kfree(s);

        //printk("[PLOG]: Reader detaching...\n");
    }

    return 0;
}

// Convert a monotonic read/write pointer to a byte offset within the
// actual (ring) byte buffer.
static inline int plog_bufoffset(monotonic_ptr p)
{
    return p % PLOG_LENGTH;
}

// Given an event size computed by plog_getsize() below, return true
// iff the size seems valid.
static inline int plog_sizevalid(int size)
{
    return size >= 8 && size <= PLOG_MAX_EVENTSIZE;
}

// Verify the validity of the event header located in the (ring)
// buffer at the location specified by the provided monotonic pointer.
// Return true iff the event header bytes and checksum are consistent.
//
// You should be holding the spin lock!!!
static inline int plog_hdrvalid(monotonic_ptr rp)
{
    int bufByte = plog_bufoffset(rp);
    int bufInt  = bufByte / sizeof(int);
    char b3 = (s_processLog[bufInt] & 0xff000000) >> 24;
    char b2 = (s_processLog[bufInt] & 0x00ff0000) >> 16;
    char b1 = (s_processLog[bufInt] & 0x0000ff00) >>  8;
    char b0 = (s_processLog[bufInt] & 0x000000ff) >>  0;
    return (b3 ^ b2 ^ b0) == b1;
}

// Get the size of the event located in the (ring) buffer at the
// location specified by the provided monotonic pointer.
//
// You should be holding the spin lock!!!
static inline int plog_getsize(monotonic_ptr rp)
{
    int bufByte = plog_bufoffset(rp);
    int bufInt  = bufByte / sizeof(int);
    return s_processLog[bufInt] & 0x00ff;
}

// Get a pointer into the (ring) buffer starting at the location
// specified by the provided monotonic pointer.
//
// You should be holding the spin lock!!!
static inline int* plog_getptr(monotonic_ptr p)
{
    int bufByte = plog_bufoffset(p);
    int bufInt  = bufByte / sizeof(int);
    return s_processLog + bufInt;
}

// Return the number of readable bytes between the given monotonic
// read pointer and the current global monotonic write pointer.
//
// You should be holding the spin lock!!!
static inline unsigned int plog_readable(monotonic_ptr rp)
{
    // Works for wrap case too!  Yeah for unsigned numbers!
    return s_plog_wp - rp;
}

static ssize_t plog_read(struct file *file, char *buf, size_t desired, loff_t *off)
{
    int            result = -EINVAL;

    plog_reader   *s = (plog_reader*) file->private_data;
    monotonic_ptr  file_rp = s->read_ptr;

    unsigned int   total = 0;
    unsigned int   copied;
    unsigned int   events;
    unsigned int   remaining;
    int           *temp_buf;
    int            reported_lost_sync = 0;

    // We throttle readers by requiring them to wait for at least 4k
    // bytes of data to be readable per call (even if they ask for
    // less).  This is just to generally keep the overhead caused by
    // reading plog under control.
    int read_4k_already = 0;
    int keep_copying = 1;

    unsigned long  flags;
    unsigned int   f_flags = file->f_flags & O_ACCMODE;

    if (f_flags != O_RDONLY && f_flags != O_RDWR)
    {
        goto out;
    }

    // The user buffer *must* be word aligned for efficiency (would hate to
    // slow things down significantly as we are trying to profile them).
    if (!buf || desired < 0) 
    {
        goto out;
    }

    if (((int) buf) & 0x3) 
    {
        goto out;
    }

    // We only copy out whole events, so calling with small buffer
    // sizes is /highly/ problematic.  For this reason, we just flat
    // out deny requests with small buffers.  This is for the best,
    // trust me.  :)
    if (desired < PLOG_SMALLEST_RATIONAL_READ) 
    {
        goto out;
    }

    if (!access_ok(VERIFY_WRITE,buf,desired))
    {
        goto out;
    }

more:

    // Reset the temporary buffer pointer and remaining space count.
    temp_buf = s->temp_buf;
    remaining = sizeof(s->temp_buf);
    copied = 0;
    events = 0;

    // Acquire the lock to block new plog events and get to work...
    spin_lock_irqsave(&s_lock, flags);

    // First wait for at least 4k bytes of plog events...
    while (1)
    {
        // See how many bytes are ready to be read by this reader...
        unsigned int readable = plog_readable(file_rp);

        // Did this reader fall behind?
        if (readable > (PLOG_LENGTH - PLOG_SAFETY_ZONE))
        {
            // This reader did fall behind.
            //printk("[PLOG]: detected under-run while reading!!!\n");

            // Jump forward to the global read pointer...
            file_rp = s_plog_rp;

            // ...skip any pad events...
            while (plog_getsize(file_rp) < 8 && file_rp < s_plog_wp)
                file_rp += 4;

            if (remaining >= 8 && file_rp < s_plog_wp)
            {
                // ...and inject an underrun event.
                *temp_buf++ = PLOG_USER_HEADER(PLOG_EVENT_UNDERRUN);
                *temp_buf++ = *(plog_getptr(file_rp+4));
                desired   -= 8;
                remaining -= 8;
                copied    += 8;
            } 

            readable = plog_readable(file_rp);
        }

        // Once we have at least 4k of data ready, get out...
        if (readable >= 4096)
        {
            break;
        }

        // If we don't have 4k this time, but we have previously, bail
        // out and handle anything that is left without looping around
        // anymore...
        if (read_4k_already)
        {
            keep_copying = 0;
            break;
        }

        // If we get here, there wasn't enough data and we haven't
        // even seen 4k of data ready yet.  Drop the lock and wait for
        // some more plog events...
        spin_unlock_irqrestore(&s_lock, flags);

        if (signal_pending(current)) 
        {
            result = -ERESTARTSYS;
            goto out;
        }

        interruptible_sleep_on_timeout(&plog_wait, HZ / 5);

        spin_lock_irqsave(&s_lock, flags);
    }

    read_4k_already = 1;
    events = 0;

    // Copy the readable events from the plog buffer to our temporary
    // buffer...
    while (file_rp < s_plog_wp && desired > 0 && remaining > 0)
    {
        unsigned int eventSize = plog_getsize(file_rp);
        unsigned int paddedSize = PLOG_PAD_EVENT_SIZE(eventSize);
        int *ptr;

        // Skip over filler events...
        if (eventSize == 4)
        {
            file_rp += 4;
            continue;
        }

        // Scan past invalid looking events...
        if (!plog_hdrvalid(file_rp) || !plog_sizevalid(eventSize))
        {
            if (!reported_lost_sync) {
                printk("[PLOG]: invalid event; lost stream sync?!?\n");
                reported_lost_sync = 1;
            }
            file_rp += 4;
            continue;
        }

        // Stop if this event is larger than our remaining room...
        if (desired < paddedSize || remaining < paddedSize) 
        {
            break;
        }

        events++;

        // This event looks good, let's copy it to the temporary
        // buffer...
        ptr = plog_getptr(file_rp);
        file_rp   += paddedSize;
        desired   -= paddedSize;
        remaining -= paddedSize;
        copied    += paddedSize;

        // Copy the header first (stripping checksum byte)...
        *temp_buf++ = PLOG_USER_HEADER(*ptr++);
        paddedSize -= 4;

        while (paddedSize > 0) 
        {
            *temp_buf++ = *ptr++;
            paddedSize -= 4;
        }
    }

    // Drop the lock before copying to user space...
    spin_unlock_irqrestore(&s_lock, flags);

    //printk("[PLOG]: copy %d events, %d bytes\n", events, copied);

    // ...and then copy the data to user space...
    copy_to_user((void *) buf, (void *) s->temp_buf, copied);
    buf += copied;
    total += copied;

    // Ok, we've copied one buffer worth.  Should we try for more?
    if (keep_copying && events > 0 && desired >= PLOG_SMALLEST_EVENT_IN_BYTES)
    {
        // We still want more so loop around and try for another
        // buffer worth of plog events...
        goto more;
    } 

    if (total == 0)
    {
        // Never return zero since we are a blocking read() on a
        // never ending file...
        goto more;
    }

    // We're done!
    result = total;

    // Finally, update the read pointer in the file data structure.
    s->read_ptr = file_rp;

    //printk("[PLOG]: Sent out %d bytes, %d left\n", total, desired);

out:
    return result;
}

// We need to support this function if we want the select() call to behave
// properly on this file.
static unsigned int plog_poll(struct file *file, poll_table * wait)
{
    unsigned int  readable;
    plog_reader   *s = (plog_reader*) file->private_data;
    monotonic_ptr file_rp = s->read_ptr;
    unsigned long flags;

    poll_wait(file, &plog_wait, wait);

    spin_lock_irqsave(&s_lock, flags);
    
    readable = plog_readable(file_rp);

    spin_unlock_irqrestore(&s_lock, flags);

    // If there is 4096 bytes, you can read data.
    if (readable >= 4096) 
    {
        return POLLIN | POLLRDNORM;
    }

    return 0;
}

static ssize_t plog_write(struct file *file, 
                          const char  *buf, 
                          size_t       len,
                          loff_t      *off)
{
    int error = -EPERM;
    char tempbuf[1024];
    const char *s;
    char *d;

    error = -EINVAL;
    if (!buf || len < 0 || len >= 1023) 
    {
        goto out;
    }

    error = 0;
    if (!len) 
    {
        goto out;
    }

    if (!access_ok(VERIFY_READ,buf,len))
    {
        goto out;
    }

    s = buf;
    d = tempbuf;
    error = len;
    while (len > 0) 
    {
        __get_user(*d, s);
        s++;
        d++;
        len--;
    }
    *d = 0;

    // Parse user's request
    // Request should be in form of "[+-]e1[-e2][,...]"
    s = tempbuf;
    while (*s != 0) 
    {
        int add;
        int start, end;
        int i;

        if (*s == '+') 
        {
            s++;
            add = 1;
        } 
        else if (*s == '-') 
        {
            s++;
            add = 0;
        } 
        else if (*s >= '0' && *s <= '9')
        {
            add = 1;
        }
        else 
        {
            error = -EINVAL;
            break;
        }

        start = 0;
        while (*s >= '0' && *s <= '9') 
        {
            start = start*10 + (*s - '0');
            s++;
        }

        if (*s == '-') 
        {
            s++;
            end = 0;
            while (*s >= '0' && *s <= '9') 
            {
                end = end*10 + (*s - '0');
                s++;
            }
        } 
        else 
        {
            end = start;
        }

        if (*s != 0 && *s != ',' && *s != '\n') 
        {
            error = -EINVAL;
            break;
        }

        if (*s == ',') 
        {
            s++;
        }

        if (*s == '\n') 
        {
            s++;
        }

        if (start < 0) 
        {
            start = 0;
        }

        if (end >= MAX_EVENT) 
        {
            end = MAX_EVENT - 1;
        }

        if (end >= start) 
        {
            printk("[PLOG]: %s plog type ids [%d-%d]...\n",
                   add ? "Collect" : "Ignore", start, end);
            for (i = start; i <= end; i++) 
            {
                if (add) 
                {
                    s_ptypeMask[i >> 5] &= ~(1 << (i & 0x1f));
                } 
                else 
                {
                    s_ptypeMask[i >> 5] |= (1 << (i & 0x1f));
                }
            }
        }
    }

out:
    return error;
}

static int plog_assign_region(void)
{
#ifndef USE_STATIC_PLOG
    unsigned long plog_region_start;
    unsigned long plog_size;

    // Only initialize once
    if (s_processLog != NULL) 
    {
        return 0;
    }

    if (mem_get_contigmem_region(PLOG_CONTIGMEM_REGION, 
                                 &plog_region_start, 
                                 &plog_size))
    {
        printk("[PLOG]: failed to assign contigmem region.\n");
        s_plog_disable = 1;
        return -EINVAL;
    }

    if (plog_size > 0) 
    {
        s_processLogSizeBytes = plog_size;
	s_processLog = (int *)plog_region_start;
    }

    printk("[PLOG]: Using contigmem %d, start = 0x%lx, size = %ld\n",
           PLOG_CONTIGMEM_REGION, 
           plog_region_start, 
           plog_size);

    {
        unsigned long *pUlong = (unsigned long *)plog_region_start;
        unsigned long ulongSize = plog_size / sizeof(long);
        unsigned long ulongIndex;
        for (ulongIndex = 0; ulongIndex < ulongSize; ulongIndex++)
        {
            *pUlong++ = ulongIndex;
        }
    }
#endif
    return 0;
}

/* Write to the process log. */
static void _plLogEvent(int type, int dataLength, void *data)
{
    int *d, *s, *d_start;
    unsigned int  eventValid;
    unsigned int  eventPadded;
    unsigned long flags;
    monotonic_t   now_time;
    int wake = 0;

    // Are we rejecting this type of event?
    if ( s_plog_disable )
    {
        return;
    }

    if (type < 0 || type >= MAX_EVENT)
    {
        return;
    }

    if (s_ptypeMask[type >> 5] & (1 << (type & 0x1f)))
    {
        return;
    }

    if (dataLength > PLOG_MAX_DATASIZE) 
    {
        dataLength = PLOG_MAX_DATASIZE;
    }

    /* Allocate space. */
    eventValid = PLOG_HEADER_SIZE + dataLength;
    eventPadded = PLOG_PAD_EVENT_SIZE(eventValid);

    /**
     * Making this code multi-thread safe.
     */

    spin_lock_irqsave(&s_lock, flags);

    tivo_monotonic_get_current(&now_time);

    // Check the global read pointer...
    {
        unsigned int readable = plog_readable(s_plog_rp);
        unsigned int maxBehind = PLOG_LENGTH - PLOG_SAFETY_ZONE;

        if (readable > maxBehind)
        {
            // The global read pointer is falling too far behind, push
            // it forward some...
            int skip = readable - maxBehind;
            while (skip > 0) 
            {
                unsigned int eventSize = plog_getsize(s_plog_rp);
                unsigned int paddedSize = PLOG_PAD_EVENT_SIZE(eventSize);

                // Scan past invalid looking events...
                if (!plog_hdrvalid(s_plog_rp) || !plog_sizevalid(eventSize))
                    eventSize = paddedSize = 4;

                skip      -= paddedSize;
                s_plog_rp += paddedSize;
            }
        }
    }

    // Advance the write pointer if we cannot fit the event right
    // here.
    {
        int offset_wp = plog_bufoffset(s_plog_wp);
        if (PLOG_LENGTH - offset_wp < eventPadded) 
        {
            int *ptr = plog_getptr(s_plog_wp);
            while (offset_wp < PLOG_LENGTH) 
            {
                *ptr++ = PLOG_EVENT_NOOP;
                offset_wp += 4;
                s_plog_wp += 4;
            }
        }
    }

    /* Write log header */
    d_start = d = plog_getptr(s_plog_wp);

    s_plog_wp += eventPadded;

    /**
     * On 7450 smp_processor_id() returns 0 or 1 for SMP,
     * therefore no need to use cpu_number_map().
     */

    {
        int cpu = smp_processor_id();
        *d++ = PLOG_EVENT(cpu,type,eventValid);
        *d++ = (int)tivo_monotonic_retrieve(&now_time);
    }

    s = data;
    while (dataLength > 0) 
    {
        *d++ = *s++;
        dataLength -= 4;
    }

    // Only wake waiting listeners occasionally (every 4K byte
    // boundary crossed).  Also, waking on non-SCE type events can
    // cause deadlocks (why!?) so we only wake on SCE type events.
    {
        int chunk = s_plog_wp / 4096;
        if (chunk != s_plog_lastChunk && type == PLOG_TYPE_SCE)
        {
            s_plog_lastChunk = chunk;
            wake = 1;
        }
    }

    spin_unlock_irqrestore(&s_lock, flags);

    // Flush the event out to DRAM, so it's not stuck in the cache if
    // things just happen to lock up, in which case we might want to
    // look at the process log to find out why...
    plog_cache_writeback((unsigned long)d_start, (long) eventPadded);

    // Wake someone up?
    if (wake)
    {
        wake_up_interruptible(&plog_wait);
    }
}

// This is the exported module interface, there is less checking than user
// because the caller is assumed to know what he's doing...
//
int process_log(int type, int data_len, void *data)
{
    int buffer[PLOG_MAX_DATASIZE / sizeof(int)];

    buffer[0] = current->pid;
    if (data_len > PLOG_MAX_DATASIZE - 4) 
    {
        data_len = PLOG_MAX_DATASIZE - 4;
    }

    memcpy(buffer + 1, data, data_len);

    _plLogEvent(type, data_len + 4, buffer);
    return 0;
}

EXPORT_SYMBOL(process_log);

void _plog_swcpu(struct task_struct *from, struct task_struct *to)
{
    struct _pl_swe entry;

    entry.frompid = from->pid;
    entry.topid = to->pid;
    entry.rt_priority = to->rt_priority;
    entry.policy = to->policy;
    _plLogEvent(PLOG_TYPE_SWE, sizeof(entry), &entry);
}

void _plog_swcpu_end(int pid, int flags)
{
    int buffer[2];
    buffer[0] = pid;
    buffer[1] = flags;
    _plLogEvent(PLOG_TYPE_SWL, sizeof(buffer), buffer);
}

void _plog_rq_add(struct task_struct *tp) 
{
    struct _pl_rq entry;

    entry.pid = (short)tp->pid;
    entry.fromintr = in_interrupt();
    entry.priority = (short)tp->prio;
    entry.rt_priority = (short)tp->rt_priority;
    entry.policy = (short)tp->policy;
    entry.preempt = 0;                                // XXX
    _plLogEvent(PLOG_TYPE_RQ, sizeof(entry), &entry);
}

void _plog_rq_del(struct task_struct *tp) 
{
    struct _pl_drq entry;

    entry.pid = (short)tp->pid;
    entry.priority = (short)tp->prio;
    entry.rt_priority = (short)tp->rt_priority;
    entry.policy = (short)tp->policy;
    _plLogEvent(PLOG_TYPE_DRQ, sizeof(entry), &entry);
}

void _plog_setscheduler(int pid, int sched_pid, short priority, short policy) {
    int buffer[3];

    buffer[0]= pid;
    buffer[1]= sched_pid;
    buffer[2]= (priority << 16) | policy;
    _plLogEvent(PLOG_TYPE_SETSCHED, sizeof(buffer), buffer);
}

void _plog_intr_enter(int num, unsigned long pc)
{
    struct _pl_intre entry;

    entry.irq = num;
    entry.pc = pc;
    _plLogEvent(PLOG_TYPE_INTRE, sizeof(entry), &entry);
}

void _plog_intr_leave(int num)
{
    _plLogEvent(PLOG_TYPE_INTRL, sizeof(num), &num);
}

void _plog_bh_enter(int num)
{
	_plLogEvent(PLOG_TYPE_BHE, sizeof(num), &num);
}

void _plog_bh_leave(int num)
{
    _plLogEvent(PLOG_TYPE_BHL, sizeof(num), &num);
}

void _plog_filemap_page_swap(void *mm, int physPage, int virtPage)
{
    int buffer[3];

    buffer[0] = (int) mm;
    buffer[1] = physPage;
    buffer[2] = virtPage;
    _plLogEvent(PLOG_TYPE_FMS, sizeof(buffer), buffer);
}

void _plog_generic_page_swap(void *mm, int physPage, int virtPage)
{
    int buffer[3];

    buffer[0] = (int) mm;
    buffer[1] = physPage;
    buffer[2] = virtPage;
    _plLogEvent(PLOG_TYPE_GPS, sizeof(buffer), buffer);
}

void _plog_reswap(void *mm, int physPage, int virtPage)
{
    int buffer[3];

    buffer[0] = (int) mm;
    buffer[1] = physPage;
    buffer[2] = virtPage;
    _plLogEvent(PLOG_TYPE_RESWAP, sizeof(buffer), buffer);
}

void _plog_pageout(void *mm, int physPage, int virtPage)
{
    int buffer[3];

    buffer[0] = (int) mm;
    buffer[1] = physPage;
    buffer[2] = virtPage;
    _plLogEvent(PLOG_TYPE_PAGEOUT, sizeof(buffer), buffer);
}

void _plog_diskdispatch(unsigned long start, struct request *req)
{
    int buffer[3];

    buffer[0] = (unsigned int)req;
    buffer[1] = (int)start;
    buffer[2] = (int)req->nr_sectors;

    _plLogEvent(PLOG_TYPE_DISK_DISPATCH, sizeof(buffer), buffer);
}

void _plog_diskstart_sata(struct request *rq)
{
    int buffer[3];

    buffer[0] = (unsigned int)rq;

    /* 
     * 2.6 convention:
     * 	type 0: non-RT
     * 	type 1: RT 
     */
    if ((rq->deadline == 0) || (rq->deadline == INFINITE_DEADLINE))
        buffer[1] = 0;      // type 0: non-RT request
    else
        buffer[1] = 1;      // type 1: RT request

    if (rq->flags & 0x1)
    {
        buffer[2] = 1;  // 1 - WRITE
    }
    else
    {
        buffer[2] = 0;  // 0 - READ
    }

    _plLogEvent(PLOG_TYPE_DISK_START, sizeof(buffer), buffer);
}

void _plog_diskstop_sata(struct request *rq, int write)
{
    int buffer[4];

    buffer[0] = (unsigned int)rq;
    buffer[2] = write;  // READ 0, WRITE 1

    if ((rq->deadline == 0) || (rq->deadline == INFINITE_DEADLINE))
    {
        buffer[1] = 0;  // no deadline
        buffer[3] = 0;
    }
    else
    {
        monotonic_t now_time;
        s64 diff_count;

        buffer[1] = 1;  // real time request

        tivo_monotonic_get_current(&now_time);

        /*
         * Calculate the remaining time in ms, if we missed the deadline
         * the time it was missed by will be shown with the minus sign
         */
        buffer[3] = (((rq->deadline - tivo_monotonic_retrieve(&now_time))/tivo_counts_per_256usec) << 8) / 1000;
    }


    _plLogEvent(PLOG_TYPE_DISK_STOP, sizeof(buffer), buffer);
}

void _plog_queue_congested(int congested, int rw)
{
    int buffer[1];

    buffer[0] = rw;  // 0 - READ, 1 - WRITE

    _plLogEvent((congested == 1) ? PLOG_TYPE_QUEUE_CONGESTED : PLOG_TYPE_QUEUE_UNCONGESTED, sizeof(buffer), buffer);
}

void plog_disk_request_failed_queue_full(int rw)
{
    int buffer[1];

    buffer[0] = rw;  // 0 - READ, 1 - WRITE

    _plLogEvent(PLOG_TYPE_REQ_QUEUING_FAILED, sizeof(buffer), buffer);
}

void plog_disk_request_failed_no_mem(int rw)
{
    int buffer[1];

    buffer[0] = rw;  // 0 - READ, 1 - WRITE

    _plLogEvent(PLOG_TYPE_REQ_ALLOC_FAILED, sizeof(buffer), buffer);
}

void _plog_wake_up_queue(int rw)
{
    int buffer[1];

    buffer[0] = rw;  // 0 - READ, 1 - WRITE

    _plLogEvent(PLOG_TYPE_WAKEUP_REQ_Q, sizeof(buffer), buffer);
}

void _plog_add_disk_request_to_queue(struct request *rq)
{
    int buffer[4];

    buffer[0] = (unsigned int)rq;
    if ((rq->deadline == 0) || (rq->deadline == INFINITE_DEADLINE))
        buffer[1] = 0;      // type 0: non-RT request
    else
        buffer[1] = 1;      // type 1: RT request

    if (rq->flags & 0x1)
    {
        buffer[2] = 1;  // 1 - WRITE
    }
    else
    {
        buffer[2] = 0;  // 0 - READ
    }
    buffer[3] = (int)rq->nr_sectors;

    _plLogEvent(PLOG_TYPE_ADD_REQ_TO_QUEUE, sizeof(buffer), buffer);
}

void plog_disk_credited_writes(int n)
{
    int buffer[1];

    buffer[0] = n;  // number of completed writes added to pending credits

    _plLogEvent(PLOG_TYPE_WRITE_CREDITS_PENDED, sizeof(buffer), buffer);
}

void plog_disk_repaid_credits(int n)
{
    int buffer[1];

    buffer[0] = n;  // number of completed writes paid back to congestion management

    _plLogEvent(PLOG_TYPE_WRITE_CREDITS_REPAID, sizeof(buffer), buffer);
}



#if 0 
 /*
  *XXX: below _functions are replaced with _functions_sata
  *      and will be deleted soon ...
  */
#ifdef PLOG_26_XXX /**
                    * 'tivo' is not defined for 2.6
                    */

void _plog_diskstart(struct request *rq)
{
    int buffer[1];

    if (rq->tivo == NULL) 
    {
        buffer[0] = 0;      // type 0: non-TiVo request
    } 
    else if (tivo_monotonic_is_null(&rq->tivo->deadline)) 
    {
        buffer[0] = 2;      // type 2: TiVo non-RT request
    } 
    else 
    {
        buffer[0] = 1;      // type 1: TiVo RT request
    }

    _plLogEvent(PLOG_TYPE_DISK_START, sizeof(buffer), buffer);
}

void _plog_diskstop(struct request *rq)
{
    int buffer[3];
    struct idetivo_request *trq = rq->tivo;

    buffer[0] = rq->nr_sectors;
    if (trq != NULL && rq->nr_sectors > trq->max_nrsectors) 
    {
        buffer[0] = trq->max_nrsectors;
    }

    buffer[1] = rq->cmd == READ ? 0 : 1;

    if (trq == NULL || tivo_monotonic_is_null(&trq->deadline)) 
    {
        buffer[2] = 0;  // no deadline
    } 
    else 
    {
        monotonic_t now_time;
        s64 diff_count;

        tivo_monotonic_get_current(&now_time);

        diff_count = (s64)(tivo_monotonic_retrieve(&trq->deadline) -
                           tivo_monotonic_retrieve(&now_time));

        // clamp to min/max s32
        if (diff_count > (s64)0x000000007fffffffLL) 
        {
            buffer[2] = 0x7fffffff;
        } 
        else if (diff_count < (s64)(0xffffffff80000000LL)) 
        {
            buffer[2] = 0x80000000;
        } 
        else 
        {
            buffer[2] = (int)diff_count;
        }
    }

    _plLogEvent(PLOG_TYPE_DISK_STOP, sizeof(buffer), buffer);
}

#endif // PLOG_26_XXX
#endif

void _plog_generic_event0(int event)
{
    _plLogEvent(event, 0, NULL);
}

EXPORT_SYMBOL(_plog_generic_event0);

void _plog_generic_event(int event, int somedata)
{
    int buffer[2];
    if (current == NULL) 
    {
        buffer[0] = -1;
    } 
    else 
    {
        buffer[0] = current->pid;
    }
    buffer[1] = somedata;
    _plLogEvent(event, sizeof(buffer), buffer);
}

EXPORT_SYMBOL(_plog_generic_event);

void _plog_generic_event2(int event, int data1, int data2)
{
    int buffer[3];
    if (current == NULL) 
    {
        buffer[0] = -1;
    } 
    else 
    {
        buffer[0] = current->pid;
    }
    buffer[1] = data1;
    buffer[2] = data2;
    _plLogEvent(event, sizeof(buffer), buffer);
}

EXPORT_SYMBOL(_plog_generic_event2);

void _plog_generic_event3(int event, int data1, int data2, int data3)
{
    int buffer[4];
    if (current == NULL) 
    {
        buffer[0] = -1;
    } 
    else 
    {
        buffer[0] = current->pid;
    }
    buffer[1] = data1;
    buffer[2] = data2;
    buffer[3] = data3;
    _plLogEvent(event, sizeof(buffer), buffer);
}

EXPORT_SYMBOL(_plog_generic_event3);

void _plog_generic_event4(int event, int data1, int data2, int data3, int data4)
{
    int buffer[5];
    if (current == NULL) {
        buffer[0] = -1;
    } else {
        buffer[0] = current->pid;
    }
    buffer[1] = data1;
    buffer[2] = data2;
    buffer[3] = data3;
    buffer[4] = data4;
    _plLogEvent(event, sizeof(buffer), buffer);
}

EXPORT_SYMBOL(_plog_generic_event4);

void _plog_ioctl(unsigned int fd, unsigned int cmd, unsigned long arg)
{
    struct _pl_ioctl entry;

    entry.pid = current->pid;
    entry.fd = fd;
    entry.cmd = cmd;
    entry.arg = arg;
    _plLogEvent(PLOG_TYPE_IOCTL, sizeof(entry), &entry);
}

#ifdef PLOG_26_XXX  /**
                     * 'rb_node_t' is not defined for 2.6 --> 
                     * plog_walk_vma_tree() is not build  -->
                     * plog_dump_vma() is not called.
                     */

static void plog_dump_vma(struct mm_struct *mm, struct vm_area_struct *vma)
{
    unsigned long vaddr;
    unsigned long vma_end;
    pgd_t *pgdir;
    int plogBuffer[4];

    plogBuffer[0] = (int) mm;

    // For each VMA...
    vaddr = vma->vm_start;
    vma_end = vma->vm_end;
    pgdir = pgd_offset(mm, vaddr);
    while (vaddr < vma_end) 
    {
        // Iterate over all the PGD's...
        unsigned long pgd_end;
        pmd_t *pmd;

        // On mips ... this function is just pmd = pgdir...
        // The pmd and pgd levels are the *same*.
        pmd = pmd_offset(pgdir, vaddr);
        pgd_end = (vaddr + PGDIR_SIZE) & PGDIR_MASK;
        if (pgd_end > vma_end) 
        {
            pgd_end = vma_end;
        }

        // Iterate over all the PMDs in this PGD (there is only ever one
        // since we are running with only two PTE levels)
        while (vaddr < pgd_end) 
        {
            unsigned long pmd_end;

            pmd_end = (vaddr + PMD_SIZE) & PMD_MASK;
            if (pmd_end > pgd_end) 
            {
                pmd_end = pgd_end;
            }

            if (pmd_none(*pmd)) 
            {
                vaddr = pmd_end;
            } 
            else 
            {
                pte_t *pte;

                pte = pte_offset(pmd, vaddr);

                // Iterate over the PTEs in the PMD.
                while (vaddr < pmd_end) 
                {
                    if (pte_present(*pte)) 
                    {
                        struct page *page = pte_page(*pte);
                        plogBuffer[1] = pte_val(*pte);
                        plogBuffer[2] = (int) vaddr;
                        plogBuffer[3] = page->flags;
                        _plLogEvent(PLOG_TYPE_PTE_PRESENT, sizeof(plogBuffer), 
                                plogBuffer);
                    }
                    vaddr += PAGE_SIZE;
                    pte++;
                }
            }
            pmd++;
        }
        pgdir++;
    }
}

static void plog_walk_vma_tree(struct mm_struct *mm, rb_node_t *pRbNode)
{
    if (pRbNode == NULL) return;
    plog_walk_vma_tree(mm, pRbNode->rb_left);
    plog_dump_vma(mm, rb_entry(pRbNode, struct vm_area_struct, vm_rb));
    plog_walk_vma_tree(mm, pRbNode->rb_right);
}

#endif // PLOG_26_XXX

static int plog_do_ioctl(struct inode *inode, 
                         struct file  *file,
                         unsigned int  cmd, 
                         unsigned long arg)
{
    int rc = -EINVAL;

    switch(cmd) {

    case PLOG_IOCTL_DUMP_MM:
    {
#ifdef PLOG_26_XXX
        int pid = arg;
        struct task_struct *p;

        rc = 0;
        read_lock(&tasklist_lock);

        p = find_task_by_pid(pid);
        if (p != NULL) 
        {
            struct mm_struct *mm = p->mm;
            if (mm != NULL) 
            {
                atomic_inc(&mm->mm_users);
                spin_lock(&mm->page_table_lock);
        
                plog_walk_vma_tree(mm, mm->mm_rb.rb_node);
        
                spin_unlock(&mm->page_table_lock);
                mmput(mm);
            }
        } 
        else 
        {
            rc = -ESRCH;
        }
        
        read_unlock(&tasklist_lock);
#else   
        printk("[PLOG]: PLOG_IOCTL_DUMP_MM ioctl is not ported to 2.6\n");
#endif      
    }
    break;

    case PLOG_IOCTL_PID_MMS:
    {
        struct task_struct *p;
        read_lock(&tasklist_lock);
        for_each_process(p) 
        {
            // Kernel tasks have no mm.  Don't log them.
            if (p->mm != NULL) 
            {
                int plogBuffer[2];
                plogBuffer[0] = p->pid;
                plogBuffer[1] = (int) p->mm;
                _plLogEvent(PLOG_TYPE_PID_MM, 
                            sizeof(plogBuffer), 
                            plogBuffer);
            }
        }
        read_unlock(&tasklist_lock);
        rc = 0;
    }
    break;

    case PLOG_LOG_EVENT:
    {
        struct descPlogEvent plogEventDesc;
        
        if (copy_from_user(&plogEventDesc, (void *)arg, sizeof(plogEventDesc)))
        {
            printk("[PLOG]: %s(): copy_from_user() failed?\n", 
                   __FUNCTION__);
        }
        else
        {
           
            rc = process_log(plogEventDesc.type, plogEventDesc.len, (void*)plogEventDesc.data);

            // debug debug
            // printk("[PLOG]: got PLOG_LOG_EVENT type = %d len = %d, data = %s\n", 
            //        plogEventDesc.type,
            //        plogEventDesc.len,
            //        plogEventDesc.data);
        }
    }
    break;

    case PLOG_LOG_GET_SIZE:
    {
        if (copy_to_user((void *)arg,
                         (void *)&s_processLogSizeBytes, 
                         sizeof(unsigned long)))
        {
            printk("[PLOG]: %s(): copy_to_user() failed?\n", 
                   __FUNCTION__);
        }
        else
        {
            rc = 0;
        }
    }
    break;

    case PLOG_CONTROL_TYPE:
    {
        rc = 0;
        // allow only meaningful combination of flags.
        if ((arg & FLAG_CLEAR) && !(arg & FLAG_NOCLEAR))
            _plog_clear();
        else if ((arg & FLAG_NOCLEAR) && !(arg & FLAG_CLEAR))
            {} //  do nothing
	else
            return -EINVAL;

        if ((arg & FLAG_ENABLE) && 
             !((arg & FLAG_DISABLE) | (arg & FLAG_NOCHANGE)))
            _plog_enable(1);
        else if ((arg & FLAG_DISABLE) && 
                  !((arg & FLAG_ENABLE) | (arg & FLAG_NOCHANGE)))
            _plog_enable(0);
        else if ((arg & FLAG_NOCHANGE) && 
                  !((arg & FLAG_DISABLE) | (arg & FLAG_ENABLE)))
            {} // do nothing
        else
            rc = -EINVAL;
    }
    break;
    default:              
        printk("[PLOG]: %s() got unexpeced cmd = %d\n", __FUNCTION__, cmd);
        break;
    }

    return rc;
}

/**
 * Support for /proc/plog interface.
 */
static struct file_operations proc_plog_operations = {
    read:    plog_read,
    write:   plog_write,
    poll:    plog_poll,
    ioctl:   plog_do_ioctl,
    open:    plog_open,
    release: plog_release,
};

// We usually do a 1 second delay on panic, then reboot.  If ints are
// enabled at the time, that can fill up the process log with interrupt
// events, when we're probably more interested in what caused the panic...
//
static int plog_panic_event(struct notifier_block *self,
                            unsigned long          event,
                            void                  *ptr)
{
    s_plog_disable = 1;
    printk(KERN_EMERG "PLOG kernel buffer %p size %lx\n", 
	s_processLog, PLOG_LENGTH);
    return NOTIFY_DONE;
}

/**
 * DO NOT USE THIS FUNCTION unless it's wednesday.
 * No, but seriously, it totally destroys plog, but it
 * gets events exactly as they happened at the insertion
 * point.  Which is what we need for panic logger, but
 * shouldn't be used anywhere else.
 */

void plog_destructive_liforead(int *buf, int len)
{
    /* Figure out where we should start */
    s_plog_disable = 1;
    if (s_processLog != NULL) 
    {
        // Back up the requested number of bytes.
        int bytes = len * sizeof(int);
        monotonic_ptr p = s_plog_wp - bytes;
		s_plog_wp = p;

        while (len--) 
        {
            *buf++ = *(plog_getptr(p));
            p += sizeof(int);
        }
    }
}

static struct notifier_block plog_panic_block = {
    plog_panic_event,
    NULL,
    0
};

void _plog_clear(void)
{
    unsigned long flags;
    spin_lock_irqsave(&s_lock, flags);
    s_plog_wp = s_plog_rp = 0;
    spin_unlock_irqrestore(&s_lock, flags);
}

void _plog_enable(int enable)
{
    if (enable)
        s_plog_disable = 0;
    else
        s_plog_disable = 1;
}

static int __init plog_init(void)
{
    struct proc_dir_entry *entry;

    // Assign the contigmem region for plog
    if (plog_assign_region()) 
    {
        return -ENOMEM;
    }

    s_plog_disable = 0;

    init_waitqueue_head(&plog_wait);

    entry = create_proc_entry("plog", S_IFREG | S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH, 0);
    if (entry) 
    {
        entry->nlink = 1;
        entry->proc_fops = &proc_plog_operations;

    }

    atomic_notifier_chain_register(&panic_notifier_list, &plog_panic_block);

    return 0;
}

/**
 * Assembly is passing 3 args to us in a0, a1, and a2.
 * We chose to log just two first ones.
 */
void plog_syscall_start(int syscall_num, long arg0, long arg1, long nargs)
{
    struct _pl_sce entry;
    int    entry_size = sizeof(entry);
    entry.pid = current->pid;
    entry.scnum = syscall_num;
    entry.arg0 = arg0;
    entry.arg1 = arg1;
    BUG_ON(nargs < 0);
    if (nargs < 2)
    {
        entry_size -= (sizeof(long) << (1 - nargs));
    }
    _plLogEvent(PLOG_TYPE_SCE, entry_size, &entry);
}

void plog_syscall_end(int syscall_num, int rv)
{
    struct _pl_scl entry;
    entry.pid = current->pid;
    entry.scnum = syscall_num;
    entry.rv = rv;
    _plLogEvent(PLOG_TYPE_SCL, sizeof(entry), &entry);
}

void _plog_pagefault_ent(unsigned long addr, unsigned long pc, unsigned long ExcCode)
{
	struct _pl_pfl entry;
	entry.pid = current->pid;
	entry.addr = addr;
	entry.pc = pc;
	entry.preempt_count = preempt_count(); 
	entry.ExcCode = ExcCode;
	_plLogEvent(PLOG_TYPE_PFE, sizeof(entry), &entry);
}

void _plog_flush_cache(int num, unsigned long ra)
{
	struct _pl_flush_c entry;
	entry.num = num;
	entry.ra = ra;	
	_plLogEvent(PLOG_TYPE_FLUSH_CACHE, sizeof(entry), &entry);
}

void _plog_dma_coh(int num, unsigned long addr, unsigned long size, unsigned long ra)
{
	struct _pl_dma_c entry;
	entry.num = num;
	entry.addr = addr;
	entry.size = size;
	entry.ra = ra;
	_plLogEvent(PLOG_TYPE_DMA_CACHE, sizeof(entry), &entry);
}

void _plog_pgscan_start(int order)
{
	_plLogEvent(PLOG_TYPE_PAGESCAN_START, sizeof(order), &order);
}

void _plog_pgscan_end(unsigned long total_scanned, unsigned long nr_reclaimed, struct pl_node_data n_entry)
{
	struct _pl_pgscan entry;
	entry.total_scanned = total_scanned;
	entry.nr_reclaimed = nr_reclaimed;
	entry.free_pages = n_entry.free_pages;
	entry.nr_active = n_entry.nr_active;
	entry.nr_inactive = n_entry.nr_inactive;
	_plLogEvent(PLOG_TYPE_PAGESCAN_END, sizeof(entry), &entry);
}

void _plog_pgalloc(unsigned long pfn, unsigned long gfp_mask, unsigned int order)
{
	struct _pl_pgalloc entry;
	entry.pfn = pfn;
	entry.gfp_mask = gfp_mask;
	entry.order = order;
	_plLogEvent(PLOG_TYPE_PAGE_ALLOC, sizeof(entry), &entry);
}

void _plog_pgfree(unsigned long pfn, unsigned int order)
{
	struct _pl_pgfree entry;
	entry.pfn = pfn;
	entry.order = order;
	_plLogEvent(PLOG_TYPE_PAGE_FREE, sizeof(entry), &entry);
}

void _plog_unaligned_memory(void *addr, void *pc)
{
	struct _pl_unaligned entry;
    if (in_interrupt()) {
        entry.pid = 0;
    }
    else {
        entry.pid = current->pid;
    }
	entry.addr = (unsigned long) addr;
	entry.pc = (unsigned long) pc;
	_plLogEvent(PLOG_TYPE_UNALIGNED_MEMORY_ACCESS, sizeof(entry), &entry);
}

module_init(plog_init);
