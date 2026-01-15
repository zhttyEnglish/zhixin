/*
 * rtp5905-regulator.c  --  Realsil RTP5905
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

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/err.h>
#include <linux/platform_device.h>
#include <linux/regulator/driver.h>
#include <linux/regulator/machine.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/gpio.h>
#include <linux/mfd/rtp5905-mfd.h>
#include <linux/interrupt.h>
#include <linux/regulator/of_regulator.h>

struct rtp5905_regulator_info {
	struct regulator_desc desc;
	int min_uV;
	int max_uV;
	int step_uV;
	int vol_reg;
	int vol_shift;
	int vol_nbits;
	int enable_reg;
	int enable_bit;
	int stb_vol_reg;
	int stb_enable_reg;
	uint8_t vol_offset;
};

#define RTP5905_ENABLE_TIME_US  1000

struct rtp5905_regu {
	struct rtp5905_mfd_chip *chip;
	struct regulator_dev **rdev;
	struct rtp5905_regulator_info **info;
	struct mutex mutex;
	int num_regulators;
	int mode;
	int  (*get_ctrl_reg)(int);
	unsigned int *ext_sleep_control;
	unsigned int board_ext_control[RTP5905_ID_REGULATORS_NUM];
};

static inline int check_range(struct rtp5905_regulator_info *info, int min_uV, int max_uV)
{
	if (min_uV < info->min_uV || min_uV > info->max_uV)
		return -EINVAL;

	return 0;
}

/* rtp5905 regulator common operations */
static int rtp5905_set_voltage(struct regulator_dev *rdev, int min_uV, int max_uV, unsigned *selector)
{
	struct rtp5905_regu *pmic = rdev_get_drvdata(rdev);
	int id = rdev_get_id(rdev);
	struct rtp5905_regulator_info *info = pmic->info[id];
	struct rtp5905_mfd_chip *chip = pmic->chip;
	uint8_t val, mask;

	//  printk(KERN_DEBUG "Set regulator-%d voltage as %d uV\n", id, min_uV);
	if (check_range(info, min_uV, max_uV)) {
		pr_err("invalid voltage range (%d, %d) uV\n", min_uV, max_uV);
		return -EINVAL;
	}

	val = (min_uV - info->min_uV + info->step_uV - 1) / info->step_uV;
	*selector = val;
	val += info->vol_offset;
	val <<= info->vol_shift;
	mask = ((1 << info->vol_nbits) - 1) << info->vol_shift;

	return rtp5905_update_bits(chip, info->vol_reg, val, mask);
}

static int rtp5905_get_voltage(struct regulator_dev *rdev)
{
	struct rtp5905_regu *pmic = rdev_get_drvdata(rdev);
	int id = rdev_get_id(rdev);
	struct rtp5905_regulator_info *info = pmic->info[id];
	struct rtp5905_mfd_chip *chip = pmic->chip;
	uint8_t val, mask;
	int ret;

	//  printk(KERN_DEBUG "Get regulator-%d voltage\n", id);
	ret = rtp5905_reg_read(chip, info->vol_reg, &val);
	if (ret)
		return ret;

	mask = ((1 << info->vol_nbits) - 1)  << info->vol_shift;
	val = (val & mask) >> info->vol_shift;
	val -= info->vol_offset;
	ret = info->min_uV + info->step_uV * val;

	return ret;
}

static int rtp5905_set_voltage_sel(struct regulator_dev *rdev, unsigned selector)
{
	struct rtp5905_regu *pmic = rdev_get_drvdata(rdev);
	int id = rdev_get_id(rdev);
	struct rtp5905_regulator_info *info = pmic->info[id];
	struct rtp5905_mfd_chip *chip = pmic->chip;
	uint8_t val, mask;

	//  printk(KERN_DEBUG "Set regulator-%d voltage selector as %d\n", id, selector);
	if (selector < 0 || selector >= info->desc.n_voltages) {
		pr_err("invalid selector range (%d, %d) uV, selector = %d\n", 0, info->desc.n_voltages - 1, selector);
		return -EINVAL;
	}

	val = selector;
	val += info->vol_offset;
	val <<= info->vol_shift;
	mask = ((1 << info->vol_nbits) - 1) << info->vol_shift;

	return rtp5905_update_bits(chip, info->vol_reg, val, mask);
}

