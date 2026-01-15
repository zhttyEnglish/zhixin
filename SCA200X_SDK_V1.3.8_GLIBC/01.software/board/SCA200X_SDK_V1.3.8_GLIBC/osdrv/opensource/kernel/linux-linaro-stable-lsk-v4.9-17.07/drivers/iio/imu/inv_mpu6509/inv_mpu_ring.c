/*
* Copyright (C) 2022 SmartChipc, Inc.
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
#include <linux/slab.h>
#include <linux/err.h>
#include <linux/delay.h>
#include <linux/sysfs.h>
#include <linux/jiffies.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/kfifo.h>
#include <linux/poll.h>
#include <linux/timekeeping.h>
#include <linux/spi/spi.h>
#include "inv_mpu_iio.h"
#include "inv_mpu_ioctl.h"

void dump_reg_bank(struct inv_mpu6509_state  *st)
{
	int result;
	unsigned char regval[0x80];
	int i = 0;
	unsigned int val;

	for(i = 0; i < 0x80; i++) {
		result = regmap_read(st->map, i, &val);
		regval[i] = val;
	}

	for(i = 0; i < 8; i++) {
		pr_debug("%02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x",
		    regval[i * 0x10 + 0], regval[i * 0x10 + 1], regval[i * 0x10 + 2], regval[i * 0x10 + 3],
		    regval[i * 0x10 + 4], regval[i * 0x10 + 5], regval[i * 0x10 + 6], regval[i * 0x10 + 7],
		    regval[i * 0x10 + 8], regval[i * 0x10 + 9], regval[i * 0x10 + 10], regval[i * 0x10 + 11],
		    regval[i * 0x10 + 12], regval[i * 0x10 + 13], regval[i * 0x10 + 14], regval[i * 0x10 + 15]);
		pr_debug("\n");
	}
}

void dump_fifo_data(u8 *data, unsigned int len)
{
	int i = 0;
	char data_dbg_str[INV_MPU6509_FIFO_PACKETS_LEN_MODE3 * 3 + 1] = {0};
	char data_dbg_str_tmp[4] = {0};

	for(i = 0; i < len; i++) {
		sprintf(data_dbg_str_tmp, "%02x ", data[i]);
		strcat(data_dbg_str, data_dbg_str_tmp);
	}
	pr_debug("%s", data_dbg_str);
}

static int inv_mpu6509_get_fifo_bytes_per_pkt(struct inv_mpu6509_state *st)
{
	if (st->chip_config.accl_fifo_enable && st->chip_config.gyro_fifo_enable) {
		return INV_MPU6509_FIFO_PACKETS_LEN_MODE3;
	} else if (st->chip_config.gyro_fifo_enable) {
		return INV_MPU6509_FIFO_PACKETS_LEN_MODE2;
	} else if (st->chip_config.accl_fifo_enable) {
		return INV_MPU6509_FIFO_PACKETS_LEN_MODE1;
	}

	pr_debug("%s error sensor config\n", __func__);
	return 0;
}

static int inv_mpu6509_read_lost_pkt_cnt(struct inv_mpu6509_state *st, unsigned int *cnt)
{
	int result;
	unsigned int regval;
	unsigned int lost_pkt_cnt = 0;

	result = regmap_read(st->map, st->reg->fifo_lost_pkt0, &regval);
	if (result)
		return result;

	lost_pkt_cnt = regval;

	result = regmap_read(st->map, st->reg->fifo_lost_pkt1, &regval);
	if (result)
		return result;

	*cnt = lost_pkt_cnt << 8 | regval;

	pr_debug("%s result=%d lost_pkt_cnt=%d\n", __func__, result, *cnt);

	return result;
}

static int inv_mpu6509_read_fifo_cnt(struct inv_mpu6509_state *st, unsigned int *cnt)
{
	int result;
	unsigned int regval;
	u16 fifo_count;

	result = regmap_read(st->map, st->reg->fifo_count_l, &regval);
	pr_debug("%s fifo_count_l result %d regval %d\n", __func__, result, regval);
	if (result)
		return result;

	fifo_count = (u16)regval;

	result = regmap_read(st->map, st->reg->fifo_count_h, &regval);
	pr_debug("%s fifo_count_h result %d regval %d\n", __func__, result, regval);
	if (result)
		return result;

	fifo_count = (u16)regval << 8 | fifo_count;
	*cnt = fifo_count;

	pr_debug("%s result=%d fifo_count=%d\n", __func__, result, fifo_count);

	return result;
}

int inv_enable_fifo(struct inv_mpu6509_state  *st, bool gyro_fifo_enable, bool accl_fifo_enable)
{
	int result;
	u8 d;
	unsigned int wm;

	st->chip_config.gyro_fifo_enable = gyro_fifo_enable;
	st->chip_config.accl_fifo_enable = accl_fifo_enable;

	d = INV_MPU6509_FIFO_ENABLE << INV_MPU6509_BIT_OFFSET_FIFO_RESUME_PARTIAL_RD
	    | INV_MPU6509_FIFO_ENABLE << INV_MPU6509_BIT_OFFSET_FIFO_TEMP_EN;
	if (st->chip_config.gyro_fifo_enable)
		d |= INV_MPU6509_FIFO_ENABLE << INV_MPU6509_BIT_OFFSET_FIFO_GYRO_EN;
	if (st->chip_config.accl_fifo_enable)
		d |= INV_MPU6509_FIFO_ENABLE << INV_MPU6509_BIT_OFFSET_FIFO_ACCEL_EN;
	if (st->chip_config.accl_fifo_enable && st->chip_config.gyro_fifo_enable)
		d |= INV_MPU6509_FIFO_ENABLE << INV_MPU6509_BIT_OFFSET_FIFO_TMST_FSYNC_EN;

	if(st->chip_config.fifo_wm > 1  && st->time_sync.status == INV_TIME_SYNC_NORMAL)
		d |= INV_MPU6509_FIFO_ENABLE << INV_MPU6509_BIT_OFFSET_FIFO_WM_GT_TH;

	result = regmap_write(st->map, st->reg->fifo_config1, d);
	if (result)
		return result;

	if(st->chip_config.fifo_wm > 1 && st->time_sync.status == INV_TIME_SYNC_NORMAL) {
		wm = st->chip_config.fifo_wm * inv_mpu6509_get_fifo_bytes_per_pkt(st);
		result = regmap_write(st->map, st->reg->fifo_config2, wm & 0xFF);
		if (result)
			return result;

		result = regmap_write(st->map, st->reg->fifo_config3, wm >> 8);
		if (result)
			return result;
	}

	return 0;
}

int inv_reset_fifo(struct iio_dev *indio_dev, enum inv_mpu6509_time_sync_status status)
{
	int result;
	struct inv_mpu6509_state  *st = iio_priv(indio_dev);
	u8 d;

	/* disable interrupt */
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

	/* disable the sensor output to FIFO */
	result = regmap_write(st->map, st->reg->fifo_config1, 0);
	if (result)
		goto reset_fifo_fail;

	/* flush FIFO*/
	result = regmap_write(st->map, st->reg->signal_path_rst,
	        INV_MPU6509_ABORT_AND_RESET_ENABLE << INV_MPU6509_BIT_OFFSET_ABORT_AND_RESET
	        | INV_MPU6509_TMST_STROBE_ENABLE << INV_MPU6509_BIT_OFFSET_TMST_STROBE
	        | INV_MPU6509_FIFO_FLUSH_ENABLE << INV_MPU6509_BIT_OFFSET_FIFO_FLUSH);
	if (result)
		goto reset_fifo_fail;

	/* set FIFO to bypass mode */
	result = regmap_write(st->map, st->reg->fifo_config, INV_MPU6509_FIFO_MODE_BYPASS << INV_MPU6509_BIT_OFFSET_FIFO_MODE);
	if (result)
		goto reset_fifo_fail;

	/*
	    if(st->chip_config.use_tmst) {
	        if (st->chip_config.accl_fifo_enable && st->chip_config.gyro_fifo_enable) {
	            d = 0x20
	                | (st->chip_config.tmst_res) << INV_MPU6509_BIT_OFFSET_TMST_RES
	                | INV_MPU6509_TMST_TO_REG_ENABLE << INV_MPU6509_BIT_OFFSET_TMST_TO_REGS_EN
	                | INV_MPU6509_TMST_FSYNC_ENABLE << INV_MPU6509_BIT_OFFSET_TMST_FSYNC_EN
	                | INV_MPU6509_TMST_ENABLE << INV_MPU6509_BIT_OFFSET_TMST_EN;
	            result = regmap_write(st->map, st->reg->tmst_config, d);
	            if (result)
	                goto reset_fifo_fail;
	        }
	    }
	*/

	st->time_sync.status = status;
	st->time_sync.base_kt = 0;
	st->time_sync.ref_kt = 0;
	st->time_sync.pkt_id = 0;
	st->time_sync.lost_pkt_cnt = 0;

	/* set FIFO to stream mode */
	result = regmap_write(st->map, st->reg->fifo_config,
	        INV_MPU6509_FIFO_MODE_STREAM_TO_FIFO << INV_MPU6509_BIT_OFFSET_FIFO_MODE);
	if (result)
		goto reset_fifo_fail;

	/* enable sensor output to FIFO, partial rd */
	result = inv_enable_fifo(st, st->chip_config.gyro_fifo_enable, st->chip_config.accl_fifo_enable);
	if (result)
		goto reset_fifo_fail;

	/* enable interrupt */
	if (st->chip_config.accl_fifo_enable ||
	    st->chip_config.gyro_fifo_enable) {
		if(st->chip_config.fifo_wm > 1 && status == INV_TIME_SYNC_NORMAL)
			d = INV_MPU6509_INT_ROUTE_INT1_2_ENABLE << INV_MPU6509_BIT_OFFSET_FIFO_THS_INT1_EN;
		else
			d = INV_MPU6509_INT_ROUTE_INT1_2_ENABLE << INV_MPU6509_BIT_OFFSET_UI_DRDY_INT1_EN;
		result = regmap_write(st->map, st->reg->int_source0, d);
		if (result)
			return result;
	}

	pr_debug("reset fifo success\n");
	return 0;

