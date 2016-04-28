/*******************************************************************************
*
* Common/Inc/common_dvr.h
*
* Description: Driver stuff shared across modules
*
*******************************************************************************/

/*******************************************************************************
*                        Entropic Communications, Inc.
*                         Copyright (c) 2001-2008
*                          All rights reserved.
*******************************************************************************/

/*******************************************************************************
* This file is licensed under GNU General Public license.                      *
*                                                                              *
* This file is free software: you can redistribute and/or modify it under the  *
* terms of the GNU General Public License, Version 2, as published by the Free *
* Software Foundation.                                                         *
*                                                                              *
* This program is distributed in the hope that it will be useful, but AS-IS and*
* WITHOUT ANY WARRANTY; without even the implied warranties of MERCHANTABILITY,*
* FITNESS FOR A PARTICULAR PURPOSE, TITLE, or NONINFRINGEMENT. Redistribution, *
* except as permitted by the GNU General Public License is prohibited.         *
*                                                                              * 
* You should have received a copy of the GNU General Public License, Version 2 *
* along with this file; if not, see <http://www.gnu.org/licenses/>.            *
********************************************************************************/

#ifndef __common_dvr_h__
#define __common_dvr_h__

#include "inctypes_dvr.h"
#include "entropic-config.h"

/***************** definitions originally from ClnkDefs.h ********************/
#define NETWORK_TYPE_ACCESS      ECFG_NETWORK_ACCESS
#define NETWORK_TYPE_MESH        ECFG_NETWORK_MESH
#define NODE_TYPE_ACCESS_HEADEND ECFG_NODE_HEADEND
#define NODE_TYPE_ACCESS_CPE     ECFG_NODE_CPE
#define NODE_TYPE_ACCESS_TCPE    ECFG_NODE_TCPE
#define NODE_TYPE_MESH_MOCA     (ECFG_NETWORK_MESH && ECFG_NODE_MOCA)

#if ! defined(NETWORK_TYPE_ACCESS)
#define NETWORK_TYPE_ACCESS                0
#endif

#if ! defined(HAS_FAST_IMPLEMENTATION_OF_FFS)
#define HAS_FAST_IMPLEMENTATION_OF_FFS     0
#endif

#if ! defined(HAS_FAST_IMPLEMENTATION_OF_FFZ)
#define HAS_FAST_IMPLEMENTATION_OF_FFZ     0
#endif

#if ! defined(CLNK_ETH_PER_PACKET)
#define CLNK_ETH_PER_PACKET                1
#endif

#if ! defined(ECFG_FLAVOR_VALIDATION)
#define ECFG_FLAVOR_VALIDATION             0
#endif

#if ! defined(FEATURE_IQM)
#define FEATURE_IQM                        0
#endif

/**
 * This marks code belonging to the FEIC Power Calibration project.
 */
#if ! defined(FEATURE_FEIC_PWR_CAL)
#define FEATURE_FEIC_PWR_CAL               1
#endif

/** 
 * This marks code belonging to the Pre UPnP PQoS NMS project.
 */
#if ! defined(FEATURE_PUPQ_NMS)
#define FEATURE_PUPQ_NMS                   1
#endif

/**
 * This marks code added to round out last minute customer requests. 
 */
#if ! defined(FEATURE_PUPQ_NMS_QUICK_APIS)
#define FEATURE_PUPQ_NMS_QUICK_APIS        1
#endif

/** 
 * This marks code belonging to the Pre UPnP PQoS NMS project- 
 * new configuration items.  It is not tested disabled.
 */
#if ! defined(FEATURE_PUPQ_NMS_CONF)
#define FEATURE_PUPQ_NMS_CONF              1
#endif

/**
 * This marks code (in progress) used to collect items using 
 * the L2ME Gather Queryable operation
 */
#if ! defined(FEATURE_PUPQ_GQ)
#define FEATURE_PUPQ_GQ                    1
#endif

/**
 * This marks code used to collect per-node statistics on receiver
 * received packet errors, both sync and async.
 */
#if ! defined(FEATURE_PUPQ_RX_NODE_STATS)
#define FEATURE_PUPQ_RX_NODE_STATS         1
#endif

/** 
 * Toggle this to see how code size changes when feature disabled
 */
#if ! defined(FEATURE_QOS_CCPU_ENTRY)
#define FEATURE_QOS_CCPU_ENTRY             1
#endif

/** 
 * Toggle this to see how code size changes when feature disabled
 */
#if ! defined(FEATURE_CCPU_FMR_ENTRY)
#define FEATURE_CCPU_FMR_ENTRY             1
#endif

/** 
 * Toggle this to see how code size changes when feature disabled
 */
#if ! defined(FEATURE_CCPU_GENERIC_L2ME_ENTRY)
#define FEATURE_CCPU_GENERIC_L2ME_ENTRY    1
#endif

/** 
 * Marks code that should be COMPLETELY removed from the codebase.  This
 * variable should absolutely never set to 1.  The only reason this code
 * remains in the codebase is that several projects must merge to a common
 * tree; until this merge has happened, the diffs created by deletions
 * would slow parallel progress.
 */
#define NEVER_USE_AGAIN_ECLAIR_LEGACY_TESTING  0

#if 0                                        \
    || ECFG_BOARD_PC_DVT2_PCI_ZIP2==1        \
    || ECFG_BOARD_PC_DVT_MII_ZIP2==1         \
    || ECFG_BOARD_PC_DVT_TMII_ZIP2==1        \
    || ECFG_BOARD_PC_DVT_GMII_ZIP2==1        \
    || ECFG_BOARD_COLDFIRE_DVT_FLEX_ZIP2==1  \
    || ECFG_BOARD_ECB_PCI_ZIP2==1            \
    || ECFG_BOARD_PC_PCI_ZIP2==1             \
    || ECFG_BOARD_PC_PCI_ZIP1==1             \
    || ECFG_BOARD_ECB_ROW==1                 \
    || ECFG_BOARD_PC_PCIE_MAVERICKS==1        \
    || ECFG_BOARD_ECB_4M_L3==1                 \
    || ECFG_BOARD_ECB_3M_L3==1               \
    || 0
#define L2_DONGLE 0
#else
#error NOT A KNOWN BOARD!
#endif

/**
 * Warning! this value must exactly match the value used in the ccpu image that
 * was compiled using this file.
 */
#if   (NETWORK_TYPE_ACCESS)
    #define MAX_NUM_NODES        32
    #error
#elif (NETWORK_TYPE_MESH)

/* Protem pQOS/NMS specific.  Set to 8 for
 * PROTEM builds or 16 for ADVANCED builds. 
 */
#if (ECFG_FLAVOR_PRODUCTION_PROTEM)
    #define MAX_NUM_NODES   8
#else
    #define MAX_NUM_NODES   16
#endif

#else
#error unsupported.
#endif

/* 
    log levels for HostOS_PrintLog 
    0 is highest priority

    See also:
        HOST_OS_PRINTLOG_THRESHOLD 

 */
enum
{
    L_EMERG      ,
    L_ALERT      ,
    L_CRIT       ,
    L_ERR        ,
    L_WARNING    ,
    L_NOTICE     ,
    L_INFO       ,
    L_DEBUG      ,
};


// rudimentary back-cast macro
#ifndef offsetof
#define offsetof(x, y)          ((int)&(((x *)SYS_NULL)->y))
#endif


// OS timer expiration callback function
typedef void (*timer_function_t)(unsigned long);

// Wait Q Timer exit condition function
typedef int (*HostOS_wqt_condition)(void *vp);

// Wait Q exit condition function
typedef int (*HostOS_waitq_condition)(void *vp);


/*******************************************************************************
*           G L O B A L   B I T F I E L D   M A N I P U L A T O R S            *
********************************************************************************/

/**
 * Utility to create a mask given a number of bits
 */
#define CLNKDEFS_BITMASK(num_bits)   \
   ((unsigned)(((num_bits) >= 32) ? 0xFFFFFFFF : ((1 << (num_bits)) - 1)))

/**
 * Utility to create a shifted mask given a high and low bit position
 */
#define CLNKDEFS_SHIFTED_BITMASK(bit_high, bit_low) \
   ((unsigned)(CLNKDEFS_BITMASK(1 + (bit_high) - (bit_low)) << (bit_low)))

/**
 * Utility to create a value that alters a particular set of bits
 * within a processor word
 */
#define CLNKDEFS_SUBSTITUTE_FIELD(original, bit_high, bit_low, new_field)                 \
((unsigned)(                                                                              \
    ((original)                  & (~CLNKDEFS_SHIFTED_BITMASK((bit_high), (bit_low)))) |  \
     (((new_field) << (bit_low)) &   CLNKDEFS_SHIFTED_BITMASK((bit_high), (bit_low)))     \
)          )

/**
 * Utility that alters an LVALUE with a substituted field.
 */
#define CLNKDEFS_REPLACE_FIELD(lvalue, bit_high, bit_low, new_field)            \
   do { ((lvalue)) = CLNKDEFS_SUBSTITUTE_FIELD((lvalue), (bit_high),            \
                                             (bit_low), (new_field));           \
   } while (0)

/**
 * Utility to extract a particular field from a word
 */
#define CLNKDEFS_EXTRACT_FIELD(original, bit_high, bit_low) \
     ((unsigned)(((original) & CLNKDEFS_SHIFTED_BITMASK((bit_high), (bit_low))) >> (bit_low)))

