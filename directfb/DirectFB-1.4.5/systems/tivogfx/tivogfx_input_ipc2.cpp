//////////////////////////////////////////////////////////////////////
//
// File: tivogfx_input_ipc2.cpp
//
// Copyright 2014 TiVo Inc.  All Rights Reserved.
//
//////////////////////////////////////////////////////////////////////

// always include first
#include "tivogfx_directfb.h"
#include "tivo_directfb_keymap.h"
// other directfb includes
#include <directfb_util.h>

#include <core/input.h>

// source module includes
#include "tivogfx_input_ipc2.h"

#include <string.h>

#include "tivogfx_util.h"

// get X86 / MIPS definitions
#include <util/tvtypes.h>

#include <stx/StxIpc.h>
#include <stx/StxCommandsEvents.h>

#include <tmk/TmkActivity.h>
#include <tmk/TmkCoreSemaphore.h>
#include <tmk/TmkDebugPrint.h>
#include <tmk/TmkFeatures.h>
#include <tmk/TmkInit.h>
#include <tmk/TmkListener.h>
#include <tmk/TmkLogger.h>
#include <tmk/TmkQueue.h>
#include <tmk/TmkThread.h>

SETUP_DEBUG_ENV( "tivogfx_ipc" );

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
 * globals
 */

static IDirectFBEventBuffer *tivogfx_eventBuffer = NULL;

static ref<TmkCoreSemaphore> pRequestShutdownSemaG;

static ref<TmkCoreSemaphore> pAwaitShutdownSemaG;

static TvGfxProxy hGfxProxyG = NULL;

static int nDfbIpcActivities = 0;

class DfbIpcActivity : public TmkActivity
{
public:
    DfbIpcActivity(
        const TmkInitInfo* pInitInfo,
        ref<TmkCoreSemaphore> pRequestShutdownSema,
        ref<TmkCoreSemaphore> pAwaitShutdownSema,
        ref<TmkCoreSemaphore> pAwaitStartupSema
        );

    ~DfbIpcActivity();

    void Initialize();

    void Cleanup();

private:

    ref<TmkQueue<StxEvent> > pStxEventQueueM;

    TMK_LISTENER_PROXY(
        DfbIpcActivity, // this class name
        TmkQueueEvent<StxEvent>, // event type
        queueProxyM, // proxy member variable
        ProcessQueueEvent ); // event handler method
};

DfbIpcActivity::DfbIpcActivity(
    const TmkInitInfo* pInitNode,
    ref<TmkCoreSemaphore> pRequestShutdownSema,
    ref<TmkCoreSemaphore> pAwaitShutdownSema,
    ref<TmkCoreSemaphore> pAwaitStartupSema
    ) :
    TmkActivity(
        "DFB IPC INPUT Activity",
        pInitNode,
        /* pRequestShutdownSema */ pRequestShutdownSema,
        /* pAwaitShutdownSema   */ pAwaitShutdownSema,
        /* pRequestStartupSema  */ NULL,
        /* pAwaitStartupSema    */ pAwaitStartupSema )
    , pStxEventQueueM ( NULL )
{
    DEBUG_SCOPE( 2, __PRETTY_FUNCTION__ );

    D_DEBUG_AT( tivogfx_ipc, "%s() enter\n", __FUNCTION__ );

    pStxEventQueueM = new TmkQueue<StxEvent>;

    this->SetPriority( TmkThread::RT_VIEWER );

    nDfbIpcActivities++;

    D_DEBUG_AT( tivogfx_ipc, "%s() exit\n", __FUNCTION__ );
}

DfbIpcActivity::~DfbIpcActivity()
{
    DEBUG_SCOPE( 2, __PRETTY_FUNCTION__ );

    nDfbIpcActivities--;

    D_DEBUG_AT( tivogfx_ipc, "%s() exit\n", __FUNCTION__ );
}

void DfbIpcActivity::Initialize()
{
    DEBUG_SCOPE( 2, __PRETTY_FUNCTION__ );

    D_DEBUG_AT( tivogfx_ipc, "%s() enter\n", __FUNCTION__ );

    StxIpc::RegisterForEvents(
            (StxTypeMask) ( STX_APP_EVENT_TYPE_MASK | STX_CUSTOM_EVENT_TYPE_MASK | STX_DFB_EVENT_TYPE_MASK ),
            pStxEventQueueM );

    pStxEventQueueM->SetListener( &queueProxyM );

    TmkThread::Self()->SetRTPriority( TmkThread::RT_VIEWER );
    if (TmkFeatures::FEnabled(eTvFeatureTmkProcessorAffinity))
    {
        // apply processor affinity
        TmkThread::Self()->SetProcessorAffinity( TmkThread::PROCESSOR_AFFINITY_UI );
    }

    D_DEBUG_AT( tivogfx_ipc, "%s() exit\n", __FUNCTION__ );
}

