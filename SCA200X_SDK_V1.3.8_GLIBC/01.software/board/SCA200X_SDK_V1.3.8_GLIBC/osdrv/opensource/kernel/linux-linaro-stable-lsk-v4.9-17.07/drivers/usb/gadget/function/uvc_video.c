/*
 *	uvc_video.c  --  USB Video Class Gadget driver
 *
 *	Copyright (C) 2009-2010
 *	    Laurent Pinchart (laurent.pinchart@ideasonboard.com)
 *
 *	This program is free software; you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation; either version 2 of the License, or
 *	(at your option) any later version.
 */

#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/errno.h>
#include <linux/usb/ch9.h>
#include <linux/usb/gadget.h>
#include <linux/usb/video.h>
#include <asm/arch_timer.h>

#include <media/v4l2-dev.h>
#include <asm/div64.h>

#include "uvc.h"
#include "uvc_queue.h"
#include "uvc_video.h"
/* --------------------------------------------------------------------------
 * Video codecs
 */
#ifdef CONFIG_USB_CONFIGFS_F_UVC_TIMESTAMP

static int
uvc_video_encode_header(struct uvc_video *video, struct uvc_buffer *buf,
		u8 *data, int len)
{
	u32 * pts = (u32 *)&data[2];
	u32 * src = (u32 *)&data[6];
	u16 * sof = (u16 *)&data[10];
	u64 tmp = 0;

#define SYS_CLOCK_RATE      (250000000)
#define SYS_CLOCK_RATE_MS   (SYS_CLOCK_RATE/1000)
#define SYS_CLOCK_RATE_US   (SYS_CLOCK_RATE/1000/1000)


	data[0] = 12;
	data[1] = UVC_STREAM_SCR | UVC_STREAM_PTS | video->fid;

	if (buf->bytesused - video->queue.buf_used <= len - 12)
	{
		//printk(KERN_INFO "bytesused:%d buf_used:%d len - 2:%d", buf->bytesused, video->queue.buf_used, len - 2);
		data[1] |= UVC_STREAM_EOF;
	}

	tmp = buf->buf.vb2_buf.timestamp;
	do_div(tmp, 1000);
	*pts = (u32)tmp; //unit 1us

	tmp = ktime_get_ns();
	do_div(tmp, 1000);
	//tmp = arch_counter_get_cntpct();
	//do_div(tmp, SYS_CLOCK_RATE_US);
	*src = (u32)(tmp); //unit 1us

	//printk(KERN_INFO "time diff %u %u\n", *src - *pts, *pts);

	if (gadget_is_dualspeed(dev_to_usb_gadget(video->uvc->v4l2_dev.dev)))
		*sof = (u16)(usb_gadget_frame_number(dev_to_usb_gadget(video->uvc->v4l2_dev.dev)) >> 3);
	else
		*sof = (u16)usb_gadget_frame_number(dev_to_usb_gadget(video->uvc->v4l2_dev.dev));

	video->sof = *sof;

	//*sof = (u16)((video->sof >> 3) & 0xffff);

	//++video->sof;

	return 12;
}

#else
static int
uvc_video_encode_header(struct uvc_video *video, struct uvc_buffer *buf,
		u8 *data, int len)
{
	data[0] = 2;
	data[1] = UVC_STREAM_EOH | video->fid;

	if (buf->bytesused - video->queue.buf_used <= len - 2)
	{
		//printk(KERN_INFO "bytesused:%d buf_used:%d len - 2:%d", buf->bytesused, video->queue.buf_used, len - 2);
		data[1] |= UVC_STREAM_EOF;
	}

	return 2;
}
#endif

static int
uvc_video_encode_data(struct uvc_video *video, struct uvc_buffer *buf,
		u8 *data, int len)
{
	struct uvc_video_queue *queue = &video->queue;
	unsigned int nbytes;
	void *mem;

	/* Copy video data to the USB buffer. */
	mem = buf->mem + queue->buf_used;
	nbytes = min((unsigned int)len, buf->bytesused - queue->buf_used);

	memcpy(data, mem, nbytes);
	queue->buf_used += nbytes;

	return nbytes;
}

static void
uvc_video_encode_bulk(struct usb_request *req, struct uvc_video *video,
		struct uvc_buffer *buf)
{
	void *mem = req->buf;
	int len = video->req_size;
	int ret;

