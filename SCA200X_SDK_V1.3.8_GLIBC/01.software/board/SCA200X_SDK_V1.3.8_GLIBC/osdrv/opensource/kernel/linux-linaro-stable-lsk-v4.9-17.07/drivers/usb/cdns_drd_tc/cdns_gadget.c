/**
 * cdns_gadget.c - Cadence USB3 device Controller Core file
 *
 * Copyright (C) 2016 Cadence Design Systems - http://www.cadence.com
 *
 * Authors: Pawel Jez <pjez@cadence.com>,
 *          Konrad Kociolek <konrad@cadence.com>
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

#include <linux/platform_device.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/interrupt.h>
#include <linux/dma-mapping.h>
#include <linux/usb/composite.h>
#include <linux/of_platform.h>
#include <linux/usb/gadget.h>
#include <linux/delay.h>
#include <uapi/asm-generic/errno-base.h>
#include <uapi/asm-generic/errno.h>
#include <linux/byteorder/generic.h>
#include <linux/firmware.h>

#include "cdns_gadget.h"
#include "debug.h"
#include "io.h"

#if IS_ENABLED(CONFIG_USB_CDNS_DRD_TC)
	#include <linux/usb/otg.h>
#endif

#define CDNS_ENABLE_STREAMS
#define CDNS_THREADED_IRQ_HANDLING

/*-------------------------------------------------------------------------*/
/* Structs declaration */

static const struct usb_ep_ops usb_ss_gadget_ep0_ops;
static const struct usb_ep_ops usb_ss_gadget_ep_ops;
static const struct usb_gadget_ops usb_ss_gadget_ops;

#define                 padding_req_num         4
struct usb_request     *padding_req[padding_req_num];//wk:workaround for isoerr
unsigned int            padding_data = 0xffffffff;
int                     padding_flag = 0;
/*-------------------------------------------------------------------------*/
/* Function declarations */

static u32 gadget_readl(struct usb_ss_dev *usb_ss,
    volatile uint32_t __iomem *reg);
static void gadget_writel(struct usb_ss_dev *usb_ss,
    volatile uint32_t __iomem *reg, u32 value);

static struct usb_request *gadget_next_request(struct list_head *list);

static void gadget_select_endpoint(struct usb_ss_dev *usb_ss, u32 ep);
static int gadget_allocate_trb_pool(struct usb_ss_endpoint *usb_ss_ep);
static void gadget_free_trb_pool(struct usb_ss_dev *usb_ss,
    struct usb_ss_endpoint *usb_ss_ep);
static void gadget_ep_stall_flush(struct usb_ss_endpoint *usb_ss_ep);
static void gadget_ep0_config(struct usb_ss_dev *usb_ss);
static void gadget_unconfig(struct usb_ss_dev *usb_ss);
static void gadget_ep0_run_transfer(struct usb_ss_dev *usb_ss,
    dma_addr_t dma_addr, unsigned length, int erdy);
static int gadget_ep_run_transfer(struct usb_ss_endpoint *usb_ss_ep);
static int gadget_get_setup_ret(struct usb_ss_dev *usb_ss,
    struct usb_ctrlrequest *ctrl_req);

/* Request functions */

static int gadget_req_ep0_set_address(struct usb_ss_dev *usb_ss,
    struct usb_ctrlrequest *ctrl_req);
static int gadget_req_ep0_get_status(struct usb_ss_dev *usb_ss,
    struct usb_ctrlrequest *ctrl_req);
static int gadget_req_ep0_handle_feature(struct usb_ss_dev *usb_ss,
    struct usb_ctrlrequest *ctrl_req, int set);
static int gadget_req_ep0_set_sel(struct usb_ss_dev *usb_ss,
    struct usb_ctrlrequest *ctrl_req);
static int gadget_req_ep0_set_isoch_delay(struct usb_ss_dev *usb_ss,
    struct usb_ctrlrequest *ctrl_req);
static int gadget_req_ep0_set_configuration(struct usb_ss_dev *usb_ss,
    struct usb_ctrlrequest *ctrl_req);

static int gadget_ep0_standard_request(struct usb_ss_dev *usb_ss,
    struct usb_ctrlrequest *ctrl_req);
static void gadget_ep0_setup_phase(struct usb_ss_dev *usb_ss);
static int gadget_check_ep_interrupt_proceed(struct usb_ss_endpoint *usb_ss_ep);
static void gadget_check_ep0_interrupt_proceed(struct usb_ss_dev *usb_ss,
    int dir);
static void gadget_check_usb_interrupt_proceed(struct usb_ss_dev *usb_ss,
    u32 usb_ists);
#ifdef CDNS_THREADED_IRQ_HANDLING
	static irqreturn_t gadget_irq_handler(int irq, void *_usb_ss);
#endif

/* Endpoint operation functions */

static int gadget_eop_ep0_enable(struct usb_ep *ep,
    const struct usb_endpoint_descriptor *desc);
static int gadget_eop_ep0_disable(struct usb_ep *ep);
static int gadget_eop_ep0_set_halt(struct usb_ep *ep, int value);
static int gadget_eop_ep0_queue(struct usb_ep *ep,
    struct usb_request *request, gfp_t gfp_flags);
static void gadget_ep_config(struct usb_ss_endpoint *usb_ss_ep, int index);
static int gadget_eop_ep_enable(struct usb_ep *ep,
    const struct usb_endpoint_descriptor *desc);
static int gadget_eop_ep_disable(struct usb_ep *ep);
static struct usb_request *gadget_eop_ep_alloc_request(struct usb_ep *ep,
    gfp_t gfp_flags);
static void gadget_eop_ep_free_request(struct usb_ep *ep,
    struct usb_request *request);
static int gadget_eop_ep_queue(struct usb_ep *ep,
    struct usb_request *request, gfp_t gfp_flags);
static int gadget_eop_ep_dequeue(struct usb_ep *ep,
    struct usb_request *request);
static int gadget_eop_ep_set_halt(struct usb_ep *ep, int value);
static int gadget_eop_ep_set_wedge(struct usb_ep *ep);

/* Gadget operation functions */

static int gadget_gop_get_frame(struct usb_gadget *gadget);
static int gadget_gop_wakeup(struct usb_gadget *gadget);
static int gadget_gop_set_selfpowered(struct usb_gadget *gadget,
    int is_selfpowered);
static int gadget_gop_pullup(struct usb_gadget *gadget, int is_on);
static int gadget_gop_udc_start(struct usb_gadget *gadget,
    struct usb_gadget_driver *driver);
static int gadget_gop_udc_stop(struct usb_gadget *gadget);

static int gadget_init_ep(struct usb_ss_dev *usb_ss);
static int gadget_init_ep0(struct usb_ss_dev *usb_ss);
static void gadget_stb_turn_off_ref_clock(struct usb_ss_dev *usb_ss);
static void gadget_stb_turn_on_ref_clock(struct usb_ss_dev *usb_ss);
static void gadget_stb_ensure_clock_on(struct usb_ss_dev *usb_ss);
static int gadget_probe(struct platform_device *pdev);
static int gadget_remove(struct platform_device *pdev);

#if !IS_ENABLED(CONFIG_USB_CDNS_MISC)
static void gadget_stb_turn_off_ref_clock(struct usb_ss_dev *usb_ss) { }
static void gadget_stb_turn_on_ref_clock(struct usb_ss_dev *usb_ss) { }
static void gadget_stb_ensure_clock_on(struct usb_ss_dev *usb_ss) { }
#endif

/**
 * gadget_readl - reads specified register
 * @usb_ss: extended gadget object
 * @reg: register address
 *
 * Returns register value
 */
static u32 gadget_readl(struct usb_ss_dev *usb_ss,
    volatile uint32_t __iomem *reg)
{
	if (IS_REG_REQUIRING_ACTIVE_REF_CLOCK(usb_ss, reg))
		gadget_stb_ensure_clock_on(usb_ss);
	return cdns_readl(reg);
}

/**
 * gadget_writel - writes specified register
 * @usb_ss: extended gadget object
 * @reg: register address
 * @value: value to write
 */
static void gadget_writel(struct usb_ss_dev *usb_ss,
    volatile uint32_t __iomem *reg, u32 value)
{
	if (IS_REG_REQUIRING_ACTIVE_REF_CLOCK(usb_ss, reg))
		gadget_stb_ensure_clock_on(usb_ss);
	cdns_writel(reg, value);
}

/**
 * gadget_next_request - returns next request from list
 * @list: list containing requests
 *
 * Retuns request or NULL if no requests in list
 */
static struct usb_request *gadget_next_request(struct list_head *list)
{
	if (list_empty(list))
		return NULL;
	return list_first_entry(list, struct usb_request, list);
}

/**
 * gadget_select_endpoint - selects endpoint
 * @usb_ss: extended gadget object
 * @ep: endpoint address
 */
static void gadget_select_endpoint(struct usb_ss_dev *usb_ss, u32 ep)
{
	if (!usb_ss || !usb_ss->regs) {
		cdns_err(usb_ss->dev, "Failed to select endpoint!\n");
		return;
	}

	gadget_writel(usb_ss, &usb_ss->regs->ep_sel, ep);
}

/**
 * gadget_allocate_trb_pool - Allocates TRB's pool for selected endpoint
 * @usb_ss_ep: extended endpoint object
 *
 * Function will return 0 on success or -ENOMEM on allocation error
 */
static int gadget_allocate_trb_pool(struct usb_ss_endpoint *usb_ss_ep)
{
	struct usb_ss_dev *usb_ss = usb_ss_ep->usb_ss;

	if (usb_ss_ep->trb_pool)
		return 0;

	usb_ss_ep->trb_pool = dma_zalloc_coherent(usb_ss->dev,
	        sizeof(struct usb_ss_trb) * USB_SS_TRBS_NUM,
	        &usb_ss_ep->trb_pool_dma, GFP_DMA);

	if (!usb_ss_ep->trb_pool) {
		cdns_err(usb_ss->dev,
		    "Failed to allocate TRB pool for endpoint %s\n",
		    usb_ss_ep->name);
		return -ENOMEM;
	}

	return 0;
}

/**
 * gadget_free_trb_pool - Frees TRB's pool for selected endpoint
 * @usb_ss_ep: extended endpoint object
 */
static void gadget_free_trb_pool(struct usb_ss_dev *usb_ss,
    struct usb_ss_endpoint *usb_ss_ep)
{
	if (usb_ss_ep->trb_pool) {
		dma_free_coherent(usb_ss->dev,
		    sizeof(struct usb_ss_trb) * USB_SS_TRBS_NUM,
		    usb_ss_ep->trb_pool,
		    usb_ss_ep->trb_pool_dma);

		usb_ss_ep->trb_pool = NULL;
	}
}

/**
 * gadget_ep_stall_flush - Stalls and flushes selected endpoint
 * @usb_ss_ep: extended endpoint object
 *
 * Endpoint must be selected before call to this function
 */
static void gadget_ep_stall_flush(struct usb_ss_endpoint *usb_ss_ep)
{
	struct usb_ss_dev *usb_ss = usb_ss_ep->usb_ss;

	gadget_writel(usb_ss, &usb_ss->regs->ep_cmd,
	    EP_CMD__DFLUSH__MASK | EP_CMD__ERDY__MASK |
	    EP_CMD__SSTALL__MASK);

	/* wait for DFLUSH cleared */
	while (gadget_readl(usb_ss,
	        &usb_ss->regs->ep_cmd) & EP_CMD__DFLUSH__MASK)
		;

	usb_ss_ep->stalled_flag = 1;
}

/**
 * gadget_ep0_config - Configures default endpoint
 * @usb_ss: extended gadget object
 *
 * Functions sets parameters: maximal packet size and enables interrupts
 */
static void gadget_ep0_config(struct usb_ss_dev *usb_ss)
{
	u32 max_packet_size = 0;

	switch (usb_ss->gadget.speed) {
	case USB_SPEED_LOW:
		max_packet_size = ENDPOINT_MAX_PACKET_SIZE_8;
		break;

	case USB_SPEED_FULL:
		max_packet_size = ENDPOINT_MAX_PACKET_SIZE_64;
		break;

	case USB_SPEED_HIGH:
		max_packet_size = ENDPOINT_MAX_PACKET_SIZE_64;
		break;

	case USB_SPEED_WIRELESS:
		max_packet_size = ENDPOINT_MAX_PACKET_SIZE_64;
		break;

	case USB_SPEED_SUPER:
		max_packet_size = ENDPOINT_MAX_PACKET_SIZE_512;
		break;

	case USB_SPEED_UNKNOWN:
	default:
		max_packet_size = ENDPOINT_MAX_PACKET_SIZE_0;
		break;
	}

	/* init ep out */
	gadget_select_endpoint(usb_ss, USB_DIR_OUT);

	gadget_writel(usb_ss, &usb_ss->regs->ep_cfg,
	    EP_CFG__ENABLE__MASK |
	    EP_CFG__MAXPKTSIZE__WRITE(max_packet_size));
	gadget_writel(usb_ss, &usb_ss->regs->ep_sts_en,
	    EP_STS_EN__SETUPEN__MASK |
	    EP_STS_EN__DESCMISEN__MASK |
	    EP_STS_EN__TRBERREN__MASK);

	/* init ep in */
	gadget_select_endpoint(usb_ss, USB_DIR_IN);

	gadget_writel(usb_ss, &usb_ss->regs->ep_cfg,
	    EP_CFG__ENABLE__MASK |
	    EP_CFG__MAXPKTSIZE__WRITE(max_packet_size));
	gadget_writel(usb_ss, &usb_ss->regs->ep_sts_en,
	    EP_STS_EN__SETUPEN__MASK |
	    EP_STS_EN__TRBERREN__MASK);
}

