//////////////////////////////////////////////////////////////////////
//
// File: tivogfx_system.c
//
// Copyright 2013 TiVo Inc.  All Rights Reserved.
//
//////////////////////////////////////////////////////////////////////

// always include first
#include "tivogfx_directfb.h"

// other directfb includes
#include <core/core_system.h>
#include <core/layers.h>
#include <core/screens.h>
#include <directfb_util.h>
#include <fusion/arena.h>

// source module includes
#include "tivogfx_system.h"

#include <string.h>

#include "tivogfx_init.h"
#include "tivogfx_layer.h"
#include "tivogfx_proxy.h"
#include "tivogfx_screen.h"
#include "tivogfx_surface.h"
#include "tivogfx_util.h"

/******************************************************************************/

DFB_CORE_SYSTEM( tivogfx )

D_DEBUG_DOMAIN( tivogfx_system, "tivogfx/system", "TiVo Gfx system" );

/* the per-process core handle */
CoreDFB *pCoreG = NULL;

/******************************************************************************/

TiVoGfxSystemData* TvGfx_GetSystemData( void )
{
    TiVoGfxSystemData* pGfxSystemData;

    pGfxSystemData = (TiVoGfxSystemData *) dfb_system_data();
    D_MAGIC_ASSERT( pGfxSystemData, TiVoGfxSystemData );

    return pGfxSystemData;
}

/******************************************************************************/

static void TestTvGraphicsCompositor( TvGfxProxy hGfxProxy )
{
    D_DEBUG_AT( tivogfx_system, "%s()\n", __FUNCTION__ );

    do
    {
        const U16 kMaxConfigs = 256;

        U16 numConfigs = 0;

        HPK_ScreenConfig configs[ kMaxConfigs ];

        memset( configs, 0, sizeof(configs) );

        if (!TvGfxProxy_GetNumScreenConfiguration( hGfxProxy,
            &numConfigs ))
        {
            D_DEBUG_AT( tivogfx_system, "%s() %s\n", __FUNCTION__,
                "TvGfxProxy_GetNumScreenConfiguration failed" );
            break;
        }

        if (numConfigs > kMaxConfigs)
        {
            D_DEBUG_AT( tivogfx_system, "%s() %s\n", __FUNCTION__,
                "numConfigs > kMaxConfigs" );

            numConfigs = kMaxConfigs;
        }

        if (!TvGfxProxy_GetScreenConfigs( hGfxProxy,
            numConfigs, configs ))
        {
            D_DEBUG_AT( tivogfx_system, "%s() %s\n", __FUNCTION__,
                "TvGfxProxy_GetScreenConfigs failed" );
            break;
        }

        D_DEBUG_AT( tivogfx_system, "%s() numConfigs=%u [0]={w=%d h=%d}\n",
            __FUNCTION__, numConfigs,
            configs[0].rect.w, configs[0].rect.h );
    }
    while (false);

    D_DEBUG_AT( tivogfx_system, "%s() %s\n", __FUNCTION__, "exit" );
}

/******************************************************************************/

static void
system_get_info( CoreSystemInfo *info )
{
    D_DEBUG_AT( tivogfx_system, "%s()\n", __FUNCTION__ );

    info->type = CORE_TIVOGFX;
    info->caps = CSCAPS_ACCELERATION;

    info->version.major = 0 /* TIVOGFX_SYS_MAJOR_VERSION */;
    info->version.minor = 1 /* TIVOGFX_SYS_MINOR_VERSION */;

    snprintf( info->name,    DFB_CORE_SYSTEM_INFO_NAME_LENGTH,    "TiVo Gfx system" );
    snprintf( info->vendor,  DFB_CORE_SYSTEM_INFO_VENDOR_LENGTH,  "TiVo Inc." );
    snprintf( info->url,     DFB_CORE_SYSTEM_INFO_URL_LENGTH,     "http://www.tivo.com" );
    snprintf( info->license, DFB_CORE_SYSTEM_INFO_LICENSE_LENGTH, "proprietary" );
}

