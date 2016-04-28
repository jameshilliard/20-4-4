/*
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Modified by Mark Frederiksen 9/22/2010
 *    - Added DOxygen compatible comments to help with towards the understanding of
 *          the performance counters and the control registers.
 *    - Enabled the use of the timer interrupt enable (overflow interrupt)
 * Copyright (C) 2005 Broadcom Corp by Troy Trammel
 * Copyright (C) 2004 by Ralf Baechle
 */
 
/******************************************************************************
 *	@file op_model_brcm4380.c
 */

/* Define the peformance counter specific version */
#define BMIPS4380		4380
//#define OP_DEBUG
#include "op_model_brcm.h"

/*!	@note
 *	bcm4380 has some odd fields in the control registers that must be set in 
 *	conjunction with certain events.  This table keeps track of them.
 *	This table has been updated to include the global eventd and corrected
 *	the bus interface unit error for the SetID.
 */
BEGIN_MOD_SET_TABLE(4380)
	/*   Each entry contains event ID, module ID, and set ID. */
	MOD_SET_ENTRY( 0x11,	mod_Global,				0 ) /* Global Event: Instructions executed  */
	MOD_SET_ENTRY( 0x12,	mod_Global,				0 ) /* Global Event: Cycles elapsed */
	MOD_SET_ENTRY( 0x05,	mod_Instruction_Cache ,	0 ) /* Instruction Cache Event: misses */
	MOD_SET_ENTRY( 0x06,	mod_Instruction_Cache ,	0 ) /* Instruction Cache Event: hits */
	MOD_SET_ENTRY( 0x09,	mod_Data_Cache,			1 ) /* Data Cache Event: miss */
	MOD_SET_ENTRY( 0x0a,	mod_Data_Cache,			1 ) /* Data Cache Event: hit */
	MOD_SET_ENTRY( 0x42,	mod_Readahead_Cache,	0 ) /* Readahead Cache Event: lookup miss */
	MOD_SET_ENTRY( 0x4b,	mod_Readahead_Cache,	1 ) /* Readahead Cache Event: blocked requests due to prefetch */
/*	MOD_SET_ENTRY( 0x45,	mod_BIU,				1 )  Bus Interface Unit Event: Readahead cache hits */
	MOD_SET_ENTRY( 0x45,	mod_BIU,				2 ) /* Bus Interface Unit Event: Readahead cache hits */
	/*!
	 * @note All bits greater than bit 7 of event IDs are used to make the
	 * event IDs unique
	 */
	MOD_SET_ENTRY(  0x101,	mod_Core,				0 ) /* Core Unit Event: Stall cycles */
	MOD_SET_ENTRY(  0x103,	mod_Core,				2 )	/* Core Unit Event: Kernel cycles */
	MOD_SET_ENTRY(  0x201,	mod_Core,				2 )	/* Core Unit Event: User cycles */
	MOD_SET_ENTRY(  0x10a,	mod_Core,				1 )	/* Core Unit Event: Exceptions */
	MOD_SET_ENTRY(  0x301,	mod_L2_Cache,			0 )	/* L2 Cache Event: Hits */
	MOD_SET_ENTRY(  0x300,	mod_L2_Cache,			0 )	/* L2 Cache Event: Misses */
END_MOD_SET_TABLE()

DECLARE_OP_BMIPS_MODEL(4380);
