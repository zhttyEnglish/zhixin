/**
 * cdns_drd_tc.c - Cadence USB3 DRD Controller Core file
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

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/spinlock.h>
#include <linux/platform_device.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/usb/of.h>
#include <linux/usb/otg.h>
#include <linux/of.h>
#include <linux/of_platform.h>
#include <linux/workqueue.h>

#include "cdns_drd_tc.h"
#include "io.h"
#include "otg_fsm.h"

/**
 * cdns_set_mode - change mode of OTG Core
 * @cdns: pointer to our context structure
 * @mode: selected mode from cdns_role
 */
void cdns_set_mode(struct cdns_dev *cdns, u32 mode)
{
	u32 reg;

	switch (mode) {
	case CDNS_ROLE_GADGET:
		cdns_writel(&cdns->regs->OTGCMD,
		    OTGCMD_TYPE__DEV_BUS_REQ__MASK
		    | OTGCMD_TYPE__OTG_DIS__MASK);
		break;
	case CDNS_ROLE_HOST:
		printk(KERN_ERR"set host mode cdns\n");
		cdns_writel(&cdns->regs->OTGCMD,
		    OTGCMD_TYPE__HOST_BUS_REQ__MASK
		    | OTGCMD_TYPE__OTG_DIS__MASK);
		break;
	case CDNS_ROLE_OTG:
		reg = cdns_readl(&cdns->regs->OTGCTRL1);
		cdns_writel(&cdns->regs->OTGCTRL1,
		    OTGCTRL1_TYPE__IDPULLUP__SET(reg));
		/*
		 * wait until valid ID (ID_VALUE) can be sampled (50ms)
		 */
		cdns_delay(50);
		/*
		 * OTG mode is initialized later
		 */
		break;
	default:
		cdns_err(cdns->dev, "Unsupported mode of operation %d\n", mode);
		return;
	}
	cdns->current_mode = mode;
}

/**
 * cdns_core_init - Low-level initialization of Cadence OTG Core
 * @cdns: Pointer to our controller context structure
 *
 * Returns 0 on success otherwise negative errno.
 */
static int cdns_core_init(struct cdns_dev *cdns)
{
	u32 reg;

	reg = cdns_readl(&cdns->regs->OTGVERSION);

	if ((OTGVERSION_TYPE__OTGVERSION__READ(reg)) != OTG_HW_VERSION) {
		cdns_err(cdns->dev, "this is not a Cadence DRD TypeC  Core\n");
		return -ENODEV;
	}
	cdns->otg_version = OTGVERSION_TYPE__OTGVERSION__READ(reg);

	reg = cdns_readl(&cdns->regs->OTGSTS);
	if (OTGSTS_TYPE__OTG_NRDY__READ(reg) != 0) {
		cdns_err(cdns->dev, "Cadence USB3 OTG device not ready\n");
		return -ENODEV;
	}

#ifdef USB_SIM_SPEED_UP
	cdns_dbg(cdns->dev, "Enable fast simulation timing modes\n");
	cdns_writel(&cdns->regs->OTGSIMULATE,
	    OTGSIMULATE_TYPE__OTG_CFG_FAST_SIMS__MASK);
#endif
	return 0;
}

/**
 * cdns_otg_fsm_sync - Get OTG events and sync it to OTG fsm
 * @cdns: Pointer to our controller context structure
 */
void cdns_otg_fsm_sync(struct cdns_dev *cdns)
{
	u32 reg;
	int id, vbus;

	cdns_err(cdns->dev, "bfolta: function: %s\n", __func__);

	reg = cdns_readl(&cdns->regs->OTGSTS);

	id = OTGSTS_TYPE__ID_VALUE__READ(reg);
	vbus = OTGSTS_TYPE__SESSION_VALID__READ(reg);

	cdns->fsm->id = id;
	cdns->fsm->b_sess_vld = vbus;
	cdns->fsm->overcurrent =
	    OTGIEN_TYPE__OVERCURRENT_INT_EN__READ(cdns->otg_int_vector);

	usb_otg_sync_inputs(cdns->fsm);
}

