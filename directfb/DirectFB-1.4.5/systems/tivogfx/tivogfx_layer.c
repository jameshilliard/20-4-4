//////////////////////////////////////////////////////////////////////
//
// File: tivogfx_layer.c
//
// Copyright 2013 TiVo Inc.  All Rights Reserved.
//
//////////////////////////////////////////////////////////////////////

// always include first
#include "tivogfx_directfb.h"

// other directfb includes
#include <core/surface_buffer.h>
#include <core/surface_pool.h>
#include <directfb_util.h>

// source module includes
#include "tivogfx_layer.h"

#include "tivogfx_system.h"
#include "tivogfx_util.h"

/******************************************************************************/

D_DEBUG_DOMAIN( tivogfx_layer, "tivogfx/layer", "TiVo Gfx layer" );

/******************************************************************************/

// for DEV only

static const DirectFBDisplayLayerBufferModeNames( dfb_buffermode_names );

static const char *
tivogfx_dfb_buffermode_name( DFBDisplayLayerBufferMode buffermode )
{
     int i = 0;

     do {
          if (buffermode == dfb_buffermode_names[i].mode)
               return dfb_buffermode_names[i].name;
     } while (dfb_buffermode_names[i++].mode != DLBM_UNKNOWN);

     return "<invalid>";
}

/******************************************************************************/

typedef struct
{
    int                     magic;
}
TiVoGfxLayerData;

/******************************************************************************/

static DFBResult
tivoGfxUpdateScreen(    TiVoGfxLayerData      * pGfxLayer,
                        const DFBRegion       * region,
                        CoreSurfaceBufferLock * lock )
{
    DFBRectangle            clip;
    CoreSurfaceAllocation * allocation;
    CoreSurface           * surface;
    TiVoGfxSystemData     * pGfxSystemData;
    DFBResult               result;

    // unused for now
    TIVOGFX_UNUSED_PARAMETER( pGfxLayer );

    clip = DFB_RECTANGLE_INIT_FROM_REGION( region );

    CORE_SURFACE_BUFFER_LOCK_ASSERT( lock );

    // note: allocation check for debug purposes only
    allocation = lock->allocation;
    CORE_SURFACE_ALLOCATION_ASSERT( allocation );

    // note: surface check for debug purposes only
    surface = allocation->surface;
    D_ASSERT( surface != NULL );

    pGfxSystemData = TvGfx_GetSystemData();
    D_MAGIC_ASSERT( pGfxSystemData, TiVoGfxSystemData );

    result = TvGfxProxy_Render( pGfxSystemData->hGfxProxy,
        lock, &clip );

    return result;
}

/******************************************************************************/

static int
tivogfxLayerDataSize( void )
{
    D_DEBUG_AT( tivogfx_layer, "%s()\n", __FUNCTION__ );

    return sizeof( TiVoGfxLayerData );
}

static int
tivogfxRegionDataSize( void )
{
    D_DEBUG_AT( tivogfx_layer, "%s()\n", __FUNCTION__ );

    return 0;
}

static DFBResult
tivogfxInitLayer(       CoreLayer                  * layer,
                        void                       * driver_data,
                        void                       * layer_data,
                        DFBDisplayLayerDescription * description,
                        DFBDisplayLayerConfig      * config,
                        DFBColorAdjustment         * adjustment )
{
    TiVoGfxLayerData   * pGfxLayer;

    D_DEBUG_AT( tivogfx_layer, "%s()\n", __FUNCTION__ );

    TIVOGFX_UNUSED_PARAMETER( layer );
    TIVOGFX_UNUSED_PARAMETER( driver_data );
    TIVOGFX_UNUSED_PARAMETER( adjustment );

    pGfxLayer = (TiVoGfxLayerData *) layer_data;
    D_MAGIC_SET( pGfxLayer, TiVoGfxLayerData );

    /* set type and capabilities */
    description->type = DLTF_GRAPHICS;
    description->caps = DLCAPS_SURFACE;

    /* set name */
    snprintf( description->name, DFB_DISPLAY_LAYER_DESC_NAME_LENGTH,
        "TiVo Gfx Primary Layer" );

    /* fill out the default configuration */
    config->flags =     DLCONF_WIDTH        |
                        DLCONF_HEIGHT       |
                        DLCONF_PIXELFORMAT  |
                        DLCONF_BUFFERMODE;

    /* Set default level, number of regions, sources and clipping regions */
    description->level        = 0;  /* Default level is 0. Negative means below and positive means above */
    description->regions      = 1;  /* We only support a single region per layer context */
    description->sources      = 2;  /* HD and SD graphics sources */
    description->clip_regions = 0;

    config->width             = TIVOGFX_HD_DISPLAY_WIDTH;
    config->height            = TIVOGFX_HD_DISPLAY_HEIGHT;
    config->pixelformat       = DSPF_ARGB;
    config->buffermode        = DLBM_FRONTONLY;
    config->options           = DLOP_OPACITY | DLOP_ALPHACHANNEL;
    config->surface_caps      = DSCAPS_PRIMARY | DSCAPS_VIDEOONLY | DSCAPS_PREMULTIPLIED;

    return DFB_OK;
}

