/*
 * f_mlog.c - A acm driver for output modem's trace to PC Host
 *
 * Copyright (C) 2015 RDA Microelectronics Inc.
 * Author: XiaoQing xu<xiaoqingxu@rdamicro.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */


#include <linux/module.h>
#include <linux/init.h>
#include <linux/poll.h>
#include <linux/delay.h>
#include <linux/wait.h>
#include <linux/err.h>
#include <linux/interrupt.h>
#include <linux/types.h>
#include <linux/device.h>
#include <linux/miscdevice.h>
#include <linux/usb/composite.h>
#include <linux/usb/gadget.h>
#include <linux/usb/cdc.h>
#include <linux/sched.h>
#include <linux/kthread.h>
#include <linux/dma-mapping.h>
#include <plat/rda_md.h>
#include <plat/md_sys.h>
#include <plat/dma.h>
#include <plat/reg_md.h>


/* #define ACM_NO_CONTROL_ENDPOINT */
#define MLOG_XFER_WITH_DMA

#ifdef MLOG_XFER_WITH_DMA
struct mlog_dma{
	struct rda_dma_chan_params param;
	struct completion complet;
	u8 ch;
};
#endif

struct mlog_dev {
	struct usb_function		func;
	struct usb_composite_dev *cdev;
	struct md_port *port;

#ifdef MLOG_XFER_WITH_DMA
	struct mlog_dma tx_dma;
#endif

	struct usb_ep			*in;
	struct usb_ep			*out;
	struct usb_request *rx_req;
	struct list_head tx_idle;


	spinlock_t lock;/*lock for tx_idle list*/
	atomic_t online; /*status for usb cable plug */
	atomic_t rx_done;

	wait_queue_head_t tx_wq;
	wait_queue_head_t rx_wq;

	struct task_struct *tx_task;
	struct task_struct *rx_task;

	int should_stop;/*status for android usb function change, init.usb.rc */

	u8				data_id;
	u8				pending;

	/* notify_lock is mostly for pending and notify_req ... they get accessed
	 * by callbacks both from tty (open/close/break) under its spinlock,
	 * and notify_req.complete() which can't use that lock.
	 */
	spinlock_t			notify_lock;

#ifndef ACM_NO_CONTROL_ENDPOINT
	u8				ctrl_id;
	struct usb_ep			*notify;
	struct usb_request		*notify_req;
#endif

	struct usb_cdc_line_coding	port_line_coding;	/* 8-N-1 etc */

	/* SetControlLineState request -- CDC 1.1 section 6.2.14 (INPUT) */
	u16				port_handshake_bits;
#define ACM_CTRL_RTS	(1 << 1)	/* unused with full duplex */
#define ACM_CTRL_DTR	(1 << 0)	/* host is ready for data r/w */

	/* SerialState notification -- CDC 1.1 section 6.3.5 (OUTPUT) */
	u16				serial_state;
#define ACM_CTRL_OVERRUN	(1 << 6)
#define ACM_CTRL_PARITY		(1 << 5)
#define ACM_CTRL_FRAMING	(1 << 4)
#define ACM_CTRL_RI		(1 << 3)
#define ACM_CTRL_BRK		(1 << 2)
#define ACM_CTRL_DSR		(1 << 1)
#define ACM_CTRL_DCD		(1 << 0)

};


#define MSYS_TRACE_CMD_TO_DBGHOST	0
#define MSYS_TRACE_CMD_TO_AP		2

#define MLOG_TX_BUFFER_SIZE		256
#define MLOG_RX_BUFFER_SIZE		256
#define MLOG_TX_BUFFER_NUM_MAX		128
#define SHOULD_STOP			(mlog->should_stop == 1)
#define ONLINE				(atomic_read(&mlog->online) == 1)
#define RX_DONE				(atomic_read(&mlog->rx_done) == 1)


/*-------------------------------------------------------------------------*/

/* notification endpoint uses smallish and infrequent fixed-size messages */

#define GS_NOTIFY_INTERVAL_MS		32
#define GS_NOTIFY_MAXPACKET		10	/* notification + 2 bytes */

/* interface and class descriptors: */

static struct usb_interface_assoc_descriptor
mlog_iad_descriptor = {
	.bLength =		sizeof mlog_iad_descriptor,
	.bDescriptorType =	USB_DT_INTERFACE_ASSOCIATION,

	/* .bFirstInterface =	DYNAMIC, */
#ifndef ACM_NO_CONTROL_ENDPOINT
	.bInterfaceCount = 	2,	// control + data
#else
	.bInterfaceCount = 	1,	// only data
#endif
	.bFunctionClass =	USB_CLASS_COMM,
	.bFunctionSubClass =	USB_CDC_SUBCLASS_ACM,
	.bFunctionProtocol =	USB_CDC_ACM_PROTO_AT_V25TER,
	/* .iFunction =		DYNAMIC */
};

#ifndef ACM_NO_CONTROL_ENDPOINT
static struct usb_interface_descriptor mlog_control_interface_desc = {
	.bLength =		USB_DT_INTERFACE_SIZE,
	.bDescriptorType =	USB_DT_INTERFACE,
	/* .bInterfaceNumber = DYNAMIC */
	.bNumEndpoints =	1,
	.bInterfaceClass =	USB_CLASS_COMM,
	.bInterfaceSubClass =	USB_CDC_SUBCLASS_ACM,
	.bInterfaceProtocol =	USB_CDC_ACM_PROTO_AT_V25TER,
	/* .iInterface = DYNAMIC */
};
#endif

static struct usb_interface_descriptor mlog_data_interface_desc = {
	.bLength =		USB_DT_INTERFACE_SIZE,
	.bDescriptorType =	USB_DT_INTERFACE,
	/* .bInterfaceNumber = DYNAMIC */
	.bNumEndpoints =	2,
	.bInterfaceClass =	USB_CLASS_CDC_DATA,
	.bInterfaceSubClass =	0,
	.bInterfaceProtocol =	0,
	/* .iInterface = DYNAMIC */
};

