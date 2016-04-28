/*******************************************************************************
*
* Common/Inc/drv_hdr.h
*
* Description: Generic driver compilation control
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

/*******************************************************************************
*
* This file simply includes the driver's main compilation control file.
* That file will be named after the driver in some way.
*
* The reason for this is to allow both local and common .c and .h files 
* to include a file specific to a single driver thus customizing the
* compilation for the selected driver.
*
*******************************************************************************/

#ifndef __drv_hdr_h__
#define __drv_hdr_h__


#define SONICS_ADDRESS_OF_sniff_buffer_pa 0



#if defined(PCI_DRVR_SUPPORT)
#include "pci_hdr.h"
#endif
#if defined(E1000_DRVR_SUPPORT) || defined(CONFIG_TIVO)
#include "mii_hdr.h"
#endif
#if defined(CANDD_DRVR_SUPPORT) 
#include "candd_hdr.h"
#endif

#endif // __drv_hdr_h__
