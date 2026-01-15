/**
 * Synopsys Designware PCIe Endpoint controller driver
 *
 * Copyright (C) 2017 Texas Instruments
 * Author: Kishon Vijay Abraham I <kishon@ti.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 of
 * the License as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <linux/irq.h>
#include <linux/irqdomain.h>
#include <linux/kernel.h>
#include <linux/msi.h>
#include <linux/of_address.h>
#include <linux/of_pci.h>
#include <linux/pci.h>
#include <linux/pci_regs.h>
#include <linux/platform_device.h>
#include <linux/types.h>
#include <linux/delay.h>

#include "../host/pcie-designware.h"
#include "pcie-designware-ep.h"
#include <linux/pci-epc.h>
#include <linux/pci-epf.h>
#include <linux/pci_regs.h>

#define MSI_MESSAGE_CONTROL     0x52
#define MSI_CAP_MMC_SHIFT       1
#define MSI_CAP_MME_SHIFT       4
#define MSI_CAP_MME_MASK        (7 << MSI_CAP_MME_SHIFT)
#define MSI_MESSAGE_ADDR_L32        0x54
#define MSI_MESSAGE_ADDR_U32        0x58

#define PCIE_ATU_CR1            0x904
#define PCIE_ATU_TYPE_MEM       (0x0 << 0)
#define PCIE_ATU_TYPE_IO        (0x2 << 0)
#define PCIE_ATU_TYPE_CFG0      (0x4 << 0)
#define PCIE_ATU_TYPE_CFG1      (0x5 << 0)
#define PCIE_ATU_CR2            0x908
#define PCIE_ATU_ENABLE         (0x1 << 31)
#define PCIE_ATU_BAR_MODE_ENABLE    (0x1 << 30)
#define PCIE_ATU_LOWER_BASE     0x90C
#define PCIE_ATU_UPPER_BASE     0x910
#define PCIE_ATU_LIMIT          0x914
#define PCIE_ATU_LOWER_TARGET       0x918
#define PCIE_ATU_BUS(x)         (((x) & 0xff) << 24)
#define PCIE_ATU_DEV(x)         (((x) & 0x1f) << 19)
#define PCIE_ATU_FUNC(x)        (((x) & 0x7) << 16)
#define PCIE_ATU_UPPER_TARGET       0x91C

#define PCIE_GET_ATU_INB_UNR_REG_OFFSET(region)             \
            ((0x3 << 20) | ((region) << 9) | (0x1 << 8))

#define to_dw_pcie_from_ep(endpoint)   \
        container_of((endpoint), struct dw_pcie, ep)

int dw_pcie_read(void __iomem *addr, int size, u32 *val)
{
	if ((uintptr_t)addr & (size - 1)) {
		*val = 0;
		return PCIBIOS_BAD_REGISTER_NUMBER;
	}

	if (size == 4) {
		*val = readl(addr);
	} else if (size == 2) {
		*val = readw(addr);
	} else if (size == 1) {
		*val = readb(addr);
	} else {
		*val = 0;
		return PCIBIOS_BAD_REGISTER_NUMBER;
	}

	return PCIBIOS_SUCCESSFUL;
}

int dw_pcie_write(void __iomem *addr, int size, u32 val)
{
	if ((uintptr_t)addr & (size - 1))
		return PCIBIOS_BAD_REGISTER_NUMBER;

	if (size == 4)
		writel(val, addr);
	else if (size == 2)
		writew(val, addr);
	else if (size == 1)
		writeb(val, addr);
	else
		return PCIBIOS_BAD_REGISTER_NUMBER;

	return PCIBIOS_SUCCESSFUL;
}

static u32 __dw_pcie_read_dbi(struct dw_pcie *pci, void __iomem *base, u32 reg,
    size_t size)
{
	int ret;
	u32 val;

	if (pci->ops->read_dbi)
		return pci->ops->read_dbi(pci, base, reg, size);

	ret = dw_pcie_read(base + reg, size, &val);
	if (ret)
		dev_err(pci->dev, "read DBI address failed\n");

	return val;
}

static void __dw_pcie_write_dbi(struct dw_pcie *pci, void __iomem *
    base, u32 reg, size_t size, u32 val)
{
	int ret;

	if (pci->ops->write_dbi) {
		pci->ops->write_dbi(pci, base, reg, size, val);
		return;
	}

	ret = dw_pcie_write(base + reg, size, val);
	if (ret)
		dev_err(pci->dev, "write DBI address failed\n");
}

static inline void dw_pcie_writel_dbi(struct dw_pcie *pci, u32 reg, u32 val)
{
	__dw_pcie_write_dbi(pci, pci->dbi_base, reg, 0x4, val);
}

static inline u32 dw_pcie_readl_dbi(struct dw_pcie *pci, u32 reg)
{
	return __dw_pcie_read_dbi(pci, pci->dbi_base, reg, 0x4);
}

static inline void dw_pcie_writew_dbi(struct dw_pcie *pci, u32 reg, u16 val)
{
	__dw_pcie_write_dbi(pci, pci->dbi_base, reg, 0x2, val);
}

static inline u16 dw_pcie_readw_dbi(struct dw_pcie *pci, u32 reg)
{
	return __dw_pcie_read_dbi(pci, pci->dbi_base, reg, 0x2);
}

static inline void dw_pcie_writeb_dbi(struct dw_pcie *pci, u32 reg, u8 val)
{
	__dw_pcie_write_dbi(pci, pci->dbi_base, reg, 0x1, val);
}

static inline u8 dw_pcie_readb_dbi(struct dw_pcie *pci, u32 reg)
{
	return __dw_pcie_read_dbi(pci, pci->dbi_base, reg, 0x1);
}

static inline void dw_pcie_writel_dbi2(struct dw_pcie *pci, u32 reg, u32 val)
{
	__dw_pcie_write_dbi(pci, pci->dbi_base2, reg, 0x4, val);
}

static inline u32 dw_pcie_readl_dbi2(struct dw_pcie *pci, u32 reg)
{
	return __dw_pcie_read_dbi(pci, pci->dbi_base2, reg, 0x4);
}

void dw_pcie_ep_linkup(struct dw_pcie_ep *ep)
{
	struct pci_epc *epc = ep->epc;

	pci_epc_linkup(epc);
}

static void dw_pcie_ep_reset_bar(struct dw_pcie *pci, enum pci_barno bar)
{
	u32 reg;

	reg = PCI_BASE_ADDRESS_0 + (4 * bar);
	dw_pcie_writel_dbi2(pci, reg, 0x0);
	dw_pcie_writel_dbi(pci, reg, 0x0);
}

static int dw_pcie_ep_write_header(struct pci_epc *epc,
    struct pci_epf_header *hdr)
{
	struct dw_pcie_ep *ep = epc_get_drvdata(epc);
	struct dw_pcie *pci = to_dw_pcie_from_ep(ep);

	dw_pcie_writew_dbi(pci, PCI_VENDOR_ID, hdr->vendorid);
	dw_pcie_writew_dbi(pci, PCI_DEVICE_ID, hdr->deviceid);
	dw_pcie_writeb_dbi(pci, PCI_REVISION_ID, hdr->revid);
	dw_pcie_writeb_dbi(pci, PCI_CLASS_PROG, hdr->progif_code);
	dw_pcie_writew_dbi(pci, PCI_CLASS_DEVICE,
	    hdr->subclass_code | hdr->baseclass_code << 8);
	dw_pcie_writeb_dbi(pci, PCI_CACHE_LINE_SIZE,
	    hdr->cache_line_size);
	dw_pcie_writew_dbi(pci, PCI_SUBSYSTEM_VENDOR_ID,
	    hdr->subsys_vendor_id);
	dw_pcie_writew_dbi(pci, PCI_SUBSYSTEM_ID, hdr->subsys_id);
	dw_pcie_writeb_dbi(pci, PCI_INTERRUPT_PIN,
	    hdr->interrupt_pin);

	return 0;
}

static void dw_pcie_writel_ib_unroll(struct dw_pcie *pci, u32 index, u32 reg,
    u32 val)
{
	u32 offset = PCIE_GET_ATU_INB_UNR_REG_OFFSET(index);

	dw_pcie_writel_dbi(pci, offset + reg, val);
}

static u32 dw_pcie_readl_ib_unroll(struct dw_pcie *pci, u32 index, u32 reg)
{
	u32 offset = PCIE_GET_ATU_INB_UNR_REG_OFFSET(index);

	return dw_pcie_readl_dbi(pci, offset + reg);
}

static int dw_pcie_prog_inbound_atu_unroll(struct dw_pcie *pci, int index, int bar,
    u64 cpu_addr, enum dw_pcie_as_type as_type)
{
	int type;
	u32 retries, val;

	dw_pcie_writel_ib_unroll(pci, index, PCIE_ATU_UNR_LOWER_TARGET,
	    lower_32_bits(cpu_addr));
	dw_pcie_writel_ib_unroll(pci, index, PCIE_ATU_UNR_UPPER_TARGET,
	    upper_32_bits(cpu_addr));

	switch (as_type) {
	case DW_PCIE_AS_MEM:
		type = PCIE_ATU_TYPE_MEM;
		break;
	case DW_PCIE_AS_IO:
		type = PCIE_ATU_TYPE_IO;
		break;
	default:
		return -EINVAL;
	}

	dw_pcie_writel_ib_unroll(pci, index, PCIE_ATU_UNR_REGION_CTRL1, type);
	dw_pcie_writel_ib_unroll(pci, index, PCIE_ATU_UNR_REGION_CTRL2,
	    PCIE_ATU_ENABLE |
	    PCIE_ATU_BAR_MODE_ENABLE | (bar << 8));

	/*
	 * Make sure ATU enable takes effect before any subsequent config
	 * and I/O accesses.
	 */
	for (retries = 0; retries < LINK_WAIT_MAX_IATU_RETRIES; retries++) {
		val = dw_pcie_readl_ib_unroll(pci, index,
		        PCIE_ATU_UNR_REGION_CTRL2);
		if (val & PCIE_ATU_ENABLE)
			return 0;

		usleep_range(LINK_WAIT_IATU_MIN, LINK_WAIT_IATU_MAX);
	}
	dev_err(pci->dev, "inbound iATU is not being enabled\n");

	return -EBUSY;
}

