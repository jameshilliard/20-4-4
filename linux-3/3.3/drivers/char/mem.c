/*
 *  linux/drivers/char/mem.c
 *
 *  Copyright (C) 1991, 1992  Linus Torvalds
 *
 *  Added devfs support.
 *    Jan-11-1998, C. Scott Ananian <cananian@alumni.princeton.edu>
 *  Shared /dev/zero mmapping support, Feb 2000, Kanoj Sarcar <kanoj@sgi.com>
 */

#include <linux/mm.h>
#include <linux/miscdevice.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>
#include <linux/mman.h>
#include <linux/random.h>
#include <linux/init.h>
#include <linux/raw.h>
#include <linux/tty.h>
#include <linux/capability.h>
#include <linux/ptrace.h>
#include <linux/device.h>
#include <linux/highmem.h>
#include <linux/crash_dump.h>
#include <linux/backing-dev.h>
#include <linux/bootmem.h>
#include <linux/splice.h>
#include <linux/pfn.h>
#include <linux/export.h>

#include <asm/uaccess.h>
#include <asm/io.h>

#ifdef CONFIG_IA64
# include <linux/efi.h>
#endif

#ifdef CONFIG_CONTIGMEM
#include <linux/contigmem.h>
#include <linux/bootmem.h>
#include <linux/tivoconfig.h>
#include <linux/brcmstb/brcmapi.h>

extern void _end;

#ifdef CONFIG_TIVO_DEVEL
// Amount of memory to protect after each (non-zero sized) contigmem region
#define CONTIGMEM_BARRIER_SIZE                  PAGE_SIZE
// 128 KB process log
#define DEFAULT_CONTIGMEM_PLOG_SIZE             (128 * 1024)
#else
// For release, no plog and no contigmem barriers
#define CONTIGMEM_BARRIER_SIZE                 ( 0 )
#define DEFAULT_CONTIGMEM_PLOG_SIZE            ( 0 )
#endif

// Guard signature to fill contigmem barrier with
#define CONTIGMEM_BARRIER_BYTE                  (0xCB)

#define CONTIGMEM_MINLEFT                       (4 * 1024 * 1024)

unsigned long contigmem_start = 0;
unsigned long contigmem_top = 0;
unsigned long contigmem_size[MAX_CONTIGMEM_REGION + 1];
unsigned long contigmem_barrier = CONTIGMEM_BARRIER_SIZE;

static int __init contigmem_setup(char *str)
{
        unsigned long override_size;
        int region = -1;
        char *dummy;

        if (!str || *str == '\0') {
            return 0;
        }

        // Supports original format (contigmem=YYY) to override
        // total amount of contigmem. Excess is added to last region
        if (*str == '=') {
            str++;
            override_size = (unsigned long)memparse(str, &dummy);
            SetTivoConfig(kTivoConfigContigmemSize, override_size);
        }
        // Alternative format where you can override individual regions
        // using "contigmemX=YYY". Regions are numbered from 0
        else {
            enum TivoConfigSelectors tcs;

            if (sscanf(str, "%d=", &region) != 1) {
                return 0;
            }
            if (region < 0 || region > MAX_CONTIGMEM_REGION) {
                return 0;
            }
            while (*str != '=') {
                str++;
            }
            str++;

            override_size = (unsigned long)memparse(str, &dummy);

            // don't look!
            tcs = (enum TivoConfigSelectors)((unsigned int)kTivoConfigContigmemRegion0 + region);
            SetTivoConfig(tcs, override_size);
        }
        return 1;
}

static int __init contigbarrier_setup(char *str)
{
        unsigned long override_size;
        char *dummy;

        override_size = (unsigned long)memparse(str, &dummy);
        contigmem_barrier = PAGE_ALIGN(override_size);
        return 1;
}

#ifdef CONFIG_TIVO_DEVEL
#include <linux/proc_fs.h>

unsigned long reservedmem_start=0;
unsigned long reservedmem_size=0;

static int reservedmem_proc_read(char* buffer, char** start, off_t offset,
                                 int size, int* eof, void* data) {
        int len;
        if(size < 26)
                return -EINVAL;
        *eof= 1;
        len= snprintf(buffer, 26,  "Reserved pages: %d\n", (int)reservedmem_size);
        return len;
}

void free_reserved_range(unsigned long, unsigned long);

static int reservedmem_proc_write(struct file* filp, const char* buffer,
                                  unsigned long count, void* data) {
        char buf[11];
        int toreserve, tofree;
        if(count>10)
                return -EINVAL;
        copy_from_user(buf, buffer, count);
        buf[count]='\0';
        toreserve=simple_strtoul(buf, 0, 10);
        tofree=reservedmem_size - toreserve;
        printk("Attempting to free %dk\n", tofree * 4);
#ifdef XXX_FIXME
        if(tofree <= reservedmem_size) {
                /*
                 * Yes, there should be some locking around this, but
                 * this is not a multiple simultaneous request kind
                 * of deal.  If we decide to implement bottom allocation
                 * we'll need to implement correct locking here.
                 */
                free_reserved_range(reservedmem_start,
                                    reservedmem_start + (tofree * PAGE_SIZE));
                reservedmem_start+=tofree * PAGE_SIZE;
                reservedmem_size-=tofree;
                printk("Freeing initial reserved mem: %ldk freed\n",
                       tofree * PAGE_SIZE / 1024);
        }
#else
        printk("%s:%d: NOT IMPLEMENTED!\n", __FILE__, __LINE__);
#endif
        return count;
}

