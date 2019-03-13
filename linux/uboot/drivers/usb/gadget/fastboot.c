/*
 * Copyright (c) 2009, Google Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */
#include <common.h>
#include <asm/errno.h>
#include <usbdescriptors.h>
#include <usbdevice.h>
#include <linux/ctype.h>
#include <malloc.h>
#include <command.h>
#include <nand.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/nand.h>
#include <jffs2/jffs2.h>
#include <asm/types.h>
#include <asm/io.h>
#include <android/android_boot.h>
#include <android/android_bootimg.h>
#include <android/boot_mode.h>
#include <asm/arch/rda_sys.h>
#include "fastboot.h"
#include <mmc.h>
#include <mmc/sparse.h>
#include <mmc/mmcpart.h>
#if defined(CONFIG_MUSB_UDC)
#include <usb/musb_udc.h>
#else
#error "no usb device controller"
#endif

//#define DEBUG
#ifdef DEBUG
#define fb_dbg(fmt, args...) serial_printf(fmt, ##args)
#else
#define fb_dbg(fmt, args...) do {} while(0)
#endif

#define fb_info(fmt, args...) serial_printf(fmt, ##args)

#ifdef FLASH_PAGE_SIZE
#undef FLASH_PAGE_SIZE
#endif
#define FLASH_PAGE_SIZE 2048

/** implemented in cmd_misc.c */
extern int load_boot_from_nand(void);
extern int load_boot_from_mmc(void);

struct fastboot_cmd {
	struct fastboot_cmd *next;
	const char *prefix;
	unsigned prefix_len;
	void (*handle) (const char *arg, void *data, loff_t sz);
};

struct fastboot_var {
	struct fastboot_var *next;
	const char *name;
	const char *value;
};

static struct fastboot_cmd *cmdlist;

static void fastboot_register(const char *prefix,
			      void (*handle) (const char *arg, void *data,
					      loff_t sz))
{
	struct fastboot_cmd *cmd;
	cmd = malloc(sizeof(*cmd));
	if (cmd) {
		cmd->prefix = prefix;
		cmd->prefix_len = strlen(prefix);
		cmd->handle = handle;
		cmd->next = cmdlist;
		cmdlist = cmd;
	}
}

static struct fastboot_var *varlist;

static void fastboot_publish(const char *name, const char *value)
{
	struct fastboot_var *var;
	var = malloc(sizeof(*var));
	if (var) {
		var->name = name;
		var->value = value;
		var->next = varlist;
		varlist = var;
	}
}

static unsigned char buffer[4096];

static void *download_base;
static unsigned download_max;
static unsigned download_size;

enum {
	STATE_OFFLINE,
	STATE_COMMAND,
	STATE_COMPLETE,
	STATE_EXIT,
	STATE_ERROR
};

static unsigned fastboot_state = STATE_OFFLINE;

static void show_progress(unsigned img_size, unsigned xfer)
{
	static int step = 0;
	static unsigned old_size = 0;

	if (img_size != old_size) {
		old_size = img_size;
		step = 0;
	}

	if (step % 64 == 0)
		fb_info(">");
	step++;
	if (step > 2560) {
		printf("%3d%%\n", (u32) (((u64) xfer * 100) / img_size));
		step = 0;
	}
	if (xfer >= img_size)
		fb_info("100%% finised\n");
}

static int usb_read(void *_buf, unsigned len)
{
	int count = 0;
	struct usb_endpoint_instance *ep_out = fastboot_get_out_ep();
	struct urb *current_urb = NULL;
	unsigned char *buf = _buf;
	unsigned total = len;

	if (!fastboot_configured()) {
		return 0;
	}

	if (fastboot_state == STATE_ERROR)
		goto oops;

	if (!ep_out)
		goto oops;

	fb_dbg("%s, buf 0x%p, len :%d\n", __func__, _buf, len);
	current_urb = ep_out->rcv_urb;
	while (len > 0) {
		int xfer;
		int maxPktSize = ep_out->rcv_packetSize;
		int dma = 0;

		xfer = (len > maxPktSize) ? maxPktSize : len;
		if (xfer == maxPktSize)
			dma = 1;
		usbd_setup_urb(current_urb, buf, xfer, dma);
		udc_irq();
		if (current_urb->actual_length) {
			buf += current_urb->actual_length;
			len -= current_urb->actual_length;
			count += current_urb->actual_length;
			if (xfer != current_urb->actual_length)
				break;
			show_progress(total, count);
			current_urb->actual_length = 0;
		}
	}
	current_urb->actual_length = 0;
	fb_dbg("%s, read :%d\n", __func__, count);
	return count;

oops:
	fb_info("fastboot usb read error\n");
	fastboot_state = STATE_ERROR;
	return -1;
}

