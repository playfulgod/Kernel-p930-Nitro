/*
 * u_sdio.c - utilities for USB gadget serial over sdio
 *
 * This code also borrows from drivers/usb/gadget/u_serial.c, which is
 * Copyright (C) 2003 Al Borchers (alborchers@steinerpoint.com)
 * Copyright (C) 2008 David Brownell
 * Copyright (C) 2008 by Nokia Corporation
 * Copyright (c) 2011, Code Aurora Forum. All rights reserved.
 *
 * This program from the Code Aurora Forum is free software; you can
 * redistribute it and/or modify it under the GNU General Public License
 * version 2 and only version 2 as published by the Free Software Foundation.
 * The original work available from [kernel.org] is subject to the notice below.
 *
 * This software is distributed under the terms of the GNU General
 * Public License ("GPL") as published by the Free Software Foundation,
 * either version 2 of that License or (at your option) any later version.
 *
 */

#include <linux/kernel.h>
#include <linux/interrupt.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/termios.h>
#include <linux/debugfs.h>
#include <mach/usb_gadget_fserial.h>

#include <mach/sdio_al.h>
#include <mach/sdio_cmux.h>
#include "u_serial.h"

#if 1 // eunmo.yang 2011.05.19 for PTN TEST
/* [START] hansun.lee@lge.com 2011.04.14 */
/* [ADD] add ATCMD filter function declaration */
ssize_t atcmd_queue(const char *buf, size_t count);
ssize_t atcmd_sdio_write(const char *buf, size_t count);
/* [END] hansun.lee@lge.com 2011.04.14 */
#endif

#define RX_QUEUE_SIZE		8
#define RX_BUF_SIZE		2048

#define TX_QUEUE_SIZE		8
#define TX_BUF_SIZE		2048

/* 1 - DUN, 2-NMEA/GPS */
#define N_PORTS	2
static struct portmaster {
	struct mutex lock;
	struct gsdio_port *port;
	struct platform_driver gsdio_ch;
} ports[N_PORTS];
static unsigned n_ports;

struct sdio_port_info {
	/* data channel info */
	char *data_ch_name;
	struct sdio_channel *ch;

	/* control channel info */
	int ctrl_ch_id;
};

struct sdio_port_info sport_info[N_PORTS] = {
	{
		.data_ch_name = "SDIO_DUN",
		.ctrl_ch_id = 9,
	},
	{
		.data_ch_name = "SDIO_NMEA",
		.ctrl_ch_id = 10,
	},
};

static struct workqueue_struct *gsdio_wq;

struct gsdio_port {
	unsigned			port_num;
	spinlock_t			port_lock;

	unsigned			n_read;
	struct list_head		read_pool;
	struct list_head		read_queue;
	struct work_struct		push;
	unsigned long			rp_len;
	unsigned long			rq_len;

	struct list_head		write_pool;
	struct work_struct		pull;
	unsigned long			wp_len;

	struct work_struct		notify_modem;

	struct gserial			*port_usb;
	struct usb_cdc_line_coding	line_coding;

	int				sdio_open;
	int				sdio_probe;
	int				ctrl_ch_err;
	struct sdio_port_info		*sport_info;
	struct delayed_work		sdio_open_work;

#define ACM_CTRL_RI		(1 << 3)
#define ACM_CTRL_DSR		(1 << 1)
#define ACM_CTRL_DCD		(1 << 0)
	int				cbits_to_laptop;

#define ACM_CTRL_RTS	(1 << 1)	/* unused with full duplex */
#define ACM_CTRL_DTR	(1 << 0)	/* host is ready for data r/w */
	int				cbits_to_modem;

	/* pkt logging */
	unsigned long			nbytes_tolaptop;
	unsigned long			nbytes_tomodem;
};

void gsdio_free_req(struct usb_ep *ep, struct usb_request *req)
{
	kfree(req->buf);
	usb_ep_free_request(ep, req);
}

struct usb_request *
gsdio_alloc_req(struct usb_ep *ep, unsigned len, gfp_t flags)
{
	struct usb_request *req;

	req = usb_ep_alloc_request(ep, flags);
	if (!req) {
		pr_err("%s: usb alloc request failed\n", __func__);
		return NULL;
	}

	req->length = len;
	req->buf = kmalloc(len, flags);
	if (!req->buf) {
		pr_err("%s: request buf allocation failed\n", __func__);
		usb_ep_free_request(ep, req);
		return NULL;
	}

	return req;
}

void gsdio_free_requests(struct usb_ep *ep, struct list_head *head)
{
	struct usb_request	*req;

	while (!list_empty(head)) {
		req = list_entry(head->next, struct usb_request, list);
		list_del(&req->list);
		gsdio_free_req(ep, req);
	}
}

int gsdio_alloc_requests(struct usb_ep *ep, struct list_head *head,
		int num, int size,
		void (*cb)(struct usb_ep *ep, struct usb_request *))
{
	int i;
	struct usb_request *req;

	pr_debug("%s: ep:%p head:%p num:%d size:%d cb:%p", __func__,
			ep, head, num, size, cb);

	for (i = 0; i < num; i++) {
		req = gsdio_alloc_req(ep, size, GFP_ATOMIC);
		if (!req) {
			pr_debug("%s: req allocated:%d\n", __func__, i);
			return list_empty(head) ? -ENOMEM : 0;
		}
		req->complete = cb;
		list_add(&req->list, head);
	}

	return 0;
}

void gsdio_start_rx(struct gsdio_port *port)
{
	struct list_head	*pool;
	struct usb_ep		*out;
	int ret;

	if (!port) {
		pr_err("%s: port is null\n", __func__);
		return;
	}

	pr_debug("%s: port:%p port#%d\n", __func__, port, port->port_num);

	spin_lock_irq(&port->port_lock);

	if (!port->port_usb) {
		pr_debug("%s: usb is disconnected\n", __func__);
		goto start_rx_end;
	}

	if (!port->sdio_open) {
		pr_debug("%s: sdio is not open\n", __func__);
		goto start_rx_end;
	}

	pool = &port->read_pool;
	out = port->port_usb->out;

	while (!list_empty(pool)) {
		struct usb_request	*req;

		req = list_entry(pool->next, struct usb_request, list);
		list_del(&req->list);
		req->length = RX_BUF_SIZE;
		port->rp_len--;

		spin_unlock_irq(&port->port_lock);
		ret = usb_ep_queue(out, req, GFP_ATOMIC);
		spin_lock_irq(&port->port_lock);
		if (ret) {
			pr_err("%s: usb ep out queue failed"
					"port:%p, port#%d\n",
					__func__, port, port->port_num);
			list_add_tail(&req->list, pool);
			port->rp_len++;
			break;
		}

		/* usb could have disconnected while we released spin lock */
		if (!port->port_usb) {
			pr_debug("%s: usb is disconnected\n", __func__);
			goto start_rx_end;
		}
	}

start_rx_end:
	spin_unlock_irq(&port->port_lock);
}

