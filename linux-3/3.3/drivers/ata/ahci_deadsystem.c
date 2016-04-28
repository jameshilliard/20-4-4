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

#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/errno.h>
#include <linux/delay.h>
#include <linux/fs.h>
#include <linux/root_dev.h>

#include <scsi/scsi_device.h>
#include <linux/libata.h>

#include "ahci.h"

/* Uncomment this to debug module's processing */
//#define DEBUG_AHCI_DEADSYSTEM 1

#ifdef DEBUG_AHCI_DEADSYSTEM
#define DEBUG_PRINT(...) do { printk(__VA_ARGS__); } while (false)
#else
#define DEBUG_PRINT(...) do { } while (false)
#endif

#if defined(CONFIG_TIVO_PANICLOG_IN_RAM)
#define CONTIGMEM_REGION 2

extern int mem_get_contigmem_region(unsigned int   region,
                                    unsigned long *start,
                                    unsigned long *size);

static int *panic_log = NULL;
#endif

#define KSEG_SIZE          0x20000000

#define KSEG0_BASE         0x80000000
#define KSEG1_BASE         0xa0000000

#define	KSEG_TO_PHYS(va)   (((u32)va) - KSEG0_BASE)

#define BCHP_REG_READ(  REG )         *(volatile u32*)(REG+0x90000000)
#define BCHP_REG_WRITE(REG,VAL)       *(volatile u32*)(REG+0x90000000) = (VAL)

/* Command Header DWORD 0 Definitions */
#define	PRDTL_SHIFT	16
#define	CFL_SHIFT	0

#define SWAPFS_MAJOR	SCSI_DISK0_MAJOR
#define SWAPFS_MINOR	8

#define swap_endian(x) ((unsigned short)(((x>>8) & 0xff) | ((x<<8) &0xff00)))

extern struct scsi_disk *scsi_disk_get(struct gendisk *disk);
extern void scsi_disk_put(struct scsi_disk *sdkp);
extern struct scsi_device *scsi_dev_get(struct scsi_disk *sdkp);
extern void __iomem *ahci_port_base(struct ata_port *ap);

typedef struct
{
    u32 ctrl;
    u32 prdbc;
    u32 ctba0;        /* pa[31:7], 128byte aligned */
    u32 ctba0_upper;  /* pa[63:32] */
    u32 reserved[4];  /* dw4..dw7 */
} ata_cmd_hdr_t;

typedef struct
{
    u32 cmd_fis[16];
    u8  atapi_cmd[16];
    u8  res[48];
    struct
    {
        u32 dba;
        u32 dbau;
        u32 res;
        u32 dbc; /* bit 31 = IRQ, bit 0 - always 1 */
    } prdt[32];
} ata_cmd_tbl_t;

typedef struct
{
    u8 dma_fis[32];
    u8 pio_fis[32];
    u8 d2h_fis[24];
    u8 sdb_fis[8];
    u8 bad_fis[64];
    u8 res[96];
} ata_rcv_fis_t;

ata_cmd_hdr_t *p_ata_cmd_hdr;
ata_cmd_tbl_t *p_ata_cmd_tbl;
ata_rcv_fis_t *p_ata_rcv_fis;

/* ATA dword0 FIS command byte value. */
enum
{
    ATA_IDENTIFY_COMMAND         = 0xec,
    ATA_READ_SECTOR_COMMAND      = 0x20,
    ATA_READ_SECTOR_EXT_COMMAND  = 0x24,
    ATA_WRITE_SECTOR_COMMAND     = 0x30,
    ATA_WRITE_SECTOR_EXT_COMMAND = 0x34,
};

/* ATA dword0 of command FIS */
enum
{
    ATA_IDENTIFY_FIS            = 0x00ec8027,
    ATA_READ_SECTOR_FIS         = 0x00208027,
    ATA_READ_SECTOR_EXT_FIS     = 0x00248027,
    ATA_WRITE_SECTOR_FIS        = 0x00308027,
    ATA_WRITE_SECTOR_EXT_FIS    = 0x00348027,
};