static int usb_write(void *_buf, unsigned len)
{
	struct usb_endpoint_instance *ep_in = fastboot_get_in_ep();
	struct urb *current_urb = NULL;
	unsigned char *buf = _buf;
	int count = 0;

	if (!fastboot_configured()) {
		return 0;
	}

	if (fastboot_state == STATE_ERROR)
		goto oops;

	if (!ep_in)
		goto oops;

	fb_dbg("%s, len :%d\n", __func__, len);
	current_urb = ep_in->tx_urb;

	while (len > 0) {
		int xfer;
		int maxPktSize = ep_in->tx_packetSize;

		xfer = (len > maxPktSize) ? maxPktSize : len;
		current_urb->buffer = buf;

		current_urb->actual_length = xfer;
		fb_dbg("urb actual_len :%d, ep sent, last %d\n",
		       current_urb->actual_length, ep_in->sent, ep_in->last);
		if (udc_endpoint_write(ep_in))
			goto oops;
		count += xfer;
		len -= xfer;
		buf += xfer;
	}
	fb_dbg("after write urb actual_len :%d, ep sent, last %d, count:%d\n",
	       current_urb->actual_length, ep_in->sent, ep_in->last, count);

	return count;

oops:
	fb_info("fastboot usb write error\n");
	fastboot_state = STATE_ERROR;
	return -1;
}

void fastboot_ack(const char *code, const char *reason)
{
	char response[64] = { 0 };

	if (fastboot_state != STATE_COMMAND)
		return;

	if (reason == 0)
		reason = "";

	if (strlen(code) + strlen(reason) >= 64) {
		fb_info("%s too long string\r\n", __func__);
	}
	sprintf(response, "%s%s", code, reason);
	fastboot_state = STATE_COMPLETE;

	usb_write(response, strlen(response));

}

void fastboot_fail(const char *reason)
{
	fastboot_ack("FAIL", reason);
}

void fastboot_okay(const char *info)
{
	fastboot_ack("OKAY", info);
}

void fastboot_info(const char *info)
{
	char response[64] = { 0 };
	const char *code = "INFO";

	if (info == 0)
		info = "";

	if (strlen(code) + strlen(info) >= 64) {
		fb_info("%s too long string\r\n", __func__);
	}
	snprintf(response, 63, "%s%s", code, info);

	usb_write(response, strlen(response));
}

static void cmd_getvar(const char *arg, void *data, loff_t sz)
{
	struct fastboot_var *var;
	int value_len;
	char str[60];

	for (var = varlist; var; var = var->next) {
		if (!strcmp(var->name, arg)) {
			int index = 0;

			value_len = strlen(var->value);
			if (value_len < 60) {
				fastboot_okay(var->value);
				return;
			}
			/* value length > 60(+INFO > 64) */
			while (value_len) {
				int xfer = (value_len > 50) ? 50 : value_len;

				memset(str, 0, sizeof(str));
				strncpy(str, &var->value[index], xfer);
				fastboot_info(str);
				value_len -= xfer;
				index += xfer;
			}
			fastboot_okay("");
			return;
		}
	}
	fastboot_okay("");
}

static void cmd_download(const char *arg, void *data, loff_t sz)
{
	char response[64];
	unsigned len = simple_strtoul(arg, NULL, 16);
	int r;

	fb_dbg("%s\n", __func__);

	fb_info("start downloading... length  %d is to %p\n", len, data);
	download_size = 0;
	if (len > download_max) {
		fastboot_fail("data too large");
		return;
	}

	sprintf(response, "DATA%08x", len);
	if (usb_write(response, strlen(response)) < 0)
		return;

	r = usb_read(download_base, len);
	if ((r < 0) || (r != len)) {
		fastboot_state = STATE_ERROR;
		return;
	}
	download_size = len;
	fb_info("download ok\n");
	fastboot_okay("");
	//dump_log(download_base, len);
}

