/**
 * cdns_misc.c - Cadence USB3 DRD Controller Core file
 *
 * Copyright (C) 2016 Cadence Design Systems - http://www.cadence.com
 *
 * Authors: Rafal Ozieblo <rafalo@cadence.com>,
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2  of
 * the License as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <linux/fs.h>
#include <linux/kernel.h>
#include <asm/uaccess.h>  /* for put_user */

#include "cdns_misc.h"
#include "cdns_drd_tc.h"
#include "otg_fsm.h"

/*-------------------------------------------------------------------------*/
/* Miscellaneous device for DRD */

/*
 * Weak declarations for DRD:
 * - should be defined in DRD driver and overwrite below
 * - shall not be used in gadget driver
 */
int __attribute__((weak)) cdns_otg_standby_allowed(struct cdns_dev *cdns)
{
	return -EPERM;
}

int __attribute__((weak)) cdns_otg_set_standby(struct cdns_dev *cdns)
{
	return -EPERM;
}

int __attribute__((weak)) cdns_otg_clear_standby(struct cdns_dev *cdns)
{
	return -EPERM;
}

void __attribute__((weak)) cdns_set_state(struct otg_fsm *fsm,
    enum usb_drd_tc_state new_state)
{
}

void __attribute__((weak)) cdns_otg_fsm_sync(struct cdns_dev *cdns)
{
}

static int cdns_drd_open(struct inode *inode, struct file *file)
{
	int res;
	struct cdns_drd_misc *cdns_misc =
	    container_of(file->private_data, struct cdns_drd_misc, miscdev);
	struct cdns_dev *cdns =
	    container_of(cdns_misc, struct cdns_dev, cdns_misc);

	res = cdns_otg_standby_allowed(cdns);
	if (res < 0) {
		cdns_err(cdns->dev, "Can't open because of lack of symbols!\n");
		return res;
	}
	return 0;
}

static ssize_t cdns_drd_write(struct file *file, const char __user *buf,
    size_t count, loff_t *ppos)
{
	struct cdns_drd_misc *cdns_misc =
	    container_of(file->private_data, struct cdns_drd_misc, miscdev);
	struct cdns_dev *cdns =
	    container_of(cdns_misc, struct cdns_dev, cdns_misc);
	long int cmd;
	int res;

	if (buf == NULL)
		return -ENOMEM;

	res = kstrtol(buf, 10, &cmd);
	if (res)
		return res;
	cdns_set_state(cdns->fsm, cmd);
	return 1;
}

static ssize_t cdns_drd_read(struct file *file, char __user *buf,
    size_t count, loff_t *ppos)
{
	struct cdns_drd_misc *cdns_misc =
	    container_of(file->private_data, struct cdns_drd_misc, miscdev);
	struct cdns_dev *cdns =
	    container_of(cdns_misc, struct cdns_dev, cdns_misc);
	int len, i;
	char kernel_buf[5], *kernel_ptr;

	/* EOF */
	if (*ppos > 0)
		return 0;

	sprintf(kernel_buf, "%d", cdns->fsm->otg->state);
	len = strlen(kernel_buf);
	kernel_ptr = kernel_buf;
	for (i = 0; i < len; i++)
		put_user(*(kernel_ptr++), buf++);
	(*ppos)++;
	return len;
}

