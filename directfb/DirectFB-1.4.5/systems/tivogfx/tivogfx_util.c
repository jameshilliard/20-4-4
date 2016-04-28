//////////////////////////////////////////////////////////////////////
//
// File: tivogfx_util.c
//
// Copyright 2012 TiVo Inc.  All Rights Reserved.
//
//////////////////////////////////////////////////////////////////////

// always include first
#include "tivogfx_directfb.h"

#include <directfb_keynames.h>
#include <directfb_strings.h>

#include <stdlib.h>
#include <string.h>

#include "tivogfx_util.h"

/******************************************************************************/

/**
 * TvGfxUtil_InHandcraftMode
 */
bool TvGfxUtil_InHandcraftMode( void )
{
    const char* s = getenv( "handcraft" );

    if (s == NULL)
    {
        return false;
    }

    if (strcmp( s, "true" ) != 0)
    {
        return false;
    }

    return true;
}

/******************************************************************************/

static const DirectFBSurfaceBlittingFlagsNames( surfaceblittingflags_names );

/**
 * TvGfxUtil_GetSurfaceBlittingFlagsDebugStr
 */
const char* TvGfxUtil_GetSurfaceBlittingFlagsDebugStr( DFBSurfaceBlittingFlags flags )
{
    static char s[ 1024 ];
    TvGfxUtil_GetSurfaceBlittingFlagsDebugStr_Ex( flags, s, sizeof(s) );
    return s;
}

/**
 * TvGfxUtil_GetSurfaceBlittingFlagsDebugStr_Ex
 */
void TvGfxUtil_GetSurfaceBlittingFlagsDebugStr_Ex( DFBSurfaceBlittingFlags flags,
    char* buff, size_t buffLen )
{
    char s[ 1024 ];
    char t[ 1024 ];
    const struct DFBSurfaceBlittingFlagsName* p;
    int n;

    const char* prefix = "DSBLIT_";

    if (flags == 0 /* DSBLIT_NOFX */)
    {
        snprintf( buff, buffLen, "%s%s", prefix, "NOFX" );
        return;
    }

    s[ 0 ] = 0;
    n = 0;

    for (p = surfaceblittingflags_names; p->flag != 0 /* DSBLIT_NOFX */; p++)
    {
        if (flags & p->flag)
        {
            snprintf( t, sizeof(t), "%s", s );
            snprintf( s, sizeof(s), "%s%s%s%s", t, (n > 0) ? "," : "",
                prefix, p->name );
            n++;
            flags &= ~p->flag;
        }
    }

    if (flags != 0)
    {
        snprintf( t, sizeof(t), "%s", s );
        snprintf( s, sizeof(s), "%s%s%s", t, (n > 0) ? "," : "", "?" );
    }

    snprintf( buff, buffLen, "%s", s );
}

/******************************************************************************/

static const DirectFBSurfaceDrawingFlagsNames( surfacedrawingflags_names );

/**
 * TvGfxUtil_GetSurfaceDrawingFlagsDebugStr
 */
const char* TvGfxUtil_GetSurfaceDrawingFlagsDebugStr( DFBSurfaceDrawingFlags flags )
{
    static char s[ 1024 ];
    TvGfxUtil_GetSurfaceDrawingFlagsDebugStr_Ex( flags, s, sizeof(s) );
    return s;
}

/**
 * TvGfxUtil_GetSurfaceDrawingFlagsDebugStr_Ex
 */
