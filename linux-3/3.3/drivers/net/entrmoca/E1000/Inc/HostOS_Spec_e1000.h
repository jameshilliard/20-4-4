/*******************************************************************************
*
* E1000/Inc/HostOS_Spec_e1000.h
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

#ifdef CONFIG_TIVO
#define DRV_NAME    "entrmoca"
#else
#define DRV_NAME    "e1000_z2"
#endif

// HostOS print control -- This is the lowest priority (numerically highest level) that will print
#define HOST_OS_PRINTLOG_THRESHOLD   L_INFO

// Mailbox Queue Constants
#define CLNK_MBX_CMD_QUEUE_SIZE     32  // Must be a multiple of 2
#define CLNK_MBX_SWUNSOL_QUEUE_SIZE 32  // Must be a multiple of 2
#define SW_UNSOL_HW_QUEUE_SIZE      12  // set by HW

#define CLNK_ETH_MRT_TRANSACTION_TIMEOUT    3         // in seconds

#ifdef CONFIG_TIVO

// Using the default TT_TASK_SLEEP value of 15 ms results in the kclinkd
// kernel thread consuming up to 10% of the CPU time on the TCD. The Entropic
// Applications Engineering group suggested that we could safely increase
// the polling interval of the thread to 100 ms.

#define TT_TASK_SLEEP     100   // in milliseconds
#else
#define TT_TASK_SLEEP      15   // in milliseconds
#endif

#define DEBUG_IOCTL_PRIV    0   // define for IOCTL debug prints
#define DEBUG_IOCTL_CMDQ    0   // define for IOCTL debug prints
#define DEBUG_IOCTL_MEM     1   // define for IOCTL debug prints
#define DEBUG_IOCTL_UNSOLQ  0   // define for IOCTL debug prints

#endif /* __HostOS_Spec_h__ */


