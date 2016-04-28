//////////////////////////////////////////////////////////////////////
//
// File: tivogfx_gfx.c
//
// Copyright 2013 TiVo Inc.  All Rights Reserved.
//
//////////////////////////////////////////////////////////////////////

// always include first
#include "tivogfx_directfb.h"

#include <config.h>

#include <core/gfxcard.h>
#include <core/graphics_driver.h>
#include <core/palette.h>
#include <core/state.h>
#include <core/system.h>

#include <gfx/clip.h>
#include <gfx/convert.h>

#include "tivogfx_proxy.h"
#include "tivogfx_system.h"
#include "tivogfx_util.h"

/******************************************************************************/

DFB_GRAPHICS_DRIVER( tivogfx_gfx )

D_DEBUG_DOMAIN( tivogfx_gfx, "tivogfx/gfx", "TiVo Gfx gfx" );

D_DEBUG_DOMAIN( tivogfx_gfx_x, "tivogfx_x/gfx", "TiVo Gfx gfx" );

// enable verbose diagnostics if needed
#define D_DEBUG_AT_X( d, x... )                 \
     D_DEBUG_AT( d##_x, x )

/******************************************************************************/

#define TIVOGFX_GFX_MAJOR_VERSION   0
#define TIVOGFX_GFX_MINOR_VERSION   8

/******************************************************************************/

#define TIVOGFX_SUPPORTED_DRAWINGFUNCTIONS  \
    (DFXL_FILLRECTANGLE)

#define TIVOGFX_SUPPORTED_DRAWINGFLAGS      \
    (DSDRAW_BLEND | DSDRAW_SRC_PREMULTIPLY)

#define TIVOGFX_SUPPORTED_BLITTINGFUNCTIONS \
    (DFXL_BLIT | DFXL_STRETCHBLIT)

#define TIVOGFX_SUPPORTED_BLITTINGFLAGS     \
    ( DSBLIT_NOFX                 |         \
      DSBLIT_ROTATE180            |         \
      DSBLIT_FLIP_HORIZONTAL      |         \
      DSBLIT_FLIP_VERTICAL        |         \
      DSBLIT_BLEND_ALPHACHANNEL   |         \
      DSBLIT_BLEND_COLORALPHA     |         \
      DSBLIT_SRC_PREMULTIPLY      |         \
      DSBLIT_SRC_PREMULTCOLOR     |         \
      DSBLIT_SRC_COLORKEY         |         \
      DSBLIT_DST_COLORKEY         |         \
      DSBLIT_XOR                  |         \
      DSBLIT_COLORIZE                       \
    )

// XXX TO DO (not implemented in TvGraphics, but probably will)
/*
    DSBLIT_DST_PREMULTIPLY
    DSBLIT_SOURCE2 - is already implemented in HPK but it's need to extend the
                     TvGraphics interface
*/

/******************************************************************************/

typedef struct
{
    int                     magic;
    DFBSurfaceBlittingFlags bflags;
    DFBSurfaceDrawingFlags  dflags;
}
TiVoGfxDeviceData;


typedef struct
{
    int             magic;

    TvGfxProxy      hGfxProxy;

    DFBRegion       dstClip;

    TvGfxBuffer     hFillSurface;
    DFBColor        fillColor;

    TvGfxBuffer     hBlitSurface;
    TvGfxBuffer     hBlitSrcSurface;
    DFBColor        dstColorkey;
    DFBColor        srcColorkey;
    bool            fConvertFormat;

    DFBSurfaceBlendFunction srcBlend;
    DFBSurfaceBlendFunction dstBlend;
    DFBSurfaceRenderOptions renderOpts;
}
TiVoGfxDriverData;

inline void COPY_U32_COLOR_TO_DFB_COLOR( const u32 u32Color, DFBColor* pDfbColor );

/******************************************************************************/

/*
 * Makes sure that graphics hardware has finished all operations.
 *
 * This method is called before the CPU accesses a surface' buffer
 * that had been written to by the hardware after this method has been
 * called the last time.
 *
 * It's also called before entering the OpenGL state (e.g. DRI driver).
 */
static DFBResult
tivogfx_gfxEngineSync(      void * driver_data,
                            void * device_data )
{
    TiVoGfxDriverData * pGfxDriverData;

    D_DEBUG_AT( tivogfx_gfx, "%s()\n", __FUNCTION__ );

    TIVOGFX_UNUSED_PARAMETER( device_data );

    pGfxDriverData = (TiVoGfxDriverData *) driver_data;
    D_MAGIC_ASSERT( pGfxDriverData, TiVoGfxDriverData );

    TvGfxProxy_Sync( pGfxDriverData->hGfxProxy );

    return DFB_OK;
}

/*
 * Called after driver->InitDevice() and during dfb_gfxcard_unlock( true ).
 * The driver should do the one time initialization of the engine,
 * e.g. writing some registers that are supposed to have a fixed value.
 *
 * This happens after mode switching or after returning from
 * OpenGL state (e.g. DRI driver).
 */
static void
tivogfx_gfxEngineReset(     void * driver_data,
                            void * device_data )
{
    D_DEBUG_AT( tivogfx_gfx, "%s()\n", __FUNCTION__ );

    TIVOGFX_UNUSED_PARAMETER( driver_data );
    TIVOGFX_UNUSED_PARAMETER( device_data );

    // nothing to do
}

/*
 * Start processing of queued commands if required.
 *
 * This function is called before returning from the graphics core to the application.
 * Usually that's after each rendering function. The only functions causing multiple commands
 * to be queued with a single emition at the end are DrawString(), TileBlit(), BatchBlit(),
 * DrawLines() and possibly FillTriangle() which is emulated using multiple FillRectangle() calls.
 */
static void
tivogfx_gfxEmitCommands(    void * driver_data,
                            void * device_data )
{
    D_DEBUG_AT( tivogfx_gfx, "%s()\n", __FUNCTION__ );

    TIVOGFX_UNUSED_PARAMETER( driver_data );
    TIVOGFX_UNUSED_PARAMETER( device_data );

    // nothing to do
}

/*
 * Check for acceleration of 'accel' using the given 'state'.
 */
static void
tivogfx_gfxCheckState(      void *              driver_data,
                            void *              device_data,
                            CardState *         state,
                            DFBAccelerationMask accel )
{
    TiVoGfxDriverData * pGfxDriverData;
    TiVoGfxDeviceData * pGfxDeviceData;
    DFBResult           retVal;

    D_DEBUG_AT( tivogfx_gfx, "%s( accel=%s, dest=%p )\n",
        __FUNCTION__,
        TvGfxUtil_GetAccelerationMaskDebugStr( accel ),
        state->destination );

    pGfxDriverData = (TiVoGfxDriverData *) driver_data;
    D_MAGIC_ASSERT( pGfxDriverData, TiVoGfxDriverData );

    TIVOGFX_UNUSED_VARIABLE( pGfxDriverData );

    pGfxDeviceData = (TiVoGfxDeviceData *) device_data;
    D_MAGIC_ASSERT( pGfxDeviceData, TiVoGfxDeviceData );

    // check state->destination->config.format for supported pixel format
    retVal = TvGfxProxy_TestPixelFormat( pGfxDriverData->hGfxProxy,
                                         state->destination->config.format );
    if (retVal != DFB_OK)
    {
        D_DEBUG_AT_X( tivogfx_gfx, "Pixel format %d is not supported\n",
            state->destination->config.format );
        return;
    }

    if (DFB_DRAWING_FUNCTION( accel ))
    {
        // DFXL_FILLRECTANGLE

        if (state->drawingflags & ~TIVOGFX_SUPPORTED_DRAWINGFLAGS)
        {
            D_DEBUG_AT_X( tivogfx_gfx, "Draw flags %s => not supported\n",
                TvGfxUtil_GetSurfaceDrawingFlagsDebugStr( state->drawingflags ) );
            return;
        }

        state->accel |= TIVOGFX_SUPPORTED_DRAWINGFUNCTIONS;

        D_DEBUG_AT_X( tivogfx_gfx, "Draw supported\n" );
    }
    else
    {
        // DFXL_BLIT, DFXL_STRETCHBLIT

        if (state->blittingflags & ~TIVOGFX_SUPPORTED_BLITTINGFLAGS)
        {
            D_DEBUG_AT_X( tivogfx_gfx, "Blit flags %s => not supported\n",
                TvGfxUtil_GetSurfaceBlittingFlagsDebugStr( state->blittingflags ) );
            return;
        }

        state->accel |= TIVOGFX_SUPPORTED_BLITTINGFUNCTIONS;

        D_DEBUG_AT_X( tivogfx_gfx, "Blit supported\n" );
    }
}

static void
tivogfx_gfxSetStateRect(    TiVoGfxDriverData * pGfxDriverData,
                            TiVoGfxDeviceData * pGfxDeviceData,
                            CardState *         state )
{
    DFBColor fillColor;

    TIVOGFX_UNUSED_PARAMETER( pGfxDeviceData );

    if (state->dst.handle == NULL)
    {
        D_DEBUG_AT( tivogfx_gfx, "%s() dst surface not set\n",
            __FUNCTION__ );
        return;
    }

    pGfxDriverData->hFillSurface = (TvGfxBuffer) state->dst.handle;

    // palette color for destination is not supported!
    D_ASSERT( state->destination->palette == NULL );

    fillColor = state->color;

    // check for premultiply
    if (state->drawingflags & DSDRAW_SRC_PREMULTIPLY)
    {
        // setup the premultiplied the color
        uint ca = fillColor.a + 1;
        fillColor.r = (fillColor.r * ca) >> 8;
        fillColor.g = (fillColor.g * ca) >> 8;
        fillColor.b = (fillColor.b * ca) >> 8;

        // note: YUV not supported!
    }

    pGfxDriverData->fillColor = fillColor;
    pGfxDeviceData->dflags    = state->drawingflags;
}

static void
tivogfx_gfxSetStateBlit(    TiVoGfxDriverData * pGfxDriverData,
                            TiVoGfxDeviceData * pGfxDeviceData,
                            CardState *         state )
{
    TIVOGFX_UNUSED_PARAMETER( pGfxDeviceData );

    if (state->dst.handle == NULL)
    {
        D_DEBUG_AT( tivogfx_gfx, "%s() dst surface not set\n",
            __FUNCTION__ );
        return;
    }

    pGfxDriverData->hBlitSurface = (TvGfxBuffer) state->dst.handle;

    if (state->src.handle == NULL)
    {
        D_DEBUG_AT( tivogfx_gfx, "%s() src surface not set\n",
            __FUNCTION__ );
        return;
    }

    pGfxDriverData->hBlitSrcSurface = (TvGfxBuffer) state->src.handle;

    // set palette for paletterized SRC surface
    TvGfxProxy_SetColorMap( pGfxDriverData->hGfxProxy,
                            pGfxDriverData->hBlitSrcSurface,
                            state->source->palette );

    // palette color for destination is not supported!
    D_ASSERT( state->destination->palette == NULL );

    // color is used in various blend operations
    pGfxDriverData->fillColor    = state->color;
    pGfxDeviceData->bflags       = state->blittingflags;

    // The state->src_colorkey and state->dst_colorkey is already has arranged
    // as ARGB irrespective of used byte order
    COPY_U32_COLOR_TO_DFB_COLOR( state->dst_colorkey, &pGfxDriverData->dstColorkey );
    COPY_U32_COLOR_TO_DFB_COLOR( state->src_colorkey, &pGfxDriverData->srcColorkey );

    pGfxDriverData->fConvertFormat =
        ( state->source->config.format != state->destination->config.format );

    if (state->src_blend == DSBF_SRCALPHASAT || state->dst_blend == DSBF_SRCALPHASAT)
    {
        D_DEBUG_AT( tivogfx_gfx, "%s() DSBF_SRCALPHASAT mode is unsupported\n",
            __FUNCTION__ );
        return;
    }

    pGfxDriverData->srcBlend   = state->src_blend;
    pGfxDriverData->dstBlend   = state->dst_blend;
    pGfxDriverData->renderOpts = state->render_options;
}

/*
 * Program card for execution of the function 'accel' with the 'state'.
 * 'state->modified' contains information about changed entries.
 * This function has to set at least 'accel' in 'state->set'.
 * The driver should remember 'state->modified' and clear it.
 * The driver may modify 'funcs' depending on 'state' settings.
 */
static void
tivogfx_gfxSetState(    void *                  driver_data,
                        void *                  device_data,
                        GraphicsDeviceFuncs *   funcs,
                        CardState *             state,
                        DFBAccelerationMask     accel )
{
    TiVoGfxDriverData * pGfxDriverData;
    TiVoGfxDeviceData * pGfxDeviceData;

    D_DEBUG_AT( tivogfx_gfx, "%s( state=%p, accel=%s, dest=%p, mod_hw=0x%08x )\n",
        __FUNCTION__, state,
        TvGfxUtil_GetAccelerationMaskDebugStr( accel ),
        state->destination, state->mod_hw );

    pGfxDriverData = (TiVoGfxDriverData *) driver_data;
    D_MAGIC_ASSERT( pGfxDriverData, TiVoGfxDriverData );

    pGfxDeviceData = (TiVoGfxDeviceData *) device_data;
    D_MAGIC_ASSERT( pGfxDeviceData, TiVoGfxDeviceData );

    TIVOGFX_UNUSED_PARAMETER( funcs );

    if (DFB_DRAWING_FUNCTION( accel ))
    {
        // DFXL_FILLRECTANGLE

        tivogfx_gfxSetStateRect( pGfxDriverData, pGfxDeviceData, state );
    }
    else
    {
        // DFXL_BLIT, DFXL_STRETCHBLIT

        tivogfx_gfxSetStateBlit( pGfxDriverData, pGfxDeviceData, state );
    }

    pGfxDriverData->dstClip = state->clip;

    state->mod_hw = 0;
}

/*
 * Render a filled rectangle using the current hardware state.
 */
static bool
tivogfx_gfxFillRectangle(   void *          driver_data,
                            void *          device_data,
                            DFBRectangle *  rect )
{
    TiVoGfxDriverData * pGfxDriverData;
    TiVoGfxDeviceData * pGfxDeviceData;
    DFBRectangle        clippedDstRect;
    DFBResult           result;

    D_DEBUG_AT( tivogfx_gfx, "%s()\n", __FUNCTION__ );

    pGfxDriverData = (TiVoGfxDriverData *) driver_data;
    D_MAGIC_ASSERT( pGfxDriverData, TiVoGfxDriverData );

    pGfxDeviceData = (TiVoGfxDeviceData *) device_data;
    D_MAGIC_ASSERT( pGfxDeviceData, TiVoGfxDeviceData );

    clippedDstRect = *rect;
    if (!dfb_clip_rectangle( &pGfxDriverData->dstClip, &clippedDstRect ))
    {
        // nothing to do - lies completely out of destination
        D_DEBUG_AT_X( tivogfx_gfx, "doesn't intersect with clip\n" );
        return true;
    }

    result = TvGfxProxy_Fill( pGfxDriverData->hGfxProxy,
                              pGfxDriverData->hFillSurface,
                              &clippedDstRect,
                              &pGfxDriverData->fillColor,
                              pGfxDeviceData->dflags );

    if (result != DFB_OK)
    {
        D_DEBUG_AT_X( tivogfx_gfx, "TvGfxProxy_FillRect error\n" );
        return false;
    }

    return true;
}

/*
 * Blit a rectangle using the current hardware state.
 */
static bool
tivogfx_gfxBlit(            void *          driver_data,
                            void *          device_data,
                            DFBRectangle *  rect,
                            int             dx,
                            int             dy )
{
    TiVoGfxDriverData * pGfxDriverData;
    TiVoGfxDeviceData * pGfxDeviceData;
    DFBRectangle        srcRect;
    DFBRectangle        dstRect;
    DFBRectangle        clippedDstRect;
    DFBResult           result;

    D_DEBUG_AT( tivogfx_gfx, "%s()\n", __FUNCTION__ );

    pGfxDriverData = (TiVoGfxDriverData *) driver_data;
    D_MAGIC_ASSERT( pGfxDriverData, TiVoGfxDriverData );

    pGfxDeviceData = (TiVoGfxDeviceData *) device_data;
    D_MAGIC_ASSERT( pGfxDeviceData, TiVoGfxDeviceData );

    srcRect = *rect;

    dstRect.x = dx;
    dstRect.y = dy;
    dstRect.w = rect->w;
    dstRect.h = rect->h;

    // XXX TO DO need to check that source lies entire within source bounds!

    clippedDstRect = dstRect;
    if (!dfb_clip_rectangle( &pGfxDriverData->dstClip, &clippedDstRect ))
    {
        // nothing to do - lies completely out of destination
        D_DEBUG_AT_X( tivogfx_gfx, "doesn't intersect with clip\n" );
        return true;
    }

    result = TvGfxProxy_Blit( pGfxDriverData->hGfxProxy,
                              pGfxDriverData->hBlitSurface,
                              pGfxDriverData->hBlitSrcSurface,
                              &dstRect,
                              &srcRect,
                              &pGfxDriverData->fillColor,
                              pGfxDeviceData->bflags,
                              &pGfxDriverData->srcColorkey,
                              &pGfxDriverData->dstColorkey,
                              pGfxDriverData->srcBlend,
                              pGfxDriverData->dstBlend,
                              pGfxDriverData->renderOpts );

    if (result != DFB_OK)
    {
        D_DEBUG_AT_X( tivogfx_gfx, "TvGfxProxy_Blit error\n" );
        return false;
    }

    return true;
}

/*
 * Blit a rectangle using the current hardware state.
 */
static bool
tivogfx_gfxStretchBlit(     void *          driver_data,
                            void *          device_data,
                            DFBRectangle *  srect,
                            DFBRectangle *  drect )
{
    TiVoGfxDriverData * pGfxDriverData;
    TiVoGfxDeviceData * pGfxDeviceData;
    DFBRectangle        srcRect;
    DFBRectangle        dstRect;
    DFBRectangle        clippedDstRect;
    DFBResult           result;

    D_DEBUG_AT( tivogfx_gfx, "%s()\n", __FUNCTION__ );

    pGfxDriverData = (TiVoGfxDriverData *) driver_data;
    D_MAGIC_ASSERT( pGfxDriverData, TiVoGfxDriverData );

    pGfxDeviceData = (TiVoGfxDeviceData *) device_data;
    D_MAGIC_ASSERT( pGfxDeviceData, TiVoGfxDeviceData );

    srcRect = *srect;
    dstRect = *drect;

    // XXX TO DO need to check that source lies entire within source bounds!

    // Check scaling ratio
    if ((srcRect.w == 0) || (dstRect.w == 0) || (srcRect.h == 0) || (dstRect.h == 0))
    {
        D_DEBUG_AT_X( tivogfx_gfx,
            "%s() Width or height set to 0 - can't be processed, rejecting\n",
                      __FUNCTION__);
        return false;
    }
    
    clippedDstRect = dstRect;
    if (!dfb_clip_rectangle( &pGfxDriverData->dstClip, &clippedDstRect ))
    {
        // nothing to do - lies completely out of destination
        D_DEBUG_AT_X( tivogfx_gfx, "doesn't intersect with clip\n" );
        return true;
    }

    result = TvGfxProxy_Blit( pGfxDriverData->hGfxProxy,
                              pGfxDriverData->hBlitSurface,
                              pGfxDriverData->hBlitSrcSurface,
                              &dstRect,
                              &srcRect,
                              &pGfxDriverData->fillColor,
                              pGfxDeviceData->bflags,
                              &pGfxDriverData->srcColorkey,
                              &pGfxDriverData->dstColorkey,
                              pGfxDriverData->srcBlend,
                              pGfxDriverData->dstBlend,
                              pGfxDriverData->renderOpts );

    if (result != DFB_OK)
    {
        D_DEBUG_AT_X( tivogfx_gfx,
                      "Blit is not supported by hardware, rejecting.\n" );
        return false;
    }

    return true;
}

/******************************************************************************/

/* exported symbols for a graphics driver */

static int
driver_probe(       CoreGraphicsDevice * device )
{
    D_DEBUG_AT( tivogfx_gfx, "%s()\n", __FUNCTION__ );

    TIVOGFX_UNUSED_PARAMETER( device );

    const char* s = getenv( "DFB_USE_TIVOGFX_HW" );
    if ((s != NULL) && (strcmp( s, "0" ) == 0))
    {
        D_DEBUG_AT( tivogfx_gfx, "%s: disabled by env\n", __FUNCTION__ );
        return 0;
    }

    if (dfb_system_type() == CORE_TIVOGFX)
    {
        D_DEBUG_AT( tivogfx_gfx, "%s: enabled\n", __FUNCTION__ );
        return 1;
    }

    D_DEBUG_AT( tivogfx_gfx, "%s: not matched\n", __FUNCTION__ );
    return 0;
}

static void
driver_get_info(    CoreGraphicsDevice * device,
                    GraphicsDriverInfo * info )
{
    D_DEBUG_AT( tivogfx_gfx, "%s()\n", __FUNCTION__ );

    TIVOGFX_UNUSED_PARAMETER( device );

    /* fill driver info structure */
    snprintf( info->name, DFB_GRAPHICS_DRIVER_INFO_NAME_LENGTH,
        "TiVo Gfx gfx" );

    snprintf( info->vendor, DFB_GRAPHICS_DRIVER_INFO_VENDOR_LENGTH,
        "TiVo Inc." );

    info->version.major = TIVOGFX_GFX_MAJOR_VERSION;
    info->version.minor = TIVOGFX_GFX_MINOR_VERSION;

    info->driver_data_size = sizeof( TiVoGfxDriverData );
    info->device_data_size = sizeof( TiVoGfxDeviceData );
}

/* This get called by each DFB application.
   NOTE: device data is shared, but driver data isn't.
 */
static DFBResult
driver_init_driver( CoreGraphicsDevice  * device,
                    GraphicsDeviceFuncs * funcs,
                    void                * driver_data,
                    void                * device_data,
                    CoreDFB             * core )
{
    TiVoGfxDriverData * pGfxDriverData;
    TiVoGfxSystemData * pGfxSystemData;

    D_DEBUG_AT( tivogfx_gfx, "%s()\n", __FUNCTION__ );

    TIVOGFX_UNUSED_PARAMETER( device );
    TIVOGFX_UNUSED_PARAMETER( device_data );
    TIVOGFX_UNUSED_PARAMETER( core );

    pGfxDriverData = (TiVoGfxDriverData *) driver_data;
    D_MAGIC_SET( pGfxDriverData, TiVoGfxDriverData );

    pGfxSystemData = TvGfx_GetSystemData();
    D_MAGIC_ASSERT( pGfxSystemData, TiVoGfxSystemData );

    pGfxDriverData->hGfxProxy = pGfxSystemData->hGfxProxy;

    /* Fill in function pointers */
    funcs->CheckState       = tivogfx_gfxCheckState;
    funcs->SetState         = tivogfx_gfxSetState;
    funcs->EngineSync       = tivogfx_gfxEngineSync;
    funcs->EngineReset      = tivogfx_gfxEngineReset;
    funcs->EmitCommands     = tivogfx_gfxEmitCommands;

    /* Fill in driver functions as implemented */
    funcs->FillRectangle    = tivogfx_gfxFillRectangle;
    funcs->Blit             = tivogfx_gfxBlit;
    funcs->StretchBlit      = tivogfx_gfxStretchBlit;
// XXX TO DO
//  funcs->DrawRectangle    = tivogfx_gfxDrawRectangle;
//  funcs->Blit2            = tivogfx_gfxBlit2;

    return DFB_OK;
}

/* This get called only once by the master DFB application.
   NOTE: device data is shared, but driver data isn't.
 */
static DFBResult
driver_init_device(     CoreGraphicsDevice * device,
                        GraphicsDeviceInfo * device_info,
                        void               * driver_data,
                        void               * device_data )
{
    TiVoGfxDriverData * pGfxDriverData;
    TiVoGfxDeviceData * pGfxDeviceData;

    D_DEBUG_AT( tivogfx_gfx, "%s()\n", __FUNCTION__ );

    TIVOGFX_UNUSED_PARAMETER( device );

    pGfxDriverData = (TiVoGfxDriverData *) driver_data;
    pGfxDeviceData = (TiVoGfxDeviceData *) device_data;

    D_MAGIC_SET( pGfxDeviceData, TiVoGfxDeviceData );

    /* fill device info */
    snprintf( device_info->name, DFB_GRAPHICS_DEVICE_INFO_NAME_LENGTH,
        "TiVo Gfx device" );

    snprintf( device_info->vendor, DFB_GRAPHICS_DEVICE_INFO_VENDOR_LENGTH,
        "TiVo Inc." );

    // XXX TO DO

    // XXX CCF_READSYSMEM?
    device_info->caps.flags    = 0;

    // XXX ?
    device_info->caps.clip     = DFXL_NONE;

    device_info->caps.accel    = TIVOGFX_SUPPORTED_DRAWINGFUNCTIONS |
                                  TIVOGFX_SUPPORTED_BLITTINGFUNCTIONS;
    device_info->caps.drawing  = TIVOGFX_SUPPORTED_DRAWINGFLAGS;
    device_info->caps.blitting = TIVOGFX_SUPPORTED_BLITTINGFLAGS;

    // XXX TO DO
    /* align to longword boundry */
    device_info->limits.surface_bytepitch_alignment  = 4;
    device_info->limits.surface_byteoffset_alignment = 4;

    // XXX TO DO
    /* Ensure that the size of the destination surface is 1280 x 720 */
    device_info->limits.dst_max.w = TIVOGFX_HD_DISPLAY_WIDTH;
    device_info->limits.dst_max.h = TIVOGFX_HD_DISPLAY_HEIGHT;

    return DFB_OK;
}

static void
driver_close_device(    CoreGraphicsDevice * device,
                        void               * driver_data,
                        void               * device_data )
{
    TiVoGfxDriverData * pGfxDriverData;
    TiVoGfxDeviceData * pGfxDeviceData;

    D_DEBUG_AT( tivogfx_gfx, "%s()\n", __FUNCTION__ );

    TIVOGFX_UNUSED_PARAMETER( device );

    pGfxDriverData = (TiVoGfxDriverData *) driver_data;
    pGfxDeviceData = (TiVoGfxDeviceData *) device_data;

    D_MAGIC_ASSERT( pGfxDeviceData, TiVoGfxDeviceData );
    D_MAGIC_CLEAR( pGfxDeviceData );
}

static void
driver_close_driver(    CoreGraphicsDevice * device,
                        void               * driver_data )
{
    TiVoGfxDriverData * pGfxDriverData;

    D_DEBUG_AT( tivogfx_gfx, "%s()\n", __FUNCTION__ );

    TIVOGFX_UNUSED_PARAMETER( device );

    pGfxDriverData = (TiVoGfxDriverData *) driver_data;

    pGfxDriverData->hGfxProxy = NULL;

    D_MAGIC_ASSERT( pGfxDriverData, TiVoGfxDriverData );
    D_MAGIC_CLEAR( pGfxDriverData );
}

/******************************************************************************/

/**
 * COPY_U32_COLOR_TO_DFB_COLOR
 */
inline
void COPY_U32_COLOR_TO_DFB_COLOR( const u32 u32Color, /* returns */ DFBColor* pDfbColor )
{
    pDfbColor->a = ( u32Color >> 24 ) & 0xFF;
    pDfbColor->r = ( u32Color >> 16 ) & 0xFF;
    pDfbColor->g = ( u32Color >>  8 ) & 0xFF;
    pDfbColor->b = ( u32Color >>  0 ) & 0xFF;
}
