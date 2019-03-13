
/*
 * aud_ifc.c  --  audiodma is the dma only for audio.
 *
 * Copyright (C) 2012 RDA Microelectronics (Beijing) Co., Ltd.
 *
 * Contact: Xu Mingliang <mingliangxu@rdamicro.com>
 *          
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA
 *
 */

#ifndef _RDA_AUDIFC_H
#define _RDA_AUDIFC_H

#include <linux/bitops.h>

// =============================================================================
//  MACROS
// =============================================================================

#define IFC_INUSE 1
#define IFC_NOUSE 0

typedef enum {
	RDA_AUDIFC_QUARTER_IRQ = 0,
	RDA_AUDIFC_HALF_IRQ = 1,
	RDA_AUDIFC_THREE_QUARTER_IRQ = 2,
	RDA_AUDIFC_END_IRQ = 3,

	RDA_AUDIFC_QTY_IRQ
} RDA_AUDIFC_HANDLE_ID_T;

typedef enum {
	RDA_AUDIFC_RECORD = 0,
	RDA_AUDIFC_PLAY = 1,

	RDA_AUDIFC_QTY
} RDA_AUDIFC_REQUEST_ID_T;

typedef enum {
	RDA_AUDIFC_NOERR = 0,
	RDA_AUDIFC_ERROR = 1,

} RDA_AUDIFC_ERROR_T;

typedef unsigned int audifc_addr_t;

struct rda_audifc_chan_params {
	audifc_addr_t src_addr;
	audifc_addr_t dst_addr;
	unsigned long xfer_size;
	unsigned long audifc_mode;
};

int rda_set_audifc_params(u8 ch, struct rda_audifc_chan_params *params);
audifc_addr_t rda_get_audifc_src_pos(u8 ch);

audifc_addr_t rda_get_audifc_dst_pos(u8 ch);

void rda_start_audifc(u8 ch);

void rda_stop_audifc(u8 ch);

void rda_poll_audifc(u8 ch);
;
int rda_request_audifc(int dev_id, const char *dev_name,
		       void (*callback) (int ch, int state, void *data),
		       void *data, int *audifc_ch_out);

void rda_free_audifc(u8 ch);

#endif /* _RDA_AUDIFC_H */
