
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
#include <linux/rtnetlink.h>
#include <net/cfg80211.h>
#include <net/rtnetlink.h>
#include <net/netlink.h>
#include <net/ieee80211_radiotap.h>

#include <wland_defs.h>
#include <wland_utils.h>
#include <wland_fweh.h>
#include <wland_dev.h>
#include <wland_dbg.h>
#include <wland_wid.h>
#include <wland_bus.h>
#include <wland_sdmmc.h>
#include <wland_p2p.h>
#include <wland_cfg80211.h>

#ifdef WLAND_MONITOR_SUPPORT

enum monitor_states {
	MONITOR_STATE_DEINIT = 0x0,
	MONITOR_STATE_INIT = 0x1,
	MONITOR_STATE_INTERFACE_ADDED = 0x2,
	MONITOR_STATE_INTERFACE_DELETED = 0x4
};

typedef struct monitor_interface {
	int radiotap_enabled;
	struct net_device *real_ndev;	/* The real interface that the monitor is on */
	struct net_device *mon_ndev;
} monitor_interface;

struct wland_linux_monitor {
	void *pub;
	enum monitor_states monitor_state;
	struct monitor_interface mon_if[WLAND_MAX_IFS];
	struct mutex lock;	/* lock to protect mon_if */
};

static struct wland_linux_monitor g_monitor;

extern int netdev_start_xmit(struct sk_buff *skb, struct net_device *net);

/* Look up dhd's net device table to find a match (e.g. interface "eth0" is a match for "mon.eth0"
 * "p2p-eth0-0" is a match for "mon.p2p-eth0-0")
 */
static struct net_device *lookup_real_netdev(char *name)
{
	struct net_device *ndev_found = NULL;
	struct net_device *ndev;
	int i, len = 0, last_name_len = 0;

	/*
	 * We need to find interface "p2p-p2p-0" corresponding to monitor interface "mon-p2p-0",
	 * * Once mon iface name reaches IFNAMSIZ, it is reset to p2p0-0 and corresponding mon
	 * * iface would be mon-p2p0-0.
	 */
	for (i = 0; i < WLAND_MAX_IFS; i++) {
		ndev = dhd_idx2net(g_monitor.pub, i);

		/*
		 * Skip "p2p" and look for "-p2p0-x" in monitor interface name. If it
		 * * it matches, then this netdev is the corresponding real_netdev.
		 */
		if (ndev && strstr(ndev->name, "p2p-p2p0")) {
			len = strlen("p2p");
		} else {
			/*
			 * if p2p- is not present, then the IFNAMSIZ have reached and name
			 * * would have got reset. In this casse,look for p2p0-x in mon-p2p0-x
			 */
			len = 0;
		}
		if (ndev && strstr(name, (ndev->name + len))) {
			if (strlen(ndev->name) > last_name_len) {
				ndev_found = ndev;
				last_name_len = strlen(ndev->name);
			}
		}
	}

	return ndev_found;
}

static monitor_interface *ndev_to_monif(struct net_device *ndev)
{
	int i;

	for (i = 0; i < WLAND_MAX_IFS; i++) {
		if (g_monitor.mon_if[i].mon_ndev == ndev)
			return &g_monitor.mon_if[i];
	}

	return NULL;
}

static int mon_if_open(struct net_device *ndev)
{
	WLAND_DBG(DEFAULT, TRACE, "Enter\n");
	return 0;
}

static int mon_if_stop(struct net_device *ndev)
{
	WLAND_DBG(DEFAULT, TRACE, "Enter\n");
	return 0;
}

