
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
#include <wland_android.h>

/*
 * Android private command strings, PLEASE define new private commands here
 * so they can be updated easily in the future (if needed)
 */

#define CMD_START		        "START"
#define CMD_STOP		        "STOP"
#define	CMD_SCAN_ACTIVE		    "SCAN-ACTIVE"
#define	CMD_SCAN_PASSIVE	    "SCAN-PASSIVE"
#define CMD_RSSI		        "RSSI"
#define CMD_LINKSPEED		    "LINKSPEED"
#define CMD_RXFILTER_START	    "RXFILTER-START"
#define CMD_RXFILTER_STOP	    "RXFILTER-STOP"
#define CMD_RXFILTER_ADD	    "RXFILTER-ADD"
#define CMD_RXFILTER_REMOVE	    "RXFILTER-REMOVE"
#define CMD_BTCOEXSCAN_START	"BTCOEXSCAN-START"
#define CMD_BTCOEXSCAN_STOP	    "BTCOEXSCAN-STOP"
#define CMD_BTCOEXMODE		    "BTCOEXMODE"
#define CMD_SETSUSPENDOPT	    "SETSUSPENDOPT"
#define CMD_SETSUSPENDMODE      "SETSUSPENDMODE"
#define CMD_P2P_DEV_ADDR	    "P2P_DEV_ADDR"
#define CMD_SETFWPATH		    "SETFWPATH"
#define CMD_SETBAND		        "SETBAND"
#define CMD_GETBAND		        "GETBAND"
#define CMD_COUNTRY		        "COUNTRY"
#define CMD_P2P_SET_NOA		    "P2P_SET_NOA"
#if !defined WL_ENABLE_P2P_IF
#define CMD_P2P_GET_NOA		    "P2P_GET_NOA"
#endif
#define CMD_P2P_SD_OFFLOAD		"P2P_SD_"
#define CMD_P2P_SET_PS		    "P2P_SET_PS"
#define CMD_SET_AP_WPS_P2P_IE 	"SET_AP_WPS_P2P_IE"
#define CMD_SETROAMMODE 	    "SETROAMMODE"

/* CCX Private Commands */
#ifdef PNO_SUPPORT
#define CMD_PNOSSIDCLR_SET	    "PNOSSIDCLR"
#define CMD_PNOSETUP_SET	    "PNOSETUP "
#define CMD_PNOENABLE_SET	    "PNOFORCE"
#define CMD_PNODEBUG_SET	    "PNODEBUG"

#define PNO_TLV_PREFIX			'S'
#define PNO_TLV_VERSION			'1'
#define PNO_TLV_SUBVERSION 		'2'
#define PNO_TLV_RESERVED		'0'
#define PNO_TLV_TYPE_SSID_IE	'S'
#define PNO_TLV_TYPE_TIME		'T'
#define PNO_TLV_FREQ_REPEAT		'R'
#define PNO_TLV_FREQ_EXPO_MAX	'M'

struct cmd_tlv {
	char prefix;
	char version;
	char subver;
	char reserved;
};
#endif /* PNO_SUPPORT */

#define CMD_OKC_SET_PMK		    "SET_PMK"
#define CMD_OKC_ENABLE		    "OKC_ENABLE"

struct android_wifi_priv_cmd {
	char *buf;
	int used_len;
	int total_len;
};

/*
 * Local (static) function definitions
 */
static int wl_android_get_link_speed(struct net_device *net, char *command,
	int total_len)
{
	int bytes_written = 0;

#if 0
	int link_speed;
	int error = wldev_get_link_speed(net, &link_speed);

	if (error)
		return -1;

	/*
	 * Convert Kbps to Android Mbps
	 */
	link_speed = link_speed / 1000;
	bytes_written =
		snprintf(command, total_len, "LinkSpeed %d", link_speed);
#endif
	WLAND_DBG(DEFAULT, TRACE, "command result is %s\n", command);
	return bytes_written;
}

static int wl_android_get_rssi(struct net_device *net, char *command,
	int total_len)
{
	int bytes_written = 0;

#if 0
	struct wlc_ssid ssid = { 0 };
	int rssi, error;

	error = wldev_get_rssi(net, &rssi);
	if (error)
		return -1;
#if defined(WLAND_RSSIOFFSET_SUPPORT)
	rssi = wl_update_rssi_offset(rssi);
#endif

	error = wldev_get_ssid(net, &ssid);
	if (error)
		return -1;
	if ((ssid.SSID_len == 0) || (ssid.SSID_len > DOT11_MAX_SSID_LEN)) {
		WLAND_ERR("wldev_get_ssid failed\n");
	} else {
		memcpy(command, ssid.SSID, ssid.SSID_len);
		bytes_written = ssid.SSID_len;
	}
	bytes_written +=
		snprintf(&command[bytes_written], total_len, " rssi %d", rssi);
#endif
	WLAND_DBG(DEFAULT, TRACE, "command result is %s (%d)\n", command,
		bytes_written);
	return bytes_written;
}

