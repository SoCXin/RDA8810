/*
 * (C) Copyright 2001
 * Wolfgang Denk, DENX Software Engineering, wd@denx.de.
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

/*
 * Misc functions
 */
#include <common.h>
#include <command.h>
#include <usb/usbserial.h>
#include <nand.h>
#include <jffs2/jffs2.h>
#include <asm/arch/mdcom.h>
#include <asm/arch/hwcfg.h>
#include <asm/arch/factory.h>
#include <asm/arch/prdinfo.h>
#include <asm/arch/rda_sys.h>
#include <part.h>
#include <mmc.h>
#include <mmc/mmcpart.h>
#include <nand.h>
#include <malloc.h>
#include <linux/mtd/nand.h>
#include <mtd/nand/rda_nand.h>


#if 1
#define bb_dbg(fmt, args...) printf(fmt, ##args)
#else
#define bb_dbg(fmt, args...) do {} while(0)
#endif

#define RDA_ADD_ROUNDUP(x)	(((x) + 3) & ~3)
#define RDA_CHN_BUF_LEN		140

struct rda_sys_hdr {
	u16 magic;
	u16 mod_id;
	u32 req_id;
	u32 ret_val;
	u16 msg_id;
	u16 ext_len;
	u8 ext_data[];
};

struct rda_bp_info {
	u32 bm_ver;
	u32 bm_commit_rev;
	u32 bm_build_date;
	u32 bs_ver;
	u32 bs_commit_rev;
	u32 bs_build_date;
};

extern int mtdparts_init_default(void);
extern int find_dev_and_part(const char *id, struct mtd_device **dev,
			     u8 *part_num, struct part_info **part);

#ifdef REBOOT_WHEN_CRASH
#define ROLLBACK_TO_RECOVERY_MODE		0
#endif

/*
 * Syntax:
 *  mdcom_send {channel} {adress} {size} [{timeout} {mode}]
 */
int do_mdcom_send(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	unsigned int channel;
	void *buf;
	int size;
	int timeout = 0;
	int mode = 0;

	if (argc < 4)
		return CMD_RET_USAGE;

	channel = simple_strtoul(argv[1], NULL, 10);
	buf = (void*) simple_strtoul(argv[2], NULL, 16);
	size = simple_strtoul(argv[3], NULL, 10);
	if (argc >4)
		timeout = simple_strtoul(argv[4], NULL, 10);
	if (argc >5)
		mode = simple_strtoul(argv[5], NULL, 10);

	if (mode) {
		bb_dbg("MDCOM send %d bytes stream to channel %d with returned value %d.\n",
		       size, channel, rda_mdcom_channel_buf_send_stream(channel, buf,
				       size, timeout));
	} else {
		bb_dbg("MDCOM send %d bytes dgram to channel %d with returned value %d.\n",
		       size, channel, rda_mdcom_channel_buf_send_dgram(channel, buf,
				       size, timeout));
	}

	rda_mdcom_channel_show(channel);
	return 0;
}

/*
 * Syntax:
 *  mdcom_recv {channel} {adress} {size} [{timeout} {mode}]
 */
int do_mdcom_recv(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	unsigned int channel;
	void *buf;
	int size;
	int timeout = 0;
	int mode = 0;

	if (argc < 4)
		return CMD_RET_USAGE;

	channel = simple_strtoul(argv[1], NULL, 10);
	buf = (void*) simple_strtoul(argv[2], NULL, 16);
	size = simple_strtoul(argv[3], NULL, 10);
	if (argc >4)
		timeout = simple_strtoul(argv[4], NULL, 10);
	if (argc >5)
		mode = simple_strtoul(argv[5], NULL, 10);

	if (mode) {
		bb_dbg("MDCOM receive %d bytes stream from channel %d with returned value %d.\n",
		       size, channel, rda_mdcom_channel_buf_recv_stream(channel, buf,
				       size, timeout));
	} else {
		bb_dbg("MDCOM receive %d bytes dgram from channel %d with returned value %d.\n",
		       size, channel, rda_mdcom_channel_buf_recv_dgram(channel, buf,
				       size, timeout));
	}

	rda_mdcom_channel_show(channel);
	return 0;
}

static int rda_image_verify_header(const image_header_t *hdr)
{
	bb_dbg("   Verifying Image Header ... ");
	if (!image_check_magic(hdr)) {
		bb_dbg("Bad Magic Number\n");
		return CMD_RET_FAILURE;
	}

	if (!image_check_hcrc(hdr)) {
		bb_dbg("Bad Header Checksum\n");
		return CMD_RET_FAILURE;
	}
	bb_dbg("OK\n");

	image_print_contents(hdr);

	return CMD_RET_SUCCESS;
}

static int rda_image_verify_data(const image_header_t *hdr)
{
	bb_dbg("   Verifying Data Checksum ... ");
	if (!image_check_dcrc(hdr)) {
		bb_dbg("Bad Data CRC\n");
		return CMD_RET_FAILURE;
	}
	bb_dbg("OK\n");

	return CMD_RET_SUCCESS;
}

static int rda_image_verify(const image_header_t *hdr)
{
	int ret;

	ret = rda_image_verify_header(hdr);
	if (ret == CMD_RET_SUCCESS) {
		ret = rda_image_verify_data(hdr);
	}
	return ret;
}

