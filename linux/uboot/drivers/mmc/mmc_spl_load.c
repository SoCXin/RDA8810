#include <common.h>
#include <mmc.h>
#include <asm/errno.h>
#include <asm/arch/spl_board_info.h>

DECLARE_GLOBAL_DATA_PTR;
#ifdef CONFIG_SPL_CHECK_IMAGE
int check_uimage(unsigned int *buf);
#endif

static struct mmc *boot_mmc = NULL;

int emmc_spl_load_image(uint32_t offs, unsigned int size, void *dst)
{
	unsigned long err;
	unsigned int start_block, block_num, block_size;

	block_size = 512;
	start_block = offs/block_size;
	block_num = (size + block_size - 1)/block_size;

	if (!boot_mmc) {
		printf("spl emmc not initialized\n");
		return -ENODEV;
	}

	err = boot_mmc->block_dev.block_read(CONFIG_MMC_DEV_NUM,
			start_block,
			block_num, (void *)dst);

	if (err <= 0) {
		printf("spl emmc read err %d\n", (unsigned int)err);
		return err;
	}
	return 0;
}

extern int mmc_set_host_mclk_adj_inv(struct mmc *mmc, u8 adj, u8 inv);

void emmc_init(void)
{
	int err;
	int i = 0;
	struct spl_emmc_info *info = get_bd_spl_emmc_info();

	info->manufacturer_id = 0;

	mmc_initialize(gd->bd);

	boot_mmc = find_mmc_device(CONFIG_MMC_DEV_NUM);
	if (!boot_mmc) {
		puts("spl emmc device not found\n");
		return;
	}

	for(; i < 32; i++){
		int adj = i % 16;
		int inv = (i >= 16)? 1: 0;

		if(mmc_set_host_mclk_adj_inv(boot_mmc, adj, inv) < 0){
			printf("mmc_set_host_mclk_adj_inv failed \n");
			boot_mmc = NULL;
			return;
		}

		err = mmc_init(boot_mmc);
		if (err) {
			printf("spl emmc init failed, err %d\n", err);
		} else
			break;
	}

	if (i < 32){
		info->manufacturer_id = boot_mmc->cid[0] >> 24;
		printf("boot_mmc mfr id = %x \n",  boot_mmc->cid[0] >> 24);
	} else{
		printf("After loop, spl emmc init failed.\n");
		boot_mmc = NULL;
		return;
	}
}

/*
 * The main entry for EMMC booting. It's necessary that SDRAM is already
 * configured and available since this code loads the main U-Boot image
 * from EMMC into SDRAM and starts it from there.
 */
void emmc_boot(void)
{
	__attribute__((noreturn)) void (*uboot)(void);

	/*
	 * Load U-Boot image from EMMC into RAM
	 */
	emmc_spl_load_image(CONFIG_SYS_EMMC_U_BOOT_OFFS,
			CONFIG_SYS_EMMC_U_BOOT_SIZE,
			(void *)CONFIG_SYS_EMMC_U_BOOT_DST);

#ifdef CONFIG_SPL_CHECK_IMAGE
	if (check_uimage((unsigned int*)CONFIG_SYS_EMMC_U_BOOT_DST)) {
		printf("EMMC boot failed.\n");
		return;
	}
#endif

	/*
	 * Jump to U-Boot image
	 */
	uboot = (void *)CONFIG_SYS_EMMC_U_BOOT_START;
	(*uboot)();
}
