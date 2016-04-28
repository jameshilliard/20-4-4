
/*!
******************************************************************************
**
** \file           bcm_msclock.c
**
** \par            Implementation of a millisecond clock + watchdog kernel driver for
**                 Broadcom MIPS 7xxxx series SOC's 
**                 This module provides a monotonic clock at msec resolution , immune from external 
**                 interference, for use by POS_Time_milliseconds() in user space .
**                 note - this driver also provides low level support for configuring and handling 
**                 the Broadcom hardware watchdog interrupt. This shares the same L1 interrupt and 
**                 L2 interrupt status hardware 
**                 as the timers and can only be handled here.  
**                 Note - this module NO LONGER supports kernel mode millisecond delays as these 
**                 are now handled
**                 by the 2.6 kernel 
**                 On 7401 hw the  user should also see the Broadcom timer (TMR) module sources 
**                 in magnum/basemodules/tmr/7401/btmr.c 
**                 and check the platform timer init code 
**                 in BSEAV/api/src/magnum/board/bsettop_bsp_7401.c to ensure the the BTMR_Open excludes 
**                 the use of any timers used by this module . Normally timer 0 is used by the code, 
**                 and Must be masked out in the BTMR_Open, to ensure that it is NOT configured and used by 
**                 the TMR module. 
**                 
**
** \author         Melanie Rhianna Lewis (Melanie.Lewis\@pace.co.uk)
** \author         Dave Morris(dave.morris\@pace.co.uk)
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

 
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/init.h>
#include <linux/bitops.h>
#include <linux/spinlock.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/wait.h>
#include <linux/smp_lock.h>

#include <asm/io.h>
#include <asm/uaccess.h>
#include <asm/brcmstb/brcmstb.h>

/* Copied from bcmtimer.h - this file is not in the 2.6.30 tree */

#define TIMR_ADR_BASE  KSEG1ADDR(BCHP_PHYSICAL_OFFSET+BCHP_TIMER_TIMER_IS)

#define TIMER_TIMER_IS        0x00
#define TIMER_TIMER_IE0       0x04
#define TIMER_TIMER0_CTRL     0x08
#define TIMER_TIMER0_STAT     0x18
#define TIMER_WDTIMEOUT       0x28
#define TIMER_WDCMD           0x2c
#define TIMER_WDCRS           0x34

#define TIMER_STS_COUNTER_MASK      0x3FFFFFFF

#define TIMERENABLE     0x80000000
#define RSTCNTCLR       0x40000000

/* End of copied from bcmtimer.h */


/* This driver responds to the following configs - 
   See drivers/char/watchdog/Kconfig 
and
   drivers/char/watchdog/Makefile 
   for the watchdog configs 

   drivers/char/Kconfig for the 
   drivers/char/Makefile 

   CONFIG_BCM_MSCLOCK_SUPPORTS_PROCDEVICE   support a procdevice 

   CONFIG_HWMSCLOCK_MINOR_DEV               specify which minor dev this driver uses 
   CONFIG_BCM_MSCLOCK_TIMER_NUM             specify which hw timer to use. normally 0 

   CONFIG_WATCHDOG_BCM                      support hw watchdog interrupt handling 
   CONFIG_WATCHDOG_BCM_7038                 support 7038 hw watchdog interrupt 
   CONFIG_WATCHDOG_BCM_7401                 support 7401 hw watchdog interrupt 
   CONFIG_WATCHDOG_BCM_7401                 support 7401 hw watchdog interrupt 
*/

#include <linux/hwmsclock.h>
#include <linux/proc_fs.h>

#ifdef  CONFIG_WATCHDOG_BCM_7401
#define CONFIG_WATCHDOG_BCM_7038
#endif

#ifdef CONFIG_PACE_TRACEBUFFER
/* Use timer to take samples every 10 ms */
#undef  CONFIG_BCM_MSCLOCK_RESOLUTION_IN_USEC
#define CONFIG_BCM_MSCLOCK_RESOLUTION_IN_USEC 10000
#endif

#define  MSCDEBUG_TRACE  4
#define  MSCDEBUG_ISR    2
#define  MSCDEBUG_NORMAL 1
#define  MSCDEBUG_NONE   0


#define MSCLOCK_RAW_WRITE_LONG(VALUE,ADDRESS)   __raw_writel((VALUE),(volatile unsigned long *) (ADDRESS) )
#define MSCLOCK_RAW_READ_LONG(ADDRESS)          __raw_readl((volatile unsigned long *) (ADDRESS) )

/* static int hwmsclock_debug_mode = MSCDEBUG_TRACE|MSCDEBUG_ISR|MSCDEBUG_NORMAL; */
static int hwmsclock_debug_mode = MSCDEBUG_NONE;
/* For debugging */
#if 1
 #define DPRINTK(args...)     if (hwmsclock_debug_mode & MSCDEBUG_NORMAL) {printk(KERN_DEBUG args);}
 #define DPRINTK_ISR(args...) if (hwmsclock_debug_mode & MSCDEBUG_ISR) {printk(KERN_DEBUG args);}
 #define DPRINTK_TRACE(args...) if (hwmsclock_debug_mode & MSCDEBUG_TRACE) {printk(KERN_DEBUG args);}
