/*
 * Flash mapping for Thompson HRXX boards
 *
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
#include <asm/brcmstb/common/bcmtypes.h>
#include <asm/brcmstb/common/brcmstb.h>

#if defined(CONFIG_MTD_NAND)
#include <linux/mtd/nand.h>

static struct mtd_info *nand_mtd;
static __iomem void *nand_baseaddr;
static int init_nand_flash(void);
static void cleanup_nand_flash(void);
static void nand_hwcontrol(struct mtd_info *mtd, int cmd,
				   unsigned int ctrl);
static int nand_dev_ready(void);
static void nand_set_wp(int enable);
static void nand_tmm_select_chip(struct mtd_info *mtd, int chipnr);
static struct mtd_partition nand_parts[] =
{
    { name: "rfs-all",		offset: 0x0, size: 208*1024*1024},
    { name: "rootfs",		offset: 0x80000, size: (208*1024*1024)-0x80000},
    { name: "slices",		offset: 0xE000000, size: 32*1024*1024 },
    { name: "nand",		offset: 0x0, size: 256*1024*1024 },
    /*All new partitions will be added from here*/
    { name: "persist",		offset: 0xD000000, size: 16*1024*1024 },
};

#define NAND_WINDOW_ADDR   (0x18000000)
#define NAND_CS_BASE_SIZE  (1)      // see 7401 data sheet
#define NAND_WINDOW_SIZE   (8192*(NAND_CS_BASE_SIZE+1)) // 16K : smallest to include bits 12 and 13
#define NAND_CHIP_DELAY    (25)     // tR = 25us
#define NAND_CL_MASK       (0x1000) // addr. bit 12
#define NAND_AL_MASK       (0x2000) // addr. bit 13
#endif //CONFIG_MTD_NAND

extern int gFlashSize;

#define WINDOW_SIZE 0x01000000
#define WINDOW_ADDR 0x1f000000

#define EBI_CS_CONFIG_0               *((volatile unsigned long *)(KSEG1ADDR(0x10000000 + BCHP_EBI_CS_CONFIG_0)))
#define GIO_DATA_LO                   *((volatile unsigned long *)(KSEG1ADDR(0x10000000 + 0x400704)))
#define GIO_IODIR_LO                  *((volatile unsigned long *)(KSEG1ADDR(0x10000000 + 0x400708)))
#define GIO_ODEN_LO                   *((volatile unsigned long *)(KSEG1ADDR(0x10000000 + 0x400700)))
#define SUN_TOP_CTRL_PIN_MUX_CTRL_8   *((volatile unsigned long *)(KSEG1ADDR(0x10000000 + 0x4040b4)))
#define GIO_GPIO17_HI         (0x00020000)
#define MUX_CTRL8_GPIO17_MASK (0xFFFFFFC7)

#ifdef CONFIG_MTD_NAND

#define EBI_CS_CONFIG_1               *((volatile unsigned long *)(KSEG1ADDR(0x10000000 + BCHP_EBI_CS_CONFIG_1)))
#define EBI_CS_BASE_1                 *((volatile unsigned long *)(KSEG1ADDR(0x10000000 + BCHP_EBI_CS_BASE_1)))
#define GIO_DATA_HI                   *((volatile unsigned long *)(KSEG1ADDR(0x10000000 + 0x400724)))
#define GIO_IODIR_HI                  *((volatile unsigned long *)(KSEG1ADDR(0x10000000 + 0x400728)))
#define GIO_ODEN_HI                   *((volatile unsigned long *)(KSEG1ADDR(0x10000000 + 0x400720)))
#define SUN_TOP_CTRL_PIN_MUX_CTRL_11  *((volatile unsigned long *)(KSEG1ADDR(0x10000000 + 0x4040c0)))
#define SUN_TOP_CTRL_PIN_MUX_CTRL_7   *((volatile unsigned long *)(KSEG1ADDR(0x10000000 + 0x4040b0)))
#define GIO_GPIO56_HI          (0x01000000)   // NAND_RB
#define MUX_CTRL11_GPIO56_MASK (0xFCFFFFFF)
#define GIO_GPIO13_HI          (0x00002000)   // NAND_FLASH_WP
#define MUX_CTRL7_GPIO13_MASK  (0xFF1FFFFF)

