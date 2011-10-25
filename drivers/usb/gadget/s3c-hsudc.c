/* linux/drivers/usb/gadget/s3c-hsudc.c
 *
 * Copyright (c) 2010 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com/
 *
 * S3C USB2.0 High-speed USB controller gadget driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/spinlock.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/dma-mapping.h>
#include <linux/delay.h>
#include <linux/io.h>
#include <linux/slab.h>
#include <linux/clk.h>

#include <linux/usb/ch9.h>
#include <linux/usb/gadget.h>

#define S3C_HSUDC_REG(x)	(x)

/* Non-Indexed Registers */
#define S3C_IR			S3C_HSUDC_REG(0x00)
#define S3C_EIR			S3C_HSUDC_REG(0x04)
#define S3C_EIER		S3C_HSUDC_REG(0x08)
#define S3C_FAR			S3C_HSUDC_REG(0x0c)
#define S3C_FNR			S3C_HSUDC_REG(0x10)
#define S3C_EDR			S3C_HSUDC_REG(0x14)
#define S3C_TR			S3C_HSUDC_REG(0x18)
#define S3C_SSR			S3C_HSUDC_REG(0x1c)
#define S3C_SCR			S3C_HSUDC_REG(0x20)
#define S3C_EP0SR		S3C_HSUDC_REG(0x24)
#define S3C_EP0CR		S3C_HSUDC_REG(0x28)
#define S3C_BR(_x)		S3C_HSUDC_REG(0x60 + (_x * 4))

/* Indexed Registers */
#define S3C_ESR			S3C_HSUDC_REG(0x2c)
#define S3C_ECR			S3C_HSUDC_REG(0x30)
#define S3C_ECR_DUEN		(1 << 7)
#define S3C_ECR_IEMS		(1 << 0)

#define S3C_BRCR		S3C_HSUDC_REG(0x34)
#define S3C_BWCR		S3C_HSUDC_REG(0x38)
#define S3C_MPR			S3C_HSUDC_REG(0x3c)

/* EP interrupt register Bits */
#define S3C_UDC_INT_EP3		(1<<3)
#define S3C_UDC_INT_EP2		(1<<2)
#define S3C_UDC_INT_EP1		(1<<1)
#define S3C_UDC_INT_EP0		(1<<0)

/* System status register Bits */
#define S3C_UDC_INT_CHECK	(0xff8f)
#define S3C_UDC_INT_ERR		(0xff80)
#define S3C_UDC_INT_VBUSON	(1<<8)
#define S3C_UDC_INT_HSP		(1<<4)
#define S3C_UDC_INT_SDE		(1<<3)
#define S3C_UDC_INT_RESUME	(1<<2)
#define S3C_UDC_INT_SUSPEND	(1<<1)
#define S3C_UDC_INT_RESET	(1<<0)

/* system control register Bits */
#define S3C_UDC_DTZIEN_EN	(1<<14)
#define S3C_UDC_RRD_EN		(1<<5)
#define S3C_UDC_SUS_EN		(1<<1)
#define S3C_UDC_RST_EN		(1<<0)

/* EP0 status register Bits */
#define S3C_UDC_EP0_LWO		(1<<6)
#define S3C_UDC_EP0_STALL	(1<<4)
#define S3C_UDC_EP0_TX_SUCCESS	(1<<1)
#define S3C_UDC_EP0_RX_SUCCESS	(1<<0)

/* EP status register Bits */
#define S3C_UDC_EP_FPID		(1<<11)
#define S3C_UDC_EP_OSD 		(1<<10)
#define S3C_UDC_EP_DTCZ		(1<<9)
#define S3C_UDC_EP_SPT		(1<<8)

#define S3C_UDC_EP_DOM		(1<<7)
#define S3C_UDC_EP_FIFO_FLUSH	(1<<6)
#define S3C_UDC_EP_STALL	(1<<5)
#define S3C_UDC_EP_LWO		(1<<4)
#define S3C_UDC_EP_PSIF_ONE	(1<<2)
#define S3C_UDC_EP_PSIF_TWO	(2<<2)
#define S3C_UDC_EP_TX_SUCCESS	(1<<1)
#define S3C_UDC_EP_RX_SUCCESS	(1<<0)

#define WAIT_FOR_SETUP          0
#define DATA_STATE_XMIT         1
#define DATA_STATE_NEED_ZLP     2
#define WAIT_FOR_OUT_STATUS     3
#define DATA_STATE_RECV         4

#define S3C_HSUDC_EPS		(9)

extern void s3c2416_hsudc_gpio_setup(void);

struct s3c_ep {
	struct usb_ep ep;
	char name[20];
	struct s3c_udc *dev;

	const struct usb_endpoint_descriptor *desc;
	struct list_head queue;
	unsigned long pio_irqs;

