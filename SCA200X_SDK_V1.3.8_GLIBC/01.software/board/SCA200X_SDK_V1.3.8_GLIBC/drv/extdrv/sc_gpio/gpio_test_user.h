#ifndef _DRV_GPIO_TEST_USER_H
#define _DRV_GPIO_TEST_USER_H

#include <linux/ioctl.h>

#define DEV_NAME_GPIO_TEST "sc_gpio"

struct gpio_drv {
	int group;
	int pin;
	int val;
};

#define DRV_GPIO_GET    _IOWR('p', 0x1, struct gpio_drv)
#define DRV_GPIO_SET    _IOW('p', 0x2, struct gpio_drv)

#endif /* _DRV_GPIO_TEST_USER_H */