#endif // CONFIG_MTD_NAND

static struct mtd_partition *parsed_parts = NULL;

#define BUSWIDTH 2

static struct mtd_info *thompson_mtd;

#ifdef CONFIG_MTD_COMPLEX_MAPPINGS
extern int bcm7401Cx_rev;
extern int bcm7118Ax_rev;
extern int bcm7403Ax_rev;

static inline void thompson_map_copy_from_16bytes(void *to, void *from, ssize_t len)
{
	static DEFINE_SPINLOCK(thompson_lock);
	unsigned long flags;

	spin_lock_irqsave(&thompson_lock, flags);
	*(volatile unsigned long*)0xbffff880 = 0xFFFF;
	*(volatile unsigned long*)0xbffff880 = 0xFFFF;
	*(volatile unsigned long*)0xbffff880 = 0xFFFF;
	*(volatile unsigned long*)0xbffff880 = 0xFFFF;
	*(volatile unsigned long*)0xbffff880 = 0xFFFF;
	*(volatile unsigned long*)0xbffff880 = 0xFFFF;
	*(volatile unsigned long*)0xbffff880 = 0xFFFF;
	*(volatile unsigned long*)0xbffff880 = 0xFFFF;
	memcpy_fromio(to, from, len);
	spin_unlock_irqrestore(&thompson_lock, flags);
}

static map_word thompson_map_read(struct map_info *map, unsigned long ofs)
{
	/* if it is 7401C0, then we need this workaround */
	if(bcm7401Cx_rev == 0x20 || bcm7118Ax_rev == 0x0
                                 || bcm7403Ax_rev == 0x20)
	{
		map_word r;
		static DEFINE_SPINLOCK(thompson_lock);
		unsigned long flags;

		spin_lock_irqsave(&thompson_lock, flags);
		*(volatile unsigned long*)0xb0400b1c = 0xFFFF;
		*(volatile unsigned long*)0xb0400b1c = 0xFFFF;
		*(volatile unsigned long*)0xb0400b1c = 0xFFFF;
		*(volatile unsigned long*)0xb0400b1c = 0xFFFF;
		*(volatile unsigned long*)0xb0400b1c = 0xFFFF;
		*(volatile unsigned long*)0xb0400b1c = 0xFFFF;
		*(volatile unsigned long*)0xb0400b1c = 0xFFFF;
		*(volatile unsigned long*)0xb0400b1c = 0xFFFF;
		r = inline_map_read(map, ofs);
		spin_unlock_irqrestore(&thompson_lock, flags);
		return r;
	}
	else
		return inline_map_read(map, ofs);
}

static void thompson_map_copy_from(struct map_info *map, void *to, unsigned long from, ssize_t len)
{
	/* if it is 7401C0, then we need this workaround */
	if(bcm7401Cx_rev == 0x20 || bcm7118Ax_rev == 0x0
                                 || bcm7403Ax_rev == 0x20)
	{
		if(len > 16)
		{
			while(len >= 16)
			{
				thompson_map_copy_from_16bytes(to, map->virt + from, 16);
				to += 16;
				from += 16;
				len -= 16;
			}
		}

		if(len > 0)
		thompson_map_copy_from_16bytes(to, map->virt + from, len);
	}
	else
		memcpy_fromio(to, map->virt + from, len);
}

static void thompson_map_write(struct map_info *map, const map_word d, unsigned long ofs)
{
	inline_map_write(map, d, ofs);
}

static void thompson_map_copy_to(struct map_info *map, unsigned long to, const void *from, ssize_t len)
{
	inline_map_copy_to(map, to, from, len);
}
#endif

