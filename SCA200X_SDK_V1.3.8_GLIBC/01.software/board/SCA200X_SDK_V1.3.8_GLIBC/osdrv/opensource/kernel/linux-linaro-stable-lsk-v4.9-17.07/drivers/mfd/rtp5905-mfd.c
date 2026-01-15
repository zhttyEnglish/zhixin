/*
 * rtp5905-mfd.c  --  Realsil RTP5905
 *
 * Copyright 2013 Realsil Semiconductor Corp.
 *
 * Author: Wind Han <wind_han@realsil.com.cn>
 *
 *  This program is free software; you can redistribute it and/or modify it
 *  under  the terms of the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the License, or (at your
 *  option) any later version.
 *
 */

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/gpio.h>
#include <linux/mfd/core.h>
#include <linux/mfd/rtp5905-mfd.h>
#include <linux/delay.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/of_device.h>
#include <linux/regulator/of_regulator.h>

static struct rtp5905_mfd_chip *g_chip;
static uint8_t rtp5905_reg_addr = 0;

static struct mfd_cell rtp5905s[] = {
	{
		.name = "rtp5905-pmic",
	},
	{
		.name = "rtp5905-gpio",
	},
};

/* for debug */
#ifdef rtp5905_debug
static uint8_t rtp5905_reg_buffer[0xFF];
static int rtp5905_i2c_read(struct rtp5905_mfd_chip *chip, uint8_t reg, int count, uint8_t *val)
{
	int i;

	if (reg < 0 || count <= 0 || reg + count >= 0xFF)
		return -1;

	for (i = 0; i < count; i++)
		val[i] = rtp5905_reg_buffer[reg + i];

	return 0;
}

static int rtp5905_i2c_write(struct rtp5905_mfd_chip *chip, uint8_t reg, int count, uint8_t *val)
{
	int i;

	if (reg < 0 || count <= 0 || reg + count >= 0xFF)
		return -1;

	for (i = 0; i < count; i++)
		rtp5905_reg_buffer[reg + i] = val[i];

	return 0;
}
#else
#define RTP5905_SPEED   (200 * 1000)
static int rtp5905_i2c_read(struct rtp5905_mfd_chip *chip, uint8_t reg, int count, uint8_t *dest)
{
#if 0

	struct regmap *map = chip->regmap;
	unsigned int val;
	int ret;
	dev_err(chip->dev, "i2c_read chip->regmap %p count %d\n", map, count);
	ret = regmap_read(map, reg, val);
	*dest = 0;
	return ret;

	if (count > 1) {
		ret = regmap_read(map, reg, val);
		*dest = val;
		return ret;
	} else {
		return regmap_bulk_read(map, reg, val, count);
	}
#endif
#if 1
	struct i2c_client *i2c = chip->i2c_client;
	struct i2c_msg xfer[2];
	int ret;

	/* Write register */
	xfer[0].addr = i2c->addr;
	xfer[0].flags = 0;
	xfer[0].len = 1;
	xfer[0].buf = &reg;
#ifdef CONFIG_I2C_ROCKCHIP_COMPAT
	xfer[0].scl_rate = RTP5905_SPEED;
#endif

	/* Read data */
	xfer[1].addr = i2c->addr;
	xfer[1].flags = I2C_M_RD;
	xfer[1].len = count;
	xfer[1].buf = dest;
#ifdef CONFIG_I2C_ROCKCHIP_COMPAT
	xfer[1].scl_rate = RTP5905_SPEED;
#endif

	ret = i2c_transfer(i2c->adapter, xfer, 2);

	if (ret == 2)
		ret = 0;
	else if (ret >= 0)
		ret = -EIO;

	return ret;
#endif

}

