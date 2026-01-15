/*
 * SmartChip ION Driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#define pr_fmt(fmt) "Ion: " fmt

#include <linux/err.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/of.h>
#include <linux/mm.h>
#include <linux/uaccess.h>
#include <linux/scatterlist.h>
#include <linux/smartchip_ion.h>
#include <linux/sched.h>
#include <linux/compat.h>
#include <linux/sizes.h>
#include <linux/smp.h>
#include <asm/cputype.h>
#include <asm/topology.h>
#include <asm/cacheflush.h>
#include <asm/cpu.h>

#include "../ion_priv.h"
#include "../ion.h"
#include "../ion_of.h"

struct smartchip_ion_dev {
	struct ion_heap **heaps;
	struct ion_device *idev;
	struct ion_platform_data *data;
};

static struct smartchip_ion_dev *smartchip_ion_device = NULL;

static struct ion_of_heap smartchip_heaps[] = {
	/* System heap */
	PLATFORM_HEAP("smartchip,system",
	    0,
	    ION_HEAP_TYPE_SYSTEM,
	    "system_heap"),

	/* Carveout heap */
	PLATFORM_HEAP("smartchip,carveout-dsp",
	    24,
	    ION_HEAP_TYPE_CARVEOUT,
	    "carveout-dsp"),

	PLATFORM_HEAP("smartchip,carveout-m7",
	    25,
	    ION_HEAP_TYPE_CARVEOUT,
	    "carveout-m7"),

	PLATFORM_HEAP("smartchip,carveout-icc",
	    26,
	    ION_HEAP_TYPE_CARVEOUT,
	    "carveout-icc"),

	PLATFORM_HEAP("smartchip,carveout-linux-sys-heap",
	    27,
	    ION_HEAP_TYPE_CARVEOUT,
	    "carveout-linux-sys-heap"),

	{}
};

static int smartchip_ion_probe(struct platform_device *pdev)
{
	struct smartchip_ion_dev *ipdev;
	int i;

	ipdev = devm_kzalloc(&pdev->dev, sizeof(*ipdev), GFP_KERNEL);
	if (!ipdev)
		return -ENOMEM;

	platform_set_drvdata(pdev, ipdev);

	ipdev->idev = ion_device_create(NULL);
	if (IS_ERR(ipdev->idev))
		return PTR_ERR(ipdev->idev);

	ipdev->data = ion_parse_dt(pdev, smartchip_heaps);
	if (IS_ERR(ipdev->data))
		return PTR_ERR(ipdev->data);

	ipdev->heaps = devm_kzalloc(&pdev->dev,
	        sizeof(struct ion_heap) * ipdev->data->nr,
	        GFP_KERNEL);
	if (!ipdev->heaps) {
		ion_destroy_platform_data(ipdev->data);
		return -ENOMEM;
	}

	for (i = 0; i < ipdev->data->nr; i++) {
		ipdev->heaps[i] = ion_heap_create(&pdev->dev, &ipdev->data->heaps[i]);
		if (!ipdev->heaps) {
			ion_destroy_platform_data(ipdev->data);
			return -ENOMEM;
		}
		ion_device_add_heap(ipdev->idev, ipdev->heaps[i]);
	}

	smartchip_ion_device = ipdev;

	return 0;
}

static int smartchip_ion_remove(struct platform_device *pdev)
{
	struct smartchip_ion_dev *ipdev;
	int i;

	ipdev = platform_get_drvdata(pdev);

	for (i = 0; i < ipdev->data->nr; i++)
		ion_heap_destroy(ipdev->heaps[i]);

	ion_destroy_platform_data(ipdev->data);
	ion_device_destroy(ipdev->idev);

	return 0;
}

static const struct of_device_id smartchip_ion_match_table[] = {
	{.compatible = "smartchip,ion"},
	{},
};

static struct platform_driver smartchip_ion_driver = {
	.probe = smartchip_ion_probe,
	.remove = smartchip_ion_remove,
	.driver = {
		.name = "ion-smartchip",
		.of_match_table = smartchip_ion_match_table,
	},
};

static int __init smartchip_ion_init(void)
{
	return platform_driver_register(&smartchip_ion_driver);
}

subsys_initcall(smartchip_ion_init);

/* SmartChip kernel interface */
struct smartchip_ion_client {
	struct list_head list;
	struct ion_client *client;
};

static LIST_HEAD(smartchip_ion_client_list);

static DEFINE_MUTEX(smartchip_ion_mutex);