static void thomson_dtv_flash_set_wen(int enable)
{
        unsigned long value;
        static DEFINE_SPINLOCK(thompson_lock);
        unsigned long flags; 
        spin_lock_irqsave(&thompson_lock, flags);
        value = EBI_CS_CONFIG_0;
        if (enable)
        {
                value &= ~BCHP_EBI_CS_CONFIG_0_wp_MASK; /* Set CS0 to r/w (low) */
        }
        else
        {
                /* Ideally, this would put WP to RO.  However, Intel driver doesn't easily support this. */
                /* value |= EBI_CS_CONFIG_0_WP; */  /* Set CS0 to read only (high) */
        }
        EBI_CS_CONFIG_0 = value;
        spin_unlock_irqrestore(&thompson_lock, flags);

}

/* Function to enable/disable programming. */
void thomson_dtv_flash_set_vpp(struct map_info *map, int enable)
{
        unsigned long value;
        static DEFINE_SPINLOCK(thompson_lock);
        unsigned long flags; 

        thomson_dtv_flash_set_wen(enable);


        spin_lock_irqsave(&thompson_lock, flags);
        /* Override the MUX Value */
        value = SUN_TOP_CTRL_PIN_MUX_CTRL_8;
        SUN_TOP_CTRL_PIN_MUX_CTRL_8 = (value & MUX_CTRL8_GPIO17_MASK);

        /* Override the IODIR - Make sure output */
        value = GIO_IODIR_LO;
        GIO_IODIR_LO = (value & ~GIO_GPIO17_HI);

        /* Override the ODEN - Make sure totem pole */
        value = GIO_ODEN_LO;
        GIO_ODEN_LO = (value & ~GIO_GPIO17_HI);

        value = GIO_DATA_LO;
        if (enable)
        {
                /* Set GPIO 17 output to hi in GIO_DATA_LO */
                value |= GIO_GPIO17_HI;
        }
        else
        {
                /* Set GPIO 17 output to low in GIO_DATA_LO */
                value &= ~GIO_GPIO17_HI;
        }
        GIO_DATA_LO = value;
        spin_unlock_irqrestore(&thompson_lock, flags);

}

struct map_info thompson_map
= {
        name: "HR21T_flash",
	// size: WINDOW_SIZE, // THT: Now a value to be determined at run-time.
        bankwidth: BUSWIDTH,

// jipeng - enable this for 7401C0 & 7118A0
#ifdef	CONFIG_MTD_COMPLEX_MAPPINGS
	read: thompson_map_read,
	copy_from: thompson_map_copy_from,
	write: thompson_map_write,
	copy_to: thompson_map_copy_to,
#endif
        set_vpp:    thomson_dtv_flash_set_vpp
};

/*
 * Don't define DEFAULT_SIZE_MB if the platform does not support a standard partition table.
 * Defining it will allow the user to specify flashsize=nnM at boot time for non-standard flash size, however.
 */
static struct mtd_partition thompson_parts[] = {
#define DEFAULT_SIZE_MB 16
#ifdef CONFIG_TIVO_MOJAVE
	{ name: "bsl/cfe",	offset: 0x00000000,     size: 256*1024 },/*256KB*/ 
	{ name: "rfs",          offset: 0x00040000,     size: 512*1024},/*512KB*/
	{ name: "bkb",          offset: 0x00040000,     size: (11*1024+3840)*1024},/*15MB-256KB*/
	{ name: "TiVoStorage",	offset: 0x00F00000,	size: 1024*1024},/*1MB*/
	{ name: "entire",	offset: 0x00000000,     size: 16*1024*1024},/*16MB*/ 
#else
	{ name: "bsl/cfe",	offset: 0x00000000,     size: 256*1024 },
	/*{ name: "vmlinux",	offset: 0x000E0000, size: 1664*1024 },*/
	/* DTV: increase size to overlap rootfs partition and allow larger
		rootfs image to replace kernel+rootfs */
	{ name: "vmlinux",	offset: 0x000C0000,     size: 1408*1024 },
	{ name: "rfs",	        offset: 0x00220000,	size: 14208*1024 },
	{ name: "block3",	offset: 0x00040000,	size: 512*1024 },
        { name: "entire",	offset: 0x00000000,	size: 16*1024*1024 },
#endif
};

