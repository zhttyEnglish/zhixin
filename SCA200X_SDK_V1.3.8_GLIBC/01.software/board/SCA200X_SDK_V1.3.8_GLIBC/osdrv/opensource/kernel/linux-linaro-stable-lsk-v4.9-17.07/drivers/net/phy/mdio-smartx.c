/*
 * SmartChip Smartx GMAC MDIO interface driver
 *
 * Copyright 2017 Robin Bai <haibin.bai@smartchip.cn>
 *
 * Based on the Linux driver provided by Beyond Ethernet Controller
*/

#include <linux/delay.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/of_address.h>
#include <linux/of_mdio.h>
#include <linux/phy.h>
#include <linux/platform_device.h>
#include <linux/regulator/consumer.h>

#define SMARTX_ETH_RST_CTRL   0x1004

#define SMARTX_MIICOMMAND (4 * 0x0B)
#define SMARTX_MIIADDRESS (4 * 0x0C)
#define SMARTX_MIITX_DATA (4 * 0x0D)
#define SMARTX_MIIRX_DATA (4 * 0x0E)
#define SMARTX_MIISTATUS  (4 * 0x0F)

/* MII Mode Register */
#define SMARTX_MIIMODER_CLKDIV    0x000000FF /* Clock Divider */
#define SMARTX_MIIMODER_NOPRE     0x00000100 /* No Preamble */
#define SMARTX_MIIMODER_RST       0x00000200 /* MIIM Reset */

/* MII Command Register */
#define SMARTX_MIICOMMAND_SCANSTAT  0x00000001 /* Scan Status */
#define SMARTX_MIICOMMAND_RSTAT     0x00000002 /* Read Status */
#define SMARTX_MIICOMMAND_WCTRLDATA 0x00000004 /* Write Control Data */

/* MII Address Register */
#define SMARTX_MIIADDRESS_FIAD    0x0000001F /* PHY Address */
#define SMARTX_MIIADDRESS_RGAD    0x00001F00 /* RGAD Address */

/* MII Status Register */
#define SMARTX_MIISTATUS_LINKFAIL 0x00000001 /* Link Fail */
#define SMARTX_MIISTATUS_BUSY     0x00000002 /* MII Busy */
#define SMARTX_MIISTATUS_INVALID   0x00000004 /* Data in MII Status Register is invalid */

struct smartx_mdio_data {
	void __iomem *membase;
	//TBD
};

static inline void smartx_set_miiaddress(void *baseaddr, u32 val)
{
	writel(val, baseaddr + SMARTX_MIIADDRESS);
}

static inline void smartx_set_miitx_data(void *baseaddr, u32 val)
{
	writel(val, baseaddr + SMARTX_MIITX_DATA);
}

static inline u32 smartx_get_miirx_data(void *baseaddr)
{
	return readl(baseaddr + SMARTX_MIIRX_DATA);
}

static inline void smartx_set_miicommand(void *baseaddr, u32 val)
{
	writel(val, baseaddr + SMARTX_MIICOMMAND);
}

static inline u32 smartx_get_miicommand(void *baseaddr)
{
	return readl(baseaddr + SMARTX_MIICOMMAND);
}

static inline u32 smartx_get_miistatus(void *baseaddr)
{
	return readl(baseaddr + SMARTX_MIISTATUS);
}

/*--------------------------------------------------[ Generic MII support ]---*/
static int smartx_mdio_read(struct mii_bus *bus, int phy_addr, int regnum)
{
	struct smartx_mdio_data *data = bus->priv;
	u32 st, cmd;

	/* Timeout 1s for this command */
	unsigned long timeout = jiffies + HZ;

	smartx_set_miiaddress(data->membase, (regnum << 8) | phy_addr);
	smartx_set_miicommand(data->membase, SMARTX_MIICOMMAND_RSTAT);

	do {
		st = smartx_get_miistatus(data->membase);

		if(time_after(jiffies, timeout)) {
			/* Check mii status once more, just to
			   be sure that timeout really occourred,
			   since we are not using any IRQ locking. */
			st = smartx_get_miistatus(data->membase);
			if ((st & SMARTX_MIISTATUS_BUSY) == 0)
				break;

			cmd = smartx_get_miicommand(data->membase);
			printk(KERN_ERR "MDIO read timeout (cmd: 0x%08x, "\
			    "status: 0x%08x)\n", cmd, st);
			return -EIO;
		}
	} while(st & SMARTX_MIISTATUS_BUSY);

	st = smartx_get_miirx_data(data->membase);

	//printk("smartx mdio read: %x %x %x\n", phy_addr, regnum, st);
	return st;
}