int gsdio_write(struct gsdio_port *port, struct usb_request *req)
{
	unsigned	avail;
	char		*packet = req->buf;
	unsigned	size = req->actual;
	unsigned	n;
	int		ret = 0;


	if (!port) {
		pr_err("%s: port is null\n", __func__);
		return -ENODEV;
	}

	if (!req) {
		pr_err("%s: usb request is null port#%d\n",
				__func__, port->port_num);
		return -ENODEV;
	}

	pr_debug("%s: port:%p port#%d req:%p actual:%d n_read:%d\n",
			__func__, port, port->port_num, req,
			req->actual, port->n_read);

	if (!port->sdio_open) {
		pr_debug("%s: SDIO IO is not supported\n", __func__);
		return -ENODEV;
	}

	avail = sdio_write_avail(port->sport_info->ch);

	pr_debug("%s: sdio_write_avail:%d", __func__, avail);

	if (!avail)
		return -EBUSY;

	if (!req->actual) {
		pr_debug("%s: req->actual is already zero,update bytes read\n",
				__func__);
		port->n_read = 0;
		return -ENODEV;
	}

	packet = req->buf;
	n = port->n_read;
	if (n) {
		packet += n;
		size -= n;
	}

	if (size > avail)
		size = avail;

	spin_unlock_irq(&port->port_lock);
	ret = sdio_write(port->sport_info->ch, packet, size);
	spin_lock_irq(&port->port_lock);
	if (ret) {
		pr_err("%s: port#%d sdio write failed err:%d",
				__func__, port->port_num, ret);
		/* try again later */
		return ret;
	}

	port->nbytes_tomodem += size;

	if (size + n == req->actual)
		port->n_read = 0;
	else
		port->n_read += size;

	return ret;
}

void gsdio_rx_push(struct work_struct *w)
{
	struct gsdio_port *port = container_of(w, struct gsdio_port, push);
	struct list_head *q = &port->read_queue;
	struct usb_ep		*out;
	int ret;

	pr_debug("%s: port:%p port#%d read_queue:%p", __func__,
			port, port->port_num, q);

	spin_lock_irq(&port->port_lock);

	if (!port->port_usb) {
		pr_debug("%s: usb cable is disconencted\n", __func__);
		spin_unlock_irq(&port->port_lock);
		return;
	}

	out = port->port_usb->out;

	while (!list_empty(q)) {
		struct usb_request *req;

		req = list_first_entry(q, struct usb_request, list);

		switch (req->status) {
		case -ESHUTDOWN:
			pr_debug("%s: req status shutdown portno#%d port:%p",
					__func__, port->port_num, port);
			goto rx_push_end;
		default:
			pr_warning("%s: port:%p port#%d"
					" Unexpected Rx Status:%d\n", __func__,
					port, port->port_num, req->status);
			/* FALL THROUGH */
		case 0:
			/* normal completion */

#if 1			// eunmo.yang 2011.05.19 for PTN TEST
            /* [START] hansun.lee@lge.com 2011.04.14 */
            /* [ADD] add ATCMD filter procedure */
            if (port->port_num == 0) /* modem */
            {
                char *packet = req->buf;
                unsigned size = req->actual;
                unsigned n = port->n_read;;

                //pr_info("%s: actual[%d], n_read[%d], length[%d]\n", __func__, req->actual, port->n_read, req->length);

                if (n)
                {
                    packet += n;
                    size -= n;
                }

                if (atcmd_queue(packet, size) != 0)
                {
                    list_move(&req->list, &port->read_pool);
                    goto rx_push_end;
                }
            }
            /* [END] hansun.lee@lge.com 2011.04.14 */
#endif
			break;
		}

		if (!port->sdio_open) {
			pr_err("%s: sio channel is not open\n", __func__);
			list_move(&req->list, &port->read_pool);
			port->rp_len++;
			port->rq_len--;
			goto rx_push_end;
		}


		list_del(&req->list);
		port->rq_len--;

		ret = gsdio_write(port, req);
		/* as gsdio_write drops spin_lock while writing data
		 * to sdio usb cable may have been disconnected
		 */
		if (!port->port_usb) {
			port->n_read = 0;
			gsdio_free_req(out, req);
			spin_unlock_irq(&port->port_lock);
			return;
		}

		if (ret || port->n_read) {
			list_add(&req->list, &port->read_queue);
			port->rq_len++;
			goto rx_push_end;
		}

		list_add(&req->list, &port->read_pool);
		port->rp_len++;
	}

	if (port->sdio_open && !list_empty(q)) {
		if (sdio_write_avail(port->sport_info->ch))
			queue_work(gsdio_wq, &port->push);
	}
rx_push_end:
	spin_unlock_irq(&port->port_lock);

	/* start queuing out requests again to host */
	gsdio_start_rx(port);
}

void gsdio_read_complete(struct usb_ep *ep, struct usb_request *req)
{
	struct gsdio_port *port = ep->driver_data;
	unsigned long flags;

	pr_debug("%s: ep:%p port:%p\n", __func__, ep, port);

	if (!port) {
		pr_err("%s: port is null\n", __func__);
		return;
	}

	spin_lock_irqsave(&port->port_lock, flags);
	list_add_tail(&req->list, &port->read_queue);
	port->rq_len++;
	queue_work(gsdio_wq, &port->push);
	spin_unlock_irqrestore(&port->port_lock, flags);

	return;
}

