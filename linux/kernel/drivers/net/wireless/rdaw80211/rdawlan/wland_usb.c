
/*
 * Copyright (c) 2014 Rdamicro Corporation
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION
 * OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#ifdef WLAND_USB_SUPPORT

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/firmware.h>
#include <linux/usb.h>
#include <linux/vmalloc.h>

#include "wland_utils.h"
#include "wland_bus.h"
#include "wland_dbg.h"
#include "wland_usb.h"

static void wland_usb_rx_refill(struct wland_usbdev_info *devinfo,
	struct wland_usbreq *req);

static struct usb_device_id wland_usb_devid_table[] = {
	{USB_DEVICE(USB_VENDOR_ID_RDAMICRO, USB_DEVICE_ID_RDA599X)},
	/*
	 * special entry for device with firmware loaded and running
	 */
	{USB_DEVICE(USB_VENDOR_ID_RDAMICRO, USB_DEVICE_ID_BCMFW)},
	{}
};

static struct wland_usbdev_info *wland_usb_get_businfo(struct device *dev)
{
	struct wland_bus *bus_if = dev_get_drvdata(dev);

	return bus_if->bus_priv.usb->devinfo;
}

static int wland_usb_ioctl_resp_wait(struct wland_usbdev_info *devinfo)
{
	return wait_event_timeout(devinfo->ioctl_resp_wait,
		devinfo->ctl_completed, msecs_to_jiffies(IOCTL_RESP_TIMEOUT));
}

static void wland_usb_ioctl_resp_wake(struct wland_usbdev_info *devinfo)
{
	if (waitqueue_active(&devinfo->ioctl_resp_wait))
		wake_up(&devinfo->ioctl_resp_wait);
}

static void wland_usb_ctl_complete(struct wland_usbdev_info *devinfo, int type,
	int status)
{
	WLAND_DBG(USB, TRACE, "Enter, status=%d\n", status);

	if (unlikely(devinfo == NULL))
		return;

	if (type == WLAND_USB_CBCTL_READ) {
		if (status == 0)
			devinfo->bus_pub.stats.rx_ctlpkts++;
		else
			devinfo->bus_pub.stats.rx_ctlerrs++;
	} else if (type == WLAND_USB_CBCTL_WRITE) {
		if (status == 0)
			devinfo->bus_pub.stats.tx_ctlpkts++;
		else
			devinfo->bus_pub.stats.tx_ctlerrs++;
	}

	devinfo->ctl_urb_status = status;
	devinfo->ctl_completed = true;

	wland_usb_ioctl_resp_wake(devinfo);
}

static void wland_usb_ctlread_complete(struct urb *urb)
{
	struct wland_usbdev_info *devinfo =
		(struct wland_usbdev_info *) urb->context;

	WLAND_DBG(USB, TRACE, "Enter\n");

	devinfo->ctl_urb_actual_length = urb->actual_length;

	wland_usb_ctl_complete(devinfo, WLAND_USB_CBCTL_READ, urb->status);
}

static void wland_usb_ctlwrite_complete(struct urb *urb)
{
	struct wland_usbdev_info *devinfo =
		(struct wland_usbdev_info *) urb->context;

	WLAND_DBG(USB, TRACE, "Enter\n");

	wland_usb_ctl_complete(devinfo, WLAND_USB_CBCTL_WRITE, urb->status);
}

static int wland_usb_send_ctl(struct wland_usbdev_info *devinfo, u8 * buf,
	int len)
{
	int ret;
	u16 size;

	WLAND_DBG(USB, TRACE, "Enter\n");

	if (devinfo == NULL || buf == NULL || len == 0
		|| devinfo->ctl_urb == NULL)
		return -EINVAL;

	size = len;
	devinfo->ctl_write.wLength = cpu_to_le16p(&size);
	devinfo->ctl_urb->transfer_buffer_length = size;
	devinfo->ctl_urb_status = 0;
	devinfo->ctl_urb_actual_length = 0;

	usb_fill_control_urb(devinfo->ctl_urb,
		devinfo->usbdev,
		devinfo->ctl_out_pipe,
		(u8 *) & devinfo->ctl_write,
		buf,
		size, (usb_complete_t) wland_usb_ctlwrite_complete, devinfo);

	ret = usb_submit_urb(devinfo->ctl_urb, GFP_ATOMIC);
	if (ret < 0)
		WLAND_ERR("usb_submit_urb failed %d\n", ret);

	return ret;
}

static int wland_usb_recv_ctl(struct wland_usbdev_info *devinfo, u8 * buf,
	int len)
{
	int ret;
	u16 size;

	WLAND_DBG(USB, TRACE, "Enter\n");

	if ((devinfo == NULL) || (buf == NULL) || (len == 0)
		|| (devinfo->ctl_urb == NULL))
		return -EINVAL;

	size = len;
	devinfo->ctl_read.wLength = cpu_to_le16p(&size);
	devinfo->ctl_urb->transfer_buffer_length = size;

	devinfo->ctl_read.bRequestType =
		USB_DIR_IN | USB_TYPE_CLASS | USB_RECIP_INTERFACE;
	devinfo->ctl_read.bRequest = 1;

	usb_fill_control_urb(devinfo->ctl_urb,
		devinfo->usbdev,
		devinfo->ctl_in_pipe,
		(u8 *) & devinfo->ctl_read,
		buf,
		size, (usb_complete_t) wland_usb_ctlread_complete, devinfo);

	ret = usb_submit_urb(devinfo->ctl_urb, GFP_ATOMIC);
	if (ret < 0)
		WLAND_ERR("usb_submit_urb failed %d\n", ret);

	return ret;
}

static int wland_usb_bus_tx_ctlpkt(struct device *dev, u8 * buf, u32 len)
{
	int err = 0, timeout = 0;
	struct wland_usbdev_info *devinfo = wland_usb_get_businfo(dev);

	WLAND_DBG(USB, TRACE, "Enter\n");

	if (devinfo->bus_pub.state != USB_STATE_UP)
		return -EIO;

	if (test_and_set_bit(0, &devinfo->ctl_op))
		return -EIO;

	devinfo->ctl_completed = false;

	err = wland_usb_send_ctl(devinfo, buf, len);
	if (err) {
		WLAND_ERR("fail %d bytes: %d\n", err, len);
		clear_bit(0, &devinfo->ctl_op);
		return err;
	}

	timeout = wland_usb_ioctl_resp_wait(devinfo);
	clear_bit(0, &devinfo->ctl_op);
	if (!timeout) {
		WLAND_ERR("Txctl wait timed out\n");
		err = -EIO;
	}
	return err;
}

