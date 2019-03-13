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
#include <linux/mtd/nand.h>
#include <mtd/nand/rda_nand.h>
#include <android/android_boot.h>
#include <android/android_bootimg.h>
#include <rda/tgt_ap_panel_setting.h>
#include <asm/arch/spl_board_info.h>

#ifdef FLASH_PAGE_SIZE
#undef FLASH_PAGE_SIZE
#endif
#define FLASH_PAGE_SIZE 2048

#define rda_dbg(fmt, args...) printf(fmt, ##args)

extern int mtdparts_init_default(void);
extern int find_dev_and_part(const char *id, struct mtd_device **dev,
			     u8 *part_num, struct part_info **part);

static int rollback_to_recovery_mode = 0;

int do_sleep(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	ulong start = get_timer(0);
	ulong delay;

	if (argc != 2)
		return CMD_RET_USAGE;

	delay = simple_strtoul(argv[1], NULL, 10) * CONFIG_SYS_HZ;

	while (get_timer(start) < delay) {
		if (ctrlc())
			return (-1);

		udelay(100);
	}

	return 0;
}

int do_flush_dcache(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	unsigned long start_addr = 0;
	unsigned long end_addr = 0;

	if (argc < 3) {
		printf("do flush dcache all...\n");
		flush_dcache_all();
		return 0;
	}

	start_addr = simple_strtoul(argv[1], NULL, 16);
	end_addr = simple_strtoul(argv[2], NULL, 16);

	printf("flush dcache %#x -> %#x\n", (unsigned int)start_addr, (unsigned int)end_addr);
	flush_dcache_range(start_addr, end_addr);
	return 0;
}


int do_mytest(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	ulong usec;

	if (argc != 2)
		return CMD_RET_USAGE;

	usec = simple_strtoul(argv[1], NULL, 10) * 1000000;

	udelay(usec);

	return 0;
}

int do_rdaswcfg(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	u16 swcfg;

	if (argc != 2)
		return CMD_RET_USAGE;

	swcfg = (u16)simple_strtoul(argv[1], NULL, 10);
	rda_swcfg_reg_set(swcfg);

	return 0;
}

int do_rdahwcfg(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	printf("RDA: HW_CFG 0x%04x\n", rda_hwcfg_get());
	return 0;
}

int do_rdabm(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	printf("RDA: Boot_Mode %d\n", rda_bm_get());
	return 0;
}

int do_rdabminit(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	rda_bm_init();

	return 0;
}

static int get_lcd_name(void)
{
	char lcd_paras[50] = "lcd=";
	char *lcd_name = (char *)factory_get_lcd_name();

#define PANEL_PARM	"lcd="PANEL_NAME

	setenv("lcd", PANEL_PARM);
	if (!lcd_name || strlen(lcd_name) == 0) {
		rda_dbg("factorydata does not contain lcd name, using default\n");
		return 0;
	}

	if (strstr(RDA_PANEL_SUPPORT_LIST, lcd_name) == NULL) {
		rda_dbg("RDA does not support %s lcd panel\n", lcd_name);
		rda_dbg("Using default, please contact RDA.n");
		return 0;
	}

	strncat(lcd_paras, lcd_name, 50);
	setenv("lcd", lcd_paras);
	return 0;
}


static int get_bootlogo_name(void)
{
	char logo_paras[50] = "bootlogo=";
#ifdef CONFIG_MACH_RDA8810E//need debug in future
	char *name = NULL;//(char *)factory_get_bootlogo_name();
#else
	char *name = (char *)factory_get_bootlogo_name();
#endif

	if (!name || strlen(name) == 0) {
		rda_dbg("Does not find bootlogo name, using default\n");
		return 0;
	}

	strncat(logo_paras, name, 50);
	setenv("bootlogo", logo_paras);
	return 0;
}

int do_get_bootlogo_name(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	if (get_bootlogo_name())
		return CMD_RET_FAILURE;
	else
		return CMD_RET_SUCCESS;
}

int do_ap_factory_use(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	u8* addr;

	if (argc != 2)
		return CMD_RET_USAGE;
	addr = (u8*)simple_strtoul(argv[1], NULL, 16);

	if (factory_set_ap_factory(addr))
		return CMD_RET_FAILURE;
	else
		return CMD_RET_SUCCESS;
}