#define NUM_ATA_CMDS 5
typedef struct
{
    char name[30];
    int value;
} ata_name_table_t;

typedef struct
{
    int port;
    int command;
    int direction;
    int sector;
    int numSectors;
    const u8 *buffer;
    int offset;
} hba_cmd_issue_t;

hba_cmd_issue_t pHbaCmd;

ata_cmd_hdr_t ata_cmd_hdr[32] __attribute__((aligned(1024)));
ata_cmd_tbl_t ata_cmd_tbl[32] __attribute__((aligned(64)));
ata_rcv_fis_t ata_rcv_fis[256] __attribute__((aligned(256)));

static int initdrive = 1;

/* Table of ATA command/value tuples. */
ata_name_table_t ata_name_table[NUM_ATA_CMDS] =
{
    {
        "IDENTIFY",
        0xec
    },
    {
        "READ_SECTOR",
        0x20
    },
    {
        "READ_SECTOR_EXT",
        0x24
    },
    {
        "WRITE_SECTOR",
        0x30
    },
    {
        "WRITE_SECTOR_EXT",
        0x34
    }
};

int ahci_allocate_hba_structures(void)
{
    DEBUG_PRINT("allocate_hba_structures - Entered\n");

    p_ata_cmd_hdr = ata_cmd_hdr;
    p_ata_cmd_tbl = ata_cmd_tbl;
    p_ata_rcv_fis = ata_rcv_fis;

    /* Zero-initialise the structures. */
    memset(p_ata_cmd_hdr, 0, 32*sizeof(ata_cmd_hdr_t));
    memset(p_ata_cmd_tbl, 0, 32*sizeof(ata_cmd_tbl_t));
    memset(p_ata_rcv_fis, 0, 256*sizeof(ata_rcv_fis_t));
    DEBUG_PRINT("allocate_hba_structures - cmd_hdr @%08x cmd_tbl @%08x rcv_fis @%08x \n",
                     (unsigned int)p_ata_cmd_hdr, (unsigned int)p_ata_cmd_tbl, (unsigned int)p_ata_rcv_fis);

    DEBUG_PRINT("allocate_hba_structures - Exited\n");

    return 0;
}

void ahci_init_hba(struct ata_port *ap)
{
    void __iomem *port_mmio = ahci_port_base(ap);
    u32 temp;
    int i;

    DEBUG_PRINT("init_hba - Entered\n");

    /*
     * Ensure HBA is idle before writing system structures by checking the
     * PxCMD.ST, PxCMD.FRE, PxCMD.CR, PxCMD.FR bits.
     */
    temp = readl(port_mmio + PORT_CMD);

    if (temp & 0x1 || temp & 0x10)
    {
        /* Check command start bit. */
        if (temp & 0x1)
        {
            DEBUG_PRINT("Start bit set. Reset it...\n");
            temp &= ~0x1;
        }

        /* Check command FIS receive enable bit. */
        if (temp & 0x10)
        {
            DEBUG_PRINT("FIS receive enable bit is set, resetting it...\n");
            temp &= ~0x10;
         }

         /* Write changes to PORT command register. */
        writel(temp, port_mmio + PORT_CMD);
    }

    /* Re-read PORT command register to confirm DMACs are idle.*/
    temp = readl(port_mmio + PORT_CMD);
    while ((temp & 0x4000) || (temp & 0x8000))
    {
        DEBUG_PRINT("HBA DMAC(s) active, retry in 500mS (AHCI spec)...\n");
        mdelay(500);
        temp = readl(port_mmio + PORT_CMD);
    }

    /*
     * Program Command Table Base (physical) Address field in the ATA command
     * header.
     */
    for (i = 0; i < 32; i++)
    {
        (p_ata_cmd_hdr+i)->ctba0 = KSEG_TO_PHYS(p_ata_cmd_tbl+i);
    }

    /* Write system structures to HBA registers. */

    /* HBA command list. */
    writel(KSEG_TO_PHYS((u32)p_ata_cmd_hdr), port_mmio + PORT_LST_ADDR);
    DEBUG_PRINT("PORT0 CLB contains 0x%x\n", readl(port_mmio + PORT_LST_ADDR));

    /* HBA FIS receive buffer */
    writel(KSEG_TO_PHYS((u32)p_ata_rcv_fis), port_mmio + PORT_FIS_ADDR);
    DEBUG_PRINT("PORT0 FB contains 0x%x\n", readl(port_mmio + PORT_FIS_ADDR));

    /* Clear interrupts */
    writel(0x0, port_mmio + PORT_IRQ_MASK);

    DEBUG_PRINT("init_hba - Exited\n");
}

