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
#include <linux/i2c.h>
#include <linux/i2c-mux.h>
#include <linux/kfifo.h>
#include <linux/spinlock.h>
#include <linux/iio/iio.h>
#include <linux/iio/buffer.h>
#include <linux/regmap.h>
#include <linux/iio/sysfs.h>
#include <linux/iio/kfifo_buf.h>
#include <linux/iio/trigger.h>
#include <linux/iio/triggered_buffer.h>
#include <linux/iio/trigger_consumer.h>
#include <linux/platform_data/invensense_mpu6050.h>

/**
 *  struct inv_mpu6509_reg_map - Notable registers.
 *  ---- bank0-----
 *  @dev_config:            device config
 *  @drv_config:            drive config
 *  @int_config:            interrupt config
 *  @fifo_config:           fifo config
 *  @temp_data:             Address of first temperature data register
 *  @accel_data:            Address of first accelerometer data register
 *  @gyro_data:             Address of first gyroscope data register
 *  @tmst_fsynch:           Upper byte of tmst fysnc register
 *  @int_status:            interrupt status
 *  @fifo_count_h:          Upper byte of FIFO count
 *  @fifo_count_l:          Lower byte of FIFO count
 *  @fifo_data:             FIFO register.
 *  @int_status2:           interrupt status2
 *  @signal_path_rst:       some reset function
 *  @intf_config0:          interface config0
 *  @intf_config1:          interface config1
 *  @pwr_mgmt0:             Controls chip's power state and clock source
 *  @gyro_config0:          gyro config0
 *  @accel_config0:         accel config0
 *  @gyro_config1:          gyro config1
 *  @gyro_accel_config0:    gyro accel config0
 *  @accel_config1:         accel config1
 *  @tmst_config:           tmst config
 *  @wom_config:            wom config
 *  @fifo_config1:          fifo config1
 *  @fifo_config2:          fifo config2
 *  @fifo_config3:          fifo config3
 *  @fsync_config:          fsync config
 *  @int_config0:           interrupt config0
 *  @int_config1:           interrupt config1
 *  @int_source0:           interrupt source0
 *  @int_source1:           interrupt source1
 *  @int_source3:           interrupt source3
 *  @int_source4:           interrupt source4
 *  @fifo_lost_pkt0:        Upper byte of FIFO lost packet
 *  @fifo_lost_pkt1:        Lower byte of FIFO lost packet
 *  @self_test_config:      self test config
 *  @who_am_i:              who am i
 *  @reg_bank_sel:          reg bank select
 *  ---- bank1-----
 *  @sensor_config0:        sensor config
 *  @gyro_config_static2:   gyro config static2
 *  @gyro_config_static3:   gyro config static3
 *  @gyro_config_static4:   gyro config static4
 *  @gyro_config_static5:   gyro config static5
 *  @gyro_config_static6:   gyro config static6
 *  @gyro_config_static7:   gyro config static7
 *  @gyro_config_static8:   gyro config static8
 *  @gyro_config_static9:   gyro config static9
 *  @gyro_config_static10:  gyro config static10
 *  @xg_st_data:            x-gyro self test data
 *  @yg_st_data:            y-gyro self test data
 *  @zg_st_data:            z-gyro self test data
 *  @tmstval0:              1st byte of tmst_value
 *  @tmstval1:              2nd byte of tmst_value
 *  @tmstval2:              3rd byte of tmst_value
 *  @intf_config4:          interface config4
 *  @intf_config5:          interface config5
 *  ---- bank2-----
 *  @accel_config_static2:  accel config static2
 *  @accel_config_static3:  accel config static3
 *  @accel_config_static4:  accel config static4
 *  @xa_st_data:            x-accel self test data
 *  @ya_st_data:            y-accel self test data
 *  @za_st_data:            z-accel self test data
 *  ---- bank4-----
 *  @accel_wom_x_thr:       accel wom x threshold
 *  @accel_wom_y_thr:       accel wom y threshold
 *  @accel_wom_z_thr:       accel wom z threshold
 *  @gyro_offset:           offset user0
 *  @accel_offset:          offset user4
 */
struct inv_mpu6509_reg_map {
	u8 dev_config;
	u8 drv_config;
	u8 int_config;
	u8 fifo_config;
	u8 temp_data;
	u8 accel_data;
	u8 gyro_data;
	u8 tmst_fsynch;
	u8 int_status;
	u8 fifo_count_h;
	u8 fifo_count_l;
	u8 fifo_data;
	u8 int_status2;
	u8 signal_path_rst;
	u8 intf_config0;
	u8 intf_config1;
	u8 pwr_mgmt0;
	u8 gyro_config0;
	u8 accel_config0;
	u8 gyro_config1;
	u8 gyro_accel_config0;
	u8 accel_config1;
	u8 tmst_config;
	u8 wom_config;
	u8 fifo_config1;
	u8 fifo_config2;
	u8 fifo_config3;
	u8 fsync_config;
	u8 int_config0;
	u8 int_config1;
	u8 int_source0;
	u8 int_source1;
	u8 int_source3;
	u8 int_source4;
	u8 fifo_lost_pkt0;
	u8 fifo_lost_pkt1;
	u8 self_test_config;
	u8 who_am_i;
	u8 reg_bank_sel;

	u8 sensor_config0;
	u8 gyro_config_static2;
	u8 gyro_config_static3;
	u8 gyro_config_static4;
	u8 gyro_config_static5;
	u8 gyro_config_static6;
	u8 gyro_config_static7;
	u8 gyro_config_static8;
	u8 gyro_config_static9;
	u8 gyro_config_static10;
	u8 xg_st_data;
	u8 yg_st_data;
	u8 zg_st_data;
	u8 tmstval0;
	u8 tmstval1;
	u8 tmstval2;
	u8 intf_config4;
	u8 intf_config5;

	u8 accel_config_static2;
	u8 accel_config_static3;
	u8 accel_config_static4;
	u8 xa_st_data;
	u8 ya_st_data;
	u8 za_st_data;

	u8 accel_wom_x_thr;
	u8 accel_wom_y_thr;
	u8 accel_wom_z_thr;
	u8 gyro_offset;
	u8 accel_offset;
};