static int smartx_mdio_write(struct mii_bus *bus, int phy_addr, int regnum,
    u16 val)
{
	struct smartx_mdio_data   *data = bus->priv;
	u32 st, cmd;

	/* Timeout 1s for this command */
	unsigned long timeout = jiffies + HZ;

	//printk("smartx mdio write: %x %x %x\n", phy_addr, regnum, val);

	smartx_set_miiaddress(data->membase, (regnum << 8) | phy_addr);
	smartx_set_miitx_data(data->membase, val);
	smartx_set_miicommand(data->membase, SMARTX_MIICOMMAND_WCTRLDATA);

	do {
		st = smartx_get_miistatus(data->membase);

		if(time_after(jiffies, timeout)) {
			/* Check mii status once more, just to
			   be sure that timeout really occourred,
			   since we are not using any IRQ locking. */
			st = smartx_get_miistatus(data->membase);
			if ((st & SMARTX_MIISTATUS_BUSY) == 0)
				break;

			cmd = smartx_get_miicommand(data->membase);
			printk(KERN_ERR "MDIO write timeout (cmd: 0x%08x, "\
			    "status: 0x%08x)\n", cmd, st);
			return -EIO;
		}
	} while(st & SMARTX_MIISTATUS_BUSY);

	return 0;
}

/*
 * phy hard reset
 */
static void smartx_mdio_phy_reset(struct mii_bus *bus)
{
	struct smartx_mdio_data *data = bus->priv;
	unsigned int reg;

	pr_info("mdio reset phy\n");
	reg = readl(data->membase + SMARTX_ETH_RST_CTRL);

	/* high */
	writel(reg | 0x1, data->membase + SMARTX_ETH_RST_CTRL);
	msleep(5);

	/* low: RTL8211F/RTL8201F > 10ms */
	writel(reg & 0xfffffffe, data->membase + SMARTX_ETH_RST_CTRL);
	msleep(11);

	/* high: RTL8211F > 30ms, RTL8201 > 150ms */
	writel(reg | 0x1, data->membase + SMARTX_ETH_RST_CTRL);
	msleep(150);
}

static int smartx_mdio_probe(struct platform_device *pdev)
{
	struct device_node *np = pdev->dev.of_node;
	struct mii_bus *bus;
	struct resource *res;
	struct smartx_mdio_data *data;
	int ret;

	pr_info("Smartx mdio probe...\n");
	bus = mdiobus_alloc_size(sizeof(*data));
	if(!bus)
		return -ENOMEM;

	bus->name = "smartx_mii_bus";
	bus->read = smartx_mdio_read;
	bus->write = smartx_mdio_write;
	bus->phy_hw_reset = smartx_mdio_phy_reset;
	snprintf(bus->id, MII_BUS_ID_SIZE, "%s-mii", dev_name(&pdev->dev));
	bus->parent = &pdev->dev;

	data = bus->priv;
	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	data->membase = devm_ioremap_resource(&pdev->dev, res);
	if(IS_ERR(data->membase)) {
		ret = PTR_ERR(data->membase);
		goto err_out_free_mdiobus;
	}

	smartx_mdio_phy_reset(bus);

	ret = of_mdiobus_register(bus, np);
	if(ret < 0)
		goto err_out_free_mdiobus;

	platform_set_drvdata(pdev, bus);

	pr_info("smartx mdio register succussfully.\n");
	return 0;

err_out_free_mdiobus:
	mdiobus_free(bus);
	return ret;
}

static int smartx_mdio_remove(struct platform_device *pdev)
{
	struct mii_bus *bus = platform_get_drvdata(pdev);

	//kfree(bus->irq);
	mdiobus_unregister(bus);
	mdiobus_free(bus);

	pr_info("smartx mdio remove succussfully.\n");
	return 0;
}

static const struct of_device_id smartx_mdio_of_match[] = {
	{.compatible = "smartchip,smartx-gmac-mdio",},
	{},
};
MODULE_DEVICE_TABLE(of, smartx_mdio_of_match);

static struct platform_driver smartx_mdio_driver = {
	.probe = smartx_mdio_probe,
	.remove = smartx_mdio_remove,
	.resume = NULL,
	.suspend = NULL,
	.driver = {
		.name = "smartx-mdio",
		.of_match_table = smartx_mdio_of_match,
		.owner = THIS_MODULE,
	},
};

module_platform_driver(smartx_mdio_driver);

MODULE_AUTHOR("Robin bai <haibin.bai@smartchip.cn>");
MODULE_DESCRIPTION("SmartChip Smartx GMAC MDIO driver based on beth");
MODULE_LICENSE("GPL");