/*
 * Experimental code to handle apparently hung port/disk - tested by
 * unplugging SATA cable during xfer.
 *
 * According to the AHCI 1.3 Spec, port reset is preferable to a software reset */
int ahci_reset_hba_port(struct ata_port *ap, int port)
{
    void __iomem *port_mmio = ahci_port_base(ap);
    u32 temp;

    DEBUG_PRINT("reset_hba_port  entered\n");

    /* reset port */

    /*
     * Force command DMAC to idle by clearing PxCMD.ST (start) bit.
     * This must be done beore re-initializing communication.
     */
    temp = readl(port_mmio + PORT_CMD);
    if (temp & 0x1)
    {
        temp &= ~0x1;
        writel(temp, port_mmio + PORT_CMD);

        /* wait 500mS for command DMAC to idle */
        mdelay(500);
    }

    /* check command DMAC status. */
    temp = readl(port_mmio + PORT_CMD);
    if (temp & 0x8000)
    {
        printk("command DMAC still active! Ok, port is definitely hung.\n");
    }

    /* Issue a COMRESET by asserting PxSCTL.DET */
    temp = readl(port_mmio + PORT_SCR_CTL);
    temp |= 1;
    writel(temp, port_mmio + PORT_SCR_CTL);

    /* Wait for at least 10mS delay before de-asserting PxSCTL.DET */
    mdelay(10);

    temp = readl(port_mmio + PORT_SCR_CTL);
    temp &= ~0x1;
    writel(temp, port_mmio + PORT_SCR_CTL);

    /* Wait for at least 100mS delay before checking disk detected. */
    mdelay(100);

    /* Now wait for communication to be established by polling PxSSTS.DET bit */
    temp = readl(port_mmio + PORT_SCR_STAT);
    DEBUG_PRINT("Read 0x%x from PORT 0 status register...\n", temp);

    if (!((temp & 0x1) || (temp & 0x3)))
    {
        printk("Disk not detected after 100mS! 0x%08x\n", temp);
    }

    /* Clear out bits that were set as part of port reset. */
    writel(0xffffffff, port_mmio + PORT_SCR_ERR);

    /* Assert PxCMD.ST bit. */
    temp = readl(port_mmio + PORT_CMD);
    temp |= 0x1;
    writel(temp, port_mmio + PORT_CMD);

    DEBUG_PRINT("reset_hba_port  exited\n");

    return 0;
}

