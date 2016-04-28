/*
 *  fs/partitions/mac.c
 *
 *  Code extracted from drivers/block/genhd.c
 *  Copyright (C) 1991-1998  Linus Torvalds
 *  Re-organised Feb 1998 Russell King
 */

// Copyright 2012 by TiVo Inc. All Rights Reserved

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

int mac_partition(struct parsed_partitions *state)
{
	Sector sect;
	unsigned char *data;
	int slot, blocks_in_map;
	unsigned secsize;
#ifdef CONFIG_PPC_PMAC
	int found_root = 0;
	int found_root_goodness = 0;
#endif
	struct mac_partition *part;
	struct mac_driver_desc *md;

	/* Get 0th block and look at the first partition map entry. */
	md = read_part_sector(state, 0, &sect);
	if (!md)
		return -1;
#ifdef CONFIG_TIVO_DISKPARTITION
	switch(md->signature) {
		case MAC_DRIVER_MAGIC:
			secsize = md->block_size;
			put_dev_sector(sect);
			data = read_part_sector(state, secsize/512, &sect);
			if (!data) 
				return -1;
			part = (struct mac_partition *)(data + secsize % 512);
			break;
		case TIVO_BOOT_MAGIC:
			state->bdev->bd_disk->tivo=1; // tell sysfs this is a tivo disk
			secsize = 512;
			data = read_part_sector(state, 1, &sect);
			if (!data) 
				return -1;
			part = (struct mac_partition *)data;
			break;
		default:
			put_dev_sector(sect);
			printk("block 0 has signature %x rather than %x or %x\n",
				md->signature, MAC_DRIVER_MAGIC,
				TIVO_BOOT_MAGIC);
			return 0;
	}
    if (part->signature != MAC_PARTITION_MAGIC) {
        put_dev_sector(sect);
        return 0;       /* not a MacOS disk */
    }
    blocks_in_map = part->map_count;
#else
	if (be16_to_cpu(md->signature) != MAC_DRIVER_MAGIC) {
		put_dev_sector(sect);
		return 0;
	}
	secsize = be16_to_cpu(md->block_size);
	put_dev_sector(sect);
	data = read_part_sector(state, secsize/512, &sect);
	if (!data)
		return -1;
	part = (struct mac_partition *) (data + secsize%512);
    if (be16_to_cpu(part->signature) != MAC_PARTITION_MAGIC) {
        put_dev_sector(sect);
        return 0;       /* not a MacOS disk */
    }
    blocks_in_map = be32_to_cpu(part->map_count);
#endif

	if (blocks_in_map < 0 || blocks_in_map >= DISK_MAX_PARTS) {
		put_dev_sector(sect);
		return 0;
	}

#ifdef CONFIG_TIVO_DISKPARTITION
	if (md->signature == TIVO_BOOT_MAGIC)
	{
		strlcat(state->pp_buf," [tivo]", PAGE_SIZE);
	}
	else
#endif
	strlcat(state->pp_buf, " [mac]", PAGE_SIZE);
	for (slot = 1; slot <= blocks_in_map; ++slot) {
		int pos = slot * secsize;
		put_dev_sector(sect);
		data = read_part_sector(state, pos/512, &sect);
		if (!data)
			return -1;
		part = (struct mac_partition *) (data + pos%512);
#ifdef CONFIG_TIVO_DISKPARTITION        
		if (part->signature == TIVO_BIGPARTITION_MAGIC) {
			struct tivo_bigpartition *bigpart = (void *) part;
			put_partition(state, slot,
				bigpart->start_block * (secsize/512),
				bigpart->block_count * (secsize/512));
			/* Show that we found the big-partition code */
			strlcat(state->pp_buf,"!",PAGE_SIZE);
			/* Check for media partition */
			if (bigpart->status & 0x100) {
				state->parts[slot].driver_flags = 1;
				strlcat(state->pp_buf,"[M]",PAGE_SIZE);
			}
			continue;
		}
		if (part->signature != MAC_PARTITION_MAGIC)
			break;
		put_partition(state, slot,
			part->start_block * (secsize/512),
			part->block_count * (secsize/512));

		/* Check for media partition */
		if (part->status & 0x100) {
			state->parts[slot].driver_flags = 1;
		    strlcat(state->pp_buf,"[M]",PAGE_SIZE);
		}
#else
        if (be16_to_cpu(part->signature) != MAC_PARTITION_MAGIC)
            break;
        put_partition(state, slot,
            be32_to_cpu(part->start_block) * (secsize/512),
            be32_to_cpu(part->block_count) * (secsize/512));

		if (!strnicmp(part->type, "Linux_RAID", 10))
			state->parts[slot].flags = ADDPART_FLAG_RAID;
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
				found_root = slot;
				found_root_goodness = goodness;
			}
		}
#endif /* CONFIG_PPC_PMAC */
	}
#ifdef CONFIG_PPC_PMAC
	if (found_root_goodness)
		note_bootable_part(state->bdev->bd_dev, found_root,
				   found_root_goodness);
#endif

	put_dev_sector(sect);
	strlcat(state->pp_buf, "\n", PAGE_SIZE);
	return 1;
}
