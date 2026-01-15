#include <linux/module.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>

#include <linux/init.h>
#include <linux/ioctl.h>
#include <linux/uaccess.h>

#include <linux/gpio.h>
#include <linux/interrupt.h>
#include <linux/of.h>

#include "gpio_test_user.h"

/***** sca200v100 ***************/
//F36 func0 GPIO0_5
#define SCA200V100_GPIOA_G 0
#define SCA200V100_GPIOA_P 5
//1C19 func5 GPIO1_6
#define SCA200V100_GPIOB_G 1
#define SCA200V100_GPIOB_P 6

/***** sca200v200 ***************/
//func0 GPIOA_17
#define SCA200V200_GPIOA_G 0
#define SCA200V200_GPIOA_P 17
//func0 GPIOA_18
#define SCA200V200_GPIOB_G 0
#define SCA200V200_GPIOB_P 18

#define SCA200V100_DTS_M "smartchip,smartx-sca200v100"
#define SCA200V200_DTS_M "smartchip,smartx-sca200v200"

struct gpio_info {
	int gpio_nr;
	int gpio_irq;
	char label[16];
};

struct gpio_test_ctl {
	struct gpio_info gpa;
	struct gpio_info gpb;
};

static struct gpio_test_ctl gpio_ctl;

static int gpio_use_irq = 0;
module_param(gpio_use_irq, int, S_IRUGO);
MODULE_PARM_DESC(gpio_use_irq, "0: no irq gpio");

static int b_sca200v100 = 0;

/***** sca200v100 ***************/
#define GROUP_MAX_SCA200V100 4
#define PIN_MAX_SCA200V100   32

/*
 * @group
 * @pin. [0, 7] 才支持中断
 */
static inline int sca200v100_gpio_nr(int group, int pin)
{
	return group * 32 + pin;
}

static inline int sca200v100_gpio_group(int gpio_nr)
{
	return gpio_nr / 32;
}

static inline int sca200v100_gpio_pin(int gpio_nr)
{
	return gpio_nr % 32;
}

/***** sca200v200 ***************/
#define GROUP_MAX_SCA200V200 7
#define PIN_MAX_SCA200V200   32

static unsigned char sca200v200_pin_nr[GROUP_MAX_SCA200V200] =  {
	23, 22, 26, 6,
	6,  11, 16
};

static inline int sca200v200_gpio_nr(int group, int pin)
{
	int gpio_nr = 0;
	int i;

	if(group >= GROUP_MAX_SCA200V200) {
		pr_err("invalid group %d\n", group);
		return -1;
	}
	if(pin >= sca200v200_pin_nr[group]) {
		pr_err("invalid pin %d\n", pin);
		return -1;
	}

	for (i=0; i < group; i++) {
		gpio_nr += sca200v200_pin_nr[i];
	}

	return gpio_nr + pin;
}

static inline int sca200v200_gpio_group(int gpio_num_in)
{
	int gpio_nr_tmp = 0;
	int i;

	for (i = 0; i < GROUP_MAX_SCA200V200; i++) {
		gpio_nr_tmp += sca200v200_pin_nr[i];
		if (gpio_num_in < gpio_nr_tmp)
			break;
	}

	return i;
}

static inline int sca200v200_gpio_pin(int gpio_num_in)
{
	int gpio_nr_tmp = 0;
	int i;

	for (i = 0; i < GROUP_MAX_SCA200V200; i++) {
		gpio_nr_tmp = sca200v200_pin_nr[i];
		if (gpio_num_in < gpio_nr_tmp)
			break;

		gpio_num_in -= gpio_nr_tmp;
	}

	return gpio_num_in;
}

static inline int sca200_gpio_nr(int group, int pin)
{
	if(b_sca200v100)
		return sca200v100_gpio_nr(group, pin);
	else
		return sca200v200_gpio_nr(group, pin);

}

