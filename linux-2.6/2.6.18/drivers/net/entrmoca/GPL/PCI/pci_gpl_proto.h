/*******************************************************************************
*
* GPL/PCI/pci_gpl_proto.h
*
* Description: Linux Ethernet Driver types
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
*******************************************************************************/



/* Do Not Edit! The following contents produced by script.  Thu Jan 22 16:32:47 HKT 2009  */


/*** public prototypes from GPL/PCI/eth_dvr.c ***/
SYS_INT32 clnketh_open(struct net_device *dev);
SYS_INT32 clnketh_close(struct net_device *dev);

/*** public prototypes from GPL/PCI/eth_ioctl_dvr.c ***/
SYS_INT32 clnketh_ioctl(struct net_device *dev, 
                        struct ifreq *ifr, 
                        SYS_INT32 cmd);
void clnk_set_ethtool_var(struct net_device *netdev);
                        