static int dw_pcie_prog_inbound_atu(struct dw_pcie *pci, int index, int bar,
    u64 cpu_addr, enum dw_pcie_as_type as_type)
{
	int type;
	u32 retries, val;

	if (pci->iatu_unroll_enabled)
		return dw_pcie_prog_inbound_atu_unroll(pci, index, bar,
		        cpu_addr, as_type);

	dw_pcie_writel_dbi(pci, PCIE_ATU_VIEWPORT, PCIE_ATU_REGION_INBOUND |
	    index);
	dw_pcie_writel_dbi(pci, PCIE_ATU_LOWER_TARGET, lower_32_bits(cpu_addr));
	dw_pcie_writel_dbi(pci, PCIE_ATU_UPPER_TARGET, upper_32_bits(cpu_addr));

	switch (as_type) {
	case DW_PCIE_AS_MEM:
		type = PCIE_ATU_TYPE_MEM;
		break;
	case DW_PCIE_AS_IO:
		type = PCIE_ATU_TYPE_IO;
		break;
	default:
		return -EINVAL;
	}

	dw_pcie_writel_dbi(pci, PCIE_ATU_CR1, type);
	dw_pcie_writel_dbi(pci, PCIE_ATU_CR2, PCIE_ATU_ENABLE
	    | PCIE_ATU_BAR_MODE_ENABLE | (bar << 8));

	/*
	 * Make sure ATU enable takes effect before any subsequent config
	 * and I/O accesses.
	 */
	for (retries = 0; retries < LINK_WAIT_MAX_IATU_RETRIES; retries++) {
		val = dw_pcie_readl_dbi(pci, PCIE_ATU_CR2);
		if (val & PCIE_ATU_ENABLE)
			return 0;

		usleep_range(LINK_WAIT_IATU_MIN, LINK_WAIT_IATU_MAX);
	}
	dev_err(pci->dev, "inbound iATU is not being enabled\n");

	return -EBUSY;
}

