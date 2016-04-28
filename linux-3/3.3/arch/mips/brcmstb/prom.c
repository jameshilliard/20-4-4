/*
 * Copyright (C) 2009 Broadcom Corporation
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
 */

#include <linux/ctype.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/root_dev.h>
#include <linux/types.h>
#include <linux/smp.h>
#include <linux/bmoca.h>
#include <linux/version.h>
#include <linux/serial_reg.h>
#include <linux/io.h>
#include <linux/string.h>
#include <linux/compiler.h>
#include <linux/pci.h>
#include <linux/console.h>
#include <linux/mtd/mtd.h>

#include <asm/bootinfo.h>
#include <asm/r4kcache.h>
#include <asm/traps.h>
#include <asm/cacheflush.h>
#include <asm/mipsregs.h>
#include <asm/hazards.h>
#include <asm/smp-ops.h>
#include <asm/reboot.h>
#include <asm/fw/cfe/cfe_api.h>
#include <asm/fw/cfe/cfe_error.h>

#include <linux/brcmstb/brcmstb.h>
#include <spaces.h>

#include "../tivo/common/cmdline.h"

#ifdef CONFIG_TIVO
// support up to 4 macs
#define MACS 4
static u8 mac[MACS][6];
#endif

unsigned long __initdata cfe_seal;
unsigned long __initdata cfe_entry;
unsigned long __initdata cfe_handle;

#ifdef CONFIG_TIVO
extern void cmdline_whitelist(char *cmdline);
extern int tivo_init(void);
#endif

/***********************************************************************
 * CFE bootloader queries
 ***********************************************************************/

static int __init hex(char ch)
{
	if (ch >= 'a' && ch <= 'f')
		return ch-'a'+10;
	if (ch >= '0' && ch <= '9')
		return ch-'0';
	if (ch >= 'A' && ch <= 'F')
		return ch-'A'+10;
	return -1;
}

static int __init hex16(const char *b)
{
	int d0, d1;

	d0 = hex(b[0]);
	d1 = hex(b[1]);
	if ((d0 == -1) || (d1 == -1))
		return -1;
	return (d0 << 4) | d1;
}

void __init cfe_die(char *fmt, ...)
{
	char msg[128];
	va_list ap;
	int handle;
	unsigned int count;

	va_start(ap, fmt);
	vsprintf(msg, fmt, ap);
	strcat(msg, "\r\n");

	if (cfe_seal != CFE_EPTSEAL)
		goto no_cfe;

	/* disable XKS01 so that CFE can access the registers */

#if defined(CONFIG_CPU_BMIPS4380)
	__write_32bit_c0_register($22, 3,
		__read_32bit_c0_register($22, 3) & ~BIT(12));
#elif defined(CONFIG_CPU_BMIPS5000)
	__write_32bit_c0_register($22, 5,
		__read_32bit_c0_register($22, 5) & ~BIT(8));
#endif

	handle = cfe_getstdhandle(CFE_STDHANDLE_CONSOLE);
	if (handle < 0)
		goto no_cfe;

	cfe_write(handle, msg, strlen(msg));

	for (count = 0; count < 0x7fffffff; count++)
		mb();
	cfe_exit(0, 1);
	while (1)
		;

no_cfe:
	/* probably won't print anywhere useful */
	printk(KERN_ERR "%s", msg);
	BUG();

	va_end(ap);
}

static inline int __init parse_eth0_hwaddr(const char *buf, u8 *out)
{
	int i, t;
	u8 addr[6];

	for (i = 0; i < 6; i++) {
		t = hex16(buf);
		if (t == -1)
			return -1;
		addr[i] = t;
		buf += 3;
	}
	memcpy(out, addr, 6);

	return 0;
}

static inline int __init parse_eth0_mdio_mode(const char *buf,
	unsigned long *val)
{
	if (strcmp(buf, "boot") == 0 || strcmp(buf, "0") == 0)
		*val = 1;
	return 0;
}

