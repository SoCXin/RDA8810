#include <common.h>
#include <asm/types.h>
#include <nand.h>

#include <asm/errno.h>
#include <asm/arch/hardware.h>
#include <asm/arch/reg_sysctrl.h>
#include <asm/arch/hwcfg.h>
#include <mtd/nand/rda_nand.h>
extern int rda_nand_init(struct nand_chip *nand);
extern int rda_spi_nand_init(struct nand_chip *nand);

int board_nand_init(struct nand_chip *chip) __attribute__ ((weak));

static int flash_intf_spi = 0;

#if defined(CONFIG_MACH_RDA8810)
int board_nand_init(struct nand_chip *chip)
{
	u16 hwcfg = rda_hwcfg_get();
	u16 metal_id = rda_metal_id_get();

	if (!(hwcfg & RDA_HW_CFG_BIT_3) && (hwcfg & RDA_HW_CFG_BIT_2)) {
		printf("metal %d hwcfg %x, use eMMC, skip nand init\n",
			metal_id, hwcfg);
		return -ENODEV;
	}

	if ((metal_id >= 7) && (hwcfg & RDA_HW_CFG_BIT_3)) {
		printf("metal %d hwcfg %x, use SPI NAND\n",
			metal_id, hwcfg);
		flash_intf_spi = 1;
		return rda_spi_nand_init(chip);
	} else {
		printf("metal %d hwcfg %x, use NAND\n",
			metal_id, hwcfg);
		return rda_nand_init(chip);
	}
}
#else /* 8810H*/
#if defined(CONFIG_MACH_RDA8810H)
int board_nand_init(struct nand_chip *chip)
{
	u16 hwcfg = rda_hwcfg_get();
	u16 metal_id = rda_metal_id_get();
	enum media_type media = rda_media_get();

	if (media == MEDIA_MMC) {
		printf("metal %d hwcfg %x, use eMMC, skip nand init\n",
			metal_id, hwcfg);
		return -ENODEV;
	} else if (media == MEDIA_SPINAND) {
		printf("metal %d hwcfg %x, use SPI NAND\n",
			metal_id, hwcfg);
		flash_intf_spi = 1;
		return rda_spi_nand_init(chip);
	} else if (media == MEDIA_NAND) {
		printf("metal %d hwcfg %x, use NAND\n",
			metal_id, hwcfg);
		return rda_nand_init(chip);
	} else {
		printf("invalid bootmode:hwcfg=0x%x\n", hwcfg);
		return -ENODEV;
	}
}
#else /* 8810E, 8820, 8850, 8850e */
int board_nand_init(struct nand_chip *chip)
{
	u16 hwcfg = rda_hwcfg_get();
	u16 metal_id = rda_metal_id_get();

	if (!(hwcfg & RDA_HW_CFG_BIT_3) && (hwcfg & RDA_HW_CFG_BIT_2)) {
		printf("metal %d hwcfg %x, use eMMC, skip nand init\n",
			metal_id, hwcfg);
		return -ENODEV;
	}

	if (hwcfg & RDA_HW_CFG_BIT_3) {
		printf("metal %d hwcfg %x, use SPI NAND\n",
			metal_id, hwcfg);
		flash_intf_spi = 1;
		return rda_spi_nand_init(chip);
	} else {
		printf("metal %d hwcfg %x, use NAND\n",
			metal_id, hwcfg);
		return rda_nand_init(chip);
	}
}
#endif
#endif

int rda_flash_intf_is_spi(void)
{
	return flash_intf_spi;
}