static int rtp5905_get_voltage_sel(struct regulator_dev *rdev)
{
	struct rtp5905_regu *pmic = rdev_get_drvdata(rdev);
	int id = rdev_get_id(rdev);
	struct rtp5905_regulator_info *info = pmic->info[id];
	struct rtp5905_mfd_chip *chip = pmic->chip;
	uint8_t val, mask;
	int ret;

	//  printk(KERN_DEBUG "Get regulator-%d voltage selector\n", id);
	ret = rtp5905_reg_read(chip, info->vol_reg, &val);
	if (ret)
		return ret;

	mask = ((1 << info->vol_nbits) - 1)  << info->vol_shift;
	val = (val & mask) >> info->vol_shift;
	val -= info->vol_offset;
	if (val < 0 || val >= info->desc.n_voltages) {
		pr_err("invalid selector range (%d, %d) uV, val = %d\n", 0, info->desc.n_voltages - 1, val);
		return -EINVAL;
	}
	ret = val;

	return ret;
}

static int rtp5905_list_voltage(struct regulator_dev *rdev, unsigned selector)
{
	struct rtp5905_regu *pmic = rdev_get_drvdata(rdev);
	int id = rdev_get_id(rdev);
	struct rtp5905_regulator_info *info = pmic->info[id];
	int ret;

	//  printk(KERN_DEBUG "List regulator-%d voltage of selector-%d\n", id, selector);
	ret = info->min_uV + info->step_uV * selector;

	if (ret > info->max_uV)
		return -EINVAL;

	return ret;
}

static int rtp5905_set_voltage_time_sel(struct regulator_dev *rdev,
    unsigned int old_selector, unsigned int new_selector)
{
	int old_volt, new_volt, ret, step;
	struct rtp5905_regu *pmic = rdev_get_drvdata(rdev);
	int id = rdev_get_id(rdev);
	struct rtp5905_mfd_chip *chip = pmic->chip;
	uint8_t val;

	//  printk(KERN_DEBUG "Set regulator-%d voltage time from selector %u to selector %u\n", id, old_selector, new_selector);
	old_volt = rtp5905_list_voltage(rdev, old_selector);
	if (old_volt < 0)
		return old_volt;

	new_volt = rtp5905_list_voltage(rdev, new_selector);
	if (new_volt < 0)
		return new_volt;

	ret = rtp5905_reg_read(chip, RTP5905_REG_DVS_STEP_CTRL, &val);
	if (ret)
		return ret;

	switch (id) {
	case RTP5905_ID_BUCK1:
		step = 0x20 >> (val & 0x03);
		break;
	case RTP5905_ID_BUCK2:
		step = 0x20 >> ((val >> 2) & 0x03);
		break;
	case RTP5905_ID_BUCK3:
		step = 0x20 >> ((val >> 4) & 0x03);
		break;
	case RTP5905_ID_BUCK4:
		step = 0x20 >> ((val >> 6) & 0x03);
		break;
	default:
		return -EINVAL;
	}
	ret = DIV_ROUND_UP(abs(old_volt - new_volt), RTP5905_DVS_STEP_US) * step;
	return ret;
}

static int rtp5905_enable(struct regulator_dev *rdev)
{
	struct rtp5905_regu *pmic = rdev_get_drvdata(rdev);
	int id = rdev_get_id(rdev);
	struct rtp5905_regulator_info *info = pmic->info[id];
	struct rtp5905_mfd_chip *chip = pmic->chip;

	//  printk(KERN_DEBUG "Enable regulator-%d\n", id);
	return rtp5905_set_bits(chip, info->enable_reg, 1 << info->enable_bit);
}

static int rtp5905_disable(struct regulator_dev *rdev)
{
	struct rtp5905_regu *pmic = rdev_get_drvdata(rdev);
	int id = rdev_get_id(rdev);
	struct rtp5905_regulator_info *info = pmic->info[id];
	struct rtp5905_mfd_chip *chip = pmic->chip;

	//  printk(KERN_DEBUG "Disable regulator-%d\n", id);
	return rtp5905_clr_bits(chip, info->enable_reg, 1 << info->enable_bit);
}

