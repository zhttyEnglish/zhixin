// SPDX-License-Identifier: GPL-2.0+ OR BSD-3-Clause
/*
 * (C) Copyright 2017 Rockchip Electronics Co., Ltd.
 */
#include <common.h>
#include <clk.h>
#include <debug_uart.h>
#include <dm.h>
#include <dt-structs.h>
#include <init.h>
#include <log.h>
#include <ram.h>
#include <regmap.h>
#include <syscon.h>
#include <asm/io.h>
#include <linux/delay.h>
#include <fdt.h>
#include <linux/libfdt.h>
#include <blk.h>
#include <smartchip/sc_common.h>
#include <asm/armv8/mmu.h>
#include <dt-bindings/memory/sca200v100-ddrc.h>

struct smartchip_dram_info {
	struct ram_info info;
};

static int sca200v100_dmc_probe(struct udevice *dev)
{
	struct smartchip_dram_info *priv = dev_get_priv(dev);
	int ret;
	u32 density;
	u32 width;

	ret = dev_read_u32(dev, "user-density", &density);
	if (ret) {
		return -EINVAL;
	}

	ret = dev_read_u32(dev, "user-width", &width);
	if (ret) {
		return -EINVAL;
	}

	priv->info.base = CONFIG_SYS_SDRAM_BASE;
	priv->info.size = density * 2 * ((width==USER_MODIFY_WIDTH_16BIT)?1:2);
	priv->info.size <<= 20;

	mem_map[1].size = priv->info.size;

	return 0;
}

static int sca200v100_dmc_get_info(struct udevice *dev, struct ram_info *info)
{
	struct smartchip_dram_info *priv = dev_get_priv(dev);

	*info = priv->info;

	return 0;
}

static struct ram_ops sca200v100_dmc_ops = {
	.get_info = sca200v100_dmc_get_info,
};

static const struct udevice_id sca200v100_dmc_ids[] = {
	{ .compatible = "smartchip,sca200v100-dmc" },
	{ }
};

U_BOOT_DRIVER(smartchip_sca200v100_dmc) = {
	.name = "sca200v100_dmc",
	.id = UCLASS_RAM,
	.of_match = sca200v100_dmc_ids,
	.ops = &sca200v100_dmc_ops,
	.probe = sca200v100_dmc_probe,
	.priv_auto_alloc_size = sizeof(struct smartchip_dram_info),
};