static DFBResult
system_initialize( CoreDFB *core, void **data )
{
    TiVoGfxSystemData   * pGfxSystem;

    D_DEBUG_AT( tivogfx_system, "%s()\n", __FUNCTION__ );

    // XXX this may be moved to direct_initialize in the future
    TvGfxInit_InitializeProgram();

    pGfxSystem = (TiVoGfxSystemData *) D_CALLOC( 1, sizeof(TiVoGfxSystemData) );
    if (!pGfxSystem) {
        /* print warning and return error */
        return D_OOM();
    }

    D_MAGIC_SET( pGfxSystem, TiVoGfxSystemData );

    /* Save the per-process core handle. */
    pCoreG = core;

    pthread_mutex_init( &pGfxSystem->gfxMutex, NULL );

    if (!TvGfxProxy_Create( &pGfxSystem->hGfxProxy ))
    {
        D_DEBUG_AT( tivogfx_system, "%s() %s\n", __FUNCTION__,
            "TvGfxProxy_Create failed" );

        // XXX clean up
        return DFB_FAILURE;
    }

    /* Make data accessible via dfb_system_data(). */
    /* Must be set before initializing the pools. */
    *data = pGfxSystem;

    // XXX
    TestTvGraphicsCompositor( pGfxSystem->hGfxProxy );

    /* register primary screen */
    pGfxSystem->screen = dfb_screens_register( NULL /* device */,
        pGfxSystem, &tivoGfxPrimaryScreenFuncs );
    if (!pGfxSystem->screen)
    {
        D_ERROR( "DirectFB/TiVoGfx: dfb_screens_register failed\n" );

        // XXX clean up
        return DFB_FAILURE;
    }

    dfb_layers_register( pGfxSystem->screen, pGfxSystem,
        &tivoGfxPrimaryLayerFuncs );

    dfb_surface_pool_initialize( core, &tivoGfxBackbufferPoolFuncs,
        &pGfxSystem->backbuffer_pool );

    dfb_surface_pool_initialize( core, &tivoGfxSurfacePoolFuncs,
        &pGfxSystem->surface_pool );

    D_DEBUG_AT( tivogfx_system, "%s() OK\n", __FUNCTION__ );
    return DFB_OK;
}

static DFBResult
system_shutdown( bool emergency )
{
    TiVoGfxSystemData  * pGfxSystem;

    D_DEBUG_AT( tivogfx_system, "%s()\n", __FUNCTION__ );

    TIVOGFX_UNUSED_PARAMETER( emergency );

    pGfxSystem = (TiVoGfxSystemData *) dfb_system_data();
    D_MAGIC_ASSERT( pGfxSystem, TiVoGfxSystemData );

    /* Destroy the surface pool */
    dfb_surface_pool_destroy( pGfxSystem->surface_pool );
    dfb_surface_pool_destroy( pGfxSystem->backbuffer_pool );

    TvGfxProxy_Release( /* modifies */ &pGfxSystem->hGfxProxy );

    pthread_mutex_destroy( &pGfxSystem->gfxMutex );

    D_MAGIC_CLEAR( pGfxSystem );
    D_FREE( pGfxSystem );
    pGfxSystem = NULL;

    pCoreG = NULL;

    // XXX this may be moved to direct_shutdown in the future
    TvGfxInit_ShutdownProgram();

    D_DEBUG_AT( tivogfx_system, "%s() OK\n", __FUNCTION__ );
    return DFB_OK;
}

static DFBResult
system_join( CoreDFB *core, void **data )
{
    D_DEBUG_AT( tivogfx_system, "%s()\n", __FUNCTION__ );

    TIVOGFX_UNUSED_PARAMETER( core );
    TIVOGFX_UNUSED_PARAMETER( data );

    // XXX
    D_UNIMPLEMENTED();
    return DFB_UNIMPLEMENTED;
}

static DFBResult
system_leave( bool emergency )
{
    D_DEBUG_AT( tivogfx_system, "%s()\n", __FUNCTION__ );

    TIVOGFX_UNUSED_PARAMETER( emergency );

    // XXX
    D_UNIMPLEMENTED();
    return DFB_UNIMPLEMENTED;
}