static long cdns_drd_unlocked_ioctl(struct file *file,
    unsigned int cmd, unsigned long arg)
{
	struct cdns_drd_misc *cdns_misc =
	    container_of(file->private_data, struct cdns_drd_misc, miscdev);
	struct cdns_dev *cdns =
	    container_of(cdns_misc, struct cdns_dev, cdns_misc);
	struct otg_fsm *fsm = cdns->fsm;
	struct usb_otg *otgd = container_of(fsm, struct usb_otg, fsm);

	switch (cmd) {

	case CDNS_DRD_SET_STATE_UNDEF:
		cdns_dbg(cdns->dev, "bfolta: function: %s, command: CDNS_DRD_SET_STATE_UNDEF\n", __func__);
		cdns_set_state(cdns->fsm, CDNS_DRD_STATE_UNDEF);
		break;

	case CDNS_DRD_SET_STATE_HOST:
		cdns_dbg(cdns->dev, "bfolta: function: %s, command: CDNS_DRD_SET_STATE_HOST\n", __func__);
		cdns_set_state(cdns->fsm, CDNS_DRD_STATE_HOST);
		break;

	case CDNS_DRD_SET_STATE_GADGET:
		cdns_dbg(cdns->dev, "bfolta: function: %s, command: CDNS_DRD_SET_STATE_GADGET\n", __func__);
		cdns_set_state(cdns->fsm, CDNS_DRD_STATE_GADGET);
		break;

	case CDNS_DRD_SET_STATE_ALT_HOST:
		cdns_dbg(cdns->dev, "bfolta: function: %s, command: CDNS_DRD_SET_STATE_ALT_HOST\n", __func__);
		cdns_set_state(cdns->fsm, CDNS_DRD_STATE_ALT_HOST);
		break;

	case CDNS_DRD_SET_STATE_ALT_GADGET:
		cdns_dbg(cdns->dev, "bfolta: function: %s, command: CDNS_DRD_SET_STATE_ALT_GADGET\n", __func__);
		cdns_set_state(cdns->fsm, CDNS_DRD_STATE_ALT_GADGET);
		break;

	case CDNS_DRD_GET_STATE:
		cdns_dbg(cdns->dev, "bfolta: function: %s, command: CDNS_DRD_GET_STATE\n", __func__);
		if (copy_to_user(
		        (int *)arg,
		        &cdns->fsm->otg->state,
		        sizeof(cdns->fsm->otg->state))
		) {
			return -EACCES;
		}
		return 0;

	case CDNS_DRD_GET_SPEED:
		cdns_dbg(cdns->dev, "bfolta: function: %s, command: CDNS_DRD_GET_SPEED\n", __func__);
		if (copy_to_user(
		        (int *)arg,
		        &otgd->gadget->speed,
		        sizeof(otgd->gadget->speed))
		) {
			return -EACCES;
		}
		return 0;

	default:
		cdns_err(cdns->dev, "bfolta: function: %s, requested command not supported\n", __func__);
		return -ENOTTY;
	}

	/* cdns_otg_fsm_sync(cdns); */
	return 0;
}

static const struct file_operations cdns_drd_file_ops = {
	.owner = THIS_MODULE,
	.open = cdns_drd_open,
	.read = cdns_drd_read,
	.write = cdns_drd_write,
	.unlocked_ioctl = cdns_drd_unlocked_ioctl,
};

void cdns_drd_misc_register(struct cdns_dev *cdns, int res_address)
{
	struct miscdevice *miscdev;

	miscdev = &cdns->cdns_misc.miscdev;
	miscdev->minor = MISC_DYNAMIC_MINOR;
	miscdev->name = kasprintf(GFP_KERNEL, "cdns-drd-tc-%x", res_address);
	miscdev->fops = &cdns_drd_file_ops;
	misc_register(miscdev);
}

/*-------------------------------------------------------------------------*/
/* Miscellaneous device for gadget */

static const struct file_operations cdns_dev_file_ops = {
	.owner = THIS_MODULE,
	.open = NULL,
	.unlocked_ioctl = NULL,
};

void cdns_dev_misc_register(struct usb_ss_dev *usb_ss, int res_address)
{
	struct miscdevice *miscdev;

	miscdev = &usb_ss->miscdev;
	miscdev->minor = MISC_DYNAMIC_MINOR;
	miscdev->name = kasprintf(GFP_KERNEL, "cdns-dev-%x", res_address);
	miscdev->fops = &cdns_dev_file_ops;
	misc_register(miscdev);
}
