
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
#include <linux/kernel.h>
#include <linux/if_arp.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/wireless.h>
#include <linux/ieee80211.h>
#include <linux/debugfs.h>
#include <net/cfg80211.h>

#include <wland_defs.h>
#include <wland_fweh.h>
#include <wland_dev.h>
#include <wland_bus.h>
#include <wland_dbg.h>
#include <wland_utils.h>
#include <wland_wid.h>
#include <wland_trap.h>
#include <wland_trap_90.h>
#include <wland_trap_91.h>
#include <wland_trap_91e.h>
#include <wland_trap_91f.h>
#include <wland_trap_91g.h>
#include "wland_sdmmc.h"
#include <linux/firmware.h>
#include <linux/crc16.h>

/* local flag for path and init finished */
static u8 sdio_patch_complete = 0;
static u8 sdio_init_complete = 0;

static s32 wland_set_core_init_patch(struct wland_private *priv,
	const u32(*data)[2], u8 num)
{
	struct wland_proto *prot = priv->prot;
	u8 *buf = prot->buf;
	s32 err = 0, count = 0;
	u16 wid_msg_len = FMW_HEADER_LEN;
	enum wland_firmw_wid wid;

	WLAND_DBG(TRAP, TRACE, "Enter\n");

	mutex_lock(&priv->proto_block);

	memset(prot->buf, '\0', sizeof(prot->buf));

	for (count = 0; count < num; count++) {
		/*
		 * wid body
		 */
		wid = WID_MEMORY_ADDRESS;
		buf[0] = (u8) (wid & 0x00FF);
		buf[1] = (u8) ((wid & 0xFF00) >> 8);
		buf[2] = 4;
		memcpy(buf + 3, (u8 *) (&data[count][0]), 4);
		/*
		 * offset
		 */
		wid_msg_len += 7;
		buf += 7;

		/*
		 * wid body
		 */
		wid = WID_MEMORY_ACCESS_32BIT;
		buf[0] = (u8) (wid & 0x00FF);
		buf[1] = (u8) ((wid & 0xFF00) >> 8);
		buf[2] = 4;
		memcpy(buf + 3, (u8 *) (&data[count][1]), 4);
		/*
		 * offset
		 */
		wid_msg_len += 7;
		buf += 7;
	}

	err = wland_proto_cdc_data(priv, wid_msg_len);

	if (err < 0)
		WLAND_ERR("WID Result Failed\n");

	mutex_unlock(&priv->proto_block);

	WLAND_DBG(TRAP, TRACE, "Done(err:%d)\n", err);
	return err;
}

static s32 wland_set_core_patch(struct wland_private *priv,
	const u8(*patch)[2], u8 num)
{
	struct wland_proto *prot = priv->prot;
	u8 *buf = prot->buf;
	s32 err = 0, count = 0;
	u16 wid_msg_len = FMW_HEADER_LEN;
	enum wland_firmw_wid wid;

	WLAND_DBG(TRAP, TRACE, "Enter\n");

	mutex_lock(&priv->proto_block);

	memset(prot->buf, '\0', sizeof(prot->buf));

	for (count = 0; count < num; count++) {
		/*
		 * wid body
		 */
		wid = WID_PHY_ACTIVE_REG;
		buf[0] = (u8) (wid & 0x00FF);
		buf[1] = (u8) ((wid & 0xFF00) >> 8);
		buf[2] = 1;
		buf[3] = (u8) patch[count][0];
		/*
		 * offset
		 */
		wid_msg_len += 4;
		buf += 4;

		/*
		 * wid body
		 */
		wid = WID_PHY_ACTIVE_REG_VAL;
		buf[0] = (u8) (wid & 0x00FF);
		buf[1] = (u8) ((wid & 0xFF00) >> 8);
		buf[2] = 1;
		buf[3] = (u8) patch[count][1];
		/*
		 * offset
		 */
		wid_msg_len += 4;
		buf += 4;
	}

	err = wland_proto_cdc_data(priv, wid_msg_len);

	if (err < 0){
		err = 0; //Ignore error
		WLAND_ERR("WID Result Failed\n");
	}

	mutex_unlock(&priv->proto_block);

	WLAND_DBG(TRAP, TRACE, "Done(ret:%d)\n", err);

	return err;
}