extern int mtdparts_init_default(void);
extern int mtdparts_save_ptbl(int need_erase);

#ifdef MTDPARTS_UBI_DEF
extern int ubi_part_scan(char *part_name);
extern int ubi_check_default_vols(const char *ubi_default_str);
extern int ubi_erase_vol(char *vol_name);
extern int ubi_update_vol(char *vol_name, void *buf, size_t size);
static int cmd_flash_ubi(const char *arg, void *data, loff_t sz)
{
	int ret = 0;

	ret = ubi_part_scan(MTDPARTS_UBI_PART_NAME);
	if(ret) {
		fastboot_fail("ubi init failed.");
		goto exit;
	}

	ret = ubi_check_default_vols(MTDPARTS_UBI_DEF);
	if(ret) {
		fastboot_fail("ubi volumes check failed.");
		goto exit;
	}

	fb_info("ready to update ubi volume '%s'\n", arg);
	ret = ubi_update_vol((char *)arg, data, sz);
exit:
	return ret;
}
#endif /* MTDPARTS_UBI_DEF */
static void cmd_flash_nand(const char *arg, void *data, loff_t sz)
{
	struct mtd_info *nand;
	struct mtd_device *dev;
	struct part_info *part;
	size_t size = 0;
	u8 pnum;
	nand_erase_options_t opts;
	int ret;
	unsigned addr = simple_strtoul(arg, NULL, 16);
	loff_t max_limit = 0;
	u32 bad_blocks = 0;
	sparse_header_t *header = (void *)data;

	fb_info("%s, addr: %s date: %p, sz: 0x%llx\n", __func__, arg, data, sz);

	/* use "flash" command for downloading purpose */
	if (addr && (addr >= PHYS_SDRAM_1) &&
	    (addr < (PHYS_SDRAM_1 + PHYS_SDRAM_1_SIZE))) {
		fb_info("in memory, do copy to 0x%x\n", addr);
		fastboot_okay("");
		memcpy((void *)addr, download_base, sz);
		fastboot_state = STATE_EXIT;
		return;
	}

	data = download_base;	//previous downloaded date to download_base

#ifdef MTDPARTS_UBI_DEF /* all ubi volumes are in one ubi part */
	if(strstr(MTDPARTS_UBI_DEF, arg)) {
		ret = cmd_flash_ubi(arg, data, sz);
		goto exit;
	}
#endif

	ret = find_dev_and_part(arg, &dev, &pnum, &part);
	if (ret) {
		fastboot_fail("unknown partition name");
		return;
	} else if (dev->id->type != MTD_DEV_TYPE_NAND) {
		fastboot_fail("mtd dev type error");
		return;
	}
	nand = &nand_info[dev->id->num];
	fb_info("found part '%s' offset: 0x%llx length: 0x%llx id: %d\n",
		part->name, part->offset, part->size, dev->id->num);

	memset(&opts, 0, sizeof(opts));
	opts.offset = (loff_t) part->offset;
	opts.length = (loff_t) part->size;
	opts.jffs2 = 0;
	opts.quiet = 0;

	fb_dbg("opts off  0x%08x\n", (uint32_t) opts.offset);
	fb_dbg("opts size 0x%08x\n", (uint32_t) opts.length);
	fb_dbg("nand write size 0x%08x\n", nand->writesize);
	fb_info("erase 0x%llx bytes to '%s' offset: 0x%llx\n",
		opts.length, part->name, opts.offset);
	ret = nand_erase_opts(nand, &opts);
	if (ret) {
		fastboot_fail("nand erase error");
		return;
	}

	if (!strcmp(part->name, "boot") || !strcmp(part->name, "recovery")) {
		if (memcmp((void *)data, BOOT_MAGIC, BOOT_MAGIC_SIZE)) {
			fastboot_fail("image is not a boot image");
			return;
		}
	}

	if (is_power_of_2(nand->writesize))
		sz = ROUND(sz, nand->writesize);
	else
		sz = roundup(sz, nand->writesize);
	size = sz;
	max_limit = part->offset + part->size;

	fb_info("writing 0x%x bytes to '%s' offset: 0x%llx\n", size,
		part->name, part->offset);
	header = (void *)data;
	if (header->magic != SPARSE_HEADER_MAGIC) {
		ret =
		    nand_write_skip_bad_new(nand, part->offset, &size,
					    max_limit, (u_char *) data,
					    0,
					    &bad_blocks);
	} else {
		ret = nand_write_unsparse(nand, part->offset, &size, max_limit,
					  (u_char *) data,
					  0,
					  &bad_blocks);
	}

	/* the mtd partition table is saved in 'bootloader' partition,
	 * if 'bootloader' is updated, need to re-write the partition table. */
	if(strcmp(part->name,"bootloader") == 0) {
		mtdparts_save_ptbl(1);
	}

#ifdef MTDPARTS_UBI_DEF
exit:
#endif
	if (!ret)
		fastboot_okay("");
	else
		fastboot_fail("flash error");
	fb_info("flash ok\n");
}

