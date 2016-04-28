#ifndef TIVOGFX_PROXY_H
#define TIVOGFX_PROXY_H

//////////////////////////////////////////////////////////////////////
//
// File: tivogfx_proxy.h
//
// Copyright 2014 TiVo Inc.  All Rights Reserved.
//
//////////////////////////////////////////////////////////////////////


// always include directfb first
#include <directfb.h>

#include <core/coretypes.h>
//#include <core/surface.h>
#include <core/surface_buffer.h>

#include <hpk/HpkGraphics.h>


//######################################################################
#ifdef __cplusplus
extern "C" {
#endif
//######################################################################


/**
 * TVGFXPROXY_DEF_HANDLE
 *
 * Macro to define the opaque handle for a proxy object.
 */
#define TVGFXPROXY_DEF_HANDLE( OBJ_NAME )   \
    typedef struct {} Tv##OBJ_NAME##Stub, *Tv##OBJ_NAME

/**
 * TvGfxProxy
 *
 * Define the opaque handle for the proxy object
 */
TVGFXPROXY_DEF_HANDLE( GfxProxy );

/**
 * TvGfxBuffer
 *
 * Define the opaque handle for the buffer (surface) object
 */
TVGFXPROXY_DEF_HANDLE( GfxBuffer );


/**
 * TvGfxProxy_Create
 */
bool TvGfxProxy_Create( /* returns */ TvGfxProxy* phGfxProxy );

/**
 * TvGfxProxy_Release
 */
bool TvGfxProxy_Release( /* modifies */ TvGfxProxy* phGfxProxy );

/**
 * TvGfxProxy_ShutdownGraphics
 */
void TvGfxProxy_ShutdownGraphics(void);


/**
 * TvGfxProxy_GetNumScreenConfiguration
 *
 * XXX change this to use generic + DFB types
 */
bool TvGfxProxy_GetNumScreenConfiguration( TvGfxProxy hGfxProxy,
    /* returns */ U16* pNumConfigs );

/**
 * TvGfxProxy_GetScreenConfigs
 *
 * XXX change this to use generic + DFB types
 */
bool TvGfxProxy_GetScreenConfigs( TvGfxProxy hGfxProxy,
    U16 numConfigs, /* returns */ HPK_ScreenConfig* pConfigs );

/**
 * TvGfxProxy_SetResolution
 */
DFBResult TvGfxProxy_SetResolution( TvGfxProxy hGfxProxy,
    int width, int height );


/**
 * TvGfxProxy_TestPixelFormat
 */
DFBResult TvGfxProxy_TestPixelFormat( TvGfxProxy hGfxProxy,
     DFBSurfacePixelFormat format );

/**
 * TvGfxProxy_AllocateBuffer
 */
DFBResult TvGfxProxy_AllocateBuffer( TvGfxProxy hGfxProxy,
    /* modifies */ CoreSurfaceAllocation* allocation,
    /* returns */ TvGfxBuffer* phGfxBuffer );

/**
 * TvGfxProxy_DeallocateBuffer
 */
DFBResult TvGfxProxy_DeallocateBuffer( TvGfxProxy hGfxProxy,
    /* modifies */ TvGfxBuffer* phGfxBuffer );


/**
 * TvGfxProxy_LockBuffer
 */
DFBResult TvGfxProxy_LockBuffer( TvGfxProxy hGfxProxy,
    TvGfxBuffer hGfxBuffer,
    /* modifies */ CoreSurfaceBufferLock* lock );

/**
 * TvGfxProxy_UnlockBuffer
 */
DFBResult TvGfxProxy_UnlockBuffer( TvGfxProxy hGfxProxy,
    TvGfxBuffer hGfxBuffer,
    /* modifies */ CoreSurfaceBufferLock* lock );


/**
 * TvGfxProxy_Fill
 */
DFBResult TvGfxProxy_Fill( TvGfxProxy hGfxProxy,
    TvGfxBuffer hGfxBuffer,
    const DFBRectangle* rect,
    const DFBColor* color,
    const DFBSurfaceDrawingFlags dflags);


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
    const DFBSurfaceRenderOptions renderOpts);


/**
 * TvGfxProxy_Render
 */
DFBResult TvGfxProxy_Render( TvGfxProxy hGfxProxy,
    const CoreSurfaceBufferLock* lock, const DFBRectangle* clip );


/**
 * TvGfxProxy_GetInputEvent
 */
bool TvGfxProxy_GetInputEvent( TvGfxProxy hGfxProxy,
    DFBInputEvent* pEvent );

/**
 * TvGfxProxy_PutInputEvent
 */
bool TvGfxProxy_PutInputEvent( TvGfxProxy hGfxProxy,
    const DFBInputEvent* pEvent );

/**
 * TvGfxProxy_SetColorMap
 */
DFBResult TvGfxProxy_SetColorMap( TvGfxProxy hGfxProxy,
    TvGfxBuffer hGfxBuffer,
    const CorePalette* palette );

/**
 * TvGfxProxy_Sync makes sure that graphics hardware has finished all operations
 */
DFBResult TvGfxProxy_Sync( TvGfxProxy hGfxProxy );

/**
 * TvGfxProxy_AcquireBackbuffer - Attempt to get HPK backbuffer.
 */
DFBResult TvGfxProxy_AcquireBackbuffer( TvGfxProxy hGfxProxy,
    /* modifies */ CoreSurfaceAllocation* allocation,
    /* modifies*/ TvGfxBuffer* phGfxBuffer );

/**
 * TvGfxProxy_ReleaseBackbuffer - Releases HPK backbuffer.
 */
void TvGfxProxy_ReleaseBackbuffer(TvGfxProxy hGfxProxy,
    /* modifies*/ TvGfxBuffer* phGfxBuffer);

/**
 * TvGfxProxy_AttachVideo - Set video plane mapping coordinates
 */
void TvGfxProxy_AttachVideo(TvGfxProxy hGfxProxy,
        const DFBRectangle* rect);

/**
 * TvGfxProxy_DetachVideo - Unmap video plane mapping
 */
void TvGfxProxy_DetachVideo(TvGfxProxy hGfxProxy);

//######################################################################


/**
 * TvGfxInitArgbMemory
 *
 * For debugging purposes only.
 */
void TvGfxInitArgbMemory( void* pBuff, int width, int height,
    int pitch, u32 argb );


//######################################################################
#ifdef __cplusplus
}
#endif
//######################################################################


#endif // TIVOGFX_PROXY_H
