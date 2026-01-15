/**
 * cdns_cmd.h - Cadence USB3 DRD Controller Core file
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

#ifndef __CDNS_CMD_H
#define __CDNS_CMD_H

#include <linux/ioctl.h>
#define IOC_MAGIC 'k' /* defines the magic number */

#define CDNS_DRD_SET_STATE_UNDEF    _IO(IOC_MAGIC, 1)
#define CDNS_DRD_SET_STATE_HOST     _IO(IOC_MAGIC, 2)
#define CDNS_DRD_SET_STATE_GADGET   _IO(IOC_MAGIC, 3)
#define CDNS_DRD_SET_STATE_ALT_HOST _IO(IOC_MAGIC, 4)
#define CDNS_DRD_SET_STATE_ALT_GADGET   _IO(IOC_MAGIC, 5)
#define CDNS_DRD_GET_STATE      _IOR(IOC_MAGIC, 11, int)
#define CDNS_DRD_GET_SPEED      _IOR(IOC_MAGIC, 12, int)
#endif /* __CDNS_CMD_H */
