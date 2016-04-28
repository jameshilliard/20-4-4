#ifndef TIVOGFX_INIT_H
#define TIVOGFX_INIT_H

//////////////////////////////////////////////////////////////////////
//
// File: tivogfx_init.h
//
// Copyright 2012 TiVo Inc.  All Rights Reserved.
//
//////////////////////////////////////////////////////////////////////


// always include directfb first
#include <directfb.h>


//######################################################################
#ifdef __cplusplus
extern "C" {
#endif
//######################################################################


void TvGfxInit_InitializeProgram( void );

void TvGfxInit_ShutdownProgram( void );


//######################################################################
#ifdef __cplusplus
}
#endif
//######################################################################


#endif // TIVOGFX_INIT_H