static inline int __init parse_ulong(const char *buf, unsigned long *val)
{
	char *endp;
	unsigned long tmp;

	tmp = simple_strtoul(buf, &endp, 0);
	if (*endp == 0) {
		*val = tmp;
		return 0;
	}
	return -1;
}

static inline int __init parse_hex(const char *buf, unsigned long *val)
{
	char *endp;
	unsigned long tmp;

	tmp = simple_strtoul(buf, &endp, 16);
	if (*endp == 0) {
		*val = tmp;
		return 0;
	}
	return -1;
}

static inline int __init parse_cmdline(const char *buf, char *dst)
{
	strlcpy(dst, buf, COMMAND_LINE_SIZE);
	return 0;
}

static inline int __init parse_string(const char *buf, char *dst)
{
	strlcpy(dst, buf, CFE_STRING_SIZE);
	return 0;
}

static char __initdata cfe_buf[COMMAND_LINE_SIZE];
#ifdef CONFIG_PACESTB
	static unsigned char cfe_eth0_mac[50];
#endif

static void __init __maybe_unused cfe_read_configuration(void)
{
	int fetched = 0;

	printk(KERN_INFO "Fetching vars from bootloader... ");
	if (cfe_seal != CFE_EPTSEAL) {
		printk(KERN_CONT "none present, using defaults.\n");
		return;
	}

#define DPRINTK(...) do { } while (0)
/* #define DPRINTK(...) printk(__VA_ARGS__) */

#define FETCH(name, fn, arg) do { \
	if (cfe_getenv(name, cfe_buf, COMMAND_LINE_SIZE) == CFE_OK) { \
		DPRINTK("Fetch var '%s' = '%s'\n", name, cfe_buf); \
		fn(cfe_buf, arg); \
		fetched++; \
	} else { \
		DPRINTK("Could not fetch var '%s'\n", name); \
	} \
	} while (0)

#ifndef CONFIG_TIVO
	FETCH("ETH0_HWADDR", parse_eth0_hwaddr, brcm_primary_macaddr);
	FETCH("ETH0_MDIO_MODE", parse_eth0_mdio_mode, &brcm_eth0_no_mdio);
	FETCH("ETH0_PHY", parse_string, brcm_eth0_phy);
	FETCH("ETH0_PHYADDR", parse_string, brcm_eth0_phyaddr);
	FETCH("ETH0_SPEED", parse_ulong, &brcm_eth0_speed);

	FETCH("DRAM0_SIZE", parse_ulong, &brcm_dram0_size_mb);
	FETCH("DRAM1_SIZE", parse_ulong, &brcm_dram1_size_mb);
	FETCH("CFE_BOARDNAME", parse_string, brcm_cfe_boardname);
	FETCH("BOOT_FLAGS", parse_cmdline, arcs_cmdline);

	FETCH("LINUX_FFS_STARTAD", parse_hex, &brcm_mtd_rootfs_start);
	FETCH("LINUX_FFS_SIZE", parse_hex, &brcm_mtd_rootfs_len);
	FETCH("LINUX_PART_STARTAD", parse_hex, &brcm_mtd_kernel_start);
	FETCH("LINUX_PART_SIZE", parse_hex, &brcm_mtd_kernel_len);
	FETCH("OCAP_PART_STARTAD", parse_hex, &brcm_mtd_ocap_start);
	FETCH("OCAP_PART_SIZE", parse_hex, &brcm_mtd_ocap_len);
	FETCH("FLASH_SIZE", parse_ulong, &brcm_mtd_flash_size_mb);
	FETCH("FLASH_TYPE", parse_string, brcm_mtd_flash_type);

	printk(KERN_CONT "found %d vars.\n", fetched);
#else
#ifdef CONFIG_TIVO_PAC01
/* PACE Configuration for Ethernet */
	strcpy(brcm_eth0_phy,"RGMII");
	brcm_eth0_no_mdio=1;
#endif

/* NOTE: do board specific configuration here, don't rely on information from firmware */