	u8 stopped;
	u8 bEndpointAddress;
	u32 fifo;
};

struct s3c_request {
	struct usb_request req;
	struct list_head queue;
};

struct s3c_udc {
	struct usb_gadget gadget;
	struct usb_gadget_driver *driver;
	struct device *dev;
	spinlock_t lock;

	void __iomem *regs;
	struct resource *mem_rsrc;
	int irq;
	struct clk *uclk;

	int ep0state;
	unsigned char usb_address;
	struct s3c_ep ep[S3C_HSUDC_EPS];
};

extern struct s3c_udc *the_controller;

#define ep_is_in(_ep)		((_ep)->bEndpointAddress & USB_DIR_IN)
#define ep_index(_ep) 		((_ep)->bEndpointAddress&0xF)
#define ep_maxpacket(_ep) 	((_ep)->ep.maxpacket)

struct s3c_udc	*the_controller;

static const char driver_name[] = "s3c-udc";
static const char ep0name[] = "ep0-control";

static inline struct s3c_request *our_req(struct usb_request *req)
{
	return container_of(req, struct s3c_request, req);
}

static inline struct s3c_ep *our_ep(struct usb_ep *ep)
{
	return container_of(ep, struct s3c_ep, ep);
}

static inline struct s3c_udc *to_hsudc(struct usb_gadget *gadget)
{
	return container_of(gadget, struct s3c_udc, gadget);
}

static inline void set_index(struct s3c_udc *hsudc, int ep_addr)
{
	ep_addr &= USB_ENDPOINT_NUMBER_MASK;
	__raw_writel(ep_addr, hsudc->regs + S3C_IR);
}

static inline void __orr32(void __iomem *ptr, u32 val)
{
	writel(readl(ptr) | val, ptr);
}

static void s3c_hsudc_complete_request(struct s3c_ep *ep, struct s3c_request *req, int status)
{
	unsigned int stopped = ep->stopped;
	struct s3c_udc *hsudc = ep->dev;

	list_del_init(&req->queue);
	
	if (likely(req->req.status == -EINPROGRESS))
		req->req.status = status;
	else
		status = req->req.status;

	if (!ep_index(ep)) {
		hsudc->ep0state = WAIT_FOR_SETUP;
		ep->bEndpointAddress &= ~USB_DIR_IN;
	}

	ep->stopped = 1;
	spin_unlock(&ep->dev->lock);
	req->req.complete(&ep->ep, &req->req);
	spin_lock(&ep->dev->lock);
	ep->stopped = stopped;
}

void s3c_hsudc_nuke_ep(struct s3c_ep *ep, int status)
{
	struct s3c_request *req;

	while (!list_empty(&ep->queue)) {
		req = list_entry(ep->queue.next, struct s3c_request, queue);
		s3c_hsudc_complete_request(ep, req, status);
	}
}

static int s3c_hsudc_write_packet(struct s3c_ep *hs_ep, 
			struct s3c_request *hs_req, int max)
{
	u16 *buf;
	int length, count;
	u32 fifo = hs_ep->fifo;

	buf = hs_req->req.buf + hs_req->req.actual;
	prefetch(buf);

	length = hs_req->req.length - hs_req->req.actual;
	length = min(length, max);
	hs_req->req.actual += length;

	__raw_writel(length, hs_ep->dev->regs + S3C_BWCR);
	for (count = 0; count < length; count += 2)
		__raw_writel(*buf++, fifo);
	
	return length;
}

static int s3c_hsudc_write_fifo(struct s3c_ep *ep, struct s3c_request *req)
{
	u32 max;
	unsigned count;
	int is_last = 0, is_short = 0;

	max = ep_maxpacket(ep);
	count = s3c_hsudc_write_packet(ep, req, max);

	/* last packet is usually short (or a zlp) */
	if (unlikely(count != max))
		is_last = is_short = 1;
	else {
		if (likely(req->req.length != req->req.actual) || req->req.zero)
			is_last = 0;
		else
			is_last = 1;

		/* interrupt/iso maxpacket may not fill the fifo */
		is_short = unlikely(max < ep_maxpacket(ep));
	}
	
	/* requests complete when all IN data is in the FIFO */
	if (is_last) {
		s3c_hsudc_complete_request(ep, req, 0);
		return 1;
	}

	return 0;
}

