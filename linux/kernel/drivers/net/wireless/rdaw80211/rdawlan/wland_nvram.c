
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
#include <linux/fs.h>
#include <asm/uaccess.h>

#include <linux_osl.h>

#include <wland_defs.h>
#include <wland_dbg.h>

#ifdef USE_MAC_FROM_RDA_NVRAM
#include <plat/md_sys.h>
#endif

#define WIFI_NVRAM_FILE_NAME    "/data/misc/wifi/WLANMAC"

static int nvram_read(char *filename, char *buf, ssize_t len, int offset)
{
	struct file *fd;
	int retLen = -1;

	mm_segment_t old_fs = get_fs();

	set_fs(KERNEL_DS);
	fd = osl_open_image(filename, O_WRONLY | O_CREAT, 0644);

	if (IS_ERR(fd)) {
		WLAND_ERR("[nvram_read] : failed to open!\n");
		return -1;
	}

	do {
		if ((fd->f_op == NULL) || (fd->f_op->read == NULL)) {
			WLAND_ERR("[wlan][nvram_read] : file can not be read!!\n");
			break;
		}

		if (fd->f_pos != offset) {
			if (fd->f_op->llseek) {
				if (fd->f_op->llseek(fd, offset, 0) != offset) {
					WLAND_ERR("[wlan][nvram_read] : failed to seek!!\n");
					break;
				}
			} else {
				fd->f_pos = offset;
			}
		}
		retLen = fd->f_op->read(fd, buf, len, &fd->f_pos);
	} while (false);

	osl_close_image(fd);
	set_fs(old_fs);

	return retLen;
}

static int nvram_write(char *filename, char *buf, ssize_t len, int offset)
{
	struct file *fd;
	int retLen = -1;

	mm_segment_t old_fs = get_fs();

	set_fs(KERNEL_DS);
	fd = osl_open_image(filename, O_WRONLY | O_CREAT, 0644);

	if (IS_ERR(fd)) {
		WLAND_ERR("[nvram_write] : failed to open!\n");
		return -1;
	}

	do {
		if ((fd->f_op == NULL) || (fd->f_op->write == NULL)) {
			WLAND_ERR("[nvram_write] : file can not be write!\n");
			break;
		}

		if (fd->f_pos != offset) {
			if (fd->f_op->llseek) {
				if (fd->f_op->llseek(fd, offset, 0) != offset) {
					WLAND_ERR("[nvram_write] : failed to seek!\n");
					break;
				}
			} else {
				fd->f_pos = offset;
			}
		}

		retLen = fd->f_op->write(fd, buf, len, &fd->f_pos);

	} while (false);

	osl_close_image(fd);
	set_fs(old_fs);

	return retLen;
}

int wland_get_mac_address(char *buf)
{
	return nvram_read(WIFI_NVRAM_FILE_NAME, buf, 6, 0);
}

int wland_set_mac_address(char *buf)
{
	return nvram_write(WIFI_NVRAM_FILE_NAME, buf, 6, 0);
}

#ifdef USE_MAC_FROM_RDA_NVRAM
int wlan_read_mac_from_nvram(char *buf)
{
	int ret;
	struct msys_device *wlan_msys = NULL;
	struct wlan_mac_info wlan_info;
	struct client_cmd cmd_set;
	int retry = 3;

	wlan_msys = rda_msys_alloc_device();
	if (!wlan_msys) {
		WLAND_ERR("nvram: can not allocate wlan_msys device\n");
		ret = -ENOMEM;
		goto err_handle_sys;
	}

	wlan_msys->module = SYS_GEN_MOD;
	wlan_msys->name = "rda-wlan";
	rda_msys_register_device(wlan_msys);

	//memset(&wlan_info, sizeof(wlan_info), 0);
	memset(&wlan_info, 0, sizeof(wlan_info));
	cmd_set.pmsys_dev = wlan_msys;
	cmd_set.mod_id = SYS_GEN_MOD;
	cmd_set.mesg_id = SYS_GEN_CMD_GET_WIFI_INFO;
	cmd_set.pdata = NULL;
	cmd_set.data_size = 0;
	cmd_set.pout_data = &wlan_info;
	cmd_set.out_size = sizeof(wlan_info);

	while (retry--) {
		ret = rda_msys_send_cmd(&cmd_set);
		if (ret) {
			WLAND_ERR("nvram:can not get wifi mac from nvram \n");
			ret = -EBUSY;
		} else {
			break;
		}
	}

	if (ret == -EBUSY) {
		goto err_handle_cmd;
	}

	if (wlan_info.activated != WIFI_MAC_ACTIVATED_FLAG) {
		WLAND_ERR("nvram:get invalid wifi mac address from nvram\n");
		ret = -EINVAL;
		goto err_invalid_mac;
	}

	memcpy(buf, wlan_info.mac_addr, ETH_ALEN);
	WLAND_DBG(DEFAULT, ERROR,
		"nvram: get wifi mac address [%02x:%02x:%02x:%02x:%02x:%02x].\n",
		buf[0], buf[1], buf[2], buf[3], buf[4], buf[5]);
	ret = 0;		/* success */

err_invalid_mac:
err_handle_cmd:
	rda_msys_unregister_device(wlan_msys);
	rda_msys_free_device(wlan_msys);
err_handle_sys:
	return ret;
}

int wlan_write_mac_to_nvram(const char *buf)
{
	int ret;
	struct msys_device *wlan_msys = NULL;
	struct wlan_mac_info wlan_info;
	struct client_cmd cmd_set;

	wlan_msys = rda_msys_alloc_device();
	if (!wlan_msys) {
		WLAND_ERR("nvram: can not allocate wlan_msys device\n");
		ret = -ENOMEM;
		goto err_handle_sys;
	}

	wlan_msys->module = SYS_GEN_MOD;
	wlan_msys->name = "rda-wlan";
	rda_msys_register_device(wlan_msys);

	memset(&wlan_info, 0, sizeof(wlan_info));
	wlan_info.activated = WIFI_MAC_ACTIVATED_FLAG;
	memcpy(wlan_info.mac_addr, buf, ETH_ALEN);

	cmd_set.pmsys_dev = wlan_msys;
	cmd_set.mod_id = SYS_GEN_MOD;
	cmd_set.mesg_id = SYS_GEN_CMD_SET_WIFI_INFO;
	cmd_set.pdata = &wlan_info;
	cmd_set.data_size = sizeof(wlan_info);
	cmd_set.pout_data = NULL;
	cmd_set.out_size = 0;

	ret = rda_msys_send_cmd(&cmd_set);
	if (ret) {
		WLAND_ERR("nvram:can not set wifi mac to nvram \n");
		ret = -EBUSY;
		goto err_handle_cmd;
	}

	WLAND_DBG(DEFAULT, NOTICE,
		"nvram:set wifi mac address [%02x:%02x:%02x:%02x:%02x:%02x] to nvram success.\n",
		buf[0], buf[1], buf[2], buf[3], buf[4], buf[5]);
	ret = 0;		/* success */

err_handle_cmd:
	rda_msys_unregister_device(wlan_msys);
	rda_msys_free_device(wlan_msys);
err_handle_sys:
	return ret;
}
#endif
