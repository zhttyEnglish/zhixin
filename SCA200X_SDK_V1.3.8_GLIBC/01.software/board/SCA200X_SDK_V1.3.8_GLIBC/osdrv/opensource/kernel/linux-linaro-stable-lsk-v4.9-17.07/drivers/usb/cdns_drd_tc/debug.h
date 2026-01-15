/**
 * debug.h - Cadence USB3 Device Controller debug
 *
 * Copyright (C) 2016 Cadence Design Systems - http://www.cadence.com
 *
 * Authors: Pawel Jez <pjez@cadence.com>
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
#ifndef __DRIVERS_USB_SS_DEBUG
#define __DRIVERS_USB_SS_DEBUG

#include <linux/usb/ch9.h>
#include <linux/usb/gadget.h>
#include "cdns_gadget.h"

void display_ep_desc(struct usb_ss_dev *usb_ss, struct usb_ep *usb_endpoint);
void print_all_ep(struct usb_ss_dev *usb_ss);
void usb_ss_dump_regs(struct usb_ss_dev *usb_ss);
void usb_ss_dump_reg(struct usb_ss_dev *usb_ss, uint32_t __iomem *reg);

void usb_ss_dbg_dump_lnx_usb_ep(struct device *dev, struct usb_ep *ep, int tab);
void usb_ss_dbg_dump_lnx_usb_gadget(struct usb_gadget *gadget);
void usb_ss_dbg_dump_lnx_usb_request(struct usb_ss_dev *usb_ss,
    struct usb_request *usb_req);

void usb_ss_dbg_dump_cdns_usb_ss_dev(struct usb_ss_dev *usb_ss);
void usb_ss_dbg_dump_cdns_usb_ss_ep(struct usb_ss_dev *usb_ss,
    struct usb_ss_endpoint *usb_ss_ep);
void usb_ss_dbg_dump_cdns_usb_ss_trb(struct usb_ss_dev *usb_ss,
    struct usb_ss_trb *usb_trb);

/* Enable Cadence USB debugging */
#define CDNS_DBG_ENABLED

#ifdef CDNS_DBG_ENABLED
#define cdns_dbg(dev, fmt, args...) \
    pr_emerg(fmt, ## args)
#define cdns_err(dev, fmt, args...) \
    pr_emerg(fmt, ## args)
#define cdns_warn(dev, fmt, args...) \
    pr_emerg(fmt, ## args)
#define cdns_warn_ratelimited(dev, fmt, args...) \
    pr_emerg(fmt, ## args)
#define cdns_info(dev, fmt, args...) \
    pr_emerg(fmt, ## args)
#else
#define cdns_dbg(dev, fmt, args...) \
    dev_dbg(dev, fmt, ## args)
#define cdns_err(dev, fmt, args...) \
    dev_err(dev, fmt, ## args)
#define cdns_warn(dev, fmt, args...) \
    dev_warn(dev, fmt, ## args)
#define cdns_warn_ratelimited(dev, fmt, args...) \
    dev_warn_ratelimited(dev, fmt, ## args)
#define cdns_info(dev, fmt, args...) \
    dev_info(dev, fmt, ## args)
#endif

#endif