void gsdio_write_complete(struct usb_ep *ep, struct usb_request *req)
{
	struct gsdio_port *port = ep->driver_data;
	unsigned long flags;

	pr_debug("%s: ep:%p port:%p\n", __func__, ep, port);

	if (!port) {
		pr_err("%s: port is null\n", __func__);
		return;
	}

	spin_lock_irqsave(&port->port_lock, flags);
	list_add(&req->list, &port->write_pool);
	port->wp_len++;

	switch (req->status) {
	default:
		pr_warning("%s: port:%p port#%d unexpected %s status %d\n",
				__func__, port, port->port_num,
				ep->name, req->status);
		/* FALL THROUGH */
	case 0:
		queue_work(gsdio_wq, &port->pull);
		break;

	case -ESHUTDOWN:
		/* disconnect */
		pr_debug("%s: %s shutdown\n", __func__, ep->name);
		break;
	}

	spin_unlock_irqrestore(&port->port_lock, flags);

	return;
}

void gsdio_read_pending(struct gsdio_port *port)
{
	struct sdio_channel *ch;
	char buf[1024];
	int avail;

	if (!port) {
		pr_err("%s: port is null\n", __func__);
		return;
	}

	ch = port->sport_info->ch;

	if (!ch)
		return;

	while ((avail = sdio_read_avail(ch))) {
		if (avail > 1024)
			avail = 1024;
		sdio_read(ch, buf, avail);

		pr_debug("%s: flushed out %d bytes\n", __func__, avail);
	}
}

void gsdio_tx_pull(struct work_struct *w)
{
	struct gsdio_port *port = container_of(w, struct gsdio_port, pull);
	struct list_head *pool = &port->write_pool;

	pr_debug("%s: port:%p port#%d pool:%p\n", __func__,
			port, port->port_num, pool);

	if (!port->port_usb) {
		pr_err("%s: usb disconnected\n", __func__);

		/* take out all the pending data from sdio */
		gsdio_read_pending(port);

		return;
	}

	spin_lock_irq(&port->port_lock);

	while (!list_empty(pool)) {
		int avail;
		struct usb_ep *in = port->port_usb->in;
		struct sdio_channel *ch = port->sport_info->ch;
		struct usb_request *req;
		unsigned len = TX_BUF_SIZE;
		int ret;


		req = list_entry(pool->next, struct usb_request, list);

		if (!port->sdio_open) {
			pr_debug("%s: SDIO channel is not open\n", __func__);
			goto tx_pull_end;
		}

		avail = sdio_read_avail(ch);
		if (!avail) {
			/* REVISIT: for ZLP */
			pr_debug("%s: read_avail:%d port:%p port#%d\n",
					__func__, avail, port, port->port_num);
			goto tx_pull_end;
		}

		if (avail > len)
			avail = len;

		list_del(&req->list);
		port->wp_len--;

		spin_unlock_irq(&port->port_lock);
		ret = sdio_read(ch, req->buf, avail);
		spin_lock_irq(&port->port_lock);
		if (ret) {
			pr_err("%s: port:%p port#%d sdio read failed err:%d",
					__func__, port, port->port_num, ret);

			/* check if usb is still active */
			if (!port->port_usb) {
				gsdio_free_req(in, req);
			} else {
				list_add(&req->list, pool);
				port->wp_len++;
			}
			goto tx_pull_end;
		}
#if 1			// eunmo.yang 2011.05.19 for PTN TEST
        /* [START] hansun.lee@lge.com 2011.04.14 */
        /* [ADD] add printg for tx_pull data */
        if (0) {
            char buf[256];

            memcpy(buf, req->buf, avail);
            buf[avail] = '\0';
            if (buf[avail-1] == '\r' || buf[avail-1] == '\n')
            {
                buf[avail-1] = '\0';
            }
            pr_info("%s: [%d][\n%s\n]\n", __func__, avail, buf);
        }
        /* [END] hansun.lee@lge.com 2011.04.14 */
#endif

		req->length = avail;

		spin_unlock_irq(&port->port_lock);
		ret = usb_ep_queue(in, req, GFP_KERNEL);
		spin_lock_irq(&port->port_lock);
		if (ret) {
			pr_err("%s: usb ep out queue failed"
					"port:%p, port#%d err:%d\n",
					__func__, port, port->port_num, ret);

			/* could be usb disconnected */
			if (!port->port_usb) {
				gsdio_free_req(in, req);
			} else {
				list_add(&req->list, pool);
				port->wp_len++;
			}
			goto tx_pull_end;
		}

		port->nbytes_tolaptop += avail;
	}
tx_pull_end:
	spin_unlock_irq(&port->port_lock);
}

int gsdio_start_io(struct gsdio_port *port)
{
	int			ret;
	unsigned long		flags;

	pr_debug("%s:\n", __func__);

	spin_lock_irqsave(&port->port_lock, flags);

	if (!port->port_usb) {
		spin_unlock_irqrestore(&port->port_lock, flags);
		return -ENODEV;
	}

	/* start usb out queue */
	ret = gsdio_alloc_requests(port->port_usb->out,
				&port->read_pool,
				RX_QUEUE_SIZE, RX_BUF_SIZE,
				gsdio_read_complete);
	if (ret) {
		spin_unlock_irqrestore(&port->port_lock, flags);
		pr_err("%s: unable to allocate out reqs\n", __func__);
		return ret;
	}
	port->rp_len = RX_QUEUE_SIZE;

	ret = gsdio_alloc_requests(port->port_usb->in,
				&port->write_pool,
				TX_QUEUE_SIZE, TX_BUF_SIZE,
				gsdio_write_complete);
	if (ret) {
		gsdio_free_requests(port->port_usb->out, &port->read_pool);
		port->rp_len = 0;
		spin_unlock_irqrestore(&port->port_lock, flags);
		pr_err("%s: unable to allocate in reqs\n", __func__);
		return ret;
	}
	port->wp_len = TX_QUEUE_SIZE;
	spin_unlock_irqrestore(&port->port_lock, flags);

	gsdio_start_rx(port);
	queue_work(gsdio_wq, &port->pull);

	return 0;
}

void gsdio_port_free(unsigned portno)
{
	struct gsdio_port *port = ports[portno].port;
	struct platform_driver *pdriver = &ports[portno].gsdio_ch;

	if (!port) {
		pr_err("%s: invalid portno#%d\n", __func__, portno);
		return;
	}

	platform_driver_unregister(pdriver);

	kfree(port);
}

