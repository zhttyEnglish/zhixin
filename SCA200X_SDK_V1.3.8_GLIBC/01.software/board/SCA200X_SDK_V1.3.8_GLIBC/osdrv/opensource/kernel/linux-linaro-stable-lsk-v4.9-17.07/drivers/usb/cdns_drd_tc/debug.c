/**
 * debug.c - Cadence USB3 Device Controller debug
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
#include <linux/usb/gadget.h>
#include <linux/list.h>

#include "debug.h"
#include "io.h"
#include "g_regs_map.h"

void display_ep_desc(struct usb_ss_dev *usb_ss, struct usb_ep *usb_endpoint)
{
	cdns_dbg(usb_ss->dev, "======== EP & DESC DUMP ======\n");

	cdns_dbg(usb_ss->dev, "&usb_ep: %p\n", usb_endpoint);
	cdns_dbg(usb_ss->dev, " usb_ep->address: 0x%02X\n",
	    usb_endpoint->address);

	cdns_dbg(usb_ss->dev, " usb_ep->desc: %p\n", usb_endpoint->desc);

	if (usb_endpoint->desc) {
		cdns_dbg(usb_ss->dev, "  usb_ep->desc->bLength: %d\n",
		    usb_endpoint->desc->bLength);
		cdns_dbg(usb_ss->dev,
		    "  usb_ep->desc->bDescriptorType: 0x%02X\n",
		    usb_endpoint->desc->bDescriptorType);
		cdns_dbg(usb_ss->dev,
		    "  usb_ep->desc->bEndpointAddress: 0x%02X\n",
		    usb_endpoint->desc->bEndpointAddress);
		cdns_dbg(usb_ss->dev, "  usb_ep->desc->bmAttributes: 0x%02X\n",
		    usb_endpoint->desc->bmAttributes);
		cdns_dbg(usb_ss->dev, "  usb_ep->desc->wMaxPacketSize: %d\n",
		    usb_endpoint->desc->wMaxPacketSize);
		cdns_dbg(usb_ss->dev, "  usb_ep->desc->bInterval: %d\n",
		    usb_endpoint->desc->bInterval);
		cdns_dbg(usb_ss->dev, "  usb_ep->desc->bRefresh: %d\n",
		    usb_endpoint->desc->bRefresh);
		cdns_dbg(usb_ss->dev, "  usb_ep->desc->bSynchAddress: 0x%02X\n",
		    usb_endpoint->desc->bSynchAddress);
	}

	cdns_dbg(usb_ss->dev, "==============================\n");
}

void print_all_ep(struct usb_ss_dev *usb_ss)
{
	int i;

	for (i = 0; i < USB_SS_ENDPOINTS_MAX_COUNT; i++) {
		cdns_dbg(usb_ss->dev, "/// Dumping %s\n",
		    usb_ss->eps[i]->endpoint.name);
		display_ep_desc(usb_ss, &usb_ss->eps[i]->endpoint);
	}
}

void usb_ss_dump_regs(struct usb_ss_dev *usb_ss)
{
	cdns_dbg(usb_ss->dev, ": =========== REGISTER DUMP ===========\n");

	cdns_dbg(usb_ss->dev, ": Global configuration register (1):  0x%08X\n",
	    cdns_readl(&usb_ss->regs->usb_conf));
	cdns_dbg(usb_ss->dev, ": Global status register:  0x%08X\n",
	    cdns_readl(&usb_ss->regs->usb_sts));
	cdns_dbg(usb_ss->dev, ": Global command register (1):  0x%08X\n",
	    cdns_readl(&usb_ss->regs->usb_cmd));
	cdns_dbg(usb_ss->dev, ": ITP/SOF number register:  0x%08X\n",
	    cdns_readl(&usb_ss->regs->usb_iptn));
	cdns_dbg(usb_ss->dev, ": Global command register (2):  0x%08X\n",
	    cdns_readl(&usb_ss->regs->usb_lpm));
	cdns_dbg(usb_ss->dev, ": Interrupt enable register:  0x%08X\n",
	    cdns_readl(&usb_ss->regs->usb_ien));
	cdns_dbg(usb_ss->dev, ": Interrupt status register:  0x%08X\n",
	    cdns_readl(&usb_ss->regs->usb_ists));
	cdns_dbg(usb_ss->dev, ": Endpoint select register:  0x%08X\n",
	    cdns_readl(&usb_ss->regs->ep_sel));
	cdns_dbg(usb_ss->dev,
	    ": Endpoint transfer ring address register:  0x%08X\n",
	    cdns_readl(&usb_ss->regs->ep_traddr));
	cdns_dbg(usb_ss->dev, ": Endpoint configuration register:  0x%08X\n",
	    cdns_readl(&usb_ss->regs->ep_cfg));
	cdns_dbg(usb_ss->dev, ": Endpoint command register:  0x%08X\n",
	    cdns_readl(&usb_ss->regs->ep_cmd));
	cdns_dbg(usb_ss->dev, ": Endpoint status register (1):  0x%08X\n",
	    cdns_readl(&usb_ss->regs->ep_sts));
	cdns_dbg(usb_ss->dev, ": Endpoint status register (2):  0x%08X\n",
	    cdns_readl(&usb_ss->regs->ep_sts_sid));
	cdns_dbg(usb_ss->dev, ": Endpoint status enable register:  0x%08X\n",
	    cdns_readl(&usb_ss->regs->ep_sts_en));
	cdns_dbg(usb_ss->dev, ": Doorbell register:  0x%08X\n",
	    cdns_readl(&usb_ss->regs->drbl));
	cdns_dbg(usb_ss->dev,
	    ": Endpoint interrupt enable register:  0x%08X\n",
	    cdns_readl(&usb_ss->regs->ep_ien));
	cdns_dbg(usb_ss->dev,
	    ": Endpoint interrupt status register:  0x%08X\n",
	    cdns_readl(&usb_ss->regs->ep_ists));
	cdns_dbg(usb_ss->dev,
	    ": Global power configuration register:  0x%08X\n",
	    cdns_readl(&usb_ss->regs->usb_pwr));
	cdns_dbg(usb_ss->dev, ": Global configuration register (2):  0x%08X\n",
	    cdns_readl(&usb_ss->regs->usb_conf2));
	cdns_dbg(usb_ss->dev, ": Capability register (1):  0x%08X\n",
	    cdns_readl(&usb_ss->regs->usb_cap1));
	cdns_dbg(usb_ss->dev, ": Capability register (2):  0x%08X\n",
	    cdns_readl(&usb_ss->regs->usb_cap2));
	cdns_dbg(usb_ss->dev, ": Capability register (3):  0x%08X\n",
	    cdns_readl(&usb_ss->regs->usb_cap3));
	cdns_dbg(usb_ss->dev, ": Capability register (4):  0x%08X\n",
	    cdns_readl(&usb_ss->regs->usb_cap4));
	cdns_dbg(usb_ss->dev, ": Capability register (5):  0x%08X\n",
	    cdns_readl(&usb_ss->regs->usb_cap5));
	cdns_dbg(usb_ss->dev, ": Custom packet register (1):  0x%08X\n",
	    cdns_readl(&usb_ss->regs->usb_cpkt1));
	cdns_dbg(usb_ss->dev, ": Custom packet register (2):  0x%08X\n",
	    cdns_readl(&usb_ss->regs->usb_cpkt2));
	cdns_dbg(usb_ss->dev, ": Custom packet register (3):  0x%08X\n",
	    cdns_readl(&usb_ss->regs->usb_cpkt3));

	cdns_dbg(usb_ss->dev,
	    ": ===========================================\n");
}

void usb_ss_dump_reg(struct usb_ss_dev *usb_ss, uint32_t __iomem *reg)
{
	cdns_dbg(usb_ss->dev, "Address: %p, value: 0x%08X\n", reg,
	    cdns_readl(reg));
}

void usb_ss_dbg_dump_lnx_usb_ep(struct device *dev, struct usb_ep *ep, int tab)
{
	if (!!tab) {
		cdns_dbg(dev, "\tAddress: %p\n", ep);
		cdns_dbg(dev, "\n");
		cdns_dbg(dev, "\t ep->driver_data: 0x%08X (void*)\n",
		    (unsigned int)ep->driver_data);
		cdns_dbg(dev, "\t ep->name: %s (const char*)\n", ep->name);
		cdns_dbg(dev, "\t ep->ops: %p (const struct usb_ep_ops*)\n",
		    ep->ops);
		cdns_dbg(dev, "\t&ep->ep_list: %p (struct list_head)\n",
		    &ep->ep_list);
		cdns_dbg(dev, "\t ep->maxpacket: %d (unsigned:16)\n",
		    ep->maxpacket);
		cdns_dbg(dev, "\t ep->maxpacket_limit: %d (unsigned:16)\n",
		    ep->maxpacket_limit);
		cdns_dbg(dev, "\t ep->max_streams: %d (unsigned:16)\n",
		    ep->max_streams);
		cdns_dbg(dev, "\t ep->mult: %d (unsigned:2)\n", ep->mult);
		cdns_dbg(dev, "\t ep->maxburst: %d (unsigned:5)\n",
		    ep->maxburst);
		cdns_dbg(dev, "\t ep->address: 0x%02X (u8)\n", ep->address);
		cdns_dbg(dev,
		    "\t ep->desc: %p (const struct usb_endpoint_descriptor*)\n",
		    ep->desc);
		cdns_dbg(dev,
		    "\t ep->comp_desc: %p (const struct usb_ss_ep_comp_descriptor*)\n",
		    ep->comp_desc);
		cdns_dbg(dev, "\t Caps:\n");
		cdns_dbg(dev, "\t\t ep->caps.type_control: %d\n",
		    ep->caps.type_control);
		cdns_dbg(dev, "\t\t ep->caps.type_iso: %d\n",
		    ep->caps.type_iso);
		cdns_dbg(dev, "\t\t ep->caps.type_bulk: %d\n",
		    ep->caps.type_bulk);
		cdns_dbg(dev, "\t\t ep->caps.type_int: %d\n",
		    ep->caps.type_int);
		cdns_dbg(dev, "\t\t ep->caps.dir_in: %d\n", ep->caps.dir_in);
		cdns_dbg(dev, "\t\t ep->caps.dir_out: %d\n", ep->caps.dir_out);
		cdns_dbg(dev, "\n");
		cdns_dbg(dev,
		    "// ============================================== //\n");
	} else {
		cdns_dbg(dev,
		    "// =============== DUMP: struct usb_ep ========== //\n");
		cdns_dbg(dev, "Address: %p\n", ep);
		cdns_dbg(dev, "\n");
		cdns_dbg(dev, " ep->driver_data: 0x%08X (void*)\n",
		    (unsigned int)ep->driver_data);
		cdns_dbg(dev, " ep->name: %s (const char*)\n", ep->name);
		cdns_dbg(dev, " ep->ops: %p (const struct usb_ep_ops*)\n",
		    ep->ops);
		cdns_dbg(dev, "&ep->ep_list: %p (struct list_head)\n",
		    &ep->ep_list);
		cdns_dbg(dev, " ep->maxpacket: %d (unsigned:16)\n",
		    ep->maxpacket);
		cdns_dbg(dev, " ep->maxpacket_limit: %d (unsigned:16)\n",
		    ep->maxpacket_limit);
		cdns_dbg(dev, " ep->max_streams: %d (unsigned:16)\n",
		    ep->max_streams);
		cdns_dbg(dev, " ep->mult: %d (unsigned:2)\n", ep->mult);
		cdns_dbg(dev, " ep->maxburst: %d (unsigned:5)\n", ep->maxburst);
		cdns_dbg(dev, " ep->address: 0x%02X (u8)\n", ep->address);
		cdns_dbg(dev,
		    " ep->desc: %p (const struct usb_endpoint_descriptor*)\n",
		    ep->desc);
		cdns_dbg(dev,
		    " ep->comp_desc: %p (const struct usb_ss_ep_comp_descriptor*)\n",
		    ep->comp_desc);
		cdns_dbg(dev, " Caps:\n");
		cdns_dbg(dev, "\t ep->caps.type_control: %d\n",
		    ep->caps.type_control);
		cdns_dbg(dev, "\t ep->caps.type_iso: %d\n", ep->caps.type_iso);
		cdns_dbg(dev, "\t ep->caps.type_bulk: %d\n",
		    ep->caps.type_bulk);
		cdns_dbg(dev, "\t ep->caps.type_int: %d\n", ep->caps.type_int);
		cdns_dbg(dev, "\t ep->caps.dir_in: %d\n", ep->caps.dir_in);
		cdns_dbg(dev, "\t ep->caps.dir_out: %d\n", ep->caps.dir_out);
		cdns_dbg(dev, "\n");
		cdns_dbg(dev,
		    "// ============================================== //\n");
	}
}

void usb_ss_dbg_dump_lnx_usb_gadget(struct usb_gadget *gadget)
{
	struct usb_ep *end_point;
	struct list_head *element;

	cdns_dbg(&gadget->dev,
	    "// ============ DUMP: struct usb_gadget ========= //\n");
	cdns_dbg(&gadget->dev, "Address: %p\n", gadget);
	cdns_dbg(&gadget->dev, "\n");
	cdns_dbg(&gadget->dev, "&gadget->work: %p (struct work_struct)\n",
	    &gadget->work);
	cdns_dbg(&gadget->dev, " gadget->udc: %p (struct usb_udc*)\n",
	    gadget->udc);
	cdns_dbg(&gadget->dev,
	    " gadget->ops: %p (const struct usb_gadget_ops*)\n",
	    gadget->ops);
	cdns_dbg(&gadget->dev, " gadget->ep0: %p (struct usb_ep*)\n",
	    gadget->ep0);
	cdns_dbg(&gadget->dev, "&gadget->ep_list: %p (struct list_head)\n",
	    &gadget->ep_list);

	list_for_each(element, &gadget->ep_list) {
		end_point = list_entry(element, struct usb_ep, ep_list);
		if (end_point)
			usb_ss_dbg_dump_lnx_usb_ep(&gadget->dev, end_point, 1);
	}

	cdns_dbg(&gadget->dev, " gadget->speed: %d (enum usb_device_speed)\n",
	    gadget->speed);
	cdns_dbg(&gadget->dev,
	    " gadget->max_speed: %d (enum usb_device_speed)\n",
	    gadget->max_speed);
	cdns_dbg(&gadget->dev, " gadget->state: %d (enum usb_device_state)\n",
	    gadget->state);
	cdns_dbg(&gadget->dev, " gadget->name: %s (const char*)\n",
	    gadget->name);
	cdns_dbg(&gadget->dev, "gadget->dev: %p (struct device)\n",
	    &gadget->dev);
	cdns_dbg(&gadget->dev, " gadget->out_epnum: %d (unsigned)\n",
	    gadget->out_epnum);
	cdns_dbg(&gadget->dev, " gadget->in_epnum: %d (unsigned)\n",
	    gadget->in_epnum);
	cdns_dbg(&gadget->dev, " gadget->sg_supported: %d (unsigned:1)\n",
	    gadget->sg_supported);
	cdns_dbg(&gadget->dev, " gadget->is_otg: %d (unsigned:1)\n",
	    gadget->is_otg);
	cdns_dbg(&gadget->dev, " gadget->is_a_peripheral: %d (unsigned:1)\n",
	    gadget->is_a_peripheral);
	cdns_dbg(&gadget->dev, " gadget->b_hnp_enable: %d (unsigned:1)\n",
	    gadget->b_hnp_enable);
	cdns_dbg(&gadget->dev, " gadget->a_hnp_support: %d (unsigned:1)\n",
	    gadget->a_hnp_support);
	cdns_dbg(&gadget->dev, " gadget->a_alt_hnp_support: %d (unsigned:1)\n",
	    gadget->a_alt_hnp_support);
	cdns_dbg(&gadget->dev,
	    " gadget->quirk_ep_out_aligned_size: %d (unsigned:1)\n",
	    gadget->quirk_ep_out_aligned_size);
	cdns_dbg(&gadget->dev, " gadget->is_selfpowered: %d (unsigned:1)\n",
	    gadget->is_selfpowered);
	cdns_dbg(&gadget->dev, "\n");
	cdns_dbg(&gadget->dev,
	    "// ============================================== //\n");
}

void usb_ss_dbg_dump_lnx_usb_request(struct usb_ss_dev *usb_ss,
    struct usb_request *usb_req)
{
	cdns_dbg(usb_ss->dev,
	    "// ============ DUMP: struct usb_request ======== //\n");
	cdns_dbg(usb_ss->dev, "Address: %p\n", usb_req);
	cdns_dbg(usb_ss->dev, "\n");
	cdns_dbg(usb_ss->dev, " usb_req->buf: 0x%08X (void*)\n",
	    (unsigned int)usb_req->buf);
	cdns_dbg(usb_ss->dev, " usb_req->length: %d (unsigned)\n",
	    usb_req->length);
	cdns_dbg(usb_ss->dev, " usb_req->dma: 0x%08llX (dma_addr_t)\n",
	    (unsigned long long int)usb_req->dma);
	cdns_dbg(usb_ss->dev, " usb_req->sg: %p (struct scatterlist*)\n",
	    usb_req->sg);
	cdns_dbg(usb_ss->dev, " usb_req->num_sgs: %d (unsigned)\n",
	    usb_req->num_sgs);
	cdns_dbg(usb_ss->dev, " usb_req->num_mapped_sgs: %d (unsigned)\n",
	    usb_req->num_mapped_sgs);
	cdns_dbg(usb_ss->dev, " usb_req->stream_id: %d (unsigned:16)\n",
	    usb_req->stream_id);
	cdns_dbg(usb_ss->dev, " usb_req->no_interrupt: %d (unsigned:1)\n",
	    usb_req->no_interrupt);
	cdns_dbg(usb_ss->dev, " usb_req->zero: %d (unsigned:1)\n",
	    usb_req->zero);
	cdns_dbg(usb_ss->dev, " usb_req->short_not_ok: %d (unsigned:1)\n",
	    usb_req->short_not_ok);
	cdns_dbg(usb_ss->dev,
	    " usb_req->complete: %p ((void)(f_ptr*)(struct usb_ep *ep, struct usb_request *req))\n",
	    usb_req->complete);
	cdns_dbg(usb_ss->dev, " usb_req->context: 0x%08X (void*)\n",
	    (unsigned int)usb_req->context);
	cdns_dbg(usb_ss->dev, "&usb_req->list: %p (struct list_head)\n",
	    &usb_req->list);
	cdns_dbg(usb_ss->dev, " usb_req->status: %d (int)\n", usb_req->status);
	cdns_dbg(usb_ss->dev, " usb_req->actual: %d (unsigned)\n",
	    usb_req->actual);
	cdns_dbg(usb_ss->dev, "\n");
	cdns_dbg(usb_ss->dev,
	    "// ============================================== //\n");
}

void usb_ss_dbg_dump_cdns_usb_ss_dev(struct usb_ss_dev *usb_ss)
{
	cdns_dbg(usb_ss->dev,
	    "// ============ DUMP: struct usb_ss_dev ========= //\n");
	cdns_dbg(usb_ss->dev, "Address: %p\n", usb_ss);
	cdns_dbg(usb_ss->dev, "\n");
	cdns_dbg(usb_ss->dev, " usb_ss->dev: %p (struct device*)\n",
	    usb_ss->dev);
	cdns_dbg(usb_ss->dev, " usb_ss->regs: 0x%08X (void*)\n",
	    (unsigned int)usb_ss->regs);
	cdns_dbg(usb_ss->dev, "&usb_ss->gadget: %p (struct usb_gadget)\n",
	    &usb_ss->gadget);
	cdns_dbg(usb_ss->dev,
	    " usb_ss->gadget_driver: %p (struct usb_gadget_driver*)\n",
	    usb_ss->gadget_driver);
	cdns_dbg(usb_ss->dev, " usb_ss->eps: %p (struct usb_ss_ep* a[32])\n",
	    usb_ss->eps);
	cdns_dbg(usb_ss->dev, "\n");
	cdns_dbg(usb_ss->dev,
	    "// ============================================== //\n");
}

void usb_ss_dbg_dump_cdns_usb_ss_ep(struct usb_ss_dev *usb_ss,
    struct usb_ss_endpoint *usb_ss_ep)
{
	cdns_dbg(usb_ss->dev,
	    "// ============ DUMP: struct usb_ss_ep ========== //\n");
	cdns_dbg(usb_ss->dev, "Address: %p\n", usb_ss_ep);
	cdns_dbg(usb_ss->dev, "\n");
	cdns_dbg(usb_ss->dev, " usb_ss_ep->usb_ss: %p (struct usb_ss_dev*)\n",
	    usb_ss_ep->usb_ss);
	cdns_dbg(usb_ss->dev, " usb_ss_ep->name: %s (char*)\n",
	    usb_ss_ep->name);
	cdns_dbg(usb_ss->dev, "&usb_ss_ep->endpoint: %p (struct usb_ep)\n",
	    &usb_ss_ep->endpoint);
	cdns_dbg(usb_ss->dev,
	    "&usb_ss_ep->request_list: %p (struct list_head)\n",
	    &usb_ss_ep->request_list);
	cdns_dbg(usb_ss->dev, "\n");
	cdns_dbg(usb_ss->dev,
	    "// ============================================== //\n");
}

void usb_ss_dbg_dump_cdns_usb_ss_trb(struct usb_ss_dev *usb_ss,
    struct usb_ss_trb *usb_trb)
{
	cdns_dbg(usb_ss->dev,
	    "// ============ DUMP: struct usb_ss_ep ========== //\n");
	cdns_dbg(usb_ss->dev, "Address: %p\n", usb_trb);
	cdns_dbg(usb_ss->dev, "\n");
	cdns_dbg(usb_ss->dev, " usb_trb->offset8: 0x%08X (u32)\n",
	    usb_trb->offset8);
	cdns_dbg(usb_ss->dev, " usb_trb->offset4: 0x%08X (u32)\n",
	    usb_trb->offset4);
	cdns_dbg(usb_ss->dev, " usb_trb->offset0: 0x%08X (u32)\n",
	    usb_trb->offset0);
	cdns_dbg(usb_ss->dev, "\n");
	cdns_dbg(usb_ss->dev,
	    "// ============================================== //\n");
}
