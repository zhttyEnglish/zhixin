/**
 * otg_fsm.c - Cadence USB3 OTG Controller finite state machine
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
#include <linux/kernel.h>
#include <linux/usb/otg.h>
#include <linux/usb/hcd.h>
#include <linux/usb/ch11.h>

#include "../host/xhci.h"

#include "../core/otg_whitelist.h"

#include "cdns_drd_tc.h"
#include "otg_fsm.h"
#include "io.h"

static int cdns_statemachine(struct otg_fsm *fsm);

/**
 * cdns_set_protocol - switches mode of operation between HOST and DEVICE.
 * @fsm: pointer to otg_fsm structure
 * @protocol: new mode of operation
 *
 * Returns 0 if change of mode was successful.
 */
static int cdns_set_protocol(struct otg_fsm *fsm, int protocol)
{
	struct device *dev = usb_otg_fsm_to_dev(fsm);
	struct cdns_dev *cdns = dev_get_drvdata(dev);
	int ret = 0;

	if (fsm->protocol != protocol) {
		cdns_dbg(otgd->dev, "bfolta: function: %s changing role fsm->protocol= %d; new protocol= %d\n",
		    __func__, fsm->protocol, protocol);
		/* stop old protocol */
		if (fsm->protocol == PROTO_HOST)
			ret = otg_start_host(fsm, 0);
		else if (fsm->protocol == PROTO_GADGET)
			ret = otg_start_gadget(fsm, 0);
		else if (fsm->protocol == PROTO_ALT_HOST) {
			ret = otg_start_host(fsm, 0);
			cdns_enable_host_alternate_mode(cdns, 0);
		} else if (fsm->protocol == PROTO_ALT_GADGET) {
			ret = otg_start_gadget(fsm, 0);
			cdns_enable_gadget_alternate_mode(cdns, 0);
		}
		if (ret) {
			cdns_dbg(otgd->dev, "bfolta: function: %s, ret is NON zero\n", __func__);
			return ret;
		}
		/* start new protocol */
		if (protocol == PROTO_UNDEF)
			;
		else if (protocol == PROTO_HOST)
			ret = otg_start_host(fsm, 1);
		else if (protocol == PROTO_GADGET)
			ret = otg_start_gadget(fsm, 1);
		else if (protocol == PROTO_ALT_HOST) {
			cdns_enable_host_alternate_mode(cdns, 1);
			ret = otg_start_host(fsm, 1);
		} else if (protocol == PROTO_ALT_GADGET) {
			cdns_enable_gadget_alternate_mode(cdns, 1);
			ret = otg_start_gadget(fsm, 1);
		}
		if (ret)
			return ret;

		fsm->protocol = protocol;
		return 0;
	}

	return 0;
}

/**
 * cdns_set_state - is called on transition of DRD state machine.
 * @fsm: pointer to otg_fsm structure
 * @new_state: destination state of transition
 *
 * This function does not return any value.
 */
void cdns_set_state(struct otg_fsm *fsm, enum usb_drd_tc_state new_state)
{
	struct device *dev = usb_otg_fsm_to_dev(fsm);

	if (fsm->otg->state == new_state)
		return;

	fsm->state_changed = 1;

	switch (new_state) {

	case CDNS_DRD_STATE_UNDEF:
		cdns_set_protocol(fsm, PROTO_UNDEF);
		usb_otg_sync_inputs(fsm);
		cdns_dbg(otgd->dev, "bfolta: function: %s, new state: %d\n",
		    __func__, new_state);
		break;

	case CDNS_DRD_STATE_HOST:
		cdns_set_protocol(fsm, PROTO_HOST);
		usb_otg_sync_inputs(fsm);
		cdns_dbg(otgd->dev, "bfolta: function: %s, new state: %d\n",
		    __func__, new_state);
		break;

	case CDNS_DRD_STATE_GADGET:
		cdns_set_protocol(fsm, PROTO_GADGET);
		usb_otg_sync_inputs(fsm);
		cdns_dbg(otgd->dev, "bfolta: function: %s, new state: %d\n",
		    __func__, new_state);
		break;

	case CDNS_DRD_STATE_ALT_HOST:
		cdns_set_protocol(fsm, PROTO_ALT_HOST);
		usb_otg_sync_inputs(fsm);
		cdns_dbg(otgd->dev, "bfolta: function: %s, new state: %d\n",
		    __func__, new_state);
		break;

	case CDNS_DRD_STATE_ALT_GADGET:
		cdns_set_protocol(fsm, PROTO_ALT_GADGET);
		usb_otg_sync_inputs(fsm);
		cdns_dbg(otgd->dev, "bfolta: function: %s, new state: %d\n",
		    __func__, new_state);
		break;

	default:
		cdns_dbg(otgd->dev, "bfolta: function: %s, new state: %d, new state is invalid\n",
		    __func__, new_state);
		break;

	}
	fsm->otg->state = new_state;
}

/**
 * cdns_statemachine - defines condifitons for transitions depending on
 * current state of state machine and otg_fsm structure vars.
 * @fsm: pointer to otg_fsm structure
 *
 * Returns 1 state was not changed, 0 if state was changed
 * (conditions for state change ware met, cdns_set_state() function
 * was called).
 */
static int cdns_statemachine(struct otg_fsm *fsm)
{
	struct device *dev = usb_otg_fsm_to_dev(fsm);
	enum usb_otg_state state;

	mutex_lock(&fsm->lock);

	state = fsm->otg->state;
	fsm->state_changed = 0;

	switch (state) {

	case CDNS_DRD_STATE_UNDEF:
		break;

	case CDNS_DRD_STATE_HOST:
		break;

	case CDNS_DRD_STATE_GADGET:
		break;

	case CDNS_DRD_STATE_ALT_HOST:
		break;

	case CDNS_DRD_STATE_ALT_GADGET:
		break;

	default:
		break;

	}

	mutex_unlock(&fsm->lock);
	return fsm->state_changed;
}

/**
 * Cadence OTG FSM/DRD work function called by FSM worker.
 */
void cdns_usb_otg_work(struct work_struct *work)
{
	struct usb_otg *otgd = container_of(work, struct usb_otg, work);

	/* CDNS OTG state machine */
	cdns_statemachine(&otgd->fsm);
}