void gsdio_ctrl_wq(struct work_struct *w)
{
	struct gsdio_port *port;

	port = container_of(w, struct gsdio_port, notify_modem);

	if (!port) {
		pr_err("%s: port is null\n", __func__);
		return;
	}

	if (!port->sdio_open || port->ctrl_ch_err)
		return;

	sdio_cmux_tiocmset(port->sport_info->ctrl_ch_id,
			port->cbits_to_modem, ~(port->cbits_to_modem));
}

void gsdio_ctrl_notify_modem(struct gserial *gser, u8 portno, int ctrl_bits)
{
	struct gsdio_port *port;
	int temp;

	if (portno >= n_ports) {
		pr_err("%s: invalid portno#%d\n", __func__, portno);
		return;
	}

	if (!gser) {
		pr_err("%s: gser is null\n", __func__);
		return;
	}

	port = ports[portno].port;

	temp = ctrl_bits & ACM_CTRL_DTR ? TIOCM_DTR : 0;

	if (port->cbits_to_modem == temp)
		return;

	 port->cbits_to_modem = temp;

	/* TIOCM_DTR - 0x002 - bit(1) */
	pr_debug("%s: port:%p port#%d ctrl_bits:%08x\n", __func__,
		port, port->port_num, ctrl_bits);

	if (!port->sdio_open) {
		pr_err("%s: port:%p port#%d sdio not connected\n",
				__func__, port, port->port_num);
		return;
	}

	/* whenever DTR is high let laptop know that modem status */
	if (port->cbits_to_modem && gser->send_modem_ctrl_bits)
		gser->send_modem_ctrl_bits(gser, port->cbits_to_laptop);

	queue_work(gsdio_wq, &port->notify_modem);
}

void gsdio_ctrl_modem_status(int ctrl_bits, void *_dev)
{
	struct gsdio_port *port = _dev;

	/* TIOCM_CD - 0x040 - bit(6)
	 * TIOCM_RI - 0x080 - bit(7)
	 * TIOCM_DSR- 0x100 - bit(8)
	 */
	pr_debug("%s: port:%p port#%d event:%08x\n", __func__,
		port, port->port_num, ctrl_bits);

	port->cbits_to_laptop = 0;
	ctrl_bits &= TIOCM_RI | TIOCM_CD | TIOCM_DSR;
	if (ctrl_bits & TIOCM_RI)
		port->cbits_to_laptop |= ACM_CTRL_RI;
	if (ctrl_bits & TIOCM_CD)
		port->cbits_to_laptop |= ACM_CTRL_DCD;
	if (ctrl_bits & TIOCM_DSR)
		port->cbits_to_laptop |= ACM_CTRL_DSR;

	if (port->port_usb && port->port_usb->send_modem_ctrl_bits)
		port->port_usb->send_modem_ctrl_bits(port->port_usb,
					port->cbits_to_laptop);
}

void gsdio_ch_notify(void *_dev, unsigned event)
{
	struct gsdio_port *port = _dev;

	pr_debug("%s: port:%p port#%d event:%s\n", __func__,
		port, port->port_num,
		event == 1 ? "READ AVAIL" : "WRITE_AVAIL");

	if (event == SDIO_EVENT_DATA_WRITE_AVAIL)
		queue_work(gsdio_wq, &port->push);
	if (event == SDIO_EVENT_DATA_READ_AVAIL)
		queue_work(gsdio_wq, &port->pull);
}

static void gsdio_open_work(struct work_struct *w)
{
	struct gsdio_port *port =
			container_of(w, struct gsdio_port, sdio_open_work.work);
	struct sdio_port_info *pi = port->sport_info;
	struct gserial *gser;
	int ret;
	int ctrl_bits;
	int startio;

	ret = sdio_open(pi->data_ch_name, &pi->ch, port, gsdio_ch_notify);
	if (ret) {
		pr_err("%s: port:%p port#%d unable to open sdio ch:%s\n",
				__func__, port, port->port_num,
				pi->data_ch_name);
		return;
	}

	port->ctrl_ch_err = 0;
	ret = sdio_cmux_open(pi->ctrl_ch_id, 0, 0,
			gsdio_ctrl_modem_status, port);
	if (ret) {
		pr_err("%s: port:%p port#%d unable to open ctrl ch:%d\n",
				__func__, port, port->port_num, pi->ctrl_ch_id);
		port->ctrl_ch_err = 1;
	}

	/* check for latest status update from modem */
	if (!port->ctrl_ch_err) {
		ctrl_bits = sdio_cmux_tiocmget(pi->ctrl_ch_id);
		gsdio_ctrl_modem_status(ctrl_bits, port);
	}

	pr_debug("%s: SDIO data:%s ctrl:%d are open\n", __func__,
					pi->data_ch_name,
					pi->ctrl_ch_id);

	port->sdio_open = 1;

	/* start tx if usb is open already */
	spin_lock_irq(&port->port_lock);
	startio = port->port_usb ? 1 : 0;
	gser = port->port_usb;
	spin_unlock_irq(&port->port_lock);

	if (startio) {
		pr_debug("%s: USB is already open, start io\n", __func__);
		gsdio_start_io(port);
		 if (gser->send_modem_ctrl_bits)
			gser->send_modem_ctrl_bits(gser, port->cbits_to_laptop);
	}
}

#define SDIO_CH_NAME_MAX_LEN	9
#define SDIO_OPEN_DELAY		msecs_to_jiffies(10000)
static int gsdio_ch_remove(struct platform_device *dev)
{
	struct gsdio_port	*port;
	struct sdio_port_info	*pi;
	int i;
	unsigned long		flags;

	pr_debug("%s: name:%s\n", __func__, dev->name);

	for (i = 0; i < n_ports; i++) {
		port = ports[i].port;
		pi = port->sport_info;

		if (!strncmp(pi->data_ch_name, dev->name,
					SDIO_CH_NAME_MAX_LEN)) {
			struct gserial *gser = port->port_usb;

			port->sdio_open = 0;
			port->sdio_probe = 0;
			port->ctrl_ch_err = 1;

			/* check if usb cable is connected */
			if (!gser)
				continue;

			/* indicated call status to usb host */
			gsdio_ctrl_modem_status(0, port);

			usb_ep_fifo_flush(gser->in);
			usb_ep_fifo_flush(gser->out);

			cancel_work_sync(&port->push);
			cancel_work_sync(&port->pull);

			spin_lock_irqsave(&port->port_lock, flags);
			gsdio_free_requests(gser->out, &port->read_pool);
			gsdio_free_requests(gser->out, &port->read_queue);
			gsdio_free_requests(gser->in, &port->write_pool);

			port->rp_len = 0;
			port->rq_len = 0;
			port->wp_len = 0;
			port->n_read = 0;
			spin_unlock_irqrestore(&port->port_lock, flags);

		}
	}

	return 0;
}

