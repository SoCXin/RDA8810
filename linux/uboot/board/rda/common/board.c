#include <common.h>
#include <asm/mach-types.h>

DECLARE_GLOBAL_DATA_PTR;

#define PHYS_SDRAM_1			(0x80000000) /* DRAM Start */
#define LINUX_BOOT_PARAM_ADDR		(PHYS_SDRAM_1 + 0x100)

#ifdef CONFIG_MACH_RDA8810
#define BOARD_NAME	"RDA8810"
#elif defined(CONFIG_MACH_RDA8810E)
#define BOARD_NAME	"RDA8810E"
#elif defined(CONFIG_MACH_RDA8810H)
#define BOARD_NAME	"RDA8810H"
#elif defined(CONFIG_MACH_RDA8820)
#define BOARD_NAME	"RDA8820"
#elif defined(CONFIG_MACH_RDA8850)
#define BOARD_NAME	"RDA8850"
#elif defined(CONFIG_MACH_RDA8850E)
#define BOARD_NAME	"RDA8850E"
#else
#define BOARD_NAME	"UNKNOWN"
#endif

int checkboard (void){
	puts("Board: "BOARD_NAME"\n");
	return 0;
}

/*
 * get_board_rev() - setup to pass kernel board revision information
 * Returns:
 * bit[0-3]	Maximum cpu clock rate supported by onboard SoC
 */
u32 get_board_rev(void)
{
	u32 rev = 0;
	return rev;
}

int board_init(void)
{
	/* arch number of the board */
	gd->bd->bi_arch_number = machine_arch_type;
	gd->bd->bi_boot_params = LINUX_BOOT_PARAM_ADDR;
	return 0;
}

