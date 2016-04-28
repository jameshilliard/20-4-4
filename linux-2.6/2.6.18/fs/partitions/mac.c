/*
 *  fs/partitions/mac.c
 *
 *  Code extracted from drivers/block/genhd.c
 *  Copyright 2009 TiVo Inc. All Rights Reserved. and  Linus Torvalds. All Rights Reserved. 
 *  Re-organised Feb 1998 Russell King
 */

#include <linux/ctype.h>
#include "check.h"
#include "mac.h"

#ifdef CONFIG_PPC_PMAC
#include <asm/machdep.h>
extern void note_bootable_part(dev_t dev, int part, int goodness);
#endif

/*
 * Code to understand MacOS partition tables.
 */

static inline void mac_fix_string(char *stg, int len)
{
	int i;

	for (i = len - 1; i >= 0 && stg[i] == ' '; i--)
		stg[i] = 0;
}

int mac_partition(struct parsed_partitions *state, struct block_device *bdev)
{
	int slot = 1;
	Sector sect;
	unsigned char *data;
	int blk, blocks_in_map;
	unsigned secsize;
#ifdef CONFIG_PPC_PMAC
	int found_root = 0;
	int found_root_goodness = 0;
#endif
	struct mac_partition *part;
        struct tivo_bigpartition *bigpart;
	struct mac_driver_desc *md;
#ifdef CONFIG_TIVO
        int tivo_only = 0;
#endif

	/* Get 0th block and look at the first partition map entry. */
	md = (struct mac_driver_desc *) read_dev_sector(bdev, 0, &sect);
	if (!md) 
		return -1;
#ifdef CONFIG_TIVO
        switch(be16_to_cpu(md->signature)) {
            case MAC_DRIVER_MAGIC:
                secsize = be16_to_cpu(md->block_size);
                put_dev_sector(sect);
                data = read_dev_sector(bdev, secsize/512, &sect);
                if (!data) 
                    return -1;
                part = (struct mac_partition *)(data + secsize % 512);
                break;
            case TIVO_BOOT_MAGIC:
                tivo_only = 0;
                secsize = 512;
                data = read_dev_sector(bdev, 1, &sect);
                if (!data) 
                    return -1;
                part = (struct mac_partition *)data;
                break;
            default:
                put_dev_sector(sect);
                printk("block 0 has signature %x rather than %x or %x\n",
                    be16_to_cpu(md->signature), MAC_DRIVER_MAGIC,
                    TIVO_BOOT_MAGIC);
                return 0;
        }
#else
	if (be16_to_cpu(md->signature) != MAC_DRIVER_MAGIC) {
		put_dev_sector(sect);
		return 0;
	}
	secsize = be16_to_cpu(md->block_size);
	put_dev_sector(sect);
	data = read_dev_sector(bdev, secsize/512, &sect);
	if (!data) 
		return -1;
	part = (struct mac_partition *) (data + secsize%512);
#endif
	if (be16_to_cpu(part->signature) != MAC_PARTITION_MAGIC) {
		put_dev_sector(sect);
		return 0;		/* not a MacOS disk */
	}
#ifdef CONFIG_TIVO
	printk(" [%s]", (be16_to_cpu(md->signature) == TIVO_BOOT_MAGIC) ? "tivo" : "mac");
#else
	printk(" [mac]");
#endif
	blocks_in_map = be32_to_cpu(part->map_count);
	for (blk = 1; blk <= blocks_in_map; ++blk) {
		int pos = blk * secsize;
		put_dev_sector(sect);
		data = read_dev_sector(bdev, pos/512, &sect);
		if (!data) 
			return -1;
		part = (struct mac_partition *) (data + pos%512);
                switch (be16_to_cpu(part->signature)) {
                    case MAC_PARTITION_MAGIC:
#ifdef CONFIG_TIVO
                        if (tivo_only && strncmp(part->type, "TiVo", 4) != 0) {
                            continue;
                        }
#endif
                        put_partition(state, slot,
                                      be32_to_cpu(part->start_block) * (secsize/512),
                                      be32_to_cpu(part->block_count) * (secsize/512));

#ifdef CONFIG_TIVO
                        /* Check for media partition */
                        if (be32_to_cpu(part->status) & 0x100) {
                            state->parts[slot].driver_flags = 1;
                            printk("[M]");
                        }
#endif

#ifdef CONFIG_PPC_PMAC
                        /*
                         * If this is the first bootable partition, tell the
                         * setup code, in case it wants to make this the root.
                         */
                        if (machine_is(powermac)) {
                            int goodness = 0;

                            mac_fix_string(part->processor, 16);
                            mac_fix_string(part->name, 32);
                            mac_fix_string(part->type, 32);					
		    
                            if ((be32_to_cpu(part->status) & MAC_STATUS_BOOTABLE)
                                && strcasecmp(part->processor, "powerpc") == 0)
				goodness++;

                            if (strcasecmp(part->type, "Apple_UNIX_SVR2") == 0
                                || (strnicmp(part->type, "Linux", 5) == 0
                                    && strcasecmp(part->type, "Linux_swap") != 0)) {
				int i, l;

				goodness++;
				l = strlen(part->name);
				if (strcmp(part->name, "/") == 0)
                                    goodness++;
				for (i = 0; i <= l - 4; ++i) {
                                    if (strnicmp(part->name + i, "root",
                                                 4) == 0) {
                                        goodness += 2;
                                        break;
                                    }
				}
				if (strnicmp(part->name, "swap", 4) == 0)
                                    goodness--;
                            }

                            if (goodness > found_root_goodness) {
				found_root = blk;
				found_root_goodness = goodness;
                            }
                        }
#endif /* CONFIG_PPC_PMAC */

                        ++slot;
                        break;
#ifdef CONFIG_TIVO
                    case TIVO_BIGPARTITION_MAGIC:
                        bigpart = (struct tivo_bigpartition *) part;
                        put_partition(state, slot,
                                      be64_to_cpu(bigpart->start_block) * (secsize/512),
                                      be64_to_cpu(bigpart->block_count) * (secsize/512));
                        /* Show that we found the big-partition code */
                        printk("!");
                        /* Check for media partition */
                        if (be32_to_cpu(bigpart->status) & 0x100) {
                            state->parts[slot].driver_flags = 1;
                            printk("[M]");
                        }
                        ++slot;
#endif
                        break;
                    default:
                        break;
                }
	}
#ifdef CONFIG_PPC_PMAC
	if (found_root_goodness)
		note_bootable_part(bdev->bd_dev, found_root, found_root_goodness);
#endif

	put_dev_sector(sect);
	printk("\n");
	return 1;
}
