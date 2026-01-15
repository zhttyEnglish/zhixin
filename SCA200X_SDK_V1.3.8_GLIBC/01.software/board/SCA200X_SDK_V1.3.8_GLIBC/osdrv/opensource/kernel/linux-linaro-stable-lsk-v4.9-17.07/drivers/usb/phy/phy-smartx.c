/*
 * phy-keystone - USB PHY, talking to dwc3 controller in Keystone.
 *
 * Copyright (C) 2013 Texas Instruments Incorporated - http://www.ti.com
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Author: WingMan Kwok <w-kwok2@ti.com>
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/usb/usb_phy_generic.h>
#include <linux/io.h>
#include <linux/of.h>
#include "phy-generic.h"
#include <linux/smartchip-pinshare.h>

struct smartx_usbphy {
	void __iomem        *phy_ctrl;
	void __iomem        *tcpd_base;
	void __iomem        *comm_phy_base;
	void __iomem        *user_define_base;
};

static inline u32 smartx_usbphy_readl(void __iomem *base, u32 offset)
{
	u32     tmp;

	tmp = (u32)base;
	tmp |= (offset << 2);

	return readl((void __iomem *)tmp);
}

static inline void smartx_usbphy_writel(void __iomem *base,
    u32 offset, u32 value)
{
	u32     tmp;

	tmp = (u32)base;
	tmp |= (offset << 2);

	writel(value, (void __iomem *)tmp);
}

static int smartx_usbphy_probe(struct platform_device *pdev)
{
	struct device           *dev = &pdev->dev;
	struct smartx_usbphy    *s_phy;
	struct resource         *res;
	void __iomem            *pinshare_io_addr;
	int                     ret;
	u32                     val;

	printk(KERN_ERR"smartx_usbphy_probe 111 ====================\n");

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);

	pinshare_io_addr = devm_ioremap_resource(dev, res);

	printk(KERN_ERR"real address: 0x%08x\n", res->start);

	printk(KERN_ERR"virtual address: 0x%08x\n", pinshare_io_addr);

	printk(KERN_ERR"0x603b200c: 0x%08x\n", *((u32 *)((u32)pinshare_io_addr + 0x7200C)));

#if 0
	s_phy = devm_kzalloc(dev, sizeof(*s_phy), GFP_KERNEL);
	if (!s_phy)
		return -ENOMEM;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);

	printk(KERN_ERR"platform_get_resource start: %08x\n", res->start);

	s_phy->phy_ctrl = devm_ioremap_resource(dev, res);
	s_phy->tcpd_base = s_phy->phy_ctrl;
	s_phy->comm_phy_base = s_phy->phy_ctrl + 0x40000;
	s_phy->user_define_base = s_phy->phy_ctrl + 0x80000;

	printk(KERN_ERR"before pinshare --------!!!!!!!\n");

	smartchip_pinshare(84, 1);

	printk(KERN_ERR"after pinshare --------!!!!!!!\n");

	pinshare_io_addr = ioremap(0x40440000, 12);

	printk(KERN_ERR"pinshare addr: %08x --------!!!!!!!\n", pinshare_io_addr);

	writel(0x0, pinshare_io_addr + 8);
	writel(0x4, pinshare_io_addr + 4);
	writel(0x4, pinshare_io_addr);

	printk(KERN_ERR"after enable pinshare --------!!!!!!!\n");

	/* Read TCPC cpu reset field */
	val = readl(s_phy->tcpd_base + 0x0100);
	/* TCPC cpu reset de-assert */
	writel((val & 0xfffffffe), s_phy->tcpd_base + 0x0100);

	/* SFR_REG_0 */
	val = readl(s_phy->tcpd_base + 0x0100);
	while(((val>>13)&0x01) == 1)
	{
		val = readl(s_phy->tcpd_base + 0x0100);
	}

	/* POWER_STATUS */
	val = readl(s_phy->tcpd_base + 0x4c);
	while(((val>>6)&0x01) == 1)
	{
		val = readl(s_phy->tcpd_base + 0x4c);
	}

	/* ROLE_CONTROL */
	writel(0x00000005, s_phy->tcpd_base + 0x3c);

	/* TYPEC_CTRL0 */
	writel(0x00000490, s_phy->user_define_base + 0x04);

	smartx_usbphy_writel(s_phy->comm_phy_base, 0xc800, 0x00000830);
	smartx_usbphy_writel(s_phy->comm_phy_base, 0x40f2, 0x00000090);
	smartx_usbphy_writel(s_phy->comm_phy_base, 0x4122, 0x00000960);
	smartx_usbphy_writel(s_phy->comm_phy_base, 0x4123, 0x00000030);
	smartx_usbphy_writel(s_phy->comm_phy_base, 0x42f2, 0x00000090);
	smartx_usbphy_writel(s_phy->comm_phy_base, 0x4322, 0x00000960);
	smartx_usbphy_writel(s_phy->comm_phy_base, 0x4323, 0x00000030);
	smartx_usbphy_writel(s_phy->comm_phy_base, 0x44f2, 0x00000090);
	smartx_usbphy_writel(s_phy->comm_phy_base, 0x4522, 0x00000960);
	smartx_usbphy_writel(s_phy->comm_phy_base, 0x4523, 0x00000030);
	smartx_usbphy_writel(s_phy->comm_phy_base, 0x46f2, 0x00000090);
	smartx_usbphy_writel(s_phy->comm_phy_base, 0x4722, 0x00000960);
	smartx_usbphy_writel(s_phy->comm_phy_base, 0x4723, 0x00000030);

	smartx_usbphy_writel(s_phy->comm_phy_base, 0x0084, 0x000000f0);
	smartx_usbphy_writel(s_phy->comm_phy_base, 0x0085, 0x00000018);
	smartx_usbphy_writel(s_phy->comm_phy_base, 0x0094, 0x000000d0);
	smartx_usbphy_writel(s_phy->comm_phy_base, 0x0095, 0x00004a4a);
	smartx_usbphy_writel(s_phy->comm_phy_base, 0x0096, 0x00000034);
	smartx_usbphy_writel(s_phy->comm_phy_base, 0x0098, 0x000001ee);
	smartx_usbphy_writel(s_phy->comm_phy_base, 0x0099, 0x00007f03);
	smartx_usbphy_writel(s_phy->comm_phy_base, 0x0097, 0x00000020);
	smartx_usbphy_writel(s_phy->comm_phy_base, 0x01c2, 0x00000000);
	smartx_usbphy_writel(s_phy->comm_phy_base, 0x01c0, 0x00000000);
	smartx_usbphy_writel(s_phy->comm_phy_base, 0x01c1, 0x00000000);
	smartx_usbphy_writel(s_phy->comm_phy_base, 0x01c5, 0x00000007);
	smartx_usbphy_writel(s_phy->comm_phy_base, 0x01c6, 0x00000045);
	smartx_usbphy_writel(s_phy->comm_phy_base, 0x01c7, 0x00000008);
	smartx_usbphy_writel(s_phy->comm_phy_base, 0xc009, 0x00001200);

	val = smartx_usbphy_readl(s_phy->comm_phy_base, 0x01e0);
	smartx_usbphy_writel(s_phy->comm_phy_base, 0x01e0, 0x000000f0);
	smartx_usbphy_writel(s_phy->comm_phy_base, 0xc008, 0x00000104);

	smartx_usbphy_writel(s_phy->comm_phy_base, 0x00a4, 0x000000f0);
	smartx_usbphy_writel(s_phy->comm_phy_base, 0x00a5, 0x00000018);
	smartx_usbphy_writel(s_phy->comm_phy_base, 0x00a1, 0x000030b9);
	smartx_usbphy_writel(s_phy->comm_phy_base, 0x00b4, 0x00000087);
	smartx_usbphy_writel(s_phy->comm_phy_base, 0x00b5, 0x00000000);
	smartx_usbphy_writel(s_phy->comm_phy_base, 0x00b6, 0x00000005);
	smartx_usbphy_writel(s_phy->comm_phy_base, 0x00b8, 0x00000035);
	smartx_usbphy_writel(s_phy->comm_phy_base, 0x00b9, 0x00007f1e);
	smartx_usbphy_writel(s_phy->comm_phy_base, 0x00b7, 0x00000020);
	smartx_usbphy_writel(s_phy->comm_phy_base, 0x0037, 0x00000000);
	smartx_usbphy_writel(s_phy->comm_phy_base, 0x01d2, 0x00000000);
	smartx_usbphy_writel(s_phy->comm_phy_base, 0x01d0, 0x00000000);
	smartx_usbphy_writel(s_phy->comm_phy_base, 0x01d1, 0x00000000);
	smartx_usbphy_writel(s_phy->comm_phy_base, 0x4001, 0x0000befc);
	smartx_usbphy_writel(s_phy->comm_phy_base, 0x01d5, 0x00000006);
	smartx_usbphy_writel(s_phy->comm_phy_base, 0x01d6, 0x00000045);
	smartx_usbphy_writel(s_phy->comm_phy_base, 0x01d7, 0x00000008);
	smartx_usbphy_writel(s_phy->comm_phy_base, 0x01d8, 0x00000100);
	smartx_usbphy_writel(s_phy->comm_phy_base, 0x01d9, 0x00000007);
	smartx_usbphy_writel(s_phy->comm_phy_base, 0x4100, 0x00007799);
	smartx_usbphy_writel(s_phy->comm_phy_base, 0x4101, 0x00007798);
	smartx_usbphy_writel(s_phy->comm_phy_base, 0x4102, 0x00005098);
	smartx_usbphy_writel(s_phy->comm_phy_base, 0x4103, 0x00005098);
	smartx_usbphy_writel(s_phy->comm_phy_base, 0x4050, 0x0000001d);
	smartx_usbphy_writel(s_phy->comm_phy_base, 0x40e8, 0x000000bf);
	smartx_usbphy_writel(s_phy->comm_phy_base, 0x8200, 0x0000a6fd);
	smartx_usbphy_writel(s_phy->comm_phy_base, 0x8201, 0x0000a6fd);
	smartx_usbphy_writel(s_phy->comm_phy_base, 0x8202, 0x0000a410);
	smartx_usbphy_writel(s_phy->comm_phy_base, 0x8203, 0x00002410);
	smartx_usbphy_writel(s_phy->comm_phy_base, 0x8206, 0x000023ff);
	smartx_usbphy_writel(s_phy->comm_phy_base, 0x8290, 0x00000013);
	smartx_usbphy_writel(s_phy->comm_phy_base, 0x83dc, 0x00001004);
	smartx_usbphy_writel(s_phy->comm_phy_base, 0x8207, 0x00002010);
	smartx_usbphy_writel(s_phy->comm_phy_base, 0x42e8, 0x000000fb);

	/* clk valid */
	writel(0x00001531, s_phy->user_define_base + 0x0008);

	/* cmn_rdy */
	val = smartx_usbphy_readl(s_phy->comm_phy_base, 0xc800);
	while( (val&0x01) == 0)
	{
		val = smartx_usbphy_readl(s_phy->comm_phy_base, 0xc800);

		/* debugging pll0_sm_state */
		smartx_usbphy_readl(s_phy->comm_phy_base, 0xc803);
	};

	/* lpm_clk_valid and aux_app_clk_125_valid */
	writel(0x0000153d, s_phy->user_define_base + 0x0008);

	/* CMD_VBUS_DEFAULT */
	writel(0x00000077, s_phy->tcpd_base + 0x005c);

	platform_set_drvdata(pdev, s_phy);
#endif
	printk(KERN_ERR"smartx_usbphy_probe -----------------\n");

	return 0;
}

static int smartx_usbphy_remove(struct platform_device *pdev)
{
	struct smartx_usbphy *s_phy = platform_get_drvdata(pdev);

	return 0;
}

static const struct of_device_id smartx_usbphy_ids[] = {
	{ .compatible = "smartchip, smartx-phy" },
	{ }
};
MODULE_DEVICE_TABLE(of, smartx_usbphy_ids);

static struct platform_driver smartx_usbphy_driver = {
	.probe          = smartx_usbphy_probe,
	.remove         = smartx_usbphy_remove,
	.driver         = {
		.name   = "smartx-usbphy",
		.of_match_table = smartx_usbphy_ids,
	},
};

module_platform_driver(smartx_usbphy_driver);

MODULE_ALIAS("platform:smartx-usbphy");
MODULE_AUTHOR("SmartChip Electronics Inc.");
MODULE_DESCRIPTION("Candence USB phy driver");
MODULE_LICENSE("GPL v2");
