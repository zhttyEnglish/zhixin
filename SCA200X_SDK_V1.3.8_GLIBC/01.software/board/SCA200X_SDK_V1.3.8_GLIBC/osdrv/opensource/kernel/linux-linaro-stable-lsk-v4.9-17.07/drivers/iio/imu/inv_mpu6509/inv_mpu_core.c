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
#include <linux/i2c.h>
#include <linux/err.h>
#include <linux/delay.h>
#include <linux/sysfs.h>
#include <linux/jiffies.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/kfifo.h>
#include <linux/spinlock.h>
#include <linux/iio/iio.h>
#include "inv_mpu_iio.h"
#include "inv_mpu_ioctl.h"

#define INV_MPU6509_AAF_BW_CFG_STATIC(_deltsqr, _bitshift) \
    { \
        INV_MPU6509_DELTSQR_LOW(_deltsqr), \
        INV_MPU6509_DELTSQR_HIGH(_deltsqr) | ((_bitshift) << INV_MPU6509_BIT_OFFSET_AAF_BITSHIFT) \
    }

#define INV_MPU6509_GYRO_CONFIG_STATIC4(_delt) \
    aaf_bw_cfg_6509[_delt].deltsqr_lo

#define INV_MPU6509_GYRO_CONFIG_STATIC5(_delt) \
    aaf_bw_cfg_6509[_delt].deltsqr_hi_bitshift

#define INV_MPU6509_ACCEL_CONFIG_STATIC3(_delt) \
    aaf_bw_cfg_6509[_delt].deltsqr_lo

#define INV_MPU6509_ACCEL_CONFIG_STATIC4(_delt) \
    aaf_bw_cfg_6509[_delt].deltsqr_hi_bitshift

static const struct inv_mpu6509_aaf_bw_cfg aaf_bw_cfg_6509[INV_MPU6509_GYRO_AAF_DELT_MAX] = {
	INV_MPU6509_AAF_BW_CFG_STATIC(0, 0),
	INV_MPU6509_AAF_BW_CFG_STATIC(1, 15),
	INV_MPU6509_AAF_BW_CFG_STATIC(4, 13),
	INV_MPU6509_AAF_BW_CFG_STATIC(9, 12),
	INV_MPU6509_AAF_BW_CFG_STATIC(16, 11),
	INV_MPU6509_AAF_BW_CFG_STATIC(25, 10),
	INV_MPU6509_AAF_BW_CFG_STATIC(36, 10),
	INV_MPU6509_AAF_BW_CFG_STATIC(49, 9),
	INV_MPU6509_AAF_BW_CFG_STATIC(64, 9),
	INV_MPU6509_AAF_BW_CFG_STATIC(81, 9),
	INV_MPU6509_AAF_BW_CFG_STATIC(100, 8),
	INV_MPU6509_AAF_BW_CFG_STATIC(122, 8),
	INV_MPU6509_AAF_BW_CFG_STATIC(144, 8),
	INV_MPU6509_AAF_BW_CFG_STATIC(170, 8),
	INV_MPU6509_AAF_BW_CFG_STATIC(196, 7),
	INV_MPU6509_AAF_BW_CFG_STATIC(224, 7),
	INV_MPU6509_AAF_BW_CFG_STATIC(256, 7),
	INV_MPU6509_AAF_BW_CFG_STATIC(288, 7),
	INV_MPU6509_AAF_BW_CFG_STATIC(324, 7),
	INV_MPU6509_AAF_BW_CFG_STATIC(360, 6),
	INV_MPU6509_AAF_BW_CFG_STATIC(400, 6),
	INV_MPU6509_AAF_BW_CFG_STATIC(440, 6),
	INV_MPU6509_AAF_BW_CFG_STATIC(488, 6),
	INV_MPU6509_AAF_BW_CFG_STATIC(528, 6),
	INV_MPU6509_AAF_BW_CFG_STATIC(576, 6),
	INV_MPU6509_AAF_BW_CFG_STATIC(624, 6),
	INV_MPU6509_AAF_BW_CFG_STATIC(680, 6),
	INV_MPU6509_AAF_BW_CFG_STATIC(736, 5),
	INV_MPU6509_AAF_BW_CFG_STATIC(784, 5),
	INV_MPU6509_AAF_BW_CFG_STATIC(848, 5),
	INV_MPU6509_AAF_BW_CFG_STATIC(896, 5),
	INV_MPU6509_AAF_BW_CFG_STATIC(960, 5),
	INV_MPU6509_AAF_BW_CFG_STATIC(1024, 5),
	INV_MPU6509_AAF_BW_CFG_STATIC(1088, 5),
	INV_MPU6509_AAF_BW_CFG_STATIC(1152, 5),
	INV_MPU6509_AAF_BW_CFG_STATIC(1232, 5),
	INV_MPU6509_AAF_BW_CFG_STATIC(1296, 5),
	INV_MPU6509_AAF_BW_CFG_STATIC(1376, 4),
	INV_MPU6509_AAF_BW_CFG_STATIC(1440, 4),
	INV_MPU6509_AAF_BW_CFG_STATIC(1536, 4),
	INV_MPU6509_AAF_BW_CFG_STATIC(1600, 4),
	INV_MPU6509_AAF_BW_CFG_STATIC(1696, 4),
	INV_MPU6509_AAF_BW_CFG_STATIC(1760, 4),
	INV_MPU6509_AAF_BW_CFG_STATIC(1856, 4),
	INV_MPU6509_AAF_BW_CFG_STATIC(1952, 4),
	INV_MPU6509_AAF_BW_CFG_STATIC(2016, 4),
	INV_MPU6509_AAF_BW_CFG_STATIC(2112, 4),
	INV_MPU6509_AAF_BW_CFG_STATIC(2208, 4),
	INV_MPU6509_AAF_BW_CFG_STATIC(2304, 4),
	INV_MPU6509_AAF_BW_CFG_STATIC(2400, 4),
	INV_MPU6509_AAF_BW_CFG_STATIC(2496, 4),
	INV_MPU6509_AAF_BW_CFG_STATIC(2592, 4),
	INV_MPU6509_AAF_BW_CFG_STATIC(2720, 4),
	INV_MPU6509_AAF_BW_CFG_STATIC(2816, 3),
	INV_MPU6509_AAF_BW_CFG_STATIC(2944, 3),
	INV_MPU6509_AAF_BW_CFG_STATIC(3008, 3),
	INV_MPU6509_AAF_BW_CFG_STATIC(3136, 3),
	INV_MPU6509_AAF_BW_CFG_STATIC(3264, 3),
	INV_MPU6509_AAF_BW_CFG_STATIC(3392, 3),
	INV_MPU6509_AAF_BW_CFG_STATIC(3456, 3),
	INV_MPU6509_AAF_BW_CFG_STATIC(3584, 3),
	INV_MPU6509_AAF_BW_CFG_STATIC(3712, 3),
	INV_MPU6509_AAF_BW_CFG_STATIC(3840, 3),
	INV_MPU6509_AAF_BW_CFG_STATIC(3968, 3)
};

