/*
 * linux/drivers/mmc/card/sdio_smartchip.c - SDIO smartchip driver
 *
 * Author:  DY.Meng
 * Created: September 20, 2019
 * Copyright:   SmartChip Software, Inc.
 *
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/mutex.h>
#include <linux/seq_file.h>
#include <linux/serial_reg.h>
#include <linux/circ_buf.h>
#include <linux/tty.h>
#include <linux/tty_flip.h>
#include <linux/kfifo.h>
#include <linux/slab.h>

#include <linux/mmc/core.h>
#include <linux/mmc/card.h>
#include <linux/mmc/sdio_func.h>
#include <linux/mmc/sdio_ids.h>
#include <linux/mmc/host.h>
#include <linux/mmc/sdio.h>
#include <linux/miscdevice.h>

#include "../core/sdio_ops.h"

static DEFINE_MUTEX(smartchip_mutex);

#define FIFO_ADDRESS    0x10100

struct smartchip_sdio_device {
	struct sdio_func *func;
	char buf[1024];
};

struct smartchip_rw_args {
	unsigned int func_num;
	unsigned int addr;
	unsigned int length;
	u8 val;
};

struct smartchip_cmd_args {
	unsigned int opcode;
	unsigned int arg;
	unsigned int flags;
	unsigned int retries;
	unsigned int resp[4];
};

#define CMD     _IOWR('v', 0, struct smartchip_cmd_args)
#define READ_BYTE   _IOR('v', 0, struct smartchip_rw_args)
#define WRITE_BYTE  _IOW('v', 0, struct smartchip_rw_args)

static struct smartchip_sdio_device *smartchip_dev;

static int smartchip_open(struct inode *inode, struct file *file)
{
	if (smartchip_dev)
		file->private_data = smartchip_dev;
	else
		return -EFAULT;

	return 0;
}

static ssize_t smartchip_read(struct file *file, char __user *buf,
    size_t count, loff_t *ppos)
{
	struct smartchip_sdio_device *smartchip_dev = file->private_data;
	int ret = 0;
	ssize_t ret_len = 0;

	if (unlikely(!access_ok(VERIFY_WRITE, buf, count)))
		return -EFAULT;

	if (count > 1024)
		return -EINVAL;

	sdio_claim_host(smartchip_dev->func);

	ret = sdio_memcpy_fromio(smartchip_dev->func, smartchip_dev->buf,
	        FIFO_ADDRESS, count);
	if (ret) {
		printk("smartchip sdio read failed, ret %d \n", ret);
		ret_len = ret;
		goto out;
	}

	if (copy_to_user(buf, smartchip_dev->buf, count))
		ret = -EFAULT;
	ret_len = count;

out:
	sdio_release_host(smartchip_dev->func);
	return ret_len;
}

static ssize_t smartchip_write(struct file *file, const char __user *buf,
    size_t count, loff_t *ppos)
{
	struct smartchip_sdio_device *smartchip_dev = file->private_data;
	int ret = 0;
	ssize_t ret_len = 0;

	if (unlikely(!access_ok(VERIFY_READ, buf, count)))
		return -EFAULT;

	if (count > 1024)
		return -EINVAL;

	if (copy_from_user(smartchip_dev->buf, buf, count))
		return -EFAULT;
	ret_len = count;

	sdio_claim_host(smartchip_dev->func);

	ret = sdio_memcpy_toio(smartchip_dev->func, FIFO_ADDRESS,
	        smartchip_dev->buf, count);
	if (ret)
		printk("smartchip sdio write failed, ret %d \n", ret);

	sdio_release_host(smartchip_dev->func);
	return ret_len;
}

static int
smartchip_io_send_cmd(struct sdio_func *func, struct mmc_command *mmc_cmd)
{
	struct mmc_host *host;
	int err;

	if (!func)
		return -EINVAL;

	host = func->card->host;

	err = mmc_wait_for_cmd(host, mmc_cmd, 0);
	if (err)
		return err;

	if (mmc_host_is_spi(host)) {
		/* host driver already reported errors */
	} else {
		if (mmc_cmd->resp[0] & R5_ERROR)
			return -EIO;
		if (mmc_cmd->resp[0] & R5_FUNCTION_NUMBER)
			return -EINVAL;
		if (mmc_cmd->resp[0] & R5_OUT_OF_RANGE)
			return -ERANGE;
	}

	return 0;
}

