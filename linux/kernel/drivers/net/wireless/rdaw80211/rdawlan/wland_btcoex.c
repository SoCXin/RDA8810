
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
#ifdef WLAND_BTCOEX_SUPPORT

#include <linux/slab.h>
#include <linux/netdevice.h>
#include <net/cfg80211.h>

#include <wland_utils.h>
#include <wland_defs.h>
#include <wland_dev.h>
#include <wland_dbg.h>
#include <wland_btcoex.h>
#include <wland_p2p.h>
#include <wland_cfg80211.h>

/* T1 start SCO/eSCO priority suppression */
#define BRCMF_BTCOEX_OPPR_WIN_TIME      2000

/* BT registers values during DHCP */
#define BRCMF_BT_DHCP_REG50             0x8022
#define BRCMF_BT_DHCP_REG51             0
#define BRCMF_BT_DHCP_REG64             0
#define BRCMF_BT_DHCP_REG65             0
#define BRCMF_BT_DHCP_REG71             0
#define BRCMF_BT_DHCP_REG66             0x2710
#define BRCMF_BT_DHCP_REG41             0x33
#define BRCMF_BT_DHCP_REG68             0x190

/* number of samples for SCO detection */
#define BRCMF_BT_SCO_SAMPLES            12

/**
* enum wland_btcoex_state - BT coex DHCP state machine states
* @WLAND_BT_DHCP_IDLE : DCHP is idle
* @WLAND_BT_DHCP_START: DHCP started, wait before boosting wifi priority
* @WLAND_BT_DHCP_OPPR_WIN: graceful DHCP opportunity ended,boost wifi priority
* @WLAND_BT_DHCP_FLAG_FORCE_TIMEOUT: wifi priority boost end restore defaults
*/
enum wland_btcoex_state {
	WLAND_BT_DHCP_IDLE,
	WLAND_BT_DHCP_START,
	WLAND_BT_DHCP_OPPR_WIN,
	WLAND_BT_DHCP_FLAG_FORCE_TIMEOUT
};

/**
 * struct wland_btcoex_info - BT coex related information
 * @vif: interface for which request was done.
 * @timer: timer for DHCP state machine
 * @timeout: configured timeout.
 * @timer_on:  DHCP timer active
 * @dhcp_done: DHCP finished before T1/T2 timer expiration
 * @bt_state: DHCP state machine state
 * @work: DHCP state machine work
 * @cfg: driver private data for cfg80211 interface
 * @reg66: saved value of btc_params 66
 * @reg41: saved value of btc_params 41
 * @reg68: saved value of btc_params 68
 * @saved_regs_part1: flag indicating regs 66,41,68	have been saved
 * @reg51: saved value of btc_params 51
 * @reg64: saved value of btc_params 64
 * @reg65: saved value of btc_params 65
 * @reg71: saved value of btc_params 71
 * @saved_regs_part1: flag indicating regs 50,51,64,65,71 have been saved
 */
struct wland_btcoex_info {
	struct wland_cfg80211_vif *vif;
	struct timer_list timer;
	u16 timeout;
	bool timer_on;
	bool dhcp_done;
	enum wland_btcoex_state bt_state;
	struct work_struct work;
	struct wland_cfg80211_info *cfg;
	u32 reg66;
	u32 reg41;
	u32 reg68;
	bool saved_regs_part1;
	u32 reg50;
	u32 reg51;
	u32 reg64;
	u32 reg65;
	u32 reg71;
	bool saved_regs_part2;
};

/**
 * wland_btcoex_params_write() - write btc_params firmware variable
 * @ifp:  interface
 * @addr: btc_params register number
 * @data: data to write
 */
static s32 wland_btcoex_params_write(struct wland_if *ifp, u32 addr, u32 data)
{
	struct {
		__le32 addr;
		__le32 data;
	} reg_write;

	reg_write.addr = cpu_to_le32(addr);
	reg_write.data = cpu_to_le32(data);

	return wland_fil_iovar_data_set(ifp, "btc_params", &reg_write,
		sizeof(reg_write));
}

/**
 * wland_btcoex_params_read() - read btc_params firmware variable
 * @ifp:  interface
 * @addr: btc_params register number
 * @data: read data
 */
static s32 wland_btcoex_params_read(struct wland_if *ifp, u32 addr, u32 * data)
{
	*data = addr;

	return wland_fil_iovar_data_get(ifp, "btc_params", data);
}

/**
 * wland_btcoex_boost_wifi() - control BT SCO/eSCO parameters
 * @btci: BT coex info
 * @trump_sco:
 *	true  - set SCO/eSCO parameters for compatibility during DHCP window
 *	false - restore saved parameter values
 *
 * Enhanced BT COEX settings for eSCO compatibility during DHCP window
 */