static int wland_usb_bus_rx_ctlpkt(struct device *dev, u8 * buf, u32 len)
{
	int err = 0, timeout = 0;
	struct wland_usbdev_info *devinfo = wland_usb_get_businfo(dev);

	WLAND_DBG(USB, TRACE, "Enter\n");

	if (devinfo->bus_pub.state != USB_STATE_UP)
		return -EIO;

	if (test_and_set_bit(0, &devinfo->ctl_op))
		return -EIO;

	devinfo->ctl_completed = false;

	err = wland_usb_recv_ctl(devinfo, buf, len);
	if (err) {
		WLAND_ERR("fail %d bytes: %d\n", err, len);
		clear_bit(0, &devinfo->ctl_op);
		return err;
	}

	timeout = wland_usb_ioctl_resp_wait(devinfo);
	err = devinfo->ctl_urb_status;

	clear_bit(0, &devinfo->ctl_op);

	if (!timeout) {
		WLAND_ERR("rxctl wait timed out\n");
		err = -EIO;
	}

	if (!err)
		return devinfo->ctl_urb_actual_length;
	else
		return err;
}

static struct wland_usbreq *wland_usb_deq(struct wland_usbdev_info *devinfo,
	struct list_head *q, int *counter)
{
	unsigned long flags;
	struct wland_usbreq *req;

	spin_lock_irqsave(&devinfo->qlock, flags);
	if (list_empty(q)) {
		spin_unlock_irqrestore(&devinfo->qlock, flags);
		return NULL;
	}
	req = list_entry(q->next, struct wland_usbreq, list);
	list_del_init(q->next);
	if (counter)
		(*counter)--;
	spin_unlock_irqrestore(&devinfo->qlock, flags);

	return req;
}

static void wland_usb_enq(struct wland_usbdev_info *devinfo,
	struct list_head *q, struct wland_usbreq *req, int *counter)
{
	unsigned long flags;

	spin_lock_irqsave(&devinfo->qlock, flags);
	list_add_tail(&req->list, q);
	if (counter)
		(*counter)++;
	spin_unlock_irqrestore(&devinfo->qlock, flags);
}

static struct wland_usbreq *wland_usbdev_qinit(struct list_head *q, int qsize)
{
	int i;
	struct wland_usbreq *req, *reqs;

	reqs = kcalloc(qsize, sizeof(struct wland_usbreq), GFP_ATOMIC);

	if (reqs == NULL)
		return NULL;

	req = reqs;

	for (i = 0; i < qsize; i++) {
		req->urb = usb_alloc_urb(0, GFP_ATOMIC);
		if (!req->urb)
			goto fail;

		INIT_LIST_HEAD(&req->list);
		list_add_tail(&req->list, q);
		req++;
	}
	return reqs;
fail:
	WLAND_ERR("fail!\n");
	while (!list_empty(q)) {
		req = list_entry(q->next, struct wland_usbreq, list);

		if (req && req->urb)
			usb_free_urb(req->urb);
		list_del(q->next);
	}
	return NULL;
}

static void wland_usb_free_q(struct list_head *q, bool pending)
{
	struct wland_usbreq *req, *next;
	int i = 0;

	list_for_each_entry_safe(req, next, q, list) {
		if (!req->urb) {
			WLAND_ERR("bad req\n");
			break;
		}
		i++;
		if (pending) {
			usb_kill_urb(req->urb);
		} else {
			usb_free_urb(req->urb);
			list_del_init(&req->list);
		}
	}
}

static void wland_usb_del_fromq(struct wland_usbdev_info *devinfo,
	struct wland_usbreq *req)
{
	unsigned long flags;

	spin_lock_irqsave(&devinfo->qlock, flags);
	list_del_init(&req->list);
	spin_unlock_irqrestore(&devinfo->qlock, flags);
}

static void wland_usb_tx_complete(struct urb *urb)
{
	unsigned long flags;
	struct wland_usbreq *req = (struct wland_usbreq *) urb->context;
	struct wland_usbdev_info *devinfo = req->devinfo;

	WLAND_DBG(USB, TRACE, "Enter, urb->status=%d, skb=%p\n", urb->status,
		req->skb);

	wland_usb_del_fromq(devinfo, req);

	wland_txcomplete(devinfo->dev, req->skb, urb->status == 0);

	req->skb = NULL;

	wland_usb_enq(devinfo, &devinfo->tx_freeq, req, &devinfo->tx_freecount);

	spin_lock_irqsave(&devinfo->tx_flowblock_lock, flags);
	if (devinfo->tx_freecount > devinfo->tx_high_watermark
		&& devinfo->tx_flowblock) {
		wland_txflowcontrol(devinfo->dev, false);

		devinfo->tx_flowblock = false;
	}
	spin_unlock_irqrestore(&devinfo->tx_flowblock_lock, flags);
}

static void wland_usb_rx_complete(struct urb *urb)
{
	struct wland_usbreq *req = (struct wland_usbreq *) urb->context;
	struct wland_usbdev_info *devinfo = req->devinfo;
	struct sk_buff *skb;

	WLAND_DBG(USB, TRACE, "Enter, urb->status=%d\n", urb->status);

	wland_usb_del_fromq(devinfo, req);

	skb = req->skb;
	req->skb = NULL;

	/*
	 * zero lenght packets indicate usb "failure". Do not refill
	 */
	if (urb->status != 0 || !urb->actual_length) {
		wland_pkt_buf_free_skb(skb);
		wland_usb_enq(devinfo, &devinfo->rx_freeq, req, NULL);
		return;
	}

	if (devinfo->bus_pub.state == USB_STATE_UP) {
		skb_put(skb, urb->actual_length);
		wland_rx_frames(devinfo->dev, skb);
		wland_usb_rx_refill(devinfo, req);
	} else {
		wland_pkt_buf_free_skb(skb);
		wland_usb_enq(devinfo, &devinfo->rx_freeq, req, NULL);
	}
}