static const struct inv_mpu6509_reg_map reg_set_6509 = {
	.dev_config = INV_MPU6509_REG_DEVICE_CONFIG,
	.drv_config = INV_MPU6509_REG_DRIVE_CONFIG,
	.int_config = INV_MPU6509_REG_INT_CONFIG,
	.fifo_config = INV_MPU6509_REG_FIFO_CONFIG,
	.temp_data = INV_MPU6509_REG_TEMP_DATA1,
	.accel_data = INV_MPU6509_REG_ACCEL_DATA_X1,
	.gyro_data = INV_MPU6509_REG_GYRO_DATA_X1,
	.tmst_fsynch = INV_MPU6509_REG_TMST_FSYNCH,
	.int_status = INV_MPU6509_REG_INT_STATUS,
	.fifo_count_h = INV_MPU6509_REG_FIFO_COUNTH,
	.fifo_count_l = INV_MPU6509_REG_FIFO_COUNTL,
	.fifo_data = INV_MPU6509_REG_FIFO_DATA,
	.int_status2 = INV_MPU6509_REG_INT_STATUS2,
	.signal_path_rst = INV_MPU6509_REG_SIGNAL_PATH_RESET,
	.intf_config0 = INV_MPU6509_REG_INTF_CONFIG0,
	.intf_config1 = INV_MPU6509_REG_INTF_CONFIG1,
	.pwr_mgmt0 = INV_MPU6509_REG_PWR_MGMT0,
	.gyro_config0 = INV_MPU6509_REG_GYRO_CONFIG0,
	.accel_config0 = INV_MPU6509_REG_ACCEL_CONFIG0,
	.gyro_config1 = INV_MPU6509_REG_GYRO_CONFIG1,
	.gyro_accel_config0 = INV_MPU6509_REG_GYRO_ACCEL_CONFIG0,
	.accel_config1 = INV_MPU6509_REG_ACCEL_CONFIG1,
	.tmst_config = INV_MPU6509_REG_TMST_CONFIG,
	.wom_config = INV_MPU6509_REG_WOM_CONFIG,
	.fifo_config1 = INV_MPU6509_REG_FIFO_CONFIG1,
	.fifo_config2 = INV_MPU6509_REG_FIFO_CONFIG2,
	.fifo_config3 = INV_MPU6509_REG_FIFO_CONFIG3,
	.fsync_config = INV_MPU6509_REG_FSYNC_CONFIG,
	.int_config0 = INV_MPU6509_REG_INT_CONFIG0,
	.int_config1 = INV_MPU6509_REG_INT_CONFIG1,
	.int_source0 = INV_MPU6509_REG_INT_SOURCE0,
	.int_source1 = INV_MPU6509_REG_INT_SOURCE1,
	.int_source3 = INV_MPU6509_REG_INT_SOURCE3,
	.int_source4 = INV_MPU6509_REG_INT_SOURCE4,
	.fifo_lost_pkt0 = INV_MPU6509_REG_FIFO_LOST_PKT0,
	.fifo_lost_pkt1 = INV_MPU6509_REG_FIFO_LOST_PKT1,
	.self_test_config = INV_MPU6509_REG_SELF_TEST_CONFIG,
	.who_am_i = INV_MPU6509_REG_WHO_AM_I,
	.reg_bank_sel = INV_MPU6509_REG_REG_BANK_SEL,

	.sensor_config0 = INV_MPU6509_REG_SENSOR_CONFIG0,
	.gyro_config_static2 = INV_MPU6509_REG_GYRO_CONFIG_STATIC2,
	.gyro_config_static3 = INV_MPU6509_REG_GYRO_CONFIG_STATIC3,
	.gyro_config_static4 = INV_MPU6509_REG_GYRO_CONFIG_STATIC4,
	.gyro_config_static5 = INV_MPU6509_REG_GYRO_CONFIG_STATIC5,
	.gyro_config_static6 = INV_MPU6509_REG_GYRO_CONFIG_STATIC6,
	.gyro_config_static7 = INV_MPU6509_REG_GYRO_CONFIG_STATIC7,
	.gyro_config_static8 = INV_MPU6509_REG_GYRO_CONFIG_STATIC8,
	.gyro_config_static9 = INV_MPU6509_REG_GYRO_CONFIG_STATIC9,
	.gyro_config_static10 = INV_MPU6509_REG_GYRO_CONFIG_STATIC10,
	.xg_st_data = INV_MPU6509_REG_XG_ST_DATA,
	.yg_st_data = INV_MPU6509_REG_YG_ST_DATA,
	.zg_st_data = INV_MPU6509_REG_ZG_ST_DATA,
	.tmstval0 = INV_MPU6509_REG_TMSTVAL0,
	.tmstval1 = INV_MPU6509_REG_TMSTVAL1,
	.tmstval2 = INV_MPU6509_REG_TMSTVAL2,
	.intf_config4 = INV_MPU6509_REG_INTF_CONFIG4,
	.intf_config5 = INV_MPU6509_REG_INTF_CONFIG5,

	.accel_config_static2 = INV_MPU6509_REG_ACCEL_CONFIG_STATIC2,
	.accel_config_static3 = INV_MPU6509_REG_ACCEL_CONFIG_STATIC3,
	.accel_config_static4 = INV_MPU6509_REG_ACCEL_CONFIG_STATIC4,
	.xa_st_data = INV_MPU6509_REG_XA_ST_DATA,
	.ya_st_data = INV_MPU6509_REG_YA_ST_DATA,
	.za_st_data = INV_MPU6509_REG_ZA_ST_DATA,

	.accel_wom_x_thr = INV_MPU6509_REG_ACCEL_WOM_X_THR,
	.accel_wom_y_thr = INV_MPU6509_REG_ACCEL_WOM_Y_THR,
	.accel_wom_z_thr = INV_MPU6509_REG_ACCEL_WOM_Z_THR,
	.gyro_offset = INV_MPU6509_REG_OFFSET_USER0,
	.accel_offset = INV_MPU6509_REG_OFFSET_USER4,
};

#define INV_MPU6509_VAL_GYRO_AAF_DELT_DEFAULT           6
#define INV_MPU6509_VAL_ACCEL_AAF_DELT_DEFAULT          1

static const struct inv_mpu6509_chip_config chip_config_6509 = {
	.fsr = INV_MPU6509_GYRO_FS_SEL_2000,
	.ui_filt_ord = INV_MPU6509_GYRO_UI_FILT_ORD_1,
	.ui_filt_bw = INV_MPU6509_GYRO_UI_FILT_BW_MAX_400_ODR_DIV_BY_5,
	.accl_aaf_enable = INV_MPU6509_ACCEL_AAF_ENABLE,
	.accl_aaf_delt = INV_MPU6509_VAL_ACCEL_AAF_DELT_DEFAULT,
	.gyro_aaf_enable = INV_MPU6509_GYRO_AAF_ENABLE,
	.gyro_aaf_delt = INV_MPU6509_VAL_GYRO_AAF_DELT_DEFAULT,
	.accl_fs = INV_MPU6509_ACCEL_FS_SEL_4,
	.enable = false,
	.accl_fifo_enable = true,
	.gyro_fifo_enable = true,
	.odr = INV_MPU6509_GYRO_ODR_100,
	.fifo_wm = 1
};

