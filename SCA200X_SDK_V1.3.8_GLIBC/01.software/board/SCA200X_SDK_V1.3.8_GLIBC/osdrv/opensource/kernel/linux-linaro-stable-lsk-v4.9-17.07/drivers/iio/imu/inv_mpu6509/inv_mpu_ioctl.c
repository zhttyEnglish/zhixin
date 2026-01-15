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
#include "inv_mpu_iio.h"
#include "inv_mpu_ioctl.h"

static UNLOCKED_IOCTL iio_legacy_unlocked_ioctl;
static INV_MPU_BUS_SETUP inv_mpu6509_bus_setup;

static void inv_mpu6509_convert_config_from_user(MPU_CONFIG_T *usr_cfg, struct inv_mpu6509_chip_config *drv_cfg)
{
	drv_cfg->odr = usr_cfg->odr;
	drv_cfg->fsr = usr_cfg->gyro_fsr;
	drv_cfg->ui_filt_ord = usr_cfg->ui_filt_ord;
	drv_cfg->ui_filt_bw = usr_cfg->ui_filt_bw;

	if(usr_cfg->accl_aaf_enable)
		drv_cfg->accl_aaf_enable = INV_MPU6509_ACCEL_AAF_ENABLE;
	else
		drv_cfg->accl_aaf_enable = INV_MPU6509_ACCEL_AAF_DISABLE;

	drv_cfg->accl_aaf_delt = usr_cfg->accl_aaf_delt;

	if(usr_cfg->gyro_aaf_enable)
		drv_cfg->gyro_aaf_enable = INV_MPU6509_GYRO_AAF_ENABLE;
	else
		drv_cfg->gyro_aaf_enable = INV_MPU6509_GYRO_AAF_DISABLE;

	drv_cfg->gyro_aaf_delt = usr_cfg->gyro_aaf_delt;
	drv_cfg->accl_fs = usr_cfg->accl_fsr;

	if(usr_cfg->gyro_enable || usr_cfg->accl_enable)
		drv_cfg->enable = 1;
	else
		drv_cfg->enable = 0;

	drv_cfg->gyro_fifo_enable = usr_cfg->gyro_fifo_enable ? 1 : 0;
	drv_cfg->accl_fifo_enable = usr_cfg->accl_fifo_enable ? 1 : 0;

	drv_cfg->use_tmst = usr_cfg->use_tmst ? 1 : 0;
	if(usr_cfg->tmst_res == 1)
		drv_cfg->tmst_res = 0;
	else
		drv_cfg->tmst_res = 1;

	drv_cfg->fifo_wm = usr_cfg->fifo_wm;
}

static void inv_mpu6509_convert_config_to_user(MPU_CONFIG_T *usr_cfg, struct inv_mpu6509_chip_config *drv_cfg)
{
	usr_cfg->odr = drv_cfg->odr;
	usr_cfg->gyro_fsr = drv_cfg->fsr;
	usr_cfg->ui_filt_ord = drv_cfg->ui_filt_ord;
	usr_cfg->ui_filt_bw = drv_cfg->ui_filt_bw;
	usr_cfg->accl_aaf_enable = !!(drv_cfg->accl_aaf_enable == INV_MPU6509_ACCEL_AAF_ENABLE);
	usr_cfg->accl_aaf_delt = drv_cfg->accl_aaf_delt;
	usr_cfg->gyro_aaf_enable = !!(drv_cfg->gyro_aaf_enable == INV_MPU6509_GYRO_AAF_ENABLE);
	usr_cfg->gyro_aaf_delt = drv_cfg->gyro_aaf_delt;
	usr_cfg->accl_fsr = drv_cfg->accl_fs;
	usr_cfg->gyro_enable = !!(drv_cfg->enable == 1);
	usr_cfg->accl_enable = !!(drv_cfg->enable == 1);
	usr_cfg->gyro_fifo_enable = !!(drv_cfg->gyro_fifo_enable == 1);
	usr_cfg->accl_fifo_enable = !!(drv_cfg->accl_fifo_enable == 1);
	usr_cfg->use_tmst = !!(drv_cfg->use_tmst == 1);
	if(drv_cfg->tmst_res == 0)
		usr_cfg->tmst_res = 1;
	else
		usr_cfg->tmst_res = 16;
	usr_cfg->fifo_wm = drv_cfg->fifo_wm;
}