static s32 wland_write_sdio32_polling(struct wland_private *priv,
	const u32(*data)[2], u32 size)
{
	int count = size, index = 0;
	s32 ret = 0;

	WLAND_DBG(TRAP, TRACE, "Enter\n");

	//each time write five init data
	for (index = 0; index < count / 8; index++) {
		ret = wland_set_core_init_patch(priv,
			(const u32(*)[2]) &data[8 * index][0], 8);
		if (ret < 0)
			goto err;
		WLAND_DBG(TRAP, TRACE, "index:%d\n", index);
	}

	if ((count % 8) > 0) {
		ret = wland_set_core_init_patch(priv,
			(const u32(*)[2]) &data[8 * index][0], count % 8);
		if (ret < 0)
			goto err;
	}
err:
	WLAND_DBG(TRAP, TRACE, "Done(ret:%d)\n", ret);
	return ret;
}

static s32 wland_write_sdio8_polling(struct wland_private *priv,
	const u8(*data)[2], u32 size)
{
	int count = size, index = 0;
	s32 ret = 0;

	WLAND_DBG(TRAP, TRACE, "Enter\n");

	//each time write five init data
	for (index = 0; index < count / 8; index++) {
		WLAND_DBG(TRAP, TRACE, "index:%d\n", index);
		ret = wland_set_core_patch(priv,
			(const u8(*)[2]) data[8 * index], 8);
		if (ret < 0)
			goto err;
	}

	if (count % 8 > 0) {
		ret = wland_set_core_patch(priv,
			(const u8(*)[2]) data[8 * index], count % 8);
		if (ret < 0)
			goto err;
	}

err:
	WLAND_DBG(TRAP, TRACE, "Done(ret:%d)\n", ret);
	return ret;
}

#ifdef RDA_WLAND_FROM_FIRMWARE
static s32 check_firmware_data(const struct firmware *fw_entry)
{
	return crc16(0, fw_entry->data, fw_entry->size);
}

static s32 wland_write_sdio_from_firmware(struct wland_private *priv,
	const struct firmware *fw_entry, char *data_name, int bit_size)
{
	int ret = 0;
	struct rda_device_firmware_head *rda_wland_firmware_head;
	struct rda_firmware_data_type *rda_wland_firmware_data_type;
	const u8 *rda_wland_firmware_data;
	u32 num = 0;
	u32 size = sizeof(struct rda_device_firmware_head);

	WLAND_DBG(TRAP, TRACE, "Enter\n");
	rda_wland_firmware_head =
		(struct rda_device_firmware_head *) (fw_entry->data);
	if (strcmp(rda_wland_firmware_head->firmware_type,
			RDA_WLAND_FIRMWARE_NAME)
		!= 0) {
		WLAND_ERR("wland_write_sdio: firmware data error\n");
		ret = -1;
		goto err;
	}
	if (rda_wland_firmware_head->version != RDA_FIRMWARE_VERSION) {
		printk("firmware data version error. version %d is needed\n",
			RDA_FIRMWARE_VERSION);
		ret = -1;
		goto err;
	}

	rda_wland_firmware_data_type =
		(struct rda_firmware_data_type *) (rda_wland_firmware_head + 1);

	while (1) {
		rda_wland_firmware_data =
			(u8 *) (rda_wland_firmware_data_type + 1);

		num++;
		if (num > rda_wland_firmware_head->data_num) {
			WLAND_ERR("error: could not find data %s\n", data_name);
			ret = -1;
			goto err;
		}

		size += (sizeof(struct rda_firmware_data_type) +
			rda_wland_firmware_data_type->size);
		if (size > (fw_entry->size - 2)) {
			WLAND_ERR("error: could not find data %s\n", data_name);
			ret = -1;
			goto err;
		}

		if (strcmp(rda_wland_firmware_data_type->data_name,
				data_name) == 0)
			break;

		rda_wland_firmware_data_type = (struct rda_firmware_data_type *)
			(rda_wland_firmware_data +
			rda_wland_firmware_data_type->size);
	}

	if (crc16(0, rda_wland_firmware_data,
			rda_wland_firmware_data_type->size) !=
		rda_wland_firmware_data_type->crc) {
		printk("error: data %s crc error\n", data_name);
		ret = -1;
		goto err;
	}

	if (bit_size == 32)
		ret = wland_write_sdio32_polling(priv,
			(const u32(*)[2]) (rda_wland_firmware_data),
			(rda_wland_firmware_data_type->size) / 8);
	else if (bit_size == 8)
		ret = wland_write_sdio8_polling(priv,
			(const u8(*)[2]) (rda_wland_firmware_data),
			(rda_wland_firmware_data_type->size) / 2);
	else {
		WLAND_ERR("wland_write_sdio failed: bit_size error!\n");
		ret = -1;
		goto err;
	}

	WLAND_DBG(TRAP, TRACE, "Done(ret:%d)\n", ret);
err:
	return ret;
}

