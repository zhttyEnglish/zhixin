#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "gpio.h"

#define SCA200V100_DTS_M "smartchip,smartx-sca200v100"

typedef enum
{
    GROUP_0 = 0,
    GROUP_1,
    GROUP_2,
    GROUP_3,
    GROUP_MAX
} ENUM_GPIO_GROUP;

#define PIN_PER_GROUP_MAX 32

/*
 * GPIO name: <GROUP>_<PIN>
 *   GROUP:[0-3]. base is 32.
 *   PIN:[0-31].  base is 1.
 */
static int sysfs_gpio_nr(unsigned int group, unsigned int pin)
{
    if(group >= GROUP_MAX)
    {
        printf("invalid group num!\n");
        return -1;
    }

    if(pin >= PIN_PER_GROUP_MAX)
    {
        printf("invalid pin num!\n");
        return -1;
    }

    return (group * 32 + pin);
}

int sca200v100_gpio_probe(struct gpio_chip *pchip, const char *comp)
{
    if(strcmp(SCA200V100_DTS_M, comp))
        return -1;

    memset(pchip, 0, sizeof(*pchip));

    pchip->group_max = GROUP_MAX;
    pchip->sysfs_gpio_nr = sysfs_gpio_nr;

    return 0;
}