#else
 #define DPRINTK(args...)       do { } while(0)
 #define DPRINTK_ISR(args...)   do { } while(0)
 #define DPRINTK_TRACE(args...) do { } while(0)
#endif

/* The minor device.  This is currently unused but may clash in 
   later kernels. It can be load time selected to be another
   value using the minor_dev param. */
#ifdef  CONFIG_HWMSCLOCK_MINOR_DEV
 #define BCM_MSCLOCK_MINOR_DEV (CONFIG_HWMSCLOCK_MINOR_DEV)
#else
 #define BCM_MSCLOCK_MINOR_DEV 240
#endif

/* The hardware timer number.  On a Broadcom there are four timers
   which could be used. It can be load time set to be another
   value using the timer_num param. */
#ifdef CONFIG_BCM_MSCLOCK_TIMER_NUM
 #define BCM_MSCLOCK_TIMER_NUM (CONFIG_BCM_MSCLOCK_TIMER_NUM)
#else
 #define BCM_MSCLOCK_TIMER_NUM 0
#endif

/* 
   new config variable for finer control of interrupts down to usec precision
   The number of timer cycles per interrupt in usec .  
   On BCM7038 or 7111 this is
   typically 27 , i.e. 1000 microsecond. at 27 Mhz */

#ifdef CONFIG_BCM_MSCLOCK_TIMER_CYCLES_PER_USEC
 #define BCM_MSCLOCK_TIMER_CYCLES_PER_USEC (CONFIG_BCM_MSCLOCK_TIMER_CYCLES_PER_USEC)
#else
 #define BCM_MSCLOCK_TIMER_CYCLES_PER_USEC  27
#endif

/* set usec resolution of the timer. */
#ifdef CONFIG_BCM_MSCLOCK_RESOLUTION_IN_USEC
 #define BCM_MSCLOCK_RESOLUTION_IN_USEC  CONFIG_BCM_MSCLOCK_RESOLUTION_IN_USEC
#else
 #define BCM_MSCLOCK_RESOLUTION_IN_USEC  1000000 
#endif

 

/* The linux interrupt for the hardware timers.  This may be defined
   externally but don't bet on it! */
#ifndef BCM_LINUX_TIMR_IRQ
#ifdef CONFIG_BCM7425
 #define BCM_LINUX_TIMR_IRQ \
         (32 + BCHP_HIF_CPU_INTR1_INTR_W1_STATUS_UPG_TMR_CPU_INTR_SHIFT + 1)
#else
 #define BCM_LINUX_TIMR_IRQ \
         (BCHP_HIF_CPU_INTR1_INTR_W0_STATUS_UPG_TMR_CPU_INTR_SHIFT + 1)
#endif
#endif

/* Hardware timer tick is 27MHz */
#define USECS_TO_BCMTICKS(USECS) \
        ((USECS) * timer_cycles_per_usec)

/* Hardware timer tick is 27MHz */
#define MSECS_TO_BCMTICKS(MSECS) \
        (((MSECS) * timer_cycles_per_usec) * 1000)

/* One millisecond. */
#define ONE_MSEC \
        (MSECS_TO_BCMTICKS(1))

#ifdef CONFIG_PACE_TRACEBUFFER
#define BCM_MSCLOCK_MSECS_PER_INT 10
#else
#define BCM_MSCLOCK_MSECS_PER_INT 1000
#endif


/* Mask interrupt status bits for timer NUM. */
#define BCM_TIMER_MASK(NUM) \
        (0x01 << (NUM))

/* Mask interrupt enable bits for timer NUM. */
#define BCM_TIMER_INT_EN_MASK(NUM) \
        BCM_TIMER_MASK(NUM)

/* Mask interrupt enable bits. */
#define BCM_TIMER_INT_MASK \
        (BCM_TIMER_INT_EN_MASK(0) | \
         BCM_TIMER_INT_EN_MASK(1) | \
         BCM_TIMER_INT_EN_MASK(2) | \
         BCM_TIMER_INT_EN_MASK(3))

/* Control register address for timer NUM. */
#define BCM_TIMER_CONTROL(NUM) \
        (TIMR_ADR_BASE + TIMER_TIMER0_CTRL + ((NUM) << 2))

/* Status register address for timer NUM. */
#define BCM_TIMER_STATUS(NUM) \
        (TIMR_ADR_BASE + TIMER_TIMER0_STAT + ((NUM) << 2))

#ifdef CONFIG_WATCHDOG_BCM
 
