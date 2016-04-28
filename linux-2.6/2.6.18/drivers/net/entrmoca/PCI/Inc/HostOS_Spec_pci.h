/*******************************************************************************
*
* PCI/Inc/HostOS_Spec_pci.h
*
* Description: Host OS Specifications
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

#ifndef __HostOS_Spec_h__
#define __HostOS_Spec_h__

/*
 * USER CONFIGURABLES
 */

#define DRV_NAME "clnk_pci"


// HostOS print control -- This is the lowest priority (numerically highest level) that will print
#define HOST_OS_PRINTLOG_THRESHOLD   L_INFO

// Mailbox Queue Constants
#define CLNK_MBX_CMD_QUEUE_SIZE     32  // Must be a multiple of 2
#define CLNK_MBX_SWUNSOL_QUEUE_SIZE 32  // Must be a multiple of 2
#define SW_UNSOL_HW_QUEUE_SIZE      12  // set by HW

#define TT_TASK_SLEEP       15  // in milliseconds, for future using

#define DEBUG_IOCTL_PRIV    0   // define 1 for IOCTL debug prints
#define DEBUG_IOCTL_CMDQ    0   // define 1 for IOCTL debug prints
#define DEBUG_IOCTL_MEM     0   // define 1 for IOCTL debug prints
#define DEBUG_IOCTL_UNSOLQ  0   // define 1 for IOCTL debug prints

#endif /* __HostOS_Spec_h__ */


