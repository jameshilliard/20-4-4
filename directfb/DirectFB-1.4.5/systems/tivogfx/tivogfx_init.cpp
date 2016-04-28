//////////////////////////////////////////////////////////////////////
//
// File: tivogfx_init.cpp
//
// Copyright 2014 TiVo Inc.  All Rights Reserved.
//
//////////////////////////////////////////////////////////////////////

// always include first
#include "tivogfx_directfb.h"

// other directfb includes
#include <directfb_util.h>

// source module includes
#include "tivogfx_init.h"
#include "tivogfx_proxy.h"

#include <string.h>

#include <gfx/TvGraphicsCompositor.h>

#include <stx/StxIpc.h>

#include <tmk/TmkThread.h>

//####################################################################

D_DEBUG_DOMAIN( tivogfx_init, "tivogfx/init", "TiVo Gfx init" );

//####################################################################

/**
 * TvGfxDirectFbApp_Start
 */
static TvStatus TvGfxDirectFbApp_Start( TmkMempool* pMempool )
{
    D_DEBUG_AT( tivogfx_init, "%s()\n", __FUNCTION__ );

    // later may save this globally
    (void) pMempool;

    return TV_OK;
}

/**
 * TvGfxDirectFbApp_Stop
 */
static void TvGfxDirectFbApp_Stop( void )
{
    D_DEBUG_AT( tivogfx_init, "%s()\n", __FUNCTION__ );
}

/**
 * TvGfxInit vDepends
 */
static const TmkInitInfo* vDepends[] =
{
    &TvGraphicsCompositor::initInfo
};

/**
 * TvGfxInit vDepends
 */
static const TmkInitInfo* vDepends2[] =
{
    &TvGraphicsCompositor::initInfo,
    &StxIpc::initInfo
};


static char progNameG[ 64 ];
static int argcG;
static char* argvG[ 2 ];

static void (*prevCallbackS)( int );

/**
 * TvGfxInit_IsTmkApp
 */
static bool TvGfxInit_IsTmkApp( void )
{
    static bool sInit     = false;
    static bool sIsTmkApp = false;

    if (!sInit)
    {
        // XXX perhaps there is some way we can check global data
        // or thread structures to know if TmkInit::InitializeProgramOrDie
        // has already been called.
        // But for now, make the app set an environment variable if they
        // are a Tmk program to let us know not to do our own initialization.
        const char* s = getenv( "TIVO_TMK_DIRECTFB_CLIENT" );
        if ((s != NULL) && (strcmp( s, "1" ) == 0))
        {
            sIsTmkApp = true;
        }

        sInit = true;
    }

    return sIsTmkApp;
}

static void TvGfxInit_CleanupGraphicsCallback( int signal )
{
    TvGfxProxy_ShutdownGraphics();

    if (prevCallbackS != NULL)
    {
        prevCallbackS(signal);
    }
    else
    {
        TmkInit::ShutdownProgram();
    }
}

/**
 * TvGfxInit_InitializeProgram
 */
void TvGfxInit_InitializeProgram( void )
{
    D_DEBUG_AT( tivogfx_init, "%s()\n", __FUNCTION__ );

    progNameG[0] = 0;
    const char *env_prog_name = getenv("TIVO_TMK_DIRECTFB_PROGRAM_NAME");
    if (env_prog_name) {
        snprintf(progNameG, sizeof(progNameG), "%s", env_prog_name);
    }
    else {
        char stat[256];
        FILE *f = fopen("/proc/self/stat", "r");
        size_t amt_read = 0;
        if (f) {
            amt_read = fread(stat, 1, sizeof(stat) - 1, f);
            fclose(f);
        }
        if (amt_read > 0) {
            stat[amt_read] = 0;
            char *openparen = stat;
            while (*openparen && (*openparen != '(')) {
                openparen++;
            }
            if (*openparen) {
                openparen++;
                char *closeparen = openparen;
                while (*closeparen && (*closeparen != ')')) {
                    closeparen++;
                }
                if (*closeparen) {
                    *closeparen = 0;
                    snprintf(progNameG, sizeof(progNameG), "%s", openparen);
                }
            }
        }
    }

    if (!progNameG[0]) {
        strncpy( progNameG, "TvGfxDirectFbApp", sizeof(progNameG) );
        progNameG[ sizeof(progNameG) - 1 ] = 0;
    }

    argcG = 1;

    argvG[ 0 ] = progNameG;
    argvG[ 1 ] = NULL;

    if (TvGfxInit_IsTmkApp())
    {
        D_DEBUG_AT( tivogfx_init, "%s() %s\n", __FUNCTION__,
            "Client said they did TmkInit, nothing to do." );
    }
    else
    {
        if (getenv("TIVO_DIRECTFB_EXIT_ON_UNHANDLED_SIGNAL")) {
            TmkInit::SetUnexpectedSignalRebootPolicy(
                TmkInit::UnexpectedSignalRebootPolicyExit);
        }
        else if (getenv("TIVO_DIRECTFB_CONTINUE_ON_UNHANDLED_SIGNAL")) {
            TmkInit::SetUnexpectedSignalRebootPolicy(
                TmkInit::UnexpectedSignalRebootPolicyDoNothing);
        }

        D_DEBUG_AT( tivogfx_init, "%s() %s\n", __FUNCTION__,
            "Calling TmkInit::InitializeProgramOrDie." );

        /**
         * TvGfxInit initInfo - should be initialized only if we do TmkInit.
         */
        static const TmkInitInfo initInfo(
            "TvGfxDirectFbApp",
            TvGfxDirectFbApp_Start,
            TvGfxDirectFbApp_Stop,
            ARRAY_COUNT(vDepends), vDepends,
            0, NULL
            );

        /**
         * TvGfxInit initInfo - should be initialized only if we do TmkInit.
         */
        static const TmkInitInfo initInfo2(
            "TvGfxDirectFbApp",
            TvGfxDirectFbApp_Start,
            TvGfxDirectFbApp_Stop,
            ARRAY_COUNT(vDepends2), vDepends2,
            0, NULL
            );

        if (TmkFeatures::FEnabled(eTvFeatureStxIpc) == true)
        {
            TmkInit::InitializeProgramOrDie( &argcG, argvG, 1 * 1024 * 1024,
                &initInfo2, TMK_INIT_MEMPOOL_CLASSIC );
        }
        else
        {
            TmkInit::InitializeProgramOrDie( &argcG, argvG, 1 * 1024 * 1024,
                &initInfo, TMK_INIT_MEMPOOL_CLASSIC );
        }
    }

    prevCallbackS = TmkThread::SetApplicationCallbackForSignalHandler(
        TvGfxInit_CleanupGraphicsCallback
        );

    D_DEBUG_AT( tivogfx_init, "%s() %s\n", __FUNCTION__, "exit" );
}

/**
 * TvGfxInit_ShutdownProgram
 */
void TvGfxInit_ShutdownProgram( void )
{
    D_DEBUG_AT( tivogfx_init, "%s()\n", __FUNCTION__ );

    if (TvGfxInit_IsTmkApp())
    {
        D_DEBUG_AT( tivogfx_init, "%s() %s\n", __FUNCTION__,
            "Client said they did TmkInit, nothing to do." );
    }
    else
    {
        D_DEBUG_AT( tivogfx_init, "%s() %s\n", __FUNCTION__,
            "Calling TmkInit::ShutdownProgram." );

        TmkInit::ShutdownProgram();
    }

    D_DEBUG_AT( tivogfx_init, "%s() %s\n", __FUNCTION__, "exit" );
}

//####################################################################