/**
 * cdns_otg_standby_allowed - standby (aka slow reference clock)
 * is allowed only when both modes (host/device) are off
 * @cdns: Pointer to our controller context structure
 *
 *  Returns 1 if allowed otherwise 0.
 */
int cdns_otg_standby_allowed(struct cdns_dev *cdns)
{
	if ((cdns->fsm->otg->state == OTG_STATE_B_IDLE) ||
	    (cdns->fsm->otg->state == OTG_STATE_A_IDLE))
		return 1;
	return 0;
}

/**
 * cdns_otg_set_standby - set standby mode aka slow reference clock
 * @cdns: Pointer to our controller context structure
 *
 * Returns 0 on success otherwise negative errno.
 */
int cdns_otg_set_standby(struct cdns_dev *cdns)
{
	u32 reg;
	int phy_refclk_valid = 0;

	if (!cdns_otg_standby_allowed(cdns))
		return -EPERM;
	reg = cdns_readl(&cdns->regs->OTGREFCLK);
	cdns_writel(&cdns->regs->OTGREFCLK,
	    OTGREFCLK_TYPE__OTG_STB_CLK_SWITCH_EN__SET(reg));
	/*
	 * signal from the PHY Reference Clock Control interface
	 * should fall to 0
	 */
	do {
		reg = cdns_readl(&cdns->regs->OTGSTATE);
		phy_refclk_valid = OTGSTATE_TYPE__PHY_REFCLK_VALID__READ(reg);
		if (phy_refclk_valid)
			cdns_delay(100);
	} while (phy_refclk_valid);
	return 0;
}

/**
 * cdns_otg_clear_standby - switch off standby mode aka slow reference clock
 * @cdns: Pointer to our controller context structure
 *
 * Returns 0 on success otherwise negative errno.
 */
int cdns_otg_clear_standby(struct cdns_dev *cdns)
{
	u32 reg;
	int phy_refclk_valid = 0;

	reg = cdns_readl(&cdns->regs->OTGREFCLK);
	if (!OTGREFCLK_TYPE__OTG_STB_CLK_SWITCH_EN__READ(reg))
		/* Don't try to stop clock which has been already stopped */
		return -EPERM;
	cdns_writel(&cdns->regs->OTGREFCLK,
	    OTGREFCLK_TYPE__OTG_STB_CLK_SWITCH_EN__CLR(reg));
	/*
	 * signal from the PHY Reference Clock Control interface
	 * should rise to 1
	 */
	do {
		reg = cdns_readl(&cdns->regs->OTGSTATE);
		phy_refclk_valid = OTGSTATE_TYPE__PHY_REFCLK_VALID__READ(reg);
		if (!phy_refclk_valid)
			cdns_delay(100);
	} while (!phy_refclk_valid);
	return 0;
}

/**
 * cdns_otg_mask_irq - Mask all interrupts
 * @cdns: Pointer to our controller context structure
 */
static void cdns_otg_mask_irq(struct cdns_dev *cdns)
{
	cdns_writel(&cdns->regs->OTGIEN, 0);
}

/**
 * cdns_otg_unmask_irq - Unmask id and sess_valid interrupts
 * @cdns: Pointer to our controller context structure
 */
static void cdns_otg_unmask_irq(struct cdns_dev *cdns)
{
	cdns_writel(&cdns->regs->OTGIEN,
	    OTGIEN_TYPE__OTGSESSVALID_FALL_INT_EN__MASK
	    | OTGIEN_TYPE__OTGSESSVALID_RISE_INT_EN__MASK
	    | OTGIEN_TYPE__ID_CHANGE_INT_EN__MASK
	    | OTGIEN_TYPE__OVERCURRENT_INT_EN__MASK);
}

/**
 * cdns_otg_enable_irq - enable desired interrupts
 * @cdns: Pointer to our controller context structure
 * @irq: interrupt mask
 */
void cdns_otg_enable_irq(struct cdns_dev *cdns, u32 irq)
{
	u32 reg;

	reg = cdns_readl(&cdns->regs->OTGIEN);
	cdns_writel(&cdns->regs->OTGIEN, reg | irq);
}