static int wl_android_set_suspendopt(struct net_device *dev, char *command,
	int total_len)
{
	int ret = 0;

#if 0
	int suspend_flag, ret_now;

	suspend_flag = *(command + strlen(CMD_SETSUSPENDOPT) + 1) - '0';

	if (suspend_flag)
		suspend_flag = 1;
	ret_now = net_os_set_suspend_disable(dev, suspend_flag);

	if (ret_now != suspend_flag) {
		if (!(ret = net_os_set_suspend(dev, ret_now, 1)))
			WLAND_DBG(DEFAULT, TRACE, "Suspend Flag %d -> %d\n",
				ret_now, suspend_flag);
		else
			WLAND_DBG(DEFAULT, TRACE, "failed %d\n", ret);
	}
#endif
	WLAND_DBG(DEFAULT, TRACE, "command result is %s (%d)\n", command,
		total_len);

	return ret;
}

static int wl_android_set_suspendmode(struct net_device *dev, char *command,
	int total_len)
{
	int ret = 0;

#if 0
#if !defined(CONFIG_HAS_EARLYSUSPEND) || !defined(DHD_USE_EARLYSUSPEND)
	int suspend_flag = *(command + strlen(CMD_SETSUSPENDMODE) + 1) - '0';

	if (suspend_flag)
		suspend_flag = 1;

	if (!(ret = net_os_set_suspend(dev, suspend_flag, 0)))
		WLAND_DBG(DEFAULT, TRACE, "Suspend Mode %d\n", suspend_flag);
	else
		WLAND_DBG(DEFAULT, TRACE, "failed %d\n", ret);
#endif
#endif
	WLAND_DBG(DEFAULT, TRACE, "command result is %s (%d)\n", command,
		total_len);

	return ret;
}

static int wl_android_get_band(struct net_device *dev, char *command,
	int total_len)
{
	int bytes_written = 0;

#if 0
	uint band;
	int error = wldev_get_band(dev, &band);

	if (error)
		return -1;
	bytes_written = snprintf(command, total_len, "Band %d", band);
#endif
	return bytes_written;
}

#if defined(PNO_SUPPORT)
static int wl_android_set_pno_setup(struct net_device *dev, char *command,
	int total_len)
{
	struct wlc_ssid ssids_local[MAX_PFN_LIST_COUNT];
	int res = -1;
	int nssid = 0;
	struct cmd_tlv *cmd_tlv_temp;
	char *str_ptr;
	int tlv_size_left;
	int pno_time = 0;
	int pno_repeat = 0;
	int pno_freq_expo_max = 0;

	WLAND_DBG(DEFAULT, TRACE, "command=%s, len=%d\n", command, total_len);

	if (total_len < (strlen(CMD_PNOSETUP_SET) + sizeof(struct cmd_tlv))) {
		WLAND_ERR("argument=%d less min size\n", total_len);
		goto exit_proc;
	}

	str_ptr = command + strlen(CMD_PNOSETUP_SET);

	tlv_size_left = total_len - strlen(CMD_PNOSETUP_SET);
	cmd_tlv_temp = (struct cmd_tlv *) str_ptr;

	memset(ssids_local, 0, sizeof(ssids_local));

	if ((cmd_tlv_temp->prefix == PNO_TLV_PREFIX) &&
		(cmd_tlv_temp->version == PNO_TLV_VERSION) &&
		(cmd_tlv_temp->subver == PNO_TLV_SUBVERSION)) {
		str_ptr += sizeof(struct cmd_tlv);
		tlv_size_left -= sizeof(struct cmd_tlv);

		if ((nssid = wl_iw_parse_ssid_list_tlv(&str_ptr, ssids_local,
					MAX_PFN_LIST_COUNT,
					&tlv_size_left)) <= 0) {
			WLAND_ERR("SSID is not presented or corrupted ret=%d\n",
				nssid);
			goto exit_proc;
		} else {
			if ((str_ptr[0] != PNO_TLV_TYPE_TIME)
				|| (tlv_size_left <= 1)) {
				WLAND_ERR
					("scan duration corrupted field size %d\n",
					tlv_size_left);
				goto exit_proc;
			}
			str_ptr++;
			pno_time = simple_strtoul(str_ptr, &str_ptr, 16);

			WLAND_DBG(DEFAULT, TRACE, "pno_time=%d\n", pno_time);

			if (str_ptr[0] != 0) {
				if ((str_ptr[0] != PNO_TLV_FREQ_REPEAT)) {
					WLAND_ERR
						("pno repeat : corrupted field\n");
					goto exit_proc;
				}
				str_ptr++;
				pno_repeat =
					simple_strtoul(str_ptr, &str_ptr, 16);
				WLAND_DBG(DEFAULT, TRACE,
					"%s :got pno_repeat=%d\n", pno_repeat);
				if (str_ptr[0] != PNO_TLV_FREQ_EXPO_MAX) {
					WLAND_ERR
						("FREQ_EXPO_MAX corrupted field size\n");
					goto exit_proc;
				}
				str_ptr++;
				pno_freq_expo_max =
					simple_strtoul(str_ptr, &str_ptr, 16);
				WLAND_DBG(DEFAULT, TRACE,
					"%s: pno_freq_expo_max=%d\n",
					pno_freq_expo_max);
			}
		}
	} else {
		WLAND_ERR("get wrong TLV command\n");
		goto exit_proc;
	}

	res = dhd_dev_pno_set(dev, ssids_local, nssid, pno_time, pno_repeat,
		pno_freq_expo_max);

exit_proc:
	return res;
}
#endif /* PNO_SUPPORT */