/**
 * gadget_unconfig - Unconfigures device controller
 * @usb_ss: extended gadget object
 */
static void gadget_unconfig(struct usb_ss_dev *usb_ss)
{
	/* RESET CONFIGURATION */
	gadget_writel(usb_ss, &usb_ss->regs->usb_conf,
	    USB_CONF__CFGRST__MASK);

	usb_ss->hw_configured_flag = 0;
}

/**
 * gadget_ep0_run_transfer - Do transfer on default endpoint hardware
 * @usb_ss: extended gadget object
 * @dma_addr: physical address where data is/will be stored
 * @length: data length
 * @erdy: set it to 1 when ERDY packet should be sent -
 *        exit from flow control state
 */
static void gadget_ep0_run_transfer(struct usb_ss_dev *usb_ss,
    dma_addr_t dma_addr, unsigned length, int erdy)
{
	gadget_stb_ensure_clock_on(usb_ss);

	usb_ss->trb_ep0[0] = TRB_SET_DATA_BUFFER_POINTER(dma_addr);
	usb_ss->trb_ep0[1] = TRB_SET_TRANSFER_LENGTH((u32)length);
	usb_ss->trb_ep0[2] = TRB_SET_CYCLE_BIT |
	    TRB_SET_INT_ON_COMPLETION | TRB_TYPE_NORMAL;

	cdns_dbg(usb_ss->dev, "DRBL(%02X)\n",
	    usb_ss->ep0_data_dir ? USB_DIR_IN : USB_DIR_OUT);

	gadget_select_endpoint(usb_ss, usb_ss->ep0_data_dir
	    ? USB_DIR_IN : USB_DIR_OUT);

	gadget_writel(usb_ss, &usb_ss->regs->ep_traddr,
	    EP_TRADDR__TRADDR__WRITE(usb_ss->trb_ep0_dma));

	gadget_writel(usb_ss, &usb_ss->regs->ep_cmd,
	    EP_CMD__DRDY__MASK |
	    (erdy ? EP_CMD__ERDY__MASK : 0));
}

/**
 * gadget_ep_run_transfer - Do transfer on no-default endpoint hardware
 * @usb_ss: extended gadget object
 */
static int gadget_ep_run_transfer(struct usb_ss_endpoint *usb_ss_ep)
{
	dma_addr_t trb_dma;
	struct usb_request *request =
	    gadget_next_request(&usb_ss_ep->request_list);
	struct usb_ss_dev *usb_ss = usb_ss_ep->usb_ss;
	int sg_iter = 0;
	struct usb_ss_trb *trb;
	u32 tdl, sid, transfer_length, ep_max_packet;
	unsigned stream_id = 0;

	if (request == NULL)
		return -EINVAL;

	if (request->num_sgs > USB_SS_TRBS_NUM)
		return -EINVAL;

	gadget_stb_ensure_clock_on(usb_ss);

	cdns_dbg(usb_ss->dev, "DRBL(%02X)\n",
	    usb_ss_ep->endpoint.desc->bEndpointAddress);

	usb_ss_ep->hw_pending_flag = 1;
	trb_dma = request->dma;
#ifdef CDNS_ENABLE_STREAMS
	stream_id = request->stream_id;
#endif
	sid = stream_id << EP_CMD__ERDY_SID__SHIFT;

	/* must allocate buffer aligned to 8 */
	if (ADDR_MODULO_8(request->dma)) {
		memcpy(usb_ss_ep->cpu_addr, request->buf, request->length);
		trb_dma = usb_ss_ep->dma_addr;
	}

	trb = usb_ss_ep->trb_pool;

	do {
		/* fill TRB */
		trb->offset0 = TRB_SET_DATA_BUFFER_POINTER(request->num_sgs == 0
		        ? trb_dma : request->sg[sg_iter].dma_address);

		trb->offset4 = TRB_SET_BURST_LENGTH(16) |
		    TRB_SET_TRANSFER_LENGTH(request->num_sgs == 0 ?
		        request->length : request->sg[sg_iter].length);

		trb->offset8 = TRB_SET_CYCLE_BIT
		    | TRB_TYPE_NORMAL
		    | sid
		    | (stream_id ? 0 : (TRB_SET_INT_ON_COMPLETION
		            | TRB_SET_INT_ON_SHORT_PACKET));

		++sg_iter;
		++trb;

	} while (sg_iter < request->num_sgs);

	/* arm transfer on selected endpoint */
	gadget_select_endpoint(usb_ss_ep->usb_ss,
	    usb_ss_ep->endpoint.desc->bEndpointAddress);

	gadget_writel(usb_ss, &usb_ss->regs->ep_traddr,
	    EP_TRADDR__TRADDR__WRITE(usb_ss_ep->trb_pool_dma));

	if (stream_id) {

		transfer_length = (usb_ss_ep->trb_pool->offset4
		        & TRB_TRANSFER_LENGTH_MASK);
		ep_max_packet = usb_ss_ep->endpoint.maxpacket;
		tdl = transfer_length / ep_max_packet
		    + ((transfer_length % ep_max_packet) ? 1 : 0);

		tdl <<= EP_CMD__TDL__SHIFT;

		gadget_writel(usb_ss, &usb_ss->regs->ep_cmd,
		    EP_CMD__DRDY__MASK | EP_CMD__STDL__MASK | tdl | sid);
		while (gadget_readl(usb_ss, &usb_ss->regs->ep_sts)
		    & EP_STS__BUFFEMPTY__MASK)
			;
		gadget_writel(usb_ss, &usb_ss->regs->ep_cmd,
		    sid | (stream_id ? EP_CMD__ERDY__MASK : 0));
	} else {
		gadget_writel(usb_ss, &usb_ss->regs->ep_cmd,
		    EP_CMD__DRDY__MASK);
		// kqiu add for debug
		if (usb_ss_ep->endpoint.desc->bEndpointAddress == 0x81) {
			unsigned ep_cmd = gadget_readl(usb_ss, &usb_ss->regs->ep_cmd);
			printk(KERN_INFO "ep_cmd = 0x%x, ep_sts = 0x%x, usb_sts = 0x%x\n", ep_cmd, gadget_readl(usb_ss, &usb_ss->regs->ep_sts),
			    gadget_readl(usb_ss, &usb_ss->regs->usb_sts));
		}
		// kqiu ends
	}

	return 0;
}

/**
 * gadget_get_setup_ret - Returns status of handling setup packet
 * Setup is handled by gadget driver
 * @usb_ss: extended gadget object
 * @ctrl_req: pointer to received setup packet
 */
static int gadget_get_setup_ret(struct usb_ss_dev *usb_ss,
    struct usb_ctrlrequest *ctrl_req)
{
	int ret;

	spin_unlock(&usb_ss->lock);
	usb_ss->setup_pending = 1;
	if (usb_ss->gadget_driver != NULL) {
		printk(KERN_ERR"gadget driver not null, 0x%08x \n", usb_ss->gadget_driver);
		if (usb_ss->gadget_driver->setup != NULL) {
			printk(KERN_ERR"setup not null: 0x%08x \n", usb_ss->gadget_driver->setup);
		}
	}

	printk(KERN_ERR"request: %02x, %02x, %04x, %04x, %04x\n", ctrl_req->bRequestType, ctrl_req->bRequest, ctrl_req->wValue,
	    ctrl_req->wIndex, ctrl_req->wLength);

	ret = usb_ss->gadget_driver->setup(&usb_ss->gadget, ctrl_req);
	usb_ss->setup_pending = 0;
	spin_lock(&usb_ss->lock);

	return ret;
}

/**
 * gadget_req_ep0_set_address - Handling of SET_ADDRESS standard USB request
 * @usb_ss: extended gadget object
 * @ctrl_req: pointer to received setup packet
 *
 * Returns 0 if success, error code on error
 */
static int gadget_req_ep0_set_address(struct usb_ss_dev *usb_ss,
    struct usb_ctrlrequest *ctrl_req)
{
	enum usb_device_state device_state = usb_ss->gadget.state;
	u32 reg;
	u32 addr;

	addr = le16_to_cpu(ctrl_req->wValue);

	if (addr > DEVICE_ADDRESS_MAX) {
		cdns_err(usb_ss->dev,
		    "Device address (%d) cannot be greater than %d\n",
		    addr, DEVICE_ADDRESS_MAX);
		return -EINVAL;
	}

	if (device_state == USB_STATE_CONFIGURED) {
		cdns_err(usb_ss->dev, "USB device already configured\n");
		return -EINVAL;
	}

	reg = gadget_readl(usb_ss, &usb_ss->regs->usb_cmd);

	gadget_writel(usb_ss, &usb_ss->regs->usb_cmd, reg
	    | USB_CMD__FADDR__WRITE(addr)
	    | USB_CMD__SET_ADDR__MASK);

	usb_gadget_set_state(&usb_ss->gadget,
	    (addr ? USB_STATE_ADDRESS : USB_STATE_DEFAULT));

	gadget_writel(usb_ss, &usb_ss->regs->ep_cmd,
	    EP_CMD__ERDY__MASK | EP_CMD__REQ_CMPL__MASK);
	return 0;
}

/**
 * gadget_req_ep0_get_status - Handling of GET_STATUS standard USB request
 * @usb_ss: extended gadget object
 * @ctrl_req: pointer to received setup packet
 *
 * Returns 0 if success, error code on error
 */
static int gadget_req_ep0_get_status(struct usb_ss_dev *usb_ss,
    struct usb_ctrlrequest *ctrl_req)
{
	u16 usb_status = 0;
	unsigned length = 2;
	u32 recip = ctrl_req->bRequestType & USB_RECIP_MASK;
	u32 reg;

	switch (recip) {

	case USB_RECIP_DEVICE:
		/* handling otg features */
		if (ctrl_req->wIndex == OTG_STS_SELECTOR) {
			length = 1;
			usb_status = usb_ss->gadget.host_request_flag;
		} else {

			reg = gadget_readl(usb_ss, &usb_ss->regs->usb_sts);

			if (reg & USB_STS__U1ENS__MASK)
				usb_status |= 1uL << USB_DEV_STAT_U1_ENABLED;

			if (reg & USB_STS__U2ENS__MASK)
				usb_status |= 1uL << USB_DEV_STAT_U2_ENABLED;

			if (usb_ss->wake_up_flag)
				usb_status |= 1uL << USB_DEVICE_REMOTE_WAKEUP;

			/* self powered */
			usb_status |= 1uL << USB_DEVICE_SELF_POWERED;
		}
		break;

	case USB_RECIP_INTERFACE:
		return gadget_get_setup_ret(usb_ss, ctrl_req);

	case USB_RECIP_ENDPOINT:
		/* check if endpoint is stalled */
		gadget_select_endpoint(usb_ss, ctrl_req->wIndex);
		if (gadget_readl(usb_ss, &usb_ss->regs->ep_sts)
		    & EP_STS__STALL__MASK)
			usb_status = 1;
		break;

	default:
		return -EINVAL;
	}

	*(u16 *)usb_ss->setup = cpu_to_le16(usb_status);

	usb_ss->actual_ep0_request = NULL;
	gadget_ep0_run_transfer(usb_ss, usb_ss->setup_dma, length, 1);
	return 0;
}

/**
 * gadget_req_ep0_handle_feature -
 * Handling of GET/SET_FEATURE standard USB request
 * @usb_ss: extended gadget object
 * @ctrl_req: pointer to received setup packet
 * @set: must be set to 1 for SET_FEATURE request
 *
 * Returns 0 if success, error code on error
 */
static int gadget_req_ep0_handle_feature(struct usb_ss_dev *usb_ss,
    struct usb_ctrlrequest *ctrl_req, int set)
{
	u32 recip = ctrl_req->bRequestType & USB_RECIP_MASK;
	struct usb_ss_endpoint *usb_ss_ep;
	u32 reg;