static int __init reservedmem_proc_init(void) {
        struct proc_dir_entry* reservedmem_proc_entry = create_proc_entry(
                "reservedmem", S_IRUGO |S_IWUSR, NULL);
        if(reservedmem_proc_entry) {
                reservedmem_proc_entry->read_proc=reservedmem_proc_read;
                reservedmem_proc_entry->write_proc=reservedmem_proc_write;
        }
        return 0;
}

module_init(reservedmem_proc_init);

void __init reserve_excess_platform_memory(unsigned long contigmemend) {
        /* Reserve 32MB to keep release-mips running with the
           same ammount of RAM.  This is our 64MB devel
           boxes dealy bob.  A syscall will come later to free
           up this memory if appropriate.
         */
        unsigned long mem_end;
        unsigned long mem_excess;
        unsigned long physmem_top = max_low_pfn * PAGE_SIZE;
        contigmemend=(contigmemend + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);
        if( GetTivoConfig( kTivoConfigMemSize, &mem_end ) != 0 )
        {
            mem_end = 32 * 1024 * 1024;
        }

        if(physmem_top > mem_end) {
            printk("top = %ld, mem = %ld\n", physmem_top, mem_end);
                /* We have more memory than we should on this platform,
                   reserve the excess.
                */

                /* Round physmem_top up to nearest 32M, this is because
                   of math games played by bcm-setup */
                physmem_top+=(32 * 1024 * 1024) - 1;
                physmem_top=physmem_top & ~(32 * 1024 *1024 - 1);
                mem_excess=physmem_top-mem_end;
                if(contigmemend + mem_excess > (unsigned long)__va(physmem_top)) {
                        printk("\t**ERROR**: Unable to reserve excess memory "
                               "for platform!\n");
                        return;
                }

                printk("Amount of memory allowed by platform exceeded, "
                       "reserving excess %lu MB\n", mem_excess/(1024*1024));
                reserve_bootmem(__pa(contigmemend), mem_excess, BOOTMEM_DEFAULT);
                reservedmem_start=contigmemend;
                reservedmem_size=mem_excess/PAGE_SIZE;
        }
}

#endif

__setup("contigmem", contigmem_setup);
__setup("contigbarrier=", contigbarrier_setup);