/* Status register address for watchdog */
#ifdef CONFIG_WATCHDOG_BCM_71XX
  #define BCM_TIMER_WATCHDOG_CMD     (TIMR_ADR_BASE + WATCHDOG_CMD)
  #define BCM_TIMER_WATCHDOG_TIMEOUT (TIMR_ADR_BASE + WATCHDOG_TIMEOUT)
#endif
#ifdef CONFIG_WATCHDOG_BCM_7038
  #define BCM_TIMER_WATCHDOG_CMD     (TIMR_ADR_BASE + TIMER_WDCMD)
  #define BCM_TIMER_WATCHDOG_TIMEOUT (TIMR_ADR_BASE + TIMER_WDTIMEOUT)
  #define BCM_TIMER_WATCHDOG_WDCRS   (TIMR_ADR_BASE + TIMER_WDCRS)
#endif

/* there are 4 timers 0..3 in status reg + 1 nmi watchdog at bit 4 */ 
#define BCM_TIMER_WATCHDOG_INT_MASK BCM_TIMER_MASK(4)

/* handler called from isr . installed by the 
   bcm_msclock_hw_watchdog_arm() function
*/
static bcm_msclock_hw_watchdog_cb  bcm_msclock_hw_watchdog_handler = 0;

#ifdef CONFIG_WATCHDOG_BCM_7038
static int bcm7038_wdog_int_enabled ;
#endif

#endif



/* The minor device number. */
static int minor_dev = BCM_MSCLOCK_MINOR_DEV;

/* The timer number. */
static int timer_num = BCM_MSCLOCK_TIMER_NUM;

/* The number of timer cycles per usec.*/
static int timer_cycles_per_usec    = BCM_MSCLOCK_TIMER_CYCLES_PER_USEC;

/* The number of usecs between interrupts .*/
static int timer_resolution_in_usec = BCM_MSCLOCK_RESOLUTION_IN_USEC;


/* current time in msecs, immune from overflow  */
static volatile hwmsclock_clock64_t   inttime_msecs = 0;


/* locking for the enable/disable + isr  */
 
static spinlock_t bcm_msclock_lock = SPIN_LOCK_UNLOCKED; 

#define bcm_msclock_lock(flags)   spin_lock_irqsave(&bcm_msclock_lock,flags)
#define bcm_msclock_unlock(flags) spin_unlock_irqrestore(&bcm_msclock_lock,flags)

#define hwmsclock_currentCount() (MSCLOCK_RAW_READ_LONG(BCM_TIMER_STATUS(timer_num)) & \
		                  TIMER_STS_COUNTER_MASK)
#define hwmsclock_currentDelayCount() (MSCLOCK_RAW_READ_LONG(BCM_TIMER_STATUS(delay_timer_num)) & \
		                  TIMER_STS_COUNTER_MASK)

/* current state - to prevent multiple registrations of the same interrupt */

#define BCM_MSCLOCK_TIMER_ENABLED            1

static int bcm_msclock_timer_state = ~BCM_MSCLOCK_TIMER_ENABLED;

#define BCM_CLOCK64_LOW_WORD(T)   ((unsigned int)((T) & 0xFFFFFFFF))
#define BCM_CLOCK64_HI_WORD(T)    ((unsigned int)(((T)>> 32) & 0xFFFFFFFF))


static void bcm_msclock_clear_clock_interrupt( int this_timer_num );

static void bcm_msclock_enable_clock_interrupt( int this_timer_num );

static void bcm_msclock_disable_clock_interrupt( int this_timer_num );

static void bcm_msclock_setup_timer_delay(int this_timer_num, u32 usecs);

static void bcm_msclock_reset_clock_counter( int this_timer_num );

static void bcm_msclock_write_enable_bit(int this_timer_num);



extern hwmsclock_clock64_t hwmsclock_value_msecs( void )
{
   hwmsclock_clock64_t prev_inttime_msecs;
   hwmsclock_clock64_t mstime;
   
   do 
   {
       prev_inttime_msecs = inttime_msecs;
       mstime = inttime_msecs  + (hwmsclock_currentCount() / ONE_MSEC );    
                                
   } while (inttime_msecs != prev_inttime_msecs);
   
   return mstime;
}



