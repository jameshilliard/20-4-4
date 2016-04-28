#ifndef TIVOGFX_INPUT_IPC_H
#define TIVOGFX_INPUT_IPC_H

//////////////////////////////////////////////////////////////////////
//
// File: tivogfx_input_ipc.h
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


void TvGfx_IpcInputOpen( TvGfxProxy hGfxProxy );

void TvGfx_IpcInputClose( void );


//######################################################################
#ifdef __cplusplus
}
#endif
//######################################################################


#endif // TIVOGFX_INPUT_IPC_H