static int wl_android_get_p2p_dev_addr(struct net_device *ndev, char *command,
	int total_len)
{
	int bytes_written = 0;

#if 0
	int ret = wl_cfg80211_get_p2p_dev_addr(ndev,
		(struct ether_addr *) command);

	if (ret)
		return 0;
	bytes_written = sizeof(struct ether_addr);
#endif
	return bytes_written;
}

static int wl_android_set_pmk(struct net_device *dev, char *command,
	int total_len)
{
#if 0
	u8 pmk[33];
	char smbuf[WLC_IOCTL_SMLEN];

	memset(pmk, '\0', sizeof(pmk));
	memcpy(pmk, command + strlen("SET_PMK "), 32);

	return wldev_iovar_setbuf(dev, "okc_info_pmk", pmk, 32, smbuf,
		sizeof(smbuf), NULL);
#endif
	return 0;
}

static int wl_android_okc_enable(struct net_device *dev, char *command,
	int total_len)
{
#if 0
	char okc_enable = command[strlen(CMD_OKC_ENABLE) + 1] - '0';

	WLAND_DBG(DEFAULT, TRACE, "Failed to %s OKC.\n",
		okc_enable ? "enable" : "disable");

	return wldev_iovar_setint(dev, "okc_enable", okc_enable);
#endif
	return 0;
}

int wl_android_set_roam_mode(struct net_device *dev, char *command,
	int total_len)
{
#if 0
	int mode = 0;

	if (sscanf(command, "%*s %d", &mode) != 1) {
		WLAND_ERR("Failed to get Parameter\n", __FUNCTION__);
		return -1;
	}

	return wldev_iovar_setint(dev, "roam_off", mode);
#endif
	return 0;
}

