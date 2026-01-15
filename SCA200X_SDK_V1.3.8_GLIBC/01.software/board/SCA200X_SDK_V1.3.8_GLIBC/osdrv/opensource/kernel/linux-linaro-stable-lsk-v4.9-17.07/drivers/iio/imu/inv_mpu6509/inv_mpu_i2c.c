/*
* Copyright (C) 2012 Invensense, Inc.
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

#include <linux/acpi.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <linux/i2c.h>
#include <linux/iio/iio.h>
#include <linux/module.h>
#include "inv_mpu_iio.h"
#include "designware_i2c.h"

static const struct regmap_config inv_mpu_regmap_config = {
	.reg_bits = 8,
	.val_bits = 8,
};

static int inv_mpu_i2c_enable(struct iio_dev *indio_dev)
{
	struct inv_mpu6509_state *st = iio_priv(indio_dev);
	int ret = 0;

	pr_debug("%s enter\n", __func__);
	ret = inv_mpu6509_set_power_itg(st, true);
	if (ret)
		return ret;

	ret = regmap_write_bits(st->map, st->reg->intf_config0,
	        INV_MPU6509_VAL_UI_SIFS_CFG_MASK,
	        INV_MPU6509_UI_SIFS_CFG_DISABLE_SPI);
	if (ret) {
		inv_mpu6509_set_power_itg(st, false);
		return ret;
	}

	ret = regmap_write(st->map, st->reg->drv_config,
	        INV_MPU6509_SLEW_RATE_12_36_NS << INV_MPU6509_BIT_OFFSET_I2C_SLEW_RATE
	        | INV_MPU6509_SLEW_RATE_12_36_NS << INV_MPU6509_BIT_OFFSET_SPI_SLEW_RATE);
	if (ret) {
		inv_mpu6509_set_power_itg(st, false);
		return ret;
	}

	dw_i2c_init();
	st->time_sync.reg_read = dw_i2c_reg_read;

	pr_debug("%s exit\n", __func__);
	return inv_mpu6509_set_power_itg(st, false);
}

static int inv_mpu6509_select(struct i2c_mux_core *muxc, u32 chan_id)
{
	return 0;
}

static int inv_mpu6509_deselect(struct i2c_mux_core *muxc, u32 chan_id)
{
	return 0;
}

static int inv_mpu_acpi_create_mux_client(struct i2c_client *client)
{
	return 0;
}

static void inv_mpu_acpi_delete_mux_client(struct i2c_client *client)
{
}

static const char *inv_mpu_match_acpi_device(struct device *dev, int *chip_id)
{
	const struct acpi_device_id *id;

	id = acpi_match_device(dev->driver->acpi_match_table, dev);
	if (!id)
		return NULL;

	*chip_id = (int)id->driver_data;

	return dev_name(dev);
}

/**
 *  inv_mpu_probe() - probe function.
 *  @client:          i2c client.
 *  @id:              i2c device id.
 *
 *  Returns 0 on success, a negative error code otherwise.
 */
static int inv_mpu_probe(struct i2c_client *client,
    const struct i2c_device_id *id)
{
	struct inv_mpu6509_state *st;
	int result, chip_type;
	struct regmap *regmap;
	const char *name;

	if (!i2c_check_functionality(client->adapter,
	        I2C_FUNC_SMBUS_I2C_BLOCK))
		return -EOPNOTSUPP;

	if (id) {
		chip_type = (int)id->driver_data;
		name = id->name;
	} else if (ACPI_HANDLE(&client->dev)) {
		name = inv_mpu_match_acpi_device(&client->dev, &chip_type);
		if (!name)
			return -ENODEV;
	} else {
		return -ENOSYS;
	}

	regmap = devm_regmap_init_i2c(client, &inv_mpu_regmap_config);
	if (IS_ERR(regmap)) {
		dev_err(&client->dev, "Failed to register i2c regmap %d\n",
		    (int)PTR_ERR(regmap));
		return PTR_ERR(regmap);
	}

	result = inv_mpu_core_probe(regmap, client->irq, name,
	        inv_mpu_i2c_enable, chip_type);
	if (result < 0)
		return result;

	st = iio_priv(dev_get_drvdata(&client->dev));
	st->muxc = i2c_mux_alloc(client->adapter, &client->dev,
	        1, 0, I2C_MUX_LOCKED,
	        inv_mpu6509_select,
	        inv_mpu6509_deselect);
	if (!st->muxc) {
		result = -ENOMEM;
		dev_err(&client->dev, "%s fail 1\n", __func__);
		goto out_unreg_device;
	}
	st->muxc->priv = dev_get_drvdata(&client->dev);
	result = i2c_mux_add_adapter(st->muxc, 0, 0, 0);
	if (result) {
		dev_err(&client->dev, "%s fail 2\n", __func__);
		goto out_unreg_device;
	}

	result = inv_mpu_acpi_create_mux_client(client);
	if (result) {
		dev_err(&client->dev, "%s fail 3\n", __func__);
		goto out_del_mux;
	}

	return 0;

out_del_mux:
	i2c_mux_del_adapters(st->muxc);
out_unreg_device:
	inv_mpu_core_remove(&client->dev);
	return result;
}

static int inv_mpu_remove(struct i2c_client *client)
{
	struct iio_dev *indio_dev = i2c_get_clientdata(client);
	struct inv_mpu6509_state *st = iio_priv(indio_dev);

	inv_mpu_acpi_delete_mux_client(client);
	i2c_mux_del_adapters(st->muxc);

	dw_i2c_remove();

	return inv_mpu_core_remove(&client->dev);
}

/*
 * device id table is used to identify what device can be
 * supported by this driver
 */
static const struct i2c_device_id inv_mpu_id[] = {
	{"mpu6509", INV_MPU6509},
	{}
};

MODULE_DEVICE_TABLE(i2c, inv_mpu_id);

static const struct acpi_device_id inv_acpi_match[] = {
	{"INVN6509", INV_MPU6509},
	{ },
};

MODULE_DEVICE_TABLE(acpi, inv_acpi_match);

static const struct of_device_id inv_mpu6509_of_match[] = {
	{ .compatible = "tdk,mpu6509" },
	{ },
};

MODULE_DEVICE_TABLE(of, inv_mpu6509_of_match);

static struct i2c_driver inv_mpu_driver = {
	.probe      =   inv_mpu_probe,
	.remove     =   inv_mpu_remove,
	.id_table   =   inv_mpu_id,
	.driver = {
		.acpi_match_table = ACPI_PTR(inv_acpi_match),
		.name   =   "inv-mpu6509-i2c",
		.of_match_table = of_match_ptr(inv_mpu6509_of_match),
		.pm     =       &inv_mpu_pmops,
	},
};

module_i2c_driver(inv_mpu_driver);

MODULE_AUTHOR("SmartChip Corporation");
MODULE_DESCRIPTION("Invensense device MPU6509 driver");
MODULE_LICENSE("GPL");