int do_ap_factory_update(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	u8* addr;

	if (argc != 2)
		return CMD_RET_USAGE;
	addr = (u8*)simple_strtoul(argv[1], NULL, 16);

	if (factory_update_ap_factory(addr))
		return CMD_RET_FAILURE;
	else
		return CMD_RET_SUCCESS;
}

int do_get_lcd_name(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	if (get_lcd_name())
		return CMD_RET_FAILURE;
	else
		return CMD_RET_SUCCESS;
}

int do_get_emmc_id(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	struct spl_emmc_info * emmc_info = get_bd_spl_emmc_info();
	char emmc_id[16] = {0};

	if(CONFIG_MMC_DEV_NUM == 0)
		return CMD_RET_SUCCESS;

	sprintf(emmc_id, "emmc_id=%d", emmc_info->manufacturer_id);

	setenv("emmc_id", emmc_id);

	return CMD_RET_SUCCESS;
}

extern int rda_flash_intf_is_spi(void);

static int do_get_flash_intf(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	if (rda_media_get() == MEDIA_MMC ) {
#ifdef CONFIG_SDMMC_BOOT
		setenv("flash_if", "flash_if=sdcard");
#else
		setenv("flash_if", "flash_if=emmc");
#endif
	} else {
		if(rda_flash_intf_is_spi())
			setenv("flash_if", "flash_if=spi");
		else
			setenv("flash_if", "flash_if=normal");
	}

	return CMD_RET_SUCCESS;
}

extern int hal_BoardSetup(void);

static int do_board_mux_config(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
#ifdef CONFIG_RDA_MUX_CONFIG
	int ret = 0;
	ret = hal_BoardSetup();
	if(ret == CMD_RET_SUCCESS)
		puts("Board Mux : Done. \n");
	else
		puts("Board Mux : Error. \n");

	return ret;
#else
	puts("Board Mux : Bypass bootloader board mux config. \n");
	return CMD_RET_SUCCESS;
#endif
}

static int do_get_android_bm(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	enum rda_bm_type bm;
	u32 reset_cause;

	setenv("chargerboot", "0");
	bm = rda_bm_get();
	switch (bm) {
	case RDA_BM_RECOVERY:
		setenv("androidboot", "androidboot.mode=recovery");
		break;
	case RDA_BM_FACTORY:
		setenv("androidboot", "androidboot.mode=factory");
		break;
	default:
		reset_cause = rda_mdcom_get_reset_cause();
		if (reset_cause == RDA_RESET_CAUSE_CHARGER) {
			setenv("androidboot", "androidboot.mode=charger");
#ifdef CHARGER_IN_RECOVERY
/* only move charger to recovery partition after aosp 4.4 (including)*/
			setenv("chargerboot", "1");
#endif
		} else {
			setenv("androidboot", "androidboot.mode=normal");
		}
		break;
	}

	return CMD_RET_SUCCESS;
}

static u8 *recovery_image = (u8 *)SCRATCH_ADDR;

static int load_recovery_from_nand(void)
{
	void *data = (void *)recovery_image;
	struct mtd_info *nand;
	struct mtd_device *dev;
	struct part_info *part;
	size_t size = 0;
	u8 pnum;
	int ret;

	ret = find_dev_and_part("recovery", &dev, &pnum, &part);
	if (ret) {
		serial_printf("unknown partition name");
		return CMD_RET_FAILURE;
	} else if (dev->id->type != MTD_DEV_TYPE_NAND) {
		serial_printf("mtd dev type error");
		return CMD_RET_FAILURE;
	}
	nand = &nand_info[dev->id->num];
	serial_printf("found part '%s' offset: 0x%llx length: 0x%llx\n",
		part->name, part->offset, part->size);

	size = part->size;

	serial_printf("read 0x%x bytes from '%s' offset: 0x%llx\n",
		size, part->name, part->offset);
	ret = nand_read_skip_bad(nand, part->offset, &size, (u_char *) data);
	if (ret) {
		serial_printf("nand read fail");
		return CMD_RET_FAILURE;
	}

	return CMD_RET_SUCCESS;
}