void DfbIpcActivity::Cleanup()
{
    DEBUG_SCOPE( 2, __PRETTY_FUNCTION__ );

    D_DEBUG_AT( tivogfx_ipc, "%s() enter\n", __FUNCTION__ );

    pStxEventQueueM->ClearListener();

    StxIpc::UnregisterForEvents( pStxEventQueueM );

    TvDebugAssert( pStxEventQueueM->Count() == 0 );

    pStxEventQueueM = NULL;

    D_DEBUG_AT( tivogfx_ipc, "%s() exit\n", __FUNCTION__ );
}

static void PostUserEvent( int evtType, void* data, size_t len )
{
    DEBUG_SCOPE( 2, __PRETTY_FUNCTION__ );

    DFBEvent event;
    memset( &event, 0, sizeof( event ) );
    event.clazz = DFEC_USER; // unnecessary since user type has it set
    event.user.type = evtType;

    void * newBuffer = NULL;
    if (data != NULL)
    {
        // and now malloc a buffer that will not go away when we post it
        // receiver must free it...
        newBuffer = malloc( len );
        memcpy( newBuffer, data, len );
    }
    event.user.data = newBuffer;

    TvCrashAssert( tivogfx_eventBuffer != NULL,
        "Unable to PostUserEvent(), tivogfx_eventBuffer is NULL. "
            "Was a non-NULL IDirectFBEventBuffer passed to StxIpc::Run()?" );
    tivogfx_eventBuffer->PostEvent( tivogfx_eventBuffer, &event );
}

static void PostSimpleBagKeyValuesUserEvent( int actualEvtType )
{
    StxBuffWriter data;
    data.WriteU32( actualEvtType );
    // Nothing else in the event
    data.WriteU32( 0 );

    DBG_PRINTF( "%s: EventType=0x%08X length=%d\n",
            __PRETTY_FUNCTION__, actualEvtType, data.GetLength() );

    PostUserEvent( TIVO_DFB_USER_TYPE_BAG_KEY_VALUES, (void*) data.GetBuffer(),
            data.GetLength() );
}

