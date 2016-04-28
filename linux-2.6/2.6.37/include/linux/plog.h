/*
 * This file contains information about how to build and interpret the process
 * log.
 *
 * Copyright (C) 2012 TiVo, Inc. All Rights Reserved.
 */

#ifndef __LINUX_PLOG_H
#define __LINUX_PLOG_H

#define PLOG_MAX_DATASIZE   100
                                                                              
// You can issue an ioctl to the /proc/plog file to ask it to dump the mm for a
// specified process.  The ioctl number should be as shown here ... and the
// second argument should the the pid whose mm you want to see.
#define PLOG_IOCTL_DUMP_MM  1

// You issue this ioctl (no args) to request that a pid to mm mapping be dumped
// to the plog.
#define PLOG_IOCTL_PID_MMS  2

// Note: PLOG_DIFF() is depricated.  Do not assume a hardcoded relation
// between time and reported cycle counts, calibrate instead.

// Given the actual size (header+payload) for an event, compute the
// four byte padded event size.
#define PLOG_PAD_EVENT_SIZE(s)   ((s + 3) & ~0x3)

// Plog type of all 1s is special and signifies that the reader fell
// behind in the ring buffer, experiencing an under-run.
#define PLOG_TYPE_UNDERRUN 32767
// "Plog buffer under-run!"

#define PLOG_TYPE_SCE	1
// "System call start"
//
// Note: the real size of plog entry will be variable, see plog_syscall_start().
//
struct _pl_sce {
    short pid;
    short scnum;
    long  arg0;
    long  arg1;
};

#define PLOG_TYPE_SCL	2
// "syscall finish: pid=%d, rv=%d"
struct _pl_scl {
    short pid;
    short scnum;
    int   rv;
};

#define PLOG_TYPE_RQ	3
// "[nopid] Process added to run queue"
struct _pl_rq {
    short pid;
    short priority;
    short rt_priority;
    short policy;
    short pad; // it's happier if we're 4-byte aligned
    char  fromintr;
    char  preempt;
};

#define PLOG_TYPE_INTRE	4
// "[nopid] interrupt: inum=%d pc=0x%08x"
struct _pl_intre {
    int           irq;
    unsigned long pc;
};

#define PLOG_TYPE_INTRL	5
// "[nopid] Interrupt end: inum=%d"

#define PLOG_TYPE_BHE	6
// "[nopid] Bottom half start: %d"

#define PLOG_TYPE_BHL	7
// "[nopid] Bottom half end: %d"

// We can't really measure reads versus writes since many (most?) disks report
// success quickly for write requests even though the data hasn't been written
// yet (it is in the disk buffer still).  After such a write, the next read
// request takes a long time as the write has to complete first.  So, in fact,
// our writes make the reads take longer.  Therefore, trying to distinguish
// time spent doing a read from time spent doing a write is impossible, so we
// don't bother trying.
#define PLOG_TYPE_DISK_START    8
// "[nopid] Disk: 0x%08x (RT:%d,WR:%d) I/O started"

#define PLOG_TYPE_DISK_STOP     9
// "[nopid] Disk: 0x%08x (RT:%d,WR:%d) I/O finished (%dms remaining)"

#define PLOG_TYPE_DISK_FLUSH    10
// "Disk flush started in preparation for reading or writing"
// PLOG_TYPE_DISK_DISPATCH can be found below...

#define PLOG_TYPE_SWE	11
// "Process switch start"
struct _pl_swe {
    short frompid;
    short topid;
    short rt_priority;
    short policy;
};

#define PLOG_TYPE_SWL 12
// "[nopid] Process switch end, prev pid = %d flags = 0x%x"

#define PLOG_TYPE_SPS 13
// "Shared page at 0x%08x swapped out"

// A page that has a mapping is swapped out.  It may have simply been written
// out to swap ... or if the mapping is to a rw memmapped file, it may have
// been written to the file.
// Memory of this type includes shared memory, initialized data segments and
// memmapped files.
#define PLOG_TYPE_FMS 14
// "[nopid] mm 0x%08x: Mapped page ot 0x%08x (vaddr=0x%08x) swapped out"

