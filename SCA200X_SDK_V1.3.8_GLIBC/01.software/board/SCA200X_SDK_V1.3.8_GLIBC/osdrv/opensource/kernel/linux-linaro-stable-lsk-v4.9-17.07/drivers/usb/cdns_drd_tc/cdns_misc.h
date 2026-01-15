/**
 * cdns_misc.h - Cadence USB3 DRD Controller Core file
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

#ifndef __CDNS_MISC_H
#define __CDNS_MISC_H

#include <linux/miscdevice.h>
#include <linux/usb/phy.h>
#include <linux/usb/otg-fsm.h>

#include "cdns_cmd.h"
#include "otg_fsm.h"

struct cdns_dev;
struct usb_ss_dev;

struct cdns_drd_misc {
	struct miscdevice   miscdev;
};

void cdns_dev_misc_register(struct usb_ss_dev *usb_ss, int res_address);
void cdns_drd_misc_register(struct cdns_dev *cdns, int res_address);

#endif /* __CDNS_MISC_H */