int smartchip_client_create(unsigned char *client_name)
{
	struct smartchip_ion_client *smart_client;
	struct ion_client *client;

	mutex_lock(&smartchip_ion_mutex);

	/* Client already exits */
	list_for_each_entry(smart_client, &smartchip_ion_client_list, list) {
		if(strcmp(smart_client->client->name, client_name) == 0) {
			mutex_unlock(&smartchip_ion_mutex);
			return -EINVAL;
		}
	}

	smart_client = kzalloc(sizeof(*smart_client), GFP_KERNEL);
	if(!smart_client) {
		mutex_unlock(&smartchip_ion_mutex);
		return -ENOMEM;
	}

	client = ion_client_create(smartchip_ion_device->idev, client_name);
	if(IS_ERR(client)) {
		mutex_unlock(&smartchip_ion_mutex);
		return PTR_ERR(client);
	}

	smart_client->client = client;
	list_add(&smart_client->list, &smartchip_ion_client_list);

	mutex_unlock(&smartchip_ion_mutex);
	return 0;
}
EXPORT_SYMBOL(smartchip_client_create);

int smartchip_alloc_buffer(unsigned char *client_name, size_t len, size_t align,
    int heap_id_mask, unsigned int flags, void **va, void **pa)
{
	struct smartchip_ion_client *smart_client;
	struct ion_handle *handle = NULL;
	struct ion_buffer *buffer;
	void *vaddr;
	struct page *page;
	int found = 0;

	mutex_lock(&smartchip_ion_mutex);

	list_for_each_entry(smart_client, &smartchip_ion_client_list, list) {
		if(strcmp(smart_client->client->name, client_name) == 0) {
			found = 1;
			break;
		}
	}
	if(!found) {
		mutex_unlock(&smartchip_ion_mutex);
		return -EINVAL;
	}

	handle = ion_alloc(smart_client->client, len, align, heap_id_mask, flags);
	if(IS_ERR(handle)) {
		mutex_unlock(&smartchip_ion_mutex);
		return PTR_ERR(handle);
	}

	buffer = handle->buffer;

	vaddr = ion_map_kernel(smart_client->client, handle);
	if(IS_ERR(vaddr)) {
		mutex_unlock(&smartchip_ion_mutex);
		return PTR_ERR(vaddr);
	}

	buffer->vaddr = vaddr;

	*va = vaddr;
	if(buffer->sg_table->nents == 1) {
		page = sg_page(buffer->sg_table->sgl);
		*pa = (void *)page_to_phys(page);
	} else
		*pa = 0;

	mutex_unlock(&smartchip_ion_mutex);
	return handle->id;
}
EXPORT_SYMBOL(smartchip_alloc_buffer);

int smartchip_free_buffer(unsigned char *client_name, int handle_id)
{
	struct smartchip_ion_client *smart_client;
	struct ion_handle *handle = NULL;
	struct ion_buffer *buffer = NULL;
	int found = 0;

	mutex_lock(&smartchip_ion_mutex);

	list_for_each_entry(smart_client, &smartchip_ion_client_list, list) {
		if(strcmp(smart_client->client->name, client_name) == 0) {
			found = 1;
			break;
		}
	}
	if(!found) {
		mutex_unlock(&smartchip_ion_mutex);
		return -EINVAL;
	}

	mutex_lock(&smart_client->client->lock);

	handle = ion_handle_get_by_id_nolock(smart_client->client, handle_id);
	if (IS_ERR(handle)) {
		mutex_unlock(&smart_client->client->lock);
		mutex_unlock(&smartchip_ion_mutex);
		return PTR_ERR(handle);
	}
	buffer = handle->buffer;
	ion_free_nolock(smart_client->client, handle);

	mutex_unlock(&smart_client->client->lock);
	mutex_unlock(&smartchip_ion_mutex);
	return 0;
}
EXPORT_SYMBOL(smartchip_free_buffer);

int smartchip_destroy_client(unsigned char *client_name)
{
	struct smartchip_ion_client *smart_client;
	int found = 0;

	mutex_lock(&smartchip_ion_mutex);

	list_for_each_entry(smart_client, &smartchip_ion_client_list, list) {
		if(strcmp(smart_client->client->name, client_name) == 0) {
			found = 1;
			break;
		}
	}
	if(!found) {
		mutex_unlock(&smartchip_ion_mutex);
		return -EINVAL;
	}
	ion_client_destroy(smart_client->client);
	list_del(&smart_client->list);
	kfree(smart_client);
	mutex_unlock(&smartchip_ion_mutex);
	return 0;
}
EXPORT_SYMBOL(smartchip_destroy_client);

/*static void smartchip_ion_flush_cache_all(void *dummy)
{
    flush_cache_all();
}

void ion_flush_all_cpus_caches(void)
{
    int cpu;
    cpumask_t mask;

    preempt_disable();

    cpumask_clear(&mask);
    for_each_online_cpu(cpu) {
        cpumask_set_cpu(cpu, &mask);
    }

    on_each_cpu_mask(&mask, smartchip_ion_flush_cache_all, NULL, 1);

    preempt_enable();
}*/