static int rtp5905_enable_time(struct regulator_dev *rdev)
{
	int id = rdev_get_id(rdev);

	//  printk(KERN_DEBUG "Get regulator-%d enable time\n", id);
	return RTP5905_ENABLE_TIME_US;
}

static int rtp5905_is_enabled(struct regulator_dev *rdev)
{
	struct rtp5905_regu *pmic = rdev_get_drvdata(rdev);
	int id = rdev_get_id(rdev);
	struct rtp5905_regulator_info *info = pmic->info[id];
	struct rtp5905_mfd_chip *chip = pmic->chip;
	uint8_t reg_val;
	int ret;

	//  printk(KERN_DEBUG "Get whether regulator-%d is enabled\n", id);
	ret = rtp5905_reg_read(chip, info->enable_reg, &reg_val);
	if (ret)
		return ret;

	ret = !!(reg_val & (1 << info->enable_bit));

	return ret;
}

static int rtp5905_suspend_enable(struct regulator_dev *rdev)
{
	struct rtp5905_regu *pmic = rdev_get_drvdata(rdev);
	int id = rdev_get_id(rdev);
	struct rtp5905_regulator_info *info = pmic->info[id];
	struct rtp5905_mfd_chip *chip = pmic->chip;

	//  printk(KERN_DEBUG "Enable regulator-%d on suspend state\n", id);
	return rtp5905_clr_bits(chip, info->stb_enable_reg, 1 << info->enable_bit);
}

static int rtp5905_suspend_disable(struct regulator_dev *rdev)
{
	struct rtp5905_regu *pmic = rdev_get_drvdata(rdev);
	int id = rdev_get_id(rdev);
	struct rtp5905_regulator_info *info = pmic->info[id];
	struct rtp5905_mfd_chip *chip = pmic->chip;

	//  printk(KERN_DEBUG "Disable regulator-%d on suspend state\n", id);
	return rtp5905_set_bits(chip, info->stb_enable_reg, 1 << info->enable_bit);
}

static int rtp5905_set_suspend_voltage(struct regulator_dev *rdev, int uV)
{
	struct rtp5905_regu *pmic = rdev_get_drvdata(rdev);
	int id = rdev_get_id(rdev);
	struct rtp5905_regulator_info *info = pmic->info[id];
	struct rtp5905_mfd_chip *chip = pmic->chip;
	uint8_t val, mask;

	//  printk(KERN_DEBUG "Set regulator-%d suspend voltage as %d uV\n", id, uV);
	if (check_range(info, uV, uV)) {
		pr_err("invalid voltage range (%d, %d) uV\n", uV, uV);
		return -EINVAL;
	}

	val = (uV - info->min_uV + info->step_uV - 1) / info->step_uV;
	val += info->vol_offset;
	val <<= info->vol_shift;
	mask = ((1 << info->vol_nbits) - 1) << info->vol_shift;

	return rtp5905_update_bits(chip, info->stb_vol_reg, val, mask);
}

static struct regulator_ops rtp5905_ops_dcdc = {
	.set_voltage_sel = rtp5905_set_voltage_sel,
	.get_voltage_sel = rtp5905_get_voltage_sel,
	.list_voltage = rtp5905_list_voltage,
	.set_voltage_time_sel = rtp5905_set_voltage_time_sel,
	.enable = rtp5905_enable,
	.disable = rtp5905_disable,
	.enable_time = rtp5905_enable_time,
	.is_enabled = rtp5905_is_enabled,
	.set_suspend_enable = rtp5905_suspend_enable,
	.set_suspend_disable = rtp5905_suspend_disable,
	.set_suspend_voltage = rtp5905_set_suspend_voltage,
};

