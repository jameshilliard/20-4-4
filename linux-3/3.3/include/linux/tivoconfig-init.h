// Copyright 2013 TiVo Inc. All Rights Reserved.
#ifndef _LINUX_TIVOCONFIG_INIT_H
#define _LINUX_TIVOCONFIG_INIT_H

typedef struct {
	TivoConfigSelector selector;
	TivoConfigValue value;
} TivoConfig;

/* Gen08 Leo P1 */
static TivoConfig gLeoConfigTable[] __initdata = { 
    { kTivoConfigBoardID, kTivoConfigBoardIDLeo8P1 },
    { kTivoConfigMemSize, 0x1ffff000 },             // 512 MB, minus one page WAR
    { kTivoConfigContigmemRegion2, 0x00000000 },    // 0MB
    { kTivoConfigContigmemRegion3, 0x00527000 },    // 5280kB for ?
    { kTivoConfigGraphicsSurfacePoolSize, 0x04C00000 }, // 76.0 MiB 
    // use bmem from broadcom drivers
    { kTivoConfigBmem0Size, 0x04200000 },   // First bmem region: Main nexus driver
    { kTivoConfigBmem0Address, 0x04000000 }, // bmem=66M@64M
    { kTivoConfigBmem1Size, 0x07E00000 },   // Second bmem region
    { kTivoConfigBmem1Address, 0x20000000 }, // bmem=126M@512M
    { 0,0 }
};

/* Gen08 Leo3 */
static TivoConfig gLeo3ConfigTable[] __initdata = { 
    { kTivoConfigBoardID, kTivoConfigBoardIDLeo3 },
    { kTivoConfigMemSize, 0x1ffff000 },             // 512 MB, minus one page WAR
    { kTivoConfigContigmemRegion2, 0x00000000 },    // 0MB
    { kTivoConfigContigmemRegion3, 0x00527000 },    // 5280kB for ?
    { kTivoConfigGraphicsSurfacePoolSize, 0x04C00000 }, // 76.0 MiB 
    // use bmem from broadcom drivers
    { kTivoConfigBmem0Size, 0x04200000 },   // First bmem region: Main nexus driver
    { kTivoConfigBmem0Address, 0x04000000 }, // bmem=66M@64M
    { kTivoConfigBmem1Size, 0x07E00000 },   // Second bmem region
    { kTivoConfigBmem1Address, 0x20000000 }, // bmem=126M@512M
    { 0,0 }
};

// Initialization array for the TiVo configuration registry.
// Note a given table should either have entry for kTivoConfigBoardID, or
// entries for kTivoConfigBoardIDBase and kTivoConfigBoardIDMax, but not both.

// Generic Gen10 Titan Rev C 
// Non-overlapping memory regions are:
// BMEM1     - 512M to (512M + kTivoConfigBmem1Size)
// Contigmem - (256M - XMB) to 256M. For X look at bootup log
//             coming from drivers/char/mem.c function contigmem()
// BMEM0     - 64M  to (64M  + kTivoConfigBmem1Size)
static TivoConfig gTitanRevCConfigTable[] __initdata = {
    { kTivoConfigBoardIDBase, kTivoConfigBoardIDTitanRevCBase }, // Allow anything in TitanRevC range
    { kTivoConfigBoardIDMax, kTivoConfigBoardIDTitanRevCMax },
    { kTivoConfigMemSize, 0x3ffff000 },                          // 1 GB, minus one page WAR
    { kTivoConfigContigmemRegion2, 0x00000000 },                // 0MB
    { kTivoConfigContigmemRegion3, 0x00C00000 },                // 12MB
    { kTivoConfigGraphicsSurfacePoolSize, 0x05a00000 },          // 90.0 MiB
    { kTivoConfigBmem0Size, 0x0b3db000},                         // First bmem region: Main nexus driver
    { kTivoConfigBmem0Address, 0x04000000 },                     // bmem=(184M-PAGE_SIZE)@64M
    { kTivoConfigBmem1Size, 0x0ec00000 },                        // Second bmem region
    { kTivoConfigBmem1Address, 0x20000000 },                     // bmem=236M@512M
    { 0,0 }
};

// Generic Gen10 Argon XL
// Non-overlapping memory regions are:
// BMEM1     - 512M to (512M + kTivoConfigBmem1Size)
// Contigmem - (256M - XMB) to 256M. For X look at bootup log
//             coming from drivers/char/mem.c function contigmem()
// BMEM0     - 64M  to (64M  + kTivoConfigBmem1Size)
static TivoConfig gArgonXLConfigTable[] __initdata = {
    { kTivoConfigBoardIDBase, kTivoConfigBoardIDArgonXLBase },  // Allow anything in ArgonXL range
    { kTivoConfigBoardIDMax, kTivoConfigBoardIDArgonXLMax },
    { kTivoConfigMemSize, 0x3ffff000 },                         // 1 GB, minus one page WAR
    { kTivoConfigContigmemRegion2, 0x00000000 },                // 0MB
    { kTivoConfigContigmemRegion3, 0x00000000 },                // 0MB
    { kTivoConfigGraphicsSurfacePoolSize, 0x05a00000 },         // 90.0 MiB
    { kTivoConfigBmem0Size, 0x0a1db000},                        // First bmem region: Main nexus driver
    { kTivoConfigBmem0Address, 0x04000000 },                    // bmem=(162M-PAGE_SIZE)@64M
    { kTivoConfigBmem1Size, 0x0ec00000 },                       // Second bmem region
    { kTivoConfigBmem1Address, 0x20000000 },                    // bmem=236M@512M
    { 0,0 }
};