static long inv_mpu6509_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	struct iio_dev *indio_dev = filp->private_data;
	struct inv_mpu6509_state *st = iio_priv(indio_dev);
	int result = 0;

	if (!indio_dev->info)
		return -ENODEV;

	switch(cmd) {
	case MPU_REQ_INIT: {
		MPU_INIT_T mpu_init;
		struct inv_mpu6509_chip_config drv_cfg;
		result = copy_from_user(&mpu_init, (const void __user *)arg, sizeof(MPU_INIT_T));
		if(result) {
			pr_debug("copy_from_user failed %d\n", result);
			return result;
		}
		inv_mpu6509_convert_config_from_user(&mpu_init.config, &drv_cfg);
		result = inv_check_and_setup_chip(st);
		if (result) {
			pr_debug("check and setup chip failed 0x%x\n", result);
			return result;
		}

		if (inv_mpu6509_bus_setup)
			inv_mpu6509_bus_setup(indio_dev);

		memcpy(&st->chip_config, &drv_cfg, sizeof(drv_cfg));
		result = inv_mpu6509_init_config(indio_dev);
		if (result) {
			pr_debug("Could not initialize device 0x%x\n", result);
			return result;
		}

		result = inv_mpu6509_init_buffer(indio_dev);
		if(result) {
			pr_debug("inv_mpu6509_init_buffer failed %d\n", result);
			return result;
		}

		result = inv_mpu6509_set_enable(indio_dev, true);
		if(result) {
			pr_debug("inv_mpu6509_set_enable failed 0x%x\n", result);
			return result;
		}
	}
	return result;

	case MPU_REQ_SET_CONFIG: {
		MPU_SET_CONFIG_T set_cfg;
		struct inv_mpu6509_chip_config drv_cfg;
		result = copy_from_user(&set_cfg, (const void __user *)arg, sizeof(MPU_SET_CONFIG_T));
		if(result)
			return result;
		inv_mpu6509_convert_config_from_user(&set_cfg.config, &drv_cfg);
		memcpy(&st->chip_config, &drv_cfg, sizeof(drv_cfg));
		result = inv_mpu6509_init_config(indio_dev);
		if (result) {
			pr_debug("Could not initialize device.\n");
			return result;
		}
	}
	return result;

	case MPU_REQ_CTRL_FIFO: {
		MPU_CTRL_FIFO_T usr_ctrl;
		result = copy_from_user(&usr_ctrl, (const void __user *)arg, sizeof(MPU_CTRL_FIFO_T));
		if(result)
			return result;
		result = inv_enable_fifo(st, usr_ctrl.gyro_fifo_enable, usr_ctrl.accl_fifo_enable);
		if(result)
			return result;
	}
	return result;

	case MPU_REQ_RESET:
		result = inv_mpu6509_set_enable(indio_dev, false);
		if(result) {
			pr_debug("inv_mpu6509_set_enable failed 0x%x\n", result);
		}
		return result;

	case MPU_REQ_GET_CONFIG: {
		MPU_GET_CONFIG_T get_cfg;
		inv_mpu6509_convert_config_to_user(&get_cfg.config, &st->chip_config);
		result = copy_to_user((void __user *)arg, &get_cfg, sizeof(MPU_GET_CONFIG_T));
	}
	return result;

	default:
		return iio_legacy_unlocked_ioctl(filp, cmd, arg);
	}
}

static struct file_operations inv_mpu6509_fileops;

void inv_mpu6509_override_ioctl(struct iio_dev *indio_dev, INV_MPU_BUS_SETUP inv_mpu_bus_setup)
{
	iio_legacy_unlocked_ioctl = indio_dev->chrdev.ops->unlocked_ioctl;
	inv_mpu6509_fileops = *(indio_dev->chrdev.ops);
	inv_mpu6509_fileops.compat_ioctl = inv_mpu6509_ioctl;
	inv_mpu6509_fileops.unlocked_ioctl = inv_mpu6509_ioctl;
	indio_dev->chrdev.ops = &inv_mpu6509_fileops;
	inv_mpu6509_bus_setup = inv_mpu_bus_setup;
}