// A heap or swap page was swapped out.
#define PLOG_TYPE_GPS 15
// "[nopid] mm 0x%08x: Data page at 0x%08x (vaddr=0x%08x) swapped out"

// This event can be for data pages already saved in swap or text pages backed
// by the executable.
#define PLOG_TYPE_RESWAP 16
// "[nopid] mm 0x%08x: Page at 0x%08x (vaddr=0x%08x) tossed out"

// I don't think this event ever happens anymore.  We always get the
// PLOG_TYPE_RESWAP events now.
#define PLOG_TYPE_PAGEOUT 17
// "[nopid] mm 0x%08x: Memory at 0x%08x (vaddr=0x%08x) tossed out"

// Obsolete ... the PLOG_TYPE_PAGEIN_DONE event is all we use now.
#define PLOG_TYPE_PAGEIN 18
// "Memory at vaddr 0x%08x paged in"

#define PLOG_TYPE_DEMAND_ZERO 19
// "Memory at vaddr 0x%08x demand zeroed"

#define PLOG_TYPE_PAGE_DIRTY 20
// "PTE 0x%08x being marked dirty"

#define PLOG_TYPE_NO_SWAP_PAGE 21
// "No swap page available to swap out pid %d, vaddr 0x%08x"

#define PLOG_TYPE_SHARED_PAGE_IGNORED 22
// "Shared page for pid %d, vaddr 0x%08x multiply referenced, not paged"

#define PLOG_TYPE_DIRTY_MULTIPLE_NO_SWAP 23
// "Page for pid %d, vaddr 0x%08x not swapped out -- it's being DMAed"

#define PLOG_TYPE_I2C 24
// "I2C event type %d, user %d"

#define PLOG_TYPE_DRQ 25
// "[nopid] marked unrunnable"
struct _pl_drq {
    short pid;
    short priority;
    short rt_priority;
    short policy;
};

// Obsolete ... the PLOG_TYPE_WP_PAGE_DONE event is all we use now.
#define PLOG_TYPE_WP_PAGE 26
// "Memory at vaddr 0x%08x needs copy-on-write"

#define PLOG_TYPE_WP_PAGE_DONE 27
// "mm 0x%08x, pte 0x%08x, vaddr 0x%08x copied-on-write (page flags: 0x%08x)"

#define PLOG_TYPE_EXIT 28
// "Exiting with code 0x%x"

#define PLOG_TYPE_PAGEIN_DONE 29
// "mm 0x%08x, pte 0x%08x, vaddr 0x%08x now present (page flags: 0x%08x)"

#define PLOG_TYPE_AUDIO_LEVEL   30
// "Audio Level: Fifo levels at %d %d %d"
struct _pl_avfifo {
    int audioLevel;
    int videoLevel;
};

#define PLOG_TYPE_VIDEO_LEVEL   31
// "Video Level: Fifo levels at %d %d %d"

#define PLOG_TYPE_LOCK_BT 32
// "Spinlock at addr %08X took %d times.  %08x %08x %08x"

#define PLOG_TYPE_IOCTL 33
// "ioctl: pid=%d, fd=%d, cmd=%d, arg=0x%lx"
struct _pl_ioctl {
    int pid;
    int fd;
    int cmd;
    unsigned long arg;
};

#define PLOG_TYPE_WRITE_STATS 34
// "read stats: %ld calls, %ld writes (%ld, %ld), time %ld (%ld, %ld)"

// PLOG_TYPE 35 was once PLOG_TYPE_EVENT_SEND
#define PLOG_TYPE_MM_CLEARED 35
// "mm 0x%08x cleared out"

#define PLOG_TYPE_DISK_DISPATCH 36
// "[nopid] Disk: 0x%08x MFS I/O (LBA=%d, sectors=%d)"

