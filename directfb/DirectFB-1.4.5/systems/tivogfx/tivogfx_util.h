#ifndef TIVOGFX_UTIL_H
#define TIVOGFX_UTIL_H

//////////////////////////////////////////////////////////////////////
//
// File: tivogfx_util.h
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


// macro to indicate an unused parameter, by dummy referencing it
#define TIVOGFX_UNUSED_PARAMETER( x )   (void) x

// macro to indicate an unused local, by dummy referencing it
#define TIVOGFX_UNUSED_VARIABLE( x )    (void) x


//######################################################################


/**
 * TvGfxUtil_InHandcraftMode
 */
bool TvGfxUtil_InHandcraftMode( void );


/**
 * TvGfxUtil_GetSurfaceBlittingFlagsDebugStr
 */
const char* TvGfxUtil_GetSurfaceBlittingFlagsDebugStr( DFBSurfaceBlittingFlags flags );

/**
 * TvGfxUtil_GetSurfaceBlittingFlagsDebugStr_Ex
 */
void TvGfxUtil_GetSurfaceBlittingFlagsDebugStr_Ex( DFBSurfaceBlittingFlags flags,
    char* buff, size_t buffLen );


/**
 * TvGfxUtil_GetSurfaceDrawingFlagsDebugStr
 */
const char* TvGfxUtil_GetSurfaceDrawingFlagsDebugStr( DFBSurfaceDrawingFlags flags );

/**
 * TvGfxUtil_GetSurfaceDrawingFlagsDebugStr_Ex
 */
void TvGfxUtil_GetSurfaceDrawingFlagsDebugStr_Ex( DFBSurfaceDrawingFlags flags,
    char* buff, size_t buffLen );


/**
 * TvGfxUtil_GetAccelerationMaskDebugStr
 */
const char* TvGfxUtil_GetAccelerationMaskDebugStr( DFBAccelerationMask mask );

/**
 * TvGfxUtil_GetAccelerationMaskDebugStr_Ex
 */
void TvGfxUtil_GetAccelerationMaskDebugStr_Ex( DFBAccelerationMask mask,
    char* buff, size_t buffLen );


/**
 * TvGfxUtil_GetKeyIdentifierDebugStr
 */
const char* TvGfxUtil_GetKeyIdentifierDebugStr( DFBInputDeviceKeyIdentifier keyId );

/**
 * TvGfxUtil_GetKeySymbolDebugStr
 */
const char* TvGfxUtil_GetKeySymbolDebugStr( DFBInputDeviceKeySymbol keySym );


//######################################################################
#ifdef __cplusplus
}
#endif
//######################################################################



#endif // TIVOGFX_UTIL_H