#ifdef CONFIG_I2C_ROCKCHIP_COMPAT
static int i2c_master_normal_send(const struct i2c_client *client, const char *buf, int count, int scl_rate)
{
	int ret;
	struct i2c_adapter *adap = client->adapter;
	struct i2c_msg msg;

	msg.addr = client->addr;
	msg.flags = client->flags;
	msg.len = count;
	msg.buf = (char *)buf;
	msg.scl_rate = scl_rate;
	//msg.udelay = client->udelay;

	ret = i2c_transfer(adap, &msg, 1);
	return (ret == 1) ? count : ret;
}
#endif

static int rtp5905_i2c_write(struct rtp5905_mfd_chip *chip, uint8_t reg, int count, uint8_t *val)
{
#if 1
	struct i2c_client *i2c = chip->i2c_client;
	uint8_t msg[RTP5905_REG_MAX_REGISTER + 1];
	int ret;

	if (count > RTP5905_REG_MAX_REGISTER)
		return -EINVAL;

	msg[0] = reg;
	memcpy(&msg[1], val, count);

#ifdef CONFIG_I2C_ROCKCHIP_COMPAT
	ret = i2c_master_normal_send(i2c, msg, count + 1, RTP5905_SPEED);
#else
	ret = i2c_master_send(i2c, msg, count + 1);
#endif
	if (ret < 0)
		return ret;
	if (ret != count + 1)
		return -EIO;
#endif
	return 0;
}
#endif

int rtp5905_reg_read(struct rtp5905_mfd_chip *chip, uint8_t reg, uint8_t *val)
{
	int ret;

	//  printk(KERN_DEBUG "Read from reg 0x%x\n", reg);
	mutex_lock(&chip->io_mutex);

	ret = chip->read(chip, reg, 1, val);
	if (ret < 0)
		dev_err(chip->dev, "Read from reg 0x%x failed\n", reg);

	mutex_unlock(&chip->io_mutex);
	//  printk(KERN_DEBUG "Read from reg 0x%x val=0x%x\n", reg, *val);
	return ret;
}
EXPORT_SYMBOL_GPL(rtp5905_reg_read);

int rtp5905_reg_write(struct rtp5905_mfd_chip *chip, uint8_t reg, uint8_t *val)
{
	int ret;

	mutex_lock(&chip->io_mutex);

	ret = chip->write(chip, reg, 1, val);
	if (ret < 0)
		dev_err(chip->dev, "Write for reg 0x%x failed\n", reg);

	mutex_unlock(&chip->io_mutex);
	//  printk(KERN_DEBUG "Write for reg 0x%x val=0x%x\n", reg, *val);
	return ret;
}
EXPORT_SYMBOL_GPL(rtp5905_reg_write);

int rtp5905_bulk_read(struct rtp5905_mfd_chip *chip, uint8_t reg, int count, uint8_t *val)
{
	int ret;

	//  printk(KERN_DEBUG "Read bulk from reg 0x%x\n", reg);
	mutex_lock(&chip->io_mutex);

	ret = chip->read(chip, reg, count, val);
	if (ret < 0)
		dev_err(chip->dev, "Read bulk from reg 0x%x failed\n", reg);

	mutex_unlock(&chip->io_mutex);

	return ret;
}
EXPORT_SYMBOL_GPL(rtp5905_bulk_read);

int rtp5905_bulk_write(struct rtp5905_mfd_chip *chip, uint8_t reg, int count, uint8_t *val)
{
	int ret;

	//  printk(KERN_DEBUG "Write bulk for reg 0x%x\n", reg);
	mutex_lock(&chip->io_mutex);

	ret = chip->write(chip, reg, count, val);
	if (ret < 0)
		dev_err(chip->dev, "Write bulk for reg 0x%x failed\n", reg);

	mutex_unlock(&chip->io_mutex);

	return ret;
}
EXPORT_SYMBOL_GPL(rtp5905_bulk_write);