static irqreturn_t bcm_msclock_interrupt(int   irq, 
                                         void *dev_id)
{
   unsigned int status;
   int retval = 0; /* not handled */

   /* If it's us! */
   if (dev_id == bcm_msclock_interrupt)
   {
      unsigned int enable = 0;
      
      /* If it's our timer. */
      status = MSCLOCK_RAW_READ_LONG(TIMR_ADR_BASE + TIMER_TIMER_IS);

#ifdef CONFIG_WATCHDOG_BCM_7038
      if (!bcm7038_wdog_int_enabled)
      {
         status &= ~BCM_TIMER_WATCHDOG_INT_MASK ;
      }
#endif
      
      if (status & BCM_TIMER_MASK(timer_num))
      { 
	 inttime_msecs += BCM_MSCLOCK_MSECS_PER_INT;
	 
	 DPRINTK_ISR("%s:ticktimer %llu\n", __FUNCTION__, inttime_msecs);

          /* Set the status bit to restart the timer and clear
             the interrupt condition. */
         MSCLOCK_RAW_WRITE_LONG(enable | BCM_TIMER_MASK(timer_num), 
	               TIMR_ADR_BASE + TIMER_TIMER_IS);
#ifdef CONFIG_PACE_TRACEBUFFER
         {
            extern void store_trace_entry(void);

            store_trace_entry();
         }
#endif
         retval = IRQ_HANDLED;
      }

#ifdef CONFIG_WATCHDOG_BCM

      else if (status & BCM_TIMER_WATCHDOG_INT_MASK)
      { 	 
         /* Set the status bit to restart the timer and clear
             the interrupt condition. */

#ifdef CONFIG_WATCHDOG_BCM_71XX
         /* 71xx specific sequence to dismiss interrupt . Writing a 1 to the ststus register 
            clears the appropriate interrupt
         */
         MSCLOCK_RAW_WRITE_LONG(BCM_TIMER_WATCHDOG_INT_MASK, 
                                TIMR_ADR_BASE + TIMER_TIMER_IS);
#endif

#ifdef CONFIG_WATCHDOG_BCM_7038
         bcm7038_wdog_int_enabled = 0 ;
#endif

#ifdef CONFIG_WATCHDOG_BCM_7401
         /* dismiss 7401 watchdog timer interrupt .
            docs5381_7401-PR200-RDS-registerspec.pdf, TIMER_TIMER_IS description
            says that we do this by 
            writing the STOP command sequence to WDCMD . 
         */
         MSCLOCK_RAW_WRITE_LONG(((u32)0xee00),BCM_TIMER_WATCHDOG_CMD);
         MSCLOCK_RAW_WRITE_LONG(((u32)0x00ee),BCM_TIMER_WATCHDOG_CMD);
#endif
	 /* call callback registered by bcm_msclock_hw_watchdog_arm */	 
	 if (bcm_msclock_hw_watchdog_handler)
	 {
	    DPRINTK_ISR("%s:watchdog lastchance ISR at t=%llu\n", 
	            __FUNCTION__, inttime_msecs);
            
	    (*bcm_msclock_hw_watchdog_handler)(NULL);
	 }
	 else
	 {
	    printk(KERN_CRIT "%s:WARNING watchdog interrupt at t=%llu was IGNORED\n", 
	            __FUNCTION__, inttime_msecs); 
         }
         retval = IRQ_HANDLED;
      }
#endif

   }
   return retval;
}



/******************************************************/
/* common functions for handling timers               */
/******************************************************/

static void bcm_msclock_clear_clock_interrupt( int this_timer_num )
{
   unsigned int status = 0;
   unsigned int final  = 0;
   
   DPRINTK("bcm_msclock_clear_clock_interrupt(timer=%u) \n",
              this_timer_num);
	      
   status = MSCLOCK_RAW_READ_LONG(TIMR_ADR_BASE + TIMER_TIMER_IS);
   /* if the interrupt was ON , only then 
      turn it off
   */
   if (status & BCM_TIMER_MASK(this_timer_num))
   {
      final = BCM_TIMER_MASK(this_timer_num);
   }
   MSCLOCK_RAW_WRITE_LONG(final, TIMR_ADR_BASE + TIMER_TIMER_IS);
   DPRINTK("bcm_msclock_clear_clock_interrupt(timer=%u) EXIT  IE0=0x%x \n",
              this_timer_num,final);  
}

static void bcm_msclock_reset_clock_counter( int this_timer_num )
{
   unsigned int status = 0;
   unsigned int final  = 0;
   unsigned int cnt    = 0;
   
   
   /* write the TIMRRST bits in the low 4 bits of the
      interrupt control register , so loading the 
      count down  value into the counter 
      Note these are overloaded onto the interrupt bits, so we 
      clear them elsewhere first  
   */
   DPRINTK("bcm_msclock_reset_clock_counter(timer=%u) \n",
              this_timer_num);
	      
   status = MSCLOCK_RAW_READ_LONG(TIMR_ADR_BASE + TIMER_TIMER_IS);
    
   final = BCM_TIMER_MASK(this_timer_num);
   MSCLOCK_RAW_WRITE_LONG(final, TIMR_ADR_BASE + TIMER_TIMER_IS);
   cnt = MSCLOCK_RAW_READ_LONG(BCM_TIMER_STATUS(this_timer_num)) & TIMER_STS_COUNTER_MASK;
   DPRINTK("bcm_msclock_reset_clock_counter(timer=%u) EXIT IE0 IS NOW 0x%x, count 0x%x \n",
              this_timer_num,final,
	      BCM_CLOCK64_LOW_WORD(cnt));
}
 