static void wland_btcoex_boost_wifi(struct wland_btcoex_info *btci,
	bool trump_sco)
{
	struct wland_if *ifp = btci->cfg->pub->iflist[0];

	if (trump_sco && !btci->saved_regs_part2) {
		/*
		 * this should reduce eSCO agressive retransmit w/o breaking it
		 */

		/*
		 * save current
		 */
		WLAND_DBG(DEFAULT, TRACE,
			"new SCO/eSCO coex algo {save & override}\n");

		wland_btcoex_params_read(ifp, 50, &btci->reg50);
		wland_btcoex_params_read(ifp, 51, &btci->reg51);
		wland_btcoex_params_read(ifp, 64, &btci->reg64);
		wland_btcoex_params_read(ifp, 65, &btci->reg65);
		wland_btcoex_params_read(ifp, 71, &btci->reg71);

		btci->saved_regs_part2 = true;
		WLAND_DBG(DEFAULT, TRACE,
			"saved bt_params[50,51,64,65,71]: 0x%x 0x%x 0x%x 0x%x 0x%x\n",
			btci->reg50, btci->reg51, btci->reg64, btci->reg65,
			btci->reg71);

		/*
		 * pacify the eSco
		 */
		wland_btcoex_params_write(ifp, 50, BRCMF_BT_DHCP_REG50);
		wland_btcoex_params_write(ifp, 51, BRCMF_BT_DHCP_REG51);
		wland_btcoex_params_write(ifp, 64, BRCMF_BT_DHCP_REG64);
		wland_btcoex_params_write(ifp, 65, BRCMF_BT_DHCP_REG65);
		wland_btcoex_params_write(ifp, 71, BRCMF_BT_DHCP_REG71);
	} else if (btci->saved_regs_part2) {
		/*
		 * restore previously saved bt params
		 */
		WLAND_DBG(DEFAULT, TRACE,
			"Do new SCO/eSCO coex algo {restore}\n");
		wland_btcoex_params_write(ifp, 50, btci->reg50);
		wland_btcoex_params_write(ifp, 51, btci->reg51);
		wland_btcoex_params_write(ifp, 64, btci->reg64);
		wland_btcoex_params_write(ifp, 65, btci->reg65);
		wland_btcoex_params_write(ifp, 71, btci->reg71);

		WLAND_DBG(DEFAULT, TRACE,
			"restored bt_params[50,51,64,65,71]: 0x%x 0x%x 0x%x 0x%x 0x%x\n",
			btci->reg50, btci->reg51, btci->reg64, btci->reg65,
			btci->reg71);

		btci->saved_regs_part2 = false;
	} else {
		WLAND_ERR("attempted to restore not saved BTCOEX params\n");
	}
}

/**
 * wland_btcoex_is_sco_active() - check if SCO/eSCO is active
 * @ifp: interface
 *
 * return: true if SCO/eSCO session is active
 */
static bool wland_btcoex_is_sco_active(struct wland_if *ifp)
{
	bool res = false;
	int sco_id_cnt = 0, ioc_res = 0, i;
	u32 param27;

	for (i = 0; i < BRCMF_BT_SCO_SAMPLES; i++) {
		ioc_res = wland_btcoex_params_read(ifp, 27, &param27);

		if (ioc_res < 0) {
			WLAND_ERR("ioc read btc params error\n");
			break;
		}

		WLAND_DBG(DEFAULT, TRACE, "sample[%d], btc_params 27:%x\n", i,
			param27);

		if ((param27 & 0x6) == 2) {	/* count both sco & esco  */
			sco_id_cnt++;
		}

		if (sco_id_cnt > 2) {
			WLAND_DBG(DEFAULT, TRACE,
				"sco/esco detected, pkt id_cnt:%d samples:%d\n",
				sco_id_cnt, i);
			res = true;
			break;
		}
	}
	WLAND_DBG(DEFAULT, TRACE, "exit: result=%d\n", res);
	return res;
}

/*
 * wland_btcoex_save_part1() - save first step parameters.
 */
static void wland_btcoex_save_part1(struct wland_btcoex_info *btci)
{
	struct wland_if *ifp = btci->vif->ifp;

	if (!btci->saved_regs_part1) {
		/*
		 * Retrieve and save original reg value
		 */
		wland_btcoex_params_read(ifp, 66, &btci->reg66);
		wland_btcoex_params_read(ifp, 41, &btci->reg41);
		wland_btcoex_params_read(ifp, 68, &btci->reg68);
		btci->saved_regs_part1 = true;
		WLAND_DBG(DEFAULT, TRACE,
			"saved btc_params regs (66,41,68) 0x%x 0x%x 0x%x\n",
			btci->reg66, btci->reg41, btci->reg68);
	}
}