/* Indexed by enum inv_devices */
static const struct inv_mpu6509_hw hw_info[INV_NUM_PARTS] = {
	[INV_MPU6509] = {
		.whoami = INV_MPU6509_VAL_WHO_AM_I,
		.name = "MPU6509",
		.reg = &reg_set_6509,
		.config = &chip_config_6509,
	}
};

static const struct iio_mount_matrix *
inv_get_mount_matrix(const struct iio_dev *indio_dev,
    const struct iio_chan_spec *chan)
{
	return &((struct inv_mpu6509_state *)iio_priv(indio_dev))->orientation;
}

static const struct iio_chan_spec_ext_info inv_ext_info[] = {
	IIO_MOUNT_MATRIX(IIO_SHARED_BY_TYPE, inv_get_mount_matrix),
	{ },
};

#define INV_MPU6509_CHAN(_type, _channel2, _index)                    \
    {                                                             \
        .type = _type,                                        \
        .modified = 1,                                        \
        .channel2 = _channel2,                                \
        .info_mask_shared_by_type = BIT(IIO_CHAN_INFO_SCALE), \
        .info_mask_separate = BIT(IIO_CHAN_INFO_RAW) |        \
                      BIT(IIO_CHAN_INFO_CALIBBIAS),   \
        .scan_index = _index,                                 \
        .scan_type = {                                        \
                .sign = 's',                          \
                .realbits = 16,                       \
                .storagebits = 16,                    \
                .shift = 0,                           \
                .endianness = IIO_BE,                 \
                 },                                       \
        .ext_info = inv_ext_info,                             \
    }

#define INV_MPU6509_CHAN_TEMP(_index)                    \
    {                                                             \
        .type = IIO_TEMP,                                        \
        .channel = -1,                                        \
        .info_mask_separate = BIT(IIO_CHAN_INFO_RAW) |        \
                      BIT(IIO_CHAN_INFO_OFFSET) |   \
                      BIT(IIO_CHAN_INFO_SCALE), \
        .scan_index = _index,                                 \
        .scan_type = {                                        \
                .sign = 's',                          \
                .realbits = 8,                       \
                .storagebits = 8,                    \
                 },                                       \
    }

static const struct iio_chan_spec inv_mpu_channels_soft_timestamp[] = {
	IIO_CHAN_SOFT_TIMESTAMP(INV_MPU6509_SCAN_TIMESTAMP),

	INV_MPU6509_CHAN_TEMP(INV_MPU6509_SCAN_TEMPERATURE),

	INV_MPU6509_CHAN(IIO_ANGL_VEL, IIO_MOD_X, INV_MPU6509_SCAN_GYRO_X),
	INV_MPU6509_CHAN(IIO_ANGL_VEL, IIO_MOD_Y, INV_MPU6509_SCAN_GYRO_Y),
	INV_MPU6509_CHAN(IIO_ANGL_VEL, IIO_MOD_Z, INV_MPU6509_SCAN_GYRO_Z),

	INV_MPU6509_CHAN(IIO_ACCEL, IIO_MOD_X, INV_MPU6509_SCAN_ACCL_X),
	INV_MPU6509_CHAN(IIO_ACCEL, IIO_MOD_Y, INV_MPU6509_SCAN_ACCL_Y),
	INV_MPU6509_CHAN(IIO_ACCEL, IIO_MOD_Z, INV_MPU6509_SCAN_ACCL_Z),
};

#define INV_MPU6509_CHAN_TIMESTAMP(_index)                    \
    {                                                             \
        .type = IIO_TIMESTAMP,                                    \
        .channel = -1,                                        \
        .info_mask_separate = BIT(IIO_CHAN_INFO_RAW) |        \
                      BIT(IIO_CHAN_INFO_SCALE), \
        .scan_index = _index,                                 \
        .scan_type = {                                        \
                .sign = 's',                          \
                .realbits = 16,                       \
                .storagebits = 16,                    \
                .shift = 0,                           \
                .endianness = IIO_BE,                 \
                 },                                       \
    }

static const struct iio_chan_spec inv_mpu_channels_hard_timestamp[] = {
	INV_MPU6509_CHAN_TIMESTAMP(INV_MPU6509_SCAN_TIMESTAMP),

	INV_MPU6509_CHAN_TEMP(INV_MPU6509_SCAN_TEMPERATURE),

	INV_MPU6509_CHAN(IIO_ANGL_VEL, IIO_MOD_X, INV_MPU6509_SCAN_GYRO_X),
	INV_MPU6509_CHAN(IIO_ANGL_VEL, IIO_MOD_Y, INV_MPU6509_SCAN_GYRO_Y),
	INV_MPU6509_CHAN(IIO_ANGL_VEL, IIO_MOD_Z, INV_MPU6509_SCAN_GYRO_Z),

	INV_MPU6509_CHAN(IIO_ACCEL, IIO_MOD_X, INV_MPU6509_SCAN_ACCL_X),
	INV_MPU6509_CHAN(IIO_ACCEL, IIO_MOD_Y, INV_MPU6509_SCAN_ACCL_Y),
	INV_MPU6509_CHAN(IIO_ACCEL, IIO_MOD_Z, INV_MPU6509_SCAN_ACCL_Z),
};

static const unsigned long mpu6509_scan_masks[] = {
	BIT(INV_MPU6509_SCAN_ACCL_X) | BIT(INV_MPU6509_SCAN_ACCL_Y) | BIT(INV_MPU6509_SCAN_ACCL_Z) |
	BIT(INV_MPU6509_SCAN_GYRO_X) | BIT(INV_MPU6509_SCAN_GYRO_Y) | BIT(INV_MPU6509_SCAN_GYRO_Z) |
	BIT(INV_MPU6509_SCAN_TIMESTAMP) | BIT(INV_MPU6509_SCAN_TEMPERATURE),
	0
};

static const char *inv_mpu6509_accel_odr_string[INV_MPU6509_ACCEL_ODR_MAX] = {
	"32000",
	"16000",
	"8000",
	"4000",
	"2000",
	"1000",
	"200",
	"100",
	"50",
	"25",
	"12.5",
	"6.25",
	"3.125",
	"1.5625",
	"resv_14",
	"500"
};