static struct usb_cdc_header_desc mlog_header_desc = {
	.bLength =		sizeof(mlog_header_desc),
	.bDescriptorType =	USB_DT_CS_INTERFACE,
	.bDescriptorSubType =	USB_CDC_HEADER_TYPE,
	.bcdCDC =		cpu_to_le16(0x0110),
};

static struct usb_cdc_call_mgmt_descriptor
mlog_call_mgmt_descriptor = {
	.bLength =		sizeof(mlog_call_mgmt_descriptor),
	.bDescriptorType =	USB_DT_CS_INTERFACE,
	.bDescriptorSubType =	USB_CDC_CALL_MANAGEMENT_TYPE,
	.bmCapabilities =	0,
	/* .bDataInterface = DYNAMIC */
};

static struct usb_cdc_acm_descriptor mlog_descriptor = {
	.bLength =		sizeof(mlog_descriptor),
	.bDescriptorType =	USB_DT_CS_INTERFACE,
	.bDescriptorSubType =	USB_CDC_ACM_TYPE,
	.bmCapabilities =	USB_CDC_CAP_LINE,
};

static struct usb_cdc_union_desc mlog_union_desc = {
	.bLength =		sizeof(mlog_union_desc),
	.bDescriptorType =	USB_DT_CS_INTERFACE,
	.bDescriptorSubType =	USB_CDC_UNION_TYPE,
	/* .bMasterInterface0 =	DYNAMIC */
	/* .bSlaveInterface0 =	DYNAMIC */
};

/* full speed support: */
#ifndef ACM_NO_CONTROL_ENDPOINT
static struct usb_endpoint_descriptor mlog_fs_notify_desc = {
	.bLength =		USB_DT_ENDPOINT_SIZE,
	.bDescriptorType =	USB_DT_ENDPOINT,
	.bEndpointAddress =	USB_DIR_IN,
	.bmAttributes =		USB_ENDPOINT_XFER_INT,
	.wMaxPacketSize =	cpu_to_le16(GS_NOTIFY_MAXPACKET),
	.bInterval =		GS_NOTIFY_INTERVAL_MS,
};
#endif

static struct usb_endpoint_descriptor mlog_fs_in_desc = {
	.bLength =		USB_DT_ENDPOINT_SIZE,
	.bDescriptorType =	USB_DT_ENDPOINT,
	.bEndpointAddress =	USB_DIR_IN,
	.bmAttributes =		USB_ENDPOINT_XFER_BULK,
};

static struct usb_endpoint_descriptor mlog_fs_out_desc = {
	.bLength =		USB_DT_ENDPOINT_SIZE,
	.bDescriptorType =	USB_DT_ENDPOINT,
	.bEndpointAddress =	USB_DIR_OUT,
	.bmAttributes =		USB_ENDPOINT_XFER_BULK,
};

static struct usb_descriptor_header *mlog_fs_function[] = {
	(struct usb_descriptor_header *) &mlog_iad_descriptor,
#ifndef ACM_NO_CONTROL_ENDPOINT
	(struct usb_descriptor_header *) &mlog_control_interface_desc,
#endif
	(struct usb_descriptor_header *) &mlog_header_desc,
	(struct usb_descriptor_header *) &mlog_call_mgmt_descriptor,
	(struct usb_descriptor_header *) &mlog_descriptor,
	(struct usb_descriptor_header *) &mlog_union_desc,
#ifndef ACM_NO_CONTROL_ENDPOINT
	(struct usb_descriptor_header *) &mlog_fs_notify_desc,
#endif
	(struct usb_descriptor_header *) &mlog_data_interface_desc,
	(struct usb_descriptor_header *) &mlog_fs_in_desc,
	(struct usb_descriptor_header *) &mlog_fs_out_desc,
	NULL,
};

/* high speed support: */
#ifndef ACM_NO_CONTROL_ENDPOINT
static struct usb_endpoint_descriptor mlog_hs_notify_desc = {
	.bLength =		USB_DT_ENDPOINT_SIZE,
	.bDescriptorType =	USB_DT_ENDPOINT,
	.bEndpointAddress =	USB_DIR_IN,
	.bmAttributes =		USB_ENDPOINT_XFER_INT,
	.wMaxPacketSize =	cpu_to_le16(GS_NOTIFY_MAXPACKET),
	.bInterval =		USB_MS_TO_HS_INTERVAL(GS_NOTIFY_INTERVAL_MS),
};
#endif

static struct usb_endpoint_descriptor mlog_hs_in_desc = {
	.bLength =		USB_DT_ENDPOINT_SIZE,
	.bDescriptorType =	USB_DT_ENDPOINT,
	.bmAttributes =		USB_ENDPOINT_XFER_BULK,
	.wMaxPacketSize =	cpu_to_le16(512),
};

static struct usb_endpoint_descriptor mlog_hs_out_desc = {
	.bLength =		USB_DT_ENDPOINT_SIZE,
	.bDescriptorType =	USB_DT_ENDPOINT,
	.bmAttributes =		USB_ENDPOINT_XFER_BULK,
	.wMaxPacketSize =	cpu_to_le16(512),
};

static struct usb_descriptor_header *mlog_hs_function[] = {
	(struct usb_descriptor_header *) &mlog_iad_descriptor,
#ifndef ACM_NO_CONTROL_ENDPOINT
	(struct usb_descriptor_header *) &mlog_control_interface_desc,
#endif
	(struct usb_descriptor_header *) &mlog_header_desc,
	(struct usb_descriptor_header *) &mlog_call_mgmt_descriptor,
	(struct usb_descriptor_header *) &mlog_descriptor,
	(struct usb_descriptor_header *) &mlog_union_desc,
#ifndef ACM_NO_CONTROL_ENDPOINT
	(struct usb_descriptor_header *) &mlog_hs_notify_desc,
#endif
	(struct usb_descriptor_header *) &mlog_data_interface_desc,
	(struct usb_descriptor_header *) &mlog_hs_in_desc,
	(struct usb_descriptor_header *) &mlog_hs_out_desc,
	NULL,
};

