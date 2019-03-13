#ifndef __CONFIG_H
#define __CONFIG_H

/*
 * Board
 */

/*
 * SoC Configuration
 */
#define CONFIG_MACH_RDAARM926EJS
#define CONFIG_ARM926EJS		/* arm926ejs CPU core */
#define CONFIG_SYS_HZ_CLOCK		16384
#define CONFIG_SYS_HZ			1000
#define CONFIG_SKIP_LOWLEVEL_INIT
#define CONFIG_SYS_TEXT_BASE		0x84008000

/*
 * Memory Info
 */
#define CONFIG_SYS_MALLOC_LEN	(0x10000 + 1*1024*1024) /* malloc() len */
#define PHYS_SDRAM_1		0x84000000 /* DDR Start */
#define PHYS_SDRAM_1_SIZE	(64 << 20) /* SDRAM size 32MB */
#define CONFIG_MAX_RAM_BANK_SIZE (64 << 20) /* max size from RDA */

/* memtest start addr */
#define CONFIG_SYS_MEMTEST_START	(PHYS_SDRAM_1 + 0x00800000)

/* memtest will be run on 16MB */
#define CONFIG_SYS_MEMTEST_END 	(PHYS_SDRAM_1 + 0x00800000 + 16*1024*1024)

#define CONFIG_NR_DRAM_BANKS	1 /* we have 1 bank of DRAM */
#define CONFIG_STACKSIZE	(256*1024) /* regular stack */

/*
 * Serial Driver info
 */
#define CONFIG_BAUDRATE		921600		/* Default baud rate */
#define CONFIG_SYS_BAUDRATE_TABLE 	{ 9600, 38400, 115200, 921600 }

/*
 * SD/MMC
 */
#define CONFIG_MMC
#define CONFIG_DOS_PARTITION
/*#define CONFIG_RDA_MMC_LEGACY*/
#define CONFIG_GENERIC_MMC
#define CONFIG_RDA_MMC

/*
 * I2C Configuration
 */

/*
 * Flash & Environment
 */
#define CONFIG_NAND_RDA
#define CONFIG_SYS_NAND_USE_FLASH_BBT
/*#define CONFIG_SYS_NAND_HW_ECC*/
/*#define CONFIG_SYS_NAND_4BIT_HW_ECC_OOBFIRST*/
#define CONFIG_SYS_NAND_PAGE_2K

#define CONFIG_SYS_NAND_LARGEPAGE
#define CONFIG_SYS_NAND_BASE		0x01a26000 
/*#define CONFIG_SYS_NAND_BASE_LIST	{ 0x01a26000, } */
#define CONFIG_SYS_MAX_NAND_DEVICE	1
#define CONFIG_SYS_NAND_MAX_CHIPS	1

/*
 * Network & Ethernet Configuration
 */

/*
 *USB Gadget Configuration
 */

#define CONFIG_USB_RDA
#define CONFIG_MUSB_UDC
/*
#define CONFIG_USB_TTY
*/
#define CONFIG_USB_DEVICE
#define CONFIG_USBD_HS
#define CONFIG_USB_FASTBOOT
#define CONFIG_CMD_FASTBOOT
/*
#define CONFIG_USB_GADGET
#define CONFIG_USB_GADGET_DUALSPEED
#define CONFIG_USB_GADGET_RDA
#define CONFIG_USB_FASTBOOT
#define CONFIG_CMD_FASTBOOT
*/
#define SCRATCH_ADDR	0x86000000
#define FB_DOWNLOAD_BUF_SIZE	(100*1024*1024)

/*
 * U-Boot general configuration
 */
#define CONFIG_BOOTFILE		"uImage" /* Boot file name */
#define CONFIG_SYS_PROMPT	"RDA > " /* Command Prompt */
#define CONFIG_SYS_CBSIZE	1024 /* Console I/O Buffer Size	*/
#define CONFIG_SYS_PBSIZE	(CONFIG_SYS_CBSIZE+sizeof(CONFIG_SYS_PROMPT)+16)
#define CONFIG_SYS_MAXARGS	16 /* max number of command args */
#define CONFIG_SYS_BARGSIZE	CONFIG_SYS_CBSIZE /* Boot Args Buffer Size */
#define CONFIG_SYS_LOAD_ADDR	(PHYS_SDRAM_1 + 0x700000)
#define CONFIG_VERSION_VARIABLE
#define CONFIG_AUTO_COMPLETE
#define CONFIG_SYS_HUSH_PARSER
#define CONFIG_SYS_PROMPT_HUSH_PS2	"> "
#define CONFIG_CMDLINE_EDITING
#define CONFIG_SYS_LONGHELP
#define CONFIG_CRC32_VERIFY
#define CONFIG_MX_CYCLIC

/*
 * Environment settings
 */
#define	CONFIG_EXTRA_ENV_SETTINGS					\
	"usbtty=cdc_acm\0"