int ahci_start_hba(struct ata_port *ap, int port)
{
    void __iomem *port_mmio = ahci_port_base(ap);
    int i;
    u32 temp;

    DEBUG_PRINT("start_hba - Entered\n");

    /* Clear HBA interrupt status register. */
    writel(0xffffffff, port_mmio + PORT_IRQ_STAT);

    /* Set FIS Receive Enable bit in the command register for port 0 */
    writel(0x10000010, port_mmio + PORT_CMD);

    /* Tell attached disk to spin up */
    writel(0x10000012, port_mmio + PORT_CMD);

    /* SATA spec (2.6) suggests a 10mS latency for detection. */
    mdelay(100);

    /* Now POLL detect bit in status register (PxSSTS.DET), timeout after 10mS */
    temp = readl(port_mmio + PORT_SCR_STAT);
    DEBUG_PRINT("Read 0x%x from PORT 0 status register...\n", temp);

    if ((!(temp & 0x1) || !(temp & 0x3)))
    {
        printk("Disk not detected after 10mS!\n");
        return -1;
    }

    DEBUG_PRINT("Device DETECTed on PORT 0...\n");

    /* Verify COMWAKE/COMINIT handshake has taken place between PHYs */
    temp = readl(port_mmio + PORT_IRQ_STAT);
    if (temp & 0x00400000) /*PCS bit */
    {
        temp =  readl(port_mmio + PORT_SCR_ERR);
        while (!(temp & 0x04000000)) /* DIAG.X bit */
        {
            printk("reChecking X bit.\n");
            temp =  readl(port_mmio + PORT_SCR_ERR);
        }

        while (!(temp & 0x00040000)) /* DIAG.W bit */
        {
            printk("reChecking W bit.\n");
            temp =  readl(port_mmio + PORT_SCR_ERR);
        }
    }

    /* Device detected, clear PxSERR and wait for device to be ready. Maximum wait
     * time is ~400mS (empirically determined). */
    temp = readl(port_mmio + PORT_SCR_ERR);
    writel(temp, port_mmio + PORT_SCR_ERR);

    temp =  readl(port_mmio + PORT_TFDATA);
    DEBUG_PRINT("Read 0x%x from PORT 0 task file data register...\n", temp);
    i = 0;
    while (((temp & 0x1)  ||
           (temp & 0x8)   ||
           (temp & 0x80)) &&
           i < 40)
    {
        printk("Disk not ready. recheck in 10mS...\n");
        mdelay(10);
        temp = readl(port_mmio + PORT_TFDATA);
        i++;
    }

    if (i >=40)
    {
        printk("Timed out awaiting device READY on PORT0...\n");
        return -1;
    }

    DEBUG_PRINT("Device READY on PORT 0...\n");

    /* Set interface communication rate according to device. */

    /* Read device communication rate from the status register. */
    temp = readl(port_mmio + PORT_SCR_STAT);
    temp &= 0xf0;

    /*
     * Write device communication rate and disable support for HBA partial and
     * slumber states.
     */
    temp |= (0x3 << 8);
    writel(temp, port_mmio + PORT_SCR_CTL);

    /* readback to ensure write completed. */
    readl(port_mmio + PORT_SCR_CTL);

    /* Clear PxSERR register. */
    temp = readl(port_mmio + PORT_SCR_ERR);
    writel(temp, port_mmio + PORT_SCR_ERR);

    /* Set start bit to enable command processing only if HBA idle. */
    temp = readl(port_mmio + PORT_CMD);
    while (temp & 0x8000)
    {
        printk("Awaiting HBA IDLE by checking command DMAC\n");
        mdelay(5);
        temp = readl(port_mmio + PORT_CMD);
    }

    writel(0x10000013, port_mmio + PORT_CMD);

    DEBUG_PRINT("start_hba - Exited\n");

    return 0;
}