int rda_modem_image_load(const image_header_t *hdr, int cal_en)
{
	u32 pc, param;
	u32 modem_load_addr = image_get_load(hdr);
	u32 ap_load_addr = rda_mdcom_address_modem2ap(modem_load_addr);

	bb_dbg("%08x (AP address %08x) ...\n", modem_load_addr, ap_load_addr);

	memcpy((void *)ap_load_addr, (const void *)image_get_data(hdr),
			image_get_size(hdr));

	pc = image_get_ep(hdr);
	param = cal_en ? 0xCA1BCA1B : 0;
	if (rda_mdcom_setup_run_env(pc, param))
		return -1;

	bb_dbg("    pc = %08x\n", pc);
	bb_dbg("    param = %08x\n", param);
	bb_dbg("Done\n");

	return 0;
}

int rda_modem_cal_save(void)
{
	u32 addr, size;
	int ret;

	/* update all data to factorydata part. */
	rda_mdcom_get_calib_section(&addr, &size);
	factory_set_modem_calib((unsigned char *)addr);
	rda_mdcom_get_ext_calib_section(&addr, &size);
	factory_set_modem_ext_calib((unsigned char *)addr);
	rda_mdcom_get_factory_section(&addr, &size);
	factory_set_modem_factory((unsigned char *)addr);
	rda_mdcom_get_ap_factory_section(&addr, &size);
	factory_set_ap_factory((unsigned char *)addr);

	ret = factory_burn();
	if (ret) {
		bb_dbg("Error when saving calibration data\n");
		return ret;
	}
	return 0;
}

static void rda_modem_handle_syscmd(void)
{
	u8 md_com_buf[RDA_CHN_BUF_LEN / sizeof(u8)];
	struct rda_sys_hdr *phdr = (struct rda_sys_hdr *)md_com_buf;

#ifdef TEST_SYS
	static int test_num = 0;
	u8 tx_buf[RDA_CHN_BUF_LEN / sizeof(u8)];
	struct rda_sys_hdr *ptx_hdr;
	u32 *pext;
	int ret;

	if (test_num < 40) {
		ptx_hdr = (struct rda_sys_hdr *) tx_buf;

		ptx_hdr->magic = 0xA8B1;
		ptx_hdr->mod_id = 0x1;
		ptx_hdr->msg_id = 0x2000 + test_num;
		ptx_hdr->req_id = 0xA1234567;
		ptx_hdr->ret_val = 0xF7654321;

		ptx_hdr->ext_len = 4;
		pext = (u32 *)ptx_hdr->ext_data;
		pext[0] = 0x1000123F;

		ret = rda_mdcom_channel_buf_send_dgram(RDA_MDCOM_CHANNEL_SYSTEM, tx_buf, 16, 0);
		if (!ret) {
			rda_mdcom_line_set(RDA_MDCOM_PORT1, 2);
			test_num++;
		}
	}
#endif /* TEST_SYS */

	if (!rda_mdcom_tstc(RDA_MDCOM_CHANNEL_SYSTEM))
		return;

	if (rda_mdcom_channel_buf_recv_dgram(
				RDA_MDCOM_CHANNEL_SYSTEM,
				md_com_buf,
				16,
				usec2ticks(1000)))
		return;

	if (phdr->magic == 0xA8B1) {
		u16 len;
		u8 *pdata;
		u32 i;
		u32 *pret;

		len = phdr->ext_len;

		bb_dbg("## RECEIVED SYSTEM CMD:\n");
		bb_dbg("MODID - 0x%04x\n", phdr->mod_id);
		bb_dbg("REQID - 0x%08x\n", phdr->req_id);
		bb_dbg("RETVAL - 0x%08x\n", phdr->ret_val);
		bb_dbg("MSGID - 0x%04x\n", phdr->msg_id);
		bb_dbg("LEN - 0x%04x\n", phdr->ext_len);

		if (len) {
			pdata = phdr->ext_data;

			if (len > 128) {
				bb_dbg("Length is invalid!\n");
			} else if (len != rda_mdcom_channel_buf_recv_stream(
						RDA_MDCOM_CHANNEL_SYSTEM, pdata, len, 0)) {
				bb_dbg("Read Parameters Failed!\n");
			} else {
				bb_dbg("Params - ");
				for (i = 0; i < len; i++) {
					bb_dbg("%02x ", pdata[i]);
				}
				bb_dbg("\n\n");
			}
		} else {
			if ((phdr->mod_id == 1) && (phdr->msg_id == 2)) {
				bb_dbg("Calibration done\n");
				phdr->msg_id = 0x1001;
				phdr->ext_len = 4;
				pret = (u32 *) phdr->ext_data;

				if (rda_modem_cal_save()) {
					pret[0] = 0x0;
					bb_dbg("Calibration data save failed\n");
				} else {
					pret[0] = 0x1;
					bb_dbg("Calibration data save succeed\n");
					prdinfo_set_cali_result(1);
				}

				rda_mdcom_channel_buf_send_dgram(
					RDA_MDCOM_CHANNEL_SYSTEM, md_com_buf, 20, 0);
				rda_mdcom_line_set(RDA_MDCOM_PORT1, 2);
			}
		}
	}
}

static void rda_modem_handle_trace(void)
{
	u8 md_com_buf[RDA_CHN_BUF_LEN / sizeof(u8)];
	int ch;

	while (rda_mdcom_tstc(RDA_MDCOM_CHANNEL_TRACE)) {
		int len = rda_mdcom_channel_buf_recv_stream(
				  RDA_MDCOM_CHANNEL_TRACE,
				  md_com_buf,
				  128,
				  0);
		usbser_write(USB_ACM_CHAN_0, (unsigned char *) md_com_buf, len);
	}

	while (usbser_tstc(USB_ACM_CHAN_0) &&
			rda_mdcom_channel_buf_send_available(RDA_MDCOM_CHANNEL_TRACE)) {
		ch = usbser_getc(USB_ACM_CHAN_0);
		rda_mdcom_putc((char) ch, RDA_MDCOM_CHANNEL_TRACE);
	}
}