#ifdef CONFIG_TIVO_GEN08
    memset(mac, 0xff, sizeof(mac));

    // Get MAC and MOCA
    if (cfe_getenv("MAC", cfe_buf, COMMAND_LINE_SIZE)==CFE_OK)
    {   
        parse_eth0_hwaddr(cfe_buf, mac[0]);
        *mac[0] &= ~1; // ensure not broadcast
    }    
    else
        // no MAC, use default
        memcpy(mac[0], (u8 []){0x02, 0x11, 0xd9, 0x11, 0x11, 0x11}, 6); 
    
    if (cfe_getenv("MOCA", cfe_buf, COMMAND_LINE_SIZE)==CFE_OK)
    {   
        parse_eth0_hwaddr(cfe_buf, mac[1]); // function is misnomer!
        *mac[1] &= ~1; // ensure not broadcast
    }    
    else if (!(*mac[0] & 2)) 
    {   
        // no MOCA, maybe clone MAC
        memcpy(mac[1], mac[0], 6); 
        *mac[1]|=2; // "locally administered"
    }   
    else    
        // otherwise, use default
        memcpy(mac[1], (u8 []){0x02, 0x11, 0xd9, 0x22, 0x22, 0x22}, 6); 

    // BRCM kernel allocates two MACs for MoCA: the first is the MoCA
    // GUID and the second is the eth1 MAC address. Use the same MAC 
    // address for both. 
    memcpy(mac[2], mac[1], 6); 
    
    FETCH("BOOT_FLAGS", parse_cmdline, arcs_cmdline);
#elif defined(CONFIG_PACESTB)
    if ((cfe_getenv("ETH0_HWADDR", cfe_buf, COMMAND_LINE_SIZE)==CFE_OK))
    {
        sprintf(cfe_eth0_mac,"brcmgenet.macaddr_eth=%s",cfe_buf);
    }    
    FETCH("BOOT_FLAGS", parse_cmdline, arcs_cmdline);
#else //  defined(CONFIG_TIVO_GEN10)
    if ((cfe_getenv("MAC", cfe_buf, COMMAND_LINE_SIZE)==CFE_OK) || 
        (cfe_getenv("ETH0_HWADDR", cfe_buf, COMMAND_LINE_SIZE)==CFE_OK))
    {
        parse_eth0_hwaddr(cfe_buf, mac[0]);
        *mac[0] &= ~1; // ensure not broadcast
    }    
    else
        // no MAC, use default
        memcpy(mac[0], (u8 []){0x02, 0x11, 0xd9, 0x11, 0x11, 0x11}, 6);

    
    // FETCH("ETH0_MDIO_MODE", parse_eth0_mdio_mode, &brcm_eth0_no_mdio); 
    // FETCH("ETH0_PHY", parse_string, brcm_eth0_phy);
    // FETCH("ETH0_SPEED", parse_ulong, &brcm_eth0_speed);

    // XXX - MOCA is hard-coded as of now
    memcpy(mac[1], (u8 []){0x02, 0x11, 0xd9, 0x22, 0x22, 0x22}, 6);

    // BRCM kernel allocates two MACs for MoCA: the first is the MoCA
    // GUID and the second is the eth1 MAC address. Use the same MAC 
    // address for both. 
    memcpy(mac[2], mac[1], 6);

    FETCH("BOOT_FLAGS", parse_cmdline, arcs_cmdline);
#endif //CONFIG_TIVO_GEN08
#endif //CONFIG_TIVO
}

/***********************************************************************
 * Early printk
 ***********************************************************************/

static unsigned long brcm_early_uart;

#define UART_REG(x)		(brcm_early_uart + ((x) << 2))
#define BAUD			115200