static block_dev_desc_t *mmc_blkdev;
void cmd_flash_mmc_img(const char *name, void *data, loff_t sz)
{
	disk_partition_t *ptn = 0;
	u64 size = 0;

	fb_info("flash to mmc part %s\n", name);
	ptn = partition_find_ptn(name);
	if (ptn == 0) {
		fastboot_fail("partition table doesn't exist");
		return;
	}

	if (!strcmp(name, "boot") || !strcmp(name, "recovery")) {
		if (memcmp((void *)data, BOOT_MAGIC, BOOT_MAGIC_SIZE)) {
			fastboot_fail("image is not a boot image");
			return;
		}
	}

	size = (u64) ptn->size * ptn->blksz;
	fb_info("partition %s, type %s blocks " LBAFU ", size %llx\n",
		ptn->name, ptn->type, ptn->size, size);
	if (ROUND(sz, 512) > size) {
		fastboot_fail("size too large");
		return;
	} else if (partition_write_bytes(mmc_blkdev, ptn, &sz, data)) {
		fastboot_fail("flash write failure");
		return;
	}

	fastboot_okay("");
	return;
}

void cmd_flash_mmc_sparse_img(const char *part_name, void *data, loff_t sz)
{
	disk_partition_t *ptn;
	unsigned long long size = 0;
	int ret;

	fb_info("unsparese and flash part %s\n", part_name);
	ptn = partition_find_ptn(part_name);
	if (ptn == 0) {
		fastboot_fail("partition table doesn't exist");
		return;
	}

	size = (u64) ptn->blksz * ptn->size;
	if (ROUND(sz, 512) > size) {
		fastboot_fail("size too large");
		return;
	}

	ret = partition_unsparse(mmc_blkdev, ptn, data, ptn->start, ptn->size);
	if (ret) {
		fastboot_fail("partition cannot unsparse");
		return;
	}
	fastboot_okay("");
	return;
}

void cmd_flash_mmc(const char *arg, void *data, loff_t sz)
{
	sparse_header_t *sparse_header;

	sparse_header = (sparse_header_t *) data;

	/* Using to judge if ext4 file system */
	if (sparse_header->magic != SPARSE_HEADER_MAGIC)
		cmd_flash_mmc_img(arg, data, sz);
	else
		cmd_flash_mmc_sparse_img(arg, data, sz);

	return;
}

static void cmd_flash(const char *arg, void *data, loff_t sz)
{
	if (rda_media_get() == MEDIA_MMC)
		cmd_flash_mmc(arg, data, sz);
	else
		cmd_flash_nand(arg, data, sz);
}