int rtp5905_set_bits(struct rtp5905_mfd_chip *chip, int reg, uint8_t bit_mask)
{
	uint8_t val;
	int ret = 0;

	//printk(KERN_DEBUG "Set reg 0x%x with bit mask 0x%x\n", reg, bit_mask);
	mutex_lock(&chip->io_mutex);

	ret = chip->read(chip, reg, 1, &val);
	if (ret < 0) {
		dev_err(chip->dev, "Read from reg 0x%x failed\n", reg);
		goto out;
	}

	if ((val & bit_mask) != bit_mask) {
		val |= bit_mask;
		ret = chip->write(chip, reg, 1, &val);
		if (ret < 0)
			dev_err(chip->dev, "Write for reg 0x%x failed\n", reg);
	}
out:
	mutex_unlock(&chip->io_mutex);
	return ret;
}
EXPORT_SYMBOL_GPL(rtp5905_set_bits);

int rtp5905_clr_bits(struct rtp5905_mfd_chip *chip, int reg, uint8_t bit_mask)
{
	uint8_t val;
	int ret = 0;

	//printk(KERN_DEBUG "Clear reg 0x%x with bit mask 0x%x\n", reg, bit_mask);
	mutex_lock(&chip->io_mutex);

	ret = chip->read(chip, reg, 1, &val);
	if (ret < 0) {
		dev_err(chip->dev, "Read from reg 0x%x failed\n", reg);
		goto out;
	}

	if (val & bit_mask) {
		val &= ~bit_mask;
		ret = chip->write(chip, reg, 1, &val);
		if (ret < 0)
			dev_err(chip->dev, "Write for reg 0x%x failed\n", reg);
	}
out:
	mutex_unlock(&chip->io_mutex);
	return ret;
}
EXPORT_SYMBOL_GPL(rtp5905_clr_bits);

int rtp5905_update_bits(struct rtp5905_mfd_chip *chip, int reg, uint8_t reg_val, uint8_t mask)
{
	uint8_t val;
	int ret = 0;

	//printk(KERN_DEBUG "Update reg 0x%x with val 0x%x & bit mask 0x%x\n", reg, reg_val, mask);
	mutex_lock(&chip->io_mutex);

	ret = chip->read(chip, reg, 1, &val);
	if (ret < 0) {
		dev_err(chip->dev, "Read from reg 0x%x failed\n", reg);
		goto out;
	}

	if ((val & mask) != reg_val) {
		val = (val & ~mask) | reg_val;
		ret = chip->write(chip, reg, 1, &val);
		if (ret < 0)
			dev_err(chip->dev, "Write for reg 0x%x failed\n", reg);
	}
out:
	mutex_unlock(&chip->io_mutex);
	return ret;
}
EXPORT_SYMBOL_GPL(rtp5905_update_bits);

int rtp5905_register_notifier(struct rtp5905_mfd_chip *chip, struct notifier_block *nb, uint64_t irqs)
{
	if (NULL != nb) {
		chip->irq_enable |= irqs;
		chip->update_irqs_en(chip);
		return blocking_notifier_chain_register(&chip->notifier_list, nb);
	}

	return -EINVAL;
}
EXPORT_SYMBOL_GPL(rtp5905_register_notifier);

int rtp5905_unregister_notifier(struct rtp5905_mfd_chip *chip, struct notifier_block *nb, uint64_t irqs)
{
	if (NULL != nb) {
		chip->irq_enable &= ~irqs;
		chip->update_irqs_en(chip);
		return blocking_notifier_chain_unregister(&chip->notifier_list, nb);
	}

	return -EINVAL;
}
EXPORT_SYMBOL_GPL(rtp5905_unregister_notifier);