#define PLOG_TYPE_SETSCHED 37
// "setscheduler: pid %d, pri %hd, pol %hd"

#define PLOG_TYPE_PFE 38
// "Page fault at memory location 0x%08x, pc=0x%08x, ra=0x%08x"

#define PLOG_TYPE_PFL 39
// "Page fault complete, type %d"

#define PLOG_TYPE_MUNMAP 40
// "mm: 0x%08x, unmapping from vaddr 0x%08x, len=0x%08x"

// This is *only* logged as a result of an ioctl() request (PLOG_IOCTL_DUMP_MM)
// to /proc/plog to log information about which PTEs are present in a pid's mm.
#define PLOG_TYPE_PTE_PRESENT 41
// "[nopid] mm: 0x%08x, pte 0x%08x, vaddr 0x%08x is present (page flags: 0x%08x)"

// This is *only* logged as a result of an ioctl() request (PLOG_IOCTL_PID_MMS)
// to /proc/plog to log information about the pid to mm mappings.
#define PLOG_TYPE_PID_MM 42
// "mm: 0x%08x"

#define PLOG_TYPE_CACHE_FLUSH_ALL_PC 55
// "Flush cache all"

#define PLOG_TYPE_DCACHE_FLUSH 56
// "Flush dcache"

#define PLOG_TYPE_FLUSH_ALL_PC_RANGE 57
// "Flush cache all due to range"

#define PLOG_TYPE_FLUSH_ALL_MM 58
// "Flush cache all due to mm flush"

#define PLOG_TYPE_FLUSH_PAGE 59
// "Flush page"

#define PLOG_TYPE_WBACK_INVALIDATE 60
// "Write back invalidate size 0x%08x"

#define PLOG_TYPE_INVALIDATE 61
// "Invalidate size 0x%08x"

#define PLOG_TYPE_QUEUE_CONGESTED 64
// "[nopid] Disk queue SYNC:%d congested"

#define PLOG_TYPE_QUEUE_UNCONGESTED 65
// "[nopid] Disk queue SYNC:%d uncongested"

#define PLOG_TYPE_REQ_ALLOC_FAILED 66
// "[nopid] Disk queue SYNC:%d queuing failed, no mem"

#define PLOG_TYPE_REQ_QUEUING_FAILED 67
// "[nopid] Disk queue SYNC:%d queuing failed, full"

#define PLOG_TYPE_WAKEUP_REQ_Q 68
// "[nopid] Disk queue SYNC:%d wake-up"

#define PLOG_TYPE_ADD_REQ_TO_QUEUE 69
// "[nopid] Disk: 0x%08x (RT:%d,WR:%d) I/O queued (sectors=%d)"

//
// Module PLOGs number space beginning at 500
//

#define PLOG_TYPE_AES_DMASTART 500
// "[dma] AES dest: 0x%08x source: 0x%08x, len: 0x%08x"

#define PLOG_TYPE_XPT_DMASTART 501
// "[dma] XPT source: 0x%08x, len: 0x%08x"

#define PLOG_TYPE_MVD_PICTURE_DATA_READY 502
// "[mvd] PictureDataReady" 

#define PLOG_TYPE_MVD_PICTURE_DATA_READY_DONE 503
// "[mvd] PictureDataReady done"

#define PLOG_TYPE_BCM_DISPLAY_LOCK    504
// "Display lock take"

#define PLOG_TYPE_BCM_DISPLAY_UNLOCK  505
// "Display lock give"


// The ioctl (user) interface...
//
struct descPlogEvent {
    int  type;
    int  len;
    char data[PLOG_MAX_DATASIZE];
};

#define PLOG_TYPE_UNBLACK_DONE 101
// "The unblack command is complete"

#define PLOG_TYPE_BLACK_DONE   102
// "The black command is complete"

#define PLOG_TYPE_IRKEY_DOWN 103
// "Received remote key down: 0x%lx"

#define PLOG_TYPE_MOM_TUNE_START 104
// "Starting tuning"