static void rda_modem_handle_atcmd(void)
{
	u8 md_com_buf[RDA_CHN_BUF_LEN / sizeof(u8)];
	if (!rda_mdcom_tstc(RDA_MDCOM_CHANNEL_AT))
		return;

	bb_dbg("## RECEIVED AT CMD:");
	do {
		unsigned short len = rda_mdcom_channel_buf_recv_stream(
					     RDA_MDCOM_CHANNEL_AT,
					     md_com_buf,
					     128,
					     0);
		*((char*) md_com_buf + len) = 0;
		puts((const char *) md_com_buf);
	} while (rda_mdcom_tstc(RDA_MDCOM_CHANNEL_AT));
	bb_dbg("\n");
}

int rda_modem_cal_load(void)
{
	u32 addr, size;
	const u8 *data;

	/* extended calibration data */
	rda_mdcom_get_ext_calib_section(&addr, &size);
	data = factory_get_modem_ext_calib();
	memcpy((u8 *)addr, data, size);
	bb_dbg("MOMEM EXT CALIB address %08x ...\n", addr);

	/* calibration data */
	rda_mdcom_get_calib_section(&addr, &size);
	data = factory_get_modem_calib();
	memcpy((u8 *)addr, data, size);
	bb_dbg("MOMEM CALIB address %08x ...\n", addr);

	/* modem fact data */
	rda_mdcom_get_factory_section(&addr, &size);
	data = factory_get_modem_factory();
	memcpy((u8 *)addr, data, size);
	bb_dbg("MOMEM FACT address %08x ...\n", addr);

	/* ap fact data */
	rda_mdcom_get_ap_factory_section(&addr, &size);
	data = factory_get_ap_factory();
	memcpy((u8 *)addr, data, size);
	bb_dbg("AP FACT address %08x ...\n", addr);

	return 0;
}

static int rda_check_ap_calib_msg(void)
{
	unsigned int msg_id = 0;
	unsigned int msg_size = 0;
	unsigned char *msg_data = malloc(RDA_AP_CALIB_MSG_DATA_LEN);

	if (!msg_data)
		return -1;

	if (factory_get_ap_calib_msg(&msg_id, &msg_size, msg_data)) {
		free(msg_data);
		return -1;
	}

	if (msg_id & RDA_AP_CALIB_MSG_SET_PRDINFO)
		prdinfo_set_data((struct prdinfo *)msg_data);

	free(msg_data);
	return 0;
}

/* use this function after factorydata is loaded */
static int rda_send_prdinfo_data(void)
{
	unsigned int msg_id = 0;
	unsigned int msg_size = 0;
	unsigned char *msg_data = malloc(RDA_AP_CALIB_MSG_DATA_LEN);

	if (!msg_data)
		return -1;

	memset(msg_data, 0, RDA_AP_CALIB_MSG_DATA_LEN);
	prdinfo_get_data((struct prdinfo *)msg_data);

	msg_id = RDA_AP_CALIB_MSG_GET_PRDINFO;
	msg_size = sizeof(struct prdinfo);
	factory_set_ap_calib_msg(msg_id, msg_size, msg_data);
	free(msg_data);
	return 0;
}

void rda_modem_cal_loop(void)
{
	/* send prdinfo data to calib tool */
	rda_send_prdinfo_data();

	while (1) {
#if defined(CONFIG_HW_WATCHDOG) || defined(CONFIG_WATCHDOG)
		WATCHDOG_RESET();
#endif
		/* check Ctrl-C */
		ctrlc();
		if ((had_ctrlc())) {
			bb_dbg("\n## ABORT MANUALLY!\n\n");
			return;
		}

		rda_modem_handle_syscmd();

		rda_modem_handle_trace();

		rda_modem_handle_atcmd();

		/* check message to u-boot from PC calib tool*/
		rda_check_ap_calib_msg();
	}
}

