/**
 * otg_fsm.h - Cadence USB3 OTG Controller finite state machine
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
 */

#ifndef __CDNS_OTG_FSM_H
#define __CDNS_OTG_FSM_H

#include <linux/workqueue.h>

enum usb_drd_tc_state {
	CDNS_DRD_INVALID_STATE = 0,
	CDNS_DRD_STATE_UNDEF,
	CDNS_DRD_STATE_HOST,
	CDNS_DRD_STATE_GADGET,
	CDNS_DRD_STATE_ALT_HOST,
	CDNS_DRD_STATE_ALT_GADGET,
};

void cdns_usb_otg_work(struct work_struct *work);
void cdns_set_state(struct otg_fsm *fsm, enum usb_drd_tc_state new_state);

#endif /* __CDNS_OTG_FSM_H */
