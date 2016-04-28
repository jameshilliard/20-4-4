//////////////////////////////////////////////////////////////////////
//
// File: tivogfx_surface.c
//
// Copyright 2013 TiVo Inc  All Rights Reserved
//
//////////////////////////////////////////////////////////////////////

// always include first
#include "tivogfx_directfb.h"

// other directfb includes
#include <directfb_util.h>

// source module includes
#include "tivogfx_surface.h"

#include "tivogfx_system.h"
#include "tivogfx_util.h"

/******************************************************************************/

D_DEBUG_DOMAIN( tivogfx_surface, "tivogfx/surface", "TiVo Gfx surface" );

/******************************************************************************/

typedef struct
{
    int     magic;

    // XXX TO DO
}
TiVoGfxPoolData;

typedef struct
{
    int     magic;

    // XXX TO DO
}
TiVoGfxPoolLocalData;

typedef struct
{
    int     magic;

    TvGfxBuffer hGfxBuffer;
}
TiVoGfxAllocationData;

#define USE_BACKBUFFER 1

/******************************************************************************/

static int
tivogfxPoolDataSize( void )
{
    D_DEBUG_AT( tivogfx_surface, "%s()\n", __FUNCTION__ );

    return sizeof( TiVoGfxPoolData );
}

static int
tivogfxPoolLocalDataSize( void )
{
    D_DEBUG_AT( tivogfx_surface, "%s()\n", __FUNCTION__ );

    return sizeof( TiVoGfxPoolLocalData );
}

static int
tivogfxAllocationDataSize( void )
{
    D_DEBUG_AT( tivogfx_surface, "%s()\n", __FUNCTION__ );

    return sizeof( TiVoGfxAllocationData );
}

/******************************************************************************/

static DFBResult
tivogfxInitPool(    CoreDFB                    * core,
                    CoreSurfacePool            * pool,
                    void                       * pool_data,
                    void                       * pool_local,
                    void                       * system_data,
                    CoreSurfacePoolDescription * ret_desc )
{
    TiVoGfxSystemData      * pGfxSystemData;
    TiVoGfxPoolData        * pGfxPoolData;
    TiVoGfxPoolLocalData   * pGfxPoolLocalData;

    D_DEBUG_AT( tivogfx_surface, "%s()\n", __FUNCTION__ );

    D_ASSERT( core != NULL );
    D_MAGIC_ASSERT( pool, CoreSurfacePool );
    D_ASSERT( pool_data != NULL );
    D_ASSERT( pool_local != NULL );
    D_ASSERT( system_data != NULL );
    D_ASSERT( ret_desc != NULL );

    pGfxSystemData = TvGfx_GetSystemData();
    D_MAGIC_ASSERT( pGfxSystemData, TiVoGfxSystemData );

    // Note: we use the global accessor for consistency, rather than the
    // passed in value.  They are expected to be the same.
    D_ASSERT( ((void*) pGfxSystemData) == system_data );

    pGfxPoolData = (TiVoGfxPoolData *) pool_data;
    D_MAGIC_SET( pGfxPoolData, TiVoGfxPoolData );

    pGfxPoolLocalData = (TiVoGfxPoolLocalData *) pool_local;
    D_MAGIC_SET( pGfxPoolLocalData, TiVoGfxPoolLocalData );

    ret_desc->caps              = CSPCAPS_VIRTUAL;
    ret_desc->access[CSAID_CPU] = CSAF_READ   | CSAF_WRITE  | CSAF_SHARED;
    ret_desc->access[CSAID_GPU] = CSAF_READ   | CSAF_WRITE  | CSAF_SHARED;

    ret_desc->types             = CSTF_WINDOW | CSTF_CURSOR | CSTF_FONT |
                                  CSTF_SHARED | CSTF_EXTERNAL;

    ret_desc->priority          = CSPP_PREFERED;

    snprintf( ret_desc->name, DFB_SURFACE_POOL_DESC_NAME_LENGTH,
        "TiVo Gfx Pool" );

    return DFB_OK;
}

