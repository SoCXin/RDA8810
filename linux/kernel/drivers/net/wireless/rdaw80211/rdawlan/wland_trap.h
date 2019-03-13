
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
#ifndef _WLAND_TRAP_H_
#define _WLAND_TRAP_H_
#include <linux/kernel.h>

#define RDA_WLAND_FIRMWARE_NAME "rda_wland.bin"
#define RDA_FIRMWARE_VERSION 3
#define RDA_FIRMWARE_TYPE_SIZE 16	//firmware type size
#define RDA_FIRMWARE_DATA_NAME_SIZE 50	//firmware data_type size

#pragma pack (push)
#pragma pack(1)
struct rda_device_firmware_head {
	char firmware_type[RDA_FIRMWARE_TYPE_SIZE];
	u32 version;
	u32 data_num;
};
struct rda_firmware_data_type {
	char data_name[RDA_FIRMWARE_DATA_NAME_SIZE];
	u16 crc;
	s8 chip_version;
	u32 size;
};

#pragma pack (pop)

/*get data from firmware*/
//#define RDA_WLAND_FROM_FIRMWARE

#ifdef RDA_WLAND_FROM_FIRMWARE
#define rda_write_data_to_wland(CLIENT, FIRMWARE_DATA, DATA_NAME, BIT_SIZE) \
	 wland_write_sdio_from_firmware(CLIENT, FIRMWARE_DATA, #DATA_NAME,\
	 BIT_SIZE)
#else
#define rda_write_data_to_wland(CLIENT, FIRMWARE_DATA, ARRAY_DATA, BIT_SIZE) \
	wland_write_sdio_polling_from_array(CLIENT, (const u8(*)[])ARRAY_DATA,\
	ARRAY_SIZE(ARRAY_DATA), BIT_SIZE)
#endif

/* Flag sdio or patch init complete */
extern u8 check_sdio_init(void);
extern u8 check_sdio_patch(void);

extern int wland_sdio_trap_attach(struct wland_private *priv);
extern s32 wland_assoc_power_save(struct wland_private *priv);
extern s32 wland_set_phy_timeout(struct wland_private *priv, u32 cipher_pairwise);

#endif /*_WLAND_TRAP_H_ */
