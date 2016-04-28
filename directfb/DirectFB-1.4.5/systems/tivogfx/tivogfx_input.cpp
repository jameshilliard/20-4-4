//////////////////////////////////////////////////////////////////////
//
// File: tivogfx_input.cpp
//
// Copyright 2013 TiVo Inc.  All Rights Reserved.
//
//////////////////////////////////////////////////////////////////////

// always include first
#include "tivogfx_directfb.h"

#include <fcntl.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "tivogfx_proxy.h"
#include "tivogfx_system.h"
#include "tivogfx_util.h"
#include "tivogfx_input.h"

#include <tmk/TmkThread.h>

/******************************************************************************/

D_DEBUG_DOMAIN( tivogfx_input, "tivogfx/input", "TiVo Gfx input" );

/******************************************************************************/

/*
 * tivoGfxGetKeySim
 */
static bool
tivoGfxGetKeySim( DFBInputDeviceKeyIdentifier* pKeyId )
{
    const char* keyDataPath = "/tmp/dfb-key";

    int keyDataFd;
    char keyData[ 32 ];
    DFBInputDeviceKeyIdentifier theKey;

    keyDataFd = open( keyDataPath, O_RDONLY );
    if (keyDataFd == -1)
    {
        return false;
    }

    memset( &keyData, 0, sizeof(keyData) );
    (void) read( keyDataFd, keyData, sizeof(keyData) - 1 );

    (void) close( keyDataFd );
    keyDataFd = -1;

    (void) unlink( keyDataPath );

    theKey = DIKI_UNKNOWN;

    switch (keyData[ 0 ])
    {
        case 'u':   theKey = DIKI_UP;           break;
        case 'd':   theKey = DIKI_DOWN;         break;
        case 'l':   theKey = DIKI_LEFT;         break;
        case 'r':   theKey = DIKI_RIGHT;        break;
        case 's':   theKey = DIKI_ENTER;        break; // select
        case 't':   theKey = DIKI_TAB;          break;
        case 'p':   theKey = DIKI_PRINT;        break;
        case '+':   theKey = DIKI_KP_PLUS;      break; // power
        case 'e':   theKey = DIKI_ESCAPE;       break; // will quit app
    }

    if (theKey == DIKI_UNKNOWN)
    {
        printf( "TiVo Gfx: simulated input key not recognized!\n" );
        return false;
    }

    printf( "TiVo Gfx: simulating key press %s\n",
        TvGfxUtil_GetKeyIdentifierDebugStr( theKey ) );

    *pKeyId = theKey;
    return true;
}

/******************************************************************************/

/*
 * declaration of private data
 */
typedef struct
{
    int                 magic;
    CoreInputDevice   * device;
    ref<TmkThread>      thread;
    bool                stop;
}
TiVoGfxInputData;

/*
 * Input thread reading from device.
 * Generates events on incoming data.
 */
static int
tivoGfxEventThread( long arg )
{
    direct_thread_set_name( "TiVo Gfx input" );

    D_DEBUG_AT( tivogfx_input, "%s() running\n", __FUNCTION__ );

    TiVoGfxInputData * pGfxInputData = (TiVoGfxInputData *) arg;
    D_MAGIC_ASSERT( pGfxInputData, TiVoGfxInputData );

    TiVoGfxSystemData * pGfxSystemData = TvGfx_GetSystemData();
    D_MAGIC_ASSERT( pGfxSystemData, TiVoGfxSystemData );

    while (!pGfxInputData->stop)
    {
        DFBInputEvent evt;

        memset( &evt, 0, sizeof(evt) );

        if (TvGfxProxy_GetInputEvent( pGfxSystemData->hGfxProxy,
            &evt ))
        {
            TvGfx_InputDispatch( pGfxInputData->device, &evt );
        }

        memset( &evt, 0, sizeof(evt) );

        if (tivoGfxGetKeySim( &evt.key_id ))
        {
            evt.flags = DIEF_KEYID;

            evt.type = DIET_KEYPRESS;
            TvGfx_InputDispatch( pGfxInputData->device, &evt );

            // delay a bit else the key press may not be detected
            usleep( 100 * 1000 );

            evt.type = DIET_KEYRELEASE;
            TvGfx_InputDispatch( pGfxInputData->device, &evt );
        }

        usleep( 10 * 1000 );
    }

    D_DEBUG_AT( tivogfx_input, "%s() exiting\n", __FUNCTION__ );

    return 0;
}

/******************************************************************************/

/*
 * Open the device, allocate and fill private data, start input thread.
 */
void * TvGfx_InputOpen( CoreInputDevice * device )
{
    D_DEBUG_AT( tivogfx_input, "%s()\n", __FUNCTION__ );

    /* allocate and fill private data */
    TiVoGfxInputData * pGfxInputData = new TiVoGfxInputData();

    D_MAGIC_SET( pGfxInputData, TiVoGfxInputData );

    pGfxInputData->device = device;
    pGfxInputData->stop = false;

    TmkMempool* mempool = TmkInit::GetMainMempool();

    /* start input thread */
    pGfxInputData->thread =
        new (mempool) TmkThread( tivoGfxEventThread, "TiVoGfx_Input" );

    pGfxInputData->thread->SetRTPriority( TmkThread::RT_VIEWER );

    TvStatus status = pGfxInputData->thread->Start( (long) pGfxInputData );
    TvDebugAssert( status == TV_OK );

    /* return private data pointer */
    return pGfxInputData;
}

/*
 * End thread, close device and free private data.
 */
void TvGfx_InputClose( void * driver_data )
{
    D_DEBUG_AT( tivogfx_input, "%s()\n", __FUNCTION__ );

    TiVoGfxInputData * pGfxInputData = (TiVoGfxInputData *) driver_data;
    D_MAGIC_ASSERT( pGfxInputData, TiVoGfxInputData );

    /* stop input thread */
    pGfxInputData->stop = true;

    pGfxInputData->thread->Join();

    pGfxInputData->thread = NULL;

    /* free private data */
    D_MAGIC_CLEAR( pGfxInputData );
    delete pGfxInputData;
}

/******************************************************************************/

