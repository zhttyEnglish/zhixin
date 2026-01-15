#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "gpio.h"

#define SCA200V200_DTS_M "smartchip,smartx-sca200v200"

typedef enum
{
    GROUP_A = 0,
    GROUP_B,
    GROUP_C,
    GROUP_D,
    GROUP_E,
    GROUP_F,
    GROUP_G,
    GROUP_MAX
} ENUM_GPIO_GROUP;

static unsigned char sca200v200_pin_nr[GROUP_MAX] =  {
    23, 22, 26, 6,
    6,  11, 16
};

/*
 * GPIO name: <GROUP>_<PIN>
 *   GROUP:[0-6].
 *   PIN:[0-31].
 */
static int sysfs_gpio_nr(unsigned int group, unsigned int pin)
{
    int gpio_nr = 0;
    int i;

    if(group >= GROUP_MAX) {
        printf("invalid group %d\n", group);
        return -1;
    }
    if(pin >= sca200v200_pin_nr[group]) {
        printf("invalid pin %d\n", pin);
        return -1;
    }

    for (i=0; i < group; i++) {
        gpio_nr += sca200v200_pin_nr[i];
    }

    return gpio_nr + pin;
}

int sca200v200_gpio_probe(struct gpio_chip *pchip, const char *comp)
{
    if(strcmp(SCA200V200_DTS_M, comp))
        return -1;

    memset(pchip, 0, sizeof(*pchip));

    pchip->group_max = GROUP_MAX;
    pchip->sysfs_gpio_nr = sysfs_gpio_nr;

    return 0;
}
