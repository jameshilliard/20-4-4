/*
 * tivo-genioctl.h
 * Copyright (C) 2005, 2006 TiVo Inc. All Rights Reserved.
 */

#include<linux/ioctl.h>

#define GIOCTL_IOC_MAGIC        'g'

#define CRASHALLASSRT           _IO(GIOCTL_IOC_MAGIC, 1)
#define UPDATE_EXEC_CTX         _IO(GIOCTL_IOC_MAGIC, 2)
#define TEST_KSTACK_GUARD       _IO(GIOCTL_IOC_MAGIC, 3)
#define TEST_SGAT_DIO           _IO(GIOCTL_IOC_MAGIC, 4)
#define TICK_KILLER             _IO(GIOCTL_IOC_MAGIC, 5)
#define SATA_UNPLUG_INT         _IO(GIOCTL_IOC_MAGIC, 6)
#define SATA_PLUG_INT           _IO(GIOCTL_IOC_MAGIC, 7)
#define LOCKMEM                 _IO(GIOCTL_IOC_MAGIC, 8)
#define UNLOCKMEM               _IO(GIOCTL_IOC_MAGIC, 9)
#define IS_PM_FWUPGD_REQD       _IO(GIOCTL_IOC_MAGIC, 10)
#define DWNLD_SATA_PM_FW        _IO(GIOCTL_IOC_MAGIC, 11)
#define GET_VMA_INFO            _IO(GIOCTL_IOC_MAGIC, 12)
#define GO_BIST                 _IO(GIOCTL_IOC_MAGIC, 13)

struct iov
{
    unsigned int iov_base;
    int iov_len;
};

struct vma_info_struct
{
    unsigned long address,  // requested address
                  start,    // first vma address
                  end,      // last vma address
                  read:1, write:1, exec:1, shared:1,   // vma permissions
                  device,   // mapped device, 0 = none
                  inode,    // mapped inode
                  offset;   // inode offset of start address
};   
    
#ifndef __KERNEL__
#include <sys/ioctl.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <linux/errno.h>
#include <linux/tivo-crashallassert.h>

static int gen_ioctl_fd = -1;

inline static int genioctl(int cmd, unsigned int arg)
{
    if (gen_ioctl_fd <= 0)
    {
        gen_ioctl_fd = open("/dev/gen-ioctl", O_RDONLY);
        if (!gen_ioctl_fd)
            return -EINVAL;
    }

    return ioctl(gen_ioctl_fd, (unsigned int)cmd, arg);
}

inline static int mLock(unsigned int addr, int len)
{
    struct iov iovec;
    iovec.iov_base = addr;
    iovec.iov_len = len;
    return genioctl(LOCKMEM, (unsigned int)&iovec);
}

inline static int mUnlock(unsigned int addr, int len)
{
    struct iov iovec;
    iovec.iov_base = addr;
    iovec.iov_len = len;
    return genioctl(UNLOCKMEM, (unsigned int)&iovec);
}

inline static void update_exec_context(const char *str)
{
    struct k_arg arg;
    arg.tag = TAG_EXEC_CTX;
    arg.ptr = (unsigned long)str;
    if (str)
        arg.len = strlen(str);
    else
        arg.len = 0;
    genioctl(UPDATE_EXEC_CTX, (unsigned int)&arg);
}

inline static void kill_tcd_soon(int seconds)
{
    genioctl(TICK_KILLER, (unsigned int)(seconds * 100));
}

inline static int get_vma_info(struct vma_info_struct *s)
{
    return genioctl(GET_VMA_INFO, (unsigned int)s);
}
#endif