void __init contigmem(void) {
        unsigned long physmem_top = max_low_pfn * PAGE_SIZE;
        unsigned long cm_offset;
        unsigned long contigmem_total = 0;
        unsigned long bmem_region_start, bmem_region_size;
        TivoConfigValue contigmem_set;
        int i, cms_top = 0;
        int res;

        printk("physmem_top = 0x%lx\n", physmem_top);

        /* Cap to 256MB for this calculation */
        if (physmem_top > 256 * 1024 * 1024) {
                contigmem_top = (unsigned long)__va(256 * 1024 * 1024);
        } else {
                contigmem_top = (unsigned long)__va(physmem_top);
        }

        printk("Preliminary contigmem_top = 0x%08lx\n", contigmem_top);

        contigmem_size[CONTIGMEM_REGION_PLOG] = DEFAULT_CONTIGMEM_PLOG_SIZE;

        /*
         * If the query fails on the plog region, just use the
         * hardcoded default. This is the normal case.
         */
        for (i = 0; i <= MAX_CONTIGMEM_REGION; i++) {
                if (!GetTivoConfig(kTivoConfigContigmemRegion0 + i,
                                   &contigmem_size[i])) {
                        cms_top = i;
                }
                contigmem_total += contigmem_size[i];
                if (contigmem_size[i]) {
                        // place barrier after a nonzero-sized region
                        contigmem_total += contigmem_barrier;
                }
        }
        /* Add any excess to the last defined region */
        if (!GetTivoConfig(kTivoConfigContigmemSize, &contigmem_set) &&
            contigmem_set != contigmem_total &&
            contigmem_set + contigmem_size[cms_top] >= contigmem_total) {
                contigmem_size[cms_top] += contigmem_set - contigmem_total;
                printk("Adjusted region %d to %ld bytes\n",
                        cms_top, contigmem_size[cms_top]);
                contigmem_total = contigmem_set;
        }

        /* Check for collisions with bmem */
        for (i = 0; contigmem_top > contigmem_total; i++) {
                if (bmem_region_info(i, &bmem_region_start, &bmem_region_size)) {
                        /* No more regions to check */
                        break;
                }
                if ((contigmem_top - contigmem_total <= bmem_region_start + bmem_region_size) &&
                    (contigmem_top > bmem_region_start)) {
                        contigmem_top = bmem_region_start;
                        printk("Adjusted contigmem_top to 0x%08lx due to collision with bmem region %d\n",
                            contigmem_top, i);
                        /* Start over, in case we now collide with an earlier region */
                        i = -1;
                        continue;
                }
        }

        /* Remainder of logic works with virtual addrs */
//        contigmem_top = (unsigned long)__va(contigmem_top);

        /* Page align the start address, rounding down */
        contigmem_start = (contigmem_top - contigmem_total) & PAGE_MASK;
        if (contigmem_total < contigmem_top - contigmem_start) {
                contigmem_size[cms_top] += contigmem_top - contigmem_start -
                                           contigmem_total;
                contigmem_total = contigmem_top - contigmem_start;
        }

        /* This is an approximate check at best... */
        if (contigmem_start < (unsigned long)&_end + CONTIGMEM_MINLEFT) {
                printk("Insufficient real memory to allocate contiguous region.\n");
                goto err_out;
        }

        /* In case it wasn't set, update the CMSZ config entry */
        SetTivoConfig(kTivoConfigContigmemSize, contigmem_total);
        res = reserve_bootmem(__pa(contigmem_start), contigmem_total, BOOTMEM_DEFAULT);
        if (res) {
                printk("Unable to allocate contigmem (%d).\n", res);
                goto err_out;
        }

        /* Print offsets and initialize barrier signature */
        cm_offset = contigmem_start;
        for (i = 0; i <= cms_top; ++i) {
            if (contigmem_size[i]) {
                printk("Contiguous region %d: %ld bytes @ address 0x%lx\n",
                       i, contigmem_size[i], cm_offset);
                cm_offset += contigmem_size[i];
                if (contigmem_barrier) {
                    memset((void *)cm_offset, CONTIGMEM_BARRIER_BYTE, contigmem_barrier);
                    printk("Contigmem barrier %ld bytes at 0x%lx.\n",
                           contigmem_barrier, cm_offset);
                    cm_offset += contigmem_barrier;
                }
            }
        }

        printk("Contiguous region of %ld bytes total reserved at 0x%lx.\n",
               contigmem_total, contigmem_start);

        return;

err_out:
        contigmem_start = 0;
        contigmem_top = 0;
        for (i = 0; i <= MAX_CONTIGMEM_REGION; i++) {
                contigmem_size[i] = 0;
        }
}

// This is the exported interface for modules
//
// Regions are numbered beginning from 0
//
int mem_get_contigmem_region(unsigned int region, unsigned long *start,
                             unsigned long *size)
{
        unsigned int i;

        if (region <= MAX_CONTIGMEM_REGION) {
                *start = contigmem_start;
                for (i = 0; i < region; i++) {
                        *start += contigmem_size[i];
                        // account for the barrier between each non empty region
                        if (contigmem_size[i]) {
                            *start += contigmem_barrier;
                        }
                }
                *size = contigmem_size[region];

                return 0;
        } else {
            printk("Invalid contigmem region!\n");
                return -EINVAL;
        }
}
EXPORT_SYMBOL(mem_get_contigmem_region);

// Check if address range falls within contigmem memory
int contigmem_check(unsigned long addr, unsigned long size)
{
        return (addr >= (unsigned long)__pa(contigmem_start)) &&
               (addr + size <= (unsigned long)__pa(contigmem_top));
}

static long mem_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
        unsigned int region;
        unsigned long region_start = 0;
        unsigned long region_size = 0;
        struct contigmem_region_request *request =
            (struct contigmem_region_request *)arg;
        int ret;

        if ( contigmem_start == 0 )
                return -ENOMEM;

        switch (_IOC_NR(cmd)) {
                /* 1,2,4,5 are compatibiliy cruft... */
                case 1:
                case 2:
                        region = 1;
                        break;
                case 4:
                case 5:
                        region = 2;
                        break;
                default:
                        if ( get_user( region, &request->region ) < 0 )
                                return -EFAULT;
                        break;
        }

        ret = mem_get_contigmem_region( region, &region_start, &region_size );
        if ( ret < 0 )
                return ret;

        region_start = virt_to_phys( (void *)region_start );

        if ( cmd == (CONTIGMEM_GET_REGION) ) {
                if ( put_user( region_start, &request->phys_start ) < 0 ||
                     put_user( region_size, &request->size ) < 0 )
                        return -EFAULT;
                return 0;
        } else {
                /* even more compatibility cruft */
                switch (_IOC_NR(cmd)) {
                        case 1:
                        case 4:
                                return (int)region_start;
                        case 2:
                        case 5:
                                return (int)region_size;
                        default:
                                break;
                }
        }

        return -EINVAL;
}
#endif /* CONFIG_CONTIGMEM */

static inline unsigned long size_inside_page(unsigned long start,
					     unsigned long size)
{
	unsigned long sz;

	sz = PAGE_SIZE - (start & (PAGE_SIZE - 1));

	return min(sz, size);
}