/* register definition */
#define INV_MPU6509_REG_DEVICE_CONFIG                   0x11
#define INV_MPU6509_REG_DRIVE_CONFIG                    0x13
#define INV_MPU6509_REG_INT_CONFIG                      0x14
#define INV_MPU6509_REG_FIFO_CONFIG                     0x16
#define INV_MPU6509_REG_TEMP_DATA1                      0x1D
#define INV_MPU6509_REG_TEMP_DATA0                      0x1E
#define INV_MPU6509_REG_ACCEL_DATA_X1                   0x1F
#define INV_MPU6509_REG_ACCEL_DATA_X0                   0x20
#define INV_MPU6509_REG_ACCEL_DATA_Y1                   0x21
#define INV_MPU6509_REG_ACCEL_DATA_Y0                   0x22
#define INV_MPU6509_REG_ACCEL_DATA_Z1                   0x23
#define INV_MPU6509_REG_ACCEL_DATA_Z0                   0x24
#define INV_MPU6509_REG_GYRO_DATA_X1                    0x25
#define INV_MPU6509_REG_GYRO_DATA_X0                    0x26
#define INV_MPU6509_REG_GYRO_DATA_Y1                    0x27
#define INV_MPU6509_REG_GYRO_DATA_Y0                    0x28
#define INV_MPU6509_REG_GYRO_DATA_Z1                    0x29
#define INV_MPU6509_REG_GYRO_DATA_Z0                    0x2A
#define INV_MPU6509_REG_TMST_FSYNCH                     0x2B
#define INV_MPU6509_REG_TMST_FSYNCL                     0x2C
#define INV_MPU6509_REG_INT_STATUS                      0x2D
#define INV_MPU6509_REG_FIFO_COUNTH                     0x2E
#define INV_MPU6509_REG_FIFO_COUNTL                     0x2F
#define INV_MPU6509_REG_FIFO_DATA                       0x30
#define INV_MPU6509_REG_INT_STATUS2                     0x37
#define INV_MPU6509_REG_SIGNAL_PATH_RESET               0x4B
#define INV_MPU6509_REG_INTF_CONFIG0                    0x4C
#define INV_MPU6509_REG_INTF_CONFIG1                    0x4D
#define INV_MPU6509_REG_PWR_MGMT0                       0x4E
#define INV_MPU6509_REG_GYRO_CONFIG0                    0x4F
#define INV_MPU6509_REG_ACCEL_CONFIG0                   0x50
#define INV_MPU6509_REG_GYRO_CONFIG1                    0x51
#define INV_MPU6509_REG_GYRO_ACCEL_CONFIG0              0x52
#define INV_MPU6509_REG_ACCEL_CONFIG1                   0x53
#define INV_MPU6509_REG_TMST_CONFIG                     0x54
#define INV_MPU6509_REG_WOM_CONFIG                      0x57
#define INV_MPU6509_REG_FIFO_CONFIG1                    0x5F
#define INV_MPU6509_REG_FIFO_CONFIG2                    0x60
#define INV_MPU6509_REG_FIFO_CONFIG3                    0x61
#define INV_MPU6509_REG_FSYNC_CONFIG                    0x62
#define INV_MPU6509_REG_INT_CONFIG0                     0x63
#define INV_MPU6509_REG_INT_CONFIG1                     0x64
#define INV_MPU6509_REG_INT_SOURCE0                     0x65
#define INV_MPU6509_REG_INT_SOURCE1                     0x66
#define INV_MPU6509_REG_INT_SOURCE3                     0x68
#define INV_MPU6509_REG_INT_SOURCE4                     0x69
#define INV_MPU6509_REG_FIFO_LOST_PKT0                  0x6C
#define INV_MPU6509_REG_FIFO_LOST_PKT1                  0x6D
#define INV_MPU6509_REG_SELF_TEST_CONFIG                0x70
#define INV_MPU6509_REG_WHO_AM_I                        0x75
#define INV_MPU6509_REG_REG_BANK_SEL                    0x76

#define INV_MPU6509_REG_SENSOR_CONFIG0                  0x03
#define INV_MPU6509_REG_GYRO_CONFIG_STATIC2             0x0B
#define INV_MPU6509_REG_GYRO_CONFIG_STATIC3             0x0C
#define INV_MPU6509_REG_GYRO_CONFIG_STATIC4             0x0D
#define INV_MPU6509_REG_GYRO_CONFIG_STATIC5             0x0E
#define INV_MPU6509_REG_GYRO_CONFIG_STATIC6             0x0F
#define INV_MPU6509_REG_GYRO_CONFIG_STATIC7             0x10
#define INV_MPU6509_REG_GYRO_CONFIG_STATIC8             0x11
#define INV_MPU6509_REG_GYRO_CONFIG_STATIC9             0x12
#define INV_MPU6509_REG_GYRO_CONFIG_STATIC10            0x13
#define INV_MPU6509_REG_XG_ST_DATA                      0x5F
#define INV_MPU6509_REG_YG_ST_DATA                      0x60
#define INV_MPU6509_REG_ZG_ST_DATA                      0x61
#define INV_MPU6509_REG_TMSTVAL0                        0x62
#define INV_MPU6509_REG_TMSTVAL1                        0x63
#define INV_MPU6509_REG_TMSTVAL2                        0x64
#define INV_MPU6509_REG_INTF_CONFIG4                    0x7A
#define INV_MPU6509_REG_INTF_CONFIG5                    0x7B

#define INV_MPU6509_REG_ACCEL_CONFIG_STATIC2            0x03
#define INV_MPU6509_REG_ACCEL_CONFIG_STATIC3            0x04
#define INV_MPU6509_REG_ACCEL_CONFIG_STATIC4            0x05
#define INV_MPU6509_REG_XA_ST_DATA                      0x3B
#define INV_MPU6509_REG_YA_ST_DATA                      0x3C
#define INV_MPU6509_REG_ZA_ST_DATA                      0x3D

