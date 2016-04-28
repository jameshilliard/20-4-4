/* 
 * TiVo /proc/watchpoint 
 *
 * Copyright (C) 2003 TiVo Inc. All Rights Reserved.
 */

#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/proc_fs.h>
#include <asm/bootinfo.h>
#include <asm/cpu.h>
#include <asm/page.h>
#include <asm/watch.h>
#include <asm/io.h>

static struct page *lockedPage = NULL;
spinlock_t watchLock = SPIN_LOCK_UNLOCKED;

/*
 * For now you must make sure to pin the page before setting a watchpoint
 * 
 * Low-order 3 bits of addr must be 0RW
 *  where R is set to 1 to break on reads and W is set to 1 to break on write
 *  currently always forced to 001
 */ 
static int write_proc_watchpoint(struct file * pFile, const char * pBuffer,
                                 unsigned long input_length, void *data )
{
    unsigned long addr;
    struct vm_area_struct * vma;
    struct page * page;
    int phys = 0;
    int ret;
    int rwflags;

    if (pBuffer[0] == 'P') {
        pBuffer++;
        phys = 1;
    }

    addr = simple_strtoul(pBuffer, NULL, 0);
   
    // mask off rwflags
    rwflags = addr & 0x3;

    // 8 byte align addr
    addr = addr & 0xFFFFFFF8;

    spin_lock(&watchLock);

    if (lockedPage) {
        // decrement page reference count
        page_cache_release(lockedPage);
        lockedPage = NULL;
    }

    // convert to physical addr if necessary
    if (addr && !phys) {
        // code in mm/memory.c:map_user_kiobuf()
        // seems to think it important to take this sem
        down_read(&current->mm->mmap_sem);
        ret = get_user_pages(current, current->mm, addr, 1,
                             0, 1 /* force it in */, &page, &vma);
        up_read(&current->mm->mmap_sem);

        if (ret <= 0) {
            printk("get_user_pages failed ret = %d\n", ret);
            spin_unlock(&watchLock);
            return 0;
        }
        printk("vma  = %p\n", vma);

        if (vma->vm_flags & VM_RESERVED) {
            printk("vma is reserved\n");
        }
        
        if (vma->vm_flags & VM_SHM) {
            printk("vma is shared memory\n");
        }
        
        if (vma->vm_flags & VM_LOCKED) {
            printk("vma is locked\n");
        }
        else {
            printk("WATCHPOINT warning: vma not locked\n");
        }
        
        //printk("page = 0x%lx\n", page);
        //printk(" virtual = 0x%lx\n", page_address(page));
        //printk(" index   = 0x%lx\n", page->index);
        //printk(" locked  = %i\n", page->flags & PG_locked);
        
        addr = virt_to_phys(page_address(page)) + (addr % PAGE_SIZE);

        lockedPage = page;
    }

    printk("WATCH: phys addr = 0x%lx\n", addr);

    // force watchpoint to trap writes (only) for now
    // since the exception handler for WATCH will skip the offending
    // instruction, we should only be using the watchpoint to protect
    // addresses we think should not be overwritten
    rwflags = 1;
    addr |= rwflags;

    // write watchpoint register
    write_32bit_cp0_register(CP0_WATCHLO, addr);
    write_32bit_cp0_register(CP0_WATCHHI, 0);

    spin_unlock(&watchLock);
    return input_length;
} 

static int read_proc_watchpoint(char * pBuffer, char **start, off_t offset,
                                int length, int *eof, void *data)
{
    int len = 0;
    unsigned long watchHi, watchLo;

    watchHi = read_32bit_cp0_register(CP0_WATCHHI);
    watchLo = read_32bit_cp0_register(CP0_WATCHLO);

    len = sprintf(pBuffer, "0x%08lx\n", watchLo) - offset;
    if (len < length) {
        *eof = 1;
        if (len <= 0) return 0;
    }
    else {
        len = length;
    }

    *start = pBuffer + offset;
    return len;
}

// Create /proc entry
static int __init watchpoint_init(void)
{
    struct proc_dir_entry *entry;

    if (mips_cpu.options & MIPS_CPU_WATCH ) {   
        entry = create_proc_entry("watchpoint", 0, &proc_root);
        if (entry) {
            entry->write_proc = &write_proc_watchpoint;
            entry->read_proc = &read_proc_watchpoint;
        }
    }

    return 0;
}

module_init(watchpoint_init);
