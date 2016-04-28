//////////////////////////////////////////////////////////////////////
//
// File: tivogfx_input_hpk.cpp
//
// Copyright 2012 TiVo Inc.  All Rights Reserved.
//
//////////////////////////////////////////////////////////////////////

// always include first
#include "tivogfx_directfb.h"

// source module includes
#include "tivogfx_input_hpk.h"

//#include <stdlib.h>
#include <string.h>
//#include <unistd.h>

#include "tivogfx_proxy.h"
#include "tivogfx_util.h"

#include <tmk/TmkMonotonicTime.h>

// get X86 / MIPS definitions
#include <util/tvtypes.h>

#ifdef DEBUG
#include "directfb_keynames.h"
#endif

#if BOOTARCH == X86
    #define TIVOGFX_ENABLE_HPK_REMOTE   0
#else
    #define TIVOGFX_ENABLE_HPK_REMOTE   1
#endif

//####################################################################

D_DEBUG_DOMAIN( tivogfx_input_hpk, "tivogfx/input/hpk", "TiVo Gfx input (HPK)" );

#if 1
    // enable verbose diagnostics if needed
    D_DEBUG_DOMAIN( tivogfx_input_hpk_x, "tivogfx_x/input/hpk", "TiVo Gfx input (HPK)" );

    #define D_DEBUG_AT_X( d, x... ) \
        D_DEBUG_AT( d##_x, x )
#else
    #define D_DEBUG_AT_X( d, x... )
#endif

//####################################################################

//###########################################################################
#if TIVOGFX_ENABLE_HPK_REMOTE
//###########################################################################

#include <tmk/TmkThread.h>

#include <hpk/HpkPlatform.h>
#include <hpk/HpkRemoteInputEvents.h>
#include <hpk/HpkReservable.h>

/**
 * globals
 */

static TvGfxProxy       hGfxProxyG      = NULL;
static ref<TmkThread>   pInputThreadG   = NULL;
static bool             quitG           = false;
static HPK_RemoteInput  hpkRemoteInputG;

/**
 * TvGfxPrv_HpkInput_TranslateRemoteInputEvent
 *
 * The keys and symbols that are mapped to match those mapped by
 * tivogfx_input_ipc2.cpp, which maps the STX identifier versions of these
 * input events to a standardized set of keys and symbols.
 */
static bool
TvGfxPrv_HpkInput_TranslateRemoteInputEvent(
    const HPK_RemoteInputPacket &hpkPacket,
    /*returns*/ DFBInputDeviceKeyIdentifier &dfbKey,
    /*returns*/ DFBInputDeviceKeySymbol     &dfbSym )
{
    dfbKey = DIKI_UNKNOWN;
    dfbSym = DIKS_NULL;

    switch( hpkPacket.event )
    {
        case HPK_REMOTEINPUT_EVENT_TIVO:
            dfbSym = DIKS_CUSTOM0;
            break;

        case HPK_REMOTEINPUT_EVENT_RECORD:
            dfbSym = DIKS_RECORD;
            break;

        case HPK_REMOTEINPUT_EVENT_PLAY:
            dfbSym = DIKS_PLAY;
            break;

        case HPK_REMOTEINPUT_EVENT_PAUSE:
            dfbKey = DIKI_PAUSE;
            dfbSym = DIKS_PLAYPAUSE;
            break;

        case HPK_REMOTEINPUT_EVENT_REVERSE:
            dfbSym = DIKS_REWIND;
            break;

        case HPK_REMOTEINPUT_EVENT_FORWARD:
            dfbSym = DIKS_FASTFORWARD;
            break;

        case HPK_REMOTEINPUT_EVENT_REPLAY:
            dfbKey = DIKI_BACKSPACE;
            dfbSym = DIKS_BACK;
            break;

        case HPK_REMOTEINPUT_EVENT_SLOW:
            dfbSym = DIKS_SLOW;
            break;

        case HPK_REMOTEINPUT_EVENT_ADVANCE:
            dfbSym = DIKS_NEXT;
            break;

        case HPK_REMOTEINPUT_EVENT_CLEAR:
            dfbSym = DIKS_CLEAR;
            break;

        case HPK_REMOTEINPUT_EVENT_ENTER:
            dfbKey = DIKI_ENTER;
            dfbSym = DIKS_ENTER;
            break;

        case HPK_REMOTEINPUT_EVENT_CHANNELUP:
            dfbSym = DIKS_CHANNEL_UP;
            break;

        case HPK_REMOTEINPUT_EVENT_CHANNELDOWN:
            dfbSym = DIKS_CHANNEL_DOWN;
            break;

        case HPK_REMOTEINPUT_EVENT_GUIDE:
            dfbSym = DIKS_EPG;
            break;

        case HPK_REMOTEINPUT_EVENT_UP:
            dfbKey = DIKI_UP;
            dfbSym = DIKS_CURSOR_UP;
            break;

        case HPK_REMOTEINPUT_EVENT_DOWN:
            dfbKey = DIKI_DOWN;
            dfbSym = DIKS_CURSOR_DOWN;
            break;

        case HPK_REMOTEINPUT_EVENT_LEFT:
            dfbKey = DIKI_LEFT;
            dfbSym = DIKS_CURSOR_LEFT;
            break;

        case HPK_REMOTEINPUT_EVENT_RIGHT:
            dfbKey = DIKI_RIGHT;
            dfbSym = DIKS_CURSOR_RIGHT;
            break;

        case HPK_REMOTEINPUT_EVENT_SELECT:
            dfbKey = DIKI_ENTER;
            dfbSym = DIKS_SELECT;
            break;

        case HPK_REMOTEINPUT_EVENT_NUM0:
            dfbKey = DIKI_0;
            dfbSym = DIKS_0;
            break;

        case HPK_REMOTEINPUT_EVENT_NUM1:
            dfbKey = DIKI_1;
            dfbSym = DIKS_1;
            break;

        case HPK_REMOTEINPUT_EVENT_NUM2:
            dfbKey = DIKI_2;
            dfbSym = DIKS_2;
            break;

        case HPK_REMOTEINPUT_EVENT_NUM3:
            dfbKey = DIKI_3;
            dfbSym = DIKS_3;
            break;

        case HPK_REMOTEINPUT_EVENT_NUM4:
            dfbKey = DIKI_4;
            dfbSym = DIKS_4;
            break;

        case HPK_REMOTEINPUT_EVENT_NUM5:
            dfbKey = DIKI_5;
            dfbSym = DIKS_5;
            break;

        case HPK_REMOTEINPUT_EVENT_NUM6:
            dfbKey = DIKI_6;
            dfbSym = DIKS_6;
            break;

        case HPK_REMOTEINPUT_EVENT_NUM7:
            dfbKey = DIKI_7;
            dfbSym = DIKS_7;
            break;

        case HPK_REMOTEINPUT_EVENT_NUM8:
            dfbKey = DIKI_8;
            dfbSym = DIKS_8;
            break;

        case HPK_REMOTEINPUT_EVENT_NUM9:
            dfbKey = DIKI_9;
            dfbSym = DIKS_9;
            break;

        case HPK_REMOTEINPUT_EVENT_VOLUMEUP:
            dfbSym = DIKS_VOLUME_UP;
            break;

        case HPK_REMOTEINPUT_EVENT_VOLUMEDOWN:
            dfbSym = DIKS_VOLUME_DOWN;
            break;

        case HPK_REMOTEINPUT_EVENT_MUTE:
            dfbSym = DIKS_MUTE;
            break;

        case HPK_REMOTEINPUT_EVENT_WINDOW:
            dfbSym = DIKS_ZOOM;
            break;

        case HPK_REMOTEINPUT_EVENT_RETURN:
            dfbSym = DIKS_CUSTOM44;
            break;

        case HPK_REMOTEINPUT_EVENT_ACTION_A:
            dfbSym = (hpkPacket.source == HPK_REMOTEINPUT_SOURCE_RC5_COMPASS) ? DIKS_RED : DIKS_YELLOW;
            break;

        case HPK_REMOTEINPUT_EVENT_ACTION_B:
            dfbSym = (hpkPacket.source == HPK_REMOTEINPUT_SOURCE_RC5_COMPASS) ? DIKS_GREEN : DIKS_BLUE;
            break;

        case HPK_REMOTEINPUT_EVENT_ACTION_C:
            dfbSym = (hpkPacket.source == HPK_REMOTEINPUT_SOURCE_RC5_COMPASS) ? DIKS_YELLOW : DIKS_RED;
            break;

        case HPK_REMOTEINPUT_EVENT_ACTION_D:
            dfbSym = (hpkPacket.source == HPK_REMOTEINPUT_SOURCE_RC5_COMPASS) ? DIKS_BLUE : DIKS_GREEN;
            break;

        case HPK_REMOTEINPUT_EVENT_BACK:
            dfbKey = DIKI_BACKSPACE;
            break;

        case HPK_REMOTEINPUT_EVENT_THUMBSUP:
            dfbSym = DIKS_CUSTOM10;
            break;

        case HPK_REMOTEINPUT_EVENT_THUMBSDOWN:
            dfbSym = DIKS_CUSTOM11;
            break;
            
        case HPK_REMOTEINPUT_EVENT_LIVETV:
            dfbSym = DIKS_CUSTOM1;
            break;

        case HPK_REMOTEINPUT_EVENT_DISPLAY:
            dfbSym = DIKS_CUSTOM2;
            break;

        case HPK_REMOTEINPUT_EVENT_DVDMENU:
            dfbSym = DIKS_CUSTOM18;
            break;

        case HPK_REMOTEINPUT_EVENT_STANDBY:
            dfbSym = DIKS_CUSTOM3;
            break;

        case HPK_REMOTEINPUT_EVENT_DELIMITER:
            dfbSym = DIKS_CUSTOM12;
            break;

        case HPK_REMOTEINPUT_EVENT_DVD:
            dfbSym = DIKS_DVD;
            break;

        case HPK_REMOTEINPUT_EVENT_SKIPREVERSE:
            dfbSym = DIKS_CUSTOM36;
            break;

        case HPK_REMOTEINPUT_EVENT_SKIPFORWARD:
            dfbSym = DIKS_CUSTOM35;
            break;

        case HPK_REMOTEINPUT_EVENT_DVDTOPMENU:
            dfbSym = DIKS_CUSTOM17;
            break;

        case HPK_REMOTEINPUT_EVENT_FPPLAY:
            dfbSym = DIKS_CUSTOM33;
            break;

        case HPK_REMOTEINPUT_EVENT_FPPAUSE:
            dfbSym = DIKS_CUSTOM34;
            break;

        case HPK_REMOTEINPUT_EVENT_FPSKIPFORWARD:
            dfbSym = DIKS_CUSTOM35;
            break;

        case HPK_REMOTEINPUT_EVENT_FPSKIPREVERSE:
            dfbSym = DIKS_CUSTOM36;
            break;

        case HPK_REMOTEINPUT_EVENT_FPSTOP:
            dfbSym = DIKS_CUSTOM37;
            break;

        case HPK_REMOTEINPUT_EVENT_VIDEO_MODE_FIXED_480i:
            dfbSym = DIKS_CUSTOM19;
            break;

        case HPK_REMOTEINPUT_EVENT_VIDEO_MODE_FIXED_480p:
            dfbSym = DIKS_CUSTOM20;
            break;

        case HPK_REMOTEINPUT_EVENT_VIDEO_MODE_FIXED_720p:
            dfbSym = DIKS_CUSTOM21;
            break;

        case HPK_REMOTEINPUT_EVENT_VIDEO_MODE_FIXED_1080i:
            dfbSym = DIKS_CUSTOM22;
            break;

        case HPK_REMOTEINPUT_EVENT_VIDEO_MODE_HYBRID:
            dfbSym = DIKS_CUSTOM23;
            break;

        case HPK_REMOTEINPUT_EVENT_VIDEO_MODE_NATIVE:
            dfbSym = DIKS_CUSTOM24;
            break;

        case HPK_REMOTEINPUT_EVENT_ASPECT_CORRECTION_FULL:
            dfbSym = DIKS_CUSTOM25;
            break;

        case HPK_REMOTEINPUT_EVENT_ASPECT_CORRECTION_PANEL:
            dfbSym = DIKS_CUSTOM26;
            break;

        case HPK_REMOTEINPUT_EVENT_ASPECT_CORRECTION_ZOOM:
            dfbSym = DIKS_CUSTOM27;
            break;

        case HPK_REMOTEINPUT_EVENT_ASPECT_CORRECTION_WIDE_ZOOM:
            dfbSym = DIKS_CUSTOM28;
            break;

        case HPK_REMOTEINPUT_EVENT_CC_ON:
            dfbSym = DIKS_CUSTOM29;
            break;

        case HPK_REMOTEINPUT_EVENT_CC_OFF:
            dfbSym = DIKS_CUSTOM30;
            break;

        case HPK_REMOTEINPUT_EVENT_TUNER_SWITCH:
            dfbSym = DIKS_TUNER;
            break;

        case HPK_REMOTEINPUT_EVENT_VIDEO_MODE_HYBRID_720p:
            dfbSym = DIKS_CUSTOM31;
            break;

        case HPK_REMOTEINPUT_EVENT_VIDEO_MODE_HYBRID_1080i:
            dfbSym = DIKS_CUSTOM32;
            break;

        case HPK_REMOTEINPUT_EVENT_VIDEO_ON_DEMAND:
            dfbSym = DIKS_CUSTOM39;
            break;

        case HPK_REMOTEINPUT_EVENT_LOW_BATTERY:
            dfbSym = DIKS_CUSTOM38;
            break;

        case HPK_REMOTEINPUT_EVENT_TELETEXT:
            dfbSym = DIKS_CUSTOM40;
            break;
            
        case HPK_REMOTEINPUT_EVENT_FIND_REMOTE:
            dfbSym = DIKS_CUSTOM9;
            break;

        case HPK_REMOTEINPUT_EVENT_EXIT:
            dfbSym = DIKS_EXIT;
            break;

        case HPK_REMOTEINPUT_EVENT_TVPOWER:
            dfbSym = DIKS_POWER;
            break;
            
        case HPK_REMOTEINPUT_EVENT_TVINPUT:
            dfbSym = DIKS_CUSTOM5;
            break;

        case HPK_REMOTEINPUT_EVENT_STOP:
            dfbSym = DIKS_STOP;
            break;

        case HPK_REMOTEINPUT_EVENT_ANGLE:
            dfbSym = DIKS_ANGLE;
            break;
            
        case HPK_REMOTEINPUT_EVENT_SUBTITLE:
            dfbSym = DIKS_SUBTITLE;
            break;

        case HPK_REMOTEINPUT_EVENT_AUDIO:
            dfbSym = DIKS_AUDIO;
            break;

        /* The following HPK remote input events are not mapped to any
           DirectFB key.  This is because these events do not have
           corresponding STX remote event types, and only STX remote event
           types have been assigned DirectFB keys and symbols.  If any of
           these are needed, they should be added to STX as well and then a
           common mapping for STX and HPK should be defined and used here as
           well as in tivogfx_input_ipc2.cpp */

//        case HPK_REMOTEINPUT_EVENT_PLUS10:
//        case HPK_REMOTEINPUT_EVENT_DVDSETUP:
//        case HPK_REMOTEINPUT_EVENT_OPENCLOSE:
//        case HPK_REMOTEINPUT_EVENT_PROGRESSIVE:
//        case HPK_REMOTEINPUT_EVENT_SEARCH:
//        case HPK_REMOTEINPUT_EVENT_NAVI:
//        case HPK_REMOTEINPUT_EVENT_VREMOTE:
//        case HPK_REMOTEINPUT_EVENT_FLSELECT:
//        case HPK_REMOTEINPUT_EVENT_FLDIMMER:
//        case HPK_REMOTEINPUT_EVENT_VCRPLUS:
//        case HPK_REMOTEINPUT_EVENT_VIDEO_FORMAT:
//        case HPK_REMOTEINPUT_EVENT_ACTIVE:
//        case HPK_REMOTEINPUT_EVENT_DIAGMENU:
//        case HPK_REMOTEINPUT_EVENT_LAST_CHANNEL:
//        case HPK_REMOTEINPUT_EVENT_MENU:
//        case HPK_REMOTEINPUT_EVENT_LIST:
//        case HPK_REMOTEINPUT_EVENT_TV:

        default:
            return false;
    }

    return true;
}

/**
 * TvGfxPrv_HpkInput_GetEvent
 */
static bool TvGfxPrv_HpkInput_GetEvent( DFBInputEvent* pEvent )
{
    HPK_Result result;
    HPK_RemoteInputPacket packet;
    HPK_Size size;

// too verbose - this is in a poll loop
//    D_DEBUG_AT( tivogfx_input_hpk, "%s()\n", __FUNCTION__ );

    memset( &packet, 0, sizeof(packet) );
    size = 1;
    result = HPK_RemoteInput_Read( &hpkRemoteInputG,
        HPK_TIME_SECOND / 10, 1, &packet, &size );

    if (result == HPK_SUCCESS)
    {
        DFBInputDeviceKeyIdentifier dfbKey;
        DFBInputDeviceKeySymbol     dfbSym;

        if (!TvGfxPrv_HpkInput_TranslateRemoteInputEvent(
                packet,
                /*returns*/ dfbKey,
                /*returns*/ dfbSym))
        {
            D_DEBUG_AT( tivogfx_input_hpk, "%s: HPK input key not mapped\n",
                        __FUNCTION__ );
            return false;
        }

        // from Tv*App's SendKey()...
        TmkMonotonicTime t;
        t.LoadCurrent();
        pEvent->timestamp.tv_sec = (long int) (t.ToNative() >> 32);
        pEvent->timestamp.tv_usec = (long int) (t.ToNative() & 0xFFFFFFFF);

        pEvent->flags = DIEF_TIMESTAMP;

        if (dfbKey != DIKI_UNKNOWN)
        {
            pEvent->flags = ( DFBInputEventFlags )
                ( pEvent->flags | DIEF_KEYID );
            pEvent->key_id = dfbKey;

            pEvent->flags = ( DFBInputEventFlags )
                ( pEvent->flags | DIEF_KEYCODE );
            pEvent->key_code = packet.event;
        }

        if (dfbSym != DIKS_NULL)
        {
            pEvent->flags = ( DFBInputEventFlags )
                ( pEvent->flags | DIEF_KEYSYMBOL );
            pEvent->key_symbol = dfbSym;
        }

//        if ( key == TV_REMOTE_KEY_LSHIFT ||
//             key == TV_REMOTE_KEY_RSHIFT )
//        {
//            evt.dfbModifiers = ( DFBInputDeviceModifierMask )
//                ( evt.dfbModifiers | DIMM_SHIFT );
//            evt.dfbEventFlags = ( DFBInputEventFlags )
//                ( evt.dfbEventFlags | DIEF_MODIFIERS );
//        }
//        else if ( key == TV_REMOTE_KEY_LCONTROL ||
//                  key == TV_REMOTE_KEY_RCONTROL)
//        {
//            evt.dfbModifiers = ( DFBInputDeviceModifierMask )
//                ( evt.dfbModifiers | DIMM_CONTROL );
//            evt.dfbEventFlags = ( DFBInputEventFlags )
//                ( evt.dfbEventFlags | DIEF_MODIFIERS );
//        }
//        else if ( key == TV_REMOTE_KEY_LMETA ||
//                  key == TV_REMOTE_KEY_RMETA )
//        {
//            evt.dfbModifiers = ( DFBInputDeviceModifierMask )
//                ( evt.dfbModifiers | DIMM_META );
//            evt.dfbEventFlags = ( DFBInputEventFlags )
//                ( evt.dfbEventFlags | DIEF_MODIFIERS );
//        }

        if (packet.type == HPK_REMOTEINPUT_TYPE_UP)
        {
            pEvent->type = DIET_KEYRELEASE;
        }
        else
        {
            pEvent->type = DIET_KEYPRESS;
        }

        D_DEBUG_AT( tivogfx_input_hpk, "%s: dispatching HPK input key\n",
            __FUNCTION__ );

#ifdef DEBUG
        DirectFBKeyIdentifierNames(keyNames);
        DirectFBKeySymbolNames(symNames);

        const char* dfbKeyName = NULL;
        const char* dfbSymName = NULL;

        int i = 0;
        while ( keyNames[ i ].identifier != DIKI_UNKNOWN )
        {
            if ( keyNames[ i ].identifier == dfbKey )
            {
                dfbKeyName = keyNames[ i ].name;
                break;
            }
            i++;
        }

        i = 0;
        while ( symNames[ i ].symbol != DIKS_NULL )
        {
            if ( symNames[ i ].symbol == dfbSym )
            {
                dfbSymName = symNames[ i ].name;
                break;
            }
            i++;
        }

        D_DEBUG_AT(
            tivogfx_input_hpk,
            "%s: HPK remote input %d -> dfb key=%s sym=%s\n",
            __FUNCTION__,
            packet.event,
            dfbKeyName,
            dfbSymName );
#endif

        return true;
    }

    if (result != HPK_ERROR_TIMEOUT)
    {
        D_DEBUG_AT( tivogfx_input_hpk, "%s: HPK_RemoteInput_Read error 0x%08X\n",
            __FUNCTION__, result );

        quitG = true;
    }

    return false;
}

/**
 * TvGfxPrv_HpkInput_Acquire
 */
static bool TvGfxPrv_HpkInput_Acquire( void )
{
    HPK_Platform platform;
    HPK_Reservable reserv;
    const char* s;

    D_DEBUG_AT( tivogfx_input_hpk, "%s()\n", __FUNCTION__ );

    s = getenv( "DFB_DISABLE_HPK_INPUT" );

    if ((s != NULL) && (strcmp( s, "1" ) == 0))
    {
        D_DEBUG_AT( tivogfx_input_hpk, "%s: DFB_DISABLE_HPK_INPUT set\n",
            __FUNCTION__ );
        return false;
    }

    if (!TvGfxUtil_InHandcraftMode())
    {
        D_DEBUG_AT( tivogfx_input_hpk, "%s: handcraft is not set\n",
            __FUNCTION__ );
        return false;
    }

    HPK_Result result = HPK_GetPlatform( &platform );

    if (result != HPK_SUCCESS)
    {
        D_DEBUG_AT( tivogfx_input_hpk, "%s: HPK_GetPlatform error\n",
            __FUNCTION__ );
        return false;
    }

    result = HPK_PlatformGetRemoteInput( &platform,
        &hpkRemoteInputG );

    if (result != HPK_SUCCESS)
    {
        D_DEBUG_AT( tivogfx_input_hpk, "%s: HPK_PlatformGetRemoteInput error\n",
            __FUNCTION__ );
        return false;
    }

    result = HPK_CAST_OBJECT( &hpkRemoteInputG, &reserv,
        HPK_INTERFACE_ID( HPK_Reservable ) );

    if (result != HPK_SUCCESS)
    {
        D_DEBUG_AT( tivogfx_input_hpk, "%s: HPK_Reservable cast error\n",
            __FUNCTION__ );
        return false;
    }

    result = HPK_Reservable_Reserve( &reserv, HPK_TIME_INSTANT );

    if (result != HPK_SUCCESS)
    {
        D_DEBUG_AT( tivogfx_input_hpk, "%s: Remote input already reserved\n",
            __FUNCTION__ );
        return false;
    }

    D_DEBUG_AT( tivogfx_input_hpk, "%s() success\n", __FUNCTION__ );
    return true;
}

/**
 * TvGfxPrv_HpkInput_Release
 */
static void TvGfxPrv_HpkInput_Release( void )
{
    D_DEBUG_AT( tivogfx_input_hpk, "%s()\n", __FUNCTION__ );

    HPK_RELEASE_OBJECT( &hpkRemoteInputG );
}

/**
 * TvGfxPrv_HpkInputThread
 */
static int TvGfxPrv_HpkInputThread( long arg )
{
    direct_thread_set_name( "TiVo Gfx HPK input" );

    D_DEBUG_AT( tivogfx_input_hpk, "%s() entered\n", __FUNCTION__ );

    // just use the global for now
    (void) arg;

    if (!TvGfxPrv_HpkInput_Acquire())
    {
        // diagnostic already printed
    }
    else
    {
        D_DEBUG_AT( tivogfx_input_hpk, "%s: running\n", __FUNCTION__ );

        while (!quitG)
        {
            DFBInputEvent evt;

            memset( &evt, 0, sizeof( evt ) );

            if (TvGfxPrv_HpkInput_GetEvent( &evt ))
            {
                D_DEBUG_AT( tivogfx_input_hpk,
                        "%s %d key_id %d key_symbol %d key_code %d \n",
                        __FUNCTION__, __LINE__,
                        evt.key_id, evt.key_symbol, evt.key_code);
                (void) TvGfxProxy_PutInputEvent( hGfxProxyG, &evt );
            }

            usleep( 10 * 1000 );
        }

        TvGfxPrv_HpkInput_Release();
    }

    D_DEBUG_AT( tivogfx_input_hpk, "%s() exiting\n", __FUNCTION__ );

    return 0;
}

/**
 * TvGfx_HpkInputOpen
 */
void TvGfx_HpkInputOpen( TvGfxProxy hGfxProxy )
{
    D_DEBUG_AT( tivogfx_input_hpk, "%s() enter\n", __FUNCTION__ );

    hGfxProxyG = hGfxProxy;

    D_DEBUG_AT( tivogfx_input_hpk, "%s: creating thread\n", __FUNCTION__ );

    TmkMempool* mempool = TmkInit::GetMainMempool();

    pInputThreadG =
        new (mempool) TmkThread( TvGfxPrv_HpkInputThread, "TiVoGfx_HpkInput" );

    pInputThreadG->SetRTPriority( TmkThread::RT_VIEWER );

    D_DEBUG_AT( tivogfx_input_hpk, "%s: starting thread\n", __FUNCTION__ );

    TvStatus status = pInputThreadG->Start( NULL );
    TvDebugAssert( status == TV_OK );

    D_DEBUG_AT( tivogfx_input_hpk, "%s() exit\n", __FUNCTION__ );
}

/**
 * TvGfx_HpkInputClose
 */
void TvGfx_HpkInputClose( void )
{
    D_DEBUG_AT( tivogfx_input_hpk, "%s() enter\n", __FUNCTION__ );

    quitG = true;

    D_DEBUG_AT( tivogfx_input_hpk, "%s: waiting for thread\n", __FUNCTION__ );

    pInputThreadG->Join();

    D_DEBUG_AT( tivogfx_input_hpk, "%s: destroying thread\n", __FUNCTION__ );

    pInputThreadG = NULL;

    D_DEBUG_AT( tivogfx_input_hpk, "%s() exit\n", __FUNCTION__ );
}

//###########################################################################
#else // TIVOGFX_ENABLE_HPK_REMOTE
//###########################################################################

/**
 * TvGfx_HpkInputOpen
 */
void TvGfx_HpkInputOpen( TvGfxProxy hGfxProxy )
{
    TIVOGFX_UNUSED_PARAMETER( hGfxProxy );

    // nothing to do
}

/**
 * TvGfx_HpkInputClose
 */
void TvGfx_HpkInputClose( void )
{
    // nothing to do
}

//###########################################################################
#endif // TIVOGFX_ENABLE_HPK_REMOTE
//###########################################################################


