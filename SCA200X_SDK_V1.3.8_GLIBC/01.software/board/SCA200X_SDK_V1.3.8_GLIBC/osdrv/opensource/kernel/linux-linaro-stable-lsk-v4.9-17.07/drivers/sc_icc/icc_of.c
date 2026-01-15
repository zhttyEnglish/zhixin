/*
 * Icc of driver to parse dtb
 */

#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/of_platform.h>
#include <linux/of_address.h>
#include <linux/of_reserved_mem.h>

#include "icc_priv.h"

int icc_of_mem_init(struct platform_device *pdev)
{
	return of_reserved_mem_device_init(&pdev->dev);
}

int icc_of_peer_init(struct platform_device *pdev)
{
	int peer_cnt;
	int ret;
	struct icc_core *icc = pdev->dev.platform_data;
	struct device_node *dt_node = pdev->dev.of_node;
	struct device_node *node;

	peer_cnt = of_get_available_child_count(dt_node);
	if(!peer_cnt || peer_cnt > ICC_CORE_TOTAL) {
		return -EINVAL;
	}

	icc_debug("peer count=%d\n", peer_cnt);

	for_each_available_child_of_node(dt_node, node) {
		ret = icc_peer_create(icc, node);
		if(ret)
			return ret;
	}

	return 0;
}

#ifdef CONFIG_OF_RESERVED_MEM
#include <linux/of.h>
#include <linux/of_fdt.h>
#include <linux/of_reserved_mem.h>

static int rmem_icc_device_init(struct reserved_mem *rmem, struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct icc_core *icc_core = pdev->dev.platform_data;

	icc_core->phys_addr = rmem->base;
	icc_core->mem_size  = rmem->size;
	pr_debug("%s: icc base %pa size %pa dev %p\n", __func__,
	    &rmem->base, &rmem->size, dev);

	return 0;
}

static void rmem_icc_device_release(struct reserved_mem *rmem,
    struct device *dev)
{
	return;
}

static const struct reserved_mem_ops rmem_icc_ops = {
	.device_init    = rmem_icc_device_init,
	.device_release = rmem_icc_device_release,
};

static int __init rmem_icc_setup(struct reserved_mem *rmem)
{
	phys_addr_t size = rmem->size;

	size = size / 1024;

	pr_info("Icc memory setup at %pa size %pa KB\n",
	    &rmem->base, &size);

	rmem->ops = &rmem_icc_ops;
	return 0;
}

RESERVEDMEM_OF_DECLARE(ion, "icc-region", rmem_icc_setup);
#endif