// Firmware download constants
#if ECFG_CHIP_ZIP1
#define ISPRAM_START    0xb0040000
#define DSPRAM_START    0x80000000
#define ISPRAM_SIZE     0x20000
#define DSPRAM_SIZE     0x10000
#elif ECFG_CHIP_ZIP2
#define ISPRAM_START    0x0c040000
#define DSPRAM_START    0x0c000000
#define ISPRAM_SIZE     0x20000
#define DSPRAM_SIZE     0x18000
#elif ECFG_CHIP_MAVERICKS
#define ISPRAM_START    0x0c040000
#define DSPRAM_START    0x0c000000
#define ISPRAM_SIZE     0x30000
#define DSPRAM_SIZE     0x20000
#endif

/* for hw_z2_pci and clnkEth */
#define SYS_GET_PARAM(x, p, mask, offset) (((x)->p >> (offset)) & (mask))
#define SYS_SET_PARAM(x, p, mask, offset, val) do \
            { \
                (x)->p = ((x)->p & ~((mask) << (offset))) | \
                         (((val) & (mask)) << (offset)); \
            } while(0)
#define WR_LINEBUF_PDU_SIZE(x, y)   SYS_SET_PARAM((x), packed_0, 0xffff, 0, (y))
#define WR_LINEBUF_ENTRY_SIZE(x, y) SYS_SET_PARAM((x), packed_0, 0xff, 16, (y))
#define WR_LINEBUF_PTR_MODE(x, y)   SYS_SET_PARAM((x), packed_0, 0x1, 24, (y))
/* end for hw_z2_pci and clnkEth */

#define NUM_DEBUG_REG               8
#define NUM_MAILBOX_RD_REG          8
#define NUM_MAILBOX_WR_REG          8
#define NUM_MAILBOX_REG             (NUM_MAILBOX_RD_REG + NUM_MAILBOX_WR_REG)

/// Returns the SoC memory address of element 'x' in dev_shared
#define DEV_SHARED(x) ((SYS_UINT32)(SYS_UINTPTR)(&(((struct shared_data *)DSPRAM_START)->x)))

struct clnk_hw_params
{
    SYS_UINT32              numRxHostDesc;
    SYS_UINT32              numTxHostDesc;
#if ECFG_CHIP_ZIP1
    SYS_UINT32              *minRxLens;
#endif
    void                    *at_lock;
};

typedef struct
{
    SYS_UINT32      zip_major;
    SYS_UINT32      zip_minor;
    SYS_UINT32      datapath;
}
ClnkDef_ZipInfo_t;

//linebuffer descriptor
struct linebuf
{
    SYS_UINT32      packed_0;       /* offset 0x0 */
    SYS_UINT32      n_entries;      /* offset 0x4 */
    SYS_UINT32      buffer_base;    /* offset 0x8 */
    SYS_UINT32      buffer_len;     /* offset 0xc */
    SYS_UINT32      entry_wptr;     /* offset 0x10 */
    SYS_UINT32      entry_rptr;     /* offset 0x14 */
    SYS_UINT32      entry_windex;   /* offset 0x18 */
    SYS_UINT32      entry_rindex;   /* offset 0x1c */
    SYS_UINT32      packed_8;       /* offset 0x20 */
};

#define BRIDGE_ENTRIES      192
#if ECFG_CHIP_MAVERICKS
#define CAM_ENTRIES         160
#else
#define CAM_ENTRIES         128
#endif
#define DATABUF_CAM_ENTRIES 16

typedef struct
{
    SYS_UINT32 macAddrHigh;
    SYS_UINT32 macAddrLow;
}
ClnkDef_BridgeEntry_t; 

/* dumps out a portion of the software CAM (ClnkCam.c) */
typedef struct
{
    SYS_UINT32 num_entries;
    ClnkDef_BridgeEntry_t   ent[BRIDGE_ENTRIES];
}
ClnkDef_BridgeTable_t;

#define NUM_TX_PRIO         3
// clnk_set_eth_fifo_size struct
typedef struct
{
    SYS_UINT32 tx_prio[NUM_TX_PRIO];
}
ClnkDef_TxFifoCfg_t;

#if FEATURE_IQM
// Increase the trace buffer size when DEBUG_DUMP_FFT_VALUES enabled in ccpu code (iqm.h)
#define CLNK_DEF_TRACE_ENTRIES      /*2505*/ 150
#else
#define CLNK_DEF_TRACE_ENTRIES      150
#endif
typedef struct
{
    SYS_UINT32 ctc_time;
    SYS_UINT32 info1;
    SYS_UINT32 info2;
}
ClnkDef_TraceEntry_t;

typedef struct
{
    SYS_UINT32 dropped;
    SYS_UINT32 ent_valid;
    ClnkDef_TraceEntry_t ent;
}
ClnkDef_TraceBuf_t;

/*
 * Frequency Scanning (FS) configuration data
 */
typedef struct 
{
    SYS_UINT32    scan_mask;
    SYS_UINT32    prod_mask;
    SYS_UINT32    chanl_mask;
    SYS_UINT32    chanl_plan;
    SYS_UINT32    bias_and_max_passes;
    SYS_UINT32    cm_ratio;
    SYS_UINT32    taboo_info;
    SYS_INT32     lof;
} fs_config_t;

/*
 * Frequency Scanning status flags
 */
typedef struct
{
    SYS_UINT8    lof_avail:1;     // LOF available (1:true, 0:false) 
    SYS_UINT8    lof_scan:1;      // LOF scan status (1:true, 0:false)
    SYS_UINT8    single_chanl:1;  // Single channel prod mask (1:true, 0:false)
    SYS_UINT8    listen_once:1;   // Listen only channel (1:true, 0:false)
    SYS_UINT8    fs_reset:1;      // FS state machine reset (1:true, 0:false)
    SYS_UINT8    reserved:3;      // Reserved for future usage
} fs_flags_t;

/*
 * Frequency Scanning context data
 */
typedef union 
{
    SYS_UINT32    word[8];
    struct 
    {
        fs_flags_t   flags;         // FS status flags
        SYS_UINT8    scan_count;    // Number of full scans
        SYS_INT8     scan_dir;      // Scan direction (+1:lo->hi, -1:hi->lo)
        SYS_INT8     tune_idx;      // Tuned freq index
        SYS_INT8     last_idx;      // Last freq index
        SYS_INT8     low_idx;       // lowest freq index for scan in single dir
        SYS_INT8     high_idx;      // highest freq index for scan in single dir
        SYS_INT8     lof_idx;       // LOF index
        SYS_UINT8    fs_state;      // Current FS state
        SYS_UINT8    upd_cause;     // FS state update cause
        SYS_UINT16   hop_count;     // Running count of frequency hops
        SYS_UINT32   chanl_qual[2]; // Channel quality array
    } members;
} fs_context_t;

/** Structure for storing cached pqos maintenace values in shared
 *  data. */
typedef struct {
    SYS_UINT32 ioc_nodemask;
    SYS_UINT32 allocated_stps;
    /** Top 16 bits store the allocated txps received (but thresholded
     *  in the error event that the value exceeded 16 bits) and the 
     *  bottom 16 bits store the count of maintenance events since
     *  joining the network. */
    SYS_UINT32 trunc_alloc_txps_hi__ctr_lo;
} shared_qos_maint_cache_t;

#if FEATURE_PUPQ_NMS_CONF
// pqos NMS initialization structure.
typedef struct
{
    /** This member contains the vendor id 16 bit value in the high
     * 16 bits and the personality in the low 16 bits. Saves CCPU space. */
    SYS_UINT32 mfrVendorIdHi_personalityLo;
    SYS_UINT32 mfrHwVer;
    SYS_UINT32 mfrSwVer;
} pupq_nms_init_context_t;
#endif /* FEATURE_PUPQ_NMS_CONF */

#if FEATURE_PUPQ_RX_NODE_STATS
// per-node based statistics
typedef struct
{
    /** Number of received MoCA transmissions from this node which were
     *  NOT correctly received or processed. */
    SYS_UINT32 mtrans_p2p_rx_count_error;

    /** Number of received MoCA transmissions from this node which were
     *  correctly received and processed. */
    SYS_UINT32 mtrans_p2p_rx_count_good;
}                           clnkdefs_per_node_stats_t;
INCTYPES_VERIFY_STRUCT_SIZE(clnkdefs_per_node_stats_t, 8);

typedef struct
{
    clnkdefs_per_node_stats_t nodes[MAX_NUM_NODES];
}                           clnkdefs_all_node_stats_t;
#if MAX_NUM_NODES == 16
INCTYPES_VERIFY_STRUCT_SIZE(clnkdefs_all_node_stats_t, 128);
#else
INCTYPES_VERIFY_STRUCT_SIZE(clnkdefs_all_node_stats_t, 64);
#endif
#endif /* FEATURE_PUPQ_RX_NODE_STATS */

#if FEATURE_FEIC_PWR_CAL
// defines for FEIC Profiles
#define NUM_ADC_MEASUREMENTS       64       // must be mustiple of 4
#define MAX_FEIC_PROFILES          (10+1)   // feic.conf must not have more profiles than this
#define MAX_RF_BAND_STR_SIZE       (7+1)    // null terminated
#define MAX_FEIC_TYPE_STR_SIZE     (15+1)   // null terminated
#define MAX_FREQ_INDEXES           8        // 8 frequencies interpolated to 29
                                            // by the SoC firmware
#define MAX_FREQ_WORDS             ((MAX_FREQ_INDEXES + sizeof(SYS_UINT32) - 1) / sizeof(SYS_UINT32))   // 2
                                            // by the SoC firmware
#define MAX_TEMPERATURE_INDEXES    5        // 5 temperatures interpolated to 32
                                            // by the SoC firmware
                                            // use 9, 13, 17, 21, 25