static int load_recovery_from_mmc(void)
{
	void *data = (void *)recovery_image;
	disk_partition_t *ptn;
	block_dev_desc_t *mmc_blkdev;
	loff_t size = 0;

	serial_printf("%s\n", __func__);
	mmc_blkdev = get_dev_by_name(CONFIG_MMC_DEV_NAME);
	ptn = partition_find_ptn("recovery");

	if (ptn == 0) {
		serial_printf("mmc partition table doesn't exist");
		puts("Board Mux : Done. \n");
		return CMD_RET_FAILURE;
	}

	size = ptn->size * ptn->blksz;

	serial_printf("read 0x%x bytes from recovery offset: %lx\n", (unsigned int)size,
		ptn->start);
	if (partition_read_bytes(mmc_blkdev, ptn, &size, data)) {
		serial_printf("mmc read failure");
		return CMD_RET_FAILURE;
	}

	return CMD_RET_SUCCESS;
}

static int check_recovery_image(void)
{
	boot_img_hdr *hdr;

	/* Check recovery image header */
	hdr = (boot_img_hdr *)recovery_image;
	if (memcmp(hdr->magic, BOOT_MAGIC, BOOT_MAGIC_SIZE)) {
		serial_printf("Invalid magic\n");
		return CMD_RET_FAILURE;
	}

	if (hdr->page_size != FLASH_PAGE_SIZE) {
		serial_printf("Invalid page size: %d (expecting %d)\n",
				hdr->page_size, FLASH_PAGE_SIZE);
		return CMD_RET_FAILURE;
	}

	return CMD_RET_SUCCESS;
}


static int load_ap_recovery(void)
{
	int ret = CMD_RET_SUCCESS;
	enum media_type media;

	media = rda_media_get();
	if (media == MEDIA_MMC)
		ret = load_recovery_from_mmc();
	else if ((media == MEDIA_NAND) || media == MEDIA_SPINAND)
		ret = load_recovery_from_nand();
	else {
		serial_printf("%s can't find boot media\n", __func__);
		ret = CMD_RET_FAILURE;
	}

	if (ret != CMD_RET_SUCCESS) {
		serial_printf("%s failed, ret is %d\n", __func__, ret);
		return ret;
	}
	ret = check_recovery_image();

	return ret;
}

/*
 * For moving charger to recovery image to save 500KB in ramdisk
 *     CHARGER BOOT mode will need to load recovery kernel and rootfs
 * 1. load modem from boot image and normal boot image first, so no extra delay
 *    for normal boot. (rda_mdcom_get_reset_cause only works after modem is up)
 * 2. after modem is up and CHARGER MODE detected, load recovery image
 */
static int do_load_ap_recovery(cmd_tbl_t *cmdtp, int flag, int argc,
		                   char * const argv[])
{
    return load_ap_recovery();
}

extern int mdcom_load_from_flash(int cal_en);
extern int mdcom_load_from_mem(const u8 *data, int cal_en);
extern int mdcom_check_and_wait_modem(int check_boot_key);

static int do_load_recovery(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	int ret = CMD_RET_SUCCESS;
	enum rda_bm_type bm;
	boot_img_hdr *hdr;
	unsigned kernel_actual;
	unsigned ramdisk_actual;
	unsigned second_actual;
	unsigned size;

	/* Load recovery modem only in recovery mode, and
	 * load normal modem in factory mode */
	bm = rda_bm_get();
	if (bm != RDA_BM_RECOVERY) {
		rollback_to_recovery_mode = 1;
		ret = mdcom_load_from_flash(0);
		if (ret != CMD_RET_SUCCESS) {
			serial_printf("Bad modem image. Try recovery mode\n");
			udelay(5000);
			rda_reboot(REBOOT_TO_RECOVERY_MODE);
		}
	}

	ret = load_ap_recovery();
	if (ret != CMD_RET_SUCCESS)
		return ret;


	if (bm == RDA_BM_RECOVERY) {
		hdr = (boot_img_hdr *)recovery_image;
		kernel_actual = ROUND(hdr->kernel_size, FLASH_PAGE_SIZE);
		ramdisk_actual = ROUND(hdr->ramdisk_size, FLASH_PAGE_SIZE);
		second_actual = ROUND(hdr->second_size, FLASH_PAGE_SIZE);
		size = FLASH_PAGE_SIZE + kernel_actual + ramdisk_actual +
			second_actual;

		ret = mdcom_load_from_mem(recovery_image + size, 0);

		if (ret != CMD_RET_SUCCESS)
			return ret;
	}

	ret = mdcom_check_and_wait_modem(0);
	if (ret != CMD_RET_SUCCESS && bm != RDA_BM_RECOVERY) {
		serial_printf("Modem no response. Try recovery mode\n");
		udelay(5000);
		rda_reboot(REBOOT_TO_RECOVERY_MODE);
	}

	return ret;
}