static int dw_pcie_ep_inbound_atu(struct dw_pcie_ep *ep, enum pci_barno bar,
    dma_addr_t cpu_addr,
    enum dw_pcie_as_type as_type)
{
	int ret;
	u32 free_win;
	struct dw_pcie *pci = to_dw_pcie_from_ep(ep);

	free_win = find_first_zero_bit(&ep->ib_window_map,
	        sizeof(ep->ib_window_map));
	if (free_win >= ep->num_ib_windows) {
		dev_err(pci->dev, "no free inbound window\n");
		return -EINVAL;
	}

	ret = dw_pcie_prog_inbound_atu(pci, free_win, bar, cpu_addr,
	        as_type);
	if (ret < 0) {
		dev_err(pci->dev, "Failed to program IB window\n");
		return ret;
	}

	ep->bar_to_atu[bar] = free_win;
	set_bit(free_win, &ep->ib_window_map);

	return 0;
}

static u32 dw_pcie_readl_ob_unroll(struct dw_pcie *pci, u32 index, u32 reg)
{
	u32 offset = PCIE_GET_ATU_OUTB_UNR_REG_OFFSET(index);

	return dw_pcie_readl_dbi(pci, offset + reg);
}

static void dw_pcie_writel_ob_unroll(struct dw_pcie *pci, u32 index, u32 reg,
    u32 val)
{
	u32 offset = PCIE_GET_ATU_OUTB_UNR_REG_OFFSET(index);

	dw_pcie_writel_dbi(pci, offset + reg, val);
}

