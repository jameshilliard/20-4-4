#ifndef TIVOGFX_SURFACE_H
#define TIVOGFX_SURFACE_H

//////////////////////////////////////////////////////////////////////
//
// File: tivogfx_surface.h
//
// Copyright 2013 TiVo Inc.  All Rights Reserved.
//
//////////////////////////////////////////////////////////////////////


// always include directfb first
#include <directfb.h>


#include <core/surface_pool.h>

extern const SurfacePoolFuncs tivoGfxSurfacePoolFuncs;

extern const SurfacePoolFuncs tivoGfxBackbufferPoolFuncs;

#endif // TIVOGFX_SURFACE_H