static struct regulator_ops rtp5905_ops = {
	.set_voltage = rtp5905_set_voltage,
	.get_voltage = rtp5905_get_voltage,
	.list_voltage = rtp5905_list_voltage,
	.enable = rtp5905_enable,
	.disable = rtp5905_disable,
	.enable_time = rtp5905_enable_time,
	.is_enabled = rtp5905_is_enabled,
	.set_suspend_enable = rtp5905_suspend_enable,
	.set_suspend_disable = rtp5905_suspend_disable,
	.set_suspend_voltage = rtp5905_set_suspend_voltage,
};

static struct rtp5905_regulator_info rtp5905_regulator_infos[] = {
	RTP5905_LDO("rtp5905_LDO1", RTP5905_ID_LDO1, 1800, 3300, 100, RTP5905_REG_LDO1VOUT_SET, 0, 7, RTP5905_REG_POWER_EN, 5, RTP5905_REG_STB_LDO1VOUT, RTP5905_REG_POWER_STB_EN, 0),
	RTP5905_LDO("rtp5905_LDO2", RTP5905_ID_LDO2, 1800, 3300, 50, RTP5905_REG_LDO2VOUT_SET, 0, 7, RTP5905_REG_POWER_EN, 7, RTP5905_REG_STB_LDO2VOUT, RTP5905_REG_POWER_STB_EN, 0),
	RTP5905_BUCK("rtp5905_BUCK1", RTP5905_ID_BUCK1, 700, 2275, 25, RTP5905_REG_DC1VOUT_SET, 0, 6, RTP5905_REG_POWER_EN, 0, RTP5905_REG_STB_DC1VOUT, RTP5905_REG_POWER_STB_EN, 0),
	RTP5905_BUCK("rtp5905_BUCK2", RTP5905_ID_BUCK2, 700, 2275, 25, RTP5905_REG_DC2VOUT_SET, 0, 6, RTP5905_REG_POWER_EN, 1, RTP5905_REG_STB_DC2VOUT, RTP5905_REG_POWER_STB_EN, 0),
	RTP5905_BUCK("rtp5905_BUCK3", RTP5905_ID_BUCK3, 700, 2275, 25, RTP5905_REG_DC3VOUT_SET, 0, 6, RTP5905_REG_POWER_EN, 2, RTP5905_REG_STB_DC3VOUT, RTP5905_REG_POWER_STB_EN, 0),
	RTP5905_BUCK("rtp5905_BUCK4", RTP5905_ID_BUCK4, 1700, 3500, 100, RTP5905_REG_DC4VOUT_SET, 0, 6, RTP5905_REG_POWER_EN, 3, RTP5905_REG_STB_DC4VOUT, RTP5905_REG_POWER_STB_EN, 0),
};

static struct of_regulator_match rtp5905_reg_matches [] = {
	{ .name = "rtp5905_LDO1", .driver_data = (void *)RTP5905_ID_LDO1},
	{ .name = "rtp5905_LDO2", .driver_data = (void *)RTP5905_ID_LDO2},
	{ .name = "rtp5905_BUCK1", .driver_data = (void *)RTP5905_ID_BUCK1},
	{ .name = "rtp5905_BUCK2", .driver_data = (void *)RTP5905_ID_BUCK2},
	{ .name = "rtp5905_BUCK3", .driver_data = (void *)RTP5905_ID_BUCK3},
	{ .name = "rtp5905_BUCK4", .driver_data = (void *)RTP5905_ID_BUCK4},
};

static int rtp5905_parse_dt_reg_data(struct platform_device *pdev)
{
	struct rtp5905_mfd_chip *rtp5905 = dev_get_drvdata(pdev->dev.parent);
	struct rtp5905_board *pmic_plat_data = dev_get_platdata(rtp5905->dev);

	struct device_node *np, *regulators;
	unsigned int prop;
	int i = 0, ret, count;

	np = pdev->dev.parent->of_node;
	regulators = of_get_child_by_name(np, "regulators");
	if (!regulators) {
		dev_err(&pdev->dev, "regulator node not found\n");
		return -EINVAL;
	}

	count = of_regulator_match(&pdev->dev, regulators, rtp5905_reg_matches, RTP5905_ID_REGULATORS_NUM);
	of_node_put(regulators);
	if ((count < 0) || (count > RTP5905_REGULATOR_NUM))
		return -EINVAL;

	for (i = 0; i < count; i++) {
		if (!rtp5905_reg_matches[i].init_data || !rtp5905_reg_matches[i].of_node)
			continue;
		pmic_plat_data->rtp5905_pmic_init_data[i] = rtp5905_reg_matches[i].init_data;
	}
	return 0;
}

