#ifndef TIVOGFX_INPUT_IPC2_H
#define TIVOGFX_INPUT_IPC2_H

//////////////////////////////////////////////////////////////////////
//
// File: tivogfx_input_ipc2.h
//
// Copyright 2013 TiVo Inc.  All Rights Reserved.
//
//////////////////////////////////////////////////////////////////////


// always include directfb first
#include <directfb.h>

#include "tivogfx_proxy.h"
#include "tivogfx_user_events.h"


//######################################################################
#ifdef __cplusplus
extern "C" {
#endif
//######################################################################


void TvGfx_IpcInputOpen2( TvGfxProxy hGfxProxy );

void TvGfx_IpcInputClose2( void );


//######################################################################
#ifdef __cplusplus
}
#endif
//######################################################################


#endif // TIVOGFX_INPUT_IPC2_H
