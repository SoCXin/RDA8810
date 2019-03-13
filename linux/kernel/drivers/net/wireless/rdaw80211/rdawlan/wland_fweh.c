
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
#include <linuxver.h>
#include <linux_osl.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/if_ether.h>
#include <linux/spinlock.h>
#include <linux/skbuff.h>
#include <linux/if_arp.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/wireless.h>
#include <linux/ieee80211.h>
#include <linux/debugfs.h>
#include <net/cfg80211.h>
#include <net/rtnetlink.h>

#include <wland_defs.h>
#include <wland_utils.h>
#include <wland_fweh.h>
#include <wland_dev.h>
#include <wland_dbg.h>
#include <wland_wid.h>
#include <wland_bus.h>
#include <wland_trap.h>
#include <wland_p2p.h>
#include <wland_cfg80211.h>

#ifdef DEBUG
#define WLAND_ENUM_DEF(id, val)  	{ val, #id },

struct wland_fweh_event_name {
	enum wland_fweh_event_code code;
	const char *name;
};

/* array for mapping code to event name */
static struct wland_fweh_event_name fweh_event_names[] = {
	FIRMW_EVENT_ENUM_DEFLIST
};

#undef WLAND_ENUM_DEF
#endif /* DEBUG */

static const char *fweh_event_name(enum wland_fweh_event_code code)
{
#ifdef DEBUG
	int i;

	for (i = 0; i < ARRAY_SIZE(fweh_event_names); i++) {
		if (fweh_event_names[i].code == code)
			return fweh_event_names[i].name;
	}
	return "unknown";
#else
	return "nodebug";
#endif
}

#if 0

/* This function extracts the 'from ds' bit from the MAC header of the input */

/* frame.                                                                    */

/* Returns the value in the LSB of the returned value.                       */
static inline u8 get_from_ds(u8 * header)
{
	return ((header[1] & 0x02) >> 1);
}

/* This function extracts the 'to ds' bit from the MAC header of the input   */

/* frame.                                                                    */

/* Returns the value in the LSB of the returned value.                       */
static inline u8 get_to_ds(u8 * header)
{
	return (header[1] & 0x01);
}

/* This function extracts the BSSID from the incoming WLAN packet based on   */

/* the 'from ds' bit, and updates the MAC Address in the allocated 'addr'    */

/* variable.                                                                 */
static void get_BSSID(u8 * data, u8 * bssid)
{
	if (get_from_ds(data) == 1)
		memcpy(bssid, data + 10, 6);
	else if (get_to_ds(data) == 1)
		memcpy(bssid, data + 4, 6);
	else
		memcpy(bssid, data + 16, 6);
}
#endif

/*
 * fweh_dequeue_event() - get event from the queue.
 *
 * @fweh: firmware event handling info.
 */
static struct wland_fweh_queue_item *fweh_dequeue_event(struct wland_fw_info
	*fweh)
{
	struct wland_fweh_queue_item *event = NULL;
	ulong flags;

	spin_lock_irqsave(&fweh->evt_q_lock, flags);
	if (!list_empty(&fweh->event_q)) {
		event = list_first_entry(&fweh->event_q,
			struct wland_fweh_queue_item, q);
		list_del(&event->q);
	}
	spin_unlock_irqrestore(&fweh->evt_q_lock, flags);

	return event;
}

/*
 * fweh_event_worker() - firmware event worker.
 *
 * @work: worker object.
 */