static void wland_usb_rx_refill(struct wland_usbdev_info *devinfo,
	struct wland_usbreq *req)
{
	struct sk_buff *skb;
	int ret;

	if (!req || !devinfo)
		return;

	skb = dev_alloc_skb(devinfo->bus_pub.bus_mtu);
	if (!skb) {
		wland_usb_enq(devinfo, &devinfo->rx_freeq, req, NULL);
		return;
	}

	req->skb = skb;

	usb_fill_bulk_urb(req->urb,
		devinfo->usbdev,
		devinfo->rx_pipe,
		skb->data, skb_tailroom(skb), wland_usb_rx_complete, req);

	req->devinfo = devinfo;

	wland_usb_enq(devinfo, &devinfo->rx_postq, req, NULL);

	ret = usb_submit_urb(req->urb, GFP_ATOMIC);

	if (ret) {
		wland_usb_del_fromq(devinfo, req);
		wland_pkt_buf_free_skb(req->skb);
		req->skb = NULL;
		wland_usb_enq(devinfo, &devinfo->rx_freeq, req, NULL);
	}
}

static void wland_usb_rx_fill_all(struct wland_usbdev_info *devinfo)
{
	struct wland_usbreq *req;

	if (devinfo->bus_pub.state != USB_STATE_UP) {
		WLAND_ERR("bus is not up=%d\n", devinfo->bus_pub.state);
		return;
	}

	while ((req = wland_usb_deq(devinfo, &devinfo->rx_freeq, NULL)) != NULL) {
		wland_usb_rx_refill(devinfo, req);
	}
}

static void wland_usb_state_change(struct wland_usbdev_info *devinfo, int state)
{
	struct wland_bus *bcmf_bus = devinfo->bus_pub.bus;
	int old_state;

	WLAND_DBG(USB, TRACE, "Enter, current state=%d, new state=%d\n",
		devinfo->bus_pub.state, state);

	if (devinfo->bus_pub.state == state)
		return;

	old_state = devinfo->bus_pub.state;
	devinfo->bus_pub.state = state;

	/*
	 * update state of upper layer
	 */
	if (state == USB_STATE_DOWN) {
		WLAND_DBG(USB, TRACE, "DBUS is down\n");
		bcmf_bus->state = WLAND_BUS_DOWN;
	} else if (state == USB_STATE_UP) {
		WLAND_DBG(USB, TRACE, "DBUS is up\n");
		bcmf_bus->state = WLAND_BUS_DATA;
	} else {
		WLAND_DBG(USB, TRACE, "DBUS current state=%d\n", state);
	}
}

static void wland_usb_intr_complete(struct urb *urb)
{
	struct wland_usbdev_info *devinfo =
		(struct wland_usbdev_info *) urb->context;
	int err;

	WLAND_DBG(USB, TRACE, "Enter, urb->status=%d\n", urb->status);

	if (devinfo == NULL)
		return;

	if (unlikely(urb->status)) {
		if (urb->status == -ENOENT ||
			urb->status == -ESHUTDOWN || urb->status == -ENODEV) {
			wland_usb_state_change(devinfo, USB_STATE_DOWN);
		}
	}

	if (devinfo->bus_pub.state == USB_STATE_DOWN) {
		WLAND_ERR("intr cb when DBUS down, ignoring\n");
		return;
	}

	if (devinfo->bus_pub.state == USB_STATE_UP) {
		err = usb_submit_urb(devinfo->intr_urb, GFP_ATOMIC);
		if (err)
			WLAND_ERR("usb_submit_urb, err=%d\n", err);
	}
}

static int wland_usb_bus_tx(struct device *dev, struct sk_buff *skb)
{
	struct wland_usbreq *req;
	int ret;
	unsigned long flags;
	struct wland_usbdev_info *devinfo = wland_usb_get_businfo(dev);

	WLAND_DBG(USB, TRACE, "Enter, skb=%p\n", skb);

	if (devinfo->bus_pub.state != USB_STATE_UP) {
		ret = -EIO;
		goto fail;
	}

	req = wland_usb_deq(devinfo, &devinfo->tx_freeq,
		&devinfo->tx_freecount);
	if (!req) {
		WLAND_ERR("no req to send\n");
		ret = -ENOMEM;
		goto fail;
	}

	req->skb = skb;
	req->devinfo = devinfo;

	usb_fill_bulk_urb(req->urb, devinfo->usbdev, devinfo->tx_pipe,
		skb->data, skb->len, wland_usb_tx_complete, req);

	req->urb->transfer_flags |= URB_ZERO_PACKET;

	wland_usb_enq(devinfo, &devinfo->tx_postq, req, NULL);

	ret = usb_submit_urb(req->urb, GFP_ATOMIC);
	if (ret) {
		WLAND_ERR("wland_usb_bus_tx usb_submit_urb FAILED\n");
		wland_usb_del_fromq(devinfo, req);
		req->skb = NULL;
		wland_usb_enq(devinfo, &devinfo->tx_freeq, req,
			&devinfo->tx_freecount);
		goto fail;
	}

	spin_lock_irqsave(&devinfo->tx_flowblock_lock, flags);
	if (devinfo->tx_freecount < devinfo->tx_low_watermark
		&& !devinfo->tx_flowblock) {
		wland_txflowcontrol(dev, true);
		devinfo->tx_flowblock = true;
	}
	spin_unlock_irqrestore(&devinfo->tx_flowblock_lock, flags);
	return 0;

fail:
	return ret;
}

