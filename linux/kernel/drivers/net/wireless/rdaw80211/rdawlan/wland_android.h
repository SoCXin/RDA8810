
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
#ifndef _WLAND_ANDROID_H_
#define _WLAND_ANDROID_H_

#define RSSI_MAXVAL                 -2
#define RSSI_MINVAL                 -200
#define REPEATED_SCAN_RESULT_CNT	4

#ifdef WLAND_RSSIAVG_SUPPORT
#define RSSIAVG_LEN                 (4*REPEATED_SCAN_RESULT_CNT)
#define RSSICACHE_LEN               (4*REPEATED_SCAN_RESULT_CNT)

struct wland_rssi_cache {
	struct wland_rssi_cache *next;
	int dirty;
	u8 BSSID[ETH_ALEN];
	s16 RSSI[RSSIAVG_LEN];
};

struct wland_rssi_cache_ctrl {
	struct wland_rssi_cache *m_cache_head;
};

void wl_free_rssi_cache(struct wland_rssi_cache_ctrl *rssi_cache_ctrl);
void wl_delete_dirty_rssi_cache(struct wland_rssi_cache_ctrl *rssi_cache_ctrl);
void wl_delete_disconnected_rssi_cache(struct wland_rssi_cache_ctrl
	*rssi_cache_ctrl, u8 * bssid);
void wl_reset_rssi_cache(struct wland_rssi_cache_ctrl *rssi_cache_ctrl);
void wl_update_rssi_cache(struct wland_rssi_cache_ctrl *rssi_cache_ctrl,
	struct wland_bss_info_le *bss);
int wl_update_connected_rssi_cache(struct net_device *net,
	struct wland_rssi_cache_ctrl *rssi_cache_ctrl, s16 * rssi_avg);
s16 wl_get_avg_rssi(struct wland_rssi_cache_ctrl *rssi_cache_ctrl, void *addr);
#endif /* WLAND_RSSIAVG_SUPPORT */


#ifdef WLAND_BSSCACHE_SUPPORT
#define BSSCACHE_LEN	            (REPEATED_SCAN_RESULT_CNT)

struct wland_bss_cache {
	struct wland_bss_cache *next;
	int dirty;
	u32 version;
	struct wland_bss_info_le bss;
};
struct wland_bss_cache_ctrl {
	struct wland_bss_cache *m_cache_head;
};

void wl_free_bss_cache(struct wland_bss_cache_ctrl *bss_cache_ctrl);
void wl_delete_dirty_bss_cache(struct wland_bss_cache_ctrl *bss_cache_ctrl);
void wl_delete_disconnected_bss_cache(struct wland_bss_cache_ctrl
	*bss_cache_ctrl, u8 * bssid);
void wl_reset_bss_cache(struct wland_bss_cache_ctrl *bss_cache_ctrl);
void wl_update_bss_cache(struct wland_bss_cache_ctrl *bss_cache_ctrl,
	struct list_head *scan_results_list);
void wl_release_bss_cache_ctrl(struct wland_bss_cache_ctrl *bss_cache_ctrl);
#endif /* WLAND_BSSCACHE_SUPPORT */

int wland_android_priv_cmd(struct net_device *net, struct ifreq *ifr, int cmd);
#endif /* _WLAND_ANDROID_H_ */