void DfbIpcActivity::ProcessQueueEvent( TmkQueueEvent<StxEvent>* pEvent )
{
    DEBUG_SCOPE( 2, __PRETTY_FUNCTION__ );

    DBG_PRINTF( "queue count=%d\n", pStxEventQueueM->Count() );

    if ( pEvent->GetSource() != pStxEventQueueM )
    {
        DBG_PRINTF( "%s: incorrect source\n", __PRETTY_FUNCTION__ );
        return;
    }

    ref<StxEvent> pEvt = pEvent->GetItem();
    if ( pEvt == NULL )
    {
        DBG_PRINTF( "%s: no item?\n", __PRETTY_FUNCTION__ );
        return;
    }

    DBG_PRINTF( "pEvt->typeM = 0x%08X\n", pEvt->typeM );

    if ( pEvt->typeM & STX_CUSTOM_EVENT_TYPE_MASK )
    {
        ref<StxCustomEvent> pCustomEvt = dynamic_cast<StxCustomEvent*>( pEvt.Get() );
        if ( ! pCustomEvt )
        {
            DBG_PRINTF( "NOT a CUSTOM EVENT!\n" );
            return;
        }

        StxBuffReader* data = pCustomEvt->pDataBuf;
        DBG_PRINTF( "%s: STX_CUSTOM_EVENT_TYPE_MASK length=%d\n",
                __PRETTY_FUNCTION__, data->GetLength() );

        PostUserEvent( TIVO_DFB_USER_TYPE_VOID_STAR, data->GetBuffer(),
                data->GetLength() );
    }
    else if ( pEvt->typeM == TV_STX_RUNNING_EVENT )
    {
        ref<StxRunningEvent> pRunningEvt = dynamic_cast<StxRunningEvent*>( pEvt.Get() );
        if ( ! pRunningEvt )
        {
            DBG_PRINTF( "NOT a RUNNING EVENT!\n" );
            return;
        }

        tivogfx_eventBuffer = (IDirectFBEventBuffer *) pRunningEvt->pc;
        DBG_PRINTF( "%s: TV_STX_RUNNING_EVENT pc=0x%08X\n", __PRETTY_FUNCTION__,
                (int) pRunningEvt->pc );
    }
    else if ( pEvt->typeM == TV_STX_ATTACH_VIDEO_EVENT )
    {
        ref<StxAttachVideoEvent> pAttachVideoEvt = dynamic_cast<StxAttachVideoEvent*>( pEvt.Get() );
        if ( ! pAttachVideoEvt )
        {
            DBG_PRINTF( "NOT a ATTACH VIDEO EVENT!\n" );
            return;
        }

        DBG_PRINTF( "%s: TV_STX_ATTACH_VIDEO_EVENT x=%d y=%d w=%d h=%d\n",
                __PRETTY_FUNCTION__, (int) pAttachVideoEvt->x, (int) pAttachVideoEvt->y,
                (int) pAttachVideoEvt->w, (int) pAttachVideoEvt->h );

        DFBRectangle rect;
        rect.x = pAttachVideoEvt->x;
        rect.y = pAttachVideoEvt->y;
        rect.w = pAttachVideoEvt->w;
        rect.h = pAttachVideoEvt->h;
        TvGfxProxy_AttachVideo( hGfxProxyG, &rect );
    }
    else if ( pEvt->typeM == TV_STX_EVENT_KEY )
    {
        ref<StxKeyEvent> pKeyEvt = dynamic_cast<StxKeyEvent*>( pEvt.Get() );
        if ( ! pKeyEvt )
        {
            DBG_PRINTF( "NOT a KEY EVENT!\n" );
            return;
        }

        DFBInputEvent inputEvent;
        memset( &inputEvent, 0, sizeof( inputEvent ) );
        if (PopulateDFBInputEvent(pKeyEvt, &inputEvent)) {
            (void) TvGfxProxy_PutInputEvent( hGfxProxyG, &inputEvent );
        }
    }
    else if ( pEvt->typeM == TV_STX_EVENT_QUIT )
    {
        DBG_PRINTF( "%s: quit requested! \n", __PRETTY_FUNCTION__ );

        ref<StxQuitEvent> pQuitEvt = dynamic_cast<StxQuitEvent*>( pEvt.Get() );
        if ( ! pQuitEvt )
        {
            DBG_PRINTF( "NOT a QUIT EVENT!\n" );
            return;
        }

        PostSimpleBagKeyValuesUserEvent( (int) TV_STX_EVENT_QUIT );

        TmkLogger::Log(TmkLogger::facExec, TmkLogger::priInfo,  "tivogfx_ipc",
                "DfbIpcActivity has received TV_STX_EVENT_QUIT event");

        // don't listen for any more events, we are done...
        pStxEventQueueM->ClearListener();

        DBG_PRINTF( "%s: TV_STX_EVENT_QUIT \n", __PRETTY_FUNCTION__ );
    }
    else if ( pEvt->typeM == TV_STX_EVENT_VIDEO_STATUS )
    {
        ref<StxVideoStatusEvent> pVideoStatusEvt = dynamic_cast<StxVideoStatusEvent*>( pEvt.Get() );
        if ( ! pVideoStatusEvt )
        {
            DBG_PRINTF( "NOT a VIDEO STATUS EVENT!\n" );
            return;
        }

        // transmogrify video status event into bag of strings...
        // slam status into bag...
        pVideoStatusEvt->infoMap.SetEntryV( TV_STX_STATUS_EVENT_INFO_KEY_STATUS,
                "%d", (int) pVideoStatusEvt->videoStatus );
        // and now get those bytes...
        StxBuffWriter data;
        data.WriteU32( (int) TV_STX_EVENT_VIDEO_STATUS );
        data.WriteU32( pVideoStatusEvt->infoMap.GetLength() );
        data.WriteInfoMap( pVideoStatusEvt->infoMap );
        DBG_PRINTF( "%s: TV_STX_EVENT_VIDEO_STATUS length=%d\n",
                __PRETTY_FUNCTION__, data.GetLength() );

        PostUserEvent( TIVO_DFB_USER_TYPE_BAG_KEY_VALUES, (void*) data.GetBuffer(),
                data.GetLength() );
    }
    else if ( pEvt->typeM == TV_STX_EVENT_SUSPEND )
    {
        ref<StxSuspendEvent> pSuspendEvt = dynamic_cast<StxSuspendEvent*>( pEvt.Get() );
        if ( ! pSuspendEvt )
        {
            DBG_PRINTF( "NOT a SUSPEND EVENT!\n" );
            return;
        }

        PostSimpleBagKeyValuesUserEvent( (int) TV_STX_EVENT_SUSPEND );
        DBG_PRINTF( "%s: TV_STX_EVENT_SUSPEND\n", __PRETTY_FUNCTION__ );
    }
    else if ( pEvt->typeM == TV_STX_EVENT_RESUME )
    {
        ref<StxResumeEvent> pSuspendEvt = dynamic_cast<StxResumeEvent*>( pEvt.Get() );
        if ( ! pSuspendEvt )
        {
            DBG_PRINTF( "NOT a RESUME EVENT!\n" );
            return;
        }

        PostSimpleBagKeyValuesUserEvent( (int) TV_STX_EVENT_RESUME );
        DBG_PRINTF( "%s: TV_STX_EVENT_RESUME\n", __PRETTY_FUNCTION__ );
    }
    else if ( pEvt->typeM == TV_STX_EVENT_START )
    {
        DBG_PRINTF( "%s: start requested! \n", __PRETTY_FUNCTION__ );

        ref<StxStartEvent> pStartEvt = dynamic_cast<StxStartEvent*>( pEvt.Get() );
        if ( ! pStartEvt )
        {
            DBG_PRINTF( "NOT a START EVENT!\n" );
            return;
        }

        TvDebugAssert( tivogfx_eventBuffer != NULL, "should have event buffer by now!" );

        PostSimpleBagKeyValuesUserEvent( (int) TV_STX_EVENT_START );

        DBG_PRINTF( "%s: TV_STX_EVENT_START \n", __PRETTY_FUNCTION__ );
    }
    else if ( pEvt->typeM == TV_STX_EVENT_MUSIC_STATUS )
    {
        ref<StxMusicStatusEvent> pMusicStatusEvt = dynamic_cast<StxMusicStatusEvent*>( pEvt.Get() );
        if ( ! pMusicStatusEvt )
        {
            DBG_PRINTF( "NOT a MUSIC STATUS EVENT!\n" );
            return;
        }

        // transmogrify music status event into bag of strings...
        // slam status into bag...
        // create a bag first...
        StxInfoMap infoMap;

        infoMap.SetEntryV( TV_STX_STATUS_EVENT_INFO_KEY_STATUS,
                "%d", (int) pMusicStatusEvt->musicStatus );
        // and now get those bytes...
        StxBuffWriter data;
        data.WriteU32( (int) TV_STX_EVENT_MUSIC_STATUS );
        data.WriteU32( infoMap.GetLength() );
        data.WriteInfoMap( infoMap );
        DBG_PRINTF( "%s: TV_STX_EVENT_MUSIC_STATUS length=%d status=%d\n",
                __PRETTY_FUNCTION__, data.GetLength(),  (int) pMusicStatusEvt->musicStatus );

        PostUserEvent( TIVO_DFB_USER_TYPE_BAG_KEY_VALUES, (void*) data.GetBuffer(),
                data.GetLength() );
    }
    else
    {
        TvDebugAssert( false, "unknown event" );
    }
}

