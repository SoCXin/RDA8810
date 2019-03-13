
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
#ifndef _WLAND_USB_H_
#define _WLAND_USB_H_

#ifdef WLAND_USB_SUPPORT

#define PACKET_HEADER_LEN           (2)

/* define the usd op mode read or write */
#define WLAND_USB_CBCTL_WRITE	    0
#define WLAND_USB_CBCTL_READ	    1

#define USB_RESET_GETVER_SPINWAIT	100	/* in unit of ms */
#define USB_RESET_GETVER_LOOP_CNT	10

#define WLAND_POSTBOOT_ID		    0xA123	/* ID to detect if dongle has boot up */
#define WLAND_USB_NRXQ	            50
#define WLAND_USB_NTXQ	            50

#define CONFIGDESC(usb)             (&((usb)->actconfig)->desc)
#define IFPTR(usb, idx)             ((usb)->actconfig->interface[(idx)])
#define IFALTS(usb, idx)            (IFPTR((usb), (idx))->altsetting[0])
#define IFDESC(usb, idx)             IFALTS((usb), (idx)).desc
#define IFEPDESC(usb, idx, ep)      (IFALTS((usb), (idx)).endpoint[(ep)]).desc

#define CONTROL_IF                  0
#define BULK_IF                     0

#define USB_MAX_PKT_SIZE	        1600

/* Control messages: bRequest values */
#define DL_GETSTATE	                0	/* returns the rdl_state_t struct */
#define DL_CHECK_CRC	            1	/* currently unused */
#define DL_GO		                2	/* execute downloaded image */
#define DL_START	                3	/* initialize dl state */
#define DL_REBOOT	                4	/* reboot the device in 2 seconds */
#define DL_GETVER	                5	/* returns the bootrom_id_t struct */
#define DL_GO_PROTECTED	            6	/* execute the downloaded code and set reset
					 * event to occur in 2 seconds.  It is the
					 * responsibility of the downloaded code to clear this event
					 */
#define DL_EXEC		                7	/* jump to a supplied address */
#define DL_RESETCFG	                8	/* To support single enum on dongle - Not used by bootloader */
#define DL_DEFER_RESP_OK            9	/* Potentially defer the response to setup if resp unavailable */

/* states */
#define DL_WAITING	                0	/* waiting to rx first pkt */
#define DL_READY	                1	/* hdr was good, waiting for more of the compressed image */
#define DL_BAD_HDR	                2	/* hdr was corrupted */
#define DL_BAD_CRC	                3	/* compressed image was corrupted */
#define DL_RUNNABLE	                4	/* download was successful,waiting for go cmd */
#define DL_START_FAIL	            5	/* failed to initialize correctly */

struct rdl_state_le {
	__le32 state;
	__le32 bytes;
};

struct bootrom_id_le {
	__le32 chip;		/* Chip id */
	__le32 ramsize;		/* Size of  RAM */
	__le32 remapbase;	/* Current remap base address */
	__le32 boardtype;	/* Type of board */
	__le32 boardrev;	/* Board revision */
};

enum wlan_usb_tx_status {
	WLAN_USB_TX_IDLE = 0,
	WLAN_USB_TX_SENDING,
	WLAN_USB_TX_SEND_COMPLETE
};

struct wlan_usb_card {
	struct usb_device *usbdev;
	struct device *dev;
	struct urb *rx_urb, *tx_urb;
	struct sk_buff *rx_skb;
	uint tx_pipe;
	uint rx_pipe;
	wlan_private *priv;
};

struct intr_transfer_buf {
	u32 notification;
	u32 reserved;
};

struct wland_usbdev_info {
	struct wland_usb_dev bus_pub;	/* MUST BE FIRST */
	spinlock_t qlock;
	struct list_head rx_freeq;
	struct list_head rx_postq;
	struct list_head tx_freeq;
	struct list_head tx_postq;

	uint rx_pipe, tx_pipe, intr_pipe, rx_pipe2;

	int rx_low_watermark;
	int tx_low_watermark;
	int tx_high_watermark;
	int tx_freecount;
	bool tx_flowblock;
	spinlock_t tx_flowblock_lock;

	struct wland_usbreq *tx_reqs;
	struct wland_usbreq *rx_reqs;

	struct usb_device *usbdev;
	struct device *dev;

	int ctl_in_pipe, ctl_out_pipe;

	struct urb *ctl_urb;	/* URB for control endpoint */
	struct usb_ctrlrequest ctl_write;
	struct usb_ctrlrequest ctl_read;

	u32 ctl_urb_actual_length;
	int ctl_urb_status;
	int ctl_completed;

	wait_queue_head_t ioctl_resp_wait;

	ulong ctl_op;

	struct urb *bulk_urb;	/* used for FW download       */
	struct urb *intr_urb;	/* URB for interrupt endpoint */
	int intr_size;		/* Size of interrupt message  */
	int interval;		/* Interrupt polling interval */
	struct intr_transfer_buf intr;	/* Data buffer for interrupt endpoint */
};

enum wland_usb_state {
	USB_STATE_DOWN,
	USB_STATE_DL_FAIL,
	USB_STATE_DL_DONE,
	USB_STATE_UP,
	USB_STATE_SLEEP
};

struct wland_stats {
	u32 tx_ctlpkts;
	u32 tx_ctlerrs;
	u32 rx_ctlpkts;
	u32 rx_ctlerrs;
};

struct wland_usb_dev {
	struct wland_bus *bus;
	struct wland_usbdev_info *devinfo;
	enum wland_usb_state state;
	struct wland_stats stats;
	int ntxq, nrxq, rxsize;
	u32 bus_mtu;
	int devid;
};

/* IO Request Block (IRB) */
struct wland_usbreq {
	struct list_head list;
	struct wland_usbdev_info *devinfo;
	struct urb *urb;
	struct sk_buff *skb;
};

#ifdef WLAND_USB_SUPPORT
extern void wland_usb_exit(void);
extern void wland_usb_register(void);
#endif /*WLAND_USB_SUPPORT */

#endif /* WLAND_USB_SUPPORT */
#endif /* _WLAND_USB_H_       */
