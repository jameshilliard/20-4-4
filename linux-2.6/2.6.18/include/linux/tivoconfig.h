#ifndef _LINUX_TIVOCONFIG_H
#define _LINUX_TIVOCONFIG_H

/**
 * @file tivoconfig.h
 *
 * \#include <linux/tivoconfig.h>
 *
 * TiVo configuration registry
 *
 * Copyright (C) 2010 TiVo Inc. All Rights Reserved.
 */

#include <linux/config.h>

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


/**
 * Selectors for the available configuration parameters
 * 
 * To create a new 32-bit selector, use:
 * <pre>
 *   echo -n BORD | od -t x1
 * </pre>
 * This is to avoid multi-char literals, which can have compiler-specific
 * byte ordering
 */
enum TivoConfigSelectors {

    /** 'BORD' Platform/PCB identifier. See ::TivoConfigBoardID below */
    kTivoConfigBoardID             = 0x424F5244,

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

    /** 'GSUR' Graphics surface memory, in bytes */
    kTivoConfigGraphicsSurfacePoolSize = 0x47535552,

};

#define MAX_CONTIGMEM_REGION 9

/**
 * 'BORD' Platform/PCB identifier
 *
 * <pre>
 *  MSB  8 bits identify OEM platform vendor (0x00 == TiVo)
 *      16 bits generally OEM vendor-specific, but should represent an
 *              enumeration of system config types
 *  LSB  8 bits hardware board revision (PCB) ID, if present </pre>
 */
enum TivoConfigBoardID {

    // Gen07 definitions
    kTivoConfigBoardIDGen07Base    = 0x00106000,     

    // Gen07 Neutron
    kTivoConfigBoardIDNeutronBase  = 0x00106000, 
    kTivoConfigBoardIDNeutron      = 0x00106000,   
    kTivoConfigBoardIDNeutronMax   = 0x00106F00, 

    // Gen07 Lego
    kTivoConfigBoardIDLegoBase     = 0x00108000, 
    kTivoConfigBoardIDLego         = 0x00108000, // Lego P0-1
    kTivoConfigBoardIDLegoP0_2     = 0x00108100, // Lego P0-2
    kTivoConfigBoardIDLegoMax      = 0x00108F00,
   
    // Gen07 Helium
    kTivoConfigBoardIDHeliumBase   = 0x00109000, 
    kTivoConfigBoardIDHelium       = 0x00109000, 
    kTivoConfigBoardIDHeliumMax    = 0x00109F00, 

    // End of Gen07 definitions
    kTivoConfigBoardIDGen07Max     = 0x00109F00, 

    // Int08
    kTivoConfigBoardIDNeoBase      = 0x00500000, /**< Base for Neo boards */
    kTivoConfigBoardIDNeoPX        = 0x00500100, /**< Neo PX; SharkRiver */
    kTivoConfigBoardIDNeoPY1       = 0x00500200, /**< Neo PY-1: XYZ P0 */
    kTivoConfigBoardIDNeoPY2       = 0x00500300, /**< Neo PY-2: XYZ P0.1 */
    kTivoConfigBoardIDNeoP0        = 0x00500400, /**< Neo P0 */
    kTivoConfigBoardIDNeoMax       = 0x00500FFF, /**< Neo Max */
    kTivoConfigBoardIDMorpheusBase = 0x00501000, /**< Base for Morpheus Boards */
    kTivoConfigBoardIDMorpheusPY   = 0x00501100, /**< Morpheus PY (likely P0) */
    kTivoConfigBoardIDMorpheusMax  = 0x00501FFF, /**< Morpheus Max */

    // Koherence definitions
    kTivoConfigBoardIDKoherenceBase = 0x06000000, /** Koherence Base (Mike Minakami) */
    kTivoConfigBoardIDLeoBase      = 0x06000000, /* TiVo Leo */
    kTivoConfigBoardIDLeo          = 0x06000000, 
    kTivoConfigBoardIDLeoMax       = 0x06000F00, 
    kTivoConfigBoardIDCerberusBase = 0x06001000, /* TiVo 3tuner Neutron */
    kTivoConfigBoardIDCerberus     = 0x06001000, 
    kTivoConfigBoardIDCerberusMax  = 0x06001F00, 
    kTivoConfigBoardIDDuploBase    = 0x06002000, /* Tiny TiVo */
    kTivoConfigBoardIDDuploPX      = 0x06002000,
    kTivoConfigBoardIDDuplo        = 0x06002100,
    kTivoConfigBoardIDDuploMax     = 0x06002800,
    kTivoConfigBoardIDScyllaBase   = 0x06003800, /* Hexatuner Helium */
    kTivoConfigBoardIDScylla       = 0x06003800,
    kTivoConfigBoardIDScyllaMax    = 0x06003F00,
    kTivoConfigBoardIDKoherenceMax = 0x06FFFFFF, /*  End of Koherence*/
    
    kTivoConfigBoardIDThomsonBase  = 0x07000000, /**< 0x060000 Start of Thomson range */
    kTivoConfigBoardIDMojave       = 0x07000100, /**< 0x060001 Thomson HR22 */
    kTivoConfigBoardIDKontiki      = 0x07000200, /**< 0x060002 Technicolor Kontiki */
    kTivoConfigBoardIDThomsonMax   = 0x07FFFFFF, /**< 0x06FFFF End of Thomson range */
    
//    kTivoConfigBoardIDUnknown128M  = 0xFFFFFC00, /**< 0xFFFFFC Unknown Platform with 128MB DRAM */
//    kTivoConfigBoardIDUnknown64M   = 0xFFFFFD00, /**< 0xFFFFFD Unknown Platform with 64MB DRAM */
//    kTivoConfigBoardIDUnknown32M   = 0xFFFFFE00, /**< 0xFFFFFE Unknown Platform with 32MB DRAM */
//    kTivoConfigBoardIDUnknown      = 0xFFFFFF00, /**< 0xFFFFFF Unknown Platform */
};

#define IS_BOARD(VAL, NM) (((VAL) & 0xFFFFFF00) >= (TivoConfigValue) kTivoConfigBoardID##NM##Base && ((VAL) & 0xFFFFFF00) <= (TivoConfigValue) kTivoConfigBoardID##NM##Max)

// Gen07 'virtual' hardware options in LSB of BORD
#define kTivoConfigGen07Strap_Not7405   1 // set if not a 7405 processor
#define kTivoConfigGen07Strap_NoSATA    2 // set if SATA hardware is not present (7414)
#define kTivoConfigGen07Strap_IsBridge  4 // set if board has AR8328 switch on EMAC1 (Helium)
#define kTivoConfigGen07Strap_HasMoCA   8 // set if board has MoCA modem 
#define kTivoConfigGen07Strap_HasISSD  16 // set if board has iSSD drive

#endif /* _LINUX_TIVOCONFIG_H */
