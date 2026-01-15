/*
 * PCIe host controller driver for SmartChip SoCs
 *
 * Copyright (C) 2017 SmartChip Co., Ltd.
 *
 * Authors: Kai Qiu <kai.qiu@smartchip.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/interrupt.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/of_device.h>
#include <linux/of_gpio.h>
#include <linux/pci.h>
#include <linux/platform_device.h>
#include <linux/phy/phy.h>
#include <linux/resource.h>
#include <linux/signal.h>
#include <linux/types.h>

#include "pcie-designware.h"

#define to_dw_pcie(x)       container_of(x, struct dw_pcie, pp)
#define to_sca200v100_pcie(x)   dev_get_drvdata((x)->dev)
#define to_dw_pcie_from_ep(endpoint)   \
        container_of((endpoint), struct dw_pcie, ep)

// elba_base = 0x64200000
#define PCIE_PWRUP_RESET                0x80040
#define PCIE_PWRUP_RESET_ENABLE         BIT(8)
#define PCIE_ELBI_SLV_AWMISC            0x80020
#define PCIE_ELBI_SLV_ARMISC            0x8002c
#define PCIE_ELBI_SLV_DBI_ENABLE        BIT(21)

#define ELBI_INTX_ASSERT_BASE           0x0918
#define ELBI_INTA_ASSERT                (ELBI_INTX_ASSERT_BASE)
#define ELBI_INTB_ASSERT                (ELBI_INTX_ASSERT_BASE + 0x04)
#define ELBI_INTC_ASSERT                (ELBI_INTX_ASSERT_BASE + 0x08)
#define ELBI_INTD_ASSERT                (ELBI_INTX_ASSERT_BASE + 0x0c)
#define ELBI_INTX_DEASSERT_BASE         0x0928
#define ELBI_INTA_DEASSERT              (ELBI_INTX_DEASSERT_BASE)
#define ELBI_INTB_DEASSERT              (ELBI_INTX_DEASSERT_BASE + 0x04)
#define ELBI_INTC_DEASSERT              (ELBI_INTX_DEASSERT_BASE + 0x08)
#define ELBI_INTD_DEASSERT              (ELBI_INTX_DEASSERT_BASE + 0x0c)
#define ELBI_HP_PME                     (ELBI_INTX_DEASSERT_BASE + 0x10)
#define ELBI_HP_MSI                     (ELBI_INTX_DEASSERT_BASE + 0x14)

#define PCIE_APP_LTSSM_ENABLE           0x800d8
#define PCIE_ELBI_LTSSM_ENABLE          (0x1 << 2)
#define PCIE_DEVICE_TYPE_OFFSET         3
#define PCIE_DEVICE_TYPE_MASK           (0xf << PCIE_DEVICE_TYPE_OFFSET)
#define PCIE_DEVICE_TYPE_EP             (0x0 << PCIE_DEVICE_TYPE_OFFSET)
#define PCIE_DEVICE_TYPE_LEGACY_EP      (0x1 << PCIE_DEVICE_TYPE_OFFSET)
#define PCIE_DEVICE_TYPE_RC             (0x4 << PCIE_DEVICE_TYPE_OFFSET)
#define PCIE_APP_REQ_RETRY_EN           (1 << 7)

struct sc_pcie {
	struct dw_pcie  pci;
	//struct pcie_port    pp;
	void __iomem    *elbi_base;
	void __iomem    *phy_base;
	void __iomem    *block_base;
	int     reset_gpio;
	struct clk  *clk;
	struct clk  *bus_clk;
};

struct sc_pcie_ops {
	int (*get_mem_resources)(struct platform_device *pdev,
	    struct sc_pcie *ep);
	int (*get_clk_resources)(struct sc_pcie *ep);
	int (*init_clk_resources)(struct sc_pcie *ep);
	void (*deinit_clk_resources)(struct sc_pcie *ep);
};

#if 0
static int ar9101_pcie_get_clk_resources(struct sc_pcie *ep)
{
	//TODO:
	return 0;
}

static int ar9101_pcie_init_clk_resources(struct sc_pcie *ep)
{
	//TODO:
	return 0;
}

static void ar9101_pcie_deinit_clk_resources(struct sc_pcie *ep)
{
	//TODO:
}

static const struct sc_pcie_ops ar9101_pcie_ops = {
	.get_mem_resources  = ar9101_pcie_get_mem_resources,
	.get_clk_resources  = ar9101_pcie_get_clk_resources,
	.init_clk_resources = ar9101_pcie_init_clk_resources,
	.deinit_clk_resources   = ar9101_pcie_deinit_clk_resources,
};
#endif

static void sc_pcie_writel(void __iomem *base, u32 val, u32 reg)
{
	writel(val, base + reg);
}

static u32 sc_pcie_readl(void __iomem *base, u32 reg)
{
	return readl(base + reg);
}

static void sc_pcie_assert_core_reset(struct sc_pcie *sc_pcie)
{
	/*
	 * Cold Reset Sequence
	 */
	u32 val;
	val = sc_pcie_readl(sc_pcie->elbi_base, PCIE_PWRUP_RESET);
	val &= ~PCIE_PWRUP_RESET_ENABLE;
	sc_pcie_writel(sc_pcie->elbi_base, val, PCIE_PWRUP_RESET);
}