static int mon_if_subif_start_xmit(struct sk_buff *skb, struct net_device *ndev)
{
	int rtap_len, qos_len = 0, dot11_hdr_len = 24, snap_len = 6;
	u8 *pdata;
	u16 frame_ctl;
	u8 src_mac_addr[6];
	u8 dst_mac_addr[6];
	struct ieee80211_hdr *dot11_hdr;
	struct ieee80211_radiotap_header *rtap_hdr;
	struct monitor_interface *mon_if;

	WLAND_DBG(DEFAULT, TRACE, "Enter\n");

	mon_if = ndev_to_monif(ndev);

	if (mon_if == NULL || mon_if->real_ndev == NULL) {
		WLAND_ERR("cannot find matched net dev, skip the packet\n");
		goto fail;
	}

	if (unlikely(skb->len < sizeof(struct ieee80211_radiotap_header)))
		goto fail;

	rtap_hdr = (struct ieee80211_radiotap_header *) skb->data;
	if (unlikely(rtap_hdr->it_version))
		goto fail;

	rtap_len = ieee80211_get_radiotap_len(skb->data);
	if (unlikely(skb->len < rtap_len))
		goto fail;

	WLAND_DBG(DEFAULT, TRACE, "radiotap len (should be 14): %d\n",
		rtap_len);

	/*
	 * Skip the ratio tap header
	 */
	PKTPULL(NULL, skb, rtap_len);

	dot11_hdr = (struct ieee80211_hdr *) skb->data;
	frame_ctl = le16_to_cpu(dot11_hdr->frame_control);

	/*
	 * Check if the QoS bit is set
	 */
	if ((frame_ctl & IEEE80211_FCTL_FTYPE) == IEEE80211_FTYPE_DATA) {
		/*
		 * Check if this ia a Wireless Distribution System (WDS) frame which has 4 MAC addresses
		 */
		if (dot11_hdr->frame_control & 0x0080)
			qos_len = 2;
		if ((dot11_hdr->frame_control & 0x0300) == 0x0300)
			dot11_hdr_len += 6;

		memcpy(dst_mac_addr, dot11_hdr->addr1, sizeof(dst_mac_addr));
		memcpy(src_mac_addr, dot11_hdr->addr2, sizeof(src_mac_addr));

		/*
		 * Skip the 802.11 header, QoS (if any) and SNAP, but leave spaces for for two MAC addresses
		 */
		PKTPULL(NULL, skb,
			dot11_hdr_len + qos_len + snap_len -
			sizeof(src_mac_addr) * 2);
		pdata = (unsigned char *) skb->data;
		memcpy(pdata, dst_mac_addr, sizeof(dst_mac_addr));
		memcpy(pdata + sizeof(dst_mac_addr), src_mac_addr,
			sizeof(src_mac_addr));
		PKTSETPRIO(skb, 0);

		WLAND_DBG(DEFAULT, TRACE, "if name: %s, matched if name: %s.\n",
			ndev->name, mon_if->real_ndev->name);

		/*
		 * Use the real net device to transmit the packet
		 */
		return netdev_start_xmit(skb, mon_if->real_ndev);
	}
fail:
	dev_kfree_skb(skb);
	return 0;
}

static void mon_if_set_multicast_list(struct net_device *ndev)
{
	monitor_interface *mon_if = ndev_to_monif(ndev);

	if (mon_if == NULL || mon_if->real_ndev == NULL) {
		WLAND_ERR("cannot find matched net dev, skip the packet\n");
	} else {
		WLAND_DBG(DEFAULT, TRACE,
			"Enter, if name: %s, matched if name %s\n", ndev->name,
			mon_if->real_ndev->name);
	}
}

static int mon_if_change_mac(struct net_device *ndev, void *addr)
{
	monitor_interface *mon_if = ndev_to_monif(ndev);

	if (mon_if == NULL || mon_if->real_ndev == NULL) {
		WLAND_ERR("cannot find matched net dev, skip the packet\n");
	} else {
		WLAND_DBG(DEFAULT, TRACE,
			"Enter, if name: %s, matched if name %s\n", ndev->name,
			mon_if->real_ndev->name);
	}
	return 0;
}

static const struct net_device_ops mon_if_ops = {
	.ndo_open = mon_if_open,
	.ndo_stop = mon_if_stop,
	.ndo_start_xmit = mon_if_subif_start_xmit,
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 2, 0))
	.ndo_set_rx_mode = mon_if_set_multicast_list,
#else /*LINUX_VERSION_CODE >= KERNEL_VERSION(3, 2, 0) */
	.ndo_set_multicast_list = mon_if_set_multicast_list,
#endif /*LINUX_VERSION_CODE >= KERNEL_VERSION(3, 2, 0) */
	.ndo_set_mac_address = mon_if_change_mac,
};

