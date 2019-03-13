
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
#ifndef _WLAND_FWEH_H_
#define _WLAND_FWEH_H_

struct wland_if;
struct wland_private;

/* Firmware Header - Used on data packets to convey priority. */
#define	FMW_HEADER_LEN		                4
#define FMW_TYPE_MASK	                    0xF0	/* Firmware Host Type Mask */
#define FMWID_HEADER_LEN                    7

#define BDC_FLAG_VER_SHIFT	                4	/* Protocol version shift */
#define BDC_FLAG2_IF_SHIFT	                0

#define BDC_GET_IF_IDX(hdr)                 ((int)((((hdr)->flags2) & FMW_TYPE_MASK) >> BDC_FLAG2_IF_SHIFT))

#define WLAND_ENUM_DEF(id, val)  	        WLAND_E_##id = (val),

/* list of firmware events */
#define FIRMW_EVENT_ENUM_DEFLIST \
    WLAND_ENUM_DEF(ESCAN_RESULT,            0)\
    WLAND_ENUM_DEF(CONNECT_IND,             1)\
    WLAND_ENUM_DEF(DISCONNECT_IND,          2)\
    WLAND_ENUM_DEF(ROAM,                    3)\
    WLAND_ENUM_DEF(PFN_NET_FOUND,           4)\
    WLAND_ENUM_DEF(P2P_DISC_LISTEN_COMPLETE,5)\
    WLAND_ENUM_DEF(ACTION_FRAME_COMPLETE,   6)\
    WLAND_ENUM_DEF(ACTION_FRAME_OFF_CHAN_COMPLETE, 7)\
    WLAND_ENUM_DEF(P2P_PROBEREQ_MSG,        8)\
    WLAND_ENUM_DEF(FIFO_CREDIT_MAP,         9)\
    WLAND_ENUM_DEF(ACTION_FRAME_RX,         10)\
    WLAND_ENUM_DEF(IF_ADD,                  11)\
    WLAND_ENUM_DEF(IF_DEL,                  12)\
    WLAND_ENUM_DEF(IF_CHANGE,               13)

/* firmware event codes sent by the dongle */
enum wland_fweh_event_code {
#if 1
	WLAND_E_ESCAN_RESULT,
	WLAND_E_CONNECT_IND,
	WLAND_E_DISCONNECT_IND,
	WLAND_E_ROAM,
	WLAND_E_PFN_NET_FOUND,
	WLAND_E_P2P_DISC_LISTEN_COMPLETE,
	WLAND_E_ACT_FRAME_COMPLETE,
	WLAND_E_ACT_FRAME_OFF_CHAN_COMPLETE,
	WLAND_E_P2P_PROBEREQ_MSG,
	WLAND_E_FIFO_CREDIT_MAP,
	WLAND_E_ACTION_FRAME_RX,
	WLAND_E_IF_ADD,
	WLAND_E_IF_DEL,
	WLAND_E_IF_CHANGE,
#else
	FIRMW_EVENT_ENUM_DEFLIST
#endif
	WLAND_E_LAST
};

#undef WLAND_ENUM_DEF

/*
 * struct wland_event_msg - firmware event message.
 *
 * @version:    version information.
 * @flags:      event flags.
 * @event_code: firmware event code.
 * @status:     status information.
 * @reason:     reason code.
 * @auth_type:  authentication type.
 * @datalen:    lenght of event data buffer.
 * @addr:       ether address.
 * @ifname:     interface name.
 * @ifidx:      interface index.
 * @bsscfgidx:  bsscfg index.
 */
struct wland_event_msg {
	u32 event_code;		/* firmware response eventcode or rspwid */
	u32 status;
	u32 reason;
	s32 auth_type;
	u32 datalen;		/* firmware request or response datalen  */
	u8 addr[ETH_ALEN];
	char ifname[IFNAMSIZ];
	u8 action;
	u8 role;
	u8 ifidx;
	u8 bsscfgidx;
};

/*
 * struct wland_fweh_queue_item - event item on event queue.
 *
 * @q       : list element for queuing.
 * @code    : event code.
 * @ifidx   : interface index related to this event.
 * @ifaddr  : ethernet address for interface.
 * @emsg    : common parameters of the firmware event message.
 * @data    : event specific data part of the firmware event.
 */
struct wland_fweh_queue_item {
	struct list_head q;
	enum wland_fweh_event_code code;
	u8 ifidx;
	u8 ifaddr[ETH_ALEN];
	struct wland_event_msg emsg;
	u8 data[0];
};

/* firmare event handle cb */
typedef s32(*fw_handler_t) (struct wland_if * ifp,
	const struct wland_event_msg * evtmsg, void *data);

/*
 * struct wland_fw_info - firmware event handling information.
 *
 * @event_work:  event worker.
 * @evt_q_lock:  lock for event queue protection.
 * @event_q:     event queue.
 * @evt_handler: registered event handlers.
 */
struct wland_fw_info {
	struct work_struct event_work;
	spinlock_t evt_q_lock;
	struct list_head event_q;
	fw_handler_t evt_handler[WLAND_E_LAST];
};

/* handle firmware rx skb */
extern void wland_netif_rx(struct wland_if *ifp, struct sk_buff *skb);

/* push event to queue */
extern void firmweh_push_event(struct wland_private *drvr,
	struct wland_event_msg *event_packet, void *data);

/* register/unregister for firmware event handler */
extern int firmweh_register(struct wland_private *drvr,
	enum wland_fweh_event_code code, fw_handler_t handler);
extern void firmweh_unregister(struct wland_private *drvr,
	enum wland_fweh_event_code code);

/* attach firmware event handler moodle */
extern void wland_fweh_attach(struct wland_private *drvr);
extern void wland_fweh_detach(struct wland_private *drvr);

#endif /* _WLAND_FWEH_H_ */