reset_fifo_fail:
	pr_debug("reset fifo failed %d\n", result);
	if(st->chip_config.fifo_wm > 1)
		d = INV_MPU6509_INT_ROUTE_INT1_2_ENABLE << INV_MPU6509_BIT_OFFSET_FIFO_THS_INT1_EN;
	else
		d = INV_MPU6509_INT_ROUTE_INT1_2_ENABLE << INV_MPU6509_BIT_OFFSET_UI_DRDY_INT1_EN;
	result = regmap_write(st->map, st->reg->int_source0, d);
	return result;
}

static unsigned int odr_factor[INV_MPU6509_ACCEL_ODR_MAX] = {
	0, 1, 2, 4, 8, 16, 32, 160, 320, 640, 1280, 2560, 5120, 10240, 20480, 64
};

static unsigned int sync_cycle[INV_MPU6509_ACCEL_ODR_MAX] = {
	1, 1, 1, 2, 4, 8, 4, 32, 64, 128, 256, 512, 1024, 1024, 1024, 32
};

#ifdef CONFIG_INV_MPU6509_TRIGGER
#define INV_MPU6509_IRQ_GET_DEV(_p)       (((struct iio_poll_func *)_p)->indio_dev)

int inv_mpu6509_init_buffer(struct iio_dev *indio_dev)
{
	return 0;
}

