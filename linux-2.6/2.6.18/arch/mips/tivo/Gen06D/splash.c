/*
 *
 * Copyright (C) 2009 TiVo Inc. All Rights Reserved.
 */
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/bootmem.h>
#include <linux/mm.h>
#include <linux/pfn.h>
#include <asm-generic/page.h>
#include <asm/uaccess.h>
#include <asm/io.h>
#include <asm/brcmstb/brcm97401c0/bchp_common.h>
#include <asm/tivo/seals.h>

extern unsigned long totalram_pages;
extern unsigned int cfe_seal;
extern void fastcall __init __free_pages_bootmem(struct page *page,
						unsigned int order);
extern unsigned long contigmem_start;
extern unsigned long contigmem_total;

static unsigned int fb_memory_addr_0, fb_memory_addr_1;
static int fb_size_in_bytes_0, fb_size_in_bytes_1;
static unsigned int rdc_pgs[8]; /*4 outputs*/
static DECLARE_MUTEX(FBProcMutex);
static int framebuf_write_proc_freed=0;

void reserve_fb_as_bootmem()
{
        if ( cfe_seal == CFESEAL )
                return ;
#define BCM_REG_PHYS            0x10000000
#define BCM_REG_BASE            ( KSEG1 + BCM_REG_PHYS )
#define SYS_REG(offset)         ( BCM_REG_BASE + (offset) )
#define BCHP_RDC_desc_0_addr    0x00102200
#define BCHP_RDC_desc_0_config  0x00102204 

        unsigned long ulSlotOffset;
        int stride, vsize;
        int i,j;
        unsigned int pg;
        
        printk("Reserving RUL area\n");
        /*find pages where RULs are*/
        memset(rdc_pgs,0,sizeof(rdc_pgs));
        for ( i = 0; i < 4 ; i++){
            ulSlotOffset = 4 * i * sizeof(unsigned long);
            pg = (*(unsigned int *)SYS_REG(BCHP_RDC_desc_0_addr + ulSlotOffset))& ~(PAGE_SIZE-1);
            rdc_pgs[(i*2)]=pg;
            rdc_pgs[(i*2)+1]=pg+PAGE_SIZE;
        }
        /*remove duplicate pages from list*/
        for ( i = 0 ; i < 8 ; i++ ){
            for ( j = i+1 ; j < 8 ; j++ ){
                if ( rdc_pgs[j] == rdc_pgs[i] ){
                        rdc_pgs[j] = 0;
                }
            }
        }
        /*reserve*/
        for ( i = 0; i < 8 ; i++){
            if ( rdc_pgs[i] ){
                reserve_bootmem(rdc_pgs[i], PAGE_SIZE);
                printk("Reserving page @0x%08x\n",__va(rdc_pgs[i]));
            }
        }

        stride = *(unsigned int *)SYS_REG(BCHP_GFD_0_REG_START+0x44);
        vsize = *(unsigned int *)SYS_REG(BCHP_GFD_0_REG_START+0x5C) & 0x7FF;
        fb_memory_addr_0 = *(unsigned int *)SYS_REG(BCHP_GFD_0_REG_START+0x40);
        fb_size_in_bytes_0 = vsize * stride;
        fb_size_in_bytes_0 += (PAGE_SIZE - (fb_size_in_bytes_0 & (PAGE_SIZE-1)));

        printk("Reserving Bootloader Framebuffer %d bytes @%08x\n",fb_size_in_bytes_0,__va(fb_memory_addr_0));
        reserve_bootmem(fb_memory_addr_0, fb_size_in_bytes_0);

        stride = *(unsigned int *)SYS_REG(BCHP_GFD_1_REG_START+0x44);
        vsize = *(unsigned int *)SYS_REG(BCHP_GFD_1_REG_START+0x5C) & 0x7FF;
        fb_memory_addr_1 = *(unsigned int *)SYS_REG(BCHP_GFD_1_REG_START+0x40);
        fb_size_in_bytes_1 = vsize * stride;
        fb_size_in_bytes_1 += (PAGE_SIZE - (fb_size_in_bytes_1 & (PAGE_SIZE-1)));

        printk("Reserving Bootloader Framebuffer %d bytes @%08x\n",fb_size_in_bytes_1,__va(fb_memory_addr_1));
        reserve_bootmem(fb_memory_addr_1, fb_size_in_bytes_1);
}