/* Global function definitions  */
int wland_add_monitor(char *name, struct net_device **new_ndev)
{
	int i, idx = -1, ret = 0;
	struct net_device *ndev = NULL;
	struct wland_linux_monitor **mon_ptr;

	WLAND_DBG(DEFAULT, TRACE, "Enter, if name: %s\n", name);

	mutex_lock(&g_monitor.lock);
	if (!name || !new_ndev) {
		WLAND_ERR("invalid parameters\n");
		ret = -EINVAL;
		goto out;
	}

	/*
	 * Find a vacancy
	 */
	for (i = 0; i < WLAND_MAX_IFS; i++) {
		if (g_monitor.mon_if[i].mon_ndev == NULL) {
			idx = i;
			break;
		}
	}

	if (idx == -1) {
		WLAND_ERR("exceeds maximum interfaces\n");
		ret = -EFAULT;
		goto out;
	}

	ndev = alloc_etherdev(sizeof(struct wland_linux_monitor *));
	if (!ndev) {
		WLAND_ERR("failed to allocate memory\n");
		ret = -ENOMEM;
		goto out;
	}

	ndev->type = ARPHRD_IEEE80211_RADIOTAP;

	strncpy(ndev->name, name, IFNAMSIZ);
	ndev->name[IFNAMSIZ - 1] = 0;
	ndev->netdev_ops = &mon_if_ops;

	ret = register_netdevice(ndev);
	if (ret) {
		WLAND_ERR("register_netdevice failed (%d)\n", ret);
		goto out;
	}

	*new_ndev = ndev;

	g_monitor.mon_if[idx].radiotap_enabled = true;
	g_monitor.mon_if[idx].mon_ndev = ndev;
	g_monitor.mon_if[idx].real_ndev = lookup_real_netdev(name);
	mon_ptr = (struct wland_linux_monitor **) netdev_priv(ndev);
	*mon_ptr = &g_monitor;
	g_monitor.monitor_state = MONITOR_STATE_INTERFACE_ADDED;

	WLAND_DBG(DEFAULT, TRACE,
		"net device returned: 0x%p,found a matched net device, name %s\n",
		ndev, g_monitor.mon_if[idx].real_ndev->name);

out:
	if (ret && ndev)
		free_netdev(ndev);

	mutex_unlock(&g_monitor.lock);

	return ret;
}

int wland_del_monitor(struct net_device *ndev)
{
	int i;

	if (!ndev)
		return -EINVAL;

	mutex_lock(&g_monitor.lock);
	for (i = 0; i < WLAND_MAX_IFS; i++) {
		if (g_monitor.mon_if[i].mon_ndev == ndev ||
			g_monitor.mon_if[i].real_ndev == ndev) {
			g_monitor.mon_if[i].real_ndev = NULL;
			unregister_netdev(g_monitor.mon_if[i].mon_ndev);
			free_netdev(g_monitor.mon_if[i].mon_ndev);
			g_monitor.mon_if[i].mon_ndev = NULL;
			g_monitor.monitor_state =
				MONITOR_STATE_INTERFACE_DELETED;
			break;
		}
	}

	if (g_monitor.monitor_state != MONITOR_STATE_INTERFACE_DELETED)
		WLAND_ERR
			("interface not found in monitor IF array, is this a monitor IF? 0x%p\n",
			ndev);
	mutex_unlock(&g_monitor.lock);

	return 0;
}

int wland_monitor_init(void *pub)
{
	WLAND_DBG(DEFAULT, TRACE, "Enter,(monitor_state:%d)\n",
		g_monitor.monitor_state);

	if (g_monitor.monitor_state == MONITOR_STATE_DEINIT) {
		g_monitor.pub = pub;
		mutex_init(&g_monitor.lock);
		g_monitor.monitor_state = MONITOR_STATE_INIT;
	}
	return 0;
}

int wland_monitor_deinit(void)
{
	int i;
	struct net_device *ndev;

	WLAND_DBG(DEFAULT, TRACE, "Enter\n");

	mutex_lock(&g_monitor.lock);
	if (g_monitor.monitor_state != MONITOR_STATE_DEINIT) {
		for (i = 0; i < WLAND_MAX_IFS; i++) {
			ndev = g_monitor.mon_if[i].mon_ndev;
			if (ndev) {
				unregister_netdev(ndev);
				free_netdev(ndev);
				g_monitor.mon_if[i].real_ndev = NULL;
				g_monitor.mon_if[i].mon_ndev = NULL;
			}
		}
		g_monitor.monitor_state = MONITOR_STATE_DEINIT;
	}
	mutex_unlock(&g_monitor.lock);
	return 0;
}

#endif /* WLAND_MONITOR_SUPPORT */