typedef struct
{
    SYS_CHAR    rfBandStr[MAX_RF_BAND_STR_SIZE];
    SYS_CHAR    feicTypeStr[MAX_FEIC_TYPE_STR_SIZE];
    SYS_INT32   feicProfileId;      // only used to pass pid through API calls
    SYS_UINT32  tempClass;          // 0=Industrial (-40 degC to +85 degC)
                                    // 1=Commercial (0 degC to +70 degC)
    SYS_UINT32  M0Vars;             // bits0-7=adcOffCorrection, bits8-15=m0DeltaAdcOn;
    SYS_INT32   targetLsbBias;      // same for all profiles
    SYS_UINT32  nominalGainsPacked; // profile 0 specific nominal gains packed
    SYS_UINT32  feicFileFound;      // indicates feic.conf file found
    union
    {
        struct
        {
            SYS_INT8    errThreshold2[4];
            SYS_INT8    errThreshold3[4];
            SYS_INT8    errThreshold4[4];
            SYS_UINT8   tgtDeltaAdcEsf[MAX_FREQ_INDEXES];
            SYS_INT8    tgtDeltaAdcDbm[MAX_TEMPERATURE_INDEXES][MAX_FREQ_INDEXES];
            SYS_UINT8   tgtDeltaAdcLsb[MAX_TEMPERATURE_INDEXES][MAX_FREQ_INDEXES];
        } bytes;
        struct
        {
            SYS_UINT32  errThreshold2;
            SYS_UINT32  errThreshold3;
            SYS_UINT32  errThreshold4;
            SYS_UINT32  tgtDeltaAdcEsf[MAX_FREQ_WORDS];
            SYS_UINT32  tgtDeltaAdcDbm[MAX_TEMPERATURE_INDEXES][MAX_FREQ_WORDS];
            SYS_UINT32  tgtDeltaAdcLsb[MAX_TEMPERATURE_INDEXES][MAX_FREQ_WORDS];
        } words;
    };
} feic_profile_t;

typedef struct
{
    SYS_INT32       cfgFeicProfileId;
    feic_profile_t  profiles[MAX_FEIC_PROFILES];
} feic_cfg_t;

typedef struct
{
    SYS_UINT8   tempAdc;
    SYS_UINT8   freqIndex;                  // frequency index
    SYS_INT8    targetLsbBias;              // signed, units of 1/8th LSB
                                            // from feic.conf
    SYS_UINT8   tLsbPlusTargetLsbBias;      // units of lsb, lookup from
                                            // TargetDeltaAdcLsb in feic.conf +
                                            // targetLsbBias
    SYS_INT8    targetPwrDbmT10;            // signed, units of 1/10th dBm
                                            // from feic.conf
    SYS_UINT8   dummy1;
    SYS_UINT8   dummy2;
    SYS_UINT8   dummy3;
} TargetPwrStatus_t;

typedef struct
{
    SYS_INT8        m0TxdCGainSel;
    SYS_UINT8       m0RficTxCal;
    SYS_UINT8       m0FeicAtt1;
    SYS_UINT8       m0FeicAtt2;
    SYS_UINT8       m0TxOn[NUM_ADC_MEASUREMENTS];
    SYS_UINT8       m0TxOnAvg;
    SYS_UINT8       m0TxOffAvg;
    SYS_UINT8       m0DeltaAdcBelowMin;
    SYS_INT8        m0ErrorQtrDb;
    SYS_INT8        m1TxdCGainSel;
    SYS_UINT8       m1RficTxCal;
    SYS_UINT8       m1FeicAtt1;
    SYS_UINT8       m1FeicAtt2;
    SYS_UINT8       m1TxOn[NUM_ADC_MEASUREMENTS];
    SYS_UINT8       m1TxOnAvg;
    SYS_INT8        m1ErrorQtrDb;
    SYS_INT8        m2TxdCGainSel;
    SYS_UINT8       m2RficTxCal;
    SYS_UINT8       m2FeicAtt1;
    SYS_UINT8       m2FeicAtt2;
    SYS_UINT8       m2Dummy1;
    SYS_UINT8       m2Dummy2;
    SYS_UINT8       m2TxOn[NUM_ADC_MEASUREMENTS];
    SYS_UINT8       m2TxOnAvg;
    SYS_INT8        m2DcOffset;
    SYS_INT8        m2Dummy;
    SYS_UINT8       m0m1ErrorSmall;
    SYS_INT8        m3TxdCGainSel;
    SYS_UINT8       m3RficTxCal;
    SYS_UINT8       m3FeicAtt1;
    SYS_UINT8       m3FeicAtt2;
    SYS_UINT8       m3TxOn[NUM_ADC_MEASUREMENTS];
    SYS_UINT8       m3TxOnAvg;
    SYS_INT8        m3ErrorQtrDb;
    SYS_UINT8       m3ErrorSmall;
    SYS_INT8        m4TxdCGainSel;
    SYS_UINT8       m4RficTxCal;
    SYS_UINT8       m4FeicAtt1;
    SYS_UINT8       m4FeicAtt2;
    SYS_UINT8       m4Dummy1;
    SYS_UINT8       m4TxOn[NUM_ADC_MEASUREMENTS];
    SYS_UINT8       m4TxOnAvg;
    SYS_INT8        m4ErrorQtrDb;
    SYS_UINT8       m4Dummy2;
    SYS_UINT8       m4Dummy3;
} IntermediateDetectorStatus_t;

typedef struct
{
    SYS_INT8        feicProfileId;
    SYS_UINT8       feicFileFound;          // 0=No, 1=Yes
    SYS_UINT8       tempClass;
    SYS_UINT8       CalDisabled;
    SYS_UINT8       CalBypassed;
    SYS_INT8        targetLsbBias;          // signed
    SYS_INT8        txdCGainSel;            // signed
    SYS_UINT8       rficTxCal;
    SYS_UINT8       feicAtt1;
    SYS_UINT8       feicAtt2;
    SYS_INT8        pwrEstimateT10;         // signed, units of 1/10th dBm
    SYS_UINT8       calErr;                 // 0=No, 1=Yes
    SYS_UINT8       freqIndex;              // frequency index
    SYS_UINT8       dummy1;
    SYS_UINT8       dummy2;
    SYS_UINT8       dummy3;
    SYS_CHAR        rfBandStr[MAX_RF_BAND_STR_SIZE];
} FinalConfigStatus_t;

// this structure is sent to host via unsolicited ctl msg for output
typedef struct
{
    TargetPwrStatus_t               targetPwrStatus;            // only when pwr cal enabled
    IntermediateDetectorStatus_t    intermediateDetectorStatus; // always reported
    FinalConfigStatus_t             finalConfigStatus;          // always reported
} FeicStatusData_t;

typedef struct
{
    FinalConfigStatus_t finalConfigStatus;
} feic_final_results_t;
#endif /* FEATURE_FEIC_PWR_CAL */

// shared data between the host and SoC (Zip2 only)
struct shared_data
{
#if !ECFG_CHIP_ZIP1
    SYS_UINT32      mailbox_reg[NUM_MAILBOX_REG];
    SYS_UINT32      debug_reg[NUM_DEBUG_REG];
    SYS_UINT32      read_csr_reg;
    SYS_UINT32      write_csr_reg;

    SYS_UINT32      linebuf_host_rx;
    SYS_UINT32      linebuf_host_tx[4]; /* 4 priorities */

    SYS_UINT32      mac_addr_hi;
    SYS_UINT32      mac_addr_lo;

    SYS_UINT32      rx_active;
#endif /* !ECFG_CHIP_ZIP1 */

    SYS_UINT32      trace_dropped;

    fs_config_t     fs_cfg;                // Frequency Scan config parameters
    fs_context_t    fs_ctx;                // Frequency Scan context data
    SYS_UINT32      dbgMask;

    SYS_UINT32      target_phy_rate;
    SYS_UINT32      node_adm_req_rx_ctr;
    SYS_UINT32      node_link_up_event_ctr;
    SYS_UINT32      lmo_advanced_ctr;
    SYS_UINT32      ingr_routing_conflict_ctr;
    /** This counter counts in tenths of a second. */
    SYS_UINT32      node_link_time_bad_deciseconds;

    SYS_UINT32      mtrans_tx_pkt_ctr; 

#if FEATURE_FEIC_PWR_CAL
    SYS_INT32       feicProfileId;
    feic_profile_t  feic_profile;
    feic_final_results_t  feic_final_results;
#endif

#if L2_DONGLE
    SYS_UINT32      version_id; 
#endif  

#if FEATURE_PUPQ_RX_NODE_STATS
    /** ONLY MODIFIED BY CCPU.  Kept here as a convenience since
     *  the host may want to do a fast read of these statistics without
     *  mailbox op.  */
    clnkdefs_all_node_stats_t  all_node_stats;
#endif

#if FEATURE_PUPQ_NMS_CONF
    pupq_nms_init_context_t  pni_ctx;
#endif /* FEATURE_PUPQ_NMS_CONF */

#if FEATURE_PUPQ_NMS_QUICK_APIS
    SYS_UINT32      pqos_mode;
    SYS_UINT32      hard_reset;
    /* added shared_data entries 
     *   vinirn is (UINT16 vendor_id << 16) | (UINT8 node_id << 8) | UINT8 request_number
     */
    SYS_UINT32      vinirn;    // vendor id, node id, request number

    /* eventually fit this in a byte somewhere else in shared_data 
     * Both nmspush_change_ctr and nmspush_change_ack are 
     * each only 8 bits 
     */
    SYS_UINT32      nmspush_change_ctr;
    SYS_UINT32      nmspush_change_ack;
    SYS_UINT32      dword_array[6];
#endif

#if FEATURE_IQM
    SYS_UINT32      iqm_debug_mask1;
    SYS_UINT32      iqm_debug_mask2;
    SYS_UINT32      iqm_debug_mask3;
#endif

    ClnkDef_TraceEntry_t trace_buf[CLNK_DEF_TRACE_ENTRIES];