int ahci_issue_disk_command(struct ata_port *ap, const hba_cmd_issue_t *cmd)
{
    void __iomem *port_mmio = ahci_port_base(ap);
    u32 ci_reg;
    u32 sact_reg;
    u32 cmd_tbl_index;
    u8  lba_lo;      /* [7:0] */
    u8  lba_mid;     /* [15:8] */
    u8  lba_hi;      /* [23:16] */
    u8  lba_lo_exp;  /* [31:24] */ /* lba_?_exp needed for 48-bit LBA support */
    u8  lba_mid_exp; /* [39:32] */
    u8  lba_hi_exp;  /* [47:40] */
    int    busyCount;

    DEBUG_PRINT("Issue_disk_command - Entered\n");

    /* Buffer must be word-aligned. */
    if ((u32)(cmd->buffer) & 0x3)
    {
        printk("Buffer not aligned on word boundary!\n");
        return -1;
    }

    DEBUG_PRINT("Prepare HBA command structure for port %d ...\n", cmd->port);

    /* Setup command header. */
    p_ata_cmd_hdr->ctrl = (1 << PRDTL_SHIFT) | (5 << CFL_SHIFT);
    p_ata_cmd_hdr->prdbc = 0;

    /* Setup LBA.  The values will be written to the command FIS. */
    lba_lo = cmd->sector & 0xff;
    lba_mid = (cmd->sector >> 8) & 0xff;
    lba_hi  = (cmd->sector >> 16) & 0xff;
    lba_lo_exp = (cmd->sector >> 24) & 0xff;
    lba_mid_exp = 0;
    lba_hi_exp = 0;

    /* Setup command FIS in system memory. */
    if (cmd->command == ATA_IDENTIFY_COMMAND)
    {
        p_ata_cmd_tbl->cmd_fis[0] = ATA_IDENTIFY_FIS;
        p_ata_cmd_tbl->cmd_fis[1] = 0x40000000;
        p_ata_cmd_tbl->cmd_fis[2] = 0x00000000;
        p_ata_cmd_tbl->cmd_fis[3] = 0x00000000;
        p_ata_cmd_tbl->cmd_fis[4] = 0x00000000;
    }
    else if (cmd->command == ATA_READ_SECTOR_COMMAND || cmd->command == ATA_READ_SECTOR_EXT_COMMAND)
    {
        p_ata_cmd_tbl->cmd_fis[0] = cmd->command == ATA_READ_SECTOR_COMMAND
                                             ? ATA_READ_SECTOR_FIS
                                             : ATA_READ_SECTOR_EXT_FIS;
        p_ata_cmd_tbl->cmd_fis[1] = 4 << 28 | lba_hi << 16 | lba_mid << 8 | lba_lo;
        p_ata_cmd_tbl->cmd_fis[2] = lba_lo_exp;
        p_ata_cmd_tbl->cmd_fis[3] = cmd->numSectors;
        p_ata_cmd_tbl->cmd_fis[4] = 0x00000000;
    }
    else if (cmd->command == ATA_WRITE_SECTOR_COMMAND || cmd->command == ATA_WRITE_SECTOR_EXT_COMMAND)
    {
        p_ata_cmd_tbl->cmd_fis[0] = cmd->command == ATA_WRITE_SECTOR_COMMAND
                                             ? ATA_WRITE_SECTOR_FIS
                                             : ATA_WRITE_SECTOR_EXT_FIS;
        p_ata_cmd_tbl->cmd_fis[1] = 4 << 28 | lba_hi << 16 | lba_mid << 8 | lba_lo;
        p_ata_cmd_tbl->cmd_fis[2] = lba_lo_exp;
        p_ata_cmd_tbl->cmd_fis[3] = cmd->numSectors;
        p_ata_cmd_tbl->cmd_fis[4] = 0x00000000;
    }

    /* Remap KSEG address to physical address */
    p_ata_cmd_tbl->prdt[0].dba = KSEG_TO_PHYS(cmd->buffer+cmd->offset*512);

    if (cmd->numSectors == 0) /* 256 sectors - XXX this does not seem to work. */
    {
        p_ata_cmd_tbl->prdt[0].dbc = 0x8 << 28 | ((256 * 512) - 1);
    }
    else
    {
        p_ata_cmd_tbl->prdt[0].dbc = 0x8 << 28 | ((cmd->numSectors * 512) - 1);
    }

    DEBUG_PRINT("DBC set to: 0x%x\n", p_ata_cmd_tbl->prdt[0].dbc);

    /*
     * Read PORT 0 Command Issue (CI) and Active (SACT) registers.
     * Each bit represents a command table entry.
     */
    ci_reg = readl(port_mmio + PORT_CMD_ISSUE);
    sact_reg = readl(port_mmio + PORT_SCR_ACT);
    for (cmd_tbl_index = 0; cmd_tbl_index < 32; cmd_tbl_index++)
    {
        if (!(ci_reg & (1 << cmd_tbl_index)) && !(sact_reg & (1 << cmd_tbl_index)))
        {
            DEBUG_PRINT("Found freeslot at index %d\n", cmd_tbl_index);
            break;
        }
    }

    /* Flush data cache to ensure HBA data structures are in RAM */
    flush_dcache_all();

    DEBUG_PRINT("Issue a command to the disk...\n");

    writel(1 << cmd_tbl_index, port_mmio + PORT_CMD_ISSUE);

    /*
     * Give disk drive ~10mS before checking PxTFD busy bit. We use a counter to
     * approximate a timeout of ~200mS.  If 200mS elapses return an error.
     * This timeout value is derived empirically.
     */
    mdelay(10);
    busyCount = 0;

    while ((readl(port_mmio + PORT_TFDATA) & 0x80) && (busyCount < 40))
    {
        DEBUG_PRINT("PxTFD has busy bit set. Recheck in 200mS\n");
        mdelay(200);
        busyCount++;
    }

    if (busyCount >= 40)
    {
        DEBUG_PRINT("PxTFD busy stuck high!\n");

        return -3;
    }

    /* Didn't timeout, but not all data requested was transferred. */
    if (p_ata_cmd_hdr->prdbc < (cmd->numSectors * 512))
    {
        printk("Disk transferred %d out of %d bytes...\n", p_ata_cmd_hdr->prdbc, cmd->numSectors * 512);

        return -4;
    }
    DEBUG_PRINT("Issue_disk_command - Exited\n");

    return 0;
}

