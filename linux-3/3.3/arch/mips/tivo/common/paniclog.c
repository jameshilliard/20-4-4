/* 
 * This is to write out to the disk in the case that we have
 * essentially a non-functional TiVo.  The idea here is we're
 * going to write out ot the swap partition some log of what
 * happened.  The two cases to get us here generally are
 * a kernel panic, or a watch dog fire.
 *
 * We are going to write out to swap, on boot up, we'll scan
 * the swap before turning it on to see our signature.  If
 * the signature is present, then we'll extract the data
 * and put it in our normal logs.
 *
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/notifier.h>
#include <linux/ahci_deadsystem.h>
#include <linux/string.h>
#include <linux/paniclog.h>
#include <linux/plog.h>
#include <linux/proc_fs.h>

#define PLOG_CAPTURE_SIZE      (32*1024)     /* 32k seems like a fun number */

int pio_write_plog(int secoffset, u32 *csum) {
	/*
	 * So, this is kind of strange.  We start at the end,
	 * and iterate backwards.  So, we'll write our last
	 * sector first, and our first sector last.
	 */

#if defined(CONFIG_TIVO_PLOG)

	int cursect = secoffset + (PLOG_CAPTURE_SIZE-512)/512;
	u8 secbuf[512];
	int ret;
	
	while(cursect >= secoffset) {
#if !defined(CONFIG_TIVO_PANICLOG_IN_RAM)
		plog_destructive_liforead((int*)secbuf, 512/4);
#else
		memset(secbuf, 0, 512);
#endif
		ret=ahci_write_buffer_to_swap(secbuf, 512, cursect, csum);
		if(ret < 0)
			return ret;
		cursect--;
	}
	return PLOG_CAPTURE_SIZE/512;
#else
	return 0;
#endif
}

#if defined(CONFIG_MODULES)

#define MAX_SYMNAME_SIZE 120

int calc_sym_size(struct kernel_symbol *sym) {
        int size;
        for(size=0;size < MAX_SYMNAME_SIZE && sym->name[size] != '\0';size++);
        return size + 4 + 1;
}
#endif

int pio_write_syms(int secoffset, u32 *csum) {
#if defined(CONFIG_MODULES)

        /*
         * Iterate through modules, printing out binary data
         * when we flow over the buffer.  Oh to have an unlimited
         * stack.  Hrmph.
         */


        int wrsects = 0;
        u8 secbuf[512];
        int symsizeused=0;

        extern struct list_head modules;
        struct module *mod;
        struct kernel_symbol *sym;
        int i;

        /*Kernel Loadable Module information*/
        list_for_each_entry(mod, &modules, list) {
                int symsize;
                char sym_name[MAX_SYMNAME_SIZE];
                char mod_name[MAX_SYMNAME_SIZE];
                unsigned long load_addr;
                strcpy(mod_name,mod->name);
#ifdef CONFIG_TIVO_MOJAVE
                if(!strcmp(mod->name,"bcm7401")){
                        strcat(mod_name, "_HR22");
                }
#endif
                /* 
                 * it appears that all kernel modules are present in /platform/lib/modules/
                 * in Gen07 and Gen06D 
                 */
                sprintf(sym_name, "__insmod_%s_O/platform/lib/modules/%s.ko_%u",mod_name, mod_name,
                                mod->init_size + mod->core_size);

                {
                        int size;
                        for(size=0;size < MAX_SYMNAME_SIZE && sym_name[size] != '\0';size++);
                        symsize = size + 4 + 1;

                        if(symsizeused + symsize > 512) {
                                /* Time to flush out the secbuf */
                                if(symsizeused < 512) {
                                        memset(secbuf + symsizeused, 0,
                                                        512 - symsizeused);
                                }
#if defined(CONFIG_TIVO_PANICLOG_IN_RAM)
                                memset(secbuf, 0, 512);
#endif
                                ahci_write_buffer_to_swap(secbuf, 512,
                                		wrsects++ + secoffset, csum);
                                symsizeused=0;
                        }
                        load_addr = htonl((unsigned long)mod->module_core);
                        memcpy((void*)(secbuf + symsizeused), &load_addr, 4);
                        memcpy((void*)(secbuf + symsizeused + 4), sym_name,
                                        symsize - 5);

                        secbuf[symsize + symsizeused - 1] = '\0';
                        symsizeused += symsize;
                }

                for(i = mod->num_syms , sym = (struct kernel_symbol *)mod->syms ; i > 0; --i , ++sym) {
                        symsize = calc_sym_size(sym);

                        if(symsizeused + symsize > 512) {
                                /* Time to flush out the secbuf */
                                if(symsizeused < 512) {
                                        memset(secbuf + symsizeused, 0,
                                                        512 - symsizeused);
                                }
#if defined(CONFIG_TIVO_PANICLOG_IN_RAM)
                                memset(secbuf, 0, 512);
#endif
                                ahci_write_buffer_to_swap(secbuf, 512,
                                		wrsects++ + secoffset, csum);
                                symsizeused=0;
                        }
                        load_addr = htonl((unsigned long)sym->value);
                        memcpy((void*)(secbuf + symsizeused),&load_addr, 4);
                        memcpy((void*)(secbuf + symsizeused + 4), sym->name,
                                        symsize - 5);

                        secbuf[symsize + symsizeused - 1] = '\0';
                        symsizeused += symsize;
                }
        }

        if(symsizeused > 0) {
                if(symsizeused < 512) {
                        memset(secbuf + symsizeused, 0, 512 - symsizeused);
                }
#if defined(CONFIG_TIVO_PANICLOG_IN_RAM)
                memset(secbuf, 0, 512);
#endif
                ahci_write_buffer_to_swap(secbuf, 512, wrsects++ + secoffset,
                		csum);
        }

        return wrsects;

#else
        return 0;
#endif
}

