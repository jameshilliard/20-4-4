/*
 * tivoRefCount.h
 *
 * This file contains declarations for TiVo's extensions
 * to gcc for checking our use of ref counted objects.
 *
 */

#ifndef __TIVO_CHECK_CALL_H__
#define __TIVO_CHECK_CALL_H__

// effects: if tivoRefCounting warnings are enabled,
//          causes warnings for any violations found
//          in this function call
void tivoCheckCall2( tree call );

#endif // __TIVO_CHECK_CALL_H__