/**
 * cdns_otg_disable_irq - disable desired interrupts
 * @cdns: Pointer to our controller context structure
 * @irq: interrupt mask
 */
void cdns_otg_disable_irq(struct cdns_dev *cdns, u32 irq)
{
	u32 reg;

	reg = cdns_readl(&cdns->regs->OTGIEN);
	cdns_writel(&cdns->regs->OTGIEN, reg & ~irq);
}

/**
 * cdns_otg_irq - interrupt thread handler
 * @irq: interrupt number
 * @cdns_ptr: Pointer to our controller context structure
 *
 * Returns IRQ_HANDLED on success.
 */
static irqreturn_t cdns_otg_thread_irq(int irq, void *cdns_ptr)
{
	struct cdns_dev *cdns = cdns_ptr;
	unsigned long flags;

	spin_lock_irqsave(&cdns->lock, flags);
	cdns_otg_fsm_sync(cdns);
	spin_unlock_irqrestore(&cdns->lock, flags);

	return IRQ_HANDLED;
}

/**
 * cdns_otg_irq - interrupt handler
 * @irq: interrupt number
 * @cdns_ptr: Pointer to our controller context structure
 *
 * Returns IRQ_WAKE_THREAD on success otherwise IRQ_NONE.
 */
static irqreturn_t cdns_otg_irq(int irq, void *cdns_ptr)
{
	struct cdns_dev *cdns = cdns_ptr;
	irqreturn_t ret = IRQ_NONE;
	u32 reg;

	spin_lock(&cdns->lock);

	reg = cdns_readl(&cdns->regs->OTGIVECT);
	cdns->otg_int_vector = reg;
	if (reg) {
		cdns_writel(&cdns->regs->OTGIVECT, reg);
		ret = IRQ_WAKE_THREAD;
	}

	spin_unlock(&cdns->lock);

	return ret;
}

/**
 * cdns_wait_for_ready - wait for host or gadget to be ready
 * for working
 * @cdns: Pointer to our controller context structure
 * @otgsts_bit_ready: which bit should be monitored
 *
 * Returns 0 on success otherwise negative errno
 */
int cdns_wait_for_ready(struct cdns_dev *cdns, int otgsts_bit_ready)
{
	char ready = 0;
	u32 reg;

	do {
		/*
		 * TODO: it should be considered to add
		 * some timeout here and return error.
		 */
		reg = cdns_readl(&cdns->regs->OTGSTS);
		ready = (reg >> otgsts_bit_ready) & 0x0001;
		if (!ready)
			cdns_delay(100);
	} while (!ready);

	return 0;
}

/**
 * cdns_wait_for_idle - wait for host or gadget switched
 * to idle
 * @cdns: Pointer to our controller context structure
 * @otgsts_bit_ready: which bit should be monitored
 *
 * Returns 0 on success otherwise negative errno
 */
int cdns_wait_for_idle(struct cdns_dev *cdns, int otgsts_bits_idle)
{
	char not_idle = 0;
	u32 reg;

	do {
		/*
		 * TODO: it should be considered to add
		 * some timeout here and return error.
		 */
		reg = cdns_readl(&cdns->regs->OTGSTATE);
		not_idle = otgsts_bits_idle & reg;
		if (not_idle)
			cdns_delay(100);
	} while (not_idle);

	return 0;
}

/**
 * cdns_drd_start_host - start/stop host
 * @fsm: Pointer to our finite state machine
 * @on: 1 for start, 0 for stop
 *
 * Returns 0 on success otherwise negative errno
 */