int wland_android_priv_cmd(struct net_device *net, struct ifreq *ifr, int cmd)
{
#define PRIVATE_COMMAND_MAX_LEN	8192
	int ret = 0, bytes_written = 0;
	char *command = NULL;
	struct android_wifi_priv_cmd priv_cmd;

	WLAND_DBG(DEFAULT, TRACE, "Enter\n");

#if 0
	net_os_wake_lock(net);
#endif
	if (!ifr->ifr_data) {
		ret = -EINVAL;
		goto exit;
	}
	if (copy_from_user(&priv_cmd, ifr->ifr_data,
			sizeof(struct android_wifi_priv_cmd))) {
		ret = -EFAULT;
		goto exit;
	}
	if (priv_cmd.total_len > PRIVATE_COMMAND_MAX_LEN) {
		WLAND_ERR("too long priavte command\n");
		ret = -EINVAL;
	}
	command = memdup_user(priv_cmd.buf, priv_cmd.total_len);
	if (IS_ERR(command)) {
		WLAND_ERR("failed to allocate or write memory\n");
		ret= PTR_ERR(command);
		goto exit;
	}

	WLAND_DBG(DEFAULT, TRACE, "Android private cmd \"%s\" on %s\n", command,
		ifr->ifr_name);

	if (strnicmp(command, CMD_START, strlen(CMD_START)) == 0) {
		WLAND_DBG(DEFAULT, INFO, "Received regular Start command\n");
#if 0
		bytes_written = wl_android_wifi_on(net);
#endif
	} else if (strnicmp(command, CMD_SETFWPATH, strlen(CMD_SETFWPATH)) == 0) {
		WLAND_DBG(DEFAULT, INFO, "Received regular Set_fwpath command\n");
	} else if (strnicmp(command, CMD_STOP, strlen(CMD_STOP)) == 0) {
		WLAND_DBG(DEFAULT, INFO, "Received regular Stop command\n");
#if 0
		bytes_written = wl_android_wifi_off(net);
#endif
	} else if (strnicmp(command, CMD_SCAN_ACTIVE,
			strlen(CMD_SCAN_ACTIVE)) == 0) {
		/*
		 * TBD: SCAN-ACTIVE
		 */
		 WLAND_DBG(DEFAULT, INFO, "Received regular SCAN_ACTIVE command\n");
	} else if (strnicmp(command, CMD_SCAN_PASSIVE,
			strlen(CMD_SCAN_PASSIVE)) == 0) {
		/*
		 * TBD: SCAN-PASSIVE
		 */
		  WLAND_DBG(DEFAULT, INFO, "Received regular SCAN_PASSIVE command\n");
	} else if (strnicmp(command, CMD_RSSI, strlen(CMD_RSSI)) == 0) {
		bytes_written =
			wl_android_get_rssi(net, command, priv_cmd.total_len);
		WLAND_DBG(DEFAULT, INFO, "Received regular RSSI command\n");
	} else if (strnicmp(command, CMD_LINKSPEED, strlen(CMD_LINKSPEED)) == 0) {
		bytes_written =
			wl_android_get_link_speed(net, command,
			priv_cmd.total_len);
		WLAND_DBG(DEFAULT, INFO, "Received regular LINKSPEED command\n");
	}
#ifdef PKT_FILTER_SUPPORT
	else if (strnicmp(command, CMD_RXFILTER_START,
			strlen(CMD_RXFILTER_START)) == 0) {
		bytes_written = net_os_enable_packet_filter(net, 1);
	} else if (strnicmp(command, CMD_RXFILTER_STOP,
			strlen(CMD_RXFILTER_STOP)) == 0) {
		bytes_written = net_os_enable_packet_filter(net, 0);
	} else if (strnicmp(command, CMD_RXFILTER_ADD,
			strlen(CMD_RXFILTER_ADD)) == 0) {
		int filter_num =
			*(command + strlen(CMD_RXFILTER_ADD) + 1) - '0';
		bytes_written =
			net_os_rxfilter_add_remove(net, true, filter_num);
	} else if (strnicmp(command, CMD_RXFILTER_REMOVE,
			strlen(CMD_RXFILTER_REMOVE)) == 0) {
		int filter_num =
			*(command + strlen(CMD_RXFILTER_REMOVE) + 1) - '0';
		bytes_written =
			net_os_rxfilter_add_remove(net, FALSE, filter_num);
	}
#endif /* PKT_FILTER_SUPPORT */
	else if (strnicmp(command, CMD_BTCOEXSCAN_START,
			strlen(CMD_BTCOEXSCAN_START)) == 0) {
		/*
		 * TBD: BTCOEXSCAN-START
		 */
		 WLAND_DBG(DEFAULT, INFO, "Received regular BTCOEXSCAN_START command\n");
	} else if (strnicmp(command, CMD_BTCOEXSCAN_STOP,
			strlen(CMD_BTCOEXSCAN_STOP)) == 0) {
		/*
		 * TBD: BTCOEXSCAN-STOP
		 */
		  WLAND_DBG(DEFAULT, INFO, "Received regular BTCOEXSCAN_STOP command\n");
	} else if (strnicmp(command, CMD_BTCOEXMODE,
			strlen(CMD_BTCOEXMODE)) == 0) {
			WLAND_DBG(DEFAULT, INFO, "Received regular BTCOEXMODE command\n");
#ifdef WLAND_CFG80211_SUPPORT
#if 0
		bytes_written = wl_cfg80211_set_btcoex_dhcp(net, command);
#endif
#else
#ifdef PKT_FILTER_SUPPORT
		uint mode = *(command + strlen(CMD_BTCOEXMODE) + 1) - '0';

		if (mode == 1)
			net_os_enable_packet_filter(net, 0);	/* DHCP starts */
		else
			net_os_enable_packet_filter(net, 1);	/* DHCP ends */
#endif /* PKT_FILTER_SUPPORT */
#endif /* WLAND_CFG80211_SUPPORT */
	} else if (strnicmp(command, CMD_SETSUSPENDOPT,
			strlen(CMD_SETSUSPENDOPT)) == 0) {
		WLAND_DBG(DEFAULT, INFO, "Received regular SETSUSPENDOPT command\n");
		bytes_written =
			wl_android_set_suspendopt(net, command,
			priv_cmd.total_len);
	} else if (strnicmp(command, CMD_SETSUSPENDMODE,
			strlen(CMD_SETSUSPENDMODE)) == 0) {
		WLAND_DBG(DEFAULT, INFO, "Received regular SETSUSPENDMODE command\n");
		bytes_written =
			wl_android_set_suspendmode(net, command,
			priv_cmd.total_len);
	} else if (strnicmp(command, CMD_SETBAND, strlen(CMD_SETBAND)) == 0) {
		WLAND_DBG(DEFAULT, INFO, "Received regular SETBAND command\n");
#if 0
		uint band = *(command + strlen(CMD_SETBAND) + 1) - '0';

		bytes_written = wldev_set_band(net, band);
#endif
	} else if (strnicmp(command, CMD_GETBAND, strlen(CMD_GETBAND)) == 0) {
		WLAND_DBG(DEFAULT, INFO, "Received regular GETBAND command\n");
		bytes_written =
			wl_android_get_band(net, command, priv_cmd.total_len);
	}
#ifdef WLAND_CFG80211_SUPPORT
	/*
	 * CUSTOMER_SET_COUNTRY feature is define for only GGSM model
	 */
	else if (strnicmp(command, CMD_COUNTRY, strlen(CMD_COUNTRY)) == 0) {
		WLAND_DBG(DEFAULT, INFO, "Received regular COUNTRY command\n");
#if 0
		char *country_code = command + strlen(CMD_COUNTRY) + 1;

		bytes_written = wldev_set_country(net, country_code);
#endif
	}
#endif /* WLAND_CFG80211_SUPPORT */
#if defined(PNO_SUPPORT)
	else if (strnicmp(command, CMD_PNOSSIDCLR_SET,
			strlen(CMD_PNOSSIDCLR_SET)) == 0) {
		bytes_written = dhd_dev_pno_reset(net);
	} else if (strnicmp(command, CMD_PNOSETUP_SET,
			strlen(CMD_PNOSETUP_SET)) == 0) {
		bytes_written =
			wl_android_set_pno_setup(net, command,
			priv_cmd.total_len);
	} else if (strnicmp(command, CMD_PNOENABLE_SET,
			strlen(CMD_PNOENABLE_SET)) == 0) {
		uint pfn_enabled =
			*(command + strlen(CMD_PNOENABLE_SET) + 1) - '0';
		bytes_written = dhd_dev_pno_enable(net, pfn_enabled);
	}
#endif /* PNO_SUPPORT */
	else if (strnicmp(command, CMD_P2P_DEV_ADDR,
			strlen(CMD_P2P_DEV_ADDR)) == 0) {
		WLAND_DBG(DEFAULT, INFO, "Received regular P2P_DEV_ADDR command\n");
		bytes_written =
			wl_android_get_p2p_dev_addr(net, command,
			priv_cmd.total_len);
	} else if (strnicmp(command, CMD_P2P_SET_NOA,
			strlen(CMD_P2P_SET_NOA)) == 0) {
		WLAND_DBG(DEFAULT, INFO, "Received regular P2P_SET_NOA command\n");
#if 0
		int skip = strlen(CMD_P2P_SET_NOA) + 1;

		bytes_written =
			wl_cfg80211_set_p2p_noa(net, command + skip,
			priv_cmd.total_len - skip);
#endif
	} else if (strnicmp(command, CMD_P2P_GET_NOA,
			strlen(CMD_P2P_GET_NOA)) == 0) {
		WLAND_DBG(DEFAULT, INFO, "Received regular P2P_GET_NOA command\n");
#if 0
		bytes_written =
			wl_cfg80211_get_p2p_noa(net, command,
			priv_cmd.total_len);
#endif
	} else if (strnicmp(command, CMD_P2P_SET_PS,
			strlen(CMD_P2P_SET_PS)) == 0) {
		WLAND_DBG(DEFAULT, INFO, "Received regular P2P_SET_PS command\n");
#if 0
		int skip = strlen(CMD_P2P_SET_PS) + 1;

		bytes_written =
			wl_cfg80211_set_p2p_ps(net, command + skip,
			priv_cmd.total_len - skip);
#endif
	}
#ifdef WLAND_CFG80211_SUPPORT
	else if (strnicmp(command, CMD_SET_AP_WPS_P2P_IE,
			strlen(CMD_SET_AP_WPS_P2P_IE)) == 0) {
		WLAND_DBG(DEFAULT, INFO, "Received regular SET_AP_WPS_P2P_IE command\n");
#if 0
		int skip = strlen(CMD_SET_AP_WPS_P2P_IE) + 3;

		bytes_written =
			wl_cfg80211_set_wps_p2p_ie(net, command + skip,
			priv_cmd.total_len - skip, *(command + skip - 2) - '0');
#endif
	}
#endif /* WLAND_CFG80211_SUPPORT */
	else if (strnicmp(command, CMD_OKC_SET_PMK,
			strlen(CMD_OKC_SET_PMK)) == 0) {
		WLAND_DBG(DEFAULT, INFO, "Received regular OKC_SET_PMK command\n");
		bytes_written =
			wl_android_set_pmk(net, command, priv_cmd.total_len);
	} else if (strnicmp(command, CMD_OKC_ENABLE,
			strlen(CMD_OKC_ENABLE)) == 0) {
		WLAND_DBG(DEFAULT, INFO, "Received regular OKC_ENABLE command\n");
		bytes_written =
			wl_android_okc_enable(net, command, priv_cmd.total_len);
	} else if (strnicmp(command, CMD_SETROAMMODE,
			strlen(CMD_SETROAMMODE)) == 0) {
		WLAND_DBG(DEFAULT, INFO, "Received regular SETROAMMODE command\n");
		bytes_written =
			wl_android_set_roam_mode(net, command,
			priv_cmd.total_len);
	} else {
		WLAND_DBG(DEFAULT, DEBUG,
			"Unknown PRIVATE command %s - ignored\n", command);
		snprintf(command, 3, "OK");
		bytes_written = strlen("OK");
	}

	if (bytes_written >= 0) {
		if ((bytes_written == 0) && (priv_cmd.total_len > 0))
			command[0] = '\0';
		if (bytes_written >= priv_cmd.total_len) {
			WLAND_ERR("bytes_written = %d\n", bytes_written);
			bytes_written = priv_cmd.total_len;
		} else {
			bytes_written++;
		}
		priv_cmd.used_len = bytes_written;
		if (copy_to_user(priv_cmd.buf, command, bytes_written)) {
			WLAND_ERR("failed to copy data to user buffer\n");
			ret = -EFAULT;
		}
	} else {
		ret = bytes_written;
	}

exit:
#if 0
	net_os_wake_unlock(net);
#endif
	WLAND_DBG(DEFAULT, TRACE, "Done(\"%s\",on:%s,ret:%d)\n", command,
		ifr->ifr_name, ret);

	if (command)
		kfree(command);

	return ret;
}

