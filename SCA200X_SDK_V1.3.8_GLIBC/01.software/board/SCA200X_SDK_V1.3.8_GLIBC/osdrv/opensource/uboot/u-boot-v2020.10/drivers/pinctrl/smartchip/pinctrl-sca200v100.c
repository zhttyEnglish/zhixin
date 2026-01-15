// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) EETS GmbH, 2017, Felix Brack <f.brack@eets.ch>
 */

#include <common.h>
#include <dm.h>
#include <dm/device_compat.h>
#include <dm/pinctrl.h>
#include <linux/libfdt.h>
#include <asm/io.h>
#include <regmap.h>

#define PAD_FUNC_SEL0           0x00
#define PAD_FUNC_SEL1           0x04
#define PAD_FUNC_SEL2           0x08
#define PAD_FUNC_SEL3           0x0c
#define PAD_FUNC_SEL4           0x10
#define PAD_FUNC_SEL5           0x14
#define PAD_FUNC_SEL6           0x18
#define PAD_FUNC_SEL7           0x1c
#define PAD_FUNC_SEL8           0x20
#define PAD_FUNC_SEL9           0x24
#define PAD_FUNC_SEL10          0x28
#define PAD_FUNC_SEL11          0x2c
#define PAD_FUNC_SEL12          0x30
#define PAD_FUNC_SEL13          0x34
#define PAD_FUNC_SEL14          0x38
#define PAD_FUNC_SEL15          0x3c
#define PAD_FUNC_SEL16          0x40
#define PAD_FUNC_SEL17          0x44
#define PAD_FUNC_SEL18          0x48
#define PAD_FUNC_SEL19          0x4c
#define PAD_FUNC_SEL20          0x50
#define PAD_FUNC_SEL21          0x54
#define PAD_FUNC_SEL22          0x58
#define PAD_FUNC_SEL23          0x5c

#define DT_CONFIG_MASK          0x0000ffff
#define DT_MODE_MASK            0x00000007
#define DT_PAD_MASK         0xffff0000
#define DT_CONFIG_SHIFT         0
#define DT_PAD_SHIFT            16

#define REG_CONFIG_ONEPAD_MASK      0x000003ff
#define REG_CONFIG_ONEPAD_WIDTH     10
#define REG_FUNCSEL_ONEPAD_MASK     0x00000007

#define PB_MSC_REG0_OFFSET          0x00000104
#define PB_MSC_SHIFT                16
#define PB_MSC_BIT_MASK             GENMASK(PB_MSC_SHIFT-1, 0)
#define PB_MSC_CONFIG_MASK          GENMASK(PB_MSC_SHIFT+1, PB_MSC_SHIFT)
#define PB_MSC_RESERVED_MASK        GENMASK(31, 11)

struct pad_func_sel {
	unsigned char pad_index;
	unsigned char mode;
	unsigned char offset;
	unsigned char shift;
	char val;
};

struct sca200v100_pinctrl_priv {
	void __iomem *pb_sw_reg_base;
	void __iomem *pad_func_sel_base;
	struct pad_func_sel *p_pad_sels;
};