char * ahci_get_ata_command(int command)
{
    int i;

    for (i = 0; i < NUM_ATA_CMDS; i++)
    {
        if (ata_name_table[i].value == command)
        {
            return &ata_name_table[i].name[0];
        }
    }

    return "UNKNOWN";
}

void ahci_dump_hba_regs(struct ata_port *ap)
{
    void __iomem *port_mmio = ahci_port_base(ap);

    DEBUG_PRINT("dump_hba_regs - Entered\n");

    DEBUG_PRINT("PORT0 Command register  0x%x\n", readl(port_mmio + PORT_CMD));
    DEBUG_PRINT("PORT0 CI register       0x%x\n", readl(port_mmio + PORT_CMD_ISSUE));
    DEBUG_PRINT("PORT0 SACT register     0x%x\n", readl(port_mmio + PORT_SCR_ACT));
    DEBUG_PRINT("PORT0 Status register   0x%x\n", readl(port_mmio + PORT_SCR_STAT));
    DEBUG_PRINT("PORT0 Error register    0x%x\n", readl(port_mmio + PORT_SCR_ERR));
    DEBUG_PRINT("PORT0 IRQ Stat register 0x%x\n", readl(port_mmio + PORT_IRQ_STAT));
    DEBUG_PRINT("PORT0 IRQ Enab register 0x%x\n", readl(port_mmio + PORT_IRQ_MASK));
    DEBUG_PRINT("PORT0 TFD register      0x%x\n", readl(port_mmio + PORT_TFDATA));
    DEBUG_PRINT("PORT0 SIG register      0x%x\n", readl(port_mmio + PORT_SIG));
    DEBUG_PRINT("PORT0 control register  0x%x\n", readl(port_mmio + PORT_SCR_CTL));
    DEBUG_PRINT("PORT0 SNTF register     0x%x\n", readl(port_mmio + PORT_SCR_NTF));
    DEBUG_PRINT("%s - Exited\n",__FUNCTION__);
}

