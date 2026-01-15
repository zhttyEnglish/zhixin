#define pr_fmt(fmt) "adc: " fmt

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/types.h>
#include <linux/wait.h>
#include <linux/platform_device.h>

#include <linux/fs.h>
#include <linux/miscdevice.h>

#include <linux/fcntl.h>
#include <linux/ioctl.h>
#include <linux/uaccess.h>

#include <linux/compat.h>
#include <asm/io.h>

#include "sc_adc_user.h"

/* control offset */
#define SC_ADC_ENABLE               0x18
#define SC_ADC_CHN_SELECT           0x24
#define SC_ADC_CALIBRATION          0x2c

/* adc reg offset */
#define SC_ADC_WORK_MODE_SELECT     0x00
#define SC_ADC_VAL                  0x10

#define SC_ADC_AUTOCALIB_VOLT       900 //mv
#define SC_ADC_AUTOCALIB_EN         BIT(3)

/* millivoltage = raw * a / b + c */
#define ADC_ADJV_A 2024
#define ADC_ADJV_B ADC_VAL_MAX
#define ADC_ADJV_C 0

#define ADC_POWER_EN_MAX 1024

struct adc_dev_ctl {
	void __iomem *base;
	void __iomem *global_ctrl;

	spinlock_t         lock;
	int                calc_a;
	int                adc_en;
};

static struct adc_dev_ctl gctl;

static inline uint32_t adc_read_reg32(void __iomem *addr)
{
	return readl(addr);
}

static inline void adc_write_reg32(void __iomem *addr, uint32_t reg_data)
{
	writel(reg_data, addr);
}

/* @enable: 1: enable
 */
static void _sc_hal_adc_enable(int enable)
{
	//SC_ADC_ENABLE: 0: enable, 1: disable
	adc_write_reg32(gctl.global_ctrl + SC_ADC_ENABLE, !enable);
}

/* @chn: [0, 7]
 */
static void _sc_hal_adc_chn_select(int chn)
{
	adc_write_reg32(gctl.global_ctrl + SC_ADC_CHN_SELECT, chn);
}

/* @mode: 0: sw mode, 1: hw mode
 */
static void _sc_hal_adc_work_mode_select(int mode)
{
	adc_write_reg32(gctl.base + SC_ADC_WORK_MODE_SELECT, mode);
}

/* return value: 10bit: [0, 0x3ff] */
static unsigned int _sc_hal_adc_val_get(void)
{
	return adc_read_reg32(gctl.base + SC_ADC_VAL) & 0x3ff;
}

static void _sc_hal_adc_auto_calibration(int chn)
{
	uint32_t raw = 0;
	uint32_t val = 0;

	val = adc_read_reg32(gctl.global_ctrl + SC_ADC_CALIBRATION);
	val |= SC_ADC_AUTOCALIB_EN;
	adc_write_reg32(gctl.global_ctrl + SC_ADC_CALIBRATION, val);

	_sc_hal_adc_chn_select(chn);
	raw = _sc_hal_adc_val_get();
	gctl.calc_a = SC_ADC_AUTOCALIB_VOLT * ADC_ADJV_B / raw;
	pr_debug("calibration adc_a: %d\n", gctl.calc_a);

	val &= ~(SC_ADC_AUTOCALIB_EN);
	adc_write_reg32(gctl.global_ctrl + SC_ADC_CALIBRATION, val);
}

static int sc_hal_adc_enable(void)
{
	spin_lock(&gctl.lock);

	if(gctl.adc_en >= ADC_POWER_EN_MAX) {
		pr_info("too much power enable\n");
		goto EXIT;
	}
	if(! gctl.adc_en) {
		_sc_hal_adc_enable(1);
		_sc_hal_adc_work_mode_select(0);
		_sc_hal_adc_auto_calibration(0);
	}

	gctl.adc_en++;

EXIT:
	spin_unlock(&gctl.lock);

	return 0;
}

static int sc_hal_adc_disable(void)
{
	spin_lock(&gctl.lock);

	if(gctl.adc_en <= 0) {
		pr_info("too much power disable\n");
		goto EXIT;
	}

	gctl.adc_en--;
	if(! gctl.adc_en) {
		_sc_hal_adc_enable(0);
	}

EXIT:
	spin_unlock(&gctl.lock);

	return 0;
}

static int sc_hal_adc_power(int en)
{
	int ret;

	if(en)
		ret = sc_hal_adc_enable();
	else
		ret = sc_hal_adc_disable();
	return ret;
}

