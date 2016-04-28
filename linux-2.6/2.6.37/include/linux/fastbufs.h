/*
 * Copyright (C) 2006 TiVo Inc. All Rights Reserved.
 *
 * Fastbuffers are owned by kernel and theoretically 
 * user tasks should have read-only access to these
 * buffers. On MIPS while sharing buffers across both
 * domains by manupulating TLBs, exclusive read-only
 * protection for user domain cannot be acheived. Hence for 
 * safety reasons always access fastbuffer elements using the
 * following API.
 *
 */ 
#ifndef FASTBUFS_H
#define FASTBUFS_H   

#include<linux/ioctl.h>

#ifndef __KERNEL__
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <string.h>
#include <linux/errno.h>
#endif /*__KERNEL*/

/*ioctl*/
#define FBUF_IOC_MAGIC          'f'

#define FBUF_ALLOC              _IO(FBUF_IOC_MAGIC, 1)
#define FBUF_FREE               _IO(FBUF_IOC_MAGIC, 2)
#define FBUF_LIST_CNTXTS        _IO(FBUF_IOC_MAGIC, 3)
#define FBUF_LIST_ALL_CNTXTS    _IO(FBUF_IOC_MAGIC, 4)

#define FBUF_MAGIC_VMSTART      0x40000000
#define FBUF_VERSION_1          1

#define FBUF_MAX_CNTXTS         384 /*Bytes*/
#define FBUF_MAX_THREADS        256 /*Bytes*/
#define FBUF_MAX_ENTS           FBUF_MAX_CNTXTS /*Greatest of all*/

#define THREAD_NAME_STR_LEN     24 /*Bytes*/
#define CNTXT_NAME_STR_LEN      28 /*Bytes*/
#define FBUF_VER_STR_LEN        32 /*Bytes*/
#define LIST_CNTXTS_REC_SZ      (THREAD_NAME_STR_LEN+CNTXT_NAME_STR_LEN+64)

/*This can be changed under the carpet in linux/tivo-montime.h*/
typedef struct { unsigned long long count; } monotonic_t;

typedef enum {
    FBUF_THREAD_TYPE=0,
    FBUF_CNTXT_TYPE,
    FBUF_MAX_TYPES
}fbuf_type_t;

struct context_fbuf_s{
    struct thread_fbuf_s *thread_info;
    char name[CNTXT_NAME_STR_LEN];
    int id;
    monotonic_t t,s;
    unsigned long active_msecs;
    unsigned int priority;
};
typedef struct context_fbuf_s context_fbuf;

struct thread_fbuf_s{
    struct context_fbuf_s *cntxt;
    char name[THREAD_NAME_STR_LEN];
    pid_t pid;
};
typedef struct thread_fbuf_s thread_fbuf;

struct fastbuf_hdr_s{
    int version;
    thread_fbuf *cur_thread_info;
    monotonic_t t; 
    unsigned long mono_counts_per_msec;
};
typedef struct fastbuf_hdr_s fastbuf_hdr;

struct fbuf_req{
    fbuf_type_t fbuf_type;
    void *ptr;  
};

#define THREAD_FBUF_SZ    (sizeof(thread_fbuf))
#define CNTXT_FBUF_SZ     (sizeof(context_fbuf)) 

#ifdef __KERNEL__
/* The fastbuf is not used currently */
/* static fastbuf_hdr *fastbuf = (fastbuf_hdr *)FBUF_MAGIC_VMSTART; */
#define ENABLE_FASTBUFS 1
#endif /*__KERNEL*/

#endif /* FASTBUFS_H */