static void fweh_event_worker(struct work_struct *work)
{
	struct wland_fw_info *fweh =
		container_of(work, struct wland_fw_info, event_work);
	struct wland_private *drvr =
		container_of(fweh, struct wland_private, fweh);
	struct wland_fweh_queue_item *event;
	struct wland_event_msg *emsg_be;
	struct wland_if *ifp = NULL;
	int err = 0;

	while ((event = fweh_dequeue_event(fweh))) {
		WLAND_DBG(EVENT, TRACE,
			"event:%s(%u), status:%u, reason:%u, ifidx:%u, bsscfg:%u, addr:%pM\n",
			fweh_event_name(event->code), event->code,
			event->emsg.status, event->emsg.reason,
			event->emsg.ifidx, event->emsg.bsscfgidx,
			event->emsg.addr);

		/*
		 * convert event message
		 */
		emsg_be = &event->emsg;

		/*
		 * special handling of interface event
		 */
		if (emsg_be->ifidx >= WLAND_MAX_IFS) {
			WLAND_ERR("invalid interface index: %u\n",
				emsg_be->ifidx);

			goto event_free;
		}

		if (event->code == WLAND_E_IF_ADD) {
			WLAND_DBG(EVENT, DEBUG, "adding %s (%pM)\n",
				emsg_be->ifname, emsg_be->addr);

			ifp = wland_add_if(drvr, emsg_be->bsscfgidx,
				emsg_be->ifidx, emsg_be->ifname, emsg_be->addr);
			if (IS_ERR(ifp))
				goto event_free;
			wland_fws_add_interface(ifp);

			if (netdev_attach(ifp) < 0)
				goto event_free;
		} else if (event->code == WLAND_E_IF_CHANGE) {
			WLAND_DBG(EVENT, DEBUG, "enter: idx=%d\n", ifp->bssidx);

			wland_fws_macdesc_init(ifp->fws_desc, ifp->mac_addr,
				ifp->ifidx);
		} else if (event->code == WLAND_E_IF_DEL) {
			wland_fws_del_interface(ifp);
			wland_del_if(drvr, emsg_be->bsscfgidx);
		}

		/*
		 * handle the event if valid interface and handler
		 */
		if (fweh->evt_handler[emsg_be->event_code])
			err = fweh->evt_handler[emsg_be->event_code] (drvr->
				iflist[emsg_be->bsscfgidx], emsg_be,
				event->data);
		else
			WLAND_ERR("unhandled event %d ignored\n",
				emsg_be->event_code);

		if (err < 0) {
			WLAND_ERR("event handler failed (%d)\n", event->code);
			err = 0;
		}
event_free:
		kfree(event);
	}
}

#if 0

/*                         imode                          */

/* BIT0: 1 -> Security ON              0 -> OFF           */

/* BIT1: 1 -> WEP40  cypher supported  0 -> Not supported */

/* BIT2: 1 -> WEP104 cypher supported  0 -> Not supported */

/* BIT3: 1 -> WPA mode      supported  0 -> Not supported */

/* BIT4: 1 -> WPA2 (RSN)    supported  0 -> Not supported */

/* BIT5: 1 -> AES-CCMP cphr supported  0 -> Not supported */

/* BIT6: 1 -> TKIP   cypher supported  0 -> Not supported */

/* BIT7: 1 -> TSN           supported  0 -> Not supported */

/*                        authtype                        */

/* BIT0: 1 -> OPEN SYSTEM                                 */

/* BIT1: 1 -> SHARED KEY                                  */

/* BIT3: 1 -> WAPI                                        */

static int assoc_helper_secinfo(struct wlan_private *priv,
	struct bss_descriptor *assoc_bss)
{
	int ret = 0;

	WLAND_DBG(DEFAULT, TRACE, "%s <<<\n", __func__);

	/*
	 * set imode and key
	 */
	if (!priv->secinfo.wep_enabled
		&& !priv->secinfo.WPAenabled && !priv->secinfo.WPA2enabled) {
		WLAND_DBG(DEFAULT, TRACE, "%s, NO SEC\n", __func__);
		priv->imode = 0;
	} else {
		u16 key_len = 0;

		if (priv->secinfo.wep_enabled
			&& !priv->secinfo.WPAenabled
			&& !priv->secinfo.WPA2enabled) {
			/*
			 * WEP
			 */
			key_len = priv->wep_keys[0].len;
			WLAND_DBG(DEFAULT, TRACE, "%s, WEP, len = %d\n",
				__func__, key_len * 8);
			if (key_len == KEY_LEN_WEP_40) {
				priv->imode = BIT0 | BIT1;
			} else if (key_len == KEY_LEN_WEP_104) {
				priv->imode = BIT0 | BIT2;
			} else {
				WLAND_ERR("Invalide WEP Key length %d\n",
					key_len);
				ret = -EINVAL;
				goto out;
			}
		} else if (!priv->secinfo.wep_enabled
			&& (priv->secinfo.WPAenabled
				|| priv->secinfo.WPA2enabled)) {
			/*
			 * WPA
			 */
			struct enc_key *pkey = NULL;

			WLAND_DBG(DEFAULT, TRACE,
				"%s, WPA cp:%x wpa:%d wpa2:%d \n", __func__,
				priv->secinfo.cipther_type,
				priv->secinfo.WPAenabled,
				priv->secinfo.WPA2enabled);

			if (priv->wpa_mcast_key.len
				&& (priv->wpa_mcast_key.flags &
					KEY_INFO_WPA_ENABLED))
				pkey = &priv->wpa_mcast_key;
			else if (priv->wpa_unicast_key.len
				&& (priv->wpa_unicast_key.flags &
					KEY_INFO_WPA_ENABLED))
				pkey = &priv->wpa_unicast_key;

			priv->imode = 0;
			/*
			 * turn on security
			 */
			priv->imode |= (BIT0);
			priv->imode &= ~(BIT3 | BIT4);

			if (priv->secinfo.WPA2enabled)
				priv->imode |= (BIT4);
			else if (priv->secinfo.WPAenabled)
				priv->imode |= (BIT3);
			/*
			 * we don't know the cipher type by now
			 * use dot11i_info to decide and use CCMP if possible
			 */
			priv->imode &= ~(BIT5 | BIT6);

			if (priv->secinfo.cipther_type & IW_AUTH_CIPHER_CCMP)
				priv->imode |= BIT5;
			else if (priv->secinfo.
				cipther_type & IW_AUTH_CIPHER_TKIP)
				priv->imode |= BIT6;
		} else {
			WLAND_ERR("WEP and WPA/WPA2 enabled simutanously\n");
			ret = -EINVAL;
			goto out;
		}
	}

