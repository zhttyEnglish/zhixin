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
#include <linux/ioctl.h>
#include <linux/types.h>

#define INV_MPU_DEV_NAME    "sc_imu"

// include/uapi/linux/iio/events.h
#define MPU_IOCTL_MAGIC     'i'

#define INV_MPU6509_BYTES_FIFO_HDR           1
#define INV_MPU6509_BYTES_PER_AXIS_SENSOR    2
#define INV_MPU6509_BYTES_PER_3AXIS_SENSOR   (3*INV_MPU6509_BYTES_PER_AXIS_SENSOR)
#define INV_MPU6509_BYTES_FIFO_TEMP          1
#define INV_MPU6509_BYTES_FIFO_TMST          2

#define INV_MPU6509_FIFO_PACKETS_LEN_MODE1   \
    (INV_MPU6509_BYTES_FIFO_HDR + \
    INV_MPU6509_BYTES_PER_3AXIS_SENSOR + \
    INV_MPU6509_BYTES_FIFO_TEMP)

#define INV_MPU6509_FIFO_PACKETS_LEN_MODE2    INV_MPU6509_FIFO_PACKETS_LEN_MODE1

#define INV_MPU6509_FIFO_PACKETS_LEN_MODE3   \
    (INV_MPU6509_BYTES_FIFO_HDR + \
    INV_MPU6509_BYTES_PER_3AXIS_SENSOR + \
    INV_MPU6509_BYTES_PER_3AXIS_SENSOR + \
    INV_MPU6509_BYTES_FIFO_TEMP + \
    INV_MPU6509_BYTES_FIFO_TMST)

#define INV_MPU6509_SOFT_TIMESTATMP_LEN      (sizeof(signed long long))

//related to wm
#define INV_MPU6509_TIME_SYNC_PKT_MAX_CNT    (16)

// enum define below, don't modify
enum inv_mpu6509_gyro_fs_sel_e {
	INV_MPU6509_GYRO_FS_SEL_2000,
	INV_MPU6509_GYRO_FS_SEL_1000,
	INV_MPU6509_GYRO_FS_SEL_500,
	INV_MPU6509_GYRO_FS_SEL_250,
	INV_MPU6509_GYRO_FS_SEL_125,
	INV_MPU6509_GYRO_FS_SEL_62_5,
	INV_MPU6509_GYRO_FS_SEL_31_25,
	INV_MPU6509_GYRO_FS_SEL_15_625,
	INV_MPU6509_GYRO_FS_SEL_MAX
};

#define INV_MPU6509_VAL_ODR_MASK                            0x0F

enum inv_mpu6509_gyro_odr_e {
	INV_MPU6509_GYRO_ODR_RESV_0,
	INV_MPU6509_GYRO_ODR_32K,
	INV_MPU6509_GYRO_ODR_16K,
	INV_MPU6509_GYRO_ODR_8K,
	INV_MPU6509_GYRO_ODR_4K,
	INV_MPU6509_GYRO_ODR_2K,
	INV_MPU6509_GYRO_ODR_1K,
	INV_MPU6509_GYRO_ODR_200,
	INV_MPU6509_GYRO_ODR_100,
	INV_MPU6509_GYRO_ODR_50,
	INV_MPU6509_GYRO_ODR_25,
	INV_MPU6509_GYRO_ODR_12_5,
	INV_MPU6509_GYRO_ODR_RESV_12,
	INV_MPU6509_GYRO_ODR_RESV_13,
	INV_MPU6509_GYRO_ODR_RESV_14,
	INV_MPU6509_GYRO_ODR_500,
	INV_MPU6509_GYRO_ODR_MAX
};

enum inv_mpu6509_accel_fs_sel_e {
	INV_MPU6509_ACCEL_FS_SEL_32,
	INV_MPU6509_ACCEL_FS_SEL_16,
	INV_MPU6509_ACCEL_FS_SEL_8,
	INV_MPU6509_ACCEL_FS_SEL_4,
	INV_MPU6509_ACCEL_FS_SEL_RESV_4,
	INV_MPU6509_ACCEL_FS_SEL_RESV_5,
	INV_MPU6509_ACCEL_FS_SEL_RESV_6,
	INV_MPU6509_ACCEL_FS_SEL_RESV_7,
	INV_MPU6509_ACCEL_FS_SEL_MAX = INV_MPU6509_ACCEL_FS_SEL_RESV_4
};

// gyro_config_static3
#define INV_MPU6509_GYRO_AAF_DELT_MAX      0x40

// accl_config_static2
#define INV_MPU6509_ACCL_AAF_DELT_MAX      0x40