#define PLOG_TYPE_MOM_TUNE_DONE 105
// "Done tuning"

#define PLOG_TYPE_MOM_SEQHDR_RECVED 106
// "Received Seq Hdr"

#define PLOG_TYPE_MOM_TUNE_REQ 107
// "Tune command requested"

#define PLOG_TYPE_MOM_SINK_CC 108
// "Sink performing Channel Change command"

#define PLOG_TYPE_MOM_SINK_UNROLL 109
// "Sink starting playback"

#define PLOG_TYPE_DTUNELIB_CC 110
// "Sending DTune message over Oslink, freq %ld, vscid %lx, ascid %lx"


#define PLOG_TYPE_COBRA_IOCTL 111
// "Receiving Cobra Tuner ioctl %ld (tuner num %ld, fEnter: %ld)"

#define PLOG_TYPE_MOM_SRC_VIDEO_SEG 112
// "MOM source received a video segment, type = %ld"

#define PLOG_TYPE_MOM_SINK_VIDEO_SEG 113
// "MOM Sink writing a video segment, type = %ld"

#define PLOG_TYPE_MOM_SINK_UNBLACK 114
// "MOM Sink was asked to unblack" 

#define PLOG_TYPE_CTXTMGR_RECVD_KEY 115
// "ContextMgr received up key"

#define PLOG_TYPE_VIDEOMGR_TUNE_REQUEST 116
// "VideoMgr received tune request"

#define PLOG_TYPE_WATCHLIVE_CHANNEL_CHANGING_START 117
// "WatchLive told ChannelChanging from video mgr"

#define PLOG_TYPE_WATCHLIVE_CHANNEL_CHANGING_END 118
// "WatchLive told ChannelChanging from video mgr"

#define PLOG_TYPE_WATCHLIVE_CHANNEL_CHANGED 119
// "WatchLive told ChannelChanged from video mgr"

#define PLOG_TYPE_BANNER_QUERY_INFO 120
// "Banner query information returned"

#define PLOG_TYPE_MIXAUD_GOT_PLAY 121
// "MixAud got play"

#define PLOG_TYPE_TVSOUNDPLAYER_CLIENT_SENDING_REQUEST 122
// "TvSoundPlayerClient sending request %ld"

#define PLOG_TYPE_MOM_SINK_NEXT_FULL_BUF_START 123
// "MOM Sink requesting a buffer"

#define PLOG_TYPE_MOM_SINK_NEXT_FULL_BUF_DONE 124
// "MOM Sink nextFullBuf completed"

#define PLOG_TYPE_MOM_SINK_PLAYBACK_BUF_START 125
// "MOM Sink playing a buffer"

#define PLOG_TYPE_MOM_SINK_PLAYBACK_BUF_DONE 126
// "MOM Sink done playing a buffer"

#define PLOG_TYPE_MOM_READ_AHEAD_GET_MULTIPLE 127
// "MOM ReadAheadCache requests buffers"

#define PLOG_TYPE_VDEC_VIDEO_LOCK_LOST      128
// "Input %d VDEC video lock lost"

#define PLOG_TYPE_VDEC_VIDEO_LOCK_ACQUIRED  129
// "Input %d VDEC video lock acquired"

/**
 * In 2.4 due to the hackish way this is implemented in sys_ioctl, this
 * ioctl cmd will log a plog event when used against any valid file
 * descriptor, as long as the asscociated device doesn't implement that
 * ioctl cmd...
 * 
 * In 2.6 normal open()/close() is required.
 * 
 */

#define PLOG_TYPE_SW_WDOG_KEEPALIVE 144
#define PLOG_TYPE_HW_WDOG_KEEPALIVE 145

#include <asm/ioctl.h>
#define PLOG_LOG_EVENT     _IOW(0xc4,0xfd,struct descPlogEvent)
#define PLOG_LOG_GET_SIZE  _IOW(0xc4,0xfe,unsigned long)
#define PLOG_CONTROL_TYPE  _IOW(0xc4,0xff,unsigned long) 