#define INV_MPU6509_REG_ACCEL_WOM_X_THR                 0x4A
#define INV_MPU6509_REG_ACCEL_WOM_Y_THR                 0x4B
#define INV_MPU6509_REG_ACCEL_WOM_Z_THR                 0x4C
#define INV_MPU6509_REG_OFFSET_USER0                    0x77
#define INV_MPU6509_REG_OFFSET_USER1                    0x78
#define INV_MPU6509_REG_OFFSET_USER2                    0x79
#define INV_MPU6509_REG_OFFSET_USER3                    0x7A
#define INV_MPU6509_REG_OFFSET_USER4                    0x7B
#define INV_MPU6509_REG_OFFSET_USER5                    0x7C
#define INV_MPU6509_REG_OFFSET_USER6                    0x7D
#define INV_MPU6509_REG_OFFSET_USER7                    0x7E
#define INV_MPU6509_REG_OFFSET_USER8                    0x7F

/* register bit offset & val definition */
// device_config
enum inv_mpu6509_reset_e {
	INV_MPU6509_SOFT_RESET_NORMAL,
	INV_MPU6509_SOFT_RESET_RESET
};

#define INV_MPU6509_BIT_OFFSET_SOFT_RESET_CONFIG        0
#define INV_MPU6509_VAL_SOFT_RESET_WAIT_MS              1       // after soft reset, wait 1ms before access regs

#define INV_MPU6509_VAL_SOFT_RESET_MASK                 0x01

enum inv_mpu6509_spi_mode_e {
	INV_MPU6509_SPI_MODE_0_3,
	INV_MPU6509_SPI_MODE_1_2
};

#define INV_MPU6509_BIT_OFFSET_SPI_MODE                 4

// drive config
enum inv_mpu6509_slew_rate_e {
	INV_MPU6509_SLEW_RATE_20_60_NS,
	INV_MPU6509_SLEW_RATE_12_36_NS,
	INV_MPU6509_SLEW_RATE_6_18_NS,
	INV_MPU6509_SLEW_RATE_4_12_NS,
	INV_MPU6509_SLEW_RATE_2_6_NS,
	INV_MPU6509_SLEW_RATE_LESS_THAN_2_NS,
	INV_MPU6509_SLEW_RATE_RESV_6,
	INV_MPU6509_SLEW_RATE_RESV_7
};

#define INV_MPU6509_BIT_OFFSET_SPI_SLEW_RATE            0   // SPI driver default: INV_MPU6509_SLEW_RATE_LESS_THAN_2_NS
// I2C driver default: INV_MPU6509_SLEW_RATE_12_36_NS

#define INV_MPU6509_BIT_OFFSET_I2C_SLEW_RATE            3   // SPI driver default: INV_MPU6509_SLEW_RATE_20_60_NS
// I2C driver default: INV_MPU6509_SLEW_RATE_12_36_NS

// int config
enum inv_mpu6509_int_polarity_e {
	INV_MPU6509_INT_ACTIVE_LOW,                             // default
	INV_MPU6509_INT_ACTIVE_HIGH
};

#define INV_MPU6509_BIT_OFFSET_INT1_POLARITY            0

enum inv_mpu6509_int_drive_circuit_e {
	INV_MPU6509_INT_DRIVE_CIRCUIT_OPEN_DRAIN,
	INV_MPU6509_INT_DRIVE_CIRCUIT_PUSH_PULL
};
#define INV_MPU6509_BIT_OFFSET_INT1_DRIVE_CIRCUIT       1

enum inv_mpu6509_int_mode_e {
	INV_MPU6509_INT_MODE_PULSED,
	INV_MPU6509_INT_MODE_LATCHED
};
#define INV_MPU6509_BIT_OFFSET_INT1_MODE                2

#define INV_MPU6509_BIT_OFFSET_INT2_POLARITY            3
#define INV_MPU6509_BIT_OFFSET_INT2_DRIVE_CIRCUIT       4
#define INV_MPU6509_BIT_OFFSET_INT2_MODE                5

// int_config0
enum inv_mpu_6509_latched_mode_int_clear_e {
	INV_MPU_6509_INT_CLEAR_ON_STATUS_BIT_READ_0,
	INV_MPU_6509_INT_CLEAR_ON_STATUS_BIT_READ_1,
	INV_MPU_6509_INT_CLEAR_ON_FIFO_DATA_1BYTE_READ,
	INV_MPU_6509_INT_CLEAR_ON_STATUS_BIT_READ_AND_FIFO_DATA_1BYTE_READ
};

#define INV_MPU6509_BIT_OFFSET_FIFO_FULL_INT_CLEAR      0
#define INV_MPU6509_BIT_OFFSET_FIFO_THS_INT_CLEAR       2
#define INV_MPU6509_BIT_OFFSET_UI_DRDY_INT_CLEAR        4

// int_config1
enum inv_mpu6509_int_async_reset_e {
	INV_MPU6509_INT_ASYNC_RESET_0,
	INV_MPU6509_INT_ASYNC_RESET_1
};

#define INV_MPU6509_BIT_OFFSET_INT_ASYNC_RESET          4

enum inv_mpu6509_int_deassert_duration_e {
	INV_MPU6509_INT_DEASSERT_DURATION_100US,                // default. min 100us, used only if odr < 4khz
	INV_MPU6509_INT_DEASSERT_DURATION_DISABLE               // required if odr >= 4khz, option for odr < 4khz
};
#define INV_MPU6509_BIT_OFFSET_INT_TDEASSERT_DISABLE    5

enum inv_mpu6509_int_pulse_duration_e {
	INV_MPU6509_INT_PULSE_DURATION_100US,               // default. used only if odr < 4khz
	INV_MPU6509_INT_PULSE_DURATION_8US                  // required if odr >= 4khz, option for odr < 4khz
};

#define INV_MPU6509_BIT_OFFSET_INT_TPULSE_DURATION      6

// int_source0
enum inv_mpu6509_int_route_e {
	INV_MPU6509_INT_ROUTE_INT1_2_DISABLE,
	INV_MPU6509_INT_ROUTE_INT1_2_ENABLE
};

#define INV_MPU6509_BIT_OFFSET_UI_AGC_RDY_INT1_EN       0
#define INV_MPU6509_BIT_OFFSET_FIFO_FULL_INT1_EN        1
#define INV_MPU6509_BIT_OFFSET_FIFO_THS_INT1_EN         2
#define INV_MPU6509_BIT_OFFSET_UI_DRDY_INT1_EN          3
#define INV_MPU6509_BIT_OFFSET_RESET_DONE_INT1_EN       4
#define INV_MPU6509_BIT_OFFSET_PLL_RDY_INT1_EN          5
#define INV_MPU6509_BIT_OFFSET_UI_FSYNC_INT1_EN         6