int rda_modem_bbimages_load_from_nand(const image_header_t **hdrInit,
		const image_header_t **hdrWork)
{
	struct mtd_info *nand;
	struct mtd_device *dev;
	struct part_info *part;
	size_t page_size, block_size;
	loff_t offset;
	size_t read_size, image_size;
	u8 pnum;
	int ret;
	u_char *buffer;
	image_header_t *hdr;
	u32 bad_blocks;
	char buf[256];
	u32 ih_len;
	int i;

	ret = mtdparts_init_default();
	if (ret != 0) {
		bb_dbg("mtdparts init error\n");
		return ret;
	}

	ret = find_dev_and_part("modem", &dev, &pnum, &part);
	if (ret) {
		bb_dbg("unknown partition name - modem\n");
		return ret;
	} else if (dev->id->type != MTD_DEV_TYPE_NAND) {
		bb_dbg("mtd dev type error");
		return CMD_RET_FAILURE;
	}
	bb_dbg("found modem part '%s' offset: 0x%llx length: 0x%llx\n",
		part->name, part->offset, part->size);

	nand = &nand_info[dev->id->num];
	page_size = nand->writesize;
	block_size = nand->erasesize;

	bb_dbg("\n## Checking Modem Code Image ...\n\n");

	for (i = 0; i < 2; ++i) {
		if (i == 0) {
			offset = part->offset;
			buffer = (u_char *)SCRATCH_ADDR;
			hdr = (image_header_t *)buffer;
		} else {
			hdr = (image_header_t *)((u8 *)hdr + image_size);
		}

		if (buffer < (u8 *)hdr + sizeof(image_header_t)) {
			read_size = (u8 *)hdr + sizeof(image_header_t) - buffer;
			if (is_power_of_2(page_size))
				read_size = ROUND(read_size, page_size);
			else
				read_size = roundup(read_size, page_size);
			bad_blocks = 0;

			bb_dbg("read 0x%x bytes from '%s' offset: 0x%08x\n",
			       read_size, part->name, (u32)offset);
			ret = nand_read_skip_bad_new(nand, offset, &read_size,
					buffer, &bad_blocks);
			if (ret) {
				bb_dbg("nand read fail\n");
				return ret;
			}

			offset += read_size + block_size * bad_blocks;
			buffer += read_size;
		}

		ret = rda_image_verify_header(hdr);
		if (ret)
			return ret;

		image_size = image_get_image_size(hdr);

		if (buffer < (u8 *)hdr + image_size) {
			read_size = (u8 *)hdr + image_size - buffer;
			if (is_power_of_2(page_size))
				read_size = ROUND(read_size, page_size);
			else
				read_size = roundup(read_size, page_size);

			bb_dbg("\nread 0x%x bytes from '%s' offset: 0x%08x\n",
			       read_size, part->name, (u32)offset);
			ret = nand_read_skip_bad_new(nand, offset, &read_size,
					buffer, &bad_blocks);
			if (ret) {
				bb_dbg("nand read fail\n");
				return ret;
			}

			offset += read_size + block_size * bad_blocks;
			buffer += read_size;
		}

		ret = rda_image_verify_data(hdr);
		if (ret)
			return ret;

		if (i == 0) {
			ih_len = min(sizeof(buf), IH_NMLEN);
			strncpy(buf, (const char *)hdr->ih_name, ih_len);
			buf[ih_len - 1] = '\0';
		}
		if (i == 0 && strstr(buf, "raminit")) {
			bb_dbg("## Raminit Image Detected\n");
			bb_dbg("\n");
			*hdrInit = hdr;
		} else {
			bb_dbg("## Work Image Detected\n");
			bb_dbg("\n");
			*hdrWork = hdr;
			break;;
		}
	}

	return CMD_RET_SUCCESS;
}

int rda_modem_bbimages_load_from_mmc(const image_header_t **hdrInit,
		const image_header_t **hdrWork)
{
	u64  offset = 0;
	size_t read_size, image_size;
	u_char *buffer;
	image_header_t *hdr;
	char buf[256];
	u32 ih_len;
	int i;
	int ret;
	disk_partition_t *ptn;
	block_dev_desc_t *mmc_blkdev;
	struct mmc *mmc;

	mmc_blkdev = get_dev_by_name(CONFIG_MMC_DEV_NAME);
	if (mmc_blkdev)
		mmc = container_of(mmc_blkdev, struct mmc, block_dev);
	else
		return CMD_RET_FAILURE;

	if (!mmc) {
                bb_dbg("mmc doesn't exist");
		return CMD_RET_FAILURE;
	}

	ptn = partition_find_ptn("modem");
        if(ptn == 0) {
                bb_dbg("mmc partition table doesn't exist");
                return -1;
        }

	bb_dbg("found modem part  offset: 0x%lx\n", ptn->start);

	bb_dbg("\n## Checking Modem Code Image ...\n\n");

	for (i = 0; i < 2; ++i) {
		if (i == 0) {
			offset = ptn->start * ptn->blksz;
			buffer = (u_char *)SCRATCH_ADDR;
			hdr = (image_header_t *)buffer;
		} else {
			hdr = (image_header_t *)((u8 *)hdr + image_size);
		}

		if (buffer < (u8 *)hdr + sizeof(image_header_t)) {
			read_size = (u8 *)hdr + sizeof(image_header_t) - buffer;
			read_size = ROUND(read_size, 512);

			bb_dbg("read 0x%x bytes  offset: 0x%08x\n",
				read_size, (u32)offset);
			ret = mmc_read(mmc, offset, buffer, read_size);
			if (ret <= 0) {
				bb_dbg("mmc read fail\n");
				return ret;
			}

			offset += read_size;
			buffer += read_size;
		}

		ret = rda_image_verify_header(hdr);
		if (ret)
			return ret;

		image_size = image_get_image_size(hdr);

		if (buffer < (u8 *)hdr + image_size) {
			read_size = (u8 *)hdr + image_size - buffer;
			read_size = ROUND(read_size, 512);

			bb_dbg("\nread 0x%x bytes offset: 0x%08x\n",
			       read_size, (u32)offset);
			ret = mmc_read(mmc, offset, buffer, read_size);
			if (ret <= 0) {
				bb_dbg("mmc read fail\n");
				return ret;
			}

			offset += read_size;
			buffer += read_size;
		}

		ret = rda_image_verify_data(hdr);
		if (ret)
			return ret;

		if (i == 0) {
			ih_len = min(sizeof(buf), IH_NMLEN);
			strncpy(buf, (const char *)hdr->ih_name, ih_len);
			buf[ih_len - 1] = '\0';
		}
		if (i == 0 && strstr(buf, "raminit")) {
			bb_dbg("## Raminit Image Detected\n");
			bb_dbg("\n");
			*hdrInit = hdr;
		} else {
			bb_dbg("## Work Image Detected\n");
			bb_dbg("\n");
			*hdrWork = hdr;
			break;;
		}
	}

	return CMD_RET_SUCCESS;
}

