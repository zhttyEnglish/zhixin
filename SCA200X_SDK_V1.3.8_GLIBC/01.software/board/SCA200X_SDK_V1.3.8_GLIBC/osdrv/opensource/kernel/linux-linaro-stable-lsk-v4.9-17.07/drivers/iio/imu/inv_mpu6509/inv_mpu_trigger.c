/*
* Copyright (C) 2022 SmartChip, Inc.
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

#include "inv_mpu_iio.h"

static int inv_mpu6509_setup_irq(struct iio_dev *indio_dev);
static void inv_mpu6509_cleanup_irq(struct iio_dev *indio_dev);

void inv_scan_query(struct iio_dev *indio_dev)
{
	struct inv_mpu6509_state  *st = iio_priv(indio_dev);

	st->chip_config.gyro_fifo_enable =
	    test_bit(INV_MPU6509_SCAN_GYRO_X,
	        indio_dev->active_scan_mask) ||
	    test_bit(INV_MPU6509_SCAN_GYRO_Y,
	        indio_dev->active_scan_mask) ||
	    test_bit(INV_MPU6509_SCAN_GYRO_Z,
	        indio_dev->active_scan_mask);

	st->chip_config.accl_fifo_enable =
	    test_bit(INV_MPU6509_SCAN_ACCL_X,
	        indio_dev->active_scan_mask) ||
	    test_bit(INV_MPU6509_SCAN_ACCL_Y,
	        indio_dev->active_scan_mask) ||
	    test_bit(INV_MPU6509_SCAN_ACCL_Z,
	        indio_dev->active_scan_mask);
}

/**
 *  inv_mpu6509_set_enable() - enable chip functions.
 *  @indio_dev: Device driver instance.
 *  @enable: enable/disable
 */
int inv_mpu6509_set_enable(struct iio_dev *indio_dev, bool enable)
{
	struct inv_mpu6509_state *st = iio_priv(indio_dev);
	int result;

	if (enable) {
		result = inv_mpu6509_set_power_itg(st, true);
		if (result)
			return result;
		//      inv_scan_query(indio_dev);
		if (st->chip_config.gyro_fifo_enable) {
			result = inv_mpu6509_switch_engine(st, true,
			        INV_MPU6509_VAL_GYRO_AXIS_MASK);
			if (result)
				return result;
		}
		if (st->chip_config.accl_fifo_enable) {
			result = inv_mpu6509_switch_engine(st, true,
			        INV_MPU6509_VAL_ACCEL_AXIS_MASK);
			if (result)
				return result;
		}
		result = inv_reset_fifo(indio_dev, INV_TIME_SYNC_PLL_RDY);
		if (result)
			return result;

		result = inv_mpu6509_setup_irq(indio_dev);
		if (result) {
			inv_mpu6509_cleanup_irq(indio_dev);
			return result;
		}
	} else {
		result = regmap_write_bits(st->map, st->reg->fifo_config1, INV_MPU6509_VAL_FIFO_EN_MASK, 0);
		if (result)
			return result;

		result = regmap_write(st->map, st->reg->int_source0, 0);
		if (result)
			return result;

		result = regmap_write(st->map, st->reg->int_source1, 0);
		if (result)
			return result;

		result = regmap_write(st->map, st->reg->int_source3, 0);
		if (result)
			return result;

		result = regmap_write(st->map, st->reg->int_source4, 0);
		if (result)
			return result;

		inv_mpu6509_cleanup_irq(indio_dev);

		result = regmap_write(st->map, st->reg->signal_path_rst, 0x0F);
		if (result)
			return result;

		result = regmap_write_bits(st->map, st->reg->dev_config,
		        INV_MPU6509_VAL_SOFT_RESET_MASK,
		        INV_MPU6509_SOFT_RESET_RESET);
		if (result)
			return result;
		msleep(INV_MPU6509_VAL_SOFT_RESET_WAIT_MS);

		result = inv_mpu6509_switch_engine(st, false,
		        INV_MPU6509_VAL_GYRO_AXIS_MASK);
		if (result)
			return result;

		result = inv_mpu6509_switch_engine(st, false,
		        INV_MPU6509_VAL_ACCEL_AXIS_MASK);
		if (result)
			return result;
		result = inv_mpu6509_set_power_itg(st, false);
		if (result)
			return result;
	}
	st->chip_config.enable = enable;

	return 0;
}

#ifdef CONFIG_OF
static int inv_mpu6509_get_irqflag(const struct device *dev,
    unsigned long *flag)
{
	if (dev->of_node) {
		unsigned int arr[2];
		int err = of_property_read_u32_array(dev->of_node,
		        "interrupts",
		        arr,
		        ARRAY_SIZE(arr));

		pr_debug("err %d, interrupts: %d %d", err, arr[0], arr[1]);
		*flag = arr[1];
		return err;
	}

	return 0;
}
#else
static int inv_mpu6509_get_irqflag(const struct device *dev,
    unsigned long *flag)
{
	(void)dev;
	(void)flag;
	return 0;
}
#endif