static int s3c_hsudc_read_fifo(struct s3c_ep *ep, struct s3c_request *req)
{
	struct s3c_udc *hsudc = ep->dev;
	u32 csr;
	u16 *buf;
	unsigned bufferspace, count, count_bytes, is_short = 0;
	u32 fifo = ep->fifo;
	u32 offset;

	offset = (ep_index(ep)) ? S3C_ESR : S3C_EP0SR;
	csr = __raw_readl(hsudc->regs + offset);

	/* make sure there's a packet in the FIFO. */
	if (!(csr & S3C_UDC_EP_RX_SUCCESS))
		return -EINVAL;

	buf = req->req.buf + req->req.actual;
	prefetchw(buf);
	bufferspace = req->req.length - req->req.actual;

	/* read all bytes from this packet */
	count = __raw_readl(hsudc->regs + S3C_BRCR);
	if (csr & S3C_UDC_EP_LWO)
		count_bytes = count * 2 - 1;
	else
		count_bytes = count * 2;

	req->req.actual += min(count_bytes, bufferspace);
	is_short = (count_bytes < ep->ep.maxpacket);

	while (likely(count-- != 0)) {
		u16 byte = (u16) __raw_readl(fifo);

		if (unlikely(bufferspace == 0)) {
			/* this happens when the driver's buffer
		 	* is smaller than what the host sent.
		 	* discard the extra data.
		 	*/
			if (req->req.status != -EOVERFLOW)
				printk("%s overflow %d\n", ep->ep.name, count);
			req->req.status = -EOVERFLOW;
		} else {
			*buf++ = byte;
			bufferspace--;
		}
	}

	__orr32(hsudc->regs + offset, S3C_UDC_EP_RX_SUCCESS);

	/* completion */
	if (is_short || req->req.actual == req->req.length) {
		s3c_hsudc_complete_request(ep, req, 0);
		return 1;
	}

	/* finished that packet.  the next one may be waiting... */
	return 0;
}

static void s3c_hsudc_epin_intr(struct s3c_udc *dev, u32 ep_idx)
{
	u32 csr;
	struct s3c_ep *ep = &dev->ep[ep_idx];
	struct s3c_request *req;

	csr = __raw_readl((u32)dev->regs + S3C_ESR);
	if (csr & S3C_UDC_EP_STALL) {
		csr = __raw_readl(dev->regs + S3C_ESR) | S3C_UDC_EP_STALL;
		__raw_writel(csr, dev->regs + S3C_ESR);
		return;
	}
	
	if (csr & S3C_UDC_EP_TX_SUCCESS) {
		__orr32(dev->regs + S3C_ESR, S3C_UDC_EP_TX_SUCCESS);
		if (list_empty(&ep->queue))
			return;

		req = list_entry(ep->queue.next, struct s3c_request, queue);
		if ((s3c_hsudc_write_fifo(ep, req) == 0) && (csr & S3C_UDC_EP_PSIF_TWO))
			s3c_hsudc_write_fifo(ep, req);
	}
}

static void s3c_hsudc_epout_intr(struct s3c_udc *dev, u32 ep_idx)
{
	struct s3c_ep *ep = &dev->ep[ep_idx];
	struct s3c_request *req;
	u32 csr;
	
	csr = __raw_readl((u32)dev->regs + S3C_ESR);
	if (csr & S3C_UDC_EP_STALL) {
		__orr32(dev->regs + S3C_ESR, S3C_UDC_EP_STALL);
		return;
	}

	if (csr & S3C_UDC_EP_FIFO_FLUSH) {
		__orr32(dev->regs + S3C_ECR, S3C_UDC_EP_FIFO_FLUSH);
		return;
	}

	if (csr & S3C_UDC_EP_RX_SUCCESS) {
		if (list_empty(&ep->queue))
			return;
	
		req = list_entry(ep->queue.next, struct s3c_request, queue);
		if(((s3c_hsudc_read_fifo(ep, req)) == 0) && (csr & S3C_UDC_EP_PSIF_TWO))
			s3c_hsudc_read_fifo(ep, req);
	}
}

static void s3c_hsudc_stop_activity(struct s3c_udc *hsudc,
			  struct usb_gadget_driver *driver)
{
	struct s3c_ep *hs_ep;
	int epnum;

	hsudc->gadget.speed = USB_SPEED_UNKNOWN;
	
	for (epnum = 0; epnum < S3C_HSUDC_EPS; epnum++) {
		hs_ep = &hsudc->ep[epnum];
		hs_ep->stopped = 1;
		s3c_hsudc_nuke_ep(hs_ep, -ESHUTDOWN);
	}

	spin_unlock(&hsudc->lock);
	driver->disconnect(&hsudc->gadget);
	spin_lock(&hsudc->lock);
}

static void s3c_hsudc_reconfig(struct s3c_udc *hsudc)
{
	__raw_writel(0xAA, hsudc->regs + S3C_EDR);
	__raw_writel(1, hsudc->regs + S3C_EIER);
	__raw_writel(0, hsudc->regs + S3C_TR);
	__raw_writel(S3C_UDC_DTZIEN_EN | S3C_UDC_RRD_EN | S3C_UDC_SUS_EN |
			S3C_UDC_RST_EN,	hsudc->regs + S3C_SCR);
	__raw_writel(0, hsudc->regs + S3C_EP0CR);
}