static int gsdio_ch_probe(struct platform_device *dev)
{
	struct gsdio_port	*port;
	struct sdio_port_info	*pi;
	int i;

	pr_debug("%s: name:%s\n", __func__, dev->name);

	for (i = 0; i < n_ports; i++) {
		port = ports[i].port;
		pi = port->sport_info;

		pr_debug("%s: sdio_ch_name:%s dev_name:%s\n", __func__,
				pi->data_ch_name, dev->name);

		/* unfortunately cmux channle might not be ready even if
		 * sdio channel is ready. as we dont have good notification
		 * mechanism schedule a delayed work
		 */
		if (!strncmp(pi->data_ch_name, dev->name,
					SDIO_CH_NAME_MAX_LEN)) {
			port->sdio_probe = 1;
			queue_delayed_work(gsdio_wq,
				&port->sdio_open_work, SDIO_OPEN_DELAY);
			return 0;
		}
	}

	pr_info("%s: name:%s is not found\n", __func__, dev->name);

	return -ENODEV;
}

int gsdio_port_alloc(unsigned portno,
		struct usb_cdc_line_coding *coding,
		struct sdio_port_info *pi)
{
	struct gsdio_port *port;
	struct platform_driver *pdriver;

	port = kzalloc(sizeof(struct gsdio_port), GFP_KERNEL);
	if (!port) {
		pr_err("%s: port allocation failed\n", __func__);
		return -ENOMEM;
	}

	port->port_num = portno;
	spin_lock_init(&port->port_lock);
	port->line_coding = *coding;

	/* READ: read from usb and write into sdio */
	INIT_LIST_HEAD(&port->read_pool);
	INIT_LIST_HEAD(&port->read_queue);
	INIT_WORK(&port->push, gsdio_rx_push);

	INIT_LIST_HEAD(&port->write_pool);
	INIT_WORK(&port->pull, gsdio_tx_pull);

	INIT_WORK(&port->notify_modem, gsdio_ctrl_wq);

	INIT_DELAYED_WORK(&port->sdio_open_work, gsdio_open_work);

	ports[portno].port = port;

	port->sport_info = pi;
	pdriver = &ports[portno].gsdio_ch;

	pdriver->probe = gsdio_ch_probe;
	pdriver->remove = gsdio_ch_remove;
	pdriver->driver.name = pi->data_ch_name;
	pdriver->driver.owner = THIS_MODULE;

	pr_debug("%s: port:%p port#%d sdio_name: %s\n", __func__,
			port, port->port_num, pi->data_ch_name);

	platform_driver_register(pdriver);

	pr_debug("%s: port:%p port#%d\n", __func__, port, port->port_num);

	return 0;
}

int gsdio_connect(struct gserial *gser, u8 portno)
{
	struct gsdio_port *port;
	int ret = 0;
	unsigned long flags;

	if (portno >= n_ports) {
		pr_err("%s: invalid portno#%d\n", __func__, portno);
		return -EINVAL;
	}

	if (!gser) {
		pr_err("%s: gser is null\n", __func__);
		return -EINVAL;
	}

	port = ports[portno].port;

	spin_lock_irqsave(&port->port_lock, flags);
	port->port_usb = gser;
	gser->notify_modem = gsdio_ctrl_notify_modem;
	spin_unlock_irqrestore(&port->port_lock, flags);

	ret = usb_ep_enable(gser->in, gser->in_desc);
	if (ret) {
		pr_err("%s: failed to enable in ep w/ err:%d\n",
					__func__, ret);
		port->port_usb = 0;
		return ret;
	}
	gser->in->driver_data = port;

	ret = usb_ep_enable(gser->out, gser->out_desc);
	if (ret) {
		pr_err("%s: failed to enable in ep w/ err:%d\n",
					__func__, ret);
		usb_ep_disable(gser->in);
		port->port_usb = 0;
		gser->in->driver_data = 0;
		return ret;
	}
	gser->out->driver_data = port;

	if (port->sdio_open) {
		pr_debug("%s: sdio is already open, start io\n", __func__);
		gsdio_start_io(port);
		 if (gser->send_modem_ctrl_bits)
			gser->send_modem_ctrl_bits(gser, port->cbits_to_laptop);
	}

	return 0;
}

void gsdio_disconnect(struct gserial *gser, u8 portno)
{
	unsigned long flags;
	struct gsdio_port *port;

	if (portno >= n_ports) {
		pr_err("%s: invalid portno#%d\n", __func__, portno);
		return;
	}

	if (!gser) {
		pr_err("%s: gser is null\n", __func__);
		return;
	}

	port = ports[portno].port;

	/* send dtr zero to modem to notify disconnect */
	port->cbits_to_modem = 0;
	queue_work(gsdio_wq, &port->notify_modem);

	spin_lock_irqsave(&port->port_lock, flags);
	port->port_usb = 0;
	port->nbytes_tomodem = 0;
	port->nbytes_tolaptop = 0;
	spin_unlock_irqrestore(&port->port_lock, flags);

	/* disable endpoints, aborting down any active I/O */
	usb_ep_disable(gser->out);

	usb_ep_disable(gser->in);

	spin_lock_irqsave(&port->port_lock, flags);
	gsdio_free_requests(gser->out, &port->read_pool);
	gsdio_free_requests(gser->out, &port->read_queue);
	gsdio_free_requests(gser->in, &port->write_pool);

	port->rp_len = 0;
	port->rq_len = 0;
	port->wp_len = 0;
	port->n_read = 0;
	spin_unlock_irqrestore(&port->port_lock, flags);
}