#ifdef WLAND_RSSIAVG_SUPPORT
void wl_free_rssi_cache(struct wland_rssi_cache_ctrl *rssi_cache_ctrl)
{
	struct wland_rssi_cache *node, *cur, **rssi_head;
	int i = 0;

	rssi_head = &rssi_cache_ctrl->m_cache_head;
	node = *rssi_head;

	while (node) {
		WLAND_DBG(DEFAULT, TRACE, "Free %d with BSSID %pM\n", i,
			node->BSSID);
		cur = node;
		node = cur->next;
		kfree(cur);
		i++;
	}
	*rssi_head = NULL;
}

void wl_delete_dirty_rssi_cache(struct wland_rssi_cache_ctrl *rssi_cache_ctrl)
{
	struct wland_rssi_cache *node, *prev, **rssi_head;
	int i = -1, tmp = 0;
	int max = RSSICACHE_LEN;

	max = min(max, RSSICACHE_LEN);

	rssi_head = &rssi_cache_ctrl->m_cache_head;
	node = *rssi_head;
	prev = node;

	for (; node;) {
		i++;
		if (node->dirty > max) {
			if (node == *rssi_head) {
				tmp = 1;
				*rssi_head = node->next;
			} else {
				tmp = 0;
				prev->next = node->next;
			}
			WLAND_DBG(DEFAULT, TRACE, "Del %d with BSSID %pM\n", i,
				node->BSSID);
			kfree(node);
			if (tmp == 1) {
				node = *rssi_head;
				prev = node;
			} else {
				node = prev->next;
			}
			continue;
		}
		prev = node;
		node = node->next;
	}
}

