//////////////////////////////////////////////////////////////////////
//
// File: tivogfx_proxy.cpp
//
// Copyright 2014 TiVo Inc.  All Rights Reserved.
//
//////////////////////////////////////////////////////////////////////

// always include first
#include "tivogfx_directfb.h"

// other directfb includes
#include <directfb_util.h>
#include <core/palette.h>
#include <direct/mem.h>

// source module includes
#include "tivogfx_proxy.h"

#include <stdlib.h>
#include <string.h>

#include <tmk/TmkAssert.h>

#include <gfx/TvGraphicsCompositor.h>
#include <gfx/TvGraphicsCompositorDebug.h>
#include <hpktivo/HpkGfxConstants.h>
#include <hpk/HpkPlatform.h>

#include "tivogfx_init.h"
#include "tivogfx_input_hpk.h"
#include "tivogfx_input_ipc.h"
#include "tivogfx_input_ipc2.h"
#include "tivogfx_util.h"

#include <plog/TcdPlatformPlogIds.h>
#include <tmk/TmkProcessLog.h>

//####################################################################

D_DEBUG_DOMAIN( tivogfx_proxy, "tivogfx/proxy", "TiVo Gfx proxy" );

D_DEBUG_DOMAIN( tivogfx_proxy_x, "tivogfx_x/proxy", "TiVo Gfx proxy" );

D_DEBUG_DOMAIN( tivogfx_proxy_ops_x, "tivogfx_x/proxy/ops", "TiVo Gfx proxy" );