void inv_mpu6509_free_buffer(struct iio_dev *indio_dev)
{
}

static int inv_mpu6509_write_buffer(struct iio_dev *indio_dev, const void *data)
{
	return iio_push_to_buffers(indio_dev, data);
}

static int inv_mpu6509_write_buffer_with_tmst(struct iio_dev *indio_dev, const void *data, s64 timestamp)
{
	return iio_push_to_buffers_with_timestamp(indio_dev, data, timestamp);
}
#else
#define INV_MPU6509_IRQ_GET_DEV(_p)       (_p)

int inv_mpu6509_init_buffer(struct iio_dev *indio_dev)
{
	if(!indio_dev->buffer) {
		struct inv_mpu6509_state *st = iio_priv(indio_dev);
		struct iio_buffer *buffer;
		int result;

		result = inv_mpu6509_get_fifo_bytes_per_pkt(st);
		if(!result)
			return -EINVAL;

		buffer = iio_kfifo_allocate();
		if (!buffer)
			return -ENOMEM;

		pr_debug("%s buffer %p\n", __func__, buffer);

		iio_device_attach_buffer(indio_dev, buffer);

		buffer->bytes_per_datum = inv_mpu6509_get_fifo_bytes_per_pkt(st);
		if(st->chip_config.use_tmst)
			buffer->bytes_per_datum += INV_MPU6509_SOFT_TIMESTATMP_LEN;

		buffer->length = INV_MPU6509_OUTPUT_BUFF_LEN;

		result = buffer->access->request_update(buffer);
		pr_debug("%s kf %p %d %d result %d", __func__, buffer, buffer->bytes_per_datum, buffer->length, result);
		return result;
	}

	return 0;
}