	/*
	 * set authtype
	 */
	if (priv->secinfo.auth_mode & IW_AUTH_ALG_OPEN_SYSTEM
		|| priv->secinfo.auth_mode & IW_AUTH_ALG_SHARED_KEY) {
		if (priv->secinfo.auth_mode & IW_AUTH_ALG_OPEN_SYSTEM) {
			WLAND_DBG(DEFAULT, TRACE,
				"Open Auth, KEY_MGMT = %d, AUTH_ALG mode:%x\n",
				priv->secinfo.key_mgmt,
				priv->secinfo.auth_mode);

			if (priv->secinfo.key_mgmt == 0x01)
				priv->authtype = BIT2;
			else
				priv->authtype = BIT0;
		} else if (priv->secinfo.auth_mode & IW_AUTH_ALG_SHARED_KEY) {
			WLAND_DBG(DEFAULT, TRACE,
				"Shared-Key Auth AUTH_ALG mode:%x \n",
				priv->secinfo.auth_mode);
			priv->authtype = BIT1;
		}

		if (priv->secinfo.key_mgmt == WAPI_KEY_MGMT_PSK
			|| priv->secinfo.key_mgmt == WAPI_KEY_MGMT_CERT)
			priv->authtype = BIT3;
	} else if (priv->secinfo.auth_mode == IW_AUTH_ALG_WAPI) {
		WLAND_DBG(DEFAULT, TRACE, "Wapi.\n");

		priv->authtype = IW_AUTH_ALG_WAPI;
	} else if (priv->secinfo.auth_mode == IW_AUTH_ALG_LEAP) {
		WLAND_DBG(DEFAULT, TRACE, "LEAP Auth, Not supported.\n");
		ret = -EINVAL;
		goto out;
	} else {
		WLAND_ERR("Unknown Auth\n");
		ret = -EINVAL;
		goto out;
	}
out:
	return ret;
}
#endif

void wland_timer_handler(ulong fcontext)
{
#if 0
	struct wland_drv_timer *timer = (struct wland_drv_timer *) fcontext;

	if (timer->func) {
		timer->func(timer->data);
	} else {
		wlan_event *event = wlan_alloc_event(timer->event);

		if (event) {
			event->Para = timer->data;
			wlan_push_event((wlan_private *) timer->data, event,
				true);
		}
	}

	if (timer->timer_is_periodic == true) {
		mod_timer(&timer->tl,
			jiffies + msecs_to_jiffies(timer->time_period));
	} else {
		timer->timer_is_canceled = true;
	}
#endif
}

/* The format of the message is:                                         */

/* +-------------------------------------------------------------------+ */

/* | pkt Type  | Message Type |  Message body according type           | */

/* +-------------------------------------------------------------------+ */

/* |  1 Byte   |   1 Byte     |                                        | */

/* +-------------------------------------------------------------------+ */

