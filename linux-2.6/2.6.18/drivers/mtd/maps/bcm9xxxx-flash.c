/*
 * Flash mapping for BCM7xxx boards
 *
 * Copyright (C) 2001 Broadcom Corporation
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 *
 * THT				10-23-2002
 * Steven J. Hill	09-25-2001
 * Mark Huang		09-15-2001
 */

#include <linux/module.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <asm/io.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/map.h>
#include <linux/mtd/partitions.h>
#include <linux/config.h>
#include <linux/init.h>
#include <asm/brcmstb/common/brcmstb.h>

#define PRINTK(...)
//#define PRINTK printk

extern int gFlashSize;

extern unsigned long getPhysFlashBase(void);
#define WINDOW_ADDR getPhysFlashBase()

/* 
 * All other 97XXX boards.  May have 2 flash chips, but we only use 1.
 * and since they are of different sizes, no interleaving.
 */
#define WINDOW_SIZE (0x20000000	 - WINDOW_ADDR)

#ifdef CONFIG_TIVO_KONTIKI
#define GIO_ODEN_LO             (BCHP_GIO_REG_START + 0)
#define GIO_DATA_LO             (BCHP_GIO_REG_START + 4)
#define GIO_IODIR_LO            (BCHP_GIO_REG_START + 8)
#define NOR_FLASH_WP_PIN        (1 << 28)
#define NOR_FLASH_WP_MUX_OFFSET (BCHP_SUN_TOP_CTRL_PIN_MUX_CTRL_6_gpio_028_SHIFT)
#define NOR_FLASH_WP_MUX_MASK   (BCHP_SUN_TOP_CTRL_PIN_MUX_CTRL_6_gpio_028_MASK)
#define NOR_FLASH_WP_MUX_VALUE  (0 << NOR_FLASH_WP_MUX_OFFSET)
/* Temporary until we get an official 7335B0 kernel */
#define NOR_FLASH_WP_MUX_OFFSET_B0 (21)
#define NOR_FLASH_WP_MUX_MASK_B0   (7 << NOR_FLASH_WP_MUX_OFFSET_B0)
#define NOR_FLASH_WP_MUX_VALUE_B0  (0 << NOR_FLASH_WP_MUX_OFFSET_B0)
#endif /* CONFIG_TIVO_KONTIKI */

#define BUSWIDTH 2

static struct mtd_info *bcm9XXXX_mtd;

/* Whether splash screen has been enabled on command line */
extern int gBcmSplash;

#ifdef CONFIG_MTD_COMPLEX_MAPPINGS
extern int bcm7401Cx_rev;
extern int bcm7118Ax_rev;
extern int bcm7403Ax_rev;

static DEFINE_SPINLOCK(bcm9XXXX_lock);

static inline void bcm9XXXX_map_copy_from_16bytes(void *to, unsigned long from, ssize_t len)
{
	unsigned long flags;
	
	spin_lock_irqsave(&bcm9XXXX_lock, flags);
	*(volatile unsigned long*)0xbffff880 = 0xFFFF;
	*(volatile unsigned long*)0xbffff880 = 0xFFFF;
	*(volatile unsigned long*)0xbffff880 = 0xFFFF;
	*(volatile unsigned long*)0xbffff880 = 0xFFFF;
	*(volatile unsigned long*)0xbffff880 = 0xFFFF;
	*(volatile unsigned long*)0xbffff880 = 0xFFFF;
	*(volatile unsigned long*)0xbffff880 = 0xFFFF;
	*(volatile unsigned long*)0xbffff880 = 0xFFFF;
	memcpy_fromio(to, (void *)from, len);
	spin_unlock_irqrestore(&bcm9XXXX_lock, flags);
}

static map_word bcm9XXXX_map_read(struct map_info *map, unsigned long ofs)
{
	/* if it is 7401C0, then we need this workaround */
	if(bcm7401Cx_rev == 0x20 || bcm7118Ax_rev == 0x0 
                                 || bcm7403Ax_rev == 0x20)
	{	
		map_word r;
		unsigned long flags;
	
		spin_lock_irqsave(&bcm9XXXX_lock, flags);
		*(volatile unsigned long*)0xb0400b1c = 0xFFFF;
		*(volatile unsigned long*)0xb0400b1c = 0xFFFF;
		*(volatile unsigned long*)0xb0400b1c = 0xFFFF;
		*(volatile unsigned long*)0xb0400b1c = 0xFFFF;
		*(volatile unsigned long*)0xb0400b1c = 0xFFFF;
		*(volatile unsigned long*)0xb0400b1c = 0xFFFF;
		*(volatile unsigned long*)0xb0400b1c = 0xFFFF;
		*(volatile unsigned long*)0xb0400b1c = 0xFFFF;
		r = inline_map_read(map, ofs);
		spin_unlock_irqrestore(&bcm9XXXX_lock, flags);
		return r;
	}
	else
		return inline_map_read(map, ofs);
}