#if defined(CONFIG_DEBUG_FS)
static ssize_t debug_read_stats(struct file *file, char __user *ubuf,
		size_t count, loff_t *ppos)
{
	struct gsdio_port *port;
	char *buf;
	unsigned long flags;
	int i = 0;
	int temp = 0;
	int ret;

	buf = kzalloc(sizeof(char) * 1024, GFP_KERNEL);
	if (!buf)
		return -ENOMEM;

	while (i < n_ports) {
		port = ports[i].port;
		spin_lock_irqsave(&port->port_lock, flags);
		temp += scnprintf(buf + temp, PAGE_SIZE - temp,
				"###PORT:%d port:%p###\n"
				"nbytes_tolaptop: %lu\n"
				"nbytes_tomodem:  %lu\n"
				"cbits_to_modem:  %u\n"
				"cbits_to_laptop: %u\n"
				"read_pool_len:   %lu\n"
				"read_queue_len:  %lu\n"
				"write_pool_len:  %lu\n"
				"n_read:          %u\n"
				"sdio_open:       %d\n"
				"sdio_probe:      %d\n",
				i, port,
				port->nbytes_tolaptop, port->nbytes_tomodem,
				port->cbits_to_modem, port->cbits_to_laptop,
				port->rp_len, port->rq_len, port->wp_len,
				port->n_read,
				port->sdio_open, port->sdio_probe);
		spin_unlock_irqrestore(&port->port_lock, flags);
		i++;
	}

	ret = simple_read_from_buffer(ubuf, count, ppos, buf, temp);

	kfree(buf);

	return ret;
}

static ssize_t debug_reset_stats(struct file *file, const char __user *buf,
				 size_t count, loff_t *ppos)
{
	struct gsdio_port *port;
	unsigned long flags;
	int i = 0;

	while (i < n_ports) {
		port = ports[i].port;

		spin_lock_irqsave(&port->port_lock, flags);
		port->nbytes_tolaptop = 0;
		port->nbytes_tomodem = 0;
		spin_unlock_irqrestore(&port->port_lock, flags);
		i++;
	}

	return count;
}

static int debug_open(struct inode *inode, struct file *file)
{
	return 0;
}

static const struct file_operations debug_gsdio_ops = {
	.open = debug_open,
	.read = debug_read_stats,
	.write = debug_reset_stats,
};

static void gsdio_debugfs_init(void)
{
	struct dentry *dent;

	dent = debugfs_create_dir("usb_gsdio", 0);
	if (IS_ERR(dent))
		return;

	debugfs_create_file("status", 0444, dent, 0, &debug_gsdio_ops);
}
#else
static void gsdio_debugfs_init(void)
{
	return;
}
#endif

/* connect, disconnect, alloc_requests, free_requests */
int gsdio_setup(struct usb_gadget *g, unsigned count)
{
	struct usb_cdc_line_coding	coding;
	int i;
	int ret = 0;
	struct sdio_port_info *port_info;

	pr_debug("%s: gadget:(%p) count:%d\n", __func__, g, count);

	if (count == 0 || count > N_PORTS) {
		pr_err("%s: invalid number of ports count:%d max_ports:%d\n",
				__func__, count, N_PORTS);
		return -EINVAL;
	}

	coding.dwDTERate = cpu_to_le32(9600);
	coding.bCharFormat = 8;
	coding.bParityType = USB_CDC_NO_PARITY;
	coding.bDataBits = USB_CDC_1_STOP_BITS;

	gsdio_wq = create_singlethread_workqueue("k_gserial");
	if (!gsdio_wq) {
		pr_err("%s: unable to create workqueue gsdio_wq\n",
				__func__);
		return -ENOMEM;
	}

	for (i = 0; i < count; i++) {
		mutex_init(&ports[i].lock);
		ret = gsdio_port_alloc(i, &coding, sport_info + i);
		if (ret) {
			pr_err("%s: sdio logical port allocation failed\n",
					__func__);
			goto free_ports;
		}
		n_ports++;
		port_info++;

#ifdef DEBUG
		/* REVISIT: create one file per port
		 * or do not create any file
		 */
		if (i == 0) {
			ret = device_create_file(&g->dev, &dev_attr_input);
			if (ret)
				pr_err("%s: unable to create device file\n",
						__func__);
		}
#endif

	}

	gsdio_debugfs_init();

	return 0;

free_ports:
	for (i = 0; i < n_ports; i++)
		gsdio_port_free(i);
	destroy_workqueue(gsdio_wq);

	return ret;
}

/* TODO: Add gserial_cleanup */

#if 1			// eunmo.yang 2011.05.19 for PTN TEST
/* [START] hansun.lee@lge.com 2011.04.14 */
/* [ADD] add ATCMD filter driver */
/*******************************************************************************
 * AT Command Handler
 ******************************************************************************/

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/types.h>
#include <linux/fcntl.h>
#include <linux/device.h>

#include <asm/uaccess.h>
#include <asm/io.h>

#define list_last_entry(ptr, type, member) \
    list_entry((ptr)->prev, type, member)

static int atcmd_major;
static DECLARE_WAIT_QUEUE_HEAD(atcmd_read_wait);
static int atcmd_modem = 0;

static struct list_head atcmd_pool;
struct atcmd_request {
    char buf[2048];
    unsigned length;
    unsigned status;

    struct list_head list;
};

static struct class *atcmd_class;
static struct device *atcmd_dev;

struct device *get_atcmd_dev(void)
{
    return atcmd_dev;
}
EXPORT_SYMBOL(get_atcmd_dev);

static char atcmd_name[2048];
static char atcmd_state[2048];

struct atcmd_request *atcmd_alloc_req(void)
{
    struct atcmd_request *req;

    req = kmalloc(sizeof(struct atcmd_request), GFP_ATOMIC);
    if (req == NULL)
        return NULL;

    req->length = 0;
    req->status = 0;

    return req;
}

void atcmd_free_req(struct atcmd_request *req)
{
    kfree(req);
}

#define ATCMD_TO_AP 0
#define ATCMD_TO_CP 1