static int sc_pcie_establish_link(struct sc_pcie *sc_pcie)
{
	struct dw_pcie *pci = &sc_pcie->pci;
	struct pcie_port *pp = &pci->pp;
	struct device *dev = pp->dev;

	if (dw_pcie_link_up(pp)) {
		dev_err(dev, "Link already up\n");
		return 0;
	}

	//TODO:
	sc_pcie_assert_core_reset(sc_pcie);

	// TODO: yet another common reset

	dw_pcie_setup_rc(pp);

	/* assert LTSSM enable */
	sc_pcie_writel(sc_pcie->elbi_base, PCIE_ELBI_LTSSM_ENABLE,
	    PCIE_APP_LTSSM_ENABLE);

	/* check if the link is up or not */
	if (!dw_pcie_wait_for_link(pp))
		return 0;

	//TODO: check PLL lock
	return 0;
}

static void sc_pcie_enable_irq_pulse(struct sc_pcie *ep)
{
	/*
	 * Have no module specific irq enable signal
	 */
}

static void sc_pcie_clear_irq_pulse(struct sc_pcie *sc_pcie)
{
	u32 val, idx;
	struct dw_pcie *pci = &sc_pcie->pci;
	struct pcie_port *pp = &pci->pp;

	for (idx = 0; idx < 4; idx++) {
		val = sc_pcie_readl(sc_pcie->elbi_base, ELBI_INTX_ASSERT_BASE + idx * 4);
		if (val)
			dev_dbg(pp->dev, "legacy [%d] asserted\n", idx);
	}

	for (idx = 0; idx < 4; idx++) {
		val = sc_pcie_readl(sc_pcie->elbi_base, ELBI_INTX_DEASSERT_BASE + idx * 4);
		if (val)
			dev_dbg(pp->dev, "legacy [%d] deassert\n", idx);
	}
}

static irqreturn_t sc_pcie_irq_handler(int irq, void *arg)
{
	struct sc_pcie *sc_pcie = arg;

	sc_pcie_clear_irq_pulse(sc_pcie);

	return IRQ_HANDLED;
}

static void sc_pcie_sideband_dbi_w_mode(struct sc_pcie *sc_pcie, bool on)
{
	u32 val;

	val = sc_pcie_readl(sc_pcie->elbi_base, PCIE_ELBI_SLV_AWMISC);

	if (on)
		val |= PCIE_ELBI_SLV_DBI_ENABLE;
	else
		val &= ~PCIE_ELBI_SLV_DBI_ENABLE;
	sc_pcie_writel(sc_pcie->elbi_base, val, PCIE_ELBI_SLV_AWMISC);
}

static void sc_pcie_sideband_dbi_r_mode(struct sc_pcie *sc_pcie,
    bool on)
{
	u32 val;
	val = sc_pcie_readl(sc_pcie->elbi_base, PCIE_ELBI_SLV_ARMISC);

	if (on)
		val |= PCIE_ELBI_SLV_DBI_ENABLE;
	else
		val &= ~PCIE_ELBI_SLV_DBI_ENABLE;
	sc_pcie_writel(sc_pcie->elbi_base, val, PCIE_ELBI_SLV_ARMISC);
}

static void sc_pcie_msi_init(struct sc_pcie *sc_pcie)
{
	//TODO:
}

static void sc_pcie_enable_interrupts(struct sc_pcie *sc_pcie)
{
	sc_pcie_enable_irq_pulse(sc_pcie);

	if (IS_ENABLED(CONFIG_PCI_MSI))
		sc_pcie_msi_init(sc_pcie);
}

static int sc_pcie_rd_own_conf(struct pcie_port *pp, int where,
    int size, u32 *val)
{
	struct dw_pcie *pci = to_dw_pcie(pp);
	struct sc_pcie *sc_pcie = to_sca200v100_pcie(pci);
	int ret;

	sc_pcie_sideband_dbi_r_mode(sc_pcie, true);
	ret = dw_pcie_read(pp->dbi_base + where, size, val);
	sc_pcie_sideband_dbi_r_mode(sc_pcie, false);

	return ret;
}

