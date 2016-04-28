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

/* Do Not Edit! Contents produced by script.  Wed Jul 30 19:54:39 PDT 2008  */


/*** public prototypes from Src/ClnkBus_iface.c ***/

/*** public prototypes from Src/Clnk_ctl_candd.c ***/

/*** public prototypes from Src/mdio.c ***/
int clnk_write( void *vctx, SYS_UINT32 addr, SYS_UINT32 data);
int clnk_read( void *vctx, SYS_UINT32 addr, SYS_UINT32 *data);
int clnk_write_burst( void *vctx, SYS_UINT32 addr, SYS_UINT32 *data, unsigned int size, int inc);
int clnk_read_burst( void *vctx, SYS_UINT32 addr, SYS_UINT32 *data, unsigned int size, int inc);