	switch (recip) {

	case USB_RECIP_DEVICE:

		switch (ctrl_req->wValue) {

		case USB_DEVICE_U1_ENABLE:
			if (usb_ss->gadget.state != USB_STATE_CONFIGURED)
				return -EINVAL;
			if (usb_ss->gadget.speed != USB_SPEED_SUPER)
				return -EINVAL;

			reg = gadget_readl(usb_ss, &usb_ss->regs->usb_conf);
			if (set)
				/* set U1EN */
				reg |= USB_CONF__U1EN__MASK;
			else
				/* set U1 disable */
				reg |= USB_CONF__U1DS__MASK;
			gadget_writel(usb_ss, &usb_ss->regs->usb_conf, reg);
			break;

		case USB_DEVICE_U2_ENABLE:
			if (usb_ss->gadget.state != USB_STATE_CONFIGURED)
				return -EINVAL;
			if (usb_ss->gadget.speed != USB_SPEED_SUPER)
				return -EINVAL;

			reg = gadget_readl(usb_ss, &usb_ss->regs->usb_conf);
			if (set)
				/* set U2EN */
				reg |= USB_CONF__U2EN__MASK;
			else
				/* set U2 disable */
				reg |= USB_CONF__U2DS__MASK;
			gadget_writel(usb_ss, &usb_ss->regs->usb_conf, reg);
			break;

		case USB_DEVICE_A_ALT_HNP_SUPPORT:
			break;

		case USB_DEVICE_A_HNP_SUPPORT:
			break;

		case USB_DEVICE_B_HNP_ENABLE:
			if (!usb_ss->gadget.b_hnp_enable && set)
				usb_ss->gadget.b_hnp_enable = 1;
			break;

		case USB_DEVICE_REMOTE_WAKEUP:
			usb_ss->wake_up_flag = !!set;
			break;

		default:
			return -EINVAL;

		}
		break;

	case USB_RECIP_INTERFACE:
		return gadget_get_setup_ret(usb_ss, ctrl_req);

	case USB_RECIP_ENDPOINT:
		gadget_select_endpoint(usb_ss, ctrl_req->wIndex);

		if (set) {
			/* set stall */
			gadget_writel(usb_ss, &usb_ss->regs->ep_cmd,
			    EP_CMD__SSTALL__MASK);

			/* handle non zero endpoint software endpoint */
			if (ctrl_req->wIndex & 0x7F) {
				usb_ss_ep = usb_ss->eps[CAST_EP_ADDR_TO_INDEX(
				            ctrl_req->wIndex)];
				usb_ss_ep->stalled_flag = 1;
			}
		} else {
			struct usb_request *request;

			if (ctrl_req->wIndex & 0x7F) {
				if (usb_ss->eps[CAST_EP_ADDR_TO_INDEX(
				            ctrl_req->wIndex)]->wedge_flag)
					goto jmp_wedge;
			}

			/* clear stall */
			gadget_writel(usb_ss, &usb_ss->regs->ep_cmd,
			    EP_CMD__CSTALL__MASK | EP_CMD__EPRST__MASK);
			/* wait for EPRST cleared */
			while (gadget_readl(usb_ss, &usb_ss->regs->ep_cmd)
			    & EP_CMD__EPRST__MASK)
				;

			/* handle non zero endpoint software endpoint */
			if (ctrl_req->wIndex & 0x7F) {
				usb_ss_ep = usb_ss->eps[CAST_EP_ADDR_TO_INDEX(
				            ctrl_req->wIndex)];
				usb_ss_ep->stalled_flag = 0;

				request = gadget_next_request(
				        &usb_ss_ep->request_list);
				if (request)
					gadget_ep_run_transfer(usb_ss_ep);
			}
		}
jmp_wedge:
		gadget_select_endpoint(usb_ss, USB_DIR_OUT);
		break;

	default:
		return -EINVAL;
	}

	gadget_writel(usb_ss, &usb_ss->regs->ep_cmd,
	    EP_CMD__ERDY__MASK | EP_CMD__REQ_CMPL__MASK);

	return 0;
}

/**
 * gadget_req_ep0_set_sel - Handling of SET_SEL standard USB request
 * @usb_ss: extended gadget object
 * @ctrl_req: pointer to received setup packet
 *
 * Returns 0 if success, error code on error
 */
static int gadget_req_ep0_set_sel(struct usb_ss_dev *usb_ss,
    struct usb_ctrlrequest *ctrl_req)
{
	if (usb_ss->gadget.state < USB_STATE_ADDRESS)
		return -EINVAL;

	if (ctrl_req->wLength != SET_SEL_REQ_SIZE) {
		cdns_err(usb_ss->dev, "Set SEL should be 6 bytes, got %d\n",
		    ctrl_req->wLength);
		return -EINVAL;
	}

	usb_ss->ep0_data_dir = USB_DIR_OUT;
	usb_ss->actual_ep0_request = NULL;
	gadget_ep0_run_transfer(usb_ss, usb_ss->setup_dma, 6, 1);

	return 0;
}

/**
 * gadget_req_ep0_set_isoch_delay -
 * Handling of GET_ISOCH_DELAY standard USB request
 * @usb_ss: extended gadget object
 * @ctrl_req: pointer to received setup packet
 *
 * Returns 0 if success, error code on error
 */
static int gadget_req_ep0_set_isoch_delay(struct usb_ss_dev *usb_ss,
    struct usb_ctrlrequest *ctrl_req)
{
	if (ctrl_req->wIndex || ctrl_req->wLength)
		return -EINVAL;

	usb_ss->isoch_delay = ctrl_req->wValue;
	gadget_writel(usb_ss, &usb_ss->regs->ep_cmd,
	    EP_CMD__ERDY__MASK | EP_CMD__REQ_CMPL__MASK);
	return 0;
}

/**
 * gadget_req_ep0_set_configuration -
 * Handling of SET_CONFIG standard USB request
 * @usb_ss: extended gadget object
 * @ctrl_req: pointer to received setup packet
 *
 * Returns 0 if success, 0x7FFF on deferred status stage, error code on error
 */
static int gadget_req_ep0_set_configuration(struct usb_ss_dev *usb_ss,
    struct usb_ctrlrequest *ctrl_req)
{
	enum usb_device_state device_state = usb_ss->gadget.state;
	u32 config = le16_to_cpu(ctrl_req->wValue);
	struct usb_ep *ep;
	int i, result = 0;

	switch (device_state) {

	case USB_STATE_ADDRESS:

		if (config) {
			for (i = 0; i < usb_ss->ep_nums; i++)
				gadget_ep_config(usb_ss->eps[i], i);
		}
#ifdef CDNS_THREADED_IRQ_HANDLING
		usb_ss->ep_ien = gadget_readl(usb_ss, &usb_ss->regs->ep_ien)
		    | EP_IEN__EOUTEN0__MASK | EP_IEN__EINEN0__MASK;
#endif
		result = gadget_get_setup_ret(usb_ss, ctrl_req);

		if (result != 0)
			return result;

		if (config) {

			if (!usb_ss->hw_configured_flag) {
				/* SET CONFIGURATION */
				gadget_writel(usb_ss, &usb_ss->regs->usb_conf,
				    USB_CONF__CFGSET__MASK);
				gadget_writel(usb_ss, &usb_ss->regs->ep_cmd,
				    EP_CMD__ERDY__MASK |
				    EP_CMD__REQ_CMPL__MASK);
				while (!(gadget_readl(usb_ss,
				            &usb_ss->regs->usb_sts)
				        & USB_STS__CFGSTS__MASK))
					;
				/* wait until configuration set */
				usb_ss->hw_configured_flag = 1;

				list_for_each_entry(ep,
				    &usb_ss->gadget.ep_list,
				    ep_list) {
					if (ep->enabled)
						gadget_ep_run_transfer(
						    TO_USB_SS_EP(ep));
				}
			}
		} else {
			gadget_unconfig(usb_ss);
			for (i = 0; i < usb_ss->ep_nums; i++)
				usb_ss->eps[i]->endpoint.enabled = 0;
			usb_gadget_set_state(&usb_ss->gadget,
			    USB_STATE_ADDRESS);
		}
		break;

	case USB_STATE_CONFIGURED:
		result = gadget_get_setup_ret(usb_ss, ctrl_req);
		if (!config && !result) {
			gadget_unconfig(usb_ss);
			for (i = 0; i < usb_ss->ep_nums; i++)
				usb_ss->eps[i]->endpoint.enabled = 0;
			usb_gadget_set_state(&usb_ss->gadget,
			    USB_STATE_ADDRESS);
		}
		break;

	default:
		result = -EINVAL;
	}

	return result;
}

/**
 * gadget_ep0_standard_request - Handling standard USB requests
 * @usb_ss: extended gadget object
 * @ctrl_req: pointer to received setup packet
 */
static int gadget_ep0_standard_request(struct usb_ss_dev *usb_ss,
    struct usb_ctrlrequest *ctrl_req)
{
	switch (ctrl_req->bRequest) {
	case USB_REQ_SET_ADDRESS:
		return gadget_req_ep0_set_address(usb_ss, ctrl_req);

	case USB_REQ_SET_CONFIGURATION:
		return gadget_req_ep0_set_configuration(usb_ss, ctrl_req);

	case USB_REQ_GET_STATUS:
		return gadget_req_ep0_get_status(usb_ss, ctrl_req);

	case USB_REQ_CLEAR_FEATURE:
		return gadget_req_ep0_handle_feature(usb_ss, ctrl_req, 0);

	case USB_REQ_SET_FEATURE:
		//return gadget_req_ep0_handle_feature(usb_ss, ctrl_req, 1);
		return gadget_req_ep0_handle_feature(usb_ss, ctrl_req, 0);

	case USB_REQ_SET_SEL:
		return gadget_req_ep0_set_sel(usb_ss, ctrl_req);

	case USB_REQ_SET_ISOCH_DELAY:
		return gadget_req_ep0_set_isoch_delay(usb_ss, ctrl_req);

	default:
		return gadget_get_setup_ret(usb_ss, ctrl_req);
	}
}

/**
 * gadget_ep0_setup_phase - Handling setup USB requests
 * @usb_ss: extended gadget object
 */
static void gadget_ep0_setup_phase(struct usb_ss_dev *usb_ss)
{
	int result;
	struct usb_ctrlrequest *ctrl_req =
	    (struct usb_ctrlrequest *) usb_ss->setup;

	if ((ctrl_req->bRequestType & USB_TYPE_MASK) == USB_TYPE_STANDARD)
		result = gadget_ep0_standard_request(usb_ss, ctrl_req);
	else
		result = gadget_get_setup_ret(usb_ss, ctrl_req);

	if (result != 0 && result != USB_GADGET_DELAYED_STATUS) {
		cdns_dbg(usb_ss->dev, "STALL(00) %d\n", result);

		/* set_stall on ep0 */
		gadget_select_endpoint(usb_ss, USB_DIR_OUT);
		gadget_writel(usb_ss, &usb_ss->regs->ep_cmd,
		    EP_CMD__SSTALL__MASK);
		gadget_writel(usb_ss, &usb_ss->regs->ep_cmd,
		    EP_CMD__ERDY__MASK | EP_CMD__REQ_CMPL__MASK);
		return;
	}
}

/**
 * gadget_check_ep_interrupt_proceed - Processes interrupt related to endpoint
 * @usb_ss_ep: extended endpoint object
 */