// int_source1
#define INV_MPU6509_BIT_OFFSET_WOM_X_INT1               0
#define INV_MPU6509_BIT_OFFSET_WOM_Y_INT1               1
#define INV_MPU6509_BIT_OFFSET_WOM_Z_INT1               2

// int_source3
#define INV_MPU6509_BIT_OFFSET_UI_AGC_RDY_INT2_EN       0
#define INV_MPU6509_BIT_OFFSET_FIFO_FULL_INT2_EN        1
#define INV_MPU6509_BIT_OFFSET_FIFO_THS_INT2_EN         2
#define INV_MPU6509_BIT_OFFSET_UI_DRDY_INT2_EN          3
#define INV_MPU6509_BIT_OFFSET_RESET_DONE_INT2_EN       4
#define INV_MPU6509_BIT_OFFSET_PLL_RDY_INT2_EN          5
#define INV_MPU6509_BIT_OFFSET_UI_FSYNC_INT2_EN         6

// int_source4
#define INV_MPU6509_BIT_OFFSET_WOM_X_INT2               0
#define INV_MPU6509_BIT_OFFSET_WOM_Y_INT2               1
#define INV_MPU6509_BIT_OFFSET_WOM_Z_INT2               2

// fifo config
enum inv_mpu6509_fifo_mode_e {
	INV_MPU6509_FIFO_MODE_BYPASS,                       // default
	INV_MPU6509_FIFO_MODE_STREAM_TO_FIFO,
	INV_MPU6509_FIFO_MODE_STOP_ON_FULL_2,
	INV_MPU6509_FIFO_MODE_STOP_ON_FULL_3
};

#define INV_MPU6509_BIT_OFFSET_FIFO_MODE                6

// fifo_config1
enum inv_mpu6509_fifo_config_e {
	INV_MPU6509_FIFO_DISABLE,
	INV_MPU6509_FIFO_ENABLE
};

#define INV_MPU6509_BIT_OFFSET_FIFO_ACCEL_EN                0
#define INV_MPU6509_BIT_OFFSET_FIFO_GYRO_EN                 1
#define INV_MPU6509_BIT_OFFSET_FIFO_TEMP_EN                 2
#define INV_MPU6509_BIT_OFFSET_FIFO_TMST_FSYNC_EN           3
#define INV_MPU6509_BIT_OFFSET_FIFO_WM_GT_TH                5
#define INV_MPU6509_BIT_OFFSET_FIFO_RESUME_PARTIAL_RD       6

#define INV_MPU6509_VAL_FIFO_EN_MASK                        0x0F

// int status
enum inv_mpu6509_int_status_e {
	INV_MPU6509_INT_STATUS_CLEAR,
	INV_MPU6509_INT_STATUS_SET
};

#define INV_MPU6509_BIT_OFFSET_AGC_RDY_INT              0
#define INV_MPU6509_BIT_OFFSET_FIFO_FULL_INT            1
#define INV_MPU6509_BIT_OFFSET_FIFO_THS_INT             2
#define INV_MPU6509_BIT_OFFSET_DATA_RDY_INT             3
#define INV_MPU6509_BIT_OFFSET_RESET_DONE_INT           4
#define INV_MPU6509_BIT_OFFSET_PLL_RDY_INT              5
#define INV_MPU6509_BIT_OFFSET_UI_FSYNC_INT             6

// int status2
#define INV_MPU6509_BIT_OFFSET_WOM_X_INT                0
#define INV_MPU6509_BIT_OFFSET_WOM_Y_INT                1
#define INV_MPU6509_BIT_OFFSET_WOM_Z_INT                2

// signal_path_reset
enum inv_mpu6509_fifo_flush_e {
	INV_MPU6509_FIFO_FLUSH_DISABLE,
	INV_MPU6509_FIFO_FLUSH_ENABLE
};

#define INV_MPU6509_BIT_OFFSET_FIFO_FLUSH               1

enum inv_mpu6509_tmst_strobe_e {
	INV_MPU6509_TMST_STROBE_DISABLE,
	INV_MPU6509_TMST_STROBE_ENABLE
};

#define INV_MPU6509_BIT_OFFSET_TMST_STROBE              2

enum inv_mpu6509_abort_and_reset_e {
	INV_MPU6509_ABORT_AND_RESET_DISABLE,
	INV_MPU6509_ABORT_AND_RESET_ENABLE
};

#define INV_MPU6509_BIT_OFFSET_ABORT_AND_RESET          3

// intf_config0
enum inv_mpu6509_ui_sifs_cfg_e {
	INV_MPU6509_UI_SIFS_CFG_RESV_0,
	INV_MPU6509_UI_SIFS_CFG_RESV_1,
	INV_MPU6509_UI_SIFS_CFG_DISABLE_SPI,
	INV_MPU6509_UI_SIFS_CFG_DISABLE_I2C
};

#define INV_MPU6509_BIT_OFFSET_UI_SIFS_CFG              0
#define INV_MPU6509_VAL_UI_SIFS_CFG_MASK                0x03

enum inv_mpu6509_data_endian_e {
	INV_MPU6509_DATA_ENDIAN_LITTLE,
	INV_MPU6509_DATA_ENDIAN_BIG                         // default
};

#define INV_MPU6509_BIT_OFFSET_SENSOR_DATA_ENDIAN       4
#define INV_MPU6509_BIT_OFFSET_FIFO_COUNT_ENDIAN        5

enum inv_mpu6509_fifo_count_rec_e {
	INV_MPU6509_FIFO_COUNT_REC_IN_BYTES,
	INV_MPU6509_FIFO_COUNT_REC_IN_RECORDS
};

#define INV_MPU6509_BIT_OFFSET_FIFO_COUNT_REC           6

#define INV_MPU6509_BIT_OFFSET_FIFO_HOLD_LAST_DATA_EN   7

// intf_config1
enum inv_mpu6509_clksel_e {
	INV_MPU6509_CLKSEL_INTERNAL_RC_OSC,
	INV_MPU6509_CLKSEL_PLL_THEN_INTERNAL_RC_OSC,        // default
	INV_MPU6509_CLKSEL_RESV,
	INV_MPU6509_CLKSEL_DISABLE_ALL_CLOCK
};

