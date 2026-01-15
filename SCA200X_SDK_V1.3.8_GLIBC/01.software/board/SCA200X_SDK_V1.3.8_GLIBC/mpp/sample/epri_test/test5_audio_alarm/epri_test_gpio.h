#ifndef __EPRI_TEST_GPIO_H
#define __EPRI_TEST_GPIO_H

//gpip port
typedef struct
{
    unsigned char port;
    unsigned char pinnum;
    unsigned char val;

}HalGpioProp;


int halGPIOInit(void);
int halGPIOWrite(HalGpioProp *pGpioPro);



#endif