int __init init_thompson_map(void)
{

        unsigned long value;
        int num_cmdline_parts = 0;
        const char *part_probes[] = { "cmdlinepart", NULL };

	printk(KERN_NOTICE "Thompson flash device: 0x%8x at 0x%8x\n", WINDOW_SIZE, WINDOW_ADDR);
	thompson_map.size = WINDOW_SIZE;
	/* Adjust partition table */
#ifdef DEFAULT_SIZE_MB
	if (WINDOW_SIZE != (DEFAULT_SIZE_MB << 20)) {
		int i;

		thompson_parts[0].size += WINDOW_SIZE - (DEFAULT_SIZE_MB << 20);
                //printk("Part[0] name=%s, size=%x, offset=%x\n", 
                // thompson_parts[0].name, thompson_parts[0].size, thompson_parts[0].offset);
		for (i=1; i<ARRAY_SIZE(thompson_parts); i++) {
			thompson_parts[i].offset += WINDOW_SIZE - (DEFAULT_SIZE_MB << 20);
                        //printk("Part[%d] name=%s, size=%x, offset=%x\n", i,
                        // thompson_parts[i].name, thompson_parts[i].size, thompson_parts[i].offset);
		}
	}
#endif
	thompson_map.virt = (void *)ioremap(WINDOW_ADDR, WINDOW_SIZE);

	if (!thompson_map.virt) {
		printk("Failed to ioremap\n");
		return -EIO;
	}

        {
                static DEFINE_SPINLOCK(thompson_lock);
                unsigned long flags; 

                spin_lock_irqsave(&thompson_lock, flags); 
                // HR21T : Turn off EBI inversion to simplify everything.
                value = EBI_CS_CONFIG_0;
                value &= ~BCHP_EBI_CS_CONFIG_0_mask_en_MASK;
                EBI_CS_CONFIG_0 = value;
                /* Setup Chip Select for R/W.  This allows commands which don't actually attempt
                to write to Flash. (Status, etc.) */
                spin_unlock_irqrestore(&thompson_lock, flags);
        }
        thomson_dtv_flash_set_wen(1);

	thompson_mtd = do_map_probe("cfi_probe", &thompson_map);
	if (!thompson_mtd) {
		iounmap((void *)thompson_map.virt);
		return -ENXIO;
	}

        /* Disable Flash Write. Will be enabled when needed in programming routines. */
        thomson_dtv_flash_set_vpp(&thompson_map, 0);

        /* Check command line partition information */
        num_cmdline_parts = parse_mtd_partitions(thompson_mtd, part_probes, &parsed_parts, 0);

        if (num_cmdline_parts > 0)
        {
            int i;
            int tablesize = sizeof(thompson_parts)/sizeof(struct mtd_partition);

            printk("Using MTD partitions specified on command line\n");
            /* Thomson change : Assumes command line will create mtdblocks0-3.
              This will allow block with entire flash to be created. */
            if (num_cmdline_parts > (tablesize - 1))
            {
               printk("WARNING: Too many partitions specified by command line.  Using defaults.\n");
            }
            else
            {
              for ( i = 0; i < num_cmdline_parts; i++)
              {
                 thompson_parts[i] = parsed_parts[i];
              }
            }
                add_mtd_partitions(thompson_mtd, thompson_parts, tablesize);
            }
        else
        {
            add_mtd_partitions(thompson_mtd, thompson_parts, sizeof(thompson_parts)/sizeof(thompson_parts[0]));
        }

        thompson_mtd->owner = THIS_MODULE;

#if defined(CONFIG_MTD_NAND)
        init_nand_flash();
#endif // CONFIG_MTD_NAND
        return 0;
}

        void __exit cleanup_thompson_map(void)
{
	if (thompson_mtd) {
		del_mtd_partitions(thompson_mtd);
		map_destroy(thompson_mtd);
	}
	if (thompson_map.virt) {
		iounmap((void *)thompson_map.virt);
		thompson_map.virt = 0;
	}

#if defined(CONFIG_MTD_NAND)
    cleanup_nand_flash();
#endif // CONFIG_MTD_NAND

}

