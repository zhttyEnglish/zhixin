/**
 * cdns_drd_pci.c - Cadence USB3 DRD Controller PCIe wrapper
 *
 * Copyright (C) 2016 Cadence Design Systems - http://www.cadence.com
 *
 * Authors: Pawel Jez <pjez@cadence.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2  of
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
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/pci.h>
#include <linux/platform_device.h>

/**
 * cdn_pci_otg_probe - Probe function for Cadence USB wrapper driver
 * @pdev: platform device object
 * @id: pci device id
 *
 * Returns 0 on success otherwise negative errno
 */
static int cdn_pci_otg_probe(struct pci_dev *pdev,
    const struct pci_device_id *id)
{

	int rv = 0;
	struct platform_device *cdns;
	struct resource res[2];

	if (!id)
		return -EINVAL;

	if (pci_enable_device(pdev) < 0)
		return -EIO;

	pci_set_master(pdev);

	/* for OTG function number is 1 */
	if (pdev->devfn != 1)
		return (-EIO);

	/*
	 * function 0: device(BAR_2) + host(BAR_0)
	 * function 1: otg(BAR_0))
	 */
	cdns = platform_device_alloc("cdns-drd-tc", PLATFORM_DEVID_AUTO);
	memset(res, 0x00, sizeof(struct resource) * ARRAY_SIZE(res));

	dev_emerg(&pdev->dev, "Initialize OTG\n");
	res[0].start = pdev->resource[0].start;
	res[0].end = pdev->resource[0].end;

	res[0].name = "cdns_usb3";
	res[0].flags = IORESOURCE_MEM;

	/* Interrupt common for both device and XHCI */
	res[1].start = pdev->irq;
	res[1].name = "cdns_usb3";
	res[1].flags = IORESOURCE_IRQ;

	rv = platform_device_add_resources(cdns, res, ARRAY_SIZE(res));
	if (rv) {
		dev_err(&pdev->dev, "couldn't add resources to cdns device\n");
		return rv;
	}

	pdev->dev.parent->platform_data = cdns;
	cdns->dev.parent = &pdev->dev;

	pci_set_drvdata(pdev, cdns);

	rv = platform_device_add(cdns);
	if (rv)
		dev_err(&pdev->dev, "failed to register cdns device\n");

	return rv;
}

/**
 * cdn_pci_otg_remove - unregister Cadence device
 * @pdev: platform device object
 */
void cdn_pci_otg_remove(struct pci_dev *pdev)
{
	platform_device_unregister(pci_get_drvdata(pdev));
}

#define PCI_VENDOR_ID_CDZ 0x17CD
static const struct pci_device_id cdn_pci_ids[] = {
	{PCI_VDEVICE(CDZ, 0x0100), },
	{ /* terminate list */}
};

static struct pci_driver cdns_pci_otg_driver = {
	.name = "cdn-pci-usb-otg",
	.id_table = &cdn_pci_ids[0],
	.probe = cdn_pci_otg_probe,
	.remove = cdn_pci_otg_remove,
};

module_pci_driver(cdns_pci_otg_driver);
MODULE_DEVICE_TABLE(pci, cdn_pci_ids);
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Cadence USB Super Speed DRD PCI wrapper");