void dw_pcie_prog_outbound_atu_unroll(struct dw_pcie *pci, int index, int type,
    u64 cpu_addr, u64 pci_addr, u32 size)
{
	u32 retries, val;

	dw_pcie_writel_ob_unroll(pci, index, PCIE_ATU_UNR_LOWER_BASE,
	    lower_32_bits(cpu_addr));
	dw_pcie_writel_ob_unroll(pci, index, PCIE_ATU_UNR_UPPER_BASE,
	    upper_32_bits(cpu_addr));
	dw_pcie_writel_ob_unroll(pci, index, PCIE_ATU_UNR_LIMIT,
	    lower_32_bits(cpu_addr + size - 1));
	dw_pcie_writel_ob_unroll(pci, index, PCIE_ATU_UNR_LOWER_TARGET,
	    lower_32_bits(pci_addr));
	dw_pcie_writel_ob_unroll(pci, index, PCIE_ATU_UNR_UPPER_TARGET,
	    upper_32_bits(pci_addr));
	dw_pcie_writel_ob_unroll(pci, index, PCIE_ATU_UNR_REGION_CTRL1,
	    type);
	dw_pcie_writel_ob_unroll(pci, index, PCIE_ATU_UNR_REGION_CTRL2,
	    PCIE_ATU_ENABLE);

	/*
	 * Make sure ATU enable takes effect before any subsequent config
	 * and I/O accesses.
	 */
	for (retries = 0; retries < LINK_WAIT_MAX_IATU_RETRIES; retries++) {
		val = dw_pcie_readl_ob_unroll(pci, index,
		        PCIE_ATU_UNR_REGION_CTRL2);
		if (val & PCIE_ATU_ENABLE)
			return;

		usleep_range(LINK_WAIT_IATU_MIN, LINK_WAIT_IATU_MAX);
	}
	dev_err(pci->dev, "outbound iATU is not being enabled\n");
}