static DFBResult
tivogfxDestroyPool( CoreSurfacePool * pool,
                    void            * pool_data,
                    void            * pool_local )
{
    TiVoGfxPoolData        * pGfxPoolData;
    TiVoGfxPoolLocalData   * pGfxPoolLocalData;

    D_DEBUG_AT( tivogfx_surface, "%s()\n", __FUNCTION__ );

    D_MAGIC_ASSERT( pool, CoreSurfacePool );
    D_ASSERT( pool_data != NULL );
    D_ASSERT( pool_local != NULL );

    pGfxPoolData = (TiVoGfxPoolData *) pool_data;
    D_MAGIC_ASSERT( pGfxPoolData, TiVoGfxPoolData );

    pGfxPoolLocalData = (TiVoGfxPoolLocalData *) pool_local;
    D_MAGIC_ASSERT( pGfxPoolLocalData, TiVoGfxPoolLocalData );

    // XXX TO DO

    D_MAGIC_CLEAR( pGfxPoolData );
    D_MAGIC_CLEAR( pGfxPoolLocalData );

    return DFB_OK;
}

static DFBResult
tivogfxJoinPool(    CoreDFB            * core,
                    CoreSurfacePool    * pool,
                    void               * pool_data,
                    void               * pool_local,
                    void               * system_data )
{
    //TiVoGfxSystemData      *pGfxSystemData;
    //TiVoGfxPoolData        *pGfxPoolData;
    //TiVoGfxPoolLocalData   *pGfxPoolLocalData;

    D_DEBUG_AT( tivogfx_surface, "%s()\n", __FUNCTION__ );

    TIVOGFX_UNUSED_PARAMETER( core );
    D_MAGIC_ASSERT( pool, CoreSurfacePool );
    TIVOGFX_UNUSED_PARAMETER( pool_data );
    TIVOGFX_UNUSED_PARAMETER( pool_local );
    TIVOGFX_UNUSED_PARAMETER( system_data );

    // XXX TO DO
    D_WARN( "tivogfxJoinPool not implemented!\n" );

    return DFB_OK;
}

static DFBResult
tivogfxLeavePool(   CoreSurfacePool * pool,
                    void            * pool_data,
                    void            * pool_local )
{
    D_DEBUG_AT( tivogfx_surface, "%s()\n", __FUNCTION__ );

    D_MAGIC_ASSERT( pool, CoreSurfacePool );
    TIVOGFX_UNUSED_PARAMETER( pool_data );
    TIVOGFX_UNUSED_PARAMETER( pool_local );

    // XXX TO DO
    D_WARN( "tivogfxLeavePool not implemented!\n" );

    return DFB_OK;
}

/******************************************************************************/

static DFBResult
tivogfxTestConfig(  CoreSurfacePool         * pool,
                    void                    * pool_data,
                    void                    * pool_local,
                    CoreSurfaceBuffer       * buffer,
                    const CoreSurfaceConfig * config )
{
    TiVoGfxSystemData     * pGfxSystemData;
    DFBResult               result;

    D_ASSERT( config != NULL );

    D_DEBUG_AT( tivogfx_surface, "%s( format=%s )\n", __FUNCTION__,
        dfb_pixelformat_name( config->format ) );

    pGfxSystemData = TvGfx_GetSystemData();
    D_MAGIC_ASSERT( pGfxSystemData, TiVoGfxSystemData );

    D_MAGIC_ASSERT( pool, CoreSurfacePool );
    TIVOGFX_UNUSED_PARAMETER( pool_data );
    TIVOGFX_UNUSED_PARAMETER( pool_local );
    TIVOGFX_UNUSED_PARAMETER( buffer );

    result = TvGfxProxy_TestPixelFormat( pGfxSystemData->hGfxProxy,
        config->format );
    if (result != DFB_OK)
    {
        D_DEBUG_AT( tivogfx_surface, "%s() => DFB_UNSUPPORTED\n", __FUNCTION__ );
        return DFB_UNSUPPORTED;
    }

    return DFB_OK;
}

/******************************************************************************/