static int sc_pcie_wr_own_conf(struct pcie_port *pp, int where,
    int size, u32 val)
{
	struct dw_pcie *pci = to_dw_pcie(pp);
	struct sc_pcie *sc_pcie = to_sca200v100_pcie(pci);
	int ret;

	sc_pcie_sideband_dbi_w_mode(sc_pcie, true);
	ret = dw_pcie_write(pp->dbi_base + where, size, val);
	sc_pcie_sideband_dbi_w_mode(sc_pcie, false);

	return ret;
}

static void sc_pcie_host_init(struct pcie_port *pp)
{
	struct dw_pcie *pci = to_dw_pcie(pp);
	struct sc_pcie *sc_pcie = to_sca200v100_pcie(pci);

	sc_pcie_establish_link(sc_pcie);
	sc_pcie_enable_interrupts(sc_pcie);
}

static struct pcie_host_ops sc_pcie_host_ops = {
	.rd_own_conf = sc_pcie_rd_own_conf,
	.wr_own_conf = sc_pcie_wr_own_conf,
	.host_init = sc_pcie_host_init,
};

static int __init sc_add_pcie_port(struct sc_pcie *sc_pcie,
    struct platform_device *pdev)
{
	struct dw_pcie *pci = &sc_pcie->pci;
	struct pcie_port *pp = &pci->pp;
	struct device *dev = pci->dev;
	int ret;

	pp->irq = platform_get_irq(pdev, 1);
	if (!pp->irq) {
		dev_err(dev, "failed to get AR PCIe irq\n");
		return -ENODEV;
	}
	// pp->irq = 157
	ret = devm_request_irq(dev, pp->irq, sc_pcie_irq_handler,
	        IRQF_SHARED, "sc-pcie", sc_pcie);
	if (ret) {
		dev_err(dev, "failed to request AR PCIe irq\n");
		return ret;
	}

	//TODO: add msi request if enabled

	pp->root_bus_nr = -1;
	pp->ops = &sc_pcie_host_ops;

	ret = dw_pcie_host_init(pp);
	if (ret) {
		dev_err(dev, "failed to initialize host\n");
		return ret;
	}

	return 0;
}

void sca200v100_pcie_ep_init(struct dw_pcie_ep *ep)
{

}

static void sc_pcie_raise_legacy_irq(struct sc_pcie *sc_pci)
{

}

static void sc_pcie_raise_msi_irq(struct sc_pcie *sc_pci,
    int interrupt_num)
{

}

int sca200v100_pcie_raise_irq(struct dw_pcie_ep *ep,
    enum pci_epc_irq_type type, u8 interrupt_num)
{
	struct dw_pcie *pci = to_dw_pcie_from_ep(ep);
	struct sc_pcie *sc_pci = to_sca200v100_pcie(pci);

	switch (type) {
	case PCI_EPC_IRQ_LEGACY:
		sc_pcie_raise_legacy_irq(sc_pci);
		break;
	case PCI_EPC_IRQ_MSI:
		sc_pcie_raise_msi_irq(sc_pci, interrupt_num);
		break;
	default:
		dev_err(pci->dev, "UNKNOWN IRQ type\n");
	}

	return 0;
}

static struct dw_pcie_ep_ops pcie_ep_ops = {
	.ep_init = sca200v100_pcie_ep_init,
	.raise_irq = sca200v100_pcie_raise_irq,
};

static int __init sc_add_pcie_ep(struct sc_pcie *sc_pcie,
    struct platform_device *pdev)
{
	int ret;
	struct dw_pcie_ep *ep;
	struct resource *res;
	struct device *dev = &pdev->dev;
	struct dw_pcie *pci = &sc_pcie->pci;

	ep = &pci->ep;
	ep->ops = &pcie_ep_ops;

	res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "ep_dbics");
	pci->dbi_base = devm_ioremap(dev, res->start, resource_size(res));
	if (!pci->dbi_base)
		return -ENOMEM;

	res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "ep_dbics2");
	pci->dbi_base2 = devm_ioremap(dev, res->start, resource_size(res));
	if (!pci->dbi_base2)
		return -ENOMEM;

	res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "addr_space");
	if (!res)
		return -EINVAL;

	ep->phys_base = res->start;
	ep->addr_size = resource_size(res);

	ret = dw_pcie_ep_init(ep);
	if (ret) {
		dev_err(dev, "failed to initialize endpoint\n");
		return ret;
	}

	return 0;
}

struct sca200v100_pcie_of_data {
	enum dw_pcie_device_mode mode;
};

static const struct sca200v100_pcie_of_data sca200v100_pcie_rc_of_data = {
	.mode = DW_PCIE_RC_TYPE,
};

static const struct sca200v100_pcie_of_data sca200v100_pcie_ep_of_data = {
	.mode = DW_PCIE_EP_TYPE,
};