	/* Add a header at the beginning of the payload. */
	if (video->payload_size == 0) {
		ret = uvc_video_encode_header(video, buf, mem, len);
		video->payload_size += ret;
		mem += ret;
		len -= ret;
	}

	/* Process video data. */
	len = min((int)(video->max_payload_size - video->payload_size), len);
	ret = uvc_video_encode_data(video, buf, mem, len);

	video->payload_size += ret;
	len -= ret;

	req->length = video->req_size - len;
#ifndef CONFIG_USB_CONFIGFS_F_UVC_BULK_TRANSFER_SINGLE
	req->zero = video->payload_size == video->max_payload_size;
#else
	req->zero = 0;
#endif

	if (buf->bytesused <= video->queue.buf_used) {
		struct uvc_video *video = req->context;
		struct uvc_device *uvc = video->uvc;

		video->queue.buf_used = 0;
		buf->state = UVC_BUF_STATE_DONE;
		uvcg_queue_next_buffer(&video->queue, buf);
		video->fid ^= UVC_STREAM_FID;

		if(0 == req->zero &&
		   0 == (video->payload_size % uvc->video.ep->maxpacket))
			req->zero = 1;

		video->payload_size = 0;
	}

	if (video->payload_size == video->max_payload_size ||
	    buf->bytesused == video->queue.buf_used)
		video->payload_size = 0;
}

static void
uvc_video_encode_isoc(struct usb_request *req, struct uvc_video *video,
		struct uvc_buffer *buf)
{
	void *mem = req->buf;
	int len = video->req_size;
	int ret;

	/* Add the header. */
	ret = uvc_video_encode_header(video, buf, mem, len);
	mem += ret;
	len -= ret;

	/* Process video data. */
	ret = uvc_video_encode_data(video, buf, mem, len);
	len -= ret;

	req->length = video->req_size - len;

	if (buf->bytesused <= video->queue.buf_used) {
		video->queue.buf_used = 0;
		buf->state = UVC_BUF_STATE_DONE;
		uvcg_queue_next_buffer(&video->queue, buf);
		video->fid ^= UVC_STREAM_FID;
	}
}

/* --------------------------------------------------------------------------
 * Request handling
 */

static int uvcg_video_ep_queue(struct uvc_video *video, struct usb_request *req)
{
	int ret;

	ret = usb_ep_queue(video->ep, req, GFP_ATOMIC);
	if (ret < 0) {
		printk(KERN_ERR "Failed to queue request (%d).\n",
			 ret);

		/* Isochronous endpoints can't be halted. */
		if (-ESHUTDOWN != ret && NULL != video && NULL != video->ep &&
			NULL != video->ep->desc && usb_endpoint_xfer_bulk(video->ep->desc))
		{
			usb_ep_set_halt(video->ep);
		}
	}

	return ret;
}

/*
 * I somehow feel that synchronisation won't be easy to achieve here. We have
 * three events that control USB requests submission:
 *
 * - USB request completion: the completion handler will resubmit the request
 *   if a video buffer is available.
 *
 * - USB interface setting selection: in response to a SET_INTERFACE request,
 *   the handler will start streaming if a video buffer is available and if
 *   video is not currently streaming.
 *
 * - V4L2 buffer queueing: the driver will start streaming if video is not
 *   currently streaming.
 *
 * Race conditions between those 3 events might lead to deadlocks or other
 * nasty side effects.
 *
 * The "video currently streaming" condition can't be detected by the irqqueue
 * being empty, as a request can still be in flight. A separate "queue paused"
 * flag is thus needed.
 *
 * The paused flag will be set when we try to retrieve the irqqueue head if the
 * queue is empty, and cleared when we queue a buffer.
 *
 * The USB request completion handler will get the buffer at the irqqueue head
 * under protection of the queue spinlock. If the queue is empty, the streaming
 * paused flag will be set. Right after releasing the spinlock a userspace
 * application can queue a buffer. The flag will then cleared, and the ioctl
 * handler will restart the video stream.
 */