static DFBResult
tivogfxAllocateBuffer(  CoreSurfacePool       * pool,
                        void                  * pool_data,
                        void                  * pool_local,
                        CoreSurfaceBuffer     * buffer,
                        CoreSurfaceAllocation * allocation,
                        void                  * alloc_data )
{
    TiVoGfxSystemData     * pGfxSystemData;
    CoreSurface           * surface;
    TiVoGfxAllocationData * pGfxAllocationData;
    DFBResult               result;
    TvGfxBuffer             hGfxBuffer;

    D_DEBUG_AT( tivogfx_surface, "%s()\n", __FUNCTION__ );

    pGfxSystemData = TvGfx_GetSystemData();
    D_MAGIC_ASSERT( pGfxSystemData, TiVoGfxSystemData );

    D_MAGIC_ASSERT( pool, CoreSurfacePool );
    TIVOGFX_UNUSED_PARAMETER( pool_data );
    TIVOGFX_UNUSED_PARAMETER( pool_local );
    D_MAGIC_ASSERT( buffer, CoreSurfaceBuffer );
    D_ASSERT( alloc_data != NULL );

    surface = buffer->surface;
    D_MAGIC_ASSERT( surface, CoreSurface );

    D_DEBUG_AT( tivogfx_surface, "%s() surface=%p, buffer=%p, "
        "surface type=0x%08x, caps=0x%08x, size=%dx%d, format=%s, "
        "policy=0x%08x\n",
        __FUNCTION__, (void *) surface, (void *) buffer,
        surface->type, surface->config.caps, surface->config.size.w, surface->config.size.h,
        dfb_pixelformat_name( buffer->format ),
        buffer->policy );

    hGfxBuffer = NULL;

    result = TvGfxProxy_AllocateBuffer( pGfxSystemData->hGfxProxy,
        allocation, /* returns */ &hGfxBuffer );

    if (result != DFB_OK)
    {
        return result;
    }

    pGfxAllocationData = (TiVoGfxAllocationData *) alloc_data;
    D_MAGIC_SET( pGfxAllocationData, TiVoGfxAllocationData );

    pGfxAllocationData->hGfxBuffer = hGfxBuffer;

    return DFB_OK;
}

static DFBResult
tivogfxDeallocateBuffer(    CoreSurfacePool       * pool,
                            void                  * pool_data,
                            void                  * pool_local,
                            CoreSurfaceBuffer     * buffer,
                            CoreSurfaceAllocation * allocation,
                            void                  * alloc_data )
{
    TiVoGfxSystemData     * pGfxSystemData;
    TiVoGfxAllocationData * pGfxAllocationData;
    DFBResult               result;

    D_DEBUG_AT( tivogfx_surface, "%s()\n", __FUNCTION__ );

    pGfxSystemData = TvGfx_GetSystemData();
    D_MAGIC_ASSERT( pGfxSystemData, TiVoGfxSystemData );

    D_MAGIC_ASSERT( pool, CoreSurfacePool );
    TIVOGFX_UNUSED_PARAMETER( pool_data );
    TIVOGFX_UNUSED_PARAMETER( pool_local );
    D_MAGIC_ASSERT( buffer, CoreSurfaceBuffer );
    D_ASSERT( alloc_data != NULL );

    CORE_SURFACE_ALLOCATION_ASSERT( allocation );

    pGfxAllocationData = (TiVoGfxAllocationData *) alloc_data;
    D_MAGIC_ASSERT( pGfxAllocationData, TiVoGfxAllocationData );

    result = TvGfxProxy_DeallocateBuffer( pGfxSystemData->hGfxProxy,
        /* modifies */ &pGfxAllocationData->hGfxBuffer );

    D_ASSERT( result == DFB_OK );

    D_MAGIC_CLEAR( pGfxAllocationData );

    return DFB_OK;
}

/******************************************************************************/