static int sc_hal_adc_get_raw(int chn)
{
	int ret = -1;

	if(chn < ADC_CHN_MIN || chn > ADC_CHN_MAX) {
		pr_err("chn %d out of index!\n", chn);
		return -1;
	}

	spin_lock(&gctl.lock);
	if(gctl.adc_en) {
		_sc_hal_adc_chn_select(chn);
		ret = _sc_hal_adc_val_get();
	}
	spin_unlock(&gctl.lock);

	if(-1 == ret) {
		pr_err("enable adc first!\n");
	}

	return ret;
}

static int sc_hal_adc_get_millivolt(int chn)
{
	int ret = -1;

	ret = sc_hal_adc_get_raw(chn);
	if(-1 == ret) {
		pr_err("enable adc first!\n");
		return ret;
	}

	return ret * gctl.calc_a / ADC_ADJV_B + ADC_ADJV_C;
}

static long scadc_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	int ret = -EFAULT;

	switch (cmd) {
	case IOCMD_ADC_POWER: {
		int adc_power_en;

		if (copy_from_user (&adc_power_en, (void *)arg, sizeof(adc_power_en)))
			return -EFAULT;
		ret = sc_hal_adc_power(adc_power_en);
	}
	break;

	case IOCMD_ADC_GET_RAW: {
		struct adc_val scadc_arg;

		if (copy_from_user (&scadc_arg, (void *)arg, sizeof(scadc_arg)))
			return -EFAULT;

		ret = sc_hal_adc_get_raw(scadc_arg.chn);
		if (ret < 0)
			return -EINVAL;
		scadc_arg.val = ret;
		if (copy_to_user ((void *)arg, &scadc_arg, sizeof(scadc_arg)))
			return -EFAULT;

		ret = 0;
	}
	break;

	case IOCMD_ADC_GET_MV: {
		struct adc_val scadc_arg;

		if (copy_from_user (&scadc_arg, (void *)arg, sizeof(scadc_arg)))
			return -EFAULT;

		ret = sc_hal_adc_get_millivolt(scadc_arg.chn);
		if (ret < 0)
			return -EINVAL;
		scadc_arg.val = ret;
		if (copy_to_user ((void *)arg, &scadc_arg, sizeof(scadc_arg)))
			return -EFAULT;

		ret = 0;
	}
	break;

	default:
		return -EACCES;
	};

	return ret;
}

static int scadc_open(struct inode *inode, struct file *file)
{
	return 0;
}

static int scadc_close(struct inode *inode, struct file *file)
{
	return 0;
}

static struct file_operations scadc_fops = {
	.owner = THIS_MODULE,
	.unlocked_ioctl = scadc_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl = scadc_ioctl,
#endif /* CONFIG_COMPAT*/
	.open = scadc_open,
	.release = scadc_close
};

static struct miscdevice scadc_dev = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = DEV_NAME_ADC,
	.fops = &scadc_fops,
};

static int sc_adc_probe(struct platform_device *pdev)
{

	int ret;
	struct resource *mem;

	spin_lock_init(&gctl.lock);
	gctl.base = NULL;
	gctl.global_ctrl = NULL;

	ret = -ENOMEM;

	mem = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	gctl.base = devm_ioremap_resource(&pdev->dev, mem);
	if (IS_ERR(gctl.base))
		return -ENOMEM;

	mem = platform_get_resource(pdev, IORESOURCE_MEM, 1);
	gctl.global_ctrl = devm_ioremap_resource(&pdev->dev, mem);
	if (IS_ERR(gctl.global_ctrl))
		return -ENOMEM;

	ret = misc_register(&scadc_dev);
	if (ret) {
		goto err_unmap;
	}

	printk("sc adc init is ok!\n");
	return 0;

err_unmap:
	return ret;
}

static int sc_adc_remove(struct platform_device *pdev)
{
	misc_deregister(&scadc_dev);

	pr_info("sc adc exit\n");
	return 0;
}

static const struct of_device_id sc_adc_match[] = {
	{.compatible = "smartchip,adc"},
	{},
};

MODULE_DEVICE_TABLE(of, sc_adc_match);

static struct platform_driver sc_adc_platform_driver = {
	.probe = sc_adc_probe,
	.remove = sc_adc_remove,
	.driver = {
		.name = "smartchip adc",
		.of_match_table = sc_adc_match,
	},
};

module_platform_driver(sc_adc_platform_driver);

MODULE_AUTHOR("SmartChip");
MODULE_LICENSE("GPL");
MODULE_VERSION("1.0");

