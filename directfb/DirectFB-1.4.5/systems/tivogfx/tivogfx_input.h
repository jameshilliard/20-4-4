//////////////////////////////////////////////////////////////////////
//
// File: tivogfx_input.h
//
// Copyright 2013 TiVo Inc.  All Rights Reserved.
//
//////////////////////////////////////////////////////////////////////

#ifndef TIVOGFX_INPUT_H
#define TIVOGFX_INPUT_H

// always include directfb first
#include <directfb.h>

#include "tivogfx_proxy.h"


//######################################################################
#ifdef __cplusplus
extern "C" {
#endif
//######################################################################


    void * TvGfx_InputOpen( CoreInputDevice * device );

    void TvGfx_InputClose( void * driver_data );

    void TvGfx_InputDispatch( CoreInputDevice * device, DFBInputEvent * evt );


//######################################################################
#ifdef __cplusplus
}
#endif
//######################################################################


#endif // TIVOGFX_INPUT_H