void wl_delete_disconnected_rssi_cache(struct wland_rssi_cache_ctrl
	*rssi_cache_ctrl, u8 * bssid)
{
	struct wland_rssi_cache *node, *prev, **rssi_head;
	int i = -1, tmp = 0;

	rssi_head = &rssi_cache_ctrl->m_cache_head;
	node = *rssi_head;
	prev = node;

	for (; node;) {
		i++;
		if (!memcmp(node->BSSID, bssid, ETH_ALEN)) {
			if (node == *rssi_head) {
				tmp = 1;
				*rssi_head = node->next;
			} else {
				tmp = 0;
				prev->next = node->next;
			}
			WLAND_DBG(DEFAULT, TRACE, "Del %d with BSSID %pM\n", i,
				node->BSSID);
			kfree(node);
			if (tmp == 1) {
				node = *rssi_head;
				prev = node;
			} else {
				node = prev->next;
			}
			continue;
		}
		prev = node;
		node = node->next;
	}
}

void wl_reset_rssi_cache(struct wland_rssi_cache_ctrl *rssi_cache_ctrl)
{
	struct wland_rssi_cache *node, **rssi_head;

	rssi_head = &rssi_cache_ctrl->m_cache_head;
	/*
	 * reset dirty
	 */
	node = *rssi_head;
	for (; node;) {
		node->dirty += 1;
		node = node->next;
	}
}

int wl_update_connected_rssi_cache(struct net_device *ndev,
	struct wland_rssi_cache_ctrl *rssi_cache_ctrl, s16 * rssi_avg)
{
	struct wland_cfg80211_profile *profile = ndev_to_prof(ndev);
	struct wland_rssi_cache *node, *prev, *leaf, **rssi_head;
	int j, k = 0, error = 0;
	s16 rssi = 0;
	u8 *bssid = profile->bssid;

	if (!profile->valid_bssid) {
		WLAND_ERR("Invalid BSSID:%pM\n", bssid);
		return -1;
	}
	//get rssi default value rssi = wl_get_avg_rssi(rssi_cache_ctrl, bssid);

	error = wland_dev_get_rssi(ndev, &rssi);
	if (error) {
		WLAND_ERR("Could not get rssi (%d)\n", error);
		return error;
	}
	/*
	 * update RSSI
	 */
	rssi_head = &rssi_cache_ctrl->m_cache_head;
	node = *rssi_head;
	prev = NULL;

	for (; node;) {
		if (!memcmp(node->BSSID, bssid, ETH_ALEN)) {
			WLAND_DBG(DEFAULT, TRACE,
				"Update %d with BSSID %pM, RSSI=%d\n", k, bssid,
				rssi);
			for (j = 0; j < RSSIAVG_LEN - 1; j++)
				node->RSSI[j] = node->RSSI[j + 1];
			node->RSSI[j] = rssi;
			node->dirty = 0;
			goto exit;
		}
		prev = node;
		node = node->next;
		k++;
	}

	leaf = kmalloc(sizeof(struct wland_rssi_cache), GFP_KERNEL);
	if (!leaf) {
		WLAND_ERR("Memory alloc failure %d\n",
			sizeof(struct wland_rssi_cache));
		return 0;
	}
	WLAND_DBG(DEFAULT, TRACE,
		"Add %d with cached BSSID %pM, RSSI=%d in the leaf\n", k,
		&bssid, rssi);

	leaf->next = NULL;
	leaf->dirty = 0;

	memcpy(leaf->BSSID, bssid, ETH_ALEN);