int load_boot_from_nand(void)
{
	boot_img_hdr *hdr;
	void *data = (void *)SCRATCH_ADDR;
	struct mtd_info *nand;
	struct mtd_device *dev;
	struct part_info *part;
	size_t size = 0;
	u8 pnum;
	unsigned kernel_actual;
	unsigned ramdisk_actual;
	int ret;

	ret = find_dev_and_part("boot", &dev, &pnum, &part);
	if (ret) {
		rda_dbg("unknown partition name\n");
		return CMD_RET_FAILURE;
	} else if (dev->id->type != MTD_DEV_TYPE_NAND) {
		rda_dbg("mtd dev type error\n");
		return CMD_RET_FAILURE;
	}
	nand = &nand_info[dev->id->num];
	rda_dbg("found part '%s' offset: 0x%llx length: 0x%llx\n",
		part->name, part->offset, part->size);

	/* get hdr first */
	size = ROUND(2048, FLASH_PAGE_SIZE);
	rda_dbg("read 0x%x bytes from '%s' offset: 0x%llx\n",
		size, part->name, part->offset);

	ret = nand_read_skip_bad(nand, part->offset, &size, (u_char *) data);
	if (ret) {
		rda_dbg("nand read fail\n");
		return CMD_RET_FAILURE;
	}

	/* get size from hdr */
	hdr = (boot_img_hdr *) data;
	kernel_actual = ROUND(hdr->kernel_size, FLASH_PAGE_SIZE);
	ramdisk_actual = ROUND(hdr->ramdisk_size, FLASH_PAGE_SIZE);
	size = 2048 + kernel_actual + ramdisk_actual;

	/* load whole boot.img */
	rda_dbg("read 0x%x bytes from '%s' offset: 0x%llx\n",
		size, part->name, part->offset);
	ret = nand_read_skip_bad(nand, part->offset, &size, (u_char *) data);
	if (ret) {
		rda_dbg("nand read fail\n");
		return CMD_RET_FAILURE;
	}

	return CMD_RET_SUCCESS;
}

static block_dev_desc_t *mmc_blkdev;
int load_boot_from_mmc(void)
{
	void *data = (void *)SCRATCH_ADDR;
	disk_partition_t *ptn;
	loff_t size = 0;

	rda_dbg("%s\n", __func__);
	mmc_blkdev = get_dev_by_name(CONFIG_MMC_DEV_NAME);
	ptn = partition_find_ptn("boot");

	if (ptn == 0) {
		rda_dbg("mmc partition table doesn't exist\n");
		return CMD_RET_FAILURE;
	}

	size = ptn->size * ptn->blksz;

	rda_dbg("read 0x%x bytes from boot offset: %lx\n", (unsigned int)size,
		ptn->start);
	if (partition_read_bytes(mmc_blkdev, ptn, &size, data)) {
		rda_dbg("mmc read failure\n");
		return CMD_RET_FAILURE;
	}

	return CMD_RET_SUCCESS;
}

static int do_load_boot(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	int ret = CMD_RET_FAILURE;
	enum media_type media;

	media = rda_media_get();
	serial_printf("load boot image ...\n");
	if (media == MEDIA_MMC)
		ret = load_boot_from_mmc();
	else if ((media == MEDIA_NAND) || (media == MEDIA_SPINAND))
		ret = load_boot_from_nand();
	else
		serial_printf("load_boot can't find boot media\n");

	return ret;
}