static void bcm_msclock_enable_clock_interrupt( int this_timer_num )
{
   unsigned int enable = 0;
   DPRINTK("bcm_msclock_enable_clock_interrupt(timer=%u) \n",
              this_timer_num);
 
   /* Enable the interrupt. */
   enable = MSCLOCK_RAW_READ_LONG(TIMR_ADR_BASE + TIMER_TIMER_IE0);
   /* ensure all zeros for unused bits as per manual */
   enable &= BCM_TIMER_INT_MASK;
   enable |= BCM_TIMER_INT_EN_MASK(this_timer_num);
#ifdef CONFIG_WATCHDOG_BCM_7038
   if (bcm7038_wdog_int_enabled)
   {
      enable |= BCM_TIMER_WATCHDOG_INT_MASK;
   }
#endif
   MSCLOCK_RAW_WRITE_LONG(enable, TIMR_ADR_BASE + TIMER_TIMER_IE0);
   
   DPRINTK("bcm_msclock_enable_clock_interrupt(timer=%u) EXIT IE0 IS NOW 0x%x \n",
              this_timer_num,enable);
  
}

static void bcm_msclock_disable_clock_interrupt( int this_timer_num )
{
   unsigned int enable = 0;
 
   DPRINTK("bcm_msclock_disable_clock_interrupt(timer=%u) \n",
              this_timer_num);
  /* Disable the interrupt. */
   enable  = MSCLOCK_RAW_READ_LONG(TIMR_ADR_BASE + TIMER_TIMER_IE0);
   /* ensure that we only affect the actual bits, not unused ones */
   enable &= BCM_TIMER_INT_MASK;
   enable &= ~BCM_TIMER_INT_EN_MASK(this_timer_num);
#ifdef CONFIG_WATCHDOG_BCM_7038
   if (bcm7038_wdog_int_enabled)
   {
      enable |= BCM_TIMER_WATCHDOG_INT_MASK;
   }
#endif
   MSCLOCK_RAW_WRITE_LONG(enable, TIMR_ADR_BASE + TIMER_TIMER_IE0);
  
  /* Reset the counter turning off ENA , MODE + setting counter to 0 */
   MSCLOCK_RAW_WRITE_LONG(0, BCM_TIMER_CONTROL(this_timer_num));
   DPRINTK("bcm_msclock_disable_clock_interrupt(timer=%u) EXIT IE0 IS NOW 0x%x, CTRL=0 \n",
              this_timer_num,enable);
}

static void bcm_msclock_setup_timer_delay(int this_timer_num, u32 usecs)
{
     u32 controlreg; 
     
     DPRINTK("bcm_msclock_setup_timer_delay(timer=%u,usecs=%u) \n",
              this_timer_num,usecs);

    /* Turn on the timer, turning on ENA , MODE to count down,
        + setting counter to appropraite ticks  */
    controlreg = RSTCNTCLR | USECS_TO_BCMTICKS(usecs);
    /* controlreg = TIMERENABLE | RSTCNTCLR | USECS_TO_BCMTICKS(usecs); */
   MSCLOCK_RAW_WRITE_LONG(controlreg,BCM_TIMER_CONTROL(this_timer_num));
    
    DPRINTK("setup_timer_delay(timer=%u,usecs=%u) EXIT CTRL=0x%x \n",
              this_timer_num,usecs,controlreg);
}

static void bcm_msclock_write_enable_bit(int this_timer_num)
{
   unsigned int  regvalue;
   DPRINTK("bcm_msclock_write_enable_bit(timer=%u) \n",
              this_timer_num);
  
   regvalue  = MSCLOCK_RAW_READ_LONG(BCM_TIMER_CONTROL(this_timer_num));
   regvalue |= TIMERENABLE;
   MSCLOCK_RAW_WRITE_LONG(regvalue,BCM_TIMER_CONTROL(this_timer_num));
   
   DPRINTK("bcm_msclock_write_enable_bit(timer=%u) newctrl=%u\n",
              this_timer_num,regvalue);
    
}