static int wland_usb_bus_up(struct device *dev)
{
	u16 ifnum;
	int ret;
	struct wland_usbdev_info *devinfo = wland_usb_get_businfo(dev);

	WLAND_DBG(USB, TRACE, "Enter\n");

	if (devinfo->bus_pub.state == USB_STATE_UP)
		return 0;

	/*
	 * Success, indicate devinfo is fully up
	 */
	wland_usb_state_change(devinfo, USB_STATE_UP);

	if (devinfo->intr_urb) {
		usb_fill_int_urb(devinfo->intr_urb,
			devinfo->usbdev,
			devinfo->intr_pipe,
			&devinfo->intr,
			devinfo->intr_size,
			(usb_complete_t) wland_usb_intr_complete,
			devinfo, devinfo->interval);

		ret = usb_submit_urb(devinfo->intr_urb, GFP_ATOMIC);
		if (ret) {
			WLAND_ERR("USB_SUBMIT_URB failed with status %d\n",
				ret);
			return -EINVAL;
		}
	}

	if (devinfo->ctl_urb) {
		devinfo->ctl_in_pipe = usb_rcvctrlpipe(devinfo->usbdev, 0);
		devinfo->ctl_out_pipe = usb_sndctrlpipe(devinfo->usbdev, 0);

		ifnum = IFDESC(devinfo->usbdev, CONTROL_IF).bInterfaceNumber;

		/*
		 * CTL Write
		 */
		devinfo->ctl_write.bRequestType =
			USB_DIR_OUT | USB_TYPE_CLASS | USB_RECIP_INTERFACE;
		devinfo->ctl_write.bRequest = 0;
		devinfo->ctl_write.wValue = cpu_to_le16(0);
		devinfo->ctl_write.wIndex = cpu_to_le16p(&ifnum);

		/*
		 * CTL Read
		 */
		devinfo->ctl_read.bRequestType =
			USB_DIR_IN | USB_TYPE_CLASS | USB_RECIP_INTERFACE;
		devinfo->ctl_read.bRequest = 1;
		devinfo->ctl_read.wValue = cpu_to_le16(0);
		devinfo->ctl_read.wIndex = cpu_to_le16p(&ifnum);
	}
	wland_usb_rx_fill_all(devinfo);
	return 0;
}

static void wland_usb_bus_down(struct device *dev)
{
	struct wland_usbdev_info *devinfo = wland_usb_get_businfo(dev);

	WLAND_DBG(USB, TRACE, "Enter\n");

	if (devinfo == NULL)
		return;

	if (devinfo->bus_pub.state == USB_STATE_DOWN)
		return;

	wland_usb_state_change(devinfo, USB_STATE_DOWN);

	if (devinfo->intr_urb)
		usb_kill_urb(devinfo->intr_urb);

	if (devinfo->ctl_urb)
		usb_kill_urb(devinfo->ctl_urb);

	if (devinfo->bulk_urb)
		usb_kill_urb(devinfo->bulk_urb);

	wland_usb_free_q(&devinfo->tx_postq, true);
	wland_usb_free_q(&devinfo->rx_postq, true);
}

static void wland_usb_sync_complete(struct urb *urb)
{
	struct wland_usbdev_info *devinfo =
		(struct wland_usbdev_info *) urb->context;

	devinfo->ctl_completed = true;

	wland_usb_ioctl_resp_wake(devinfo);
}

static bool wland_usb_dl_cmd(struct wland_usbdev_info *devinfo, u8 cmd,
	void *buffer, int buflen)
{
	int ret = 0;
	char *tmpbuf;
	u16 size;

	if ((!devinfo) || (devinfo->ctl_urb == NULL))
		return false;

	tmpbuf = kmalloc(buflen, GFP_ATOMIC);

	if (!tmpbuf)
		return false;

	size = buflen;
	devinfo->ctl_urb->transfer_buffer_length = size;

	devinfo->ctl_read.wLength = cpu_to_le16p(&size);
	devinfo->ctl_read.bRequestType =
		USB_DIR_IN | USB_TYPE_VENDOR | USB_RECIP_INTERFACE;
	devinfo->ctl_read.bRequest = cmd;

	usb_fill_control_urb(devinfo->ctl_urb,
		devinfo->usbdev,
		usb_rcvctrlpipe(devinfo->usbdev, 0),
		(u8 *) & devinfo->ctl_read,
		tmpbuf,
		size, (usb_complete_t) wland_usb_sync_complete, devinfo);

	devinfo->ctl_completed = false;

	ret = usb_submit_urb(devinfo->ctl_urb, GFP_ATOMIC);
	if (ret < 0) {
		WLAND_ERR("usb_submit_urb failed %d\n", ret);
		kfree(tmpbuf);
		return false;
	}

	ret = wland_usb_ioctl_resp_wait(devinfo);
	memcpy(buffer, tmpbuf, buflen);
	kfree(tmpbuf);
	return ret;
}

static bool wland_usb_dlneeded(struct wland_usbdev_info *devinfo)
{
	struct bootrom_id_le id;
	u32 chipid;

	WLAND_DBG(USB, TRACE, "Enter\n");

	if (devinfo == NULL)
		return false;

	/*
	 * Check if firmware downloaded already by querying runtime ID
	 */
	id.chip = cpu_to_le32(0xDEAD);

	wland_usb_dl_cmd(devinfo, DL_GETVER, &id, sizeof(id));

	chipid = le32_to_cpu(id.chip);

	WLAND_DBG(USB, TRACE, "chip %d.\n", chipid);

	if (chipid == WLAND_POSTBOOT_ID) {
		WLAND_DBG(USB, TRACE, "firmware already downloaded\n");
		wland_usb_dl_cmd(devinfo, DL_RESETCFG, &id, sizeof(id));
		return false;
	} else {
		devinfo->bus_pub.devid = chipid;
	}
	return true;
}

static int wland_usb_resetcfg(struct wland_usbdev_info *devinfo)
{
	struct bootrom_id_le id;
	u32 loop_cnt = 0;

	WLAND_DBG(USB, TRACE, "Enter\n");

	do {
		mdelay(USB_RESET_GETVER_SPINWAIT);
		loop_cnt++;
		id.chip = cpu_to_le32(0xDEAD);	/* Get the ID */
		wland_usb_dl_cmd(devinfo, DL_GETVER, &id, sizeof(id));
		if (id.chip == cpu_to_le32(WLAND_POSTBOOT_ID))
			break;
	} while (loop_cnt < USB_RESET_GETVER_LOOP_CNT);

	if (id.chip == cpu_to_le32(WLAND_POSTBOOT_ID)) {
		WLAND_DBG(USB, TRACE, "postboot chip 0x%x.\n",
			le32_to_cpu(id.chip));

		wland_usb_dl_cmd(devinfo, DL_RESETCFG, &id, sizeof(id));
		return 0;
	} else {
		WLAND_ERR("Cannot talk to chip. Firmware is not UP, %d ms\n",
			USB_RESET_GETVER_SPINWAIT * loop_cnt);
		return -EINVAL;
	}
}