static int gadget_check_ep_interrupt_proceed(struct usb_ss_endpoint *usb_ss_ep)
{
	struct usb_ss_dev *usb_ss = usb_ss_ep->usb_ss;
	struct usb_request *request;
	u32 ep_sts_reg;

	gadget_select_endpoint(usb_ss, usb_ss_ep->endpoint.address);
	ep_sts_reg = gadget_readl(usb_ss, &usb_ss->regs->ep_sts);

	cdns_dbg(usb_ss->dev, "EP_STS: %08X EP_NUM: %d\n", ep_sts_reg, usb_ss_ep->endpoint.address);

	if (ep_sts_reg & EP_STS__TRBERR__MASK) {
		gadget_writel(usb_ss,
		    &usb_ss->regs->ep_sts, EP_STS__TRBERR__MASK);

		cdns_dbg(usb_ss->dev, "TRBERR(%02X)\n",
		    usb_ss_ep->endpoint.desc->bEndpointAddress);
	}

	if (ep_sts_reg & EP_STS__ISOERR__MASK) {

		gadget_writel(usb_ss,
		    &usb_ss->regs->ep_sts, EP_STS__ISOERR__MASK);
		cdns_dbg(usb_ss->dev, "ISOERR(%02X)\n",
		    usb_ss_ep->endpoint.desc->bEndpointAddress);
	}

	if (ep_sts_reg & EP_STS__OUTSMM__MASK) {

		gadget_writel(usb_ss, &usb_ss->regs->ep_sts,
		    EP_STS__OUTSMM__MASK);
		cdns_dbg(usb_ss->dev, "OUTSMM(%02X)\n",
		    usb_ss_ep->endpoint.desc->bEndpointAddress);
	}

	if (ep_sts_reg & EP_STS__NRDY__MASK) {

		gadget_writel(usb_ss,
		    &usb_ss->regs->ep_sts, EP_STS__NRDY__MASK);
		cdns_dbg(usb_ss->dev, "NRDY(%02X)\n",
		    usb_ss_ep->endpoint.desc->bEndpointAddress);
	}

	if (ep_sts_reg & EP_STS__SIDERR__MASK) {

		gadget_writel(usb_ss, &usb_ss->regs->ep_sts,
		    EP_STS__SIDERR__MASK);
		cdns_dbg(usb_ss->dev, "SIDERR(%02X)\n",
		    usb_ss_ep->endpoint.desc->bEndpointAddress);
	}

	if (ep_sts_reg & EP_STS__STREAMR__MASK) {

		gadget_writel(usb_ss, &usb_ss->regs->ep_sts,
		    EP_STS__STREAMR__MASK);
		cdns_dbg(usb_ss->dev, "STREAMR(%02X)\n",
		    usb_ss_ep->endpoint.desc->bEndpointAddress);
		gadget_writel(usb_ss, &usb_ss->regs->ep_cmd,
		    EP_CMD__DFLUSH__MASK);

		/* wait for DFLUSH cleared */
		while (gadget_readl(usb_ss, &usb_ss->regs->ep_cmd)
		    & EP_CMD__DFLUSH__MASK)
			;
		usb_ss_ep->hw_pending_flag = 0;
	}

	if (ep_sts_reg & EP_STS__PRIME__MASK) {

		gadget_writel(usb_ss, &usb_ss->regs->ep_sts,
		    EP_STS__PRIME__MASK);
		cdns_dbg(usb_ss->dev, "PRIME(%02X)\n",
		    usb_ss_ep->endpoint.desc->bEndpointAddress);
		usb_ss_ep->prime_flag = 1;

		/* exit if hardware transfer already started */
		if (usb_ss_ep->hw_pending_flag) {
			gadget_writel(usb_ss, &usb_ss->regs->ep_cmd,
			    EP_CMD__DFLUSH__MASK);

			/* wait for DFLUSH cleared */
			while (gadget_readl(usb_ss,
			        &usb_ss->regs->ep_cmd) & EP_CMD__DFLUSH__MASK)
				;
		}

		/* if any request queued run it! */
		if (!list_empty(&usb_ss_ep->request_list))
			gadget_ep_run_transfer(usb_ss_ep);
	}

	if ((ep_sts_reg & EP_STS__IOC__MASK)
	    || (ep_sts_reg & EP_STS__ISP__MASK)
	    || (ep_sts_reg & EP_STS__IOT__MASK)
	    || (ep_sts_reg & EP_STS__ISOERR__MASK)) {
		gadget_writel(usb_ss, &usb_ss->regs->ep_sts,
		    EP_STS__IOC__MASK |
		    EP_STS__ISP__MASK |
		    EP_STS__IOT__MASK);

		/* get just completed request */
		request = gadget_next_request(&usb_ss_ep->request_list);
		usb_gadget_unmap_request(&usb_ss->gadget, request,
		    usb_ss_ep->endpoint.desc->bEndpointAddress
		    & ENDPOINT_DIR_MASK);

		request->status = 0;
		request->actual =
		    le32_to_cpu(((u32 *) usb_ss_ep->trb_pool)[1])
		    & ACTUAL_TRANSFERRED_BYTES_MASK;

		cdns_dbg(usb_ss->dev, "IOC(%02X) %d\n",
		    usb_ss_ep->endpoint.desc->bEndpointAddress,
		    request->actual);

		list_del(&request->list);

		usb_ss_ep->hw_pending_flag = 0;
		if (request->complete) {
			spin_unlock(&usb_ss->lock);
			request->complete(&usb_ss_ep->endpoint, request);
			spin_lock(&usb_ss->lock);
		}

		/* handle deferred STALL */
		if (usb_ss_ep->stalled_flag) {
			gadget_ep_stall_flush(usb_ss_ep);
			return 0;
		}

		/* exit if hardware transfer already started */
		if (usb_ss_ep->hw_pending_flag)
			return 0;

		/* if any request queued run it! */
		if (!list_empty(&usb_ss_ep->request_list))
			gadget_ep_run_transfer(usb_ss_ep);
		else {
			//lock
			padding_flag = 1;
			//unlock
			cdns_dbg(usb_ss->dev, "request empty!\n");
		}
	}

	if (ep_sts_reg & EP_STS__DESCMIS__MASK) {

		gadget_writel(usb_ss,
		    &usb_ss->regs->ep_sts, EP_STS__DESCMIS__MASK);
		cdns_dbg(usb_ss->dev, "DESCMIS(%02X)\n",
		    usb_ss_ep->endpoint.desc->bEndpointAddress);
	}

	return 0;
}

/**
 * gadget_check_ep0_interrupt_proceed -
 * Processes interrupt related to endpoint 0
 * @usb_ss: extended gadget object
 * @dir: 1 for IN direction, 0 for OUT direction
 */
static void gadget_check_ep0_interrupt_proceed(struct usb_ss_dev *usb_ss,
    int dir)
{
	u32 ep_sts_reg;
	int i;

	gadget_select_endpoint(usb_ss, (dir ? USB_DIR_IN : USB_DIR_OUT));
	ep_sts_reg = gadget_readl(usb_ss, &usb_ss->regs->ep_sts);

	cdns_dbg(usb_ss->dev, "0: EP_STS: %08X\n", ep_sts_reg);

	if ((ep_sts_reg & EP_STS__SETUP__MASK) && (dir == USB_DIR_OUT)) {

		cdns_dbg(usb_ss->dev, "0: SETUP(%02X)\n", 0x00);

		gadget_writel(usb_ss, &usb_ss->regs->ep_sts,
		    EP_STS__SETUP__MASK |
		    EP_STS__IOC__MASK | EP_STS__ISP__MASK);

		cdns_dbg(usb_ss->dev, "0: SETUP: ");
		for (i = 0; i < 8; i++)
			cdns_dbg(usb_ss->dev, "%02X ", usb_ss->setup[i]);
		cdns_dbg(usb_ss->dev, "\n0: STATE: %d\n", usb_ss->gadget.state);
		usb_ss->ep0_data_dir = usb_ss->setup[0] & USB_DIR_IN;
		gadget_ep0_setup_phase(usb_ss);
		ep_sts_reg &= ~(EP_STS__SETUP__MASK |
		        EP_STS__IOC__MASK |
		        EP_STS__ISP__MASK);
	}

	if (ep_sts_reg & EP_STS__TRBERR__MASK) {
		gadget_writel(usb_ss,
		    &usb_ss->regs->ep_sts, EP_STS__TRBERR__MASK);
		cdns_dbg(usb_ss->dev, "0: TRBERR(%02X)\n",
		    dir ? USB_DIR_IN : USB_DIR_OUT);
	}

	if (ep_sts_reg & EP_STS__DESCMIS__MASK) {

		gadget_writel(usb_ss,
		    &usb_ss->regs->ep_sts, EP_STS__DESCMIS__MASK);

		cdns_dbg(usb_ss->dev, "0: DESCMIS(%02X)\n",
		    dir ? USB_DIR_IN : USB_DIR_OUT);

		if (dir == USB_DIR_OUT && !usb_ss->setup_pending) {
			usb_ss->ep0_data_dir = USB_DIR_OUT;
			gadget_ep0_run_transfer(usb_ss,
			    usb_ss->setup_dma, 8, 0);
		}
	}

	if ((ep_sts_reg & EP_STS__IOC__MASK)
	    || (ep_sts_reg & EP_STS__ISP__MASK)) {

		gadget_writel(usb_ss,
		    &usb_ss->regs->ep_sts, EP_STS__IOC__MASK);
		gadget_writel(usb_ss,
		    &usb_ss->regs->ep_cmd, EP_CMD__REQ_CMPL__MASK);

		if (usb_ss->actual_ep0_request) {
			usb_gadget_unmap_request(&usb_ss->gadget,
			    usb_ss->actual_ep0_request,
			    usb_ss->ep0_data_dir);

			usb_ss->actual_ep0_request->actual =
			    le32_to_cpu((usb_ss->trb_ep0)[1])
			    & ACTUAL_TRANSFERRED_BYTES_MASK;

			cdns_dbg(usb_ss->dev, "0: IOC(%02X) %d\n",
			    dir ? USB_DIR_IN : USB_DIR_OUT,
			    usb_ss->actual_ep0_request->actual);
		}

		if (usb_ss->actual_ep0_request
		    && usb_ss->actual_ep0_request->complete) {
			spin_unlock(&usb_ss->lock);
			usb_ss->actual_ep0_request->complete(usb_ss->gadget.ep0,
			    usb_ss->actual_ep0_request);
			spin_lock(&usb_ss->lock);
		}
	}
}

/**
 * gadget_check_usb_interrupt_proceed - Processes interrupt related to device
 * @usb_ss: extended gadget object
 * @usb_ists: bitmap representation of device's reported interrupts
 * (usb_ists register value)
 */
static void gadget_check_usb_interrupt_proceed(struct usb_ss_dev *usb_ss,
    u32 usb_ists)
{
	int interrupt_bit = ffs(usb_ists) - 1;
	int speed;

	cdns_dbg(usb_ss->dev, "USB interrupt detected\n");

	switch (interrupt_bit) {
	case USB_ISTS__CON2I__SHIFT:
		/* FS/HS Connection detected */
		cdns_dbg(usb_ss->dev, "[Interrupt] FS/HS Connection detected\n");
		speed = USB_STS__USBSPEED__READ(
		        gadget_readl(usb_ss, &usb_ss->regs->usb_sts));
		if (speed == USB_SPEED_WIRELESS)
			speed = USB_SPEED_SUPER;
		cdns_dbg(usb_ss->dev, "Setting speed value to: %s (%d)\n",
		    usb_speed_string(speed), speed);
		usb_ss->gadget.speed = speed;
		if (usb_ss->in_standby_mode && !usb_ss->is_connected)
			gadget_stb_turn_on_ref_clock(usb_ss);
		usb_ss->is_connected = 1;
		usb_gadget_set_state(&usb_ss->gadget, USB_STATE_POWERED);
		gadget_ep0_config(usb_ss);
		break;

	case USB_ISTS__CONI__SHIFT:
		/* SS Connection detected */
		cdns_dbg(usb_ss->dev, "[Interrupt] SS Connection detected\n");
		speed = USB_STS__USBSPEED__READ(
		        gadget_readl(usb_ss, &usb_ss->regs->usb_sts));
		if (speed == USB_SPEED_WIRELESS)
			speed = USB_SPEED_SUPER;
		cdns_dbg(usb_ss->dev, "Setting speed value to: %s (%d)\n",
		    usb_speed_string(speed), speed);
		usb_ss->gadget.speed = speed;
		if (usb_ss->in_standby_mode && !usb_ss->is_connected)
			gadget_stb_turn_on_ref_clock(usb_ss);
		usb_ss->is_connected = 1;
		usb_gadget_set_state(&usb_ss->gadget, USB_STATE_POWERED);
		gadget_ep0_config(usb_ss);
		break;

	case USB_ISTS__DIS2I__SHIFT:
	case USB_ISTS__DISI__SHIFT:
		/* SS Disconnection detected */
		cdns_dbg(usb_ss->dev,
		    "[Interrupt] Disconnection detected\n");
		if (usb_ss->gadget_driver
		    && usb_ss->gadget_driver->disconnect) {

			spin_unlock(&usb_ss->lock);
			usb_ss->gadget_driver->disconnect(&usb_ss->gadget);
			spin_lock(&usb_ss->lock);
		}
		usb_ss->gadget.speed = USB_SPEED_UNKNOWN;
		usb_gadget_set_state(&usb_ss->gadget, USB_STATE_NOTATTACHED);
		usb_ss->is_connected = 0;
		gadget_unconfig(usb_ss);
		break;

	case USB_ISTS__L2ENTI__SHIFT:
		cdns_dbg(usb_ss->dev,
		    "[Interrupt] Device suspended\n");
#if IS_ENABLED(CONFIG_USB_CDNS_DRD_TC)
		usb_ss->gadget.suspended = 1;
#endif
		break;

	case USB_ISTS__L2EXTI__SHIFT:
		/*
		 * Exit from standby mode
		 * on L2 exit (Suspend in HS/FS or SS)
		 */
		cdns_dbg(usb_ss->dev, "[Interrupt] L2 exit detected\n");
#if IS_ENABLED(CONFIG_USB_CDNS_DRD_TC)
		usb_ss->gadget.suspended = 0;
#endif
		gadget_stb_turn_on_ref_clock(usb_ss);
		break;
	case USB_ISTS__U3EXTI__SHIFT:
		/*
		 * Exit from standby mode
		 * on U3 exit (Suspend in HS/FS or SS)
		 */
		cdns_dbg(usb_ss->dev, "[Interrupt] U3 exit detected\n");
		gadget_stb_turn_on_ref_clock(usb_ss);
		break;

	/* resets cases */
	case USB_ISTS__UWRESI__SHIFT:
	case USB_ISTS__UHRESI__SHIFT:
	case USB_ISTS__U2RESI__SHIFT:
		cdns_dbg(usb_ss->dev, "[Interrupt] Reset detected\n");
		speed = USB_STS__USBSPEED__READ(
		        gadget_readl(usb_ss, &usb_ss->regs->usb_sts));
		if (speed == USB_SPEED_WIRELESS)
			speed = USB_SPEED_SUPER;
		usb_gadget_set_state(&usb_ss->gadget, USB_STATE_DEFAULT);
		usb_ss->gadget.speed = speed;
		gadget_unconfig(usb_ss);
		gadget_ep0_config(usb_ss);
		break;
	default:
		break;
	}

	/* Clear interrupt bit */
	gadget_writel(usb_ss, &usb_ss->regs->usb_ists, (1uL << interrupt_bit));
}