static struct usb_endpoint_descriptor mlog_ss_in_desc = {
	.bLength =		USB_DT_ENDPOINT_SIZE,
	.bDescriptorType =	USB_DT_ENDPOINT,
	.bmAttributes =		USB_ENDPOINT_XFER_BULK,
	.wMaxPacketSize =	cpu_to_le16(1024),
};

static struct usb_endpoint_descriptor mlog_ss_out_desc = {
	.bLength =		USB_DT_ENDPOINT_SIZE,
	.bDescriptorType =	USB_DT_ENDPOINT,
	.bmAttributes =		USB_ENDPOINT_XFER_BULK,
	.wMaxPacketSize =	cpu_to_le16(1024),
};

static struct usb_ss_ep_comp_descriptor mlog_ss_bulk_comp_desc = {
	.bLength =              sizeof mlog_ss_bulk_comp_desc,
	.bDescriptorType =      USB_DT_SS_ENDPOINT_COMP,
};

static struct usb_descriptor_header *mlog_ss_function[] = {
	(struct usb_descriptor_header *) &mlog_iad_descriptor,
#ifndef ACM_NO_CONTROL_ENDPOINT
	(struct usb_descriptor_header *) &mlog_control_interface_desc,
#endif
	(struct usb_descriptor_header *) &mlog_header_desc,
	(struct usb_descriptor_header *) &mlog_call_mgmt_descriptor,
	(struct usb_descriptor_header *) &mlog_descriptor,
	(struct usb_descriptor_header *) &mlog_union_desc,
#ifndef ACM_NO_CONTROL_ENDPOINT
	(struct usb_descriptor_header *) &mlog_hs_notify_desc,
#endif
	(struct usb_descriptor_header *) &mlog_ss_bulk_comp_desc,
	(struct usb_descriptor_header *) &mlog_data_interface_desc,
	(struct usb_descriptor_header *) &mlog_ss_in_desc,
	(struct usb_descriptor_header *) &mlog_ss_bulk_comp_desc,
	(struct usb_descriptor_header *) &mlog_ss_out_desc,
	(struct usb_descriptor_header *) &mlog_ss_bulk_comp_desc,
	NULL,
};

/* string descriptors: */

#define MLOG_CTRL_IDX	0
#define MLOG_DATA_IDX	1
#define MLOG_IAD_IDX	2

/* static strings, in UTF-8 */
static struct usb_string mlog_string_defs[] = {
	[MLOG_CTRL_IDX].s = "CDC Abstract Control Model (mlog)",
	[MLOG_DATA_IDX].s = "CDC mlog Data",
	[MLOG_IAD_IDX ].s = "CDC Serial",
	{  } /* end of list */
};

static struct usb_gadget_strings mlog_string_table = {
	.language =		0x0409,	/* en-us */
	.strings =		mlog_string_defs,
};

static struct usb_gadget_strings *mlog_strings[] = {
	&mlog_string_table,
	NULL,
};

#ifndef ACM_NO_CONTROL_ENDPOINT
static int mlog_notify_serial_state(struct mlog_dev *mlog);
#endif

static inline struct mlog_dev *func_to_mlog(struct usb_function *f)
{
	return container_of(f, struct mlog_dev, func);
}


/* add a request to the tail of a list */
static void mlog_req_put(struct mlog_dev *dev, struct list_head *head,
		struct usb_request *req)
{
	unsigned long flags;

	spin_lock_irqsave(&dev->lock, flags);
	list_add_tail(&req->list, head);
	spin_unlock_irqrestore(&dev->lock, flags);
}

/* remove a request from the head of a list */
static struct usb_request *mlog_req_get(struct mlog_dev *dev,
		struct list_head *head)
{
	unsigned long flags;
	struct usb_request *req;

	spin_lock_irqsave(&dev->lock, flags);
	if (list_empty(head)) {
		req = 0;
	} else {
		req = list_first_entry(head, struct usb_request, list);
		list_del(&req->list);
	}
	spin_unlock_irqrestore(&dev->lock, flags);
	return req;
}


static struct usb_request *mlog_alloc_req(struct usb_ep *ep,
		unsigned len, gfp_t kmalloc_flags)
{
	struct usb_request *req;

	req = usb_ep_alloc_request(ep, kmalloc_flags);

	if (req) {
		req->length = len;
		req->buf = kmalloc(len, kmalloc_flags);
		if (req->buf == NULL) {
			usb_ep_free_request(ep, req);
			return NULL;
		}
	}

	return req;
}

static void mlog_free_req(struct usb_ep *ep, struct usb_request *req)
{
	if (req) {
		kfree(req->buf);
		usb_ep_free_request(ep, req);
	}
}

static void mlog_complete_in(struct usb_ep *ep, struct usb_request *req)
{
	struct mlog_dev *mlog = req->context;

	mlog_req_put(mlog, &mlog->tx_idle, req);
	wake_up(&mlog->tx_wq);
}

/* when usb cable plugout, all submitted req will return directly with req.actual=0*/
static void mlog_complete_out(struct usb_ep *ep, struct usb_request *req)
{
	struct mlog_dev *mlog = req->context;

	if(req->actual > 0){
		atomic_set(&mlog->rx_done, 1);
		wake_up(&mlog->rx_wq);
	}else if(ONLINE){
		atomic_set(&mlog->rx_done, 0);
		usb_ep_queue(mlog->out, req, GFP_ATOMIC);
		pr_err(" %s :get empty rx buf \n", __func__);
	}
}

static void mlog_md_notify(void *arg, unsigned int event)
{
	struct mlog_dev *mlog = (struct mlog_dev *)arg;
	int len = 0;

	if (event != MD_EVENT_DATA)
		return;

	wake_up(&mlog->tx_wq);

	if((len = rda_md_read_avail(mlog->port))<=0){
		pr_err("%s: interrupt, data N/A, tail=%x,head=%x\n",__func__,
			mlog->port->rx.pctrl->tail, mlog->port->rx.pctrl->head);
	}else{
		disable_md_irq(mlog->port);
	}
}