#if 0
static void
uvc_video_complete(struct usb_ep *ep, struct usb_request *req)
{
	struct uvc_video *video = req->context;
	struct uvc_video_queue *queue = &video->queue;
	struct uvc_buffer *buf;
	unsigned long flags;
	int ret;
#ifndef CONFIG_USB_CONFIGFS_F_UVC_BULK_TRANSFER
	switch (req->status) {
	case 0:
		break;
	case -EXDEV:
#ifdef CONFIG_USB_CONFIGFS_F_UVC_POOR_HOST
		//printk(KERN_INFO "1.VS request completed with status %d.\n",
		//        req->status);
#else
		printk(KERN_INFO "1.VS request completed with status %d.\n",
		        req->status);
#endif
		break;

	case -ESHUTDOWN:	/* disconnect from host. */
		printk(KERN_DEBUG "VS request cancelled.\n");
		uvcg_queue_cancel(queue, 1);
		goto requeue;

	default:
		printk(KERN_INFO "VS request completed with status %d.\n",
			req->status);
		uvcg_queue_cancel(queue, 0);
		goto requeue;
	}

	spin_lock_irqsave(&video->queue.irqlock, flags);

#ifdef CONFIG_USB_CONFIGFS_F_UVC_POOR_HOST
	if(-EXDEV == req->status)
	{
		ret = uvcg_video_ep_queue(video, req);
		spin_unlock_irqrestore(&video->queue.irqlock, flags);
	}
	else
	{
#endif
		buf = uvcg_queue_head(&video->queue);
		if (buf == NULL)
		{
			req->length = 0;

			ret = uvcg_video_ep_queue(video, req);
			spin_unlock_irqrestore(&video->queue.irqlock, flags);
			//goto requeue;
		}
		else
		{
			video->encode(req, video, buf);

			ret = uvcg_video_ep_queue(video, req);
			spin_unlock_irqrestore(&video->queue.irqlock, flags);
		}
#ifdef CONFIG_USB_CONFIGFS_F_UVC_POOR_HOST
	}
#endif
#else

	switch (req->status) {
	case 0:
		break;

	case -ESHUTDOWN:	/* disconnect from host. */
		printk(KERN_DEBUG "VS request cancelled.\n");
		uvcg_queue_cancel(queue, 1);
		goto requeue;

	default:
		printk(KERN_INFO "VS request completed with status %d.\n",
			req->status);
		uvcg_queue_cancel(queue, 0);
		goto requeue;
	}

	spin_lock_irqsave(&video->queue.irqlock, flags);

	buf = uvcg_queue_head(&video->queue);
	if (buf == NULL)
	{
#ifdef CONFIG_USB_CONFIGFS_F_UVC_BULK_TRANSFER_ENHANCE
		req->length = 0;
		ret = uvcg_video_ep_queue(video, req);
		spin_unlock_irqrestore(&video->queue.irqlock, flags);
#else
		spin_unlock_irqrestore(&video->queue.irqlock, flags);
		goto requeue;
#endif
	}
	else
	{
		video->encode(req, video, buf);
		ret = uvcg_video_ep_queue(video, req);
		spin_unlock_irqrestore(&video->queue.irqlock, flags);
	}
#endif

	if (ret < 0) {
		uvcg_queue_cancel(queue, 0);
		goto requeue;
	}
	/*if ((ret = usb_ep_queue(ep, req, GFP_ATOMIC)) < 0) {
		printk(KERN_INFO "Failed to queue request (%d).\n", ret);
		usb_ep_set_halt(ep);
		spin_unlock_irqrestore(&video->queue.irqlock, flags);
		uvcg_queue_cancel(queue, 0);
		goto requeue;
	}
	spin_unlock_irqrestore(&video->queue.irqlock, flags);*/

	return;

requeue:
	spin_lock_irqsave(&video->req_lock, flags);
	list_add_tail(&req->list, &video->req_free);
	spin_unlock_irqrestore(&video->req_lock, flags);
}
#endif
static void
uvc_video_complete(struct usb_ep *ep, struct usb_request *req)
{
	struct uvc_video *video = req->context;
	struct uvc_video_queue *queue = &video->queue;
	unsigned long flags;

	switch (req->status) {
	case 0:
		break;

	case -ESHUTDOWN:	/* disconnect from host. */
		printk(KERN_DEBUG "VS request cancelled.\n");
		uvcg_queue_cancel(queue, 1);
		break;

	default:
		printk(KERN_INFO "VS request completed with status %d.\n",
			req->status);
		uvcg_queue_cancel(queue, 0);
		break;
	}

	spin_lock_irqsave(&video->req_lock, flags);
	list_add_tail(&req->list, &video->req_free);
	spin_unlock_irqrestore(&video->req_lock, flags);

	schedule_work(&video->pump);
}