static void s3c_hsudc_read_setup_pkt(struct s3c_ep *hs_ep, u16 *buf)
{
	struct s3c_udc *hsudc = hs_ep->dev;
	int count;
	
	count = __raw_readl(hsudc->regs + S3C_BRCR);
	while (count--)
		*buf++ = (u16)__raw_readl(hsudc->regs + S3C_BR(0));
	
	__raw_writel(S3C_UDC_EP0_RX_SUCCESS, hsudc->regs + S3C_EP0SR);
}

static void s3c_hsudc_set_address(struct s3c_udc *hsudc, unsigned char address)
{
	hsudc->usb_address = address;
	hsudc->ep0state = WAIT_FOR_SETUP;
}

static int s3c_hsudc_set_halt(struct usb_ep *_ep, int value)
{
	return 0;
}

static int s3c_hsudc_handle_reqfeat(struct s3c_udc *hsudc,
					struct usb_ctrlrequest *ctrl)
{
	struct s3c_ep *hs_ep;
	bool set = (ctrl->bRequest == USB_REQ_SET_FEATURE);
	u8 ep_num = ctrl->wIndex & USB_ENDPOINT_NUMBER_MASK;

	if (ctrl->bRequestType == USB_RECIP_ENDPOINT) {
		hs_ep = &hsudc->ep[ep_num];
		switch (le16_to_cpu(ctrl->wValue)) {
		case USB_ENDPOINT_HALT:
			s3c_hsudc_set_halt(&hs_ep->ep, set);
			return 0;
		}
	}

	return -ENOENT;
}

static void s3c_hsudc_process_setup(struct s3c_udc *hsudc, u32 csr)
{
	struct s3c_ep *hs_ep = &hsudc->ep[0];
	struct usb_ctrlrequest ctrl = {0};
	int ret;

	/* Terminate all previous control transfers */
	s3c_hsudc_nuke_ep(hs_ep, -EPROTO);

	/* Read control req from ep0 fifo (8 bytes) */
	s3c_hsudc_read_setup_pkt(hs_ep, (u16 *)&ctrl);

	/* Set direction of EP0 */
	if (likely(ctrl.bRequestType & USB_DIR_IN)) {
		hs_ep->bEndpointAddress |= USB_DIR_IN;
		hsudc->ep0state = DATA_STATE_XMIT;
	} else {
		hs_ep->bEndpointAddress &= ~USB_DIR_IN;
		hsudc->ep0state = DATA_STATE_RECV;
	}

	switch (ctrl.bRequest) {
	case USB_REQ_SET_ADDRESS:
		if (ctrl.bRequestType != (USB_TYPE_STANDARD | USB_RECIP_DEVICE))
			break;
		s3c_hsudc_set_address(hsudc, ctrl.wValue);
		return;

	case USB_REQ_SET_FEATURE:
	case USB_REQ_CLEAR_FEATURE:
		if (!s3c_hsudc_handle_reqfeat(hsudc, &ctrl))
			return;
		break;
	}

	if (likely(hsudc->driver)) {
		spin_unlock(&hsudc->lock);
		ret = hsudc->driver->setup(&hsudc->gadget, &ctrl);
		spin_lock(&hsudc->lock);

		if (ret < 0) {
			dev_err(hsudc->dev, "setup failed, returned %d\n",
						ret);
			hsudc->ep0state = WAIT_FOR_SETUP;
		}
	}
}

static void s3c_hsudc_handle_ep0_intr(struct s3c_udc *hsudc)
{
	struct s3c_ep *hs_ep = &hsudc->ep[0];
	struct s3c_request *req;
	u32 csr = __raw_readl(hsudc->regs + S3C_EP0SR);

	if (csr & S3C_UDC_EP0_STALL) {
		__raw_writel(S3C_UDC_EP0_STALL, hsudc->regs + S3C_EP0SR);
		s3c_hsudc_nuke_ep(hs_ep, -ECONNABORTED);
		hsudc->ep0state = WAIT_FOR_SETUP;
		return;
	}
		
	if (csr & S3C_UDC_EP0_TX_SUCCESS) {
		__raw_writel(S3C_UDC_EP0_TX_SUCCESS, hsudc->regs + S3C_EP0SR);
		if (ep_is_in(hs_ep)) {
			if (list_empty(&hs_ep->queue))
				return;

			req = list_entry(hs_ep->queue.next, struct s3c_request, queue);
			s3c_hsudc_write_fifo(hs_ep, req);
		}
	}

	if (csr & S3C_UDC_EP0_RX_SUCCESS) {
		if (hsudc->ep0state == WAIT_FOR_SETUP)
			s3c_hsudc_process_setup(hsudc, csr);
		else {
			if (!ep_is_in(hs_ep)) {
				if (list_empty(&hs_ep->queue))
					return;
				req = list_entry(hs_ep->queue.next, 
					struct s3c_request, queue);
				s3c_hsudc_read_fifo(hs_ep, req);
			}
		}
	}
}

