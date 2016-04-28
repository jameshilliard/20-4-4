/*
 * gen-ioctl: Ioctl interface for generic ioctl calls
 * 
 * Copyright 2010 TiVo Inc. All Rights Reserved.
 */

#include <linux/init.h>
#include <linux/mm.h>
#include <linux/fs.h>
#include <linux/module.h>
#include <asm/uaccess.h>
#include <linux/slab.h>
#include <linux/tivo-genioctl.h>
#include <linux/tivo-crashallassert.h>
#include <asm/scatterlist.h>
#include <linux/uio.h>
#include <linux/pagemap.h> 
#include <linux/sgat-dio.h>

#ifdef CONFIG_TIVO_SATA_HOTPLUG
#include <asm/bcm7405.h>
#endif

#define SKIP_SATA_PM_FW_UPGD 1

int (*upgd_fw_init) (void) = NULL;
int (*is_fw_upgd_reqd) (void) = NULL;

#ifdef CONFIG_KSTACK_GUARD
extern void test_kstack_guard_check(struct task_struct *p);
#endif

#define GIOCTL_MAJOR 243

#ifdef CONFIG_TIVO_SATA_HOTPLUG

#define NUM_SATA_PORTS      2
#define MMIO_SATA_STATUS    0x40
#define PORT_OFFSET         0x100
#define PHY_IS_RDY          0x3
#define PHY_IN_BIST         0x4


void mmio_outl(unsigned long port, unsigned int val)
{
	/* Need to swap because the output is in little endiam mode
	 * even though PCI controller is configured in big-endian.
	 */
	*(volatile unsigned int *)( ((unsigned int)SATA_MEM_BASE) + (port)) = swab32(val);
}

unsigned int mmio_inl(unsigned long port)
{
	/* Need to swap because the output is in little endiam mode
	 * even though PCI controller is configured in big-endian.
	 */
	return swab32(*(volatile unsigned int *)(((unsigned int) SATA_MEM_BASE)+ port));
}

#endif

int gioctl_open(struct inode *inode, struct file *filp)
{
    return 0;
}