#define INV_MPU6509_BIT_OFFSET_CLKSEL                   0

#define INV_MPU6509_BIT_OFFSET_ACCEL_LP_CLK_SEL         3

#define INV_MPU6509_BIT_OFFSET_EN_TEST_MODE             6
#define INV_MPU6509_VAL_EN_TEST_MODE_NORAML_OP          0x01

// intf_config4
enum inv_mpu6509_spi_mode_wire_e {
	INV_MPU6509_SPI_MODE_3WIRE,
	INV_MPU6509_SPI_MODE_4WIRE
};

#define INV_MPU6509_BIT_OFFSET_SPI_AP_4WIRE             1

// intf_config5
enum inv_mpu6509_pin19_func_e {
	INV_MPU6509_PIN19_FUNC_INT2,
	INV_MPU6509_PIN19_FUNC_FSYNC,
	INV_MPU6509_PIN19_FUNC_RESV_2,
	INV_MPU6509_PIN19_FUNC_RESV_3
};

#define INV_MPU6509_BIT_OFFSET_PIN19_FUNC               1

// pwr_mgmt0
enum inv_mpu6509_accel_mode_e {
	INV_MPU6509_ACCEL_MODE_OFF_0,                       // default
	INV_MPU6509_ACCEL_MODE_OFF_1,
	INV_MPU6509_ACCEL_MODE_LP,
	INV_MPU6509_ACCEL_MODE_LN
};

#define INV_MPU6509_BIT_OFFSET_ACCEL_MODE               0
#define INV_MPU6509_VAL_ACCEL_MODE_WRITE_REG_UP_TIME_US 200

enum inv_mpu6509_gyro_mode_e {
	INV_MPU6509_GYRO_MODE_OFF,                          // default
	INV_MPU6509_GYRO_MODE_STANDBY,
	INV_MPU6509_GYRO_MODE_RESV,
	INV_MPU6509_GYRO_MODE_LN
};

#define INV_MPU6509_BIT_OFFSET_GYRO_MODE                2
#define INV_MPU6509_VAL_GYRO_MODE_WRITE_REG_UP_TIME_US  200
#define INV_MPU6509_VAL_GYRO_MODE_ON_MIN_TIME_MS        45

enum inv_mpu6509_idle_e {
	INV_MPU6509_IDLE_RC_OSC_POWER_OFF,
	INV_MPU6509_IDLE_RC_OSC_POWER_ON
};

#define INV_MPU6509_BIT_OFFSET_IDLE                     4

enum inv_mpu6509_temp_sensor_en_e {
	INV_MPU6509_TEMP_SENSOR_ENABLE,                     // default
	INV_MPU6509_TEMP_SENSOR_DISABLE
};

#define INV_MPU6509_BIT_OFFSET_TEMP_DIS                 5

// gyro_config0
#define INV_MPU6509_BIT_OFFSET_GYRO_ODR                     0

#define INV_MPU6509_BIT_OFFSET_GYRO_FS_SEL                  5

// gyro_config1
enum inv_mpu6509_gyro_dec2_m2_ord_e {
	INV_MPU6509_GYRO_DEC2_M2_RESV_0,
	INV_MPU6509_GYRO_DEC2_M2_RESV_1,
	INV_MPU6509_GYRO_DEC2_M2_ORD_3,
	INV_MPU6509_GYRO_DEC2_M2_RESV_3
};

#define INV_MPU6509_BIT_OFFSET_GYRO_DEC2_M2_ORD             0

#define INV_MPU6509_BIT_OFFSET_GYRO_UI_FILT_ORD             2

enum inv_mpu6509_temp_filt_bw_e {
	INV_MPU6509_TEMP_FILT_BW_4000,
	INV_MPU6509_TEMP_FILT_BW_170,
	INV_MPU6509_TEMP_FILT_BW_82,
	INV_MPU6509_TEMP_FILT_BW_40,
	INV_MPU6509_TEMP_FILT_BW_20,
	INV_MPU6509_TEMP_FILT_BW_10,
	INV_MPU6509_TEMP_FILT_BW_5_1,
	INV_MPU6509_TEMP_FILT_BW_5_2
};

#define INV_MPU6509_BIT_OFFSET_GYRO_TEMP_FILT_BW                5

// gyro_config_static2
enum inv_mpu6509_gyro_config_aaf_dis_e {
	INV_MPU6509_GYRO_AAF_ENABLE,
	INV_MPU6509_GYRO_AAF_DISABLE
};

#define INV_MPU6509_BIT_OFFSET_GYRO_NF_DIS                      0

enum inv_mpu6509_gyro_config_nf_dis_e {
	INV_MPU6509_GYRO_NF_ENABLE,
	INV_MPU6509_GYRO_NF_DISABLE
};

#define INV_MPU6509_BIT_OFFSET_GYRO_AAF_DIS                     1

// gyro_config_static4
// gyro_config_static5
#define INV_MPU6509_BIT_OFFSET_AAF_BITSHIFT                     4

struct inv_mpu6509_aaf_bw_cfg {
	u8 deltsqr_lo;
	u8 deltsqr_hi_bitshift;
};

#define INV_MPU6509_DELTSQR_LOW(_deltsqr)                       ((_deltsqr) && 0xff)
#define INV_MPU6509_DELTSQR_HIGH(_deltsqr)                      ((_deltsqr >> 8) && ((1 << INV_MPU6509_BIT_OFFSET_AAF_BITSHIFT) - 1))

// gyro_config_static6
// gyro_config_static7
// gyro_config_static8
// gyro_config_static9

// gyro_config_static10
enum inv_mpu6509_gyro_hpf_ord_ind_e {
	INV_MPU6509_GYRO_HPF_ORD_IND_1,
	INV_MPU6509_GYRO_HPF_ORD_IND_2
};

#define INV_MPU6509_BIT_OFFSET_GYRO_HPF_ORD_IND                 0
#define INV_MPU6509_BIT_OFFSET_GYRO_HPF_BW_IND                  1
#define INV_MPU6509_BIT_OFFSET_GYRO_NF_BW_SEL                   4