static DFBResult
tivogfxLock(        CoreSurfacePool       * pool,
                    void                  * pool_data,
                    void                  * pool_local,
                    CoreSurfaceAllocation * allocation,
                    void                  * alloc_data,
                    CoreSurfaceBufferLock * lock )
{
    TiVoGfxSystemData     * pGfxSystemData;
    TiVoGfxAllocationData * pGfxAllocationData;
    CoreSurfaceBuffer     * buffer;
    CoreSurface           * surface;
    DFBResult               result;

    //D_DEBUG_AT_X( tivogfx_surface, "%s()\n", __FUNCTION__ );

    pGfxSystemData = TvGfx_GetSystemData();
    D_MAGIC_ASSERT( pGfxSystemData, TiVoGfxSystemData );

    D_MAGIC_ASSERT( pool, CoreSurfacePool );
    D_ASSERT( pool_data != NULL );
    TIVOGFX_UNUSED_PARAMETER( pool_local );
    D_MAGIC_ASSERT( allocation, CoreSurfaceAllocation );
    D_ASSERT( alloc_data != NULL );
    D_MAGIC_ASSERT( lock, CoreSurfaceBufferLock );

    buffer = allocation->buffer;
    D_MAGIC_ASSERT( buffer, CoreSurfaceBuffer );

    surface = buffer->surface;
    D_MAGIC_ASSERT( surface, CoreSurface );

    pGfxAllocationData = (TiVoGfxAllocationData *) alloc_data;
    D_MAGIC_ASSERT( pGfxAllocationData, TiVoGfxAllocationData );

    result = TvGfxProxy_LockBuffer( pGfxSystemData->hGfxProxy,
        pGfxAllocationData->hGfxBuffer, lock );
    if (result != DFB_OK)
    {
        return result;
    }

    return DFB_OK;
}

static DFBResult
tivogfxUnlock(      CoreSurfacePool       * pool,
                    void                  * pool_data,
                    void                  * pool_local,
                    CoreSurfaceAllocation * allocation,
                    void                  * alloc_data,
                    CoreSurfaceBufferLock * lock )
{
    TiVoGfxSystemData     * pGfxSystemData;
    TiVoGfxAllocationData * pGfxAllocationData;
    DFBResult               result;

    //D_DEBUG_AT_X( tivogfx_surface, "%s()\n", __FUNCTION__ );

    pGfxSystemData = TvGfx_GetSystemData();
    D_MAGIC_ASSERT( pGfxSystemData, TiVoGfxSystemData );

    D_MAGIC_ASSERT( pool, CoreSurfacePool );
    TIVOGFX_UNUSED_PARAMETER( pool_data );
    TIVOGFX_UNUSED_PARAMETER( pool_local );
    D_MAGIC_ASSERT( allocation, CoreSurfaceAllocation );

    pGfxAllocationData = (TiVoGfxAllocationData *) alloc_data;
    D_MAGIC_ASSERT( pGfxAllocationData, TiVoGfxAllocationData );

    D_MAGIC_ASSERT( lock, CoreSurfaceBufferLock );

    result = TvGfxProxy_UnlockBuffer( pGfxSystemData->hGfxProxy,
        pGfxAllocationData->hGfxBuffer, lock );
    if (result != DFB_OK)
    {
        return result;
    }

    return DFB_OK;
}

/******************************************************************************/
/******************************************************************************/

static DFBResult
tivogfxInitPoolBackbuffer(  CoreDFB                    * core,
                            CoreSurfacePool            * pool,
                            void                       * pool_data,
                            void                       * pool_local,
                            void                       * system_data,
                            CoreSurfacePoolDescription * ret_desc )
{
    TiVoGfxSystemData      * pGfxSystemData;
    TiVoGfxPoolData        * pGfxPoolData;
    TiVoGfxPoolLocalData   * pGfxPoolLocalData;

    D_DEBUG_AT( tivogfx_surface, "%s()\n", __FUNCTION__ );

    D_ASSERT( core != NULL );
    D_MAGIC_ASSERT( pool, CoreSurfacePool );
    D_ASSERT( pool_data != NULL );
    D_ASSERT( pool_local != NULL );
    D_ASSERT( system_data != NULL );
    D_ASSERT( ret_desc != NULL );

    pGfxSystemData = TvGfx_GetSystemData();
    D_MAGIC_ASSERT( pGfxSystemData, TiVoGfxSystemData );

    // Note: we use the global accessor for consistency, rather than the
    // passed in value.  They are expected to be the same.
    D_ASSERT( ((void*) pGfxSystemData) == system_data );

    pGfxPoolData = (TiVoGfxPoolData *) pool_data;
    D_MAGIC_SET( pGfxPoolData, TiVoGfxPoolData );

    pGfxPoolLocalData = (TiVoGfxPoolLocalData *) pool_local;
    D_MAGIC_SET( pGfxPoolLocalData, TiVoGfxPoolLocalData );

    ret_desc->caps                  = CSPCAPS_VIRTUAL;
    ret_desc->access[CSAID_CPU]     = CSAF_READ  | CSAF_WRITE  | CSAF_SHARED;
    ret_desc->access[CSAID_GPU]     = CSAF_READ  | CSAF_WRITE  | CSAF_SHARED;
    ret_desc->access[CSAID_LAYER0]  = CSAF_READ  | CSAF_WRITE  | CSAF_SHARED;
    ret_desc->types                 = CSTF_LAYER | CSTF_SHARED | CSTF_EXTERNAL;
    ret_desc->priority              = CSPP_PREFERED;

    snprintf( ret_desc->name, DFB_SURFACE_POOL_DESC_NAME_LENGTH,
        "TiVo Gfx Backbuffer" );

    return DFB_OK;
}

