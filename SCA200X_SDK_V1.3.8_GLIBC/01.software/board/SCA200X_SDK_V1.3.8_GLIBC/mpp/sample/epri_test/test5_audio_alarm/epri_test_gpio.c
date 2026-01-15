#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>



#include "epri_test_gpio.h"

static int g_gpiofd = -1;


#define GPIO_DEVICE_NAME "/dev/gpio"
#define GPIO_READ           _IOR('p', 0x30, HalGpioProp)
#define GPIO_WRITE          _IOR('p', 0x31, HalGpioProp)


int halGPIOInit(void)
{
    if(g_gpiofd <= 0)
    {
        g_gpiofd = open(GPIO_DEVICE_NAME, O_RDWR);
        if (g_gpiofd <= 0)
        {
            printf("%s L%d open device %s error!\n",
                __FUNCTION__, __LINE__, GPIO_DEVICE_NAME);
            return -1;
        }
    }
    return 0;
}

int halGPIOWrite(HalGpioProp *pGpioPro)
{
    int retVal = 0;

    retVal = ioctl(g_gpiofd, GPIO_WRITE, pGpioPro);
    if(retVal != 0)
        return -1;
    else
        return 0;
}