// accel_config0
#define INV_MPU6509_BIT_OFFSET_ACCEL_ODR                        0

#define INV_MPU6509_BIT_OFFSET_ACCEL_FS_SEL                 5

// accel_config1
enum inv_mpu6509_accel_dec2_m2_ord_e {
	INV_MPU6509_ACCEL_DEC2_M2_RESV_0,
	INV_MPU6509_ACCEL_DEC2_M2_RESV_1,
	INV_MPU6509_ACCEL_DEC2_M2_ORD_3,
	INV_MPU6509_ACCEL_DEC2_M2_RESV_3
};

#define INV_MPU6509_BIT_OFFSET_ACCEL_DEC2_M2_ORD                1

enum inv_mpu6509_accel_ui_filt_ord_e {
	INV_MPU6509_ACCEL_UI_FILT_ORD_1,
	INV_MPU6509_ACCEL_UI_FILT_ORD_2,
	INV_MPU6509_ACCEL_UI_FILT_ORD_3,
	INV_MPU6509_ACCEL_UI_FILT_ORD_RESV
};

#define INV_MPU6509_BIT_OFFSET_ACCEL_UI_FILT_ORD                3

// accel_config_static2
enum inv_mpu6509_accel_config_aaf_dis_e {
	INV_MPU6509_ACCEL_AAF_ENABLE,
	INV_MPU6509_ACCEL_AAF_DISABLE
};

#define INV_MPU6509_BIT_OFFSET_ACCEL_AAF_DIS                        0
#define INV_MPU6509_BIT_OFFSET_ACCEL_AAF_DELT                       1

// gyro_accel_config0
#define INV_MPU6509_BIT_OFFSET_GYRO_UI_FILT_BW                  0

enum inv_6509_accel_ui_filt_bw_ln_mode_e {
	INV_MPU6509_ACCEL_UI_FILT_BW_ODR_DIV_BY_2,
	INV_MPU6509_ACCEL_UI_FILT_BW_MAX_400_ODR_DIV_BY_4,
	INV_MPU6509_ACCEL_UI_FILT_BW_MAX_400_ODR_DIV_BY_5,
	INV_MPU6509_ACCEL_UI_FILT_BW_MAX_400_ODR_DIV_BY_8,
	INV_MPU6509_ACCEL_UI_FILT_BW_MAX_400_ODR_DIV_BY_10,
	INV_MPU6509_ACCEL_UI_FILT_BW_MAX_400_ODR_DIV_BY_16,
	INV_MPU6509_ACCEL_UI_FILT_BW_MAX_400_ODR_DIV_BY_20,
	INV_MPU6509_ACCEL_UI_FILT_BW_MAX_400_ODR_DIV_BY_40,
	INV_MPU6509_ACCEL_UI_FILT_BW_RESV_8,
	INV_MPU6509_ACCEL_UI_FILT_BW_RESV_9,
	INV_MPU6509_ACCEL_UI_FILT_BW_RESV_10,
	INV_MPU6509_ACCEL_UI_FILT_BW_RESV_11,
	INV_MPU6509_ACCEL_UI_FILT_BW_RESV_12,
	INV_MPU6509_ACCEL_UI_FILT_BW_RESV_13,
	INV_MPU6509_ACCEL_UI_FILT_DEC2_MAX_400_ODR,
	INV_MPU6509_ACCEL_UI_FILT_DEC2_MAX_200_8ODR
};

enum inv_6509_accel_ui_filt_bw_lp_mode_e {
	INV_MPU6509_ACCEL_UI_FILT_RESV_0,
	INV_MPU6509_ACCEL_UI_FILT_1X_AVG,
	INV_MPU6509_ACCEL_UI_FILT_RESV_2,
	INV_MPU6509_ACCEL_UI_FILT_RESV_3,
	INV_MPU6509_ACCEL_UI_FILT_RESV_4,
	INV_MPU6509_ACCEL_UI_FILT_RESV_5,
	INV_MPU6509_ACCEL_UI_FILT_16X_AVG,
	INV_MPU6509_ACCEL_UI_FILT_RESV_7,
	INV_MPU6509_ACCEL_UI_FILT_RESV_8,
	INV_MPU6509_ACCEL_UI_FILT_RESV_9,
	INV_MPU6509_ACCEL_UI_FILT_RESV_10,
	INV_MPU6509_ACCEL_UI_FILT_RESV_11,
	INV_MPU6509_ACCEL_UI_FILT_RESV_12,
	INV_MPU6509_ACCEL_UI_FILT_RESV_13,
	INV_MPU6509_ACCEL_UI_FILT_RESV_14,
	INV_MPU6509_ACCEL_UI_FILT_RESV_15
};

#define INV_MPU6509_BIT_OFFSET_ACCEL_UI_FILT_BW             4

// tmst_config
enum inv_mpu6509_tmst_en_e {
	INV_MPU6509_TMST_DISABLE,
	INV_MPU6509_TMST_ENABLE
};

#define INV_MPU6509_BIT_OFFSET_TMST_EN                          0

enum inv_mpu6509_tmst_fsync_e {
	INV_MPU6509_TMST_FSYNC_DISABLE,
	INV_MPU6509_TMST_FSYNC_ENABLE
};

#define INV_MPU6509_BIT_OFFSET_TMST_FSYNC_EN                    1

enum inv_mpu6509_tmst_delta_en_e {
	INV_MPU6509_TMST_DELTA_DISABLE,
	INV_MPU6509_TMST_DELTA_ENABLE
};

#define INV_MPU6509_BIT_OFFSET_TMST_DELTA_EN                    2

enum inv_mpu6509_tmst_to_regs_en_e {
	INV_MPU6509_TMST_TO_REG_DISABLE,
	INV_MPU6509_TMST_TO_REG_ENABLE
};

#define INV_MPU6509_BIT_OFFSET_TMST_TO_REGS_EN                  4

// wom_config
enum inv_mpu6509_wom_mode1_e {
	INV_MPU6509_WOM_MODE1_RESV_0,
	INV_MPU6509_WOM_MODE1_CONFIG_INT,
	INV_MPU6509_WOM_MODE1_RESV_2,
	INV_MPU6509_WOM_MODE1_RESV_3
};

#define INV_MPU6509_BIT_OFFSET_WOM_MODE1                    0