static void bcm9XXXX_map_copy_from(struct map_info *map, void *to, unsigned long from, ssize_t len)
{
	/* if it is 7401C0, then we need this workaround */
	if(bcm7401Cx_rev == 0x20 || bcm7118Ax_rev == 0x0
                                 || bcm7403Ax_rev == 0x20)
	{
		if(len > 16)
		{
			while(len >= 16)
			{
				bcm9XXXX_map_copy_from_16bytes(to,
					(unsigned long) map->virt + from,
					16);
				to += 16;
				from += 16;
				len -= 16;
			}
		}
	
		if(len > 0)
			bcm9XXXX_map_copy_from_16bytes(to, (unsigned long) map->virt + from, len);
	}	
	else	
		memcpy_fromio(to, map->virt + from, len);
}

static void bcm9XXXX_map_write(struct map_info *map, const map_word d, unsigned long ofs)
{
	inline_map_write(map, d, ofs);
}

static void bcm9XXXX_map_copy_to(struct map_info *map, unsigned long to, const void *from, ssize_t len)
{
	inline_map_copy_to(map, to, from, len);
}
#endif

#ifdef CONFIG_TIVO_KONTIKI
/* Function to enable/disable programming. */
void thomson_dtv_flash_set_vpp(struct map_info *map, int enable)
{
  unsigned long value;

  value = BDEV_RD(GIO_DATA_LO);
  if (enable)
  {
    /* Set NOR WP pin output to hi in GIO_DATA_LO */
    value |= NOR_FLASH_WP_PIN;
  }
  else
  {
    /* Set NOR WP pin output to low in GIO_DATA_LO */
    value &= ~NOR_FLASH_WP_PIN;
  }
  BDEV_WR(GIO_DATA_LO, value);

}
#endif /* CONFIG_TIVO_KONTIKI */

struct map_info bcm9XXXX_map
= {
#ifdef CONFIG_TIVO_KONTIKI
	name: "Kontiki_flash",
#else
	name: "Broadcom 9xxxx mapped flash",
#endif
	// size: WINDOW_SIZE, // THT: Now a value to be determined at run-time.
	bankwidth: BUSWIDTH,

// jipeng - enable this for 7401C0 & 7118A0
#ifdef	CONFIG_MTD_COMPLEX_MAPPINGS
	read: bcm9XXXX_map_read,
	copy_from: bcm9XXXX_map_copy_from,
	write: bcm9XXXX_map_write,
#ifdef CONFIG_TIVO_KONTIKI
	copy_to: bcm9XXXX_map_copy_to,
#else
	copy_to: bcm9XXXX_map_copy_to
#endif
#endif 

#ifdef CONFIG_TIVO_KONTIKI
    set_vpp:    thomson_dtv_flash_set_vpp
#endif 
};

/*
 * Don't define DEFAULT_SIZE_MB if the platform does not support a standard partition table.
 * Defining it will allow the user to specify flashsize=nnM at boot time for non-standard flash size, however.
 */

#define SMALLEST_FLASH_SIZE	(16<<20)
#define DEFAULT_RESERVED_SIZE 	(4<<20)  // CFE areas, from 1FC0_0000H to 2000_0000H
#define DEFAULT_SPLASH_SIZE 	(512<<10) // 512KB
#define ROOTFS_PART				(0)

#ifdef CONFIG_MTD_ECM_PARTITION
#define DEFAULT_OCAP_SIZE		(6<<20)
#define DEFAULT_AVAIL1_SIZE 	(32<<20)
#define DEFAULT_ECM_SIZE 		(DEFAULT_OCAP_SIZE+DEFAULT_AVAIL1_SIZE)
#define AVAIL1_PART				(1)
#define OCAP_PART				(2)
#else
#define DEFAULT_ECM_SIZE		(0)
#define DEFAULT_OCAP_SIZE		(0)
#define DEFAULT_AVAIL1_SIZE		(0)
#define AVAIL1_PART				(-1)
#endif
// DEFAULT_SIZE_MB will be defined later based on platforms.
#define DEFAULT_ROOTFS_SIZE (DEFAULT_SIZE_MB - DEFAULT_RESERVED_SIZE - DEFAULT_ECM_SIZE)


