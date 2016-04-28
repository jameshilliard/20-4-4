/*
 * arch/mips/brcmstb/common/cfe_call.c
 *
 * Copyright (C) 2001-2004 Broadcom Corporation
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
 * Interface between CFE boot loader and Linux Kernel.
 *
 * 
 */
#include <linux/ctype.h>
#include <linux/init.h>
#include <linux/mm.h>
#include <asm/bootinfo.h>

#include "../common/cfe_xiocb.h"
#include <asm/brcmstb/common/cfe_call.h>

#ifdef CONFIG_TIVO_KONTIKI
#include <linux/module.h> // SYM EXPORT */
#include <asm/brcmstb/common/brcmstb.h>
#endif

extern unsigned int cfe_seal;

#ifdef CONFIG_TIVO_KONTIKI
cfeEnvVarPairs_t gCfeEnvVarPairs[] = {
	{ "LINUX_FFS_STARTAD", 		"LINUX_FFS_SIZE" },
	{ "SPLASH_PART_STARTAD", 	"SPLASH_PART_SIZE" },
	{ "LINUX_PART_STARTAD", 		"LINUX_PART_SIZE" },
	{ "OCAP_PART_STARTAD", 		"OCAP_PART_SIZE" },
/*
	{ "DRAM0_OFFSET", 			"DRAM0_SIZE" },
	{ "DRAM1_OFFSET", 			"DRAM1_SIZE" },
*/
	{ NULL, NULL },
};
EXPORT_SYMBOL(gCfeEnvVarPairs);

#else  /* if !defined(CONFIG_TIVO_KONTIKI) */
#define ETH_HWADDR_LEN 18	/* 12:45:78:ab:de:01\0 */
#endif /* !defined(CONFIG_TIVO_KONTIKI) */

/*
 * Convert ch from a hex digit to an int
 */
#ifdef CONFIG_TIVO_KONTIKI
static inline int hex(char ch)
#else
static inline int hex(unsigned char ch)
#endif
{
	if (ch >= 'a' && ch <= 'f')
		return ch-'a'+10;
	if (ch >= '0' && ch <= '9')
		return ch-'0';
	if (ch >= 'A' && ch <= 'F')
		return ch-'A'+10;
	return -1;
}

#ifdef CONFIG_TIVO_KONTIKI
static int hex16(char *b)
{
	int d0, d1;

	d0 = hex(b[0]);
	d1 = hex(b[1]);
	if((d0 == -1) || (d1 == -1))
		return(-1);
	return((d0 << 4) | d1);
}
#endif /* CONFIG_TIVO_KONTIKI */

/* NOTE: do not put this on the stack.  It can exceed 3kB. */
static cfe_xiocb_t cfeparam;

#ifdef CONFIG_TIVO_KONTIKI
int get_cfe_env_variable(char *name_ptr, char *val_ptr, int val_length)
{
	int res = 0;

	cfeparam.xiocb_fcode  = CFE_CMD_ENV_GET;
	cfeparam.xiocb_status = 0;
	cfeparam.xiocb_handle = 0;
	cfeparam.xiocb_flags  = 0;
	cfeparam.xiocb_psize  = sizeof(xiocb_envbuf_t);
	cfeparam.plist.xiocb_envbuf.name_ptr    = (unsigned int)name_ptr;
	cfeparam.plist.xiocb_envbuf.name_length = strlen(name_ptr);
	cfeparam.plist.xiocb_envbuf.val_ptr     = (unsigned int)val_ptr;
	cfeparam.plist.xiocb_envbuf.val_length  = val_length;

	if (cfe_seal == CFE_SEAL) {
		res = cfe_call(&cfeparam);
	}
	else
		res = -1;

	return (res);
}
#else  /* if !defined(CONFIG_TIVO_KONTIKI) */
int get_cfe_env_variable(cfe_xiocb_t *cfeparam,
				void * name_ptr, int name_length,
				void * val_ptr,  int val_length)
{
	int res = 0;

	cfeparam->xiocb_fcode  = CFE_CMD_ENV_GET;
	cfeparam->xiocb_status = 0;
	cfeparam->xiocb_handle = 0;
	cfeparam->xiocb_flags  = 0;
	cfeparam->xiocb_psize  = sizeof(xiocb_envbuf_t);
	cfeparam->plist.xiocb_envbuf.name_ptr    = (unsigned int)name_ptr;
	cfeparam->plist.xiocb_envbuf.name_length = name_length;
	cfeparam->plist.xiocb_envbuf.val_ptr     = (unsigned int)val_ptr;
	cfeparam->plist.xiocb_envbuf.val_length  = val_length;

	if (cfe_seal == CFE_SEAL) {
		res = cfe_call(cfeparam);
	}
	else
		res = -1;

	return (res);
}
#endif /* !defined(CONFIG_TIVO_KONTIKI) */