void rdaHexPuts(char* p, int l)
{
	char strBuf[64];
	char* s = strBuf;
	if (l > 20)
		l = 20;

	while (l--) {
		unsigned char c = (unsigned char) *p++;
		*s = ((c >> 4) & 0xF) + '0';
		if (*s > '9')
			*s = *s + 'A' - '9' - 1;

		*++s = (c & 0xF) + '0';
		if (*s > '9')
			*s = *s + 'A' - '9' - 1;

		*++s = ' ';
		++s;
	}
	*s = '\r';
	*++s = '\n';
	*++s = 0;
	puts((const char *) strBuf);
}

void rda_modem_cal_test(void)
{
	char cmdbuf[16];
	unsigned long long end_time = get_ticks();

	while (1) {
#if defined(CONFIG_HW_WATCHDOG) || defined(CONFIG_WATCHDOG)
		WATCHDOG_RESET();
#endif
		/* check Ctrl-C */
		ctrlc();
		if ((had_ctrlc())) {
			bb_dbg("\n## ABORT MANUALLY!\n\n");
			return;
		}

		if (end_time < get_ticks()) {
			char* p = cmdbuf;
			*p++ = 0xad;
			*p++ = 0x00;
			*p++ = 0x0a;
			*p++ = 0xFF;
			*p++ = 0x82;
			*p++ = 0x00;
			*p++ = 0x00;
			*p++ = 0x00;
			*p++ = 0x82;
			*p++ = (char)((end_time >> 0) & 0xFF);
			*p++ = (char)((end_time >> 8) & 0xFF);
			*p++ = (char)((end_time >> 16) & 0xFF);
			*p++ = (char)((end_time >> 24) & 0xFF);
			*p++ = 0xFF;
			bb_dbg("## Send write cmd to modem: ");
			rdaHexPuts(cmdbuf, rda_mdcom_channel_buf_send_stream(
					   RDA_MDCOM_CHANNEL_TRACE, cmdbuf, 14, 0));

			p = cmdbuf;
			*p++ = 0xad;
			*p++ = 0x00;
			*p++ = 0x07;
			*p++ = 0xFF;
			*p++ = 0x02;
			*p++ = 0x00;
			*p++ = 0x00;
			*p++ = 0x00;
			*p++ = 0x82;
			*p++ = (char)((end_time >> 24) & 0xFF);
			*p++ = 0xFF;
			bb_dbg("## Send read cmd to modem: ");
			rdaHexPuts(cmdbuf, rda_mdcom_channel_buf_send_stream(
					   RDA_MDCOM_CHANNEL_TRACE, cmdbuf, 11, 0));

			end_time = get_ticks() + usec2ticks(20000);
		}

		if (rda_mdcom_tstc(RDA_MDCOM_CHANNEL_TRACE)) {
			bb_dbg("## received from modem:");
			do {
				rdaHexPuts(cmdbuf, rda_mdcom_channel_buf_recv_stream(
						   RDA_MDCOM_CHANNEL_TRACE, cmdbuf,
						   sizeof(cmdbuf), 0));
			} while (rda_mdcom_tstc(RDA_MDCOM_CHANNEL_TRACE));
		}
	}
}

static void rda_show_modem_exception(void)
{
	char *addr;
	unsigned int len;
	char buf[256];

	rda_mdcom_get_modem_exception_info((u32 *)&addr, &len);
	snprintf(buf, sizeof(buf), "%s\n", addr);
	buf[sizeof(buf) - 1] = '\0';
	puts("\n");
	if (addr) {
		puts("Modem exception info:\n");
		puts("---------------------\n");
		puts(buf);
		puts("---------------------\n");
	} else {
		puts("No valid modem exception info\n");
	}
}

extern void shutdown_system(void);

static void rda_modem_shutdown_system(void)
{
	struct rda_sys_hdr hdr;

	bb_dbg("\n## System is being shutdown ... ");

	hdr.magic = 0xA8B1;
	hdr.mod_id = 0x0;
	hdr.msg_id = 0x1001;
	hdr.req_id = 0x0;
	hdr.ret_val = 0x0;
	hdr.ext_len = 0;

	rda_mdcom_channel_buf_send_dgram(
		RDA_MDCOM_CHANNEL_SYSTEM, &hdr, sizeof(hdr),
		usec2ticks(100000));
	rda_mdcom_line_set(RDA_MDCOM_PORT1, 2);
	/* Wait sometime until modem shutdowns the system (about 1.8s) */
	udelay(5000000);
	/* Modem is in trouble. Shutdown the system directly. */
	bb_dbg("\n## Bootloader directly shutdown ... ");
	shutdown_system();
}

int bbimages_get_header(const u8 *p_image, const image_header_t **hdrInit,
		const image_header_t **hdrWork)
{
	int retVal;
	char buf[256];
	u32 ih_len;
	int i;
	image_header_t *hdr;

	for (i = 0; i < 2; i++) {
		hdr = (image_header_t *) RDA_ADD_ROUNDUP((int) p_image);

		bb_dbg("\n## Checking Modem Code Image at %08x ...\n",
				(unsigned int)hdr);
		retVal = rda_image_verify(hdr);
		if (retVal) {
			/* If there is no image, we initialize channels
			 * still.  Otherwise kernel would not be
			 * loaded. */
			rda_mdcom_channel_all_init();
			return retVal;
		}

		if (i == 0) {
			ih_len = min(sizeof(buf), IH_NMLEN);
			strncpy(buf, (const char *)hdr->ih_name, ih_len);
			buf[ih_len - 1] = '\0';
		}
		if (i == 0 && strstr(buf, "raminit")) {
			bb_dbg("## Raminit Image Detected\n");
			p_image += image_get_image_size(hdr);
			*hdrInit = hdr;
		} else {
			bb_dbg("## Work Image Detected\n");
			*hdrWork = hdr;
			break;
		}
	}

	return CMD_RET_SUCCESS;
}