static int wland_usb_dl_send_bulk(struct wland_usbdev_info *devinfo,
	void *buffer, int len)
{
	int ret;

	if ((devinfo == NULL) || (devinfo->bulk_urb == NULL))
		return -EINVAL;

	/*
	 * Prepare the URB
	 */
	usb_fill_bulk_urb(devinfo->bulk_urb,
		devinfo->usbdev,
		devinfo->tx_pipe,
		buffer, len, (usb_complete_t) wland_usb_sync_complete, devinfo);

	devinfo->bulk_urb->transfer_flags |= URB_ZERO_PACKET;

	devinfo->ctl_completed = false;

	ret = usb_submit_urb(devinfo->bulk_urb, GFP_ATOMIC);
	if (ret) {
		WLAND_ERR("usb_submit_urb failed %d\n", ret);
		return ret;
	}
	ret = wland_usb_ioctl_resp_wait(devinfo);
	return (ret == 0);
}

static int wland_usb_fw_download(struct wland_usbdev_info *devinfo)
{
	int devid;
	struct rdl_state_le state;

	WLAND_DBG(USB, TRACE, "Enter\n");

	if (devinfo == NULL) {
		WLAND_ERR("No Device Info!\n");
		return -ENODEV;
	}

	devid = devinfo->bus_pub.devid;

	if (devid == USB_DEVICE_ID_RDA599X) {
		WLAND_ERR("Unmatch Device Id!\n");
		return -EINVAL;
	}

	devinfo->bus_pub.state = USB_STATE_DL_DONE;

	/*
	 * Check we are runnable
	 */
	wland_usb_dl_cmd(devinfo, DL_GETSTATE, &state,
		sizeof(struct rdl_state_le));

	/*
	 * Start the image
	 */
	if (state.state == cpu_to_le32(DL_RUNNABLE)) {
		if (!wland_usb_dl_cmd(devinfo, DL_GO, &state,
				sizeof(struct rdl_state_le)))
			return -ENODEV;
		if (wland_usb_resetcfg(devinfo))
			return -ENODEV;
	} else {
		WLAND_ERR("Dongle not runnable\n");
		return -EINVAL;
	}

	WLAND_DBG(USB, TRACE, "Exit.\n");

	return 0;
}

static void wland_usb_detach(struct wland_usbdev_info *devinfo)
{
	WLAND_DBG(USB, TRACE, "Enter, devinfo %p\n", devinfo);

	/*
	 * free the URBS
	 */
	wland_usb_free_q(&devinfo->rx_freeq, false);
	wland_usb_free_q(&devinfo->tx_freeq, false);

	usb_free_urb(devinfo->intr_urb);
	usb_free_urb(devinfo->ctl_urb);
	usb_free_urb(devinfo->bulk_urb);

	kfree(devinfo->tx_reqs);
	kfree(devinfo->rx_reqs);
}

static struct wland_usb_dev *wland_usb_attach(struct wland_usbdev_info *devinfo,
	int nrxq, int ntxq)
{
	WLAND_DBG(USB, TRACE, "Enter\n");

	devinfo->bus_pub.nrxq = nrxq;
	devinfo->rx_low_watermark = nrxq / 2;
	devinfo->bus_pub.devinfo = devinfo;
	devinfo->bus_pub.ntxq = ntxq;
	devinfo->bus_pub.state = USB_STATE_DOWN;

	/*
	 * flow control when too many tx urbs posted
	 */
	devinfo->tx_low_watermark = ntxq / 4;
	devinfo->tx_high_watermark = devinfo->tx_low_watermark * 3;
	devinfo->bus_pub.bus_mtu = USB_MAX_PKT_SIZE;

	/*
	 * Initialize other structure content
	 */
	init_waitqueue_head(&devinfo->ioctl_resp_wait);

	/*
	 * Initialize the spinlocks
	 */
	spin_lock_init(&devinfo->qlock);
	spin_lock_init(&devinfo->tx_flowblock_lock);

	INIT_LIST_HEAD(&devinfo->rx_freeq);
	INIT_LIST_HEAD(&devinfo->rx_postq);

	INIT_LIST_HEAD(&devinfo->tx_freeq);
	INIT_LIST_HEAD(&devinfo->tx_postq);

	devinfo->tx_flowblock = false;

	devinfo->rx_reqs = wland_usbdev_qinit(&devinfo->rx_freeq, nrxq);
	if (!devinfo->rx_reqs)
		goto error;

	devinfo->tx_reqs = wland_usbdev_qinit(&devinfo->tx_freeq, ntxq);
	if (!devinfo->tx_reqs)
		goto error;

	devinfo->tx_freecount = ntxq;

	devinfo->intr_urb = usb_alloc_urb(0, GFP_ATOMIC);
	if (!devinfo->intr_urb) {
		WLAND_ERR("usb_alloc_urb (intr) failed\n");
		goto error;
	}

	devinfo->ctl_urb = usb_alloc_urb(0, GFP_ATOMIC);
	if (!devinfo->ctl_urb) {
		WLAND_ERR("usb_alloc_urb (ctl) failed\n");
		goto error;
	}

	devinfo->bulk_urb = usb_alloc_urb(0, GFP_ATOMIC);
	if (!devinfo->bulk_urb) {
		WLAND_ERR("usb_alloc_urb (bulk) failed\n");
		goto error;
	}

	if (!wland_usb_dlneeded(devinfo))
		return &devinfo->bus_pub;

	WLAND_DBG(USB, TRACE, "Start fw downloading\n");

	if (wland_usb_fw_download(devinfo))
		goto error;

	WLAND_DBG(USB, TRACE, "Exit.\n");

	return &devinfo->bus_pub;

error:
	WLAND_ERR("failed!\n");
	wland_usb_detach(devinfo);
	return NULL;
}