#ifdef CONFIG_PROC_FS
static int framebuf_read_proc(char *buffer, char **start, off_t off,
                                 int count, int *eof, void *data)
{
        int len = 0;
        if(count < 96)
                return -EINVAL;
        len += snprintf(buffer, 96,  "Bootloader Framebuffers %d bytes @%08x and %d bytes @%08x\n",
                        fb_size_in_bytes_0, __va(fb_memory_addr_0),fb_size_in_bytes_1,__va(fb_memory_addr_1));
        *eof= 1;
        return len;
}

static int framebuf_write_proc (struct file *file, const char __user *buffer, unsigned long count, void *data)
{
        int i , pgs;
        unsigned long pg_virt_addr;

        down(&FBProcMutex);
        if ( framebuf_write_proc_freed == 1 )
            return -1;

        printk("Free'ing Bootloader Framebuffer of %d bytes @%08x\n",fb_size_in_bytes_0, __va(fb_memory_addr_0));
        pgs = (fb_size_in_bytes_0/PAGE_SIZE) + ((fb_size_in_bytes_0%PAGE_SIZE) ? 1: 0 );
        printk("Trying to free %d pages\n",pgs);
        for ( i = 0 ; i < pgs ; i++){
                pg_virt_addr = __va(fb_memory_addr_0+(i*PAGE_SIZE));
                if ( pg_virt_addr < contigmem_start || pg_virt_addr > contigmem_start+contigmem_total){
                        __free_pages_bootmem(virt_to_page(__va(fb_memory_addr_0+(i*PAGE_SIZE))),0);
                        totalram_pages++;
                }
                else
                        continue;
        }

        printk("Free'ing Bootloader Framebuffer of %d bytes @%08x\n",fb_size_in_bytes_1, __va(fb_memory_addr_1));
        pgs = (fb_size_in_bytes_1/PAGE_SIZE) + ((fb_size_in_bytes_1%PAGE_SIZE) ? 1: 0 );
        printk("Trying to free %d pages\n",pgs);
        for ( i = 0 ; i < pgs ; i++){
                pg_virt_addr = __va(fb_memory_addr_1+(i*PAGE_SIZE));
                if ( pg_virt_addr < contigmem_start || pg_virt_addr > contigmem_start+contigmem_total){
                        __free_pages_bootmem(virt_to_page(__va(fb_memory_addr_1+(i*PAGE_SIZE))),0);
                        totalram_pages++;
                }
                else
                        continue;
        }
        
        printk("Free'ing RUL area\n");
        /*reserve*/
        for ( i = 0; i < 8 ; i++){
            if ( rdc_pgs[i] ){
                //printk("Free'ing RUL 0x%08x\n",__va(rdc_pgs[i]));
                pg_virt_addr = __va(rdc_pgs[i]);
                if ( pg_virt_addr < contigmem_start || pg_virt_addr > contigmem_start+contigmem_total){
                        __free_pages_bootmem(virt_to_page(__va(rdc_pgs[i])), 0);
                        totalram_pages++;
                }
                else
                        continue;
            }
        }
        framebuf_write_proc_freed = 1;
        up(&FBProcMutex);

        printk("Removing bootframebuf proc entry\n");
        remove_proc_entry("bootframebuf", NULL);
        return 0;
}

void framebuf_proc(){
        struct proc_dir_entry *entry;

        if ( cfe_seal == CFESEAL )
                return;

        entry = create_proc_entry("bootframebuf", S_IFREG | S_IRUSR | S_IWUSR, 0);
        if (entry) 
        {
                printk("Creating proc entry for Boot Frame Buffer\n");
                entry->read_proc = framebuf_read_proc;
                entry->write_proc = framebuf_write_proc;
                entry->uid   = 0;
                entry->gid   = 0;
                entry->size  = 96;
        }
}
#endif