#else
static inline s32 wland_write_sdio_polling_from_array(struct wland_private
	*priv, const u8(*data)[2], int size, int bit_size)
{
	if (bit_size == 32)
		return wland_write_sdio32_polling(priv, (const u32(*)[]) data,
			size);
	else if (bit_size == 8)
		return wland_write_sdio8_polling(priv, data, size);
	else {
		WLAND_ERR("wland_write_sdio failed: bit_size error\n");
		return -1;
	}
}
#endif

static s32 wland_sdio_core_patch_attach(const struct firmware *fw_entry,
	struct wland_private *priv)
{
	s32 ret = 0;

	WLAND_DBG(TRAP, TRACE, "Enter\n");

	if (priv->bus_if->chip == WLAND_VER_90_D)
		ret = rda_write_data_to_wland(priv, fw_entry,
			wifi_core_patch_data_32_90_D, 32);
	else if (priv->bus_if->chip == WLAND_VER_90_E)
		ret = rda_write_data_to_wland(priv, fw_entry,
			wifi_core_patch_data_32_90_E, 32);
	else if (priv->bus_if->chip == WLAND_VER_91) {
		ret = rda_write_data_to_wland(priv, fw_entry,
			wifi_core_patch_data_32_91, 32);
		ret |= rda_write_data_to_wland(priv, fw_entry,
			wifi_clock_switch_91, 32);
	} else if (priv->bus_if->chip == WLAND_VER_91_E) {
		ret = rda_write_data_to_wland(priv, fw_entry,
			wifi_core_patch_data_32_91e, 32);
		ret |= rda_write_data_to_wland(priv, fw_entry,
			wifi_clock_switch_91e, 32);
	} else if (priv->bus_if->chip == WLAND_VER_91_F) {
		ret = rda_write_data_to_wland(priv, fw_entry,
			wifi_core_patch_data_32_91f, 32);
		ret |= rda_write_data_to_wland(priv, fw_entry,
			wifi_clock_switch_91f, 32);
	} else if (priv->bus_if->chip == WLAND_VER_91_G) {
		ret = rda_write_data_to_wland(priv, fw_entry,
			wifi_core_patch_data_32_91g, 32);
		ret |= rda_write_data_to_wland(priv, fw_entry,
			wifi_clock_switch_91g, 32);
	} else {
		WLAND_ERR("sdio_patch_core_32 failed,unknow chipid:0x%x",
			priv->bus_if->chip);
	}

	WLAND_DBG(TRAP, TRACE, "Done(ret:%d)\n", ret);

	return ret;

}

static s32 wland_sdio_additional_patch_attach(const struct firmware *fw_entry,
	struct wland_private *priv)
{
	s32 ret = 0;

	WLAND_DBG(TRAP, TRACE, "Enter\n");