#ifndef ARCH_HAS_VALID_PHYS_ADDR_RANGE
static inline int valid_phys_addr_range(unsigned long addr, size_t count)
{
	return addr + count <= __pa(high_memory);
}

static inline int valid_mmap_phys_addr_range(unsigned long pfn, size_t size)
{
	return 1;
}
#endif

#ifdef CONFIG_STRICT_DEVMEM
static inline int range_is_allowed(unsigned long pfn, unsigned long size)
{
	u64 from = ((u64)pfn) << PAGE_SHIFT;
	u64 to = from + size;
	u64 cursor = from;

	while (cursor < to) {
		if (!devmem_is_allowed(pfn)) {
			printk(KERN_INFO
		"Program %s tried to access /dev/mem between %Lx->%Lx.\n",
				current->comm, from, to);
			return 0;
		}
		cursor += PAGE_SIZE;
		pfn++;
	}
	return 1;
}
#else
static inline int range_is_allowed(unsigned long pfn, unsigned long size)
{
	return 1;
}
#endif

void __weak unxlate_dev_mem_ptr(unsigned long phys, void *addr)
{
}

/*
 * This funcion reads the *physical* memory. The f_pos points directly to the
 * memory location.
 */
static ssize_t read_mem(struct file *file, char __user *buf,
			size_t count, loff_t *ppos)
{
	unsigned long p = *ppos;
	ssize_t read, sz;
	char *ptr;

	if (!valid_phys_addr_range(p, count))
		return -EFAULT;
	read = 0;
#ifdef __ARCH_HAS_NO_PAGE_ZERO_MAPPED
	/* we don't have page 0 mapped on sparc and m68k.. */
	if (p < PAGE_SIZE) {
		sz = size_inside_page(p, count);
		if (sz > 0) {
			if (clear_user(buf, sz))
				return -EFAULT;
			buf += sz;
			p += sz;
			count -= sz;
			read += sz;
		}
	}
#endif

	while (count > 0) {
		unsigned long remaining;

		sz = size_inside_page(p, count);

		if (!range_is_allowed(p >> PAGE_SHIFT, count))
			return -EPERM;

		/*
		 * On ia64 if a page has been mapped somewhere as uncached, then
		 * it must also be accessed uncached by the kernel or data
		 * corruption may occur.
		 */
		ptr = xlate_dev_mem_ptr(p);
		if (!ptr)
			return -EFAULT;

		remaining = copy_to_user(buf, ptr, sz);
		unxlate_dev_mem_ptr(p, ptr);
		if (remaining)
			return -EFAULT;

		buf += sz;
		p += sz;
		count -= sz;
		read += sz;
	}

	*ppos += read;
	return read;
}

static ssize_t write_mem(struct file *file, const char __user *buf,
			 size_t count, loff_t *ppos)
{
	unsigned long p = *ppos;
	ssize_t written, sz;
	unsigned long copied;
	void *ptr;

	if (!valid_phys_addr_range(p, count))
		return -EFAULT;

	written = 0;

#ifdef __ARCH_HAS_NO_PAGE_ZERO_MAPPED
	/* we don't have page 0 mapped on sparc and m68k.. */
	if (p < PAGE_SIZE) {
		sz = size_inside_page(p, count);
		/* Hmm. Do something? */
		buf += sz;
		p += sz;
		count -= sz;
		written += sz;
	}
#endif

	while (count > 0) {
		sz = size_inside_page(p, count);

		if (!range_is_allowed(p >> PAGE_SHIFT, sz))
			return -EPERM;

		/*
		 * On ia64 if a page has been mapped somewhere as uncached, then
		 * it must also be accessed uncached by the kernel or data
		 * corruption may occur.
		 */
		ptr = xlate_dev_mem_ptr(p);
		if (!ptr) {
			if (written)
				break;
			return -EFAULT;
		}

		copied = copy_from_user(ptr, buf, sz);
		unxlate_dev_mem_ptr(p, ptr);
		if (copied) {
			written += sz - copied;
			if (written)
				break;
			return -EFAULT;
		}

		buf += sz;
		p += sz;
		count -= sz;
		written += sz;
	}

	*ppos += written;
	return written;
}

int __weak phys_mem_access_prot_allowed(struct file *file,
	unsigned long pfn, unsigned long size, pgprot_t *vma_prot)
{
	return 1;
}

#ifndef __HAVE_PHYS_MEM_ACCESS_PROT

/*
 * Architectures vary in how they handle caching for addresses
 * outside of main memory.
 *
 */
#ifdef pgprot_noncached
static int uncached_access(struct file *file, unsigned long addr)
{
#if defined(CONFIG_IA64)
	/*
	 * On ia64, we ignore O_DSYNC because we cannot tolerate memory
	 * attribute aliases.
	 */
	return !(efi_mem_attributes(addr) & EFI_MEMORY_WB);
#elif defined(CONFIG_MIPS)
	{
		extern int __uncached_access(struct file *file,
					     unsigned long addr);

		return __uncached_access(file, addr);
	}
#else
	/*
	 * Accessing memory above the top the kernel knows about or through a
	 * file pointer
	 * that was marked O_DSYNC will be done non-cached.
	 */
	if (file->f_flags & O_DSYNC)
		return 1;
	return addr >= __pa(high_memory);
#endif
}
#endif