static void cmd_erase_nand(const char *arg, void *data, unsigned sz)
{
	struct mtd_info *nand;
	struct mtd_device *dev;
	struct part_info *part;
	u8 pnum;
	nand_erase_options_t opts;
	int ret;

	fb_info("erase part: %s\n", arg);

#ifdef MTDPARTS_UBI_DEF /* all ubi volumes are in one ubi part */
	if(strstr(MTDPARTS_UBI_DEF, arg)) {
		ubi_part_scan(MTDPARTS_UBI_PART_NAME);
		ret = ubi_erase_vol((char *)arg);
		if(ret)
			fastboot_fail("ubi volume erase error");
		else
			fastboot_okay("");
		return;
	}
#endif

	ret = find_dev_and_part(arg, &dev, &pnum, &part);
	if (ret) {
		fastboot_fail("unknown partition name");
		return;
	} else if (dev->id->type != MTD_DEV_TYPE_NAND) {
		fastboot_fail("mtd dev type error");
		return;
	}
	nand = &nand_info[dev->id->num];
	fb_info("found part '%s' offset: 0x%llx length: 0x%llx id: %d\n",
		part->name, part->offset, part->size, dev->id->num);

	memset(&opts, 0, sizeof(opts));
	opts.offset = (loff_t) part->offset;
	opts.length = (loff_t) part->size;
	opts.jffs2 = 0;
	opts.quiet = 0;

	fb_dbg("opts off  0x%08x\n", (uint32_t) opts.offset);
	fb_dbg("opts size 0x%08x\n", (uint32_t) opts.length);
	fb_dbg("nand write size 0x%08x\n", nand->writesize);
	ret = nand_erase_opts(nand, &opts);
	if (ret)
		fastboot_fail("nand erase error");
	else
		fastboot_okay("");
}

static void cmd_erase_mmc(const char *arg, void *data, loff_t sz)
{
	int ret;
	disk_partition_t *ptn;
	lbaint_t blkcnt;

	fb_info("erase part: %s\n", arg);
	ptn = partition_find_ptn(arg);
	if (!ptn) {
		fastboot_fail("partition table doesn't exist");
		return;
	}

	blkcnt = ptn->size;
	fb_info("erase mmc from %#x, blocks %#x\n",
				(uint32_t)ptn->start, (uint32_t)blkcnt);
	ret = partition_erase_blks(mmc_blkdev, ptn, &blkcnt);
	if (ret || blkcnt != ptn->size) {
		fastboot_fail("partition erase failed");
		return;
	}
	fastboot_okay("");
}

static void cmd_erase(const char *arg, void *data, loff_t sz)
{
	if (rda_media_get() == MEDIA_MMC)
		return cmd_erase_mmc(arg, data, sz);
	else
		return cmd_erase_nand(arg, data, sz);
}

static unsigned char raw_header[2048];

static void cmd_boot(const char *arg, void *data, loff_t sz)
{
	boot_img_hdr *hdr = (boot_img_hdr *) raw_header;
	unsigned kernel_actual;
	unsigned ramdisk_actual;
	char *cmdline;

	fb_info("%s, arg: %s, data: %p, sz: 0x%llx\n", __func__, arg, data, sz);
	memcpy(raw_header, data, 2048);
	if (memcmp(hdr->magic, BOOT_MAGIC, BOOT_MAGIC_SIZE)) {
		fb_info("boot image headr: %s\n", hdr->magic);
		fastboot_fail("bad boot image header");
		return;
	}
	kernel_actual = ROUND(hdr->kernel_size, FLASH_PAGE_SIZE);
	if (kernel_actual <= 0) {
		fastboot_fail("kernel image should not be zero");
		return;
	}
	ramdisk_actual = ROUND(hdr->ramdisk_size, FLASH_PAGE_SIZE);
	if (ramdisk_actual < 0) {
		fastboot_fail("ramdisk size error");
		return;
	}

	memcpy((void *)hdr->kernel_addr, (void *)data + FLASH_PAGE_SIZE,
	       kernel_actual);
	memcpy((void *)hdr->ramdisk_addr,
	       (void *)data + FLASH_PAGE_SIZE + kernel_actual, ramdisk_actual);

	fb_info("kernel @0x%08x (0x%08x bytes)\n", hdr->kernel_addr,
		kernel_actual);
	fb_info("ramdisk @0x%08x (0x%08x bytes)\n", hdr->ramdisk_addr,
		ramdisk_actual);
	//set boot environment
	if (hdr->cmdline[0]) {
		cmdline = (char *)hdr->cmdline;
	} else {
		cmdline = getenv("bootargs");
	}

	fb_info("cmdline %s\n", cmdline);
	fastboot_okay("");
	udc_power_off();
	creat_atags(hdr->tags_addr, cmdline, hdr->ramdisk_addr,
		    hdr->ramdisk_size);

	cleanup_before_linux();
	boot_linux(hdr->kernel_addr, hdr->tags_addr);
}