	if (priv->bus_if->chip == WLAND_VER_90_D) {
		if (!check_test_mode()) {	//Normal mode
			WLAND_DBG(TRAP, TRACE,
				"Writing patch: wifi_core_init_data_32_90_D ...\n");
			ret = rda_write_data_to_wland(priv, fw_entry,
				wifi_core_init_data_32_90_D, 32);
			if (ret < 0) {
				WLAND_ERR
					("Write wifi_core_init_data_32_90_D failed!");
				goto err;
			}

			WLAND_DBG(TRAP, TRACE,
				"Writing patch: wifi_core_patch_data_90_8 ...\n");
			ret = rda_write_data_to_wland(priv, fw_entry,
				wifi_core_patch_data_90_8, 8);
			if (ret < 0) {
				WLAND_ERR
					("Write wifi_core_patch_data_90_8 failed!");
				goto err;
			}

			WLAND_DBG(TRAP, TRACE,
				"Writing patch: wifi_core_data_wake ...\n");
			ret = rda_write_data_to_wland(priv, fw_entry,
				wifi_core_data_wake, 32);
			if (ret < 0) {
				WLAND_ERR("Write wifi_core_data_wake failed!");
				goto err;
			}

			WLAND_DBG(TRAP, TRACE,
				"Writing patch: wifi_notch_data_90_D ...\n");
			ret = rda_write_data_to_wland(priv, fw_entry,
				wifi_notch_data_90_D, 32);
			if (ret < 0) {
				WLAND_ERR("Write wifi_notch_data_90_D failed!");
				goto err;
			}
		} else {	//Test mode
			WLAND_DBG(TRAP, TRACE,
				"Writing patch: wifi_core_patch_data_90_8 ...\n");
			ret = rda_write_data_to_wland(priv, fw_entry,
				wifi_core_patch_data_90_8, 8);
			if (ret < 0) {
				WLAND_ERR
					("Write wifi_core_patch_data_90_8 failed!");
				goto err;
			}

			WLAND_DBG(TRAP, TRACE,
				"Writing patch: wlan_test_mode_digital32_90 ...\n");
			ret = rda_write_data_to_wland(priv, fw_entry,
				wlan_test_mode_digital32_90, 32);
			if (ret < 0) {
				WLAND_ERR
					("Write wlan_test_mode_digital32_90 failed!");
				goto err;
			}

			WLAND_DBG(TRAP, TRACE,
				"Writing patch: wifi_test_mode_agc_patch32_90 ...\n");
			ret = rda_write_data_to_wland(priv, fw_entry,
				wifi_test_mode_agc_patch32_90, 32);
			if (ret < 0) {
				WLAND_ERR
					("Write wifi_test_mode_agc_patch32_90 failed!");
				goto err;
			}

			WLAND_DBG(TRAP, TRACE,
				"Writing patch: wifi_test_mode_rx_notch_32_90 ...\n");
			ret = rda_write_data_to_wland(priv, fw_entry,
				wifi_test_mode_rx_notch_32_90, 32);
			if (ret < 0) {
				WLAND_ERR
					("Write wifi_test_mode_rx_notch_32_90 failed!");
				goto err;
			}

			WLAND_DBG(TRAP, TRACE,
				"Writing patch: wifi_notch_data_90_D ...\n");
			ret = rda_write_data_to_wland(priv, fw_entry,
				wifi_notch_data_90_D, 32);
			if (ret < 0) {
				WLAND_ERR("Write wifi_notch_data_90_D failed!");
				goto err;
			}
		}

	} else if (priv->bus_if->chip == WLAND_VER_90_E) {
		if (!check_test_mode()) {	//Normal mode
			WLAND_DBG(TRAP, TRACE,
				"Writing patch: wifi_core_init_data_32_90_E ...\n");
			ret = rda_write_data_to_wland(priv, fw_entry,
				wifi_core_init_data_32_90_E, 32);
			if (ret < 0) {
				WLAND_ERR
					("Write wifi_core_init_data_32_90_E failed!");
				goto err;
			}

			WLAND_DBG(TRAP, TRACE,
				"Writing patch: wifi_core_patch_data_90_8 ...\n");
			ret = rda_write_data_to_wland(priv, fw_entry,
				wifi_core_patch_data_90_8, 8);
			if (ret < 0) {
				WLAND_ERR
					("Write wifi_core_patch_data_90_8 failed!");
				goto err;
			}

			WLAND_DBG(TRAP, TRACE,
				"Writing patch: wifi_core_data_wake ...\n");
			ret = rda_write_data_to_wland(priv, fw_entry,
				wifi_core_data_wake, 32);
			if (ret < 0) {
				WLAND_ERR("Write wifi_core_data_wake failed!");
				goto err;
			}

			WLAND_DBG(TRAP, TRACE,
				"Writing patch: wifi_notch_data_90_E ...\n");
			ret = rda_write_data_to_wland(priv, fw_entry,
				wifi_notch_data_90_E, 32);
			if (ret < 0) {
				WLAND_ERR("Write wifi_notch_data_90_E failed!");
				goto err;
			}
		} else {	//Test mode
			WLAND_DBG(TRAP, TRACE,
				"Writing patch: wifi_core_patch_data_90_8 ...\n");
			ret = rda_write_data_to_wland(priv, fw_entry,
				wifi_core_patch_data_90_8, 8);
			if (ret < 0) {
				WLAND_ERR
					("Write wifi_core_patch_data_90_8 failed!");
				goto err;
			}

			WLAND_DBG(TRAP, TRACE,
				"Writing patch: wlan_test_mode_digital32_90 ...\n");
			ret = rda_write_data_to_wland(priv, fw_entry,
				wlan_test_mode_digital32_90, 32);
			if (ret < 0) {
				WLAND_ERR
					("Write wlan_test_mode_digital32_90 failed!");
				goto err;
			}

			WLAND_DBG(TRAP, TRACE,
				"Writing patch: wifi_test_mode_agc_patch32_90 ...\n");
			ret = rda_write_data_to_wland(priv, fw_entry,
				wifi_test_mode_agc_patch32_90, 32);
			if (ret < 0) {
				WLAND_ERR
					("Write wifi_test_mode_agc_patch32_90 failed!");
				goto err;
			}

			WLAND_DBG(TRAP, TRACE,
				"Writing patch: wifi_test_mode_rx_notch_32_90 ...\n");
			ret = rda_write_data_to_wland(priv, fw_entry,
				wifi_test_mode_rx_notch_32_90, 32);
			if (ret < 0) {
				WLAND_ERR
					("Write wifi_test_mode_rx_notch_32_90 failed!");
				goto err;
			}

			WLAND_DBG(TRAP, TRACE,
				"Writing patch: wifi_notch_data_90_E ...\n");
			ret = rda_write_data_to_wland(priv, fw_entry,
				wifi_notch_data_90_E, 32);
			if (ret < 0) {
				WLAND_ERR("Write wifi_notch_data_90_E failed!");
				goto err;
			}
		}
	} else if (priv->bus_if->chip == WLAND_VER_91) {
		WLAND_DBG(TRAP, TRACE,
			"Writing patch: wifi_core_init_data_32_91 ...\n");
		ret = rda_write_data_to_wland(priv, fw_entry,
			wifi_core_init_data_32_91, 32);
		if (ret < 0) {
			WLAND_ERR("Write wifi_core_init_data_32_91 failed!");
			goto err;
		}

		WLAND_DBG(TRAP, TRACE,
			"Writing patch: wifi_core_AM_PM_data_32_91 ...\n");
		ret = rda_write_data_to_wland(priv, fw_entry,
			wifi_core_AM_PM_data_32_91, 32);
		if (ret < 0) {
			WLAND_ERR("Write wifi_core_AM_PM_data_32_91 failed!");
			goto err;
		}

		WLAND_DBG(TRAP, TRACE,
			"Writing patch: wifi_core_patch_data_91_8 ...\n");
		ret = rda_write_data_to_wland(priv, fw_entry,
			wifi_core_patch_data_91_8, 8);
		if (ret < 0) {
			WLAND_ERR("Write wifi_core_patch_data_91_8 failed!");
			goto err;
		}
	} else if (priv->bus_if->chip == WLAND_VER_91_E) {
		WLAND_DBG(TRAP, TRACE,
			"Writing patch: wifi_core_init_data_32_91e ...\n");
		ret = rda_write_data_to_wland(priv, fw_entry,
			wifi_core_init_data_32_91e, 32);
		if (ret < 0) {
			WLAND_ERR("Write wifi_core_init_data_32_91e failed!");
			goto err;
		}

		WLAND_DBG(TRAP, TRACE,
			"Writing patch: wifi_core_AM_PM_data_32_91e ...\n");
		ret = rda_write_data_to_wland(priv, fw_entry,
			wifi_core_AM_PM_data_32_91e, 32);
		if (ret < 0) {
			WLAND_ERR("Write wifi_core_AM_PM_data_32_91e failed!");
			goto err;
		}

		WLAND_DBG(TRAP, TRACE,
			"Writing patch: wifi_core_patch_data_91e_8 ...\n");
		ret = rda_write_data_to_wland(priv, fw_entry,
			wifi_core_patch_data_91e_8, 8);
		if (ret < 0) {
			WLAND_ERR("Write wifi_core_patch_data_91e_8 failed!");
			goto err;
		}
	} else if (priv->bus_if->chip == WLAND_VER_91_F) {
		WLAND_DBG(TRAP, TRACE,
			"Writing patch: wifi_core_init_data_32_91f ...\n");
		ret = rda_write_data_to_wland(priv, fw_entry,
			wifi_core_init_data_32_91f, 32);
		if (ret < 0) {
			WLAND_ERR("Write wifi_core_init_data_32_91f failed!");
			goto err;
		}

		WLAND_DBG(TRAP, TRACE,
			"Writing patch: wifi_core_AM_PM_data_32_91f ...\n");
		ret = rda_write_data_to_wland(priv, fw_entry,
			wifi_core_AM_PM_data_32_91f, 32);
		if (ret < 0) {
			WLAND_ERR("Write wifi_core_AM_PM_data_32_91f failed!");
			goto err;
		}

		WLAND_DBG(TRAP, TRACE,
			"Writing patch: wifi_core_patch_data_91f_8 ...\n");
		ret = rda_write_data_to_wland(priv, fw_entry,
			wifi_core_patch_data_91f_8, 8);
		if (ret < 0) {
			WLAND_ERR("Write wifi_core_patch_data_91f_8 failed!");
			goto err;
		}
	} else if (priv->bus_if->chip == WLAND_VER_91_G) {
		WLAND_DBG(TRAP, TRACE,
			"Writing patch: wifi_core_init_data_32_91g ...\n");
		ret = rda_write_data_to_wland(priv, fw_entry,
			wifi_core_init_data_32_91g, 32);
		if (ret < 0) {
			WLAND_ERR("Write wifi_core_init_data_32_91g failed!");
			goto err;
		}

		WLAND_DBG(TRAP, TRACE,
			"Writing patch: wifi_core_AM_PM_data_32_91g ...\n");
		ret = rda_write_data_to_wland(priv, fw_entry,
			wifi_core_AM_PM_data_32_91g, 32);
		if (ret < 0) {
			WLAND_ERR("Write wifi_core_AM_PM_data_32_91g failed!");
			goto err;
		}

		WLAND_DBG(TRAP, TRACE,
			"Writing patch: wifi_core_patch_data_91g_8 ...\n");
		ret = rda_write_data_to_wland(priv, fw_entry,
			wifi_core_patch_data_91g_8, 8);
		if (ret < 0) {
			WLAND_ERR("Write wifi_core_patch_data_91g_8 failed!");
			goto err;
		}
	} else {
		WLAND_ERR
			("wland_sdio_additional_patch_attach failed, unknow chipid:0x%x",
			priv->bus_if->chip);
	}

err:
	WLAND_DBG(TRAP, TRACE, "Done(ret:%d)\n", ret);
	return ret;
}

