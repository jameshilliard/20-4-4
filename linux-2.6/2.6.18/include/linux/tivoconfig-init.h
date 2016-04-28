#ifndef _LINUX_TIVOCONFIG_INIT_H
#define _LINUX_TIVOCONFIG_INIT_H

/*
 * Initialization array for the TiVo configuration registry
 *
 * Copyright (C) 2009 TiVo Inc. All Rights Reserved.
 */

typedef struct {
	TivoConfigSelector selector;
	TivoConfigValue value;
} TivoConfig;


/* Gen07 */
static TivoConfig gGen07ConfigTable[] __initdata = {
    { kTivoConfigBoardID, kTivoConfigBoardIDNeutron },
    { kTivoConfigMemSize, 0x1ffff000 }, // 512 MB, minus one page WAR
    { kTivoConfigContigmemRegion1, 0x00000000 }, /* 0MB - input section now in contigmem8 */
    { kTivoConfigContigmemRegion2, 0x00000000 }, /* 0MB - BufferedStream mempools used instead */
    { kTivoConfigContigmemRegion3, 0x00000000 }, /* 0kB */
    { kTivoConfigContigmemRegion8, 0x0B400000 }, /* 180.0 MiB */
    { kTivoConfigGraphicsSurfacePoolSize, 0x04600000 }, /* 70.0 MiB */
    { 0,0 }
};

/* Gen07Leo */
/* OBSOLETE PLATFORM, DO NOT USE */
static TivoConfig gLeoConfigTable[] __initdata = {
    { kTivoConfigBoardID, kTivoConfigBoardIDLeo },
    { kTivoConfigMemSize, 0x1ffff000 }, // 512 MB, minus one page WAR
    { kTivoConfigContigmemRegion1, 0x00000000 }, /* 0MB - input section now in contigmem8 */
    { kTivoConfigContigmemRegion2, 0x00000000 }, /* 0MB - BufferedStream mempools used instead */
    { kTivoConfigContigmemRegion3, 0x00527000 }, /* 5280kB */
    { kTivoConfigContigmemRegion8, 0x0A000000 }, /* 160.0MB */
    { kTivoConfigGraphicsSurfacePoolSize, 0x03C00000 }, /* 60.0 MiB */
    { 0,0 }
};

/* Gen07Cerberus */
static TivoConfig gCerberusConfigTable[] __initdata = {
    { kTivoConfigBoardID, kTivoConfigBoardIDCerberus },
    { kTivoConfigMemSize, 0x1ffff000 }, // 512 MB, minus one page WAR
    { kTivoConfigContigmemRegion1, 0x00000000 }, /* 0MB - input section now in contigmem8 */
    { kTivoConfigContigmemRegion2, 0x00000000 }, /* 0MB - BufferedStream mempools used instead */
    { kTivoConfigContigmemRegion3, 0x00527000 }, /* 5280kB */
    { kTivoConfigContigmemRegion8, 0x0C000000 }, /* 192.0MB */
    { kTivoConfigGraphicsSurfacePoolSize, 0x04600000 }, /* 70.0 MiB */
    { 0,0 }
};

/* Gen07 Scylla (H6) */
static TivoConfig gScyllaConfigTable[] __initdata = {
    { kTivoConfigBoardID, kTivoConfigBoardIDScylla },
    { kTivoConfigMemSize, 0x1ffff000 }, // 512 MB, minus one page WAR
    { kTivoConfigContigmemRegion1, 0x00000000 }, /* 0MB - input section now in contigmem8 */
    { kTivoConfigContigmemRegion2, 0x00000000 }, /* 0MB - BufferedStream mempools used instead */
    { kTivoConfigContigmemRegion3, 0x00527000 }, /* 5280kB */
    { kTivoConfigContigmemRegion8, 0x0D200000 }, /* 210.0MB */
    { kTivoConfigGraphicsSurfacePoolSize, 0x04600000 }, /* 70.0 MiB */
    { 0,0 }
};

static TivoConfig gDuploPXConfigTable[] __initdata = {
    { kTivoConfigBoardID, kTivoConfigBoardIDDuploPX },
    { kTivoConfigMemSize, 0x1ffff000 }, // 512 MB, minus one page WAR
    { kTivoConfigContigmemRegion1, 0x00000000 }, /* 0MB - input section now in contigmem8 */
    { kTivoConfigContigmemRegion2, 0x00000000 }, /* 0MB - BufferedStream mempools used instead */
    { kTivoConfigContigmemRegion3, 0x00527000 }, /* 5280kB */
    { kTivoConfigContigmemRegion8, 0x0BA00000 }, /* 186.0 MiB */
    { kTivoConfigGraphicsSurfacePoolSize, 0x04600000 }, /* 70.0 MiB */
    { 0,0 }
};