#ifdef MLOG_XFER_WITH_DMA

#define DMA_XFER_MIN_BYTES 16

static unsigned mlog_dma_read_sm(struct mlog_dev *mlog, void **ptr)
{
	struct md_ch *ch;
	unsigned int head, tail;

	ch = &mlog->port->rx;
	head = ch->pctrl->head;
	tail = ch->pctrl->tail;

	*ptr = (void *) (RDA_MD_MAILBOX_PHYS + MD_TRACE_READ_BUF_OFFSET + tail);

	if (tail <= head)
		return head - tail;

	return ch->fifo_size - tail;
}


static void mlog_tx_dma_cb(u8 ch, void *data)
{
	struct mlog_dev *mlog = (struct mlog_dev *)data;

	complete(&mlog->tx_dma.complet);
}

static int mlog_tx_task_fun(void *arg)
{
	int ret, len, dma_len;
	struct usb_request *req = NULL;
	struct mlog_dev *mlog = (struct mlog_dev *)arg;
	void *sm_buf_phy;

	do {
		while(!ONLINE){
			wait_event_interruptible(mlog->tx_wq, (ONLINE || SHOULD_STOP));
			if(SHOULD_STOP)
				goto exit;
		}

		len = rda_md_read_avail(mlog->port);
		while(len <= 0){
			clr_md_irq(mlog->port, TRACE_READY_BIT);
			enable_md_irq(mlog->port);
			wait_event_interruptible(mlog->tx_wq,
				((len = rda_md_read_avail(mlog->port))>0 || SHOULD_STOP));

			if(SHOULD_STOP)
				goto exit;
		}

		req = mlog_req_get(mlog, &mlog->tx_idle);
		while(!req){
			//pr_err("%s, no usb buffer,maybe lost trace data!\n",__func__);
			wait_event_interruptible(mlog->tx_wq,
				((req = mlog_req_get(mlog, &mlog->tx_idle))|| SHOULD_STOP));

			if(SHOULD_STOP)
				goto exit;
		}

		dma_len = mlog_dma_read_sm(mlog, &sm_buf_phy);
		if((len <= DMA_XFER_MIN_BYTES) ||
			((len=dma_len)<= DMA_XFER_MIN_BYTES)){

			ret = rda_md_read(mlog->port, req->buf, len);
			if (ret != len){
				mlog_req_put(mlog, &mlog->tx_idle, req);
				continue;
			}
		}else{
			if(dma_len > MLOG_TX_BUFFER_SIZE)
				dma_len = MLOG_TX_BUFFER_SIZE;

			mlog->tx_dma.param.src_addr = (dma_addr_t)sm_buf_phy;
			mlog->tx_dma.param.dst_addr = req->dma;
			mlog->tx_dma.param.xfer_size = dma_len;
			mlog->tx_dma.param.dma_mode = RDA_DMA_NOR_MODE;
			mlog->tx_dma.param.enable_int = 1;

			ret = rda_set_dma_params(mlog->tx_dma.ch, &mlog->tx_dma.param);
			if (ret < 0) {
				pr_err("%s:failed to set parameter,ret = 0x%x\n",
						__func__,ret);

				mlog_req_put(mlog, &mlog->tx_idle, req);
				continue;
			}

			rda_start_dma(mlog->tx_dma.ch);

			wait_for_completion(&mlog->tx_dma.complet);
			INIT_COMPLETION(mlog->tx_dma.complet);

			rda_md_read_done(&mlog->port->rx, dma_len);
		}

		req->length = dma_len;
		ret = usb_ep_queue(mlog->in, req, GFP_ATOMIC);
		if (ret < 0){
			mlog_req_put(mlog, &mlog->tx_idle, req);
			pr_err("%s: tx error %d,disconnect or disable ep\n",
					__func__,ret);
		}
	} while (!kthread_should_stop());

exit:
   pr_err("%s: exit !\n", __func__);

   return 0;

}

#else
static int mlog_tx_task_fun(void *arg)
{
	int ret, len;
	struct usb_request *req = NULL;
	struct mlog_dev *mlog = (struct mlog_dev *)arg;

	do {
		while(!ONLINE){
			wait_event_interruptible(mlog->tx_wq, (ONLINE || SHOULD_STOP));

			if(SHOULD_STOP)
				goto exit;
		}

		len = rda_md_read_avail(mlog->port);
		while(len <= 0){
			clr_md_irq(mlog->port, TRACE_READY_BIT);
			enable_md_irq(mlog->port);

			wait_event_interruptible(mlog->tx_wq,
				((len = rda_md_read_avail(mlog->port))>0 || SHOULD_STOP));

			if(SHOULD_STOP)
				goto exit;
		}

		req = mlog_req_get(mlog, &mlog->tx_idle);
		while(!req){
			//pr_err("%s, no usb buffer,maybe lost trace data!\n",__func__);
			wait_event_interruptible(mlog->tx_wq,
				((req = mlog_req_get(mlog, &mlog->tx_idle))|| SHOULD_STOP));

			if(SHOULD_STOP)
				goto exit;
		}

		if(len > MLOG_TX_BUFFER_SIZE)
			len = MLOG_TX_BUFFER_SIZE;

		ret = rda_md_read(mlog->port, req->buf, len);
		if (ret != len){
			mlog_req_put(mlog, &mlog->tx_idle, req);
			continue;
		}

		req->length = len;
		ret = usb_ep_queue(mlog->in, req, GFP_ATOMIC);
		if (ret < 0){
			mlog_req_put(mlog, &mlog->tx_idle, req);
			pr_err("%s: tx error %d,disconnect or disable ep\n", __func__,ret);
		}
	} while (!kthread_should_stop());

exit:
   pr_err("%s: exit !\n", __func__);

   return 0;
}

#endif