enum inv_mpu6509_wom_mode_e {
	INV_MPU6509_WOM_MODE_COMP_SAMPLE_INITIAL,
	INV_MPU6509_WOM_MODE_COMP_SAMPLE_PREVIOUS
};

#define INV_MPU6509_BIT_OFFSET_WOM_MODE                     2

enum inv_mpu6509_wom_int_mode_e {
	INV_MPU6509_WOM_INT_MODE_ALL_ACCEL_THR_OR,
	INV_MPU6509_WOM_INT_MODE_ALL_ACCEL_THR_AND
};

#define INV_MPU6509_BIT_OFFSET_WOM_INT_MODE                 3

// fsync_config
enum inv_mpu6509_fsync_polarity_e {
	INV_MPU6509_FSYNC_POLARITY_RISING_EDGE,
	INV_MPU6509_FSYNC_POLARITY_FALLING_EDGE
};

#define INV_MPU6509_BIT_OFFSET_FSYNC_POLARITY                   0

enum inv_mpu6509_fsync_ui_flag_clear_sel_e {
	INV_MPU6509_FSYNC_UI_FLAG_CLEAR_AT_UPDATED,
	INV_MPU6509_FSYNC_UI_FLAG_CLEAR_AT_READ
};

#define INV_MPU6509_BIT_OFFSET_FSYNC_UI_FLAG_CLEAR_SEL          1

enum inv_mpu6509_fsync_ui_sel_e {
	INV_MPU6509_FSYNC_UI_SEL_FSYNC_FLAG_TAG_NOT,
	INV_MPU6509_FSYNC_UI_SEL_FSYNC_FLAG_TAG_TEMP_OUT_LSB,
	INV_MPU6509_FSYNC_UI_SEL_FSYNC_FLAG_TAG_GYRO_XOUT_LSB,
	INV_MPU6509_FSYNC_UI_SEL_FSYNC_FLAG_TAG_GYRO_YOUT_LSB,
	INV_MPU6509_FSYNC_UI_SEL_FSYNC_FLAG_TAG_GYRO_ZOUT_LSB,
	INV_MPU6509_FSYNC_UI_SEL_FSYNC_FLAG_TAG_ACCEL_XOUT_LSB,
	INV_MPU6509_FSYNC_UI_SEL_FSYNC_FLAG_TAG_ACCEL_YOUT_LSB,
	INV_MPU6509_FSYNC_UI_SEL_FSYNC_FLAG_TAG_ACCEL_ZOUT_LSB
};

#define INV_MPU6509_BIT_OFFSET_FSYNC_UI_SEL                 4

// self_test_config
enum inv_mpu6509_self_test_e {
	INV_MPU6509_SELF_TEST_DISABLE,
	INV_MPU6509_SELF_TEST_ENABLE
};

#define INV_MPU6509_BIT_OFFSET_EN_GX_ST                     0
#define INV_MPU6509_BIT_OFFSET_EN_GY_ST                     1
#define INV_MPU6509_BIT_OFFSET_EN_GZ_ST                     2
#define INV_MPU6509_BIT_OFFSET_EN_AX_ST                     3
#define INV_MPU6509_BIT_OFFSET_EN_AY_ST                     4
#define INV_MPU6509_BIT_OFFSET_EN_AZ_ST                     5
#define INV_MPU6509_BIT_OFFSET_ACCEL_ST_POWER               6

// reg_bank_sel
enum inv_mpu6509_reg_bank_sel_e {
	INV_MPU6509_REG_BANK_0,
	INV_MPU6509_REG_BANK_1,
	INV_MPU6509_REG_BANK_2,
	INV_MPU6509_REG_BANK_3,
	INV_MPU6509_REG_BANK_4,
	INV_MPU6509_REG_BANK_RESV_5,
	INV_MPU6509_REG_BANK_RESV_6,
	INV_MPU6509_REG_BANK_RESV_7
};

// sensor_config0
enum inv_mpu6509_sensor_config_e {
	INV_MPU6509_SENSOR_CONFIG_ON,
	INV_MPU6509_SENSOR_CONFIG_OFF
};

#define INV_MPU6509_BIT_OFFSET_XA_DISABLE                   0
#define INV_MPU6509_BIT_OFFSET_YA_DISABLE                   1
#define INV_MPU6509_BIT_OFFSET_ZA_DISABLE                   2
#define INV_MPU6509_BIT_OFFSET_XG_DISABLE                   3
#define INV_MPU6509_BIT_OFFSET_YG_DISABLE                   4
#define INV_MPU6509_BIT_OFFSET_ZG_DISABLE                   5
#define INV_MPU6509_VAL_ACCEL_AXIS_MASK                     0x07
#define INV_MPU6509_VAL_GYRO_AXIS_MASK                      0x07 << INV_MPU6509_BIT_OFFSET_XG_DISABLE

#define INV_MPU6509_VAL_WHO_AM_I                            0x3D

/**
 *  struct inv_mpu6509_chip_config - Cached chip configuration data.
 *  @odr:               output data rate
 *  @fsr:               gyro full scale range.
 *  @ui_filt_ord:       ui filter order
 *  @ui_filt_bw:        ui filter bandwidth
 *  @accl_aaf_enable:   enable accelanti-aliasing filter
 *  @accl_aaf_delt:     accel anti-aliasing filter control
 *  @gyro_aaf_enable:   enable gyro anti-aliasing filter
 *  @gyro_aaf_delt:     gyro anti-aliasing filter control
 *  @accl_fs:           accel full scale range.
 *  @enable:            master enable state.
 *  @accl_fifo_enable:  enable accel data output
 *  @gyro_fifo_enable:  enable gyro data output
 *  @fifo_wm:           fifo watermark
 *  @use_tmst:          use timestamp
 *  @tmst_res:          timestamp resolution
 */
struct inv_mpu6509_chip_config {
	u16 odr;
	unsigned int fsr: 3;
	unsigned int ui_filt_ord: 2;
	unsigned int ui_filt_bw: 4;
	unsigned int accl_aaf_enable: 1;
	unsigned int accl_aaf_delt: 6;
	unsigned int gyro_aaf_enable: 1;
	unsigned int gyro_aaf_delt: 6;
	unsigned int accl_fs: 3;
	unsigned int enable: 1;
	unsigned int accl_fifo_enable: 1;
	unsigned int gyro_fifo_enable: 1;
	unsigned int fifo_wm: 12;
	unsigned int use_tmst: 1;
	unsigned int tmst_res: 1;
};