static DFBResult
tivogfxShutdownLayer(   CoreLayer * layer,
                        void      * driver_data,
                        void      * layer_data )
{
    TiVoGfxLayerData   * pGfxLayer;

    D_DEBUG_AT( tivogfx_layer, "%s()\n", __FUNCTION__ );

    TIVOGFX_UNUSED_PARAMETER( layer );
    TIVOGFX_UNUSED_PARAMETER( driver_data );

    pGfxLayer = (TiVoGfxLayerData *) layer_data;
    D_MAGIC_ASSERT( pGfxLayer, TiVoGfxLayerData );
    D_MAGIC_CLEAR( pGfxLayer );

    return DFB_OK;
}

static DFBResult
tivogfxTestRegion(      CoreLayer                  * layer,
                        void                       * driver_data,
                        void                       * layer_data,
                        CoreLayerRegionConfig      * config,
                        CoreLayerRegionConfigFlags * failed )
{
    TiVoGfxLayerData   * pGfxLayer;

    D_DEBUG_AT( tivogfx_layer, "%s( %dx%d, format %s, "
        "buffermode %s, options %d )\n",
        __FUNCTION__, config->source.w, config->source.h,
        dfb_pixelformat_name( config->format ),
        tivogfx_dfb_buffermode_name( config->buffermode ),
        config->options );

    TIVOGFX_UNUSED_PARAMETER( layer );
    TIVOGFX_UNUSED_PARAMETER( driver_data );

    pGfxLayer = (TiVoGfxLayerData *) layer_data;
    D_MAGIC_ASSERT( pGfxLayer, TiVoGfxLayerData );

    CoreLayerRegionConfigFlags fail = 0;

    switch (config->buffermode) {
        case DLBM_FRONTONLY:
        case DLBM_BACKVIDEO:
        case DLBM_BACKSYSTEM:
        case DLBM_TRIPLE:
            break;

        default:
            D_DEBUG_AT( tivogfx_layer, "%s() => unsupported buffer mode\n",
                __FUNCTION__ );
            fail |= CLRCF_BUFFERMODE;
            break;
    }

    switch (config->format) {
        case DSPF_ARGB:
            break;

        default:
            D_DEBUG_AT( tivogfx_layer, "%s() => unsupported format\n",
                __FUNCTION__ );
            fail |= CLRCF_FORMAT;
            break;
    }

    if (!D_FLAGS_ARE_IN( config->options, DLOP_OPACITY | DLOP_ALPHACHANNEL )) {
        D_DEBUG_AT( tivogfx_layer, "%s() => unsupported options\n",
            __FUNCTION__ );
        fail |= CLRCF_OPTIONS;
    }

    if (failed) {
        *failed = fail;
    }

    if (fail) {
        D_DEBUG_AT( tivogfx_layer, "%s() => DFB_UNSUPPORTED\n",
            __FUNCTION__ );

        return DFB_UNSUPPORTED;
    }

    D_DEBUG_AT( tivogfx_layer, "%s() => DFB_OK\n",
        __FUNCTION__ );

    return DFB_OK;
}

static DFBResult
tivogfxAddRegion(       CoreLayer             * layer,
                        void                  * driver_data,
                        void                  * layer_data,
                        void                  * region_data,
                        CoreLayerRegionConfig * config )
{
    D_DEBUG_AT( tivogfx_layer, "%s()\n", __FUNCTION__ );

    TIVOGFX_UNUSED_PARAMETER( layer );
    TIVOGFX_UNUSED_PARAMETER( driver_data );
    TIVOGFX_UNUSED_PARAMETER( layer_data );
    TIVOGFX_UNUSED_PARAMETER( region_data );
    TIVOGFX_UNUSED_PARAMETER( config );

    return DFB_OK;
}