int inv_mpu6509_set_power_itg(struct inv_mpu6509_state *st, bool power_on)
{
	int result = 0;

	if (power_on) {
		/* Already under indio-dev->mlock mutex */
		if (!st->powerup_count) {
			result = regmap_write(st->map, st->reg->pwr_mgmt0,
			        INV_MPU6509_ACCEL_MODE_LN << INV_MPU6509_BIT_OFFSET_ACCEL_MODE
			        | INV_MPU6509_GYRO_MODE_LN << INV_MPU6509_BIT_OFFSET_GYRO_MODE);
			if (!result)
				msleep(INV_MPU6509_VAL_GYRO_MODE_ON_MIN_TIME_MS);
		}
		if (!result)
			st->powerup_count++;
	} else {
		st->powerup_count--;
		if (!st->powerup_count)
			result = regmap_write(st->map, st->reg->pwr_mgmt0,
			        INV_MPU6509_ACCEL_MODE_OFF_0 << INV_MPU6509_BIT_OFFSET_ACCEL_MODE
			        | INV_MPU6509_GYRO_MODE_OFF << INV_MPU6509_BIT_OFFSET_GYRO_MODE);
	}

	if (result)
		return result;

	return 0;
}
EXPORT_SYMBOL_GPL(inv_mpu6509_set_power_itg);

/**
 * inv_fifo_rate_show() - Get the current sampling rate.
 */
static ssize_t
inv_fifo_rate_show(struct device *dev, struct device_attribute *attr,
    char *buf)
{
	struct inv_mpu6509_state *st = iio_priv(dev_to_iio_dev(dev));

	return sprintf(buf, "%s\n", inv_mpu6509_accel_odr_string[st->chip_config.odr]);
}

/**
 * inv_mpu6509_fifo_rate_store() - Set fifo rate.
 */
static ssize_t
inv_mpu6509_fifo_rate_store(struct device *dev, struct device_attribute *attr,
    const char *buf, size_t count)
{
	s32 odr;
	int result;
	struct iio_dev *indio_dev = dev_to_iio_dev(dev);
	struct inv_mpu6509_state *st = iio_priv(indio_dev);

	if (kstrtoint(buf, 10, &odr))
		return -EINVAL;

	if (odr < 0 || odr >= INV_MPU6509_ACCEL_ODR_MAX)
		return -EINVAL;

	if (odr == st->chip_config.odr)
		return count;

	mutex_lock(&indio_dev->mlock);
	if (st->chip_config.enable) {
		result = -EBUSY;
		mutex_unlock(&indio_dev->mlock);
		return result;
	}

	result = inv_mpu6509_set_power_itg(st, true);
	if (result)
		goto fifo_rate_fail;

	if(odr == INV_MPU6509_GYRO_ODR_RESV_12
	    || odr == INV_MPU6509_GYRO_ODR_RESV_13
	    || odr == INV_MPU6509_GYRO_ODR_RESV_14) {
		dev_warn(dev, "odr uses resv value %d for gyro\n", odr);
	}
	result = regmap_write_bits(st->map, st->reg->gyro_config0, INV_MPU6509_VAL_ODR_MASK, odr);
	if (result)
		goto fifo_rate_fail;

	// check accel power mode ???
	result = regmap_write_bits(st->map, st->reg->accel_config0, INV_MPU6509_VAL_ODR_MASK, odr);
	if (result)
		goto fifo_rate_fail;

	st->chip_config.odr = odr;

fifo_rate_fail:
	result |= inv_mpu6509_set_power_itg(st, false);
	mutex_unlock(&indio_dev->mlock);
	if (result)
		return result;

	return count;
}

/**
 * inv_attr_show() - calling this function will show current
 *                    parameters.
 *
 * Deprecated in favor of IIO mounting matrix API.
 *
 * See inv_get_mount_matrix()
 */
static ssize_t inv_attr_show(struct device *dev, struct device_attribute *attr,
    char *buf)
{
	struct inv_mpu6509_state *st = iio_priv(dev_to_iio_dev(dev));
	struct iio_dev_attr *this_attr = to_iio_dev_attr(attr);
	s8 *m;

	switch (this_attr->address) {
	/*
	 * In MPU6509, the two matrix are the same because gyro and accel
	 * are integrated in one chip
	 */
	case ATTR_GYRO_MATRIX:
	case ATTR_ACCL_MATRIX:
		m = st->plat_data.orientation;

		return sprintf(buf, "%d, %d, %d; %d, %d, %d; %d, %d, %d\n",
		        m[0], m[1], m[2], m[3], m[4], m[5], m[6], m[7], m[8]);
	default:
		return -EINVAL;
	}
}

/* constant IIO attribute */
static IIO_CONST_ATTR_SAMP_FREQ_AVAIL("1.5625 3.125 6.25 12.5 25 50 100 200 500 1000 2000 4000 8000 16000 32000");
static IIO_CONST_ATTR(in_anglvel_scale_available,
    "0.000008318 0.000016636 0.000033272 0.000066545 0.000133090 0.000266181 0.000532362 0.001064724");
static IIO_CONST_ATTR(in_accel_scale_available,
    "0.001196 0.002392 0.004785 0.009570");
static IIO_DEV_ATTR_SAMP_FREQ(S_IRUGO | S_IWUSR, inv_fifo_rate_show,
    inv_mpu6509_fifo_rate_store);

/* Deprecated: kept for userspace backward compatibility. */
static IIO_DEVICE_ATTR(in_gyro_matrix, S_IRUGO, inv_attr_show, NULL,
    ATTR_GYRO_MATRIX);
static IIO_DEVICE_ATTR(in_accel_matrix, S_IRUGO, inv_attr_show, NULL,
    ATTR_ACCL_MATRIX);

static struct attribute *inv_attributes[] = {
	&iio_dev_attr_in_gyro_matrix.dev_attr.attr,  /* deprecated */
	&iio_dev_attr_in_accel_matrix.dev_attr.attr, /* deprecated */
	&iio_dev_attr_sampling_frequency.dev_attr.attr,
	&iio_const_attr_sampling_frequency_available.dev_attr.attr,
	&iio_const_attr_in_accel_scale_available.dev_attr.attr,
	&iio_const_attr_in_anglvel_scale_available.dev_attr.attr,
	NULL,
};

static const struct attribute_group inv_attribute_group = {
	.attrs = inv_attributes
};

/*
 * this is the gyro scale translated from dynamic range plus/minus
 * {2000, 1000, 500, 250, 125, 62.5, 31.25, 15.625} to rad/s
 */
static const int gyro_scale_6509[] = {1064724, 532362, 266181, 133090, 66545, 33272, 16636, 8318};

/*
 * this is the accel scale translated from dynamic range plus/minus
 * {32, 16, 8, 4} to m/s^2
 */
static const int accel_scale_6509[] = {9570, 4785, 2392, 1196};

