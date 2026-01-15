/*
 * (C) Copyright 2022 SmartChip Electronics Co., Ltd
 *
 * SPDX-License-Identifier: GPL-2.0+
 */

#include <common.h>
#include <dm.h>
#include <init.h>
#include <log.h>
#include <ram.h>
#include <asm/io.h>
#include <dm/uclass-internal.h>

DECLARE_GLOBAL_DATA_PTR;

#define TRUST_PARAMETER_OFFSET    (34 * 1024 * 1024)

struct tos_parameter_t {
	u32 version;
	u32 checksum;
	struct {
		char name[8];
		s64 phy_addr;
		u32 size;
		u32 flags;
	} tee_mem;
	struct {
		char name[8];
		s64 phy_addr;
		u32 size;
		u32 flags;
	} drm_mem;
	s64 reserve[8];
};

int dram_init_banksize(void)
{
#ifdef CONFIG_ARM64
	/* Reserve 0x200000 for ATF bl31 */
	/* struct global_data { struct bd_info *bd; }; */
	gd->bd->bi_dram[0].start = CONFIG_SYS_SDRAM_BASE;
	gd->bd->bi_dram[0].size = gd->ram_size;

#else
	size_t top = min((gd->ram_size + CONFIG_SYS_SDRAM_BASE), gd->ram_top);
#ifdef CONFIG_SPL_OPTEE
	struct tos_parameter_t *tos_parameter;

	tos_parameter = (struct tos_parameter_t *)(CONFIG_SYS_SDRAM_BASE +
	        TRUST_PARAMETER_OFFSET);

	if (tos_parameter->tee_mem.flags == 1) {
		gd->bd->bi_dram[0].start = CONFIG_SYS_SDRAM_BASE;
		gd->bd->bi_dram[0].size = tos_parameter->tee_mem.phy_addr
		    - CONFIG_SYS_SDRAM_BASE;
		gd->bd->bi_dram[1].start = tos_parameter->tee_mem.phy_addr +
		    tos_parameter->tee_mem.size;
		gd->bd->bi_dram[1].size = top - gd->bd->bi_dram[1].start;
	} else {
		gd->bd->bi_dram[0].start = CONFIG_SYS_SDRAM_BASE;
		gd->bd->bi_dram[0].size = 0x8400000;
		/* Reserve 32M for OPTEE with TA */
		gd->bd->bi_dram[1].start = CONFIG_SYS_SDRAM_BASE
		    + gd->bd->bi_dram[0].size + 0x2000000;
		gd->bd->bi_dram[1].size = top - gd->bd->bi_dram[1].start;
	}
#else
	gd->bd->bi_dram[0].start = CONFIG_SYS_SDRAM_BASE;
	gd->bd->bi_dram[0].size = top - gd->bd->bi_dram[0].start;
#endif
#endif

	return 0;
}

int dram_init(void)
{
	struct ram_info ram;
	struct udevice *dev;
	int ret;

	gd->ram_size = 0x40000000;

	ret = uclass_get_device(UCLASS_RAM, 0, &dev);
	if (ret) {
		debug("DRAM init failed: %d\n", ret);
		return ret;
	}
	ret = ram_get_info(dev, &ram);
	if (ret) {
		debug("Cannot get DRAM size: %d\n", ret);
		return ret;
	}
	gd->ram_size = ram.size;
	debug("SDRAM base=%lx, size=%lx\n",
	    (unsigned long)ram.base, (unsigned long)ram.size);

	return 0;
}

ulong board_get_usable_ram_top(ulong total_size)
{
	unsigned long top = CONFIG_SYS_SDRAM_BASE + SDRAM_MAX_SIZE;

	return (gd->ram_top > top) ? top : gd->ram_top;
}