static inline int sca200_gpio_group(int gpio_nr)
{
	if(b_sca200v100)
		return sca200v100_gpio_group(gpio_nr);
	else
		return sca200v200_gpio_group(gpio_nr);

}
static inline int sca200_gpio_pin(int gpio_nr)
{
	if(b_sca200v100)
		return sca200v100_gpio_pin(gpio_nr);
	else
		return sca200v200_gpio_pin(gpio_nr);

}

static inline int gpio_nr_a(void)
{
	if(b_sca200v100)
		return sca200_gpio_nr(SCA200V100_GPIOA_G, SCA200V100_GPIOA_P);
	else
		return sca200_gpio_nr(SCA200V200_GPIOA_G, SCA200V200_GPIOA_P);
}
static inline int gpio_nr_b(void)
{
	if(b_sca200v100)
		return sca200_gpio_nr(SCA200V100_GPIOB_G, SCA200V100_GPIOB_P);
	else
		return sca200_gpio_nr(SCA200V200_GPIOB_G, SCA200V200_GPIOB_P);
}

static irqreturn_t gpio_test_irq_handle(int irq, void *dev_id)
{
	struct gpio_info *pgp = dev_id;
	int value;

	value = gpio_get_value(pgp->gpio_nr);
	pr_info("GPIO%d_%d = %d\n",
			sca200_gpio_group(pgp->gpio_nr),
			sca200_gpio_pin(pgp->gpio_nr),
			value);

	return IRQ_HANDLED;
}

static int gpio_get(struct gpio_drv *gp)
{
	int ret;
	int gpio_nr = sca200_gpio_nr(gp->group, gp->pin);

	ret = gpio_request(gpio_nr, "test");
	if (ret) {
		pr_info("err: gpio_request, ret = %#x\n", ret);
		return ret;
	}

	gp->val = gpio_get_value(gpio_nr);
	gpio_free(gpio_nr);

	return 0;
}

static int gpio_set(const struct gpio_drv *gp)
{
	int ret;
	int gpio_nr = sca200_gpio_nr(gp->group, gp->pin);

	ret = gpio_request(gpio_nr, "test");
	if (ret) {
		pr_info("err: gpio_request, ret = %#x\n", ret);
		return ret;
	}

	gpio_direction_output(gpio_nr, gp->val);
	gpio_free(gpio_nr);

	return 0;
}

static long gpio_test_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	int ret;
	void __user *argp = (void __user *)arg;

	pr_info("gpio test ioctl cmd: 0x%x\n",  cmd);
	switch (cmd) {
	case DRV_GPIO_GET: {
		struct gpio_drv data;
		if(copy_from_user(&data, argp, sizeof(data))) {
			return -1;
		}

		ret = gpio_get(&data);
		if(ret)
			return ret;

		if(copy_to_user(argp, &data, sizeof(data))) {
			return -1;
		}

	}
	break;

	case DRV_GPIO_SET: {
		struct gpio_drv data;
		if(copy_from_user(&data, argp, sizeof(data))) {
			return -1;
		}

		ret = gpio_set(&data);
		if(ret)
			return ret;
	}
	break;

	default:
		break;
	}
	return 0;
}

static int gpio_test_open(struct inode *inode, struct file *file)
{
	return 0;
}

static int gpio_test_release(struct inode *inode, struct file *file)
{
	return 0;
}

static const struct file_operations gpio_test_ops = {
	.owner              = THIS_MODULE,
	.open               = gpio_test_open,
	.release            = gpio_test_release,
	.unlocked_ioctl     = gpio_test_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl = gpio_test_ioctl,
#endif /* CONFIG_COMPAT*/
};

static struct miscdevice gpio_dev = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = DEV_NAME_GPIO_TEST,
	.fops = &gpio_test_ops,
};

static int __init dev_init(void)
{

	int ret;

	ret = misc_register(&gpio_dev);
	if (ret)
		return ret;

	return 0;
}

static void __exit dev_exit(void)
{
	misc_deregister(&gpio_dev);
}