// {pad_num, mod, pad_func_selx, bit_offset, val}
struct pad_func_sel pad_sels[] = {
	{167, 2, PAD_FUNC_SEL0,  0, 0},
	{191, 3, PAD_FUNC_SEL0,  0, 1},
	{ 27, 1, PAD_FUNC_SEL0,  3, 0},
	{101, 3, PAD_FUNC_SEL0,  3, 1},
	{111, 1, PAD_FUNC_SEL0,  3, 2},
	{169, 2, PAD_FUNC_SEL0,  3, 3},
	{ 29, 1, PAD_FUNC_SEL0,  6, 0},
	{103, 3, PAD_FUNC_SEL0,  6, 1},
	{113, 1, PAD_FUNC_SEL0,  6, 2},
	{171, 2, PAD_FUNC_SEL0,  6, 3},
	{ 40, 1, PAD_FUNC_SEL0,  9, 0},
	{115, 1, PAD_FUNC_SEL0,  9, 1},
	{173, 2, PAD_FUNC_SEL0,  9, 2},
	{ 40, 1, PAD_FUNC_SEL0,  9, 3},
	{ 42, 1, PAD_FUNC_SEL0, 12, 0},
	{ 80, 4, PAD_FUNC_SEL0, 12, 1},
	{ 43, 1, PAD_FUNC_SEL0, 15, 0},
	{ 81, 4, PAD_FUNC_SEL0, 15, 1},
	{ 44, 1, PAD_FUNC_SEL0, 18, 0},
	{ 82, 4, PAD_FUNC_SEL0, 18, 1},
	{ 30, 5, PAD_FUNC_SEL0, 21, 0},
	{ 83, 4, PAD_FUNC_SEL0, 21, 1},
	{ 35, 5, PAD_FUNC_SEL0, 24, 0},
	{187, 3, PAD_FUNC_SEL0, 24, 1},
	{ 36, 5, PAD_FUNC_SEL0, 27, 0},
	{188, 3, PAD_FUNC_SEL0, 27, 1},
	{ 37, 5, PAD_FUNC_SEL1,  0, 0},
	{189, 3, PAD_FUNC_SEL1,  0, 1},
	{ 38, 5, PAD_FUNC_SEL1,  3, 0},
	{190, 3, PAD_FUNC_SEL1,  3, 1},
	{ 28, 1, PAD_FUNC_SEL1,  6, 0},
	{102, 3, PAD_FUNC_SEL1,  6, 1},
	{112, 1, PAD_FUNC_SEL1,  6, 2},
	{170, 2, PAD_FUNC_SEL1,  6, 3},
	{ 39, 1, PAD_FUNC_SEL1,  9, 0},
	{104, 3, PAD_FUNC_SEL1,  9, 1},
	{114, 1, PAD_FUNC_SEL1,  9, 2},
	{172, 2, PAD_FUNC_SEL1,  9, 3},
	{ 41, 1, PAD_FUNC_SEL1, 12, 0},
	{116, 1, PAD_FUNC_SEL1, 12, 1},
	{174, 2, PAD_FUNC_SEL1, 12, 2},
	{ 41, 1, PAD_FUNC_SEL1, 12, 3},
	{  3, 5, PAD_FUNC_SEL1, 15, 0},
	{ 62, 6, PAD_FUNC_SEL1, 15, 1},
	{ 54, 0, PAD_FUNC_SEL1, 18, 0},
	{  2, 2, PAD_FUNC_SEL1, 18, 1},
	{ 82, 5, PAD_FUNC_SEL1, 18, 2},
	{186, 4, PAD_FUNC_SEL1, 18, 3},
	{ 84, 0, PAD_FUNC_SEL1, 21, 0},
	{  3, 2, PAD_FUNC_SEL1, 21, 1},
	{ 75, 6, PAD_FUNC_SEL1, 21, 2},
	{ 83, 5, PAD_FUNC_SEL1, 21, 3},
	{187, 4, PAD_FUNC_SEL1, 21, 4},
	{ 84, 0, PAD_FUNC_SEL1, 21, 5},
	{ 84, 0, PAD_FUNC_SEL1, 21, 6},
	{ 84, 0, PAD_FUNC_SEL1, 21, 7},
	{ 85, 0, PAD_FUNC_SEL1, 24, 0},
	{  4, 2, PAD_FUNC_SEL1, 24, 1},
	{188, 4, PAD_FUNC_SEL1, 24, 2},
	{ 85, 0, PAD_FUNC_SEL1, 24, 3},
	{ 86, 0, PAD_FUNC_SEL1, 27, 0},
	{  5, 2, PAD_FUNC_SEL1, 27, 1},
	{137, 4, PAD_FUNC_SEL1, 27, 2},
	{ 86, 0, PAD_FUNC_SEL1, 27, 3},
	{ 87, 0, PAD_FUNC_SEL2,  0, 0},
	{  6, 2, PAD_FUNC_SEL2,  0, 1},
	{ 56, 5, PAD_FUNC_SEL2,  0, 2},
	{ 78, 5, PAD_FUNC_SEL2,  0, 3},
	{138, 4, PAD_FUNC_SEL2,  0, 4},
	{ 87, 0, PAD_FUNC_SEL2,  0, 5},
	{ 87, 0, PAD_FUNC_SEL2,  0, 6},
	{ 87, 0, PAD_FUNC_SEL2,  0, 7},
	{ 88, 0, PAD_FUNC_SEL2,  3, 0},
	{  7, 2, PAD_FUNC_SEL2,  3, 1},
	{ 57, 5, PAD_FUNC_SEL2,  3, 2},
	{ 79, 5, PAD_FUNC_SEL2,  3, 3},
	{144, 4, PAD_FUNC_SEL2,  3, 4},
	{ 88, 0, PAD_FUNC_SEL2,  3, 5},
	{ 88, 0, PAD_FUNC_SEL2,  3, 6},
	{ 88, 0, PAD_FUNC_SEL2,  3, 7},
	{ 89, 0, PAD_FUNC_SEL2,  6, 0},
	{  8, 2, PAD_FUNC_SEL2,  6, 1},
	{ 58, 5, PAD_FUNC_SEL2,  6, 2},
	{ 62, 5, PAD_FUNC_SEL2,  6, 3},
	{ 80, 5, PAD_FUNC_SEL2,  6, 4},
	{145, 6, PAD_FUNC_SEL2,  6, 5},
	{ 89, 0, PAD_FUNC_SEL2,  6, 6},
	{ 89, 0, PAD_FUNC_SEL2,  6, 7},
	{ 90, 0, PAD_FUNC_SEL2,  9, 0},
	{  9, 2, PAD_FUNC_SEL2,  9, 1},
	{ 59, 5, PAD_FUNC_SEL2,  9, 2},
	{ 63, 5, PAD_FUNC_SEL2,  9, 3},
	{ 81, 5, PAD_FUNC_SEL2,  9, 4},
	{146, 6, PAD_FUNC_SEL2,  9, 5},
	{ 90, 0, PAD_FUNC_SEL2,  9, 6},
	{ 90, 0, PAD_FUNC_SEL2,  9, 7},
	{ 93, 0, PAD_FUNC_SEL2, 12, 0},
	{149, 6, PAD_FUNC_SEL2, 12, 1},
	{117, 0, PAD_FUNC_SEL2, 15, 0},
	{ 18, 2, PAD_FUNC_SEL2, 15, 1},
	{111, 5, PAD_FUNC_SEL2, 15, 2},
	{161, 4, PAD_FUNC_SEL2, 15, 3},
	{118, 0, PAD_FUNC_SEL2, 18, 0},
	{ 19, 2, PAD_FUNC_SEL2, 18, 1},
	{172, 4, PAD_FUNC_SEL2, 18, 2},
	{118, 0, PAD_FUNC_SEL2, 18, 3},
	{119, 0, PAD_FUNC_SEL2, 21, 0},
	{ 20, 2, PAD_FUNC_SEL2, 21, 1},
	{ 27, 2, PAD_FUNC_SEL2, 21, 2},
	{108, 5, PAD_FUNC_SEL2, 21, 3},
	{173, 4, PAD_FUNC_SEL2, 21, 4},
	{119, 0, PAD_FUNC_SEL2, 21, 5},
	{119, 0, PAD_FUNC_SEL2, 21, 6},
	{119, 0, PAD_FUNC_SEL2, 21, 7},
	{120, 0, PAD_FUNC_SEL2, 24, 0},
	{ 21, 2, PAD_FUNC_SEL2, 24, 1},
	{ 28, 2, PAD_FUNC_SEL2, 24, 2},
	{109, 5, PAD_FUNC_SEL2, 24, 3},
	{174, 4, PAD_FUNC_SEL2, 24, 4},
	{120, 0, PAD_FUNC_SEL2, 24, 5},
	{120, 0, PAD_FUNC_SEL2, 24, 6},
	{120, 0, PAD_FUNC_SEL2, 24, 7},
	{121, 0, PAD_FUNC_SEL2, 27, 0},
	{ 22, 2, PAD_FUNC_SEL2, 27, 1},
	{ 29, 2, PAD_FUNC_SEL2, 27, 2},
	{152, 6, PAD_FUNC_SEL2, 27, 3},
	{110, 0, PAD_FUNC_SEL3,  0, 0},
	{ 54, 1, PAD_FUNC_SEL3,  0, 1},
	{182, 0, PAD_FUNC_SEL3,  3, 0},
	{ 55, 1, PAD_FUNC_SEL3,  3, 1},
	{183, 0, PAD_FUNC_SEL3,  6, 0},
	{ 84, 3, PAD_FUNC_SEL3,  6, 1},
	{189, 0, PAD_FUNC_SEL3,  9, 0},
	{ 87, 1, PAD_FUNC_SEL3,  9, 1},
	{155, 4, PAD_FUNC_SEL3,  9, 2},
	{189, 0, PAD_FUNC_SEL3,  9, 3},
	{190, 0, PAD_FUNC_SEL3, 12, 0},
	{156, 4, PAD_FUNC_SEL3, 12, 1},
	{191, 0, PAD_FUNC_SEL3, 15, 0},
	{157, 4, PAD_FUNC_SEL3, 15, 1},
	{ 94, 0, PAD_FUNC_SEL3, 18, 0},
	{ 88, 1, PAD_FUNC_SEL3, 18, 1},
	{150, 6, PAD_FUNC_SEL3, 18, 2},
	{ 94, 0, PAD_FUNC_SEL3, 18, 3},
	{ 95, 0, PAD_FUNC_SEL3, 21, 0},
	{151, 6, PAD_FUNC_SEL3, 21, 1},
	{ 96, 0, PAD_FUNC_SEL3, 24, 0},
	{158, 6, PAD_FUNC_SEL3, 24, 1},
	{ 97, 0, PAD_FUNC_SEL3, 27, 0},
	{153, 6, PAD_FUNC_SEL3, 27, 1},
	{ 98, 0, PAD_FUNC_SEL4,  0, 0},
	{154, 6, PAD_FUNC_SEL4,  0, 1},
	{102, 0, PAD_FUNC_SEL4,  3, 0},
	{115, 4, PAD_FUNC_SEL4,  3, 1},
	{103, 0, PAD_FUNC_SEL4,  6, 0},
	{116, 4, PAD_FUNC_SEL4,  6, 1},
	{104, 0, PAD_FUNC_SEL4,  9, 0},
	{117, 4, PAD_FUNC_SEL4,  9, 1},
	{105, 0, PAD_FUNC_SEL4, 12, 0},
	{118, 4, PAD_FUNC_SEL4, 12, 1},
	{106, 0, PAD_FUNC_SEL4, 15, 0},
	{119, 4, PAD_FUNC_SEL4, 15, 1},
	{107, 0, PAD_FUNC_SEL4, 18, 0},
	{120, 4, PAD_FUNC_SEL4, 18, 1},
	{108, 0, PAD_FUNC_SEL4, 21, 0},
	{121, 4, PAD_FUNC_SEL4, 21, 1},
	{109, 0, PAD_FUNC_SEL4, 24, 0},
	{192, 2, PAD_FUNC_SEL4, 24, 1},
	{122, 0, PAD_FUNC_SEL4, 27, 0},
	{ 30, 2, PAD_FUNC_SEL4, 27, 1},
	{123, 0, PAD_FUNC_SEL5,  0, 0},
	{ 31, 2, PAD_FUNC_SEL5,  0, 1},
	{124, 0, PAD_FUNC_SEL5,  3, 0},
	{ 32, 2, PAD_FUNC_SEL5,  3, 1},
	{125, 0, PAD_FUNC_SEL5,  6, 0},
	{ 33, 2, PAD_FUNC_SEL5,  6, 1},
	{126, 0, PAD_FUNC_SEL5,  9, 0},
	{ 34, 2, PAD_FUNC_SEL5,  9, 1},
	{127, 0, PAD_FUNC_SEL5, 12, 0},
	{ 35, 2, PAD_FUNC_SEL5, 12, 1},
	{128, 0, PAD_FUNC_SEL5, 15, 0},
	{ 36, 2, PAD_FUNC_SEL5, 15, 1},
	{129, 0, PAD_FUNC_SEL5, 18, 0},
	{ 37, 2, PAD_FUNC_SEL5, 18, 1},
	{ 56, 0, PAD_FUNC_SEL5, 21, 0},
	{131, 4, PAD_FUNC_SEL5, 21, 1},
	{ 57, 0, PAD_FUNC_SEL5, 24, 0},
	{132, 4, PAD_FUNC_SEL5, 24, 1},
	{ 58, 0, PAD_FUNC_SEL5, 27, 0},
	{133, 4, PAD_FUNC_SEL5, 27, 1},
	{ 59, 0, PAD_FUNC_SEL6,  0, 0},
	{134, 4, PAD_FUNC_SEL6,  0, 1},
	{ 60, 0, PAD_FUNC_SEL6,  3, 0},
	{135, 4, PAD_FUNC_SEL6,  3, 1},
	{ 61, 0, PAD_FUNC_SEL6,  6, 0},
	{136, 4, PAD_FUNC_SEL6,  6, 1},
	{163, 0, PAD_FUNC_SEL6,  9, 0},
	{ 85, 5, PAD_FUNC_SEL6,  9, 1},
	{122, 4, PAD_FUNC_SEL6,  9, 2},
	{163, 0, PAD_FUNC_SEL6,  9, 3},
	{164, 0, PAD_FUNC_SEL6, 12, 0},
	{ 86, 5, PAD_FUNC_SEL6, 12, 1},
	{165, 0, PAD_FUNC_SEL6, 15, 0},
	{140, 5, PAD_FUNC_SEL6, 15, 1},
	{166, 0, PAD_FUNC_SEL6, 18, 0},
	{141, 5, PAD_FUNC_SEL6, 18, 1},
	{167, 0, PAD_FUNC_SEL6, 21, 0},
	{142, 5, PAD_FUNC_SEL6, 21, 1},
	{169, 0, PAD_FUNC_SEL6, 24, 0},
	{ 82, 3, PAD_FUNC_SEL6, 24, 1},
	{170, 0, PAD_FUNC_SEL6, 27, 0},
	{ 83, 3, PAD_FUNC_SEL6, 27, 1},
	{ 91, 0, PAD_FUNC_SEL7,  0, 0},
	{ 10, 2, PAD_FUNC_SEL7,  0, 1},
	{ 64, 5, PAD_FUNC_SEL7,  0, 2},
	{123, 4, PAD_FUNC_SEL7,  0, 3},
	{147, 6, PAD_FUNC_SEL7,  0, 4},
	{159, 2, PAD_FUNC_SEL7,  0, 5},
	{ 91, 0, PAD_FUNC_SEL7,  0, 6},
	{ 91, 0, PAD_FUNC_SEL7,  0, 7},
	{ 92, 0, PAD_FUNC_SEL7,  3, 0},
	{ 11, 2, PAD_FUNC_SEL7,  3, 1},
	{ 65, 5, PAD_FUNC_SEL7,  3, 2},
	{124, 4, PAD_FUNC_SEL7,  3, 3},
	{148, 6, PAD_FUNC_SEL7,  3, 4},
	{160, 2, PAD_FUNC_SEL7,  3, 5},
	{ 92, 0, PAD_FUNC_SEL7,  3, 6},
	{ 92, 0, PAD_FUNC_SEL7,  3, 7},
	{111, 0, PAD_FUNC_SEL7,  6, 0},
	{ 12, 2, PAD_FUNC_SEL7,  6, 1},
	{125, 4, PAD_FUNC_SEL7,  6, 2},
	{111, 0, PAD_FUNC_SEL7,  6, 3},
	{112, 0, PAD_FUNC_SEL7,  9, 0},
	{ 13, 2, PAD_FUNC_SEL7,  9, 1},
	{126, 4, PAD_FUNC_SEL7,  9, 2},
	{112, 0, PAD_FUNC_SEL7,  9, 3},
	{113, 0, PAD_FUNC_SEL7, 12, 0},
	{ 14, 2, PAD_FUNC_SEL7, 12, 1},
	{114, 0, PAD_FUNC_SEL7, 15, 0},
	{ 15, 2, PAD_FUNC_SEL7, 15, 1},
	{115, 0, PAD_FUNC_SEL7, 18, 0},
	{ 16, 2, PAD_FUNC_SEL7, 18, 1},
	{116, 0, PAD_FUNC_SEL7, 21, 0},
	{ 17, 2, PAD_FUNC_SEL7, 21, 1},
	{130, 0, PAD_FUNC_SEL7, 24, 0},
	{ 38, 2, PAD_FUNC_SEL7, 24, 1},
	{131, 0, PAD_FUNC_SEL7, 27, 0},
	{ 39, 2, PAD_FUNC_SEL7, 27, 1},
	{132, 0, PAD_FUNC_SEL8,  0, 0},
	{ 40, 2, PAD_FUNC_SEL8,  0, 1},
	{133, 0, PAD_FUNC_SEL8,  3, 0},
	{ 41, 2, PAD_FUNC_SEL8,  3, 1},
	{134, 0, PAD_FUNC_SEL8,  6, 0},
	{ 42, 2, PAD_FUNC_SEL8,  6, 1},
	{135, 0, PAD_FUNC_SEL8,  9, 0},
	{ 43, 2, PAD_FUNC_SEL8,  9, 1},
	{136, 0, PAD_FUNC_SEL8, 12, 0},
	{ 44, 2, PAD_FUNC_SEL8, 12, 1},
	{137, 0, PAD_FUNC_SEL8, 15, 0},
	{ 45, 2, PAD_FUNC_SEL8, 15, 1},
	{ 66, 0, PAD_FUNC_SEL8, 18, 0},
	{ 89, 5, PAD_FUNC_SEL8, 18, 1},
	{ 67, 0, PAD_FUNC_SEL8, 21, 0},
	{ 90, 5, PAD_FUNC_SEL8, 21, 1},
	{ 68, 0, PAD_FUNC_SEL8, 24, 0},
	{ 91, 5, PAD_FUNC_SEL8, 24, 1},
	{ 69, 0, PAD_FUNC_SEL8, 27, 0},
	{ 92, 5, PAD_FUNC_SEL8, 27, 1},
	{ 70, 0, PAD_FUNC_SEL9,  0, 0},
	{112, 4, PAD_FUNC_SEL9,  0, 1},
	{ 71, 0, PAD_FUNC_SEL9,  3, 0},
	{113, 4, PAD_FUNC_SEL9,  3, 1},
	{ 72, 0, PAD_FUNC_SEL9,  6, 0},
	{114, 4, PAD_FUNC_SEL9,  6, 1},
	{138, 0, PAD_FUNC_SEL9,  9, 0},
	{ 46, 2, PAD_FUNC_SEL9,  9, 1},
	{186, 0, PAD_FUNC_SEL9, 12, 0},
	{ 48, 2, PAD_FUNC_SEL9, 12, 1},
	{187, 0, PAD_FUNC_SEL9, 15, 0},
	{ 49, 2, PAD_FUNC_SEL9, 15, 1},
	{ 50, 0, PAD_FUNC_SEL9, 18, 0},
	{ 23, 3, PAD_FUNC_SEL9, 18, 1},
	{ 28, 5, PAD_FUNC_SEL9, 18, 2},
	{127, 4, PAD_FUNC_SEL9, 18, 3},
	{ 51, 0, PAD_FUNC_SEL9, 21, 0},
	{ 24, 3, PAD_FUNC_SEL9, 21, 1},
	{ 29, 5, PAD_FUNC_SEL9, 21, 2},
	{128, 4, PAD_FUNC_SEL9, 21, 3},
	{ 52, 0, PAD_FUNC_SEL9, 24, 0},
	{ 25, 3, PAD_FUNC_SEL9, 24, 1},
	{ 54, 5, PAD_FUNC_SEL9, 24, 2},
	{129, 4, PAD_FUNC_SEL9, 24, 3},
	{ 53, 0, PAD_FUNC_SEL9, 27, 0},
	{ 26, 3, PAD_FUNC_SEL9, 27, 1},
	{ 55, 5, PAD_FUNC_SEL9, 27, 2},
	{130, 4, PAD_FUNC_SEL9, 27, 3},
	{ 27, 6, PAD_FUNC_SEL10,  0, 0},
	{ 90, 6, PAD_FUNC_SEL10,  0, 1},
	{128, 1, PAD_FUNC_SEL10,  0, 2},
	{136, 1, PAD_FUNC_SEL10,  0, 3},
	{152, 2, PAD_FUNC_SEL10,  0, 4},
	{ 27, 6, PAD_FUNC_SEL10,  0, 5},
	{ 27, 6, PAD_FUNC_SEL10,  0, 6},
	{ 27, 6, PAD_FUNC_SEL10,  0, 7},
	{ 29, 6, PAD_FUNC_SEL10,  3, 0},
	{ 92, 6, PAD_FUNC_SEL10,  3, 1},
	{130, 1, PAD_FUNC_SEL10,  3, 2},
	{154, 2, PAD_FUNC_SEL10,  3, 3},
	{  4, 3, PAD_FUNC_SEL10,  6, 0},
	{  7, 1, PAD_FUNC_SEL10,  6, 1},
	{ 46, 6, PAD_FUNC_SEL10,  6, 2},
	{132, 1, PAD_FUNC_SEL10,  6, 3},
	{156, 2, PAD_FUNC_SEL10,  6, 4},
	{  4, 3, PAD_FUNC_SEL10,  6, 5},
	{  4, 3, PAD_FUNC_SEL10,  6, 6},
	{  4, 3, PAD_FUNC_SEL10,  6, 7},
	{  9, 1, PAD_FUNC_SEL10,  9, 0},
	{ 48, 6, PAD_FUNC_SEL10,  9, 1},
	{ 52, 1, PAD_FUNC_SEL10,  9, 2},
	{126, 1, PAD_FUNC_SEL10,  9, 3},
	{134, 1, PAD_FUNC_SEL10,  9, 4},
	{158, 2, PAD_FUNC_SEL10,  9, 5},
	{  9, 1, PAD_FUNC_SEL10,  9, 6},
	{  9, 1, PAD_FUNC_SEL10,  9, 7},
	{ 26, 6, PAD_FUNC_SEL10, 12, 0},
	{ 75, 5, PAD_FUNC_SEL10, 12, 1},
	{ 89, 6, PAD_FUNC_SEL10, 12, 2},
	{127, 1, PAD_FUNC_SEL10, 12, 3},
	{135, 1, PAD_FUNC_SEL10, 12, 4},
	{151, 2, PAD_FUNC_SEL10, 12, 5},
	{ 26, 6, PAD_FUNC_SEL10, 12, 6},
	{ 26, 6, PAD_FUNC_SEL10, 12, 7},
	{ 28, 6, PAD_FUNC_SEL10, 15, 0},
	{ 91, 6, PAD_FUNC_SEL10, 15, 1},
	{129, 1, PAD_FUNC_SEL10, 15, 2},
	{153, 2, PAD_FUNC_SEL10, 15, 3},
	{  3, 3, PAD_FUNC_SEL10, 18, 0},
	{  6, 1, PAD_FUNC_SEL10, 18, 1},
	{ 45, 6, PAD_FUNC_SEL10, 18, 2},
	{131, 1, PAD_FUNC_SEL10, 18, 3},
	{155, 2, PAD_FUNC_SEL10, 18, 4},
	{  3, 3, PAD_FUNC_SEL10, 18, 5},
	{  3, 3, PAD_FUNC_SEL10, 18, 6},
	{  3, 3, PAD_FUNC_SEL10, 18, 7},
	{  8, 1, PAD_FUNC_SEL10, 21, 0},
	{ 47, 6, PAD_FUNC_SEL10, 21, 1},
	{ 53, 1, PAD_FUNC_SEL10, 21, 2},
	{125, 1, PAD_FUNC_SEL10, 21, 3},
	{133, 1, PAD_FUNC_SEL10, 21, 4},
	{157, 2, PAD_FUNC_SEL10, 21, 5},
	{  8, 1, PAD_FUNC_SEL10, 21, 6},
	{  8, 1, PAD_FUNC_SEL10, 21, 7},
	{ 51, 1, PAD_FUNC_SEL10, 24, 0},
	{167, 3, PAD_FUNC_SEL10, 24, 1},
	{ 22, 1, PAD_FUNC_SEL10, 27, 0},
	{ 66, 1, PAD_FUNC_SEL10, 27, 1},
	{144, 2, PAD_FUNC_SEL10, 27, 2},
	{154, 1, PAD_FUNC_SEL10, 27, 3},
	{ 23, 1, PAD_FUNC_SEL11,  0, 0},
	{ 67, 1, PAD_FUNC_SEL11,  0, 1},
	{145, 2, PAD_FUNC_SEL11,  0, 2},
	{155, 1, PAD_FUNC_SEL11,  0, 3},
	{ 64, 1, PAD_FUNC_SEL11,  3, 0},
	{138, 2, PAD_FUNC_SEL11,  3, 1},
	{ 72, 3, PAD_FUNC_SEL11,  6, 0},
	{ 79, 6, PAD_FUNC_SEL11,  6, 1},
	{184, 3, PAD_FUNC_SEL11,  6, 2},
	{ 72, 3, PAD_FUNC_SEL11,  6, 3},
	{ 74, 3, PAD_FUNC_SEL11,  9, 0},
	{ 81, 6, PAD_FUNC_SEL11,  9, 1},
	{186, 3, PAD_FUNC_SEL11,  9, 2},
	{ 74, 3, PAD_FUNC_SEL11,  9, 3},
	{ 44, 3, PAD_FUNC_SEL11, 12, 0},
	{ 46, 3, PAD_FUNC_SEL11, 12, 1},
	{ 25, 1, PAD_FUNC_SEL11, 15, 0},
	{ 69, 1, PAD_FUNC_SEL11, 15, 1},
	{147, 2, PAD_FUNC_SEL11, 15, 2},
	{157, 1, PAD_FUNC_SEL11, 15, 3},
	{ 71, 3, PAD_FUNC_SEL11, 18, 0},
	{ 78, 6, PAD_FUNC_SEL11, 18, 1},
	{183, 3, PAD_FUNC_SEL11, 18, 2},
	{ 71, 3, PAD_FUNC_SEL11, 18, 3},
	{ 73, 3, PAD_FUNC_SEL11, 21, 0},
	{ 80, 6, PAD_FUNC_SEL11, 21, 1},
	{185, 3, PAD_FUNC_SEL11, 21, 2},
	{ 73, 3, PAD_FUNC_SEL11, 21, 3},
	{ 43, 3, PAD_FUNC_SEL11, 24, 0},
	{ 45, 3, PAD_FUNC_SEL11, 24, 1},
	{ 26, 1, PAD_FUNC_SEL11, 27, 0},
	{ 70, 1, PAD_FUNC_SEL11, 27, 1},
	{148, 2, PAD_FUNC_SEL11, 27, 2},
	{158, 1, PAD_FUNC_SEL11, 27, 3},
	{ 63, 1, PAD_FUNC_SEL12,  0, 0},
	{137, 2, PAD_FUNC_SEL12,  0, 1},
	{ 24, 1, PAD_FUNC_SEL12,  3, 0},
	{ 68, 1, PAD_FUNC_SEL12,  3, 1},
	{146, 2, PAD_FUNC_SEL12,  3, 2},
	{156, 1, PAD_FUNC_SEL12,  3, 3},
	{ 92, 1, PAD_FUNC_SEL12,  6, 0},
	{126, 2, PAD_FUNC_SEL12,  6, 1},
	{141, 4, PAD_FUNC_SEL12,  6, 2},
	{ 92, 1, PAD_FUNC_SEL12,  6, 3},
	{ 96, 1, PAD_FUNC_SEL12,  9, 0},
	{130, 2, PAD_FUNC_SEL12,  9, 1},
	{ 83, 2, PAD_FUNC_SEL12, 12, 0},
	{ 90, 1, PAD_FUNC_SEL12, 12, 1},
	{124, 2, PAD_FUNC_SEL12, 12, 2},
	{ 83, 2, PAD_FUNC_SEL12, 12, 3},
	{  4, 5, PAD_FUNC_SEL12, 15, 0},
	{ 20, 4, PAD_FUNC_SEL12, 15, 1},
	{148, 3, PAD_FUNC_SEL12, 15, 2},
	{  4, 5, PAD_FUNC_SEL12, 15, 3},
	{ 13, 5, PAD_FUNC_SEL12, 18, 0},
	{ 19, 4, PAD_FUNC_SEL12, 18, 1},
	{147, 3, PAD_FUNC_SEL12, 18, 2},
	{ 13, 5, PAD_FUNC_SEL12, 18, 3},
	{ 16, 5, PAD_FUNC_SEL12, 21, 0},
	{ 18, 4, PAD_FUNC_SEL12, 21, 1},
	{146, 3, PAD_FUNC_SEL12, 21, 2},
	{ 16, 5, PAD_FUNC_SEL12, 21, 3},
	{ 94, 1, PAD_FUNC_SEL12, 24, 0},
	{128, 2, PAD_FUNC_SEL12, 24, 1},
	{ 10, 4, PAD_FUNC_SEL12, 27, 0},
	{ 97, 1, PAD_FUNC_SEL12, 27, 1},
	{131, 2, PAD_FUNC_SEL12, 27, 2},
	{ 10, 4, PAD_FUNC_SEL12, 27, 3},
	{ 34, 6, PAD_FUNC_SEL13,  0, 0},
	{ 75, 3, PAD_FUNC_SEL13,  0, 1},
	{ 82, 6, PAD_FUNC_SEL13,  0, 2},
	{144, 3, PAD_FUNC_SEL13,  0, 3},
	{ 36, 6, PAD_FUNC_SEL13,  3, 0},
	{ 84, 6, PAD_FUNC_SEL13,  3, 1},
	{ 38, 6, PAD_FUNC_SEL13,  6, 0},
	{ 86, 6, PAD_FUNC_SEL13,  6, 1},
	{ 91, 1, PAD_FUNC_SEL13,  9, 0},
	{125, 2, PAD_FUNC_SEL13,  9, 1},
	{140, 4, PAD_FUNC_SEL13,  9, 2},
	{149, 3, PAD_FUNC_SEL13,  9, 3},
	{  5, 5, PAD_FUNC_SEL13, 12, 0},
	{ 94, 3, PAD_FUNC_SEL13, 12, 1},
	{152, 3, PAD_FUNC_SEL13, 12, 2},
	{  5, 5, PAD_FUNC_SEL13, 12, 3},
	{ 14, 5, PAD_FUNC_SEL13, 15, 0},
	{ 93, 3, PAD_FUNC_SEL13, 15, 1},
	{151, 3, PAD_FUNC_SEL13, 15, 2},
	{ 14, 5, PAD_FUNC_SEL13, 15, 3},
	{ 17, 5, PAD_FUNC_SEL13, 18, 0},
	{ 21, 4, PAD_FUNC_SEL13, 18, 1},
	{150, 3, PAD_FUNC_SEL13, 18, 2},
	{ 17, 5, PAD_FUNC_SEL13, 18, 3},
	{ 95, 1, PAD_FUNC_SEL13, 21, 0},
	{129, 2, PAD_FUNC_SEL13, 21, 1},
	{ 11, 4, PAD_FUNC_SEL13, 24, 0},
	{ 98, 1, PAD_FUNC_SEL13, 24, 1},
	{132, 2, PAD_FUNC_SEL13, 24, 2},
	{ 11, 4, PAD_FUNC_SEL13, 24, 3},
	{ 35, 6, PAD_FUNC_SEL13, 27, 0},
	{ 76, 3, PAD_FUNC_SEL13, 27, 1},
	{ 83, 6, PAD_FUNC_SEL13, 27, 2},
	{145, 3, PAD_FUNC_SEL13, 27, 3},
	{ 37, 6, PAD_FUNC_SEL14,  0, 0},
	{ 85, 6, PAD_FUNC_SEL14,  0, 1},
	{ 39, 6, PAD_FUNC_SEL14,  3, 0},
	{ 87, 6, PAD_FUNC_SEL14,  3, 1},
	{ 82, 2, PAD_FUNC_SEL14,  6, 0},
	{ 89, 1, PAD_FUNC_SEL14,  6, 1},
	{123, 2, PAD_FUNC_SEL14,  6, 2},
	{ 82, 2, PAD_FUNC_SEL14,  6, 3},
	{ 93, 1, PAD_FUNC_SEL14,  9, 0},
	{127, 2, PAD_FUNC_SEL14,  9, 1},
	{ 64, 0, PAD_FUNC_SEL14, 12, 0},
	{ 73, 1, PAD_FUNC_SEL14, 12, 1},
	{ 65, 0, PAD_FUNC_SEL14, 15, 0},
	{ 74, 1, PAD_FUNC_SEL14, 15, 1},
	{192, 0, PAD_FUNC_SEL14, 18, 0},
	{ 75, 1, PAD_FUNC_SEL14, 18, 1},
	{ 63, 0, PAD_FUNC_SEL14, 21, 0},
	{ 72, 1, PAD_FUNC_SEL14, 21, 1},
	{ 62, 0, PAD_FUNC_SEL14, 24, 0},
	{ 71, 1, PAD_FUNC_SEL14, 24, 1},
	{ 56, 4, PAD_FUNC_SEL14, 27, 0},
	{ 58, 3, PAD_FUNC_SEL14, 27, 1},
	{ 57, 4, PAD_FUNC_SEL15,  0, 0},
	{ 59, 3, PAD_FUNC_SEL15,  0, 1},
	{ 58, 4, PAD_FUNC_SEL15,  3, 0},
	{ 60, 3, PAD_FUNC_SEL15,  3, 1},
	{ 59, 4, PAD_FUNC_SEL15,  6, 0},
	{ 61, 3, PAD_FUNC_SEL15,  6, 1},
	{ 60, 4, PAD_FUNC_SEL15,  9, 0},
	{177, 5, PAD_FUNC_SEL15,  9, 1},
	{ 61, 4, PAD_FUNC_SEL15, 12, 0},
	{178, 5, PAD_FUNC_SEL15, 12, 1},
	{ 48, 4, PAD_FUNC_SEL15, 15, 0},
	{ 50, 3, PAD_FUNC_SEL15, 15, 1},
	{ 49, 4, PAD_FUNC_SEL15, 18, 0},
	{ 51, 3, PAD_FUNC_SEL15, 18, 1},
	{ 50, 4, PAD_FUNC_SEL15, 21, 0},
	{ 52, 3, PAD_FUNC_SEL15, 21, 1},
	{ 51, 4, PAD_FUNC_SEL15, 24, 0},
	{ 53, 3, PAD_FUNC_SEL15, 24, 1},
	{ 52, 4, PAD_FUNC_SEL15, 27, 0},
	{ 54, 3, PAD_FUNC_SEL15, 27, 1},
	{ 53, 4, PAD_FUNC_SEL16,  0, 0},
	{ 55, 3, PAD_FUNC_SEL16,  0, 1},
	{ 54, 4, PAD_FUNC_SEL16,  3, 0},
	{ 56, 3, PAD_FUNC_SEL16,  3, 1},
	{ 55, 4, PAD_FUNC_SEL16,  6, 0},
	{ 57, 3, PAD_FUNC_SEL16,  6, 1},
	{ 30, 3, PAD_FUNC_SEL16,  9, 0},
	{144, 6, PAD_FUNC_SEL16,  9, 1},
	{ 31, 3, PAD_FUNC_SEL16, 12, 0},
	{131, 6, PAD_FUNC_SEL16, 12, 1},
	{ 32, 3, PAD_FUNC_SEL16, 15, 0},
	{132, 6, PAD_FUNC_SEL16, 15, 1},
	{ 33, 3, PAD_FUNC_SEL16, 18, 0},
	{133, 6, PAD_FUNC_SEL16, 18, 1},
	{ 34, 3, PAD_FUNC_SEL16, 21, 0},
	{134, 6, PAD_FUNC_SEL16, 21, 1},
	{ 35, 3, PAD_FUNC_SEL16, 24, 0},
	{135, 6, PAD_FUNC_SEL16, 24, 1},
	{ 36, 3, PAD_FUNC_SEL16, 27, 0},
	{136, 6, PAD_FUNC_SEL16, 27, 1},
	{ 37, 3, PAD_FUNC_SEL17,  0, 0},
	{137, 6, PAD_FUNC_SEL17,  0, 1},
	{ 38, 3, PAD_FUNC_SEL17,  3, 0},
	{138, 6, PAD_FUNC_SEL17,  3, 1},
	{ 13, 4, PAD_FUNC_SEL17,  6, 0},
	{143, 3, PAD_FUNC_SEL17,  6, 1},
	{153, 3, PAD_FUNC_SEL17,  6, 2},
	{ 13, 4, PAD_FUNC_SEL17,  6, 3},
	{ 29, 3, PAD_FUNC_SEL17,  9, 0},
	{ 62, 3, PAD_FUNC_SEL17,  9, 1},
	{ 77, 5, PAD_FUNC_SEL17,  9, 2},
	{ 84, 2, PAD_FUNC_SEL17,  9, 3},
	{ 28, 3, PAD_FUNC_SEL17, 12, 0},
	{ 62, 1, PAD_FUNC_SEL17, 12, 1},
	{ 76, 5, PAD_FUNC_SEL17, 12, 2},
	{122, 1, PAD_FUNC_SEL17, 12, 3},
	{140, 3, PAD_FUNC_SEL17, 12, 4},
	{145, 4, PAD_FUNC_SEL17, 12, 5},
	{ 28, 3, PAD_FUNC_SEL17, 12, 6},
	{ 28, 3, PAD_FUNC_SEL17, 12, 7},
	{  4, 4, PAD_FUNC_SEL17, 15, 0},
	{147, 4, PAD_FUNC_SEL17, 15, 1},
	{  3, 4, PAD_FUNC_SEL17, 18, 0},
	{ 74, 6, PAD_FUNC_SEL17, 18, 1},
	{146, 4, PAD_FUNC_SEL17, 18, 2},
	{  3, 4, PAD_FUNC_SEL17, 18, 3},
	{ 25, 0, PAD_FUNC_SEL17, 21, 0},
	{131, 3, PAD_FUNC_SEL17, 21, 1},
	{ 26, 0, PAD_FUNC_SEL17, 24, 0},
	{132, 3, PAD_FUNC_SEL17, 24, 1},
	{ 23, 0, PAD_FUNC_SEL17, 27, 0},
	{129, 3, PAD_FUNC_SEL17, 27, 1},
	{ 24, 0, PAD_FUNC_SEL18,  0, 0},
	{130, 3, PAD_FUNC_SEL18,  0, 1},
	{  9, 3, PAD_FUNC_SEL18,  3, 0},
	{ 29, 4, PAD_FUNC_SEL18,  3, 1},
	{ 57, 1, PAD_FUNC_SEL18,  3, 2},
	{ 74, 5, PAD_FUNC_SEL18,  3, 3},
	{ 88, 3, PAD_FUNC_SEL18,  3, 4},
	{  9, 3, PAD_FUNC_SEL18,  3, 5},
	{  9, 3, PAD_FUNC_SEL18,  3, 6},
	{  9, 3, PAD_FUNC_SEL18,  3, 7},
	{ 55, 0, PAD_FUNC_SEL18,  6, 0},
	{  6, 3, PAD_FUNC_SEL18,  6, 1},
	{ 26, 4, PAD_FUNC_SEL18,  6, 2},
	{ 71, 5, PAD_FUNC_SEL18,  6, 3},
	{ 85, 3, PAD_FUNC_SEL18,  6, 4},
	{ 55, 0, PAD_FUNC_SEL18,  6, 5},
	{ 55, 0, PAD_FUNC_SEL18,  6, 6},
	{ 55, 0, PAD_FUNC_SEL18,  6, 7},
	{  7, 3, PAD_FUNC_SEL18,  9, 0},
	{ 27, 4, PAD_FUNC_SEL18,  9, 1},
	{ 59, 1, PAD_FUNC_SEL18,  9, 2},
	{ 72, 5, PAD_FUNC_SEL18,  9, 3},
	{ 86, 3, PAD_FUNC_SEL18,  9, 4},
	{  7, 3, PAD_FUNC_SEL18,  9, 5},
	{  7, 3, PAD_FUNC_SEL18,  9, 6},
	{  7, 3, PAD_FUNC_SEL18,  9, 7},
	{  8, 3, PAD_FUNC_SEL18, 12, 0},
	{ 28, 4, PAD_FUNC_SEL18, 12, 1},
	{ 56, 1, PAD_FUNC_SEL18, 12, 2},
	{ 73, 5, PAD_FUNC_SEL18, 12, 3},
	{ 87, 3, PAD_FUNC_SEL18, 12, 4},
	{  8, 3, PAD_FUNC_SEL18, 12, 5},
	{  8, 3, PAD_FUNC_SEL18, 12, 6},
	{  8, 3, PAD_FUNC_SEL18, 12, 7},
	{ 17, 4, PAD_FUNC_SEL18, 15, 0},
	{ 33, 4, PAD_FUNC_SEL18, 15, 1},
	{141, 1, PAD_FUNC_SEL18, 15, 2},
	{187, 1, PAD_FUNC_SEL18, 15, 3},
	{ 22, 4, PAD_FUNC_SEL18, 18, 0},
	{ 34, 4, PAD_FUNC_SEL18, 18, 1},
	{ 93, 4, PAD_FUNC_SEL18, 18, 2},
	{ 22, 4, PAD_FUNC_SEL18, 18, 3},
	{ 23, 4, PAD_FUNC_SEL18, 21, 0},
	{ 35, 4, PAD_FUNC_SEL18, 21, 1},
	{ 94, 4, PAD_FUNC_SEL18, 21, 2},
	{ 23, 4, PAD_FUNC_SEL18, 21, 3},
	{ 24, 4, PAD_FUNC_SEL18, 24, 0},
	{ 36, 4, PAD_FUNC_SEL18, 24, 1},
	{ 95, 4, PAD_FUNC_SEL18, 24, 2},
	{ 24, 4, PAD_FUNC_SEL18, 24, 3},
	{ 25, 4, PAD_FUNC_SEL18, 27, 0},
	{ 37, 4, PAD_FUNC_SEL18, 27, 1},
	{ 96, 4, PAD_FUNC_SEL18, 27, 2},
	{ 25, 4, PAD_FUNC_SEL18, 27, 3},
	{188, 0, PAD_FUNC_SEL19,  0, 0},
	{ 14, 4, PAD_FUNC_SEL19,  0, 1},
	{ 30, 4, PAD_FUNC_SEL19,  0, 2},
	{142, 1, PAD_FUNC_SEL19,  0, 3},
	{ 15, 4, PAD_FUNC_SEL19,  3, 0},
	{ 31, 4, PAD_FUNC_SEL19,  3, 1},
	{143, 1, PAD_FUNC_SEL19,  3, 2},
	{189, 1, PAD_FUNC_SEL19,  3, 3},
	{ 16, 4, PAD_FUNC_SEL19,  6, 0},
	{ 32, 4, PAD_FUNC_SEL19,  6, 1},
	{140, 1, PAD_FUNC_SEL19,  6, 2},
	{186, 1, PAD_FUNC_SEL19,  6, 3},
	{ 21, 1, PAD_FUNC_SEL19,  9, 0},
	{ 45, 4, PAD_FUNC_SEL19,  9, 1},
	{ 18, 1, PAD_FUNC_SEL19, 12, 0},
	{ 42, 4, PAD_FUNC_SEL19, 12, 1},
	{ 19, 1, PAD_FUNC_SEL19, 15, 0},
	{ 43, 4, PAD_FUNC_SEL19, 15, 1},
	{ 20, 1, PAD_FUNC_SEL19, 18, 0},
	{ 44, 4, PAD_FUNC_SEL19, 18, 1},
	{ 13, 1, PAD_FUNC_SEL19, 21, 0},
	{ 41, 4, PAD_FUNC_SEL19, 21, 1},
	{ 10, 1, PAD_FUNC_SEL19, 24, 0},
	{ 38, 4, PAD_FUNC_SEL19, 24, 1},
	{ 11, 1, PAD_FUNC_SEL19, 27, 0},
	{ 39, 4, PAD_FUNC_SEL19, 27, 1},
	{ 12, 1, PAD_FUNC_SEL20,  0, 0},
	{ 40, 4, PAD_FUNC_SEL20,  0, 1},
	{  6, 4, PAD_FUNC_SEL20,  3, 0},
	{ 30, 1, PAD_FUNC_SEL20,  3, 1},
	{ 45, 1, PAD_FUNC_SEL20,  3, 2},
	{ 66, 3, PAD_FUNC_SEL20,  3, 3},
	{ 78, 1, PAD_FUNC_SEL20,  3, 4},
	{ 97, 3, PAD_FUNC_SEL20,  3, 5},
	{112, 6, PAD_FUNC_SEL20,  3, 6},
	{137, 1, PAD_FUNC_SEL20,  3, 7},
	{  7, 4, PAD_FUNC_SEL20,  6, 0},
	{ 31, 1, PAD_FUNC_SEL20,  6, 1},
	{ 46, 1, PAD_FUNC_SEL20,  6, 2},
	{ 67, 3, PAD_FUNC_SEL20,  6, 3},
	{ 79, 1, PAD_FUNC_SEL20,  6, 4},
	{ 98, 3, PAD_FUNC_SEL20,  6, 5},
	{113, 6, PAD_FUNC_SEL20,  6, 6},
	{138, 1, PAD_FUNC_SEL20,  6, 7},
	{  8, 4, PAD_FUNC_SEL20,  9, 0},
	{ 24, 6, PAD_FUNC_SEL20,  9, 1},
	{ 32, 1, PAD_FUNC_SEL20,  9, 2},
	{ 47, 1, PAD_FUNC_SEL20,  9, 3},
	{ 68, 3, PAD_FUNC_SEL20,  9, 4},
	{ 80, 1, PAD_FUNC_SEL20,  9, 5},
	{ 99, 3, PAD_FUNC_SEL20,  9, 6},
	{114, 6, PAD_FUNC_SEL20,  9, 7},
	{ 33, 1, PAD_FUNC_SEL20, 12, 0},
	{ 69, 3, PAD_FUNC_SEL20, 12, 1},
	{ 81, 1, PAD_FUNC_SEL20, 12, 2},
	{100, 3, PAD_FUNC_SEL20, 12, 3},
	{115, 6, PAD_FUNC_SEL20, 12, 4},
	{143, 6, PAD_FUNC_SEL20, 12, 5},
	{159, 3, PAD_FUNC_SEL20, 12, 6},
	{182, 3, PAD_FUNC_SEL20, 12, 7},
	{111, 3, PAD_FUNC_SEL20, 15, 0},
	{120, 3, PAD_FUNC_SEL20, 15, 1},
	{144, 5, PAD_FUNC_SEL20, 15, 2},
	{153, 5, PAD_FUNC_SEL20, 15, 3},
	{119, 3, PAD_FUNC_SEL20, 18, 0},
	{128, 3, PAD_FUNC_SEL20, 18, 1},
	{152, 5, PAD_FUNC_SEL20, 18, 2},
	{119, 3, PAD_FUNC_SEL20, 18, 3},
	{118, 3, PAD_FUNC_SEL20, 21, 0},
	{127, 3, PAD_FUNC_SEL20, 21, 1},
	{151, 5, PAD_FUNC_SEL20, 21, 2},
	{118, 3, PAD_FUNC_SEL20, 21, 3},
	{117, 3, PAD_FUNC_SEL20, 24, 0},
	{126, 3, PAD_FUNC_SEL20, 24, 1},
	{150, 5, PAD_FUNC_SEL20, 24, 2},
	{117, 3, PAD_FUNC_SEL20, 24, 3},
	{116, 3, PAD_FUNC_SEL20, 27, 0},
	{125, 3, PAD_FUNC_SEL20, 27, 1},
	{149, 5, PAD_FUNC_SEL20, 27, 2},
	{116, 3, PAD_FUNC_SEL20, 27, 3},
	{115, 3, PAD_FUNC_SEL21,  0, 0},
	{124, 3, PAD_FUNC_SEL21,  0, 1},
	{148, 5, PAD_FUNC_SEL21,  0, 2},
	{115, 3, PAD_FUNC_SEL21,  0, 3},
	{114, 3, PAD_FUNC_SEL21,  3, 0},
	{123, 3, PAD_FUNC_SEL21,  3, 1},
	{147, 5, PAD_FUNC_SEL21,  3, 2},
	{114, 3, PAD_FUNC_SEL21,  3, 3},
	{113, 3, PAD_FUNC_SEL21,  6, 0},
	{122, 3, PAD_FUNC_SEL21,  6, 1},
	{146, 5, PAD_FUNC_SEL21,  6, 2},
	{155, 5, PAD_FUNC_SEL21,  6, 3},
	{112, 3, PAD_FUNC_SEL21,  9, 0},
	{121, 3, PAD_FUNC_SEL21,  9, 1},
	{145, 5, PAD_FUNC_SEL21,  9, 2},
	{154, 5, PAD_FUNC_SEL21,  9, 3},
	{167, 4, PAD_FUNC_SEL21, 12, 0},
	{171, 3, PAD_FUNC_SEL21, 12, 1},
	{  6, 5, PAD_FUNC_SEL21, 15, 0},
	{ 60, 6, PAD_FUNC_SEL21, 15, 1},
	{ 47, 3, PAD_FUNC_SEL21, 18, 0},
	{133, 3, PAD_FUNC_SEL21, 18, 1},
	{  8, 5, PAD_FUNC_SEL21, 21, 0},
	{ 54, 6, PAD_FUNC_SEL21, 21, 1},
	{ 56, 6, PAD_FUNC_SEL21, 24, 0},
	{135, 3, PAD_FUNC_SEL21, 24, 1},
	{ 58, 6, PAD_FUNC_SEL21, 27, 0},
	{137, 3, PAD_FUNC_SEL21, 27, 1},
	{  7, 5, PAD_FUNC_SEL22,  0, 0},
	{ 61, 6, PAD_FUNC_SEL22,  0, 1},
	{ 48, 3, PAD_FUNC_SEL22,  3, 0},
	{134, 3, PAD_FUNC_SEL22,  3, 1},
	{  9, 5, PAD_FUNC_SEL22,  6, 0},
	{ 55, 6, PAD_FUNC_SEL22,  6, 1},
	{ 57, 6, PAD_FUNC_SEL22,  9, 0},
	{136, 3, PAD_FUNC_SEL22,  9, 1},
	{ 59, 6, PAD_FUNC_SEL22, 12, 0},
	{138, 3, PAD_FUNC_SEL22, 12, 1},
	{ 40, 3, PAD_FUNC_SEL22, 15, 0},
	{ 76, 1, PAD_FUNC_SEL22, 15, 1},
	{ 95, 3, PAD_FUNC_SEL22, 15, 2},
	{149, 2, PAD_FUNC_SEL22, 15, 3},
	{  4, 1, PAD_FUNC_SEL22, 18, 0},
	{ 10, 6, PAD_FUNC_SEL22, 18, 1},
	{ 16, 3, PAD_FUNC_SEL22, 18, 2},
	{ 22, 5, PAD_FUNC_SEL22, 18, 3},
	{ 97, 5, PAD_FUNC_SEL22, 18, 4},
	{161, 2, PAD_FUNC_SEL22, 18, 5},
	{  4, 1, PAD_FUNC_SEL22, 18, 6},
	{  4, 1, PAD_FUNC_SEL22, 18, 7},
	{  2, 1, PAD_FUNC_SEL22, 21, 0},
	{ 12, 6, PAD_FUNC_SEL22, 21, 1},
	{ 18, 3, PAD_FUNC_SEL22, 21, 2},
	{ 24, 5, PAD_FUNC_SEL22, 21, 3},
	{ 99, 5, PAD_FUNC_SEL22, 21, 4},
	{123, 1, PAD_FUNC_SEL22, 21, 5},
	{163, 2, PAD_FUNC_SEL22, 21, 6},
	{  2, 1, PAD_FUNC_SEL22, 21, 7},
	{ 20, 3, PAD_FUNC_SEL22, 24, 0},
	{ 93, 5, PAD_FUNC_SEL22, 24, 1},
	{165, 2, PAD_FUNC_SEL22, 24, 2},
	{ 20, 3, PAD_FUNC_SEL22, 24, 3},
	{ 41, 3, PAD_FUNC_SEL22, 27, 0},
	{ 77, 1, PAD_FUNC_SEL22, 27, 1},
	{ 96, 3, PAD_FUNC_SEL22, 27, 2},
	{150, 2, PAD_FUNC_SEL22, 27, 3},
	{  3, 1, PAD_FUNC_SEL23,  0, 0},
	{ 11, 6, PAD_FUNC_SEL23,  0, 1},
	{ 17, 3, PAD_FUNC_SEL23,  0, 2},
	{ 23, 5, PAD_FUNC_SEL23,  0, 3},
	{ 98, 5, PAD_FUNC_SEL23,  0, 4},
	{  3, 1, PAD_FUNC_SEL23,  0, 5},
	{  3, 1, PAD_FUNC_SEL23,  0, 6},
	{  3, 1, PAD_FUNC_SEL23,  0, 7},
	{  5, 1, PAD_FUNC_SEL23,  3, 0},
	{ 13, 6, PAD_FUNC_SEL23,  3, 1},
	{ 19, 3, PAD_FUNC_SEL23,  3, 2},
	{ 25, 5, PAD_FUNC_SEL23,  3, 3},
	{100, 5, PAD_FUNC_SEL23,  3, 4},
	{124, 1, PAD_FUNC_SEL23,  3, 5},
	{164, 2, PAD_FUNC_SEL23,  3, 6},
	{  5, 1, PAD_FUNC_SEL23,  3, 7},
	{ 21, 3, PAD_FUNC_SEL23,  6, 0},
	{ 94, 5, PAD_FUNC_SEL23,  6, 1},
	{166, 2, PAD_FUNC_SEL23,  6, 2},
	{ 21, 3, PAD_FUNC_SEL23,  6, 3},
	{ 33, 5, PAD_FUNC_SEL23,  9, 0},
	{ 49, 3, PAD_FUNC_SEL23,  9, 1},
	{161, 3, PAD_FUNC_SEL23,  9, 2},
	{ 33, 5, PAD_FUNC_SEL23,  9, 3},
	{ 34, 5, PAD_FUNC_SEL23, 12, 0},
	{ 78, 4, PAD_FUNC_SEL23, 12, 1},
	{142, 3, PAD_FUNC_SEL23, 12, 2},
	{ 34, 5, PAD_FUNC_SEL23, 12, 3},
	{ 79, 4, PAD_FUNC_SEL23, 15, 0},
	{141, 3, PAD_FUNC_SEL23, 15, 1},
	{154, 3, PAD_FUNC_SEL23, 15, 2},
	{ 79, 4, PAD_FUNC_SEL23, 15, 3}
};

