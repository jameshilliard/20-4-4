////////////////////////////////////////////////////////////////////////////////
//
// File: tivo_directfb_keymap.h
//
// Copyright 2013 TiVo Inc.  All Rights Reserved.
//
////////////////////////////////////////////////////////////////////////////////

#ifndef TIVO_DIRECTFB_KEY_MAP_H
#define TIVO_DIRECTFB_KEY_MAP_H

#include <directfb.h>
#include <util/tvtypes.h>
#include <stx/StxCommandsEvents.h>

/**
 * PopulateDFBInputEvent
 *
 * This method receives StxKeyEvent (TvRemoteKey) events, from host
 * maps to DFBInputEvent, and populates the fields of DFBInputEvent.
 *
 * Returns true on success, false on failure due to the input event not
 * being mappable to a valid DirectFB key event, likely because the input
 * event included a key code and symbol that were not valid according to
 * the STX protocol.
 */
bool PopulateDFBInputEvent(
    ref<StxKeyEvent> pStxKeyEvt,
    DFBInputEvent* pDFBInputEvent);

#endif // TIVO_DIRECTFB_KEY_MAP_H