int get_cfe_hw_info(cfe_xiocb_t* cfe_boardinfo)
{
	int res = -1;
	
	/*
	** Get CFE HW INFO
	*/
	memset(cfe_boardinfo, 0, sizeof(cfe_xiocb_t));
	cfe_boardinfo->xiocb_fcode  = CFE_CMD_GET_BOARD_INFO;
	cfe_boardinfo->xiocb_status = 0;
	cfe_boardinfo->xiocb_handle = 0;
	cfe_boardinfo->xiocb_flags  = 0;
	cfe_boardinfo->xiocb_psize  = sizeof(xiocb_boardinfo_t);

	if (cfe_seal == CFE_SEAL) {
		res = cfe_call(cfe_boardinfo);
	}
	return res;
}

#ifdef CONFIG_TIVO_KONTIKI
static int parse_eth0_hwaddr(char *buf)
{
	int i, t;
	u8 addr[6];
	extern unsigned char *gHwAddrs[];
	extern int gNumHwAddrs;

	for(i = 0; i < 6; i++) {
		t = hex16(buf);
		if(t == -1)
			return(-1);
		addr[i] = t;
		buf += 3;
	}
	memcpy(&gHwAddrs[0][0], addr, 6);
	gNumHwAddrs = 1;

	return(0);
}

static int parse_dram0_size(char *buf)
{
	extern unsigned long brcm_dram0_size;
	char *endp;
	unsigned long tmp;

	tmp = simple_strtoul(buf, &endp, 0);
	if(*endp == 0) {
		brcm_dram0_size = tmp << 20;
		return(0);
	}
	return(-1);
}

static int parse_dram1_size(char *buf)
{
	extern unsigned long brcm_dram1_size;
	char *endp;
	unsigned long tmp;

	tmp = simple_strtoul(buf, &endp, 0);
	if(*endp == 0) {
		brcm_dram1_size = tmp << 20;
		return(0);
	}
	return(-1);
}

static int parse_boardname(char *buf)
{
#if defined(CONFIG_MIPS_BCM7401) || defined(CONFIG_MIPS_BCM7400) || \
	defined(CONFIG_MIPS_BCM7403) || defined(CONFIG_MIPS_BCM7405)
	/* autodetect 97455, 97456, 97458, 97459 DOCSIS boards */
	if(strncmp("BCM9745", buf, 7) == 0)
		brcm_docsis_platform = 1;
#endif
#if defined(CONFIG_MIPS_BCM7420)
	/* BCM97420CBMB */
	brcm_docsis_platform = 1;
#endif

#if defined(CONFIG_MIPS_BCM7405)
	/* autodetect 97405-MSG board (special MII configuration) */
	if(strstr(buf, "_MSG") != NULL)
		brcm_enet_no_mdio = 1;
#endif
	return(0);
}

static int parse_boot_flags(char *buf)
{
	extern char cfeBootParms[];

	strcpy(cfeBootParms, buf);
	return(0);
}

#define BUF_SIZE		COMMAND_LINE_SIZE
static char cfe_buf[BUF_SIZE];

#ifdef CONFIG_TIVO
// export function that uses the static buffer above, why burn 3k if we don't have to?
int tivo_get_cfe_env_variable(void *name_ptr, int name_length, void *val_ptr,  int val_length)
{
   return get_cfe_env_variable((char *)name_ptr, (char *)val_ptr, val_length);
}    
#endif    

