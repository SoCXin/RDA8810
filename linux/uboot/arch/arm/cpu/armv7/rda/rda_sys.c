#include <common.h>
#include <asm/arch/hardware.h>
#include <asm/io.h>
#include <asm/arch/reg_sysctrl.h>
#include <asm/arch/reg_md_sysctrl.h>
#include <asm/arch/reg_cfg_regs.h>
#include <asm/arch/reg_keypad.h>
#include <asm/arch/reg_gpio.h>
#include <asm/arch/hwcfg.h>
#include <asm/arch/ispi.h>
#include <asm/arch/reg_md_sysctrl.h>
#include <asm/arch/hwcfg.h>
#include <asm/arch/rda_sys.h>
#include <asm/arch/spl_board_info.h>
#ifdef CONFIG_CMD_MISC
#include <usb/usbserial.h>
#include <asm/arch/mdcom.h>
#endif

#define RDA_AP_MBX_HWCFG_SWCFG_ADD	(RDA_MD_MAILBOX_BASE + 0x1AE0)
#define RDA_HWCFG_SWCFG			(*(u32 *)RDA_AP_MBX_HWCFG_SWCFG_ADD)

/*
 * Hardware and software configuration
 */
void hwcfg_swcfg_init(void)
{
	u16 hwcfg, swcfg;

	RDA_HWCFG_SWCFG = hwp_sysCtrlMd->Reset_Cause;

	hwcfg = rda_hwcfg_reg_get();
	swcfg = rda_swcfg_reg_get();

	/* clear the sw boot modes handled by bootloader */
	rda_swcfg_reg_set(swcfg & ~(RDA_SW_CFG_BIT_2 |
				RDA_SW_CFG_BIT_3 |
				RDA_SW_CFG_BIT_4 |
				RDA_SW_CFG_BIT_5 |
				RDA_SW_CFG_BIT_6));

	/* clear the hw boot modes for download/factory modes */
	rda_hwcfg_reg_set(hwcfg & ~(RDA_HW_CFG_BIT_10 |
				RDA_HW_CFG_BIT_11 |
				RDA_HW_CFG_BIT_12 |
				RDA_HW_CFG_BIT_13 |
				RDA_HW_CFG_BIT_14 |
				RDA_HW_CFG_BIT_15));

}

void rda_hwcfg_reg_set(u16 hwcfg)
{
	hwp_sysCtrlMd->Reset_Cause = SET_BITFIELD(hwp_sysCtrlMd->Reset_Cause,
			SYS_CTRL_BOOT_MODE, hwcfg);
	hwp_sysCtrlAp->Reset_Cause = hwp_sysCtrlMd->Reset_Cause;
}

u16 rda_hwcfg_reg_get(void)
{
	u16 hwcfg = GET_BITFIELD(hwp_sysCtrlMd->Reset_Cause, SYS_CTRL_BOOT_MODE);
	return hwcfg;
}

u16 rda_hwcfg_get(void)
{
	u16 hwcfg = GET_BITFIELD(RDA_HWCFG_SWCFG, SYS_CTRL_BOOT_MODE);
	return hwcfg;
}

void rda_swcfg_reg_set(u16 swcfg)
{
	hwp_sysCtrlMd->Reset_Cause = SET_BITFIELD(hwp_sysCtrlMd->Reset_Cause,
			SYS_CTRL_SW_BOOT_MODE, swcfg);
	hwp_sysCtrlAp->Reset_Cause = hwp_sysCtrlMd->Reset_Cause;
}

u16 rda_swcfg_reg_get(void)
{
	u16 swcfg = GET_BITFIELD(hwp_sysCtrlMd->Reset_Cause, SYS_CTRL_SW_BOOT_MODE);
	return swcfg;
}

u16 rda_swcfg_get(void)
{
	u16 swcfg = GET_BITFIELD(RDA_HWCFG_SWCFG, SYS_CTRL_SW_BOOT_MODE);
	return swcfg;
}

u16 rda_prod_id_get(void)
{
	u16 prod_id = GET_BITFIELD(hwp_configRegs->CHIP_ID, CFG_REGS_PROD_ID);
	return prod_id;
}

u16 rda_metal_id_get(void)
{
	u16 metal_id = GET_BITFIELD(hwp_configRegs->CHIP_ID, CFG_REGS_METAL_ID);
	return metal_id;
}

u16 rda_bond_id_get(void)
{
	u16 bond_id = GET_BITFIELD(hwp_configRegs->CHIP_ID, CFG_REGS_BOND_ID);
	return bond_id;
}