#ifdef CDNS_THREADED_IRQ_HANDLING
static irqreturn_t gadget_irq_handler(int irq, void *_usb_ss)
{
	struct usb_ss_dev *usb_ss = _usb_ss;

	printk(KERN_ERR"gadget_irq_handler\n");
	usb_ss->usb_ien = gadget_readl(usb_ss, &usb_ss->regs->usb_ien);
	usb_ss->ep_ien = gadget_readl(usb_ss, &usb_ss->regs->ep_ien);

	if (!gadget_readl(usb_ss, &usb_ss->regs->usb_ists)
	    && !gadget_readl(usb_ss, &usb_ss->regs->ep_ists)) {
		cdns_dbg(usb_ss->dev, "--BUBBLE INTERRUPT 0 !!!\n");
		if (gadget_readl(usb_ss, &usb_ss->regs->usb_sts) & 1uL)
			return IRQ_HANDLED;
		return IRQ_NONE;
	}

	gadget_writel(usb_ss, &usb_ss->regs->usb_ien, 0);
	gadget_writel(usb_ss, &usb_ss->regs->ep_ien, 0);

	gadget_readl(usb_ss, &usb_ss->regs->dma_axi_ctrl);
	return IRQ_WAKE_THREAD;
}
#endif

/**
 * gadget_irq_handler_thread - irq line interrupt handler
 * @irq: interrupt line number
 * @_usb_ss: pointer to extended gadget object
 *
 * Returns IRQ_HANDLED when interrupt raised by USBSS_DEV,
 * IRQ_NONE when interrupt raised by other device connected
 * to the irq line
 */
static irqreturn_t gadget_irq_handler_thread(int irq, void *_usb_ss)
{
	struct usb_ss_dev *usb_ss = _usb_ss;
	u32 reg;
	enum irqreturn ret = IRQ_NONE;
	unsigned long flags;

	printk(KERN_ERR"gadget_irq_handler_thread\n");
	spin_lock_irqsave(&usb_ss->lock, flags);

	/* check USB device interrupt */
	reg = gadget_readl(usb_ss, &usb_ss->regs->usb_ists);
	if (reg) {
		cdns_dbg(usb_ss->dev, "usb_ists: %08X\n", reg);
		gadget_check_usb_interrupt_proceed(usb_ss, reg);
		ret = IRQ_HANDLED;
	}

	/* check endpoint interrupt */
	reg = gadget_readl(usb_ss, &usb_ss->regs->ep_ists);
	if (reg != 0)
		cdns_dbg(usb_ss->dev, "ep_ists: %08X\n", reg);

	if (reg == 0) {
		if (gadget_readl(usb_ss, &usb_ss->regs->usb_sts) & 1uL)
			ret = IRQ_HANDLED;
		goto irqend;
	}

	/* handle default endpoint OUT */
	if (reg & EP_ISTS__EOUT0__MASK) {
		gadget_check_ep0_interrupt_proceed(usb_ss, 0);
		ret = IRQ_HANDLED;
	}

	/* handle default endpoint IN */
	if (reg & EP_ISTS__EIN0__MASK) {
		gadget_check_ep0_interrupt_proceed(usb_ss, 1);
		ret = IRQ_HANDLED;
	}

	/* check if interrupt from non default endpoint, if no exit */
	reg &= ~(EP_ISTS__EOUT0__MASK | EP_ISTS__EIN0__MASK);
	if (!reg)
		goto irqend;

	do {
		unsigned bit_pos = ffs(reg);
		u32 bit_mask = 1 << (bit_pos - 1);

		cdns_dbg(usb_ss->dev, "Interrupt on index: %d bitmask %08X\n",
		    CAST_EP_REG_POS_TO_INDEX(bit_pos), bit_mask);
		gadget_check_ep_interrupt_proceed(
		    usb_ss->eps[CAST_EP_REG_POS_TO_INDEX(bit_pos)]);
		reg &= ~bit_mask;
		ret = IRQ_HANDLED;//wkwk add
	} while (reg);

irqend:

	spin_unlock_irqrestore(&usb_ss->lock, flags);
#ifdef CDNS_THREADED_IRQ_HANDLING
	local_irq_save(flags);
	gadget_writel(usb_ss, &usb_ss->regs->usb_ien, usb_ss->usb_ien);
	gadget_writel(usb_ss, &usb_ss->regs->ep_ien, usb_ss->ep_ien);
	local_irq_restore(flags);
#endif
	return ret;
}

/**
 * gadget_eop_ep0_enable
 * Function shouldn't be called by gadget driver,
 * endpoint 0 is allways active
 */
static int gadget_eop_ep0_enable(struct usb_ep *ep,
    const struct usb_endpoint_descriptor *desc)
{
	return -EINVAL;
}

/**
 * gadget_eop_ep0_disable
 * Function shouldn't be called by gadget driver,
 * endpoint 0 is allways active
 */
static int gadget_eop_ep0_disable(struct usb_ep *ep)
{
	return -EINVAL;
}

/**
 * gadget_eop_ep0_set_halt
 * @ep: pointer to endpoint zero object
 * @value: 1 for set stall, 0 for clear stall
 *
 * Returns 0
 */
static int gadget_eop_ep0_set_halt(struct usb_ep *ep, int value)
{
	/* TODO */
	return 0;
}

/**
 * gadget_eop_ep0_queue Transfer data on endpoint zero
 * @ep: pointer to endpoint zero object
 * @request: pointer to request object
 * @gfp_flags: gfp flags
 *
 * Returns 0 on success, error code elsewhere
 */
static int gadget_eop_ep0_queue(struct usb_ep *ep,
    struct usb_request *request, gfp_t gfp_flags)
{
	int ret;
	unsigned long flags;
	int erdy_sent = 0;
	/* get extended endpoint */
	struct usb_ss_endpoint *usb_ss_ep =
	    TO_USB_SS_EP(ep);
	struct usb_ss_dev *usb_ss = usb_ss_ep->usb_ss;

	cdns_dbg(usb_ss->dev, "QUEUE(%02X) %d\n",
	    usb_ss->ep0_data_dir ? USB_DIR_IN : USB_DIR_OUT,
	    request->length);

	/* send STATUS stage */
	if (request->length == 0 && request->zero == 0) {
		spin_lock_irqsave(&usb_ss->lock, flags);
		gadget_select_endpoint(usb_ss, USB_DIR_OUT);
		if (!usb_ss->hw_configured_flag) {

			gadget_writel(usb_ss, &usb_ss->regs->usb_conf,
			    USB_CONF__CFGSET__MASK); /* SET CONFIGURATION */
			gadget_writel(usb_ss, &usb_ss->regs->ep_cmd,
			    EP_CMD__ERDY__MASK | EP_CMD__REQ_CMPL__MASK);
			while (!(gadget_readl(usb_ss, &usb_ss->regs->usb_sts)
			        & USB_STS__CFGSTS__MASK))
				;
			/* wait until configuration set */
			erdy_sent = 1;
			usb_ss->hw_configured_flag = 1;

			list_for_each_entry(ep,
			    &usb_ss->gadget.ep_list,
			    ep_list) {

				if (ep->enabled)
					gadget_ep_run_transfer(
					    TO_USB_SS_EP(ep));
			}
		}
		if (!erdy_sent)
			gadget_writel(usb_ss, &usb_ss->regs->ep_cmd,
			    EP_CMD__ERDY__MASK | EP_CMD__REQ_CMPL__MASK);
		if (request->complete)
			request->complete(usb_ss->gadget.ep0, request);
		spin_unlock_irqrestore(&usb_ss->lock, flags);
		return 0;
	}

	spin_lock_irqsave(&usb_ss->lock, flags);
	ret = usb_gadget_map_request(&usb_ss->gadget, request,
	        usb_ss->ep0_data_dir);
	if (ret) {
		cdns_err(usb_ss->dev, "failed to map request\n");
		return -EINVAL;
	}

	usb_ss->actual_ep0_request = request;
	gadget_ep0_run_transfer(usb_ss, request->dma, request->length, 1);

	spin_unlock_irqrestore(&usb_ss->lock, flags);

	return 0;
}

/**
 * gadget_ep_config Configure hardware endpoint
 * @usb_ss_ep: extended endpoint object
 * @desc: endpoint descriptor
 * @comp_desc: endpoint companion descriptor
 */
static void gadget_ep_config(struct usb_ss_endpoint *usb_ss_ep, int index)
{
	struct usb_ss_dev *usb_ss = usb_ss_ep->usb_ss;
	u32 ep_cfg = 0;
	u32 max_packet_size = 0;
	u32 endpointAddress = (u32)CAST_INDEX_TO_EP_ADDR(index);
	u32 interrupt_mask = 0;

	usb_ss_ep->endpoint.address = endpointAddress;
	if (usb_ss_ep->is_iso_flag) {
		ep_cfg = EP_CFG__EPTYPE__WRITE(USB_ENDPOINT_XFER_ISOC);
		usb_ss_ep->is_iso_flag = 1;
		interrupt_mask = ENABLE_ALL_INTERRUPTS_MASK;
	} else {
		ep_cfg = EP_CFG__EPTYPE__WRITE(USB_ENDPOINT_XFER_BULK);
#ifdef CDNS_ENABLE_STREAMS
		interrupt_mask = EP_STS_EN__IOTEN__MASK |
		    EP_STS_EN__PRIMEEN__MASK |
		    EP_STS_EN__SIDERREN__MASK |
		    EP_STS_EN__STREAMREN__MASK;
#endif
	}

	switch (usb_ss->gadget.speed) {
	case USB_SPEED_LOW:
		max_packet_size = ENDPOINT_MAX_PACKET_SIZE_8;
		break;

	case USB_SPEED_FULL:
		max_packet_size = (usb_ss_ep->is_iso_flag ?
		        ENDPOINT_MAX_PACKET_SIZE_1023 :
		        ENDPOINT_MAX_PACKET_SIZE_64);
		break;

	case USB_SPEED_HIGH:
		max_packet_size = (usb_ss_ep->is_iso_flag ?
		        ENDPOINT_MAX_PACKET_SIZE_1024 :
		        ENDPOINT_MAX_PACKET_SIZE_512);
		if(usb_ss_ep->is_iso_flag)
			ep_cfg |= EP_CFG__MAXMULT__WRITE(2);
		break;

	case USB_SPEED_WIRELESS:
		max_packet_size = ENDPOINT_MAX_PACKET_SIZE_512;
		break;

	case USB_SPEED_SUPER:
		max_packet_size = ENDPOINT_MAX_PACKET_SIZE_1024;
		if(usb_ss_ep->is_iso_flag)
			ep_cfg |= EP_CFG__MAXMULT__WRITE(2);
		break;

	case USB_SPEED_UNKNOWN:
	default:
		max_packet_size = ENDPOINT_MAX_PACKET_SIZE_0;
		break;
	}

	ep_cfg |= EP_CFG__MAXPKTSIZE__WRITE(max_packet_size);

	if (usb_ss_ep->is_iso_flag) {
		ep_cfg |= EP_CFG__BUFFERING__WRITE(6);
		ep_cfg |= EP_CFG__MAXBURST__WRITE(1);
	} else {
		ep_cfg |= EP_CFG__BUFFERING__WRITE(3);
		ep_cfg |= EP_CFG__MAXBURST__WRITE(15);
#ifdef CDNS_ENABLE_STREAMS
		ep_cfg |= EP_CFG__STREAM_EN__MASK;
#endif
	}

	ep_cfg |= EP_CFG__ENABLE__MASK;

	gadget_select_endpoint(usb_ss, endpointAddress);
	gadget_writel(usb_ss, &usb_ss->regs->ep_cfg, ep_cfg);
	gadget_writel(usb_ss, &usb_ss->regs->ep_sts_en,
	    EP_STS_EN__TRBERREN__MASK | interrupt_mask);

	/* enable interrupt for selected endpoint */
	ep_cfg = gadget_readl(usb_ss, &usb_ss->regs->ep_ien);
	ep_cfg |= CAST_EP_ADDR_TO_BIT_POS(endpointAddress);
	gadget_writel(usb_ss, &usb_ss->regs->ep_ien, ep_cfg);
	usb_ss_ep->stalled_flag = 0;
}

/**
 * gadget_eop_ep_enable Enable endpoint
 * @ep: endpoint object
 * @desc: endpoint descriptor
 *
 * Returns 0 on success, error code elsewhere
 */