    shared_qos_maint_cache_t  qos_maint_cache;
};

#define FEATURE_ECLAIR                     1

#if (NETWORK_TYPE_ACCESS)
    #define FEATURE_FREQ_SCAN              0      // 0: disable
    #define FEATURE_OLDQOS                 0
    #define FEATURE_MRT                    0
    #define FEATURE_QOS                    0
    #define ENTROPIC_ECLAIR                0
    #define BROADCAST_LMO                  0
#else
    #define FEATURE_FREQ_SCAN              1      // 1: enable
    #define FEATURE_OLDQOS                 0
    #define FEATURE_MRT                    1
    #define FEATURE_QOS                    1
    #define ENTROPIC_ECLAIR                1
    #define BROADCAST_LMO                  1

    /** Expect this flag to be set from the build file *.bld */
    #ifndef FEATURE_ECLAIR_WHITE_BOX_TEST
        #define FEATURE_ECLAIR_WHITE_BOX_TEST  0
    #endif

#endif

// C.Link Ethernet Statistics Structureyy
typedef struct
{
    // Driver stats
    SYS_UINT32 drvSwRevNum;
    SYS_UINT32 embSwRevNum;
    SYS_UINT32 upTime;
    SYS_UINT32 linkUpTime;
    SYS_UINT32 socResetCount;       // Number of C.Link resets
    SYS_UINT32 socResetHistory;     // Reason for last 4 resets

    // Transmit and recieve stats
    SYS_UINT32 rxPackets;           // Total packets received
    SYS_UINT32 txPackets;           // Total packets transmitted
    SYS_UINT32 rxBytes;             // Total bytes received
    SYS_UINT32 txBytes;             // Total bytes transmitted
    SYS_UINT32 rxPacketsGood;       // Packet receive with no errors
    SYS_UINT32 txPacketsGood;       // Packet transmitted with no errors
    SYS_UINT32 rxPacketErrs;        // Packets received with errors
    SYS_UINT32 txPacketErrs;        // Packets transmitted with errors
    SYS_UINT32 rxDroppedErrs;       // Packets dropped with no host buffers
    SYS_UINT32 txDroppedErrs;       // Packet dropped with no host buffers
    SYS_UINT32 rxMulticastPackets;  // Total multicast packets received
    SYS_UINT32 txMulticastPackets;  // Total multicast packets transmitted
    SYS_UINT32 rxMulticastBytes;    // Total multicast bytes received
    SYS_UINT32 txMulticastBytes;    // Total multicast bytes transmitted

    // Detailed receive errors
    SYS_UINT32 rxLengthErrs;
    SYS_UINT32 rxCrc32Errs;         // Packets rx'd with CRC-32 errors
    SYS_UINT32 rxFrameHeaderErrs;   // Packets rx'd with frame header errors
    SYS_UINT32 rxFifoFullErrs;      // Packets rx'd with FIFO full
    SYS_UINT32 rxListHaltErr[3];

    // Detailed transmit errors
    SYS_UINT32 txCrc32Errs;         // Packets tx'd with CRC-32 errors
    SYS_UINT32 txFrameHeaderErrs;   // Packets tx'd with frame header errors
    SYS_UINT32 txFifoFullErrs;      // Packets tx'd with FIFO full
    SYS_UINT32 txFifoHaltErr[8];
}
ClnkDef_EthStats_t;

// EVM Data Structure
typedef struct
{
    SYS_UINT32 valid;
    SYS_UINT32 NodeId;
    SYS_UINT32 Data[256];
}
ClnkDef_EvmData_t;

#if FEATURE_FEIC_PWR_CAL
// FEIC Status Structure
typedef struct
{
    SYS_UINT32 valid;
    FeicStatusData_t Data;
}
ClnkDef_FeicStatus_t;
#endif

// Echo Profile Structures
typedef struct
{
    SYS_UINT32 valid;
    SYS_UINT32 NodeId;
    SYS_UINT32 Data[(256 + 3*(256+64))];    /* 0x4c0 words = 4864 bytes */
}
ClnkDef_EppData_t;      

//values returned to the host in the initial mailbox
struct mb_return
{
    SYS_UINT32 linebuf0_soc_tx;         /* word 0 */
    SYS_UINT32 linebuf1_soc_tx;         /* word 1 */
    SYS_UINT32 linebuf2_soc_tx;         /* word 2 */
    SYS_UINT32 extra_pkt_mem;           /* word 3 */
    SYS_UINT32 linebuf3_soc_tx;         /* word 4 */
    SYS_UINT32 linebuf_soc_rx;          /* word 5 */
    SYS_UINT32 tx_did;                  /* word 6 */
    SYS_UINT32 rx_did;                  /* word 7 */
    SYS_UINT32 tx_gphy_desc;            /* word 8 */
    SYS_UINT32 rx_gphy_desc;            /* word 9 */
    SYS_UINT32 pd_queue_0;              /* word 10 */
    SYS_UINT32 pd_queue_1;              /* word 11 */
    SYS_UINT32 pd_queue_2;              /* word 12 */
    SYS_UINT32 pd_queue_3;              /* word 13 */
    SYS_UINT32 pd_entries;              /* word 14 */
    SYS_UINT32 unsol_msgbuf;            /* word 15 - also the semaphore */
};
////////// END defines needed by ClnkEth.h

////////// defines for ClnkCam

#define    CLNKMAC_BCAST_ADDR      0x3F
#define    QOS_MAX_FLOWS           24

/* Enums for PQOS Classification mode */
enum {
    CLNK_PQOS_MODE_UNKNOWN       = -1,
    CLNK_PQOS_MODE_MOCA_11       = 0,
    CLNK_PQOS_MODE_PUPQ_CLASSIFY = 1,
};
typedef SYS_UINT32 clnk_pqos_mode_t;

////////// end defines for ClnkCam

////////// defines for eth_ioctl

// Software Configuration Bits
#define CLNK_DEF_SW_CONFIG_SOFTCAM_BIT                 (1 << 14)
#define CLNK_DEF_SW_CONFIG_AGGREGATION_METHOD_BIT      (1 << 24)  // used in hw_z2_pci.c
                                                                   
// SOC Status Values
typedef enum
{
    CLNK_DEF_SOC_STATUS_SUCCESS           = 0,
    CLNK_DEF_SOC_STATUS_EMBEDDED_TIMEOUT  = 1,
    CLNK_DEF_SOC_STATUS_EMBEDDED_FAILURE  = 2,
    CLNK_DEF_SOC_STATUS_LINK_CTRL_RESTART = 3,
    CLNK_DEF_SOC_STATUS_LINK_DOWN_FAILURE = 4,
    CLNK_DEF_SOC_STATUS_TX_HALTED_FAILURE = 5,
    CLNK_DEF_SOC_STATUS_RX_HALTED_FAILURE = 6,
    CLNK_DEF_SOC_STATUS_FORCED_RESET      = 7,

    CLNK_DEF_SOC_STATUS_MAX  // This must always be last
}
CLNK_DEF_SOC_STATUS_VALUES;

// a subset Embedded failure reasons
typedef enum                                    // Hex
{
#if (FEATURE_FREQ_SCAN)
    CLNK_DEF_FREQ_RESET_BAD_CRC       = 23,     // 17
    CLNK_DEF_FREQ_RESET_FTM_EXPIRE    = 24,     // 18
    CLNK_DEF_FREQ_RESET_CHNL_MISMATCH = 25,     // 19
    CLNK_DEF_FREQ_ADM_FAILURE         = 26,     // 1A
    CLNK_DEF_FREQ_PREAMBLE_MISS       = 27,     // 1B
    CLNK_DEF_FREQ_SCAN_TABOO_ONLY     = 28,     // 1C
#endif /* FEATURE_FREQ_SCAN */
    CLNK_DEF_ACCESS_DENY              = 29,     // 1D
    CLNK_DEF_LINK_CTRL_RESTART_MAX  // This must always be last

}
CLNK_DEF_LINK_CTRL_RESTART_EVENTS;

//variables to be used by driver
typedef struct
{
    SYS_UINT32 swConfig;     // data plane variables for PCI driver
    SYS_UINT32 unsol_msgbuf; // for mailbox init
    SYS_UINT32 pqosClassifyMode;
    SYS_UINT32 pSwUnsolQueue;
    SYS_UINT32 swUnsolQueueSize;
}
ClnkDef_dataPlaneVars_t;

