#ifndef __RDA_SYS_H__
#define __RDA_SYS_H__

/*RDA_BOOTMODE_TYPE*/
#define    RDA_MODE_EMMC                0
#define    RDA_MODE_SPINAND             1
#define    RDA_MODE_SPINOR              2
#define    RDA_MODE_TCARD_RUN           3
#define    RDA_MODE_TCARD_UPDATE        4
#define    RDA_MODE_NAND_8BIT           5
#define    RDA_MODE_NAND_16BIT          6
#define    RDA_MODE_RESERVED            7

#define RDA_HW_CFG_GET_BM_IDX(r)     (((r)>>0)&0x7)

enum media_type {
	MEDIA_NAND = 0x10,
	MEDIA_MMC = 0x11,
	MEDIA_SPINAND = 0x12,
	MEDIA_UNKNOWN = 0xff,
};
enum media_type rda_media_get(void);
u16 rda_metal_id_get(void);
void shutdown_system(void);

int rda_bm_is_calib(void);
int rda_bm_is_autocall(void);
int rda_bm_is_download(void);
int rda_bm_download_key_pressed(void);

enum reboot_type {
	REBOOT_TO_NORMAL_MODE,
	REBOOT_TO_DOWNLOAD_MODE,
	REBOOT_TO_FASTBOOT_MODE,
	REBOOT_TO_RECOVERY_MODE,
	REBOOT_TO_CALIB_MODE,
	REBOOT_TO_PDL2_MODE,
};
void rda_reboot(enum reboot_type type);

void enable_vibrator(int enable);
void enable_charger(int enable);

void rda_dump_buf(char *data, size_t len);
void print_cur_time(void);

#ifdef CONFIG_CMD_MISC
int usb_cable_connected(void);
int system_rebooted(void);
void save_current_boot_key_state(void);
int get_saved_boot_key_state(void);
enum rda_bm_type
{
	RDA_BM_NORMAL = 0,
	RDA_BM_CALIB = 1,
	RDA_BM_FACTORY = 2,
	RDA_BM_FASTBOOT = 3,
	RDA_BM_RECOVERY = 4,
	RDA_BM_AUTOCALL = 5,
	RDA_BM_FORCEDOWNLOAD = 6,
};
void rda_bm_init(void);
enum rda_bm_type rda_bm_get(void);
void rda_bm_set(enum rda_bm_type bm);
#endif

#endif
