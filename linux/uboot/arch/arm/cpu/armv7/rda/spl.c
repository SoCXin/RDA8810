#include <common.h>
#include <asm/u-boot.h>
#include <asm/utils.h>
#include <nand.h>
#include <malloc.h>
#include <image.h>
#include <usb/usbserial.h>
#include <asm/arch/rda_sys.h>
#include <asm/arch/spl_board_info.h>
#include <asm/arch/rda_crypto.h>
#include <pdl.h>

#ifdef CONFIG_SPL_LIBCOMMON_SUPPORT

DECLARE_GLOBAL_DATA_PTR;
/* Define global data structure pointer to it*/
static gd_t gdata __attribute__ ((section(".data")));
static bd_t bdata __attribute__ ((section(".data")));

#else

void puts(const char *str)
{
	while (*str)
		putc(*str++);
}

void putc(char c)
{
}

#endif /* CONFIG_SPL_LIBCOMMON_SUPPORT */

void board_init_f(ulong dummy)
{
	relocate_code(CONFIG_SPL_STACK, NULL, CONFIG_SPL_TEXT_BASE);
}

#ifdef CONFIG_SPL_CHECK_IMAGE
int check_uimage(unsigned int *buf)
{
	image_header_t *hdr = (image_header_t *)buf;
	puts("Check Image ");
	if (!image_check_magic(hdr)) {
		printf("Magic Error %x\n", image_get_magic(hdr));
		rda_dump_buf((char *)buf, 256);
		return -1;
	}
	puts(".");
	if (!image_check_hcrc(hdr)) {
		printf("HCRC Error %x\n", image_get_hcrc(hdr));
		rda_dump_buf((char *)buf, 256);
		return -2;
	}
	puts(".");
	if (!image_check_dcrc(hdr)) {
		printf("DCRC Error %x\n", image_get_dcrc(hdr));
		rda_dump_buf((char *)buf, 256);
		//rda_dump_buf(buf, 
		//	image_get_header_size() + image_get_size(hdr));
		return -3;
	}
	puts(".");
#ifdef CONFIG_SIGNATURE_CHECK_IMAGE
	if (image_sign_verify_uimage(hdr) != 0) {
		printf("Image Signature check failed!\n");
		return -4;
	}
#endif
	puts(" Done\n");
	return 0;
}
#endif

#ifdef CONFIG_SPL_EMMC_LOAD
extern void emmc_init(void);
extern void emmc_boot(void);
#endif

#ifdef CONFIG_SPL_XMODEM_LOAD
extern void xmodem_boot(void);
#endif

extern void hwcfg_swcfg_init(void);
extern int clock_init(void);


#ifdef CONFIG_SIGNATURE_CHECK_IMAGE
static const uint8_t *get_spl_load_addr(void)
{
	const uint8_t *spl_image = (uint8_t*)CONFIG_SPL_LOAD_ADDRESS + CONFIG_UIMAGEHDR_SIZE;
	return spl_image;
}
#endif

void board_init_r(gd_t *id, ulong dummy)
{
#ifdef CONFIG_SPL_LIBCOMMON_SUPPORT
	gd = &gdata;
	gd->bd = &bdata;
	gd->flags |= GD_FLG_RELOC;
	gd->baudrate = CONFIG_BAUDRATE;
	serial_init();          /* serial communications setup */
	gd->have_console = 1;
#endif
	hwcfg_swcfg_init();
	clock_init();

#ifdef CONFIG_SIGNATURE_CHECK_IMAGE
	// Set the security contex (or get the unique ID of the device)
	struct spl_security_info *info = get_bd_spl_security_info();
	const uint8_t *spl_image = get_spl_load_addr();

	puts("SETTING SECURITY CONTEXT\n");
	set_security_context(info, spl_image);
#endif

#ifndef CONFIG_RDA_PDL
	/* Note BIT3 for SPINAND got higher priority */
#ifdef CONFIG_SPL_EMMC_SUPPORT
	if (rda_media_get() == MEDIA_MMC) {
		puts("Init emmc ...\n");
		emmc_init();
	}
#endif

#ifdef CONFIG_SPL_NAND_SUPPORT
	if (rda_media_get() == MEDIA_NAND || rda_media_get() == MEDIA_SPINAND) {
		puts("Init nand ...\n");
		nand_init();
	}
#endif
#endif

#ifdef CONFIG_RDA_PDL
	drv_usbser_init();
	pdl_main();
#else
	/* A workaround to handle the timing issue when detecting
	 * download mode in h/w */
	if( !rda_bm_is_autocall() &&
		!rda_bm_is_calib() &&
		!rda_bm_is_download() &&
		rda_bm_download_key_pressed()) {
		puts("Download key pressed. Enter download mode ...\n");
		rda_reboot(REBOOT_TO_DOWNLOAD_MODE);
	}
#endif

#ifdef CONFIG_SPL_EMMC_LOAD
	if (rda_media_get() == MEDIA_MMC) {
		puts("EMMC boot ...\n");
		emmc_boot();
	}
#endif

#ifdef CONFIG_SPL_NAND_LOAD
	if (rda_media_get() == MEDIA_NAND || rda_media_get() == MEDIA_SPINAND) {
		puts("NAND boot ...\n");
		nand_boot();
	}
#endif

#ifdef CONFIG_SPL_SPI_LOAD
	mem_malloc_init(CONFIG_SYS_TEXT_BASE - CONFIG_SYS_MALLOC_LEN,
			CONFIG_SYS_MALLOC_LEN);

	gd = &gdata;
	gd->bd = &bdata;
	gd->flags |= GD_FLG_RELOC;
	gd->baudrate = CONFIG_BAUDRATE;
	serial_init();          /* serial communications setup */
	gd->have_console = 1;

	puts("SPI boot...\n");
	spi_boot();
#endif

#ifdef CONFIG_SPL_XMODEM_LOAD
	puts("Xmodem boot ...\n");
	xmodem_boot();
#endif

	/*
	should never go here
	*/
	while(1)
	;
}