void inv_mpu6509_free_buffer(struct iio_dev *indio_dev)
{
	iio_kfifo_free(indio_dev->buffer);
	indio_dev->buffer = NULL;
}

static int inv_mpu6509_write_buffer(struct iio_dev *indio_dev, const void *data)
{
	int result = 0;

	if(!indio_dev->buffer) {
		pr_debug("buffer not init\n");
		return 0;
	}

	result = indio_dev->buffer->access->store_to(indio_dev->buffer, data);
	if(result)
		return result;

	wake_up_interruptible_poll(&indio_dev->buffer->pollq, POLLIN | POLLRDNORM);
	return 0;
}

static int inv_mpu6509_write_buffer_with_tmst(struct iio_dev *indio_dev, const void *data, s64 timestamp)
{
	struct inv_mpu6509_state *st = iio_priv(indio_dev);

	int bpfp = inv_mpu6509_get_fifo_bytes_per_pkt(st);

	*((int64_t *)((u8 *)data + bpfp)) = timestamp;

	return inv_mpu6509_write_buffer(indio_dev, data);
}
#endif

static int inv_mpu6509_read_fifo_no_tmst(int irq, struct iio_dev *indio_dev)
{
	struct inv_mpu6509_state *st = iio_priv(indio_dev);
	u8 *data = NULL;
	int bpfp;
	int result = 0;
	unsigned int regval;
	u16 fifo_count;

	if(!indio_dev->buffer) {
		pr_debug("%s buffer not init\n", __func__);
		goto end_session_nofree;
	}

	bpfp = inv_mpu6509_get_fifo_bytes_per_pkt(st);
	if(bpfp == 0)
		goto end_session_nofree;

	data = kzalloc(indio_dev->buffer->bytes_per_datum, GFP_KERNEL);
	if(!data) {
		result = -ENOMEM;
		goto end_session_nofree;
	}

	/*
	 * read fifo_count register to know how many bytes inside FIFO
	 * right now
	 */
	result = inv_mpu6509_read_fifo_cnt(st, &regval);
	if (result)
		goto end_session;
	fifo_count = regval;
	if (fifo_count < bpfp)
		goto end_session;
	/* fifo count can't be odd number, if it is odd, reset fifo*/
	if (fifo_count & 1)
		goto flush_fifo;

	while (fifo_count >= bpfp) {
		result = regmap_bulk_read(st->map, st->reg->fifo_data,
		        data, bpfp);
		if (result) {
			pr_debug("%d %d\n", __LINE__, result);
			goto flush_fifo;
		}
		//dump_fifo_data(data, bpfp);

		result = inv_mpu6509_write_buffer(indio_dev, data);
		if (result) {
			pr_debug("%d %d %d\n", __LINE__, fifo_count, result);
			goto flush_fifo;
		}
		fifo_count -= bpfp;
	}

end_session:
	kfree(data);
end_session_nofree:
	inv_read_int_status(st, &regval);
	inv_mpu6509_trigger_notify_done(indio_dev);
	pr_debug("%s result=%x exit end_session\n", __func__, result);
	return result;

flush_fifo:
	kfree(data);
	inv_read_int_status(st, &regval);
	/* Flush HW and SW FIFOs. */
	inv_reset_fifo(indio_dev, INV_TIME_SYNC_NORMAL);
	inv_mpu6509_trigger_notify_done(indio_dev);
	pr_debug("%s result=%x exit flush_fifo\n", __func__, result);
	return result;
}

/**
 * inv_mpu6509_read_fifo() - Transfer data from hardware FIFO to KFIFO.
 */
