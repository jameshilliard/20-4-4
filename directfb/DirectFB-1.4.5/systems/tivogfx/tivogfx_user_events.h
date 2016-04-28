#ifndef TIVOGFX_USER_EVENTS_H
#define TIVOGFX_USER_EVENTS_H

//////////////////////////////////////////////////////////////////////
//
// File: tivogfx_user_events.h
//
// Copyright 2013 TiVo Inc.  All Rights Reserved.
//
//////////////////////////////////////////////////////////////////////


//######################################################################
#ifdef __cplusplus
extern "C" {
#endif
//######################################################################


/**
 * The TiVoGfxIpc system sends up DirectFB Custom User events of type DFEC_USER
 * The encoding of these specific events varies, and here are the known type
 * encodings
 */

/**
 * A "Bag" of key value pairs of strings.
 *
 * Format of buffer is:
 * 4 bytes Big Endian encoded type ID of the actual event.
 * 4 bytes Big Endian encoded length of the rest of the buffer not including
 * these 4 bytes or the type ID, so an empty buffer would read length 0.
 *
 * The remaining fields are optional and will not be included if length is 0.
 *
 * 4 bytes Big Endian encoded count of key/values pairs. This count must be an
 * even number (since any # N pairs = N*2 = 2*N is always even).
 * Then, N pairs of strings (ie., 2*N strings), each string having format:
 * 4 bytes Big Endian encoded length of string, followed by
 * Length bytes of the string, NOT null terminated.
 */
#define TIVO_DFB_USER_TYPE_BAG_KEY_VALUES 0x53545831 // hex of STX1

/**
 * Data is a pointer to arbitrary data.
 *
 * Format of buffer is:
 *
 * 4 bytes Big Endian encoded type ID of the actual event.
 * 4 bytes Big Endian encoded length of the rest of the buffer not including
 * these 4 bytes or the type ID, so an empty buffer would read length 0.
 *
 * If length is non-zero there will be a buffer that will need to be interpreted
 * by the app.
 */
#define TIVO_DFB_USER_TYPE_VOID_STAR 0x53545832


//######################################################################
#ifdef __cplusplus
}
#endif
//######################################################################


#endif // TIVOGFX_USER_EVENTS_H