static int cdns_drd_start_host(struct otg_fsm *fsm, int on)
{
	struct device *dev = usb_otg_fsm_to_dev(fsm);
	struct cdns_dev *cdns = dev_get_drvdata(dev);
	int ret;

	cdns_dbg(dev, "%s: %d\n", __func__, on);

	/* switch OTG core */
	if (on) {
		cdns_writel(&cdns->regs->OTGCMD,
		    OTGCMD_TYPE__HOST_BUS_REQ__MASK
		    | OTGCMD_TYPE__OTG_EN__MASK
		    | OTGCMD_TYPE__A_DEV_EN__MASK);

		cdns_err(cdns->dev, "Waiting for XHC...\n");
		ret = cdns_wait_for_ready(cdns, OTGSTS_TYPE__XHC_READY__SHIFT);
		if (ret)
			return ret;

		/* start the HCD */
		usb_otg_start_host(fsm, true);
	} else {
		/* stop the HCD */
		usb_otg_start_host(fsm, false);

		/* stop OTG */
		cdns_writel(&cdns->regs->OTGCMD,
		    OTGCMD_TYPE__HOST_BUS_DROP__MASK
		    | OTGCMD_TYPE__HOST_POWER_OFF__MASK
		    | OTGCMD_TYPE__DEV_BUS_DROP__MASK
		    | OTGCMD_TYPE__DEV_POWER_OFF__MASK);
		ret = cdns_wait_for_idle(cdns,
		        OTGSTATE_TYPE__HOST_OTG_STATE__MASK);
	}

	return 0;
}

/**
 * cdns_drd_start_gadget - start/stop gadget
 * @fsm: Pointer to our finite state machine
 * @on: 1 for start, 0 for stop
 *
 * Returns 0 on success otherwise negative errno
 */
static int cdns_drd_start_gadget(struct otg_fsm *fsm, int on)
{
	struct device *dev = usb_otg_fsm_to_dev(fsm);
	struct cdns_dev *cdns = dev_get_drvdata(dev);
	int ret;

	cdns_dbg(dev, "%s: %d\n", __func__, on);

	/* switch OTG core */
	if (on) {
		cdns_writel(&cdns->regs->OTGCMD,
		    OTGCMD_TYPE__DEV_BUS_REQ__MASK
		    | OTGCMD_TYPE__OTG_EN__MASK
		    | OTGCMD_TYPE__A_DEV_DIS__MASK);

		cdns_dbg(dev, "Waiting for CDNS_GADGET...\n");
		ret = cdns_wait_for_ready(cdns, OTGSTS_TYPE__DEV_READY__SHIFT);
		if (ret)
			return ret;

		/* start the UDC */
		usb_otg_start_gadget(fsm, true);
	} else {
		/* stop the UDC */
		usb_otg_start_gadget(fsm, false);

		/*
		 * driver should wait at least 10us after disabling Device
		 * before turning-off Device (DEV_BUS_DROP)
		 */
		udelay(30);

		/* stop OTG */
		cdns_writel(&cdns->regs->OTGCMD,
		    OTGCMD_TYPE__HOST_BUS_DROP__MASK
		    | OTGCMD_TYPE__HOST_POWER_OFF__MASK
		    | OTGCMD_TYPE__DEV_BUS_DROP__MASK
		    | OTGCMD_TYPE__DEV_POWER_OFF__MASK);
		ret = cdns_wait_for_idle(cdns,
		        OTGSTATE_TYPE__DEV_OTG_STATE__MASK);
	}

	return 0;
}

static struct otg_fsm_ops cdns_drd_ops = {
	.start_host = cdns_drd_start_host,
	.start_gadget = cdns_drd_start_gadget,
};

/**
 * cdns_enable_host_alternate_mode - enables host alternate mode of drdb
 * controller
 * @cdns: Pointer to our controller context structure
 */
void cdns_enable_host_alternate_mode(struct cdns_dev *cdns, int on)
{
	cdns_err(cdns->dev, "bfolta: function: %s\n", __func__);

	if (on) {
		cdns_writel(&cdns->regs->TYPEC_CFG,
		    cdns_readl(&cdns->regs->TYPEC_CFG)
		    | TYPEC_CFG_TYPE__TYPEC_ALT_MODE__MASK);
		cdns_writel(&cdns->regs->OTGCMD,
		    OTGCMD_TYPE__SS_HOST_DISABLED_SET__MASK
		    | OTGCMD_TYPE__DEV_VBUS_DEB_SHORT_SET__MASK);
	} else {
		cdns_writel(&cdns->regs->TYPEC_CFG,
		    cdns_readl(&cdns->regs->TYPEC_CFG)
		    & ~TYPEC_CFG_TYPE__TYPEC_ALT_MODE__MASK);
		cdns_writel(&cdns->regs->OTGCMD,
		    OTGCMD_TYPE__SS_HOST_DISABLED_CLR__MASK
		    | OTGCMD_TYPE__DEV_VBUS_DEB_SHORT_CLR__MASK);
	}
}

