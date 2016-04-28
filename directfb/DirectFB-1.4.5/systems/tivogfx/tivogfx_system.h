#ifndef TIVOGFX_SYSTEM_H
#define TIVOGFX_SYSTEM_H

//////////////////////////////////////////////////////////////////////
//
// File: tivogfx_system.h
//
// Copyright 2013 TiVo Inc.  All Rights Reserved.
//
//////////////////////////////////////////////////////////////////////


// always include directfb first
#include <directfb.h>


#include "tivogfx_proxy.h"


//######################################################################
#ifdef __cplusplus
extern "C" {
#endif
//######################################################################


typedef struct
{
    int                 magic;
    CoreScreen        * screen;
    CoreSurfacePool   * surface_pool;
    CoreSurfacePool   * backbuffer_pool;
    TvGfxProxy          hGfxProxy;
    TvGfxBuffer         hBackBuffer;
    pthread_mutex_t     gfxMutex;
}
TiVoGfxSystemData;


/**
 * TvGfx_GetSystemData
 */
TiVoGfxSystemData* TvGfx_GetSystemData( void );


//######################################################################
#ifdef __cplusplus
}
#endif
//######################################################################


#endif // TIVOGFX_SYSTEM_H
