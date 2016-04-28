/*******************************************************************************
*
* Common/Inc/Clnk_ctl_dvr.h
*
* Description: c.LINK control interface layer
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

#ifndef __CLNK_CTL_DVR_H__
#define __CLNK_CTL_DVR_H__

int clnk_ctl_drv(void *vdkcp, int cmd, struct clnk_io *io);
int Clnk_ETH_Control_drv(void *vdccp, 
                         int option, 
                         SYS_UINTPTR reg, 
                         SYS_UINTPTR val,  // io block pointer or register value
                         SYS_UINTPTR length);
void clnk_ctl_postprocess(void *vcp, IfrDataStruct *kifr, struct clnk_io *kio);

#endif /* __CLNK_CTL_DVR_H__ */