static irqreturn_t s3c_hsudc_irq(int irq, void *_dev)
{
	struct s3c_udc *dev = _dev;
	u32 intr_out;
	u32 intr_in;
	u32 intr_status, intr_status_chk;
	
	spin_lock(&dev->lock);

	intr_status = __raw_readl(dev->regs + S3C_SSR);
	intr_out = intr_in = __raw_readl(dev->regs + S3C_EIR);

	/* We have only 3 usable eps now */
	intr_status_chk = intr_status & S3C_UDC_INT_CHECK;
	intr_in &= (S3C_UDC_INT_EP1 | S3C_UDC_INT_EP0 | S3C_UDC_INT_EP3);
	intr_out &= S3C_UDC_INT_EP2;

	if (!intr_out && !intr_in && !intr_status_chk){
		spin_unlock(&dev->lock);
		return IRQ_HANDLED;
	}

	if (intr_status) {
		if (intr_status & S3C_UDC_INT_VBUSON) {
			__raw_writel(S3C_UDC_INT_VBUSON, dev->regs + S3C_SSR);
		}

		if (intr_status & S3C_UDC_INT_ERR) {
			__raw_writel(S3C_UDC_INT_ERR, dev->regs + S3C_SSR);
		}

		if (intr_status & S3C_UDC_INT_SDE) {
			__raw_writel(S3C_UDC_INT_SDE, dev->regs + S3C_SSR);

			if (intr_status & S3C_UDC_INT_HSP) {
				dev->gadget.speed = USB_SPEED_HIGH;
			}
			else {
				dev->gadget.speed = USB_SPEED_FULL;
			}
		}
		
		if (intr_status & S3C_UDC_INT_SUSPEND) {
			__raw_writel(S3C_UDC_INT_SUSPEND, dev->regs + S3C_SSR);
			if (dev->gadget.speed != USB_SPEED_UNKNOWN
			    && dev->driver
			    && dev->driver->suspend) {
				dev->driver->suspend(&dev->gadget);
			}
		}

		if (intr_status & S3C_UDC_INT_RESUME) {
			__raw_writel(S3C_UDC_INT_RESUME, dev->regs + S3C_SSR);
			if (dev->gadget.speed != USB_SPEED_UNKNOWN
			    && dev->driver
			    && dev->driver->resume) {
				dev->driver->resume(&dev->gadget);
			}
		}

		if (intr_status & S3C_UDC_INT_RESET) {
			__raw_writel(S3C_UDC_INT_RESET, dev->regs + S3C_SSR);
			s3c_hsudc_reconfig(dev);
			dev->ep0state = WAIT_FOR_SETUP;
		}
	}

	if (intr_in) {
		if (intr_in & S3C_UDC_INT_EP0){
			set_index(dev, 0);
			__raw_writel(S3C_UDC_INT_EP0, dev->regs + S3C_EIR);
			s3c_hsudc_handle_ep0_intr(dev);
		}else if (intr_in & S3C_UDC_INT_EP1){
			set_index(dev, 1);
			__raw_writel(S3C_UDC_INT_EP1, dev->regs + S3C_EIR);
			s3c_hsudc_epin_intr(dev, 1);	// hard coded !!!   
		}else{
			set_index(dev, 3);
			__raw_writel(S3C_UDC_INT_EP3, dev->regs + S3C_EIR);
			s3c_hsudc_epin_intr(dev, 3);	// hard coded !!!   
		}
	}

	if (intr_out) {
		set_index(dev, 2);
		__raw_writel(S3C_UDC_INT_EP2, (u32)dev->regs + S3C_EIR);
		s3c_hsudc_epout_intr(dev, 2);		// hard coded !!!   
	}
	
	spin_unlock(&dev->lock);
	return IRQ_HANDLED;
}