// Generic Gen10 Argon S
// Non-overlapping memory regions are:
// BMEM1     - 512M to (512M + kTivoConfigBmem1Size)
// Contigmem - (256M - XMB) to 256M. For X look at bootup log
//             coming from drivers/char/mem.c function contigmem()
// BMEM0     - 64M  to (64M  + kTivoConfigBmem1Size)
static TivoConfig gArgonSConfigTable[] __initdata = {
    { kTivoConfigBoardIDBase, kTivoConfigBoardIDArgonSBase },   // allow anything in ArgonS range
    { kTivoConfigBoardIDMax, kTivoConfigBoardIDArgonSMax },
    { kTivoConfigMemSize, 0x3ffff000 },                         // 1 GB, minus one page WAR
    { kTivoConfigContigmemRegion2, 0x00000000 },                // 0MB
    { kTivoConfigContigmemRegion3, 0x00000000 },                // 0kB
    { kTivoConfigGraphicsSurfacePoolSize, 0x05a00000 },         // 90.0 MiB
    { kTivoConfigBmem0Size, 0x0acb4000},                        // First bmem region: Main nexus driver
    { kTivoConfigBmem0Address, 0x04000000 },                    // bmem=(172M-PAGE_SIZE)@64M
    { kTivoConfigBmem1Size, 0x0ec00000 },                       // Second bmem region
    { kTivoConfigBmem1Address, 0x20000000 },                    // bmem=236M@512M
    { 0,0 }
};

/**
 * Pac01 Pace
 *
 * Non-overlapping memory regions are:
 * BMEM1     - 512M to (512M + kTivoConfigBmem1Size)
 * Contigmem - (256M - XMB) to 256M. For X look at bootup log
 *             coming from drivers/char/mem.c function contigmem()
 * BMEM0     - 64M  to (64M  + kTivoConfigBmem1Size)
 */
static TivoConfig gPac01ConfigTable[] __initdata = {
    { kTivoConfigBoardID, kTivoConfigBoardIDXG1 },
    { kTivoConfigMemSize, 0x3ffff000 },             // 1 GB, minus one page WAR
    { kTivoConfigContigmemRegion2, 0x00200000 },    // 2.0MB for ?
    { kTivoConfigContigmemRegion3, 0x00527000 },    // 5280kB for ?
    { kTivoConfigGraphicsSurfacePoolSize, 0x00000000 }, // 70.0 MiB
    // use bmem from broadcom drivers
    { kTivoConfigBmem0Size, 0x08000000 },   // First bmem region: Main nexus driver
    { kTivoConfigBmem0Address, 0x08000000 }, // bmem=(128M-PAGE_SIZE)@128M
    { kTivoConfigBmem1Size, 0x0C000000 },   // Second bmem region
    { kTivoConfigBmem1Address, 0x20000000 }, // bmem=192M@512M
    { 0,0 }
};

/**
 * Pac02 Pace
 *
 * Non-overlapping memory regions are:
 * BMEM1     - 512M to (512M + kTivoConfigBmem1Size)
 * Contigmem - (256M - XMB) to 256M. For X look at bootup log
 *             coming from drivers/char/mem.c function contigmem()
 * BMEM0     - 64M  to (64M  + kTivoConfigBmem1Size)
 */
static TivoConfig gPac02ConfigTable[] __initdata = {
    { kTivoConfigBoardID, kTivoConfigBoardIDX3 },
    { kTivoConfigMemSize, 0x3ffff000 },             // 1 GB, minus one page WAR
    { kTivoConfigContigmemRegion2, 0x00000000 },    // 0MB
    { kTivoConfigContigmemRegion3, 0x00000000 },    // kB
    { kTivoConfigGraphicsSurfacePoolSize, 0x00000000 }, // 70.0 MiB
    // use bmem from broadcom drivers
    { kTivoConfigBmem0Size, 0x08800000 },   // First bmem region: Main nexus driver
    { kTivoConfigBmem0Address, 0x07800000 }, // bmem=(136M-PAGE_SIZE)@64M
    { kTivoConfigBmem1Size, 0x11200000 },   // Second bmem region
    { kTivoConfigBmem1Address, 0x20000000 }, // bmem=274M@512M (184M bmem1 + 90M GFX mem)
    { 0,0 }
};


static TivoConfig *gGlobalConfigTable[] __initdata = { 
    gLeoConfigTable,
    gLeo3ConfigTable,
    gTitanRevCConfigTable,
    gArgonXLConfigTable,
    gArgonSConfigTable,
    gPac01ConfigTable,
    gPac02ConfigTable,
    0
};


#else

#error "Nobody should ever #include <linux/tivoconfig-init.h> more than once!"

#endif /* _LINUX_TIVOCONFIG_INIT_H */
