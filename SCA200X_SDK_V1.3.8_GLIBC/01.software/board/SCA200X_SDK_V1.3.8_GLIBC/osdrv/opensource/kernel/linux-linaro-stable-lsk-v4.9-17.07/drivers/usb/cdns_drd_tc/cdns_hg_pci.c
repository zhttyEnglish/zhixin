/**
 * cdns_hg_pci.c - Cadence USB3 device Controller PCIe wrapper
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
#include <linux/dma-mapping.h>
#include <linux/slab.h>

struct pci_hg_type {
	struct platform_device *cdns;
	struct platform_device *xhci;
};

/**
 * cdn_pci_probe - Probe function for Cadence USB wrapper driver
 * @pdev: platform device object
 * @id: pci device id
 *
 * Returns 0 on success otherwise negative errno
 */
static int cdn_pci_probe(struct pci_dev *pdev, const struct pci_device_id *id)
{
	int rv = 0;
	struct platform_device *cdns;
	struct resource res[2];
#if IS_ENABLED(CONFIG_USB_CDNS_DRD_TC)
	struct platform_device *xhci;
	struct resource res_xhci[2];
	struct pci_hg_type *pci_hg;
#endif

	if (!id)
		return -EINVAL;

	if (pci_enable_device(pdev) < 0)
		return -EIO;

	pci_set_master(pdev);

	/* for GADGET/HOST function number is 0 */
	if (pdev->devfn != 0)
		return -EIO;

	/*
	 * function 0: device(BAR_2) + host(BAR_0)
	 * function 1: otg(BAR_0))
	 */
	cdns = platform_device_alloc("usb-ss", PLATFORM_DEVID_AUTO);
	memset(res, 0x00, sizeof(struct resource) * ARRAY_SIZE(res));

	dev_emerg(&pdev->dev, "Initialize DEVICE\n");
	res[0].start = pdev->resource[2].start;
	res[0].end = pdev->resource[2].end;

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

	/*Setting DMA mask for device*/
	if (dma_set_mask(&pdev->dev, DMA_BIT_MASK(32))) {
		dev_err(&pdev->dev, "request 32bit mask failed\n");
		pci_disable_device(pdev);
		return (-EIO);
	}
	dma_set_coherent_mask(&pdev->dev, DMA_BIT_MASK(32));

	if (!cdns->dev.dma_mask) {
		cdns->dev.dma_parms = pdev->dev.dma_parms;
		cdns->dev.dma_mask = pdev->dev.dma_mask;
		dma_coerce_mask_and_coherent(&cdns->dev, DMA_BIT_MASK(32));
	}

	rv = platform_device_add(cdns);
	if (rv)
		dev_err(&pdev->dev, "failed to register cdns device\n");

#if IS_ENABLED(CONFIG_USB_CDNS_DRD_TC)
	cdns->dev.parent =
	    &((struct platform_device *)pdev->dev.parent->platform_data)->dev;
#else
	cdns->dev.parent = &pdev->dev;
#endif

#if IS_ENABLED(CONFIG_USB_CDNS_DRD_TC)
	xhci = platform_device_alloc("xhci-hcd", PLATFORM_DEVID_AUTO);
	if (!xhci) {
		dev_err(&pdev->dev, "couldn't allocate xHCI device\n");
		return -ENOMEM;
	}

	memset(res_xhci, 0x00, sizeof(res_xhci));

	dev_emerg(&pdev->dev, "Initialize XHCI\n");
	res_xhci[0].start = pdev->resource[0].start;
	res_xhci[0].end = pdev->resource[0].end;

	res_xhci[0].name = "cdns_xhci";
	res_xhci[0].flags = IORESOURCE_MEM;

	/* Interrupt common for both device and XHCI */
	res_xhci[1].start = pdev->irq;
	res_xhci[1].name = "cdns_xhci";
	res_xhci[1].flags = IORESOURCE_IRQ;

	rv = platform_device_add_resources(xhci, res_xhci,
	        ARRAY_SIZE(res_xhci));
	if (rv) {
		dev_err(&pdev->dev, "couldn't add resources to cdns device\n");
		return rv;
	}

	if (!xhci->dev.dma_mask) {
		xhci->dev.dma_parms = pdev->dev.dma_parms;
		xhci->dev.dma_mask = pdev->dev.dma_mask;
		dma_set_coherent_mask(&xhci->dev, pdev->dev.coherent_dma_mask);
	}

	rv = platform_device_add(xhci);
	if (rv) {
		dev_err(&pdev->dev, "failed to register cdns xhci\n");
		return rv;
	}

	xhci->dev.of_node = (struct device_node *)1;
	xhci->dev.parent =
	    &((struct platform_device *)pdev->dev.parent->platform_data)->dev;

	pci_hg = kmalloc(sizeof(struct pci_hg_type), GFP_KERNEL);
	if (!pci_hg)
		return -ENOMEM;

	pci_hg->cdns = cdns;
	pci_hg->xhci = xhci;
	pci_set_drvdata(pdev, pci_hg);
#else
	pci_set_drvdata(pdev, cdns);
#endif
	return rv;
}

void cdn_pci_remove(struct pci_dev *pdev)
{

#if IS_ENABLED(CONFIG_USB_CDNS_DRD_TC)
	platform_device_unregister(
	    ((struct pci_hg_type *)pci_get_drvdata(pdev))->cdns);
	platform_device_unregister(
	    ((struct pci_hg_type *)pci_get_drvdata(pdev))->xhci);
	kfree(pci_get_drvdata(pdev));
#else
	platform_device_unregister(pci_get_drvdata(pdev));
#endif
}

#define PCI_VENDOR_ID_CDZ 0x17CD
static const struct pci_device_id cdn_pci_ids[] = {
	{PCI_VDEVICE(CDZ, 0xc100), },
	{ /* terminate list */}
};

static struct pci_driver cdn_cusb_pci_driver = {
	.name = "cdn-pci-usb-hg",
	.id_table = &cdn_pci_ids[0],
	.probe = cdn_pci_probe,
	.remove = cdn_pci_remove,
};

module_pci_driver(cdn_cusb_pci_driver);
MODULE_DEVICE_TABLE(pci, cdn_pci_ids);
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Cadence USB Super Speed device driver");