static int __init gpio_init_gp(int gpio_nr, struct gpio_info *pgp)
{
	int ret;
	unsigned long irq_flags;

	/* req */
	pgp->gpio_nr = gpio_nr;
	snprintf(pgp->label, sizeof(pgp->label), "gpio%d_%d",
	    sca200_gpio_group(pgp->gpio_nr),
	    sca200_gpio_pin(pgp->gpio_nr));
	pgp->label[sizeof(pgp->label) - 1 ] = 0;
	ret = gpio_request(pgp->gpio_nr, pgp->label);
	if (ret) {
		pr_info("error: gpio_request ret = %d\n", ret);
		return ret;
	}

	ret = gpio_direction_input(pgp->gpio_nr);
	if (ret) {
		goto FREE_GPIO;
	}

	pgp->gpio_irq = gpio_to_irq(pgp->gpio_nr);
	if(-1 == pgp->gpio_irq) {
		goto FREE_GPIO;
	}

	irq_flags = IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING ;
	ret = request_irq(pgp->gpio_irq, gpio_test_irq_handle, irq_flags, pgp->label, pgp);
	if(ret) {
		pgp->gpio_irq = -1;
		goto FREE_GPIO;
	}

	return 0;

FREE_GPIO:
	if(pgp->gpio_nr >= 0) {
		gpio_free(pgp->gpio_nr);
		pgp->gpio_nr = -1;
	}
	return ret;
}

static int __init gpio_init(void)
{
	int ret;

	/* init */
	gpio_ctl.gpa.gpio_nr = -1;
	gpio_ctl.gpa.gpio_irq  = -1;

	gpio_ctl.gpb.gpio_nr = -1;
	gpio_ctl.gpb.gpio_irq  = -1;

	if(!gpio_use_irq)
		return 0;

	ret = gpio_init_gp(gpio_nr_a(), &gpio_ctl.gpa);
	if (ret) {
		return ret;
	}

	ret = gpio_init_gp(gpio_nr_b(), &gpio_ctl.gpb);
	if (ret) {
		goto EXIT_GPIO_A;
	}

	return 0;

EXIT_GPIO_A:
	free_irq(gpio_ctl.gpa.gpio_irq, &gpio_ctl.gpa);
	gpio_free(gpio_ctl.gpa.gpio_nr);
	return ret;
}

static void __exit gpio_exit(void)
{
	if(gpio_ctl.gpa.gpio_irq >= 0) {
		free_irq(gpio_ctl.gpa.gpio_irq, &gpio_ctl.gpa);
	}
	if(gpio_ctl.gpa.gpio_nr >= 0) {
		gpio_free(gpio_ctl.gpa.gpio_nr);
	}

	if(gpio_ctl.gpb.gpio_irq >= 0) {
		free_irq(gpio_ctl.gpb.gpio_irq, &gpio_ctl.gpb);
	}
	if(gpio_ctl.gpb.gpio_nr >= 0) {
		gpio_free(gpio_ctl.gpb.gpio_nr);
	}

}

static int __init chip_init(void)
{
	if(of_machine_is_compatible(SCA200V100_DTS_M)) {
		pr_info("sca200v100 gpio\n");
		b_sca200v100 = 1;
	} else if(of_machine_is_compatible(SCA200V200_DTS_M)) {
		pr_info("sca200v200 gpio\n");
		b_sca200v100 = 0;
	} else {
		pr_info("dts chip err\n");
		return -1;
	}

	return 0;
}

static int __init driver_module_init (void)
{
	int ret;

	ret = chip_init();
	if(ret)
		return ret;

	ret = gpio_init();
	if(ret)
		return ret;

	ret = dev_init();
	if(ret)
		return ret;

	pr_info ("sc gpio module init ok\n");
	return 0;
}

static void __exit driver_module_exit (void)
{
	dev_exit();

	gpio_exit();

	pr_info ("sc gpio module exit ok\n");
	return;
}

module_init (driver_module_init);
module_exit (driver_module_exit);

MODULE_AUTHOR("smartchip");
MODULE_DESCRIPTION("sc gpio test driver");
MODULE_LICENSE("GPL");