void rda_nand_iodrive_set(void)
{
	u32 value = hwp_configRegs->IO_Drive1_Select & (~ CFG_REGS_NFLSH_DRIVE_MASK);

	 hwp_configRegs->IO_Drive1_Select = value | CFG_REGS_NFLSH_DRIVE_SLOW_AND_WEAK;
}

/*
 * System-level control
 */
enum media_type rda_media_get(void)
{
#ifdef CONFIG_SDMMC_BOOT
	return MEDIA_MMC;
#else
	u16 hwcfg = rda_hwcfg_get();
#ifdef CONFIG_MACH_RDA8810H
/*
 * HW BOOT MODE
 *
 * BIT_2   BIT1   BIT0         MODE
 *    0     0       0          emmc
 *    0     0       1          spi nand
 *    0     1       0          spi nor
 *    0     1       1          t-card0(run)
 *    1     0       0          t-card1(update)
 *    1     0       1          nand 8bit
 *    1     1       0          nand 16bit
 *    1     1       1          reserved
 */
	u32 bm_media = RDA_HW_CFG_GET_BM_IDX(hwcfg);

	if (bm_media == RDA_MODE_EMMC)
		return MEDIA_MMC;
	else if (bm_media == RDA_MODE_SPINAND)
		return MEDIA_SPINAND;
	else if ((bm_media == RDA_MODE_NAND_8BIT)
		|| (bm_media == RDA_MODE_NAND_16BIT))
		return MEDIA_NAND;
#else
	u16 metal_id = rda_metal_id_get();
	u16 prod_id = rda_prod_id_get();
	u16 metal_new_bm;

	if (prod_id == 0x8810)
		metal_new_bm = 0xB;
	else if (prod_id == 0x810E || prod_id == 0x8850 || prod_id == 0x850E)
		metal_new_bm = 0x2;
	else
		metal_new_bm = 0;

	if (metal_id < metal_new_bm) {
		/* SDMMC or SPI NAND */
		if (hwcfg & RDA_HW_CFG_BIT_3) {
			if (hwcfg & RDA_HW_CFG_BIT_7)
				return MEDIA_MMC;
			else
				return MEDIA_NAND;
		} else if (hwcfg & RDA_HW_CFG_BIT_4) {
			return MEDIA_MMC;
		}
		/* EMMC */
		if (hwcfg & RDA_HW_CFG_BIT_2)
			return MEDIA_MMC;
		/* PARALLEL NAND */
		if (!(hwcfg & RDA_HW_CFG_BIT_4))
			return MEDIA_NAND;

	} else {
		/* SPI NAND */
		if (hwcfg & RDA_HW_CFG_BIT_3)
			return MEDIA_NAND;
		/* EMMC */
		if (hwcfg & RDA_HW_CFG_BIT_2)
			return MEDIA_MMC;
		/* PARALLEL NAND */
		if (!(hwcfg & RDA_HW_CFG_BIT_4))
			return MEDIA_NAND;
	}
#endif
#endif
	return MEDIA_UNKNOWN;
}

void reset_cpu(ulong addr)
{
#ifdef CONFIG_RDA_PDL
	enable_charger(1);
#endif

	while (1) {
		/* to unlock first */
		hwp_sysCtrlMd->REG_DBG = 0xa50001;
		/* reset */
		hwp_sysCtrlMd->Sys_Rst_Set |= SYS_CTRL_SOFT_RST;
	}
}

void shutdown_system(void)
{
#ifdef CONFIG_RDA_PDL
	enable_charger(1);
#endif

	while (1) {
		/* to unlock first */
		hwp_sysCtrlMd->REG_DBG = 0xa50001;
		/* shutdown */
		hwp_sysCtrlMd->WakeUp = 0;
	}
}

int rda_bm_is_calib(void)
{
	u16 swcfg = rda_swcfg_get();
	return !!(swcfg & RDA_SW_CFG_BIT_4);
}

int rda_bm_is_autocall(void)
{
	u16 swcfg = rda_swcfg_get();
	return !!(swcfg & RDA_SW_CFG_BIT_5);
}

int rda_bm_is_download(void)
{
#ifdef CONFIG_RDA_PDL /* for PDL mode, always enable download mode */
	return 1;
#else
	u16 hwcfg = rda_hwcfg_get();
	return !!(hwcfg & RDA_HW_CFG_BIT_11);
#endif
}