/**
 * inv_mpu_data_rdy_trigger_set_state() - set data ready interrupt state
 * @trig: Trigger instance
 * @state: Desired trigger state
 */
static int inv_mpu_data_rdy_trigger_set_state(struct iio_trigger *trig,
    bool state)
{
	return inv_mpu6509_set_enable(iio_trigger_get_drvdata(trig), state);
}

static const struct iio_trigger_ops inv_mpu_trigger_ops = {
	.owner = THIS_MODULE,
	.set_trigger_state = &inv_mpu_data_rdy_trigger_set_state,
};

int inv_mpu6509_probe_trigger(struct iio_dev *indio_dev)
{
	int ret;
	struct inv_mpu6509_state *st = iio_priv(indio_dev);
	unsigned long flag = IRQF_TRIGGER_HIGH;

	ret = inv_mpu6509_get_irqflag(indio_dev->dev.parent, &flag);
	if(ret) {
		pr_debug("get irq flag failed %d\n", ret);
		return ret;
	}

	st->trig = devm_iio_trigger_alloc(&indio_dev->dev,
	        "%s-dev%d",
	        indio_dev->name,
	        indio_dev->id);
	if (!st->trig)
		return -ENOMEM;

	ret = devm_request_irq(&indio_dev->dev, st->irq,
	        &iio_trigger_generic_data_rdy_poll,
	        flag | IRQF_ONESHOT,
	        "inv_mpu",
	        st->trig);
	if (ret)
		return ret;

	st->trig->dev.parent = regmap_get_device(st->map);
	st->trig->ops = &inv_mpu_trigger_ops;
	iio_trigger_set_drvdata(st->trig, indio_dev);

	ret = iio_trigger_register(st->trig);
	if (ret)
		return ret;

	indio_dev->trig = iio_trigger_get(st->trig);

	return 0;
}

void inv_mpu6509_remove_trigger(struct inv_mpu6509_state *st)
{
	iio_trigger_unregister(st->trig);
}

#ifdef CONFIG_INV_MPU6509_TRIGGER
int inv_read_int_status(struct inv_mpu6509_state *st)
{
	(void)st;
	return 0;
}

int inv_mpu6509_setup_trigger(struct iio_dev *indio_dev)
{
	int result;
	result = iio_triggered_buffer_setup(indio_dev,
	        inv_mpu6509_irq_handler,
	        inv_mpu6509_read_fifo,
	        NULL);
	if (result) {
		pr_debug("configure buffer fail %d\n", result);
		return result;
	}
	result = inv_mpu6509_probe_trigger(indio_dev);
	if (result) {
		pr_debug("trigger probe fail %d\n", result);
		iio_triggered_buffer_cleanup(indio_dev);
	}
	return result;
}

void inv_mpu6509_cleanup_trigger(struct iio_dev *indio_dev)
{
	struct inv_mpu6509_state *st = iio_priv(indio_dev);
	inv_mpu6509_remove_trigger(st);
	iio_triggered_buffer_cleanup(indio_dev);
}

void inv_mpu6509_trigger_notify_done(struct iio_dev *indio_dev)
{
	iio_trigger_notify_done(indio_dev->trig);
}

static int inv_mpu6509_setup_irq(struct iio_dev *indio_dev)
{
	return 0;
}

static void inv_mpu6509_cleanup_irq(struct iio_dev *indio_dev)
{
}

#else
int inv_read_int_status(struct inv_mpu6509_state *st, unsigned int *status)
{
	int result;

	result = regmap_read(st->map, st->reg->int_status, status);
	if(result || *status)
		pr_debug("%s result=%d status=%x\n", __func__, result, *status);

	return result;
}

int inv_mpu6509_setup_trigger(struct iio_dev *indio_dev)
{
	return 0;
}

void inv_mpu6509_cleanup_trigger(struct iio_dev *indio_dev)
{
}

void inv_mpu6509_trigger_notify_done(struct iio_dev *indio_dev)
{
}

static int inv_mpu6509_setup_irq(struct iio_dev *indio_dev)
{
	struct inv_mpu6509_state *st = iio_priv(indio_dev);
	int result;
	unsigned long flag = IRQF_TRIGGER_HIGH;

	result = inv_mpu6509_get_irqflag(indio_dev->dev.parent, &flag);
	if(result) {
		pr_debug("get irq flag failed %d\n", result);
		return result;
	}

	result = request_threaded_irq(st->irq,
	        inv_mpu6509_irq_handler,
	        inv_mpu6509_irq_thread,
	        flag | IRQF_ONESHOT,
	        indio_dev->name,
	        indio_dev);
	if (result) {
		pr_debug("request irq fail %d\n", result);
	}
	return result;
}

static void inv_mpu6509_cleanup_irq(struct iio_dev *indio_dev)
{
	struct inv_mpu6509_state *st = iio_priv(indio_dev);
	free_irq(st->irq, indio_dev);
	inv_mpu6509_free_buffer(indio_dev);
}

#endif