int inv_mpu6509_switch_engine(struct inv_mpu6509_state *st, bool en, u32 mask)
{
	int result;
	int result2;
	unsigned int d;

	result = regmap_write(st->map, st->reg->reg_bank_sel, INV_MPU6509_REG_BANK_1);
	if (result)
		return result;

	if(mask == INV_MPU6509_VAL_GYRO_AXIS_MASK) {
		if(en)
			d = INV_MPU6509_SENSOR_CONFIG_ON << INV_MPU6509_BIT_OFFSET_XG_DISABLE
			    | INV_MPU6509_SENSOR_CONFIG_ON << INV_MPU6509_BIT_OFFSET_YG_DISABLE
			    | INV_MPU6509_SENSOR_CONFIG_ON << INV_MPU6509_BIT_OFFSET_ZG_DISABLE;
		else
			d = INV_MPU6509_SENSOR_CONFIG_OFF << INV_MPU6509_BIT_OFFSET_XG_DISABLE
			    | INV_MPU6509_SENSOR_CONFIG_OFF << INV_MPU6509_BIT_OFFSET_YG_DISABLE
			    | INV_MPU6509_SENSOR_CONFIG_OFF << INV_MPU6509_BIT_OFFSET_ZG_DISABLE;
	} else {
		if(en)
			d = INV_MPU6509_SENSOR_CONFIG_ON << INV_MPU6509_BIT_OFFSET_XA_DISABLE
			    | INV_MPU6509_SENSOR_CONFIG_ON << INV_MPU6509_BIT_OFFSET_YA_DISABLE
			    | INV_MPU6509_SENSOR_CONFIG_ON << INV_MPU6509_BIT_OFFSET_ZA_DISABLE;
		else
			d = INV_MPU6509_SENSOR_CONFIG_OFF << INV_MPU6509_BIT_OFFSET_XA_DISABLE
			    | INV_MPU6509_SENSOR_CONFIG_OFF << INV_MPU6509_BIT_OFFSET_YA_DISABLE
			    | INV_MPU6509_SENSOR_CONFIG_OFF << INV_MPU6509_BIT_OFFSET_ZA_DISABLE;
	}
	result = regmap_write_bits(st->map, st->reg->sensor_config0, mask, d);

	result2 = regmap_write(st->map, st->reg->reg_bank_sel, INV_MPU6509_REG_BANK_0);

	return result || result2;
}

static int inv_mpu6509_sensor_data_show(struct inv_mpu6509_state  *st, int reg,
    int axis, int *val)
{
	int ind, result;
	__be16 d;

	ind = (axis - IIO_MOD_X) * 2;
	result = regmap_bulk_read(st->map, reg + ind, (u8 *)&d, 2);
	if (result)
		return -EINVAL;
	*val = (short)be16_to_cpup(&d);

	return IIO_VAL_INT;
}

static const int usr_offset_off[2][3] = { {0, 1, 3}, {0, 2, 3} };
static const int usr_offset_flag[2][3] = { {0, 1, 0}, {1, 0, 1} };

static int inv_mpu6509_sensor_offset_show(struct inv_mpu6509_state  *st, int reg,
    int axis, int *val)
{
	int ind, result;
	int ret;
	u16 d;

	result = regmap_write(st->map, st->reg->reg_bank_sel, INV_MPU6509_REG_BANK_4);
	if (result)
		return result;

	if(reg == st->reg->gyro_offset) {
		ind = 0;
	} else if(reg == st->reg->accel_offset) {
		ind = 1;
	} else {
		ret = -EINVAL;
		goto SHOW_EXIT;
	}

	result = regmap_bulk_read(st->map, reg + usr_offset_off[ind][axis - IIO_MOD_X], (u8 *)&d, 2);
	if (result) {
		ret = -EINVAL;
		goto SHOW_EXIT;
	}

	if(usr_offset_flag[ind][axis - IIO_MOD_X]) {
		*val = ((d & 0xF0) << 4) | (d >> 8);
	} else {
		*val = d & 0xFFF;
	}

	ret = IIO_VAL_INT;

SHOW_EXIT:
	result = regmap_write(st->map, st->reg->reg_bank_sel, INV_MPU6509_REG_BANK_0);

	return ret;
}

static int
inv_mpu6509_read_raw(struct iio_dev *indio_dev,
    struct iio_chan_spec const *chan,
    int *val, int *val2, long mask)
{
	struct inv_mpu6509_state  *st = iio_priv(indio_dev);
	int ret = 0;

	switch (mask) {
	case IIO_CHAN_INFO_RAW: {
		int result;

		ret = IIO_VAL_INT;
		result = 0;
		mutex_lock(&indio_dev->mlock);
		if (!st->chip_config.enable) {
			result = inv_mpu6509_set_power_itg(st, true);
			if (result)
				goto error_read_raw;
		}
		/* when enable is on, power is already on */
		switch (chan->type) {
		case IIO_ANGL_VEL:
			if (!st->chip_config.gyro_fifo_enable ||
			    !st->chip_config.enable) {
				result = inv_mpu6509_switch_engine(st, true,
				        INV_MPU6509_VAL_GYRO_AXIS_MASK);
				if (result)
					goto error_read_raw;
			}
			ret = inv_mpu6509_sensor_data_show(st, st->reg->gyro_data,
			        chan->channel2, val);
			if (!st->chip_config.gyro_fifo_enable ||
			    !st->chip_config.enable) {
				result = inv_mpu6509_switch_engine(st, false,
				        INV_MPU6509_VAL_GYRO_AXIS_MASK);
				if (result)
					goto error_read_raw;
			}
			break;
		case IIO_ACCEL:
			if (!st->chip_config.accl_fifo_enable ||
			    !st->chip_config.enable) {
				result = inv_mpu6509_switch_engine(st, true,
				        INV_MPU6509_VAL_ACCEL_AXIS_MASK);
				if (result)
					goto error_read_raw;
			}
			ret = inv_mpu6509_sensor_data_show(st, st->reg->accel_data,
			        chan->channel2, val);
			if (!st->chip_config.accl_fifo_enable ||
			    !st->chip_config.enable) {
				result = inv_mpu6509_switch_engine(st, false,
				        INV_MPU6509_VAL_ACCEL_AXIS_MASK);
				if (result)
					goto error_read_raw;
			}
			break;
		case IIO_TEMP:
			/* wait for stablization */
			msleep(INV_MPU6509_SENSOR_UP_TIME);
			ret = inv_mpu6509_sensor_data_show(st, st->reg->temp_data,
			        IIO_MOD_X, val);
			break;
		default:
			ret = -EINVAL;
			break;
		}
error_read_raw:
		if (!st->chip_config.enable)
			result |= inv_mpu6509_set_power_itg(st, false);
		mutex_unlock(&indio_dev->mlock);
		if (result)
			return result;

		return ret;
	}
	case IIO_CHAN_INFO_SCALE:
		switch (chan->type) {
		case IIO_ANGL_VEL:
			*val  = 0;
			*val2 = gyro_scale_6509[st->chip_config.fsr];

			return IIO_VAL_INT_PLUS_NANO;
		case IIO_ACCEL:
			*val = 0;
			*val2 = accel_scale_6509[st->chip_config.accl_fs];

			return IIO_VAL_INT_PLUS_MICRO;
		case IIO_TEMP:
			*val = 0;
			*val2 = INV_MPU6509_TEMP_SCALE;

			return IIO_VAL_INT_PLUS_MICRO;
		default:
			return -EINVAL;
		}
	case IIO_CHAN_INFO_OFFSET:
		switch (chan->type) {
		case IIO_TEMP:
			*val = INV_MPU6509_TEMP_OFFSET;

			return IIO_VAL_INT;
		default:
			return -EINVAL;
		}
	case IIO_CHAN_INFO_CALIBBIAS:
		switch (chan->type) {
		case IIO_ANGL_VEL:
			ret = inv_mpu6509_sensor_offset_show(st, st->reg->gyro_offset,
			        chan->channel2, val);
			return IIO_VAL_INT;
		case IIO_ACCEL:
			ret = inv_mpu6509_sensor_offset_show(st, st->reg->accel_offset,
			        chan->channel2, val);
			return IIO_VAL_INT;

		default:
			return -EINVAL;
		}
	default:
		return -EINVAL;
	}
}