// ioctl to do CLEAR/NOCLEAR along with ENABLE/DISABLE/NOCHANGE for plog op
// specify one & only one flag from each of two sets below
#define FLAG_CLEAR	1
#define FLAG_NOCLEAR	2
// ---- 
#define FLAG_ENABLE	4
#define FLAG_DISABLE	8
#define FLAG_NOCHANGE	16

#ifdef __KERNEL__


#ifdef MODULE
// A module may be compiled with a development kernel and run with a production kernel 
// The production kernel does not have these generic_event interfaces 
// In drivers, we never compile out this interfece, we test at run time. 

extern void _plog_generic_event0(int event) __attribute__((weak)) ;
extern void _plog_generic_event(int event, int somedata) __attribute__((weak));
extern void _plog_generic_event2(int event, int data1, int data2) __attribute__((weak));
extern void _plog_generic_event3(int event, int data1, int data2, int data3) __attribute__((weak));
extern void _plog_generic_event4(int event, 
                                 int data1, 
                                 int data2, 
                                 int data3, 
                                 int data4) __attribute__((weak));

#define plog_generic_event0(x) \
	do { if ( _plog_generic_event0 != NULL) { _plog_generic_event0(x);} } while (0)

#define plog_generic_event(x,y) \
	do { if ( _plog_generic_event != NULL) { _plog_generic_event(x,y);}} while (0)

#define plog_generic_event2(x,y,z)\
	do { if ( _plog_generic_event2 != NULL) { _plog_generic_event2(x,y,z);}} while (0)
	
#define plog_generic_event3(x,y,z,w)\
	do { if ( _plog_generic_event3 != NULL) { _plog_generic_event3(x,y,z,w);}} while (0)

#define plog_generic_event4(x,y,z,w,v) \
	do { if ( _plog_generic_event4 != NULL) { _plog_generic_event4(x,y,z,w,v);}} while (0)

#endif /* MODULE */


#ifdef CONFIG_TIVO_PLOG

struct request;

void plog_destructive_liforead(int *buf, int entries);

extern void _plog_enable(int enable);

#ifndef MODULE
extern void _plog_generic_event0(int event);
extern void _plog_generic_event(int event, int somedata);
extern void _plog_generic_event2(int event, int data1, int data2);
extern void _plog_generic_event3(int event, int data1, int data2, int data3);
extern void _plog_generic_event4(int event, 
                                 int data1, 
                                 int data2, 
                                 int data3, 
                                 int data4);
#endif /* MODULE */

extern void _plog_rq_add(struct task_struct *tp);
extern void _plog_rq_del(struct task_struct *tp);
extern void _plog_swcpu(struct task_struct *from, struct task_struct *to);
extern void _plog_swcpu_end(int pid, int flags);
extern void _plog_setscheduler(int   pid, 
                               int   sched_pid, 
                               short priority, 
                               short policy);
extern void _plog_intr_enter(int num, unsigned long pc);
extern void _plog_intr_leave(int num);
extern void _plog_bh_enter(int num);
extern void _plog_bh_leave(int num);
extern void _plog_reswap(void *mm, int physPage, int virtPage);
extern void _plog_pageout(void *mm, int physPage, int virtPage);
extern void _plog_diskdispatch(unsigned long start, struct request *req);
extern void _plog_diskstart_sata(struct request *rq);
extern void _plog_diskstop_sata(struct request *rq, int write);
extern void _plog_queue_congested(int congested, int sync);
extern void _plog_disk_request_failed_queue_full(int sync);
extern void _plog_disk_request_failed_no_mem(int sync);
extern void _plog_wake_up_queue(int sync);
extern void _plog_add_disk_request_to_queue(struct request *rq);

extern void _plog_generic_page_swap(void *mm, int physPage, int virtPage);
extern void _plog_filemap_page_swap(void *mm, int physPage, int virtPage);
extern void _plog_ioctl(unsigned int fd, unsigned int cmd, unsigned long arg);
extern int _plog_user(struct descPlogEvent *arg);
extern void _plog_clear(void);
extern void _plog_enable(int yes);