static int rtp5905_init_irqs(struct rtp5905_mfd_chip *chip)
{
#if 0
	//  uint8_t v1[7] = {0x00, 0xff, 0x00, 0x00, 0xc0, 0x0f, 0x37};
	uint8_t v1[7] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
	uint8_t v2[7] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
	int err;

	printk(KERN_DEBUG "Init rtp5905 irqs\n");
	err =  rtp5905_bulk_write(chip, RTP5905_REG_AVCCIN_IRQ_EN, 7, v1);
	if (err) {
		printk(KERN_ERR "[RTP5905-MFD] try to init irq_en failed!\n");
		return err;
	}

	err =  rtp5905_bulk_write(chip, RTP5905_REG_AVCC_IRQ_STS2, 7, v2);
	if (err) {
		printk(KERN_ERR "[RTP5905-MFD] try to init irq_sts failed!\n");
		return err;
	}

	chip->irq_enable = 0x00000000 | (uint64_t) 0x00000000 << 32;
	chip->update_irqs_en(chip);
#endif
	return 0;
}

static int rtp5905_update_irqs_enable(struct rtp5905_mfd_chip *chip)
{

	int ret = 0;
	uint64_t irqs;
	uint8_t v[7] = {0, 0, 0, 0, 0, 0, 0};
#if 0

	printk(KERN_DEBUG "Update rtp5905 irqs enable\n");
	ret =  rtp5905_bulk_read(chip, RTP5905_REG_AVCCIN_IRQ_EN, 7, v);
	if (ret < 0)
		return ret;

	irqs = (((uint64_t) v[6]) << 48) | (((uint64_t) v[5]) << 40) |
	    (((uint64_t) v[4]) << 32) | (((uint64_t) v[3]) << 24) |
	    (((uint64_t) v[2]) << 16) | (((uint64_t) v[1]) << 8) |
	    ((uint64_t) v[0]);

	if (chip->irq_enable != irqs) {
		v[0] = ((chip->irq_enable) & 0xff);
		v[1] = ((chip->irq_enable) >> 8) & 0xff;
		v[2] = ((chip->irq_enable) >> 16) & 0xff;
		v[3] = ((chip->irq_enable) >> 24) & 0xff;
		v[4] = ((chip->irq_enable) >> 32) & 0xff;
		v[5] = ((chip->irq_enable) >> 40) & 0xff;
		v[6] = ((chip->irq_enable) >> 48) & 0xff;
		ret =  rtp5905_bulk_write(chip, RTP5905_REG_ALDOIN_IRQ_EN, 7, v);
	}
#endif
	return ret;
}

static int rtp5905_read_irqs(struct rtp5905_mfd_chip *chip, uint64_t *irqs)
{
	uint8_t v[7] = {0, 0, 0, 0, 0, 0, 0};
	int ret;

	//printk(KERN_DEBUG "Read rtp5905 irqs status\n");
	ret =  rtp5905_bulk_read(chip, RTP5905_REG_AVCC_IRQ_STS2, 7, v);
	if (ret < 0)
		return ret;

	*irqs = (((uint64_t) v[0]) << 48) | (((uint64_t) v[6]) << 40) |
	    (((uint64_t) v[2]) << 32) | (((uint64_t) v[4]) << 24) |
	    (((uint64_t) v[3]) << 16) | (((uint64_t) v[5]) << 8) |
	    ((uint64_t) v[1]);

	return 0;
}

static int rtp5905_write_irqs(struct rtp5905_mfd_chip *chip, uint64_t irqs)
{
	uint8_t v[7];
	int ret;

	//printk(KERN_DEBUG "Write rtp5905 irqs status\n");
	v[0] = (irqs >> 48) & 0xff;
	v[1] = (irqs >> 0) & 0xff;
	v[2] = (irqs >> 32) & 0xff;
	v[3] = (irqs >> 16) & 0xff;
	v[4] = (irqs >> 24) & 0xff;
	v[5] = (irqs >> 8) & 0xff;
	v[6] = (irqs >> 40) & 0xff;

	ret = rtp5905_bulk_write(chip, RTP5905_REG_AVCC_IRQ_STS2, 7, v);
	if (ret < 0)
		return ret;

	return 0;
}