static int inv_mpu6509_read_fifo(int irq, struct iio_dev *indio_dev)
{
	struct inv_mpu6509_state *st = iio_priv(indio_dev);
	u8 *data = NULL;
	s64 timestamp;
	unsigned int regval;
	int bpfp;
	int result = 0;
	u16 offset;
	u16 fifo_count;

	if(!indio_dev->buffer) {
		pr_debug("%s buffer not init\n", __func__);
		return 0;
	}

	bpfp = inv_mpu6509_get_fifo_bytes_per_pkt(st);
	if(bpfp == 0)
		return 0;

	data = kzalloc(indio_dev->buffer->bytes_per_datum, GFP_KERNEL);
	if(!data)
		return -ENOMEM;

	/*
	 * read fifo_count register to know how many bytes inside FIFO
	 * right now
	 */
	result = inv_mpu6509_read_fifo_cnt(st, &regval);
	if (result)
		goto end_session;
	fifo_count = regval;
	if (fifo_count < bpfp)
		goto end_session;
	/* fifo count can't be odd number, if it is odd, reset fifo*/
	if (fifo_count & 1)
		goto flush_fifo;

	while (fifo_count >= bpfp) {
		result = regmap_bulk_read(st->map, st->reg->fifo_data,
		        data, bpfp);
		if (result) {
			pr_debug("%d %d\n", __LINE__, result);
			goto flush_fifo;
		}
		//dump_fifo_data(data, bpfp);

		offset = st->time_sync.pkt_id % sync_cycle[st->chip_config.odr];
		timestamp = st->time_sync.ref_kt + offset * odr_factor[st->chip_config.odr] * 31250;
		result = inv_mpu6509_write_buffer_with_tmst(indio_dev, data, timestamp);
		if(result)
			inv_reset_fifo(indio_dev, INV_TIME_SYNC_NORMAL);

		st->time_sync.pkt_id++;
		fifo_count -= bpfp;
	}

end_session:
	kfree(data);
	return result;

flush_fifo:
	kfree(data);
	/* Flush HW and SW FIFOs. */
	inv_reset_fifo(indio_dev, INV_TIME_SYNC_NORMAL);
	return result;
}

int inv_mpu6509_update_ref_tmst(struct inv_mpu6509_state *st, s64 *timestamp)
{
	int result = 0;
	unsigned int regval = 0;
	unsigned long flags;
	u16 fifo_count_l = 0;
	u16 fifo_count_h = 0;
	s64 timestamp1;

	timestamp1 = ktime_get_raw_ns();
	pr_debug("before %lld", timestamp1);

	spin_lock_irqsave(&st->time_stamp_lock, flags);
	do {
		result = st->time_sync.reg_read(st->reg->fifo_count_l, &regval);
		if (result)
			break;
		if(regval) {
			fifo_count_l = (u16)regval;
			break;
		}

		result = st->time_sync.reg_read(st->reg->fifo_count_h, &regval);
		if (result)
			break;
		if(regval) {
			fifo_count_h = (u16)regval << 8;
			break;
		}
	} while(fifo_count_l == 0 && fifo_count_h == 0);
	spin_unlock_irqrestore(&st->time_stamp_lock, flags);

	if (!result) {
		*timestamp = ktime_get_raw_ns();
		pr_debug("after %lld %lld fifo_count_l=%d fifo_count_h=%d", *timestamp, *timestamp - timestamp1, fifo_count_l,
		    fifo_count_h);
	} else {
		pr_debug("after reg failed %d", result);
	}

	return result;
}

/**
 * inv_mpu6509_irq_handler() - Cache a timestamp at each data ready interrupt.
 */
irqreturn_t inv_mpu6509_irq_handler(int irq, void *p)
{
	struct iio_dev *indio_dev = INV_MPU6509_IRQ_GET_DEV(p);
	struct inv_mpu6509_state *st = iio_priv(indio_dev);

	if (st->chip_config.use_tmst) {
		if (st->chip_config.accl_fifo_enable || st->chip_config.gyro_fifo_enable) {
			enum inv_mpu6509_time_sync_status status = st->time_sync.status;
			s64 timestamp = ktime_get_raw_ns();
			pr_debug("%s timestamp %lld %lld status=%d\n", __func__, timestamp, timestamp / 1000000, status);

			if(status == INV_TIME_SYNC_PLL_RDY)
				st->time_sync.base_kt = timestamp;
		}
	}

	return IRQ_WAKE_THREAD;
}