/**
 * cdns_enable_gadget_alternate_mode - enables gadget alternate mode of drdb
 * controller
 * @cdns: Pointer to our controller context structure
 */
void cdns_enable_gadget_alternate_mode(struct cdns_dev *cdns, int on)
{
	cdns_err(cdns->dev, "bfolta: function: %s\n", __func__);

	if (on) {
		cdns_writel(&cdns->regs->TYPEC_CFG,
		    cdns_readl(&cdns->regs->TYPEC_CFG)
		    | TYPEC_CFG_TYPE__TYPEC_ALT_MODE__MASK);
		cdns_writel(&cdns->regs->OTGCMD,
		    OTGCMD_TYPE__SS_PERIPH_DISABLED_SET__MASK
		    | OTGCMD_TYPE__DEV_VBUS_DEB_SHORT_SET__MASK);
	} else {
		cdns_writel(&cdns->regs->TYPEC_CFG,
		    cdns_readl(&cdns->regs->TYPEC_CFG)
		    & ~TYPEC_CFG_TYPE__TYPEC_ALT_MODE__MASK);
		cdns_writel(&cdns->regs->OTGCMD,
		    OTGCMD_TYPE__SS_PERIPH_DISABLED_CLR__MASK
		    | OTGCMD_TYPE__DEV_VBUS_DEB_SHORT_CLR__MASK);
	}
}

/**
 * cdns_drd_register - register out drd controller
 * into otg framework
 * @cdns: Pointer to our controller context structure
 *
 * Returns 0 on success otherwise negative errno
 */
static int cdns_drd_register(struct cdns_dev *cdns)
{
	int ret;

	/* register parent as DRD device with OTG core */
	cdns->fsm = usb_otg_register(cdns->dev,
	        &cdns->otg_config, cdns_usb_otg_work);
	if (IS_ERR(cdns->fsm)) {
		ret = PTR_ERR(cdns->fsm);
		if (ret == -ENOTSUPP)
			cdns_err(cdns->dev, "CONFIG_USB_OTG needed for dual-role\n");
		else
			cdns_err(cdns->dev, "Failed to register with OTG core\n");

		return ret;
	}

	/*
	 * The memory of host_req_flag should be allocated by
	 * controller driver, otherwise, hnp polling is not started.
	 */
	cdns->fsm->host_req_flag =
	    kmalloc(sizeof(*cdns->fsm->host_req_flag), GFP_KERNEL);
	if (!cdns->fsm->host_req_flag)
		return -ENOTSUPP;

	return 0;
}

/**
 * cdns_drd_init - initialize our drd controller
 * @cdns: Pointer to our controller context structure
 *
 * Returns 0 on success otherwise negative errno
 */
static int cdns_drd_init(struct cdns_dev *cdns)
{
	int ret;
	struct usb_otg_caps *otgcaps = &cdns->otg_config.otg_caps;
	unsigned long flags;
	u32 reg;

	reg = cdns_readl(&cdns->regs->OTGCAPABILITY);
	otgcaps->otg_rev = OTGCAPABILITY_TYPE__OTG2REVISION__READ(reg);

	/* Update otg capabilities by DT properties */
	ret = of_usb_update_otg_caps(cdns->dev->of_node, otgcaps);
	if (ret)
		return ret;

	cdns->otg_config.fsm_ops = &cdns_drd_ops;

	cdns_dbg(cdns->dev, "rev:0x%x\n",
	    otgcaps->otg_rev);

	ret = cdns_drd_register(cdns);
	if (ret)
		goto error0;

	/* disable all irqs */
	cdns_otg_mask_irq(cdns);
	/* clear all interrupts */
	cdns_writel(&cdns->regs->OTGIVECT, ~0);

	ret = devm_request_threaded_irq(cdns->dev, cdns->otg_irq, cdns_otg_irq,
	        cdns_otg_thread_irq,
	        IRQF_SHARED, "cdns-drd-tc", cdns);
	if (ret) {
		cdns_err(cdns->dev, "failed to request irq #%d --> %d\n",
		    cdns->otg_irq, ret);
		ret = -ENODEV;
		goto error1;
	}

	spin_lock_irqsave(&cdns->lock, flags);

	/* we need to set OTG to get events from OTG core */
	cdns_set_mode(cdns, CDNS_ROLE_OTG);

	/* Enable ID ans sess_valid event interrupt */
	cdns_otg_unmask_irq(cdns);

	spin_unlock_irqrestore(&cdns->lock, flags);

	cdns_otg_fsm_sync(cdns);
	usb_otg_sync_inputs(cdns->fsm);

	return 0;

error1:
	usb_otg_unregister(cdns->dev);
error0:
	return ret;
}