s32 wland_sdio_trap_attach(struct wland_private * priv)
{
	s32 ret = 0;
	const struct firmware *fw_entry = NULL;

	sdio_init_complete = 0;
	sdio_patch_complete = 0;

	if (priv->dev_mode) {
		WLAND_ERR("*** Softap Mode ***\n");
		return ret;
	}
#ifdef RDA_WLAND_FROM_FIRMWARE
	WLAND_DBG(TRAP, TRACE, "wland_sdio_trap_attach: Request firmware\n");
	ret = request_firmware(&fw_entry, RDA_WLAND_FIRMWARE_NAME,
		(priv->bus_if->bus_priv.sdio)->dev);
	if (ret) {
		WLAND_ERR("Request firmware: request firmware failed\n");
		return ret;
	}
	if (check_firmware_data(fw_entry)) {
		WLAND_ERR("firmware data crc check error\n");
		ret = -1;
		goto err;
	}
#endif
	WLAND_DBG(TRAP, DEBUG,
		"Write core patch: wland_sdio_core_patch_attach\n");

	ret = wland_sdio_core_patch_attach(fw_entry, priv);
	if (ret < 0) {
		WLAND_ERR("wland_sdio_core_patch_attach failed!\n");
		goto err;
	}

	sdio_patch_complete = 1;

	WLAND_DBG(TRAP, DEBUG,
		"Write additional patch: wland_sdio_additional_patch_attach.\n");
	ret = wland_sdio_additional_patch_attach(fw_entry, priv);
	if (ret < 0) {
		WLAND_ERR("wland_sdio_additional_patch_attach failed!\n");
		goto err;
	}
	WLAND_DBG(TRAP, DEBUG, "Write additional patch finshed.\n");
err:
#ifdef RDA_WLAND_FROM_FIRMWARE
	release_firmware(fw_entry);
#endif
	sdio_init_complete = 1;
	return ret;
}