/* Gen07 Lego Helium */
static TivoConfig gHeliumConfigTable[] __initdata = {
    { kTivoConfigBoardID, kTivoConfigBoardIDHelium },
    { kTivoConfigMemSize, 0x1ffff000 }, // 512 MB, minus one page WAR
    { kTivoConfigContigmemRegion1, 0x00000000 }, /* 0MB - input section now in contigmem8 */
    { kTivoConfigContigmemRegion2, 0x00000000 }, /* 0MB - BufferedStream mempools used instead */
    { kTivoConfigContigmemRegion3, 0x00000000 }, /* 0kB */
    { kTivoConfigContigmemRegion8, 0x0BA00000 }, /* 186.0MB */
    { kTivoConfigGraphicsSurfacePoolSize, 0x04600000 }, /* 70.0 MiB */
    { 0,0 }
};

/* Gen07 Lego Lego (Thin Client) */
static TivoConfig gLegoConfigTable[] __initdata = {
    { kTivoConfigBoardID, kTivoConfigBoardIDLego },
    { kTivoConfigMemSize, 0x1ffff000 }, // 512 MB, minus one page WAR
    { kTivoConfigContigmemRegion1, 0x00000000 }, /* 0MB - input section now in contigmem8 */
    { kTivoConfigContigmemRegion2, 0x00000000 }, /* 0MB - BufferedStream mempools used instead */
    { kTivoConfigContigmemRegion3, 0x00527000 }, /* 5280kB */
    { kTivoConfigContigmemRegion8, 0x0B400000 }, /* 180.0MB */
    { kTivoConfigGraphicsSurfacePoolSize, 0x04600000 }, /* 70.0 MiB */
    { 0,0 }
};

/* Gen07 Lego Lego (Thin Client) */
static TivoConfig gLegoP0_2ConfigTable[] __initdata = {
    { kTivoConfigBoardID, kTivoConfigBoardIDLegoP0_2 },
    { kTivoConfigMemSize, 0x1ffff000 }, // 512 MB, minus one page WAR
    { kTivoConfigContigmemRegion1, 0x00000000 }, /* 0MB - input section now in contigmem8 */
    { kTivoConfigContigmemRegion2, 0x00000000 }, /* 0MB - BufferedStream mempools used instead */
    { kTivoConfigContigmemRegion3, 0x00527000 }, /* 5280kB */
    { kTivoConfigContigmemRegion8, 0x0B400000 }, /* 180.0MB */
    { kTivoConfigGraphicsSurfacePoolSize, 0x04600000 }, /* 70.0 MiB */
    { 0,0 }
};

/* Mojave/Thompson HR22 */
static TivoConfig gMojaveConfigTable[] __initdata = {
    { kTivoConfigBoardID, kTivoConfigBoardIDMojave },
    { kTivoConfigMemSize, 0x0ffff000 }, // 256 MB, minus one page WAR
    { kTivoConfigContigmemRegion1, 0x01800000 }, /* 24.0MB */
    { kTivoConfigContigmemRegion2, 0x00200000 }, /* 2.0MB */
    { kTivoConfigContigmemRegion3, 0x00527000 }, /* 5280kB */
    { kTivoConfigContigmemRegion4, 0x00120000 }, /* 1152KB */
    { kTivoConfigContigmemRegion8, 0x07000000 }, /* 112.0MB */
    { 0,0 }
};

// TODO FIXME 
/* Kontiki/Technicolor */
static TivoConfig gKontikiConfigTable[] __initdata = {
    { kTivoConfigBoardID, kTivoConfigBoardIDKontiki },
    { kTivoConfigMemSize, 0x1ffff000 }, // 512 MB, minus one page WAR
    { kTivoConfigContigmemRegion1, 0x01800000 }, /* 24.0MB */
    { kTivoConfigContigmemRegion2, 0x00200000 }, /* 2.0MB */
    { kTivoConfigContigmemRegion3, 0x00527000 }, /* 5280kB */
    { kTivoConfigContigmemRegion8, 0x0B600000 }, /* 182.0MB */
    { 0,0 }
};

static TivoConfig *gGlobalConfigTable[] __initdata = { 
    gGen07ConfigTable,
    gMojaveConfigTable,
    gKontikiConfigTable,
    gLeoConfigTable,
    gCerberusConfigTable,
    gHeliumConfigTable,
    gLegoConfigTable,
    gLegoP0_2ConfigTable,
    gScyllaConfigTable,
    gDuploPXConfigTable,
    0
};

#else

#error "Nobody should ever #include <linux/tivoconfig-init.h> more than once!"

#endif /* _LINUX_TIVOCONFIG_INIT_H */