enum inv_mpu6509_accel_odr_e {
	INV_MPU6509_ACCEL_ODR_RESV_0,
	INV_MPU6509_ACCEL_ODR_32K,
	INV_MPU6509_ACCEL_ODR_16K,
	INV_MPU6509_ACCEL_ODR_8K,
	INV_MPU6509_ACCEL_ODR_4K,
	INV_MPU6509_ACCEL_ODR_2K,
	INV_MPU6509_ACCEL_ODR_1K,
	INV_MPU6509_ACCEL_ODR_200,
	INV_MPU6509_ACCEL_ODR_100,
	INV_MPU6509_ACCEL_ODR_50,
	INV_MPU6509_ACCEL_ODR_25,
	INV_MPU6509_ACCEL_ODR_12_5,
	INV_MPU6509_ACCEL_ODR_6_25,
	INV_MPU6509_ACCEL_ODR_3_125,
	INV_MPU6509_ACCEL_ODR_1_5625,
	INV_MPU6509_ACCEL_ODR_500,
	INV_MPU6509_ACCEL_ODR_MAX
};

// gyro_config1
enum inv_mpu6509_gyro_ui_filt_ord_e {
	INV_MPU6509_GYRO_UI_FILT_ORD_1,
	INV_MPU6509_GYRO_UI_FILT_ORD_2,
	INV_MPU6509_GYRO_UI_FILT_ORD_3,
	INV_MPU6509_GYRO_UI_FILT_ORD_RESV,
	INV_MPU6509_GYRO_UI_FILT_ORD_MAX
};

// gyro_accel_config0
enum inv_mpu6509_gyro_ui_filt_bw_e {
	INV_MPU6509_GYRO_UI_FILT_BW_ODR_DIV_BY_2,
	INV_MPU6509_GYRO_UI_FILT_BW_MAX_400_ODR_DIV_BY_4,
	INV_MPU6509_GYRO_UI_FILT_BW_MAX_400_ODR_DIV_BY_5,
	INV_MPU6509_GYRO_UI_FILT_BW_MAX_400_ODR_DIV_BY_8,
	INV_MPU6509_GYRO_UI_FILT_BW_MAX_400_ODR_DIV_BY_10,
	INV_MPU6509_GYRO_UI_FILT_BW_MAX_400_ODR_DIV_BY_16,
	INV_MPU6509_GYRO_UI_FILT_BW_MAX_400_ODR_DIV_BY_20,
	INV_MPU6509_GYRO_UI_FILT_BW_MAX_400_ODR_DIV_BY_40,
	INV_MPU6509_GYRO_UI_FILT_BW_RESV_8,
	INV_MPU6509_GYRO_UI_FILT_BW_RESV_9,
	INV_MPU6509_GYRO_UI_FILT_BW_RESV_10,
	INV_MPU6509_GYRO_UI_FILT_BW_RESV_11,
	INV_MPU6509_GYRO_UI_FILT_BW_RESV_12,
	INV_MPU6509_GYRO_UI_FILT_BW_RESV_13,
	INV_MPU6509_GYRO_UI_FILT_DEC2_MAX_400_ODR,
	INV_MPU6509_GYRO_UI_FILT_DEC2_MAX_200_8ODR,
	INV_MPU6509_GYRO_UI_FILT_BW_MAX
};

enum inv_mpu6509_tmst_resolution_e {
	INV_MPU6509_TMST_RES_1US,
	INV_MPU6509_TMST_RES_16US
};

#define INV_MPU6509_BIT_OFFSET_TMST_RES                     3

//fifo_config3
#define INV_MPU6509_FIFO_WM_BIT_HIGHEST                     11

// ioctl define below
typedef struct {
	int             dummy;
} MPU_RESET_T;

typedef struct {
	unsigned int    odr;

	bool            gyro_enable;
	unsigned int    gyro_fsr;
	bool            gyro_aaf_enable;
	unsigned int    gyro_aaf_delt;
	bool            gyro_fifo_enable;

	bool            accl_enable;
	unsigned int    accl_fsr;
	bool            accl_aaf_enable;
	unsigned int    accl_aaf_delt;
	bool            accl_fifo_enable;

	unsigned int    fifo_wm;

	unsigned int    ui_filt_ord;
	unsigned int    ui_filt_bw;

	bool            use_tmst;
	unsigned int    tmst_res;
} MPU_CONFIG_T;

typedef struct {
	MPU_CONFIG_T    config;
} MPU_INIT_T;

typedef struct {
	MPU_CONFIG_T    config;
} MPU_SET_CONFIG_T;

typedef struct {
	MPU_CONFIG_T    config;
} MPU_GET_CONFIG_T;

typedef struct {
	bool            gyro_fifo_enable;
	bool            accl_fifo_enable;
} MPU_CTRL_FIFO_T;

#define MPU_REQ_INIT                    _IOW(MPU_IOCTL_MAGIC, 0, MPU_INIT_T)
#define MPU_REQ_SET_CONFIG          _IOW(MPU_IOCTL_MAGIC, 1, MPU_SET_CONFIG_T)
#define MPU_REQ_CTRL_FIFO           _IOW(MPU_IOCTL_MAGIC, 2, MPU_CTRL_FIFO_T)
#define MPU_REQ_RESET               _IOW(MPU_IOCTL_MAGIC, 3, MPU_RESET_T)
#define MPU_REQ_GET_CONFIG          _IOR(MPU_IOCTL_MAGIC, 4, MPU_GET_CONFIG_T)

