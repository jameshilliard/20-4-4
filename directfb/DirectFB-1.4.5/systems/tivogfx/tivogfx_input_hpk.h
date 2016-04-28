#ifndef TIVOGFX_INPUT_HPK_H
#define TIVOGFX_INPUT_HPK_H

//////////////////////////////////////////////////////////////////////
//
// File: tivogfx_input_hpk.h
//
// Copyright 2012 TiVo Inc.  All Rights Reserved.
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


void TvGfx_HpkInputOpen( TvGfxProxy hGfxProxy );

void TvGfx_HpkInputClose( void );


//######################################################################
#ifdef __cplusplus
}
#endif
//######################################################################


#endif // TIVOGFX_INPUT_HPK_H
