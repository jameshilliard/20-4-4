
/*!
******************************************************************************
**
** \file           bcm_msclock.h
**
** \par            Interface for a millisecond clock kernel driver for
**                 Broadcom MIPS.
**
** \author         Melanie Rhianna Lewis (Melanie.Lewis\@pace.co.uk)
**
** \date           22 August 2005
**
** \note
**
** \code
** Copyright   :   PACE Microtechnology 2005 (c)
**
**                 The copyright of this material is owned by Pace
**                 Microtechnology PLC ("PACE").  This material is
**                 regarded as a highly confidential trade secret of
**                 Pace.  It may not be reproduced, used, sold or in any
**                 other way exploited or transferred to any other
**                 third party without the prior consent of Pace.
** \endcode
**
******************************************************************************
*/

#ifndef _MSHWCLOCK_H
#define _MSHWCLOCK_H

#include <linux/ioctl.h>

/* The magic number. */
#define	MSHWCLOCK_IOCTL_BASE	'M'

/* Get the current millisecond time. */
#define MSHWCLOCK_IOC_GETTIME              _IOR(MSHWCLOCK_IOCTL_BASE, 0, u_int64_t)
/* set clock resolution (interval between interrupts */
#define MSHWCLOCK_IOC_SET_RESOLUTION       _IOW(MSHWCLOCK_IOCTL_BASE, 1, u_int32_t)
/* get current clock resolution */
#define MSHWCLOCK_IOC_GET_RESOLUTION       _IOR(MSHWCLOCK_IOCTL_BASE, 2, u_int32_t)
#define MSHWCLOCK_IOC_DEBUG_ENABLE         _IOW(MSHWCLOCK_IOCTL_BASE, 6, u_int32_t)

#define HWMSCLOCK_RESOLUTION_MIN 500

#define BCM_MSCLOCK_TIME_MSECS_SINCE(TIME)       ((TIME) - hwmsclock_value_msecs())
#define BCM_MSCLOCK_TIME_MSECS_DIFFERENCE(T1,T2) ((T1) - (T2))

#define MSHWCLOCK_DEVICE       "/dev/msclock"  
#define BCM_MSCLOCK_PROCDEVICE_INFO_NAME    "hwmsclock_info"
#define BCM_MSCLOCK_PROCDEVICE_MSECS_NAME   "hwmsclock_msecs"
#define BCM_MSCLOCK_PROCDEVICE_WDCR_NAME    "hwmsclock_wdcr"


/* types for all the types in the interface */

typedef u_int64_t hwmsclock_clock64_t       ;

typedef u_int32_t hwmsclock_time_usecs       ;
typedef u_int32_t hwmsclock_time_msecs       ;
typedef u_int32_t hwmsclock_resolution       ;




/* kernel mode callable methods ( ONLY USABLE FROM USER CONTEXT ) 

*/


extern hwmsclock_clock64_t hwmsclock_value_msecs( void );


#ifdef CONFIG_WATCHDOG_BCM

/* include watchdog support. This has to be in this driver as the 
   watchdogs are implemented using the same ISR + hardware block
   as the fast timers.  
*/
typedef int (*bcm_msclock_hw_watchdog_cb)( void * private_data );

/* hardware watchdog control functions provided by this driver 
   this driver owns the timer ISR + the bcm7111 timer and watchdog 
   hardware block, so its the only one that really knows 
   whats going on
*/

extern void bcm_msclock_hw_watchdog_init( void );

extern void bcm_msclock_hw_watchdog_arm(u32 msecs,
                                        bcm_msclock_hw_watchdog_cb isr_handler);

extern void bcm_msclock_hw_watchdog_keepalive( void );

extern void bcm_msclock_hw_watchdog_disarm( void );

extern void bcm_msclock_hw_watchdog_shutdown( void );

 
#endif

#endif /* _MSHWCLOCK_H */