int mdcom_check_and_wait_modem(int check_boot_key)
{
	int shutdown = 0;
	unsigned long long end_time;
	u32 reset_cause;
	u32 itf_version;

	/* Set a timeout of 2 seconds to wait for BP */
	bb_dbg("\n## Waiting for modem response ... ");

	end_time = get_ticks() + usec2ticks(2000000);
	while (!rda_mdcom_line_set_wait(RDA_MDCOM_PORT0,
				RDA_MDCOM_LINE_CMD_START, 100)) {
		/* check exception */
		if (rda_mdcom_line_set_check(RDA_MDCOM_PORT0,
					RDA_MDCOM_LINE_EXCEPTION)) {
			bb_dbg("\n\n");
			bb_dbg("**********************************\n");
			bb_dbg("*** Error: Detect modem exception!\n");
			rda_show_modem_exception();
#ifdef REBOOT_WHEN_CRASH
			bb_dbg("Rebooting ...\n\n\n");
			udelay(5000);
			disable_interrupts();
			if (ROLLBACK_TO_RECOVERY_MODE)
				rda_reboot(REBOOT_TO_RECOVERY_MODE);
			else
				rda_reboot(REBOOT_TO_NORMAL_MODE);
#endif
			return CMD_RET_FAILURE;
		}
		/* check timeout */
		if (get_ticks() > end_time) {
#ifdef REBOOT_WHEN_CRASH
			bb_dbg("\n\n## Error: Timeout when waiting for "
			       "modem response. Rebooting ...\n\n\n");
			udelay(5000);
			disable_interrupts();
			if (ROLLBACK_TO_RECOVERY_MODE)
				rda_reboot(REBOOT_TO_RECOVERY_MODE);
			else
				rda_reboot(REBOOT_TO_NORMAL_MODE);
#else
			end_time = -1;
			bb_dbg("\n\n## Warning: Timeout when waiting for "
			       "modem response. Press Ctrl-C to abort.\n");
#endif
		}
		/* check Ctrl-C */
		ctrlc();
		if ((had_ctrlc())) {
			bb_dbg("\n## ABORT MANUALLY!\n\n");
			return CMD_RET_FAILURE;
		}
	}
	bb_dbg("Done\n");

	reset_cause = rda_mdcom_get_reset_cause();
	itf_version = rda_mdcom_get_interface_version();
	bb_dbg("\n## Reset cause : 0x%08x\n", reset_cause);
	bb_dbg("\n## Communication interface version : 0x%08x\n", itf_version);
	if (itf_version != RDA_BP_VER) {
		bb_dbg("*** Error: Unsupported version. 0x%08x is expected.\n",
				RDA_BP_VER);
		return CMD_RET_FAILURE;
	}

	if (reset_cause == RDA_RESET_CAUSE_NORMAL) {
		/* Check power-on key status */
		if (check_boot_key && !get_saved_boot_key_state()) {
			shutdown = 0;
			bb_dbg("\n## Power-on key is not pressed for normal boot.");
			bb_dbg("\n## ****** Shutdown is needed later ******\n");
		}
	}

	bb_dbg("\n## Init mdcom channels ... ");
	rda_mdcom_channel_all_init();
	rda_mdcom_line_clear(RDA_MDCOM_PORT0, RDA_MDCOM_LINE_CMD_START);
	bb_dbg("Done\n");

	/* Check if shutdown is needed after modem fully starts up */
	if (shutdown)
		rda_modem_shutdown_system();

	/* rda_mdcom_show_software_version(); */
	return CMD_RET_SUCCESS;
}

static int do_check_and_wait_modem(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	int check_boot_key = 0;

	if (argc >= 2)
		check_boot_key = (int) simple_strtoul(argv[1], NULL, 10);

	return mdcom_check_and_wait_modem(check_boot_key);
}