static int
uvc_video_free_requests(struct uvc_video *video)
{
	unsigned int i;

	for (i = 0; i < UVC_NUM_REQUESTS; ++i) {
		if (video->req[i]) {
			usb_ep_free_request(video->ep, video->req[i]);
			video->req[i] = NULL;
		}

		if (video->req_buffer[i]) {
			kfree(video->req_buffer[i]);
			video->req_buffer[i] = NULL;
		}
	}

	INIT_LIST_HEAD(&video->req_free);
	video->req_size = 0;
	return 0;
}

static int
uvc_video_alloc_requests(struct uvc_video *video)
{
	unsigned int req_size;
	unsigned int i;
	int ret = -ENOMEM;

	BUG_ON(video->req_size);
#ifndef CONFIG_USB_CONFIGFS_F_UVC_BULK_TRANSFER
	req_size = video->ep->maxpacket
		 * max_t(unsigned int, video->ep->maxburst, 1)
		 * (video->ep->mult);
#else
	req_size = video->ep->maxpacket
		 * max_t(unsigned int, video->ep->maxburst, 1);
	if(video->max_payload_size > req_size)
		req_size = video->max_payload_size;
#endif
	for (i = 0; i < UVC_NUM_REQUESTS; ++i) {
		video->req_buffer[i] = kmalloc(req_size, GFP_KERNEL);
		if (video->req_buffer[i] == NULL)
			goto error;

		video->req[i] = usb_ep_alloc_request(video->ep, GFP_KERNEL);
		if (video->req[i] == NULL)
			goto error;

		video->req[i]->buf = video->req_buffer[i];
		video->req[i]->length = 0;
		video->req[i]->complete = uvc_video_complete;
		video->req[i]->context = video;

		list_add_tail(&video->req[i]->list, &video->req_free);
	}

	video->req_size = req_size;

	return 0;

error:
	uvc_video_free_requests(video);
	return ret;
}

/* --------------------------------------------------------------------------
 * Video streaming
 */

/*
 * uvcg_video_pump - Pump video data into the USB requests
 *
 * This function fills the available USB requests (listed in req_free) with
 * video data from the queued buffers.
 */