static int gadget_eop_ep_enable(struct usb_ep *ep,
    const struct usb_endpoint_descriptor *desc)
{
	struct usb_ss_endpoint *usb_ss_ep;
	struct usb_ss_dev *usb_ss;
	unsigned long flags;
	int ret;

	usb_ss_ep = TO_USB_SS_EP(ep);
	usb_ss = usb_ss_ep->usb_ss;

	if (!ep || !desc || desc->bDescriptorType != USB_DT_ENDPOINT) {
		cdns_err(usb_ss->dev, "usb-ss: invalid parameters\n");
		return -EINVAL;
	}

	if (!desc->wMaxPacketSize) {
		cdns_err(usb_ss->dev, "usb-ss: missing wMaxPacketSize\n");
		return -EINVAL;
	}

	ret = gadget_allocate_trb_pool(usb_ss_ep);
	if (ret)
		return ret;

	if (!usb_ss_ep->cpu_addr) {
		usb_ss_ep->cpu_addr = dma_alloc_coherent(usb_ss->dev,
		        ENDPOINT_CPU_ADDR_ALLOC_SIZE,
		        &usb_ss_ep->dma_addr, GFP_DMA);

		if (!usb_ss_ep->cpu_addr) {
			gadget_free_trb_pool(usb_ss, usb_ss_ep);
			return -ENOMEM;
		}
	}

	spin_lock_irqsave(&usb_ss->lock, flags);
	cdns_dbg(usb_ss->dev, "Enabling endpoint: %s\n", ep->name);
	ep->enabled = 1;
	usb_ss_ep->hw_pending_flag = 0;
	usb_ss_ep->endpoint.desc = desc;
	spin_unlock_irqrestore(&usb_ss->lock, flags);

	return 0;
}

/**
 * gadget_eop_ep_disable Disable endpoint
 * @ep: endpoint object
 *
 * Returns 0 on success, error code elsewhere
 */
static int gadget_eop_ep_disable(struct usb_ep *ep)
{
	struct usb_ss_endpoint *usb_ss_ep;
	struct usb_ss_dev *usb_ss;
	unsigned long flags;
	int ret = 0;
	struct usb_request *request;

	if (!ep) {
		pr_debug("usb-ss: invalid parameters\n");
		return -EINVAL;
	}

	usb_ss_ep = TO_USB_SS_EP(ep);
	usb_ss = usb_ss_ep->usb_ss;

	spin_lock_irqsave(&usb_ss->lock, flags);
	cdns_dbg(usb_ss->dev,
	    "Disabling endpoint: %s\n", ep->name);
	gadget_select_endpoint(usb_ss, ep->desc->bEndpointAddress);
	gadget_writel(usb_ss, &usb_ss->regs->ep_cmd,
	    EP_CMD__EPRST__MASK);
	while (gadget_readl(usb_ss,
	        &usb_ss->regs->ep_cmd) & EP_CMD__EPRST__MASK)
		;

	while (!list_empty(&usb_ss_ep->request_list)) {

		request = gadget_next_request(&usb_ss_ep->request_list);
		usb_gadget_unmap_request(&usb_ss->gadget, request,
		    ep->desc->bEndpointAddress & USB_DIR_IN);
		request->status = -ESHUTDOWN;
		list_del(&request->list);
		spin_unlock(&usb_ss->lock);
		if(NULL != request->complete)
			usb_gadget_giveback_request(ep, request);
		spin_lock(&usb_ss->lock);
	}

	ep->desc = NULL;
	ep->enabled = 0;

	spin_unlock_irqrestore(&usb_ss->lock, flags);

	if (usb_ss_ep->cpu_addr) {
		dma_free_coherent(usb_ss->dev,
		    ENDPOINT_CPU_ADDR_ALLOC_SIZE,
		    usb_ss_ep->cpu_addr,
		    usb_ss_ep->dma_addr);

		usb_ss_ep->cpu_addr = NULL;
	}

	gadget_free_trb_pool(usb_ss, usb_ss_ep);

	return ret;
}

/**
 * gadget_eop_ep_alloc_request Allocates request
 * @ep: endpoint object associated with request
 * @gfp_flags: gfp flags
 *
 * Returns allocated request address, NULL on allocation error
 */
static struct usb_request *gadget_eop_ep_alloc_request(struct usb_ep *ep,
    gfp_t gfp_flags)
{
	struct usb_request *request;

	request = kzalloc(sizeof(struct usb_request), gfp_flags);
	if (!request)
		return NULL;

	return request;
}

/**
 * gadget_eop_ep_free_request Free memory occupied by request
 * @ep: endpoint object associated with request
 * @request: request to free memory
 */
static void gadget_eop_ep_free_request(struct usb_ep *ep,
    struct usb_request *request)
{
	kfree(request);
}

/**
 * gadget_eop_ep_queue Transfer data on endpoint
 * @ep: endpoint object
 * @request: request object
 * @gfp_flags: gfp flags
 *
 * Returns 0 on success, error code elsewhere
 */
static int gadget_eop_ep_queue(struct usb_ep *ep,
    struct usb_request *request, gfp_t gfp_flags)
{
	struct usb_ss_endpoint *usb_ss_ep =
	    TO_USB_SS_EP(ep);
	struct usb_ss_dev *usb_ss = usb_ss_ep->usb_ss;
	unsigned long flags;
	int ret = 0;
	int empty_list = 0;
	int i;

	spin_lock_irqsave(&usb_ss->lock, flags);

	request->actual = 0;
	request->status = -EINPROGRESS;

	cdns_dbg(usb_ss->dev,
	    "Queuing endpoint: %s\n", usb_ss_ep->name);

	cdns_dbg(usb_ss->dev, "QUEUE(%02X) %d\n",
	    ep->desc->bEndpointAddress, request->length);

	ret = usb_gadget_map_request(&usb_ss->gadget, request,
	        ep->desc->bEndpointAddress & USB_DIR_IN);

	if (ret) {
		spin_unlock_irqrestore(&usb_ss->lock, flags);
		return ret;
	}

	empty_list = list_empty(&usb_ss_ep->request_list);

	//lock
	if(1 == padding_flag) {
		//workaround for isoc error
		for(i = 0; i < padding_req_num; ++i) {
			if (padding_req[i]) {
				list_add_tail(&padding_req[i]->list, &usb_ss_ep->request_list);
			}
		}
		padding_flag = 0;
	}
	//unlock

	list_add_tail(&request->list, &usb_ss_ep->request_list);

	if (!usb_ss->hw_configured_flag) {
		spin_unlock_irqrestore(&usb_ss->lock, flags);
		return 0;
	}

	if (empty_list) {
		u32 ep_sts_reg;

		gadget_select_endpoint(usb_ss, usb_ss_ep->endpoint.address);
		ep_sts_reg = gadget_readl(usb_ss, &usb_ss->regs->ep_sts);
		if (ep_sts_reg & EP_STS__ISOERR__MASK) {

			gadget_writel(usb_ss,
			    &usb_ss->regs->ep_sts, EP_STS__ISOERR__MASK);
			cdns_dbg(usb_ss->dev, "start ISOERR(%02X)\n",
			    usb_ss_ep->endpoint.desc->bEndpointAddress);
		}

		if (!usb_ss_ep->stalled_flag) {
#ifdef CDNS_ENABLE_STREAMS
			if (!request->stream_id ||
			    (request->stream_id && usb_ss_ep->prime_flag))
				gadget_ep_run_transfer(usb_ss_ep);
#else
			printk(KERN_INFO "usb xmit starts\n");
			gadget_ep_run_transfer(usb_ss_ep);
#endif
		} else {
			cdns_dbg(usb_ss->dev, "EP(%02X) STALLED!\n",
			    ep->desc->bEndpointAddress);

		}
	}
	spin_unlock_irqrestore(&usb_ss->lock, flags);

	return ret;
}

/**
 * gadget_eop_ep_dequeue Remove request from transfer queue
 * @ep: endpoint object associated with request
 * @request: request object
 *
 * Returns 0 on success, error code elsewhere
 */
static int gadget_eop_ep_dequeue(struct usb_ep *ep,
    struct usb_request *request)
{
	struct usb_ss_endpoint *usb_ss_ep =
	    TO_USB_SS_EP(ep);
	struct usb_ss_dev *usb_ss = usb_ss_ep->usb_ss;
	unsigned long flags;

	spin_lock_irqsave(&usb_ss->lock, flags);
	cdns_dbg(usb_ss->dev, "DEQUEUE(%02X) %d\n",
	    ep->address, request->length);
	usb_gadget_unmap_request(&usb_ss->gadget, request,
	    ep->address & USB_DIR_IN);
	request->status = -ECONNRESET;

	if (ep->address)
		list_del(&request->list);

	if (request->complete) {
		spin_unlock(&usb_ss->lock);
		request->complete(ep, request);
		spin_lock(&usb_ss->lock);
	}

	spin_unlock_irqrestore(&usb_ss->lock, flags);
	return 0;
}

/**
 * gadget_eop_ep_set_halt Sets/clears stall on selected endpoint
 * @ep: endpoint object to set/clear stall on
 * @value: 1 for set stall, 0 for clear stall
 *
 * Returns 0 on success, error code elsewhere
 */
static int gadget_eop_ep_set_halt(struct usb_ep *ep, int value)
{
	struct usb_ss_endpoint *usb_ss_ep =
	    TO_USB_SS_EP(ep);
	struct usb_ss_dev *usb_ss = usb_ss_ep->usb_ss;
	unsigned long flags;

	/* return error when endpoint disabled */
	if (!ep->enabled)
		return -EPERM;

	/* if actual transfer is pending defer setting stall on this endpoint */
	if (usb_ss_ep->hw_pending_flag && value) {
		usb_ss_ep->stalled_flag = 1;
		return 0;
	}

	cdns_dbg(usb_ss->dev, "HALT(%02X) %d\n", ep->address, value);

	spin_lock_irqsave(&usb_ss->lock, flags);

	gadget_select_endpoint(usb_ss, ep->desc->bEndpointAddress);
	if (value) {
		gadget_ep_stall_flush(usb_ss_ep);
	} else {
		/*
		 * TODO:
		 * epp->wedgeFlag = 0;
		 */
		usb_ss_ep->wedge_flag = 0;
		gadget_writel(usb_ss, &usb_ss->regs->ep_cmd,
		    EP_CMD__CSTALL__MASK | EP_CMD__EPRST__MASK);
		/* wait for EPRST cleared */
		while (gadget_readl(usb_ss,
		        &usb_ss->regs->ep_cmd) & EP_CMD__EPRST__MASK)
			;
		usb_ss_ep->stalled_flag = 0;
	}
	usb_ss_ep->hw_pending_flag = 0;

	spin_unlock_irqrestore(&usb_ss->lock, flags);

	return 0;
}

/**
 * gadget_eop_ep_set_wedge Set wedge on selected endpoint
 * @ep: endpoint object
 *
 * Returns 0 on success, error code elsewhere
 */
static int gadget_eop_ep_set_wedge(struct usb_ep *ep)
{
	struct usb_ss_endpoint *usb_ss_ep = TO_USB_SS_EP(ep);
	struct usb_ss_dev *usb_ss = usb_ss_ep->usb_ss;

	cdns_dbg(usb_ss->dev, "WEDGE(%02X)\n", ep->address);
	gadget_eop_ep_set_halt(ep, 1);
	usb_ss_ep->wedge_flag = 1;
	return 0;
}

static const struct usb_ep_ops usb_ss_gadget_ep0_ops = {
	.enable = gadget_eop_ep0_enable,
	.disable = gadget_eop_ep0_disable,
	.alloc_request = gadget_eop_ep_alloc_request,
	.free_request = gadget_eop_ep_free_request,
	.queue = gadget_eop_ep0_queue,
	.dequeue = gadget_eop_ep_dequeue,
	.set_halt = gadget_eop_ep0_set_halt,
	.set_wedge = gadget_eop_ep_set_wedge,
};

static const struct usb_ep_ops usb_ss_gadget_ep_ops = {
	.enable = gadget_eop_ep_enable,
	.disable = gadget_eop_ep_disable,
	.alloc_request = gadget_eop_ep_alloc_request,
	.free_request = gadget_eop_ep_free_request,
	.queue = gadget_eop_ep_queue,
	.dequeue = gadget_eop_ep_dequeue,
	.set_halt = gadget_eop_ep_set_halt,
	.set_wedge = gadget_eop_ep_set_wedge,
};

/**
 * gadget_gop_get_frame Returns number of actual ITP frame
 * @gadget: gadget object
 *
 * Returns number of actual ITP frame
 */
static int gadget_gop_get_frame(struct usb_gadget *gadget)
{
	struct usb_ss_dev *usb_ss = TO_USB_SS(gadget);

	cdns_dbg(usb_ss->dev, "gadget_gop_get_frame\n");
	return gadget_readl(usb_ss, &usb_ss->regs->usb_iptn);
}

static int gadget_gop_wakeup(struct usb_gadget *gadget)
{
	struct usb_ss_dev *usb_ss = TO_USB_SS(gadget);

	cdns_dbg(usb_ss->dev, "gadget_gop_wakeup\n");
	return 0;
}

static int gadget_gop_set_selfpowered(struct usb_gadget *gadget,
    int is_selfpowered)
{
	struct usb_ss_dev *usb_ss = TO_USB_SS(gadget);

	cdns_dbg(usb_ss->dev, "gadget_gop_set_selfpowered: %d\n",
	    is_selfpowered);
	return 0;
}

static int gadget_gop_pullup(struct usb_gadget *gadget, int is_on)
{
	struct usb_ss_dev *usb_ss = TO_USB_SS(gadget);

	cdns_dbg(usb_ss->dev, "gadget_gop_pullup: %d\n", is_on);
	return 0;
}