//
// Public kernel interface starts here:
//
#ifndef MODULE
#define plog_generic_event0 _plog_generic_event0
#define plog_generic_event _plog_generic_event
#define plog_generic_event2 _plog_generic_event2
#define plog_generic_event3 _plog_generic_event3
#define plog_generic_event4 _plog_generic_event4
#endif /* MODULE */


#define plog_rq_add _plog_rq_add
#define plog_rq_del _plog_rq_del
#define plog_swcpu _plog_swcpu
#define plog_swcpu_end _plog_swcpu_end
#define plog_setscheduler _plog_setscheduler
#define plog_intr_enter _plog_intr_enter
#define plog_intr_leave _plog_intr_leave
#define plog_bh_enter _plog_bh_enter
#define plog_bh_leave _plog_bh_leave
#define plog_reswap _plog_reswap
#define plog_pageout _plog_pageout
#define plog_diskdispatch _plog_diskdispatch
#define plog_diskstart_sata _plog_diskstart_sata
#define plog_diskstop_sata _plog_diskstop_sata
#define plog_queue_congested _plog_queue_congested
#define plog_disk_request_failed_queue_full _plog_disk_request_failed_queue_full
#define plog_disk_request_failed_no_mem _plog_disk_request_failed_no_mem
#define plog_wake_up_queue _plog_wake_up_queue
#define plog_add_disk_request_to_queue _plog_add_disk_request_to_queue
#define plog_generic_page_swap _plog_generic_page_swap
#define plog_filemap_page_swap _plog_filemap_page_swap
#define plog_ioctl _plog_ioctl
#define plog_user _plog_user

// These are exported to modules, too:
void plog_dump(int max, int swflags);
int process_log(int type, int data_len, void *data);

#else // !CONFIG_TIVO_PLOG

#define _plog_enable(enable) do { } while (0)

#define plog_destructive_read(x,y) do { } while (0)

#ifndef MODULE
#define plog_generic_event0(x) do { } while (0)
#define plog_generic_event(x,y) do { } while (0)
#define plog_generic_event2(x,y,z) do { } while (0)
#define plog_generic_event3(x,y,z,w) do { } while (0)
#define plog_generic_event4(x,y,z,w,v) do { } while (0)
#endif /* MODULE */


#define plog_rq_add(x,y) do { } while (0)
#define plog_rq_del(x) do { } while (0)
#define plog_swcpu(x,y) do { } while (0)
#define plog_swcpu_end() do { } while (0)
#define plog_setscheduler(x,y,z,w) do { } while (0)
#define plog_intr_enter(x,y) do { } while (0)
#define plog_intr_leave(x) do { } while (0)
#define plog_bh_enter(x) do { } while (0)
#define plog_bh_leave(x) do { } while (0)
#define plog_reswap(x,y,z) do { } while (0)
#define plog_pageout(x,y,z) do { } while (0)
#define plog_diskdispatch(x,y) do { } while (0)
#define plog_diskstart_sata(x) do { } while (0)
#define plog_diskstop_sata(x,y) do { } while (0)
#define plog_queue_congested(x,y) do { } while (0)
#define plog_disk_request_failed_queue_full(x) do { } while (0)
#define plog_disk_request_failed_no_mem(x) do { } while (0)
#define plog_wake_up_queue(x)  do { } while (0)
#define plog_add_disk_request_to_queue(x)  do { } while (0)
#define plog_generic_page_swap(x,y,z) do { } while (0)
#define plog_filemap_page_swap(x,y,z) do { } while (0)
#define plog_ioctl(x,y,z) do { } while (0)
extern inline int plog_user(struct descPlogEvent *arg) { return -EINVAL; }

#endif // CONFIG_TIVO_PLOG
#endif // __KERNEL__

#endif // __LINUX_PLOG_H