static int inv_mpu6509_write_gyro_scale(struct inv_mpu6509_state *st, int val)
{
	int result, i;
	u8 d;

	for (i = 0; i < ARRAY_SIZE(gyro_scale_6509); ++i) {
		if (gyro_scale_6509[i] == val) {
			d = (i << INV_MPU6509_BIT_OFFSET_GYRO_FS_SEL);
			result = regmap_write(st->map, st->reg->gyro_config0, d);
			if (result)
				return result;

			st->chip_config.fsr = i;
			return 0;
		}
	}

	return -EINVAL;
}

static int inv_mpu6509_write_accel_scale(struct inv_mpu6509_state *st, int val)
{
	int result, i;
	u8 d;

	for (i = 0; i < ARRAY_SIZE(accel_scale_6509); ++i) {
		if (accel_scale_6509[i] == val) {
			d = (i << INV_MPU6509_BIT_OFFSET_ACCEL_FS_SEL);
			result = regmap_write(st->map, st->reg->accel_config0, d);
			if (result)
				return result;

			st->chip_config.accl_fs = i;
			return 0;
		}
	}

	return -EINVAL;
}

static int inv_mpu6509_sensor_offset_set(struct inv_mpu6509_state  *st, int reg,
    int axis, int val)
{
	int ind, result;
	int ret;
	u16 d;

	result = regmap_write(st->map, st->reg->reg_bank_sel, INV_MPU6509_REG_BANK_4);
	if (result)
		return result;

	if(reg == st->reg->gyro_offset) {
		ind = 0;
	} else if(reg == st->reg->accel_offset) {
		ind = 1;
	} else {
		ret = -EINVAL;
		goto SET_EXIT;
	}

	if(usr_offset_flag[ind][axis - IIO_MOD_X]) {
		d = ((val & 0xF0) << 4) | ((val >> 8) & 0xFF);
	} else {
		d = val & 0xFFF;
	}

	result = regmap_bulk_write(st->map, reg + usr_offset_off[ind][axis - IIO_MOD_X], (u8 *)&d, 2);
	if (result) {
		ret = -EINVAL;
		goto SET_EXIT;
	}

	ret = 0;

SET_EXIT:
	result = regmap_write(st->map, st->reg->reg_bank_sel, INV_MPU6509_REG_BANK_0);

	return ret;
}

static int inv_mpu6509_write_raw(struct iio_dev *indio_dev,
    struct iio_chan_spec const *chan,
    int val, int val2, long mask)
{
	struct inv_mpu6509_state  *st = iio_priv(indio_dev);
	int result;

	mutex_lock(&indio_dev->mlock);
	/*
	 * we should only update scale when the chip is disabled, i.e.
	 * not running
	 */
	if (st->chip_config.enable) {
		result = -EBUSY;
		goto error_write_raw;
	}
	result = inv_mpu6509_set_power_itg(st, true);
	if (result)
		goto error_write_raw;

	switch (mask) {
	case IIO_CHAN_INFO_SCALE:
		switch (chan->type) {
		case IIO_ANGL_VEL:
			result = inv_mpu6509_write_gyro_scale(st, val2);
			break;
		case IIO_ACCEL:
			result = inv_mpu6509_write_accel_scale(st, val2);
			break;
		default:
			result = -EINVAL;
			break;
		}
		break;
	case IIO_CHAN_INFO_CALIBBIAS:
		switch (chan->type) {
		case IIO_ANGL_VEL:
			result = inv_mpu6509_sensor_offset_set(st,
			        st->reg->gyro_offset,
			        chan->channel2, val);
			break;
		case IIO_ACCEL:
			result = inv_mpu6509_sensor_offset_set(st,
			        st->reg->accel_offset,
			        chan->channel2, val);
			break;
		default:
			result = -EINVAL;
		}
	default:
		result = -EINVAL;
		break;
	}

error_write_raw:
	result |= inv_mpu6509_set_power_itg(st, false);
	mutex_unlock(&indio_dev->mlock);

	return result;
}

static int inv_write_raw_get_fmt(struct iio_dev *indio_dev,
    struct iio_chan_spec const *chan, long mask)
{
	switch (mask) {
	case IIO_CHAN_INFO_SCALE:
		switch (chan->type) {
		case IIO_ANGL_VEL:
			return IIO_VAL_INT_PLUS_NANO;
		default:
			return IIO_VAL_INT_PLUS_MICRO;
		}
	default:
		return IIO_VAL_INT_PLUS_MICRO;
	}

	return -EINVAL;
}

/**
 * inv_mpu6509_validate_trigger() - validate_trigger callback for invensense
 *                                  MPU6509 device.
 * @indio_dev: The IIO device
 * @trig: The new trigger
 *
 * Returns: 0 if the 'trig' matches the trigger registered by the MPU6509
 * device, -EINVAL otherwise.
 */
static int inv_mpu6509_validate_trigger(struct iio_dev *indio_dev,
    struct iio_trigger *trig)
{
	struct inv_mpu6509_state *st = iio_priv(indio_dev);

	if (st->trig != trig)
		return -EINVAL;

	return 0;
}

static const struct iio_info mpu_info = {
	.driver_module = THIS_MODULE,
	.read_raw = &inv_mpu6509_read_raw,
	.write_raw = &inv_mpu6509_write_raw,
	.write_raw_get_fmt = &inv_write_raw_get_fmt,
	.attrs = &inv_attribute_group,
	.validate_trigger = inv_mpu6509_validate_trigger,
};