static pgprot_t phys_mem_access_prot(struct file *file, unsigned long pfn,
				     unsigned long size, pgprot_t vma_prot)
{
#ifdef pgprot_noncached
	unsigned long offset = pfn << PAGE_SHIFT;

	if (uncached_access(file, offset))
		return pgprot_noncached(vma_prot);
#endif
	return vma_prot;
}
#endif

#ifndef CONFIG_MMU
static unsigned long get_unmapped_area_mem(struct file *file,
					   unsigned long addr,
					   unsigned long len,
					   unsigned long pgoff,
					   unsigned long flags)
{
	if (!valid_mmap_phys_addr_range(pgoff, len))
		return (unsigned long) -EINVAL;
	return pgoff << PAGE_SHIFT;
}

/* can't do an in-place private mapping if there's no MMU */
static inline int private_mapping_ok(struct vm_area_struct *vma)
{
	return vma->vm_flags & VM_MAYSHARE;
}
#else
#define get_unmapped_area_mem	NULL

static inline int private_mapping_ok(struct vm_area_struct *vma)
{
	return 1;
}
#endif

static const struct vm_operations_struct mmap_mem_ops = {
#ifdef CONFIG_HAVE_IOREMAP_PROT
	.access = generic_access_phys
#endif
};

static int mmap_mem(struct file *file, struct vm_area_struct *vma)
{
	size_t size = vma->vm_end - vma->vm_start;

	if (!valid_mmap_phys_addr_range(vma->vm_pgoff, size))
		return -EINVAL;

	if (!private_mapping_ok(vma))
		return -ENOSYS;

	if (!range_is_allowed(vma->vm_pgoff, size))
		return -EPERM;

	if (!phys_mem_access_prot_allowed(file, vma->vm_pgoff, size,
						&vma->vm_page_prot))
		return -EINVAL;

	vma->vm_page_prot = phys_mem_access_prot(file, vma->vm_pgoff,
						 size,
						 vma->vm_page_prot);

	vma->vm_ops = &mmap_mem_ops;

	/* Remap-pfn-range will mark the range VM_IO and VM_RESERVED */
	if (remap_pfn_range(vma,
			    vma->vm_start,
			    vma->vm_pgoff,
			    size,
			    vma->vm_page_prot)) {
		return -EAGAIN;
	}
	return 0;
}

#ifdef CONFIG_DEVKMEM
static int mmap_kmem(struct file *file, struct vm_area_struct *vma)
{
	unsigned long pfn;

	/* Turn a kernel-virtual address into a physical page frame */
	pfn = __pa((u64)vma->vm_pgoff << PAGE_SHIFT) >> PAGE_SHIFT;

	/*
	 * RED-PEN: on some architectures there is more mapped memory than
	 * available in mem_map which pfn_valid checks for. Perhaps should add a
	 * new macro here.
	 *
	 * RED-PEN: vmalloc is not supported right now.
	 */
	if (!pfn_valid(pfn))
		return -EIO;

	vma->vm_pgoff = pfn;
	return mmap_mem(file, vma);
}
#endif

#ifdef CONFIG_CRASH_DUMP
/*
 * Read memory corresponding to the old kernel.
 */
static ssize_t read_oldmem(struct file *file, char __user *buf,
				size_t count, loff_t *ppos)
{
	unsigned long pfn, offset;
	size_t read = 0, csize;
	int rc = 0;

	while (count) {
		pfn = *ppos / PAGE_SIZE;
		if (pfn > saved_max_pfn)
			return read;

		offset = (unsigned long)(*ppos % PAGE_SIZE);
		if (count > PAGE_SIZE - offset)
			csize = PAGE_SIZE - offset;
		else
			csize = count;

		rc = copy_oldmem_page(pfn, buf, csize, offset, 1);
		if (rc < 0)
			return rc;
		buf += csize;
		*ppos += csize;
		read += csize;
		count -= csize;
	}
	return read;
}
#endif

#ifdef CONFIG_DEVKMEM
/*
 * This function reads the *virtual* memory as seen by the kernel.
 */