static DFBResult
system_suspend( void )
{
    D_DEBUG_AT( tivogfx_system, "%s()\n", __FUNCTION__ );

    D_UNIMPLEMENTED();
    return DFB_UNIMPLEMENTED;
}

static DFBResult
system_resume( void )
{
    D_DEBUG_AT( tivogfx_system, "%s()\n", __FUNCTION__ );

    D_UNIMPLEMENTED();
    return DFB_UNIMPLEMENTED;
}

static VideoMode*
system_get_modes( void )
{
    D_DEBUG_AT( tivogfx_system, "%s()\n", __FUNCTION__ );

    return NULL;
}

static VideoMode*
system_get_current_mode( void )
{
    D_DEBUG_AT( tivogfx_system, "%s()\n", __FUNCTION__ );

    return NULL;
}

static DFBResult
system_thread_init( void )
{
    D_DEBUG_AT( tivogfx_system, "%s()\n", __FUNCTION__ );

    return DFB_OK;
}

static bool
system_input_filter( CoreInputDevice * device,
                     DFBInputEvent   * event )
{
    D_DEBUG_AT( tivogfx_system, "%s()\n", __FUNCTION__ );

    TIVOGFX_UNUSED_PARAMETER( device );
    TIVOGFX_UNUSED_PARAMETER( event );

    return false;
}

static volatile void*
system_map_mmio( unsigned int    offset,
                 int             length )
{
    D_DEBUG_AT( tivogfx_system, "%s()\n", __FUNCTION__ );

    TIVOGFX_UNUSED_PARAMETER( offset );
    TIVOGFX_UNUSED_PARAMETER( length );

    return NULL;
}

static void
system_unmap_mmio( volatile void * addr,
                   int             length )
{
    D_DEBUG_AT( tivogfx_system, "%s()\n", __FUNCTION__ );

    TIVOGFX_UNUSED_PARAMETER( addr );
    TIVOGFX_UNUSED_PARAMETER( length );
}

static int
system_get_accelerator( void )
{
    D_DEBUG_AT( tivogfx_system, "%s()\n", __FUNCTION__ );

    return -1;
}

static unsigned long
system_video_memory_physical( unsigned int offset )
{
    D_DEBUG_AT( tivogfx_system, "%s()\n", __FUNCTION__ );

    TIVOGFX_UNUSED_PARAMETER( offset );

    return 0;
}

static void*
system_video_memory_virtual( unsigned int offset )
{
    D_DEBUG_AT( tivogfx_system, "%s()\n", __FUNCTION__ );

    TIVOGFX_UNUSED_PARAMETER( offset );

    return NULL;
}

static unsigned int
system_videoram_length( void )
{
    D_DEBUG_AT( tivogfx_system, "%s()\n", __FUNCTION__ );

    return 0;
}

static unsigned long
system_aux_memory_physical( unsigned int offset )
{
    D_DEBUG_AT( tivogfx_system, "%s()\n", __FUNCTION__ );

    TIVOGFX_UNUSED_PARAMETER( offset );

    return 0;
}

static void*
system_aux_memory_virtual( unsigned int offset )
{
    D_DEBUG_AT( tivogfx_system, "%s()\n", __FUNCTION__ );

    TIVOGFX_UNUSED_PARAMETER( offset );

    return NULL;
}

static unsigned int
system_auxram_length( void )
{
    D_DEBUG_AT( tivogfx_system, "%s()\n", __FUNCTION__ );

    return 0;
}

static void
system_get_busid( int *ret_bus, int * ret_dev, int * ret_func )
{
    D_DEBUG_AT( tivogfx_system, "%s()\n", __FUNCTION__ );

    *ret_bus  = 0;
    *ret_dev  = 0;
    *ret_func = 0;
}

static void
system_get_deviceid( unsigned int * ret_vendor_id, unsigned int * ret_device_id )
{
    D_DEBUG_AT( tivogfx_system, "%s()\n", __FUNCTION__ );

    *ret_vendor_id = 0;
    *ret_device_id = 0;
}

/******************************************************************************/