int inv_mpu6509_init_config(struct iio_dev *indio_dev)
{
	int result;
	u8 d;
	struct inv_mpu6509_state *st = iio_priv(indio_dev);

	result = regmap_write(st->map, st->reg->reg_bank_sel, INV_MPU6509_REG_BANK_0);
	if (result)
		return result;

	result = inv_mpu6509_set_power_itg(st, true);
	if (result)
		return result;

	d = INV_MPU6509_ACCEL_MODE_LN << INV_MPU6509_BIT_OFFSET_ACCEL_MODE
	    | INV_MPU6509_GYRO_MODE_LN << INV_MPU6509_BIT_OFFSET_GYRO_MODE;
	result = regmap_write(st->map, st->reg->pwr_mgmt0, d);
	if (result)
		return result;

	d = (st->chip_config.odr) << INV_MPU6509_BIT_OFFSET_GYRO_ODR
	    | (st->chip_config.fsr) << INV_MPU6509_BIT_OFFSET_GYRO_FS_SEL;
	result = regmap_write(st->map, st->reg->gyro_config0, d);
	if (result)
		return result;

	d = (st->chip_config.ui_filt_bw) << INV_MPU6509_BIT_OFFSET_GYRO_UI_FILT_BW
	    | (st->chip_config.ui_filt_bw) << INV_MPU6509_BIT_OFFSET_ACCEL_UI_FILT_BW;
	result = regmap_write(st->map, st->reg->gyro_accel_config0, d);
	if (result)
		return result;

	d = (st->chip_config.odr) << INV_MPU6509_BIT_OFFSET_ACCEL_ODR
	    | (st->chip_config.accl_fs) << INV_MPU6509_BIT_OFFSET_ACCEL_FS_SEL;
	result = regmap_write(st->map, st->reg->accel_config0, d);
	if (result)
		return result;

	d = INV_MPU6509_INT_ACTIVE_HIGH << INV_MPU6509_BIT_OFFSET_INT1_POLARITY
	    | INV_MPU6509_INT_MODE_LATCHED << INV_MPU6509_BIT_OFFSET_INT1_MODE
	    | INV_MPU6509_INT_DRIVE_CIRCUIT_PUSH_PULL << INV_MPU6509_BIT_OFFSET_INT1_DRIVE_CIRCUIT;
	result = regmap_write(st->map, st->reg->int_config, d);
	if (result)
		return result;

	d = INV_MPU6509_INT_ASYNC_RESET_0 << INV_MPU6509_BIT_OFFSET_INT_ASYNC_RESET;
	result = regmap_write(st->map, st->reg->int_config1, d);
	if (result)
		return result;

	d = INV_MPU6509_VAL_EN_TEST_MODE_NORAML_OP << INV_MPU6509_BIT_OFFSET_EN_TEST_MODE
	    | INV_MPU6509_CLKSEL_PLL_THEN_INTERNAL_RC_OSC << INV_MPU6509_BIT_OFFSET_CLKSEL;
	result = regmap_write(st->map, st->reg->intf_config1, d);
	if (result)
		return result;

	/*
	    d = INV_MPU6509_TEMP_FILT_BW_170 << INV_MPU6509_BIT_OFFSET_GYRO_TEMP_FILT_BW
	        | INV_MPU6509_GYRO_DEC2_M2_ORD_3 << INV_MPU6509_BIT_OFFSET_GYRO_DEC2_M2_ORD
	        | INV_MPU6509_GYRO_UI_FILT_ORD_1 << INV_MPU6509_BIT_OFFSET_GYRO_UI_FILT_ORD;
	    result = regmap_write(st->map, st->reg->gyro_config1, d);
	    if (result)
	        return result;

	    d = INV_MPU6509_ACCEL_DEC2_M2_ORD_3 << INV_MPU6509_BIT_OFFSET_ACCEL_DEC2_M2_ORD
	        | INV_MPU6509_ACCEL_UI_FILT_ORD_1 << INV_MPU6509_BIT_OFFSET_ACCEL_UI_FILT_ORD;
	    result = regmap_write(st->map, st->reg->accel_config1, d);
	    if (result)
	        return result;
	*/
	result = regmap_write(st->map, st->reg->reg_bank_sel, INV_MPU6509_REG_BANK_1);
	if (result)
		return result;

	d = (st->chip_config.gyro_aaf_enable) << INV_MPU6509_BIT_OFFSET_GYRO_AAF_DIS
	    | INV_MPU6509_GYRO_NF_DISABLE << INV_MPU6509_BIT_OFFSET_GYRO_NF_DIS;
	//      | INV_MPU6509_GYRO_NF_ENABLE << INV_MPU6509_BIT_OFFSET_GYRO_NF_DIS;
	result = regmap_write(st->map, st->reg->gyro_config_static2, d);
	if (result)
		return result;

	result = regmap_write(st->map, st->reg->gyro_config_static3,
	        (st->chip_config.gyro_aaf_delt));
	if (result)
		return result;

	result = regmap_write(st->map, st->reg->gyro_config_static4,
	        INV_MPU6509_GYRO_CONFIG_STATIC4(st->chip_config.gyro_aaf_delt));
	if (result)
		return result;

	result = regmap_write(st->map, st->reg->gyro_config_static5,
	        INV_MPU6509_GYRO_CONFIG_STATIC5(st->chip_config.gyro_aaf_delt));
	if (result)
		return result;

	/*
	    d = 0xAE;
	    result = regmap_write(st->map, st->reg->gyro_config_static6, d);
	    if (result)
	        return result;

	    result = regmap_write(st->map, st->reg->gyro_config_static7, d);
	    if (result)
	        return result;

	    result = regmap_write(st->map, st->reg->gyro_config_static8, d);
	    if (result)
	        return result;

	    d = 0x3F;
	    result = regmap_write(st->map, st->reg->gyro_config_static9, d);
	    if (result)
	        return result;

	    d = 1;
	    result = regmap_write(st->map, st->reg->gyro_config_static10, d);
	    if (result)
	        return result;
	*/

	d = INV_MPU6509_PIN19_FUNC_INT2 << INV_MPU6509_BIT_OFFSET_PIN19_FUNC;
	result = regmap_write(st->map, st->reg->intf_config5, d);
	if (result)
		return result;

	result = regmap_write(st->map, st->reg->reg_bank_sel, INV_MPU6509_REG_BANK_2);
	if (result)
		return result;

	d = (st->chip_config.accl_aaf_enable) << INV_MPU6509_BIT_OFFSET_GYRO_AAF_DIS
	    | (st->chip_config.accl_aaf_delt) << INV_MPU6509_BIT_OFFSET_ACCEL_AAF_DELT;
	result = regmap_write(st->map, st->reg->accel_config_static2, d);
	if (result)
		return result;

	result = regmap_write(st->map, st->reg->accel_config_static3,
	        INV_MPU6509_ACCEL_CONFIG_STATIC3(st->chip_config.accl_aaf_delt));
	if (result)
		return result;

	result = regmap_write(st->map, st->reg->accel_config_static4,
	        INV_MPU6509_ACCEL_CONFIG_STATIC4(st->chip_config.accl_aaf_delt));
	if (result)
		return result;

	result = regmap_write(st->map, st->reg->reg_bank_sel, INV_MPU6509_REG_BANK_0);
	if (result)
		return result;

	result = inv_mpu6509_set_power_itg(st, false);

	return result;
}