static ssize_t read_kmem(struct file *file, char __user *buf,
			 size_t count, loff_t *ppos)
{
	unsigned long p = *ppos;
	ssize_t low_count, read, sz;
	char * kbuf; /* k-addr because vread() takes vmlist_lock rwlock */
	int err = 0;

	read = 0;
	if (p < (unsigned long) high_memory) {
		low_count = count;
		if (count > (unsigned long)high_memory - p)
			low_count = (unsigned long)high_memory - p;

#ifdef __ARCH_HAS_NO_PAGE_ZERO_MAPPED
		/* we don't have page 0 mapped on sparc and m68k.. */
		if (p < PAGE_SIZE && low_count > 0) {
			sz = size_inside_page(p, low_count);
			if (clear_user(buf, sz))
				return -EFAULT;
			buf += sz;
			p += sz;
			read += sz;
			low_count -= sz;
			count -= sz;
		}
#endif
		while (low_count > 0) {
			sz = size_inside_page(p, low_count);

			/*
			 * On ia64 if a page has been mapped somewhere as
			 * uncached, then it must also be accessed uncached
			 * by the kernel or data corruption may occur
			 */
			kbuf = xlate_dev_kmem_ptr((char *)p);

			if (copy_to_user(buf, kbuf, sz))
				return -EFAULT;
			buf += sz;
			p += sz;
			read += sz;
			low_count -= sz;
			count -= sz;
		}
	}

	if (count > 0) {
		kbuf = (char *)__get_free_page(GFP_KERNEL);
		if (!kbuf)
			return -ENOMEM;
		while (count > 0) {
			sz = size_inside_page(p, count);
			if (!is_vmalloc_or_module_addr((void *)p)) {
				err = -ENXIO;
				break;
			}
			sz = vread(kbuf, (char *)p, sz);
			if (!sz)
				break;
			if (copy_to_user(buf, kbuf, sz)) {
				err = -EFAULT;
				break;
			}
			count -= sz;
			buf += sz;
			read += sz;
			p += sz;
		}
		free_page((unsigned long)kbuf);
	}
	*ppos = p;
	return read ? read : err;
}


static ssize_t do_write_kmem(unsigned long p, const char __user *buf,
				size_t count, loff_t *ppos)
{
	ssize_t written, sz;
	unsigned long copied;

	written = 0;
#ifdef __ARCH_HAS_NO_PAGE_ZERO_MAPPED
	/* we don't have page 0 mapped on sparc and m68k.. */
	if (p < PAGE_SIZE) {
		sz = size_inside_page(p, count);
		/* Hmm. Do something? */
		buf += sz;
		p += sz;
		count -= sz;
		written += sz;
	}
#endif

	while (count > 0) {
		char *ptr;

		sz = size_inside_page(p, count);

		/*
		 * On ia64 if a page has been mapped somewhere as uncached, then
		 * it must also be accessed uncached by the kernel or data
		 * corruption may occur.
		 */
		ptr = xlate_dev_kmem_ptr((char *)p);

		copied = copy_from_user(ptr, buf, sz);
		if (copied) {
			written += sz - copied;
			if (written)
				break;
			return -EFAULT;
		}
		buf += sz;
		p += sz;
		count -= sz;
		written += sz;
	}

	*ppos += written;
	return written;
}

/*
 * This function writes to the *virtual* memory as seen by the kernel.
 */
static ssize_t write_kmem(struct file *file, const char __user *buf,
			  size_t count, loff_t *ppos)
{
	unsigned long p = *ppos;
	ssize_t wrote = 0;
	ssize_t virtr = 0;
	char * kbuf; /* k-addr because vwrite() takes vmlist_lock rwlock */
	int err = 0;

	if (p < (unsigned long) high_memory) {
		unsigned long to_write = min_t(unsigned long, count,
					       (unsigned long)high_memory - p);
		wrote = do_write_kmem(p, buf, to_write, ppos);
		if (wrote != to_write)
			return wrote;
		p += wrote;
		buf += wrote;
		count -= wrote;
	}

	if (count > 0) {
		kbuf = (char *)__get_free_page(GFP_KERNEL);
		if (!kbuf)
			return wrote ? wrote : -ENOMEM;
		while (count > 0) {
			unsigned long sz = size_inside_page(p, count);
			unsigned long n;

			if (!is_vmalloc_or_module_addr((void *)p)) {
				err = -ENXIO;
				break;
			}
			n = copy_from_user(kbuf, buf, sz);
			if (n) {
				err = -EFAULT;
				break;
			}
			vwrite(kbuf, (char *)p, sz);
			count -= sz;
			buf += sz;
			virtr += sz;
			p += sz;
		}
		free_page((unsigned long)kbuf);
	}

	*ppos = p;
	return virtr + wrote ? : err;
}
#endif

#ifdef CONFIG_DEVPORT
static ssize_t read_port(struct file *file, char __user *buf,
			 size_t count, loff_t *ppos)
{
	unsigned long i = *ppos;
	char __user *tmp = buf;

	if (!access_ok(VERIFY_WRITE, buf, count))
		return -EFAULT;
	while (count-- > 0 && i < 65536) {
		if (__put_user(inb(i), tmp) < 0)
			return -EFAULT;
		i++;
		tmp++;
	}
	*ppos = i;
	return tmp-buf;
}