/*
 * Linux Information
 */
#define LINUX_BOOT_PARAM_ADDR	(PHYS_SDRAM_1 + 0x100)
#define CONFIG_CMDLINE_TAG
#define CONFIG_INITRD_TAG
#define CONFIG_REVISION_TAG
#define CONFIG_SETUP_MEMORY_TAGS
#define CONFIG_BOOTARGS		\
	"mem=64M console=ttyS0,115200n8 root=/dev/ram rw rdinit=/linuxrc init=/init "
#define CONFIG_BOOTDELAY	3

/*
 * U-Boot commands
 */
#include <config_cmd_default.h>
#define CONFIG_CMD_ENV
#define CONFIG_CMD_ASKENV
#define CONFIG_CMD_DIAG
#define CONFIG_CMD_SAVES
#define CONFIG_CMD_MEMORY
#define CONFIG_CMD_MMC

#ifdef CONFIG_NAND_RDA
#define CONFIG_CMD_MTDPARTS
#define CONFIG_MTD_PARTITIONS
#define CONFIG_MTD_DEVICE
#define CONFIG_CMD_NAND
#define CONFIG_CMD_NAND_YAFFS
/*#define CONFIG_CMD_UBI*/
/*#define CONFIG_CMD_UBIFS*/ 
#define CONFIG_YAFFS2 
#define CONFIG_RBTREE
#define MTDIDS_DEFAULT      "nand0=rda_nand"
#define MTDPARTS_DEFAULT    "mtdparts=rda_nand:128k@0(preloader)," \
                            "256k(config)," \
                            "128k(u-boot_env)," \
                            "512k(test_image)," \
                            "1M(u-boot)," \
                            "6M(kernel)," \
                            "504M(system)," \
                            "-(data)"
#endif

#undef CONFIG_CMD_NET
#undef CONFIG_CMD_DHCP
#undef CONFIG_CMD_MII
#undef CONFIG_CMD_PING
#undef CONFIG_CMD_DNS
#undef CONFIG_CMD_NFS
#undef CONFIG_CMD_RARP
#undef CONFIG_CMD_SNTP

#undef CONFIG_CMD_FLASH
#undef CONFIG_CMD_IMLS
#undef CONFIG_CMD_SPI
#undef CONFIG_CMD_SF
#undef CONFIG_CMD_SAVEENV

#if !defined(CONFIG_USE_NAND) && \
	!defined(CONFIG_USE_NOR) && \
	!defined(CONFIG_USE_SPIFLASH)
#define CONFIG_ENV_IS_NOWHERE
#define CONFIG_SYS_NO_FLASH
#define CONFIG_ENV_SIZE		(16 << 10)
#undef CONFIG_CMD_IMLS
#undef CONFIG_CMD_ENV
#endif

/* additions for new relocation code, must added to all boards */
#define CONFIG_SYS_SDRAM_BASE	0x84000000
#define CONFIG_SYS_INIT_SP_ADDR	(CONFIG_SYS_SDRAM_BASE + 0x1000 - /* Fix this */ \
					GENERATED_GBL_DATA_SIZE)

/* Defines for SPL */
#define CONFIG_SPL
#define CONFIG_SPL_NAND_SIMPLE
#define CONFIG_SYS_SRAM_BASE	0x01c00000
#define CONFIG_SYS_SRAM_SIZE	0x00008000  /* 32k */
#define CONFIG_SPL_TEXT_BASE	0x01c01000  /* first 4k leave for boot_rom bss */
#define CONFIG_SPL_MAX_SIZE		(20 * 1024)	/* 8 KB for stack */
#define CONFIG_SPL_STACK		(CONFIG_SYS_SRAM_BASE + CONFIG_SYS_SRAM_SIZE)

#define CONFIG_SPL_BSS_START_ADDR	CONFIG_SYS_SRAM_BASE  /* reuse boot_rom bss */
#define CONFIG_SPL_BSS_MAX_SIZE	0x1000		/* 4 KB */

#define CONFIG_SPL_BOARD_INIT
#define CONFIG_SPL_LIBCOMMON_SUPPORT
/*#define CONFIG_SPL_LIBDISK_SUPPORT*/
/*#define CONFIG_SPL_I2C_SUPPORT*/
#define CONFIG_SPL_LIBGENERIC_SUPPORT
/*#define CONFIG_SPL_MMC_SUPPORT*/
/*#define CONFIG_SPL_FAT_SUPPORT*/
#define CONFIG_SPL_SERIAL_SUPPORT
/*#define CONFIG_SPL_NAND_SUPPORT*/
/*#define CONFIG_SPL_POWER_SUPPORT*/
/*#define CONFIG_SPL_LDSCRIPT		"$(CPUDIR)/omap-common/u-boot-spl.lds"*/

#endif /* __CONFIG_H */