// enable verbose diagnostics if needed
#define D_DEBUG_AT_X( d, x... )                 \
     D_DEBUG_AT( d##_x, x )

#define SYNC_TIMEOUT_500MS  (500000000ULL)


static TvGraphicsCompositor* pTvGraphicsCompositorS;

//####################################################################

/**
 * COPY_DFB_RECT_TO_HPK_RECT
 */
inline
void COPY_DFB_RECT_TO_HPK_RECT( const DFBRectangle* pDfbRect, HPK_Rect* pHpkRect )
{
    pHpkRect->x = pDfbRect->x;
    pHpkRect->y = pDfbRect->y;
    pHpkRect->w = pDfbRect->w;
    pHpkRect->h = pDfbRect->h;
}

/**
 * COPY_DFB_COLOR_TO_HPK_COLOR
 */
inline
void COPY_DFB_COLOR_TO_HPK_COLOR( const DFBColor* pDfbColor, HPK_Color* pHpkColor )
{
    pHpkColor->a = pDfbColor->a;
    pHpkColor->r = pDfbColor->r;
    pHpkColor->g = pDfbColor->g;
    pHpkColor->b = pDfbColor->b;
}

//####################################################################

// XXX move these to Mempool or DirectFB allocator later,
// for now use raw allocator


/**
 * TvGfxProxy_Malloc
 */
static
void* TvGfxProxy_Malloc( size_t size )
{
    return malloc( size );
}


/**
 * TvGfxProxy_Free
 */
static
void TvGfxProxy_Free( void* ptr )
{
    free( ptr );
}

/**
 * TvGfxProxy_Calloc
 */
static
void* TvGfxProxy_Calloc( size_t nmemb, size_t size )
{
    return calloc( nmemb, size );
}

static
HPK_BlitFlag TvGfxProxy_ConvertFlagsDfb2Hpk( DFBSurfaceBlittingFlags blitFlags );

static
HPK_BlendFunction TvGfxProxy_ConvertDfb2HpkBlendMode( DFBSurfaceBlendFunction dfbBlend );

static
void TvGfxProxy_CopyDfbRenderOpts2HpkBlitfilter( const DFBRectangle* srcRect,
    const DFBRectangle* dstRect,
    const DFBSurfaceRenderOptions pDfbRenderOpts,
    HPK_BlitFilter* pHpkBlitFilter );


#define malloc  Dont_Use_malloc
#define free    Dont_Use_malloc
#define calloc  Dont_Use_calloc
#define realloc Dont_Use_calloc

//####################################################################

/**
 * TvGfxInitArgbMemory
 *
 * For debugging purposes only.
 */
void TvGfxInitArgbMemory( void* pBuff, int width, int height,
    int pitch, u32 argb )
{
    TvDebugAssert( pBuff != NULL );
    TvDebugAssert( (width > 0)  && (width < TIVOGFX_HD_DISPLAY_WIDTH * 2) );
    TvDebugAssert( (height > 0) && (height < TIVOGFX_HD_DISPLAY_HEIGHT * 2) );
    TvDebugAssert( (pitch >= width * 4) && (pitch <= width * 4 + 64) );

    // Note: look like pixels are treated as U32 not U8[ 4 ]?
    u8* pRow = reinterpret_cast< u8* >( pBuff );
    for (int y = 0; y < height; y++)
    {
        u32* pPixel = (u32*) pRow;
        for (int x = 0; x < width; x++)
        {
            *pPixel++ = argb;
        }
        pRow += pitch;
    }
}

//####################################################################

/**
 * TVGFXPROXY_PROXY_NAME( OBJ_NAME )
 *
 * The name of the opaque proxy object parameter,
 * e.g. GfxProxy => hGfxProxy.
 */
#define TVGFXPROXY_PROXY_NAME( OBJ_NAME )    \
    h##OBJ_NAME

/**
 * TVGFXPROXY_PROXY_TYPE( OBJ_NAME )
 *
 * The name of the opaque proxy type,
 * e.g. GfxProxy => TvGfxProxy.
 */
#define TVGFXPROXY_PROXY_TYPE( OBJ_NAME )    \
    Tv##OBJ_NAME

/**
 * TVGFXPROXY_IMPL_NAME( OBJ_NAME )
 *
 * The name of the local implementation object variable,
 * e.g. GfxProxy => pGfxProxy.
 */
#define TVGFXPROXY_IMPL_NAME( OBJ_NAME )    \
    p##OBJ_NAME

/**
 * TVGFXPROXY_IMPL_TYPE( OBJ_NAME )
 *
 * The name of the object implementation struct,
 * e.g. GfxProxy => TvGfxProxyImpl.
 */
#define TVGFXPROXY_IMPL_TYPE( OBJ_NAME )    \
    Tv##OBJ_NAME##Impl

/**
 * TVGFXPROXY_ALLOCATE_OBJECT( OBJ_NAME )
 */
#define TVGFXPROXY_ALLOCATE_OBJECT( OBJ_NAME )   \
                                                                                \
    TVGFXPROXY_IMPL_TYPE( OBJ_NAME )* TVGFXPROXY_IMPL_NAME( OBJ_NAME ) =        \
        reinterpret_cast< TVGFXPROXY_IMPL_TYPE( OBJ_NAME )* >(                  \
            TvGfxProxy_Calloc( 1, sizeof( TVGFXPROXY_IMPL_TYPE( OBJ_NAME ) ) )  \
        );                                                                      \
                                                                                \
    D_MAGIC_SET( TVGFXPROXY_IMPL_NAME( OBJ_NAME ),                              \
        TVGFXPROXY_IMPL_TYPE( OBJ_NAME ) );

/**
 * TVGFXPROXY_FREE_OBJECT( OBJ_NAME )
 */
#define TVGFXPROXY_FREE_OBJECT( OBJ_NAME )   \
                                                                                \
    D_MAGIC_ASSERT( TVGFXPROXY_IMPL_NAME( OBJ_NAME ),                           \
        TVGFXPROXY_IMPL_TYPE( OBJ_NAME ) );                                     \
                                                                                \
    D_MAGIC_CLEAR( TVGFXPROXY_IMPL_NAME( OBJ_NAME ) );                          \
                                                                                \
    TvGfxProxy_Free( TVGFXPROXY_IMPL_NAME( OBJ_NAME ) );                        \
                                                                                \
    TVGFXPROXY_IMPL_NAME( OBJ_NAME ) = NULL;

/**
 * TVGFXPROXY_OBJECT_TO_PROXY( OBJ_NAME )
 */
#define TVGFXPROXY_OBJECT_TO_PROXY( OBJ_NAME )   \
    reinterpret_cast< TVGFXPROXY_PROXY_TYPE( OBJ_NAME ) >(                      \
        TVGFXPROXY_IMPL_NAME( OBJ_NAME )                                        \
    )

/**
 * TVGFXPROXY_GET_OBJECT_FROM_PROXY( OBJ_NAME )
 */
#define TVGFXPROXY_GET_OBJECT_FROM_PROXY( OBJ_NAME )   \
                                                                                \
    TVGFXPROXY_IMPL_TYPE( OBJ_NAME )* TVGFXPROXY_IMPL_NAME( OBJ_NAME ) =        \
        reinterpret_cast< TVGFXPROXY_IMPL_TYPE( OBJ_NAME ) * >(                 \
            TVGFXPROXY_PROXY_NAME( OBJ_NAME )                                   \
        );                                                                      \
                                                                                \
    D_MAGIC_ASSERT( TVGFXPROXY_IMPL_NAME( OBJ_NAME ),                           \
        TVGFXPROXY_IMPL_TYPE( OBJ_NAME ) );

/**
 * Convenient macro for returned TvGraphics status.
 */
#define TV_SUCCEEDED( result ) (( result ) == TV_OK )

/**
 * Convenient macro for returned TvGraphics status.
 */
#define TV_FAILED( result )    (( result ) != TV_OK )

/**
 * Continue operation if status OK, throw assert if operation fails
 */
#define TV_CALL( func )                             \
    if (TV_SUCCEEDED( result ))                     \
    {                                               \
        result = ( func );                          \
        TvDebugAssert( TV_SUCCEEDED( result ));     \
    }

//####################################################################

/**
 * TvGfxProxy_GetHpkPixelFormat
 */
static HPK_PixelFormat TvGfxProxy_GetHpkPixelFormat(
    DFBSurfacePixelFormat dfbPixelFormat )
{
    HPK_PixelFormat hpkPixelFormat = HPK_PIXFMT_NONE;

    switch (dfbPixelFormat)
    {
        case DSPF_A1:
            hpkPixelFormat = HPK_PIXFMT_ALPHA_1;
            break;

        case DSPF_A4:
            hpkPixelFormat = HPK_PIXFMT_ALPHA_4;
            break;

        case DSPF_A8:
            hpkPixelFormat = HPK_PIXFMT_ALPHA_8;
            break;

        case DSPF_RGB16:
            hpkPixelFormat = HPK_PIXFMT_RGB_565;
            break;

        case DSPF_ARGB:
            hpkPixelFormat = HPK_PIXFMT_ARGB_8888;
            break;

        case DSPF_RGBA4444:
            hpkPixelFormat = HPK_PIXFMT_RGBA_4444;
            break;

        case DSPF_UYVY:
            hpkPixelFormat = HPK_PIXFMT_YCbCr_422;
            break;

        case DSPF_LUT2:
            hpkPixelFormat = HPK_PIXFMT_INDEX_2;
            break;

        case DSPF_LUT8:
            hpkPixelFormat = HPK_PIXFMT_INDEX_8;
            break;

        default:
            hpkPixelFormat = HPK_PIXFMT_NONE;
            break;
    }

    return hpkPixelFormat;
}

//####################################################################

// arbitrary limit for code robustness
#define TIVOGFX_MAX_SCREEN_CONFIGS  256

//####################################################################

// arbitrary throttling of how many key/input events can be queued
// Keep in mind that using a Slide Remote keyboard you can generate
// a dozen key press + release events fairly quickly when typing.
#define TIVOGFX_MAX_INPUT_EVENTS    32

/**
 * TvGfxProxyImpl
 */
struct TVGFXPROXY_IMPL_TYPE( GfxProxy )
{
    int     magic;

    bool    fDebugInitSurfaces;
    bool    fScratchSurfaceAllocated;

    TvGraphicsMemoryPool *  pSharedGfxMemoryPoolM;
    TvGraphicsMemoryPool *  pPrivateGfxMemoryPoolM;
    TvGraphicsCompositor *  pTvGraphicsCompositorM;

    pthread_mutex_t inputMutex;
    int             numInputEvents;
    DFBInputEvent   inputEvents[ TIVOGFX_MAX_INPUT_EVENTS ];

    bool         fMapVideo;
    bool         fMapVideoOnceM;       // re-map the video plane once, typically at startup
    DFBRectangle videoPlaneRectM;
};

/**
 * TvGfxBufferImpl
 */
struct TVGFXPROXY_IMPL_TYPE( GfxBuffer )
{
    int     magic;

    TvSurface*      pTvSurfaceM;

    // XXX to start keep the TvSurface locked!
    TvPixmap*       pTvPixmapM;
    HPK_PixelData   pixelDataM;

    CoreSurfaceAccessorID writeAccessor;
};

/**
 * TvGfxProxy_Create
 */
bool TvGfxProxy_Create( /* returns */ TvGfxProxy* phGfxProxy )
{
    D_DEBUG_AT( tivogfx_proxy, "%s()\n", __FUNCTION__ );

    TvDebugAssert( phGfxProxy != NULL );

    // no return in case of error
    *phGfxProxy = NULL;

    // XXX config to be reviewed
    U32    DFB_GRAPHICS_CLIENT_ID       = HPK_GFX_CLIENT_FLASH_3RD_PARTY;

    const char* DFB_GRAPHICS_CLIENT = "DFB_GRAPHICS_CLIENT";
    const char* dfbGraphicsClientStr = getenv( DFB_GRAPHICS_CLIENT );
    if ( dfbGraphicsClientStr != NULL )
    {
        int id = atoi( dfbGraphicsClientStr );
        if ( id > 0 )
        {
            DFB_GRAPHICS_CLIENT_ID = id; //HPK_GFX_CLIENT_FLASH;//HPK_GFX_CLIENT_ATLAS;
        }
    }
    size_t DFB_GRAPHICS_MEMPOOL_SIZE    = 26 * 1024 * 1024;

    printf( "==========================================================\n" );
    printf( "DirectFB-TiVoGfx TvGfxProxy_Create\n" );

    //========================================================================

    const char* ENV_DFB_GRAPHICS_SIZE_MB = "DFB_GRAPHICS_SIZE_MB";
    const char* dfbGraphicsSizeMbStr = getenv( ENV_DFB_GRAPHICS_SIZE_MB );

    if ((dfbGraphicsSizeMbStr != NULL) && (*dfbGraphicsSizeMbStr != 0))
    {
        // no error checking!
        int sizeInMb = atoi( dfbGraphicsSizeMbStr );

        // arbitrary sanity checks
        const int minSizeInMb = 4;
        const int maxSizeInMb = 64;

        if ((sizeInMb < minSizeInMb) || (sizeInMb > maxSizeInMb))
        {
            printf( "ERROR: %s env. var value %s is not in range %d..%d, "
                "ignoring.\n",
                ENV_DFB_GRAPHICS_SIZE_MB, dfbGraphicsSizeMbStr,
                minSizeInMb, maxSizeInMb );
        }
        else
        {
            printf( "%s = %s => request size %d MB.\n",
                ENV_DFB_GRAPHICS_SIZE_MB, dfbGraphicsSizeMbStr, sizeInMb );

            DFB_GRAPHICS_MEMPOOL_SIZE = sizeInMb * 1024 * 1024;
        }
    }

    //========================================================================

    // By default DirectFB will be 'pool-less' client for TvGraphicsCompositor
    U32 poolSize = 0;

#if BOOTARCH == X86
        poolSize = DFB_GRAPHICS_MEMPOOL_SIZE;
#endif
    //========================================================================

    printf("Calling TvGraphicsCompositorCreate with %s @ %3.1f MB\n",
            ((DFB_GRAPHICS_CLIENT_ID == HPK_GFX_CLIENT_FLASH_3RD_PARTY) ?
                    "HPK_GFX_CLIENT_FLASH_3RD_PARTY" : dfbGraphicsClientStr),
           (double) poolSize / (double) (1024 * 1024) );


    printf("==========================================================\n" );

    TvGraphicsCompositor* pTvGraphicsCompositor;
    TvGraphicsMemoryPool* pSharedGraphicsMemoryPool;
    TvGraphicsMemoryPool* pPrivateGraphicsMemoryPool;

#if BOOTARCH != X86

    // XXX FIXME: remove it after Gen07P got support for scratch shared
    //            graphics memory pool.

    HPK_Platform   platform;
    HPK_PlatformId id;

    HPK_Result result = HPK_GetPlatform( &platform );

    if (result != HPK_SUCCESS)
    {
        D_DEBUG_AT_X( tivogfx_proxy,
                      "%s: HPK_GetPlatform error\n",
                      __FUNCTION__ );
        return false;
    }

    result = HPK_Platform_GetId( &platform, &id );

    if (result != HPK_SUCCESS)
    {
        D_DEBUG_AT_X( tivogfx_proxy,
                      "%s: FAILED to get platform ID, result = 0x%x",
                      __FUNCTION__,
                      result );
        return false;
    }

    if (id.type != HPK_PLATFORM_TYPE_PICASSO)
    {
//
    pSharedGraphicsMemoryPool =
        new TvGraphicsMemoryPool(HPK_GFX_SHARED_GRAPHICS_POOL_SIZE,
                                 HPK_GFX_SHARED_GRAPHICS_POOL_NAME,
                                 false
                                 );

    pTvGraphicsCompositor =
        TvGraphicsCompositorCreate(
            DFB_GRAPHICS_CLIENT_ID,
            pSharedGraphicsMemoryPool,
            false,
            TV_GFX_LOG_LEVEL_IGNORE_MEM_ERR
            );

    pPrivateGraphicsMemoryPool =
        new TvGraphicsMemoryPool(poolSize);
//
    }
    else
    {
        pTvGraphicsCompositor =
            TvGraphicsCompositorCreate(
                DFB_GRAPHICS_CLIENT_ID,
                DFB_GRAPHICS_MEMPOOL_SIZE,
                false,    // !fFillFBTransparent
                NULL,     // no shared gfx mempool
                TV_GFX_LOG_LEVEL_IGNORE_MEM_ERR
                );

        pSharedGraphicsMemoryPool = NULL;
        pPrivateGraphicsMemoryPool = NULL;
    }
// XXX FIXME XXX

#else  // X86
    pTvGraphicsCompositor =
        TvGraphicsCompositorCreate(
            DFB_GRAPHICS_CLIENT_ID,
            poolSize, // DFB_GRAPHICS_MEMPOOL_SIZE,
            false,    // !fFillFBTransparent
            NULL,     // no shared gfx mempool
            TV_GFX_LOG_LEVEL_IGNORE_MEM_ERR
            );

    pSharedGraphicsMemoryPool = NULL;
    pPrivateGraphicsMemoryPool = NULL;
#endif

    if (pTvGraphicsCompositor == NULL)
    {
        TvDebugAssert( false, "TvGraphicsCompositorCreate failed" );
        return false;
    }

    TvCrashAssert( pTvGraphicsCompositorS == NULL );
    pTvGraphicsCompositorS = pTvGraphicsCompositor;

    TVGFXPROXY_ALLOCATE_OBJECT( GfxProxy );

    // preserve legacy old full screen video mapping behavior
    if ( DFB_GRAPHICS_CLIENT_ID == HPK_GFX_CLIENT_FLASH_3RD_PARTY )
    {
        pGfxProxy->fMapVideo = false;
        pGfxProxy->fMapVideoOnceM = true;
        pGfxProxy->videoPlaneRectM.x = 0;
        pGfxProxy->videoPlaneRectM.y = 0;
        pGfxProxy->videoPlaneRectM.w = TIVOGFX_HD_DISPLAY_WIDTH;
        pGfxProxy->videoPlaneRectM.h = TIVOGFX_HD_DISPLAY_HEIGHT;
    }

    const char* debugInitSurfacesStr = getenv( "DFB_DEBUG_INIT_SURFACES" );
    if ((debugInitSurfacesStr != NULL) &&
        (strcmp( debugInitSurfacesStr, "1" ) == 0))
    {
        pGfxProxy->fDebugInitSurfaces = true;
    }

    pGfxProxy->pTvGraphicsCompositorM = pTvGraphicsCompositor;
    pGfxProxy->pSharedGfxMemoryPoolM = pSharedGraphicsMemoryPool;
    pGfxProxy->pPrivateGfxMemoryPoolM = pPrivateGraphicsMemoryPool;

    pthread_mutex_init( &pGfxProxy->inputMutex, NULL );

    *phGfxProxy = TVGFXPROXY_OBJECT_TO_PROXY( GfxProxy );

    TvGfx_HpkInputOpen( *phGfxProxy );
    if (TmkFeatures::FEnabled(eTvFeatureStxIpc) == true)
    {
        TvGfx_IpcInputOpen2( *phGfxProxy );
    }
    else
    {
        TvGfx_IpcInputOpen( *phGfxProxy );
    }

    (void) TvGfxProxy_SetResolution( *phGfxProxy,
                                     TIVOGFX_HD_DISPLAY_WIDTH,
                                     TIVOGFX_HD_DISPLAY_HEIGHT );

    // switching to asynchronous mode
    (void) TvGraphicsCompositorSetAsynchronousMode( pTvGraphicsCompositor, true );

    // force H/W operations
    TvGraphicsCompositorDebug *debugCompositor =
        TvGraphicsCompositorGetDebugInterface( pTvGraphicsCompositor );

    ( void )debugCompositor->UseHardwareAccel( true, true );

    D_DEBUG_AT( tivogfx_proxy, "%s() %s\n", __FUNCTION__, "exit" );
    return true;
}

/**
 * TvGfxProxy_Release
 */
bool TvGfxProxy_Release( /* modifies */ TvGfxProxy* phGfxProxy )
{
    D_DEBUG_AT( tivogfx_proxy, "%s()\n", __FUNCTION__ );

    // make a local copy of the handle
    TvDebugAssert( phGfxProxy != NULL );
    TvGfxProxy hGfxProxy = *phGfxProxy;

    // NULL out the caller's copy
    *phGfxProxy = NULL;

    TVGFXPROXY_GET_OBJECT_FROM_PROXY( GfxProxy );

    if (TmkFeatures::FEnabled(eTvFeatureStxIpc) == true)
    {
        TvGfx_IpcInputClose2();
    }
    else
    {
        TvGfx_IpcInputClose();
    }
    TvGfx_HpkInputClose();

    pthread_mutex_destroy( &pGfxProxy->inputMutex );

    TvStatus status;

    status = TvGraphicsCompositorSetSurfaceMappings(
        pGfxProxy->pTvGraphicsCompositorM, 0, NULL, -1, NULL );

    TvDebugAssert( status == TV_OK );

    delete pGfxProxy->pPrivateGfxMemoryPoolM;
    pGfxProxy->pPrivateGfxMemoryPoolM = NULL;

    TvGraphicsCompositorRelease( pGfxProxy->pTvGraphicsCompositorM );
    pGfxProxy->pTvGraphicsCompositorM = NULL;

    delete pGfxProxy->pSharedGfxMemoryPoolM;
    pGfxProxy->pSharedGfxMemoryPoolM = NULL;

    TVGFXPROXY_FREE_OBJECT( GfxProxy );

    pTvGraphicsCompositorS = NULL;

    D_DEBUG_AT( tivogfx_proxy, "%s() %s\n", __FUNCTION__, "exit" );
    return true;
}

/**
 * TvGfxProxy_ShutdownGraphics
 */
void TvGfxProxy_ShutdownGraphics()
{
    if ( pTvGraphicsCompositorS != NULL )
    {
        TvGraphicsCompositor::Shutdown( pTvGraphicsCompositorS );
    }
}

/**
 * TvGfxProxy_GetNumScreenConfiguration
 */
bool TvGfxProxy_GetNumScreenConfiguration( TvGfxProxy hGfxProxy,
    /* returns */ U16* pNumConfigs )
{
    D_DEBUG_AT( tivogfx_proxy, "%s()\n", __FUNCTION__ );

    TvDebugAssert( pNumConfigs != NULL );

    // no return in case of error
    *pNumConfigs = 0;

    TVGFXPROXY_GET_OBJECT_FROM_PROXY( GfxProxy );

    U16 numConfigs = 0;

    TvGraphicsCompositorGetNumScreenConfigurations(
        pGfxProxy->pTvGraphicsCompositorM, &numConfigs );

    // sanity check
    if (! ((numConfigs > 0) &&
           (numConfigs <= TIVOGFX_MAX_SCREEN_CONFIGS)) )
    {
        TvDebugAssert( false,
            "TvGraphicsCompositorGetNumScreenConfigurations bogus" );
        return false;
    }

    *pNumConfigs = numConfigs;

    D_DEBUG_AT( tivogfx_proxy, "%s() %s\n", __FUNCTION__, "exit" );
    return true;
}

/**
 * TvGfxProxy_GetScreenConfigs
 */
bool TvGfxProxy_GetScreenConfigs( TvGfxProxy hGfxProxy,
    U16 numConfigs, /* returns */ HPK_ScreenConfig* pConfigs )
{
    D_DEBUG_AT( tivogfx_proxy, "%s()\n", __FUNCTION__ );

    TVGFXPROXY_GET_OBJECT_FROM_PROXY( GfxProxy );

    TvDebugAssert( numConfigs > 0 );
    TvDebugAssert( pConfigs != NULL );

    TvStatus status;

    status = TvGraphicsCompositorGetScreenConfigurations(
        pGfxProxy->pTvGraphicsCompositorM, numConfigs, pConfigs );

    if (status != TV_OK)
    {
        TvDebugAssert( false,
            "TvGraphicsCompositorGetScreenConfigurations failed" );
        return false;
    }

    D_DEBUG_AT( tivogfx_proxy, "%s() %s\n", __FUNCTION__, "exit" );
    return true;
}

/**
 * TvGfxProxy_SetResolution
 */
DFBResult TvGfxProxy_SetResolution( TvGfxProxy hGfxProxy,
    int width, int height )
{
    D_DEBUG_AT( tivogfx_proxy, "%s( w=%d, h=%d )\n", __FUNCTION__,
        width, height );

    TVGFXPROXY_GET_OBJECT_FROM_PROXY( GfxProxy );

    TvStatus status;

    U16 numConfigs = 0;

    TvGraphicsCompositorGetNumScreenConfigurations(
        pGfxProxy->pTvGraphicsCompositorM, &numConfigs );

    // sanity check
    if (! ((numConfigs > 0) &&
           (numConfigs <= TIVOGFX_MAX_SCREEN_CONFIGS)) )
    {
        TvDebugAssert( false,
            "TvGraphicsCompositorGetNumScreenConfigurations bogus" );
        return DFB_FAILURE;
    }

    HPK_ScreenConfig configs[ TIVOGFX_MAX_SCREEN_CONFIGS ];

    status = TvGraphicsCompositorGetScreenConfigurations(
        pGfxProxy->pTvGraphicsCompositorM, numConfigs, configs );

    if (status != TV_OK)
    {
        TvDebugAssert( false,
            "TvGraphicsCompositorGetScreenConfigurations failed" );
        return DFB_FAILURE;
    }

    const HPK_ScreenConfig* pConfig = NULL;

    // XXX may need to refine this for additional 'best match' criteria
    for (int i = 0; i < numConfigs; i++)
    {
        const HPK_ScreenConfig* pTestConfig = &configs[ i ];

        if ((pTestConfig->rect.w == width) &&
            (pTestConfig->rect.h == height))
        {
            pConfig = pTestConfig;
            break;
        }
    }

    if (pConfig == NULL)
    {
        TvDebugAssert( false,
            "ScreenConfiguration not found" );
        return DFB_INVARG;
    }

    status = TvGraphicsCompositorSetScreenConfiguration(
        pGfxProxy->pTvGraphicsCompositorM, pConfig );

    if (status != TV_OK)
    {
        TvDebugAssert( false,
            "TvGraphicsCompositorSetScreenConfiguration failed" );
        return DFB_FAILURE;
    }

    D_DEBUG_AT( tivogfx_proxy, "%s() %s\n", __FUNCTION__, "exit" );
    return DFB_OK;
}

/**
 * TvGfxProxy_TestPixelFormat
 */
DFBResult TvGfxProxy_TestPixelFormat( TvGfxProxy hGfxProxy,
     DFBSurfacePixelFormat format )
{
    D_DEBUG_AT( tivogfx_proxy, "%s()\n", __FUNCTION__ );

    TVGFXPROXY_GET_OBJECT_FROM_PROXY( GfxProxy );

    HPK_PixelFormat hpkPixelFormat =
        TvGfxProxy_GetHpkPixelFormat( format );

    if (hpkPixelFormat == HPK_PIXFMT_NONE)
    {
        return DFB_UNSUPPORTED;
    }

    return DFB_OK;
}

/**
 * TvGfxProxy_AllocateBuffer
 */
DFBResult TvGfxProxy_AllocateBuffer( TvGfxProxy hGfxProxy,
    /* modifies */ CoreSurfaceAllocation* allocation,
    /* returns */ TvGfxBuffer* phGfxBuffer )
{
    D_DEBUG_AT( tivogfx_proxy, "%s()\n", __FUNCTION__ );

    TvDebugAssert( phGfxBuffer != NULL );

    // no return in case of error
    *phGfxBuffer = NULL;

    TVGFXPROXY_GET_OBJECT_FROM_PROXY( GfxProxy );

    const CoreSurfaceBuffer*  buffer;
    const CoreSurface*        surface;
//  const CorePalette*        palette;

    buffer  = allocation->buffer;
    surface = buffer->surface;
//  palette = surface->palette;

    HPK_PixelFormat hpkPixelFormat =
        TvGfxProxy_GetHpkPixelFormat( buffer->format );

    if (hpkPixelFormat == HPK_PIXFMT_NONE)
    {
        D_ERROR( "%s: unsupported format %s\n",__FUNCTION__,
            dfb_pixelformat_name( buffer->format ) );

        return DFB_UNSUPPORTED;
    }

    int width   = surface->config.size.w;
    int height  = surface->config.size.h;

    TvStatus    status;
    TvSurface*  pTvSurface  = NULL;
    TvPixmap*   pTvPixmap   = NULL;

    TvGraphicsMemoryPool * pPool = pGfxProxy->pPrivateGfxMemoryPoolM;

    if (!pGfxProxy->fScratchSurfaceAllocated
        && width == TIVOGFX_HD_DISPLAY_WIDTH
        && height == TIVOGFX_HD_DISPLAY_HEIGHT
        && surface->type & CSTF_SHARED)
    {
        pPool = pGfxProxy->pSharedGfxMemoryPoolM;
        pGfxProxy->fScratchSurfaceAllocated = true;
    }

    status = TvGraphicsCompositorAllocateSurface(
        pGfxProxy->pTvGraphicsCompositorM,
        width,
        height,
        hpkPixelFormat,
        &pTvSurface,
        pPool
        );
    if (status != TV_OK)
    {
        D_WARN( "%s: TvGraphicsCompositorAllocateSurface failed %s\n",
                __FUNCTION__, TvStatusGetString( status ) );

        return DFB_LIMITEXCEEDED; // XXX what result to use?
    }

    status = TvSurfaceLock( pTvSurface );
    if (status != TV_OK)
    {
        D_ERROR( "%s: TvSurfaceLock failed %s\n",
            __FUNCTION__, TvStatusGetString( status ) );

        // XXX clean up
        return DFB_LIMITEXCEEDED; // XXX what result to use?
    }

    status = TvSurfaceGetPixmap( pTvSurface, &pTvPixmap );
    if (status != TV_OK)
    {
        D_ERROR( "%s: TvSurfaceGetPixmap failed %s\n",
            __FUNCTION__, TvStatusGetString( status ) );

        // XXX clean up
        return DFB_LIMITEXCEEDED; // XXX what result to use?
    }

    HPK_PixelData pixelData;
    memset( &pixelData, 0, sizeof(pixelData) );

    status = TvPixmapGetPixelData( pTvPixmap, &pixelData );
    if (status != TV_OK)
    {
        D_ERROR( "%s: TvSurfaceGetPixmap failed %s\n",
            __FUNCTION__, TvStatusGetString( status ) );

        // XXX clean up
        return DFB_LIMITEXCEEDED; // XXX what result to use?
    }

    // assuming these aren't padded
    TvDebugAssert( pixelData.width  == width );
    TvDebugAssert( pixelData.height == height );

    int numBytes = pixelData.stride * pixelData.height;

    TVGFXPROXY_ALLOCATE_OBJECT( GfxBuffer );

    pGfxBuffer->pTvSurfaceM = pTvSurface;
    pGfxBuffer->pTvPixmapM  = pTvPixmap;
    pGfxBuffer->pixelDataM  = pixelData;
    pGfxBuffer->writeAccessor = CSAID_NONE;

    // Clear the primary surface.
    if (surface->type & CSTF_SHARED)
    {
        HPK_Rect  hpkRect  = { 0, 0, width, height };
        HPK_Color hpkColor = { 0, 0, 0, 0 };

        status = TvSurfaceFill( pGfxBuffer->pTvSurfaceM,
                                &hpkRect,
                                hpkColor );

        if (status != TV_OK)
        {
            D_ERROR( "%s: TvSurfaceFill failed %s\n",
                     __FUNCTION__,
                     TvStatusGetString( status ) );
        }
    }

    // for debugging only
    if (pGfxProxy->fDebugInitSurfaces)
    {
        TvGfxInitArgbMemory( pixelData.data, width, height, pixelData.stride,
            0x80FF33CC );

        // Flush after software access.
        status = TvSurfaceFlush( pGfxBuffer->pTvSurfaceM );
        TvDebugAssert( status == TV_OK );
    }

    *phGfxBuffer = TVGFXPROXY_OBJECT_TO_PROXY( GfxBuffer );

    allocation->size = numBytes;

    D_DEBUG_AT( tivogfx_proxy, "%s() %s\n", __FUNCTION__, "exit" );
    return DFB_OK;
}

/**
 * TvGfxProxy_DeallocateBuffer
 */
DFBResult TvGfxProxy_DeallocateBuffer( TvGfxProxy hGfxProxy,
    /* modifies */ TvGfxBuffer* phGfxBuffer )
{
    D_DEBUG_AT( tivogfx_proxy, "%s()\n", __FUNCTION__ );

    TVGFXPROXY_GET_OBJECT_FROM_PROXY( GfxProxy );

    // make a local copy of the handle
    TvDebugAssert( phGfxBuffer != NULL );
    TvGfxBuffer hGfxBuffer = *phGfxBuffer;

    // NULL out the caller's copy
    *phGfxBuffer = NULL;

    TVGFXPROXY_GET_OBJECT_FROM_PROXY( GfxBuffer );

    TvStatus status;

    TvSurfaceReleasePixmap( pGfxBuffer->pTvSurfaceM, pGfxBuffer->pTvPixmapM );
    pGfxBuffer->pTvPixmapM = NULL;

    memset( &pGfxBuffer->pixelDataM, 0, sizeof(pGfxBuffer->pixelDataM) );

    status = TvSurfaceUnLock( pGfxBuffer->pTvSurfaceM );
    TvDebugAssert( status == TV_OK );

    status = TvGraphicsCompositorFreeSurface(
        pGfxProxy->pTvGraphicsCompositorM, pGfxBuffer->pTvSurfaceM );
    TvDebugAssert( status == TV_OK );
    pGfxBuffer->pTvSurfaceM = NULL;

    TVGFXPROXY_FREE_OBJECT( GfxBuffer );

    D_DEBUG_AT( tivogfx_proxy, "%s() %s\n", __FUNCTION__, "exit" );
    return DFB_OK;
}

/**
 * TvGfxProxy_LockBuffer
 */
DFBResult TvGfxProxy_LockBuffer( TvGfxProxy hGfxProxy,
    TvGfxBuffer hGfxBuffer,
    /* modifies */ CoreSurfaceBufferLock* lock )
{
    D_DEBUG_AT_X( tivogfx_proxy, "%s()\n", __FUNCTION__ );

    TVGFXPROXY_GET_OBJECT_FROM_PROXY( GfxProxy );

    TVGFXPROXY_GET_OBJECT_FROM_PROXY( GfxBuffer );

    D_MAGIC_ASSERT( lock, CoreSurfaceBufferLock );

    TvStatus status = TV_OK;

    if ( lock->accessor == CSAID_CPU )
    {
        if ( pGfxBuffer->writeAccessor == CSAID_GPU )
        {
            status = TvSurfaceFlush( pGfxBuffer->pTvSurfaceM );
            pGfxBuffer->writeAccessor = CSAID_NONE;
        }

        if ( lock->access & CSAF_WRITE )
        {
            pGfxBuffer->writeAccessor = CSAID_CPU;
        }
    }
    else
    {
        if ( pGfxBuffer->writeAccessor == CSAID_CPU )
        {
            status = TvSurfaceFlush( pGfxBuffer->pTvSurfaceM );
            pGfxBuffer->writeAccessor = CSAID_NONE;
        }

        if ( lock->access & CSAF_WRITE )
        {
            pGfxBuffer->writeAccessor = CSAID_GPU;
        }
    }

    TvDebugAssert( status == TV_OK );

    lock->pitch  = pGfxBuffer->pixelDataM.stride;
    lock->offset = 0;
    lock->addr   = pGfxBuffer->pixelDataM.data;
    lock->phys   = (unsigned long) pGfxBuffer->pixelDataM.data;

    lock->handle = reinterpret_cast< void* >( pGfxBuffer );

    D_DEBUG_AT_X( tivogfx_proxy, "%s() %s\n", __FUNCTION__, "exit" );
    return DFB_OK;
}

/**
 * TvGfxProxy_UnlockBuffer
 */
DFBResult TvGfxProxy_UnlockBuffer( TvGfxProxy hGfxProxy,
    TvGfxBuffer hGfxBuffer,
    /* modifies */ CoreSurfaceBufferLock* lock )
{
    D_DEBUG_AT_X( tivogfx_proxy, "%s()\n", __FUNCTION__ );

    TVGFXPROXY_GET_OBJECT_FROM_PROXY( GfxProxy );

    TVGFXPROXY_GET_OBJECT_FROM_PROXY( GfxBuffer );

    lock->handle = NULL;

    D_DEBUG_AT_X( tivogfx_proxy, "%s() %s\n", __FUNCTION__, "exit" );
    return DFB_OK;
}

/**
 * TvGfxProxy_Fill
 */
DFBResult TvGfxProxy_Fill( TvGfxProxy hGfxProxy,
    TvGfxBuffer hGfxBuffer,
    const DFBRectangle* rect,
    const DFBColor* color,
    const DFBSurfaceDrawingFlags dflags )
{
    D_DEBUG_AT( tivogfx_proxy, "%s()\n", __FUNCTION__ );

    TVGFXPROXY_GET_OBJECT_FROM_PROXY( GfxProxy );

    TVGFXPROXY_GET_OBJECT_FROM_PROXY( GfxBuffer );

    HPK_Rect  hpkRect;
    COPY_DFB_RECT_TO_HPK_RECT( rect, &hpkRect );

    HPK_Color hpkColor;
    COPY_DFB_COLOR_TO_HPK_COLOR( color, &hpkColor );

    D_DEBUG_AT_X( tivogfx_proxy_ops, "%s: color=0x%08X dst={%d,%d,%dx%d}\n",
        __FUNCTION__,
        ((hpkColor.a << 24) | (hpkColor.r << 16) | (hpkColor.g << 8) | hpkColor.b),
        hpkRect.x, hpkRect.y, hpkRect.w, hpkRect.h
        );

    TvStatus status;
    if (dflags & DSDRAW_BLEND)
    {
        status = TvSurfaceBlendedFill( pGfxBuffer->pTvSurfaceM,
                                       &hpkRect,
                                       hpkColor,
                                       true );
    }
    else
    {
        status = TvSurfaceFill( pGfxBuffer->pTvSurfaceM,
                                &hpkRect,
                                hpkColor );
    }

    TvDebugAssert( status == TV_OK );

    if (TmkProcessLog::FAvailable())
    {
        unsigned int fill_size[4] = { hpkRect.w, hpkRect.h, hpkRect.x, hpkRect.y };

        TmkProcessLog::Log( PLOG_TYPE_DFB_TVGFX_FILL, sizeof(fill_size), fill_size);
    }

    return DFB_OK;
}

/**
 * TvGfxProxy_Blit
 */
DFBResult TvGfxProxy_Blit( TvGfxProxy hGfxProxy,
    TvGfxBuffer hGfxBuffer,
    TvGfxBuffer hGfxBufferSrc,
    const DFBRectangle* dstRect,
    const DFBRectangle* srcRect,
    const DFBColor* color,
    const DFBSurfaceBlittingFlags blitFlags,
    const DFBColor* srcColorkey,
    const DFBColor* dstColorkey,
    const DFBSurfaceBlendFunction srcBlend,
    const DFBSurfaceBlendFunction dstBlend,
    const DFBSurfaceRenderOptions renderOpts)
{
    int             flip        = HPK_FLIP_NONE;
    u32             mode        = TvGfxProxy_ConvertFlagsDfb2Hpk( blitFlags );
    TvStatus        result      = TV_OK;
    HPK_PixelFormat pixFormat;
    bool            fDoubleBlit = false;
    bool            fIsAlphaOnly;
    bool            fStretchBlit = false;

    D_DEBUG_AT( tivogfx_proxy, "%s()\n", __FUNCTION__ );

    TVGFXPROXY_GET_OBJECT_FROM_PROXY( GfxProxy );

    TVGFXPROXY_GET_OBJECT_FROM_PROXY( GfxBuffer );

    TVGFXPROXY_IMPL_TYPE( GfxBuffer )* pGfxBufferSrc =
        reinterpret_cast< TVGFXPROXY_IMPL_TYPE( GfxBuffer )* >( hGfxBufferSrc );

    D_MAGIC_ASSERT( pGfxBufferSrc, TVGFXPROXY_IMPL_TYPE( GfxBuffer ) );

    HPK_Rect  hpkDstRect;
    COPY_DFB_RECT_TO_HPK_RECT( dstRect, &hpkDstRect );

    HPK_Rect  hpkSrcRect;
    COPY_DFB_RECT_TO_HPK_RECT( srcRect, &hpkSrcRect );

    HPK_Color hpkColor;
    COPY_DFB_COLOR_TO_HPK_COLOR( color, &hpkColor );

    HPK_BlitFilter pHpkBlitFilter = HPK_BLIT_FILTER_POINTSAMPLE;
    TvGfxProxy_CopyDfbRenderOpts2HpkBlitfilter( srcRect,
                                                dstRect,
                                                renderOpts,
                                                &pHpkBlitFilter );

    D_DEBUG_AT_X( tivogfx_proxy_ops, "%s: src={%d,%d,%dx%d} "
        "dst={%d,%d,%dx%d} color=0x%08X sbl=%d dbl=%d rOpts=%d\n",
        __FUNCTION__,
        hpkSrcRect.x, hpkSrcRect.y, hpkSrcRect.w, hpkSrcRect.h,
        hpkDstRect.x, hpkDstRect.y, hpkDstRect.w, hpkDstRect.h,
        ((hpkColor.a << 24) | (hpkColor.r << 16) | (hpkColor.g << 8) | hpkColor.b),
        srcBlend, dstBlend, renderOpts );

    if ( (mode & HPK_BLIT_FLAG_ROTATE180) &&
            ((mode & HPK_BLIT_FLAG_FLIP_HORIZONTAL) || (mode & HPK_BLIT_FLAG_FLIP_VERTICAL)) )
    {
        D_DEBUG_AT_X( tivogfx_proxy,
            "%s: Unsupported set of blitting flags - %#x\n",
            __FUNCTION__, mode );

        return DFB_NOIMPL;
    }

    TV_CALL( pGfxBufferSrc->pTvSurfaceM->GetPixelFormat( &pixFormat ));

    fIsAlphaOnly = ( pixFormat == HPK_PIXFMT_ALPHA_8 ) ||
                   ( pixFormat == HPK_PIXFMT_ALPHA_4 ) ||
                   ( pixFormat == HPK_PIXFMT_ALPHA_2 ) ||
                   ( pixFormat == HPK_PIXFMT_ALPHA_1 ) ||
                   ( pixFormat == HPK_PIXFMT_ALPHA_0 );

    /* A set of BLEND_ALPHACHANNEL + SRC_PREMULTIPLY (0x21)
     * when the srcBlend = HPK_BFACTOR_ONE can be processed 
     * in a single blit:
     *
     * Two passes blit use formula:
     *     Oa = Sa
     *     Oc = Sc*Sa - for the 1-st pass
     *
     *     Oa = 1*Sa + (1-Sa)*Da
     *     Oc = 1*Sc + (1-Sa)*Dc - for the 2-nd pass
     *
     * After optimization the blending formula will be the following:
     *     Oa = 1*Sa  + (1-Sa)*Da
     *     Oc = Sa*Sc + (1-Sa)*Dc
     */
    if ((mode & HPK_BLIT_FLAG_BLEND_ALPHACHANNEL) &&
        (mode & HPK_BLIT_FLAG_SRC_PREMULTIPLY))
    {
        if (srcBlend != DSBF_ONE)
        {
            fDoubleBlit = true;
        }
    }

    // Not to use the TvSurfaceCopy() operation for the stretch blit
    if ((srcRect->w != dstRect->w) || (srcRect->h != dstRect->h))
    {
        fStretchBlit = true;
    }

    // Alpha surface should processed in TvSurfaceBlit()
    if ((mode == HPK_BLIT_FLAG_NOFX) && (!fIsAlphaOnly) && !fStretchBlit)
    {
        TV_CALL( TvSurfaceCopy( pGfxBuffer->pTvSurfaceM,
                                pGfxBufferSrc->pTvSurfaceM,
                                &hpkDstRect,
                                &hpkSrcRect ) );
    }
    else
    {
        HPK_Color srcColorkeyHpk = { 0, 0, 0, 0 };
        HPK_Color dstColorkeyHpk = { 0, 0, 0, 0 };

        mode |= HPK_BLEND_USE_BLIT_FLAGS_MODE;

        if (mode & HPK_BLIT_FLAG_SRC_COLORKEY)
        {
            COPY_DFB_COLOR_TO_HPK_COLOR( srcColorkey, &srcColorkeyHpk );
        }

        if (mode & HPK_BLIT_FLAG_DST_COLORKEY)
        {
            COPY_DFB_COLOR_TO_HPK_COLOR( dstColorkey, &dstColorkeyHpk );
        }

        if (!fDoubleBlit)
        {
            TV_CALL( TvSurfaceBlit( pGfxBuffer->pTvSurfaceM,
                                    pGfxBufferSrc->pTvSurfaceM,
                                    &hpkDstRect,
                                    &hpkSrcRect,
                                    hpkColor,
                                    ( HPK_BlendMode )mode,
                                    pHpkBlitFilter,
                                    NULL,
                                    ( HPK_FlipMode )flip,
                                    TvGfxProxy_ConvertDfb2HpkBlendMode( srcBlend ),
                                    TvGfxProxy_ConvertDfb2HpkBlendMode( dstBlend ),
                                    srcColorkeyHpk,
                                    dstColorkeyHpk ) );
        }
        else
        {
            /* Unfortunately NEXUS can't provide the premultiply operation
             * together with alpha blending, performed in a single iteration.
             * Thus a set of operations DSBLIT_BLEND_ALPHACHANNEL+DSBLIT_SRC_PREMULTIPLY
             * is splitting on two separate ones with involving supplementary
             * surface to keep the premultiplied surface. The same mechanism
             * has been implemented in the reference software,
             * see bcmnexus_gfx.c */

            u32             firstBlitMode;
            TvSurface*      pTvSurface  = NULL;
            HPK_PixelFormat pixfmt      = HPK_PIXFMT_NONE;
            HPK_Rect        pTvRect     = { 0, 0, hpkSrcRect.w, hpkSrcRect.h };

            // will created the same type of surface as the destination is
            TV_CALL( pGfxBuffer->pTvSurfaceM->GetPixelFormat( &pixfmt ));

            if (TV_FAILED( TvGraphicsCompositorAllocateSurface(
                   pGfxProxy->pTvGraphicsCompositorM,
                   hpkSrcRect.w,
                   hpkSrcRect.h,
                   pixfmt,
                   &pTvSurface,
                   pGfxProxy->pPrivateGfxMemoryPoolM )))
            {
               D_ERROR( "%s: Can't allocate supplementary surface for two "
                       "blit operation\n", __FUNCTION__ );
               return DFB_LIMITEXCEEDED;
            }

           TV_CALL( TvSurfaceLock( pTvSurface ) );

           // do premultiply in the first blit
           firstBlitMode = HPK_BLIT_FLAG_SRC_PREMULTIPLY | HPK_BLEND_USE_BLIT_FLAGS_MODE;

           TV_CALL( TvSurfaceBlit( pTvSurface,
                                   pGfxBufferSrc->pTvSurfaceM,
                                   &pTvRect,
                                   &hpkSrcRect,
                                   hpkColor,
                                   ( HPK_BlendMode )firstBlitMode,
                                   HPK_BLIT_FILTER_POINTSAMPLE,
                                   NULL,
                                   ( HPK_FlipMode )flip ,
                                   TvGfxProxy_ConvertDfb2HpkBlendMode( srcBlend ),
                                   TvGfxProxy_ConvertDfb2HpkBlendMode( dstBlend ),
                                   srcColorkeyHpk,
                                   dstColorkeyHpk ) );

           // do alpha blending in the second blit, filter out the premultiply flag
           mode &= ~HPK_BLIT_FLAG_SRC_PREMULTIPLY;

           TV_CALL( TvSurfaceBlit( pGfxBuffer->pTvSurfaceM,
                                   pTvSurface,
                                   &hpkDstRect,
                                   &pTvRect,
                                   hpkColor,
                                   ( HPK_BlendMode )mode,
                                   pHpkBlitFilter,
                                   NULL,
                                   ( HPK_FlipMode )flip ,
                                   TvGfxProxy_ConvertDfb2HpkBlendMode( srcBlend ),
                                   TvGfxProxy_ConvertDfb2HpkBlendMode( dstBlend ),
                                   srcColorkeyHpk,
                                   dstColorkeyHpk ) );

           TV_CALL( TvSurfaceUnLock( pTvSurface ));

           TV_CALL( TvGraphicsCompositorFreeSurface(
               pGfxProxy->pTvGraphicsCompositorM,
               pTvSurface ));
        }
    }

    if (TmkProcessLog::FAvailable())
    {
        unsigned int blit_size[6] = { hpkSrcRect.w, hpkSrcRect.h,
                                      hpkDstRect.w, hpkDstRect.h,
                                      hpkDstRect.x, hpkDstRect.y };
        TmkProcessLog::Log( PLOG_TYPE_DFB_TVGFX_BLIT, sizeof(blit_size), blit_size);
    }

    return result == TV_OK ? DFB_OK : DFB_FAILURE;
}

/**
 * TvGfxProxy_Render
 */
DFBResult TvGfxProxy_Render( TvGfxProxy hGfxProxy,
    const CoreSurfaceBufferLock* lock, const DFBRectangle* clip )
{
    if (TmkProcessLog::FAvailable())
    {
        TmkProcessLog::Log( PLOG_TYPE_DFB_TVGFX_RENDER_ENTER, 0, NULL);
    }

    D_DEBUG_AT( tivogfx_proxy, "%s()\n", __FUNCTION__ );

    TVGFXPROXY_GET_OBJECT_FROM_PROXY( GfxProxy );

    TVGFXPROXY_IMPL_TYPE( GfxBuffer )* pGfxBuffer =
        reinterpret_cast< TVGFXPROXY_IMPL_TYPE( GfxBuffer )* >( lock->handle );

    D_MAGIC_ASSERT( pGfxBuffer, TVGFXPROXY_IMPL_TYPE( GfxBuffer ) );

    TvStatus status;

    const int kMaxMappings = 2;
    TvSurfaceMapping vSurfaceMaps[ kMaxMappings ];
    const TvSurfaceMapping* vMappings[ kMaxMappings ];

    int nMappings = 0;

    //========================================================================

    // add the video surface mapping (if enabled)
    if ( pGfxProxy->fMapVideo || pGfxProxy->fMapVideoOnceM )
    {
        TvDebugAssert( nMappings < kMaxMappings );
        TvSurfaceMapping& vidMapping = vSurfaceMaps[ nMappings ];
        memset( &vidMapping, 0, sizeof(vidMapping) );

        vidMapping.id              = nMappings;
        vidMapping.surface         = NULL;
        vidMapping.isSurfaceOpaque = HPK_TRUE;
        vidMapping.x               = pGfxProxy->videoPlaneRectM.x;
        vidMapping.y               = pGfxProxy->videoPlaneRectM.y;
        vidMapping.alpha           = 0xFF;
        //
        // XXX hard-coding screen dims to start
        vidMapping.clip.x          = pGfxProxy->videoPlaneRectM.x;
        vidMapping.clip.y          = pGfxProxy->videoPlaneRectM.y;
        vidMapping.clip.w          = pGfxProxy->videoPlaneRectM.w;
        vidMapping.clip.h          = pGfxProxy->videoPlaneRectM.h;
        //
        vidMapping.fgcolor.a       = 0xFF;
        vidMapping.fgcolor.r       = 0;
        vidMapping.fgcolor.g       = 0;
        vidMapping.fgcolor.b       = 0;
        float widthScaleFactor = (float)TIVOGFX_HD_DISPLAY_WIDTH/(float)pGfxProxy->videoPlaneRectM.w;
        float heightScaleFactor = (float)TIVOGFX_HD_DISPLAY_HEIGHT/(float)pGfxProxy->videoPlaneRectM.h;
        vidMapping.xScale          = (int long)(HPK_S_16_DOT_16_ONE / widthScaleFactor );
        vidMapping.yScale          = (int long)(HPK_S_16_DOT_16_ONE / heightScaleFactor );
        vMappings[ nMappings ] = &vidMapping;
        nMappings++;

        // we've performed one video mapping
        pGfxProxy->fMapVideoOnceM = false;

        D_DEBUG_AT_X(tivogfx_proxy_ops, " Scale Factor Width(%08X): %f Scale Factor Height(%08X): %f\n",
            (unsigned int)vidMapping.xScale,
            (float)HPK_S_16_DOT_16_ONE/(float)vidMapping.xScale,
            (unsigned int)vidMapping.yScale,
            (float)HPK_S_16_DOT_16_ONE/(float)vidMapping.yScale);

    }

    //========================================================================

    // add the graphics surface mapping
    {
        TvDebugAssert( nMappings < kMaxMappings );
        TvSurfaceMapping& gfxMapping = vSurfaceMaps[ nMappings ];
        memset( &gfxMapping, 0, sizeof(gfxMapping) );

        // XXX Note: pTvSurfaceM needs to be locked for the
        // TvGraphicsCompositorSetSurfaceMappings call.
        // For now, that is taken care of since we leave it locked
        // all the time, but that needs to be fixed in the future.

        gfxMapping.id              = nMappings;
        gfxMapping.surface         = pGfxBuffer->pTvSurfaceM;
        gfxMapping.isSurfaceOpaque = HPK_TRUE;
        gfxMapping.x               = 0;
        gfxMapping.y               = 0;
        gfxMapping.alpha           = 0xFF;
        //
        // XXX hard-coding screen dims to start
        gfxMapping.clip.x          = 0;
        gfxMapping.clip.y          = 0;
        gfxMapping.clip.w          = TIVOGFX_HD_DISPLAY_WIDTH;
        gfxMapping.clip.h          = TIVOGFX_HD_DISPLAY_HEIGHT;
        //
        gfxMapping.fgcolor.a       = 0xFF;
        gfxMapping.fgcolor.r       = 0;
        gfxMapping.fgcolor.g       = 0;
        gfxMapping.fgcolor.b       = 0;
        gfxMapping.xScale          = HPK_S_16_DOT_16_ONE;
        gfxMapping.yScale          = HPK_S_16_DOT_16_ONE;

        vMappings[ nMappings ] = &gfxMapping;
        nMappings++;
    }

    //========================================================================

    const int nRects = 1;
    const HPK_Rect* damageRects[ nRects ];
    HPK_Rect rect;
    rect.x = clip->x;
    rect.y = clip->y;
    rect.w = clip->w;
    rect.h = clip->h;
    damageRects[ 0 ] = &rect;

    status = TvGraphicsCompositorSetSurfaceMappings(
        pGfxProxy->pTvGraphicsCompositorM,
        nMappings, vMappings, nRects, damageRects );
    TvDebugAssert( status == TV_OK );

    D_DEBUG_AT( tivogfx_proxy, "%s() %s\n", __FUNCTION__, "exit" );

    if (TmkProcessLog::FAvailable())
    {
        TmkProcessLog::Log( PLOG_TYPE_DFB_TVGFX_RENDER_EXIT, 0, NULL);
    }

    return DFB_OK;
}

/**
 * TvGfxProxy_GetInputEvent
 */
bool TvGfxProxy_GetInputEvent( TvGfxProxy hGfxProxy,
    DFBInputEvent* pEvent )
{
    //D_DEBUG_AT( tivogfx_proxy, "%s()\n", __FUNCTION__ );

    TVGFXPROXY_GET_OBJECT_FROM_PROXY( GfxProxy );

    bool result = false;

    pthread_mutex_lock( &pGfxProxy->inputMutex );

    if (pGfxProxy->numInputEvents > 0)
    {
        *pEvent = pGfxProxy->inputEvents[ 0 ];
        result = true;

        int lastEvent = pGfxProxy->numInputEvents - 1;

        for (int i = 0; i < lastEvent; i++)
        {
            pGfxProxy->inputEvents[ i ] = pGfxProxy->inputEvents[ i + 1 ];
        }

        memset( &pGfxProxy->inputEvents[ lastEvent ], 0,
            sizeof( pGfxProxy->inputEvents[ lastEvent ] ) );

        pGfxProxy->numInputEvents--;
    }

    pthread_mutex_unlock( &pGfxProxy->inputMutex );

    return result;
}

/**
 * TvGfxProxy_PutInputEvent
 */
bool TvGfxProxy_PutInputEvent( TvGfxProxy hGfxProxy,
    const DFBInputEvent* pEvent )
{
    //D_DEBUG_AT( tivogfx_proxy, "%s()\n", __FUNCTION__ );

    TVGFXPROXY_GET_OBJECT_FROM_PROXY( GfxProxy );

    bool result = false;

    pthread_mutex_lock( &pGfxProxy->inputMutex );

    if ((pEvent->type == DIET_KEYPRESS) &&
        (pEvent->flags & DIEF_REPEAT) &&
        (pGfxProxy->numInputEvents >= 1) &&
        (pGfxProxy->inputEvents[ pGfxProxy->numInputEvents - 1 ].type == DIET_KEYPRESS) &&
        (pGfxProxy->inputEvents[ pGfxProxy->numInputEvents - 1 ].flags & DIEF_REPEAT))
    {
        // If this key event is a repeat and so was the last key event,
        // just drop this one.
        // That is so we don't fill up the event queue with key repeat events.
        // It the application can't process them as fast as they occur we
        // don't want a big backlog.
        D_WARN( "%s: discarding duplicate key repeat event\n",
            __FUNCTION__ );
    }
    else if ((pEvent->type == DIET_KEYPRESS) &&
        (pGfxProxy->numInputEvents >= TIVOGFX_MAX_INPUT_EVENTS - 1))
    {
        // Try to make sure we don't add a press event without
        // leaving room for a subsequent release.
        D_ERROR( "%s: event queue is full, discarding!\n",
            __FUNCTION__ );
    }
    else if (pGfxProxy->numInputEvents >= TIVOGFX_MAX_INPUT_EVENTS)
    {
        D_ERROR( "%s: event queue is full, discarding!\n",
            __FUNCTION__ );
    }
    else
    {
        pGfxProxy->inputEvents[ pGfxProxy->numInputEvents ] = *pEvent;
        pGfxProxy->numInputEvents++;
    }

    pthread_mutex_unlock( &pGfxProxy->inputMutex );

    return result;
}

/**
 * TvGfxProxy_SetColorMap
 */
DFBResult TvGfxProxy_SetColorMap( TvGfxProxy hGfxProxy,
    TvGfxBuffer hGfxBuffer,
    const CorePalette* palette )
{
    TvStatus status = TV_OK;

    D_DEBUG_AT( tivogfx_proxy, "%s()\n", __FUNCTION__ );

    TVGFXPROXY_GET_OBJECT_FROM_PROXY( GfxProxy );

    TVGFXPROXY_GET_OBJECT_FROM_PROXY( GfxBuffer );

    D_MAGIC_ASSERT( pGfxBuffer, TVGFXPROXY_IMPL_TYPE( GfxBuffer ) );

    if (NULL != palette && palette->num_entries > 0)
    {
        HPK_ColorMap colorMap;
        colorMap.size = palette->num_entries;
        colorMap.data = ( HPK_UInt32* )TvGfxProxy_Malloc( sizeof( HPK_UInt32 ) * colorMap.size );

        if (NULL != colorMap.data)
        {
            for (int i = 0; i < colorMap.size; i++)
            {
                COPY_DFB_COLOR_TO_HPK_COLOR( &palette->entries[i],
                    ( HPK_Color* )&colorMap.data[i] );
            }

            status = TvSurfaceSetColorMap( pGfxBuffer->pTvSurfaceM, &colorMap );
            TvGfxProxy_Free( colorMap.data );
        }
    }

    TvDebugAssert( status == TV_OK );
    return DFB_OK;
}

/**
 * TvGfxProxy_Sync makes sure that graphics hardware
 * has finished all operations
 */
DFBResult TvGfxProxy_Sync( TvGfxProxy hGfxProxy )
{
    if (TmkProcessLog::FAvailable())
    {
        TmkProcessLog::Log( PLOG_TYPE_DFB_TVGFX_SYNC, 0, NULL);
    }

    D_DEBUG_AT( tivogfx_proxy, "%s()\n", __FUNCTION__ );

    TVGFXPROXY_GET_OBJECT_FROM_PROXY( GfxProxy );

    TvStatus status = TvGraphicsCompositorWaitForCompletion(
        pGfxProxy->pTvGraphicsCompositorM, SYNC_TIMEOUT_500MS );

    TvDebugAssert( status == TV_OK );

    return DFB_OK;
}

/**
 * TvGfxProxy_AcquireBackbuffer - Get HPK backbuffer as if it allocated in
 *  the common memory pool.
 */
DFBResult TvGfxProxy_AcquireBackbuffer( TvGfxProxy hGfxProxy,
    /* modifies */ CoreSurfaceAllocation* allocation,
    /* modifies*/ TvGfxBuffer* phGfxBuffer )
{
    TvDebugAssert( phGfxBuffer != NULL );

    // no return in case of error
    *phGfxBuffer = NULL;

    TVGFXPROXY_GET_OBJECT_FROM_PROXY( GfxProxy );

    TvStatus    result      = TV_OK;
    TvSurface*  pTvSurface  = NULL;
    TvPixmap*   pTvPixmap   = NULL;

    TV_CALL(TvGraphicsCompositorAcquireBackbuffer(pGfxProxy->pTvGraphicsCompositorM, &pTvSurface ));
    TV_CALL(TvSurfaceGetPixmap( pTvSurface, &pTvPixmap ));

    HPK_PixelData pixelData;
    memset( &pixelData, 0, sizeof(pixelData) );

    TV_CALL(TvPixmapGetPixelData( pTvPixmap, &pixelData ));

    if (result == TV_OK)
    {
        TVGFXPROXY_ALLOCATE_OBJECT( GfxBuffer );
        pGfxBuffer->pTvSurfaceM = pTvSurface;
        pGfxBuffer->pTvPixmapM  = pTvPixmap;
        pGfxBuffer->pixelDataM  = pixelData;
        pGfxBuffer->writeAccessor = CSAID_NONE;

        *phGfxBuffer = TVGFXPROXY_OBJECT_TO_PROXY( GfxBuffer );

        allocation->size = pixelData.stride * pixelData.height;

        D_DEBUG_AT( tivogfx_proxy, "%s() success, buffer=%p\n", __FUNCTION__, pTvSurface);

        if (TmkProcessLog::FAvailable())
        {
            TmkProcessLog::Log( PLOG_TYPE_DFB_TVGFX_ACQBB, 0, NULL);
        }

        return DFB_OK;
    }
    else
    {
        D_DEBUG_AT( tivogfx_proxy, "%s() failed\n", __FUNCTION__ );

        if (TmkProcessLog::FAvailable())
        {
            TmkProcessLog::Log( PLOG_TYPE_DFB_TVGFX_ACQBB, 0, NULL);
        }

        return DFB_FAILURE;
    }
}

/**
 * TvGfxProxy_ReleaseBackbuffer - Releases HPK backbuffer.
 */
void TvGfxProxy_ReleaseBackbuffer(TvGfxProxy hGfxProxy, TvGfxBuffer* phGfxBuffer)
{
    D_DEBUG_AT( tivogfx_proxy, "%s()\n", __FUNCTION__ );

    TVGFXPROXY_GET_OBJECT_FROM_PROXY( GfxProxy );

    // make a local copy of the handle
    TvDebugAssert( phGfxBuffer != NULL );
    TvGfxBuffer hGfxBuffer = *phGfxBuffer;

    // NULL out the caller's copy
    *phGfxBuffer = NULL;

    TVGFXPROXY_GET_OBJECT_FROM_PROXY( GfxBuffer );

    TvStatus result = TV_OK;

    TvSurfaceReleasePixmap( pGfxBuffer->pTvSurfaceM, pGfxBuffer->pTvPixmapM );
    pGfxBuffer->pTvPixmapM = NULL;

    memset( &pGfxBuffer->pixelDataM, 0, sizeof(pGfxBuffer->pixelDataM) );

    TV_CALL(TvGraphicsCompositorReleaseBackbuffer(pGfxProxy->pTvGraphicsCompositorM));

    if (result == TV_OK)
    {
        pGfxBuffer->pTvSurfaceM = NULL;
    }

    TVGFXPROXY_FREE_OBJECT( GfxBuffer );
}

/**
 * TvGfxProxy_AttachVideo - Set video plane mapping coordinates
 */
void TvGfxProxy_AttachVideo(TvGfxProxy hGfxProxy,
        const DFBRectangle* rect)
{
    TvDebugAssert( rect != NULL );

    TVGFXPROXY_GET_OBJECT_FROM_PROXY( GfxProxy );

    pGfxProxy->fMapVideo = rect->x || rect->y || rect->w || rect->h; // if all 0 then false...
    pGfxProxy->videoPlaneRectM.x = rect->x;
    pGfxProxy->videoPlaneRectM.y = rect->y;
    pGfxProxy->videoPlaneRectM.w = rect->w;
    pGfxProxy->videoPlaneRectM.h = rect->h;

    // Do a surface mapping if the video is being mapped full screen.  Doing
    // it when the video is being set to any size other than full screen will
    // cause those portions of the screen not covered by the video window to
    // be cleared to black, until the next call to set surface mappings (that
    // happens on the next render), and this is undesirable.  But if the video
    // surface is being mapped to the whole screen, there is nothing that
    // would be drawn black anyway and so it's safe to do it now, and also
    // ensures that any DirectFB program that is setting video mapping to full
    // screen before doing any active rendering, will get full screen video
    // (this helps with the Long Play opener)
    if ( ( rect->x == 0 ) &&
         ( rect->y == 0 ) &&
         ( rect->w == TIVOGFX_HD_DISPLAY_WIDTH ) &&
         ( rect->h == TIVOGFX_HD_DISPLAY_HEIGHT ) )
    {
        TvStatus status;

        int nMappings = 0;

        const int kMaxMappings = 1;
        TvSurfaceMapping vSurfaceMaps[ kMaxMappings ];
        const TvSurfaceMapping* vMappings[ kMaxMappings ];

        TvDebugAssert( nMappings < kMaxMappings );
        TvSurfaceMapping& vidMapping = vSurfaceMaps[ nMappings ];
        memset( &vidMapping, 0, sizeof(vidMapping) );

        vidMapping.id              = nMappings;
        vidMapping.surface         = NULL;
        vidMapping.isSurfaceOpaque = HPK_TRUE;
        vidMapping.x               = pGfxProxy->videoPlaneRectM.x;
        vidMapping.y               = pGfxProxy->videoPlaneRectM.y;
        vidMapping.alpha           = 0xFF;
        //
        // XXX hard-coding screen dims to start
        vidMapping.clip.x          = pGfxProxy->videoPlaneRectM.x;
        vidMapping.clip.y          = pGfxProxy->videoPlaneRectM.y;
        vidMapping.clip.w          = pGfxProxy->videoPlaneRectM.w;
        vidMapping.clip.h          = pGfxProxy->videoPlaneRectM.h;
        //
        vidMapping.fgcolor.a       = 0xFF;
        vidMapping.fgcolor.r       = 0;
        vidMapping.fgcolor.g       = 0;
        vidMapping.fgcolor.b       = 0;
        float widthScaleFactor = (float)TIVOGFX_HD_DISPLAY_WIDTH/(float)pGfxProxy->videoPlaneRectM.w;
        float heightScaleFactor = (float)TIVOGFX_HD_DISPLAY_HEIGHT/(float)pGfxProxy->videoPlaneRectM.h;
        vidMapping.xScale          = (int long)(HPK_S_16_DOT_16_ONE / widthScaleFactor );
        vidMapping.yScale          = (int long)(HPK_S_16_DOT_16_ONE / heightScaleFactor );

        vMappings[ nMappings ] = &vidMapping;
        nMappings++;

        D_DEBUG_AT_X(tivogfx_proxy_ops, " Scale Factor Width(%08X): %f Scale Factor Height(%08X): %f\n",
            (unsigned int)vidMapping.xScale,
            (float)HPK_S_16_DOT_16_ONE/(float)vidMapping.xScale,
            (unsigned int)vidMapping.yScale,
            (float)HPK_S_16_DOT_16_ONE/(float)vidMapping.yScale);

        const int nRects = 1;
        const HPK_Rect* damageRects[ nRects ];
        HPK_Rect rect;
        rect.x = pGfxProxy->videoPlaneRectM.x;
        rect.y = pGfxProxy->videoPlaneRectM.y;
        rect.w = pGfxProxy->videoPlaneRectM.w;
        rect.h = pGfxProxy->videoPlaneRectM.h;
        damageRects[ 0 ] = &rect;

        status = TvGraphicsCompositorSetSurfaceMappings(
            pGfxProxy->pTvGraphicsCompositorM,
            nMappings, vMappings, nRects, damageRects );
        TvDebugAssert( status == TV_OK );
    }
}

/**
 * TvGfxProxy_DetachVideo - Unmap video plane mapping
 */
void TvGfxProxy_DetachVideo(TvGfxProxy hGfxProxy)
{
    TVGFXPROXY_GET_OBJECT_FROM_PROXY( GfxProxy );

    pGfxProxy->fMapVideo = false;
    pGfxProxy->videoPlaneRectM.x = 0;
    pGfxProxy->videoPlaneRectM.y = 0;
    pGfxProxy->videoPlaneRectM.w = 0;
    pGfxProxy->videoPlaneRectM.h = 0;
}

/**
 * TvGfxProxy_ConvertDfb2HpkMode converts DirectFB blitting flags into
 *     HPK blitting flags.
 */
static
HPK_BlitFlag TvGfxProxy_ConvertFlagsDfb2Hpk( DFBSurfaceBlittingFlags blitFlags )
{
    u32 hpkFlags    = 0;
    u32 mask        = 1;
    u8 i;

    for (i = 0; i < sizeof(HPK_BlitFlag) * 8; i++)  // number of bits
    {
        DFBSurfaceBlittingFlags flag = (DFBSurfaceBlittingFlags)(blitFlags & mask);
        mask <<= 1;

        switch (flag)
        {
            case DSBLIT_NOFX:
                hpkFlags |= HPK_BLIT_FLAG_NOFX;
                break;
            case DSBLIT_BLEND_ALPHACHANNEL:
                hpkFlags |= HPK_BLIT_FLAG_BLEND_ALPHACHANNEL;
                break;
            case DSBLIT_BLEND_COLORALPHA:
                hpkFlags |= HPK_BLIT_FLAG_BLEND_COLORALPHA;
                break;
            case DSBLIT_COLORIZE:
                hpkFlags |= HPK_BLIT_FLAG_COLORIZE;
                break;
            case DSBLIT_SRC_COLORKEY:
                hpkFlags |= HPK_BLIT_FLAG_SRC_COLORKEY;
                break;
            case DSBLIT_DST_COLORKEY:
                hpkFlags |= HPK_BLIT_FLAG_DST_COLORKEY;
                break;
            case DSBLIT_SRC_PREMULTIPLY:
                hpkFlags |= HPK_BLIT_FLAG_SRC_PREMULTIPLY;
                break;
            case DSBLIT_DST_PREMULTIPLY:
                hpkFlags |= HPK_BLIT_FLAG_DST_PREMULTIPLY;
                break;
            case DSBLIT_DEMULTIPLY:
                hpkFlags |= HPK_BLIT_FLAG_DEMULTIPLY;
                break;
            case DSBLIT_DEINTERLACE:
                hpkFlags |= HPK_BLIT_FLAG_DEINTERLACE;
                break;
            case DSBLIT_SRC_PREMULTCOLOR:
                hpkFlags |= HPK_BLIT_FLAG_SRC_PREMULTCOLOR;
                break;
            case DSBLIT_XOR:
                hpkFlags |= HPK_BLIT_FLAG_XOR;
                break;
            case DSBLIT_INDEX_TRANSLATION:
                hpkFlags |= HPK_BLIT_FLAG_INDEX_TRANSLATION;
                break;
            case DSBLIT_ROTATE90:
                hpkFlags |= HPK_BLIT_FLAG_ROTATE90;
                break;
            case DSBLIT_ROTATE180:
                hpkFlags |= HPK_BLIT_FLAG_ROTATE180;
                break;
            case DSBLIT_ROTATE270:
                hpkFlags |= HPK_BLIT_FLAG_ROTATE270;
                break;
            case DSBLIT_COLORKEY_PROTECT:
                hpkFlags |= HPK_BLIT_FLAG_COLORKEY_PROTECT;
                break;
            case DSBLIT_SRC_MASK_ALPHA:
                hpkFlags |= HPK_BLIT_FLAG_SRC_MASK_ALPHA;
                break;
            case DSBLIT_SRC_MASK_COLOR:
                hpkFlags |= HPK_BLIT_FLAG_SRC_MASK_COLOR;
                break;
            case DSBLIT_SOURCE2:
                hpkFlags |= HPK_BLIT_FLAG_SOURCE2;
                break;
            case DSBLIT_FLIP_HORIZONTAL:
                hpkFlags |= HPK_BLIT_FLAG_FLIP_HORIZONTAL;
                break;
            case DSBLIT_FLIP_VERTICAL:
                hpkFlags |= HPK_BLIT_FLAG_FLIP_VERTICAL;
                break;
        }
    }

    return (HPK_BlitFlag)hpkFlags;
}

static
HPK_BlendFunction TvGfxProxy_ConvertDfb2HpkBlendMode( DFBSurfaceBlendFunction dfbBlend )
{
    HPK_BlendFunction hpkBlend = HPK_BFACTOR_UNKNOWN;

    switch( dfbBlend )
    {
        case DSBF_UNKNOWN:
            hpkBlend = HPK_BFACTOR_UNKNOWN;
            break;
        case DSBF_ZERO:
            hpkBlend = HPK_BFACTOR_ZERO;
            break;
        case DSBF_ONE:
            hpkBlend = HPK_BFACTOR_ONE;
            break;
        case DSBF_SRCCOLOR:
            hpkBlend = HPK_BFACTOR_SRCCOLOR;
            break;
        case DSBF_INVSRCCOLOR:
            hpkBlend = HPK_BFACTOR_INVSRCCOLOR;
            break;
        case DSBF_SRCALPHA:
            hpkBlend = HPK_BFACTOR_SRCALPHA;
            break;
        case DSBF_INVSRCALPHA:
            hpkBlend = HPK_BFACTOR_INVSRCALPHA;
            break;
        case DSBF_DESTALPHA:
            hpkBlend = HPK_BFACTOR_DESTALPHA;
            break;
        case DSBF_INVDESTALPHA:
            hpkBlend = HPK_BFACTOR_INVDESTALPHA;
            break;
        case DSBF_DESTCOLOR:
            hpkBlend = HPK_BFACTOR_DESTCOLOR;
            break;
        case DSBF_INVDESTCOLOR:
            hpkBlend = HPK_BFACTOR_INVDESTCOLOR;
            break;
        case DSBF_SRCALPHASAT:
            hpkBlend = HPK_BFACTOR_SRCALPHASAT;
            break;
    }

    return hpkBlend;
}

static
void TvGfxProxy_CopyDfbRenderOpts2HpkBlitfilter(
    const DFBRectangle* srcRect,
    const DFBRectangle* dstRect,
    const DFBSurfaceRenderOptions pDfbRenderOpts,
    HPK_BlitFilter* pHpkBlitFilter )
{
    unsigned int renderOpts = (unsigned int)pDfbRenderOpts;
    bool upscale = ((srcRect->w < dstRect->w) || (srcRect->h < dstRect->h)) ?
                       true : false;

    *pHpkBlitFilter = HPK_BLIT_FILTER_POINTSAMPLE;

    /* TODO transformation matrix set via IDirectFBSurface::SetMatrix() is
     * not supported yet, use default filter */
    renderOpts &= ~DSRO_MATRIX;

    /* These flags are used for the stretch blit only and will not be used for
     * the casual blit */
    if ((srcRect->w == dstRect->w) && (srcRect->h == dstRect->h))
    {
        renderOpts &= ~DSRO_SMOOTH_UPSCALE;
        renderOpts &= ~DSRO_SMOOTH_DOWNSCALE;
    }

    while(true)
    {
        if (renderOpts == DSRO_NONE)
        {
            break;
        }

        /* Enable anti-aliasing for edges and parse various combinations
         * AA+SMOOTH filters
         *
         * Note: DirectFB can set DSRO_SMOOTH_UPSCALE and
         * DSRO_SMOOTH_DOWNSCALE all together, so we need to decide on the fly
         * which filter will be the best */
        if (renderOpts & DSRO_ANTIALIAS)
        {
            if ((renderOpts & DSRO_SMOOTH_UPSCALE) ||
                (renderOpts & DSRO_SMOOTH_DOWNSCALE))
            {
                if (upscale)
                {
                    *pHpkBlitFilter = HPK_BLIT_FILTER_AFF_SHARP;
                }
                else
                {
                    *pHpkBlitFilter = HPK_BLIT_FILTER_AFF_BLURRY;
                }
            }
            else
            {
                *pHpkBlitFilter = HPK_BLIT_FILTER_AFF;
            }
        }
        else    /* Use interpolation for upscale/downscale StretchBlit(). */
        {
            if ((renderOpts & DSRO_SMOOTH_UPSCALE) ||
                (renderOpts & DSRO_SMOOTH_DOWNSCALE))
            {
                if (upscale)
                {
                    /* TODO HPK_BLIT_FILTER_SHARP gives the picture better than
                     * HPK_BLIT_FILTER_ANISOTROPIC, but anisotropic filtering
                     * gives picture much closer to the reference one */
                    *pHpkBlitFilter = HPK_BLIT_FILTER_ANISOTROPIC;
                }
                else
                {
                    *pHpkBlitFilter = HPK_BLIT_FILTER_BILINEAR;
                }
            }
        }

        break;
    }
}