static struct wland_bus_ops wland_usb_bus_ops = {
	.txdata = wland_usb_bus_tx,
	.init = wland_usb_bus_up,
	.stop = wland_usb_bus_down,
	.txctl = wland_usb_bus_tx_ctlpkt,
	.rxctl = wland_usb_bus_rx_ctlpkt,
};

static int wland_usb_probe(struct usb_interface *intf,
	const struct usb_device_id *id)
{
#if 1
	int ep, ret = 0, num_of_eps;
	u8 endpoint_num;
	struct usb_endpoint_descriptor *endpoint;
	struct usb_device *usb = interface_to_usbdev(intf);
	struct wland_usbdev_info *devinfo;
	struct wland_bus *bus = NULL;
	struct wland_usb_dev *bus_pub = NULL;
	struct device *dev = NULL;

	WLAND_DBG(USB, TRACE, "Enter\n");

	devinfo = kzalloc(sizeof(*devinfo), GFP_ATOMIC);
	if (devinfo == NULL)
		return -ENOMEM;

	devinfo->usbdev = usb;
	devinfo->dev = &usb->dev;

	usb_set_intfdata(intf, devinfo);

	/*
	 * Check that the device supports only one configuration
	 */
	if (usb->descriptor.bNumConfigurations != 1) {
		ret = -1;
		goto fail;
	}

	if (usb->descriptor.bDeviceClass != USB_CLASS_VENDOR_SPEC) {
		ret = -1;
		goto fail;
	}

	/*
	 * Only the BDC interface configuration is supported:
	 *      Device class: USB_CLASS_VENDOR_SPEC
	 *      if0 class:    USB_CLASS_VENDOR_SPEC
	 *      if0/ep0: control
	 *      if0/ep1: bulk in
	 *      if0/ep2: bulk out (ok if swapped with bulk in)
	 */
	if (CONFIGDESC(usb)->bNumInterfaces != 1) {
		ret = -1;
		goto fail;
	}

	/*
	 * Check interface
	 */
	if (IFDESC(usb, CONTROL_IF).bInterfaceClass != USB_CLASS_VENDOR_SPEC ||
		IFDESC(usb, CONTROL_IF).bInterfaceSubClass != 2 ||
		IFDESC(usb, CONTROL_IF).bInterfaceProtocol != 0xff) {
		WLAND_ERR
			("invalid control interface: class %d, subclass %d, proto %d\n",
			IFDESC(usb, CONTROL_IF).bInterfaceClass, IFDESC(usb,
				CONTROL_IF).bInterfaceSubClass, IFDESC(usb,
				CONTROL_IF).bInterfaceProtocol);
		ret = -1;
		goto fail;
	}

	/*
	 * Check control endpoint
	 */
	endpoint = &IFEPDESC(usb, CONTROL_IF, 0);

	if ((endpoint->bmAttributes & USB_ENDPOINT_XFERTYPE_MASK) !=
		USB_ENDPOINT_XFER_INT) {
		WLAND_ERR("invalid control endpoint %d\n",
			endpoint->bmAttributes & USB_ENDPOINT_XFERTYPE_MASK);
		ret = -1;
		goto fail;
	}

	endpoint_num = endpoint->bEndpointAddress & USB_ENDPOINT_NUMBER_MASK;
	devinfo->intr_pipe = usb_rcvintpipe(usb, endpoint_num);
	devinfo->rx_pipe = 0;
	devinfo->rx_pipe2 = 0;
	devinfo->tx_pipe = 0;
	num_of_eps = IFDESC(usb, BULK_IF).bNumEndpoints - 1;

	/*
	 * Check data endpoints and get pipes
	 */
	for (ep = 1; ep <= num_of_eps; ep++) {
		endpoint = &IFEPDESC(usb, BULK_IF, ep);

		if ((endpoint->bmAttributes & USB_ENDPOINT_XFERTYPE_MASK) !=
			USB_ENDPOINT_XFER_BULK) {
			WLAND_ERR("invalid data endpoint %d\n", ep);
			ret = -1;
			goto fail;
		}

		endpoint_num =
			endpoint->bEndpointAddress & USB_ENDPOINT_NUMBER_MASK;

		if ((endpoint->bEndpointAddress & USB_ENDPOINT_DIR_MASK) ==
			USB_DIR_IN) {
			if (!devinfo->rx_pipe) {
				devinfo->rx_pipe =
					usb_rcvbulkpipe(usb, endpoint_num);
			} else {
				devinfo->rx_pipe2 =
					usb_rcvbulkpipe(usb, endpoint_num);
			}
		} else {
			devinfo->tx_pipe = usb_sndbulkpipe(usb, endpoint_num);
		}
	}

	/*
	 * Allocate interrupt URB and data buffer
	 */
	/*
	 * RNDIS says 8-byte intr, our old drivers used 4-byte
	 */
	if (IFEPDESC(usb, CONTROL_IF, 0).wMaxPacketSize == cpu_to_le16(16))
		devinfo->intr_size = 8;
	else
		devinfo->intr_size = 4;

	devinfo->interval = IFEPDESC(usb, CONTROL_IF, 0).bInterval;

	if (usb->speed == USB_SPEED_HIGH)
		WLAND_DBG(USB, TRACE,
			"Rdamicro high speed USB wireless device detected\n");
	else
		WLAND_DBG(USB, TRACE,
			"Rdamicro full speed USB wireless device detected\n");

	dev = devinfo->dev;

	bus_pub = wland_usb_attach(devinfo, WLAND_USB_NRXQ, WLAND_USB_NTXQ);
	if (!bus_pub)
		goto fail;

	bus = kzalloc(sizeof(struct wland_bus), GFP_ATOMIC);
	if (!bus)
		goto fail2;

	bus->dev = dev;
	bus_pub->bus = bus;
	bus->bus_priv.usb = bus_pub;

	dev_set_drvdata(dev, bus);

	bus->ops = &wland_usb_bus_ops;
	bus->chip = bus_pub->devid;

	/*
	 * Attach to the common driver interface
	 */
	ret = wland_bus_attach(0, dev);
	if (ret < 0) {
		WLAND_ERR("bus_attach failed\n");
		goto fail1;
	}

	ret = wland_bus_start(dev);
	if (ret < 0) {
		WLAND_ERR("chip is not responding\n");
		goto fail1;
	}
	/*
	 * Success
	 */
	return 0;
fail1:
	kfree(bus);
fail2:
	wland_usb_detach(devinfo);
fail:
	WLAND_ERR("failed with errno %d\n", ret);
	kfree(devinfo);
	usb_set_intfdata(intf, NULL);
	return ret;
#else
	struct usb_host_interface *iface_desc;

	wlan_private *priv = NULL;
	wlan_event *event = NULL;
	int i;
	wlan_usb_card *cardP = NULL;
	int ret = 0;
	u8 mac_addr[ETH_ALEN];

	WLAND_DBG(KERN_INFO, "USB Probe\n");

	cardP = kzalloc(sizeof(wlan_usb_card), GFP_KERNEL);
	if (!cardP) {
		WLAND_ERR("Allocating USB card failed.\n");
		return -ENOMEM;
	}

	usbdev = interface_to_usbdev(intf);

	cardP->usbdev = usbdev;
	cardP->dev = &usbdev->dev;

	usb_set_intfdata(intf, cardP);

	/*
	 * Check that the device supports only one configuration
	 */
	if (usbdev->descriptor.bNumConfigurations != 1) {
		WLAND_ERR("USB No.Configuration(%d) != 1\n",
			usbdev->descriptor.bNumConfigurations);
		ret = -1;
		goto fail;
	}

	iface_desc = intf->cur_altsetting;

	for (i = 0; i < iface_desc->desc.bNumEndpoints; ++i) {
		struct usb_endpoint_descriptor *endpoint;
		u8 endpoint_num;

		endpoint = &iface_desc->endpoint[i].desc;
		endpoint_num =
			endpoint->bEndpointAddress & USB_ENDPOINT_NUMBER_MASK;

		if ((endpoint->bEndpointAddress & USB_ENDPOINT_DIR_MASK) ==
			USB_DIR_IN) {
			cardP->rx_pipe = usb_rcvbulkpipe(usbdev, endpoint_num);
		} else {
			cardP->tx_pipe = usb_sndbulkpipe(usbdev, endpoint_num);;
		}
	}

	if (usbdev->speed == USB_SPEED_HIGH)
		WLAND_DBG(KERN_INFO, "RDA high speed USB wireless device\n");
	else
		WLAND_DBG(KERN_INFO, "RDA full speed USB wireless device\n");

	cardP->rx_urb = usb_alloc_urb(0, GFP_KERNEL);
	if (!cardP->rx_urb) {
		WLAND_ERR("Alloc Rx URB failed\n");
		goto dealloc;
	}

	cardP->tx_urb = usb_alloc_urb(0, GFP_KERNEL);
	if (!cardP->tx_urb) {
		WLAND_ERR("Alloc Tx URB failed\n");
		goto dealloc;
	}

	priv = cardP->priv;

	/*
	 * Attach and link in the cfg80211
	 */
	if (unlikely(wl_cfg80211_attach(priv))) {
		WLAND_ERR("Attach CFG80211 failed\n");
		return 0;
	} else {
		WLAND_DBG(KERN_INFO, "Attach CFG80211 success\n");
	}

	mac_addr[0] = 0x00;
	mac_addr[1] = 0xc0;
	mac_addr[2] = 0x52;

	mac_addr[3] = 0x5E;
	mac_addr[4] = 0x59;
	mac_addr[5] = 0x95;

	memcpy(priv->netDev->dev_addr, mac_addr, ETH_ALEN);

	return 0;
#endif
}

