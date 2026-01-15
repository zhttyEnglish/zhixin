/*
 * smartchip mempool driver
 *
 * map physical memory into process user address space
 */
#include <linux/vmalloc.h>
#include <linux/miscdevice.h>
#include <linux/fs.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/module.h>

#define MEMPOOL_MAP_IDX 32

struct mempool_ioctl_io_set {
	off_t  start;
	size_t size;
};

union mempool_ioctl_arg {
	struct mempool_ioctl_io_set info_set;
};

#define MEMPOOL_IOC_MAGIC       'M'
#define MEMPOOL_IOC_SET_MAPINFO     _IOWR(MEMPOOL_IOC_MAGIC, 0, struct mempool_ioctl_io_set)

struct mempool_map_info {
	phys_addr_t phys_start;
	size_t size;
	unsigned long vaddr;
};

struct mempool_client {
	struct list_head list;
	struct task_struct *tsk;
	struct mempool_map_info map_info[MEMPOOL_MAP_IDX];
	int map_info_idx;
	struct mutex mutex;
};

struct mempool {
	struct list_head   client_list;
	struct mutex       mutex;
	struct miscdevice  dev;
};

static struct mempool mempool_core;

static int mempool_open(struct inode *inode, struct file *file)
{
	struct mempool_client *client;

	client = kzalloc(sizeof(*client), GFP_KERNEL);
	if (IS_ERR(client))
		return PTR_ERR(client);

	file->private_data = client;
	mutex_lock(&mempool_core.mutex);
	list_add(&client->list, &mempool_core.client_list);
	mutex_unlock(&mempool_core.mutex);

	mutex_init(&client->mutex);

	return 0;
}

static int mempool_release(struct inode *inode, struct file *file)
{
	struct mempool_client *client = file->private_data;

	if(!client)
		BUG();
	list_del(&client->list);
	kfree(client);
	return 0;
}

static int mempool_vma_fault(struct vm_area_struct *vma, struct vm_fault *vmf)
{
	struct mempool_client *client = vma->vm_private_data;
	struct mempool_map_info *map_info;
	unsigned long pfn;
	int ret, i;

	mutex_lock(&client->mutex);
	for(i = 0; i < client->map_info_idx; i++) {
		map_info = &client->map_info[i];
		if(map_info->vaddr <= vmf->address &&
		    map_info->vaddr + map_info->size >= vmf->address) {
			break;
		}
	}
	if(i == client->map_info_idx) {
		mutex_unlock(&client->mutex);
		return -EINVAL;
	}

	mutex_unlock(&client->mutex);

	pfn = (map_info->phys_start + vmf->address - vma->vm_start) >> PAGE_SHIFT;

	ret = vm_insert_pfn(vma, vmf->address, pfn);

	switch (ret) {
	case 0:
	case -EBUSY:
		return VM_FAULT_NOPAGE;
	case -ENOMEM:
		return VM_FAULT_OOM;
	default:
		return VM_FAULT_SIGBUS;
	}
}

static const struct vm_operations_struct mempool_vmops = {
	.fault = mempool_vma_fault,
};

static int mempool_mmap(struct file *file, struct vm_area_struct *vma)
{
	struct mempool_client *client = file->private_data;
	struct mempool_map_info *map_info =
		    &client->map_info[client->map_info_idx];

	if(!client)
		BUG();

	vma->vm_flags |= VM_IO | VM_PFNMAP
	    | VM_DONTEXPAND | VM_DONTDUMP;
	vma->vm_private_data = client;
	vma->vm_ops          = &mempool_vmops;

	map_info->vaddr = vma->vm_start;
	mutex_unlock(&client->mutex);
	client->map_info_idx++;
	mutex_unlock(&client->mutex);
	return 0;
}

static long mempool_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	struct mempool_client *client = file->private_data;
	union mempool_ioctl_arg data;

	if (copy_from_user(&data, (void __user *)arg, _IOC_SIZE(cmd)))
		return -EFAULT;

	switch (cmd) {
	case MEMPOOL_IOC_SET_MAPINFO:
		mutex_lock(&client->mutex);
		client->map_info[client->map_info_idx].phys_start = data.info_set.start;
		client->map_info[client->map_info_idx].size       = data.info_set.size;
		mutex_unlock(&client->mutex);
		break;
	default:
		return -EINVAL;
	}
	return 0;
}

static const struct file_operations mempool_fops = {
	.owner          = THIS_MODULE,
	.open           = mempool_open,
	.release        = mempool_release,
	.mmap           = mempool_mmap,
	.unlocked_ioctl = mempool_ioctl,
	/* TODO */
	/* .compat_ioctl   = compat_mempool_ioctl, */
};

static int __init smartchip_mempool_init(void)
{
	int ret;

	memset(&mempool_core, 0, sizeof(mempool_core));
	mempool_core.dev.minor  = MISC_DYNAMIC_MINOR;
	mempool_core.dev.name   = "mempool";
	mempool_core.dev.fops   = &mempool_fops;
	mempool_core.dev.parent = NULL;

	INIT_LIST_HEAD(&mempool_core.client_list);

	mutex_init(&mempool_core.mutex);

	ret = misc_register(&mempool_core.dev);
	if(ret < 0) {
		printk("register smartchip memory pool driver fail\n");
	}
	return ret;
}

static void __exit smartchip_mempool_exit(void)
{
	misc_deregister(&mempool_core.dev);
}

MODULE_AUTHOR("smartchip");
MODULE_DESCRIPTION("smartchip driver to map physical memory to user address space");
MODULE_LICENSE("GPL");

module_init(smartchip_mempool_init);
module_exit(smartchip_mempool_exit);

