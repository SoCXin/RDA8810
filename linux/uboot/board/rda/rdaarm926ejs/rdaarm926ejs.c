#include <common.h>
#include <asm/arch/hardware.h>
#include <asm/io.h>

DECLARE_GLOBAL_DATA_PTR;

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
            printf("%s\n", temp_buf);
            memset(temp_buf, 0, 64);
            off = 0;
        }
    }
    printf("\n");
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
	gd->bd->bi_arch_number = MACH_TYPE_RDAARM926EJS;

	/* address of boot parameters */
	gd->bd->bi_boot_params = LINUX_BOOT_PARAM_ADDR;

	return 0;
}

#ifdef CONFIG_RDA_MMC
int rda_mmc_init(void);
#endif

int board_mmc_init(bd_t *bis)
{
	int err = -1;

#ifdef CONFIG_RDA_MMC
	err = rda_mmc_init();
	if (err)
		return err;
#endif

	return err;
}

