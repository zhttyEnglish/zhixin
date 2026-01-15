/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Header providing constants for SmartChip pinctrl bindings.
 *
 * Copyright (c) 2022 SmartChip
 */

#include <common.h>
#include <dm.h>
#include <dm/device_compat.h>
#include <dm/pinctrl.h>
#include <linux/libfdt.h>
#include <asm/io.h>
#include <regmap.h>

#define DT_CONFIG_MASK          0x0000ffff
#define DT_MODE_MASK            0x00000007
#define DT_PAD_MASK         0xffff0000
#define DT_CONFIG_SHIFT         3
#define DT_PAD_SHIFT            16

#define REG_CONFIG_ONEPAD_MASK      0x000000ff
#define REG_CONFIG_ONEPAD_WIDTH     8

#define REG_FUNCSEL_ONEPAD_MASK     0x00000007
#define REG_FUNCSEL_ONEPAD_WIDTH    4

#define PB_MSC_REG0_OFFSET      0x00000000
#define PB_MSC_SW_OFFSET        0x00000004
#define PB_MSC_SEL_OFFSET       0x0000007c
#define PB_MSC_WDT_OFFSET       0x00000138

#define PB_MSC_SHIFT            16
#define PB_MSC_BIT_MASK         GENMASK(PB_MSC_SHIFT-1, 0)
#define PB_MSC_CONFIG_MASK      GENMASK(PB_MSC_SHIFT+1, PB_MSC_SHIFT)
#define PB_MSC_RESERVED_MASK        GENMASK(31, 11)

#define WDT_PAD_0           (10)
#define WDT_PAD_1           (22)
#define WDT_PAD_2           (61)

#define WDT_PAD_0_MODE          (3)
#define WDT_PAD_1_MODE          (4)
#define WDT_PAD_2_MODE          (2)

struct sca200v200_pin_info {
	unsigned char pin_num;
	unsigned int select_offset;
	unsigned int config_offset;
};

struct sca200v200_pad_info {
	unsigned char pin_index;
	unsigned int select_offset;
	unsigned int config_offset;
};

struct sca200v200_pin_info group_info[] = {
	{23, 0x7c, 0x04},
	{22, 0x88, 0x1c},
	{26, 0x94, 0x34},
	{6,  0xa4, 0x50},
	{6,  0xa8, 0x58},
	{11, 0xac, 0x60},
	{16, 0xb4, 0x6c}
};

struct sca200v200_pinctrl_priv {
	void __iomem *pb_sw_reg_base;
};