	for (j = 0; j < RSSIAVG_LEN; j++)
		leaf->RSSI[j] = rssi;

	if (!prev)
		*rssi_head = leaf;
	else
		prev->next = leaf;

exit:
	*rssi_avg = wl_get_avg_rssi(rssi_cache_ctrl, bssid);

	return error;
}

void wl_update_rssi_cache(struct wland_rssi_cache_ctrl *rssi_cache_ctrl,
	struct wland_bss_info_le *bss)
{
	struct wland_rssi_cache *node, *prev, *leaf, **rssi_head;
	struct wland_bss_info_le *bi = NULL;
	int j, k;

	rssi_head = &rssi_cache_ctrl->m_cache_head;

	/*
	 * update RSSI
	 */

	bi = bss;
	node = *rssi_head;
	prev = NULL;
	k = 0;
	for (; node;) {
		if (!memcmp(node->BSSID, bi->BSSID, ETH_ALEN)) {
			WLAND_DBG(DEFAULT, TRACE,
				"Update %d with BSSID %pM, RSSI=%d, SSID \"%s\"\n",
				k, bi->BSSID, bi->RSSI, bi->SSID);
			for (j = 0; j < RSSIAVG_LEN - 1; j++)
				node->RSSI[j] = node->RSSI[j + 1];
			node->RSSI[j] = bi->RSSI;
			node->dirty = 0;
			break;
		}
		prev = node;

		node = node->next;
		k++;
	}

	if (node)
		return;

	leaf = kmalloc(sizeof(struct wland_rssi_cache), GFP_KERNEL);
	if (!leaf) {
		WLAND_ERR("Memory alloc failure %d\n",
			sizeof(struct wland_rssi_cache));
		return;
	}
	WLAND_DBG(DEFAULT, TRACE,
		"Add %d with cached BSSID %pM, RSSI=%d, SSID \"%s\" in the leaf\n",
		k, &bi->BSSID, bi->RSSI, bi->SSID);

	leaf->next = NULL;
	leaf->dirty = 0;

	memcpy(leaf->BSSID, bi->BSSID, ETH_ALEN);

	for (j = 0; j < RSSIAVG_LEN; j++)
		leaf->RSSI[j] = bi->RSSI;

	if (!prev)
		*rssi_head = leaf;
	else
		prev->next = leaf;

}

s16 wl_get_avg_rssi(struct wland_rssi_cache_ctrl *rssi_cache_ctrl, void *addr)
{
	struct wland_rssi_cache *node, **rssi_head;
	int j, rssi_sum;
	s16 rssi = RSSI_MINVAL;

	rssi_head = &rssi_cache_ctrl->m_cache_head;
	/*
	 * reset dirty
	 */
	node = *rssi_head;
	for (; node;) {
		if (!memcmp(node->BSSID, addr, ETH_ALEN)) {
			rssi_sum = 0;
			rssi = 0;
			for (j = 0; j < RSSIAVG_LEN; j++)
				rssi_sum += node->RSSI[RSSIAVG_LEN - j - 1];
			rssi = rssi_sum / j;
			break;
		}
		node = node->next;
	}
	rssi = MIN(rssi, RSSI_MAXVAL);

	if (rssi == RSSI_MINVAL) {
		WLAND_ERR("BSSID %pM does not in RSSI cache\n", addr);
	}

	return rssi;
}
#endif /* WLAND_RSSIAVG_SUPPORT */


#ifdef WLAND_BSSCACHE_SUPPORT
void wl_free_bss_cache(struct wland_bss_cache_ctrl *bss_cache_ctrl)
{
	struct wland_bss_cache *node, *cur, **bss_head;
	int i = 0;

	WLAND_DBG(DEFAULT, TRACE, "Enter\n");

	bss_head = &bss_cache_ctrl->m_cache_head;
	node = *bss_head;

	for (; node;) {
		WLAND_DBG(DEFAULT, TRACE, "Free %d with BSSID %pM\n", i,
			node->bss.BSSID);
		cur = node;
		node = cur->next;
		if(cur->bss.ie)
			kfree(cur->bss.ie);
		kfree(cur);
		i++;
	}
	*bss_head = NULL;
}

void wl_delete_dirty_bss_cache(struct wland_bss_cache_ctrl *bss_cache_ctrl)
{
	struct wland_bss_cache *node, *prev, **bss_head;
	int i = -1, tmp = 0;

	WLAND_DBG(CFG80211, TRACE, "Enter\n");
	bss_head = &bss_cache_ctrl->m_cache_head;
	node = *bss_head;
	prev = node;

	for (; node;) {
		i++;
		if (node->dirty > BSSCACHE_LEN) {
			if (node == *bss_head) {
				tmp = 1;
				*bss_head = node->next;
			} else {
				tmp = 0;
				prev->next = node->next;
			}
			WLAND_DBG(DEFAULT, TRACE,
				"Del %d with BSSID %pM, RSSI=%d, SSID \"%s\"\n",
				i, node->bss.BSSID,
				node->bss.RSSI,
				node->bss.SSID);
			if(node->bss.ie)
				kfree(node->bss.ie);
			kfree(node);
			if (tmp == 1) {
				node = *bss_head;
				prev = node;
			} else {
				node = prev->next;
			}
			continue;
		}
		prev = node;
		node = node->next;
	}
}

