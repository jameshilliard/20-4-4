/* GPIO driver for BCM7xxx chips
 *
 * This file is licensed under  the terms of the GNU General Public
 * License version 2. This program is licensed "as is" without any
 * warranty of any kind, whether express or implied. */

#include <linux/module.h>
#include <linux/pace_gpio.h>
#include <asm/io.h>
#include <asm/brcmstb/brcmstb.h>

/* The registers below are correct for 7420, 7425 and 7125 */
/* Need to check if other chips are used             */

#if defined(CONFIG_BCM7420)
#define PACE_GPIO_SHIFT 10
#elif defined(CONFIG_BCM7125)
#define PACE_GPIO_SHIFT 8
#elif defined(CONFIG_BCM7425)
#define PACE_GPIO_SHIFT 6


#else
#error Need to define a chip type
#endif

static DEFINE_SPINLOCK(gpio_lock);
static DEFINE_PER_CPU(unsigned long, flags);

void gpio_lock_for_iodir_and_level(void)
{
	spin_lock_irqsave( &gpio_lock, __get_cpu_var(flags) );
}

void gpio_unlock_for_iodir_and_level(void)
{
	spin_unlock_irqrestore( &gpio_lock, __get_cpu_var(flags) );
}

static void gpio_write_reg( unsigned int pin, unsigned int value, int is_iodir )
{
	u32 reg_val;
	u32 pin_bit;
	u32 address ;

	gpio_lock_for_iodir_and_level();

	/*
	 * because of the numbering of GPIO pins in the GPIO registers,
	 * it is convenient to add a shift to the pin number if pin>64
	 */
        if (pin >= 64) pin += PACE_GPIO_SHIFT ;

        pin_bit = pin % 32 ;

	address = is_iodir ? BCHP_GIO_IODIR_LO : BCHP_GIO_DATA_LO;
	address += (pin/32) * (BCHP_GIO_DATA_HI-BCHP_GIO_DATA_LO);

	reg_val  = BDEV_RD(address);
	reg_val &= ~(0x1 << pin_bit);
	reg_val |= ((value & 0x1) << pin_bit);
	BDEV_WR(address, reg_val );

	gpio_unlock_for_iodir_and_level();
}

#if defined(CONFIG_BCM7425)
static void gpio_write_7425_aon_reg( unsigned int pin, unsigned int value, int is_iodir )
{
	u32 reg_val;
	u32 pin_bit;
	u32 address ;

	gpio_lock_for_iodir_and_level();

    if (pin >= 18) 
	{
    	address = is_iodir ? BCHP_GIO_AON_IODIR_EXT : BCHP_GIO_AON_DATA_EXT;
        /* BCM_GIO_AON_xxx_EXT [3:0] are sgpio bits so add a 4 to this */
        pin_bit = (pin - 18 + 4 ) % 32 ;
	}
	else
	{
  	   address = is_iodir ? BCHP_GIO_AON_IODIR_LO : BCHP_GIO_AON_DATA_LO;
       pin_bit = pin ;

	}

	reg_val  = BDEV_RD(address);
	reg_val &= ~(0x1 << pin_bit);
	reg_val |= ((value & 0x1) << pin_bit);

	BDEV_WR(address, reg_val );

	gpio_unlock_for_iodir_and_level();
}
#endif

/* if pin > 1000 then assume it is an AON pin */

void gpio_set_level(unsigned int pin, unsigned int value)
{
   if (pin > 1000)
   {
#if defined(CONFIG_BCM7425)
      gpio_write_7425_aon_reg(pin-1000, value, 0);
#endif
   }
   else
   {
      gpio_write_reg(pin, value, 0);
   }    
}

void gpio_set_iodir(unsigned int pin, unsigned int value)
{
printk("%s pin=%d value=%d\n", __FUNCTION__, pin, value);
   if (pin > 1000)
   {
#if defined(CONFIG_BCM7425)
      gpio_write_7425_aon_reg(pin-1000, value, 1);
#endif
   }
   else
   {
      gpio_write_reg(pin, value, 1);
   }
}

EXPORT_SYMBOL(gpio_set_level);
EXPORT_SYMBOL(gpio_lock_for_iodir_and_level);
EXPORT_SYMBOL(gpio_unlock_for_iodir_and_level);

MODULE_AUTHOR("James Chapman <jchapman@katalix.com>">);
MODULE_DESCRIPTION("PACE GPIO driver");
MODULE_LICENSE("GPL");