void TvGfxUtil_GetSurfaceDrawingFlagsDebugStr_Ex( DFBSurfaceDrawingFlags flags,
    char* buff, size_t buffLen )
{
    char s[ 1024 ];
    char t[ 1024 ];
    const struct DFBSurfaceDrawingFlagsName* p;
    int n;

    const char* prefix = "DSDRAW_";

    if (flags == 0 /* DSDRAW_NOFX */)
    {
        snprintf( buff, buffLen, "%s%s", prefix, "NOFX" );
        return;
    }

    s[ 0 ] = 0;
    n = 0;

    for (p = surfacedrawingflags_names; p->flag != 0 /* DSDRAW_NOFX */; p++)
    {
        if (flags & p->flag)
        {
            snprintf( t, sizeof(t), "%s", s );
            snprintf( s, sizeof(s), "%s%s%s%s", t, (n > 0) ? "," : "",
                prefix, p->name );
            n++;
            flags &= ~p->flag;
        }
    }

    if (flags != 0)
    {
        snprintf( t, sizeof(t), "%s", s );
        snprintf( s, sizeof(s), "%s%s%s", t, (n > 0) ? "," : "", "?" );
    }

    snprintf( buff, buffLen, "%s", s );
}

/******************************************************************************/

static const DirectFBAccelerationMaskNames( accelerationmask_names );

/**
 * TvGfxUtil_GetAccelerationMaskDebugStr
 */
const char* TvGfxUtil_GetAccelerationMaskDebugStr( DFBAccelerationMask mask )
{
    static char s[ 1024 ];
    TvGfxUtil_GetAccelerationMaskDebugStr_Ex( mask, s, sizeof(s) );
    return s;
}

/**
 * TvGfxUtil_GetAccelerationMaskDebugStr_Ex
 */
void TvGfxUtil_GetAccelerationMaskDebugStr_Ex( DFBAccelerationMask mask,
    char* buff, size_t buffLen )
{
    char s[ 1024 ];
    char t[ 1024 ];
    const struct DFBAccelerationMaskName* p;
    int n;

    const char* prefix = "DFXL_";

    if (mask == 0 /* DFXL_NONE */)
    {
        snprintf( buff, buffLen, "%s%s", prefix, "NONE" );
        return;
    }

    s[ 0 ] = 0;
    n = 0;

    for (p = accelerationmask_names; p->mask != 0 /* DFXL_NONE */; p++)
    {
        if (mask & p->mask)
        {
            snprintf( t, sizeof(t), "%s", s );
            snprintf( s, sizeof(s), "%s%s%s%s", t, (n > 0) ? "," : "",
                prefix, p->name );
            n++;
            mask &= ~p->mask;
        }
    }

    if (mask != 0)
    {
        snprintf( t, sizeof(t), "%s", s );
        snprintf( s, sizeof(s), "%s%s%s", t, (n > 0) ? "," : "", "?" );
    }

    snprintf( buff, buffLen, "%s", s );
}

/******************************************************************************/

// defined in src/core/input.c
extern struct DFBKeyIdentifierName KeyIdentifierNames[];

/**
 * TvGfxUtil_GetKeyIdentifierDebugStr
 */
const char* TvGfxUtil_GetKeyIdentifierDebugStr( DFBInputDeviceKeyIdentifier keyId )
{
    static char s[ 64 ];

    const struct DFBKeyIdentifierName* p;
    const char* prefix = "DIKI_";

    for (p = KeyIdentifierNames; p->identifier != DIKI_UNKNOWN; p++)
    {
        if (p->identifier == keyId)
        {
            snprintf( s, sizeof(s), "%s%s", prefix, p->name );
            return s;
        }
    }

    // shouldn't get here
    return "UNKNOWN";
}

/******************************************************************************/

// defined in src/core/input.c
extern struct DFBKeySymbolName KeySymbolNames[];

/**
 * TvGfxUtil_GetKeySymbolDebugStr
 */
const char* TvGfxUtil_GetKeySymbolDebugStr( DFBInputDeviceKeySymbol keySym )
{
    static char s[ 64 ];

    const struct DFBKeySymbolName* p;
    const char* prefix = "DIKS_";

    for (p = KeySymbolNames; p->symbol != DIKS_NULL; p++)
    {
        if (p->symbol == keySym)
        {
            snprintf( s, sizeof(s), "%s%s", prefix, p->name );
            return s;
        }
    }

    // shouldn't get here
    return "UNKNOWN";
}

/******************************************************************************/

