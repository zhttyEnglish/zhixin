/*
 * Synopsys Designware PCIe host controller driver
 *
 * Copyright (C) 2013 Samsung Electronics Co., Ltd.
 *      http://www.samsung.com
 *
 * Author: Jingoo Han <jg1.han@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef _PCIE_DESIGNWARE_EP_H
#define _PCIE_DESIGNWARE_EP_H

#include <linux/irq.h>
#include <linux/msi.h>
#include <linux/pci.h>

#include <linux/pci-epc.h>
#include <linux/pci-epf.h>

struct dw_pcie_ep;

struct dw_pcie_ep_ops {
	void    (*ep_init)(struct dw_pcie_ep *ep);
	int (*raise_irq)(struct dw_pcie_ep *ep, enum pci_epc_irq_type type,
	    u8 interrupt_num);
};

struct dw_pcie_ep {
	struct pci_epc      *epc;
	struct dw_pcie_ep_ops   *ops;
	phys_addr_t     phys_base;
	size_t          addr_size;
	u8          bar_to_atu[6];
	phys_addr_t     *outbound_addr;
	unsigned long       ib_window_map;
	unsigned long       ob_window_map;
	u32         num_ib_windows;
	u32         num_ob_windows;
};

struct dw_pcie;

struct dw_pcie_ops {
	u64 (*cpu_addr_fixup)(u64 cpu_addr);
	u32 (*read_dbi)(struct dw_pcie *pcie, void __iomem *base, u32 reg,
	    size_t size);
	void    (*write_dbi)(struct dw_pcie *pcie, void __iomem *base, u32 reg,
	    size_t size, u32 val);
	int (*link_up)(struct dw_pcie *pcie);
	int (*start_link)(struct dw_pcie *pcie);
	void    (*stop_link)(struct dw_pcie *pcie);
};

struct dw_pcie {
	struct device   *dev;
	void __iomem        *dbi_base;
	void __iomem        *dbi_base2;
	u32         num_viewport;
	u8          iatu_unroll_enabled;
	struct pcie_port    pp;
	struct dw_pcie_ep   ep;
	const struct dw_pcie_ops *ops;
};

enum dw_pcie_as_type {
	DW_PCIE_AS_UNKNOWN,
	DW_PCIE_AS_MEM,
	DW_PCIE_AS_IO,
};

enum dw_pcie_region_type {
	DW_PCIE_REGION_UNKNOWN,
	DW_PCIE_REGION_INBOUND,
	DW_PCIE_REGION_OUTBOUND,
};

int dw_pcie_ep_init(struct dw_pcie_ep *ep);

#endif