static ssize_t rtp5905_reg_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	uint8_t val;
	struct rtp5905_mfd_chip *chip = i2c_get_clientdata(to_i2c_client(dev));

	rtp5905_reg_read(chip, rtp5905_reg_addr, &val);

	return sprintf(buf, "REG[%x]=%x\n", rtp5905_reg_addr, val);
}

static ssize_t rtp5905_reg_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	int tmp;
	uint8_t val;
	struct rtp5905_mfd_chip *chip = i2c_get_clientdata(to_i2c_client(dev));

	tmp = simple_strtoul(buf, NULL, 16);
	if (tmp < 256)
		rtp5905_reg_addr = tmp;
	else {
		val = tmp & 0x00FF;
		rtp5905_reg_addr = (tmp >> 8) & 0x00FF;
		rtp5905_reg_write(chip, rtp5905_reg_addr, &val);
	}

	return count;
}

static uint8_t rtp5905_regs_addr = 0;
static int rtp5905_regs_len = 0;
static int rtp5905_regs_rw = 0;
static uint8_t rtp5905_regs_datas[256];
static ssize_t rtp5905_regs_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	int count = 0;
	int i;
	struct rtp5905_mfd_chip *chip = i2c_get_clientdata(to_i2c_client(dev));

	if (rtp5905_regs_rw == 0) {
		for (i = 0; i < 256; i++)
			rtp5905_regs_datas[i] = 0x00;
		rtp5905_bulk_read(chip, rtp5905_regs_addr, rtp5905_regs_len, rtp5905_regs_datas);
		for (i = 0; i < rtp5905_regs_len; i++)
			count += sprintf(buf + count, "REG[%x]=%x\n", rtp5905_regs_addr + i, rtp5905_regs_datas[i]);

	} else if (rtp5905_regs_rw == 1) {
		rtp5905_bulk_write(chip, rtp5905_regs_addr, rtp5905_regs_len, rtp5905_regs_datas);
		sprintf(buf, "bulk write ok\n");
	}

	return count;
}

static ssize_t rtp5905_regs_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	int tmp;
	char buftemp[3];

	if (count < 6)
		return count;

	/* rw flag */
	buftemp[0] = buf[0];
	buftemp[1] = '\0';
	rtp5905_regs_rw = simple_strtoul(buftemp, NULL, 16);
	//printk("<0>" "rtp5905_regs_store, rtp5905_regs_rw = %d\n", rtp5905_regs_rw);
	/* addr */
	buftemp[0] = buf[1];
	buftemp[1] = buf[2];
	buftemp[2] = '\0';
	rtp5905_regs_addr = simple_strtoul(buftemp, NULL, 16);
	//printk("<0>" "rtp5905_regs_store, rtp5905_regs_addr = 0x%x\n", rtp5905_regs_addr);
	/* addr */
	buftemp[0] = buf[3];
	buftemp[1] = buf[4];
	buftemp[2] = '\0';
	rtp5905_regs_len = simple_strtoul(buftemp, NULL, 16);
	//printk("<0>" "rtp5905_regs_store, rtp5905_regs_len = %d\n", rtp5905_regs_len);
	if (rtp5905_regs_rw == 1) {
		if (count != 5 + rtp5905_regs_len * 2 + 1) {
			printk("<0>" "rtp5905_regs_store error, count = %d\n", count);
		}
		for (tmp = 0; tmp < rtp5905_regs_len; tmp++) {
			buftemp[0] = buf[tmp * 2 + 5];
			buftemp[1] = buf[tmp * 2 + 6];
			buftemp[2] = '\0';
			rtp5905_regs_datas[tmp] = simple_strtoul(buftemp, NULL, 16);
			printk("<0>" "rtp5905_regs_store, val[%x] = 0x%x\n", tmp + rtp5905_regs_addr, rtp5905_regs_datas[tmp]);
		}
	}

	return count;
}

static struct device_attribute rtp5905_mfd_attrs[] = {
	RTP_MFD_ATTR(rtp5905_reg),
	RTP_MFD_ATTR(rtp5905_regs),
};