int ahci_write_buffer_to_swap(const u8 *buffer, unsigned int size, int secoffset, u32 *csum)
{
    struct block_device *bdev = NULL;
    struct gendisk *gp = NULL;
    struct scsi_disk *sdkp = NULL;
    struct scsi_device *sdev = NULL;
    struct ata_port *ap = NULL;
    struct hd_struct *swap_part = NULL;
    u32 checksum = 0;
    u16 *cs_buffer = (u16*)buffer;
    int writtensects = (size % 512) ? (size / 512) + 1 : (size/512);
    unsigned long start_sec, nr_sectors;

#if defined(CONFIG_TIVO_PANICLOG_IN_RAM)
    unsigned long panic_log_region_start;
    unsigned long panic_log_region_size = 0;

    if (panic_log == NULL)
    {
        if (mem_get_contigmem_region(CONTIGMEM_REGION,
                                     &panic_log_region_start,
                                     &panic_log_region_size))
        {
            printk("Failed to retrieve contigmem region parameters!\n");
            return -ENODEV;
        }

        panic_log = (int *)panic_log_region_start;

        memset(panic_log, 0, panic_log_region_size);
    }

    memcpy((void *)(((char *)panic_log) + (secoffset * 512)), (void *)buffer, size);

#else

    if ((bdev = bdget(MKDEV(SWAPFS_MAJOR, SWAPFS_MINOR))) != NULL)
    {
        if ((gp = bdev->bd_disk) == NULL)
        {
            return -ENODEV;
        }
        else
        {
            /* Retrieve swap partition info */
            swap_part = disk_get_part(gp, SWAPFS_MINOR);

            if (swap_part == NULL)
            {
                return -ENODEV;
            }
            else
            {
                start_sec = (unsigned long)swap_part->start_sect;
                nr_sectors = (unsigned long)swap_part->nr_sects;
            }

            if ((sdkp = scsi_disk_get(gp)) == NULL)
            {
                return -ENXIO;
            }
        }
    }
    else
    {
        return -ENODEV;
    }

    if((sdev = scsi_dev_get(sdkp)) == NULL)
    {
        return -ENXIO;
    }
    else
    {
        if((ap = ata_shost_to_port(sdev->host)) == NULL)
        {
            return -ENXIO;
        }
    }

    /*
     * We adjust secoffset to write to the last 1MB of the device,
     * this is because we don't want to rebuild swap mostly, and
     * because we can use "lspanic" to grab the last tivo panic.
     *
     * We will track which one we've logged by bit-inverting
     * the checksum.
     *
     */
    /* Partition must be at least 2 MB */
    if (nr_sectors < (2* 1024 * 1024 / 512))
    {
        return -ENODEV;
    }

    /* Offset by 1MB from end of partition */
    secoffset += nr_sectors - (1 * 1024 * 1024 /512);

    /* Make sure partition is sane size-wise */
    if (nr_sectors < secoffset)
    {
        return -ENODEV;
    }

    if (nr_sectors - secoffset < ((size + 511)/512))
    {
	    return -ENODEV;
    }

    if (initdrive)
    {
        /* Initialize the internal data structures */
        ahci_allocate_hba_structures();

        ahci_init_hba(ap);

        if (ahci_start_hba(ap, 0) < 0)
        {
            printk("Failed to start HBA controller!\n");
            return -ENODEV;
        }

        initdrive = 0;
    }

    /* Prepare HBA command */
    pHbaCmd.port = 0;
    pHbaCmd.command = ATA_WRITE_SECTOR_EXT_COMMAND;
    pHbaCmd.direction = 1;
    pHbaCmd.numSectors = (size % 512) ? (size / 512) + 1 : (size/512);
    pHbaCmd.buffer = buffer;
    pHbaCmd.offset = 0;
    pHbaCmd.sector = start_sec + secoffset;

    if (ahci_issue_disk_command(ap, &pHbaCmd) < 0)
    {
        printk("Disk command %s (0x%x) failed, resetting port and retrying!\n",
                    ahci_get_ata_command(pHbaCmd.command), pHbaCmd.command);

        if (ahci_reset_hba_port(ap, pHbaCmd.port) < 0)
        {
            printk("Failed to reset port %d! Time to bail!\n", pHbaCmd.port);
            return -ENODEV;
        }

        /* Re-issue the command */
        if (ahci_issue_disk_command(ap, &pHbaCmd) < 0)
        {
            printk("Disk command %s (0x%x) failed, even after resetting port!\n",
                        ahci_get_ata_command(pHbaCmd.command), pHbaCmd.command);
            return -EINVAL;
        }
    }
#endif

    /* Calculate the checksum */
    if (csum != NULL)
    {
        size /= 2;
        while (size--)
        {
            checksum += swap_endian(cs_buffer[size]);
            if (swap_endian(cs_buffer[size]) > checksum)
                checksum++;
        }

        *csum += checksum;
    }

    scsi_disk_put(sdkp);

    return writtensects;
}