extern int do_cboot(cmd_tbl_t * cmdtp, int flag, int argc, char *const argv[]);

static void cmd_continue(const char *arg, void *data, loff_t sz)
{
	fb_info("continue....\n");
	fastboot_okay("");
	udc_power_off();
	fastboot_state = STATE_EXIT;
	//do_cboot(NULL, 0, 1, NULL);
	//normal_mode();
}

static void cmd_reboot(const char *arg, void *data, loff_t sz)
{
	fb_info("reboot....\n");
	fastboot_okay("");
	mdelay(10);
	udc_power_off();
	fastboot_state = STATE_EXIT;
	rda_reboot(REBOOT_TO_NORMAL_MODE);
}

static void cmd_reboot_bootloader(const char *arg, void *data, loff_t sz)
{
	fb_info("reboot to bootloader....\n");
	fastboot_okay("");
	mdelay(10);
	udc_power_off();
	fastboot_state = STATE_EXIT;
	rda_reboot(REBOOT_TO_FASTBOOT_MODE);
}

static void cmd_powerdown(const char *arg, void *data, loff_t sz)
{
	fb_info("power down....\n");
	fastboot_okay("");
	fastboot_state = STATE_EXIT;
	//power_down_devices(0);
}

static void fastboot_command_loop(void)
{
	struct fastboot_cmd *cmd;
	int r;
	fb_dbg("fastboot: processing commands\n");

again:
	while ((fastboot_state != STATE_ERROR)
	       && (fastboot_state != STATE_EXIT)) {
		memset(buffer, 0, 64);
		r = usb_read(buffer, 64);
		if (r < 0)
			break;
		buffer[r] = 0;
		fb_dbg("fastboot: %s, r:%d\n", buffer, r);

		for (cmd = cmdlist; cmd; cmd = cmd->next) {
			fb_dbg("cmd list :%s \n", cmd->prefix);
			if (memcmp(buffer, cmd->prefix, cmd->prefix_len))
				continue;
			fastboot_state = STATE_COMMAND;
			cmd->handle((const char *)buffer + cmd->prefix_len,
				    (void *)download_base, download_size);
			if (fastboot_state == STATE_COMMAND)
				fastboot_fail("unknown reason");
			if (fastboot_state == STATE_EXIT)
				goto fb_done;
			goto again;
		}

		fastboot_fail("unknown command");
	}
	fastboot_state = STATE_OFFLINE;
fb_done:
	fb_dbg("fastboot: done\n");
}

static int fastboot_handler(void *arg)
{
	while (fastboot_state != STATE_EXIT) {
		fastboot_command_loop();
	}
	return 0;
}

static int fastboot_start(void *base, unsigned size)
{
	char *parts;

	fb_info("fastboot start\n");

	download_base = base;
	download_max = size;

	fastboot_register("getvar:", cmd_getvar);
	fastboot_register("download:", cmd_download);
	fastboot_register("flash:", cmd_flash);
	fastboot_register("erase:", cmd_erase);
	fastboot_register("boot", cmd_boot);
	fastboot_register("reboot", cmd_reboot);
	fastboot_register("powerdown", cmd_powerdown);
	fastboot_register("continue", cmd_continue);
	fastboot_register("reboot-bootloader", cmd_reboot_bootloader);

	fastboot_publish("version", "1.0");

	parts = getenv("mtdparts");
	fastboot_publish("mtd", parts);

	fastboot_handler(0);
	return 0;
}

/*
 * Since interrupt handling has not yet been implemented, we use this function
 * to handle polling.  .
 */
static void fastboot_wait_for_configed(void)
{
	/* New interrupts? */
	while (!fastboot_configured())
		udc_irq();
}