/* Check if download keys are pressed. */
int rda_bm_download_key_pressed(void)
{
	int key_power = !!(hwp_apKeypad->KP_STATUS & KEYPAD_KP_ON);
	int key_vol_up = !!(hwp_apGpioD->gpio_val & 0x40); /* GPIO D6 */
#ifdef CONFIG_SDMMC_BOOT
	int key_vol_down = !!(hwp_apGpioD->gpio_val & 0x20); /* GPIO D5 */
	return (key_power && key_vol_up && !key_vol_down);
#else
	return (key_power && key_vol_up);
#endif
}

void rda_reboot(enum reboot_type type)
{
	u16 hwcfg = rda_hwcfg_reg_get();
	u16 swcfg = rda_swcfg_reg_get();
	int key_vol_down;

	hwcfg &= ~(RDA_HW_CFG_BIT_10 | RDA_HW_CFG_BIT_11 |
		RDA_HW_CFG_BIT_12 | RDA_HW_CFG_BIT_13 |
		RDA_HW_CFG_BIT_14 | RDA_HW_CFG_BIT_15);

	switch (type) {
	case REBOOT_TO_DOWNLOAD_MODE:
		hwcfg |= RDA_HW_CFG_BIT_11 | RDA_HW_CFG_BIT_12 |
			RDA_HW_CFG_BIT_14;
		key_vol_down = !!(hwp_apGpioD->gpio_val & 0x20); /* GPIO D5 */
		if (key_vol_down)
			hwcfg |= RDA_HW_CFG_BIT_10 | RDA_HW_CFG_BIT_13;
		break;
	case REBOOT_TO_FASTBOOT_MODE:
		swcfg |= RDA_SW_CFG_BIT_2;
		break;
	case REBOOT_TO_RECOVERY_MODE:
		swcfg |= RDA_SW_CFG_BIT_3;
		break;
	case REBOOT_TO_CALIB_MODE:
		swcfg |= RDA_SW_CFG_BIT_4;
		break;
	case REBOOT_TO_PDL2_MODE:
		swcfg |= RDA_SW_CFG_BIT_6;
		break;
	default:
		break;
	}

	rda_hwcfg_reg_set(hwcfg);
	rda_swcfg_reg_set(swcfg);
	reset_cpu(0);
}

void enable_vibrator(int enable)
{
	u32 value;

	ispi_open(1);

	value = ispi_reg_read(0x03);

	if (enable)
		value |= 0x20;
	else
		value &= ~0x20;
	if (rda_metal_id_get() >= 9)
		value ^= 0x20;

	ispi_reg_write(0x03, value);

	ispi_open(0);
}

void enable_charger(int enable)
{
	u32 val;

	ispi_open(1);
	val = ispi_reg_read(0x15);
	if (enable) {
		val &= ~((1 << 15) | (1 << 14));
	} else {
		val |= (1 << 15);
		val &= ~(1 << 14);
	}
	ispi_reg_write(0x15, val);
	ispi_open(0);
}

void rda_dump_buf(char *data, size_t len)
{
    char temp_buf[64];
    size_t i, off = 0;

    memset(temp_buf, 0, 64);
    for (i=0;i<len;i++) {
        if(i%8 == 0) {
            sprintf(&temp_buf[off], "  ");
            off += 2;
        }
        sprintf(&temp_buf[off], "%02x ", data[i]);
        off += 3;
        if((i+1)%16 == 0 || (i+1) == len) {
            printf("%8d %s\n", (unsigned int)i/16,temp_buf);
            memset(temp_buf, 0, 64);
            off = 0;
        }
    }
    printf("\n");
}

void print_cur_time(void)
{
	unsigned long long time = ticks2usec(get_ticks());
	printf("\n****** [CURRENT TIME: %3d.%06d] ******\n\n",
			(int)(time / 1000000),
			(int)(time % 1000000));
}

#ifdef CONFIG_CMD_MISC

int usb_cable_connected(void)
{
	u32 val;

	ispi_open(1);
	val = ispi_reg_read(0x14);
	ispi_open(0);
	if (val & (1 << 8))
		return 1;
	else
		return 0;
}

int system_rebooted(void)
{
	static int system_rebooted_flag = -1;

	if (system_rebooted_flag == -1) {
		if (rda_mdcom_system_started_before())
			system_rebooted_flag = 1;
		else
			system_rebooted_flag = 0;
		rda_mdcom_set_system_started_flag();
	}
	return system_rebooted_flag;
}

static int boot_key_long_pressed = 0;

void save_current_boot_key_state(void)
{
	int boot_key_on = ((hwp_apKeypad->KP_STATUS & KEYPAD_KP_ON) != 0);
	int start_by_boot_key = ((rda_hwcfg_get() & RDA_HW_CFG_BIT_12) != 0);
	boot_key_long_pressed = (boot_key_on && start_by_boot_key);
}