static int rtp5905_regulator_probe(struct platform_device *pdev)
{
	struct rtp5905_mfd_chip *chip = dev_get_drvdata(pdev->dev.parent);
	struct regulator_config config = { };
	struct rtp5905_regulator_info *info;
	struct regulator_init_data *reg_data;
	struct regulator_dev *rdev;
	struct rtp5905_regu *pmic;
	struct rtp5905_board *pmic_plat_data;
	int i, err;

	pmic_plat_data = dev_get_platdata(chip->dev);
	if (!pmic_plat_data) {
		return -EINVAL;
	}

	err = rtp5905_parse_dt_reg_data(pdev);
	if (err < 0) {
		goto err;
	}

	pmic = devm_kzalloc(&pdev->dev, sizeof(*pmic), GFP_KERNEL);
	if (!pmic)
		return -ENOMEM;

	mutex_init(&pmic->mutex);
	pmic->chip = chip;
	platform_set_drvdata(pdev, pmic);

	switch (rtp5905_chip_id(chip)) {
	case RTP5905_ID:
		pmic->num_regulators = ARRAY_SIZE(rtp5905_regulator_infos);
		info = rtp5905_regulator_infos;
		break;
	default:
		pr_err("Invalid rtp chip version\n");
		return -ENODEV;
	}

	pmic->info = devm_kcalloc(&pdev->dev, pmic->num_regulators, sizeof(struct rtp5905_regulator_info *),
	        GFP_KERNEL);
	if (!pmic->info) {
		err = -ENOMEM;
		goto err;
	}

	pmic->rdev = devm_kcalloc(&pdev->dev, pmic->num_regulators, sizeof(struct regulator_dev *),
	        GFP_KERNEL);
	if (!pmic->rdev) {
		err = -ENOMEM;
		goto err;
	}

	for (i = 0; i < pmic->num_regulators && i < RTP5905_ID_REGULATORS_NUM; i++, info++) {
		reg_data = pmic_plat_data->rtp5905_pmic_init_data[i];
		if (!reg_data)
			continue;

		if (i < RTP5905_ID_BUCK1)
			info->desc.ops = &rtp5905_ops;
		else
			info->desc.ops = &rtp5905_ops_dcdc;

		pmic->info[i] = info;

		config.dev = chip->dev;
		config.init_data = reg_data;
		config.driver_data = pmic;
		config.regmap = chip->regmap;

		config.of_node = rtp5905_reg_matches[i].of_node;

		rdev = devm_regulator_register(&pdev->dev, &pmic->info[i]->desc, &config);
		if (IS_ERR(rdev)) {
			dev_err(chip->dev, "failed to register %s regulator\n", pdev->name);
			return PTR_ERR(rdev);
		}

		pmic->rdev[i] = rdev;
	}

	return 0;
err:
	return err;
}

static int rtp5905_regulator_remove(struct platform_device *pdev)
{
	struct rtp5905_regu *pmic = platform_get_drvdata(pdev);
	int i;

	for (i = 0; i < pmic->num_regulators; i++)
		regulator_unregister(pmic->rdev[i]);

	return 0;
}

static struct platform_driver rtp5905_regulator_driver = {
	.driver = {
		.name = "rtp5905-pmic",
		.owner = THIS_MODULE,
	},
	.probe = rtp5905_regulator_probe,
	.remove = rtp5905_regulator_remove,
};

static int __init rtp5905_regulator_init(void)
{
	return platform_driver_register(&rtp5905_regulator_driver);
}
subsys_initcall(rtp5905_regulator_init);

static void __exit rtp5905_regulator_exit(void)
{
	platform_driver_unregister(&rtp5905_regulator_driver);
}
module_exit(rtp5905_regulator_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Wind_Han <wind_han@realsil.com.cn>");
MODULE_DESCRIPTION("Realtek Power Manager Driver");