int bbimages_load(const image_header_t *hdrInit, const image_header_t *hdrWork,
		int cal_en)
{
	unsigned long long end_time;
	u32 addr, len;
	char buf[256];
	int update_calib = 0, update_factory = 0, update_ap_factory = 0;
	u32 modem_load_addr = image_get_load(hdrWork);

	/* set modem logic base address for modem side,
	   by the work code loading address in image header */
	rda_mdcom_set_logic_base_addr(modem_load_addr);

	bb_dbg("\n## Init mdcom ports ... ");
	rda_mdcom_init_port(RDA_MDCOM_PORT0);
	rda_mdcom_init_port(RDA_MDCOM_PORT1);
	bb_dbg("Done\n");

	if (hdrInit) {
		bb_dbg("\n## Load Raminit Code to modem at ");
		rda_modem_image_load(hdrInit, 0);

		bb_dbg("\n## Start modem and waiting for response ... ");
		rda_mdcom_line_set(RDA_MDCOM_PORT0, RDA_MDCOM_LINE_DL_HANDSHAKE);
		/* Set a timeout of 2 seconds to wait for BP */
		end_time = get_ticks() + usec2ticks(2000000);
		while (!rda_mdcom_line_set_wait(RDA_MDCOM_PORT0,
					RDA_MDCOM_LINE_DL_HANDSHAKE, 100)) {
			/* check timeout */
			if (get_ticks() > end_time) {
#ifdef REBOOT_WHEN_CRASH
				bb_dbg("\n\n## Error: Timeout when waiting for "
				       "modem response. Rebooting ...\n\n\n");
				udelay(5000);
				disable_interrupts();
				if (ROLLBACK_TO_RECOVERY_MODE)
					rda_reboot(REBOOT_TO_RECOVERY_MODE);
				else
					rda_reboot(REBOOT_TO_NORMAL_MODE);
#else
				end_time = -1;
				bb_dbg("\n\n## Warning: Timeout when waiting for "
				       "modem response. Press Ctrl-C to abort.\n");
#endif
			}
			/* check Ctrl-C */
			ctrlc();
			if ((had_ctrlc())) {
				bb_dbg("\n## ABORT MANUALLY!\n\n");
				return CMD_RET_FAILURE;
			}
		}
		rda_mdcom_line_clear(RDA_MDCOM_PORT0, RDA_MDCOM_LINE_DL_HANDSHAKE);
		bb_dbg("Done\n");
	}

	/* Check whether modem crashed in the last run */
	if (rda_mdcom_modem_crashed_before()) {
		rda_mdcom_get_modem_exception_info(&addr, &len);
		buf[0] = '\0';
		if (addr) {
			snprintf(buf, sizeof(buf), "%s", (char *)addr);
			buf[sizeof(buf) - 1] = '\0';
			/* TODO: Save the exception info to nand/sdmmc */
		}
		rda_mdcom_get_modem_log_info(&addr, &len);
		if (addr) {
			/* TODO: Save the log info to nand/sdmmc */
			/* NOTE: The log is in binary format! */
		}
	}
	/* Check whether to udpate modem calib and factory data */
	if (system_rebooted()) {
		update_calib = rda_mdcom_calib_update_cmd_valid();
		if (update_calib) {
			bb_dbg("\n## Detect calib update command\n");
			rda_mdcom_get_calib_section(&addr, &len);
			factory_set_modem_calib((unsigned char *)addr);
			rda_mdcom_get_ext_calib_section(&addr, &len);
			factory_set_modem_ext_calib((unsigned char *)addr);
		}
		update_factory = rda_mdcom_factory_update_cmd_valid();
		if (update_factory) {
			bb_dbg("\n## Detect factory update command\n");
			rda_mdcom_get_factory_section(&addr, &len);
			factory_set_modem_factory((unsigned char *)addr);
		}
		update_ap_factory = rda_mdcom_ap_factory_update_cmd_valid();
		if (update_ap_factory) {
			bb_dbg("\n## Detect AP factory update command\n");
			rda_mdcom_get_ap_factory_section(&addr, &len);
			factory_set_ap_factory((unsigned char *)addr);
		}
		if (update_calib || update_factory || update_ap_factory) {
			bb_dbg("\n## Burn calib and/or factory data\n");
			if (factory_burn())
				bb_dbg("\n** Error when burning data\n");
		}
	}
	/* Init all log info and some magic numbers */
	rda_mdcom_init_all_log_info();

	bb_dbg("\n## Load Work Code to modem at ");
	if (rda_modem_image_load(hdrWork, cal_en)) {
		printf("\n## Load modem failed!\n");
		return CMD_RET_FAILURE;
	}

	bb_dbg("\n## Load Calib Data to modem at ");
	rda_modem_cal_load();

	bb_dbg("\n## Start modem ...\n");
	rda_mdcom_line_set(RDA_MDCOM_PORT0, RDA_MDCOM_LINE_DL_HANDSHAKE);

	return CMD_RET_SUCCESS;
}

int mdcom_load_from_mem(const u8 *data, int cal_en)
{
	const image_header_t *hdrInit = NULL;
	const image_header_t *hdrWork = NULL;

	if (bbimages_get_header(data, &hdrInit, &hdrWork)) {
		/* If there is no image, we initialize channels still.
		 * Otherwise kernel would not be loaded. */
		rda_mdcom_channel_all_init();
		return CMD_RET_FAILURE;
	}

	return bbimages_load(hdrInit, hdrWork, cal_en);
}

int mdcom_load_from_flash(int cal_en)
{
	int ret;
	const image_header_t *hdrInit = NULL;
	const image_header_t *hdrWork = NULL;

	/*
	 * Save the boot key state now.
	 * It takes about 1.9s to run into here if boot delay is 1s.
	 */
	save_current_boot_key_state();

	if (rda_media_get() == MEDIA_MMC)
		ret = rda_modem_bbimages_load_from_mmc(&hdrInit, &hdrWork);
	else
		ret = rda_modem_bbimages_load_from_nand(&hdrInit, &hdrWork);

	if (ret) {
		/*
		 * If there is no image, we initialize channels still.
		 * Otherwise kernel would not be loaded.
		 */
		rda_mdcom_channel_all_init();
		return CMD_RET_FAILURE;
	}
	return bbimages_load(hdrInit, hdrWork, cal_en);
}

/*
 * Syntax:
 *  mdcom_loadm {adress} [cal_en]
 */
int do_mdcom_loadm(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	int cal_en = 0;
	const u8 *data;

	if (argc < 2)
		return CMD_RET_USAGE;
	if (argc >= 3)
		cal_en = (int) simple_strtoul(argv[2], NULL, 10);

	data = (const u8 *)simple_strtoul(argv[1], NULL, 16);

	return mdcom_load_from_mem(data, cal_en);
}

int do_mdcom_cal_loadm(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	const u8 *data;
	if (argc != 2)
		return CMD_RET_USAGE;

	data = (const u8 *)simple_strtoul(argv[1], NULL, 16);
	if (rda_image_verify_header((const image_header_t *)data))
		return CMD_RET_FAILURE;

	if (factory_copy_from_mem(data))
		return CMD_RET_FAILURE;

	return CMD_RET_SUCCESS;
}