#define CORE_START (0x80000000)
#define CORE_SIZE  (32 * 1024 * 1024)

int ahci_write_corefile(int offset)
{
    /*
     * The original idea here is to write to the "alternate
     * application partition".  This has some problems though,
     * spec. with developer boxes.  For now, we're going to
     * assume this won't be called from a dev build.  (This is so
     * broken it bothers me, we could write it to swap -32M -1M)
     *
     * The second broken thing here is we don't account for
     * actual size of the memory of the box in question.  We
     * assume it's 32Meg.  This is somewhat easily overcome by
     * just checking the config variables.  However, I'm loathe
     * to do that at this point since having variable size input
     * just risks adding a new bug to the system.  (Something I don't
     * want to do at this point in the release cycle)
     *
     * This is a *very basic* implementation of what we want to do.
     * It starts at 0x80000000, goes for 32MB writing out to alternate
     * application at either "offset 0", or "offset 1".  Offset 0 is
     * at the beginning of the partition.  Offset 1 is 32MB into the
     * partition.  (ah, now you see why I don't want to deal with
     * variable sizes)
     *
     * Cache flushing is not required as we're doing it PIO, which just
     * pulls from cache.  Of course, we're taking the kernel centric
     * as opposed to hardware centric view of memory.  This is reasonable
     * because really, the core is just a bunch of kernel memory
     * structures for the most part.
     *
     */
    struct block_device *bdev;
    struct gendisk *gp;
    struct scsi_disk *sdkp = NULL;
    struct scsi_device *sdev = NULL;
    struct ata_port *ap = NULL;
    int altpartition;
    unsigned long secoffset = (CORE_SIZE / 512) * offset;
    static int core_written = 0;

    /* Only 0 or 1 are valid offset values */
    if (!(offset == 0 || offset == 1))
    	return -EINVAL;

    /* If the root is sda4, then altap is sda7 and vice-versa */
    if (ROOT_DEV == MKDEV(SCSI_DISK0_MAJOR, 4))
    	altpartition = 7;
    else if (ROOT_DEV == MKDEV(SCSI_DISK0_MAJOR, 7))
    	altpartition = 4;
    else
    	return -ENODEV;

    if ((bdev = bdget(MKDEV(SWAPFS_MAJOR, SWAPFS_MINOR))) != NULL)
    {
        if ((gp = bdev->bd_disk) == NULL)
        {
            return -ENODEV;
        }
        else
        {
            if ((sdkp = scsi_disk_get(gp)) == NULL)
            {
                return -ENXIO;
            }
        }
    }
    else
    {
        return -ENODEV;
    }

    if((sdev = scsi_dev_get(sdkp)) == NULL)
    {
        return -ENXIO;
    }
    else
    {
        if((ap = ata_shost_to_port(sdev->host)) == NULL)
        {
            return -ENXIO;
        }
    }

    if (core_written != 0)
    {
        printk("Core already written\n");
        return -EINVAL;
    }

    core_written = 1;

    /* Partition must be at least twice a CORE */
    if (gp->part_tbl->part[altpartition - 1]->nr_sects < ((2 * CORE_SIZE) / 512))
    	return -ENODEV;

    if (initdrive)
    {
        /* Reset HBA port */
        if (ahci_reset_hba_port(ap, pHbaCmd.port) < 0)
        {
            return -ENODEV;
        }
        initdrive=0;
    }

    secoffset += gp->part_tbl->part[altpartition - 1]->start_sect;

    pHbaCmd.port = 0;
    pHbaCmd.command = ATA_WRITE_SECTOR_EXT_COMMAND;
    pHbaCmd.direction = 1;
    pHbaCmd.offset = 0;
    pHbaCmd.numSectors = (CORE_SIZE % 512) ? (CORE_SIZE / 512) + 1 : (CORE_SIZE/512);
    pHbaCmd.buffer = (const u8*)CORE_START;

    return ahci_issue_disk_command(ap, &pHbaCmd);
}
