/*
 * Copyright (C) 2002 ARM Limited, All Rights Reserved.
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

#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/irq.h>
#include <linux/irqchip/arm-gic.h>

#include "irq-gic-common.h"

static const struct gic_kvm_info *gic_kvm_info;

const struct gic_kvm_info *gic_get_kvm_info(void)
{
	return gic_kvm_info;
}

void gic_set_kvm_info(const struct gic_kvm_info *info)
{
	BUG_ON(gic_kvm_info != NULL);
	gic_kvm_info = info;
}

void gic_enable_quirks(u32 iidr, const struct gic_quirk *quirks,
		void *data)
{
	for (; quirks->desc; quirks++) {
		if (quirks->iidr != (quirks->mask & iidr))
			continue;
		quirks->init(data);
		pr_info("GIC: enabling workaround for %s\n", quirks->desc);
	}
}

int gic_configure_irq(unsigned int irq, unsigned int type,
		       void __iomem *base, void (*sync_access)(void))
{
	u32 confmask = 0x2 << ((irq % 16) * 2);
	u32 confoff = (irq / 16) * 4;
	u32 val, oldval;
	int ret = 0;

	/*
	 * Read current configuration register, and insert the config
	 * for "irq", depending on "type".
	 */
	val = oldval = readl_relaxed(base + GIC_DIST_CONFIG + confoff);
	if (type & IRQ_TYPE_LEVEL_MASK)
		val &= ~confmask;
	else if (type & IRQ_TYPE_EDGE_BOTH)
		val |= confmask;

	/* If the current configuration is the same, then we are done */
	if (val == oldval)
		return 0;

	/*
	 * Write back the new configuration, and possibly re-enable
	 * the interrupt. If we fail to write a new configuration for
	 * an SPI then WARN and return an error. If we fail to write the
	 * configuration for a PPI this is most likely because the GIC
	 * does not allow us to set the configuration or we are in a
	 * non-secure mode, and hence it may not be catastrophic.
	 */
	writel_relaxed(val, base + GIC_DIST_CONFIG + confoff);
	if (readl_relaxed(base + GIC_DIST_CONFIG + confoff) != val) {
		if (WARN_ON(irq >= 32))
			ret = -EINVAL;
		else
			pr_warn("GIC: PPI%d is secure or misconfigured\n",
				irq - 16);
	}

	if (sync_access)
		sync_access();

	return ret;
}

void gic_dist_config(void __iomem *base, int gic_irqs,
		     void (*sync_access)(void))
{
	u32 i;
	u32 target_cpu_set[32] = {0};
	u8 mask;

	for (i = mask = 0; i < 32; i += 4) {
		mask = readl_relaxed(base + GIC_DIST_TARGET + i);
		mask |= mask >> 16;
		mask |= mask >> 8;
		if (mask)
			break;
	}

	/*
	 * get all irq SPI traget cpu, if the target cpu already set, irq used by firmware, not need to config
	*/
	for (i = 32 ; i < gic_irqs; i += 4) {
		u32 target = readl_relaxed(base + GIC_DIST_TARGET + i);
		u32 tmp;
		tmp = ((target & 0xff) & mask);
		target_cpu_set[i/32] |= (tmp << (i%32));

		tmp = (((target >> 8) & 0xff) & mask);
		target_cpu_set[i/32] |= (((target >> 8)  & 0xff) << (i%32 + 1));

		tmp = (((target >> 16) & 0xff) & mask);
		target_cpu_set[i/32] |= (((target >> 16) & 0xff) << (i%32 + 2));

		tmp = (((target >> 24) & 0xff) & mask);
		target_cpu_set[i/32] |= (((target >> 24) & 0xff) << (i%32 + 3));
	}

	/*
	 * if cpu target is not set, set this irq to Linux
	 * Set all global interrupts to be level triggered, active low.
	 */
	for (i = 32; i < gic_irqs; i += 16) {
		u32 trig = readl_relaxed(base + GIC_DIST_CONFIG + i / 4);
		u32 j;
		for (j = 0 ; j < 16; j++) {
			if ((target_cpu_set[i/32] >> ((i + j)%32)) & 0x01) { // irq is not regitered by another cpu.(rtos)
				trig &= (~(0x03L << (j * 2)));
				trig |= (GICD_INT_ACTLOW_LVLTRIG << (j * 2));
			}
		}
		writel_relaxed(trig, base + GIC_DIST_CONFIG + i / 4);
	}

	/*
	 * the same, set only the irq not enabled by rtos
	 * Set priority on all global interrupts, 
	 */
	for (i = 32; i < gic_irqs; i += 4) {
		u32 j;
		u32 prio = readl_relaxed(base + GIC_DIST_CONFIG + i);

		for (j = 0 ; j < 4; j++) {
			//handle irq is not regitered by another cpu.(rtos)
			if ((target_cpu_set[i/32] >> ((i + j)%32)) & 0x01) { 
				prio &= (~(0xffL << (j * 8)));
				prio |= (GICD_INT_DEF_PRI << (j * 8));
			}
		}
		writel_relaxed(prio, base + GIC_DIST_PRI + i);
	}

	/*
	 * the same, set only the irq not enabled by rtos, write 1 to clear the irq
	 * Deactivate and disable all SPIs. Leave the PPI and SGIs
	 * alone as they are in the redistributor registers on GICv3.
	 */
	for (i = 32; i < gic_irqs; i += 32) {
		writel_relaxed(target_cpu_set[i/32],
			       base + GIC_DIST_ACTIVE_CLEAR + i / 8);
		writel_relaxed(target_cpu_set[i/32],
			       base + GIC_DIST_ENABLE_CLEAR + i / 8);
	}

	if (sync_access)
		sync_access();
}

void gic_cpu_config(void __iomem *base, void (*sync_access)(void))
{
	int i;

	/*
	 * Deal with the banked PPI and SGI interrupts - disable all
	 * PPI interrupts, ensure all SGI interrupts are enabled.
	 * Make sure everything is deactivated.
	 */
	writel_relaxed(GICD_INT_EN_CLR_X32, base + GIC_DIST_ACTIVE_CLEAR);
	writel_relaxed(GICD_INT_EN_CLR_PPI, base + GIC_DIST_ENABLE_CLEAR);
	writel_relaxed(GICD_INT_EN_SET_SGI, base + GIC_DIST_ENABLE_SET);

	/*
	 * Set priority on PPI and SGI interrupts
	 */
	for (i = 0; i < 32; i += 4)
		writel_relaxed(GICD_INT_DEF_PRI_X4,
					base + GIC_DIST_PRI + i * 4 / 4);

	if (sync_access)
		sync_access();
}