void dw_pcie_prog_outbound_atu(struct dw_pcie *pci, int index, int type,
    u64 cpu_addr, u64 pci_addr, u32 size)
{
	u32 retries, val;

	if (pci->ops->cpu_addr_fixup)
		cpu_addr = pci->ops->cpu_addr_fixup(cpu_addr);

	if (pci->iatu_unroll_enabled) {
		dw_pcie_prog_outbound_atu_unroll(pci, index, type, cpu_addr,
		    pci_addr, size);
		return;
	}

	dw_pcie_writel_dbi(pci, PCIE_ATU_VIEWPORT,
	    PCIE_ATU_REGION_OUTBOUND | index);
	dw_pcie_writel_dbi(pci, PCIE_ATU_LOWER_BASE,
	    lower_32_bits(cpu_addr));
	dw_pcie_writel_dbi(pci, PCIE_ATU_UPPER_BASE,
	    upper_32_bits(cpu_addr));
	dw_pcie_writel_dbi(pci, PCIE_ATU_LIMIT,
	    lower_32_bits(cpu_addr + size - 1));
	dw_pcie_writel_dbi(pci, PCIE_ATU_LOWER_TARGET,
	    lower_32_bits(pci_addr));
	dw_pcie_writel_dbi(pci, PCIE_ATU_UPPER_TARGET,
	    upper_32_bits(pci_addr));
	dw_pcie_writel_dbi(pci, PCIE_ATU_CR1, type);
	dw_pcie_writel_dbi(pci, PCIE_ATU_CR2, PCIE_ATU_ENABLE);

	/*
	 * Make sure ATU enable takes effect before any subsequent config
	 * and I/O accesses.
	 */
	for (retries = 0; retries < LINK_WAIT_MAX_IATU_RETRIES; retries++) {
		val = dw_pcie_readl_dbi(pci, PCIE_ATU_CR2);
		if (val == PCIE_ATU_ENABLE)
			return;

		usleep_range(LINK_WAIT_IATU_MIN, LINK_WAIT_IATU_MAX);
	}
	dev_err(pci->dev, "outbound iATU is not being enabled\n");
}

static int dw_pcie_ep_outbound_atu(struct dw_pcie_ep *ep, phys_addr_t phys_addr,
    u64 pci_addr, size_t size)
{
	u32 free_win;
	struct dw_pcie *pci = to_dw_pcie_from_ep(ep);

	free_win = find_first_zero_bit(&ep->ob_window_map,
	        sizeof(ep->ob_window_map));
	if (free_win >= ep->num_ob_windows) {
		dev_err(pci->dev, "no free outbound window\n");
		return -EINVAL;
	}

	dw_pcie_prog_outbound_atu(pci, free_win, PCIE_ATU_TYPE_MEM,
	    phys_addr, pci_addr, size);

	set_bit(free_win, &ep->ob_window_map);
	ep->outbound_addr[free_win] = phys_addr;

	return 0;
}

void dw_pcie_disable_atu(struct dw_pcie *pci, int index,
    enum dw_pcie_region_type type)
{
	int region;

	switch (type) {
	case DW_PCIE_REGION_INBOUND:
		region = PCIE_ATU_REGION_INBOUND;
		break;
	case DW_PCIE_REGION_OUTBOUND:
		region = PCIE_ATU_REGION_OUTBOUND;
		break;
	default:
		return;
	}

	dw_pcie_writel_dbi(pci, PCIE_ATU_VIEWPORT, region | index);
	dw_pcie_writel_dbi(pci, PCIE_ATU_CR2, ~PCIE_ATU_ENABLE);
}

static void dw_pcie_ep_clear_bar(struct pci_epc *epc, enum pci_barno bar)
{
	struct dw_pcie_ep *ep = epc_get_drvdata(epc);
	struct dw_pcie *pci = to_dw_pcie_from_ep(ep);
	u32 atu_index = ep->bar_to_atu[bar];

	dw_pcie_ep_reset_bar(pci, bar);

	dw_pcie_disable_atu(pci, atu_index, DW_PCIE_REGION_INBOUND);
	clear_bit(atu_index, &ep->ib_window_map);
}

static int dw_pcie_ep_set_bar(struct pci_epc *epc, enum pci_barno bar,
    dma_addr_t bar_phys, size_t size, int flags)
{
	int ret;
	struct dw_pcie_ep *ep = epc_get_drvdata(epc);
	struct dw_pcie *pci = to_dw_pcie_from_ep(ep);
	enum dw_pcie_as_type as_type;
	u32 reg = PCI_BASE_ADDRESS_0 + (4 * bar);

	if (!(flags & PCI_BASE_ADDRESS_SPACE))
		as_type = DW_PCIE_AS_MEM;
	else
		as_type = DW_PCIE_AS_IO;

	ret = dw_pcie_ep_inbound_atu(ep, bar, bar_phys, as_type);
	if (ret)
		return ret;

	dw_pcie_writel_dbi2(pci, reg, size - 1);
	dw_pcie_writel_dbi(pci, reg, flags);

	return 0;
}