static struct mtd_partition bcm9XXXX_parts[] = {

#if defined( CONFIG_TIVO_KONTIKI )

#define DEFAULT_SIZE_MB 128
	{ name: "loader",	offset: 0x00000000,	size:    384*1024 },
	{ name: "kernel",	offset: 0x000E0000,	size:   1792*1024 },
	{ name: "rootfs",	offset: 0x002A0000,	size:  16384*1024 },
	{ name: "reserved",	offset: 0x00060000,	size:    512*1024 },
	{ name: "HR24.NOR",	offset: 0x012A0000,	size: 112000*1024 },
	{ name: "entire",	offset: 0x00000000,	size: (DEFAULT_SIZE_MB << 20) },

#elif defined( CONFIG_MIPS_BCM7440 )

#define DEFAULT_SIZE_MB 64 /* 64MB flash */

	{ name: "rootfs",		offset: 0,		size: 60*1024*1024 },
	{ name: "cfe",		offset: 0x03C00000, size: 512*1024 },
	{ name: "vmlinux",      offset: 0x03C80000, size: 3454*1024 },
	{ name: "drmregion",    offset: 0x03FDF800, size:  128*1024 },
	{ name: "config",		offset: 0x03FFF800,	size: 144 },
	{ name: "nvram",		offset: 0x03FFF890,	size: 1904 },

#elif BRCM_FLASH_SIZE == 0x1000000

/* 3563 */
#define DEFAULT_SIZE_MB 16
	{ name: "rootfs",	offset: 0,		    size: 12*1024*1024 },
	{ name: "cfe",	        offset: 0x00C00000, size: 512*1024 },
	{ name: "vmlinux",	offset: 0x00C80000, size: 3582*1024 },
	{ name: "config",	offset: 0x00FFF800,	size: 144 },
	{ name: "nvram",	offset: 0x00FFF890,	size: 1904 },

#else

/* default for 32MB+ platforms */

#define DEFAULT_SIZE_MB 32 /* 32MB flash */  
  #if defined( CONFIG_MTD_ECM_PARTITION)
	{ name: "rootfs",		offset: 0,		    		size: DEFAULT_ROOTFS_SIZE },
	{ name: "avail1",		offset: DEFAULT_ROOTFS_SIZE,	size: DEFAULT_AVAIL1_SIZE },
	{ name: "ocap",		offset: DEFAULT_ROOTFS_SIZE+DEFAULT_AVAIL1_SIZE,	size: DEFAULT_OCAP_SIZE },

  #else
	{ name: "rootfs",		offset: 0,			size: 28*1024*1024 },
  #endif
	{ name: "cfe",	        offset: 0x01C00000, size: 512*1024 },
	{ name: "vmlinux",	offset: 0x01C80000, size: 3582*1024 },
	{ name: "config",	offset: 0x01FFF800,	size: 144 },
	{ name: "nvram",	offset: 0x01FFF890,	size: 1904 },

#endif
/* Add 1 extra place-holder partition for splash, and a safety guard element */
	{name: NULL, offset: 0, size: 0},
	{name: NULL, offset: 0, size: 0}
};


