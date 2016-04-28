#ifndef _PACE_GPIO_H_
#define _PACE_GPIO_H_

#define GPIO_DIR_OUTPUT 0
#define GPIO_DIR_INPUT  1

#ifdef __KERNEL__
extern void gpio_lock_for_iodir_and_level(void);
extern void gpio_unlock_for_iodir_and_level(void);
extern void gpio_set_level(unsigned int pin, unsigned int value);
extern void gpio_set_iodir(unsigned int pin, unsigned int value);
#endif /* __KERNEL__ */

#endif /* _PACE_GPIO_H_ */