int gioctl_ioctl(struct inode *inode, struct file *file, unsigned int cmd, unsigned long arg)
{
    if (!arg)
    {
        printk(KERN_ERR "[gen_ioctl] Invalid arg\n");
        return -EINVAL;
    }

    if (_IOC_TYPE(cmd) != GIOCTL_IOC_MAGIC)
    {
        printk(KERN_ERR "[gen_ioctl] cmd (0x%x) mismatch, " "called with 0x%x while expecting 0x%x\n", cmd, _IOC_TYPE(cmd),
               GIOCTL_IOC_MAGIC);
        return -EINVAL;
    }

    switch (cmd)
    {
#ifdef CONFIG_TIVO_MOJAVE
#include <asm/brcmstb/common/brcmstb.h>
        case GO_BIST:
        {
                extern void emergency_sync(void);
                extern unsigned long get_RAM_size();
                unsigned long *p = (unsigned long *) (KSEG1+get_RAM_size()-PAGE_SIZE);

                /*
                 * flush all disk buffers
                 */ 
                emergency_sync();

                memset(p,0,PAGE_SIZE);
                *p++ = 0x42495354; /*BIST*/
                *p++ = 1; /*Version*/
                strcpy(p, arg); /*Boot Parameters*/
                
                /*Disable Interrupts*/
                local_irq_disable();
                /*soft reset*/
                BDEV_SET(BCHP_SUN_TOP_CTRL_RESET_CTRL, BCHP_SUN_TOP_CTRL_RESET_CTRL_master_reset_en_MASK);
                BDEV_RD(BCHP_SUN_TOP_CTRL_RESET_CTRL);

                BDEV_SET(BCHP_SUN_TOP_CTRL_SW_RESET, BCHP_SUN_TOP_CTRL_SW_RESET_chip_master_reset_MASK);
                BDEV_RD(BCHP_SUN_TOP_CTRL_SW_RESET);

                while(1);
                
        }
#endif
        case CRASHALLASSRT:
            return tivocrashallassert(arg);

#ifdef CONFIG_TICK_KILLER
        case TICK_KILLER:
            return tick_killer_start(arg);
#endif

        case UPDATE_EXEC_CTX:
            /*Ineffective ioctl. Handling for the backward compatibility purposes.
             * Has be removed.*/
            return 0;

#ifdef CONFIG_KSTACK_GUARD
        case TEST_KSTACK_GUARD:
            test_kstack_guard_check(current);
            return 0;
#endif
        case TEST_SGAT_DIO:
        {
            struct sgat_dio_fops gfops;
            sgat_dio_req dio_req;
            int c;
            int k = 'z';

            sgat_dio_get_default_fops(&gfops);

            if (sgat_dio_init_req(&gfops, &dio_req, (sgat_dio_hdr_t *) arg))
            {
                printk("Error: sgat_dio_init_req\n");
                return -EINVAL;
            }

            if (sgat_dio_build_sg_list(&gfops, &dio_req, 1))
            {
                printk("Error: sgat_dio_build_sg_list\n");
                return -EINVAL;
            }

            struct scatterlist *p = (struct scatterlist *)dio_req.sclist;
            for (c = 0; c < dio_req.k_useg; c++)
            {
                memset(p->address, k, p->length);
                p++;
            }
            sgat_dio_exit_req(&gfops, &dio_req, 1);
            return 0;
        }
#ifdef CONFIG_TIVO_SATA_HOTPLUG
        case SATA_UNPLUG_INT:
        {
            /* This is a blocking call */
            int port, offset, regvalue;

            port = 1;                            /* Currently hard coding to port 1 */

            if (port >= NUM_SATA_PORTS)
                return -EINVAL;

            /* SATA STATUS port */
            offset = MMIO_SATA_STATUS + port * PORT_OFFSET;

            /* Check the status */
            regvalue = mmio_inl(offset);
            regvalue &= PHY_IN_BIST;

            if (regvalue)
                return 0;

            /* If user request for non blocking */
            if (arg & 0x2)
            {
                /* Any positive number is treated error output of command (not ioctl error).
                 */
                return 1;
            }

            /* Now we have to wait for interrupt */
            //return sata_wait_for_IRQ(port, offset, PHY_IN_BIST);
            return 1;

        }

        case SATA_PLUG_INT:
        {
            /* This is a blocking call */
            int port, offset, regvalue;

            port = 1;                            /* Currently hard coding to port 1 */

            if (port >= NUM_SATA_PORTS)
                return -EINVAL;

            /* SATA STATUS port */
            offset = MMIO_SATA_STATUS + port * PORT_OFFSET;

            /* Check the status */
            regvalue = mmio_inl(offset);
            regvalue &= PHY_IS_RDY;

            if (regvalue)
                return 0;

            /* If user request for non blocking */
            if (arg & 0x2)
            {
                /* Any positive number is treated error output of command (not ioctl error).
                 */
                return 1;
            }
            /* Now we have to wait for interrupt */
            //return sata_wait_for_irq(port, offset, PHY_IS_RDY);
            return 1;
        }
#endif
        case LOCKMEM:
        {
            //printk("Pretending to LOCKMEM %p\n", (void *)arg);
            //struct iovec *iovec = (struct iovec *) arg;
            //if ( pindown_pages(NULL, iovec, 1, 1, WRITE) )
            //    return -EINVAL;
            return 0;
        }
        
        case UNLOCKMEM:
        {
            printk("Pretending to UNLOCKMEM %p\n", (void *)arg);
            // struct iovec *iovec = (struct iovec *) arg;
            // struct page **page_list;
            // struct page **pg_list;
            // unsigned int pg_addr;
            // int pgcount,pg;

            // pgcount = (((u_long)iovec->iov_base) + 
            //     iovec->iov_len + PAGE_SIZE - 1)/PAGE_SIZE - 
            //     ((u_long)iovec->iov_base)/PAGE_SIZE;
            // pg_addr = (unsigned int )(iovec->iov_base) & (~(PAGE_SIZE-1));
            // page_list = kmalloc(sizeof(struct page **) * pgcount, GFP_KERNEL);
            // pg_list = page_list;
            // for ( pg = 0; pg < pgcount; pg++){
            //     *(pg_list++) = virt_to_page(pg_addr);
            //     pg_addr += PAGE_SIZE;
            // }
            // unlock_pages(page_list);
            // kfree(*page_list);
            return 0;
        }

        case IS_PM_FWUPGD_REQD:
        {
            int ret = -EINVAL;
#if SKIP_SATA_PM_FW_UPGD
            return -EACCES;
#endif
#ifdef CONFIG_TIVO_DEVEL
            extern void sw_watchdog_threads_hold(void);
            extern void sw_watchdog_threads_release(void);

            /* Turn off sw watchdog for a while */
            sw_watchdog_threads_hold();
#endif
            if (is_fw_upgd_reqd)
                ret = is_fw_upgd_reqd();
#ifdef CONFIG_TIVO_DEVEL
            sw_watchdog_threads_release();
#endif
            return ret;
        }

        case DWNLD_SATA_PM_FW:
        {
            int ret = -EINVAL;
#if SKIP_SATA_PM_FW_UPGD
            return -EACCES;
#endif
#ifdef CONFIG_TIVO_DEVEL
            extern void sw_watchdog_threads_hold(void);
            extern void sw_watchdog_threads_release(void);
            /* Turn off sw watchdog for a while until we complete
             * upgrading FW in blocking PIO mode
             */
            sw_watchdog_threads_hold();
#endif
            if (upgd_fw_init)
                ret = upgd_fw_init();
#ifdef CONFIG_TIVO_DEVEL
            /*Trun on sw wdog */
            sw_watchdog_threads_release();
#endif
            return ret;
        }

        case GET_VMA_INFO:
        {
            struct vma_info_struct s;
            
            copy_from_user(&s,(void *)arg,sizeof(s));

            down_read(&current->mm->mmap_sem);
            struct vm_area_struct *vma = find_vma(current->mm, s.address);
            
            if (!vma || vma->vm_start > s.address)
            {
                up_read(&current->mm->mmap_sem);
                return -ENOMEM;
            }    

            s.start = vma->vm_start;
            s.end = vma->vm_end-1;
            s.read = (vma->vm_flags & VM_READ)!=0;
            s.write = (vma->vm_flags & VM_WRITE)!=0;
            s.exec = (vma->vm_flags & VM_EXEC)!=0;
            s.shared = (vma->vm_flags & VM_SHARED)!=0;
            s.offset = vma->vm_pgoff*PAGE_SIZE;
            s.device=s.inode=0;
            if (vma->vm_file)
            {
                s.device=vma->vm_file->f_dentry->d_inode->i_rdev;
                s.inode=vma->vm_file->f_dentry->d_inode->i_ino;
            } 
            
            up_read(&current->mm->mmap_sem);
            copy_to_user((void *)arg, &s, sizeof(s));
            return 0;
        }

    }

    // none of the above...
    printk(KERN_ERR "gen_ioctl: Invalid command\n");
    return -EINVAL;
}

static struct file_operations gioctl_fops = {
  ioctl:gioctl_ioctl,
  open:gioctl_open,
};

static int init_gioctl_if(void)
{
    int result;
    result = register_chrdev(GIOCTL_MAJOR, "gioctl", &gioctl_fops);
    if (result < 0)
        printk(KERN_ERR "gioctl: " "failed to register char dev [major %d]\n", GIOCTL_MAJOR);
    return result;    
}

module_init(init_gioctl_if);