static ssize_t write_port(struct file *file, const char __user *buf,
			  size_t count, loff_t *ppos)
{
	unsigned long i = *ppos;
	const char __user * tmp = buf;

	if (!access_ok(VERIFY_READ, buf, count))
		return -EFAULT;
	while (count-- > 0 && i < 65536) {
		char c;
		if (__get_user(c, tmp)) {
			if (tmp > buf)
				break;
			return -EFAULT;
		}
		outb(c, i);
		i++;
		tmp++;
	}
	*ppos = i;
	return tmp-buf;
}
#endif

static ssize_t read_null(struct file *file, char __user *buf,
			 size_t count, loff_t *ppos)
{
	return 0;
}

static ssize_t write_null(struct file *file, const char __user *buf,
			  size_t count, loff_t *ppos)
{
	return count;
}

static int pipe_to_null(struct pipe_inode_info *info, struct pipe_buffer *buf,
			struct splice_desc *sd)
{
	return sd->len;
}

static ssize_t splice_write_null(struct pipe_inode_info *pipe, struct file *out,
				 loff_t *ppos, size_t len, unsigned int flags)
{
	return splice_from_pipe(pipe, out, ppos, len, flags, pipe_to_null);
}

static ssize_t read_zero(struct file *file, char __user *buf,
			 size_t count, loff_t *ppos)
{
	size_t written;

	if (!count)
		return 0;

	if (!access_ok(VERIFY_WRITE, buf, count))
		return -EFAULT;

	written = 0;
	while (count) {
		unsigned long unwritten;
		size_t chunk = count;

		if (chunk > PAGE_SIZE)
			chunk = PAGE_SIZE;	/* Just for latency reasons */
		unwritten = __clear_user(buf, chunk);
		written += chunk - unwritten;
		if (unwritten)
			break;
		if (signal_pending(current))
			return written ? written : -ERESTARTSYS;
		buf += chunk;
		count -= chunk;
		cond_resched();
	}
	return written ? written : -EFAULT;
}

static int mmap_zero(struct file *file, struct vm_area_struct *vma)
{
#ifndef CONFIG_MMU
	return -ENOSYS;
#endif
	if (vma->vm_flags & VM_SHARED)
		return shmem_zero_setup(vma);
	return 0;
}

static ssize_t write_full(struct file *file, const char __user *buf,
			  size_t count, loff_t *ppos)
{
	return -ENOSPC;
}

/*
 * Special lseek() function for /dev/null and /dev/zero.  Most notably, you
 * can fopen() both devices with "a" now.  This was previously impossible.
 * -- SRB.
 */
static loff_t null_lseek(struct file *file, loff_t offset, int orig)
{
	return file->f_pos = 0;
}

/*
 * The memory devices use the full 32/64 bits of the offset, and so we cannot
 * check against negative addresses: they are ok. The return value is weird,
 * though, in that case (0).
 *
 * also note that seeking relative to the "end of file" isn't supported:
 * it has no meaning, so it returns -EINVAL.
 */
static loff_t memory_lseek(struct file *file, loff_t offset, int orig)
{
	loff_t ret;

	mutex_lock(&file->f_path.dentry->d_inode->i_mutex);
	switch (orig) {
	case SEEK_CUR:
		offset += file->f_pos;
	case SEEK_SET:
		/* to avoid userland mistaking f_pos=-9 as -EBADF=-9 */
		if ((unsigned long long)offset >= ~0xFFFULL) {
			ret = -EOVERFLOW;
			break;
		}
		file->f_pos = offset;
		ret = file->f_pos;
		force_successful_syscall_return();
		break;
	default:
		ret = -EINVAL;
	}
	mutex_unlock(&file->f_path.dentry->d_inode->i_mutex);
	return ret;
}

static int open_port(struct inode * inode, struct file * filp)
{
	return capable(CAP_SYS_RAWIO) ? 0 : -EPERM;
}

#define zero_lseek	null_lseek
#define full_lseek      null_lseek
#define write_zero	write_null
#define read_full       read_zero
#define open_mem	open_port
#define open_kmem	open_mem
#define open_oldmem	open_mem

static const struct file_operations mem_fops = {
	.llseek		= memory_lseek,
	.read		= read_mem,
	.write		= write_mem,
#ifdef CONFIG_CONTIGMEM
	.unlocked_ioctl = mem_ioctl,
#endif
	.mmap		= mmap_mem,
	.open		= open_mem,
	.get_unmapped_area = get_unmapped_area_mem,
};

#ifdef CONFIG_DEVKMEM
static const struct file_operations kmem_fops = {
	.llseek		= memory_lseek,
	.read		= read_kmem,
	.write		= write_kmem,
	.mmap		= mmap_kmem,
	.open		= open_kmem,
	.get_unmapped_area = get_unmapped_area_mem,
};
#endif

static const struct file_operations null_fops = {
	.llseek		= null_lseek,
	.read		= read_null,
	.write		= write_null,
	.splice_write	= splice_write_null,
};

#ifdef CONFIG_DEVPORT
static const struct file_operations port_fops = {
	.llseek		= memory_lseek,
	.read		= read_port,
	.write		= write_port,
	.open		= open_port,
};
#endif