/*
 * wland_btcoex_restore_part1() - restore first step parameters.
 */
static void wland_btcoex_restore_part1(struct wland_btcoex_info *btci)
{
	struct wland_if *ifp;

	if (btci->saved_regs_part1) {
		btci->saved_regs_part1 = false;
		ifp = btci->vif->ifp;
		wland_btcoex_params_write(ifp, 66, btci->reg66);
		wland_btcoex_params_write(ifp, 41, btci->reg41);
		wland_btcoex_params_write(ifp, 68, btci->reg68);

		WLAND_DBG(DEFAULT, TRACE,
			"restored btc_params regs {66,41,68} 0x%x 0x%x 0x%x\n",
			btci->reg66, btci->reg41, btci->reg68);
	}
}

/**
 * wland_btcoex_timerfunc() - BT coex timer callback
 */
static void wland_btcoex_timerfunc(ulong data)
{
	struct wland_btcoex_info *bt_local;

	WLAND_DBG(DEFAULT, TRACE, "enter\n");

	bt_local = (struct wland_btcoex_info *) data;

	bt_local->timer_on = false;

	schedule_work(&bt_local->work);
}

/**
 * wland_btcoex_handler() - BT coex state machine work handler
 * @work: work
 */
static void wland_btcoex_handler(struct work_struct *work)
{
	struct wland_btcoex_info *btci =
		container_of(work, struct wland_btcoex_info, work);

	if (btci->timer_on) {
		btci->timer_on = false;
		if (timer_pending(&btci->timer)) {
			WLAND_DBG(DEFAULT, TRACE, "Del timer btci->timer\n");
			del_timer_sync(&btci->timer);
		}
	}

	switch (btci->bt_state) {
	case WLAND_BT_DHCP_START:
		/*
		 * DHCP started provide OPPORTUNITY window
		 * to get DHCP address
		 */
		WLAND_DBG(DEFAULT, TRACE, "DHCP started\n");
		btci->bt_state = WLAND_BT_DHCP_OPPR_WIN;
		if (btci->timeout < BRCMF_BTCOEX_OPPR_WIN_TIME) {
			mod_timer(&btci->timer, btci->timer.expires);
		} else {
			btci->timeout -= BRCMF_BTCOEX_OPPR_WIN_TIME;
			mod_timer(&btci->timer, jiffies +
				msecs_to_jiffies(BRCMF_BTCOEX_OPPR_WIN_TIME));
		}
		btci->timer_on = true;
		break;

	case WLAND_BT_DHCP_OPPR_WIN:
		if (btci->dhcp_done) {
			WLAND_DBG(DEFAULT, TRACE,
				"DHCP done before T1 expiration\n");
			goto idle;
		}

		/*
		 * DHCP is not over yet, start lowering BT priority
		 */
		WLAND_DBG(DEFAULT, TRACE, "DHCP T1:%d expired\n",
			BRCMF_BTCOEX_OPPR_WIN_TIME);
		wland_btcoex_boost_wifi(btci, true);

		btci->bt_state = WLAND_BT_DHCP_FLAG_FORCE_TIMEOUT;
		mod_timer(&btci->timer,
			jiffies + msecs_to_jiffies(btci->timeout));
		btci->timer_on = true;
		break;

	case WLAND_BT_DHCP_FLAG_FORCE_TIMEOUT:
		if (btci->dhcp_done)
			WLAND_DBG(DEFAULT, TRACE,
				"DHCP done before T2 expiration\n");
		else
			WLAND_DBG(DEFAULT, TRACE, "DHCP T2:%d expired\n",
				WLAND_BT_DHCP_FLAG_FORCE_TIMEOUT);

		goto idle;

	default:
		WLAND_ERR("invalid state=%d !!!\n", btci->bt_state);
		goto idle;
	}

	return;

idle:
	btci->bt_state = WLAND_BT_DHCP_IDLE;
	btci->timer_on = false;
	wland_btcoex_boost_wifi(btci, false);
	cfg80211_crit_proto_stopped(&btci->vif->wdev, GFP_KERNEL);
	wland_btcoex_restore_part1(btci);
	btci->vif = NULL;
}

/**
 * wland_btcoex_attach() - initialize BT coex data
 * @cfg: driver private cfg80211 data
 *
 * return: 0 on success
 */