int rtp5905_mfd_create_attrs(struct rtp5905_mfd_chip *chip)
{
	int j, ret;

	for (j = 0; j < ARRAY_SIZE(rtp5905_mfd_attrs); j++) {
		ret = device_create_file(chip->dev, &rtp5905_mfd_attrs[j]);
		if (ret)
			goto sysfs_failed;
	}
	goto succeed;

sysfs_failed:
	while (j--)
		device_remove_file(chip->dev, &rtp5905_mfd_attrs[j]);
succeed:
	return ret;
}

#ifdef CONFIG_PM
__weak void  rtp5905_device_suspend(void) {}
__weak void  rtp5905_device_resume(void) {}

static int rtp5905_suspend(struct device *dev)
{
	rtp5905_device_suspend();
	return 0;
}

static int rtp5905_resume(struct device *dev)
{
	rtp5905_device_resume();
	return 0;
}
#else
static int rtp5905_suspend(struct device *dev)
{
	return 0;
}

static int rtp5905_resume(struct device *dev)
{
	return 0;
}
#endif

static const struct regmap_config rtp5905_regmap_config = {
	.reg_bits = 8,
	.val_bits = 8,
	.max_register = RTP5905_REG_DC_OCOT_IRQ_STS,
	.num_reg_defaults_raw = RTP5905_REG_MAX_REGISTER,
	.cache_type = REGCACHE_NONE,
};

#ifdef CONFIG_OF

static const struct i2c_device_id rtp5905_mfd_id_table[] = {
	{ "rtp5905_mfd", RTP5905_ID },
	{},
};
MODULE_DEVICE_TABLE(i2c, rtp5905_mfd_id_table);

static struct of_device_id rtp5905_of_match[] = {
	{ .compatible = "realtek,rtp5905", .data = (void *)RTP5905_ID},
	{},
};

MODULE_DEVICE_TABLE(of, rtp5905_of_match);

static struct rtp5905_board *rtp5905_parse_dt(struct i2c_client *client, unsigned long *chip_id)
{
	struct device_node *np = client->dev.of_node;
	const struct of_device_id *match = NULL;
	struct rtp5905_board *board_info = NULL;

	match = of_match_device(rtp5905_of_match, &client->dev);
	if (!match) {
		dev_err(&client->dev, "Failed to find matching dt id\n");
		return NULL;
	}

	*chip_id  = (unsigned long)match->data;

	board_info = devm_kzalloc(&client->dev, sizeof(*board_info), GFP_KERNEL);
	if (!board_info) {
		dev_err(&client->dev, "Failed to allocate pdata\n");
		return NULL;
	}

	board_info->pm_off = of_property_read_bool(np, "rtp5905,system-power-controller");

	return board_info;

#if 0
	regulators = of_find_node_by_name(pmic, "regulators");
	if (!regulators)
		return -EINVAL;

	count = of_regulator_match(&i2c->dev, regulators, rtp5905_reg_matches, RTP5905_REGULATOR_NUM);
	of_node_put(regulators);
	if ((count < 0) || (count > RTP5905_REGULATOR_NUM))
		return -EINVAL;

	for (i = 0; i < count; i++) {
		if (!rtp5905_reg_matches[i].init_data || !rtp5905_reg_matches[i].of_node)
			continue;
		board->rtp5905_pmic_init_data[i] = rtp5905_reg_matches[i].init_data;
	}

	/* gpio */
	gpio = of_get_named_gpio(pmic, "gpios", 0);
	if (!gpio_is_valid(gpio))
		printk("invalid gpio: %d\n", gpio);
	board->pmic_sleep_gpio = gpio;
	board->pmic_sleep = true;

	gpio = of_get_named_gpio(pmic, "gpios", 1);
	if (!gpio_is_valid(gpio))
		printk("invalid gpio: %d\n", gpio);
	board->pmic_hold_gpio = gpio;
#endif
	/* pm off */

