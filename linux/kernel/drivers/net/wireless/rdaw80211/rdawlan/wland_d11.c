
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
#include <wland_sdmmc.h>
#include <wland_trap.h>
#include <wland_p2p.h>
#include <wland_cfg80211.h>
#include <wland_d11.h>

static void wland_d11n_encchspec(struct wland_chan *ch)
{
	ch->chspec = ch->chnum & WLAND_CHSPEC_CH_MASK;

	switch (ch->bw) {
	case CHAN_BW_20:
		ch->chspec |= WLAND_CHSPEC_D11N_BW_20 | WLAND_CHSPEC_D11N_SB_N;
		break;
	case CHAN_BW_40:
	default:
		WLAND_ERR("Invalid,ch->bw:%d\n", ch->bw);
		break;
	}

	if (ch->chnum <= CH_MAX_2G_CHANNEL)
		ch->chspec |= WLAND_CHSPEC_D11N_BND_2G;
	else
		ch->chspec |= WLAND_CHSPEC_D11N_BND_5G;
}

static void wland_d11ac_encchspec(struct wland_chan *ch)
{
	ch->chspec = ch->chnum & WLAND_CHSPEC_CH_MASK;

	switch (ch->bw) {
	case CHAN_BW_20:
		ch->chspec |= WLAND_CHSPEC_D11AC_BW_20;
		break;
	case CHAN_BW_40:
	case CHAN_BW_80:
	case CHAN_BW_80P80:
	case CHAN_BW_160:
	default:
		WLAND_ERR("Invalid,ch->bw:%d\n", ch->bw);
		break;
	}

	if (ch->chnum <= CH_MAX_2G_CHANNEL)
		ch->chspec |= WLAND_CHSPEC_D11AC_BND_2G;
	else
		ch->chspec |= WLAND_CHSPEC_D11AC_BND_5G;
}

static void wland_d11n_decchspec(struct wland_chan *ch)
{
	u16 val;

	ch->chnum = (u8) (ch->chspec & WLAND_CHSPEC_CH_MASK);

	switch (ch->chspec & WLAND_CHSPEC_D11N_BW_MASK) {
	case WLAND_CHSPEC_D11N_BW_20:
		ch->bw = CHAN_BW_20;
		break;
	case WLAND_CHSPEC_D11N_BW_40:
		ch->bw = CHAN_BW_40;
		val = ch->chspec & WLAND_CHSPEC_D11N_SB_MASK;
		if (val == WLAND_CHSPEC_D11N_SB_L) {
			ch->sb = WLAND_CHAN_SB_L;
			ch->chnum -= CH_10MHZ_APART;
		} else {
			ch->sb = WLAND_CHAN_SB_U;
			ch->chnum += CH_10MHZ_APART;
		}
		break;
	default:
		WLAND_ERR("Invalid BW,ch->chspec :%d\n", ch->chspec);
		break;
	}

	switch (ch->chspec & WLAND_CHSPEC_D11N_BND_MASK) {
	case WLAND_CHSPEC_D11N_BND_5G:
		ch->band = CHAN_BAND_5G;
		break;
	case WLAND_CHSPEC_D11N_BND_2G:
		ch->band = CHAN_BAND_2G;
		break;
	default:
		WLAND_ERR("Invalid BND,ch->chspec :%d\n", ch->chspec);
		break;
	}
}

static void wland_d11ac_decchspec(struct wland_chan *ch)
{
	u16 val;

	ch->chnum = (u8) (ch->chspec & WLAND_CHSPEC_CH_MASK);

	switch (ch->chspec & WLAND_CHSPEC_D11AC_BW_MASK) {
	case WLAND_CHSPEC_D11AC_BW_20:
		ch->bw = CHAN_BW_20;
		break;
	case WLAND_CHSPEC_D11AC_BW_40:
		ch->bw = CHAN_BW_40;
		val = ch->chspec & WLAND_CHSPEC_D11AC_SB_MASK;
		if (val == WLAND_CHSPEC_D11AC_SB_L) {
			ch->sb = WLAND_CHAN_SB_L;
			ch->chnum -= CH_10MHZ_APART;
		} else if (val == WLAND_CHSPEC_D11AC_SB_U) {
			ch->sb = WLAND_CHAN_SB_U;
			ch->chnum += CH_10MHZ_APART;
		} else {
			WLAND_ERR("Invalid,val:%d\n", val);
		}
		break;
	case WLAND_CHSPEC_D11AC_BW_80:
		ch->bw = CHAN_BW_80;
		break;
	case WLAND_CHSPEC_D11AC_BW_8080:
	case WLAND_CHSPEC_D11AC_BW_160:
	default:
		WLAND_ERR("Invalid BW,ch->chspec :%d\n", ch->chspec);
		break;
	}

	switch (ch->chspec & WLAND_CHSPEC_D11AC_BND_MASK) {
	case WLAND_CHSPEC_D11AC_BND_5G:
		ch->band = CHAN_BAND_5G;
		break;
	case WLAND_CHSPEC_D11AC_BND_2G:
		ch->band = CHAN_BAND_2G;
		break;
	default:
		WLAND_ERR("Invalid BND,ch->chspec :%d\n", ch->chspec);
		break;
	}
}

void wland_d11_attach(struct wland_d11inf *d11inf)
{
	if (d11inf->io_type == WLAND_D11N_IOTYPE) {
		d11inf->encchspec = wland_d11n_encchspec;
		d11inf->decchspec = wland_d11n_decchspec;
	} else {
		d11inf->encchspec = wland_d11ac_encchspec;
		d11inf->decchspec = wland_d11ac_decchspec;
	}

	WLAND_DBG(DEFAULT, TRACE, "d11inf->encchspec:%p,d11inf->decchspec:%p\n",
		d11inf->encchspec, d11inf->decchspec);
}