irqreturn_t inv_mpu6509_irq_thread(int irq, void *p)
{
	struct iio_dev *indio_dev = INV_MPU6509_IRQ_GET_DEV(p);
	struct inv_mpu6509_state *st = iio_priv(indio_dev);

	int result = 0;
	unsigned int regval = 0;
	u8 *data = NULL;
	s64 timestamp;
	unsigned int wm;
	int bpfp;

	mutex_lock(&indio_dev->mlock);

	if(!st->chip_config.use_tmst) {
		result = inv_mpu6509_read_fifo_no_tmst(irq, indio_dev);
		goto END_DIRECT;
	}

	if(st->time_sync.status == INV_TIME_SYNC_PLL_RDY) {
		result = inv_read_int_status(st, &regval);
		if(regval & INV_MPU6509_INT_STATUS_SET << INV_MPU6509_BIT_OFFSET_DATA_RDY_INT) {
			result = inv_mpu6509_update_ref_tmst(st, &timestamp);
			if(result)
				goto END_FLUSH_FIFO;
			st->time_sync.ref_kt = timestamp;
			st->time_sync.pkt_id++;
			st->time_sync.status = INV_TIME_SYNC_NORMAL;

			if(st->chip_config.fifo_wm == 1) {
				bpfp = inv_mpu6509_get_fifo_bytes_per_pkt(st);
				if(bpfp == 0)
					goto END_DIRECT;

				data = kzalloc(indio_dev->buffer->bytes_per_datum, GFP_KERNEL);
				if(!data) {
					result = -ENOMEM;
					goto END_DIRECT;
				}

				result = regmap_bulk_read(st->map, st->reg->fifo_data,
				        data, bpfp);
				if (result) {
					pr_debug("%d %d\n", __LINE__, result);
					goto END_FLUSH_FIFO;
				}
				//dump_fifo_data(data, bpfp);

				result = inv_mpu6509_write_buffer_with_tmst(indio_dev, data, timestamp);
				if (result) {
					pr_debug("%d %d\n", __LINE__, result);
					goto END_FLUSH_FIFO;
				}
				goto END_SESSION;
			} else {
				result = regmap_write_bits(st->map, st->reg->fifo_config1,
				        0x1 << INV_MPU6509_BIT_OFFSET_FIFO_WM_GT_TH, INV_MPU6509_FIFO_ENABLE);
				if (result)
					goto END_DIRECT;

				wm = st->chip_config.fifo_wm * inv_mpu6509_get_fifo_bytes_per_pkt(st);
				result = regmap_write(st->map, st->reg->fifo_config2, wm & 0xFF);
				if (result)
					goto END_DIRECT;

				result = regmap_write(st->map, st->reg->fifo_config3, wm >> 8);
				if (result)
					goto END_DIRECT;

				result = regmap_write_bits(st->map, st->reg->int_source0,
				        0x1 << INV_MPU6509_BIT_OFFSET_FIFO_THS_INT1_EN, INV_MPU6509_INT_ROUTE_INT1_2_ENABLE);

				goto END_DIRECT;
			}
		} else {
			goto END_DIRECT;
		}
	} else {
		result = inv_read_int_status(st, &regval);
		if((st->time_sync.pkt_id % sync_cycle[st->chip_config.odr]) == 0) {
			result = inv_mpu6509_update_ref_tmst(st, &timestamp);
			if(result)
				goto END_FLUSH_FIFO;
			st->time_sync.ref_kt = timestamp;
		}
		result = inv_mpu6509_read_fifo(irq, indio_dev);
		goto END_DIRECT;
	}

END_FLUSH_FIFO:
	/* Flush HW and SW FIFOs. */
	inv_reset_fifo(indio_dev, INV_TIME_SYNC_NORMAL);

END_SESSION:
	kfree(data);

END_DIRECT:
	mutex_unlock(&indio_dev->mlock);
	inv_mpu6509_trigger_notify_done(indio_dev);
	pr_debug("%s result=%x\n", __func__, result);
	return IRQ_HANDLED;
}