static int mlog_rx_task_fun(void *arg)
{
	int ret, len,left;
	struct mlog_dev *mlog = (struct mlog_dev *)arg;
	static int cnt = 0;/*how many bytes write to md share buffer*/
	char * buf = (char *)mlog->rx_req->buf;

	do {
		while(!(RX_DONE && ONLINE)){
			wait_event_interruptible(mlog->rx_wq,
				((RX_DONE && ONLINE) || SHOULD_STOP));
			if(SHOULD_STOP)
				goto exit;
		}

		len = rda_md_write_avail(mlog->port);
		while(len <= 0) {
			wait_event_interruptible_timeout(mlog->rx_wq,
				((len = rda_md_write_avail(mlog->port))>0 || SHOULD_STOP),
				msecs_to_jiffies(100));

			if(SHOULD_STOP)
				goto exit;
		}

		left = mlog->rx_req->actual - cnt;
		if(len > left)
			len = left;

		ret = rda_md_write(mlog->port, (void*)(buf + cnt), len);
		if(ret <= 0){
			ret = 0;
			cnt = mlog->rx_req->actual;
			pr_err("%s: rda_md_write err, %d\n", __func__,ret);
		}

		cnt += ret;
		if(cnt == mlog->rx_req->actual){
			cnt = 0;
			atomic_set(&mlog->rx_done, 0);

			mlog->rx_req->length = MLOG_RX_BUFFER_SIZE;
			ret = usb_ep_queue(mlog->out, mlog->rx_req, GFP_ATOMIC);
			if (ret < 0)
				pr_err("%s: rx error %d,disconnect or disable ep\n",
						__func__,ret);
		}

	} while (!kthread_should_stop());

exit:
   pr_err("%s: exit !\n", __func__);

   return 0;
}

enum mlog_msys{
	MSYS_TRACE_BY_AP,
	MSYS_TRACE_BY_CP,
	MSYS_ENABLE_TRACE,
	MSYS_QUERY_TRACE_STATUS,
};

static int mlog_send_msys_cmd(enum mlog_msys cmd, void *data)
{
	static struct msys_device *msys_dev = NULL;
	struct client_cmd cmd_set;
	unsigned int sub_cmd;
	int ret = 0;
	unsigned int status;

	if(!msys_dev){
		msys_dev = rda_msys_alloc_device();
		if (!msys_dev) {
			pr_err("%s: can not allocate mlog_msys device\n", __func__);
			msys_dev = NULL;
			return -1;
		}

		msys_dev->module = SYS_GEN_MOD;
		msys_dev->name = "rda-trace";

		ret = rda_msys_register_device(msys_dev);
		if (ret < 0) {
			pr_err("%s: could not register with mlog msys_dev\n",__func__);
			rda_msys_free_device(msys_dev);
			msys_dev = NULL;
			return -1;
		}
	}

	memset(&cmd_set, 0, sizeof(cmd_set));
	cmd_set.pmsys_dev = msys_dev;
	switch(cmd){
	case MSYS_TRACE_BY_AP:
		cmd_set.mod_id = SYS_GEN_MOD;
		cmd_set.mesg_id = SYS_GEN_CMD_TRACE;
		sub_cmd = MSYS_TRACE_CMD_TO_AP;
		cmd_set.pdata = (void *)&sub_cmd;
		cmd_set.data_size = sizeof(sub_cmd);
		break;
	case MSYS_TRACE_BY_CP:
		cmd_set.mod_id = SYS_GEN_MOD;
		cmd_set.mesg_id = SYS_GEN_CMD_TRACE;
		sub_cmd = MSYS_TRACE_CMD_TO_DBGHOST;
		cmd_set.pdata = (void *)&sub_cmd;
		cmd_set.data_size = sizeof(sub_cmd);
		break;
	case MSYS_ENABLE_TRACE:
		cmd_set.mod_id = SYS_GEN_MOD;
		cmd_set.mesg_id = SYS_GEN_CMD_ENABLE_TRACE_LOG;
		sub_cmd = *(unsigned int *)data;
		cmd_set.pdata = (void *)&sub_cmd;
		cmd_set.data_size = sizeof(sub_cmd);
		break;
	case MSYS_QUERY_TRACE_STATUS:
		cmd_set.mod_id = SYS_GEN_MOD;
		cmd_set.mesg_id = SYS_GEN_CMD_QUERY_TRACE_STATUS;
		cmd_set.pout_data = (void *)&status;
		cmd_set.out_size = sizeof(status);
		break;
	default:
		return -1;
	}

	ret = rda_msys_send_cmd(&cmd_set);
	if (ret > 0){
		pr_err("mlog failed to send msys cmd!\n");
		ret = -1;
	}else if ((cmd == MSYS_QUERY_TRACE_STATUS) && data){
		*(unsigned int *)data = status;
		ret = 0;
	}

	return ret;
}



/*--------------------start control endpoint---------------------------*/


/* mlog control ... data handling is delegated to tty library code.
 * The main task of this function is to activate and deactivate
 * that code based on device state; track parameters like line
 * speed, handshake state, and so on; and issue notifications.
 */
#ifndef ACM_NO_CONTROL_ENDPOINT

static void mlog_complete_set_line_coding(struct usb_ep *ep,
		struct usb_request *req)
{
	struct mlog_dev	*mlog = ep->driver_data;

	if (req->status != 0) {
		pr_err("%s, req->status err %d\n",__func__, req->status);
		return;
	}

	/* normal completion */
	if (req->actual != sizeof(mlog->port_line_coding)) {
		pr_err("%s short resp, len %d\n", __func__, req->actual);
		usb_ep_set_halt(ep);
	} else {
		struct usb_cdc_line_coding	*value = req->buf;

		/* REVISIT:  we currently just remember this data.
		 * If we change that, (a) validate it first, then
		 * (b) update whatever hardware needs updating,
		 * (c) worry about locking.  This is information on
		 * the order of 9600-8-N-1 ... most of which means
		 * nothing unless we control a real RS232 line.
		 */
		mlog->port_line_coding = *value;
	}
}
#endif

