 /********************************************************************
 *********************************************************************
 *
 *  File        :  $Source: /home/sources/cvsroot/Thirdparty/siliconvendor/broadcom/nexus/kernel/stblinux-3.3/include/linux/pace_serial_tt.h,v $
 *
 *  Description :  Implementation of a write-only tty device.
 *                 Allows RS232 output to occur even when the
 *                 PACE_RS232_SILENT kernel config is in place.
 *                 Requires the version of brcmserial.c which
 *                 supports the PACE_RS232_SILENT kernel config
 *                 option. This device is primarily intended to allow
 *                 testtask to continue to operate when the serial
 *                 output has been completely silenced (hence the "_tt"
 *                 suffix).
 *
 *  Author      :  Steve Turner.
 *
 *  Copyright   :  Pace Micro Technology 2007 (c)
 *
 *                 The copyright in this material is owned by
 *                 Pace Micro Technology PLC ("Pace"). This
 *                 material is regarded as a highly confidential
 *                 trade secret of Pace. It may not be reproduced,
 *                 used, sold or in any other way exploited or
 *                 transferred to any third party without the prior
 *                 written permission of Pace.
 *
 *********************************************************************
 ********************************************************************/

#ifndef _PACE_SERIAL_TT_H_
#define _PACE_SERIAL_TT_H_

/**
* Driver version string.
*/
#define S_TT_DRV_VERSION   "$Revision: 1.1.26.1 $"

/**
* Driver device name.
*/
#define  S_TT_DEV_NAME     "tty_tt"

/**
* Magic number for ioctl commands ('t' for tty!).
*/
#define  S_TT_DEV_TYPE     't'

/**
* ioctl commands:
*   S_TT_DEV_SERIAL_CTRL = Turn serial on (1) or off (0)
*/
#define  S_TT_DEV_SERIAL_CTRL _IOWR(S_TT_DEV_TYPE, 1, int)

#endif /* #ifndef _PACE_SERIAL_TT_H_ */
