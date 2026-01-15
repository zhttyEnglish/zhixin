/**
 * @file     sc_gpio.h
 * @brief    GPIO控制枚举定义和接口声明
 * @version  1.0.0
 * @since    1.0.0
 * @author   陶友水<taoyoushui@sgitg.sgcc.com.cn>
 * @date     2021-11-15 创建文件
 */
/************************************************************
 *@note
    Copyright 2021, BEIJIING SMARTCHIP MICROELECTRONICS TECHNOLOGY CO., LTD.
                   ALL RIGHTS RESERVED
    Permission is hereby granted to licensees of  BEIJIING SMARTCHIP MICROELECTRONICS TECHNOLOGY CO., LTD. to use
    or abstract this computer program for the sole purpose of implementing a product based on BEIJIING SMARTCHIP
    MICROELECTRONICS TECHNOLOGY CO., LTD. No other rights to reproduce, use, or disseminate this computer program,
    whether in part or in whole, are granted. BEIJIING SMARTCHIP MICROELECTRONICS TECHNOLOGY CO., LTD. makes no
    representation or warranties with respect to the performance of this computer program, and specifically disclaims
    any responsibility for any damages, special or consequential, connected with the use of this program.
　　For details, see http://www.sgitg.sgcc.com.cn/
**********************************************************/


#ifndef __SC_GPIO_H__
#define __SC_GPIO_H__
#ifdef __cplusplus
extern "C" {
#endif

int sc_gpio_set_value(unsigned int gpio, unsigned int value);
int sc_gpio_get_value(unsigned int gpio, unsigned int *value);
int sc_gpio_name_to_num(unsigned int group, unsigned int pin);

#ifdef __cplusplus
}
#endif

#endif //__SC_GPIO_H__
