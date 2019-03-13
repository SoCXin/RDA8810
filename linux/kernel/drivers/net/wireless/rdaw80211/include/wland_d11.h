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

#ifndef	_WLAND_D11_H_
#define	_WLAND_D11_H_

/* d11 io type */
#define WLAND_D11N_IOTYPE		        1
#define WLAND_D11AC_IOTYPE		        2


/* bit 0~7 channel number
 * for 80+80 channels: bit 0~3 low channel id, bit 4~7 high channel id
 */
#define WLAND_CHSPEC_CH_MASK		    0x00FF
#define WLAND_CHSPEC_CH_SHIFT		    0
#define WLAND_CHSPEC_CHL_MASK		    0x000F
#define WLAND_CHSPEC_CHL_SHIFT		    0
#define WLAND_CHSPEC_CHH_MASK		    0x00F0
#define WLAND_CHSPEC_CHH_SHIFT		    4

/* bit 8~16 for dot 11n IO types
 * bit 8~9 sideband
 * bit 10~11 bandwidth
 * bit 12~13 spectral band
 * bit 14~15 not used
 */
#define WLAND_CHSPEC_D11N_SB_MASK	    0x0300
#define WLAND_CHSPEC_D11N_SB_L		    0x0100	/* control lower */
#define WLAND_CHSPEC_D11N_SB_N		    0x0300	/* none */
#define WLAND_CHSPEC_D11N_BW_MASK	    0x0c00
#define WLAND_CHSPEC_D11N_BW_20	        0x0800
#define WLAND_CHSPEC_D11N_BW_40	        0x0c00
#define WLAND_CHSPEC_D11N_BND_MASK	    0x3000
#define WLAND_CHSPEC_D11N_BND_5G	    0x1000
#define WLAND_CHSPEC_D11N_BND_2G	    0x2000

/* bit 8~16 for dot 11ac IO types
 * bit 8~10 sideband
 * bit 11~13 bandwidth
 * bit 14~15 spectral band
 */
#define WLAND_CHSPEC_D11AC_SB_MASK	    0x0700
#define WLAND_CHSPEC_D11AC_SB_L	        0x0900
#define WLAND_CHSPEC_D11AC_SB_U	        0x0B00
#define WLAND_CHSPEC_D11AC_BW_MASK	    0x3800
#define WLAND_CHSPEC_D11AC_BW_20	    0x1000
#define WLAND_CHSPEC_D11AC_BW_40	    0x1800
#define WLAND_CHSPEC_D11AC_BW_80	    0x2000
#define WLAND_CHSPEC_D11AC_BW_160	    0x2800
#define WLAND_CHSPEC_D11AC_BW_8080	    0x3000
#define WLAND_CHSPEC_D11AC_BND_MASK	    0xC000
#define WLAND_CHSPEC_D11AC_BND_2G	    0x0000
#define WLAND_CHSPEC_D11AC_BND_5G	    0xc000

#define CHAN_BAND_2G		            0
#define CHAN_BAND_5G		            1

/* channel defines */
#define CH_10MHZ_APART			        2
#define CH_MIN_2G_CHANNEL		        1
#define CH_MAX_2G_CHANNEL		        14	/* Max channel in 2G band */
#define CH_MIN_5G_CHANNEL		        34

/*
 * max # supported channels. The max channel no is 216, this is that + 1
 * rounded up to a multiple of NBBY (8). DO NOT MAKE it > 255: channels are
 * u8's all over
*/
#define	MAXCHANNEL		                224

#define WL_CHANSPEC_CHAN_MASK		    0x00ff

#define WL_CHANSPEC_BAND_MASK	        0xf000
#define WL_CHANSPEC_BAND_SHIFT	        12
#define WL_CHANSPEC_BAND_5G		        0x1000
#define WL_CHANSPEC_BAND_2G		        0x2000
#define INVCHANSPEC			            255

#define WL_CHAN_VALID_HW		        (1 << 0) /* valid with current HW */
#define WL_CHAN_VALID_SW		        (1 << 1) /* valid with country sett. */
#define WL_CHAN_BAND_5G			        (1 << 2) /* 5GHz-band channel */
#define WL_CHAN_RADAR			        (1 << 3) /* radar sensitive  channel */
#define WL_CHAN_INACTIVE		        (1 << 4) /* inactive due to radar */
#define WL_CHAN_PASSIVE			        (1 << 5) /* channel in passive mode */
#define WL_CHAN_RESTRICTED		        (1 << 6) /* restricted use channel */

/* values for band specific 40MHz capabilities  */
#define WLC_N_BW_20ALL			        0
#define WLC_N_BW_40ALL			        1
#define WLC_N_BW_20IN2G_40IN5G		    2

/* band types */
#define	WLC_BAND_AUTO			        0	/* auto-select */
#define	WLC_BAND_5G			            1	/* 5 Ghz */
#define	WLC_BAND_2G			            2	/* 2.4 Ghz */
#define	WLC_BAND_ALL			        3	/* all bands */

/* defined rate in 500kbps */
#define WLAND_RATE_1M	                2	/* in 500kbps units */
#define WLAND_RATE_2M	                4	/* in 500kbps units */
#define WLAND_RATE_5M5	                11	/* in 500kbps units */
#define WLAND_RATE_11M	                22	/* in 500kbps units */
#define WLAND_RATE_6M	                12	/* in 500kbps units */
#define WLAND_RATE_9M	                18	/* in 500kbps units */
#define WLAND_RATE_12M	                24	/* in 500kbps units */
#define WLAND_RATE_18M	                36	/* in 500kbps units */
#define WLAND_RATE_24M	                48	/* in 500kbps units */
#define WLAND_RATE_36M	                72	/* in 500kbps units */
#define WLAND_RATE_48M	                96	/* in 500kbps units */
#define WLAND_RATE_54M	                108	/* in 500kbps units */


enum wland_chan_bw {
	CHAN_BW_20,
	CHAN_BW_40,
	CHAN_BW_80,
	CHAN_BW_80P80,
	CHAN_BW_160
};

enum wland_chan_sb {
	WLAND_CHAN_SB_NONE = 0,
	WLAND_CHAN_SB_L,
	WLAND_CHAN_SB_U
};

struct wland_chan {
	u16                 chspec;
	u8                  chnum;
	u8                  band;
	enum wland_chan_bw  bw;
	enum wland_chan_sb  sb;
};

struct wland_d11inf {
	u8                  io_type;

	void (*encchspec)(struct wland_chan *ch);
	void (*decchspec)(struct wland_chan *ch);
};

extern void wland_d11_attach(struct wland_d11inf *d11inf);

#endif	/* _WLAND_D11_H_ */