/**
 *  inv_check_and_setup_chip() - check and setup chip.
 */
int inv_check_and_setup_chip(struct inv_mpu6509_state *st)
{
	int result;
	unsigned int regval;

	st->hw  = &hw_info[st->chip_type];
	st->reg = hw_info[st->chip_type].reg;

	result = regmap_write(st->map, st->reg->reg_bank_sel, INV_MPU6509_REG_BANK_0);
	if (result)
		return result;

	/* reset to make sure previous state are not there */
	result = regmap_write_bits(st->map, st->reg->dev_config,
	        INV_MPU6509_VAL_SOFT_RESET_MASK,
	        INV_MPU6509_SOFT_RESET_RESET);
	if (result)
		return result;
	msleep(INV_MPU6509_VAL_SOFT_RESET_WAIT_MS);

	/* check chip self-identification */
	result = regmap_read(st->map, st->reg->who_am_i, &regval);
	if (result)
		return result;
	if (regval != st->hw->whoami) {
		dev_warn(regmap_get_device(st->map),
		    "whoami mismatch got %#02x expected %#02hhx for %s\n",
		    regval, st->hw->whoami, st->hw->name);
	}

	/*
	 * toggle power state. After reset, the sleep bit could be on
	 * or off depending on the OTP settings. Toggling power would
	 * make it in a definite state as well as making the hardware
	 * state align with the software state
	 */
	result = inv_mpu6509_set_power_itg(st, false);
	if (result)
		return result;
	result = inv_mpu6509_set_power_itg(st, true);
	if (result)
		return result;

	result = inv_mpu6509_switch_engine(st, false,
	        INV_MPU6509_VAL_GYRO_AXIS_MASK);
	if (result)
		return result;
	result = inv_mpu6509_switch_engine(st, false,
	        INV_MPU6509_VAL_ACCEL_AXIS_MASK);
	if (result)
		return result;

	return 0;
}

#ifdef CONFIG_INV_MPU6509_TRIGGER
int inv_mpu6509_init(struct iio_dev *indio_dev, int (*inv_mpu_bus_setup)(struct iio_dev *))
{
	struct inv_mpu6509_state *st = iio_priv(indio_dev);
	int result;

	/* power is turned on inside check chip type*/
	result = inv_check_and_setup_chip(st);
	if (result)
		return result;

	if (inv_mpu_bus_setup)
		inv_mpu_bus_setup(indio_dev);

	memcpy(&st->chip_config, hw_info[st->chip_type].config,
	    sizeof(struct inv_mpu6509_chip_config));

	result = inv_mpu6509_init_config(indio_dev);
	if (result) {
		pr_debug("Could not initialize device.\n");
	}
	return result;
}
#else
int inv_mpu6509_init(struct iio_dev *indio_dev, int (*inv_mpu_bus_setup)(struct iio_dev *))
{
	struct inv_mpu6509_state *st = iio_priv(indio_dev);

	st->hw  = &hw_info[st->chip_type];
	st->reg = hw_info[st->chip_type].reg;
	memcpy(&st->chip_config, hw_info[st->chip_type].config,
	    sizeof(struct inv_mpu6509_chip_config));

	return 0;
}
#endif

int inv_mpu_core_probe(struct regmap *regmap, int irq, const char *name,
    int (*inv_mpu_bus_setup)(struct iio_dev *), int chip_type)
{
	struct inv_mpu6509_state *st;
	struct iio_dev *indio_dev;
	struct inv_mpu6050_platform_data *pdata;
	struct device *dev = regmap_get_device(regmap);
	int result;

	indio_dev = devm_iio_device_alloc(dev, sizeof(*st));
	if (!indio_dev)
		return -ENOMEM;
	dev_set_name(&indio_dev->dev, INV_MPU_DEV_NAME);
	BUILD_BUG_ON(ARRAY_SIZE(hw_info) != INV_NUM_PARTS);
	if (chip_type < 0 || chip_type >= INV_NUM_PARTS) {
		dev_err(dev, "Bad invensense chip_type=%d name=%s\n",
		    chip_type, name);
		return -ENODEV;
	}
	st = iio_priv(indio_dev);
	st->chip_type = chip_type;
	st->powerup_count = 0;
	st->irq = irq;
	st->map = regmap;

	pdata = dev_get_platdata(dev);
	if (!pdata) {
		result = of_iio_read_mount_matrix(dev, "mount-matrix",
		        &st->orientation);
		if (result) {
			dev_err(dev, "Failed to retrieve mounting matrix %d\n",
			    result);
			return result;
		}
	} else {
		st->plat_data = *pdata;
	}

	result = inv_mpu6509_init(indio_dev, inv_mpu_bus_setup);
	if(result)
		return result;

	dev_set_drvdata(dev, indio_dev);
	indio_dev->dev.parent = dev;
	/* name will be NULL when enumerated via ACPI */
	if (name) {
		indio_dev->name = name;
	} else {
		indio_dev->name = dev_name(dev);
	}

	indio_dev->channels = inv_mpu_channels_hard_timestamp;
	indio_dev->num_channels = ARRAY_SIZE(inv_mpu_channels_hard_timestamp);
	indio_dev->available_scan_masks = mpu6509_scan_masks;

	indio_dev->info = &mpu_info;
	indio_dev->modes = INDIO_BUFFER_TRIGGERED;
	result = inv_mpu6509_setup_trigger(indio_dev);
	if(result)
		return result;

	spin_lock_init(&st->time_stamp_lock);
	result = iio_device_register(indio_dev);
	if (result) {
		dev_err(dev, "IIO register fail %d\n", result);
		inv_mpu6509_cleanup_trigger(indio_dev);
		return result;
	}
	inv_mpu6509_override_ioctl(indio_dev, inv_mpu_bus_setup);

	return 0;
}
EXPORT_SYMBOL_GPL(inv_mpu_core_probe);

int inv_mpu_core_remove(struct device  *dev)
{
	struct iio_dev *indio_dev = dev_get_drvdata(dev);

	iio_device_unregister(indio_dev);
	inv_mpu6509_cleanup_trigger(indio_dev);

	return 0;
}
EXPORT_SYMBOL_GPL(inv_mpu_core_remove);

#ifdef CONFIG_PM_SLEEP
static int inv_mpu_resume(struct device *dev)
{
	return inv_mpu6509_set_power_itg(iio_priv(dev_get_drvdata(dev)), true);
}

static int inv_mpu_suspend(struct device *dev)
{
	return inv_mpu6509_set_power_itg(iio_priv(dev_get_drvdata(dev)), false);
}
#endif /* CONFIG_PM_SLEEP */

SIMPLE_DEV_PM_OPS(inv_mpu_pmops, inv_mpu_suspend, inv_mpu_resume);
EXPORT_SYMBOL_GPL(inv_mpu_pmops);

MODULE_AUTHOR("SmartChip Corporation");
MODULE_DESCRIPTION("SmartChip device MPU6509 driver");
MODULE_LICENSE("GPL");