static int s3c_hsudc_ep_enable(struct usb_ep *_ep,
			     const struct usb_endpoint_descriptor *desc)
{
	struct s3c_ep *ep;
	struct s3c_udc *dev;
	unsigned long flags;
	u32 ecr = 0;

	ep = container_of(_ep, struct s3c_ep, ep);
	if (!_ep || !desc || ep->desc || _ep->name == ep0name
	    || desc->bDescriptorType != USB_DT_ENDPOINT
	    || ep->bEndpointAddress != desc->bEndpointAddress
	    || ep_maxpacket(ep) < le16_to_cpu(desc->wMaxPacketSize))
		return -EINVAL;

	/* hardware _could_ do smaller, but driver doesn't */
	if ((desc->bmAttributes == USB_ENDPOINT_XFER_BULK
	     && le16_to_cpu(desc->wMaxPacketSize) != ep_maxpacket(ep))
	    || !desc->wMaxPacketSize)
		return -ERANGE;

	dev = ep->dev;
	if (!dev->driver || dev->gadget.speed == USB_SPEED_UNKNOWN)
		return -ESHUTDOWN;

	spin_lock_irqsave(&ep->dev->lock, flags);

	set_index(dev, ep->bEndpointAddress);
	
	if (usb_endpoint_xfer_int(desc))
		ecr |= S3C_ECR_IEMS;
	else
		ecr |= S3C_ECR_DUEN;

	writel(ecr, dev->regs + S3C_ECR);

	ep->stopped = 0;
	ep->desc = desc;
	ep->pio_irqs = 0;
	ep->ep.maxpacket = le16_to_cpu(desc->wMaxPacketSize);

	/* Reset halt state */
	s3c_hsudc_set_halt(_ep, 0);
	__set_bit(ep_index(ep), dev->regs + S3C_EIER);

	spin_unlock_irqrestore(&ep->dev->lock, flags);
	return 0;
}

static int s3c_hsudc_ep_disable(struct usb_ep *_ep)
{
	struct s3c_ep *ep = our_ep(_ep);
	struct s3c_udc *hsudc = ep->dev;
	unsigned long flags;

	ep = container_of(_ep, struct s3c_ep, ep);
	if (!_ep || !ep->desc)
		return -EINVAL;

	spin_lock_irqsave(&ep->dev->lock, flags);

	set_index(hsudc, ep->bEndpointAddress);
	__clear_bit(ep_index(ep), hsudc->regs + S3C_EIER);

	/* Nuke all pending requests */
	s3c_hsudc_nuke_ep(ep, -ESHUTDOWN);

	ep->desc = 0;
	ep->stopped = 1;

	spin_unlock_irqrestore(&ep->dev->lock, flags);
	return 0;
}

static struct usb_request *s3c_hsudc_alloc_request(struct usb_ep *ep,
						 gfp_t gfp_flags)
{
	struct s3c_request *req;

	req = kmalloc(sizeof *req, gfp_flags);
	if (!req)
		return 0;

	memset(req, 0, sizeof *req);
	INIT_LIST_HEAD(&req->queue);

	return &req->req;
}

static void s3c_hsudc_free_request(struct usb_ep *ep, struct usb_request *_req)
{
	struct s3c_request *req;

	req = container_of(_req, struct s3c_request, req);
	WARN_ON(!list_empty(&req->queue));
	kfree(req);
}

static int s3c_hsudc_queue(struct usb_ep *_ep, struct usb_request *_req,
			 gfp_t gfp_flags)
{
	struct s3c_request *req;
	struct s3c_ep *ep;
	struct s3c_udc *dev;
	unsigned long flags;
	u32 offset;
	u32 csr;

	req = container_of(_req, struct s3c_request, req);
	if (unlikely
	    (!_req || !_req->complete || !_req->buf
	     || !list_empty(&req->queue)))
		return -EINVAL;

	ep = container_of(_ep, struct s3c_ep, ep);
	dev = ep->dev;
	if (unlikely(!dev->driver || dev->gadget.speed == USB_SPEED_UNKNOWN)) {
		return -ESHUTDOWN;
	}

	spin_lock_irqsave(&dev->lock, flags);
	set_index(dev, ep->bEndpointAddress);

	_req->status = -EINPROGRESS;
	_req->actual = 0;

	if (!ep_index(ep)&& _req->length == 0) {
		dev->ep0state = WAIT_FOR_SETUP;
		__raw_writel(S3C_UDC_EP0_RX_SUCCESS, dev->regs + S3C_EP0SR);
		s3c_hsudc_complete_request(ep, req, 0);
		return 0;
	}

	/* kickstart this i/o queue? */
	if (list_empty(&ep->queue) && likely(!ep->stopped)) {
		offset = (ep_index(ep)) ? S3C_ESR : S3C_EP0SR;
		if (ep_is_in(ep)) {
			csr = __raw_readl((u32)dev->regs + offset);
			if(!(csr & S3C_UDC_EP_TX_SUCCESS) &&
				(s3c_hsudc_write_fifo(ep, req) == 1))
				req = 0;
		} else {
			csr = __raw_readl((u32)dev->regs + offset);
			if ((csr & S3C_UDC_EP_RX_SUCCESS)
				   && (s3c_hsudc_read_fifo(ep, req) == 1))
				req = 0;
		}
	}

	/* pio or dma irq handler advances the queue. */
	if (likely(req != 0)) 
		list_add_tail(&req->queue, &ep->queue);

	spin_unlock_irqrestore(&dev->lock, flags);

	return 0;
}