static int mlog_setup(struct usb_function *f,
		const struct usb_ctrlrequest *ctrl)
{
	struct mlog_dev		*mlog = func_to_mlog(f);
	struct usb_composite_dev *cdev = f->config->cdev;
	struct usb_request	*req = cdev->req;
	int			value = -EOPNOTSUPP;
	u16			w_value = le16_to_cpu(ctrl->wValue);
#ifndef ACM_NO_CONTROL_ENDPOINT
	u16                     w_index = le16_to_cpu(ctrl->wIndex);
	u16			w_length = le16_to_cpu(ctrl->wLength);


	/* composite driver infrastructure handles everything except
	 * CDC class messages; interface activation uses set_alt().
	 *
	 * Note CDC spec table 4 lists the acm request profile.  It requires
	 * encapsulated command support ... we don't handle any, and respond
	 * to them by stalling.  Options include get/set/clear comm features
	 * (not that useful) and SEND_BREAK.
	 */
	switch ((ctrl->bRequestType << 8) | ctrl->bRequest) {

	/* SET_LINE_CODING ... just read and save what the host sends */
	case ((USB_DIR_OUT | USB_TYPE_CLASS | USB_RECIP_INTERFACE) << 8)
			| USB_CDC_REQ_SET_LINE_CODING:
		if (w_length != sizeof(struct usb_cdc_line_coding)
				|| w_index != mlog->ctrl_id)
			goto invalid;

		value = w_length;
		cdev->gadget->ep0->driver_data = mlog;
		req->complete = mlog_complete_set_line_coding;
		break;

	/* GET_LINE_CODING ... return what host sent, or initial value */
	/* !!Notice: maybe continual into this case for some host application */
	case ((USB_DIR_IN | USB_TYPE_CLASS | USB_RECIP_INTERFACE) << 8)
			| USB_CDC_REQ_GET_LINE_CODING:
		if (w_index != mlog->ctrl_id)
			goto invalid;

		value = min_t(unsigned, w_length,
				sizeof(struct usb_cdc_line_coding));
		memcpy(req->buf, &mlog->port_line_coding, value);
		break;

	/* SET_CONTROL_LINE_STATE ... save what the host sent */
	case ((USB_DIR_OUT | USB_TYPE_CLASS | USB_RECIP_INTERFACE) << 8)
			| USB_CDC_REQ_SET_CONTROL_LINE_STATE:
		if (w_index != mlog->ctrl_id)
			goto invalid;

		value = 0;

		/* FIXME we should not allow data to flow until the
		 * host sets the mlog_CTRL_DTR bit; and when it clears
		 * that bit, we should return to that no-flow state.
		 */
		mlog->port_handshake_bits = w_value;
		break;

	default:
invalid:
		pr_err("%s invalid control req%02x.%02x v%04x i%04x l%d\n",
			__func__,ctrl->bRequestType, ctrl->bRequest,
			w_value, w_index, w_length);
	}

#endif

	if ((ctrl->bRequestType & USB_TYPE_MASK) == USB_TYPE_CLASS) {
		if((ctrl->bRequest == 0x22) && ((w_value & 0xff) == 1)) {
			 /* USB host open acm, eg :fopen()
			  *may be many time at one connection*/
			if(!ONLINE){
				atomic_set(&mlog->online, 1);
				wake_up(&mlog->tx_wq);
				atomic_set(&mlog->rx_done, 0);
				usb_ep_queue(mlog->out, mlog->rx_req, GFP_ATOMIC);
			}
			value = 0;
		}
	}

	/* respond with data transfer or status phase? */
	if (value >= 0) {
		req->zero = 0;
		req->length = value;
		value = usb_ep_queue(cdev->gadget->ep0, req, GFP_ATOMIC);
		if (value < 0)
			pr_err("%s, err %d\n", __func__, value);
	}

	return value;
}

