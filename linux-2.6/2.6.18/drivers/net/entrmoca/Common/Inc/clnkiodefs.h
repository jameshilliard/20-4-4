/*******************************************************************************
*
* Common/Inc/clnkiodefs.h
*
* Description: Driver IO definitions
*
*******************************************************************************/

/*******************************************************************************
*                        Entropic Communications, Inc.
*                         Copyright (c) 2001-2008
*                          All rights reserved.
*******************************************************************************/

/*******************************************************************************
* This file is licensed under GNU General Public license.                      *
*                                                                              *
* This file is free software: you can redistribute and/or modify it under the  *
* terms of the GNU General Public License, Version 2, as published by the Free *
* Software Foundation.                                                         *
*                                                                              *
* This program is distributed in the hope that it will be useful, but AS-IS and*
* WITHOUT ANY WARRANTY; without even the implied warranties of MERCHANTABILITY,*
* FITNESS FOR A PARTICULAR PURPOSE, TITLE, or NONINFRINGEMENT. Redistribution, *
* except as permitted by the GNU General Public License is prohibited.         *
*                                                                              * 
* You should have received a copy of the GNU General Public License, Version 2 *
* along with this file; if not, see <http://www.gnu.org/licenses/>.            *
********************************************************************************/

#ifndef __clnkiodefs_h__
#define __clnkiodefs_h__



// from sockios.h : #define SIOCDEVPRIVATE  0x89F0  /* to 89FF */

#define SIOCCLINKDRV	(SIOCDEVPRIVATE+1)  // Control plane commands for the driver
#define SIOCGCLINKMEM	(SIOCDEVPRIVATE+2)  // Reads registers/memory in c.LINK address space
#define SIOCSCLINKMEM	(SIOCDEVPRIVATE+3)  // Sets registers/memory in c.LINK address space
#define SIOCGCLNKCMD    (SIOCDEVPRIVATE+10) // pass thru c.LINK command that expects a response
#define SIOCSCLNKCMD    (SIOCDEVPRIVATE+11) // pass thru c.LINK command that expects no response
#define SIOCLNKDRV      (SIOCDEVPRIVATE+12) // a. Initialize Mailbox Queue Handler b. Get Unsolicited Message 
#define SIOCHDRCMD	    (SIOCDEVPRIVATE+13)  // Resets the SoC , Control the diplexer switch etc.

#ifndef SONICS_ADDRESS_OF_sniff_buffer_pa
#error "Sniff buffer is not defined"
#else
#if SONICS_ADDRESS_OF_sniff_buffer_pa
#define SIOCGSNIFFMEM	(SIOCDEVPRIVATE+7)  // Reads sniff buffer in driver
#endif
#endif

#ifdef CONFIG_TIVO
/* Used by the daemon to signal to the driver that the MoCA link status has
 * changed (i.e. EN2510 has joined/left a MoCA network).
 */
#define SIOCLINKCHANGE  (SIOCDEVPRIVATE+14)
#endif


#endif /* __clnkiodefs_h__ */
