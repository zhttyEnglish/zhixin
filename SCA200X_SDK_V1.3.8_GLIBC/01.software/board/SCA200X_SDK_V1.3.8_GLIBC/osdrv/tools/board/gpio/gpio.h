#ifndef _TEST_GPIO_H
#define _TEST_GPIO_H

struct gpio_chip
{
    int group_max;

    int (*sysfs_gpio_nr)(unsigned int group, unsigned int pin);
};

int sca200v100_gpio_chip(struct gpio_chip *pchip, const char *comp);
int sca200v200_gpio_chip(struct gpio_chip *pchip, const char *comp);

#endif /* _TEST_GPIO_H */