static int dw_pcie_find_index(struct dw_pcie_ep *ep, phys_addr_t addr,
    u32 *atu_index)
{
	u32 index;

	for (index = 0; index < ep->num_ob_windows; index++) {
		if (ep->outbound_addr[index] != addr)
			continue;
		*atu_index = index;
		return 0;
	}

	return -EINVAL;
}

static void dw_pcie_ep_unmap_addr(struct pci_epc *epc, phys_addr_t addr)
{
	int ret;
	u32 atu_index;
	struct dw_pcie_ep *ep = epc_get_drvdata(epc);
	struct dw_pcie *pci = to_dw_pcie_from_ep(ep);

	ret = dw_pcie_find_index(ep, addr, &atu_index);
	if (ret < 0)
		return;

	dw_pcie_disable_atu(pci, atu_index, DW_PCIE_REGION_OUTBOUND);
	clear_bit(atu_index, &ep->ob_window_map);
}

static int dw_pcie_ep_map_addr(struct pci_epc *epc, phys_addr_t addr,
    u64 pci_addr, size_t size)
{
	int ret;
	struct dw_pcie_ep *ep = epc_get_drvdata(epc);
	struct dw_pcie *pci = to_dw_pcie_from_ep(ep);

	ret = dw_pcie_ep_outbound_atu(ep, addr, pci_addr, size);
	if (ret) {
		dev_err(pci->dev, "failed to enable address\n");
		return ret;
	}

	return 0;
}

static int dw_pcie_ep_get_msi(struct pci_epc *epc)
{
	int val;
	u32 lower_addr;
	u32 upper_addr;
	struct dw_pcie_ep *ep = epc_get_drvdata(epc);
	struct dw_pcie *pci = to_dw_pcie_from_ep(ep);

	val = dw_pcie_readb_dbi(pci, MSI_MESSAGE_CONTROL);
	val = (val & MSI_CAP_MME_MASK) >> MSI_CAP_MME_SHIFT;

	lower_addr = dw_pcie_readl_dbi(pci, MSI_MESSAGE_ADDR_L32);
	upper_addr = dw_pcie_readl_dbi(pci, MSI_MESSAGE_ADDR_U32);

	if (!(lower_addr || upper_addr))
		return -EINVAL;

	return val;
}

static int dw_pcie_ep_set_msi(struct pci_epc *epc, u8 encode_int)
{
	int val;
	struct dw_pcie_ep *ep = epc_get_drvdata(epc);
	struct dw_pcie *pci = to_dw_pcie_from_ep(ep);

	val = (encode_int << MSI_CAP_MMC_SHIFT);
	dw_pcie_writew_dbi(pci, MSI_MESSAGE_CONTROL, val);

	return 0;
}

static int dw_pcie_ep_raise_irq(struct pci_epc *epc,
    enum pci_epc_irq_type type, u8 interrupt_num)
{
	struct dw_pcie_ep *ep = epc_get_drvdata(epc);

	if (!ep->ops->raise_irq)
		return -EINVAL;

	return ep->ops->raise_irq(ep, type, interrupt_num);
}

static void dw_pcie_ep_stop(struct pci_epc *epc)
{
	struct dw_pcie_ep *ep = epc_get_drvdata(epc);
	struct dw_pcie *pci = to_dw_pcie_from_ep(ep);

	if (!pci->ops->stop_link)
		return;

	pci->ops->stop_link(pci);
}

static int dw_pcie_ep_start(struct pci_epc *epc)
{
	struct dw_pcie_ep *ep = epc_get_drvdata(epc);
	struct dw_pcie *pci = to_dw_pcie_from_ep(ep);

	if (!pci->ops->start_link)
		return -EINVAL;

	return pci->ops->start_link(pci);
}

static const struct pci_epc_ops epc_ops = {
	.write_header       = dw_pcie_ep_write_header,
	.set_bar        = dw_pcie_ep_set_bar,
	.clear_bar      = dw_pcie_ep_clear_bar,
	.map_addr       = dw_pcie_ep_map_addr,
	.unmap_addr     = dw_pcie_ep_unmap_addr,
	.set_msi        = dw_pcie_ep_set_msi,
	.get_msi        = dw_pcie_ep_get_msi,
	.raise_irq      = dw_pcie_ep_raise_irq,
	.start          = dw_pcie_ep_start,
	.stop           = dw_pcie_ep_stop,
};

