/*******************************************************************************
*
* Common/Inc/debug.h
*
* Description: debug macros and such
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

#ifndef __debug_h__
#define __debug_h__



#ifdef IOCTL_DEBUG
static void _ioctl_dbg(dk_context_t *dkcp, char *str)
{
    HostOS_PrintLog(L_ERR, "%s: %s failed!\n", dkcp->name, str);
}
#else
//static inline void _ioctl_dbg(char *name, char *str) { }
#define _ioctl_dbg(NAME,STR)   do { } while (0)
#endif


#endif // __debug_h__