static int bcm_msclock_enable_timer(void)
{
   int          ret    = 0;
   unsigned long flags;

   if (timer_num > 3)
   {
      return -EINVAL;
   }
   
   bcm_msclock_lock(flags);
   
   /* don't reregister if needed */
   if (bcm_msclock_timer_state & BCM_MSCLOCK_TIMER_ENABLED)
   {
      bcm_msclock_unlock(flags);
      return 0;
   }
  
   
   DPRINTK(
          "%s:%i - Enabling timer! . Interrupts occur every %u usecs \n", 
          __FILE__, 
          __LINE__,
	  timer_resolution_in_usec); 

	  /* to be called with locks on */ 

   /* for safety, first clear any spurious pending interrupts */
   bcm_msclock_clear_clock_interrupt(timer_num);
   
   /* load the control register */
   bcm_msclock_setup_timer_delay(timer_num,timer_resolution_in_usec); /* one second */
   

   DPRINTK(KERN_ERR
          "%s:%i - USING REQUEST_IRQ on hwmstimer for irq %d \n", 
          __FILE__, 
          __LINE__,
          (int)BCM_LINUX_TIMR_IRQ); 

   /* Set up the ISR. */
   ret = request_irq(BCM_LINUX_TIMR_IRQ,
                     bcm_msclock_interrupt,
                     IRQF_DISABLED|IRQF_SHARED,
                     "hwmstimer",
                     bcm_msclock_interrupt);
   if (ret)
   {
      printk(KERN_ERR
             "%s:%i - Unable to get IRQ!, it returned %u \n", 
             __FILE__, 
             __LINE__,
             (unsigned int)ret); 
   }
   else
   {
      /* reset the counter */
      bcm_msclock_reset_clock_counter(timer_num);
      
      bcm_msclock_write_enable_bit(timer_num);

      /* and turn the interrupt on */
      bcm_msclock_enable_clock_interrupt(timer_num);
   }
    
   bcm_msclock_timer_state |= BCM_MSCLOCK_TIMER_ENABLED;
  
   bcm_msclock_unlock(flags);

   return ret;
}

static int bcm_msclock_disable_timer(void)
{
   unsigned long flags;
  
   bcm_msclock_lock(flags);
   /* don't reregister if needed */
   if ( !(bcm_msclock_timer_state & BCM_MSCLOCK_TIMER_ENABLED))
   {
      bcm_msclock_unlock(flags);
      return 0;
   }
  
 
   DPRINTK(
          "%s:%i - Disabling timer!\n", 
          __FILE__, 
          __LINE__); 

   /* Free the ISR. */
   free_irq(BCM_LINUX_TIMR_IRQ, bcm_msclock_interrupt); 

   bcm_msclock_disable_clock_interrupt(timer_num);

   
   /* and die */
   bcm_msclock_timer_state &= ~BCM_MSCLOCK_TIMER_ENABLED;
   bcm_msclock_unlock(flags);

   return 0;
}

#ifdef CONFIG_WATCHDOG_BCM

extern void bcm_msclock_hw_watchdog_init( void )
{
#if defined(CONFIG_WATCHDOG_BCM_7401)
   u32 reg = 0xB0000000 + BCHP_SUN_TOP_CTRL_RESET_CTRL ;
   u32 sun_top_reset_ctrl = MSCLOCK_RAW_READ_LONG( reg ) ;
   sun_top_reset_ctrl |= (1<<7); /* Set the watchdog enable mask */
   MSCLOCK_RAW_WRITE_LONG( sun_top_reset_ctrl, reg ) ;
#endif

   bcm_msclock_hw_watchdog_disarm();
}

extern void bcm_msclock_hw_watchdog_shutdown( void )
{
   bcm_msclock_hw_watchdog_disarm();
}

extern void bcm_msclock_hw_watchdog_arm(u32 msecs,
                                        bcm_msclock_hw_watchdog_cb isr_handler)
{
    u32 controlreg = MSECS_TO_BCMTICKS(msecs);

    DPRINTK("bcm_msclock_hw_watchdog_arm(msecs=%u) rawtimerval=0x%x \n",
              msecs,controlreg);
    
#ifdef CONFIG_WATCHDOG_BCM_7038
    if (isr_handler)
    {
       bcm7038_wdog_int_enabled = 1 ;
    }
    else
    {
       bcm7038_wdog_int_enabled = 0 ;
    }
#endif
    bcm_msclock_hw_watchdog_handler = isr_handler;
    
    MSCLOCK_RAW_WRITE_LONG(controlreg,   BCM_TIMER_WATCHDOG_TIMEOUT);
    MSCLOCK_RAW_WRITE_LONG(((u32)0xff00),BCM_TIMER_WATCHDOG_CMD);
    MSCLOCK_RAW_WRITE_LONG(((u32)0x00ff),BCM_TIMER_WATCHDOG_CMD);
    
#ifdef CONFIG_WATCHDOG_BCM_7038
    {
       unsigned int enable = MSCLOCK_RAW_READ_LONG(TIMR_ADR_BASE + TIMER_TIMER_IE0);
       enable &= BCM_TIMER_INT_MASK;
       if (bcm7038_wdog_int_enabled)
       {
          enable |= BCM_TIMER_WATCHDOG_INT_MASK ;
       }
       MSCLOCK_RAW_WRITE_LONG(enable, TIMR_ADR_BASE + TIMER_TIMER_IE0);
    }
#endif
    DPRINTK("bcm_msclock_hw_watchdog_arm(msecs=%u) done\n",msecs);
}
    
extern void bcm_msclock_hw_watchdog_keepalive( void )
{     
    DPRINTK("bcm_msclock_hw_watchdog_keepalive() called\n");
   
    MSCLOCK_RAW_WRITE_LONG(((u32)0xff00),BCM_TIMER_WATCHDOG_CMD);
    MSCLOCK_RAW_WRITE_LONG(((u32)0x00ff),BCM_TIMER_WATCHDOG_CMD);
    
    DPRINTK("bcm_msclock_hw_watchdog_keepalive() exiting\n");
}

