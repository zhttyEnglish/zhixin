/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Header providing constants for SmartChip pinctrl bindings.
 *
 * Copyright (c) 2022 SmartChip
 */

#ifndef __DT_BINDINGS_PINCTRL_SCA200V200_H__
#define __DT_BINDINGS_PINCTRL_SCA200V200_H__

#define PIN0    0x00000000
#define PIN1    0x00010000
#define PIN2    0x00020000
#define PIN3    0x00030000
#define PIN4    0x00040000
#define PIN5    0x00050000
#define PIN6    0x00060000
#define PIN7    0x00070000
#define PIN8    0x00080000
#define PIN9    0x00090000
#define PIN10   0x000A0000
#define PIN11   0x000B0000
#define PIN12   0x000C0000
#define PIN13   0x000D0000
#define PIN14   0x000E0000
#define PIN15   0x000F0000
#define PIN16   0x00100000
#define PIN17   0x00110000
#define PIN18   0x00120000
#define PIN19   0x00130000
#define PIN20   0x00140000
#define PIN21   0x00150000
#define PIN22   0x00160000
#define PIN23   0x00170000
#define PIN24   0x00180000
#define PIN25   0x00190000
#define PIN26   0x001A0000
#define PIN27   0x001B0000
#define PIN28   0x001C0000
#define PIN29   0x001D0000
#define PIN30   0x001E0000
#define PIN31   0x001F0000
#define PIN32   0x00200000
#define PIN33   0x00210000
#define PIN34   0x00220000
#define PIN35   0x00230000
#define PIN36   0x00240000
#define PIN37   0x00250000
#define PIN38   0x00260000
#define PIN39   0x00270000
#define PIN40   0x00280000
#define PIN41   0x00290000
#define PIN42   0x002A0000
#define PIN43   0x002B0000
#define PIN44   0x002C0000
#define PIN45   0x002D0000
#define PIN46   0x002E0000
#define PIN47   0x002F0000
#define PIN48   0x00300000
#define PIN49   0x00310000
#define PIN50   0x00320000
#define PIN51   0x00330000
#define PIN52   0x00340000
#define PIN53   0x00350000
#define PIN54   0x00360000
#define PIN55   0x00370000
#define PIN56   0x00380000
#define PIN57   0x00390000
#define PIN58   0x003A0000
#define PIN59   0x003B0000
#define PIN60   0x003C0000
#define PIN61   0x003D0000
#define PIN62   0x003E0000
#define PIN63   0x003F0000
#define PIN64   0x00400000
#define PIN65   0x00410000
#define PIN66   0x00420000
#define PIN67   0x00430000
#define PIN68   0x00440000
#define PIN69   0x00450000
#define PIN70   0x00460000
#define PIN71   0x00470000
#define PIN72   0x00480000
#define PIN73   0x00490000
#define PIN74   0x004A0000
#define PIN75   0x004B0000
#define PIN76   0x004C0000
#define PIN77   0x004D0000
#define PIN78   0x004E0000
#define PIN79   0x004F0000
#define PIN80   0x00500000
#define PIN81   0x00510000
#define PIN82   0x00520000
#define PIN83   0x00530000
#define PIN84   0x00540000
#define PIN85   0x00550000
#define PIN86   0x00560000
#define PIN87   0x00570000
#define PIN88   0x00580000
#define PIN89   0x00590000
#define PIN90   0x005A0000
#define PIN91   0x005B0000
#define PIN92   0x005C0000
#define PIN93   0x005D0000
#define PIN94   0x005E0000
#define PIN95   0x005F0000
#define PIN96   0x00600000
#define PIN97   0x00610000
#define PIN98   0x00620000
#define PIN99   0x00630000
#define PIN100  0x00640000
#define PIN101  0x00650000
#define PIN102  0x00660000
#define PIN103  0x00670000
#define PIN104  0x00680000
#define PIN105  0x00690000
#define PIN106  0x006A0000
#define PIN107  0x006B0000
#define PIN108  0x006C0000
#define PIN109  0x006D0000

#define FUNCTION_MODE0  0x0000
#define FUNCTION_MODE1  0x0001
#define FUNCTION_MODE2  0x0002
#define FUNCTION_MODE3  0x0003
#define FUNCTION_MODE4  0x0004
#define FUNCTION_MODE5  0x0005
#define FUNCTION_MODE6  0x0006

#define NO_PULL         0x0000
#define PULL_UP         0x0008
#define PULL_DOWN       0x0010
#define PULL_UP_AND_DOWN    0x0018

#define SLEW_RATE_DISABLE   0x0000
#define SLEW_RATE_ENABLE    0x0020

#define SCHMITT_TRIGGER_DISABLE 0x0000
#define SCHMITT_TRIGGER_ENABLE  0x0040

#define DRIVING_SELECTOR_0  0x0000
#define DRIVING_SELECTOR_1  0x0080
#define DRIVING_SELECTOR_2  0x0100
#define DRIVING_SELECTOR_3  0x0180
#define DRIVING_SELECTOR_4  0x0200      /**< SD PAD only */
#define DRIVING_SELECTOR_5  0x0280      /**< SD PAD only */
#define DRIVING_SELECTOR_6  0x0300      /**< SD PAD only */
#define DRIVING_SELECTOR_7  0x0380      /**< SD PAD only */

#define PB_MSC_BIT0               0     /**< GPIOA */
#define PB_MSC_BIT1               1     /**< GPIOB */
#define PB_MSC_BIT2               2     /**< GPIOC */
#define PB_MSC_BIT3               3     /**< GPIOD */
#define PB_MSC_BIT4               4     /**< GPIOE */
#define PB_MSC_BIT5               5     /**< GPIOF */
#define PB_MSC_BIT6               6     /**< GPIOG */

#define PB_MSC_REG_PIN_V18        0x00010000    /**< 1.8v */
#define PB_MSC_REG_PIN_V33        0x00000000    /**< 3.3v */

#endif