/******************************************************************************/

static DFBResult
tivogfxAcquireBackbuffer( CoreSurfacePool       * pool,
                          void                  * pool_data,
                          void                  * pool_local,
                          CoreSurfaceBuffer     * buffer,
                          CoreSurfaceAllocation * allocation,
                          void                  * alloc_data )
{
    TiVoGfxSystemData     * pGfxSystemData;
#if USE_BACKBUFFER
    CoreSurface           * surface;
    TiVoGfxAllocationData * pGfxAllocationData;
#endif // USE_BACKBUFFER
    DFBResult               result = DFB_BUG;
#if USE_BACKBUFFER
    TvGfxBuffer             hGfxBuffer;
#endif // USE_BACKBUFFER

    D_DEBUG_AT( tivogfx_surface, "%s()\n", __FUNCTION__ );

    pGfxSystemData = TvGfx_GetSystemData();
    D_MAGIC_ASSERT( pGfxSystemData, TiVoGfxSystemData );

    pthread_mutex_lock( &pGfxSystemData->gfxMutex );

#if USE_BACKBUFFER
    if (pGfxSystemData->hBackBuffer != NULL)
    {
#endif // USE_BACKBUFFER
        // The backbuffer is already acquired, allocate as a usual surface.
        result = tivogfxAllocateBuffer(pool,
                                       pool_data,
                                       pool_local,
                                       buffer,
                                       allocation,
                                       alloc_data);
#if USE_BACKBUFFER
    }
    else
    {
        D_MAGIC_ASSERT( pool, CoreSurfacePool );
        D_MAGIC_ASSERT( buffer, CoreSurfaceBuffer );
        D_ASSERT( alloc_data != NULL );

        surface = buffer->surface;
        D_MAGIC_ASSERT( surface, CoreSurface );

        D_DEBUG_AT( tivogfx_surface, "%s() surface=%p, buffer=%p, "
                    "surface type=0x%08x, caps=0x%08x, size=%dx%d, format=%s, "
                    "policy=0x%08x\n",
                    __FUNCTION__, (void *) surface, (void *) buffer,
                    surface->type, surface->config.caps, surface->config.size.w,
                    surface->config.size.h, dfb_pixelformat_name( buffer->format ),
                    buffer->policy );

        hGfxBuffer = NULL;

        if ((surface->type & (CSTF_LAYER | CSTF_EXTERNAL))
                          == (CSTF_LAYER | CSTF_EXTERNAL))
        {
            if (surface->config.size.w == TIVOGFX_HD_DISPLAY_WIDTH &&
                surface->config.size.h == TIVOGFX_HD_DISPLAY_HEIGHT)
            {
                D_DEBUG_AT( tivogfx_surface, "Attempt to AcquireBackbuffer\n" );
                result = TvGfxProxy_AcquireBackbuffer( pGfxSystemData->hGfxProxy,
                                                       allocation, &hGfxBuffer );
            }
            else
            {
                D_DEBUG_AT( tivogfx_surface,
                            "Can't allocate backbuffer %d x %d, dedicated size is %d x %d\n",
                            surface->config.size.w, surface->config.size.h,
                            TIVOGFX_HD_DISPLAY_WIDTH, TIVOGFX_HD_DISPLAY_HEIGHT );

                result = DFB_UNSUPPORTED;
            }
        }

        if (result == DFB_OK)
        {
            pGfxAllocationData = (TiVoGfxAllocationData *) alloc_data;
            D_MAGIC_SET( pGfxAllocationData, TiVoGfxAllocationData );
            pGfxAllocationData->hGfxBuffer = hGfxBuffer;
            pGfxSystemData->hBackBuffer = hGfxBuffer;
        }
    }
#endif // USE_BACKBUFFER

    pthread_mutex_unlock( &pGfxSystemData->gfxMutex );

    return result;
}