// C.Link Node Statistics Structure
typedef struct
{
    SYS_UINT32 NumOfMapTx;
    SYS_UINT32 NumOfRsrvTx;
    SYS_UINT32 NumOfLCTx;
    SYS_UINT32 NumOfAdmTx;
    SYS_UINT32 NumOfProbeTx;
    SYS_UINT32 NumOfAsyncTx;

    SYS_UINT32 NumOfMapTxErr;
    SYS_UINT32 NumOfRsrvTxErr;
    SYS_UINT32 NumOfLCTxErr;
    SYS_UINT32 NumOfAdmTxErr;
    SYS_UINT32 NumOfProbeTxErr;
    SYS_UINT32 NumOfAsyncTxErr;

    SYS_UINT32 NumOfMapRx;
    SYS_UINT32 NumOfRsrvRx;
    SYS_UINT32 NumOfLCRx;
    SYS_UINT32 NumOfAdmRx;
    SYS_UINT32 NumOfProbeRx;
    SYS_UINT32 NumOfAsyncRx;

    SYS_UINT32 NumOfMapRxErr;
    SYS_UINT32 NumOfRsrvRxErr;
    SYS_UINT32 NumOfLCRxErr;
    SYS_UINT32 NumOfAdmRxErr;
    SYS_UINT32 NumOfProbeRxErr;
    SYS_UINT32 NumOfAsyncRxErr;

    SYS_UINT32 NumOfMapRxDropped;
    SYS_UINT32 NumOfRsrvRxDropped;
    SYS_UINT32 NumOfLCRxDropped;
    SYS_UINT32 NumOfAdmRxDropped;
    SYS_UINT32 NumOfProbeRxDropped;
    SYS_UINT32 NumOfAsyncRxDropped;

    SYS_UINT32 NumOfBadIsocTx;
    SYS_UINT32 NumOfCtlDescrFailed;
    SYS_UINT32 NumOfUpdateDescrFailed;
    SYS_UINT32 NumOfStatDescrFailed;
    SYS_UINT32 NumOfBufferAllocFailed;
    SYS_UINT32 NumOfRSCorrectedBytes;
    SYS_UINT32 Events;
    SYS_UINT32 Interrupts;

    SYS_UINT32 InternalWarnings;
    SYS_UINT32 InternalErrors;

    SYS_UINT32 qosRcvdSubmits;
    SYS_UINT32 qosRcvdRequests;
    SYS_UINT32 qosRcvdResponses;
    SYS_UINT32 qosAcceptedSubmits;
    SYS_UINT32 qosDroppedSubmits;
    SYS_UINT32 qosIssuedWaves;
    SYS_UINT32 qosSkippedWaves;
    SYS_UINT32 qosSuccessWaves;
    SYS_UINT32 qosFailedWaves;

    SYS_UINT32 qosIssuedTxns;
    SYS_UINT32 qosSuccessTxns;
    SYS_UINT32 qosEarlyTerminatedTxns;
    SYS_UINT32 qosEntryCancelledTxns;
    SYS_UINT32 qosTxnErrors;
    SYS_UINT32 qosPerformedEntryCancels;
    SYS_UINT32 qosRcvdTxnErrors;

    SYS_UINT32 qosTotalEgressBurst;
    SYS_UINT32 qosMaxEgressBurst;
}
ClnkDef_Stats_t;

// C.Link My Node Info Structure
typedef struct
{
    SYS_UINT32      ClearStats;
    SYS_UINT32      SwRevNum;
    SYS_UINT32      EmbSwRevNum;
    SYS_UINT32      LinkStatus;
    SYS_UINT32      TxChannelBitMask;
    SYS_UINT32      RxChannelBitMask;
    SYS_UINT32      IsCyclemaster;
    SYS_UINT32      RFChanFreq;
    SYS_UINT32      CurrNetworkState;
    SYS_UINT32      NetworkType;
    SYS_UINT32      NodeId;
    SYS_UINT32      CMNodeId;
    SYS_UINT32      BestCMNodeId;
    SYS_UINT32      BackupCMNodeId;
    SYS_UINT32      NetworkNodeBitMask;
    SYS_UINT32      TxIsocChanInfo;
    SYS_UINT32      RxIsocChanInfo;
    SYS_UINT32      PrivacyStat;
    SYS_UINT32      MocaField;
    SYS_UINT32      TxIQImbalance;
    SYS_UINT32      RxIQImbalance;
    SYS_UINT32      RxIQImbalance2;
    ClnkDef_Stats_t Stats;
    SYS_UINT32      TabooChanMask;
    SYS_UINT32      TabooMaskStart;
}
ClnkDef_MyNodeInfo_t;
// end defines for eth_ioctl
// **********************************************
// END Inserting all ClnkDefs.h neeeded by driver 

/**********    Common to ClnkCAM and ClnkETH   ***************/
#if (NETWORK_TYPE_ACCESS)

    #error not tested for a long time

    #define CLNK_ETH_VLAN_ACTUAL      (2)
    #define MAX_TX_FIFOS              (CLNK_ETH_VLAN_ACTUAL)     /* HW Queues */
    #define MAX_TX_PRIORITY           (32*CLNK_ETH_VLAN_ACTUAL)  /* SW Queues */
    #define TX_ADVERTIZE_SIZE         (162)                      //- (200)
    #define TX_MAPPING_SIZE           ((TX_ADVERTIZE_SIZE+8) * 32)
    #define MAP_PRIO_TO_FIFO(PRIO)    ((PRIO) >> 5)
    #define TMR_2_JIFFIES             (2456)

#else

  #define  CLNK_VLAN_MODE_HWVLAN   1  /* Nominal: 3 SW Q ---> 3 HW Q */
  #define  CLNK_VLAN_MODE_SWVLAN   0  /* Alternate: 3 SW Q ---> 1 HW Q */

  #if (CLNK_VLAN_MODE_HWVLAN)

    #define CLNK_ETH_VLAN_ACTUAL      3 /* Nominal 3 but any value up to 8 */
    #define MAX_TX_FIFOS              (CLNK_ETH_VLAN_ACTUAL+FEATURE_QOS)  /* HW Qs*/
    #define MAX_TX_PRIORITY           (CLNK_ETH_VLAN_ACTUAL+FEATURE_QOS)  /* SW Qs*/
    #define TX_ADVERTIZE_SIZE         256
#if ECFG_CHIP_ZIP1
    #define TX_MAPPING_SIZE           400
    #define CAM_TMR_JIFFIES           TMR_2_JIFFIES
#else
    #define TX_MAPPING_SIZE           ((TX_ADVERTIZE_SIZE + 1) * MAX_TX_PRIORITY)
    #define CAM_TMR_JIFFIES           100               /* 100 clocks/sec = 10ms period */
#endif
    #define MAP_PRIO_TO_FIFO(PRIO)    (PRIO)
    #if (CLNK_ETH_PER_PACKET)
      #define TMR_2_JIFFIES             (1111)
   #else
      #define TMR_2_JIFFIES             (3333)
   #endif

  #elif (CLNK_VLAN_MODE_SWVLAN)

    #error not tested for a long time

    #define CLNK_ETH_VLAN_ACTUAL      8
    #define MAX_TX_FIFOS              1                                   /* HW Qs */
    #define MAX_TX_PRIORITY           (CLNK_ETH_VLAN_ACTUAL+FEATURE_QOS)  /* SW Qs */
    #define TX_ADVERTIZE_SIZE         128
    #define TX_MAPPING_SIZE           (TX_ADVERTIZE_SIZE * MAX_TX_PRIORITY)
    #define MAP_PRIO_TO_FIFO(PRIO)    (0)
    #if (CLNK_ETH_PER_PACKET)
      #define TMR_2_JIFFIES             (1111)
   #else
      #define TMR_2_JIFFIES             (2468)
   #endif

  #endif

#endif
/************* End definitions originally from ClnkDefs.h ********************/


/************************** original common.h defines ************************/
// Number of VLANs
#define CLNK_ETH_VLAN_8021Q 8      // Number Logical 802.1Q VLAN Queues
//#define CLNK_ETH_VLAN_ACTUAL xx  // See ClnkEth.h
#define MAX_POSSIBLE_FIFOS    8

/* Firmware image struct
 *
 * fw.pFirmware can point to one of three things:
 *
 * 1) the builtin firmware
 * 2) a user-supplied firmware image
 * 3) NULL (don't download firmware)
 *
 * All fields are native-endian.
 */
struct fw_img
{
    SYS_UINT32  *fw_text;       // instruction memory image
    SYS_UINT32  fw_text_size;   // image size
    SYS_UINT32  *fw_data;       // initialized data memory image
    SYS_UINT32  fw_data_size;   // image size
    SYS_UINT32  version;        // firmware version
    SYS_UINT8   builtin;        // 1=built in to the driver, 0 if user-supplied
    SYS_UINT8   native;         // 1=native endianness, 0=SoC (BE) endianness
};

// Firmware Structure
typedef struct
{
    const struct fw_img *pFirmware; // Pointer to firmware info struct
    SYS_UINT32  DistanceMode;
    SYS_UINT32  privacyKey[4];  // MMK and PMKi
    SYS_UINT32  cmRatio;        // Cycle master ratio (0% to 100%)
    SYS_UINT32  txPower;        // Transmit power
    SYS_UINT32  phyMargin;      // Phy margin settings
    SYS_UINT32  phyBitMask;     // Phy #bits
    SYS_UINT32  swConfig;       // Software configuration
#if defined(L3_DONGLE_HIRF)
    SYS_UINT32  Diplexer_;	// Diplxere Mode
#endif    
    SYS_UINT32  beaconPwrLevel; // Beacon power level setting. 
    SYS_UINT32* pRFIC_Tuning_Data; // pointer to RFIC tuning data channel table for all channels
    SYS_UINT32* pAGC_Gain_Table;// pointer to AGC gain table
    SYS_UINT32  macAddrHigh;    // C.Link MAC address high (bytes 0-3)
    SYS_UINT32  macAddrLow;     // C.Link MAC address low (bytes 4-5)
    SYS_UINT8   txFifoLut[CLNK_ETH_VLAN_8021Q]; // Priority Mapping
#if ECFG_CHIP_ZIP1
    SYS_UINT16  txFifoPct[MAX_POSSIBLE_FIFOS]; // Tx fifo size (initial, backup)
    SYS_UINT16  txFifoSize[MAX_POSSIBLE_FIFOS]; // Tx fifo size (w/o flows)
#if FEATURE_QOS
    SYS_UINT16  txFifoSizeQos[MAX_POSSIBLE_FIFOS]; // Tx fifo size (w/ flows)
#endif /* FEATURE_QOS */
    SYS_UINT16  txFifoChunks[MAX_POSSIBLE_FIFOS]; // Tx fifo size (runtime,  in chunks)
    SYS_UINT32  rxFifoSize;      // Rx fifo size
#endif /* ECFG_CHIP_ZIP1 */
    SYS_UINT32  channelMask;     // Tunable RF Channel Mask
    SYS_UINT32  productMask;     // Product Mask
    SYS_UINT32  scanMask;        // Scan mask
    SYS_UINT32  lof;             // LOF
    SYS_UINT32  bias;            // Bias
    SYS_UINT32  channelPlan;     // Channel Plan
    SYS_UINT32  tabooInfo;       // Taboo mask + taboo offset
    SYS_UINT32  PowerCtlPhyRate; // Power control PHY rate
    SYS_UINT32  MiiPausePriLvl;  // MII Pause Priority Level
    SYS_UINT32  PQoSClassifyMode;// PQoS Classification Mode

#if FEATURE_PUPQ_NMS_CONF
    SYS_UINT32  mfrVendorId;     // Manufacturer Vendor Id (See MoCA 1.1) [we only use 16 lsbs]
    SYS_UINT32  mfrHwVer;        // Manufacturer Hardware Version (vendor specific)
    SYS_UINT32  mfrSwVer;        // Manufacturer Software Version (vendor specific)
    SYS_UINT32  personality;     // Manufacturer node personality
#endif /* FEATURE_PUPQ_NMS_CONF */

#if FEATURE_IQM
    SYS_UINT32      iqmDebugMask1;
    SYS_UINT32      iqmDebugMask2;
    SYS_UINT32      iqmDebugMask3;
#endif
    SYS_UINT32  dbgMask;
#if FEATURE_FEIC_PWR_CAL
    SYS_UINT32  feicProfileId;
#endif
}
Clnk_ETH_Firmware_t;