static const struct of_device_id smart_pcie_of_match[] = {
	{
		.compatible = "smartchip, sca200v100-pcie",
		.data       = &sca200v100_pcie_rc_of_data,
	},
	{
		.compatible = "smartchip, sca200v100-pcie-ep",
		.data = &sca200v100_pcie_ep_of_data,
	},
	{},
};

static int __init sc_pcie_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct sc_pcie *sc_pcie;
	struct dw_pcie *pci;
	struct pcie_port *pp;
	//struct device_node *np = dev->of_node;
	struct resource *elbi_base;
	struct resource *phy_base;
	struct resource *block_base;
	const struct sca200v100_pcie_of_data *data;
	const struct of_device_id *match;
	enum dw_pcie_device_mode mode;

	int ret;
	unsigned val;

	match = of_match_device(of_match_ptr(smart_pcie_of_match), dev);
	if (!match)
		return -EINVAL;

	data = (struct sca200v100_pcie_of_data *)match->data;
	mode = (enum dw_pcie_device_mode)data->mode;

	sc_pcie = devm_kzalloc(dev, sizeof(*sc_pcie), GFP_KERNEL);
	if (!sc_pcie)
		return -ENOMEM;

	pci = &sc_pcie->pci;
	pp = &pci->pp;
	//pp->dev = dev;
	pci->dev = dev;

	//pci->reset_gpio = of_get_named_gpio(np, "reset-gpio", 0);

	sc_pcie->clk = devm_clk_get(dev, "pcie");
	if (IS_ERR(sc_pcie->clk)) {
		dev_err(dev, "Failed to get pcie rc clock\n");
		return PTR_ERR(sc_pcie->clk);
	}
	ret = clk_prepare_enable(sc_pcie->clk);
	if (ret)
		return ret;

	sc_pcie->bus_clk = devm_clk_get(dev, "pcie_bus");
	if (IS_ERR(sc_pcie->bus_clk)) {
		dev_err(dev, "Failed to get pcie bus clock\n");
		ret = PTR_ERR(sc_pcie->bus_clk);
		goto fail_clk;
	}
	ret = clk_prepare_enable(sc_pcie->bus_clk);
	if (ret)
		goto fail_clk;

	elbi_base = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	sc_pcie->elbi_base = devm_ioremap_resource(dev, elbi_base);
	if (IS_ERR(sc_pcie->elbi_base)) {
		ret = PTR_ERR(sc_pcie->elbi_base);
		goto fail_bus_clk;
	}

	phy_base = platform_get_resource(pdev, IORESOURCE_MEM, 1);
	sc_pcie->phy_base = devm_ioremap_resource(dev, phy_base);
	if (IS_ERR(sc_pcie->phy_base)) {
		ret = PTR_ERR(sc_pcie->phy_base);
		goto fail_bus_clk;
	}

	block_base = platform_get_resource(pdev, IORESOURCE_MEM, 2);
	sc_pcie->block_base = devm_ioremap_resource(dev, block_base);
	if (IS_ERR(sc_pcie->block_base)) {
		ret = PTR_ERR(sc_pcie->block_base);
		goto fail_bus_clk;
	}

	val = sc_pcie_readl(sc_pcie->elbi_base, PCIE_APP_LTSSM_ENABLE);
	val &= ~PCIE_DEVICE_TYPE_MASK;
	switch (mode) {
	case DW_PCIE_RC_TYPE:
		sc_pcie_writel(sc_pcie->elbi_base, val | PCIE_DEVICE_TYPE_RC,
		    PCIE_APP_LTSSM_ENABLE);
		ret = sc_add_pcie_port(sc_pcie, pdev);
		if (ret < 0)
			goto fail_bus_clk;
		break;
	case DW_PCIE_EP_TYPE:
		sc_pcie_writel(sc_pcie->elbi_base, val | PCIE_DEVICE_TYPE_EP,
		    PCIE_APP_LTSSM_ENABLE);
		ret = sc_add_pcie_ep(sc_pcie, pdev);
		if (ret < 0)
			goto fail_bus_clk;
		break;
	default:
		dev_err(dev, "INVALID device type %d\n", mode);
	}

	platform_set_drvdata(pdev, sc_pcie);
	return 0;

fail_bus_clk:
	clk_disable_unprepare(sc_pcie->bus_clk);
fail_clk:
	clk_disable_unprepare(sc_pcie->clk);
	return ret;
}

static struct platform_driver smart_pcie_driver = {
	.probe = sc_pcie_probe,
	.driver = {
		.name = "smart-pcie",
		.of_match_table = smart_pcie_of_match,
	},
};
builtin_platform_driver(smart_pcie_driver);
