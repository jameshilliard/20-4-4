//////////////////////////////////////////////////////////////////////
//
// File: tivogfx_input_ipc.cpp
//
// Copyright 2013 TiVo Inc.  All Rights Reserved.
//
//////////////////////////////////////////////////////////////////////

// always include first
#include "tivogfx_directfb.h"

// other directfb includes
#include <directfb_util.h>

// source module includes
#include "tivogfx_input_ipc.h"

#include "tivo_directfb_ipc_defs.h"

#include <errno.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include "tivogfx_util.h"

// get X86 / MIPS definitions
#include <util/tvtypes.h>

#include <tmk/TmkThread.h>

//####################################################################

D_DEBUG_DOMAIN( tivogfx_ipc, "tivogfx/ipc", "TiVo Gfx IPC" );

#if 1
    // enable verbose diagnostics if needed
    D_DEBUG_DOMAIN( tivogfx_ipc_x, "tivogfx_x/ipc", "TiVo Gfx IPC" );

    #define D_DEBUG_AT_X( d, x... ) \
        D_DEBUG_AT( d##_x, x )
#else
    #define D_DEBUG_AT_X( d, x... )
#endif

//####################################################################

/**
 * TvGfxPrv_Socket_Read
 */
static bool TvGfxPrv_Socket_Read( int fd, void* p, size_t n )
{
    int     err;
    ssize_t len;

    // socket should be open
    TvDebugAssert( fd != -1 );

    D_DEBUG_AT_X( tivogfx_ipc, "%s(): reading socket %u bytes\n",
        __FUNCTION__, n );

    len = recv( fd, p, n, 0 );
    if (len < 0)
    {
        err = errno;
        D_DEBUG_AT( tivogfx_ipc, "%s(): recv error: %s\n",
            __FUNCTION__, strerror(err) );
        return false;
    }

    if (len == 0)
    {
        D_DEBUG_AT( tivogfx_ipc, "%s(): socket closed\n",
            __FUNCTION__ );
        return false;
    }

    // XXX need to fix this for loop and retry
    if (len != (ssize_t) n)
    {
        D_DEBUG_AT( tivogfx_ipc, "%s(): recv not enough data\n",
            __FUNCTION__ );
        return false;
    }

    D_DEBUG_AT_X( tivogfx_ipc, "%s(): read finished\n",
        __FUNCTION__ );
    return true;
}

//####################################################################

/**
 * globals
 */

static TvGfxProxy       hGfxProxyG      = NULL;
static ref<TmkThread>   pIpcThreadG     = NULL;
static bool             quitG           = false;
static int              socketFdG       = -1;

/**
 * TvGfxPrx_Ipc_Fatal
 */
static void TvGfxPrx_Ipc_Fatal( void )
{
    quitG = true;
}

/**
 * TvGfxPrv_Ipc_HandleEvent_Key
 */
static void TvGfxPrv_Ipc_HandleEvent_Key(
    const TiVoDirectFbIpcEventKey* pIpcEventKey )
{
    D_DEBUG_AT( tivogfx_ipc, "%s(): event type=%s keyId=%s keySym=%s\n",
        __FUNCTION__,
        dfb_input_event_type_name( pIpcEventKey->dfbEventType ),
        TvGfxUtil_GetKeyIdentifierDebugStr( pIpcEventKey->dfbKeyId ),
        TvGfxUtil_GetKeySymbolDebugStr( pIpcEventKey->dfbKeySymbol ) );

    // should do more sanity checks here!

    TvDebugAssert(
        (pIpcEventKey->dfbEventType == DIET_KEYPRESS) ||
        (pIpcEventKey->dfbEventType == DIET_KEYRELEASE)
        );

    DFBInputEvent event;

    memset( &event, 0, sizeof( event ) );

    event.type        = pIpcEventKey->dfbEventType;
    event.flags       = pIpcEventKey->dfbEventFlags;
    event.timestamp   = pIpcEventKey->dfbTimestamp;
    event.key_code    = pIpcEventKey->dfbKeyCode;
    event.key_id      = pIpcEventKey->dfbKeyId;
    event.key_symbol  = pIpcEventKey->dfbKeySymbol;
    event.modifiers   = pIpcEventKey->dfbModifiers;

    (void) TvGfxProxy_PutInputEvent( hGfxProxyG, &event );
}

/**
 * TvGfxPrv_Ipc_HandleEvent_Quit
 */
static void TvGfxPrv_Ipc_HandleEvent_Quit(
    const TiVoDirectFbIpcEventQuit* pIpcEventQuit )
{
    D_DEBUG_AT( tivogfx_ipc, "%s()\n", __FUNCTION__ );

    TIVOGFX_UNUSED_PARAMETER( pIpcEventQuit );

    #if 1
        // try sending directfb-recognized meta-key press
        DFBInputEvent event;

        memset( &event, 0, sizeof( event ) );

        event.type        = DIET_KEYPRESS;
        event.flags       = (DFBInputEventFlags) (DIEF_KEYSYMBOL | DIEF_MODIFIERS);
        //event.key_id      = DIKI_ESCAPE;
        event.key_symbol  = DIKS_ESCAPE;
        event.modifiers   = DIMM_META;

        #if 0
            // XXX TO DO: bypass event queue

            D_DEBUG_AT( tivogfx_ipc, "%s(): posting ESCAPE meta-key\n",
                __FUNCTION__ );

            dfb_input_dispatch( pGfxInputData->device, &event );
        #else
            D_DEBUG_AT( tivogfx_ipc, "%s(): posting ESCAPE meta-key to proxy\n",
                __FUNCTION__ );

            (void) TvGfxProxy_PutInputEvent( hGfxProxyG, &event );
        #endif
    #else
        D_DEBUG_AT( tivogfx_ipc, "%s(): killing with SIGINT!\n",
            __FUNCTION__ );

        // try sending control-C
        //kill( getpid(), SIGINT );
        kill( 0, SIGINT );
    #endif
}

/**
 * TvGfxPrv_Ipc_HandleEvent
 */
static void TvGfxPrv_Ipc_HandleEvent( void )
{
    u32 eventDataSize;
    u8  eventData[ TIVO_DIRECTFB_IPC_EVENT_MAX_SIZE ];
    u32 ipcEventType;

    D_DEBUG_AT_X( tivogfx_ipc, "%s(): entered\n",
        __FUNCTION__ );

    eventDataSize = 0;

    if (!TvGfxPrv_Socket_Read( socketFdG,
        &eventDataSize, sizeof(eventDataSize) ))
    {
        D_DEBUG_AT( tivogfx_ipc, "%s(): socket read failed\n",
            __FUNCTION__ );

        // broken connection is fatal
        TvGfxPrx_Ipc_Fatal();
        return;
    }

    D_DEBUG_AT_X( tivogfx_ipc, "%s(): got event size %u\n",
        __FUNCTION__, eventDataSize );

    if ((eventDataSize < TIVO_DIRECTFB_IPC_EVENT_MIN_SIZE) ||
        (eventDataSize > TIVO_DIRECTFB_IPC_EVENT_MAX_SIZE))
    {
        D_DEBUG_AT( tivogfx_ipc, "%s(): socket read invalid: "
            "invalid event data size %u\n", __FUNCTION__, eventDataSize );

        // invalid data is fatal
        TvGfxPrx_Ipc_Fatal();
        return;
    }

    memset( &eventData, 0, sizeof(eventData) );

    if (!TvGfxPrv_Socket_Read( socketFdG, &eventData, eventDataSize ))
    {
        D_DEBUG_AT( tivogfx_ipc, "%s(): socket read failed\n",
            __FUNCTION__ );

        // broken connection is fatal
        TvGfxPrx_Ipc_Fatal();
        return;
    }

    ipcEventType =
        ((const TiVoDirectFbIpcEventHeader*) &eventData)->ipcEventType;

    D_DEBUG_AT_X( tivogfx_ipc, "%s(): got ipc event type %u\n",
        __FUNCTION__, ipcEventType );

    if (ipcEventType == TIVO_DIRECTFB_IPC_EVENT_KEY)
    {
        if (eventDataSize != sizeof(TiVoDirectFbIpcEventKey))
        {
            D_DEBUG_AT( tivogfx_ipc, "%s(): bad size for %s: %u vs %u\n",
                __FUNCTION__, "TiVoDirectFbIpcEventKey",
                eventDataSize, sizeof(TiVoDirectFbIpcEventKey) );

            // invalid data is fatal
            TvGfxPrx_Ipc_Fatal();
            return;
        }

        TvGfxPrv_Ipc_HandleEvent_Key(
            (const TiVoDirectFbIpcEventKey*) &eventData );
        return;
    }
    else if (ipcEventType == TIVO_DIRECTFB_IPC_EVENT_QUIT)
    {
        if (eventDataSize != sizeof(TiVoDirectFbIpcEventQuit))
        {
            D_DEBUG_AT( tivogfx_ipc, "%s(): bad size for %s: %u vs %u\n",
                __FUNCTION__, "TiVoDirectFbIpcEventQuit",
                eventDataSize, sizeof(TiVoDirectFbIpcEventQuit) );

            // invalid data is fatal
            TvGfxPrx_Ipc_Fatal();

            return;
        }

        TvGfxPrv_Ipc_HandleEvent_Quit(
            (const TiVoDirectFbIpcEventQuit*) &eventData );
        return;
    }

    D_DEBUG_AT( tivogfx_ipc, "%s(): socket read invalid: "
        "unknown IPC event type %u\n", __FUNCTION__, ipcEventType );

    // invalid data is fatal
    TvGfxPrx_Ipc_Fatal();
    return;
}

/**
 * TvGfxPrv_Ipc_CheckAndHandleEvent
 */
static void TvGfxPrv_Ipc_CheckAndHandleEvent( void )
{
    fd_set          fds;
    int             maxfds;
    int             result;
    int             err;
    struct timeval  timeout;

    //D_DEBUG_AT_X( tivogfx_ipc, "%s()\n", __FUNCTION__ );

    FD_ZERO( &fds );
    maxfds = 0;

    // watch the data socket
    FD_SET( socketFdG, &fds );
    maxfds = MAX( maxfds, socketFdG );

    timeout.tv_sec = 0;
    timeout.tv_usec = 10 * 1000;// 10 ms

    // wait for data
    result = select( maxfds + 1, &fds, NULL, NULL, &timeout );
    if (result < 0)
    {
        err = errno;
        if (err == EINTR)
        {
            D_DEBUG_AT( tivogfx_ipc, "%s(): select got EINTR, ignoring\n",
                __FUNCTION__ );
            return;
        }

        D_DEBUG_AT( tivogfx_ipc, "%s(): select failed: %s\n",
            __FUNCTION__, strerror( err ) );

        // broken connection is fatal
        TvGfxPrx_Ipc_Fatal();
        return;
    }

    if (FD_ISSET( socketFdG, &fds ))
    {
        TvGfxPrv_Ipc_HandleEvent();
        return;
    }

    //D_DEBUG_AT( tivogfx_ipc, "%s(): select not handled, ignoring\n",
    //    __FUNCTION__ );
    return;
}

/**
 * TvGfxPrv_Ipc_Acquire
 */
static bool TvGfxPrv_Ipc_Acquire( void )
{
    const char*         socketPath;
    int                 fd;
    struct sockaddr_un  saUnix;
    int                 result;
    int                 err;

    D_DEBUG_AT( tivogfx_ipc, "%s()\n", __FUNCTION__ );

    if (TvGfxUtil_InHandcraftMode())
    {
        D_DEBUG_AT( tivogfx_ipc, "%s(): handcraft is set\n",
            __FUNCTION__ );
        return false;
    }

    socketPath = getenv( TIVO_DIRECTFB_IPC_SOCKET_ENV );
    if ((socketPath == NULL) || (*socketPath == 0))
    {
        D_DEBUG_AT( tivogfx_ipc, "%s(): %s is not set\n",
            __FUNCTION__, TIVO_DIRECTFB_IPC_SOCKET_ENV );
        return false;
    }

    D_DEBUG_AT_X( tivogfx_ipc, "%s(): creating socket\n", __FUNCTION__ );
    fd = socket( AF_UNIX, SOCK_STREAM, 0 );
    if (fd < 0)
    {
        err = errno;
        D_DEBUG_AT( tivogfx_ipc, "%s(): socket error: %s\n",
            __FUNCTION__, strerror(err) );
        return false;
    }

    memset( &saUnix, 0, sizeof(saUnix) );
    saUnix.sun_family = AF_UNIX;
    (void) strncpy( saUnix.sun_path, socketPath, sizeof(saUnix.sun_path) - 1 );
    saUnix.sun_path[ sizeof(saUnix.sun_path) - 1 ] = 0;

    D_DEBUG_AT_X( tivogfx_ipc, "%s(): connecting socket %s\n",
        __FUNCTION__, saUnix.sun_path );
    result = connect( fd, (struct sockaddr *) &saUnix,
        sizeof(saUnix.sun_family) + strlen(saUnix.sun_path) );
    if (result < 0)
    {
        err = errno;
        D_DEBUG_AT( tivogfx_ipc, "%s(): connect error: %s\n",
            __FUNCTION__, strerror(err) );

        result = close( fd );
        if (result != 0)
        {
            err = errno;
            D_DEBUG_AT( tivogfx_ipc, "%s(): close error: %s\n",
                __FUNCTION__, strerror(err) );
        }

        return false;
    }

    socketFdG = fd;

    D_DEBUG_AT( tivogfx_ipc, "%s() success\n", __FUNCTION__ );
    return true;
}

/**
 * TvGfxPrv_Ipc_Release
 */
static void TvGfxPrv_Ipc_Release( void )
{
    int     result;
    int     err;

    D_DEBUG_AT( tivogfx_ipc, "%s()\n", __FUNCTION__ );

    if (socketFdG < 0)
    {
        // not open
        return;
    }

    D_DEBUG_AT_X( tivogfx_ipc, "%s(): closing socket\n", __FUNCTION__ );

    result = close( socketFdG );
    if (result != 0)
    {
        err = errno;
        D_DEBUG_AT( tivogfx_ipc, "%s(): close error: %s\n", __FUNCTION__,
            strerror(err) );
    }

    socketFdG = -1;
}

/**
 * TvGfxPrv_IpcThread
 */
static int TvGfxPrv_IpcThread( long arg )
{
    direct_thread_set_name( "TiVo Gfx IPC input" );

    D_DEBUG_AT( tivogfx_ipc, "%s() entered\n", __FUNCTION__ );

    // just use the global for now
    (void) arg;

    if (!TvGfxPrv_Ipc_Acquire())
    {
        // diagnostic already printed
    }
    else
    {
        D_DEBUG_AT_X( tivogfx_ipc, "%s(): running\n", __FUNCTION__ );

        while (!quitG)
        {
            TvGfxPrv_Ipc_CheckAndHandleEvent();
        }

        TvGfxPrv_Ipc_Release();
    }

    D_DEBUG_AT( tivogfx_ipc, "%s() exiting\n", __FUNCTION__ );

    return 0;
}

//####################################################################

/**
 * TvGfx_IpcInputOpen
 */
void TvGfx_IpcInputOpen( TvGfxProxy hGfxProxy )
{
    D_DEBUG_AT( tivogfx_ipc, "%s() enter\n", __FUNCTION__ );

    hGfxProxyG = hGfxProxy;

    D_DEBUG_AT_X( tivogfx_ipc, "%s(): creating thread\n", __FUNCTION__ );

    TmkMempool* mempool = TmkInit::GetMainMempool();

    pIpcThreadG = new (mempool) TmkThread( TvGfxPrv_IpcThread, "TiVoGfx_Ipc" );

    pIpcThreadG->SetRTPriority( TmkThread::RT_VIEWER );

    D_DEBUG_AT_X( tivogfx_ipc, "%s(): starting thread\n", __FUNCTION__ );

    TvStatus status = pIpcThreadG->Start( NULL );
    TvDebugAssert( status == TV_OK );

    D_DEBUG_AT( tivogfx_ipc, "%s() exit\n", __FUNCTION__ );
}

/**
 * TvGfx_IpcInputClose
 */
void TvGfx_IpcInputClose( void )
{
    D_DEBUG_AT( tivogfx_ipc, "%s() enter\n", __FUNCTION__ );

    quitG = true;

    D_DEBUG_AT_X( tivogfx_ipc, "%s(): waiting for thread\n", __FUNCTION__ );

    pIpcThreadG->Join();

    D_DEBUG_AT_X( tivogfx_ipc, "%s(): destroying thread\n", __FUNCTION__ );

    pIpcThreadG = NULL;

    D_DEBUG_AT( tivogfx_ipc, "%s() exit\n", __FUNCTION__ );
}

//####################################################################

