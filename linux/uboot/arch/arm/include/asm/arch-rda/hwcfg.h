#ifndef __RDA_HWCFG_H
#define __RDA_HWCFG_H

/*
 * HW BOOT MODE
 * BIT_15: 1 - force download
 * BIT_14: 1 - volume up
 * BIT_13: 1 - volume down
 * BIT_12: 1 - key on (power on key)
 * BIT_11: 1 - force DL (skip NAND loading)
 * BIT_10: 1 - to skip USB init (for cases USB init may fail)
 *         (in bootloader, this bit is used for calibration and driver test)
 * BIT_7 : if BIT_3 == 1 (SPI FLASH): 1 - SDMMC, 1 - SPI FLASH
 *         if BIT_3 == 0 (NAND/eMMC): 1 - 4K SLC/8K MLC, 0 - 2K SLC/4K MLC
 * BIT_4 : if BIT_3 == 1 (SPI FLASH): 1 - SPI NAND, 0 - SPI NOR
 *         if BIT_3 == 0 (NAND/eMMC): 1 - SDMMC, 0 - NAND/eMMC
 * BIT_3 : 1 - use SPI NAND, 0 - use NAND/eMMC
 * BIT_2 : 1 - use eMMC, 0 - use NAND
 * BIT_1 : 1 - 8bit NAND, 0 - 16bit NAND
 * BIT_0 : 1 - MLC, 0 - SLC
 */
#define RDA_HW_CFG_BIT_15    (1 << 15)
#define RDA_HW_CFG_BIT_14    (1 << 14)
#define RDA_HW_CFG_BIT_13    (1 << 13)
#define RDA_HW_CFG_BIT_12    (1 << 12)
#define RDA_HW_CFG_BIT_11    (1 << 11)
#define RDA_HW_CFG_BIT_10    (1 << 10)
#define RDA_HW_CFG_BIT_7     (1 << 7)
#define RDA_HW_CFG_BIT_4     (1 << 4)
#define RDA_HW_CFG_BIT_3     (1 << 3)
#define RDA_HW_CFG_BIT_2     (1 << 2)
#define RDA_HW_CFG_BIT_1     (1 << 1)
#define RDA_HW_CFG_BIT_0     (1 << 0)

/*
 * SW BOOT MODE
 * BIT_6 : PDL2
 * BIT_5 : Autocall
 * BIT_4 : Calib
 * BIT_3 : Recovery
 * BIT_2 : Fastboot
 * BIT_1 : ROM force run
 * BIT_0 : Modem EBC valid
 */
#define RDA_SW_CFG_BIT_6	(1 << 6)
#define RDA_SW_CFG_BIT_5	(1 << 5)
#define RDA_SW_CFG_BIT_4	(1 << 4)
#define RDA_SW_CFG_BIT_3	(1 << 3)
#define RDA_SW_CFG_BIT_2	(1 << 2)
#define RDA_SW_CFG_BIT_1	(1 << 1)
#define RDA_SW_CFG_BIT_0	(1 << 0)

void rda_hwcfg_reg_set(u16);
u16 rda_hwcfg_reg_get(void);
u16 rda_hwcfg_get(void);
void rda_swcfg_reg_set(u16);
u16 rda_swcfg_reg_get(void);
u16 rda_swcfg_get(void);
u16 rda_prod_id_get(void);
u16 rda_metal_id_get(void);
u16 rda_bond_id_get(void);
void rda_nand_iodrive_set(void);

#endif // __RDA_HWCFG_H