static const struct file_operations zero_fops = {
	.llseek		= zero_lseek,
	.read		= read_zero,
	.write		= write_zero,
	.mmap		= mmap_zero,
};

/*
 * capabilities for /dev/zero
 * - permits private mappings, "copies" are taken of the source of zeros
 * - no writeback happens
 */
static struct backing_dev_info zero_bdi = {
	.name		= "char/mem",
	.capabilities	= BDI_CAP_MAP_COPY | BDI_CAP_NO_ACCT_AND_WRITEBACK,
};

static const struct file_operations full_fops = {
	.llseek		= full_lseek,
	.read		= read_full,
	.write		= write_full,
};

#ifdef CONFIG_CRASH_DUMP
static const struct file_operations oldmem_fops = {
	.read	= read_oldmem,
	.open	= open_oldmem,
	.llseek = default_llseek,
};
#endif

static ssize_t kmsg_writev(struct kiocb *iocb, const struct iovec *iv,
			   unsigned long count, loff_t pos)
{
	char *line, *p;
	int i;
	ssize_t ret = -EFAULT;
	size_t len = iov_length(iv, count);

	line = kmalloc(len + 1, GFP_KERNEL);
	if (line == NULL)
		return -ENOMEM;

	/*
	 * copy all vectors into a single string, to ensure we do
	 * not interleave our log line with other printk calls
	 */
	p = line;
	for (i = 0; i < count; i++) {
		if (copy_from_user(p, iv[i].iov_base, iv[i].iov_len))
			goto out;
		p += iv[i].iov_len;
	}
	p[0] = '\0';

	ret = printk("%s", line);
	/* printk can add a prefix */
	if (ret > len)
		ret = len;
out:
	kfree(line);
	return ret;
}

static const struct file_operations kmsg_fops = {
	.aio_write = kmsg_writev,
	.llseek = noop_llseek,
};

static const struct memdev {
	const char *name;
	umode_t mode;
	const struct file_operations *fops;
	struct backing_dev_info *dev_info;
} devlist[] = {
	 [1] = { "mem", 0, &mem_fops, &directly_mappable_cdev_bdi },
#ifdef CONFIG_DEVKMEM
	 [2] = { "kmem", 0, &kmem_fops, &directly_mappable_cdev_bdi },
#endif
	 [3] = { "null", 0666, &null_fops, NULL },
#ifdef CONFIG_DEVPORT
	 [4] = { "port", 0, &port_fops, NULL },
#endif
	 [5] = { "zero", 0666, &zero_fops, &zero_bdi },
	 [7] = { "full", 0666, &full_fops, NULL },
	 [8] = { "random", 0666, &random_fops, NULL },
	 [9] = { "urandom", 0666, &urandom_fops, NULL },
	[11] = { "kmsg", 0, &kmsg_fops, NULL },
#ifdef CONFIG_CRASH_DUMP
	[12] = { "oldmem", 0, &oldmem_fops, NULL },
#endif
#ifdef CONFIG_CONTIGMEM
	[111] = { "contigmem", S_IRUSR | S_IWUSR | S_IRGRP, &mem_fops,
            &directly_mappable_cdev_bdi},
#endif
};

static int memory_open(struct inode *inode, struct file *filp)
{
	int minor;
	const struct memdev *dev;

	minor = iminor(inode);
	if (minor >= ARRAY_SIZE(devlist))
		return -ENXIO;

	dev = &devlist[minor];
	if (!dev->fops)
		return -ENXIO;

	filp->f_op = dev->fops;
	if (dev->dev_info)
		filp->f_mapping->backing_dev_info = dev->dev_info;

	/* Is /dev/mem or /dev/kmem ? */
	if (dev->dev_info == &directly_mappable_cdev_bdi)
		filp->f_mode |= FMODE_UNSIGNED_OFFSET;

	if (dev->fops->open)
		return dev->fops->open(inode, filp);

	return 0;
}

static const struct file_operations memory_fops = {
	.open = memory_open,
	.llseek = noop_llseek,
};

static char *mem_devnode(struct device *dev, umode_t *mode)
{
	if (mode && devlist[MINOR(dev->devt)].mode)
		*mode = devlist[MINOR(dev->devt)].mode;
	return NULL;
}

static struct class *mem_class;

static int __init chr_dev_init(void)
{
	int minor;
	int err;

	err = bdi_init(&zero_bdi);
	if (err)
		return err;

	if (register_chrdev(MEM_MAJOR, "mem", &memory_fops))
		printk("unable to get major %d for memory devs\n", MEM_MAJOR);

	mem_class = class_create(THIS_MODULE, "mem");
	if (IS_ERR(mem_class))
		return PTR_ERR(mem_class);

	mem_class->devnode = mem_devnode;
	for (minor = 1; minor < ARRAY_SIZE(devlist); minor++) {
		if (!devlist[minor].name)
			continue;
		device_create(mem_class, NULL, MKDEV(MEM_MAJOR, minor),
			      NULL, devlist[minor].name);
	}

	return tty_init();
}

fs_initcall(chr_dev_init);