#if defined(CONFIG_MTD_NAND)
static int init_nand_flash(void)
{
        struct nand_chip *this;
        int err = 0;
        unsigned long value;
        static DEFINE_SPINLOCK(thompson_lock);
        unsigned long flags; 
        int i, badblocks;
        int part_count;

        printk(KERN_NOTICE "Initializing NAND Flash...\n");

        // First initialize hardware
        // EBI 1 Setup

        spin_lock_irqsave(&thompson_lock, flags); 
        EBI_CS_BASE_1 = (NAND_WINDOW_ADDR | NAND_CS_BASE_SIZE);

        value = EBI_CS_CONFIG_1;
        value |= (BCHP_EBI_CS_CONFIG_1_le_MASK | BCHP_EBI_CS_CONFIG_1_enable_MASK);
        EBI_CS_CONFIG_1 = value;
        spin_unlock_irqrestore(&thompson_lock, flags);

        // GPIOs : NAND_RB : GPIO_56, NAND_FLASH_WP : GPIO_13
        nand_set_wp(0);   // Disable writing to NAND
        nand_dev_ready(); // Read device ready which also sets it up.

        /* Allocate memory for MTD device structure and private data */
        nand_mtd = kmalloc (sizeof(struct mtd_info) + sizeof (struct nand_chip), GFP_KERNEL);
        if (!nand_mtd) {
                printk ("Unable to allocate NAND MTD device structure.\n");
                err = -ENOMEM;
                goto out;
        }

        /* Initialize structures */
        memset ((char *) nand_mtd, 0, sizeof(struct mtd_info) + sizeof(struct nand_chip));

        /* map physical adress */
        nand_baseaddr = (void *)ioremap(NAND_WINDOW_ADDR, NAND_WINDOW_SIZE);
        if(!nand_baseaddr){
                printk("Ioremap to access NAND chip failed\n");
                err = -EIO;
                goto out_mtd;
        }

        /* Get pointer to private data */
        this = (struct nand_chip *) (&nand_mtd[1]);
        /* Link the private data with the MTD structure */
        nand_mtd->priv = this;

        /* Set address of NAND IO lines */
        this->IO_ADDR_R = nand_baseaddr;
        this->IO_ADDR_W = nand_baseaddr;
        /* Set a select_chip function */
        this->select_chip = nand_tmm_select_chip;
        /* Reference hardware control function */
        this->cmd_ctrl = nand_hwcontrol;
        /* Set command delay time, see datasheet for correct value */
        this->chip_delay = NAND_CHIP_DELAY;
        /* Assign the device ready function, if available */
        this->dev_ready = (int(*)(struct mtd_info *))nand_dev_ready;
        this->ecc.mode = NAND_ECC_SOFT;

        /* Scan to find existance of the device */
        if (nand_scan (nand_mtd, 1)) {
                err = -ENXIO;
                goto out_ior;
        }
        
        part_count = (sizeof(nand_parts)/sizeof(nand_parts[0]));
        for ( i = 0, badblocks = 0; i < part_count ; i++){
                do {
                        if ( nand_isbad_bbt(nand_mtd, nand_parts[i].offset, 1)){
                                nand_parts[i].offset +=  nand_mtd->erasesize;
                                nand_parts[i].size -=  nand_mtd->erasesize;
                        }
                        else {
                                break; 
                        }
                } while ( nand_parts[i].offset <  nand_parts[i].size);
        }
        add_mtd_partitions(nand_mtd, nand_parts, (sizeof(nand_parts)/sizeof(nand_parts[0])) );
        goto out;

out_ior:
        iounmap((void *)nand_baseaddr);
out_mtd:
        kfree (nand_mtd);
out:
        return err;
}

