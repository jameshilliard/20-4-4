/*
 *
 * Copyright (C) 2009 TiVo Inc. All Rights Reserved.
 */
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/bootmem.h>
#include <asm-generic/page.h>
#include <asm/uaccess.h>
#include <asm/io.h>
#include <asm/tivo/seals.h>

#define BSI_BOOTPARMS_LOC 0x81000400

static unsigned int slicesimg, nandrfsimg,bfcaimg;
static int slices_sz = 0, nandrfs_sz = 0, bfca_sz = 0;
static int found_bsi_seal = 0;
extern unsigned int cfe_seal;

void map_sdi(){
        unsigned long *p;
        char msg[256];
        int count,i;
        unsigned int tag;
        unsigned long len, val;   

        p = BSI_BOOTPARMS_LOC;
        if ( *p == BSISEAL ){ /*BSI Seal*/
                sprintf(msg,"Found BSI Seal @%08x\n", p);
                uart_puts(msg);
                found_bsi_seal = 1;
                *p++ = 0; /*clear seal*/
                p++; /*version*/
                count = *p++;
                i = 0;
                sprintf(msg,"Found %d TLVs\n",count);
                uart_puts(msg);
                while ( i < count ){
                        tag = *p++;
                        len = *p++;
                        val = *p++;
                        if ( tag == NANDRFS ){
                                nandrfs_sz = len;
                                nandrfsimg = val;
                                //reserve_bootmem(__pa(nandrfsimg), nandrfs_sz);
                                sprintf(msg,"NANDrfs @%08x len %d\n",nandrfsimg, nandrfs_sz);
                                uart_puts(msg);
                        }
                        if ( tag == SLICES ){
                                slices_sz = len;
                                slicesimg = val;
                                //reserve_bootmem(__pa(slicesimg), slices_sz);
                                sprintf(msg,"Slices @%08x len %d\n",slicesimg, slices_sz);
                                uart_puts(msg);
                        }
                        i++;
                }
        }
}

#ifdef CONFIG_PROC_FS
static int nandrfs_read_proc(char *page, char **start, off_t off,
                                 int count, int *eof, void *data)
{
        if ( off > nandrfs_sz ){
                *eof = 1;
                return 0;
        }
        if ( off+count > nandrfs_sz ){
                count = nandrfs_sz - off;
        }
        memcpy(page, nandrfsimg + off , count);
        *start = count;
        return count;
}

static int slices_read_proc(char *page, char **start, off_t off,
				 int count, int *eof, void *data)
{
        if ( off > slices_sz ){
                *eof = 1;
                return 0;
        }
        if ( off+count > slices_sz ){
                count = slices_sz - off;
        }
        memcpy(page, slicesimg + off , count);
        *start = count;
        return count;
}

void sdi_proc(){
        struct proc_dir_entry *entry;

        if ( (cfe_seal != BSISEAL) || (!found_bsi_seal))
                return;
        
        entry = create_proc_entry("nandrfs", S_IFREG | S_IRUSR | S_IWUSR, 0);
        if (entry) 
        {
                printk("Creating proc entry for nandrfs\n");
                entry->read_proc = nandrfs_read_proc;
                entry->uid   = 0;
                entry->gid   = 0;
                entry->size  = nandrfs_sz;
        }
        entry = create_proc_entry("slices", S_IFREG | S_IRUSR | S_IWUSR, 0);
        if (entry) 
        {
                printk("Creating proc entry for slices\n");
                entry->read_proc = slices_read_proc;
                entry->uid   = 0;
                entry->gid   = 0;
                entry->size  = slices_sz;
        }
}
#endif