/**
 *  struct inv_mpu6509_hw - Other important hardware information.
 *  @whoami:    Self identification byte from WHO_AM_I register
 *  @name:      name of the chip.
 *  @reg:       register map of the chip.
 *  @config:    configuration of the chip.
 */
struct inv_mpu6509_hw {
	u8 whoami;
	u8 *name;
	const struct inv_mpu6509_reg_map *reg;
	const struct inv_mpu6509_chip_config *config;
};

/* scan element definition */
enum inv_mpu6509_scan {
	INV_MPU6509_SCAN_ACCL_X,
	INV_MPU6509_SCAN_ACCL_Y,
	INV_MPU6509_SCAN_ACCL_Z,
	INV_MPU6509_SCAN_GYRO_X,
	INV_MPU6509_SCAN_GYRO_Y,
	INV_MPU6509_SCAN_GYRO_Z,
	INV_MPU6509_SCAN_TIMESTAMP,
	INV_MPU6509_SCAN_TEMPERATURE
};

/* IIO attribute address */
enum INV_MPU6509_IIO_ATTR_ADDR {
	ATTR_GYRO_MATRIX,
	ATTR_ACCL_MATRIX,
};

/* device enum */
enum inv_devices {
	INV_MPU6509,
	INV_NUM_PARTS
};

/* time sync status */
enum inv_mpu6509_time_sync_status {
	INV_TIME_SYNC_PLL_RDY,
	INV_TIME_SYNC_NORMAL,
	INV_TIME_SYNC_MAX
};

/*
 *  struct inv_mpu6509_time_sync - Sync time struct
 *  @status:        time sync status
 *  @base_kt:       baseline of kernel timestamp
 *  @ref_kt:        reference of kernel timestamp
 *  @pkt_id:        packet index
 *  @lost_pkt_cnt:  acculated lost packet count
 */
struct inv_mpu6509_time_sync {
	enum inv_mpu6509_time_sync_status status;
	s64 base_kt;
	s64 ref_kt;
	unsigned int pkt_id;
	int (*reg_read)(unsigned int reg, unsigned int *val);
	u64 lost_pkt_cnt;
};

/*
 *  struct inv_mpu6509_state - Driver state variables.
 *  @TIMESTAMP_FIFO_SIZE: fifo size for timestamp.
 *  @trig:              IIO trigger.
 *  @chip_config:   Cached attribute information.
 *  @reg:       Map of important registers.
 *  @hw:        Other hardware-specific information.
 *  @chip_type:     chip type.
 *  @time_stamp_lock:   spin lock to time stamp.
 *  @plat_data:     platform data (deprecated in favor of @orientation).
 *  @orientation:   sensor chip orientation relative to main hardware.
 *  @map        regmap pointer.
 *  @irq        interrupt number.
 */
struct inv_mpu6509_state {
#define TIMESTAMP_FIFO_SIZE 16
	struct iio_trigger  *trig;
	struct inv_mpu6509_chip_config chip_config;
	const struct inv_mpu6509_reg_map *reg;
	const struct inv_mpu6509_hw *hw;
	enum inv_devices chip_type;
	spinlock_t time_stamp_lock;
	struct i2c_mux_core *muxc;
	struct i2c_client *mux_client;
	unsigned int powerup_count;
	struct inv_mpu6050_platform_data plat_data;
	struct iio_mount_matrix orientation;
	struct inv_mpu6509_time_sync time_sync;
	struct regmap *map;
	int irq;
};

#define INV_MPU6509_INIT_FIFO_RATE           50

#define INV_MPU6509_SENSOR_UP_TIME           30

#define INV_MPU6509_TEMP_OFFSET              3312     // 52  FIFO data
#define INV_MPU6509_TEMP_SCALE               7548     // 483092   FIFO data

#define INV_MPU6509_OUTPUT_BUFF_LEN          128 //512      // in packet

#define INV_MPU6509_TIME_STAMP_TOR           5

#define INV_MPU6509_I2C_ADDR_1               0x68
#define INV_MPU6509_I2C_ADDR_2               0x69

extern const struct dev_pm_ops inv_mpu_pmops;

typedef long (*UNLOCKED_IOCTL) (struct file *, unsigned int, unsigned long);
typedef int (*INV_MPU_BUS_SETUP)(struct iio_dev *);

// ring
int inv_reset_fifo(struct iio_dev *indio_dev, enum inv_mpu6509_time_sync_status status);
int inv_enable_fifo(struct inv_mpu6509_state  *st, bool gyro_fifo_enable, bool accl_fifo_enable);
int inv_mpu6509_init_buffer(struct iio_dev *indio_dev);
void inv_mpu6509_free_buffer(struct iio_dev *indio_dev);
irqreturn_t inv_mpu6509_irq_handler(int irq, void *p);
irqreturn_t inv_mpu6509_irq_thread(int irq, void *p);

// trigger
int inv_mpu6509_setup_trigger(struct iio_dev *indio_dev);
void inv_mpu6509_cleanup_trigger(struct iio_dev *indio_dev);
void inv_mpu6509_trigger_notify_done(struct iio_dev *indio_dev);
int inv_mpu6509_set_enable(struct iio_dev *indio_dev, bool enable);
int inv_read_int_status(struct inv_mpu6509_state *st, unsigned int *status);

// core
int inv_mpu6509_init_config(struct iio_dev *indio_dev);
int inv_check_and_setup_chip(struct inv_mpu6509_state *st);
int inv_mpu6509_switch_engine(struct inv_mpu6509_state *st, bool en, u32 mask);
int inv_mpu6509_set_power_itg(struct inv_mpu6509_state *st, bool power_on);
int inv_mpu_core_probe(struct regmap *regmap, int irq, const char *name,
    int (*inv_mpu_bus_setup)(struct iio_dev *), int chip_type);
int inv_mpu_core_remove(struct device *dev);
// ioctl
void inv_mpu6509_override_ioctl(struct iio_dev *indio_dev, int (*inv_mpu_bus_setup)(struct iio_dev *));