#if 0
int uvcg_video_pump(struct uvc_video *video)
{
	struct uvc_video_queue *queue = &video->queue;
	struct usb_request *req;
	struct uvc_buffer *buf;
	unsigned long flags;
	int ret;

	/* FIXME TODO Race between uvcg_video_pump and requests completion
	 * handler ???
	 */

	while (1) {
		/* Retrieve the first available USB request, protected by the
		 * request lock.
		 */
		spin_lock_irqsave(&video->req_lock, flags);
		if (list_empty(&video->req_free)) {
			spin_unlock_irqrestore(&video->req_lock, flags);
			return 0;
		}
		req = list_first_entry(&video->req_free, struct usb_request,
					list);
		list_del(&req->list);
		spin_unlock_irqrestore(&video->req_lock, flags);

		/* Retrieve the first available video buffer and fill the
		 * request, protected by the video queue irqlock.
		 */
		spin_lock_irqsave(&queue->irqlock, flags);
		buf = uvcg_queue_head(queue);
		if (buf == NULL) {
			spin_unlock_irqrestore(&queue->irqlock, flags);
			break;
		}

		video->encode(req, video, buf);

		/* Queue the USB request */
		ret = uvcg_video_ep_queue(video, req);
		spin_unlock_irqrestore(&queue->irqlock, flags);

		if (ret < 0) {
			uvcg_queue_cancel(queue, 0);
			break;
		}
		/*ret = usb_ep_queue(video->ep, req, GFP_ATOMIC);
		if (ret < 0) {
			printk(KERN_INFO "Failed to queue request (%d)\n", ret);

			if(-ESHUTDOWN == ret)
				printk(KERN_ERR "Can not halt usb (%d)\n", ret);
			else
				usb_ep_set_halt(video->ep);

			spin_unlock_irqrestore(&queue->irqlock, flags);
			uvcg_queue_cancel(queue, 0);
			break;
		}
		spin_unlock_irqrestore(&queue->irqlock, flags);*/
	}

	spin_lock_irqsave(&video->req_lock, flags);
	list_add_tail(&req->list, &video->req_free);
	spin_unlock_irqrestore(&video->req_lock, flags);
	return 0;
}
#endif
static void uvcg_video_pump(struct work_struct *work)
{
	struct uvc_video *video = container_of(work, struct uvc_video, pump);
	struct uvc_video_queue *queue = &video->queue;
	struct usb_request *req;
	struct uvc_buffer *buf;
	unsigned long flags;
	int ret;

	while (1) {
		/* Retrieve the first available USB request, protected by the
		 * request lock.
		 */
		spin_lock_irqsave(&video->req_lock, flags);
		if (list_empty(&video->req_free)) {
			spin_unlock_irqrestore(&video->req_lock, flags);
			return;
		}
		req = list_first_entry(&video->req_free, struct usb_request,
					list);
		list_del(&req->list);
		spin_unlock_irqrestore(&video->req_lock, flags);

		/* Retrieve the first available video buffer and fill the
		 * request, protected by the video queue irqlock.
		 */
		spin_lock_irqsave(&queue->irqlock, flags);
		buf = uvcg_queue_head(queue);
		if (buf == NULL) {
			spin_unlock_irqrestore(&queue->irqlock, flags);
			break;
		}

		video->encode(req, video, buf);

		/* Queue the USB request */
		ret = uvcg_video_ep_queue(video, req);
		spin_unlock_irqrestore(&queue->irqlock, flags);

		if (ret < 0) {
			uvcg_queue_cancel(queue, 0);
			break;
		}
	}

	spin_lock_irqsave(&video->req_lock, flags);
	list_add_tail(&req->list, &video->req_free);
	spin_unlock_irqrestore(&video->req_lock, flags);
	return;
}
/*
 * Enable or disable the video stream.
 */
int uvcg_video_enable(struct uvc_video *video, int enable)
{
	unsigned int i;
	int ret;

	if (video->ep == NULL) {
		printk(KERN_INFO "Video enable failed, device is "
			"uninitialized.\n");
		return -ENODEV;
	}

	if (!enable) {
		cancel_work_sync(&video->pump);
		uvcg_queue_cancel(&video->queue, 0);
		for (i = 0; i < UVC_NUM_REQUESTS; ++i)
			if (video->req[i])
				usb_ep_dequeue(video->ep, video->req[i]);

		uvc_video_free_requests(video);
		uvcg_queue_enable(&video->queue, 0);
#ifdef CONFIG_USB_CONFIGFS_F_UVC_BULK_TRANSFER
		if(video->ep->enabled)
		{
			usb_ep_disable(video->ep);
			usb_ep_enable(video->ep);
		}
#endif
		return 0;
	}

	if ((ret = uvcg_queue_enable(&video->queue, 1)) < 0)
		return ret;

	if ((ret = uvc_video_alloc_requests(video)) < 0)
		return ret;

	if (video->max_payload_size) {
		video->encode = uvc_video_encode_bulk;
		video->payload_size = 0;
	} else
		video->encode = uvc_video_encode_isoc;

	//return uvcg_video_pump(video);
	schedule_work(&video->pump);

	return ret;
}

/*
 * Initialize the UVC video stream.
 */
int uvcg_video_init(struct uvc_video *video, struct uvc_device *uvc)
{
	INIT_LIST_HEAD(&video->req_free);
	spin_lock_init(&video->req_lock);
	INIT_WORK(&video->pump, uvcg_video_pump);

	video->uvc = uvc;
	video->fcc = V4L2_PIX_FMT_YUYV;
	video->bpp = 16;
	video->width = 320;
	video->height = 240;
	video->imagesize = 320 * 240 * 2;

	/* Initialize the video buffers queue. */
	uvcg_queue_init(&video->queue, V4L2_BUF_TYPE_VIDEO_OUTPUT,
			&video->mutex);
	return 0;
}