s32 wland_assoc_power_save(struct wland_private * priv)
{
	int ret = 0;

	WLAND_DBG(TRAP, TRACE, "Enter\n");

	if (priv->bus_if->chip == WLAND_VER_91) {
		ret = wland_set_core_init_patch(priv,
			wifi_assoc_power_save_data_32_91,
			ARRAY_SIZE(wifi_assoc_power_save_data_32_91));
	}
	WLAND_DBG(TRAP, TRACE, "Done(ret:%d)\n", ret);

	return ret;
}

s32 wland_set_phy_timeout(struct wland_private * priv, u32 cipher_pairwise)
{
	int ret = 0;

	WLAND_DBG(TRAP, TRACE, "Enter\n");
	if (priv->bus_if->chip == WLAND_VER_90_D ||
		priv->bus_if->chip == WLAND_VER_90_E) {
		ret = wland_write_sdio32_polling(priv, wifi_phy_timeout_cfg_90,
			ARRAY_SIZE(wifi_phy_timeout_cfg_90));
	} else if (priv->bus_if->chip == WLAND_VER_91 ||
		priv->bus_if->chip == WLAND_VER_91_E ||
		priv->bus_if->chip == WLAND_VER_91_F) {
		ret = wland_write_sdio32_polling(priv, wifi_phy_timeout_cfg_91e,
			ARRAY_SIZE(wifi_phy_timeout_cfg_91e));
	} else if (priv->bus_if->chip == WLAND_VER_91_G) {
		if(cipher_pairwise == WLAN_CIPHER_SUITE_TKIP ||
			cipher_pairwise == WLAN_CIPHER_SUITE_WEP40 ||
			cipher_pairwise == WLAN_CIPHER_SUITE_WEP104) {
			ret = wland_write_sdio32_polling(priv, wifi_phy_timeout_cfg_91g_tkip,
			ARRAY_SIZE(wifi_phy_timeout_cfg_91g_tkip));
		}
	}
	WLAND_DBG(TRAP, TRACE, "Done(ret:%d)\n", ret);

	return ret;
}

u8 check_sdio_init(void)
{
	return sdio_init_complete;
}

u8 check_sdio_patch(void)
{
	return sdio_patch_complete;
}