void dw_pcie_ep_exit(struct dw_pcie_ep *ep)
{
	struct pci_epc *epc = ep->epc;

	pci_epc_mem_exit(epc);
}

void dw_pcie_setup(struct dw_pcie *pci)
{
	int ret;
	u32 val;
	u32 lanes;
	struct device *dev = pci->dev;
	struct device_node *np = dev->of_node;

	ret = of_property_read_u32(np, "num-lanes", &lanes);
	if (ret)
		lanes = 0;

	/* set the number of lanes */
	val = dw_pcie_readl_dbi(pci, PCIE_PORT_LINK_CONTROL);
	val &= ~PORT_LINK_MODE_MASK;
	switch (lanes) {
	case 1:
		val |= PORT_LINK_MODE_1_LANES;
		break;
	case 2:
		val |= PORT_LINK_MODE_2_LANES;
		break;
	case 4:
		val |= PORT_LINK_MODE_4_LANES;
		break;
	case 8:
		val |= PORT_LINK_MODE_8_LANES;
		break;
	default:
		dev_err(pci->dev, "num-lanes %u: invalid value\n", lanes);
		return;
	}
	dw_pcie_writel_dbi(pci, PCIE_PORT_LINK_CONTROL, val);

	/* set link width speed control register */
	val = dw_pcie_readl_dbi(pci, PCIE_LINK_WIDTH_SPEED_CONTROL);
	val &= ~PORT_LOGIC_LINK_WIDTH_MASK;
	switch (lanes) {
	case 1:
		val |= PORT_LOGIC_LINK_WIDTH_1_LANES;
		break;
	case 2:
		val |= PORT_LOGIC_LINK_WIDTH_2_LANES;
		break;
	case 4:
		val |= PORT_LOGIC_LINK_WIDTH_4_LANES;
		break;
	case 8:
		val |= PORT_LOGIC_LINK_WIDTH_8_LANES;
		break;
	}
	dw_pcie_writel_dbi(pci, PCIE_LINK_WIDTH_SPEED_CONTROL, val);
}

int dw_pcie_ep_init(struct dw_pcie_ep *ep)
{
	int ret;
	void *addr;
	enum pci_barno bar;
	struct pci_epc *epc;
	struct dw_pcie *pci = to_dw_pcie_from_ep(ep);
	struct device *dev = pci->dev;
	struct device_node *np = dev->of_node;

	if (!pci->dbi_base || !pci->dbi_base2) {
		dev_err(dev, "dbi_base/deb_base2 is not populated\n");
		return -EINVAL;
	}

	ret = of_property_read_u32(np, "num-ib-windows", &ep->num_ib_windows);
	if (ret < 0) {
		dev_err(dev, "unable to read *num-ib-windows* property\n");
		return ret;
	}

	ret = of_property_read_u32(np, "num-ob-windows", &ep->num_ob_windows);
	if (ret < 0) {
		dev_err(dev, "unable to read *num-ob-windows* property\n");
		return ret;
	}

	addr = devm_kzalloc(dev, sizeof(phys_addr_t) * ep->num_ob_windows,
	        GFP_KERNEL);
	if (!addr)
		return -ENOMEM;
	ep->outbound_addr = addr;

	for (bar = BAR_0; bar <= BAR_5; bar++)
		dw_pcie_ep_reset_bar(pci, bar);

	if (ep->ops->ep_init)
		ep->ops->ep_init(ep);

	epc = devm_pci_epc_create(dev, &epc_ops);
	if (IS_ERR(epc)) {
		dev_err(dev, "failed to create epc device\n");
		return PTR_ERR(epc);
	}

	ret = of_property_read_u8(np, "max-functions", &epc->max_functions);
	if (ret < 0)
		epc->max_functions = 1;

	ret = pci_epc_mem_init(epc, ep->phys_base, ep->addr_size);
	if (ret < 0) {
		dev_err(dev, "Failed to initialize address space\n");
		return ret;
	}

	ep->epc = epc;
	epc_set_drvdata(epc, ep);
	dw_pcie_setup(pci);

	return 0;
}
