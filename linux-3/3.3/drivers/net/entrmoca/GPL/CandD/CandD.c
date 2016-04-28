/*******************************************************************************
*
* GPL/CandD/CandD.c
*
* Description: C and D driver main
*
*******************************************************************************/

/*******************************************************************************
*                        Entropic Communications, Inc.
*                         Copyright (c) 2001-2008
*                          All rights reserved.
*******************************************************************************/

/*******************************************************************************
* This file is licensed under GNU General Public license.                      *
*                                                                              *
* This file is free software: you can redistribute and/or modify it under the  *
* terms of the GNU General Public License, Version 2, as published by the Free *
* Software Foundation.                                                         *
*                                                                              *
* This program is distributed in the hope that it will be useful, but AS-IS and*
* WITHOUT ANY WARRANTY; without even the implied warranties of MERCHANTABILITY,*
* FITNESS FOR A PARTICULAR PURPOSE, TITLE, or NONINFRINGEMENT. Redistribution, *
* except as permitted by the GNU General Public License is prohibited.         *
*                                                                              *
* You should have received a copy of the GNU General Public License, Version 2 *
* along with this file; if not, see <http://www.gnu.org/licenses/>.            *
*******************************************************************************/


/* Necessary includes for device drivers */
#include <linux/init.h>
#include <linux/version.h>
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 18)
#include <linux/autoconf.h>
#else
#include <linux/config.h>
#endif
#include <linux/module.h>
#include <linux/kernel.h> /* printk() */
#include <linux/slab.h> /* kmalloc() */
#include <linux/fs.h> /* everything... */
#include <linux/errno.h> /* error codes */
#include <linux/types.h> /* size_t */
#include <linux/proc_fs.h>
#include <linux/fcntl.h> /* O_ACCMODE */
#include <asm/system.h> /* cli(), *_flags */
#include <asm/uaccess.h> /* copy_from/to_user */
#include <asm/io.h>
#include <linux/ioport.h>
#include <linux/sockios.h>
#include "candd_gpl_hdr.h"
MODULE_LICENSE("Dual BSD/GPL");
// definitions
#define VERSION "CandD Version 1.22"
#define K_BUF_SIZE  512
#define G_IO_START  0xf1018100
#define G_IO_MUX    0xf101800c
#define G_IO1_START     0xf1018140
#define G_IO2_START     0xf1018180

//
// structures and definitions
//

/* Global variables of the driver */

/* Major number */
int memory_major = 60;
/* Buffer to store data */
char *memory_buffer;        // allocated
int in_use;

int driver_open = 0;   // flag indicating an open call has been made
// a close must be called before another open
void *pprivate_data=0;
//------------------------------------------------------------------------------
// This is the location of the gpio registers
//------------------------------------------------------------------------------
unsigned long G_io2;
unsigned long G_io;
void *G_base;
void *G2_base;
int reset_command;

// prototypes

int memory_init(void);
void memory_exit(void);
int memory_open(struct inode *inode, struct file *filp);
int memory_release(struct inode *inode, struct file *filp);
ssize_t memory_read(struct file *filp, char *buf,
                    size_t count, loff_t *f_pos);
ssize_t memory_write(   struct file *filp,
                        const char *buf,
                        size_t count,
                        loff_t *f_pos);
extern ssize_t memory_write_work(   struct file *filp,
                                    const char *buf,
                                    size_t count,
                                    loff_t *f_pos) ;
extern int ioctl_operation(struct inode  *fs_inode,
                    struct file   *filp,
                    unsigned int  cmd,
                    unsigned long argument);

/* Structure that declares the usual file */
/* access functions */
struct file_operations memory_fops = {
    .read    = memory_read,
    .write   = memory_write,
    .open    = memory_open,
    .release = memory_release,
    .ioctl   = ioctl_operation
};

/* Declaration of the init and exit functions */
module_init(memory_init);
module_exit(memory_exit);

/*
*     module entry point
*     initializes the module for the first open
*
*
*
*
*PUBLIC***/
int memory_init(void)
{
    int result;
    unsigned long mpp;
    unsigned int * mpp_base;
    printk(KERN_ALERT "%s\n",VERSION);

    /* Registering device */
    result = register_chrdev(memory_major, "memory", &memory_fops);
    if (result < 0)
    {
        printk(KERN_ALERT "memory: cannot obtain major number %d\n", memory_major);
        return result;
    }

    /* Allocating memory for the buffer */
    memory_buffer = kmalloc(K_BUF_SIZE, GFP_KERNEL);
    if (!memory_buffer)
    {
        result = -ENOMEM;
        printk(KERN_ALERT "DIAG: Failure to get memory buffer %d\n",memory_major);
        goto fail;
    }
    memset(memory_buffer, 0, K_BUF_SIZE);
    in_use=0;
    reset_command = 0;

    G_io2 = G_IO2_START ;
    if (!request_region(G_io2,4*5,"GPIO2"))
    {
        printk(KERN_ALERT "DIAG: Sorry - memory mapped address is in use\n");
        result = -EBUSY;
        goto fail;
    }
    G2_base=ioremap(G_io2,4*5);





    printk(KERN_ALERT "TAN: G2_base = %0x\n", (unsigned int)G2_base);





    G_io = G_IO_START ;
    if (!request_region(G_io,4*5,"GPIO"))
    {
        printk(KERN_ALERT "DIAG: Sorry - memory mapped address is in use\n");
        result = -EBUSY;
        goto fail;
    }
    G_base=ioremap(G_io,4*5);





    printk(KERN_ALERT "TAN: G_base = %0x\n", (unsigned int)G_base);





    mpp = G_IO_MUX;
    if (!request_region(mpp,4*5,"MPP"))
    {
        printk(KERN_ALERT "DIAG: Sorry - MPP memory mapped address is in use\n");
        result = -EBUSY;
        goto fail;
    }
    mpp_base = ioremap(mpp, 4);
    writel(0x0,mpp_base); /*Init the MPP for those GPIOs we are using!*/
    return 0;

fail:
    memory_exit();
    return result;
}