static void __init init_port(void)
{
	unsigned int divisor;

	BDEV_WR(UART_REG(UART_LCR), 0x3);	/* 8n1 */
	BDEV_WR(UART_REG(UART_IER), 0);		/* no interrupt */
	BDEV_WR(UART_REG(UART_FCR), 0);		/* no fifo */
	BDEV_WR(UART_REG(UART_MCR), 0x3);	/* DTR + RTS */

	BDEV_SET(UART_REG(UART_LCR), UART_LCR_DLAB);
#if defined(CONFIG_BRCM_IKOS)
	/* Reverse-engineer brcm_base_baud0 from the bootloader's setting */

	divisor = (BDEV_RD(UART_REG(UART_DLM)) << 8) |
		BDEV_RD(UART_REG(UART_DLL));
	brcm_base_baud0 = divisor * BAUD;
#endif
	divisor = (brcm_base_baud0 + BAUD/2) / BAUD;
	BDEV_WR(UART_REG(UART_DLL), divisor & 0xff);
	BDEV_WR(UART_REG(UART_DLM), (divisor >> 8) & 0xff);
	BDEV_UNSET(UART_REG(UART_LCR), UART_LCR_DLAB);
}

void prom_putchar(char x)
{
	while (!(BDEV_RD(UART_REG(UART_LSR)) & UART_LSR_THRE))
		;
	BDEV_WR(UART_REG(UART_TX), x);
}

static void __init brcm_setup_early_printk(void)
{
	char *arg = strstr(arcs_cmdline, "console=");
	int dev = CONFIG_BRCM_CONSOLE_DEVICE;
	const unsigned long base[] = {
		BCHP_UARTA_REG_START, BCHP_UARTB_REG_START,
#ifdef CONFIG_BRCM_HAS_UARTC
		BCHP_UARTC_REG_START,
#endif
		0, 0,
	};

	/*
	 * quick command line parse to pick the early printk console
	 * valid formats:
	 *   console=ttyS0,115200
	 *   console=0,115200
	 */
	while (arg && *arg != '\0' && *arg != ' ') {
		if ((*arg >= '0') && (*arg <= '3')) {
			dev = *arg - '0';
			if (base[dev] == 0)
				dev = 0;
			break;
		}
		arg++;
	}
	brcm_early_uart = base[dev];
	init_port();
}

#ifdef CONFIG_PACESTB
/* PACE - added for command line treatment */
static void  __init prom_init_cmdline( int argc, char * argv[] )
{
	char *cp;
	int actr;

#ifdef CONFIG_TIVO
	actr = 0; /* TiVo uses argv[0] */
#else
	actr = 1; /* Always ignore argv[0] */
#endif

	cp = &(arcs_cmdline[0]);
	while(actr < argc) {
		strcpy(cp, argv[actr]);
		cp += strlen(argv[actr]);
		*cp++ = ' ';
		actr++;
	}
#ifdef CONFIG_TIVO_PAC02
        /*
         * On CFE boxes booting Master Disks it is seen that there is not
         * enough time for eMMC to be ready to mount a rootfs. On CBL boxes
         * the ubifs is setup which provides the needed delay, so make the
         * root delay applicable only to CFE.
         */
	if (cfe_seal == CFE_EPTSEAL) {
                if(strstr(arcs_cmdline,"root=/dev/mmca")) {
                        if(! strstr(arcs_cmdline,"rootdelay")) {

                                /*
                                 * Add rootdelay argument with trailing space
                                 * and adjust 'cp' so it points to the end of
                                 * cmdline after the addition
                                 */

                                strcat(arcs_cmdline,"rootdelay=5 ");

                                cp += strlen("rootdelay=5 ");
                        }
                }
        }
#endif

	if (cp != &(arcs_cmdline[0])) /* get rid of trailing space */
		--cp;
	*cp = '\0';
}
#endif /* CONFIG_PACESTB */

/***********************************************************************
 * Main entry point
 ***********************************************************************/

