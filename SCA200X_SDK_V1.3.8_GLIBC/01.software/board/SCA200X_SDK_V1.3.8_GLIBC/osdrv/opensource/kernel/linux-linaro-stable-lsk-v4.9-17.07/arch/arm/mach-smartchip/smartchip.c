/*
 * Device Tree support for SmartChip SoCs
 *
 * Copyright (C) 2012 SmartChip Co.,Ltd.
 *
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#include <linux/clk-provider.h>
#include <linux/clocksource.h>
#include <linux/init.h>
#include <linux/platform_device.h>

#include <asm/mach/arch.h>
#define SMARTX_CORESIGHT_TIMESTAMP_CTRL  (0x60406000)
static void __init smartchip_dt_cpufreq_init(void)
{
	platform_device_register_simple("cpufreq-dt", -1, NULL, 0);
}

static const char *const smartx_board_dt_compat[] = {
	"smartchip,smartx-demo",
	NULL,
};
static void smartx_enable_timestamp(void)
{
	u32 val = 0;
	void __iomem *mem;

	mem = ioremap(SMARTX_CORESIGHT_TIMESTAMP_CTRL, 0x1000);
	if (!mem) {
		pr_info("Map coresight timestamp reg error!\n");
		return;
	}
	val = readl(mem);
	val |= 0x1; //bit 0 is enable bit
	writel(val, mem);
	isb();
}
//extern void __init smartx_reset_init(void);
static void __init smartx_timer_init(void)
{
	//hbbai, we need to enable Coresight timestamp to generate counter
	smartx_enable_timestamp();
	of_clk_init(NULL);

	//  if (IS_ENABLED(CONFIG_RESET_CONTROLLER))
	//      smartx_reset_init();
	clocksource_probe();
}

DT_MACHINE_START(SMARTX_DT, "SmartChip Smartx Family")
	.init_time  = smartx_timer_init,
	.dt_compat  = smartx_board_dt_compat,
	.init_late  = smartchip_dt_cpufreq_init,
	//.smp        = smp_ops(smartx_smp_dt_ops),
	//.smp_init   = smp_init_ops(smartx_smp_init_ops),
MACHINE_END

