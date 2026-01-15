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

#endif