/*
 * Syntax:
 *  mdcom_loadf [cal_en]
 */
int do_mdcom_loadf(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	int cal_en = 0;

	if (argc >= 2)
		cal_en = (int) simple_strtoul(argv[1], NULL, 10);

	return mdcom_load_from_flash(cal_en);
}

int do_mdcom_cal_test(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	rda_modem_cal_test();
	return CMD_RET_SUCCESS;
}

int do_mdcom_cal(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	drv_usbser_init();
	rda_modem_cal_loop();
	return CMD_RET_SUCCESS;
}

int do_mdcom_ch_show(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	bb_dbg("\n## MDCOM Channel AT Show:\n");
	rda_mdcom_channel_show(RDA_MDCOM_CHANNEL_AT);
	bb_dbg("\n## MDCOM Channel SYS Show:\n");
	rda_mdcom_channel_show(RDA_MDCOM_CHANNEL_SYSTEM);
	bb_dbg("\n## MDCOM Channel TRACE Show:\n");
	rda_mdcom_channel_show(RDA_MDCOM_CHANNEL_TRACE);
	return CMD_RET_SUCCESS;
}

int do_mdcom_cal_save(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	if (rda_modem_cal_save())
		return CMD_RET_FAILURE;
	else
		return CMD_RET_SUCCESS;
}

int do_mdcom_cal_load(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	if (rda_modem_cal_load())
		return CMD_RET_FAILURE;
	else
		return CMD_RET_SUCCESS;
}

int do_mdcom_diag(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	rda_mdcom_show_xcpu_info();
	return CMD_RET_SUCCESS;
}

int do_mdcom_ver(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	rda_mdcom_show_software_version();
	return CMD_RET_SUCCESS;
}

U_BOOT_CMD(
	mdcom_send ,    6,    1,     do_mdcom_send,
	"send data to mdCom channels",
	"Syntax:\n"
	"    - mdcom_send {channel} {adress} {size} [{timeout} {mode}]"
	"Parameters:\n"
	"    - channel:  the mdcom channel number\n"
	"    - adress:   the address of the data buffer to send\n"
	"    - size: 	 the size of the sending buffer\n"
	"    - timeout:  (optional) the timeout with unit ms\n"
	"    - mode: 	 (optional) 0 - stream mode; else - dgram mode\n"
);

U_BOOT_CMD(
	mdcom_recv ,    6,    1,     do_mdcom_recv,
	"send data to mdCom channels",
	"Syntax:\n"
	"    - mdcom_recv {channel} {address} {size} [{timeout} {mode}]\n"
	"Parameters:\n"
	"    - channel:  the mdcom channel number\n"
	"    - adress:   the address of the buffer to receive data\n"
	"    - size: 	 the size of the receiving buffer\n"
	"    - timeout:  (optional) the timeout with unit ms\n"
	"    - mode: 	 (optional) 0 - stream mode; else - dgram mode\n"
);

U_BOOT_CMD(
	mdcom_cal_loadm ,    2,    1,     do_mdcom_cal_loadm,
	"load modem calibration data from memory",
	"Syntax:\n"
	"    - mdcom_cal_loadm {address} \n"
	"Parameters:\n"
	"    - address:        the address of the buffer to receive data\n"
);

U_BOOT_CMD(
	mdcom_loadm ,    3,    1,     do_mdcom_loadm,
	"load modem codes from memory",
	"Syntax:\n"
	"    - mdcom_loadm {adress} [{cal_en}] \n"
	"Parameters:\n"
	"    - address:        the address of the buffer to receive data\n"
	"    - cal_en:         enable calibration\n"
);

U_BOOT_CMD(
	mdcom_loadf ,    2,    1,     do_mdcom_loadf,
	"load modem codes from flash",
	"Syntax:\n"
	"    - mdcom_loadf [{cal_en}] \n"
	"Parameters:\n"
	"    - cal_en:   enable calibration\n"
);

U_BOOT_CMD(
	mdcom_calt ,    1,    1,     do_mdcom_cal_test,
	"modem calibration test",
	"Syntax:\n"
	"    - mdcom_calt\n"
);

U_BOOT_CMD(
	mdcom_cal ,    1,    1,     do_mdcom_cal,
	"modem calibration",
	"Syntax:\n"
	"    - mdcom_cal\n"
);

U_BOOT_CMD(
	mdcom_show ,    1,    1,     do_mdcom_ch_show,
	"show mdcom all channels",
	"Syntax:\n"
	"    - mdcom_show\n"
);

U_BOOT_CMD(
	mdcom_cal_save ,    1,    1,     do_mdcom_cal_save,
	"modem calibration data save",
	"Syntax:\n"
	"    - mdcom_cal_save\n"
);

U_BOOT_CMD(
	mdcom_cal_load ,    1,    1,     do_mdcom_cal_load,
	"modem calibration data load",
	"Syntax:\n"
	"    - mdcom_cal_load\n"
);

U_BOOT_CMD(
	mdcom_diag,    1,    1,     do_mdcom_diag,
	"Show modem XCPU diagostic information",
	"Syntax:\n"
	"    - mdcom_diag\n"
);

U_BOOT_CMD(
	mdcom_ver,    1,    1,     do_mdcom_ver,
	"Show modem software versions",
	"Syntax:\n"
	"    - mdcom_ver\n"
);

U_BOOT_CMD(
	mdcom_check,    2,    1,     do_check_and_wait_modem,
	"check modem status, may wait",
	"Syntax:\n"
	"    - mdcom_check {[check_boot_key]}\n"
	"Parameters:\n"
	"    - check_boot_key: whether to check boot key status\n"
);