static int
smartchip_ioctl(struct file *file, unsigned int cmd, unsigned long data)
{
	void __user *arg = (void __user *)data;
	struct smartchip_sdio_device *smartchip_dev = file->private_data;
	struct mmc_command mmc_cmd = {0};
	struct smartchip_cmd_args cmd_args = {0};
	struct smartchip_rw_args rw_args = {0};
	char val;
	int ret;

	if (!smartchip_dev || !smartchip_dev->func) {
		printk("smartchip sdio ioctl failed, NULL pointer\n");
		return -EFAULT;
	}

	if (_IOC_DIR(cmd) & _IOC_READ)
		ret = access_ok(VERIFY_WRITE, (void __user *)data, _IOC_SIZE(cmd));
	else if (_IOC_DIR(cmd) & _IOC_WRITE)
		ret = access_ok(VERIFY_READ, (void __user *)data, _IOC_SIZE(cmd));
	else
		ret = 0;
	if (!ret)
		return -EFAULT;

	sdio_claim_host(smartchip_dev->func);
	switch(cmd) {
	case CMD:
		if (copy_from_user(&cmd_args, arg, sizeof(cmd_args))) {
			ret = -EFAULT;
			break;
		}
		mmc_cmd.opcode = cmd_args.opcode;
		mmc_cmd.arg = cmd_args.arg;
		mmc_cmd.flags = cmd_args.flags;
		mmc_cmd.retries = cmd_args.retries;
		ret = smartchip_io_send_cmd(smartchip_dev->func, &mmc_cmd);
		if (ret) {
			printk("smartchip send sdio cmd fail, ret %d !\n", ret);
			break;
		}
		memcpy(&cmd_args.resp[0], &mmc_cmd.resp[0], 4 * sizeof(cmd_args.resp[0]));
		ret = copy_to_user((struct smartchip_cmd_args *)arg, &cmd_args,
		        sizeof(struct smartchip_cmd_args));

		break;
	case READ_BYTE:
		if (copy_from_user(&rw_args, arg, sizeof(rw_args))) {
			ret = -EFAULT;
			break;
		}
		ret = mmc_io_rw_direct(smartchip_dev->func->card, 0, rw_args.func_num,
		        rw_args.addr, 0, &val);
		if (ret) {
			printk("smartchip readb fail, ret %d !\n", ret);
			rw_args.val = 0xff;
			break;
		}
		rw_args.val = val;

		ret = copy_to_user((struct smartchip_rw_args *)arg, &rw_args,
		        sizeof(struct smartchip_rw_args));

		break;
	case WRITE_BYTE:
		if (copy_from_user(&rw_args, arg, sizeof(rw_args))) {
			ret = -EFAULT;
			break;
		}
		ret = mmc_io_rw_direct(smartchip_dev->func->card, 1, rw_args.func_num,
		        rw_args.addr, rw_args.val, NULL);

		if (ret)
			printk("smartchip writeb fail, ret %d !\n", ret);

		break;
	default:
		break;
	}

	sdio_release_host(smartchip_dev->func);

	return ret;
}

static long
smartchip_unlocked_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	int ret;

	mutex_lock(&smartchip_mutex);
	ret = smartchip_ioctl(file, cmd, arg);
	mutex_unlock(&smartchip_mutex);

	return ret;
}

static const struct file_operations smartchip_fops = {
	.owner      = THIS_MODULE,
	.open       = smartchip_open,
	.read       = smartchip_read,
	.write      = smartchip_write,
	.unlocked_ioctl = smartchip_unlocked_ioctl,
	.compat_ioctl = smartchip_unlocked_ioctl,
	.llseek     = no_llseek,
};

static struct miscdevice smartchip_miscdev = {
	MISC_DYNAMIC_MINOR,
	"smartchip_sdio",
	&smartchip_fops
};

static int sdio_smartchip_probe(struct sdio_func *func,
    const struct sdio_device_id *id)
{
	int ret;

	smartchip_dev = kzalloc(sizeof(struct smartchip_sdio_device), GFP_KERNEL);
	if (!smartchip_dev)
		return -ENOMEM;

	smartchip_dev->func = func;
	sdio_set_drvdata(func, smartchip_dev);

	ret = misc_register(&smartchip_miscdev);

	return ret;
}

static void sdio_smartchip_remove(struct sdio_func *func)
{
	struct smartchip_sdio_device *smartchip_dev = sdio_get_drvdata(func);

	sdio_claim_host(func);
	smartchip_dev->func = NULL;
	sdio_disable_func(func);
	sdio_release_host(func);

	return;
}

static const struct sdio_device_id sdio_smartchip_ids[] = {
	{ SDIO_DEVICE(0x024c, 0xB723)           },
	{ SDIO_DEVICE(0x024c, 0x8179)           },
	{ SDIO_DEVICE(0x024c, 0x8821)           },
	{ SDIO_DEVICE(0x024c, 0x818B)           },
	{ SDIO_DEVICE(0x024c, 0xB703)           },
	{ SDIO_DEVICE(0x024c, 0xF179)           },
	{ SDIO_DEVICE_CLASS(SDIO_CLASS_WLAN)        },
	{ SDIO_DEVICE_CLASS(SDIO_CLASS_NONE)        },
	{ /* end: all zeroes */             },
};

MODULE_DEVICE_TABLE(sdio, sdio_smartchip_ids);

static struct sdio_driver sdio_smartchip_driver = {
	.probe      = sdio_smartchip_probe,
	.remove     = sdio_smartchip_remove,
	.name       = "sdio_smartchip",
	.id_table   = sdio_smartchip_ids,
};

static int __init sdio_smartchip_init(void)
{
	int ret;

	ret = sdio_register_driver(&sdio_smartchip_driver);
	if (ret)
		goto err;

	return 0;

err:
	return ret;
}

static void __exit sdio_smartchip_exit(void)
{
	sdio_unregister_driver(&sdio_smartchip_driver);
}

module_init(sdio_smartchip_init);
module_exit(sdio_smartchip_exit);

MODULE_AUTHOR("SmartChip");
MODULE_LICENSE("GPL");