static int s3c_hsudc_dequeue(struct usb_ep *_ep, struct usb_request *_req)
{
	struct s3c_ep *ep = our_ep(_ep);
	struct s3c_udc *hsudc = ep->dev;
	struct s3c_request *req;
	unsigned long flags;

	ep = container_of(_ep, struct s3c_ep, ep);
	if (!_ep || ep->ep.name == ep0name)
		return -EINVAL;

	spin_lock_irqsave(&ep->dev->lock, flags);

	/* make sure it's actually queued on this endpoint */
	list_for_each_entry(req, &ep->queue, queue) {
		if (&req->req == _req)
			break;
	}
	if (&req->req != _req) {
		spin_unlock_irqrestore(&ep->dev->lock, flags);
		return -EINVAL;
	}

	set_index(hsudc, ep->bEndpointAddress);
	s3c_hsudc_complete_request(ep, req, -ECONNRESET);

	spin_unlock_irqrestore(&ep->dev->lock, flags);
	return 0;
}

static struct usb_ep_ops s3c_ep_ops = {
	.enable = s3c_hsudc_ep_enable,
	.disable = s3c_hsudc_ep_disable,
	.alloc_request = s3c_hsudc_alloc_request,
	.free_request = s3c_hsudc_free_request,
	.queue = s3c_hsudc_queue,
	.dequeue = s3c_hsudc_dequeue,
	.set_halt = s3c_hsudc_set_halt,
};

int usb_gadget_register_driver(struct usb_gadget_driver *driver)
{
	struct s3c_udc *hsudc = the_controller;
	int ret;

	if (!driver
	    || (driver->speed != USB_SPEED_FULL &&
		driver->speed != USB_SPEED_HIGH)
	    || !driver->bind
	    || !driver->unbind || !driver->disconnect || !driver->setup)
		return -EINVAL;

	if (!hsudc)
		return -ENODEV;

	if (hsudc->driver)
		return -EBUSY;

	hsudc->driver = driver;
	hsudc->gadget.dev.driver = &driver->driver;
	hsudc->gadget.speed = USB_SPEED_UNKNOWN;
	ret = device_add(&hsudc->gadget.dev);
	if (ret) {
		dev_err(hsudc->dev, "failed to register gadget device");
		return ret;
	}

	ret = driver->bind(&hsudc->gadget);
	if (ret) {
		dev_err(hsudc->dev, "%s: bind failed\n", hsudc->gadget.name);
		device_del(&hsudc->gadget.dev);

		hsudc->driver = NULL;
		hsudc->gadget.dev.driver = NULL;
		return ret;
	}

	enable_irq(hsudc->irq);
	dev_info(hsudc->dev, "bound driver %s\n", driver->driver.name);

	s3c_hsudc_reconfig(hsudc);
	s3c2416_hsudc_gpio_setup();
	return 0;
}

EXPORT_SYMBOL(usb_gadget_register_driver);

int usb_gadget_unregister_driver(struct usb_gadget_driver *driver)
{
	struct s3c_udc *hsudc = the_controller;
	unsigned long flags;

	if (!hsudc)
		return -ENODEV;

	if (!driver || driver != hsudc->driver || !driver->unbind)
		return -EINVAL;

	spin_lock_irqsave(&hsudc->lock, flags);
	hsudc->driver = 0;
	s3c_hsudc_stop_activity(hsudc, driver);
	spin_unlock_irqrestore(&hsudc->lock, flags);

	driver->unbind(&hsudc->gadget);
	device_del(&hsudc->gadget.dev);
	disable_irq(hsudc->irq);

	dev_info(hsudc->dev, "unregistered gadget driver '%s'",
			driver->driver.name);
	return 0;
}

EXPORT_SYMBOL(usb_gadget_unregister_driver);

static inline u32 s3c_hsudc_read_frameno(struct s3c_udc *hsudc)
{
	return readl(hsudc->regs + S3C_FNR) & 0x3FF;
}

static int s3c_hsudc_gadget_getframe(struct usb_gadget *gadget)
{
	return s3c_hsudc_read_frameno(to_hsudc(gadget));
}

static struct usb_gadget_ops s3c_hsudc_gadget_ops = {
	.get_frame	= s3c_hsudc_gadget_getframe,
};

static void s3c_hsudc_initep(struct s3c_udc *hsudc,
				struct s3c_ep *hs_ep, int epnum)
{
	char *dir;

	if ((epnum % 2) == 0) {
		dir = "out";
	} else {
		dir = "in";
		hs_ep->bEndpointAddress = USB_DIR_IN;
	}

	hs_ep->bEndpointAddress |= epnum;
	if (epnum)
		snprintf(hs_ep->name, sizeof(hs_ep->name), "ep%d%s", epnum, dir);
	else
		snprintf(hs_ep->name, sizeof(hs_ep->name), "%s", ep0name);