static void cleanup_nand_flash(void)
{
        if (nand_mtd)
        {
                nand_release(nand_mtd);
                iounmap((void *)nand_baseaddr);
                kfree(nand_mtd);
        }
}

static void nand_hwcontrol(struct mtd_info *mtd, int cmd,
				   unsigned int ctrl)
{
        static DEFINE_SPINLOCK(thompson_lock);
        unsigned long flags;  

        struct nand_chip *this = (struct nand_chip *) mtd->priv;
        /* Not sure if this needs spinlock protection. Check with John Dyson !! */ 


        spin_lock_irqsave(&thompson_lock, flags); 

        if (ctrl & NAND_CTRL_CHANGE)
        {
                this->IO_ADDR_W = nand_baseaddr;

                if (ctrl & NAND_CLE)
                {
                        this->IO_ADDR_W += NAND_CL_MASK;
                }
                else if (ctrl & NAND_ALE)
                {
                        this->IO_ADDR_W += NAND_AL_MASK;
                }
        }

        spin_unlock_irqrestore(&thompson_lock, flags);

        if (cmd != NAND_CMD_NONE)
        {
                writeb(cmd, this->IO_ADDR_W);
        }

}

// Function to read the NAND_RB pin which is on GPIO56
static int nand_dev_ready(void)
{
        unsigned long value;
        int retval = 0;

        /* Override the MUX Value for GPIO56 */
        value = SUN_TOP_CTRL_PIN_MUX_CTRL_11;
        SUN_TOP_CTRL_PIN_MUX_CTRL_11 = (value & MUX_CTRL11_GPIO56_MASK);
        /* Override the IODIR - Make sure input */
        value = GIO_IODIR_HI;
        GIO_IODIR_HI = (value | GIO_GPIO56_HI);
        /* Override the ODEN - Make sure totem pole */
        value = GIO_ODEN_HI;
        GIO_ODEN_HI = (value & ~GIO_GPIO56_HI);

        value = GIO_DATA_HI;
        if (GIO_DATA_HI & GIO_GPIO56_HI)
        {
                retval = 1;
        }

        return retval;
}

// Function to write the NAND_FLASH_WP pin.
// enable = 1: enable writing, put pin high
// enable = 0: disable writing, put pin low.
static void nand_set_wp(int enable)
{
        unsigned long value;

        /* Override the MUX Value for GPIO13 */
        value = SUN_TOP_CTRL_PIN_MUX_CTRL_7;
        SUN_TOP_CTRL_PIN_MUX_CTRL_7 = (value & MUX_CTRL7_GPIO13_MASK);
        /* Override the IODIR - Make sure output */
        value = GIO_IODIR_LO;
        GIO_IODIR_LO = (value & ~GIO_GPIO13_HI);
        /* Override the ODEN - Make sure totem pole */
        value = GIO_ODEN_LO;
        GIO_ODEN_LO = (value & ~GIO_GPIO13_HI);

        value = GIO_DATA_LO;
        if (enable)
        {
                /* Set GPIO 13 output to hi in GIO_DATA_LO */
                value |= GIO_GPIO13_HI;
        }
        else
        {
              /* Set GPIO 13 output to low in GIO_DATA_LO */
                value &= ~GIO_GPIO13_HI;
        }
        GIO_DATA_LO = value;
}

static void nand_tmm_select_chip(struct mtd_info *mtd, int chipnr)
{
	struct nand_chip *chip = mtd->priv;

	switch (chipnr) {
	case -1:
		chip->cmd_ctrl(mtd, NAND_CMD_NONE, 0 | NAND_CTRL_CHANGE);
                nand_set_wp(0);
		break;
	case 0:
                nand_set_wp(1);
		break;

	default:
		BUG();
	}
}
#endif //CONFIG_MTD_NAND

module_init(init_thompson_map);
module_exit(cleanup_thompson_map);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Steven J. Hill <shill@broadcom.com>");
MODULE_DESCRIPTION("Thompson MTD map driver derived from Broadcom 7xxx MTD map driver");