static const char *atcmd_ap[] = {

    "+MTC", "%ACS", "%BTTM", "%READY", "%AVR", "%BATL", "%BOFF", "%CAM", "%CHARGE", "%CHCOMP",
    "%DBCOPY", "%DBCRC", "%DBCHK", "%DBDUMP", "%DRMCERT", "%DRMERASE",
    "%DRMINDEX", "%DRMTYPE", "%ECALL", "%EMT", "%FBOOT", "%FILECRC", "%FKPD", "%FLIGHT",
    "%FMR", "%FPRICRC", "%FRST", "%FRSTSTATUS", "%FUELRST", "%FUELVAL", "%GKPD",
    "%LANG", "%LCD", "%LEDON", "%MAC", "%MACCK", "%MMCDDATADEL", "%MMCDEFULTSIZE",
    "%MMCFORMAT", "%MMCTOTALSIZE", "%MMCUSEDSIZE", "%MOT", "%MPT", "%PMRST", "%OSVER",
	"%PROXIMITY", "%ALC", "%ACCEL", "%COMPASS", "%GYRO",
    "%RESTART", "%SPM", "%SURV", "%SWVCHECK", "%VLC", "%USBSW", "%WLAN", "%WLANR", "%WLANT", "%NCM", 
#ifdef CONFIG_LGE_FELICA
    "%IMA", "%IDM", "%EXTIDM", "%FELICATX", "%SWITCH", "%FREQCAL", "%RFIDCK", "%RFREGCAL", "%SWTABLE", "%CFREQ",
#endif
//add for LGE Broadcast 1Seg AT CMD   20110711-taew00k.kang [start]
	"%MTV", 
//add for LGE Broadcast 1Seg AT CMD   20110711-taew00k.kang [END]
//20110708, seunghyup.ryoo@lge.com,  [START]
#if defined(CONFIG_LGE_NFC_NXP_PN544PN65N)
	"%NFC",
#endif
//20110708, seunghyup.ryoo@lge.com,  [END]

//[START][DLOAD] jongan.kim - 2011.09.08
  "%LGANDROID", "%LGATSERVICE",
//[END][DLOAD] jongan.kim - 2011.09.08

    NULL
};

int atcmd_to(const char *buf, size_t count)
{
    int i, len;
    char *p;

//    pr_info("%s\n", __func__);

    strncpy(atcmd_name, buf, count);
    if ((p = strchr(atcmd_name, '=')) || (p = strchr(atcmd_name, '?')))
    {
        *p = '\0';
    }
    else
    {
        p = strchr(atcmd_name, '\r');
        *p = '\0';
    }

    for (i = 0; atcmd_ap[i] != NULL; i++)
    {
        len = strlen(atcmd_ap[i]);
 
        if (!strcasecmp(&atcmd_name[2], atcmd_ap[i]))
        {
            //for the bluetooth vbatt power.[[
            if(!strcasecmp(&atcmd_name[2] ,"%BTTM"))
            {
                extern int pm_chg_vbatt_fet_on(int );
                pm_chg_vbatt_fet_on(1);
            }
            //for the bluetooth vbatt power.]]

            if ((p = strchr(buf, '=')) || (p = strchr(buf, '?')))
            {
                strncpy(atcmd_state, p, count);
                p = strchr(atcmd_state, '\r');
                *p = '\0';
            }
            else
            {
                atcmd_state[0] = '\0';
            }

            pr_info("%s: ATCMD_TO_AP: matching!!! %s, %s\n", __func__, atcmd_name, atcmd_state);
            return ATCMD_TO_AP;
        }
    }

    pr_info("%s: ATCMD_TO_CP\n", __func__);
    return ATCMD_TO_CP;
}

ssize_t atcmd_queue(const char *buf, size_t count)
{
    struct gsdio_port *port = ports[0].port;
    struct atcmd_request *req;

    /* FIXME: this function is not full implementation */
    //pr_info("%s\n", __func__);

    if (count <= 0)
    {
        pr_info("%s: count <= 0\n", __func__);
        return 0;
    }

    /* atcmd_pool is empty or new pool */
    if (list_empty(&atcmd_pool) ||
        (req = list_last_entry(&atcmd_pool, struct atcmd_request, list))->status == 1)
    {
        //pr_info("%s: empty or new pool\n", __func__);

        if (count >= 3 && strncasecmp(buf, "at%", 3) && strncasecmp(buf, "at+", 3))
        {
            return 0;
        }
        else if (count >= 2 && strncasecmp(buf, "at", 2))
        {
            return 0;
        }
        else if (buf[0] != 'a' && buf[0] != 'A')
        {
            return 0;
        }

        req = atcmd_alloc_req();

        if( req == NULL ) // knk
        {
            pr_err("can't alloc for req\n" ) ;
            return -ENOMEM ;
        }
        list_add(&req->list, &atcmd_pool);
    }

//    pr_info("%s: [%d][%s]\n", __func__, req->length, req->buf);
    memcpy(&req->buf[req->length], buf, count);
    req->length += count;
    req->buf[req->length] = '\0';

    if (1) {
        char *tmp;
        char *p1 = req->buf;
        char *p2;
        unsigned remain = req->length;

        tmp = p2 = kmalloc( (remain * 4) + 1, GFP_ATOMIC);

        while (remain > 0)
        {
            if (*p1 >= 0x20)
            {
                *p2 = *p1;
            }
            else
            {
                switch (*p1)
                {
                    case '\r':
                        *p2 = '\\';
                        p2++;
                        *p2 = 'r';
                        break;
                    case '\n':
                        *p2 = '\\';
                        p2++;
                        *p2 = 'n';
                        break;
                    default:
                        sprintf(p2, "(%02X)", *p1);
                        p2 += 3;
                        break;
                }
            }

            p1++;
            p2++;
            remain--;
        }
        *p2 = '\0';
        pr_info("%s: [%d][%s]\n", __func__, req->length, tmp);
        kfree(tmp);
    }

    if ((buf[count-1] == '\r') ||(buf[count-1] == '\n') || (buf[count-1] == '\0'))
    {
        if (atcmd_to(req->buf, req->length) == ATCMD_TO_AP)
        {
            char *envp[3];
            char name[120], state[120];

            snprintf(name, 120, "AT_NAME=%s", atcmd_name);
            snprintf(state, 120, "AT_STATE=%s", atcmd_state);
            envp[0] = name;
            envp[1] = state;
            envp[2] = NULL;

            spin_unlock_irq(&port->port_lock);
            kobject_uevent_env(&atcmd_dev->kobj, KOBJ_CHANGE, envp);
            spin_lock_irq(&port->port_lock);

            if (atcmd_modem)
            {
                req->status = 1;
                wake_up_interruptible(&atcmd_read_wait);
            }
            else
            {
                list_del(&req->list);
                atcmd_free_req(req);
            }
        }
        else
        {
            /* write to sdio */
            atcmd_sdio_write(req->buf, req->length);
            list_del(&req->list);
            atcmd_free_req(req);
        }
    }

    return 1;
}