/**
 * cdns_drd_exit - unregister and clean up drd controller
 * @cdns: Pointer to our controller context structure
 */
static void cdns_drd_exit(struct cdns_dev *cdns)
{
	usb_otg_unregister(cdns->dev);
}

/**
 * cdns_core_init_mode - initialize mode of operation
 * @cdns: Pointer to our controller context structure
 *
 * Returns 0 on success otherwise negative errno
 */
static int cdns_core_init_mode(struct cdns_dev *cdns)
{
	int ret;

	switch (cdns->dr_mode) {
	case USB_DR_MODE_PERIPHERAL:
		cdns_set_mode(cdns, CDNS_ROLE_GADGET);
		break;
	case USB_DR_MODE_HOST:
		cdns_set_mode(cdns, CDNS_ROLE_HOST);
		break;
	case USB_DR_MODE_OTG:
		ret = cdns_drd_init(cdns);
		if (ret) {
			cdns_err(cdns->dev, "limiting to peripheral only\n");
			cdns->dr_mode = USB_DR_MODE_PERIPHERAL;
			cdns_set_mode(cdns, CDNS_ROLE_GADGET);
		}
		break;
	default:
		cdns_err(cdns->dev, "Unsupported mode of operation %d\n",
		    cdns->dr_mode);
		return -EINVAL;
	}

	return 0;
}

/**
 * cdns_core_exit_mode - clean up drd controller
 * @cdns: Pointer to our controller context structure
 */
static void cdns_core_exit_mode(struct cdns_dev *cdns)
{
	switch (cdns->dr_mode) {
	case USB_DR_MODE_PERIPHERAL:
		break;
	case USB_DR_MODE_HOST:
		break;
	case USB_DR_MODE_OTG:
		cdns_drd_exit(cdns);
		break;
	default:
		/* do nothing */
		break;
	}
}

/**
 * cdns_delay - delay depending on USB_SIM_SPEED_UP
 * @delay: number of delay units
 *
 * Returns no value.
 */
inline void cdns_delay(int delay)
{
#ifdef USB_SIM_SPEED_UP
	udelay(delay);
#else
	mdelay(delay);
#endif
}

/**
 * cdns_to_jiffies - returns number of jiffies
 * @m: number of time units
 *
 * Returns no value.
 */
inline unsigned long cdns_to_jiffies(const unsigned int m)
{
#ifdef USB_SIM_SPEED_UP
	return usecs_to_jiffies(m);
#else
	return msecs_to_jiffies(m);
#endif
}

/**
 * cdns_probe - bind our drd driver
 * @pdev: Pointer to Linux platform device
 *
 * Returns 0 on success otherwise negative errno
 */