static int sca200v100_pinctrl_pb_msc_config(struct udevice *dev)
{
	struct sca200v100_pinctrl_priv *priv = NULL;
	void __iomem *pb_msc_addr = NULL;
	u32 size = 0;
	u32 pb_num = 0;
	u32 *pb_info = NULL;
	int ret = 0;
	u32 i = 0;
	u32 reg_bit = 0;
	u32 config = 0;
	u32 reg_val = PB_MSC_RESERVED_MASK;

	priv = dev_get_priv(dev);
	pb_msc_addr = priv->pb_sw_reg_base + PB_MSC_REG0_OFFSET;
	size = dev_read_size(dev, "pb-msc-cfgs");
	if (size < 0) {
		return 0;
	}

	pb_num = size / sizeof(u32);
	pb_info = calloc(pb_num, sizeof(u32));
	if (!pb_info) {
		return -ENOMEM;
	}

	ret = dev_read_u32_array(dev, "pb-msc-cfgs", pb_info, pb_num);
	if (ret) {
		return -EINVAL;
	}

	for (i = 0; i < pb_num; i++) {
		reg_bit = pb_info[i] & PB_MSC_BIT_MASK;
		config = (pb_info[i] & PB_MSC_CONFIG_MASK) >> PB_MSC_SHIFT;
		reg_val |= (config << reg_bit);
	}
	writel(reg_val, pb_msc_addr);

	free(pb_info);

	return 0;
}

