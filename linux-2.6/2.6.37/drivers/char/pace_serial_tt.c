 /********************************************************************
 *********************************************************************
 *
 *  File        :  $Source: /export/storage0/sources/cvsroot/Thirdparty/siliconvendor/broadcom/nexus/kernel/stblinux-2.6.37/drivers/char/pace_serial_tt.c,v $
 *
 *  Description :  Implementation of a write-only tty device.
 *                 Allows RS232 output to occur even when the
 *                 PACE_RS232_SILENT kernel config is in place.
 *                 This device is primarily intended to allow
 *                 testtask to continue to operate when the serial
 *                 output has been completely silenced (hence the "_tt"
 *                 suffix).
 *
 *  Author      :  Steve Turner.
 *
 *  Copyright   :  Pace Micro Technology 2007 (c)
 *
 *                 The copyright in this material is owned by
 *                 Pace Micro Technology PLC ("Pace"). This
 *                 material is regarded as a highly confidential
 *                 trade secret of Pace. It may not be reproduced,
 *                 used, sold or in any other way exploited or
 *                 transferred to any third party without the prior
 *                 written permission of Pace.
 *
 *********************************************************************
 ********************************************************************/

#ifdef CONFIG_PACE_RS232_SILENT

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/smp_lock.h>
#include <linux/pace_serial_tt.h>

/**
* Major and minor device numbers. Must match the corresponding
* definitions in the "create_pace_nodes" script.
*/
#define S_TT_MAJOR          208
#define S_TT_MINOR_MAX      1
#define S_TT_MINOR_START    0

/**
* Holds instance data relating to a numerically identified device.
*/
typedef struct _tag_S_TT_INSTANCE
{
   struct cdev          cdev;
} S_TT_INSTANCE;

/**
* Stores data relating to the device instance.
*/
static S_TT_INSTANCE S_TT_Instance;

/**
* External APIs. Should be defined in tty_io.c.
*/
extern ssize_t pace_do_tty_forced_write(const char __user *buf, size_t count);
extern void pace_do_tty_console_control(int serial_ctrl);

/**
* Local APIs.
*/
static int s_tt_open(struct inode *inode, struct file *filp);
static int s_tt_release(struct inode *inode, struct file *filp);
static ssize_t s_tt_write(struct file *filp, const char __user *buff, size_t count, loff_t *offp);
static long s_tt_ioctl(struct file *filp, unsigned int cmd, unsigned long arg);

/**
* File operations structure.
*/
static struct file_operations s_tt_fops =
{
   .owner          = THIS_MODULE,
   .open           = s_tt_open,
   .release        = s_tt_release,
   .write          = s_tt_write,
   .unlocked_ioctl = s_tt_ioctl,
};

/**
* Local APIs.
*/

/********************************************************************
*
*       Routine : s_tt_init
*
*  Description  : Initialises the module. Automatically
*                 called by insmod when the module is loaded.
*
*         Input : None
*
*        Output : None
*
*       Returns : int - 0 = no error.
*
*        Author : Steve Turner
*
*       History : Created 26 November 2007
*
********************************************************************/

static int __init s_tt_init(void)
{
   int            rc;

   /* Local enumeration used to free up the resources in an orderly fashion
      if one of the calls should fail. Intended to stop resource/memory
      leaks */
   enum
   {
      S_TT_INIT_NONE = 0,
      S_TT_CHRDEV_REGION_REGISTERED,  /* register_chrdev_region() called */
      S_TT_CDEV_ADDED,                /* cdev_add() called */
   } enInitStage = S_TT_INIT_NONE;

   /* Initialise the instance data structure */
   memset(&S_TT_Instance, '\0', sizeof(S_TT_Instance));

   /* Allocate all character device numbers */
   rc = register_chrdev_region(MKDEV(S_TT_MAJOR, S_TT_MINOR_START),
      S_TT_MINOR_MAX, S_TT_DEV_NAME);

   if (rc == 0)
   {
      enInitStage = S_TT_CHRDEV_REGION_REGISTERED;

      /* Initialise character device settings */
      cdev_init(&S_TT_Instance.cdev, &s_tt_fops);
      S_TT_Instance.cdev.owner = THIS_MODULE;
      S_TT_Instance.cdev.ops = &s_tt_fops;

      /* Our device becomes live here */
      rc = cdev_add(&S_TT_Instance.cdev,
         MKDEV(S_TT_MAJOR, S_TT_MINOR_START), 1);

      if ( rc == 0)
      {
         enInitStage = S_TT_CDEV_ADDED;
      } /* if ( rc != 0) */

   } /* if (rc = 0) */

   /* If we failed to fully initialise, free up the resources in an
      orderly fashion, depending upon how far we got. This code is
      intended to stop resource/memory leaks */
   if (rc != 0)
   {
      switch (enInitStage)
      {
         case S_TT_CDEV_ADDED:

            /* Remove the added character device from the system */
            cdev_del(&S_TT_Instance.cdev);

            /* Note: no "break" here, fall through is intended */
         case S_TT_CHRDEV_REGION_REGISTERED:

            /* Deallocate all character device numbers */
            unregister_chrdev_region(MKDEV(S_TT_MAJOR, S_TT_MINOR_START),
               S_TT_MINOR_MAX);

            /* Note: no "break" here, fall through is intended */
         case S_TT_INIT_NONE:
         default:
            /* Nothing to clean up here! */
            break;
      } /* switch (enInitStage) */

   } /* if (rc != 0) */

   return rc;
}
module_init(s_tt_init);