// List Entry Structure
// This defines the structure for an entry in the linked list.
typedef struct ListEntry_t
{
    struct ListEntry_t* pNext;
    struct ListEntry_t* pPrev;
}
ListEntry_t;

// List Header Structure
// This defines the header structure for the linked list.
typedef struct ListHeader_t
{
    ListEntry_t* pHead;
    ListEntry_t* pTail;
    SYS_UINT32   numElements;
}
ListHeader_t;


// { StaticUsageCount: AH=4 AT=12 RH=15 RT=2 RN=2 MH=1 }
#define COMMON_LIST_INIT(list)      (list)->pHead = SYS_NULL; \
                             (list)->pTail = SYS_NULL;        \
                             (list)->numElements = 0
#define COMMON_LIST_SIZE(list)      ((list)->numElements)
#define COMMON_LIST_EMPTY(list)     ((list)->pHead == SYS_NULL)
#define COMMON_LIST_HEAD(T,L)       ((T *) ((L)->pHead))
#define COMMON_LIST_TAIL(T,L)       ((T *) ((L)->pTail))
#define COMMON_LIST_NEXT(T,E)       ((T *) (((ListEntry_t *) (E))->pNext))
#define COMMON_LIST_PREV(T,E)       ((T *) (((ListEntry_t *) (E))->pPrev))
#define COMMON_LIST_ADD_HEAD(list, entry)                     \
            ((ListEntry_t *)(entry))->pNext = (list)->pHead;  \
            ((ListEntry_t *)(entry))->pPrev = SYS_NULL;       \
            if (!COMMON_LIST_EMPTY(list))                     \
            {                                                 \
                ((ListEntry_t *)(entry))->pNext->pPrev =      \
                    (ListEntry_t *)(entry);                   \
            }                                                 \
            else                                              \
            {                                                 \
                (list)->pTail = (ListEntry_t *)(entry);       \
            }                                                 \
            (list)->pHead = (ListEntry_t *)(entry);           \
            (list)->numElements++
#define COMMON_LIST_ADD_TAIL(list, entry)                     \
            ((ListEntry_t *)(entry))->pNext = SYS_NULL;       \
            ((ListEntry_t *)(entry))->pPrev = (list)->pTail;  \
            if (!COMMON_LIST_EMPTY(list))                     \
            {                                                 \
                ((ListEntry_t *)(entry))->pPrev->pNext =      \
                    (ListEntry_t *)(entry);                   \
            }                                                 \
            else                                              \
            {                                                 \
                (list)->pHead = (ListEntry_t *)(entry);       \
            }                                                 \
            (list)->pTail = (ListEntry_t *)(entry);           \
            (list)->numElements++
#define COMMON_LIST_REM_HEAD(list)                            \
            (list)->pHead;                                    \
            if (!COMMON_LIST_EMPTY(list))                     \
            {                                                 \
                if ((list)->pHead == (list)->pTail)           \
                {                                             \
                    (list)->pHead = SYS_NULL;                 \
                    (list)->pTail = SYS_NULL;                 \
                }                                             \
                else                                          \
                {                                             \
                    (list)->pHead = (list)->pHead->pNext;     \
                    (list)->pHead->pPrev = SYS_NULL;          \
                }                                             \
                (list)->numElements--;                        \
            }
#define COMMON_LIST_REM_TAIL(list)                            \
            (list)->pTail;                                    \
            if (!COMMON_LIST_EMPTY(list))                     \
            {                                                 \
                if ((list)->pHead == (list)->pTail)           \
                {                                             \
                    (list)->pHead = SYS_NULL;                 \
                    (list)->pTail = SYS_NULL;                 \
                }                                             \
                else                                          \
                {                                             \
                    (list)->pTail = (list)->pTail->pPrev;     \
                    (list)->pTail->pNext = SYS_NULL;          \
                }                                             \
                (list)->numElements--;                        \
            }
#define COMMON_LIST_MOVE_TO_HEAD(list,entry)                  \
            if (!COMMON_LIST_EMPTY(list) &&                   \
                ((ListEntry_t*)(entry) != (list)->pHead))     \
            {                                                 \
                ListEntry_t *pCurr = (ListEntry_t *)(entry);  \
                ListEntry_t *pNext = pCurr->pNext;            \
                ListEntry_t *pPrev = pCurr->pPrev;            \
                /* Remove Node */                             \
                if (pNext) {                                  \
                    pNext->pPrev = pPrev;                     \
                } else {                                      \
                    (list)->pTail = pPrev;                    \
                }                                             \
                pPrev->pNext = pNext;                         \
                /* Add to Head */                             \
                pNext = (list)->pHead;                        \
                pCurr->pNext = pNext;                         \
                pNext->pPrev = pCurr;                         \
                (list)->pHead = pCurr;                        \
            }
#define COMMON_LIST_REM_NODE(list,entry)                      \
            if (!COMMON_LIST_EMPTY(list))                     \
            {                                                 \
                ListEntry_t *pCurr = (ListEntry_t*)entry;     \
                if ((list)->pHead == (list)->pTail)           \
                {                                             \
                    (list)->pHead = SYS_NULL;                 \
                    (list)->pTail = SYS_NULL;                 \
                }                                             \
                else if ((list)->pHead == pCurr)              \
                {                                             \
                    (list)->pHead = (list)->pHead->pNext;     \
                    (list)->pHead->pPrev = SYS_NULL;          \
                }                                             \
                else if ((list)->pTail == pCurr)              \
                {                                             \
                    (list)->pTail = (list)->pTail->pPrev;     \
                    (list)->pTail->pNext = SYS_NULL;          \
                }                                             \
                else                                          \
                {                                             \
                    ListEntry_t *tPrev,*tNext;                \
                    tPrev = pCurr->pPrev;                     \
                    tNext = pCurr->pNext;                     \
                    tNext->pPrev = pCurr->pPrev;              \
                    tPrev->pNext = pCurr->pNext;              \
                }                                             \
                (list)->numElements--;                        \
            }

/* end *************** original common.h defines ************************/

/******************* ClnkEth_Vlan.h definitions *************************/
// used in hw_z2_pci.c
#define CLNK_ETH_GET_DST_MAC_HI(buf)                                  \
            ((((Clnk_ETH_EthHdr_t *)(buf))->dstMacAddr[0] << 24) |    \
             (((Clnk_ETH_EthHdr_t *)(buf))->dstMacAddr[1] << 16) |    \
             (((Clnk_ETH_EthHdr_t *)(buf))->dstMacAddr[2] << 8)  |    \
             (((Clnk_ETH_EthHdr_t *)(buf))->dstMacAddr[3]))

#define CLNK_ETH_GET_DST_MAC_LO(buf)                                  \
            ((((Clnk_ETH_EthHdr_t *)(buf))->dstMacAddr[4] << 24) |    \
             (((Clnk_ETH_EthHdr_t *)(buf))->dstMacAddr[5] << 16))

#define CLNK_ETH_GET_SRC_MAC_HI(buf)                                  \
            ((((Clnk_ETH_EthHdr_t *)(buf))->srcMacAddr[0] << 24) |    \
             (((Clnk_ETH_EthHdr_t *)(buf))->srcMacAddr[1] << 16) |    \
             (((Clnk_ETH_EthHdr_t *)(buf))->srcMacAddr[2] << 8)  |    \
             (((Clnk_ETH_EthHdr_t *)(buf))->srcMacAddr[3]))

#define CLNK_ETH_GET_SRC_MAC_LO(buf)                                  \
            ((((Clnk_ETH_EthHdr_t *)(buf))->srcMacAddr[4] << 24) |    \
             (((Clnk_ETH_EthHdr_t *)(buf))->srcMacAddr[5] << 16))

#define CLNK_ETH_GET_VLAN_PRIORITY_FROM_BUF(buf)                      \
            ((((Clnk_ETH_EthHdr_t *)(buf))->lenType[0] == 0x81) ?     \
             (((Clnk_ETH_EthHdr_t *)(buf))->tagCtrlInfo[0] >> 5) : 0)

#define IS_CLNK_ETH_PKT_TAGGED(buf)                                   \
	    ((((Clnk_ETH_EthHdr_t *)(buf))->lenType[0] == 0x81)  &&   \
             (((Clnk_ETH_EthHdr_t *)(buf))->lenType[1] == 0x00)  ?    \
              SYS_TRUE : SYS_FALSE)