void boot_linux_from_mem(void *data)
{
	boot_img_hdr *hdr = (boot_img_hdr *) data;
	unsigned kernel_actual;
	unsigned ramdisk_actual;
	char *cmdline;

	fb_info("%s, data: %p\n", __func__, data);
	if (memcmp(hdr->magic, BOOT_MAGIC, BOOT_MAGIC_SIZE)) {
		fb_info("boot image headr: %s\n", hdr->magic);
		fastboot_fail("bad boot image header");
		return;
	}
	kernel_actual = ROUND(hdr->kernel_size, FLASH_PAGE_SIZE);
	if (kernel_actual <= 0) {
		fastboot_fail("kernel image should not be zero");
		return;
	}
	ramdisk_actual = ROUND(hdr->ramdisk_size, FLASH_PAGE_SIZE);
	if (ramdisk_actual < 0) {
		fastboot_fail("ramdisk size error");
		return;
	}

	memcpy((void *)hdr->kernel_addr, (void *)data + FLASH_PAGE_SIZE,
	       kernel_actual);
	memcpy((void *)hdr->ramdisk_addr,
	       (void *)data + FLASH_PAGE_SIZE + kernel_actual, ramdisk_actual);

	fb_info("kernel @0x%08x (0x%08x bytes)\n", hdr->kernel_addr,
		kernel_actual);
	fb_info("ramdisk @0x%08x (0x%08x bytes)\n", hdr->ramdisk_addr,
		ramdisk_actual);
	//set boot environment
	if (hdr->cmdline[0]) {
		cmdline = (char *)hdr->cmdline;
	} else {
		cmdline = getenv("bootargs");
	}

	fb_info("cmdline %s\n", cmdline);
	creat_atags(hdr->tags_addr, cmdline, hdr->ramdisk_addr,
		    hdr->ramdisk_size);

	cleanup_before_linux();
	boot_linux(hdr->kernel_addr, hdr->tags_addr);
}

void boot_linux_from_nand(void)
{
    if(load_boot_from_nand() == CMD_RET_FAILURE) {
        fb_info("fastboot load_boot_from_nand fail\n");
        return;
    }
	/* boot from mem */
	boot_linux_from_mem((void *)SCRATCH_ADDR);
}

void boot_linux_from_mmc(void)
{
    if(load_boot_from_mmc() == CMD_RET_FAILURE) {
        fb_info("fastboot load_boot_from_mmc fail\n");
        return;
    }
	/* boot from mem */
	boot_linux_from_mem((void *)SCRATCH_ADDR);
}

void boot_linux_from_flash(void)
{
	enum media_type media = rda_media_get();

	if (media == MEDIA_MMC)
		boot_linux_from_mmc();
	else if ((media == MEDIA_NAND) || (media == MEDIA_SPINAND))
		boot_linux_from_nand();
	else
		fb_info("fastboot can't find media\n");
}

#if defined(CONFIG_CMD_FASTBOOT)
extern int drv_fastboot_init(void);

int do_fastboot(cmd_tbl_t * cmdtp, int flag, int argc, char *const argv[])
{
	int ret;
	enum media_type media = rda_media_get();

	udc_init();
	drv_fastboot_init();

	if ((media == MEDIA_NAND) || (media == MEDIA_SPINAND)) {
		ret = mtdparts_init_default();
		if(ret) {
			fb_info("nand format partition fail %d\n", ret);
			return -1;
		}
	}

	if (media == MEDIA_MMC) {
		ret = mmc_parts_format();
		if (ret) {
			fb_info("mmc format partition fail %d\n", ret);
			return -1;
		}
		mmc_blkdev = get_dev_by_name(CONFIG_MMC_DEV_NAME);
	}
	fastboot_wait_for_configed();

	fastboot_start((void *)SCRATCH_ADDR, FB_DOWNLOAD_BUF_SIZE);
	return 0;
}

U_BOOT_CMD(fastboot, 1, 1, do_fastboot,
	   "android fastboot protocol", "flash image to nand");

int do_abootf(cmd_tbl_t * cmdtp, int flag, int argc, char *const argv[])
{
	boot_linux_from_flash();
	return 1;
}

U_BOOT_CMD(abootf, 1, 0, do_abootf,
	   "boot android boot.img from flash",
	   "    - boot android boot.image from flash");

int do_abootm(cmd_tbl_t * cmdtp, int flag, int argc, char *const argv[])
{
	ulong offset = 0;
	if (argc >= 2) {
		offset = simple_strtoul(argv[1], NULL, 16);
	}
	boot_linux_from_mem((void *)offset);
	return 1;
}

U_BOOT_CMD(abootm, 2, 0, do_abootm,
	   "boot android boot.img from memory",
	   "[ off ]\n"
	   "    - boot android boot.image from memory with offset 'off'");

#endif
