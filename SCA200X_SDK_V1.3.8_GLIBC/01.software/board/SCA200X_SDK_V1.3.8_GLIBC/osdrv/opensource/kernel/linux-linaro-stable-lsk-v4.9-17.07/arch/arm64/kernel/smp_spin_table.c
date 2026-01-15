/*
 * Spin Table SMP initialisation
 *
 * Copyright (C) 2013 ARM Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <linux/delay.h>
#include <linux/init.h>
#include <linux/of.h>
#include <linux/smp.h>
#include <linux/types.h>

#include <asm/cacheflush.h>
#include <asm/cpu_ops.h>
#include <asm/cputype.h>
#include <asm/io.h>
#include <asm/smp_plat.h>

/*
 * for smp boot. Considering some cases:
 * 1) secondary cpus need return to EL1 Non-secure, EL1 secure(to kernel or rots), need to use eret.
 *    For this case, only entry point is not enough. spsr_el3, scr_els need to set.
 *    El2 will be not used.
 * 2) Consider the possibility that need to add some patch in el3, secondary cpus need to use br to 
 *    continue run in EL3.
 */
struct sc_smp_secondary_boot_info
{
	unsigned long spsr_el3;		/*spsr_el3 if use eret instruction*/
	unsigned long scr_el3;		/*scr_ele if use eret instruction*/
};

/*
 * default secure_status = 0;  0: secure status.  1: non-secure status
 */
u64 secure_status = 0;

extern void secondary_holding_pen(void);
volatile unsigned long __section(".mmuoff.data.read")
secondary_holding_pen_release = INVALID_HWID;

static phys_addr_t cpu_release_addr[NR_CPUS];

/*
 * Write secondary_holding_pen_release in a way that is guaranteed to be
 * visible to all observers, irrespective of whether they're taking part
 * in coherency or not.  This is necessary for the hotplug code to work
 * reliably.
 */
static void write_pen_release(u64 val)
{
	void *start = (void *)&secondary_holding_pen_release;
	unsigned long size = sizeof(secondary_holding_pen_release);

	secondary_holding_pen_release = val;
	__flush_dcache_area(start, size);
}

static int smp_spin_table_cpu_init(unsigned int cpu)
{
	struct device_node *dn;
	struct device_node *dn_cpu;
	int ret;

	dn = of_get_cpu_node(cpu, NULL);
	if (!dn)
		return -ENODEV;

	/*
	 * Determine the address from which the CPU is polling.
	 */
	ret = of_property_read_u64(dn, "cpu-release-addr",
				   &cpu_release_addr[cpu]);
	if (ret)
		pr_err("CPU %d: missing or invalid cpu-release-addr property\n",
		       cpu);

	dn_cpu = of_get_parent(dn);
	if (!dn_cpu) 
			pr_err("CPU %d: missing or invalid cpu parent device node \n", cpu);

	//default to secure_status, ignore read error status. 
	if (of_property_read_u64(dn_cpu, "cpu-secure-status", &secure_status)) {
		secure_status = 1;
	}

	of_node_put(dn);

	return ret;
}

static int smp_spin_table_cpu_prepare(unsigned int cpu)
{
	__le64 __iomem *release_addr;
	struct sc_smp_secondary_boot_info __iomem *boot_info_addr;

	if (!cpu_release_addr[cpu])
		return -ENODEV;

	/*
	 * The cpu-release-addr may or may not be inside the linear mapping.
	 * As ioremap_cache will either give us a new mapping or reuse the
	 * existing linear mapping, we can use it to cover both cases. In
	 * either case the memory will be MT_NORMAL.
	 */
	release_addr = ioremap_cache(cpu_release_addr[cpu],
				     sizeof(*release_addr));
	if (!release_addr)
		return -ENOMEM;

	boot_info_addr = ioremap_cache(cpu_release_addr[cpu] + (4 - cpu) * sizeof(unsigned long), sizeof(*boot_info_addr));
	if (!boot_info_addr)
		return -ENOMEM;

	writeq_relaxed(0x3c5, &boot_info_addr->spsr_el3);
	writeq_relaxed(0x630, &boot_info_addr->scr_el3);

	__flush_dcache_area((__force void *)boot_info_addr,
			    sizeof(*boot_info_addr));
	/*
	 * We write the release address as LE regardless of the native
	 * endianess of the kernel. Therefore, any boot-loaders that
	 * read this address need to convert this address to the
	 * boot-loader's endianess before jumping. This is mandated by
	 * the boot protocol.
	 */
	writeq_relaxed(__pa(secondary_holding_pen), release_addr);
	__flush_dcache_area((__force void *)release_addr,
			    sizeof(*release_addr));

	/*
	 * Send an event to wake up the secondary CPU.
	 */
	sev();

	iounmap(release_addr);
	iounmap(boot_info_addr);

	return 0;
}

static int smp_spin_table_cpu_boot(unsigned int cpu)
{
	/*
	 * Update the pen release flag.
	 */
	write_pen_release(cpu_logical_map(cpu));

	/*
	 * Send an event, causing the secondaries to read pen_release.
	 */
	sev();

	return 0;
}

const struct cpu_operations smp_spin_table_ops = {
	.name		= "sc-spin-table",
	.cpu_init	= smp_spin_table_cpu_init,
	.cpu_prepare	= smp_spin_table_cpu_prepare,
	.cpu_boot	= smp_spin_table_cpu_boot,
};