/// Ethernet Header
typedef struct
{
    SYS_UINT8  dstMacAddr[6];
    SYS_UINT8  srcMacAddr[6];
    SYS_UINT8  lenType[2];
    SYS_UINT8  tagCtrlInfo[2];
}
Clnk_ETH_EthHdr_t;
/* end ********* ClnkEth_Vlan.h definitions *****************/

/************ clnkdvrapi.h definitions **********************/
// driver/API structure definitions
#define DRV_CLNK_CTL             55
typedef struct
{
    SYS_UINT32 cmd;
    void*      param1;
    void*      param2;
    void*      param3;
}
IfrDataStruct;

#define  MAX_MOCAPASSWORD_LEN 17
#define  MAX_MOCAPASSWORD_LEN_PADDED                                       \
            (MAX_MOCAPASSWORD_LEN + sizeof(SYS_UINT32) - 1) /              \
            sizeof(SYS_UINT32) * sizeof(SYS_UINT32)

typedef union
{
    SYS_UINT8 value[MAX_MOCAPASSWORD_LEN_PADDED];
    SYS_UINT32 _force_alignment;
} clnk_nms_mocapassword_t;

struct clnk_soc_opt
{
    /* copied from conf file */
    SYS_UINT32      CMRatio;
    SYS_UINT32      DistanceMode;
    SYS_UINT32      TxPower;
    SYS_UINT32      phyMargin;
    SYS_UINT32      phyMBitMask;
    SYS_UINT32      SwConfig;
#if defined(L3_DONGLE_HIRF)
    SYS_UINT32	    Diplexer;
#endif        
    SYS_UINT32      channelPlan;
    SYS_UINT32      scanMask;
    SYS_UINT32      productMask;
    SYS_UINT32      tabooMask;
    SYS_UINT32      tabooOffset;
    SYS_UINT32      channelMask;
    SYS_UINT32      lof;
    SYS_UINT32      bias;
    SYS_UINT32      PowerCtlPhyRate;
    SYS_UINT32      BeaconPwrLevel;
    SYS_UINT32      MiiPausePriLvl;
    SYS_UINT32      PQoSClassifyMode;  /* PQoS Classification Mode */

#if FEATURE_FEIC_PWR_CAL
    SYS_UINT32      feicProfileId;
#endif

#if FEATURE_PUPQ_NMS_CONF
    SYS_UINT32      mfrVendorId;        /* we only use 16 lsbs */
    SYS_UINT32      mfrHwVer;
    SYS_UINT32      mfrSwVer;
    SYS_UINT32      personality;
#endif /* FEATURE_PUPQ_NMS_CONF */

    SYS_UINT32      dbgMask;         // debug mask passed to SW for special debug purposes

#if FEATURE_IQM
    SYS_UINT32      iqmDebugMask1;
    SYS_UINT32      iqmDebugMask2;
    SYS_UINT32      iqmDebugMask3;
#endif

    SYS_UINT32      TargetPhyRate;

    /* derived from conf file */
    SYS_UINT32      pmki_lo;
    SYS_UINT32      pmki_hi;
    SYS_UINT32      mmk_lo;
    SYS_UINT32      mmk_hi;

    clnk_nms_mocapassword_t mocapassword;
};

/* end *********** clnkdvrapi.h definitions **********************/

/*************** ClnkCtl.h definitions **********************/

// ClnkEth.c dependecies below
#define MIN_VAL(x, y) (((x) > (y)) ? (y) : (x))

// eth.c dependecies below
struct clnk_io
{
    SYS_UINT32      *in;
    SYS_UINT32      in_len;
    SYS_UINT32      *out;
    SYS_UINT32      out_len;
};
typedef enum
{
    DATA_BUF_MY_NODE_INFO_CMD_TYPE      = 0,
    DATA_BUF_NETWORK_NODE_INFO_CMD_TYPE = 1,
    DATA_BUF_NODE_PHY_DATA_CMD_TYPE     = 2,
    DATA_BUF_TEST_PORT_CMD_TYPE         = 3,
    DATA_BUF_RFIC_TUNING_DATA_CMD_TYPE  = 4,
    DATA_BUF_RX_ERR_DATA_CMD_TYPE       = 5,
    DATA_BUF_PRIV_INFO                  = 6,
    DATA_BUF_PRIV_STATS                 = 7,
    DATA_BUF_PRIV_NODE_INFO             = 8,
    DATA_BUF_DUMP_CAM                   = 9,
    DATA_BUF_GET_MIXED_MODE_ACTIVE      = 10, // obsolete
    DATA_BUF_SET_MIXED_MODE_ACTIVE      = 11, // obsolete
    DATA_BUF_GET_PEER_RATES             = 12,
    DATA_BUF_GET_DYN_PARAMS             = 13,
    DATA_BUF_SET_DYN_PARAMS             = 14,
    DATA_BUF_GET_AGGR_STATS             = 15,
    DATA_BUF_SEND_GCAP                  = 16,
    DATA_BUF_obsolete                   = 17, 
    DATA_BUF_GET_EPHY_STATS             = 18,

#if FEATURE_QOS
    DATA_BUF_CREATE_FLOW                = 19,
    DATA_BUF_UPDATE_FLOW                = 20,
    DATA_BUF_DELETE_FLOW                = 21,
    DATA_BUF_QUERY_INGRESS_FLOW         = 22,
    DATA_BUF_LIST_INGRESS_FLOWS         = 23,
    DATA_BUF_QUERY_NODES                = 24,
    DATA_BUF_QUERY_INTERFACE_CAPS       = 26,
    DATA_BUF_GET_EVENT_COUNTS           = 27,
#endif

#if FEATURE_ECLAIR
    DATA_BUF_ECLAIR_GET_HINFO          ,
    DATA_BUF_ECLAIR_SET_TWEAKABLE      ,
    DATA_BUF_ECLAIR_RESET_TEST_CONTEXT ,

#if NEVER_USE_AGAIN_ECLAIR_LEGACY_TESTING
    DATA_BUF_ECLAIR_PUSH_COMMAND       ,
    DATA_BUF_ECLAIR_GET_COMMAND        ,
    DATA_BUF_ECLAIR_SET_RECEIPT        ,
    DATA_BUF_ECLAIR_PULL_RECEIPT       ,
#endif /* NEVER_USE_AGAIN_ECLAIR_LEGACY_TESTING */


#if FEATURE_ECLAIR_WHITE_BOX_TEST
    DATA_BUF_ECLAIR_MORPH_CFG          ,
    DATA_BUF_ECLAIR_MORPH_EXEC         ,
    DATA_BUF_ECLAIR_MULTINODE_QUERY    ,
    DATA_BUF_ECLAIR_MULTINODE_COMMIT   ,
#endif

#if ECFG_FLAVOR_VALIDATION==1
    DATA_BUF_VAL_GET_MBOX_HOST_COUNTS  , 
    DATA_BUF_VAL_GET_MBOX_CCPU_COUNTS  , 
    DATA_BUF_VAL_TRIGGER_MBOX_EVENT    ,   
#endif

#endif

    MAX_DATA_BUF_TYPE // This must always be last
} DATA_BUF_CMD_TYPE;

#define CLNK_CTL_VERSION                    0x01

#define CLNK_CTL_MAX_IN_LEN                 0x2000
#define CLNK_CTL_MAX_OUT_LEN                0x2000

#define CLNK_CTL_SOC_CMD                    0x00000000
#define CLNK_CTL_ETH_CMD                    0x00000100
#define CLNK_CTL_DRV_CMD                    0x00000200

#define CLNK_CMD_DST_MASK                   0x00000f00

#define CLNK_CMD_BYTE_MASK                 (0x000000ff)

#define CLNK_CTL_FOR_DRV(x) ((((x) & CLNK_CMD_DST_MASK) == CLNK_CTL_DRV_CMD) ? 1 : 0)
#define CLNK_CTL_FOR_SOC(x) ((((x) & CLNK_CMD_DST_MASK) == CLNK_CTL_SOC_CMD) ? 1 : 0)
#define CLNK_CTL_FOR_ETH(x) ((((x) & CLNK_CMD_DST_MASK) == CLNK_CTL_ETH_CMD) ? 1 : 0)

#define CLNK_CTL_GET_MY_NODE_INFO           (CLNK_CTL_SOC_CMD | DATA_BUF_MY_NODE_INFO_CMD_TYPE)
#define CLNK_CTL_GET_NET_NODE_INFO          (CLNK_CTL_SOC_CMD | DATA_BUF_NETWORK_NODE_INFO_CMD_TYPE)
#define CLNK_CTL_GET_PHY_DATA               (CLNK_CTL_SOC_CMD | DATA_BUF_NODE_PHY_DATA_CMD_TYPE)
#define CLNK_CTL_GET_RFIC_TUNING_DATA       (CLNK_CTL_SOC_CMD | DATA_BUF_RFIC_TUNING_DATA_CMD_TYPE)
#define CLNK_CTL_GET_RX_ERR_DATA            (CLNK_CTL_SOC_CMD | DATA_BUF_RX_ERR_DATA_CMD_TYPE)
#define CLNK_CTL_GET_PRIV_INFO              (CLNK_CTL_SOC_CMD | DATA_BUF_PRIV_INFO)
#define CLNK_CTL_GET_PRIV_STATS             (CLNK_CTL_SOC_CMD | DATA_BUF_PRIV_STATS)
#define CLNK_CTL_GET_PRIV_NODE_INFO         (CLNK_CTL_SOC_CMD | DATA_BUF_PRIV_NODE_INFO)
#define CLNK_CTL_GET_CAM                    (CLNK_CTL_SOC_CMD | DATA_BUF_DUMP_CAM)
#define CLNK_CTL_GET_PEER_RATES             (CLNK_CTL_SOC_CMD | DATA_BUF_GET_PEER_RATES)
#define CLNK_CTL_GET_DYN_PARAMS             (CLNK_CTL_SOC_CMD | DATA_BUF_GET_DYN_PARAMS)
#define CLNK_CTL_SET_DYN_PARAMS             (CLNK_CTL_SOC_CMD | DATA_BUF_SET_DYN_PARAMS)
#define CLNK_CTL_GET_MIXED_MODE_ACTIVE      (CLNK_CTL_SOC_CMD | DATA_BUF_GET_MIXED_MODE_ACTIVE)
#define CLNK_CTL_SET_MIXED_MODE_ACTIVE      (CLNK_CTL_SOC_CMD | DATA_BUF_SET_MIXED_MODE_ACTIVE)
#define CLNK_CTL_GET_AGGR_STATS             (CLNK_CTL_SOC_CMD | DATA_BUF_GET_AGGR_STATS)
#define CLNK_CTL_SEND_GCAP                  (CLNK_CTL_SOC_CMD | DATA_BUF_SEND_GCAP)
#define CLNK_CTL_GET_EPHY_STATS             (CLNK_CTL_SOC_CMD | DATA_BUF_GET_EPHY_STATS)


