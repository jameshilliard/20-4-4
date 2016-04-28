//////////////////////////////////////////////////////////////////////
//
// File: tivogfx_screen.c
//
// Copyright 2013 TiVo Inc.  All Rights Reserved.
//
//////////////////////////////////////////////////////////////////////

// always include first
#include "tivogfx_directfb.h"

// source module includes
#include "tivogfx_screen.h"

#include "tivogfx_util.h"

/******************************************************************************/

D_DEBUG_DOMAIN( tivogfx_screen, "tivogfx/screen", "TiVo Gfx screen" );

/******************************************************************************/

static DFBResult
tivogfxInitScreen(  CoreScreen           * screen,
                    CoreGraphicsDevice   * device,
                    void                 * driver_data,
                    void                 * screen_data,
                    DFBScreenDescription * description )
{
    D_DEBUG_AT( tivogfx_screen, "%s()\n", __FUNCTION__ );

    TIVOGFX_UNUSED_PARAMETER( screen );
    TIVOGFX_UNUSED_PARAMETER( device );
    TIVOGFX_UNUSED_PARAMETER( driver_data );
    TIVOGFX_UNUSED_PARAMETER( screen_data );

    /* Set the screen capabilities. */
#if 1
    description->caps = DSCCAPS_NONE;
#else
    // XXX to do
    description->caps = DSCCAPS_OUTPUTS | DSCCAPS_VSYNC | DSCCAPS_ENCODERS;
    description->outputs  = 1;
    description->encoders = 1;
#endif

    /* Set the screen name. */
    snprintf( description->name, DFB_SCREEN_DESC_NAME_LENGTH,
        "TiVo Gfx Primary Screen" );

    return DFB_OK;
}

static DFBResult
tivogfxGetScreenSize(   CoreScreen * screen,
                        void       * driver_data,
                        void       * screen_data,
                        int        * ret_width,
                        int        * ret_height )
{
    D_DEBUG_AT( tivogfx_screen, "%s()\n", __FUNCTION__ );

    TIVOGFX_UNUSED_PARAMETER( screen );
    TIVOGFX_UNUSED_PARAMETER( driver_data );
    TIVOGFX_UNUSED_PARAMETER( screen_data );

    // XXX test
    *ret_width  = TIVOGFX_HD_DISPLAY_WIDTH;
    *ret_height = TIVOGFX_HD_DISPLAY_HEIGHT;

    return DFB_OK;
}

/* const */ ScreenFuncs tivoGfxPrimaryScreenFuncs =
{

    InitScreen:    tivogfxInitScreen,
    GetScreenSize: tivogfxGetScreenSize,
};

/******************************************************************************/

