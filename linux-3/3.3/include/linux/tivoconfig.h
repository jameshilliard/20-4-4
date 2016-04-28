// Copyright 2013 TiVo Inc. All Rights Reserved.
#ifndef _LINUX_TIVOCONFIG_H
#define _LINUX_TIVOCONFIG_H

#define kTivoConfigMaxNumSelectors 64

typedef unsigned long TivoConfigSelector;
typedef unsigned long TivoConfigValue;
typedef int           TStatus;

#ifdef CONFIG_TIVO

#ifdef __KERNEL__

// global board ID, also accessible via GetTivoConfig(kTivoConfigBoardID)
extern unsigned int boardID;

/**
 * Initialize the configuration registry from the static array of initializers
 *
 * NOTE: Must be called early at boot time to initialize contigmem parameters.
 * Core kernel only.
 *
 * @param value the ::TivoConfigBoardID to search the initialization array
 * for, usually with low 8 bits masked off
 */
void    InitTivoConfig( TivoConfigValue value );

#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Look up a value in the configuration registry
 *
 * @return 0 if found, -1 if not found
 * @param selector the configuration parameter to look up. See ::TivoConfigSelectors
 * @param value    the looked up configuration value, if found
 */
TStatus GetTivoConfig( TivoConfigSelector selector, TivoConfigValue *value );

/**
 * Set a value in the configuration registry, creating a new parameter in
 * the registry if necessary
 *
 * @return 0 if set, -ENOMEM if out of space in the registry, -1 if not
 * supported
 * @param selector the configuration parameter to set. See ::TivoConfigSelectors
 * @param value    the configuration value to set
 */
TStatus SetTivoConfig( TivoConfigSelector selector, TivoConfigValue  value );

#ifdef __cplusplus
}
#endif

#else // stub functions for systems without a config registry

static inline TStatus GetTivoConfig( TivoConfigSelector selector, TivoConfigValue *value )
{
        return -1;
}

static inline TStatus SetTivoConfig( TivoConfigSelector selector, TivoConfigValue  value )
{
        return -1;
}

#endif

// Look, the tivoconfig database is really for use by the the kernel.  If
// you're in userspace, make your own BORD-to-whatever lookup, don't burn
// unswappable kernel memory.
enum TivoConfigSelectors {

    /** 'BORD' Platform/PCB identifier. See ::TivoConfigBoardID below */
    kTivoConfigBoardID             = 0x424F5244,

    // 'BASE' and 'BMAX' can be defined instead of 'BORD' 
    kTivoConfigBoardIDBase         = 0x42415345,
    kTivoConfigBoardIDMax          = 0x424D4158,

    /** 'MEMS' Size of system memory, in bytes */
    kTivoConfigMemSize             = 0x4D454D53,

    /** 'CMSZ' Total contigmem size, in bytes. Use specific region size where
     * possible, instead of looking at the total. Note that the total can be
     * overridden via kernel commandline param, which will add to the last
     * region present in the config table
     */
    kTivoConfigContigmemSize       = 0x434d535a,
    
    /** 'CMS0' Contigmem region 0 size */
    kTivoConfigContigmemRegion0    = 0x434d5330,

    /** 'CMS1' Contigmem region 1 size */
    kTivoConfigContigmemRegion1    = 0x434d5331,

    /** 'CMS2' Contigmem region 2 size */
    kTivoConfigContigmemRegion2    = 0x434d5332,

    /** 'CMS3' Contigmem region 3 size */
    kTivoConfigContigmemRegion3    = 0x434d5333,

    /** 'CMS4' Contigmem region 4 size */
    kTivoConfigContigmemRegion4    = 0x434d5334,

    /** 'CMS5' Contigmem region 5 size */
    kTivoConfigContigmemRegion5    = 0x434d5335,

    /** 'CMS6' Contigmem region 6 size */
    kTivoConfigContigmemRegion6    = 0x434d5336,

    /** 'CMS7' Contigmem region 7 size */
    kTivoConfigContigmemRegion7    = 0x434d5337,