int __init init_bcm9XXXX_map(void)
{
	unsigned int avail1_size = DEFAULT_AVAIL1_SIZE;
	int i, numparts;

#ifdef CONFIG_MTD_ECM_PARTITION
	unsigned int ecm_size = DEFAULT_ECM_SIZE;
	unsigned int ocap_size = DEFAULT_OCAP_SIZE;
#endif
#ifdef CONFIG_TIVO_KONTIKI
    unsigned long value;
    unsigned long version;
#endif
	
	printk(KERN_NOTICE "BCM97XXX flash device: 0x%08lx @ 0x%08lx\n", WINDOW_SIZE, WINDOW_ADDR);
	bcm9XXXX_map.size = WINDOW_SIZE;
	numparts = ARRAY_SIZE(bcm9XXXX_parts) - 2; /* Minus the 2 extra place holders */
	
	/* Adjust partition table */
#ifdef CONFIG_MTD_ECM_PARTITION
	if (WINDOW_SIZE < (64<<20)) {
		ecm_size = DEFAULT_OCAP_SIZE;
		avail1_size = 0;
		bcm9XXXX_parts[AVAIL1_PART].size = avail1_size;
		numparts--;
	}
	else {
		int factor = WINDOW_SIZE / (64 << 20);
		
		bcm9XXXX_parts[OCAP_PART].size = ocap_size = factor*DEFAULT_OCAP_SIZE;
		bcm9XXXX_parts[AVAIL1_PART].size = avail1_size = factor*DEFAULT_AVAIL1_SIZE;
		ecm_size = ocap_size + avail1_size;
	}

	bcm9XXXX_parts[0].size = WINDOW_SIZE - (DEFAULT_RESERVED_SIZE + ecm_size);
PRINTK("Part[0] name=%s, size=%x, offset=%x\n", bcm9XXXX_parts[0].name, bcm9XXXX_parts[0].size, bcm9XXXX_parts[0].offset);
	for (i=1; i<ARRAY_SIZE(bcm9XXXX_parts); i++) {
		
		/* Skip avail1 if 0 size */
		if (0 == bcm9XXXX_parts[i].size && i == AVAIL1_PART) {
			bcm9XXXX_parts[i].offset = bcm9XXXX_parts[i-1].size + bcm9XXXX_parts[i-1].offset;
			continue;
		}
	
		bcm9XXXX_parts[i].offset = bcm9XXXX_parts[i-1].size + bcm9XXXX_parts[i-1].offset;
		
PRINTK("Part[%d] name=%s, size=%x, offset=%x\n", avail1_size ? i : i-1, 
bcm9XXXX_parts[i].name, bcm9XXXX_parts[i].size, bcm9XXXX_parts[i].offset);
	}

	/* Shift partitions 1 up if avail1_size is 0 */
	if (0 == avail1_size) {
		for (i=AVAIL1_PART; i < numparts; i++) {
			bcm9XXXX_parts[i].offset = bcm9XXXX_parts[i+1].offset;
			bcm9XXXX_parts[i].size = bcm9XXXX_parts[i+1].size;
		}
		bcm9XXXX_parts[numparts].offset = 0;
		bcm9XXXX_parts[numparts].size = 0;
	}

		
#elif defined( DEFAULT_SIZE_MB )
	if (WINDOW_SIZE != (DEFAULT_SIZE_MB << 20)) {
		
		bcm9XXXX_parts[0].size += WINDOW_SIZE - (DEFAULT_SIZE_MB << 20);
PRINTK("Part[0] name=%s, size=%x, offset=%x\n", bcm9XXXX_parts[0].name, bcm9XXXX_parts[0].size, bcm9XXXX_parts[0].offset);
		for (i=1; i<ARRAY_SIZE(bcm9XXXX_parts); i++) {
			bcm9XXXX_parts[i].offset += WINDOW_SIZE - (DEFAULT_SIZE_MB << 20);
PRINTK("Part[%d] name=%s, size=%x, offset=%x\n", i, bcm9XXXX_parts[i].name, bcm9XXXX_parts[i].size, bcm9XXXX_parts[i].offset);
		}
	}
#endif

	if (gBcmSplash) {		
PRINTK("In bcmSplash, numParts=%d\n", numparts);
		for (i=0; i<numparts; i++) {
PRINTK("bcm9xxxx-flash.c: i=%d\n", i);
PRINTK("B4 Part[%d] name=%s, size=%x, offset=%x\n",  i, 
	bcm9XXXX_parts[i].name, bcm9XXXX_parts[i].size, bcm9XXXX_parts[i].offset);
			

			/* we will carve out 512KB from avail1 partition if it exists, or rootfs otherwise */
			if ((avail1_size && i == AVAIL1_PART) || (avail1_size == 0 && i == ROOTFS_PART)) {
				int j;

				/* i now points to the avail1 and/or rootfs partition */
				bcm9XXXX_parts[i].size -= DEFAULT_SPLASH_SIZE;
				if (i > 0) {
					bcm9XXXX_parts[i].offset = bcm9XXXX_parts[i-1].size + bcm9XXXX_parts[i-1].offset;
				}
				else {
					bcm9XXXX_parts[i].offset = 0;
				}
				
				/* Move all partitions that follow one down */
				for (j=numparts - 1; j > i; j--) {
					bcm9XXXX_parts[j+1] = bcm9XXXX_parts[j];
	PRINTK("Moved partition[%d] down to [%d], name=%s\n", j, j+1, bcm9XXXX_parts[j+1].name);
				}	
				numparts++;
				
	PRINTK("original: #parts=%d, Part[%d] name=%s, size=%x, offset=%x\n", numparts,  i, 
bcm9XXXX_parts[i].name, bcm9XXXX_parts[i].size, bcm9XXXX_parts[i].offset);

				i++;
				/* i now points to the newly created splash partition */

				bcm9XXXX_parts[i].offset = bcm9XXXX_parts[i-1].size + bcm9XXXX_parts[i-1].offset;
				bcm9XXXX_parts[i].size = DEFAULT_SPLASH_SIZE;
				bcm9XXXX_parts[i].name = "splash";
	PRINTK("splash: #parts=%d, Part[%d] name=%s, size=%x, offset=%x\n", numparts,  i, 
bcm9XXXX_parts[i].name, bcm9XXXX_parts[i].size, bcm9XXXX_parts[i].offset);
			}

		}
	}

	
	bcm9XXXX_map.virt = ioremap((unsigned long)WINDOW_ADDR, WINDOW_SIZE);

	if (!bcm9XXXX_map.virt) {
		printk("Failed to ioremap\n");
		return -EIO;
	}
	
#ifdef CONFIG_TIVO_KONTIKI
	/* Turn off EBI inversion to simplify everything and setup Chip Select
	 * for R/W allow sending flash commands.
	 */
	value = BDEV_RD(BCHP_EBI_CS_CONFIG_0);
	value &= ~(BCHP_EBI_CS_CONFIG_0_mask_en_MASK | BCHP_EBI_CS_CONFIG_0_wp_MASK);
	BDEV_WR(BCHP_EBI_CS_CONFIG_0, value);

	/* Set up GPIO Pin to control NOR Flash's Write Protect pin
	 * Test the chip revision - temporary work-around until we have an official 7335B0 kernel
	 */
	version = BDEV_RD(BCHP_SUN_TOP_CTRL_PROD_REVISION);
	printk("SOC = BCM%04lx%c%lu\n", (version >> 16) & 0xffff, 'A' + (int)((version >> 4) & 0xf), version & 0xf);

	/* Override the MUX Value */
	value = BDEV_RD(BCHP_SUN_TOP_CTRL_PIN_MUX_CTRL_6);
	if(version != 0x73350010)
	{
		value = (value & ~NOR_FLASH_WP_MUX_MASK) | NOR_FLASH_WP_MUX_VALUE;
	}
	else
	{
		value = (value & ~NOR_FLASH_WP_MUX_MASK_B0) | NOR_FLASH_WP_MUX_VALUE_B0;
	}
	/* Override the MUX Value */
	BDEV_WR(BCHP_SUN_TOP_CTRL_PIN_MUX_CTRL_6, value);

	/* Override the IODIR - Make sure output */
	value = BDEV_RD(GIO_IODIR_LO);
	BDEV_WR(GIO_IODIR_LO, value & ~NOR_FLASH_WP_PIN);

	/* Override the ODEN - Make sure totem pole */
	value = BDEV_RD(GIO_ODEN_LO);
	BDEV_WR(GIO_ODEN_LO, value & ~NOR_FLASH_WP_PIN);
#endif /* CONFIG_TIVO_KONTIKI */

	bcm9XXXX_mtd = do_map_probe("cfi_probe", &bcm9XXXX_map);
	if (!bcm9XXXX_mtd) {
		iounmap((void *)bcm9XXXX_map.virt);
		return -ENXIO;
	}
		
	add_mtd_partitions(bcm9XXXX_mtd, bcm9XXXX_parts, numparts);
	bcm9XXXX_mtd->owner = THIS_MODULE;
	return 0;
}

void __exit cleanup_bcm9XXXX_map(void)
{
	if (bcm9XXXX_mtd) {
		del_mtd_partitions(bcm9XXXX_mtd);
		map_destroy(bcm9XXXX_mtd);
	}
	if (bcm9XXXX_map.virt) {
		iounmap((void *)bcm9XXXX_map.virt);
		bcm9XXXX_map.virt = 0;
	}
}

module_init(init_bcm9XXXX_map);
module_exit(cleanup_bcm9XXXX_map);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Steven J. Hill <shill@broadcom.com>");
MODULE_DESCRIPTION("Broadcom 7xxx MTD map driver");