/*
*    module exit point
*    tears everything down
*
*
*
*
*PUBLIC***/
void memory_exit(void)
{
    /* Freeing the major number */
    unregister_chrdev(memory_major, "memory");
    /* Freeing buffer memory */
    if (memory_buffer) {
        kfree(memory_buffer);
    }

}

/*
*
*STATIC***/
static void *context_create( void )
{
    int size ;
    void *m ;
    dk_context_t *dkcp ;
    dd_context_t *ddcp ;

    size = sizeof(dk_context_t) + sizeof(dd_context_t) ;
    m = HostOS_Alloc( size ) ;
    if( m ) {
        HostOS_Memset( m, 0, size ) ;
        dkcp = (dk_context_t *)m ;
        ddcp = (dd_context_t *)(((char *)m)+sizeof(dk_context_t)) ;
        dkcp->priv = (void *)ddcp ;
        dkcp->name = DRV_NAME ;    // like net_device
    }
    return( m ) ;
}

/*
*
*STATIC***/
static void context_destroy( dk_context_t *dkcp )
{
    int size ;

    size = sizeof(dk_context_t) + sizeof(dd_context_t) ;
    HostOS_Free( dkcp, size ) ;
}

/*
*    module's file open entry point
*
*
*
*
*
*PUBLIC***/
int memory_open(struct inode *inode, struct file *filp)
{
    int          err ;
    dk_context_t *dkcp ;
    dd_context_t *ddcp ;

    err = 0 ;
    if( !driver_open ) {

        // clink init

        // create dk and dd contexts

        filp->private_data = context_create( ) ;
        // Sotre the context pointer to a global pointer, so all of users opening this driver can share it.
        pprivate_data = filp->private_data;
        if( filp->private_data )
        {
            dkcp = (dk_context_t *)filp->private_data ;
            ddcp = dk_to_dd( dkcp ) ;

            // create dg and dc contexts

            err = Clnk_init_dev( &ddcp->p_dg_ctx, ddcp, dkcp, (unsigned long)G_base );
            if( err )
            {
                err = 1 ;    // Error: no memory
            }

        }
        else
        {
            err = 1 ;    // Error: no memory
        }

    }
    if(!err)
    {
        driver_open ++;
        if(pprivate_data)
        {
            filp->private_data = pprivate_data;
        }
        else
        {
            err = 1;
        }
    }
    return( err ) ;
}

/*
*    module's file close entry point
*
*
*
*
*
*
*PUBLIC***/
int memory_release(struct inode *inode, struct file *filp)
{
    int err ;
    dk_context_t *dkcp ;
    dd_context_t *ddcp ;
    void         *dgcp ;

    err = 0 ;
    if( driver_open == 1) {

        // clink exit

        dkcp = (dk_context_t *)filp->private_data ;
        if( dkcp ) {

            // close and free dg and dc contexts

            ddcp = dk_to_dd( dkcp ) ;
            dgcp = dd_to_dg( ddcp ) ;
            Clnk_exit_dev( dgcp );
            ddcp->p_dg_ctx = 0 ;

            // free dk and dd contexts

            context_destroy( dkcp ) ;
            filp->private_data = 0 ;
        }
        pprivate_data = 0;
        driver_open = 0 ;
    }
    if(driver_open)
        driver_open --;

    return( err ) ;
}

/*
*    module's file read entry point
*
*
*
*
*
*
*PUBLIC***/
ssize_t memory_read(struct file *filp, char *buf,
                    size_t count, loff_t *f_pos)
{
    int i;

    if( driver_open ) {

        for (i=0; i<4; i++)
        {
            printk(KERN_ALERT "GPIO[%2d] = %04x\n",i,readw(G_base + i*4 + 2));
        }

        /* Transfering data to user space */
        HostOS_copy_to_user(buf,memory_buffer,1);

        /* Changing reading position as best suits */
        if (*f_pos == 0) {
            *f_pos+=1;
            return 1;
        } else {
            return 0;
        }

    } else {
        return( 0 ) ;   // Error: not already open
    }
}

/*
*    module's file write entry point
*
*
*
*
*
*
*PUBLIC***/
ssize_t memory_write(   struct file *filp,
                        const char *buf,
                        size_t count,
                        loff_t *f_pos)
{
    ssize_t s ;

    if( driver_open ) {

        s = memory_write_work(  filp, buf, count, f_pos) ;
        return( s ) ;

    } else {
        return( 0 ) ;   // Error: not already open
    }
}