void pio_print_syms(void) {
#if defined(CONFIG_MODULES)

	/*  
	 * Iterate through modules, print out all kernel module address
	 */

	extern struct list_head modules;
	struct module *mod;
	unsigned long load_addr;

	/*Kernel Loadable Module information*/
	printk("\n");
	list_for_each_entry(mod, &modules, list) {
		char sym_name[MAX_SYMNAME_SIZE];
		load_addr = (unsigned long)mod->module_core;
		sprintf(sym_name, "__insmod_%s_O/platform/lib/modules/%s.ko_%u",mod->name, mod->name,
				mod->init_size + mod->core_size);
		printk("0x%08lX-%s\n\n",(unsigned long)mod->module_core, sym_name);
	}
#endif
}

int pio_write_header(int syslogsize, int plogsize,
		     int symtabsize, unsigned int checksum) {
	union {
		struct panic_log_header plh;
		u8 sector[512];
	} secbuf;
	memset(secbuf.sector, 0, 512);
	secbuf.plh.syslogsize=syslogsize;
	secbuf.plh.plogsize=plogsize;
	secbuf.plh.symtabsize=symtabsize;
	secbuf.plh.checksum=checksum;
	memcpy(secbuf.plh.signature, PL_SIGNATURE, 11);
	strncpy(secbuf.plh.banner, linux_banner, PL_MAX_BANNER_SIZE);
	return ahci_write_buffer_to_swap(secbuf.sector, 512, 0, NULL);
}

int do_panic_log_nocli() {
	static int panic_logged=0;
	char *syslog_data[4];

	if(panic_logged == 0) {
		int wrsects, plogsects, symsects;
		u32 csum=0;

		panic_logged=1; /* This only happens once */
		get_syslog_data(syslog_data);

		/* Write out syslog offset by one */
		wrsects=ahci_write_buffer_to_swap(syslog_data[0],
				syslog_data[1] - syslog_data[0], 1, &csum);
		if(wrsects < 0)
			return NOTIFY_DONE;

		/* Write out plog after syslog */
		plogsects = pio_write_plog(wrsects + 1, &csum);
		if(plogsects < 0)
			return NOTIFY_DONE;

		wrsects += plogsects;
		symsects = pio_write_syms(wrsects + 1, &csum);
		wrsects = pio_write_header(syslog_data[1] - syslog_data[0],
					   plogsects * 512, symsects * 512,
					   csum);
		if(wrsects < 0)
			return NOTIFY_DONE;

		printk("Panic logged\n");
	}

	return NOTIFY_DONE;
}

int do_panic_log(struct notifier_block *self, unsigned long command, void *ptr)
{
	int ret;
	unsigned long flags;
	int cwritten= 0;
	local_irq_save(flags);
#if 0 && !defined(CONFIG_TIVO_DEVEL)
        /* This has not been tested on linux-2.6, probably it is broken */
	cwritten = ahci_write_corefile(1);
#endif
	printk("Core of %d bytes written\n", cwritten);
	ret = do_panic_log_nocli();
	pio_print_syms();
	local_irq_restore(flags);
	return ret;
}

static struct notifier_block panic_log_block = { do_panic_log, NULL, 10 };

static int __init panic_log_init(void) {
#ifdef CONFIG_TIVO_DEVEL
	/* Register /proc/uploadpanic so we get network feedback */
	struct proc_dir_entry *uploadpanic;
	/* Checked by rc.CheckPanicLog.sh */
	uploadpanic = create_proc_entry("uploadpanic", 0, 0);
#endif
	atomic_notifier_chain_register(&panic_notifier_list, &panic_log_block);
	printk("Kernel Panic Logger registered\n");
	return 0;
}

module_init(panic_log_init);