int get_cfe_boot_parms(void)
{
	int ret;
	int e_ok = 1, d_ok = 1, b_ok = 1, c_ok = 1;

	printk("Fetching vars from bootloader... ");
	if (cfe_seal != CFE_SEAL) {
		printk("none present, using defaults.\n");
		return(-1);
	}

	ret = get_cfe_env_variable("ETH0_HWADDR", cfe_buf, BUF_SIZE);
	if((ret != 0) || (parse_eth0_hwaddr(cfe_buf) != 0))
		e_ok = 0;
	
	ret = get_cfe_env_variable("DRAM0_SIZE", cfe_buf, BUF_SIZE);
	if((ret != 0) || (parse_dram0_size(cfe_buf) != 0))
		d_ok = 0;
	
	ret = get_cfe_env_variable("DRAM1_SIZE", cfe_buf, BUF_SIZE);
	if((ret != 0) || (parse_dram1_size(cfe_buf) != 0))
		d_ok = 0;
	
	ret = get_cfe_env_variable("CFE_BOARDNAME", cfe_buf, BUF_SIZE);
	if((ret != 0) || (parse_boardname(cfe_buf) != 0))
		b_ok = 0;
	
	ret = get_cfe_env_variable("BOOT_FLAGS", cfe_buf, BUF_SIZE);
	if((ret != 0) || (parse_boot_flags(cfe_buf) != 0))
		c_ok = 0;

	if(e_ok || d_ok || b_ok || c_ok) {
		printk("OK (%c,%c,%c,%c)\n",
			e_ok ? 'E' : 'e',
			d_ok ? 'D' : 'd',
			b_ok ? 'B' : 'b',
			c_ok ? 'C' : 'c');
		return(0);
	} else {
		printk("FAILED\n");
		return(-1);
	}
}


#else  /* if !defined(CONFIG_TIVO_KONTIKI) */
#ifdef CONFIG_TIVO
// export function that uses the static buffer above, why burn 3k if we don't have to?
int tivo_get_cfe_env_variable(void *name_ptr, int name_length, void *val_ptr,  int val_length)
{
    return get_cfe_env_variable(&cfeparam, name_ptr, name_length, val_ptr, val_length);
}    
#endif    

/*
 * ethHwAddrs is an array of 16 uchar arrays, each of length 6, allocated by the caller
 * numAddrs are the actual number of HW addresses used by CFE.
 * For now we only use 1 MAC address for eth0
 */
int get_cfe_boot_parms( char bootParms[], int* numAddrs, unsigned char* ethHwAddrs[] )
{
	/*
	 * This string can be whatever you want, as long
	 * as it is * consistant both within CFE and here.
	 */
	const char* cfe_env = "BOOT_FLAGS";
#ifdef CONFIG_MTD_BRCMNAND
	const char* eth0HwAddr_env = "ETH0_HWADDR";
#endif
	int res;
	char msg[128];

{

    res = get_cfe_env_variable(&cfeparam,
				   (void *)cfe_env,   strlen(cfe_env),
				   (void *)bootParms, CFE_CMDLINE_BUFLEN);

#ifdef CONFIG_TIVO_MOJAVE
	if (res){
	    uart_puts( "BOOT_FLAGS is not present - looking at BOOT_PARMS\n" );
	    res = get_cfe_env_variable(&cfeparam,
					   (void *)"BOOT_PARMS",   strlen("BOOT_PARMS"),
					   (void *)bootParms, CFE_CMDLINE_BUFLEN);
	}
#endif
	if (res){
		uart_puts( "No arguments presented to boot command\n" );
		res = -1;
	}
	else{

		if (strlen(bootParms) >= COMMAND_LINE_SIZE) {
			int i;
			sprintf(msg, "Warnings, CFE boot params truncated to %d bytes\n",
				COMMAND_LINE_SIZE);
			uart_puts(msg);
			for (i=COMMAND_LINE_SIZE-1; i>=0; i--) {
				if (isspace(bootParms[i])) {
					bootParms[i] = '\0';
					break;
				}
			}
		}	
		res = 0;
	}
}

#ifdef CONFIG_TIVO_NEUTRON
        if (ethHwAddrs != NULL) 
        {
                unsigned int a,b,c,d,e,f;
                res = get_cfe_env_variable(&cfeparam, "MAC", 3, (void *)msg, sizeof(msg));
                if (res || sscanf(msg,"%X:%X:%X:%X:%X:%X", &a,&b,&c,&d,&e,&f) != 6)
                {
                        printk(KERN_WARNING "WARNING: CAN'T FIND VALID MAC, USING 00:11:D9:00:00:01\n");
                        a=0x00; b=0x11; c=0xd9; d=0x00; e=0x00; f=0x01;
                }
                    
                ethHwAddrs[0][0]=a;
                ethHwAddrs[0][1]=b;
                ethHwAddrs[0][2]=c;
                ethHwAddrs[0][3]=d;
                ethHwAddrs[0][4]=e;
                ethHwAddrs[0][5]=f;

                *numAddrs = 1;

                // If MOCA is defined and valid, populate the second MAC address. Otherwise, do nothing.
                res = get_cfe_env_variable(&cfeparam, "MOCA", 4, (void *)msg, sizeof(msg));
                if (!res && sscanf(msg,"%X:%X:%X:%X:%X:%X", &a,&b,&c,&d,&e,&f)==6)
                {
                        ethHwAddrs[1][0]=a;
                        ethHwAddrs[1][1]=b;
                        ethHwAddrs[1][2]=c;
                        ethHwAddrs[1][3]=d;
                        ethHwAddrs[1][4]=e;
                        ethHwAddrs[1][5]=f;

                        *numAddrs = 2;
                }        
                
                res = 0;
        }
#else
#ifdef CONFIG_MTD_BRCMNAND
if (ethHwAddrs != NULL) {
	unsigned char eth0HwAddr[ETH_HWADDR_LEN];
	int i, j, k;

	*numAddrs = 1;
	  res = get_cfe_env_variable(&cfeparam,
					 (void *)eth0HwAddr_env, strlen(eth0HwAddr_env),
					 (void *)eth0HwAddr,     ETH_HWADDR_LEN*(*numAddrs));
	  if (res)
		  res = -2;
	  else {
	}

	if (res){
		uart_puts( "Ethernet MAC address was not set in CFE\n" );
	
		res = -2;
	}
	else {
		if (strlen(eth0HwAddr) >= ETH_HWADDR_LEN*(*numAddrs)) {
			sprintf(msg, "Warnings, CFE boot params truncated to %d\n", ETH_HWADDR_LEN);
			uart_puts(msg);
			
			eth0HwAddr[ETH_HWADDR_LEN-1] = '\0';
		}	

		/*
		 * Convert to binary format
		 */
		for (k=0; k < *numAddrs; k++) {
			unsigned char* hwAddr = ethHwAddrs[k];
			int done = 0;
			for (i=0,j=0; i<ETH_HWADDR_LEN && !done; ) {
				switch (eth0HwAddr[i]) {
				case ':':
					i++;
					continue;
				
				case '\0':
					done = 1;
					break;

				default:
					hwAddr[j] = (unsigned char)
						((hex(eth0HwAddr[i]) << 4) | hex(eth0HwAddr[i+1]));
					j++;
					i +=2;
				}
			}
		}
		res = 0;
	}
}
#endif
#endif
	return res;
}
#endif /* !defined(CONFIG_TIVO_KONTIKI) */