int wland_btcoex_attach(struct wland_cfg80211_info *cfg)
{
	struct wland_btcoex_info *btci;

	WLAND_DBG(DEFAULT, TRACE, "enter\n");

	btci = kmalloc(sizeof(struct wland_btcoex_info), GFP_KERNEL);
	if (!btci)
		return -ENOMEM;

	btci->bt_state = WLAND_BT_DHCP_IDLE;

	/*
	 * Set up timer for BT
	 */
	btci->timer_on = false;
	btci->timeout = BRCMF_BTCOEX_OPPR_WIN_TIME;
	init_timer(&btci->timer);
	btci->timer.data = (ulong) btci;
	btci->timer.function = wland_btcoex_timerfunc;
	btci->cfg = cfg;
	btci->saved_regs_part1 = false;
	btci->saved_regs_part2 = false;

	INIT_WORK(&btci->work, wland_btcoex_handler);

	cfg->btcoex = btci;
	return 0;
}

/**
 * wland_btcoex_detach - clean BT coex data
 * @cfg: driver private cfg80211 data
 */
void wland_btcoex_detach(struct wland_cfg80211_info *cfg)
{
	WLAND_DBG(DEFAULT, TRACE, "enter\n");

	if (!cfg->btcoex)
		return;

	if (cfg->btcoex->timer_on) {
		cfg->btcoex->timer_on = false;
		if (timer_pending(&cfg->btcoex->timer)) {
			WLAND_DBG(DEFAULT, TRACE,
				"del timer cfg->btcoex->timer\n");
			del_timer_sync(&cfg->btcoex->timer);
		}
	}

	cancel_work_sync(&cfg->btcoex->work);

	wland_btcoex_boost_wifi(cfg->btcoex, false);
	wland_btcoex_restore_part1(cfg->btcoex);

	kfree(cfg->btcoex);
	cfg->btcoex = NULL;
}

static void wland_btcoex_dhcp_start(struct wland_btcoex_info *btci)
{
	struct wland_if *ifp = btci->vif->ifp;

	wland_btcoex_save_part1(btci);
	/*
	 * set new regs values
	 */
	wland_btcoex_params_write(ifp, 66, BRCMF_BT_DHCP_REG66);
	wland_btcoex_params_write(ifp, 41, BRCMF_BT_DHCP_REG41);
	wland_btcoex_params_write(ifp, 68, BRCMF_BT_DHCP_REG68);
	btci->dhcp_done = false;
	btci->bt_state = WLAND_BT_DHCP_START;
	schedule_work(&btci->work);

	WLAND_DBG(DEFAULT, TRACE, "enable BT DHCP Timer\n");
}

static void wland_btcoex_dhcp_end(struct wland_btcoex_info *btci)
{
	/*
	 * Stop any bt timer because DHCP session is done
	 */
	btci->dhcp_done = true;

	if (btci->timer_on) {
		WLAND_DBG(DEFAULT, TRACE, "disable BT DHCP Timer\n");
		btci->timer_on = false;
		if (timer_pending(&btci->timer)) {
			WLAND_DBG(DEFAULT, TRACE, "Del timer btci->timer\n");
			del_timer_sync(&btci->timer);
		}

		/*
		 * schedule worker if transition to IDLE is needed
		 */
		if (btci->bt_state != WLAND_BT_DHCP_IDLE) {
			WLAND_DBG(DEFAULT, TRACE, "bt_state:%d\n",
				btci->bt_state);
			schedule_work(&btci->work);
		}
	} else {
		/*
		 * Restore original values
		 */
		wland_btcoex_restore_part1(btci);
	}
}

/**
 * wland_btcoex_set_mode - set BT coex mode
 * @cfg: driver private cfg80211 data
 * @mode: Wifi-Bluetooth coexistence mode
 *
 * return: 0 on success
 */
int wland_btcoex_set_mode(struct wland_cfg80211_vif *vif,
	enum wland_btcoex_mode mode, u16 duration)
{
	struct wland_cfg80211_info *cfg = wiphy_priv(vif->wdev.wiphy);
	struct wland_btcoex_info *btci = cfg->btcoex;
	struct wland_if *ifp = cfg->pub->iflist[0];

	switch (mode) {
	case BTCOEX_DISABLED:
		WLAND_DBG(DEFAULT, TRACE, "DHCP session starts\n");
		if (btci->bt_state != WLAND_BT_DHCP_IDLE)
			return -EBUSY;
		/*
		 * Start BT timer only for SCO connection
		 */
		if (wland_btcoex_is_sco_active(ifp)) {
			btci->timeout = duration;
			btci->vif = vif;
			wland_btcoex_dhcp_start(btci);
		}
		break;

	case BTCOEX_ENABLED:
		WLAND_DBG(DEFAULT, TRACE, "DHCP session ends\n");
		if (btci->bt_state != WLAND_BT_DHCP_IDLE && vif == btci->vif) {
			wland_btcoex_dhcp_end(btci);
		}
		break;
	default:
		WLAND_DBG(DEFAULT, TRACE, "Unknown mode, ignored\n");
	}
	return 0;
}

#endif /* WLAND_BTCOEX_SUPPORT */
