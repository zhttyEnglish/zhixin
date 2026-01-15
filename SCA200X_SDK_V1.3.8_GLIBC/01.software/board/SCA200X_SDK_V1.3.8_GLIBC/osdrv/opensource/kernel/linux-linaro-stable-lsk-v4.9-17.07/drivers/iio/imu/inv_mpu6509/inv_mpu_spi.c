/*
* Copyright (C) 2022 SmartChip Corporation Inc.
*
* This software is licensed under the terms of the GNU General Public
* License version 2, as published by the Free Software Foundation, and
* may be copied, distributed, and modified under those terms.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*/
#include <linux/module.h>
#include <linux/acpi.h>
#include <linux/spi/spi.h>
#include <linux/regmap.h>
#include <linux/iio/iio.h>
#include "inv_mpu_iio.h"
#include "designware_spi.h"

static const struct regmap_config inv_mpu_regmap_config = {
	.reg_bits = 8,
	.val_bits = 8,
};

static int inv_mpu_spi_enable(struct iio_dev *indio_dev)
{
	struct inv_mpu6509_state *st = iio_priv(indio_dev);
	int ret = 0;
	struct spi_device *spi = to_spi_device(indio_dev->dev.parent);

	pr_debug("%s enter\n", __func__);
	ret = inv_mpu6509_set_power_itg(st, true);
	if (ret)
		return ret;

	ret = regmap_write_bits(st->map, st->reg->intf_config0,
	        INV_MPU6509_VAL_UI_SIFS_CFG_MASK,
	        INV_MPU6509_UI_SIFS_CFG_DISABLE_I2C);
	if (ret) {
		inv_mpu6509_set_power_itg(st, false);
		return ret;
	}

	ret = regmap_write_bits(st->map, st->reg->dev_config,
	        0x1, INV_MPU6509_SPI_MODE_0_3 << INV_MPU6509_BIT_OFFSET_SPI_MODE);
	if (ret) {
		inv_mpu6509_set_power_itg(st, false);
		return ret;
	}

	ret = regmap_write(st->map, st->reg->drv_config,
	        INV_MPU6509_SLEW_RATE_20_60_NS << INV_MPU6509_BIT_OFFSET_I2C_SLEW_RATE
	        | INV_MPU6509_SLEW_RATE_LESS_THAN_2_NS << INV_MPU6509_BIT_OFFSET_SPI_SLEW_RATE);
	if (ret) {
		inv_mpu6509_set_power_itg(st, false);
		return ret;
	}

	dw_spi_init(spi->cs_gpio, spi->chip_select);
	st->time_sync.reg_read = dw_spi_reg_read;

	pr_debug("%s exit\n", __func__);
	return inv_mpu6509_set_power_itg(st, false);
}

static int inv_mpu_probe(struct spi_device *spi)
{
	struct regmap *regmap;
	const struct spi_device_id *spi_id;
	const struct acpi_device_id *acpi_id;
	const char *name = NULL;
	enum inv_devices chip_type;

	if ((spi_id = spi_get_device_id(spi))) {
		chip_type = (enum inv_devices)spi_id->driver_data;
		name = spi_id->name;
	} else if ((acpi_id = acpi_match_device(spi->dev.driver->acpi_match_table, &spi->dev))) {
		chip_type = (enum inv_devices)acpi_id->driver_data;
	} else {
		return -ENODEV;
	}

	regmap = devm_regmap_init_spi(spi, &inv_mpu_regmap_config);
	if (IS_ERR(regmap)) {
		dev_err(&spi->dev, "Failed to register spi regmap %d\n",
		    (int)PTR_ERR(regmap));
		return PTR_ERR(regmap);
	}

	return inv_mpu_core_probe(regmap, spi->irq, name,
	        inv_mpu_spi_enable, chip_type);
}

static int inv_mpu_remove(struct spi_device *spi)
{
	dw_spi_remove();
	return inv_mpu_core_remove(&spi->dev);
}

/*
 * device id table is used to identify what device can be
 * supported by this driver
 */
static const struct spi_device_id inv_mpu_id[] = {
	{"mpu6509", INV_MPU6509},
	{}
};

MODULE_DEVICE_TABLE(spi, inv_mpu_id);

static const struct acpi_device_id inv_acpi_match[] = {
	{"INVN6509", INV_MPU6509},
	{ },
};
MODULE_DEVICE_TABLE(acpi, inv_acpi_match);

static struct spi_driver inv_mpu_driver = {
	.probe      =   inv_mpu_probe,
	.remove     =   inv_mpu_remove,
	.id_table   =   inv_mpu_id,
	.driver = {
		.acpi_match_table = ACPI_PTR(inv_acpi_match),
		.name   =   "inv-mpu6509-spi",
		.pm     =       &inv_mpu_pmops,
	},
};

module_spi_driver(inv_mpu_driver);

MODULE_AUTHOR("SmartChip Corporation");
MODULE_DESCRIPTION("Invensense device MPU6509 driver");
MODULE_LICENSE("GPL");