#ifndef CONFIG_SPL_BUILD
static int sca200v200_pinctrl_pb_msc_config(struct udevice *dev)
{
	struct sca200v200_pinctrl_priv *priv = NULL;
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
#endif

static int sca200v200_pinctrl_probe(struct udevice *dev)
{
	struct sca200v200_pinctrl_priv *priv = dev_get_priv(dev);
	u32 size, i, j;
	u32 pads_num;
	u32 *pads_info = NULL;
	u32 pad;
	u32 sel;
	u32 config;
	u32 reg_tmp;
	//u32 pad_sels_num;
	int ret = 0;
	struct sca200v200_pad_info pad_info[110];

#define PINCTRL_BASE_NUM    2
	unsigned int tmp[PINCTRL_BASE_NUM];
	u32 idx = 0;
	for(i = 0; i < (sizeof(group_info) / sizeof(struct sca200v200_pin_info)); ++i) {
		for(j = 0; j < group_info[i].pin_num; ++j) {
			//printf("%d %d %x %x\n", i, j, group_info[i].select_offset, group_info[i].config_offset);
			pad_info[idx].pin_index = j;
			pad_info[idx].select_offset = group_info[i].select_offset;
			pad_info[idx].config_offset = group_info[i].config_offset;
			idx++;
		}
	}

	ret = dev_read_u32_array(dev, "reg", (unsigned int *)tmp, PINCTRL_BASE_NUM);
	if (ret) {
		pr_err("%s: read pinctrl base address failed %d\n", __func__, ret);
		return -EINVAL;
	}

	priv->pb_sw_reg_base = (void *)(unsigned long)tmp[0];

	size = dev_read_size(dev, "pinctrl-cfgs");
	if (size < 0)
		return 0;

	pads_num = size / sizeof(u32);
	pads_info = calloc(pads_num, sizeof(u32));
	if (!pads_info)
		return -ENOMEM;

	ret = dev_read_u32_array(dev, "pinctrl-cfgs", pads_info, pads_num);
	if (ret)
		return -EINVAL;

#ifndef CONFIG_SPL_BUILD
	ret = sca200v200_pinctrl_pb_msc_config(dev);
#endif

	for (i = 0; i < pads_num; i++) {
		pad = (pads_info[i] & DT_PAD_MASK) >> DT_PAD_SHIFT;
		config = (pads_info[i] & DT_CONFIG_MASK) >> DT_CONFIG_SHIFT;
		sel = (pads_info[i] & DT_MODE_MASK);

		//printf("%s %d pad num %d sel %d conf %x\n", __func__, __LINE__, pad, sel, config);
		//printf("%s %d pad offset %x\n", __func__, __LINE__, pad_info[pad].select_offset);
		//printf("%s %d pad offset %x\n", __func__, __LINE__, pad_info[pad].config_offset);
		//printf("%s %d pad index %d\n", __func__, __LINE__, pad_info[pad].pin_index);
		//set sw config
		unsigned int offset = pad_info[pad].config_offset + pad_info[pad].pin_index / 4 * 4;
		reg_tmp = readl(priv->pb_sw_reg_base + offset);
		reg_tmp &= ~(REG_CONFIG_ONEPAD_MASK << ((pad_info[pad].pin_index % 4) *
		            REG_CONFIG_ONEPAD_WIDTH));
		reg_tmp |= config << ((pad_info[pad].pin_index % 4) * REG_CONFIG_ONEPAD_WIDTH);
		//printf("%s %d config: 0x%x 0x%x\n", __func__, __LINE__, reg_tmp, priv->pb_sw_reg_base + offset);
		writel(reg_tmp, priv->pb_sw_reg_base + offset);

		//set pad select
		offset = pad_info[pad].select_offset + pad_info[pad].pin_index / 8 * 4;
		reg_tmp = readl(priv->pb_sw_reg_base + offset);
		reg_tmp &= ~(REG_FUNCSEL_ONEPAD_MASK << ((pad_info[pad].pin_index % 8) *
		            REG_FUNCSEL_ONEPAD_WIDTH));
		reg_tmp |= sel << ((pad_info[pad].pin_index % 8) * REG_FUNCSEL_ONEPAD_WIDTH);
		//printf("%s %d select: 0x%x 0x%x\n",  __func__, __LINE__, reg_tmp, priv->pb_sw_reg_base + offset);
		writel(reg_tmp, priv->pb_sw_reg_base + offset);

		if(WDT_PAD_0 == pad && WDT_PAD_0_MODE == sel) {
			/* PB10 is FUNC3: WDT0 */
			/* set WDT0 chip reset enable */
			reg_tmp = readl(priv->pb_sw_reg_base + PB_MSC_WDT_OFFSET);
			reg_tmp |= 0x1;
			writel(reg_tmp, priv->pb_sw_reg_base + PB_MSC_WDT_OFFSET);
		}

		if(WDT_PAD_1 == pad && WDT_PAD_1_MODE == sel) {
			/* PB22 is FUNC4: WDT1 */
			/* set WDT1 chip reset enable */
			reg_tmp = readl(priv->pb_sw_reg_base + PB_MSC_WDT_OFFSET);
			reg_tmp |= 0x2;
			writel(reg_tmp, priv->pb_sw_reg_base + PB_MSC_WDT_OFFSET);
		}

		if(WDT_PAD_2 == pad && WDT_PAD_2_MODE == sel) {
			/* PB61 is FUNC2: WDT2 */
			/* set WDT2 chip reset enable */
			reg_tmp = readl(priv->pb_sw_reg_base + PB_MSC_WDT_OFFSET);
			reg_tmp |= 0x4;
			writel(reg_tmp, priv->pb_sw_reg_base + PB_MSC_WDT_OFFSET);
		}

	}

	return ret;
}

static int sca200v200_pinctrl_set_state_simple(struct udevice *dev,
    struct udevice *periph)
{
	return 0;
}

const struct pinctrl_ops single_pinctrl_ops = {
	.set_state_simple = sca200v200_pinctrl_set_state_simple,
};

static const struct udevice_id sca200v200_pinctrl_match[] = {
	{ .compatible = "smartchip,sca200v200-pinctrl" },
	{ /* sentinel */ }
};

U_BOOT_DRIVER(sca200v200_pinctrl) = {
	.name = "sca200v200_pinctrl",
	.id = UCLASS_PINCTRL,
	.of_match = sca200v200_pinctrl_match,
	.priv_auto_alloc_size = sizeof(struct sca200v200_pinctrl_priv),
	.ops = &single_pinctrl_ops,
#if !CONFIG_IS_ENABLED(OF_PLATDATA)
	.bind       = dm_scan_fdt_dev,
#endif
	.probe      = sca200v200_pinctrl_probe,
};