#if FEATURE_QOS
#define CLNK_CTL_CREATE_FLOW                (CLNK_CTL_SOC_CMD | DATA_BUF_CREATE_FLOW)
#define CLNK_CTL_UPDATE_FLOW                (CLNK_CTL_SOC_CMD | DATA_BUF_UPDATE_FLOW)
#define CLNK_CTL_DELETE_FLOW                (CLNK_CTL_SOC_CMD | DATA_BUF_DELETE_FLOW)
#define CLNK_CTL_QUERY_INGRESS_FLOW         (CLNK_CTL_SOC_CMD | DATA_BUF_QUERY_INGRESS_FLOW)
#define CLNK_CTL_LIST_INGRESS_FLOWS         (CLNK_CTL_SOC_CMD | DATA_BUF_LIST_INGRESS_FLOWS)
#define CLNK_CTL_QUERY_NODES                (CLNK_CTL_SOC_CMD | DATA_BUF_QUERY_NODES)
#define CLNK_CTL_QUERY_INTERFACE_CAPS       (CLNK_CTL_SOC_CMD | DATA_BUF_QUERY_INTERFACE_CAPS)
#define CLNK_CTL_GET_EVENT_COUNTS           (CLNK_CTL_SOC_CMD | DATA_BUF_GET_EVENT_COUNTS)
#endif /* FEATURE_QOS */

#if FEATURE_ECLAIR
#define CLNK_CTL_ECLAIR_GET_HINFO           (CLNK_CTL_SOC_CMD | DATA_BUF_ECLAIR_GET_HINFO)
#define CLNK_CTL_ECLAIR_SET_TWEAKABLE       (CLNK_CTL_SOC_CMD | DATA_BUF_ECLAIR_SET_TWEAKABLE)
#define CLNK_CTL_ECLAIR_RESET_TEST_CONTEXT  (CLNK_CTL_SOC_CMD | DATA_BUF_ECLAIR_RESET_TEST_CONTEXT)

#if NEVER_USE_AGAIN_ECLAIR_LEGACY_TESTING
#define CLNK_CTL_ECLAIR_PUSH_COMMAND        (CLNK_CTL_SOC_CMD | DATA_BUF_ECLAIR_PUSH_COMMAND  | CLNK_CTL_ASYNC_CMD)
#define CLNK_CTL_ECLAIR_GET_COMMAND         (CLNK_CTL_SOC_CMD | DATA_BUF_ECLAIR_GET_COMMAND)
#define CLNK_CTL_ECLAIR_SET_RECEIPT         (CLNK_CTL_SOC_CMD | DATA_BUF_ECLAIR_SET_RECEIPT)
#define CLNK_CTL_ECLAIR_PULL_RECEIPT        (CLNK_CTL_SOC_CMD | DATA_BUF_ECLAIR_PULL_RECEIPT  | CLNK_CTL_ASYNC_CMD)
#endif /* NEVER_USE_AGAIN_ECLAIR_LEGACY_TESTING */

#endif /* FEATURE_ECLAIR */

#if ECFG_FLAVOR_VALIDATION==1
#define CLNK_CTL_VAL_GET_MBOX_CCPU_COUNTS   (CLNK_CTL_SOC_CMD | DATA_BUF_VAL_GET_MBOX_CCPU_COUNTS)
#define CLNK_CTL_VAL_TRIGGER_MBOX_EVENT     (CLNK_CTL_SOC_CMD | DATA_BUF_VAL_TRIGGER_MBOX_EVENT)
#endif

#if FEATURE_IQM
#define CLNK_CTL_GET_IQM_DATA               (CLNK_CTL_SOC_CMD | DATA_BUF_GET_IQM_DATA)
#endif

#define CLNK_CTL_GET_ZIP_INFO               (CLNK_CTL_ETH_CMD | 0x00)
#define CLNK_CTL_GET_SOC_OPT                (CLNK_CTL_ETH_CMD | 0x01)
#define CLNK_CTL_SET_SOC_OPT                (CLNK_CTL_ETH_CMD | 0x02)
#define CLNK_CTL_GET_MEM                    (CLNK_CTL_ETH_CMD | 0x03)
#define CLNK_CTL_SET_MEM                    (CLNK_CTL_ETH_CMD | 0x04)
#define CLNK_CTL_GET_TRACEBUF               (CLNK_CTL_ETH_CMD | 0x05)
#define CLNK_CTL_GET_ETH_STATS              (CLNK_CTL_ETH_CMD | 0x06)
#define CLNK_CTL_GET_BRIDGE_TABLE           (CLNK_CTL_ETH_CMD | 0x07)
#define CLNK_CTL_SET_ETH_FIFO_SIZE          (CLNK_CTL_ETH_CMD | 0x08)
#define CLNK_CTL_GET_EVM_DATA               (CLNK_CTL_ETH_CMD | 0x09)
#define CLNK_CTL_GET_EPP_DATA               (CLNK_CTL_ETH_CMD | 0x0a)
#define CLNK_CTL_SET_EPP_DATA               (CLNK_CTL_ETH_CMD | 0x0b) // OBSOLETE
#define CLNK_CTL_SET_DBG_MASK_DYN           (CLNK_CTL_ETH_CMD | 0x0c)

#if ECFG_FLAVOR_VALIDATION==1
#define CLNK_CTL_VAL_GET_MBOX_HOST_COUNTS   (CLNK_CTL_ETH_CMD | 0x0d)
#endif 
#if FEATURE_FEIC_PWR_CAL
#define CLNK_CTL_GET_FEIC_STATUS            (CLNK_CTL_ETH_CMD | 0x0e)
#endif

#define CLNK_CTL_RESET_DEVICE               (CLNK_CTL_DRV_CMD | 0x00)
#define CLNK_CTL_STOP_DEVICE                (CLNK_CTL_DRV_CMD | 0x01)
#define CLNK_CTL_NET_CARRIER_ON             (CLNK_CTL_DRV_CMD | 0x02) 
#define CLNK_CTL_NET_CARRIER_OFF            (CLNK_CTL_DRV_CMD | 0x03) 
#define CLNK_CTL_NET_CARRIER_OK             (CLNK_CTL_DRV_CMD | 0x04) 
#define CLNK_CTL_SOC_INIT_BUS               (CLNK_CTL_DRV_CMD | 0x05) 
#define CLNK_CTL_SOC_BOOTED                 (CLNK_CTL_DRV_CMD | 0x06) 
#define CLNK_CTL_HW_DESC_INIT               (CLNK_CTL_DRV_CMD | 0x07) 
#define CLNK_CTL_TC_DIC_INIT                (CLNK_CTL_DRV_CMD | 0x08) 
#define CLNK_CTL_GET_SOC_STATUS             (CLNK_CTL_DRV_CMD | 0x09) 
#define CLNK_CTL_GET_LINK_STATUS            (CLNK_CTL_DRV_CMD | 0x0a) 
#define CLNK_CTL_SET_MAC_ADDRESS            (CLNK_CTL_DRV_CMD | 0x0b) 
#define CLNK_CTL_GET_NMS_LOCAL_MSG          (CLNK_CTL_DRV_CMD | 0x0c) /* NMS */
#define CLNK_CTL_SET_DATA_PLANE_VARS        (CLNK_CTL_DRV_CMD | 0x0d) /* PQoS */

/* end *************** ClnkCtl.h definitions **********************/

// Driver Return Codes
typedef enum
{
    CLNK_ETH_RET_CODE_SUCCESS            = 0,
    CLNK_ETH_RET_CODE_GEN_ERR            = 1,
    CLNK_ETH_RET_CODE_MEM_ALLOC_ERR      = 2,
    CLNK_ETH_RET_CODE_RESET_ERR          = 3,
    CLNK_ETH_RET_CODE_NOT_OPEN_ERR       = 4,
    CLNK_ETH_RET_CODE_LINK_DOWN_ERR      = 5,
    CLNK_ETH_RET_CODE_NO_HOST_DESC_ERR   = 6,
    CLNK_ETH_RET_CODE_UCAST_FLOOD_ERR    = 7,
    CLNK_ETH_NO_KEY_ERROR                = 8,
    CLNK_ETH_MRT_TRANSACTION_IN_PROGRESS = 9,
    CLNK_ETH_RET_CODE_PKT_LEN_ERR        = 10,

    CLNK_ETH_RET_CODE_MAX  // This must always be last
}
CLNK_ETH_RET_CODES;




#endif /* __common_dvr_h__ */
