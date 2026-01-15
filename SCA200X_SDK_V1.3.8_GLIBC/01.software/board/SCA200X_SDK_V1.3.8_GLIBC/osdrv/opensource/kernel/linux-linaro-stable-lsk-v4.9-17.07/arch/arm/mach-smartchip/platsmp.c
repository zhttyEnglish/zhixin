/*  SmartChip Co., Ltd.
 *  Cloned from linux/arch/arm/mach-vexpress/platsmp.c
 *
 *  Copyright (C) 2002 ARM Ltd.
 *  All Rights Reserved
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <linux/init.h>
#include <linux/errno.h>
#include <linux/smp.h>
#include <linux/io.h>
#include <linux/of_address.h>
#include <linux/delay.h>
#include <linux/jiffies.h>

#include <asm/mcpm.h>
#include <asm/smp_scu.h>
#include <asm/mach/map.h>
#include <asm/smp_plat.h>

extern void smartx_secondary_startup(void);

//#include "core.h"
/*hbbai, we temporarily save entry point in this reg, may change it later..*/
#define SMARTX_CPU_CFG_SECOND_ENTRY_REG (0x6061002C)
#define SMARTX_GIC_DIST_BASE            (0x2C001000)
#define GIC_DIST_SGI_REG                (0xF00)

void smartx_flags_set(u32 data)
{
	static void __iomem *mem = NULL;
	mem = ioremap(SMARTX_CPU_CFG_SECOND_ENTRY_REG, 0x1000);
	if (!mem) {
		pr_info("Map cpu cfg reg for smp start up error!\n");
		return;
	}

	writel(data, mem);
	//isb();
}

bool __init smartx_smp_init_ops(void)
{
#ifdef CONFIG_MCPM
	/*
	 * The best way to detect a multi-cluster configuration at the moment
	 * is to look for the presence of a CCI in the system.
	 * Override the default vexpress_smp_ops if so.
	 */
	struct device_node *node;
	node = of_find_compatible_node(NULL, NULL, "arm,cci-400");
	if (node && of_device_is_available(node)) {
		mcpm_smp_set_ops();
		return true;
	}
#endif
	return false;
}

static void __init smartx_smp_dt_prepare_cpus(unsigned int max_cpus)
{
	//hbbai: assert reset of the cores, power up and then deassert reset ?
	//pr_info("Prepare cpus for smp, max %d\n", max_cpus);
	/*
	 * Write the address of secondary startup into the
	 * system-wide flags register. The boot monitor waits
	 * until it receives a soft interrupt, and then the
	 * secondary CPU branches to this address.
	 */
	smartx_flags_set(virt_to_phys(smartx_secondary_startup));
}
/*
 * Write pen_release in a way that is guaranteed to be visible to all
 * observers, irrespective of whether they're taking part in coherency
 * or not.  This is necessary for the hotplug code to work reliably.
 */
static void write_pen_release(int val)
{
	pen_release = val;
	smp_wmb();
	sync_cache_w(&pen_release);
}

static DEFINE_SPINLOCK(boot_lock);

void smartx_secondary_init(unsigned int cpu)
{
	//pr_info("cpu %d start init...\n", cpu);
	/*
	 * let the primary processor know we're out of the
	 * pen, then head off into the C entry point
	 */
	//if(pen_release == cpu)
	write_pen_release(-1);

	/*
	 * Synchronise with the boot thread.
	 */
	spin_lock(&boot_lock);
	spin_unlock(&boot_lock);
}

/* send ipi to secondary cpu #N (1-3)*/
/* Warning: This is used only to wake CPU HERE, do not use it somewhere else!*/
static void smartx_send_wakeup_ipi(unsigned int cpu)
{
	unsigned int map = 1 << cpu;
	unsigned int irq = 0; //we use ipi #0 to wake up the other cores
	void __iomem *mem = NULL;
	mem = ioremap(SMARTX_GIC_DIST_BASE, 0x1000);
	if (!mem) {
		pr_info("Map GIC reg for smp start up error!\n");
		return;
	}
	pr_info("Wake up CPU %d: %x\n", cpu, map);
	writel_relaxed(map << 16 | irq, mem + GIC_DIST_SGI_REG);
}

int smartx_boot_secondary(unsigned int cpu, struct task_struct *idle)
{
	unsigned long timeout;

	pr_info("Primary cpu starts booting brothers...%d\n", cpu);
	/*
	 * Set synchronisation state between this boot processor
	 * and the secondary one
	 */
	spin_lock(&boot_lock);

	/*
	 * This is really belt and braces; we hold unintended secondary
	 * CPUs in the holding pen until we're ready for them.  However,
	 * since we haven't sent them a soft interrupt, they shouldn't
	 * be there.
	 */
	//pr_info("pen_release = %x\n", cpu_logical_map(cpu));
	write_pen_release(cpu_logical_map(cpu));

	/*
	 * Send the secondary CPU a soft interrupt, thereby causing
	 * the boot monitor to read the system wide flags register,
	 * and branch to the address found there.
	 */
	//arch_send_wakeup_ipi_mask(cpumask_of(cpu));
	smartx_send_wakeup_ipi(cpu);

	timeout = jiffies + (1 * HZ);
	while (time_before(jiffies, timeout)) {
		smp_rmb();
		if (pen_release == -1)
			break;

		udelay(10);
	}

	/*
	 * now the secondary core is starting up let it run its
	 * calibrations, then wait for it to finish
	 */
	//pr_info("Core %d start running!\n", cpu);
	spin_unlock(&boot_lock);

	return pen_release != -1 ? -ENOSYS : 0;
}
struct smp_operations __initdata smartx_smp_ops = {
	.smp_prepare_cpus   = smartx_smp_dt_prepare_cpus,
	.smp_secondary_init = smartx_secondary_init,
	.smp_boot_secondary = smartx_boot_secondary,
#ifdef CONFIG_HOTPLUG_CPU
	.cpu_die        = smartx_cpu_die,
#endif
};

CPU_METHOD_OF_DECLARE(smartx_smp, "smartchip,smartx-smp", &smartx_smp_ops);