void __init prom_init(void)
{
	char *ptr;
#ifdef CONFIG_PACESTB
	/* PACE - get the command line args passed in from zpiggy via head.S */
	/*        see asm/bootinfo.h for the details                         */
#ifdef CONFIG_TIVO
	int argc = 1; /* Tivo Bootloader space separates and strcats all the boot args */ 
#else
	int argc = fw_arg0;
#endif
	int argv = fw_arg1;
#endif

	cfe_init(cfe_handle, cfe_entry);

	bchip_check_compat();
	brcmstb_cpu_setup();

	/* default to SATA (where available) or MTD rootfs */
#ifdef CONFIG_BRCM_HAS_SATA
	ROOT_DEV = Root_SDA1;
#else
	ROOT_DEV = MKDEV(MTD_BLOCK_MAJOR, 0);
#endif
#ifndef CONFIG_TIVO
	root_mountflags &= ~MS_RDONLY;
#endif

	bchip_set_features();

#if defined(CONFIG_BRCM_IKOS_DEBUG)
	strcpy(arcs_cmdline, "debug initcall_debug");
#elif !defined(CONFIG_BRCM_IKOS)
#ifdef CONFIG_PACESTB
	/* PACE - bypass cfe_read_configuration if not called from CFE */
	if (cfe_seal == CFE_EPTSEAL) {
		cfe_read_configuration();
		cmdline_append(arcs_cmdline, COMMAND_LINE_SIZE, "PROM_TYPE=CFE");
        cmdline_append(arcs_cmdline, COMMAND_LINE_SIZE, "%s", cfe_eth0_mac);
#ifdef CONFIG_TIVO_PAC01
        cmdline_append(arcs_cmdline, COMMAND_LINE_SIZE, "ubi.mtd=ubi_nand");
#endif

        if (argv != 0)
            prom_init_cmdline(argc, (char **)argv);

	}
	/* PACE - get cmdline data from piggy in that case */

#ifdef CONFIG_TIVO
	else if (1) /* TBL ensures the args are in place at A1 register */
#else /* CONFIG_TIVO */
	else if ((argc > 1) && (argc <= 30))
#endif
	{
		prom_init_cmdline(argc, (char **)argv);
		printk("Command passed from piggy = \n");
		printk("%s\n", arcs_cmdline);
	}
	/* PACE - use defaults otherwise */
	else {
		printk("No CFE vars or piggy cmdline provided, using defaults\n");
	}
#else
	cfe_read_configuration();
#endif /* CONFIG_PACESTB */
#endif
#ifdef CONFIG_TIVO
    cmdline_whitelist(arcs_cmdline);
#endif
	brcm_setup_early_printk();

	/* provide "ubiroot" alias to reduce typing */
	if (strstr(arcs_cmdline, "ubiroot"))
	    cmdline_append(arcs_cmdline, COMMAND_LINE_SIZE, "ubi.mtd=rootfs rootfstype=ubifs "
			"root=ubi0:rootfs");

	ptr = strstr(arcs_cmdline, "memc1=");
	if (ptr)
		brcm_dram1_linux_mb = memparse(ptr + 6, &ptr) >> 20;
#ifdef CONFIG_PACESTB
        else
             brcm_dram1_linux_mb = MEMC1_SIZE_LEFT;
#endif

#ifdef CONFIG_TIVO
    /* Hook for TiVo-specific initialization */
    tivo_init();
#endif

	printk(KERN_INFO "Options: moca=%d sata=%d pcie=%d usb=%d\n",
		brcm_moca_enabled, brcm_sata_enabled,
		brcm_pcie_enabled, brcm_usb_enabled);

	bchip_early_setup();
	board_pinmux_setup();

	board_get_ram_size(&brcm_dram0_size_mb, &brcm_dram1_size_mb);

	do {
		unsigned long dram0_mb = brcm_dram0_size_mb, mb;

		mb = min(dram0_mb, BRCM_MAX_LOWER_MB);
		dram0_mb -= mb;

		add_memory_region(0, mb << 20, BOOT_MEM_RAM);
		if (!dram0_mb)
			break;

#ifdef CONFIG_BRCM_UPPER_MEMORY
		mb = min(dram0_mb, BRCM_MAX_UPPER_MB);
		dram0_mb -= mb;

		plat_wired_tlb_setup();
		add_memory_region(UPPERMEM_START, mb << 20, BOOT_MEM_RAM);
		if (!dram0_mb)
			break;
#endif

#if defined(CONFIG_HIGHMEM)
		add_memory_region(HIGHMEM_START, dram0_mb << 20, BOOT_MEM_RAM);
		break;
#endif
		/*
		 * We wound up here because the chip's architecture cannot
		 * make use of all MEMC0 RAM in Linux.  i.e. no suitable
		 * HIGHMEM or upper memory options are supported by the CPU.
		 *
		 * But we can still report the excess memory as a "bonus"
		 * reserved (bmem) region, so the application can manage it.
		 */
		mb = brcm_dram0_size_mb - dram0_mb;	/* Linux memory */
		if (!brcm_dram1_size_mb) {
			printk(KERN_INFO "MEMC0 split: %lu MB -> Linux; "
				"%lu MB -> extra bmem\n", mb, dram0_mb);
			brcm_dram1_size_mb = dram0_mb;
			brcm_dram1_start = UPPERMEM_START;
		}
	} while (0);

#if defined(CONFIG_HIGHMEM) && defined(CONFIG_BRCM_HAS_1GB_MEMC1)
	if (brcm_dram1_linux_mb > brcm_dram1_size_mb) {
		printk(KERN_WARNING "warning: 'memc1=%luM' exceeds "
			"available memory (%lu MB); ignoring\n",
			brcm_dram1_linux_mb, brcm_dram1_size_mb);
		brcm_dram1_linux_mb = 0;
	} else if (brcm_dram1_linux_mb)
		add_memory_region(MEMC1_START, brcm_dram1_linux_mb << 20,
			BOOT_MEM_RAM);
#else
	if (brcm_dram1_linux_mb) {
		printk(KERN_WARNING "warning: MEMC1 is not available on this "
			"system; ignoring\n");
		brcm_dram1_linux_mb = 0;
	}
#endif

	board_ebase_setup = &bmips_ebase_setup;
	register_smp_ops(&bmips_smp_ops);
}