extern void bcm_msclock_hw_watchdog_disarm( void )
{     
     DPRINTK("bcm_msclock_hw_watchdog_disarm() entered \n");
   
#ifdef CONFIG_WATCHDOG_BCM_7038
     bcm7038_wdog_int_enabled = 0 ;
#endif
   MSCLOCK_RAW_WRITE_LONG(((u32)0xee00),BCM_TIMER_WATCHDOG_CMD);
   MSCLOCK_RAW_WRITE_LONG(((u32)0x00ee),BCM_TIMER_WATCHDOG_CMD);
    
     /* disable the handler also */
     bcm_msclock_hw_watchdog_handler = 0;
   
    DPRINTK("bcm_msclock_hw_watchdog_disarm() done\n");
}
#endif

static long bcm_msclock_ioctl(struct file  *filp, 
                              unsigned int  cmd,
                              unsigned long arg)
{
   int ret = -ENOIOCTLCMD;
   hwmsclock_clock64_t *result = (void *) arg;

   lock_kernel();

   switch (cmd)
   {
      /* If a timer interrupt happens (which increases inttime) while
       * we read the timer counter, repeat the read of the timer counter.
       */
      case MSHWCLOCK_IOC_GETTIME:
         {
	    hwmsclock_clock64_t mstime = hwmsclock_value_msecs();
            ret = put_user(mstime, result);
         }
         break;

      case MSHWCLOCK_IOC_DEBUG_ENABLE:
	 {
           ret = get_user(hwmsclock_debug_mode, (hwmsclock_time_msecs *)arg);
	   break;
	 }
      default:
         break;
   }

   unlock_kernel();

   return ret;
}

static int bcm_msclock_open(struct inode *inode, 
                            struct file  *filp)
{
   int ret = 0;

   if (minor_dev != MINOR(inode->i_rdev))
   {
      printk(KERN_ERR
             "%s:%i - Invalid minor device number!\n", 
             __FILE__, 
             __LINE__); 
      ret = -ENODEV;
   }
   else
   {
#ifdef MODULE
      MOD_INC_USE_COUNT;
#endif
      ret = 0;
   }

   return ret;
}

static int bcm_msclock_release(struct inode *inode, 
                               struct file  *filp)
{

#ifdef MODULE
   MOD_DEC_USE_COUNT;
#endif

   return 0;   
}


#ifdef CONFIG_BCM_MSCLOCK_SUPPORTS_PROCDEVICE

/**************************************************************************************/
/* /proc device interface for hwmsclock
    
   cat /proc/hwmsclock_msecs        -- current clock value  
   
*/
/**************************************************************************************/

/* see Chapter 4 of Linux device Drivers by Rubini + Corbett */


int bcm_msclock_procdevice_readInfo(char * buf, char ** start, off_t offset, int count, int *eof, void *data)
{
  int numWritten;
  char * ptr = buf;
  ptr += sprintf(ptr,
              "hwmsclock : state %u, resolution %u, cycles_per_usec %u\n",
			   bcm_msclock_timer_state,
			   timer_resolution_in_usec,
			   timer_cycles_per_usec);
	         
  numWritten = (ptr - buf)+1;
  /* return eof */ 
     
  *eof = 1;
  
  return numWritten;
}

int bcm_msclock_procdevice_readClock(char * buf, char ** start, off_t offset, int count, int *eof, void *data)
{
  int numWritten;
  hwmsclock_time_msecs t = hwmsclock_value_msecs();
  numWritten = sprintf(buf,"%u\n",t);
  /* return eof */ 
     
  *eof = 1;
  
  return numWritten;
}

#ifdef CONFIG_WATCHDOG_BCM_7038
int bcm_msclock_procdevice_readWDCR(char * buf, char ** start, off_t offset, int count, int *eof, void *data)
{
  int numWritten;
  unsigned int wdcr = MSCLOCK_RAW_READ_LONG(BCM_TIMER_WATCHDOG_WDCRS) & 0x1;
  numWritten = sprintf(buf,"%u\n",wdcr);
  /* return eof */ 
     
  *eof = 1;
  
  return numWritten;
}
#endif

void bcm_msclock_procdevice_register( void ) 
{
   /* Create a proc device /proc/pdqueuelog containing the queue logging information */
   
   create_proc_read_entry(BCM_MSCLOCK_PROCDEVICE_INFO_NAME,
                          S_IWUSR|S_IRUSR|S_IRGRP|S_IROTH,
			  NULL,bcm_msclock_procdevice_readInfo,(void *) 0);
   create_proc_read_entry(BCM_MSCLOCK_PROCDEVICE_MSECS_NAME,
                          S_IWUSR|S_IRUSR|S_IRGRP|S_IROTH,
			  NULL,bcm_msclock_procdevice_readClock,(void *) 0);
#ifdef CONFIG_WATCHDOG_BCM_7038
   create_proc_read_entry(BCM_MSCLOCK_PROCDEVICE_WDCR_NAME,
                          S_IWUSR|S_IRUSR|S_IRGRP|S_IROTH,
			  NULL,bcm_msclock_procdevice_readWDCR,(void *) 0);
#endif
   
}

