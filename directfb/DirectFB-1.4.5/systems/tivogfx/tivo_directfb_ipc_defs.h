#ifndef TIVO_DIRECTFB_IPC_DEFS_H
#define TIVO_DIRECTFB_IPC_DEFS_H

//////////////////////////////////////////////////////////////////////
//
// File: tivo_directfb_ipc_defs.h
//
// Copyright 2013 TiVo Inc.  All Rights Reserved.
//
//////////////////////////////////////////////////////////////////////

//######################################################################

// always include directfb first
#include <directfb.h>

#include <stx/StxIpcProtocol.h>

//######################################################################

/**
 * TIVO_DIRECTFB_IPC_SOCKET_ENV
 *
 * The name of the environment variable by which the host passes name of
 * the local domain server socket.
 *
 * If defined, this is the socket to which the TiVoGfx IPC Input Driver
 * should connect and read events.
 *
 * Each socket packet starts with a u32 (4 byte) size of the following
 * packet event data, followed by the data.
 *
 * The event data size and event data are sent in platform native endianness
 * and alignment.
 */
#define TIVO_DIRECTFB_IPC_SOCKET_ENV        "TIVO_DIRECTFB_IPC_SOCKET"

/**
 * TiVoDirectFbIpcEventHeader
 *
 * Each event starts with a u32 (4 byte) event type, followed by the
 * data (if any) specific to the event type.
 */
typedef struct
{
    u32 ipcEventType;       // TiVoDirectFbIpcEventType, but as u32
}
TiVoDirectFbIpcEventHeader;

/**
 * TIVO_DIRECTFB_IPC_EVENT_MIN_SIZE
 *
 * Each event starts with a u32 (4 byte) event type, followed by the
 * data (if any) specific to the event type.
 */
#define TIVO_DIRECTFB_IPC_EVENT_MIN_SIZE    sizeof( TiVoDirectFbIpcEventHeader )

/**
 * TIVO_DIRECTFB_IPC_EVENT_MAX_SIZE
 *
 * Arbitrary maximum event size for sanity checking
 */
#define TIVO_DIRECTFB_IPC_EVENT_MAX_SIZE    (4 + 256)

/**
 * TIVO_DIRECTFB_IPC_EVENT_*
 *
 * Enumeration for TIVO_DIRECTFB_IPC event types.
 */
typedef enum
{
    TIVO_DIRECTFB_IPC_EVENT_KEY         = TV_STX_EVENT_KEY,
    TIVO_DIRECTFB_IPC_EVENT_QUIT        = TV_STX_EVENT_QUIT
}
TiVoDirectFbIpcEventType;

/**
 * TiVoDirectFbIpcEventKey
 *
 * The payload for TIVO_DIRECTFB_IPC_EVENT_KEY events.
 */
typedef struct
{
    TiVoDirectFbIpcEventHeader      ipcEventHeader;
    //----------------------------------------
    DFBInputEventType               dfbEventType;   // type of event: DIET_KEYPRESS or DIET_KEYRELEASE
    DFBInputEventFlags              dfbEventFlags;  // which optional fields are valid?
    struct timeval                  dfbTimestamp;   // time of event creation
    int                             dfbKeyCode;     // hardware keycode, no mapping, -1 if device doesn't differentiate between several keys 
    DFBInputDeviceKeyIdentifier     dfbKeyId;       // basic mapping, modifier independent
    DFBInputDeviceKeySymbol         dfbKeySymbol;   // advanced mapping, unicode compatible, modifier dependent
    DFBInputDeviceModifierMask      dfbModifiers;   // pressed modifiers
}
TiVoDirectFbIpcEventKey;

/**
 * TiVoDirectFbIpcEventQuit
 *
 * The payload for TIVO_DIRECTFB_IPC_EVENT_QUIT events.
 */
typedef struct
{
    TiVoDirectFbIpcEventHeader      ipcEventHeader;
    //----------------------------------------
    // no parameters at this time.
}
TiVoDirectFbIpcEventQuit;

/**
 * TIVO_DIRECTFB_IPC_COMMAND_*
 *
 * Enumeration for TIVO_DIRECTFB_IPC command types.
 */
typedef enum
{
    TIVO_DIRECTFB_IPC_COMMAND_ACK_START = TV_STX_COMMAND_ACK_START,
    TIVO_DIRECTFB_IPC_COMMAND_ACK_RESUME = TV_STX_COMMAND_ACK_RESUME,
    TIVO_DIRECTFB_IPC_COMMAND_ACK_SUSPEND = TV_STX_COMMAND_ACK_SUSPEND
}
TiVoDirectFbIpcCommandType;

//######################################################################

#endif // TIVO_DIRECTFB_IPC_DEFS_H
