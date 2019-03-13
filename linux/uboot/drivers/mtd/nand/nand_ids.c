/*
 *  drivers/mtd/nandids.c
 *
 *  Copyright (C) 2002 Thomas Gleixner (tglx@linutronix.de)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */

#include <common.h>
#include <linux/mtd/nand.h>
#include <rda/tgt_ap_board_config.h>
#include <rda/tgt_ap_flash_parts.h>

#if defined(CONFIG_NAND_RDA_V1) && !defined(_TGT_AP_NAND_DISABLE_HEC)
#define RDA_NAND_WITH_HEC
#endif

/*
*	Chip ID list
*
*	Name. ID code, pagesize, chipsize in MegaByte, eraseblock size,
*	options
*
*	Pagesize; 0, 256, 512
*	0	get this information from the extended chip ID
+	256	256 Byte page size
*	512	512 Byte page size
*/
const struct nand_flash_dev nand_flash_ids[] = {

#ifdef CONFIG_MTD_NAND_MUSEUM_IDS
	{"NAND 1MiB 5V 8-bit",		0x6e, 256, 1, 0x1000, 0},
	{"NAND 2MiB 5V 8-bit",		0x64, 256, 2, 0x1000, 0},
	{"NAND 4MiB 5V 8-bit",		0x6b, 512, 4, 0x2000, 0},
	{"NAND 1MiB 3,3V 8-bit",	0xe8, 256, 1, 0x1000, 0},
	{"NAND 1MiB 3,3V 8-bit",	0xec, 256, 1, 0x1000, 0},
	{"NAND 2MiB 3,3V 8-bit",	0xea, 256, 2, 0x1000, 0},
	{"NAND 4MiB 3,3V 8-bit", 	0xd5, 512, 4, 0x2000, 0},
	{"NAND 4MiB 3,3V 8-bit",	0xe3, 512, 4, 0x2000, 0},
	{"NAND 4MiB 3,3V 8-bit",	0xe5, 512, 4, 0x2000, 0},
	{"NAND 8MiB 3,3V 8-bit",	0xd6, 512, 8, 0x2000, 0},

	{"NAND 8MiB 1,8V 8-bit",	0x39, 512, 8, 0x2000, 0},
	{"NAND 8MiB 3,3V 8-bit",	0xe6, 512, 8, 0x2000, 0},
	{"NAND 8MiB 1,8V 16-bit",	0x49, 512, 8, 0x2000, NAND_BUSWIDTH_16},
	{"NAND 8MiB 3,3V 16-bit",	0x59, 512, 8, 0x2000, NAND_BUSWIDTH_16},
#endif

	{"NAND 16MiB 1,8V 8-bit",	0x33, 512, 16, 0x4000, 0},
	{"NAND 16MiB 3,3V 8-bit",	0x73, 512, 16, 0x4000, 0},
	{"NAND 16MiB 1,8V 16-bit",	0x43, 512, 16, 0x4000, NAND_BUSWIDTH_16},
	{"NAND 16MiB 3,3V 16-bit",	0x53, 512, 16, 0x4000, NAND_BUSWIDTH_16},

	{"NAND 32MiB 1,8V 8-bit",	0x35, 512, 32, 0x4000, 0},
	{"NAND 32MiB 3,3V 8-bit",	0x75, 512, 32, 0x4000, 0},
	{"NAND 32MiB 1,8V 16-bit",	0x45, 512, 32, 0x4000, NAND_BUSWIDTH_16},
	{"NAND 32MiB 3,3V 16-bit",	0x55, 512, 32, 0x4000, NAND_BUSWIDTH_16},

	{"NAND 64MiB 1,8V 8-bit",	0x36, 512, 64, 0x4000, 0},
	{"NAND 64MiB 3,3V 8-bit",	0x76, 512, 64, 0x4000, 0},
	{"NAND 64MiB 1,8V 16-bit",	0x46, 512, 64, 0x4000, NAND_BUSWIDTH_16},
	{"NAND 64MiB 3,3V 16-bit",	0x56, 512, 64, 0x4000, NAND_BUSWIDTH_16},

	{"NAND 128MiB 1,8V 8-bit",	0x78, 512, 128, 0x4000, 0},
	{"NAND 128MiB 1,8V 8-bit",	0x39, 512, 128, 0x4000, 0},
	{"NAND 128MiB 3,3V 8-bit",	0x79, 512, 128, 0x4000, 0},
	{"NAND 128MiB 1,8V 16-bit",	0x72, 512, 128, 0x4000, NAND_BUSWIDTH_16},
	{"NAND 128MiB 1,8V 16-bit",	0x49, 512, 128, 0x4000, NAND_BUSWIDTH_16},
	{"NAND 128MiB 3,3V 16-bit",	0x74, 512, 128, 0x4000, NAND_BUSWIDTH_16},
	{"NAND 128MiB 3,3V 16-bit",	0x59, 512, 128, 0x4000, NAND_BUSWIDTH_16},

	{"NAND 256MiB 3,3V 8-bit",	0x71, 512, 256, 0x4000, 0},

	/*
	 * These are the new chips with large page size. The pagesize and the
	 * erasesize is determined from the extended id bytes
	 */
#define LP_OPTIONS (NAND_SAMSUNG_LP_OPTIONS | NAND_NO_READRDY | NAND_NO_AUTOINCR)
#define LP_OPTIONS16 (LP_OPTIONS | NAND_BUSWIDTH_16)

	/*512 Megabit */
	{"NAND 64MiB 1,8V 8-bit",	0xA2, 0,  64, 0, LP_OPTIONS},
	{"NAND 64MiB 1,8V 8-bit",	0xA0, 0,  64, 0, LP_OPTIONS},
	/*{"NAND 64MiB 3,3V 8-bit",	0xF2, 0,  64, 0, LP_OPTIONS},*/
	{"NAND 64MiB 3,3V 8-bit",	0xD0, 0,  64, 0, LP_OPTIONS},
	{"NAND 64MiB 1,8V 16-bit",	0xB2, 0,  64, 0, LP_OPTIONS16},
	{"NAND 64MiB 1,8V 16-bit",	0xB0, 0,  64, 0, LP_OPTIONS16},
	{"NAND 64MiB 3,3V 16-bit",	0xC2, 0,  64, 0, LP_OPTIONS16},
	{"NAND 64MiB 3,3V 16-bit",	0xC0, 0,  64, 0, LP_OPTIONS16},

	/* 1 Gigabit */
	{"NAND 128MiB 1,8V 8-bit",	0xA1, 0, 128, 0, LP_OPTIONS},
	{"NAND 128MiB 3,3V 8-bit",	0xF1, 0, 128, 0, LP_OPTIONS},
	{"NAND 128MiB 3,3V 8-bit",	0xD1, 0, 128, 0, LP_OPTIONS},
	{"NAND 128MiB 1,8V 16-bit",	0xB1, 0, 128, 0, LP_OPTIONS16},
	{"NAND 128MiB 3,3V 16-bit",	0xC1, 0, 128, 0, LP_OPTIONS16},
	{"NAND 128MiB 1,8V 16-bit", 0xAD, 0, 128, 0, LP_OPTIONS16},

	/* 2 Gigabit */
	{"NAND 256MiB 1,8V 8-bit",	0xAA, 0, 256, 0, LP_OPTIONS},
	{"NAND 256MiB 3,3V 8-bit",	0xDA, 0, 256, 0, LP_OPTIONS},
	{"NAND 256MiB 1,8V 16-bit",	0xBA, 0, 256, 0, LP_OPTIONS16},
	{"NAND 256MiB 3,3V 16-bit",	0xCA, 0, 256, 0, LP_OPTIONS16},

	/* 4 Gigabit */
	{"NAND 512MiB 1,8V 8-bit",	0xAC, 0, 512, 0, LP_OPTIONS},
	{"NAND 512MiB 3,3V 8-bit",	0xDC, 0, 512, 0, LP_OPTIONS},
	//{"NAND 512MiB 3,3V 8-bit",	0xDC, 0, 384, 0, LP_OPTIONS},
	{"NAND 512MiB 1,8V 16-bit",	0xBC, 0, 512, 0, LP_OPTIONS16},
	{"NAND 512MiB 3,3V 16-bit",	0xCC, 0, 512, 0, LP_OPTIONS16},

	/* 8 Gigabit */
	{"NAND 1GiB 1,8V 8-bit",	0xA3, 0, 1024, 0, LP_OPTIONS},
	//{"NAND 1GiB 3,3V 8-bit",	0xD3, 0, 1024, 0, LP_OPTIONS},
	{"NAND 1GiB 1,8V 16-bit",	0xB3, 0, 1024, 0, LP_OPTIONS16},
	{"NAND 1GiB 3,3V 16-bit",	0xC3, 0, 1024, 0, LP_OPTIONS16},

	/* 16 Gigabit */
	{"NAND 2GiB 1,8V 8-bit",	0xA5, 0, 2048, 0, LP_OPTIONS},
	//{"NAND 2GiB 3,3V 8-bit",	0xD5, 0, 2048, 0, LP_OPTIONS},
	{"NAND 2GiB 1,8V 16-bit",	0xB5, 0, 2048, 0, LP_OPTIONS16},
	{"NAND 2GiB 3,3V 16-bit",	0xC5, 0, 2048, 0, LP_OPTIONS16},

	/* 32 Gigabit */
	{"NAND 4GiB 1,8V 8-bit",	0xA7, 0, 4096, 0, LP_OPTIONS},
//	{"NAND 4GiB 3,3V 8-bit",	0xD7, 0, 4096, 0, LP_OPTIONS},
	{"NAND 4GiB 1,8V 16-bit",	0xB7, 0, 4096, 0, LP_OPTIONS16},
	{"NAND 4GiB 3,3V 16-bit",	0xC7, 0, 4096, 0, LP_OPTIONS16},

	/* 64 Gigabit */
	{"NAND 8GiB 1,8V 8-bit",	0xAE, 0, 8192, 0, LP_OPTIONS},
	//{"NAND 8GiB 3,3V 8-bit",	0xDE, 0, 8192, 0, LP_OPTIONS},
	{"NAND 8GiB 1,8V 16-bit",	0xBE, 0, 8192, 0, LP_OPTIONS16},
	{"NAND 8GiB 3,3V 16-bit",	0xCE, 0, 8192, 0, LP_OPTIONS16},

	/* 128 Gigabit */
	{"NAND 16GiB 1,8V 8-bit",	0x1A, 0, 16384, 0, LP_OPTIONS},
	//{"NAND 16GiB 3,3V 8-bit",	0x3A, 0, 16384, 0, LP_OPTIONS},
	{"NAND 16GiB 1,8V 16-bit",	0x2A, 0, 16384, 0, LP_OPTIONS16},
	{"NAND 16GiB 3,3V 16-bit",	0x4A, 0, 16384, 0, LP_OPTIONS16},

	/* 256 Gigabit */
	{"NAND 32GiB 1,8V 8-bit",	0x1C, 0, 32768, 0, LP_OPTIONS},
	//{"NAND 32GiB 3,3V 8-bit",	0x3C, 0, 32768, 0, LP_OPTIONS},
	{"NAND 32GiB 1,8V 16-bit",	0x2C, 0, 32768, 0, LP_OPTIONS16},
	{"NAND 32GiB 3,3V 16-bit",	0x4C, 0, 32768, 0, LP_OPTIONS16},

	/* 512 Gigabit */
	{"NAND 64GiB 1,8V 8-bit",	0x1E, 0, 65536, 0, LP_OPTIONS},
	{"NAND 64GiB 3,3V 8-bit",	0x3E, 0, 65536, 0, LP_OPTIONS},
	{"NAND 64GiB 1,8V 16-bit",	0x2E, 0, 65536, 0, LP_OPTIONS16},
	{"NAND 64GiB 3,3V 16-bit",	0x4E, 0, 65536, 0, LP_OPTIONS16},

	/*
	 * Renesas AND 1 Gigabit. Those chips do not support extended id and
	 * have a strange page/block layout !  The chosen minimum erasesize is
	 * 4 * 2 * 2048 = 16384 Byte, as those chips have an array of 4 page
	 * planes 1 block = 2 pages, but due to plane arrangement the blocks
	 * 0-3 consists of page 0 + 4,1 + 5, 2 + 6, 3 + 7 Anyway JFFS2 would
	 * increase the eraseblock size so we chose a combined one which can be
	 * erased in one go There are more speed improvements for reads and
	 * writes possible, but not implemented now
	 */
	{"AND 128MiB 3,3V 8-bit",	0x01, 2048, 128, 0x4000,
	 NAND_IS_AND | NAND_NO_AUTOINCR |NAND_NO_READRDY | NAND_4PAGE_ARRAY |
	 BBT_AUTO_REFRESH
	},

	/*
	 * RDA Normal HW ECC support
	 * use 4K page as 4K page, support SLC nand only
	 */
	{"NAND 512MB 1,8V 16-bit",	0xbc, 0, 512, 0x40000,
	 LP_OPTIONS16
	},
	/*
	{"NAND 1,8V 16-bit slc",	0xbc, 4096, 512, 0x40000,
	 LP_OPTIONS16
	},
	*/

#ifdef RDA_NAND_WITH_HEC
	/*
	 * RDA HEC support, for 4k MLC nand
	 * use 4K page as 2K page, leave 2K space for HEC
	 * therefore, chip_size should be half
	 * and erase_size should be 256*2k = 512k = 0x80000
	 * Now, supported NAND
	 *   - Micron MT29F16xxxCA: ID = 2C 48 xx xx (16Gb use as a 8Gb NAND)
	 *   - Micron MT29F32xxxCA: ID = 2C 68 xx xx (32Gb use as a 16Gb NAND)
	 *   - sptek              : ID = 2C 64 xx xx (32Gb use as a 16Gb NAND) 
	 *   - hynix              : ID = AD D7 xx xx (32Gb use as a 16Gb NAND) 
	 */
	{"NAND 1GiB 3,3V 8-bit",	0x48, 2048, 1024, 0x80000,
	 LP_OPTIONS
	},
#if(__TGT_AP_FLASH_USE_PART_OF_PAGE__ == 1)
#if(__TGT_AP_FLASH_LOGIC_PAGE_SIZE_N_KB__ == __TGT_AP_FLASH_USE_4KAS2K_PAGE_DIV__)
	{"NAND 2GiB 3,3V 8-bit",	0x68, 2048, 2048, 0x80000,
	 LP_OPTIONS
	},
#else
#if(__TGT_AP_FLASH_LOGIC_PAGE_SIZE_N_KB__ == __TGT_AP_FLASH_USE_4KAS3K_PAGE_DIV__)
	{"NAND 3GiB 3,3V 8-bit",	0x68, 3072, 3072, 0xc0000,
	 LP_OPTIONS
	},
#endif
#endif

#if(__TGT_AP_FLASH_LOGIC_PAGE_SIZE_N_KB__ == __TGT_AP_FLASH_USE_8KAS6K_PAGE_DIV__)
	/*
	 * RDA HEC support, for 8k MLC nand
	 * use 8K page as 6K page, leave 2K space for HEC
	 * therefore, chip_size should be three quarter
	 * and erase_size should be 256*6k = 1536k = 0x180000
	 * Now, supported NAND
	 *   - Micron MT29F32xxxDA: ID = 2C 44 xx xx (32Gb use as a 24Gb NAND)
	 */
	{"NAND 3GiB 3,3V 8-bit",	0x44, 6144, 3072, 0x180000,
	 LP_OPTIONS
	},
	{"NAND 6GiB 3,3V 8-bit",	0x64, 6144, 6144, 0x180000,
	 LP_OPTIONS
	},
/* //samsung 8k chip
	{"NAND 3GiB 3,3V 8-bit",	0xd7, 6144, 3072, 0xc0000,
	 LP_OPTIONS
	},
*/
	{"NAND 3GiB 3,3V 8-bit",	0xd7, 6144, 3072, 0x180000,
	 LP_OPTIONS
	},
#else
#if(__TGT_AP_FLASH_LOGIC_PAGE_SIZE_N_KB__ == __TGT_AP_FLASH_USE_8KAS4K_PAGE_DIV__)
	{"NAND 2GiB 3,3V 8-bit",	0x44, 4096, 2048, 0x100000,
	 LP_OPTIONS
	},
	{"NAND 4GiB 3,3V 8-bit",	0x64, 4096, 4096, 0x100000,
	 LP_OPTIONS
	},
/* //samsung 8k chip
	{"NAND 2GiB 3,3V 8-bit",	0xd7, 4096, 2048, 0x80000,
	 LP_OPTIONS
	},
*/
	{"NAND 2GiB 3,3V 8-bit",	0xd7, 4096, 2048, 0x100000,
	 LP_OPTIONS
	},
#endif
#endif

#if(__TGT_AP_FLASH_LOGIC_PAGE_SIZE_N_KB__ == __TGT_AP_FLASH_USE_16KAS8K_PAGE_DIV__)
	{"NAND 2GiB 3,3V 8-bit",	0xd7, 8192, 2048, 0x200000,
	 LP_OPTIONS
	},
	{"NAND 4GiB 3,3V 8-bit",	0xde, 8192, 4096, 0x200000,
	 LP_OPTIONS
	},
#else
#if(__TGT_AP_FLASH_LOGIC_PAGE_SIZE_N_KB__ == __TGT_AP_FLASH_USE_16KAS12K_PAGE_DIV__)
	{"NAND 3GiB 3,3V 8-bit",	0xd7, 12288, 3072, 0x300000,
	 LP_OPTIONS
	},
	{"NAND 6GiB 3,3V 8-bit",	0xde, 12288, 6144, 0x300000,
	 LP_OPTIONS
	},
#endif
#endif
#else
	{"NAND 2GiB 3,3V 8-bit",	0x68, 2048, 2048, 0x80000,
	 LP_OPTIONS
	},
	/*
	 * RDA HEC support, for 8k MLC nand
	 * use 8K page as 4K page, leave 4K space for HEC
	 * therefore, chip_size should be half
	 * and erase_size should be 256*4k = 1024k = 0x100000
	 * Now, supported NAND
	 *   - Micron MT29F32xxxDA: ID = 2C 44 xx xx (32Gb use as a 16Gb NAND)
	 */
	{"NAND 2GiB 3,3V 8-bit",	0x44, 4096, 2048, 0x100000,
	 LP_OPTIONS
	},
#endif
	{"NAND 2GiB 3,3V 8-bit",	0x64, 2048, 2048, 0x80000,
	 LP_OPTIONS
	},
	{"NAND 2GiB 3,3V 8-bit",	0xd7, 2048, 2048, 0x80000,
	 LP_OPTIONS
	},
#else
	/* 2K page SLC nand */
	{"NAND 256MiB 3,3V 8-bit",	0xD2, 0, 256, 0, LP_OPTIONS},

	/* 4K page MLC nand */
	{"NAND 2GiB 3,3V 8-bit",	0x48, 4096, 2048, 0x100000,
	 LP_OPTIONS
	},
	{"NAND 4GiB 3,3V 8-bit",	0x68, 4096, 4096, 0x100000,
	 LP_OPTIONS
	},
	{"NAND 8GiB 3,3V 8-bit",	0x64, 8192, 8192, 0x200000,
	 LP_OPTIONS
	},
	/* 8K page MLC nand */
	{"NAND 4GiB 3,3V 8-bit",	0x44, 8192, 4096, 0x200000,
	 LP_OPTIONS
	},
	/*block size is 3M, so chipsize is time of 3.*/
	{"NAND 8GiB 3,3V 8-bit",	0x88, 8192, 8190, 0x300000,
	 LP_OPTIONS
	},
	/* 16K page MLC nand */
	{"NAND 8GiB 3,3V 8-bit",	0xd3, 16384, 8192, 0x400000,
	 LP_OPTIONS
	},
	/* 16K page MLC nand */
	{"NAND 8GiB 3,3V 8-bit",	0xd5, 16384, 8192, 0x400000,
	 LP_OPTIONS
	},
	/*This is conflict with a 8k nand*/
	/* 16K page MLC nand */
	{"NAND 8GiB 3,3V 8-bit",	0xd7, 16384, 4096, 0x400000,
	 LP_OPTIONS
	},
	/*
	{"NAND 8GiB 3,3V 8-bit",	0xd7, 7168, 3584, 0x1c0000,
	 LP_OPTIONS
	},
	*/
	/* 16K page MLC nand */
	{"NAND 8GiB 3,3V 8-bit",	0xde, 16384, 8192, 0x400000,
	 LP_OPTIONS
	},
	/*
	{"NAND 8GiB 3,3V 8-bit",	0xde, 14336, 7168, 0x380000,
	 LP_OPTIONS
	},
	*/
	/* 16K page MLC nand */
	{"NAND 16GiB 3,3V 8-bit",	0x3A, 16384, 16384, 0x400000,
	 LP_OPTIONS
	},
	/* 16K page MLC nand */
	{"NAND 16GiB 3,3V 8-bit",	0x3C, 16384, 32768, 0x400000,
	 LP_OPTIONS
	},
#endif /* _TGT_AP_NAND_DISABLE_HEC */

	/* RDA SPI nand support */
	{"NAND 128MiB 1,8V 4-bit",	0xe1, 2048, 128, 0x20000,
	 LP_OPTIONS},
	{"NAND 128MiB 3,3V 4-bit",	0xf1, 2048, 128, 0x20000,
	 LP_OPTIONS},
	{"NAND 256MiB 1,8V 4-bit",	0xe2, 2048, 256, 0x20000,
	 LP_OPTIONS},
	{"NAND 256MiB 3,3V 4-bit",	0xf2, 2048, 256, 0x20000,
	 LP_OPTIONS},
	{"NAND 512MiB 1,8V 4-bit",	0xe4, 2048, 512, 0x20000,
	 LP_OPTIONS},
	{"NAND 512MiB 3,3V 4-bit",	0xf4, 2048, 512, 0x20000,
	 LP_OPTIONS},
	{"NAND 512MiB 1,8V 4-bit",	0xc4, 4096, 512, 0x40000,
	 LP_OPTIONS},
	{"NAND 512MiB 3,3V 4-bit",	0xd4, 4096, 512, 0x40000,
	 LP_OPTIONS},
	{NULL,}
};

/*
*	Manufacturer ID list
*/
const struct nand_manufacturers nand_manuf_ids[] = {
	{NAND_MFR_TOSHIBA, "Toshiba"},
	{NAND_MFR_SAMSUNG, "Samsung"},
	{NAND_MFR_FUJITSU, "Fujitsu"},
	{NAND_MFR_NATIONAL, "National"},
	{NAND_MFR_RENESAS, "Renesas"},
	{NAND_MFR_STMICRO, "ST Micro"},
	{NAND_MFR_HYNIX, "Hynix"},
	{NAND_MFR_MICRON, "Micron"},
	{NAND_MFR_AMD, "AMD"},
	{NAND_MFR_ESMT,"ESMT"},
	{0x0, "Unknown"}
};
