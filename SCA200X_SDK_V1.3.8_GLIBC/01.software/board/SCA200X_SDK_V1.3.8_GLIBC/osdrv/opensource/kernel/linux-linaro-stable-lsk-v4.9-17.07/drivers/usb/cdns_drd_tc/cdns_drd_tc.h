/**
 * cdns_drd_tc.h - Cadence USB3 DRD Controller Core file
 *
 * Copyright (C) 2016 Cadence Design Systems - http://www.cadence.com
 *
 * Authors: Rafal Ozieblo <rafalo@cadence.com>,
 *          Bartosz Folta <bfolta@cadence.com>
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

#ifndef __DRIVERS_USB_CDNS_CORE_H
#define __DRIVERS_USB_CDNS_CORE_H

#include <linux/usb/otg.h>
#include <linux/usb/otg-fsm.h>

#include <linux/phy/phy.h>

#if IS_ENABLED(CONFIG_USB_CDNS_MISC)
	#include "cdns_misc.h"
#endif

#include "drd_regs_map.h"
#include "debug.h"

#define OTG_HW_VERSION      0x200

/* STRAP 14:12 */
#define STRAP_NO_DEFAULT_CFG    0x00
#define STRAP_HOST_OTG      0x01
#define STRAP_HOST      0x02
#define STRAP_GADGET        0x04

#define A_HOST_SUSPEND      0x06
#define CDNS_ALIGN_MASK     (16 - 1)

enum cdns_role {
	CDNS_ROLE_HOST = 0,
	CDNS_ROLE_GADGET,
	CDNS_ROLE_OTG,
	CDNS_ROLE_END,
};

#if !IS_ENABLED(CONFIG_USB_CDNS_A_BIDL_ADIS_HW_TMR)
struct a_bidl_adis_worker_data {
	int worker_running;
	struct otg_fsm *fsm;
	struct delayed_work a_bidl_adis_work;
};
#endif

/**
 * struct cdns - Representation of Cadence DRD OTG controller.
 * @lock: for synchronizing
 * @dev: pointer to Cadence device struct
 * @xhci: pointer to xHCI child
 * @fsm: pointer to FSM structure
 * @otg_config: OTG controller configuration
 * @regs: pointer to base of OTG registers
 * @regs_size: size of OTG registers
 * @dr_mode: definition of OTG modes of operations
 * @otg_protocol: current OTG mode of operation
 * @otg_irq: number of OTG IRQs
 * @current_mode: current mode of operation written to PRTCAPDIR
 * @otg_version: version of OTG
 * @strap: strapped mode
 * @otg_int_vector: OTG interrupt vector
 * @mem: points to start of memory which is used for this struct
 * @cdns_misc: misc device for userspace communication
 * @a_bidl_adis_data: structure for a_bidl_adis timer implementation
 */
struct cdns_dev {
	/* device lock */
	spinlock_t      lock;

	struct device       *dev;

	struct platform_device  *xhci;

	struct otg_fsm      *fsm;
	struct usb_otg_config   otg_config;

	struct usbdrd_register_block_type __iomem *regs;
	size_t          regs_size;

	enum usb_dr_mode    dr_mode;

	int         otg_protocol;

	int         otg_irq;
	u32         current_mode;
	u16         otg_version;
	u8          strap;
	u32         otg_int_vector;
	void            *mem;

#if IS_ENABLED(CONFIG_USB_CDNS_MISC)
	struct cdns_drd_misc    cdns_misc;
#endif

#if !IS_ENABLED(CONFIG_USB_CDNS_A_BIDL_ADIS_HW_TMR)
	struct a_bidl_adis_worker_data a_bidl_adis_data;
#endif
};

/* prototypes */
void cdns_set_mode(struct cdns_dev *cdns, u32 mode);
void cdns_otg_enable_irq(struct cdns_dev *cdns, u32 irq);
void cdns_otg_disable_irq(struct cdns_dev *cdns, u32 irq);
void cdns_enable_host_alternate_mode(struct cdns_dev *cdns, int on);
void cdns_enable_gadget_alternate_mode(struct cdns_dev *cdns, int on);
int cdns_wait_for_ready(struct cdns_dev *cdns, int otgsts_bit_ready);
int cdns_wait_for_idle(struct cdns_dev *cdns, int otgsts_bits_idle);
void cdns_delay(int delay);
unsigned long cdns_to_jiffies(const unsigned int m);

#if IS_ENABLED(CONFIG_USB_CDNS_MISC)
	void cdns_otg_fsm_sync(struct cdns_dev *cdns);
	int cdns_otg_standby_allowed(struct cdns_dev *cdns);
	int cdns_otg_set_standby(struct cdns_dev *cdns);
	int cdns_otg_clear_standby(struct cdns_dev *cdns);
#endif

#endif /* __DRIVERS_USB_CDNS_CORE_H */