	return 0;
}
#endif

static int rtp5905_mfd_probe(struct i2c_client *i2c, const struct i2c_device_id *id)
{
	struct rtp5905_mfd_chip *chip;
	struct rtp5905_board *pmic_plat_data;
	int ret = 0;
	unsigned long chip_id;

	pmic_plat_data = dev_get_platdata(&i2c->dev);

	if (!pmic_plat_data && i2c->dev.of_node) {
		pmic_plat_data = rtp5905_parse_dt(i2c, &chip_id);
		i2c->dev.platform_data = pmic_plat_data;
	}

	if (!pmic_plat_data)
		return -EINVAL;

	chip = devm_kzalloc(&i2c->dev, sizeof(*chip), GFP_KERNEL);
	if (chip == NULL)
		return -ENOMEM;

	i2c_set_clientdata(i2c, chip);
	chip->dev = &i2c->dev;
	chip->i2c_client = i2c;
	chip->id = chip_id;

	chip->regmap = devm_regmap_init_i2c(i2c, &rtp5905_regmap_config);
	if (IS_ERR(chip->regmap)) {
		ret = PTR_ERR(chip->regmap);
		dev_err(&i2c->dev, "regmap_init failed with err: %d\n", ret);
		return ret;
	}

	chip->read = rtp5905_i2c_read;
	chip->write = rtp5905_i2c_write;
	chip->init_irqs = rtp5905_init_irqs;
	chip->update_irqs_en = rtp5905_update_irqs_enable;
	chip->read_irqs = rtp5905_read_irqs;
	chip->write_irqs = rtp5905_write_irqs;

	mutex_init(&chip->io_mutex);
	/* verify the ic */
	/*  ret = rtp5905_reg_read(chip, 0x22);
	 *  if ((ret < 0) || (ret == 0xff)){
	 *      printk("The device is not rtp5905\n");
	 *      goto err;
	 *  }
	 */
	g_chip = chip;

	if (pmic_plat_data && pmic_plat_data->pre_init) {
		ret = pmic_plat_data->pre_init(chip);
		if (ret != 0) {
			dev_err(chip->dev, "pre_init() failed: %d\n", ret);
			goto err;
		}
	}
#if 0
	ret = rtp5905_irq_init(chip, pmic_plat_data->irq);
	if (ret < 0) {
		dev_err(chip->dev, "rtp5905_irq_init() failed: %d\n", ret);
		goto err;
	}
#endif

	ret = devm_mfd_add_devices(chip->dev, -1, rtp5905s, ARRAY_SIZE(rtp5905s), NULL, 0, NULL);
	if (ret < 0) {
		dev_err(chip->dev, "mfd_add_devices() failed: %d\n", ret);
		goto mfd_err;
	}

	if (pmic_plat_data && pmic_plat_data->post_init) {
		ret = pmic_plat_data->post_init(chip);
		if (ret != 0) {
			dev_err(chip->dev, "post_init() failed: %d\n", ret);
			goto mfd_err;
		}
	}
#if 0
	/* hold gpio */
#ifdef CONFIG_OF
	if (pmic_plat_data->pmic_hold_gpio) {
		ret = gpio_request(pmic_plat_data->pmic_hold_gpio, "rtp5905_pmic_hold");
		if (ret < 0) {
			dev_err(chip->dev, "Failed to request gpio %d with ret:""%d\n",  pmic_plat_data->pmic_hold_gpio, ret);
			goto mfd_err;
			/* return IRQ_NONE; */
		}
		gpio_direction_output(pmic_plat_data->pmic_hold_gpio, 1);
		ret = gpio_get_value(pmic_plat_data->pmic_hold_gpio);
		gpio_free(pmic_plat_data->pmic_hold_gpio);
		printk("%s: rtp5905_pmic_hold=%x\n", __func__, ret);
	}
#endif

	/* sleep gpio */
#ifdef CONFIG_OF
	if (pmic_plat_data->pmic_sleep_gpio) {
		ret = gpio_request(pmic_plat_data->pmic_sleep_gpio, "rtp5905_pmic_sleep");
		if (ret < 0) {
			dev_err(chip->dev, "Failed to request gpio %d with ret:""%d\n",  pmic_plat_data->pmic_sleep_gpio, ret);
			goto mfd_err;
			/* return IRQ_NONE; */
		}
		gpio_direction_output(pmic_plat_data->pmic_sleep_gpio, 1);
		ret = gpio_get_value(pmic_plat_data->pmic_sleep_gpio);
		gpio_free(pmic_plat_data->pmic_sleep_gpio);
		printk("%s: rtp5905_pmic_sleep=%x\n", __func__, ret);
	}
#endif
#endif
	/* power off */
	if (pmic_plat_data->pm_off && !pm_power_off) {
		pm_power_off = rtp5905_power_off;
	}

	ret = rtp5905_mfd_create_attrs(chip);
	if (ret)
		goto mfd_err;

	return ret;

mfd_err:
	mfd_remove_devices(chip->dev);
	//  rtp5905_irq_exit(chip);
err:
	return ret;
}