static DFBResult
tivogfxSetRegion(       CoreLayer                  * layer,
                        void                       * driver_data,
                        void                       * layer_data,
                        void                       * region_data,
                        CoreLayerRegionConfig      * config,
                        CoreLayerRegionConfigFlags   updated,
                        CoreSurface                * surface,
                        CorePalette                * palette,
                        CoreSurfaceBufferLock      * lock )
{
    D_DEBUG_AT( tivogfx_layer, "%s()\n", __FUNCTION__ );

    TIVOGFX_UNUSED_PARAMETER( layer );
    TIVOGFX_UNUSED_PARAMETER( driver_data );
    TIVOGFX_UNUSED_PARAMETER( layer_data );
    TIVOGFX_UNUSED_PARAMETER( region_data );
    TIVOGFX_UNUSED_PARAMETER( config );
    TIVOGFX_UNUSED_PARAMETER( updated );
    TIVOGFX_UNUSED_PARAMETER( surface );
    TIVOGFX_UNUSED_PARAMETER( palette );
    TIVOGFX_UNUSED_PARAMETER( lock );

    return DFB_OK;
}

static DFBResult
tivogfxRemoveRegion(    CoreLayer             * layer,
                        void                  * driver_data,
                        void                  * layer_data,
                        void                  * region_data )
{
    D_DEBUG_AT( tivogfx_layer, "%s()\n", __FUNCTION__ );

    TIVOGFX_UNUSED_PARAMETER( layer );
    TIVOGFX_UNUSED_PARAMETER( driver_data );
    TIVOGFX_UNUSED_PARAMETER( layer_data );
    TIVOGFX_UNUSED_PARAMETER( region_data );

    return DFB_OK;
}

static DFBResult
tivogfxFlipRegion(      CoreLayer             * layer,
                        void                  * driver_data,
                        void                  * layer_data,
                        void                  * region_data,
                        CoreSurface           * surface,
                        DFBSurfaceFlipFlags     flags,
                        CoreSurfaceBufferLock * lock )
{
    TiVoGfxLayerData  * pGfxLayer;
    DFBRegion           region;

    D_DEBUG_AT( tivogfx_layer, "%s()\n", __FUNCTION__ );

    TIVOGFX_UNUSED_PARAMETER( layer );
    TIVOGFX_UNUSED_PARAMETER( driver_data );

    pGfxLayer = (TiVoGfxLayerData *) layer_data;
    D_MAGIC_ASSERT( pGfxLayer, TiVoGfxLayerData );

    TIVOGFX_UNUSED_PARAMETER( region_data );
    TIVOGFX_UNUSED_PARAMETER( flags );

    region = DFB_REGION_INIT_FROM_DIMENSION( &surface->config.size );

    dfb_surface_flip( surface, false );

    return tivoGfxUpdateScreen( pGfxLayer, &region, lock );
}

static DFBResult
tivogfxUpdateRegion(    CoreLayer             * layer,
                        void                  * driver_data,
                        void                  * layer_data,
                        void                  * region_data,
                        CoreSurface           * surface,
                        const DFBRegion       * update,
                        CoreSurfaceBufferLock * lock )
{
    TiVoGfxLayerData  * pGfxLayer;
    DFBRegion           region;

    D_DEBUG_AT( tivogfx_layer, "%s()\n", __FUNCTION__ );

    TIVOGFX_UNUSED_PARAMETER( layer );
    TIVOGFX_UNUSED_PARAMETER( driver_data );

    pGfxLayer = (TiVoGfxLayerData *) layer_data;
    D_MAGIC_ASSERT( pGfxLayer, TiVoGfxLayerData );

    TIVOGFX_UNUSED_PARAMETER( region_data );

    region = DFB_REGION_INIT_FROM_DIMENSION( &surface->config.size );

    if (update && !dfb_region_region_intersect( &region, update ))
    {
        return DFB_OK;
    }

    return tivoGfxUpdateScreen( pGfxLayer, &region, lock );
}

const DisplayLayerFuncs tivoGfxPrimaryLayerFuncs = {

    LayerDataSize:     tivogfxLayerDataSize,
    RegionDataSize:    tivogfxRegionDataSize,

    InitLayer:         tivogfxInitLayer,
    ShutdownLayer:     tivogfxShutdownLayer,

    TestRegion:        tivogfxTestRegion,
    AddRegion:         tivogfxAddRegion,
    SetRegion:         tivogfxSetRegion,
    RemoveRegion:      tivogfxRemoveRegion,
    FlipRegion:        tivogfxFlipRegion,
    UpdateRegion:      tivogfxUpdateRegion,
};

/******************************************************************************/