int get_saved_boot_key_state(void)
{
	return boot_key_long_pressed;
}

void get_board_serial(struct tag_serialnr *serialnr)
{
	struct spl_security_info *info = get_bd_spl_security_info();
	const uint32_t *id256 = (uint32_t *)info->chip_unique_id.id;

	serialnr->low = id256[0];
	serialnr->high = id256[1];

	if ((serialnr->low | serialnr->high) == 0) {
		serialnr->low  = 0x90abcdef;
		serialnr->high = 0x12345678;
	}

	/* Set serialno enviroment variable */
	char tmp[32];
	sprintf(tmp, "%08x%08x", serialnr->high, serialnr->low);
	setenv("serialno", tmp);
}

static int boot_mode = RDA_BM_NORMAL;

void rda_bm_init(void)
{
	u16 hwcfg = rda_hwcfg_get();
	u16 swcfg = rda_swcfg_get();
#ifndef CONFIG_RDA_PDL
	int rebooted = system_rebooted();
	u32 fastboot_key_mask, all_key_mask;
#endif
	char str[2];
	struct tag_serialnr serialnr;
	get_board_serial(&serialnr);

	printf("RDA: HW_CFG 0x%04x\n", hwcfg);
	printf("RDA: SW_CFG 0x%04x\n", swcfg);
	printf("RDA: ID PROD %04x M %03x B %01x\n",
		rda_prod_id_get(), rda_metal_id_get(),
		rda_bond_id_get());

#ifdef CONFIG_RDA_PDL
	/* PDL: always enable download mode */
	boot_mode = RDA_BM_FORCEDOWNLOAD;
#else
	/* SPL */
	boot_mode = RDA_BM_NORMAL;

	if (swcfg & RDA_SW_CFG_BIT_2) {
		boot_mode = RDA_BM_FASTBOOT;
	} else if (swcfg & RDA_SW_CFG_BIT_3) {
		boot_mode = RDA_BM_RECOVERY;
	} else if (swcfg & RDA_SW_CFG_BIT_4) {
		boot_mode = RDA_BM_CALIB;
	} else if (swcfg & RDA_SW_CFG_BIT_5) {
		boot_mode = RDA_BM_AUTOCALL;
	} else if (swcfg & RDA_SW_CFG_BIT_6) {
		/* force to 'pdl2', called by kernel */
		boot_mode = RDA_BM_FORCEDOWNLOAD;
	} else if ((hwcfg & RDA_HW_CFG_BIT_10) && (hwcfg & RDA_HW_CFG_BIT_11)) {
		/*
		 * If power key, vol-up, and vol-down are all pressed, consider
		 * it as normal boot (for T-Card)
		 */
		boot_mode = RDA_BM_NORMAL;
	} else if ((hwcfg & RDA_HW_CFG_BIT_10) && !rebooted) {
		/* Distinguish between factory mode and h/w force reset */
		boot_mode = RDA_BM_FACTORY;
	} else if (hwcfg & RDA_HW_CFG_BIT_11) {
		/* Force download mode for pdl2 */
		boot_mode = RDA_BM_FORCEDOWNLOAD;
	} else if (usb_cable_connected()) {
		all_key_mask = RDA_HW_CFG_BIT_15 | RDA_HW_CFG_BIT_14 |
			RDA_HW_CFG_BIT_13 | RDA_HW_CFG_BIT_12 |
			RDA_HW_CFG_BIT_11 | RDA_HW_CFG_BIT_10;
		/* Fastboot keys are volume-up + volume-down keys */
		fastboot_key_mask = RDA_HW_CFG_BIT_13 | RDA_HW_CFG_BIT_14;

		if ((hwcfg & fastboot_key_mask) == fastboot_key_mask &&
			(hwcfg & all_key_mask & ~fastboot_key_mask) == 0) {
			/*
			 * All the keys are up, except for the fastboot keys;
			 * and the usb cable is plugged in
			 */
			boot_mode = RDA_BM_FASTBOOT;
		}
	}
#endif /* !CONFIG_RDA_PDL */

	printf("RDA: Boot_Mode %d\n", boot_mode);

	sprintf(str, "%1d", boot_mode);
	setenv ("bootmode", str);
}

enum rda_bm_type rda_bm_get(void)
{
	return boot_mode;
}

void rda_bm_set(enum rda_bm_type bm)
{
	boot_mode = bm;
}

#endif /* CONFIG_CMD_MISC */