/**
 * TvGfx_IpcInputOpen2
 */
void TvGfx_IpcInputOpen2( TvGfxProxy hGfxProxy )
{
    DEBUG_SCOPE( 2, __PRETTY_FUNCTION__ );

    D_DEBUG_AT( tivogfx_ipc, "%s() enter\n", __FUNCTION__ );

    hGfxProxyG = hGfxProxy;

    if (TvGfxUtil_InHandcraftMode())
    {
        D_DEBUG_AT( tivogfx_ipc, "%s(): handcraft is set\n",
            __FUNCTION__ );
        return;
    }

    TmkMempool* mempool = TmkInit::GetMainMempool();

//    StxIpc::Initialize( mempool ); // done by init node

    ref<TmkCoreSemaphore> pAwaitStartupSema =
        new ( mempool ) TmkCoreSemaphore( 0 );

    pRequestShutdownSemaG =
        new ( mempool ) TmkCoreSemaphore( 0 );

    pAwaitShutdownSemaG =
        new ( mempool ) TmkCoreSemaphore( 0 );

    CREATE_TMK_ACTIVITY(
        mempool,
        DfbIpcActivity(
            &StxIpc::initInfo,
            pRequestShutdownSemaG,
            pAwaitShutdownSemaG,
            pAwaitStartupSema
            ));

    pAwaitStartupSema->Acquire();

    D_DEBUG_AT( tivogfx_ipc, "%s() exit\n", __FUNCTION__ );
}

/**
 * TvGfx_IpcInputClose2
 */
void TvGfx_IpcInputClose2( void )
{
    DEBUG_SCOPE( 2, __PRETTY_FUNCTION__ );

    D_DEBUG_AT( tivogfx_ipc, "%s() enter\n", __FUNCTION__ );

    if (TvGfxUtil_InHandcraftMode())
    {
        D_DEBUG_AT( tivogfx_ipc, "%s(): handcraft is set\n",
            __FUNCTION__ );
        return;
    }

    // signal shut down to start...
    pRequestShutdownSemaG->Release();

    pRequestShutdownSemaG = NULL;

    // wait for shut down to complete...
    pAwaitShutdownSemaG->Acquire();

    pAwaitShutdownSemaG = NULL;

    // temp fix for bug 370406
    // wait for all the DfbIpcActivities to be released
    for(int i = 0; 0<nDfbIpcActivities && i<50; i++)
    {
        usleep(100000);
    }
    // final sleep to make sure we get out of the DfbIpcActivity destructor
    usleep(100000);

    D_DEBUG_AT( tivogfx_ipc, "%s() exit\n", __FUNCTION__ );
}

//####################################################################