#ifdef CONFIG_TIVO_KONTIKI
static inline int bcm_atoi(const char *s, int base)
{
	int n;
	int neg = 0;

	n = 0;

	if (base == 10) {
		if (*s == '-') {
			neg = 1;
			s++;
		}
		while (isdigit(*s))
			n = (n * 10) + *s++ - '0';

		if (neg)
			n = 0 - n;
	}
	else if (base == 16) {
		while (*s) {
			int h = hex(*s);

			if (h >= 0) {
				n = (n*16) + h;
				s++;
			}
			else
				break;
		}
	}
	return (n);
}

void __init bcm_get_cfe_partition_env(void)
{
	char envval[128];
	int e, res;
	int numParts = 0;
	unsigned int size;

	for (e=0; gCfeEnvVarPairs[e].offset != NULL; e++) {
		int base = 16;
	
		envval[0] = '\0';

		res = get_cfe_env_variable((char*) gCfeEnvVarPairs[e].size,
				   envval, sizeof(envval));
		if (res == 0 && envval[0] != '\0') {
			/*
			if (e == DRAM0 || e == DRAM1)
				base = 10;
			*/
			
			size = bcm_atoi(envval, base);
			if (size == 0) /* Only size matters :-) */
				continue;
			gCfePartitions.parts[numParts].size = size;
			
			envval[0] =  '\0';
			res = get_cfe_env_variable((char*) gCfeEnvVarPairs[e].offset,
				   envval, sizeof(envval));
			if (res == 0 && envval[0] != '\0') {
				gCfePartitions.parts[numParts].offset = bcm_atoi(envval, 16);
	
			}
			else {
				/* Have size but no offset, its OK for DRAM, as only size matters */
				gCfePartitions.parts[numParts].offset = 0;
			}
			gCfePartitions.parts[numParts].part = e;
			gCfePartitions.numParts = ++numParts;
		}
	}

    {
	int i;
	
	for (i=0; i < gCfePartitions.numParts; i++) {
		int p = gCfePartitions.parts[i].part;
		
printk("CFE EnvVar: %s: %08x, %s: %08x\n", 
		gCfeEnvVarPairs[p].offset, gCfePartitions.parts[i].offset,
		gCfeEnvVarPairs[p].size, gCfePartitions.parts[i].size);
	}
    }
}

#endif /* CONFIG_TIVO_KONTIKI */