/* power off */
void rtp5905_power_off(void)
{
	int ret;
	struct rtp5905_mfd_chip *chip = g_chip;
	struct rtp5905_board *pmic_plat_data;

#ifndef CONFIG_OF
	pmic_plat_data = rtp5905_get_board();
#else
	pmic_plat_data = dev_get_platdata(chip->dev);
#endif

	printk("%s\n", __func__);

	if (pmic_plat_data && pmic_plat_data->late_exit) {
		ret = pmic_plat_data->late_exit(chip);
		if (ret != 0)
			dev_err(chip->dev, "late_exit() failed: %d\n", ret);
	}

	mdelay(20);
	rtp5905_set_bits(chip, RTP5905_REG_PMU_STATE_CTL, 0x80);
	mdelay(20);
	printk("warning!!! rtp can't power-off, maybe some error happend!\n");
}
EXPORT_SYMBOL_GPL(rtp5905_power_off);

static int rtp5905_mfd_remove(struct i2c_client *client)
{
	struct rtp5905_mfd_chip *chip = i2c_get_clientdata(client);
	int j;

	printk("<0>""rtp5905_mfd_remove\n");
	mfd_remove_devices(chip->dev);
	//  rtp5905_irq_exit(chip);
	i2c_set_clientdata(client, NULL);
	for (j = 0; j < ARRAY_SIZE(rtp5905_mfd_attrs); j++)
		device_remove_file(chip->dev, &rtp5905_mfd_attrs[j]);
	g_chip = NULL;

	return 0;
}

static const struct dev_pm_ops rtp5905_pm_ops = {
	.suspend = rtp5905_suspend,
	.resume =  rtp5905_resume,
};

static struct i2c_driver rtp5905_mfd_driver = {
	.driver = {
		.name = "rtp5905_mfd",
		.owner = THIS_MODULE,
#ifdef CONFIG_PM
		.pm = &rtp5905_pm_ops,
#endif
#ifdef CONFIG_OF
		.of_match_table = of_match_ptr(rtp5905_of_match),
#endif
	},
	.probe = rtp5905_mfd_probe,
	.remove = rtp5905_mfd_remove,
	.id_table = rtp5905_mfd_id_table,
};

static int __init rtp5905_mfd_init(void)
{
	return i2c_add_driver(&rtp5905_mfd_driver);
}
subsys_initcall_sync(rtp5905_mfd_init);

static void __exit rtp5905_mfd_exit(void)
{
	i2c_del_driver(&rtp5905_mfd_driver);
}
module_exit(rtp5905_mfd_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Wind_Han <wind_han@realsil.com.cn>");
MODULE_DESCRIPTION("Realtek Power Manager Driver");