static void wland_usb_disconnect(struct usb_interface *intf)
{
	struct wland_usbdev_info *devinfo =
		(struct wland_usbdev_info *) usb_get_intfdata(intf);

	WLAND_DBG(USB, TRACE, "Enter\n");

	if (!devinfo)
		return;

	wland_bus_detach(devinfo->dev);
	kfree(devinfo->bus_pub.bus);
	wland_usb_detach(devinfo);
	kfree(devinfo);

	WLAND_DBG(USB, TRACE, "Exit\n");
}

/* only need to signal the bus being down and update the state. */
static int wland_usb_suspend(struct usb_interface *intf, pm_message_t state)
{
	struct usb_device *usb = interface_to_usbdev(intf);
	struct wland_usbdev_info *devinfo = wland_usb_get_businfo(&usb->dev);

	WLAND_DBG(USB, TRACE, "Enter\n");

	devinfo->bus_pub.state = USB_STATE_SLEEP;

	wland_bus_detach(&usb->dev);

	return 0;
}

/* (re-) start the bus. */
static int wland_usb_resume(struct usb_interface *intf)
{
	struct usb_device *usb = interface_to_usbdev(intf);
	struct wland_usbdev_info *devinfo = wland_usb_get_businfo(&usb->dev);

	WLAND_DBG(USB, TRACE, "Enter\n");

	if (!wland_bus_attach(0, devinfo->dev)) {
		return wland_bus_start(&usb->dev);
	}

	return 0;
}

static int wland_usb_reset_resume(struct usb_interface *intf)
{
	struct usb_device *usb = interface_to_usbdev(intf);
	struct wland_usbdev_info *devinfo = wland_usb_get_businfo(&usb->dev);

	WLAND_DBG(USB, TRACE, "Enter\n");

	if (!wland_usb_fw_download(devinfo))
		return wland_usb_resume(intf);

	return -EIO;
}

MODULE_DEVICE_TABLE(usb, wland_usb_devid_table);

static struct usb_driver wland_usbdrvr = {
	.name = KBUILD_MODNAME,
	.probe = wland_usb_probe,
	.disconnect = wland_usb_disconnect,
	.id_table = wland_usb_devid_table,
	.suspend = wland_usb_suspend,
	.resume = wland_usb_resume,
	.reset_resume = wland_usb_reset_resume,
	.supports_autosuspend = 1,
	.disable_hub_initiated_lpm = 1,
};