void wland_netif_rx(struct wland_if *ifp, struct sk_buff *skb)
{
	skb->dev = ifp->ndev;
	skb->protocol = eth_type_trans(skb, skb->dev);
	skb->ip_summed = CHECKSUM_NONE;

	if (skb->pkt_type == PACKET_MULTICAST)
		ifp->stats.multicast++;

	/*
	 * free skb
	 */
	if (!(ifp->ndev->flags & IFF_UP)) {
		WLAND_ERR("netdev not up\n");
		wland_pkt_buf_free_skb(skb);
		return;
	}

	ifp->ndev->last_rx = jiffies;
	ifp->stats.rx_bytes += skb->len;
	ifp->stats.rx_packets++;

	WLAND_DBG(EVENT, TRACE, "rx proto:0x%X,pkt_len:%d\n",
		ntohs(skb->protocol), skb->len);

	if (in_interrupt()) {
		netif_rx(skb);
	} else {
		/*
		 * If the receive is not processed inside an ISR, the softirqd must be woken explicitly to service the NET_RX_SOFTIRQ.
		 * * In 2.6 kernels, this is handledby netif_rx_ni(), but in earlier kernels, we need to do it manually.
		 */
		netif_rx_ni(skb);
	}
}

/*
 * firmweh_push_event() - generate self event code.
 *
 * @drvr    : driver information object.
 * @code    : event code.
 * @data    : event data.
 */
void firmweh_push_event(struct wland_private *drvr,
	struct wland_event_msg *event_packet, void *data)
{
	struct wland_fw_info *fweh = &drvr->fweh;
	struct wland_fweh_queue_item *event;
	gfp_t alloc_flag = GFP_KERNEL;
	ulong flags;

	if (event_packet->event_code >= WLAND_E_LAST) {
		WLAND_ERR("invalid event code %d\n", event_packet->event_code);
		return;
	}

	if (!fweh->evt_handler[event_packet->event_code]) {
		WLAND_ERR("event code %d unregistered\n",
			event_packet->event_code);
		return;
	}

	WLAND_DBG(EVENT, TRACE, "push event for %s.\n",
		fweh_event_name(event_packet->event_code));

	if (in_interrupt())
		alloc_flag = GFP_ATOMIC;

	event = kzalloc(sizeof(*event) + event_packet->datalen, alloc_flag);
	if (!event) {
		WLAND_ERR("No memory\n");
		return;
	}

	event->code = event_packet->event_code;
	event->ifidx = event_packet->ifidx;

	/*
	 * use memcpy to get aligned event message
	 */
	memcpy(&event->emsg, event_packet, sizeof(event->emsg));
	memcpy(event->data, data, event_packet->datalen);
	memcpy(event->ifaddr, event_packet->addr, ETH_ALEN);

	/*
	 * create and queue event.
	 */
	spin_lock_irqsave(&fweh->evt_q_lock, flags);
	list_add_tail(&event->q, &fweh->event_q);
	spin_unlock_irqrestore(&fweh->evt_q_lock, flags);

	/*
	 * schedule work
	 */
	schedule_work(&fweh->event_work);
}

/*
 * firmweh_register() - register handler for given event code.
 *
 * @drvr    : driver information object.
 * @code    : event code.
 * @handler : handler for the given event code.
 */
int firmweh_register(struct wland_private *drvr,
	enum wland_fweh_event_code code, fw_handler_t handler)
{
	if (drvr->fweh.evt_handler[code]) {
		WLAND_ERR("event code %d already registered\n", code);
		return -ENOSPC;
	}
	drvr->fweh.evt_handler[code] = handler;

	WLAND_DBG(EVENT, TRACE, "event handler registered for %s\n",
		fweh_event_name(code));
	return 0;
}

/*
 * firmweh_unregister() - remove handler for given code.
 *
 * @drvr: driver information object.
 * @code: event code.
 */
void firmweh_unregister(struct wland_private *drvr,
	enum wland_fweh_event_code code)
{
	WLAND_DBG(EVENT, TRACE, "event handler cleared for %s\n",
		fweh_event_name(code));
	if (drvr->fweh.evt_handler[code])
		drvr->fweh.evt_handler[code] = NULL;
}

/*
 * wland_fweh_attach() - initialize firmware event handling.
 *
 * @drvr: driver information object.
 */
void wland_fweh_attach(struct wland_private *drvr)
{
	struct wland_fw_info *fweh = &drvr->fweh;

	INIT_WORK(&fweh->event_work, fweh_event_worker);
	spin_lock_init(&fweh->evt_q_lock);
	INIT_LIST_HEAD(&fweh->event_q);
}

/*
 * wland_fweh_detach() - cleanup firmware event handling.
 *
 * @drvr: driver information object.
 */
void wland_fweh_detach(struct wland_private *drvr)
{
	struct wland_fw_info *fweh = &drvr->fweh;

	/*
	 * cancel the worker
	 */
	cancel_work_sync(&fweh->event_work);
	WARN_ON(!list_empty(&fweh->event_q));
	memset(fweh->evt_handler, 0, sizeof(fweh->evt_handler));
}