void wl_delete_disconnected_bss_cache(struct wland_bss_cache_ctrl
	*bss_cache_ctrl, u8 * bssid)
{
	struct wland_bss_cache *node, *prev, **bss_head;
	int i = -1, tmp = 0;

	bss_head = &bss_cache_ctrl->m_cache_head;
	node = *bss_head;
	prev = node;

	for (; node;) {
		i++;

		if (!memcmp(node->bss.BSSID, bssid, ETH_ALEN)) {
			if (node == *bss_head) {
				tmp = 1;
				*bss_head = node->next;
			} else {
				tmp = 0;
				prev->next = node->next;
			}
			WLAND_DBG(DEFAULT, TRACE,
				"Del %d with BSSID %pM, RSSI=%d, SSID \"%s\"\n",
				i, node->bss.BSSID,
				node->bss.RSSI,
				node->bss.SSID);
			kfree(node);
			if (tmp == 1) {
				node = *bss_head;
				prev = node;
			} else {
				node = prev->next;
			}
			continue;
		}
		prev = node;
		node = node->next;
	}
}

void wl_reset_bss_cache(struct wland_bss_cache_ctrl *bss_cache_ctrl)
{
	struct wland_bss_cache *node, **bss_head;

	bss_head = &bss_cache_ctrl->m_cache_head;
	/*
	 * reset dirty
	 */
	node = *bss_head;
	for (; node;) {
		node->dirty += 1;
		node = node->next;
	}
}

void wl_update_bss_cache(struct wland_bss_cache_ctrl *bss_cache_ctrl,
	struct list_head *scan_result_list)
{
	struct wland_bss_cache *node, *prev, *leaf, **bss_head;
	struct wland_bss_info_le *bi = NULL;
	struct wland_cfg80211_info *cfg =
		container_of(scan_result_list, struct wland_cfg80211_info, scan_result_list);
	int k = 0;

	WLAND_DBG(CFG80211, TRACE, "Enter\n");

	if (list_empty(scan_result_list)) {
		WLAND_ERR("ss_list->count ==0 \n");
		return;
	}

	bss_head = &bss_cache_ctrl->m_cache_head;

	list_for_each_entry(bi, scan_result_list, list) {
		node = *bss_head;
		prev = NULL;

		for (; node;) {
			if (!memcmp(node->bss.BSSID, bi->BSSID,
					ETH_ALEN)) {
				leaf = node;
				if(leaf->bss.ie)
					kfree(leaf->bss.ie);
				memset(&leaf->bss, 0,
					sizeof(struct wland_bss_info_le));
				memcpy(&leaf->bss, bi,
					sizeof(struct wland_bss_info_le));
                                leaf->bss.ie = kmemdup(bi->ie, bi->ie_length,GFP_KERNEL);
				if (!leaf->bss.ie) {
					WLAND_ERR("Memory alloc failure %d\n", bi->ie_length);
					return;
				}
				leaf->next = node->next;
				leaf->dirty = 0;
				leaf->version = cfg->scan_results.version;

				WLAND_DBG(DEFAULT, TRACE,
					"Update %d with BSSID %pM, RSSI=%d, SSID \"%s\", length=%d\n",
					k, bi->BSSID, bi->RSSI, bi->SSID,
					bi->length);
				if (!prev)
					*bss_head = leaf;
				else
					prev->next = leaf;
				node = leaf;
				prev = node;

				k++;
				break;
			}
			prev = node;
			node = node->next;
		}

		if (node)
			continue;

		leaf = kmalloc(sizeof(struct wland_bss_cache), GFP_KERNEL);
		if (!leaf) {
			WLAND_ERR("Memory alloc failure %d\n", sizeof(struct wland_bss_cache));
			return;
		}

		WLAND_DBG(DEFAULT, TRACE,
			"Add %d with cached BSSID %pM, RSSI=%d, SSID \"%s\" in the leaf\n",
			k, &bi->BSSID, bi->RSSI, bi->SSID);

		memcpy(&leaf->bss, bi, sizeof(struct wland_bss_info_le));
		leaf->bss.ie = kmalloc(bi->ie_length, GFP_KERNEL);
		if (!(leaf->bss.ie)) {
			WLAND_ERR("Memory alloc failure %d\n", bi->ie_length);
			kfree(leaf);
			return;
		}
		memcpy(leaf->bss.ie, bi->ie, bi->ie_length);

		leaf->next = NULL;
		leaf->dirty = 0;
		leaf->version = cfg->scan_results.version;
		k++;

		if (!prev)
			*bss_head = leaf;
		else
			prev->next = leaf;
	}
	WLAND_DBG(CFG80211, TRACE, "Done\n");
}

void wl_release_bss_cache_ctrl(struct wland_bss_cache_ctrl *bss_cache_ctrl)
{
	WLAND_DBG(DEFAULT, TRACE, "Enter\n");
	wl_free_bss_cache(bss_cache_ctrl);
}

#endif /* WLAND_BSSCACHE_SUPPORT */
