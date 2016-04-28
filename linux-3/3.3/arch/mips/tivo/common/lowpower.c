/**
 * File: lowpower.c
 *
 * Auxiliary IO tracing for the LowPower mode.
 *
 * Copyright (C) 2013 TiVo, Inc. All Rights Reserved.
 *
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/fs.h>
#include <linux/path.h>
#include <linux/dcache.h>
#include <linux/mount.h>
#include <../../../fs/mount.h>
#include <asm/current.h>
#include <linux/sched.h>


static int fLowPower = 0;

extern void show_trace_task_bt(struct task_struct *tsk, int dispconsole);

void lowpower_audit_sync(void)
{
    if (fLowPower)
    {
        printk(KERN_INFO "sync e=%s: %s\n", current->comm, current->cmdline);
        show_trace_task_bt(current, 0);
    }
}

void lowpower_audit_file_stat(struct vfsmount *mnt, struct dentry *dentry)
{
    if (fLowPower)
    {
    
        const char* devname = real_mount(mnt)->mnt_devname;
        const char* filename = "";
        const char* dirname = "";
        
        // exclude stat() calls on binaries from rootfs
        if (!strncmp("/dev/root", devname, 9) ||
            !strncmp("/proc", devname, 5))
            return;
        
        filename = (const char *) dentry->d_name.name;
        dirname = dentry->d_parent->d_name.name;

        printk(KERN_INFO "stat: dev=%s, file=%s/%s, cmd=%s: %s", devname, dirname, filename, 
            current->comm, current->cmdline);
        show_trace_task_bt(current, 0);
    }
}


void lowpower_audit_file_access(const char* func, struct dentry* dentry)
{
    if (fLowPower)
    {
        const char *filename = (const char *) dentry->d_name.name;
        const char *dirname = "";
        if (dentry->d_parent) {
            dirname = dentry->d_parent->d_name.name;
        }

        printk(KERN_INFO "%s: file=%s/%s, cmd=%s: %s", func, dirname, filename, 
            current->comm, current->cmdline);
        show_trace_task_bt(current, 0);
    }
}

void lowpower_audit_vfs_access(const char* func, struct file * file)
{
    if (fLowPower) 
    {
        const char *filename = "";
        const char *dirname = "";
        const char *devname = "";
        struct dentry *dentry = NULL;
        struct dentry *parent = NULL;        
        if (file) {
            devname = real_mount(file->f_path.mnt)->mnt_devname;
            dentry = file->f_path.dentry;
            parent = NULL;
            if (dentry) {
                filename = (const char *) dentry->d_name.name;
                parent = dentry->d_parent;
                if (parent) {
                    dirname = (const char *) parent->d_name.name;
                }
            }
        }
        printk(KERN_INFO "%s: dev=%s, file=%s/%s, cmd=%s: %s", func, devname, dirname, filename, 
            current->comm, current->cmdline);
        show_trace_task_bt(current, 0);
    }
}

void lowpower_audit_swap_access(const char* func)
{
    if (fLowPower) 
    {
        printk(KERN_INFO "%s: cmd=%s: %s\n", func, 
            current->comm, current->cmdline);
        // getting task backtrace may freeze if
        // in the middle of swapping in
        //show_trace_task_bt(current, 0);
    }

}

void lowpower_audit_bio(void)
{
    if (fLowPower)
    {
        printk(KERN_INFO "bio: cmd=%s: %s\n", 
           current->comm, current->cmdline);
        show_trace_task_bt(current, 0);
    }
}


static int lowpower_write(struct file *file,const char __user *buf,unsigned long count,void *data )
{
    //take only first character, ignore the rest
    char locbuf[2]; 
    if(copy_from_user(locbuf, buf, 1))
    {
        return -EFAULT;
    }
    locbuf[1] = 0;
    
    sscanf(locbuf, "%d", &fLowPower);
    
    return count;

    //printk("Set fLowPower: %d\n", fLowPower);
}

static int __init lowpower_init(void)
{
    struct proc_dir_entry *entry;

    entry = create_proc_entry("lowpower", S_IFREG | S_IWUSR, 0 );
    entry->write_proc = lowpower_write;
    return 0;
}

module_init(lowpower_init);