static int wland_usb_reset_device(struct device *dev, void *notused)
{
	/*
	 * device past is the usb interface so we need to use parent here.
	 */
	struct wland_bus *bus_if = dev_get_drvdata(dev->parent);
	struct wland_private *drvr = bus_if->drvr;
	u8 val = 1;

	if (drvr == NULL)
		return;

	if (drvr->iflist[0])
		wland_fil_iovar_data_set(drvr->iflist[0], "TERMINATED", &val,
			sizeof(u8));

	return 0;
}

void wland_usb_exit(void)
{
	struct device_driver *drv = &wland_usbdrvr.drvwrap.driver;

	WLAND_DBG(USB, TRACE, "Enter\n");

	driver_for_each_device(drv, NULL, NULL, wland_usb_reset_device);

	usb_deregister(&wland_usbdrvr);

	WLAND_DBG(USB, TRACE, "Exit\n");
}

void wland_usb_register(void)
{
	WLAND_DBG(USB, TRACE, "Enter\n");

	if (usb_register(&wland_usbdrvr) < 0) {
		wland_registration_sem_up(false);
	}

	WLAND_DBG(USB, TRACE, "Exit\n")
}

#ifdef  0			/* OLD WEXT INTERFACE */

//a callback for rx_urb
static void wlan_usb_receive(struct urb *urb)
{
	wlan_usb_card *cardp = (wlan_usb_card *) urb->context;
	struct sk_buff *skb = cardp->rx_skb;
	wlan_private *priv = (wlan_private *) cardp->priv;
	int recvlength = urb->actual_length;
	wlan_rx_packet_node *rx_node = NULL;

	WLAND_DBG(USB, TRACE, "Enter\n");

	if (recvlength) {
		if (urb->status || recvlength < PACKET_HEADER_LEN) {
			WLAND_ERR("RX URB failed1: %d\n", urb->status);
			kfree_skb(skb);
			skb = NULL;
			goto next_recv;
		}

		skb_put(skb, recvlength);
		rx_node = kzalloc(sizeof(wlan_rx_packet_node), GFP_ATOMIC);
		if (!rx_node) {
			kfree_skb(skb);
			skb = NULL;
			goto next_recv;
		}

		WLAND_DBG(USB, TRACE, "Recv length = 0x%d\n", recvlength);
		rx_node->Skb = skb;

		spin_lock(&priv->RxLock);
		list_add_tail(&rx_node->List, &priv->RxQueue);
		spin_unlock(&priv->RxLock);

		complete(&priv->RxThread.comp);
	} else if (urb->status) {
		kfree_skb(skb);
		skb = NULL;
		WLAND_ERR("RX URB failed2:%d\n", urb->status);
	}

next_recv:
	if (!priv->CardRemoved)
		wlan_usb_submit_rx_urb(cardp);
	else if (skb)
		kfree_skb(skb);

	WLAND_DBG(USB, TRACE, "Done\n");
}

int wlan_usb_submit_rx_urb(wlan_usb_card * cardp)
{
	struct sk_buff *skb;
	int ret = -1;

	if (!(skb = dev_alloc_skb(WLAND_MAX_BUFSZ + NET_IP_ALIGN +
				PACKET_HEADER_LEN))) {
		WLAND_ERR("No free skb \n");
		goto rx_ret;
	}

	skb_reserve(skb, NET_IP_ALIGN);
	cardp->rx_skb = skb;

	/*
	 * Fill the receive configuration URB and initialise the Rx call back
	 */
	usb_fill_bulk_urb(cardp->rx_urb,
		cardp->usbdev,
		cardp->rx_pipe,
		skb->data, WLAND_MAX_BUFSZ, wlan_usb_receive, cardp);

	cardp->rx_urb->transfer_flags |= URB_ZERO_PACKET;

	if ((ret = usb_submit_urb(cardp->rx_urb, GFP_ATOMIC))) {
		WLAND_ERR("Submit Rx URB failed: %d\n", ret);
		kfree_skb(skb);
		cardp->rx_skb = NULL;
		ret = -1;
	} else {
		WLAND_DBG(USB, TRACE, "Submit Rx URB success\n");
		ret = 0;
	}

rx_ret:
	return ret;
}

static void wlan_usb_write_bulk_callback(struct urb *urb)
{
	wlan_usb_card *cardp = (wlan_usb_card *) urb->context;
	wlan_private *priv = (wlan_private *) cardp->priv;

	/*
	 * handle the transmission complete validations
	 */
	if (urb->status == 0) {
		WLAND_DBG(USB, TRACE, "URB status is successful\n",
			urb->actual_length);
	} else {
		/*
		 * print the failure status number for debug
		 */
		WLAND_ERR("URB in failure status: %d\n", urb->status);
	}
	if (priv) {
		priv->UsbTxStatus = WLAN_USB_TX_SEND_COMPLETE;
		wake_up_interruptible(&priv->UsbSendDone);
	}
}

int usb_send_packet(wlan_usb_card * cardp, u8 * payload, u16 nb)
{
	int ret;
	wlan_private *priv = (wlan_private *) cardp->priv;

	WLAND_DBG(USB, TRACE, "Enter\n");

	/*
	 * check if device is removed
	 */
	if (priv->CardRemoved) {
		WLAND_ERR("Device removed\n");
		ret = -ENODEV;
		goto tx_ret;
	}

	usb_fill_bulk_urb(cardp->tx_urb,
		cardp->usbdev,
		cardp->tx_pipe,
		payload, nb, wlan_usb_write_bulk_callback, cardp);

	cardp->tx_urb->transfer_flags |= URB_ZERO_PACKET;

	if ((ret = usb_submit_urb(cardp->tx_urb, GFP_ATOMIC))) {
		WLAND_ERR("usb_submit_urb failed: %d\n", ret);
		priv->UsbTxStatus = WLAN_USB_TX_IDLE;
	} else {
		WLAND_DBG(USB, TRACE, "usb_submit_urb success\n");
		ret = 0;
		priv->UsbTxStatus = WLAN_USB_TX_SENDING;
		wait_event_interruptible(priv->UsbSendDone,
			(priv->UsbTxStatus == WLAN_USB_TX_SEND_COMPLETE));
	}

tx_ret:
	WLAND_DBG(USB, TRACE, "Done\n");
	return ret;
}

#endif /* OLD WEXT INTERFACE */

#endif /* WLAND_USB_SUPPORT */