/***********************************************************************
 * NMI hook
 ***********************************************************************/

static void (*nmi_fn)(struct pt_regs *);

static int brcm_nmi_notifier(struct notifier_block *nb,
	unsigned long val, void *v)
{
	nmi_fn(NULL);
	return 0;
}

void brcm_set_nmi_handler(void (*fn)(struct pt_regs *))
{
	nmi_notifier(brcm_nmi_notifier, 0);
}
EXPORT_SYMBOL(brcm_set_nmi_handler);

/***********************************************************************
 * Miscellaneous utility functions
 ***********************************************************************/

static char brcm_system_type[64];

const char *get_system_type(void)
{
	u32 class = BRCM_CHIP_ID();

	if (class >> 16 == 0)
		class >>= 8;
	else
		class >>= 12;

	snprintf(brcm_system_type, 64, "BCM%04x%02X %s platform",
		BRCM_CHIP_ID(), BRCM_CHIP_REV() + 0xa0,
		class == 0x35 ? "DTV" :
		(class == 0x76 ? "DVD" : "STB"));

	return (const char *)brcm_system_type;
}

void __init prom_free_prom_memory(void) {}

static void brcm_machine_restart_mips(char *command)
{
	/* On ARM the argument needs to be const */
	brcm_machine_restart(command);
}

void __init plat_mem_setup(void)
{
	_machine_restart = brcm_machine_restart_mips;
	_machine_halt = brcm_machine_halt;
	pm_power_off = brcm_machine_halt;

	brcm_wraparound_check();

#ifdef CONFIG_TIVO
    panic_timeout = 3;
#else
	panic_timeout = 180;
#endif

#ifdef CONFIG_PCI
	pcibios_plat_setup = brcmstb_pcibios_setup;
#endif

	add_preferred_console("ttyS", CONFIG_BRCM_CONSOLE_DEVICE, "115200");
}

#ifdef CONFIG_TIVO
int brcm_alloc_macaddr(u8 *buf)
{
    static int next=0;
    if (next < MACS && *mac[next] != 0xFF)  memcpy(buf, &mac[next], ETH_ALEN);
    else 
        // mac undefined, fake it
        memcpy(buf, (u8 []){0xFE, next, next, next, next, next}, ETH_ALEN);
    next++;
    return 0;
}
EXPORT_SYMBOL(brcm_alloc_macaddr);
#endif