static int sca200v100_pinctl_set(struct udevice *dev, u32 pad, u32 config)
{
	struct sca200v100_pinctrl_priv *priv = dev_get_priv(dev);
	u32 reg_tmp;
	u32 j;
	u32 pad_sels_num;

	/* 1. set pin's func */
	reg_tmp = readl(priv->pb_sw_reg_base + pad / 3 * 4);
	reg_tmp &= ~(REG_CONFIG_ONEPAD_MASK << ((pad % 3) *
				REG_CONFIG_ONEPAD_WIDTH));
	reg_tmp |= config << ((pad % 3) * REG_CONFIG_ONEPAD_WIDTH);
	writel(reg_tmp, priv->pb_sw_reg_base + pad / 3 * 4);

	/* 2. set func's pin */
	pad_sels_num = sizeof(pad_sels) / sizeof(struct pad_func_sel);
	for (j = 0; j < pad_sels_num; j++) {
		if ((pad == pad_sels[j].pad_index) &&
			((config & DT_MODE_MASK) == pad_sels[j].mode)) {
			reg_tmp = readl(priv->pad_func_sel_base +
					pad_sels[j].offset);
			reg_tmp &= ~(REG_FUNCSEL_ONEPAD_MASK <<
					pad_sels[j].shift);
			reg_tmp |= pad_sels[j].val << pad_sels[j].shift;
			writel(reg_tmp, priv->pad_func_sel_base +
				pad_sels[j].offset);
		}
	}

	return 0;
}

