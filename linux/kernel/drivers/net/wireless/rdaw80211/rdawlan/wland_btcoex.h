
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
#ifndef _WLAND_BTCOEX_H_
#define _WLAND_BTCOEX_H_

#ifdef WLAND_BTCOEX_SUPPORT

enum wland_btcoex_mode {
	BTCOEX_DISABLED,
	BTCOEX_ENABLED
};

int wland_btcoex_attach(struct wland_cfg80211_info *cfg);
void wland_btcoex_detach(struct wland_cfg80211_info *cfg);
int wland_btcoex_set_mode(struct wland_cfg80211_vif *vif,
	enum wland_btcoex_mode mode, u16 duration);
#endif /* WLAND_BTCOEX_SUPPORT */
#endif /* WLAND_BTCOEX_H_ */