	INIT_LIST_HEAD(&hs_ep->queue);
	INIT_LIST_HEAD(&hs_ep->ep.ep_list);
	if (epnum)
		list_add_tail(&hs_ep->ep.ep_list, &hsudc->gadget.ep_list);

	hs_ep->dev = hsudc;
	hs_ep->ep.name = hs_ep->name;
	hs_ep->ep.maxpacket = epnum ? 512 : 64;
	hs_ep->ep.ops = &s3c_ep_ops;
	hs_ep->fifo = (u32)(hsudc->regs + S3C_BR(epnum));
	hs_ep->desc = 0;
	hs_ep->stopped = 0;

	set_index(hsudc, epnum);
	__raw_writel(hs_ep->ep.maxpacket, hsudc->regs + S3C_MPR);
}

static void s3c_hsudc_reinit(struct s3c_udc *dev)
{
	u32 epnum;

	INIT_LIST_HEAD(&dev->gadget.ep_list);
	dev->ep0state = WAIT_FOR_SETUP;
	for (epnum = 0; epnum < S3C_HSUDC_EPS; epnum++)
		s3c_hsudc_initep(dev, &dev->ep[epnum], epnum);
}

static int s3c_hsudc_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct resource *res;
	struct s3c_udc *hsudc;
	int ret;

	hsudc = kzalloc(sizeof(struct s3c_udc) +
			sizeof(struct s3c_ep) * S3C_HSUDC_EPS,
			GFP_KERNEL);
	if (!hsudc) {
		dev_err(dev, "cannot allocate memory\n");
		return -ENOMEM;
	}

	the_controller = hsudc;
	platform_set_drvdata(pdev, dev);
	hsudc->dev = dev;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res) {
		dev_err(dev, "unable to obtain driver resource data\n");
		ret = -ENODEV;
		goto err_res;
	} 

	hsudc->mem_rsrc = request_mem_region(res->start, resource_size(res),
				dev_name(&pdev->dev));
	if (!hsudc->mem_rsrc) {
		dev_err(dev, "failed to reserve register area\n");
		ret = -ENODEV;
		goto err_res;
	} 

	hsudc->regs = ioremap(res->start, resource_size(res));
	if (!hsudc->regs) {
		dev_err(dev, "error mapping device register area\n");
		ret = -EBUSY;
		goto err_remap;
	}

	ret = platform_get_irq(pdev, 0);
	if (ret < 0) {
		dev_err(dev, "unable to obtain IRQ number\n");
		goto err_irq;
	}
	hsudc->irq = ret;

	ret = request_irq(hsudc->irq, s3c_hsudc_irq, 0, driver_name, hsudc);
	if (ret < 0) {
		dev_err(dev, "irq request failed\n");
		goto err_irq;
	}

	spin_lock_init(&hsudc->lock);

	device_initialize(&hsudc->gadget.dev);
	dev_set_name(&hsudc->gadget.dev, "gadget");
	
	hsudc->gadget.is_dualspeed = 1;
	hsudc->gadget.ops = &s3c_hsudc_gadget_ops;
	hsudc->gadget.name = dev_name(dev);
	hsudc->gadget.dev.parent = dev;
	hsudc->gadget.dev.dma_mask = dev->dma_mask;
	hsudc->gadget.ep0 = &hsudc->ep[0].ep;

	hsudc->gadget.is_otg = 0;
	hsudc->gadget.is_a_peripheral = 0;
	hsudc->gadget.b_hnp_enable = 0;
	hsudc->gadget.a_hnp_support = 0;
	hsudc->gadget.a_alt_hnp_support = 0;

	s3c_hsudc_reinit(hsudc);

	hsudc->uclk = clk_get(&pdev->dev, "usb-device");
	if (hsudc->uclk == NULL) {
		dev_err(dev, "failed to find usb-device clock source\n");
		return -ENOENT;
	}
	clk_enable(hsudc->uclk);

	local_irq_disable();

	disable_irq(hsudc->irq);
	local_irq_enable();
	return 0;

err_irq:
	iounmap(hsudc->regs);

err_remap:
	release_resource(hsudc->mem_rsrc);
	kfree(hsudc->mem_rsrc);

err_res:
	kfree(hsudc);
	return ret;
}

static struct platform_driver s3c_hsudc_driver = {
	.driver		= {
		.owner	= THIS_MODULE,
		.name	= "s3c-hsudc",
	},
	.probe		= s3c_hsudc_probe,
};

static int __init s3c_hsudc_modinit(void)
{
	return platform_driver_register(&s3c_hsudc_driver);
}

static void __exit s3c_hsudc_modexit(void)
{
	platform_driver_unregister(&s3c_hsudc_driver);
}

module_init(s3c_hsudc_modinit);
module_exit(s3c_hsudc_modexit);