/**
 * gadget_gop_udc_start Gadget start
 * @gadget: gadget object
 * @driver: driver which operates on this gadget
 *
 * Returns 0 on success, error code elsewhere
 */
static int gadget_gop_udc_start(struct usb_gadget *gadget,
    struct usb_gadget_driver *driver)
{
	struct usb_ss_dev *usb_ss = TO_USB_SS(gadget);
	struct platform_device *pdev = to_platform_device(usb_ss->dev);
	int irq = platform_get_irq(pdev, 0);
	int ret;
	unsigned long flags;

	printk(KERN_ERR"gadget_gop_udc_start======\n");
	printk(KERN_ERR"gadget_gop_udc_start=======\n");
	printk(KERN_ERR"gadget_gop_udc_start========\n");
	printk(KERN_ERR"gadget_gop_udc_start=========\n");
	cdns_dbg(usb_ss->dev, "Requesting interrupt\n");
#ifdef CDNS_THREADED_IRQ_HANDLING
	printk(KERN_ERR"thread irq handler\n");
	ret = devm_request_threaded_irq(usb_ss->dev, irq, gadget_irq_handler,
	        gadget_irq_handler_thread, IRQF_SHARED, DRV_NAME, usb_ss);
#else
	printk(KERN_ERR"normal irq handler\n");
	ret = devm_request_irq(usb_ss->dev, irq, gadget_irq_handler_thread,
	        IRQF_SHARED, DRV_NAME, usb_ss);
#endif

	if (ret != 0) {
		cdns_err(usb_ss->dev, "cannot request irq %d err %d\n", irq,
		    ret);
		return -ENODEV;
	}

	cdns_dbg(usb_ss->dev, "Interrupt request done\n");
	if (usb_ss->gadget_driver) {
		cdns_err(usb_ss->dev, "%s is already bound to %s\n",
		    usb_ss->gadget.name,
		    usb_ss->gadget_driver->driver.name);
		return -EBUSY;
	}

	usb_ss->gadget_driver = driver;

	spin_lock_irqsave(&usb_ss->lock, flags);

	/* configure endpoint 0 hardware */
	gadget_ep0_config(usb_ss);

	/* enable interrupts for endpoint 0 (in and out) */
	gadget_writel(usb_ss, &usb_ss->regs->ep_ien,
	    EP_IEN__EOUTEN0__MASK | EP_IEN__EINEN0__MASK);

	/* enable interrupt for device */
	gadget_writel(usb_ss, &usb_ss->regs->usb_ien,
	    USB_IEN__U2RESIEN__MASK | USB_ISTS__DIS2I__MASK
	    | USB_IEN__CON2IEN__MASK
	    | USB_IEN__UHRESIEN__MASK
	    | USB_IEN__UWRESIEN__MASK
	    | USB_IEN__DISIEN__MASK
	    | USB_IEN__CONIEN__MASK
	    | USB_IEN__U3EXTIEN__MASK
	    | USB_IEN__L2ENTIEN__MASK
	    | USB_IEN__L2EXTIEN__MASK);

	gadget_writel(usb_ss, &usb_ss->regs->usb_conf,
	    USB_CONF__L1DS__MASK
#ifdef ENFORCE_USB3DIS
	    | USB_CONF__USB3DIS__MASK
#endif
#ifdef ENFORCE_CLK2OFFEN
	);
#else
	    | USB_CONF__CLK2OFFDS__MASK);
#endif

	gadget_writel(usb_ss, &usb_ss->regs->usb_conf,
	    USB_CONF__U2DS__MASK |
	    USB_CONF__U1DS__MASK |
	    USB_CONF__DEVEN__MASK
	    /*
	     * TODO:
	     * | USB_CONF__L1EN__MASK
	     */
	);

	gadget_writel(usb_ss, &usb_ss->regs->dbg_link1,
	    DBG_LINK1__LFPS_MIN_GEN_U1_EXIT_SET__MASK |
	    DBG_LINK1__LFPS_MIN_GEN_U1_EXIT__WRITE(0x3C));
	spin_unlock_irqrestore(&usb_ss->lock, flags);

	printk(KERN_ERR"00 - 0C: %08x, %08x, %08x, %08x\n",
	    usb_ss->regs->usb_conf,
	    usb_ss->regs->usb_sts,
	    usb_ss->regs->usb_cmd,
	    usb_ss->regs->usb_iptn);

	printk(KERN_ERR"10 - 1C: %08x, %08x, %08x, %08x\n",
	    usb_ss->regs->usb_lpm,
	    usb_ss->regs->usb_ien,
	    usb_ss->regs->usb_ists,
	    usb_ss->regs->ep_sel);

	printk(KERN_ERR"20 - 2C: %08x, %08x, %08x, %08x\n",
	    usb_ss->regs->ep_traddr,
	    usb_ss->regs->ep_cfg,
	    usb_ss->regs->ep_cmd,
	    usb_ss->regs->ep_sts);

	printk(KERN_ERR"30 - 3C: %08x, %08x, %08x, %08x\n",
	    usb_ss->regs->ep_sts_sid,
	    usb_ss->regs->ep_sts_en,
	    usb_ss->regs->drbl,
	    usb_ss->regs->ep_ien);

	printk(KERN_ERR"40 - 4C: %08x, %08x, %08x, %08x\n",
	    usb_ss->regs->ep_ists,
	    usb_ss->regs->usb_pwr,
	    usb_ss->regs->usb_conf2,
	    usb_ss->regs->usb_cap1);

	printk(KERN_ERR"50 - 5C: %08x, %08x, %08x, %08x\n",
	    usb_ss->regs->usb_cap2,
	    usb_ss->regs->usb_cap3,
	    usb_ss->regs->usb_cap4,
	    usb_ss->regs->usb_cap5);

	printk(KERN_ERR"60 - 6C: %08x, %08x, %08x, %08x\n",
	    usb_ss->regs->PAD2_74,
	    usb_ss->regs->usb_cpkt1,
	    usb_ss->regs->usb_cpkt2,
	    usb_ss->regs->usb_cpkt3);

	printk(KERN_ERR"300 - 31C: %08x, %08x, %08x, %08x, %08x\n",
	    usb_ss->regs->dma_axi_ctrl,
	    usb_ss->regs->PAD2_148,
	    usb_ss->regs->PAD2_149,
	    usb_ss->regs->PAD2_150,
	    usb_ss->regs->PAD2_151);

	return 0;
}

/**
 * gadget_gop_udc_stop Stops gadget
 * @gadget: gadget object
 *
 * Returns 0
 */
static int gadget_gop_udc_stop(struct usb_gadget *gadget)
{
	struct usb_ss_dev *usb_ss = TO_USB_SS(gadget);
	unsigned long flags;

	spin_lock_irqsave(&usb_ss->lock, flags);
	usb_ss->gadget_driver = NULL;
	spin_unlock_irqrestore(&usb_ss->lock, flags);

	return 0;
}

static const struct usb_gadget_ops usb_ss_gadget_ops = {
	.get_frame = gadget_gop_get_frame,
	.wakeup = gadget_gop_wakeup,
	.set_selfpowered = gadget_gop_set_selfpowered,
	.pullup = gadget_gop_pullup,
	.udc_start = gadget_gop_udc_start,
	.udc_stop = gadget_gop_udc_stop,
};

/**
 * gadget_init_ep Initializes software endpoints of gadget
 * @gadget: gadget object
 *
 * Returns 0 on success, error code elsewhere
 */
static int gadget_init_ep(struct usb_ss_dev *usb_ss)
{
	struct usb_ss_endpoint *usb_ss_ep;
	u32 ep_enabled_reg, iso_ep_reg, bulk_ep_reg;
	int i;
	int ep_reg_pos, ep_dir, ep_number;
	int found_endpoints = 0;

#if IS_ENABLED(CONFIG_USB_CDNS_DRD_TC)
	ep_enabled_reg = usb_ss->ep_enabled_reg;
	iso_ep_reg = usb_ss->iso_ep_reg;
	bulk_ep_reg = usb_ss->bulk_ep_reg;
#else
	ep_enabled_reg = gadget_readl(usb_ss, &usb_ss->regs->usb_cap3);
	iso_ep_reg = gadget_readl(usb_ss, &usb_ss->regs->usb_cap4);
	bulk_ep_reg = gadget_readl(usb_ss, &usb_ss->regs->usb_cap5);
#endif

	cdns_dbg(usb_ss->dev, "Initializing non-zero endpoints\n");

	for (i = 0; i < USB_SS_ENDPOINTS_MAX_COUNT; i++) {
		ep_number = (i / 2) + 1;
		ep_dir = i % 2;
		ep_reg_pos = (16 * ep_dir) + ep_number;

		if (!(ep_enabled_reg & (1uL << ep_reg_pos)))
			continue;

		/* create empty endpoint object */
		usb_ss_ep = devm_kzalloc(usb_ss->dev, sizeof(*usb_ss_ep),
		        GFP_KERNEL);
		if (!usb_ss_ep)
			return -ENOMEM;

		/* set parent of endpoint object */
		usb_ss_ep->usb_ss = usb_ss;

		/* set index of endpoint in endpoints container */
		usb_ss->eps[found_endpoints++] = usb_ss_ep;

		/* set name of endpoint */
		snprintf(usb_ss_ep->name, sizeof(usb_ss_ep->name), "ep%d%s",
		    ep_number, !!ep_dir ? "in" : "out");
		usb_ss_ep->endpoint.name = usb_ss_ep->name;
		cdns_dbg(usb_ss->dev, "Initializing endpoint: %s\n",
		    usb_ss_ep->name);

		usb_ep_set_maxpacket_limit(&usb_ss_ep->endpoint,
		    ENDPOINT_MAX_PACKET_LIMIT);
		usb_ss_ep->endpoint.max_streams = ENDPOINT_MAX_STREAMS;
		usb_ss_ep->endpoint.ops = &usb_ss_gadget_ep_ops;
		if (ep_dir)
			usb_ss_ep->endpoint.caps.dir_in = 1;
		else
			usb_ss_ep->endpoint.caps.dir_out = 1;

		/* check endpoint type */
		if (iso_ep_reg & (1uL << ep_reg_pos)) {
			usb_ss_ep->endpoint.caps.type_iso = 1;
			usb_ss_ep->is_iso_flag = 1;
		}

		if (bulk_ep_reg & (1uL << ep_reg_pos)) {
			usb_ss_ep->endpoint.caps.type_bulk = 1;
			usb_ss_ep->endpoint.caps.type_int = 1;
			usb_ss_ep->endpoint.maxburst = 15;
			usb_ss_ep->endpoint.mult = 2;

		}

		list_add_tail(&usb_ss_ep->endpoint.ep_list,
		    &usb_ss->gadget.ep_list);
		INIT_LIST_HEAD(&usb_ss_ep->request_list);
	}
	usb_ss->ep_nums = found_endpoints;
	return 0;
}

/**
 * gadget_init_ep0 Initializes software endpoint 0 of gadget
 * @gadget: gadget object
 *
 * Returns 0 on success, error code elsewhere
 */
static int gadget_init_ep0(struct usb_ss_dev *usb_ss)
{
	struct usb_ss_endpoint *ep0;

	cdns_dbg(usb_ss->dev, "Initializing EP0\n");
	ep0 = devm_kzalloc(usb_ss->dev, sizeof(struct usb_ss_endpoint),
	        GFP_KERNEL);

	if (!ep0)
		return -ENOMEM;

	/* fill CDNS fields */
	ep0->usb_ss = usb_ss;
	sprintf(ep0->name, "ep0");

	/* fill linux fields */
	ep0->endpoint.ops = &usb_ss_gadget_ep0_ops;
	ep0->endpoint.maxburst = 1;
	usb_ep_set_maxpacket_limit(&ep0->endpoint, ENDPOINT0_MAX_PACKET_LIMIT);
	ep0->endpoint.address = 0;
	ep0->endpoint.maxpacket = 64;
	ep0->endpoint.enabled = 1;
	ep0->endpoint.caps.type_control = 1;
	ep0->endpoint.caps.dir_in = 1;
	ep0->endpoint.caps.dir_out = 1;
	ep0->endpoint.name = ep0->name;

	usb_ss->gadget.ep0 = &ep0->endpoint;

	return 0;
}

#if IS_ENABLED(CONFIG_USB_CDNS_MISC)
/**
 * gadget_stb_turn_off_ref_clock
 * Low level function to enter into standby mode
 * @usb_ss: extended gadget object
 */
static void gadget_stb_turn_off_ref_clock(struct usb_ss_dev *usb_ss)
{
	u32 usb_pwr_reg = cdns_readl(&usb_ss->regs->usb_pwr);

	usb_pwr_reg |= STB_CLK_SWITCH_EN_MASK;
	cdns_writel(&usb_ss->regs->usb_pwr, usb_pwr_reg);

	while (!(cdns_readl(&usb_ss->regs->usb_pwr)
	        & STB_CLK_SWITCH_DONE_MASK))
		udelay(1000);

	usb_ss->in_standby_mode = 1;
}

/**
 * gadget_stb_turn_on_ref_clock
 * Low level function to exit from standby mode
 * @usb_ss: extended gadget object
 */