/******************************************************************************/

static DFBResult
tivogfxReleaseBackbuffer( CoreSurfacePool       * pool,
                          void                  * pool_data,
                          void                  * pool_local,
                          CoreSurfaceBuffer     * buffer,
                          CoreSurfaceAllocation * allocation,
                          void                  * alloc_data )
{
    DFBResult               result = DFB_BUG;
    TiVoGfxSystemData     * pGfxSystemData;
    TiVoGfxAllocationData * pGfxAllocationData;

    D_DEBUG_AT( tivogfx_surface, "%s()\n", __FUNCTION__ );

    pGfxSystemData = TvGfx_GetSystemData();
    D_MAGIC_ASSERT( pGfxSystemData, TiVoGfxSystemData );

    D_MAGIC_ASSERT( pool, CoreSurfacePool );
    D_MAGIC_ASSERT( buffer, CoreSurfaceBuffer );
    D_ASSERT( alloc_data != NULL );

    CORE_SURFACE_ALLOCATION_ASSERT( allocation );

    pGfxAllocationData = (TiVoGfxAllocationData *) alloc_data;
    D_MAGIC_ASSERT( pGfxAllocationData, TiVoGfxAllocationData );

    pthread_mutex_lock( &pGfxSystemData->gfxMutex );

#if USE_BACKBUFFER
    if (pGfxAllocationData->hGfxBuffer != pGfxSystemData->hBackBuffer)
    {
#endif // USE_BACKBUFFER
        // It's not the backbuffer, release as a usual surface.
        result = tivogfxDeallocateBuffer(pool,
                                         pool_data,
                                         pool_local,
                                         buffer,
                                         allocation,
                                         alloc_data);
#if USE_BACKBUFFER
    }
    else
    {
        CoreSurface * surface = buffer->surface;

        if ((surface->type & (CSTF_LAYER | CSTF_EXTERNAL))
                          == (CSTF_LAYER | CSTF_EXTERNAL))
        {
            TvGfxProxy_ReleaseBackbuffer( pGfxSystemData->hGfxProxy,
                           /* modifies */ &pGfxAllocationData->hGfxBuffer );

            D_MAGIC_CLEAR( pGfxAllocationData );
            pGfxSystemData->hBackBuffer = NULL;

            result = DFB_OK;
        }
    }
#endif // USE_BACKBUFFER

    pthread_mutex_unlock( &pGfxSystemData->gfxMutex );

    return result;
}

/******************************************************************************/

const SurfacePoolFuncs tivoGfxSurfacePoolFuncs = {

    PoolDataSize:       tivogfxPoolDataSize,
    PoolLocalDataSize:  tivogfxPoolLocalDataSize,
    AllocationDataSize: tivogfxAllocationDataSize,

    InitPool:           tivogfxInitPool,
    JoinPool:           tivogfxJoinPool,
    DestroyPool:        tivogfxDestroyPool,
    LeavePool:          tivogfxLeavePool,

    TestConfig:         tivogfxTestConfig,

    AllocateBuffer:     tivogfxAllocateBuffer,
    DeallocateBuffer:   tivogfxDeallocateBuffer,

    Lock:               tivogfxLock,
    Unlock:             tivogfxUnlock,
};

/******************************************************************************/

const SurfacePoolFuncs tivoGfxBackbufferPoolFuncs = {
    PoolDataSize:       tivogfxPoolDataSize,
    PoolLocalDataSize:  tivogfxPoolLocalDataSize,
    AllocationDataSize: tivogfxAllocationDataSize,

    InitPool:           tivogfxInitPoolBackbuffer,
    JoinPool:           tivogfxJoinPool,
    DestroyPool:        tivogfxDestroyPool,
    LeavePool:          tivogfxLeavePool,

    TestConfig:         tivogfxTestConfig,

    AllocateBuffer:     tivogfxAcquireBackbuffer,
    DeallocateBuffer:   tivogfxReleaseBackbuffer,

    Lock:               tivogfxLock,
    Unlock:             tivogfxUnlock,
};