static int do_adjust_bootdelay(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	int bootmode;

	bootmode = rda_bm_get();
	if (bootmode == RDA_BM_FASTBOOT)
		setenv("bootdelay", "3");

	return CMD_RET_SUCCESS;
}
U_BOOT_CMD(
	sleep ,    2,    1,     do_sleep,
	"delay execution for some time",
	"N\n"
	"    - delay execution for N seconds (N is _decimal_ !!!)"
);

U_BOOT_CMD(
	flush_dcache ,    3,    1,     do_flush_dcache,
	"flush dcache",
	"start_addr end_addr\n"
	"    - flush dcache from [start_addr] to [end_addr]"
);

U_BOOT_CMD(
	mytest ,    2,    1,     do_mytest,
	"exec certain test routine",
	"arg\n"
	"    - exec with arg (arg is _decimal_ !!!)"
);

U_BOOT_CMD(
	rdaswcfg ,    2,    1,     do_rdaswcfg,
	"set rda swcfg",
	"Syntax:\n"
	"    - rdaswcfg swcfg\n"
);

U_BOOT_CMD(
	rdahwcfg ,    1,    1,     do_rdahwcfg,
	"get rda hwcfg",
	"Syntax:\n"
	"    - rdahwcfg\n"
);

U_BOOT_CMD(
	rdabm ,    1,    1,     do_rdabm,
	"get rda boot_mode",
	"Syntax:\n"
	"    - rdabm\n"
);

U_BOOT_CMD(
	rdabminit ,    1,    1,     do_rdabminit,
	"init rda boot_mode",
	"Syntax:\n"
	"    - rdabminit\n"
);

U_BOOT_CMD(
	ap_factory_use ,    2,    1,     do_ap_factory_use,
	"use ap factory data from specified memory",
	"Syntax:\n"
	"    - factory_use addr\n"
);


U_BOOT_CMD(
	ap_factory_update ,    2,    1,     do_ap_factory_update,
	"update ap factory data from specified memory",
	"Syntax:\n"
	"    - factory_update addr\n"
);

U_BOOT_CMD(
	get_lcd_name ,    1,    1,     do_get_lcd_name,
	"ap find lcd panel name",
	"Syntax:\n"
	"    - find_lcd_name\n"
);

U_BOOT_CMD(
	get_flash_intf,    1,    1,     do_get_flash_intf,
	"ap find flash interface",
	"Syntax:\n"
	"    - get_flash_intf\n"
);

U_BOOT_CMD(
	get_emmc_id,    1,    1,     do_get_emmc_id,
	"ap find emmc manufacturer id and pass it to kernel",
	"Syntax:\n"
	"    - get_emmc_id\n"
);

U_BOOT_CMD(
	mux_config,	1,	1,	do_board_mux_config,
	"check and set board mux config (pinmux,io drive,io pin mode...)",
	"Syntax:\n"
	"    - mux_config\n"
);

U_BOOT_CMD(
	get_android_bm,    1,    1,     do_get_android_bm,
	"get android bootmode",
	"Syntax:\n"
	"    - get_android_bm \n"
);

U_BOOT_CMD(
	load_ap_recovery,    1,    1,     do_load_ap_recovery,
	"load recovery kernel and ramdisk to address",
	"Syntax:\n"
	"    - load_ap_recovery address \n"
);

U_BOOT_CMD(
	load_recovery,    1,    1,     do_load_recovery,
	"load recovery image to address",
	"Syntax:\n"
	"    - load_recovery address \n"
);

U_BOOT_CMD(
	load_boot,    1,    1,     do_load_boot,
	"load boot image to address",
	"Syntax:\n"
	"    - load_boot address \n"
);

U_BOOT_CMD(
	get_bootlogo_name ,    1,    1,     do_get_bootlogo_name,
	"find bootlogo name",
	"Syntax:\n"
	"    - get_bootlogo_name\n"
);

U_BOOT_CMD(
	adjust_bootdelay ,    1,    1,     do_adjust_bootdelay,
	"re-adjust bootdelay according to bootmode",
	"Syntax:\n"
	"    - adjust_bootdelay\n"
);