/********************************************************************
*
*       Routine : s_tt_shutdown
*
*  Description  : Shuts downs the module. Automatically
*                 called by rmmod when the module is unloaded.
*
*         Input : None
*
*        Output : None
*
*       Returns : Nothing (void return).
*
*        Author : Steve Turner
*
*       History : Created 26 November 2007
*
********************************************************************/

static void __exit s_tt_shutdown(void)
{
   /* Remove the added character device from the system */
   cdev_del(&S_TT_Instance.cdev);

   /* Deallocate all character device numbers */
   unregister_chrdev_region(MKDEV(S_TT_MAJOR, S_TT_MINOR_START),
      S_TT_MINOR_MAX);
}
module_exit(s_tt_shutdown);

/********************************************************************
*
*       Routine : s_tt_open
*
*  Description  : Handles all the open() commands issued to the driver
*                 from user land.
*
*         Input : inode - Pointer to the inode structure for the
*                         device.
*                 filp  - Pointer to the file structure for the
*                         device.
*
*        Output : None
*
*       Returns : int - 0 = no error, < 0 = error
*
*        Author : Steve Turner
*
*       History : Created 26 November 2007
*
********************************************************************/

static int s_tt_open(struct inode *inode, struct file *filp)
{
   int   rc = 0;

   /* We can only open this device for writing. No reading allowed... */
   if ((filp->f_flags & O_ACCMODE) != O_WRONLY)
   {
      rc = -EINVAL;
   } /* if ((filp->f_flags & O_ACCMODE) != O_WRONLY) */

   return rc;
}

/********************************************************************
*
*       Routine : s_tt_release
*
*  Description  : Handles all the release() commands issued to the
*                 driver from user land.
*
*         Input : inode - Pointer to the inode structure for the
*                         device.
*                 filp  - Pointer to the file structure for the
*                         device.
*
*        Output : None
*
*       Returns : int - 0 = no error.
*
*        Author : Steve Turner
*
*       History : Created 26 November 2007
*
********************************************************************/

static int s_tt_release(struct inode *inode, struct file *filp)
{
   return 0;
}

/********************************************************************
*
*       Routine : s_tt_write
*
*  Description  : Handles all the write() commands issued to the
*                 driver from user land.
*
*         Input : filp  - Pointer to the file structure for the
*                         device.
*                 buff  - Pointer to the buffer containing the
*                         characters to write.
*                 count - The size of the buffer.
*
*        Output : offp  - Returned offset.
*
*       Returns : ssize_t - >= 0 - no error, < 0 - error.
*
*        Author : Steve Turner
*
*       History : Created 26 November 2007
*
********************************************************************/

static ssize_t s_tt_write(struct file *filp, const char __user *buff, size_t count, loff_t *offp)
{
   return pace_do_tty_forced_write(buff, count);
}

/********************************************************************
*
*       Routine : s_tt_ioctl
*
*  Description  : Handles all the ioctl commands issued to the driver
*                 from user land.
*
*         Input : inode - Pointer to the inode structure for the
*                         device.
*                 filp  - Pointer to the file structure for the
*                         device.
*                 cmd   - The command identifier.
*                 arg   - Command arguments.
*
*        Output : None
*
*       Returns : int - 0 = no error.
*
*        Author : Steve Turner
*
*       History : Created 3 December 2007
*
********************************************************************/

static long s_tt_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
   int               rc = 0;

   union {
      unsigned long  arg;
      void __user    *argp;
   } u;

   int               serial_ctrl;

   lock_kernel();

   u.arg = arg;

   /* Check that the command contains our magic number */
   if (_IOC_TYPE(cmd) == S_TT_DEV_TYPE)
   {
      /* Call access_ok() to perform address verification without
         transferring any data */
      if (_IOC_DIR(cmd) & _IOC_READ)
      {
         rc = !access_ok(VERIFY_WRITE, u.argp, _IOC_SIZE(cmd));
      }
      else if (_IOC_DIR(cmd) & _IOC_WRITE)
      {
         rc = !access_ok(VERIFY_READ, u.argp, _IOC_SIZE(cmd));
      } /* if (_IOC_DIR(cmd) & _IOC_READ) */

      if (rc != 0)
      {
         /* access_ok() failed */
         rc = -EFAULT;
      } /* if (rc != 0) */

   }
   else
   {
      /* The command does not contain our magic number */
      rc = -EAGAIN;
   } /* if (_IOC_TYPE(cmd) == S_TT_DEV_TYPE) */

   if (rc == 0)
   {
      switch (cmd)
      {
         case S_TT_DEV_SERIAL_CTRL:
            serial_ctrl = arg;
            /* Switch serial output either on or off */
            pace_do_tty_console_control(serial_ctrl);
            break;
         default:
            /* Unrecognised command */
            rc = -EAGAIN;
            break;
      } /* switch (cmd) */

   } /* if (rc == 0) */

   unlock_kernel();

   return rc;
}

#endif /* #ifdef CONFIG_PACE_RS232_SILENT */