ssize_t atcmd_usb_write(const char *buf, size_t count)
{
    struct gsdio_port *port = ports[0].port;
    struct list_head *pool = &port->write_pool;
    struct usb_ep *in;
    struct usb_request *req;
    int ret;

//    pr_info("%s: count=%d\n", __func__, count);

    if (!port->port_usb) {
        pr_err("%s: usb disconnected\n", __func__);

        /* take out all the pending data from sdio */
        gsdio_read_pending(port);

        return -EIO;
    }

    in = port->port_usb->in;

    spin_lock_irq(&port->port_lock);
    while (list_empty(pool))
    {
//        pr_info("%s: list_empty\n", __func__);
        spin_unlock_irq(&port->port_lock);
        usleep(10);
        spin_lock_irq(&port->port_lock);
    }
    req = list_entry(pool->next, struct usb_request, list);
    list_del(&req->list);
    spin_unlock_irq(&port->port_lock);

    memcpy(req->buf, buf, count);
    req->length = count;

    ret = usb_ep_queue(in, req, GFP_KERNEL);
    spin_lock_irq(&port->port_lock);
    if (ret) {
        pr_info("%s: usb ep out queue failed"
                "port:%p, port#%d err:%d\n",
                __func__, port, port->port_num, ret);

        /* could be usb disconnected */
        if (!port->port_usb)
        {
            pr_info("%s: could be usb disconnected\n", __func__);
            gsdio_free_req(in, req);
        }
        else
        {
            list_add(&req->list, pool);
        }
        spin_unlock_irq(&port->port_lock);  // it was locked up without release knk
        return -EAGAIN;
    }

//    port->nbytes_tolaptop += count;
    spin_unlock_irq(&port->port_lock);
    return count;
}

ssize_t atcmd_sdio_write(const char *buf, size_t count)
{
    struct gsdio_port *port = ports[0].port;
    int ret;

//    pr_info("%s: count=%d\n", __func__, count);

    if (!port->sdio_open)
    {
        pr_err("%s: sio channel is not open\n", __func__);
        return -1;
    }

    spin_unlock_irq(&port->port_lock);
    ret = sdio_write(port->sport_info->ch, buf, count);
    spin_lock_irq(&port->port_lock);

    return ret;
}

int atcmd_open(struct inode *inode, struct file *filp)
{
//    pr_info("%s\n", __func__);
    atcmd_modem++;
    return 0;
}

ssize_t atcmd_read(struct file *filp, char *buf, size_t count, loff_t *f_pos)
{
    struct atcmd_request *req;

//    pr_info("%s\n", __func__);

    if (list_empty(&atcmd_pool))
    {
        if (filp->f_flags & O_NONBLOCK)
            return -EAGAIN;

//        pr_info("%s:1 sleeping?\n", __func__);
        interruptible_sleep_on(&atcmd_read_wait);
//        pr_info("%s:1 awake\n", __func__);

        if (signal_pending(current))
            return -ERESTARTSYS;

        req = list_first_entry(&atcmd_pool, struct atcmd_request, list);
    }
    else
    {
        req = list_first_entry(&atcmd_pool, struct atcmd_request, list);

//        pr_info("%s:2 sleeping?\n", __func__);
        wait_event_interruptible(atcmd_read_wait, req->status);
//        pr_info("%s:2 awake\n", __func__);
    }

    list_del(&req->list);

    if (req->status != 1)
    {
        atcmd_free_req(req);
        return 0;
    }
    memcpy(buf, req->buf, req->length-1);
    atcmd_free_req(req);

    return req->length-1;
}

ssize_t atcmd_write(struct file *filp, const char *buf, size_t count, loff_t *f_pos)
{
//    pr_info("%s\n", __func__);
    return atcmd_usb_write(buf, count);
}

int atcmd_release(struct inode *inodes, struct file *filp)
{
//    pr_info("%s\n", __func__);
    atcmd_modem--;
    if (atcmd_modem < 0) atcmd_modem = 0;
    return 0;
}

struct file_operations atcmd_fops =
{
    .owner      = THIS_MODULE,
    .read       = atcmd_read,
    .write      = atcmd_write,
    .open       = atcmd_open,
    .release    = atcmd_release,
};

static char *atcmd_dev_node(struct device *dev, mode_t *mode)
{
    *mode = 0666;
    return NULL;
}

static ssize_t atcmd_show_name(struct class *class, struct class_attribute *attr, char *buf)
{
    return snprintf(buf, PAGE_SIZE, "%s\n", atcmd_name);
}
static ssize_t atcmd_store_name(struct class *class, struct class_attribute *attr, const char *buf, size_t count)
{
    return atcmd_usb_write(buf, count);
}
static CLASS_ATTR(name, 0644, atcmd_show_name, atcmd_store_name);

static ssize_t atcmd_show_state(struct class *class, struct class_attribute *attr, char *buf)
{
    return snprintf(buf, PAGE_SIZE, "%s\n", atcmd_state);
}
static CLASS_ATTR(state, 0444, atcmd_show_state, NULL);

static __init int atcmd_init(void)
{
    int err;

    atcmd_major = register_chrdev(0, "modem", &atcmd_fops);
    if (atcmd_major < 0)
        return atcmd_major;

    pr_info("%s: ATCMD Handler /dev/lge_atcmd major=%d\n", __func__, atcmd_major);

    atcmd_class = class_create(THIS_MODULE, "atcmd");
    atcmd_class->devnode = atcmd_dev_node;
    atcmd_dev = device_create(atcmd_class, NULL, MKDEV(atcmd_major, 0), NULL, "lge_atcmd");
    atcmd_dev->class = atcmd_class;

    err = class_create_file(atcmd_class, &class_attr_name);
    err = class_create_file(atcmd_class, &class_attr_state);

    INIT_LIST_HEAD(&atcmd_pool);
    return 0;
}

static __exit void atcmd_exit(void)
{
    pr_info("%s\n", __func__);
    class_remove_file(atcmd_class, &class_attr_name);
    class_remove_file(atcmd_class, &class_attr_state);
    unregister_chrdev(atcmd_major, "modem");
}

module_init(atcmd_init);
module_exit(atcmd_exit);

MODULE_LICENSE("Dual BSD/GPL");
/* [END] hansun.lee@lge.com 2011.04.14 */
#endif