static int sca200v100_pinctrl_set_state(struct udevice *dev, struct udevice *configdev)
{
	u32 i;
	int pads_num;
	const u32 *pads_info;
	u32 data;
	u32 pad;
	u32 config;

	debug("%s: dev %s set pinctrl\n", __func__, configdev->name);
	pads_info = dev_read_prop(configdev, "pinctrl-cfgs", &pads_num);
	if (!pads_info || (pads_num < 0)) {
		pr_err("%s: read pinctrl cfgs failed\n", __func__);
		return -EINVAL;
	}

	pads_num /= sizeof(u32);
	for (i = 0; i < pads_num; i++) {
		data = fdt32_to_cpu(pads_info[i]);
		pad = (data & DT_PAD_MASK) >> DT_PAD_SHIFT;
		config = (data & DT_CONFIG_MASK) >> DT_CONFIG_SHIFT;
		sca200v100_pinctl_set(dev, pad, config);
	}

	return 0;
}

static int sca200v100_pinctrl_probe(struct udevice *dev)
{
	struct sca200v100_pinctrl_priv *priv = dev_get_priv(dev);
	int ret = 0;

#define PINCTRL_BASE_NUM    4
	unsigned int tmp[PINCTRL_BASE_NUM];

	ret = dev_read_u32_array(dev, "reg", tmp, PINCTRL_BASE_NUM);
	if (ret) {
		pr_err("%s: read pinctrl base address failed %d\n", __func__, ret);
		return -EINVAL;
	}

	priv->pb_sw_reg_base = (void *)(ulong)tmp[0];
	priv->pad_func_sel_base = (void *)(ulong)tmp[2];
	priv->p_pad_sels = pad_sels;

	ret = sca200v100_pinctrl_set_state(dev, dev);
	if(ret)
		return ret;

	ret = sca200v100_pinctrl_pb_msc_config(dev);
	return ret;
}

const struct pinctrl_ops single_pinctrl_ops = {
	.set_state = sca200v100_pinctrl_set_state,
};

static const struct udevice_id sca200v100_pinctrl_match[] = {
	{ .compatible = "smartchip,sca200v100-pinctrl" },
	{ /* sentinel */ }
};

U_BOOT_DRIVER(sca200v100_pinctrl) = {
	.name = "sca200v100_pinctrl",
	.id = UCLASS_PINCTRL,
	.of_match = sca200v100_pinctrl_match,
	.priv_auto_alloc_size = sizeof(struct sca200v100_pinctrl_priv),
	.ops = &single_pinctrl_ops,
#if !CONFIG_IS_ENABLED(OF_PLATDATA)
	.bind       = dm_scan_fdt_dev,
#endif
	.probe      = sca200v100_pinctrl_probe,
};
