#ifndef _LINUX_CONTIGMEM_H
#define _LINUX_CONTIGMEM_H

/*
 * Support for large physically contiguous memory regions
 *
 * Copyright (C) 2011 TiVo Inc. All Rights Reserved.
 */

#include <linux/ioctl.h>

#ifdef __KERNEL__
/* This is the interface that is exported to kernel modules */
extern int mem_get_contigmem_region(unsigned int region, unsigned long *start,
                                    unsigned long *size);
#endif


struct contigmem_region_request {
	unsigned int	region;	    /* in: region number */
	unsigned long	phys_start; /* out: phys addr of start of region */
	unsigned long	size;       /* out: size of region, in bytes */
};

/* ioctls to access contiguous regions */
#define CONTIGMEM_IOC_MAGIC  'N'

/* 1-5 are reserved for compatibility (or lack thereof) reasons */
#define CONTIGMEM_GET_REGION		_IOWR(CONTIGMEM_IOC_MAGIC, 6, struct contigmem_region_request)


/* the region number definitions */
#define CONTIGMEM_REGION_PLOG		0
#define CONTIGMEM_REGION_INPUT		1
#define CONTIGMEM_REGION_OUTPUT		2

#define CONTIGMEM_REGION_UMA		8

#define CONTIGMEM_REGION_MAX		9

#endif /* _LINUX_CONTIGMEM_H */