static int cdns_probe(struct platform_device *pdev)
{
	struct device       *dev = &pdev->dev;
	struct resource     *res;
	struct cdns_dev     *cdns;
	int         ret;
	u32         status;
	void __iomem        *regs;
	void            *mem;

	cdns_dbg(dev, "Cadence usb driver: probe()\n");

	printk(KERN_ERR"cadencd usb probe ************()()()()\n");

	mem = devm_kzalloc(dev, sizeof(*cdns) + CDNS_ALIGN_MASK, GFP_KERNEL);
	if (!mem)
		return -ENOMEM;

	cdns = PTR_ALIGN(mem, CDNS_ALIGN_MASK + 1);
	cdns->mem = mem;
	cdns->dev = dev;

	res = platform_get_resource(pdev, IORESOURCE_IRQ, 0);
	if (!res) {
		cdns_err(dev, "missing IRQ\n");
		return -ENODEV;
	}
	cdns->otg_irq = res->start;

	/*
	 * Request memory region
	 */
	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	
	printk(KERN_ERR"res ************ %08x, %08x\n", res->start, res->end);

	regs = devm_ioremap_resource(dev, res);
	if (IS_ERR(regs)) {
		ret = PTR_ERR(regs);
		goto err0;
	}

	cdns->regs	= regs;
	cdns->regs_size	= resource_size(res);

	printk(KERN_ERR"cdns->regs ************ %08x, %08x\n", cdns->regs, cdns->regs_size);

	printk(KERN_ERR"60300008: *********** %08x\n", cdns->regs->OTGSTATE);

	status = cdns_readl(&cdns->regs->OTGSTS);

	platform_set_drvdata(pdev, cdns);
#if IS_ENABLED(CONFIG_USB_CDNS_MISC)
	cdns_drd_misc_register(cdns, res->start);
#endif
	spin_lock_init(&cdns->lock);

	cdns->strap = OTGSTS_TYPE__STRAP__READ(status);

	printk(KERN_ERR"cdns->strap ************ %08x, %08x\n", status, cdns->strap);

	if (cdns->strap == STRAP_HOST)
		cdns->dr_mode = USB_DR_MODE_HOST;
	else if (cdns->strap == STRAP_GADGET)
		cdns->dr_mode = USB_DR_MODE_PERIPHERAL;
	else if (cdns->strap == STRAP_HOST_OTG)
		cdns->dr_mode = USB_DR_MODE_OTG;
	else {
		cdns_err(dev, "No default configuration, configuring as OTG.");
		cdns->dr_mode = USB_DR_MODE_OTG;
	}

	printk(KERN_ERR"dr_mode 1111111: %d\n", cdns->dr_mode);

	ret = cdns_core_init(cdns);
	if (ret) {
		cdns_err(dev, "failed to initialize core\n");
		goto err0;
	}

	printk(KERN_ERR"dr_mode 2222222: %d\n", cdns->dr_mode);

	ret = cdns_core_init_mode(cdns);
	if (ret)
		goto err1;

	printk(KERN_ERR"dr_mode 333333333: %d\n", cdns->dr_mode);

	printk(KERN_ERR"cadencd usb probe finish ************()()()()\n");

	return 0;

err1:
	cdns_core_exit_mode(cdns);
err0:
	return ret;
}

/**
 * cdns_remove - unbind our drd driver and clean up
 * @pdev: Pointer to Linux platform device
 *
 * Returns 0 on success otherwise negative errno
 */
static int cdns_remove(struct platform_device *pdev)
{
	struct cdns_dev	*cdns = platform_get_drvdata(pdev);

	kfree(cdns->fsm->host_req_flag);
	cancel_delayed_work(&cdns->fsm->hnp_polling_work);
#if !IS_ENABLED(CONFIG_USB_CDNS_A_BIDL_ADIS_HW_TMR)
	cancel_delayed_work(&(cdns->a_bidl_adis_data.a_bidl_adis_work));
#endif

	cdns_core_exit_mode(cdns);
	return 0;
}

#ifdef CONFIG_OF
static const struct of_device_id of_cdns_match[] = {
	{ .compatible = "Cadence,usb-drdtc" },
	{ },
};
MODULE_DEVICE_TABLE(of, of_cdns_match);
#endif

static struct platform_driver cdns_driver = {
	.probe		= cdns_probe,
	.remove		= cdns_remove,
	.driver		= {
		.name	= "cdns-drd-tc",
		.of_match_table	= of_match_ptr(of_cdns_match),
	},
};

module_platform_driver(cdns_driver);

MODULE_ALIAS("platform:cdns");
MODULE_AUTHOR("Rafal Ozieblo <rafalo@cadence.com>");
MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("Cadence USB3 DRD Controller Driver");