static int mlog_set_alt(struct usb_function *f, unsigned intf, unsigned alt)
{
	struct mlog_dev		*mlog = func_to_mlog(f);
	struct usb_composite_dev *cdev = f->config->cdev;
	int status;

#ifndef ACM_NO_CONTROL_ENDPOINT
	if (intf == mlog->ctrl_id) {
		if (!mlog->notify->desc)
			if (config_ep_by_speed(cdev->gadget, f, mlog->notify))
				return -EINVAL;

		usb_ep_enable(mlog->notify);
		mlog->notify->driver_data = mlog;
	}else if (intf == mlog->data_id) {
#else
	if (intf == mlog->data_id) {
#endif
		if (!mlog->in->desc || !mlog->out->desc) {
			if (config_ep_by_speed(cdev->gadget, f,
					       mlog->in) ||
			    config_ep_by_speed(cdev->gadget, f,
					       mlog->out)) {
				mlog->in->desc = NULL;
				mlog->out->desc = NULL;
				return -EINVAL;
			}
		}

		status = usb_ep_enable(mlog->in);
		if (status < 0)
			return status;

		status = usb_ep_enable(mlog->out);
		if (status < 0){
			usb_ep_disable(mlog->in);
			return status;
		}

	} else
		return -EINVAL;

	return 0;
}

/*--------------------end control endpoint--------------------------*/

#ifndef ACM_NO_CONTROL_ENDPOINT

/*---------------------start notify---------------------------------*/

/**
 * mlog_cdc_notify - issue CDC notification to host
 * @mlog: wraps host to be notified
 * @type: notification type
 * @value: Refer to cdc specs, wValue field.
 * @data: data to be sent
 * @length: size of data
 * Context: irqs blocked, mlog->lock held, mlog_notify_req non-null
 *
 * Returns zero on success or a negative errno.
 *
 * See section 6.3.5 of the CDC 1.1 specification for information
 * about the only notification we issue:  SerialState change.
 */
static int mlog_cdc_notify(struct mlog_dev *mlog, u8 type, u16 value,
		void *data, unsigned length)
{
	struct usb_ep			*ep = mlog->notify;
	struct usb_request		*req;
	struct usb_cdc_notification	*notify;
	const unsigned			len = sizeof(*notify) + length;
	void				*buf;
	int				status;

	req = mlog->notify_req;
	mlog->notify_req = NULL;
	mlog->pending = false;

	req->length = len;
	notify = req->buf;
	buf = notify + 1;

	notify->bmRequestType = USB_DIR_IN | USB_TYPE_CLASS
			| USB_RECIP_INTERFACE;
	notify->bNotificationType = type;
	notify->wValue = cpu_to_le16(value);
	notify->wIndex = cpu_to_le16(mlog->ctrl_id);
	notify->wLength = cpu_to_le16(length);
	memcpy(buf, data, length);

	/* ep_queue() can complete immediately if it fills the fifo... */
	spin_unlock(&mlog->notify_lock);
	status = usb_ep_queue(ep, req, GFP_ATOMIC);
	spin_lock(&mlog->notify_lock);

	if (status < 0) {
	   pr_err("%s can't notify serial state, %d\n",__func__, status);
	   mlog->notify_req = req;
	}

	return status;
}

static int mlog_notify_serial_state(struct mlog_dev *mlog)
{
	int			status;

	spin_lock(&mlog->notify_lock);
	if (mlog->notify_req) {
		pr_info("%s ,serial state %04x\n", __func__, mlog->serial_state);
		status = mlog_cdc_notify(mlog, USB_CDC_NOTIFY_SERIAL_STATE,
				0, &mlog->serial_state, sizeof(mlog->serial_state));
	} else {
		mlog->pending = true;
		status = 0;
	}
	spin_unlock(&mlog->notify_lock);

	return status;
}

static void mlog_complete_notify(struct usb_ep *ep,
		struct usb_request *req)
{
	struct mlog_dev	*mlog = req->context;
	u8	doit = false;

	spin_lock(&mlog->notify_lock);
	if (req->status != -ESHUTDOWN)
		doit = mlog->pending;
	mlog->notify_req = req;
	spin_unlock(&mlog->notify_lock);

	if (doit)
		mlog_notify_serial_state(mlog);
}

#endif

/*---------------------end notify---------------------------------*/


static void mlog_disable(struct usb_function *f)
{
	struct mlog_dev	*mlog = func_to_mlog(f);

	atomic_set(&mlog->online, 0);
	atomic_set(&mlog->rx_done, 0);
	usb_ep_disable(mlog->out);
	usb_ep_disable(mlog->in);
#ifndef ACM_NO_CONTROL_ENDPOINT
	usb_ep_disable(mlog->notify);
#endif
	pr_err("%s\n", __func__);
}


/* mlog function driver setup/binding */
static int mlog_bind(struct usb_configuration *c,
		struct usb_function *f)
{
	struct usb_composite_dev *cdev = c->cdev;
	struct mlog_dev		*mlog = func_to_mlog(f);
	struct usb_string	*us;
	int			status = -1;
	struct usb_ep		*ep;
	struct usb_request *req;
	int i;

	pr_err("%s\n", __func__);

	/* REVISIT might want instance-specific strings to help
	 * distinguish instances ...
	 */

	/* maybe allocate device-global string IDs, and patch descriptors */
	us = usb_gstrings_attach(cdev, mlog_strings,
			ARRAY_SIZE(mlog_string_defs));
	if (IS_ERR(us))
		return PTR_ERR(us);
#ifndef ACM_NO_CONTROL_ENDPOINT
	mlog_control_interface_desc.iInterface = us[MLOG_CTRL_IDX].id;
#endif
	mlog_data_interface_desc.iInterface = us[MLOG_DATA_IDX].id;
	mlog_iad_descriptor.iFunction = us[MLOG_IAD_IDX].id;
#ifndef ACM_NO_CONTROL_ENDPOINT
	/* allocate instance-specific interface IDs, and patch descriptors */
	status = usb_interface_id(c, f);
	if (status < 0)
		goto fail;
	mlog->ctrl_id = status;
	mlog_iad_descriptor.bFirstInterface = status;
	mlog_control_interface_desc.bInterfaceNumber = status;
#endif
	mlog_union_desc .bMasterInterface0 = status;

	status = usb_interface_id(c, f);
	if (status < 0)
		goto fail;
	mlog->data_id = status;

	mlog_data_interface_desc.bInterfaceNumber = status;
	mlog_union_desc.bSlaveInterface0 = status;
	mlog_call_mgmt_descriptor.bDataInterface = status;

	status = -ENODEV;

	/* allocate instance-specific endpoints */
	ep = usb_ep_autoconfig(cdev->gadget, &mlog_fs_in_desc);
	if (!ep)
		goto fail;
	mlog->in = ep;
	ep->driver_data = cdev;	/* claim */

	ep = usb_ep_autoconfig(cdev->gadget, &mlog_fs_out_desc);
	if (!ep)
		goto fail;
	mlog->out = ep;
	ep->driver_data = cdev;	/* claim */

#ifndef ACM_NO_CONTROL_ENDPOINT
	ep = usb_ep_autoconfig(cdev->gadget, &mlog_fs_notify_desc);
	if (!ep)
		goto fail;
	mlog->notify = ep;
	ep->driver_data = cdev;	/* claim */
	/* allocate notification */
	mlog->notify_req = mlog_alloc_req(ep,
			sizeof(struct usb_cdc_notification) + 2,
			GFP_KERNEL);
	if (!mlog->notify_req)
		goto fail;

	mlog->notify_req->complete = mlog_complete_notify;
	mlog->notify_req->context = mlog;
#endif

    /* allocate requests for data rx/tx */
	req = mlog_alloc_req(mlog->out, MLOG_RX_BUFFER_SIZE,GFP_KERNEL);
	if (!req)
		goto err1;
	req->complete = mlog_complete_out;
	mlog->rx_req = req;
	mlog->rx_req->context = mlog;

	for (i = 0; i < MLOG_TX_BUFFER_NUM_MAX; i++) {
		req = mlog_alloc_req(mlog->in, MLOG_TX_BUFFER_SIZE, GFP_KERNEL);
		if (!req)
			goto err2;
		req->complete = mlog_complete_in;
		req->context = mlog;
#ifdef MLOG_XFER_WITH_DMA
		req->dma = dma_map_single(0,req->buf,
							MLOG_TX_BUFFER_SIZE, DMA_FROM_DEVICE);
#endif
		mlog_req_put(mlog, &mlog->tx_idle, req);
	}

	/* support all relevant hardware speeds... we expect that when
	 * hardware is dual speed, all bulk-capable endpoints work at
	 * both speeds
	 */
	mlog_hs_in_desc.bEndpointAddress = mlog_fs_in_desc.bEndpointAddress;
	mlog_hs_out_desc.bEndpointAddress = mlog_fs_out_desc.bEndpointAddress;
#ifndef ACM_NO_CONTROL_ENDPOINT
	mlog_hs_notify_desc.bEndpointAddress =
		mlog_fs_notify_desc.bEndpointAddress;
#endif
	mlog_ss_in_desc.bEndpointAddress = mlog_fs_in_desc.bEndpointAddress;
	mlog_ss_out_desc.bEndpointAddress = mlog_fs_out_desc.bEndpointAddress;

	status = usb_assign_descriptors(f, mlog_fs_function, mlog_hs_function,
			mlog_ss_function);
	if (status)
		goto err2;

#ifndef ACM_NO_CONTROL_ENDPOINT
	pr_err("%s ttyGS: %s speed IN/%s OUT/%s NOTIFY/%s\n", __func__,
			gadget_is_superspeed(c->cdev->gadget) ? "super" :
			gadget_is_dualspeed(c->cdev->gadget) ? "dual" : "full",
			mlog->in->name, mlog->out->name,
			mlog->notify->name);
#else
	pr_err("%s ttyGS: %s speed IN/%s OUT/%s \n", __func__,
			gadget_is_superspeed(c->cdev->gadget) ? "super" :
			gadget_is_dualspeed(c->cdev->gadget) ? "dual" : "full",
			mlog->in->name, mlog->out->name);

#endif

	return 0;

err2:
	if (mlog->rx_req)
		mlog_free_req(mlog->out, mlog->rx_req);

	while ((req = mlog_req_get(mlog, &mlog->tx_idle)))
		mlog_free_req(mlog->in, req);

err1:
#ifndef ACM_NO_CONTROL_ENDPOINT
	if (mlog->notify_req)
		mlog_free_req(mlog->notify, mlog->notify_req);
#endif

fail:
	pr_err("%s/%p: can't bind, err %d\n", f->name, f, status);

	return status;
}

static void mlog_unbind(struct usb_configuration *c,
			struct usb_function *f)
{
	struct mlog_dev		*mlog = func_to_mlog(f);
	struct usb_request *req;

	if(mlog_send_msys_cmd(MSYS_TRACE_BY_CP, 0) == -1) {
		pr_err("mlog failed as change modem trace to dbghost!\n");
	}

	rda_md_close(mlog->port);
	mlog->should_stop = 1;

	kthread_stop(mlog->tx_task);
	kthread_stop(mlog->rx_task);

	mlog_string_defs[0].id = 0;
	usb_free_all_descriptors(f);

	mlog_free_req(mlog->out, mlog->rx_req);
#ifndef ACM_NO_CONTROL_ENDPOINT
	mlog_free_req(mlog->notify, mlog->notify_req);
#endif

	while ((req = mlog_req_get(mlog, &mlog->tx_idle)) != NULL){
#ifdef MLOG_XFER_WITH_DMA
		dma_unmap_single(0, req->dma, MLOG_TX_BUFFER_SIZE, DMA_FROM_DEVICE);
#endif
		mlog_free_req(mlog->in, req);
	}

#ifdef MLOG_XFER_WITH_DMA
	rda_free_dma(mlog->tx_dma.ch);
#endif
	kfree(mlog);

	pr_err("%s\n", __func__);
}

static struct mlog_dev * mlog_bind_config(struct usb_configuration *c)
{
	struct mlog_dev *mlog;
	int ret = 0;

	pr_err("%s\n", __func__);

	mlog = kzalloc(sizeof(*mlog), GFP_KERNEL);
	if (!mlog)
		return 0;

	spin_lock_init(&mlog->notify_lock);
	spin_lock_init(&mlog->lock);

	init_waitqueue_head(&mlog->tx_wq);
	init_waitqueue_head(&mlog->rx_wq);

	atomic_set(&mlog->online, 0);
	atomic_set(&mlog->rx_done, 0);

	INIT_LIST_HEAD(&mlog->tx_idle);

	mlog->should_stop = 0;

	mlog->cdev = c->cdev;
	mlog->func.name = "mlog";
	mlog->func.strings = mlog_strings;
	mlog->func.bind = mlog_bind;
	mlog->func.set_alt = mlog_set_alt;
	mlog->func.setup = mlog_setup;
	mlog->func.disable = mlog_disable;
	mlog->func.unbind = mlog_unbind;

#ifdef MLOG_XFER_WITH_DMA
	rda_request_dma(0, "mlog-tx-dma", mlog_tx_dma_cb, mlog,
				&mlog->tx_dma.ch);
	init_completion(&mlog->tx_dma.complet);
#endif

	ret = rda_md_open(MD_PORT_TRACE, &mlog->port, mlog, mlog_md_notify);
	if (ret < 0) {
		pr_err("%s:Open TARCE port failed, %d\n", __func__,ret);
		goto err;
	}

	mlog->tx_task = kthread_run(mlog_tx_task_fun, mlog, "mlog_tx");
	mlog->rx_task = kthread_run(mlog_rx_task_fun, mlog, "mlog_rx");

	ret = usb_add_function(c, &mlog->func);
	if (ret)
		goto err1;

	if(mlog_send_msys_cmd(MSYS_TRACE_BY_AP, 0)== -1){
		pr_err("mlog failed as change modem trae to ap!\n");
		goto err1;
	}

	enable_md_irq(mlog->port);

	return mlog;

err1:
	rda_md_close(mlog->port);
err:
#ifdef MLOG_XFER_WITH_DMA
	rda_free_dma(mlog->tx_dma.ch);
#endif
	kfree(mlog);
	pr_err("mlog gadget driver failed to initialize\n");
	return 0;
}