void bcm_msclock_procdevice_unregister( void ) 
{
   remove_proc_entry(BCM_MSCLOCK_PROCDEVICE_INFO_NAME,NULL/* no parent dir */);
   remove_proc_entry(BCM_MSCLOCK_PROCDEVICE_MSECS_NAME,NULL/* no parent dir */);
#ifdef CONFIG_WATCHDOG_BCM_7038
   remove_proc_entry(BCM_MSCLOCK_PROCDEVICE_WDCR_NAME,NULL/* no parent dir */);
#endif

}

#endif


static struct file_operations bcm_msclock_fops =
{
   .owner          = THIS_MODULE,
   .unlocked_ioctl = bcm_msclock_ioctl,
   .open           = bcm_msclock_open,
   .release        = bcm_msclock_release,
};

static struct miscdevice bcm_msclock_miscdev =
{
   .minor   = BCM_MSCLOCK_MINOR_DEV,
   .name    = "msclock",
   .fops    = &bcm_msclock_fops,
};

static int __init bcm_msclock_init(void)
{
   int ret = 0;

   printk("bcm_msclock:Broadcom msec hardware clock on minor %u, using hw timer %u  (c) Pace Micro Technologies Plc\n",minor_dev,timer_num);
#ifdef CONFIG_WATCHDOG_BCM
   printk("bcm_msclock:hardware watchdog timer support is available\n" );
#endif
   /* note - This code now ALWAYS starts at kernel or module init time. This driver MUST be started 
      before the broadcom timer module 
   */
   ret = bcm_msclock_enable_timer();
   if (ret)
   {
      printk(KERN_ERR
             "%s:%i - Failed to initialise timer!\n", 
             __FILE__, 
             __LINE__); 
   }
   else
   {
      bcm_msclock_miscdev.minor = minor_dev;
      ret = misc_register(&bcm_msclock_miscdev);
   }

#ifdef CONFIG_BCM_MSCLOCK_SUPPORTS_PROCDEVICE   
   bcm_msclock_procdevice_register();
#endif

#ifdef CONFIG_WATCHDOG_BCM
   bcm_msclock_hw_watchdog_init();
#endif

   return ret;
}

static void __exit bcm_msclock_exit(void)
{
#ifdef CONFIG_BCM_MSCLOCK_SUPPORTS_PROCDEVICE   
   bcm_msclock_procdevice_unregister();
#endif

   misc_deregister(&bcm_msclock_miscdev);

   bcm_msclock_disable_timer();

#ifdef CONFIG_WATCHDOG_BCM
   bcm_msclock_hw_watchdog_shutdown();
#endif

}

#ifndef MODULE

static int __init bcm_msclock_setup(char *str)
{
   int opts[5];
    int dummy_timer_cycles;
   (void) get_options(str, ARRAY_SIZE(opts), opts);
   switch (opts[0])
   {
      case 4:
         timer_resolution_in_usec = opts[4];
         /* Drop through. */
      case 3:
         dummy_timer_cycles = opts[3];
         /* Drop through. */
      case 2:
         timer_num = opts[2];
         /* Drop through. */
      case 1:
         minor_dev = opts[1];
         /* Drop through. */
      default:
         break;
   }

   return 1;
}

__setup("bcm_msclock=", bcm_msclock_setup);

#endif /* !MODULE */

module_init(bcm_msclock_init);
module_exit(bcm_msclock_exit);

MODULE_AUTHOR("Melanie Rhianna Lewis <Melanie.Lewis@pace.co.uk>");
MODULE_DESCRIPTION("BCM7038 millisecond hardware clock");

#define MINOR_DEV_DESC "The minor device number. (Default is " \
                       #BCM_MSCLOCK_MINOR_DEV ")"

module_param(minor_dev, int, 0);
MODULE_PARM_DESC(minor_dev, MINOR_DEV_DESC);

module_param(timer_num, int, 0);
MODULE_PARM_DESC(timer_num, 
                 "The timer number. "
                 "(Values 0, 1, 2 or 3.  Default is 0)");

#if 0
module_param(timer_cycles_per_usec, int, 27);
MODULE_PARM_DESC(timer_cycles_per_usec, 
                 "The timer cycles per microsec. "
                 "(Default is 27)");

module_param(timer_resolution_in_usec, int, 1000000);
MODULE_PARM_DESC(timer_resolution_in_usec, 
                 "usec interval between interrupts"
                 "(Default is 1000000)");
#endif

MODULE_LICENSE("GPL");

EXPORT_SYMBOL(hwmsclock_value_msecs);