    /** 'CMS8' Contigmem region 8 size */
    kTivoConfigContigmemRegion8    = 0x434d5338,

    /** 'CMS9' Contigmem region 9 size */
    kTivoConfigContigmemRegion9    = 0x434d5339,

    /** 'BM0S' bmem 0 size */
    kTivoConfigBmem0Size           = 0x424d3053,

    /** 'BM0A' bmem 0 address */
    kTivoConfigBmem0Address        = 0x424d3041,

    /** 'BM1S' bmem 1 size */
    kTivoConfigBmem1Size           = 0x424d3153,

    /** 'BM1A' bmem 1 address */
    kTivoConfigBmem1Address        = 0x424d3141,
    
    /** 'GSUR' Graphics surface memory, in bytes */
    kTivoConfigGraphicsSurfacePoolSize = 0x47535552,

};

#define MAX_CONTIGMEM_REGION 9

// BoardIDs and ranges known to *this* kernel, don't port the contents of this
// table elsewhere.
enum TivoConfigBoardID {

    // Gen08 definitions
    kTivoConfigBoardIDGen08Base    = 0x0010A000,    

    // Gen08 Leo
    kTivoConfigBoardIDLeo8Base     = 0x0010A000, // ignore those spurious '8's
    kTivoConfigBoardIDLeo8P1       = 0x0010A200, 
    kTivoConfigBoardIDLeo8Max      = 0x0010AFFF,

    // Gen08 Leo3
    kTivoConfigBoardIDLeo3Base     = 0x0010B000,  
    kTivoConfigBoardIDLeo3         = 0x0010B000, 
    kTivoConfigBoardIDLeo3Max      = 0x0010BFFF,

    // End of Gen08 definitions
    kTivoConfigBoardIDGen08Max     = 0x0010BFFF, 

    // Gen10 definitions
    kTivoConfigBoardIDGen10Base    = 0x0010C000,

    // Titan Rev C
    kTivoConfigBoardIDTitanRevCBase= 0x0010C000,
    kTivoConfigBoardIDTitanRevC    = 0x0010C000, // legacy, don't use 
    kTivoConfigBoardIDTitanRevCMax = 0x0010CFFF,

    // Argon XL
    kTivoConfigBoardIDArgonXLBase  = 0x0010D000,
    kTivoConfigBoardIDArgonXL      = 0x0010D000, // legacy
    kTivoConfigBoardIDArgonXLRevC  = 0x0010D100, // legacy
    kTivoConfigBoardIDArgonXLMax   = 0x0010DFFF,

    // Argon S
    kTivoConfigBoardIDArgonSBase   = 0x0010E000,
    kTivoConfigBoardIDArgonS       = 0x0010E000, // legacy
    kTivoConfigBoardIDArgonSP1     = 0x0010E100, // legacy
    kTivoConfigBoardIDArgonSMax    = 0x0010EFFF,

    kTivoConfigBoardIDGen10Max     = 0x0010EFFF,

    kTivoConfigBoardIDPaceBase     = 0x09000000, /**< 0x090000 Start of Pace range */
    kTivoConfigBoardIDXG1          = 0x09000100, /**< 0x090001 Pace XG1 board */
    kTivoConfigBoardIDX3           = 0x09000200, /**< 0x090002 Pace X3 board */
    kTivoConfigBoardIDPaceMax      = 0x09FFFFFF, /**< 0x09FFFF End of Pace range */
    // Add Gen11 here...
};

// Gen10 'virtual' hardware options in LSB of BORD
#define kTivoConfigGen10Strap_IsBridge  1    // set if board has AR8328 switch on EMAC0 (Argon/Titan RevC and above)
#define kTivoConfigGen10Strap_IsSwapBridge 2 // set if CPU connects to AR8328 port 6 instead of port 0

#define IS_BOARD(VAL, NM) (((VAL) & 0xFFFFFF00) >= (TivoConfigValue) kTivoConfigBoardID##NM##Base && ((VAL) & 0xFFFFFF00) <= (TivoConfigValue) kTivoConfigBoardID##NM##Max)

#endif /* _LINUX_TIVOCONFIG_H */