static void gadget_stb_turn_on_ref_clock(struct usb_ss_dev *usb_ss)
{
	u32 usb_pwr_reg = cdns_readl(&usb_ss->regs->usb_pwr);

	usb_pwr_reg &= ~STB_CLK_SWITCH_EN_MASK;
	cdns_writel(&usb_ss->regs->usb_pwr, usb_pwr_reg);

	while (!(cdns_readl(&usb_ss->regs->usb_pwr)
	        & STB_CLK_SWITCH_DONE_MASK))
		udelay(1000);

	usb_ss->in_standby_mode = 0;
}

/**
 * gadget_stb_ensure_clock_on
 * Ensures clock turned on while accessing following registers:
 * EP_CFG, EP_TR_ADDR, EP_CMD, EP_SEL, USB_CONF
 * @usb_ss: extended gadget object
 */
static void gadget_stb_ensure_clock_on(struct usb_ss_dev *usb_ss)
{
	if (usb_ss->in_standby_mode)
		gadget_stb_turn_on_ref_clock(usb_ss);
}

/**
 * gadget_is_stb_allowed
 * Checks if entering into standby mode is allowed,
 * called from application side
 * @usb_ss: extended gadget object
 *
 * Returns 1 if enter into standby mode is allowed or 0 if not
 */
int gadget_is_stb_allowed(struct usb_ss_dev *usb_ss)
{
	u32 ep_ists_reg = cdns_readl(&usb_ss->regs->ep_sts);
	u32 usb_sts_reg = cdns_readl(&usb_ss->regs->usb_sts);
	int speed;

	/*
	 * Allow to enter into standby mode if
	 * device is not connected and in the
	 * configured state
	 */
	if (!usb_ss->is_connected) {
		if (!(usb_sts_reg & USB_STS__CFGSTS__MASK))
			return 0;
	} else {
		/*
		 * In connected state, allow to enter
		 * into standby mode if DMA is in idle state
		 * and if device is in suspend mode
		 * (U3 in SS or L2 in FS/HS)
		 */
		if (ep_ists_reg & EP_STS__DBUSY__MASK)
			return 0;

		speed = USB_STS__USBSPEED__READ(usb_sts_reg);
		if (speed == USB_SPEED_SUPER) {
			if (USB_STS__LST__READ(usb_sts_reg)
			    != LTSSM_LINK_STATE_U3)
				return 0;
		} else if (speed == USB_SPEED_FULL ||
		    speed == USB_SPEED_HIGH) {
			if (USB_STS__LPMST__READ(usb_sts_reg)
			    != LPM_STATE_L2)
				return 0;
		}
	}
	return 1;
}

/**
 * gadget_enter_stb_request
 * Attempts to enter to standby mode, called from application side
 * @usb_ss: extended gadget object
 *
 * Returns 0 on success, error code elsewhere
 */
int gadget_enter_stb_request(struct usb_ss_dev *usb_ss)
{
	if (usb_ss->in_standby_mode || !gadget_is_stb_allowed(usb_ss))
		return -EPERM;

	gadget_stb_turn_off_ref_clock(usb_ss);
	return 0;
}

/**
 * gadget_exit_stb_request
 * Attempts to exit from standby mode, called from application side
 * @usb_ss: extended gadget object
 *
 * Returns 0 on success, error code elsewhere
 */
int gadget_exit_stb_request(struct usb_ss_dev *usb_ss)
{
	if (!usb_ss->in_standby_mode)
		return -EPERM;

	gadget_stb_turn_on_ref_clock(usb_ss);
	return 0;
}

/**
 * gadget_update_stb_state
 * Updates state of standby mode flag
 * @usb_ss: extended gadget object
 */
static void gadget_update_stb_state(struct usb_ss_dev *usb_ss)
{
	u32 usb_pwr_reg;

	usb_pwr_reg = cdns_readl(&usb_ss->regs->usb_pwr);
	usb_ss->in_standby_mode =
	    (usb_pwr_reg >> STB_CLK_SWITCH_EN_SHIFT) & 1uL;
}
#endif /* IS_ENABLED(CONFIG_USB_CDNS_MISC) */

static int gadget_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct usb_ss_dev *usb_ss;
	struct resource *res;
	int ret;
	int i;

	/* allocate memory for extended gadget object */
	usb_ss = devm_kzalloc(dev, sizeof(*usb_ss), GFP_KERNEL);
	if (!usb_ss)
		return -ENOMEM;

	/* fill extended gadget's required fields */
	usb_ss->dev = dev;
	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if(NULL == res) {
		printk(KERN_ERR "resource null!\n");
		return -ENOMEM;
	} else {
		printk(KERN_INFO "resource %p - %p, %s!\n", res->start, res->end, res->name);
	}
	usb_ss->regs = devm_ioremap_resource(dev, res);

	printk(KERN_INFO "devm_ioremap_resource success!\n");

	if (IS_ERR(usb_ss->regs))
		return PTR_ERR(usb_ss->regs);

	printk(KERN_INFO "usb_ss->regs address valid!\n");
	printk(KERN_INFO "dregs address %p!\n", usb_ss->regs);

	printk(KERN_ERR"00 - 0C: %08x, %08x, %08x, %08x\n",
	    usb_ss->regs->usb_conf,
	    usb_ss->regs->usb_sts,
	    usb_ss->regs->usb_cmd,
	    usb_ss->regs->usb_iptn);

	printk(KERN_ERR"10 - 1C: %08x, %08x, %08x, %08x\n",
	    usb_ss->regs->usb_lpm,
	    usb_ss->regs->usb_ien,
	    usb_ss->regs->usb_ists,
	    usb_ss->regs->ep_sel);

	printk(KERN_ERR"20 - 2C: %08x, %08x, %08x, %08x\n",
	    usb_ss->regs->ep_traddr,
	    usb_ss->regs->ep_cfg,
	    usb_ss->regs->ep_cmd,
	    usb_ss->regs->ep_sts);

	printk(KERN_ERR"30 - 3C: %08x, %08x, %08x, %08x\n",
	    usb_ss->regs->ep_sts_sid,
	    usb_ss->regs->ep_sts_en,
	    usb_ss->regs->drbl,
	    usb_ss->regs->ep_ien);

	printk(KERN_ERR"40 - 4C: %08x, %08x, %08x, %08x\n",
	    usb_ss->regs->ep_ists,
	    usb_ss->regs->usb_pwr,
	    usb_ss->regs->usb_conf2,
	    usb_ss->regs->usb_cap1);

	printk(KERN_ERR"50 - 5C: %08x, %08x, %08x, %08x\n",
	    usb_ss->regs->usb_cap2,
	    usb_ss->regs->usb_cap3,
	    usb_ss->regs->usb_cap4,
	    usb_ss->regs->usb_cap5);

	printk(KERN_ERR"60 - 6C: %08x, %08x, %08x, %08x\n",
	    usb_ss->regs->PAD2_74,
	    usb_ss->regs->usb_cpkt1,
	    usb_ss->regs->usb_cpkt2,
	    usb_ss->regs->usb_cpkt3);

	printk(KERN_ERR"300 - 31C: %08x, %08x, %08x, %08x, %08x\n",
	    usb_ss->regs->dma_axi_ctrl,
	    usb_ss->regs->PAD2_148,
	    usb_ss->regs->PAD2_149,
	    usb_ss->regs->PAD2_150,
	    usb_ss->regs->PAD2_151);

	/* fill gadget fields */
	usb_ss->gadget.ops = &usb_ss_gadget_ops;
	usb_ss->gadget.max_speed = USB_SPEED_SUPER;
	usb_ss->gadget.speed = USB_SPEED_UNKNOWN;
	usb_ss->gadget.name = "usb-ss-gadget";
	usb_ss->gadget.sg_supported = 1;
	usb_ss->is_connected = 0;

	/* set device DMA features */
	if (!dev->dma_mask) {
		dev->dma_parms = dev->parent->dma_parms;
		dev->dma_mask = dev->parent->dma_mask;
		dma_set_coherent_mask(dev, dev->parent->coherent_dma_mask);
	}

	usb_ss->in_standby_mode = 0;

#if IS_ENABLED(CONFIG_USB_CDNS_MISC)
	cdns_dev_misc_register(usb_ss, res->start);
	gadget_update_stb_state(usb_ss);
#endif

#if IS_ENABLED(CONFIG_USB_CDNS_DRD_TC)
	usb_ss->gadget.is_otg = 1;
	usb_ss->ep_enabled_reg = 0x000F000F;
	//usb_ss->iso_ep_reg = 0x00080008;
	usb_ss->iso_ep_reg = 0x0;
	//usb_ss->bulk_ep_reg = 0x00060006;
	usb_ss->bulk_ep_reg = 0x00060002;
	usb_ss->gadget.otg_caps = devm_kzalloc(usb_ss->dev,
	        sizeof(struct usb_otg_caps),
	        GFP_KERNEL);
	usb_ss->gadget.otg_caps->otg_rev = 0x0200;
	usb_ss->gadget.otg_caps->hnp_support = true;
	usb_ss->gadget.otg_caps->srp_support = true;
	usb_ss->gadget.otg_caps->adp_support = false;
#endif

	/* initialize endpoint container */
	INIT_LIST_HEAD(&usb_ss->gadget.ep_list);

	ret = gadget_init_ep0(usb_ss);
	if (ret) {
		cdns_err(usb_ss->dev, "Failed to create endpoint 0\n");
		return -ENOMEM;
	}

	ret = gadget_init_ep(usb_ss);
	if (ret) {
		cdns_err(usb_ss->dev, "Failed to create non zero endpoints\n");
		return -ENOMEM;
	}

	/* allocate memory for default endpoint TRB */
	usb_ss->trb_ep0 = (u32 *)dma_alloc_coherent(usb_ss->dev,
	        ENDPOINT0_TRB_ALLOC_SIZE,
	        &usb_ss->trb_ep0_dma, GFP_DMA);

	if (!usb_ss->trb_ep0) {
		cdns_err(usb_ss->dev,
		    "Failed to allocate memory for ep0 TRB\n");
		return -ENOMEM;
	}

	/* allocate memory for setup packet buffer */
	usb_ss->setup = (u8 *)dma_alloc_coherent(usb_ss->dev,
	        SETUP_BUFFER_ALLOC_SIZE,
	        &usb_ss->setup_dma,
	        GFP_DMA);
	if (!usb_ss->setup) {
		cdns_err(usb_ss->dev,
		    "Failed to allocate memory for SETUP buffer\n");

		ret = -ENOMEM;
		goto free_ep0_buffer;
	}

	/* set driver data */
	dev_set_drvdata(dev, usb_ss);

	/* add USB gadget device */
	ret = usb_add_gadget_udc(usb_ss->dev, &usb_ss->gadget);
	if (ret < 0) {
		cdns_err(usb_ss->dev,
		    "Failed to register USB device controller\n");

		goto free_buffers;
	}

	//workaround for isoc error
	for(i = 0; i < padding_req_num; ++i) {
		padding_req[i] = kzalloc(sizeof(struct usb_request), GFP_KERNEL);
		if (!padding_req[i]) {
			cdns_err(usb_ss->dev,
			    "Failed to allocate memory for padding request\n");
			ret = -ENOMEM;
			goto free_ep0_buffer;
		} else {
			padding_req[i]->buf = &padding_data;
			padding_req[i]->length = sizeof(padding_data);
			padding_req[i]->complete = NULL;
		}
	}

	return 0;

free_buffers:
	dma_free_coherent(usb_ss->dev,
	    SETUP_BUFFER_ALLOC_SIZE,
	    usb_ss->setup, usb_ss->setup_dma);

free_ep0_buffer:
	dma_free_coherent(usb_ss->dev,
	    ENDPOINT0_TRB_ALLOC_SIZE,
	    usb_ss->trb_ep0, usb_ss->trb_ep0_dma);

	return ret;
}

static int gadget_remove(struct platform_device *pdev)
{
	struct usb_ss_dev *usb_ss = dev_get_drvdata(&pdev->dev);
	int i;

	dma_free_coherent(usb_ss->dev,
	    ENDPOINT0_TRB_ALLOC_SIZE,
	    usb_ss->trb_ep0, usb_ss->trb_ep0_dma);

	dma_free_coherent(usb_ss->dev,
	    SETUP_BUFFER_ALLOC_SIZE,
	    usb_ss->setup, usb_ss->setup_dma);

	usb_del_gadget_udc(&usb_ss->gadget);
	//workaround for isoc error
	for(i = 0; i < padding_req_num; ++i) {
		if (padding_req[i]) {
			kfree(padding_req[i]);
		}
	}
	return 0;
}

static const struct of_device_id of_gadget_match[] = {
	{.compatible = "Cadence,usb-dev1.00"},
	{},
};
MODULE_DEVICE_TABLE(of, of_gadget_match);

static struct platform_driver cdns_gadget_driver = {
	.probe = gadget_probe,
	.remove = gadget_remove,
	.driver = {
		.name = "usb-ss",
		.of_match_table = of_match_ptr(of_gadget_match),
	},
};

module_platform_driver(cdns_gadget_driver);
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Cadence USB Super Speed device driver");
