#include <common.h>
#include <malloc.h>
#include <asm/errno.h>
#include <asm/io.h>
#include <asm/types.h>
#include <nand.h>

#include <asm/arch/hardware.h>
#include <asm/arch/reg_nand.h>
#include <asm/arch/reg_sysctrl.h>
#include <asm/arch/hwcfg.h>
#include <mtd/nand/rda_nand.h>

#include <rda/tgt_ap_board_config.h>
#include <rda/tgt_ap_clock_config.h>
#include <rda/tgt_ap_flash_parts.h>

#ifdef CONFIG_SPL_BUILD
#undef CONFIG_NAND_RDA_DMA
#endif

#ifdef CONFIG_NAND_RDA_DMA
#include <asm/dma-mapping.h>
#include <asm/arch/dma.h>
#endif

//#define NAND_DEBUG
//#define NAND_DEBUG_VERBOSE

#if (_TGT_NAND_TYPE_ == _PARALLEL_NAND_USED_)

#define hal_gettime     get_ticks
#define SECOND          * CONFIG_SYS_HZ_CLOCK
#define NAND_TIMEOUT    ( 2 SECOND )
#define PLL_BUS_FREQ	(_TGT_AP_PLL_BUS_FREQ * 1000000)

/***********************nand  global buffer define ************************/
/******************for 8810 support  8k/16k nand flash*********************/
#ifdef CONFIG_SPL_BUILD
/*sram space is not enough, so use sdram space as middle data buffer*/
static u8 *g_nand_flash_temp_buffer = (u8 *)CONFIG_SPL_NAND_MIDDLE_DATA_BUFFER;
#else
#define NAND_FLASH_PHYSICAL_PAGE_SIZE	16384
#define NAND_FLASH_PHYSICAL_SPARE_SIZE	1280
static u8 g_nand_flash_temp_buffer[NAND_FLASH_PHYSICAL_PAGE_SIZE + \
					NAND_FLASH_PHYSICAL_SPARE_SIZE] __attribute__ ((__aligned__(64)));
#endif
//#define NAND_CONTROLLER_BUFFER_DATA_SIZE	4096
//#define NAND_CONTROLLER_BUFFER_OOB_SIZE	224
#define NAND_CONTROLLER_TEMP_BUFFER_SIZE	4320

/*when system use ext4+ftl mode, FTL oob struct is 12bytes, and is stored in flash OOB space.*/
/*the nand page size use 3K bytes data + 128 bytes oob, or 6k bytes data + 256 bytes oob, or*/
/*12k bytes data + 512 bytes oob. but nand controller has hardware ecc error,only could 3K ecc*/
/*in 4k byte,  so OOB could not be ecc protecked. But maybe have some error bits in this space,*/
/*so the method is: copy FTL oob struct 12 bytes + 4 bytes checksum several times in oob space,*/
/*when read, find the first right checksum data, then use this array data.  */
#define __FTL_OOB_STRUCT_LENGTH__ 12  //bytes, when FTL oob struct add member, this sould increase.
#define OOB_SW_PROTECT_SWITCH_ON 1

/* clock division value map */
const static u8 clk_div_map[] = {
	4*60,	/* 0 */
	4*60,	/* 1 */
	4*60,	/* 2 */
	4*60,	/* 3 */
	4*60,	/* 4 */
	4*60,	/* 5 */
	4*60,	/* 6 */
	4*60,	/* 7 */
	4*40,	/* 8 */
	4*30,	/* 9 */
	4*24,	/* 10 */
	4*20,	/* 11 */
	4*17,	/* 12 */
	4*15,	/* 13 */
	4*13,	/* 14 */
	4*12,	/* 15 */
	4*11,	/* 16 */
	4*10,	/* 17 */
	4*9,	/* 18 */
	4*8,	/* 19 */
	4*7,	/* 20 */
	4*13/2,	/* 21 */
	4*6,	/* 22 */
	4*11/2,	/* 23 */
	4*5,	/* 24 */
	4*9/2,	/* 25 */
	4*4,	/* 26 */
	4*7/2,	/* 27 */
	4*3,	/* 28 */
	4*5/2,	/* 29 */
	4*2,	/* 30 */
	4*1,	/* 31 */
};

static struct nand_ecclayout rda_nand_oob_64 = {
	.eccbytes = 0,
	.eccpos = {},
	.oobfree = {
		{.offset = 2,
		 .length = 62} }
};

static struct nand_ecclayout rda_nand_oob_128 = {
	.eccbytes = 0,
	.eccpos = {},
	.oobfree = {
		{.offset = 2,
		 .length = 126} }
};

static struct nand_ecclayout rda_nand_oob_256 = {
	.eccbytes = 0,
	.eccpos = {},
	.oobfree = {
		{.offset = 2,
		 .length = 254} }
};

static struct nand_ecclayout rda_nand_oob_512 = {
	.eccbytes = 0,
	.eccpos = {},
	.oobfree = {
		{.offset = 2,
		 .length = 510} }
};

extern void rda_dump_buf(char *data, size_t len);

#if (OOB_SW_PROTECT_SWITCH_ON == 1)
static uint32_t nand_checksum32( unsigned char* data, uint32_t data_size)
{
	uint32_t checksum = 0;
	uint32_t i;

	for(i = 0; i < data_size; i++)
		checksum = ((checksum << 31) | (checksum >> 1)) + (uint32_t)data[i];

	return checksum;
}

struct select_data{
	u8 data;
	u8 count;
};

static void nand_sw_ecc_check(u8 * buf, u8 *dst, u8 copy_number, u8 array_length)
{
	u8 i, j, k, m;
	struct select_data select[30];
	u8 temp;

	for(i = 0; i < array_length; i++){
		m = 0;
		select[m].data = buf[i];
		select[m].count = 1;
		for(j = 1; j < copy_number; j++){
			k = 0;
			while(1){
				if(select[k].data == buf[i + j * (array_length + 4)]){
					select[k].count++;
					break;
				}

				if(k >= m){
					select[m+1].data = buf[i + j * (array_length + 4)];
					select[m+1].count = 1;
					m++;
					break;
				}
				k++;
			}
		}

		temp = select[0].count;
		dst[i] = select[0].data;
		for(j = 1; j <= m; j++){
			if(select[j].count > temp){
				dst[i] = select[j].data;
				temp = select[j].count;
			}
		}
	}
}
#endif

static void hal_send_cmd(unsigned char cmd, unsigned int page_addr)
{
	unsigned long cmd_reg;

	cmd_reg = NANDFC_DCMD(cmd) | NANDFC_PAGE_ADDR(page_addr);
#ifdef NAND_DEBUG
	printf("  hal_send_cmd 0x%08lx\n", cmd_reg);
#endif
	__raw_writel(cmd_reg, NANDFC_REG_DCMD_ADDR);
}

static void hal_set_col_addr(unsigned int col_addr)
{
	__raw_writel(col_addr, NANDFC_REG_COL_ADDR);
}

static void hal_set_col_cmd(unsigned int col_cmd)
{
	__raw_writel(0x50010000 |col_cmd, NANDFC_REG_CMD_DEF_B);
}

static void hal_flush_buf(struct mtd_info *mtd)
{
	/*
	 there is no reg NANDFC_REG_BUF_CTRL
	 */
	//__raw_writel(0x7, NANDFC_REG_BUF_CTRL);
}

static unsigned long hal_wait_cmd_complete(void)
{
	unsigned long int_stat;
	unsigned long long wait_time = NAND_TIMEOUT;
	unsigned long long start_time = hal_gettime();
	int timeout = 0;

	/* wait done */
	do {
		int_stat = __raw_readl(NANDFC_REG_INT_STAT);
		if (hal_gettime() - start_time >= wait_time) {
			timeout = 1;
		}
	} while (!(int_stat & NANDFC_INT_DONE) && !timeout);

	/* to clear */
	__raw_writel(int_stat & NANDFC_INT_CLR_MASK, NANDFC_REG_INT_STAT);

	if (timeout) {
		printf("nand error, cmd timeout\n");
		return -ETIME;
	}

	if (int_stat & NANDFC_INT_ERR_ALL) {
		printf("nand error, int_stat = %lx\n", int_stat);
		return (int_stat & NANDFC_INT_ERR_ALL);
	}
	return 0;
}

static int init_nand_info(struct rda_nand_info *info,
			int nand_type, int bus_width_16)
{
	info->type = nand_type;
	switch (nand_type) {
	case NAND_TYPE_2KL:
		info->hard_ecc_hec = 0;
		info->bus_width_16 = bus_width_16;
		info->page_shift = 11;
		info->oob_size = 64;
		info->page_num_per_block = 64;
		info->vir_page_size = 2048;
		info->vir_page_shift = 11;
		info->vir_oob_size = 64;
		info->vir_erase_size =
			info->vir_page_size * info->page_num_per_block;
		info->nand_use_type = 0;
		break;
#if(__TGT_AP_FLASH_USE_PART_OF_PAGE__ == 1)
	case NAND_TYPE_MLC4K:
		info->hard_ecc_hec = 1;
		info->bus_width_16 = bus_width_16;
		info->page_shift = 12;
		info->oob_size = 224;
		info->page_num_per_block = 256;
#if(__TGT_AP_FLASH_LOGIC_PAGE_SIZE_N_KB__ == __TGT_AP_FLASH_USE_4KAS2K_PAGE_DIV__)
		info->vir_page_size = 2048;
		info->vir_oob_size = 64;
#else
#if(__TGT_AP_FLASH_LOGIC_PAGE_SIZE_N_KB__ == __TGT_AP_FLASH_USE_4KAS3K_PAGE_DIV__)
		info->vir_page_size = 3072;
		info->vir_oob_size = 128;
#endif
#endif
		info->vir_page_shift = 0;
		info->vir_erase_size =
			info->vir_page_size * info->page_num_per_block;
		info->nand_use_type = 0;
		break;
#else
	case NAND_TYPE_MLC4K:
		info->hard_ecc_hec = 1;
		info->bus_width_16 = bus_width_16;
		info->page_shift = 12;
		info->oob_size = 224;
		info->page_num_per_block = 256;
		info->vir_page_size = 2048;
		info->vir_page_shift = 11;
		info->vir_oob_size = 64;
		info->vir_erase_size =
			info->vir_page_size * info->page_num_per_block;
		info->nand_use_type = 0;
		break;
#endif
	case NAND_TYPE_SLC4K:
		info->hard_ecc_hec = 0;
		info->bus_width_16 = bus_width_16;
		info->page_shift = 12;
		/* slc 4k, u-boot kernel yaffs use 128 oob */
		info->oob_size = 128;
#ifdef _TGT_AP_NAND_DISABLE_HEC
		/* this flag also implies using MLC */
		info->page_num_per_block = 256;
#else
		info->page_num_per_block = 64;
#endif
		info->vir_page_size = 4096;
		info->vir_page_shift = 12;
		info->vir_oob_size = 128;
		info->vir_erase_size =
			info->vir_page_size * info->page_num_per_block;
		info->nand_use_type = 0;
		break;
#if(__TGT_AP_FLASH_USE_PART_OF_PAGE__ == 1)
	case NAND_TYPE_MLC8K:
		info->hard_ecc_hec = 1;
		info->bus_width_16 = bus_width_16;
		info->page_shift = 13;
		info->oob_size = 744;
		info->page_num_per_block = 256;
#if(__TGT_AP_FLASH_LOGIC_PAGE_SIZE_N_KB__ == __TGT_AP_FLASH_USE_8KAS6K_PAGE_DIV__)
		info->vir_page_size = 6144;
		info->vir_oob_size = 256;
		info->nand_use_type = 1;
#else
#if(__TGT_AP_FLASH_LOGIC_PAGE_SIZE_N_KB__ == __TGT_AP_FLASH_USE_8KAS4K_PAGE_DIV__)
		info->vir_page_size = 4096;
		info->vir_oob_size = 128;
		info->nand_use_type = 1;
#endif
#endif
		info->vir_page_shift = 0;
		info->vir_erase_size =
			info->vir_page_size * info->page_num_per_block;
		break;
#else
	case NAND_TYPE_MLC8K:
		info->hard_ecc_hec = 1;
		info->bus_width_16 = bus_width_16;
		info->page_shift = 13;
		info->oob_size = 744;
		info->page_num_per_block = 256;
		info->vir_page_size = 4096;
		info->vir_page_shift = 12;
		info->vir_oob_size = 128;
		info->vir_erase_size =
			info->vir_page_size * info->page_num_per_block;
		info->nand_use_type = 1;
		break;
#endif
	case NAND_TYPE_SLC8K:
		info->hard_ecc_hec = 0;
		info->bus_width_16 = bus_width_16;
		info->page_shift = 13;
		info->oob_size = 744;
		info->page_num_per_block = 256;
		info->vir_page_size = 8192;
		info->vir_page_shift = 13;
		info->vir_oob_size = 256;
		info->vir_erase_size =
			info->vir_page_size * info->page_num_per_block;
		info->nand_use_type = 1;
		break;
#if(__TGT_AP_FLASH_USE_PART_OF_PAGE__ == 1)
	case NAND_TYPE_MLC16K:
		info->hard_ecc_hec = 1;
		info->bus_width_16 = bus_width_16;
		info->page_shift = 14;
		info->oob_size = 1280;
		info->page_num_per_block = 256;
#if(__TGT_AP_FLASH_LOGIC_PAGE_SIZE_N_KB__ == __TGT_AP_FLASH_USE_16KAS8K_PAGE_DIV__)
		info->vir_page_size = 8192;
		info->vir_oob_size = 256;
#else
#if(__TGT_AP_FLASH_LOGIC_PAGE_SIZE_N_KB__ == __TGT_AP_FLASH_USE_16KAS12K_PAGE_DIV__)
		info->vir_page_size = 12288;
		info->vir_oob_size = 512;
#endif
#endif
		info->vir_page_shift = 0;
		info->vir_erase_size =
			info->vir_page_size * info->page_num_per_block;
		info->nand_use_type = 3;
		break;
#else
	case NAND_TYPE_MLC16K:
		info->hard_ecc_hec = 1;
		info->bus_width_16 = bus_width_16;
		info->page_shift = 14;
		info->oob_size = 1280;
		info->page_num_per_block = 256;
		info->vir_page_size = 8192;
		info->vir_page_shift = 13;
		info->vir_oob_size = 256;
		info->vir_erase_size =
			info->vir_page_size * info->page_num_per_block;
		info->nand_use_type = 3;
		break;
#endif
	case NAND_TYPE_SLC16K:
		info->hard_ecc_hec = 0;
		info->bus_width_16 = bus_width_16;
		info->page_shift = 14;
		info->oob_size = 1280;
		info->page_num_per_block = 256;
		info->vir_page_size = 16384;
		info->vir_page_shift = 14;
		info->vir_oob_size = 512;
		info->vir_erase_size =
			info->vir_page_size * info->page_num_per_block;
		info->nand_use_type = 3;
		break;
	default:
		printf("invalid nand type\n");
		return -EINVAL;
	}

	info->logic_page_size = info->vir_page_size / (info->nand_use_type + 1);
	info->logic_oob_size = info->vir_oob_size / (info->nand_use_type + 1);
	return 0;
}

static u32 cal_freq_by_divreg(u32 basefreq, u32 reg, u32 div2)
{
	u32 newfreq;

	if (reg >= ARRAY_SIZE(clk_div_map)) {
		printf("nand:Invalid div reg: %u\n", reg);
		reg = ARRAY_SIZE(clk_div_map) - 1;
	}
	/* Assuming basefreq is smaller than 2^31 (2.147G Hz) */
	newfreq = (basefreq << (div2 ? 0 : 1)) / (clk_div_map[reg] >> 1);
	return newfreq;
}

unsigned long get_master_clk_rate(u32 reg)
{
	u32 div2;
	unsigned long rate;

	div2 = reg & SYS_CTRL_AP_AP_APB2_SRC_SEL;
	reg = GET_BITFIELD(reg, SYS_CTRL_AP_AP_APB2_FREQ);
	rate = cal_freq_by_divreg(PLL_BUS_FREQ, reg, div2);

	return rate;
}

static unsigned long rda_nand_cld_div_tbl[16] = {
	3, 4, 5, 6, 7, 8, 9, 10,
	12, 14, 16, 18, 20, 22, 24, 28
};

static unsigned long hal_calc_divider(struct rda_nand_info *info)
{
	unsigned long mclk = info->master_clk;
	unsigned long div, clk_div;
	int i;

	div = mclk / info->clk;
	if (mclk % info->clk)
		div += 1;

	if (div < 7) {
		/* 7 is minimal divider by hardware */
		div = 7;
	}

	for (i=0;i<16;i++) {
		if(div <= rda_nand_cld_div_tbl[i]) {
			clk_div = i;
			break;
		}
	}

	if (i>=16) {
		clk_div = 15;
	}

	//printf("NAND: div:%ld, clk_div:%ld\n", div, clk_div);
	return clk_div;
}

static int hal_init(struct rda_nand_info *info)
{
	unsigned long config_a, config_b;
	unsigned long clk_div=hal_calc_divider(info);

	// clk_div = 4;		// for APB2 = 48MHz,  48/7
	//clk_div = 11;		// for APB2 = 120MHz, 120/18
	//clk_div = 15;	// for APB2 = 240MHz, 240/28

	/* setup config_a and config_b */
	config_a = NANDFC_CYCLE(clk_div);
	config_b = 0;
	config_a |= NANDFC_CHIP_SEL(0x0e);
	config_a |= NANDFC_POLARITY_IO(0);	// set 0 invert IO
	config_a |= NANDFC_POLARITY_MEM(1);	// set 1 invert MEM

	switch (info->type) {
	case NAND_TYPE_512S:
	case NAND_TYPE_512L:
		printf("Not support 512 nand any more\n");
		return 1;
	case NAND_TYPE_2KS:
		config_a |= NANDFC_TIMING(0x7B);
		break;
	case NAND_TYPE_2KL:
	case NAND_TYPE_MLC2K:
		config_a |= NANDFC_TIMING(0x8B);
		break;
	case NAND_TYPE_MLC4K:
	case NAND_TYPE_SLC4K:
		config_a |= NANDFC_TIMING(0x8C);
		break;
	case NAND_TYPE_MLC8K:
	case NAND_TYPE_SLC8K:
		config_a |= NANDFC_TIMING(0x8c);
		config_b |= 0x20;
		break;
	case NAND_TYPE_MLC16K:
	case NAND_TYPE_SLC16K:
		config_a |= NANDFC_TIMING(0x8c) | (0x00 << 20);
		config_b |= 0x10;
		break;
	default:
		printf("invalid nand type\n");
		return -EINVAL;
	}

	//enable the 16bit mode;
	if (info->bus_width_16)
		config_a |= NANDFC_WDITH_16BIT(1);

	config_b |= NANDFC_HWECC(1);
	if (info->hard_ecc_hec)
		config_b |= NANDFC_ECC_MODE(0x02);
	__raw_writel(config_a, NANDFC_REG_CONFIG_A);
	__raw_writel(config_b, NANDFC_REG_CONFIG_B);

	/* set readid type, 0x06 for 8 bytes ID */
	__raw_writel(0x6, NANDFC_REG_IDTPYE);

#ifdef _TGT_AP_NAND_READDELAY
	{
	unsigned int delay;
	/* Set an interval of filter for erasing operation. */
	delay = __raw_readl(NANDFC_REG_DELAY);
	delay &= ~0xffff;
	delay |=  _TGT_AP_NAND_READDELAY;
	__raw_writel(delay, NANDFC_REG_DELAY);
	printf("Nand delay : 0x%x .\n",  (delay & 0xffff));
	}
#endif /* #if _TGT_AP_NAND_READDELAY */
	printf("Nand Init Done, %08lx %08lx\n", config_a, config_b);

	return 0;
}

static u8 nand_rda_read_byte(struct mtd_info *mtd)
{
	u8 ret;
	struct nand_chip *chip = mtd->priv;
	struct rda_nand_info *info = chip->priv;
	ret = *((u8 *) info->byte_buf + info->index);
	info->index++;
#ifdef NAND_DEBUG
	printf("nand_read_byte, ret = %02x\n", ret);
#endif
	return ret;
}

static u16 nand_rda_read_word(struct mtd_info *mtd)
{
	u16 ret;
	struct nand_chip *chip = mtd->priv;
	struct rda_nand_info *info = chip->priv;
	ret = *(u16 *) ((uint8_t *) info->byte_buf + info->index);
	info->index += 2;
#ifdef NAND_DEBUG
	printf("nand_read_word, ret = %04x\n", ret);
#endif
	return ret;
}

#ifdef CONFIG_NAND_RDA_DMA
static void nand_rda_dma_move_data(struct mtd_info *mtd, uint8_t * dst, uint8_t * src,
										int len, enum dma_data_direction dir)
{
	struct nand_chip *chip = mtd->priv;
	struct rda_nand_info *info = chip->priv;
	struct rda_dma_chan_params dma_param;
	dma_addr_t dst_phys_addr, src_phys_addr;
	void *dst_addr = (void *)dst;
	void *src_addr = (void *)src;
	int ret = 0;

	if (((u32)dst_addr & 0x7) != 0 || ((u32)src_addr & 0x7) != 0) {
		printf("ERROR, nand dma read buffer dst %p or src %p is not 8bytes aligned\n",
			dst_addr, src_addr);
		return;
	}

	if(dir == DMA_FROM_DEVICE){
		dst_phys_addr = dma_map_single(dst_addr, len, DMA_FROM_DEVICE);
		src_phys_addr = (dma_addr_t)src;
	}
	else if(dir == DMA_TO_DEVICE){
		dst_phys_addr = (dma_addr_t)dst;
		src_phys_addr = dma_map_single(src_addr, len, DMA_TO_DEVICE);
	}
	else{
		dst_phys_addr = dma_map_single(dst_addr, len, DMA_BIDIRECTIONAL);
		src_phys_addr = dma_map_single(src_addr, len, DMA_BIDIRECTIONAL);
	}

	dma_param.src_addr = src_phys_addr;
	dma_param.dst_addr = dst_phys_addr;
	dma_param.xfer_size = len;
	//dma_param.dma_mode = RDA_DMA_FR_MODE;
	dma_param.dma_mode = RDA_DMA_NOR_MODE;

	ret = rda_set_dma_params(info->dma_ch, &dma_param);
	if (ret < 0) {
		printf("rda nand : DMA failed to set parameter!\n");
		if(dir == DMA_FROM_DEVICE)
			dma_unmap_single(dst_addr, len, DMA_FROM_DEVICE);
		else if(dir == DMA_TO_DEVICE)
			dma_unmap_single(src_addr, len, DMA_TO_DEVICE);
		else{
			dma_unmap_single(dst_addr, len, DMA_BIDIRECTIONAL);
			dma_unmap_single(src_addr, len, DMA_BIDIRECTIONAL);
		}
		return;
	}

	/* use flush to avoid annoying unaligned warning */
	/* however, invalidate after the dma it the right thing to do */
	if(dir == DMA_FROM_DEVICE)
		flush_dcache_range((u32)dst_addr, (u32)(dst_addr + len));
	else if(dir == DMA_TO_DEVICE)
		flush_dcache_range((u32)src_addr, (u32)(src_addr + len));
	else{
		flush_dcache_range((u32)dst_addr, (u32)(dst_addr + len));
		flush_dcache_range((u32)src_addr, (u32)(src_addr + len));
	}

	rda_start_dma(info->dma_ch);
	rda_poll_dma(info->dma_ch);
	rda_stop_dma(info->dma_ch);

	/* use flush to avoid annoying unaligned warning */
	//invalidate_dcache_range((u32)addr, (u32)(addr + len));

	/* Free the specified physical address */
	if(dir == DMA_FROM_DEVICE)
		dma_unmap_single(dst_addr, len, DMA_FROM_DEVICE);
	else if(dir == DMA_TO_DEVICE)
		dma_unmap_single(src_addr, len, DMA_TO_DEVICE);
	else{
		dma_unmap_single(dst_addr, len, DMA_BIDIRECTIONAL);
		dma_unmap_single(src_addr, len, DMA_BIDIRECTIONAL);
	}
}
#endif/*CONFIG_NAND_RDA_DMA*/

static void nand_rda_logic_read_cache(struct mtd_info *mtd)
{
	struct nand_chip *chip = mtd->priv;
	struct rda_nand_info *info = chip->priv;
	unsigned long cmd_ret;

	hal_set_col_addr(info->col_addr);
	hal_send_cmd((unsigned char)info->cmd, info->page_addr);
	cmd_ret = hal_wait_cmd_complete();
	if (cmd_ret) {
		printf("logic read fail, cmd = %x, page_addr = %x, col_addr = %x\n",
		       info->cmd, info->page_addr, info->col_addr);
	}
}

static void nand_rda_read_cache(struct mtd_info *mtd)
{
	struct nand_chip *chip = mtd->priv;
	struct rda_nand_info *info = chip->priv;
	u8 *nand_ptr = (u8 *) (chip->IO_ADDR_R);
	u8 *u_nand_temp_buffer = &g_nand_flash_temp_buffer[0];

	if(info->nand_use_type > 0){
#ifndef CONFIG_NAND_RDA_DMA
		memcpy((void *)u_nand_temp_buffer,
				(void *)nand_ptr,
				info->logic_page_size);
		memcpy((void *)(u_nand_temp_buffer + mtd->writesize),
				(void *)(nand_ptr + info->logic_page_size),
				info->logic_oob_size);
#else
		nand_rda_dma_move_data(mtd,
				u_nand_temp_buffer,
				nand_ptr,
				info->logic_page_size,
				DMA_FROM_DEVICE);
		nand_rda_dma_move_data(mtd,
				(uint8_t*)(u_nand_temp_buffer + mtd->writesize),
				(uint8_t*)(nand_ptr + info->logic_page_size),
				info->logic_oob_size,
				DMA_FROM_DEVICE);
#endif
		//printf("\nread chip buffer0:\n");
		//rda_dump_buf(nand_ptr, 4320);

		info->col_addr += NAND_CONTROLLER_TEMP_BUFFER_SIZE;
		hal_set_col_cmd(0xe0);
		info->cmd = NAND_CMD_RNDOUT;
		nand_rda_logic_read_cache(mtd);

#ifndef CONFIG_NAND_RDA_DMA
		memcpy((void *)(u_nand_temp_buffer + info->logic_page_size),
				(void *)nand_ptr,
				info->logic_page_size);
		memcpy((void *)(u_nand_temp_buffer + mtd->writesize + info->logic_oob_size),
				(void *)(nand_ptr + info->logic_page_size),
				info->logic_oob_size);
#else
		nand_rda_dma_move_data(mtd,
				(uint8_t*)(u_nand_temp_buffer + info->logic_page_size),
				nand_ptr,
				info->logic_page_size,
				DMA_FROM_DEVICE);
		nand_rda_dma_move_data(mtd,
				(uint8_t*)(u_nand_temp_buffer + mtd->writesize + info->logic_oob_size),
				(uint8_t*)(nand_ptr + info->logic_page_size),
				info->logic_oob_size,
				DMA_FROM_DEVICE);
#endif

		//printf("\nread chip buffer1:\n");
		//rda_dump_buf(nand_ptr, 4320);
		if(info->type == NAND_TYPE_SLC16K || info->type == NAND_TYPE_MLC16K){
			info->col_addr += NAND_CONTROLLER_TEMP_BUFFER_SIZE;
			nand_rda_logic_read_cache(mtd);

#ifndef CONFIG_NAND_RDA_DMA
			memcpy((void *)(u_nand_temp_buffer + info->logic_page_size * 2),
					(void *)nand_ptr,
					info->logic_page_size);
			memcpy((void *)(u_nand_temp_buffer + mtd->writesize + info->logic_oob_size * 2),
					(void *)(nand_ptr + info->logic_page_size),
					info->logic_oob_size);
#else
			nand_rda_dma_move_data(mtd,
					(uint8_t*)(u_nand_temp_buffer + info->logic_page_size * 2),
					nand_ptr,
					info->logic_page_size,
					DMA_FROM_DEVICE);
			nand_rda_dma_move_data(mtd,
					(uint8_t*)(u_nand_temp_buffer + mtd->writesize + info->logic_oob_size * 2),
					(uint8_t*)(nand_ptr + info->logic_page_size),
					info->logic_oob_size,
					DMA_FROM_DEVICE);
#endif

			info->col_addr += NAND_CONTROLLER_TEMP_BUFFER_SIZE;
			nand_rda_logic_read_cache(mtd);

#ifndef CONFIG_NAND_RDA_DMA
			memcpy((void *)(u_nand_temp_buffer + info->logic_page_size * 3),
					(void *)nand_ptr,
					info->logic_page_size);
			memcpy((void *)(u_nand_temp_buffer + mtd->writesize + info->logic_oob_size * 3),
					(void *)(nand_ptr + info->logic_page_size),
					info->logic_oob_size);
#else
			nand_rda_dma_move_data(mtd,
					(uint8_t*)(u_nand_temp_buffer + info->logic_page_size * 3),
					nand_ptr,
					info->logic_page_size,
					DMA_FROM_DEVICE);
			nand_rda_dma_move_data(mtd,
					(uint8_t*)(u_nand_temp_buffer + mtd->writesize + info->logic_oob_size * 3),
					(uint8_t*)(nand_ptr + info->logic_page_size),
					info->logic_oob_size,
					DMA_FROM_DEVICE);
#endif
		}
		hal_set_col_cmd(0x30);
		hal_set_col_addr(0);
	}
}

static void nand_rda_read_buf(struct mtd_info *mtd, uint8_t * buf, int len)
{
	struct nand_chip *chip = mtd->priv;
	struct rda_nand_info *info = chip->priv;
	u8 *nand_ptr = (u8 *) (chip->IO_ADDR_R + info->read_ptr);
	u8 *u_nand_temp_buffer = &g_nand_flash_temp_buffer[info->read_ptr];
#if (OOB_SW_PROTECT_SWITCH_ON == 1)
	uint8_t *oob_temp_ptr = &g_nand_flash_temp_buffer[16384+512];
	uint8_t oob_byte_count = 2 + __FTL_OOB_STRUCT_LENGTH__ + 2 + 4;
	uint8_t oob_copy_number = info->vir_oob_size / oob_byte_count;
	uint32_t checksum, checksum_load;
	u8 i;
#endif

#ifdef NAND_DEBUG
	printf("read : buf addr = 0x%p, len = %d, read_ptr = %d\n", buf, len,
	       info->read_ptr);
#endif /* NAND_DEBUG */

#if 1//ndef CONFIG_NAND_RDA_DMA
	if(info->nand_use_type > 0){
#if (OOB_SW_PROTECT_SWITCH_ON == 1)
		memset(oob_temp_ptr, 0xff, info->vir_oob_size);
		if(info->read_ptr > 0){
			if(info->vir_page_size == 6144 || info->vir_page_size == 12288){
				for(i = 0; i < oob_copy_number; i++){
					u32 index = i * oob_byte_count + oob_byte_count;

					checksum = nand_checksum32(&u_nand_temp_buffer[i * oob_byte_count],
								oob_byte_count - 4);
					checksum_load = u_nand_temp_buffer[index - 4];
					checksum_load |= u_nand_temp_buffer[index - 3] << 8;
					checksum_load |= u_nand_temp_buffer[index - 2] << 16;
					checksum_load |= u_nand_temp_buffer[index - 1] << 24;
					if(checksum == checksum_load || checksum_load == ~0x0){
						memcpy(oob_temp_ptr,
							&u_nand_temp_buffer[i * oob_byte_count],
							oob_byte_count);
						break;
					}
				}
				if(i == oob_copy_number){
					printf("6k or 12k oob ecc warning, data select!!!! \n");
					nand_sw_ecc_check(u_nand_temp_buffer,
						oob_temp_ptr,
						oob_copy_number,
						oob_byte_count - 4);
				}

				u_nand_temp_buffer = oob_temp_ptr;
			}
		}
#endif
		memcpy((void *)buf, (void *)u_nand_temp_buffer, len);
	}else{
#if (OOB_SW_PROTECT_SWITCH_ON == 1)
		if(info->read_ptr > 0){
			if(info->vir_page_size == 3072){
				for(i = 0; i < oob_copy_number; i++){
					u32 index = i * oob_byte_count + oob_byte_count;

					checksum = nand_checksum32(&nand_ptr[i * oob_byte_count],
						oob_byte_count - 4);
					checksum_load = nand_ptr[index - 4];
					checksum_load |= nand_ptr[index - 3] << 8;
					checksum_load |= nand_ptr[index - 2] << 16;
					checksum_load |= nand_ptr[index - 1] << 24;
					if(checksum == checksum_load || checksum_load == ~0x0){
						memcpy(oob_temp_ptr, &nand_ptr[i * oob_byte_count], oob_byte_count);
						break;
					}
				}
				if(i == oob_copy_number){
					printf("3k oob ecc warning, data select!!!! \n");
					nand_sw_ecc_check(nand_ptr,
						oob_temp_ptr,
						oob_copy_number,
						oob_byte_count - 4);
				}

				nand_ptr = oob_temp_ptr;
			}
		}
#endif
		memcpy((void *)buf, (void *)nand_ptr, len);
	}
	info->read_ptr += len;

#ifdef NAND_DEBUG_VERBOSE
	rda_dump_buf((char *)buf, len);
#endif

#else
	struct rda_dma_chan_params dma_param;
	dma_addr_t phys_addr;
	void *addr = (void *)buf;
	int ret = 0;

	/*
	 * If size is less than the size of oob,
	 * we copy directly them to mapping buffer.
	 */
	if (len <= mtd->oobsize) {
		memcpy(buf, (void *)nand_ptr, len);
		info->read_ptr = 0;
		return;
	}

	if (((u32)addr & 0x7) != 0) {
		printf("ERROR, nand dma read buffer %p is not 8bytes aligned\n",
			addr);
		return;
	}

	phys_addr = dma_map_single(addr, len, DMA_FROM_DEVICE);

	dma_param.src_addr = (u32) info->nand_data_phys;
	dma_param.dst_addr = phys_addr;
	dma_param.xfer_size = len;
	//dma_param.dma_mode = RDA_DMA_FR_MODE;
	dma_param.dma_mode = RDA_DMA_NOR_MODE;

	ret = rda_set_dma_params(info->dma_ch, &dma_param);
	if (ret < 0) {
		printf("rda nand : Failed to set parameter\n");
		dma_unmap_single(addr, len, DMA_FROM_DEVICE);
		return;
	}

	/* use flush to avoid annoying unaligned warning */
	/* however, invalidate after the dma it the right thing to do */
	flush_dcache_range((u32)addr, (u32)(addr + len));

	rda_start_dma(info->dma_ch);
	rda_poll_dma(info->dma_ch);
	rda_stop_dma(info->dma_ch);

	/* use flush to avoid annoying unaligned warning */
	//invalidate_dcache_range((u32)addr, (u32)(addr + len));

	/* Free the specified physical address */
	dma_unmap_single(addr, len, DMA_FROM_DEVICE);
	info->read_ptr += len;

	return;
#endif /* CONFIG_NAND_RDA_DMA */
}

static void nand_rda_write_buf(struct mtd_info *mtd, const uint8_t * buf,
			       int len)
{
	struct nand_chip *chip = mtd->priv;
	struct rda_nand_info *info = chip->priv;
	u8 *nand_ptr = (u8 *) (chip->IO_ADDR_W + info->write_ptr);
	u8 *u_nand_temp_buffer = &g_nand_flash_temp_buffer[info->write_ptr];
	uint8_t *temp_ptr = (uint8_t *)buf;
#if (OOB_SW_PROTECT_SWITCH_ON == 1)
	uint8_t *oob_temp_ptr = &g_nand_flash_temp_buffer[16384+512];
	uint8_t oob_byte_count = 2 + __FTL_OOB_STRUCT_LENGTH__ + 2 + 4;
	uint8_t oob_copy_number = info->vir_oob_size / oob_byte_count;
	uint32_t checksum;
#endif

#ifdef NAND_DEBUG
	printf("write : buf addr = 0x%p, len = %d, write_ptr = %d\n", buf, len,
	       info->write_ptr);
#endif /* NAND_DEBUG */

#ifdef NAND_DEBUG_VERBOSE
	rda_dump_buf((char *)buf, len);
#endif

#if 1//ndef CONFIG_NAND_RDA_DMA
	if(info->nand_use_type > 0){
		if(info->write_ptr > 0){/*write OOB to buffer*/
#if (OOB_SW_PROTECT_SWITCH_ON == 1)
			if(info->vir_page_size == 6144 ||info->vir_page_size == 12288){
				checksum = nand_checksum32(temp_ptr, oob_byte_count - 4);
				memcpy(oob_temp_ptr, temp_ptr, oob_byte_count - 4);
				oob_temp_ptr[oob_byte_count - 4] = (uint8_t)(checksum & 0xff);
				oob_temp_ptr[oob_byte_count - 3] = (uint8_t)((checksum >> 8) & 0xff);
				oob_temp_ptr[oob_byte_count - 2] = (uint8_t)((checksum >> 16) & 0xff);
				oob_temp_ptr[oob_byte_count - 1] = (uint8_t)((checksum >> 24) & 0xff);
				while(oob_copy_number > 1){
					oob_copy_number--;
					memcpy(&oob_temp_ptr[oob_copy_number * oob_byte_count],
						oob_temp_ptr,
						oob_byte_count);
				}

				temp_ptr = oob_temp_ptr;
			}
#endif
			info->write_ptr = info->logic_page_size;
			u_nand_temp_buffer = &g_nand_flash_temp_buffer[info->write_ptr];
			memcpy((void *)u_nand_temp_buffer, (void *)temp_ptr, info->logic_oob_size);
			memcpy((void *)(u_nand_temp_buffer + NAND_CONTROLLER_TEMP_BUFFER_SIZE),
							(void *)(temp_ptr + info->logic_oob_size), info->logic_oob_size);
			if(info->type == NAND_TYPE_SLC16K || info->type == NAND_TYPE_MLC16K){
				memcpy((void *)(u_nand_temp_buffer + NAND_CONTROLLER_TEMP_BUFFER_SIZE * 2),
							(void *)(temp_ptr + info->logic_oob_size * 2), info->logic_oob_size);
				memcpy((void *)(u_nand_temp_buffer + NAND_CONTROLLER_TEMP_BUFFER_SIZE * 3),
							(void *)(temp_ptr + info->logic_oob_size * 3), info->logic_oob_size);
			}
		}else{/*write data to buffer*/
			memcpy((void *)u_nand_temp_buffer, (void *)temp_ptr, info->logic_page_size);
			memcpy((void *)(u_nand_temp_buffer + NAND_CONTROLLER_TEMP_BUFFER_SIZE),
							(void *)(temp_ptr + info->logic_page_size), info->logic_page_size);
			if(info->type == NAND_TYPE_SLC16K || info->type == NAND_TYPE_MLC16K){
				memcpy((void *)(u_nand_temp_buffer + NAND_CONTROLLER_TEMP_BUFFER_SIZE * 2),
							(void *)(temp_ptr + info->logic_page_size * 2), info->logic_page_size);
				memcpy((void *)(u_nand_temp_buffer + NAND_CONTROLLER_TEMP_BUFFER_SIZE * 3),
							(void *)(temp_ptr + info->logic_page_size * 3), info->logic_page_size);
			}
			info->write_ptr += info->logic_page_size;
		}
	}else{
#if (OOB_SW_PROTECT_SWITCH_ON == 1)
		if(info->write_ptr > 0 && info->vir_page_size == 3072){
			checksum = nand_checksum32(temp_ptr, oob_byte_count - 4);
			memcpy(oob_temp_ptr, temp_ptr, oob_byte_count - 4);
			oob_temp_ptr[oob_byte_count - 4] = (uint8_t)(checksum & 0xff);
			oob_temp_ptr[oob_byte_count - 3] = (uint8_t)((checksum >> 8) & 0xff);
			oob_temp_ptr[oob_byte_count - 2] = (uint8_t)((checksum >> 16) & 0xff);
			oob_temp_ptr[oob_byte_count - 1] = (uint8_t)((checksum >> 24) & 0xff);
			while(oob_copy_number > 1){
				oob_copy_number--;
				memcpy(&oob_temp_ptr[oob_copy_number * oob_byte_count],
					oob_temp_ptr,
					oob_byte_count);
			}

			temp_ptr = oob_temp_ptr;
		}
#endif
		memcpy((void *)nand_ptr, (void *)temp_ptr, len);
		info->write_ptr += len;
	}
#else
	struct rda_dma_chan_params dma_param;
	dma_addr_t phys_addr;
	void *addr = (void *)buf;
	int ret = 0;

	/*
	 * If size is less than the size of oob,
	 * we copy directly them to mapping buffer.
	 */
	if (len <= mtd->oobsize) {
		memcpy((void *)nand_ptr, (void *)buf, len);
		info->write_ptr += len;
		return;
	}

	if (((u32)addr & 0x7) != 0) {
		printf("ERROR, nand dma write %p buffer is not 8bytes aligned\n",
			addr);
		return;
	}
	phys_addr = dma_map_single(addr, len, DMA_TO_DEVICE);

	dma_param.src_addr = phys_addr;
	dma_param.dst_addr = (u32) info->nand_data_phys;
	dma_param.xfer_size = len;
	//dma_param.dma_mode = RDA_DMA_FW_MODE;
	dma_param.dma_mode = RDA_DMA_NOR_MODE;

	ret = rda_set_dma_params(info->dma_ch, &dma_param);
	if (ret < 0) {
		printf("rda nand : Failed to set parameter\n");
		dma_unmap_single(addr, len, DMA_TO_DEVICE);
		return;
	}

	flush_dcache_range((u32)addr, (u32)(addr + len));

	rda_start_dma(info->dma_ch);
	rda_poll_dma(info->dma_ch);
	rda_stop_dma(info->dma_ch);

	/* Free the specified physical address */
	dma_unmap_single(addr, len, DMA_TO_DEVICE);
	info->write_ptr += len;

	return;
#endif /* CONFIG_NAND_RDA_DMA */
}

static void nand_rda_write_cache(struct mtd_info *mtd)
{
	struct nand_chip *chip = mtd->priv;
	struct rda_nand_info *info = chip->priv;
	u8 *nand_ptr = (u8 *) (chip->IO_ADDR_W + info->write_ptr);
	u8 *u_nand_temp_buffer = &g_nand_flash_temp_buffer[info->col_addr];

	if(info->nand_use_type > 0){
#ifndef CONFIG_NAND_RDA_DMA
		memcpy((void *)nand_ptr,
			(void *)u_nand_temp_buffer,
			NAND_CONTROLLER_TEMP_BUFFER_SIZE);
#else
		nand_rda_dma_move_data(mtd,
			nand_ptr,
			u_nand_temp_buffer,
			NAND_CONTROLLER_TEMP_BUFFER_SIZE,
			DMA_TO_DEVICE);
#endif
	}
}

static void nand_rda_do_cmd_pre(struct mtd_info *mtd)
{
	struct nand_chip *this = mtd->priv;
	struct rda_nand_info *info = this->priv;

	__raw_writel(0xfff, NANDFC_REG_INT_STAT);
	switch (info->cmd) {
	case NAND_CMD_SEQIN:
		info->write_ptr = 0;
		nand_rda_write_cache(mtd);
		break;
	case NAND_CMD_READ0:
		hal_flush_buf(mtd);
		//info->read_ptr = 0;
		break;
	case NAND_CMD_RNDIN:
		nand_rda_write_cache(mtd);
		break;
	default:
		break;
	}
}

static void nand_rda_do_cmd_post(struct mtd_info *mtd)
{
	struct nand_chip *this = mtd->priv;
	struct rda_nand_info *info = this->priv;
	u32 temp;

	switch (info->cmd) {
	case NAND_CMD_RESET:
		return;
	case NAND_CMD_READID:
		temp = __raw_readl(NANDFC_REG_IDCODE_A);
		info->byte_buf[0] = temp;
		temp = __raw_readl(NANDFC_REG_IDCODE_B);
		info->byte_buf[1] = temp;
		info->index = 0;
		break;
	case NAND_CMD_STATUS:
		temp = __raw_readl(NANDFC_REG_OP_STATUS);
		info->byte_buf[0] = (temp & 0xFF);
		info->index = 0;
		if (info->byte_buf[0] != 0xe0) {
			printf("nand error in op status %x\n",
			       info->byte_buf[0]);
		}
#ifdef NAND_DEBUG
		printf("post_cmd read status %x\n", info->byte_buf[0]);
#endif
		break;
	case NAND_CMD_READ0:	// NAND_CMD_READOOB goes here too
		nand_rda_read_cache(mtd);
		info->index = 0;
		if(NAND_CMD_READOOB == info->cmd_flag){
			if(info->nand_use_type > 0){
				temp = g_nand_flash_temp_buffer[mtd->writesize];
				temp |= g_nand_flash_temp_buffer[mtd->writesize+1] << 8;
				info->byte_buf[0] = temp;
			}else{
				u8 *nand_ptr = (u8 *) (this->IO_ADDR_R + mtd->writesize);
				memcpy((u8*)&info->byte_buf[0], nand_ptr, 2);
			}
			info->cmd_flag = NAND_CMD_NONE;
		}
		break;
	case NAND_CMD_SEQIN:
		if(info->nand_use_type > 0){
			info->logic_operate_time += 1;
			info->col_addr += NAND_CONTROLLER_TEMP_BUFFER_SIZE;
			info->cmd = NAND_CMD_RNDIN;
		}
		break;
	case NAND_CMD_RNDIN:
		if(info->nand_use_type > 0){
			info->logic_operate_time += 1;
			info->col_addr += NAND_CONTROLLER_TEMP_BUFFER_SIZE;
		}
		break;
/* delay 10ms, for erase to complete */
	case NAND_CMD_ERASE1:
		udelay(10000);
		break;

	default:
		break;
	}
}

static void nand_rda_hwcontrol(struct mtd_info *mtd, int cmd, unsigned int ctrl)
{
#ifdef NAND_DEBUG
        printf("nand_rda_hwcontrol, cmd = %x, ctrl = %x, not use anymore\n",
               cmd, ctrl);
#endif
}

static void nand_rda_cmdfunc(struct mtd_info *mtd, unsigned int command,
			     int column, int page_addr)
{
	register struct nand_chip *chip = mtd->priv;
	struct rda_nand_info *info = chip->priv;
	unsigned long cmd_ret;

#ifdef NAND_DEBUG
	printf("nand_rda_cmdfunc, cmd = %x, page_addr = %x, col_addr = %x\n",
	       command, page_addr, column);
#endif

	/* remap command */
	switch (command) {
	case NAND_CMD_SEQIN:	/* 0x80 do nothing, wait for 0x10 */
		info->page_addr = page_addr;
		info->col_addr = column;
		info->cmd = NAND_CMD_NONE;
		info->write_ptr= column;
		info->logic_operate_time = 0;
		break;
	case NAND_CMD_READSTART:	/* hw auto gen 0x30 for read */
	case NAND_CMD_ERASE2:	/* hw auto gen 0xd0 for erase */
#ifdef NAND_DEBUG
		printf("erase block, erase_size = %x\n", mtd->erasesize);
#endif
		info->cmd = NAND_CMD_NONE;
		break;
	case NAND_CMD_PAGEPROG:	/* 0x10 do the real program */
		info->cmd = NAND_CMD_SEQIN;
		info->col_addr = 0;
		break;
	case NAND_CMD_READOOB:	/* Emulate NAND_CMD_READOOB */
		info->page_addr = page_addr;
		info->cmd = NAND_CMD_READ0;
		/* Offset of oob */
		info->col_addr = column;
		info->read_ptr = mtd->writesize;
		info->cmd_flag = NAND_CMD_READOOB;
		break;
	case NAND_CMD_READ0:	/* Emulate NAND_CMD_READOOB */
		info->read_ptr = 0;
		/*fall though, don't need break;*/
	default:
		info->page_addr = page_addr;
		info->col_addr = column;
		info->cmd = command;
		break;
	}
	if (info->cmd == NAND_CMD_NONE) {
		return;
	}
	//info->col_addr >>= 1;
#ifdef NAND_DEBUG
	printf("after cmd remap, cmd = %x, page_addr = %x, col_addr = %x\n",
	       info->cmd, info->page_addr, info->col_addr);
#endif

loop:
	if (info->col_addr != -1) {
		hal_set_col_addr(info->col_addr);
	}
	if (info->page_addr == -1)
		info->page_addr = 0;

	nand_rda_do_cmd_pre(mtd);

	hal_send_cmd((unsigned char)info->cmd, info->page_addr);
	cmd_ret = hal_wait_cmd_complete();
	if (cmd_ret) {
		printf("cmd fail, cmd = %x, page_addr = %x, col_addr = %x\n",
		       info->cmd, info->page_addr, info->col_addr);
	}

	nand_rda_do_cmd_post(mtd);

	if(info->nand_use_type > 0 && info->logic_operate_time <= info->nand_use_type && \
		(info->cmd == NAND_CMD_SEQIN || info->cmd == NAND_CMD_RNDIN))
		goto loop;

	nand_wait_ready(mtd);
}

static int nand_rda_dev_ready(struct mtd_info *mtd)
{
	return 1;
}

static int nand_reset_flash(void)
{
	int ret = 0;

	hal_send_cmd(NAND_CMD_RESET, 0);
	ret = hal_wait_cmd_complete();
	if (ret) {
		printf("reset flash failed\n");
		return ret;
	}

	return ret;
}

static int nand_read_id(unsigned int id[2])
{
	int ret = 0;

	hal_send_cmd(NAND_CMD_READID, 0);
	ret = hal_wait_cmd_complete();
	if (ret) {
		printf("nand_read_id failed\n");
		return ret;
	}

	id[0] = __raw_readl(NANDFC_REG_IDCODE_A);
	id[1] = __raw_readl(NANDFC_REG_IDCODE_B);

	printf("Nand ID: %08x %08x\n", id[0], id[1]);

	return ret;
}

static void read_ID(struct mtd_info *mtd)
{
	unsigned int id[2];

	nand_read_id(id);
}

#if(__TGT_AP_FLASH_USE_PART_OF_PAGE__ == 1)
static void nand_page_type_setting(struct rda_nand_info *info, int *bus_width_16)
{
#if(__TGT_AP_FLASH_LOGIC_PAGE_SIZE_N_KB__ == __TGT_AP_FLASH_USE_4KAS3K_PAGE_DIV__)
	printf("MLC 4K as 3K, %s bus\n", (*bus_width_16)?"16bit":"8bit");
	info->spl_adjust_ratio = 3;//actally ratio * 2
#else
#if(__TGT_AP_FLASH_LOGIC_PAGE_SIZE_N_KB__ == __TGT_AP_FLASH_USE_4KAS2K_PAGE_DIV__)
	printf("MLC 4K as 2K, %s bus\n", (*bus_width_16)?"16bit":"8bit");
	info->spl_adjust_ratio = 2;//actally ratio * 2
#endif
#endif

#if(__TGT_AP_FLASH_LOGIC_PAGE_SIZE_N_KB__ == __TGT_AP_FLASH_USE_8KAS4K_PAGE_DIV__)
	printf("MLC 8K as 4K, %s bus\n", (*bus_width_16)?"16bit":"8bit");
	info->spl_adjust_ratio = 4;//actally ratio * 2
#else
#if(__TGT_AP_FLASH_LOGIC_PAGE_SIZE_N_KB__ == __TGT_AP_FLASH_USE_8KAS6K_PAGE_DIV__)
	printf("MLC 8K as 6K, %s bus\n", (*bus_width_16)?"16bit":"8bit");
	info->spl_adjust_ratio = 6;//actally ratio * 2
#endif
#endif

#if(__TGT_AP_FLASH_LOGIC_PAGE_SIZE_N_KB__ == __TGT_AP_FLASH_USE_16KAS8K_PAGE_DIV__)
	printf("MLC 16K as 8K, %s bus\n", (*bus_width_16)?"16bit":"8bit");
	info->spl_adjust_ratio = 8;//actally ratio * 2
#else
#if(__TGT_AP_FLASH_LOGIC_PAGE_SIZE_N_KB__ == __TGT_AP_FLASH_USE_16KAS12K_PAGE_DIV__)
	printf("MLC 16K as 12K, %s bus\n", (*bus_width_16)?"16bit":"8bit");
	info->spl_adjust_ratio = 12;//actally ratio * 2
#endif
#endif
}
#endif

/********************************************************************************
******************page size bits(bit[1...0] of the fourth id byte)***************
1.hynix nand
 1) bit1  bit0  pagesize       2) bit1  bit0  pagesize
      0    0        2K             0    0        4K
      0    1        4k             0    1        8K
      1    0        8k             1    0        16K
      1    1        null           1    1        32K
 nand ID:
 (1)AD D7 94 DA 74 C3          (1)AD D7 94 91 60 44
*********************************************************************************/
static int nand_get_type(struct rda_nand_info *info, unsigned int id[2],
		int *rda_nand_type, int *bus_width_16)
{
	u16 hwcfg_nand_type;
	int metal_id;

	info->spl_adjust_ratio = 2;
	metal_id = rda_metal_id_get();

#if defined(CONFIG_MACH_RDA8810) || defined(CONFIG_MACH_RDA8850)
	printf("RDA8810/RDA8850 get type, metal %d\n", metal_id);

	if (metal_id == 0) {
		hwcfg_nand_type = rda_hwcfg_get() & 0x03;
		switch(hwcfg_nand_type) {
		case 0b00:
			printf("MLC 4K, 8bit bus\n");
			*rda_nand_type = NAND_TYPE_MLC4K;
			*bus_width_16 = 0;
			break;
		case 0b10:
			printf("SLC 4K page, 16bit bus\n");
			*rda_nand_type = NAND_TYPE_SLC4K;
			*bus_width_16 = 1;
			break;
		case 0b01:
			printf("2K page, 8bit bus\n");
			*rda_nand_type = NAND_TYPE_2KL;
			*bus_width_16 = 0;
			break;
		case 0b11:
			printf("2K page, 16bit bus\n");
			*rda_nand_type = NAND_TYPE_2KL;
			*bus_width_16 = 1;
			break;
		default:
			printf("NAND: type %d not support\n", hwcfg_nand_type);
			*rda_nand_type = NAND_TYPE_INVALID;
			return -ENODEV;
		}
	}
	else if ((metal_id == 2) || (metal_id == 1)) {
		hwcfg_nand_type = rda_hwcfg_get() & 0x03;
		switch(hwcfg_nand_type) {
		case 0b00:
			printf("SLC 4K, 16bit bus\n");
			*rda_nand_type = NAND_TYPE_SLC4K;
			*bus_width_16 = 1;
			break;
		case 0b10:
			printf("MLC 4K, 8bit bus\n");
			*rda_nand_type = NAND_TYPE_MLC4K;
			*bus_width_16 = 0;
			break;
		case 0b11:
			printf("MLC 8K, 8bit bus\n");
			*rda_nand_type = NAND_TYPE_MLC8K;
			*bus_width_16 = 0;
			break;
		default:
			printf("NAND: type %d not support\n", hwcfg_nand_type);
			*rda_nand_type = NAND_TYPE_INVALID;
			return -ENODEV;
		}
	}
	else if (metal_id >= 3) {
#define RDA_NAND_TYPE_BIT_TYPE    (1 << 0)    /* 1: MLC; 0: SLC */
#define RDA_NAND_TYPE_BIT_BUS     (1 << 1)    /* 1: 8-bit; 0: 16-bit */
#define RDA_NAND_TYPE_BIT_SIZE    (1 << 7)    /* 1: 4K SLC / 8K MLC; 0: 2K SLC / 4K MLC */
		hwcfg_nand_type = rda_hwcfg_get() & 0x83;
		if (hwcfg_nand_type & RDA_NAND_TYPE_BIT_BUS)
			*bus_width_16 = 0;
		else
			*bus_width_16 = 1;
		if (hwcfg_nand_type & RDA_NAND_TYPE_BIT_TYPE) {
			if (hwcfg_nand_type & RDA_NAND_TYPE_BIT_SIZE) {
				printf("MLC 8K, %s bus\n",
					(*bus_width_16)?"16bit":"8bit");
				*rda_nand_type = NAND_TYPE_MLC8K;
			} else {
#if(__TGT_AP_FLASH_USE_PART_OF_PAGE__ == 1)
				switch(id[0] & 0xff){
				case NAND_MFR_HYNIX:
					if(id[0] == 0xda94d7ad || id[0] == 0x9e14d7ad){//hynix for nanya
						*rda_nand_type = NAND_TYPE_MLC8K;
					}else{
						switch((id[0] >> 24)&0x3){//MLC
						case 0:
							*rda_nand_type = NAND_TYPE_MLC4K;
							break;
						case 1:
							*rda_nand_type = NAND_TYPE_MLC8K;
							break;
						case 2:
							*rda_nand_type = NAND_TYPE_MLC16K;
							break;
						case 3:
							*rda_nand_type = NAND_TYPE_MLC16K;//reserved for 32k
							break;
						}
					}
					break;
				case NAND_MFR_TOSHIBA://mlc
				case NAND_MFR_SAMSUNG:
					switch((id[0] >> 24)&0x3){
					case 0:
						printf("nand:no 2k MLC nand.\n");
						break;
					case 1:
						*rda_nand_type = NAND_TYPE_MLC4K;
						break;
					case 2:
						*rda_nand_type = NAND_TYPE_MLC8K;
						break;
					case 3:
						*rda_nand_type = NAND_TYPE_MLC16K;
						break;
					}
					break;
				default:
					switch((id[0] >> 24)&0x3){
					case 0:
						printf("nand:not support 1k nand.\n");
						break;
					case 1:
						printf("nand:no 2k MLC nand.\n");
						break;
					case 2:
						*rda_nand_type = NAND_TYPE_MLC4K;
						break;
					case 3:
						*rda_nand_type = NAND_TYPE_MLC8K;
						break;
					}
					break;
				}
				nand_page_type_setting(info, bus_width_16);
#else
				printf("MLC 4K, %s bus\n", (*bus_width_16)?"16bit":"8bit");
				*rda_nand_type = NAND_TYPE_MLC4K;
#endif
			}
		} else {
			if (hwcfg_nand_type & RDA_NAND_TYPE_BIT_SIZE) {
					printf("SLC 4K, %s bus\n", (*bus_width_16)?"16bit":"8bit");
					*rda_nand_type = NAND_TYPE_SLC4K;
			} else {
				printf("SLC 2KL, %s bus\n",
					(*bus_width_16)?"16bit":"8bit");
				*rda_nand_type = NAND_TYPE_2KL;
			}
		}
	}
	else {
		printf("NAND: invalid metal id %d\n", metal_id);
		return -ENODEV;
	}
#elif defined(CONFIG_MACH_RDA8810E) || defined(CONFIG_MACH_RDA8820)
#error "RDA8810E/RDA8820 does not match RDA NAND Controller V1"
#else
#error "unknown MACH"
#endif

	printf("SPL adjust ratio is %d. \n", info->spl_adjust_ratio);
	return 0;
}

static int nand_rda_init(struct nand_chip *this, struct rda_nand_info *info)
{
	unsigned int id[2];
	int ret = 0;
	int rda_nand_type = NAND_TYPE_INVALID;
	int bus_width_16 = 0;

	ret = nand_reset_flash();
	if (ret)
		return ret;
	ret = nand_read_id(id);
	if (ret)
		return ret;
	ret = nand_get_type(info, id, &rda_nand_type, &bus_width_16);
	if (ret)
		return ret;
	ret = init_nand_info(info, rda_nand_type, bus_width_16);
	if (ret)
		return ret;
	if (info->bus_width_16)
		this->options |= NAND_BUSWIDTH_16;

	return hal_init(info);
}

static 	int nand_rda_init_size(struct mtd_info *mtd, struct nand_chip *this,
			u8 *id_data)
{
	struct rda_nand_info *info = this->priv;

	mtd->erasesize = info->vir_erase_size;
	mtd->writesize = info->vir_page_size;
	mtd->oobsize = info->vir_oob_size;

	return (info->bus_width_16) ? NAND_BUSWIDTH_16 : 0;
}

int rda_nand_init(struct nand_chip *nand)
{
	struct rda_nand_info *info;
	static struct rda_nand_info rda_nand_info;

	info = &rda_nand_info;

	nand->chip_delay = 0;
#ifdef CONFIG_SYS_NAND_USE_FLASH_BBT
	nand->options |= NAND_USE_FLASH_BBT;
#endif
	/*
	 * in fact, nand controler we do hardware ECC silently, and
	 * we don't need tell mtd layer, and will simple nand operations
	*/
	nand->ecc.mode = NAND_ECC_NONE;
	/* Set address of hardware control function */
	nand->cmd_ctrl = nand_rda_hwcontrol;
	nand->init_size = nand_rda_init_size;
	nand->cmdfunc = nand_rda_cmdfunc;

	nand->read_byte = nand_rda_read_byte;
	nand->read_word = nand_rda_read_word;
	nand->read_buf = nand_rda_read_buf;
	nand->write_buf = nand_rda_write_buf;
	nand->read_nand_ID = read_ID;
	nand->dev_ready = nand_rda_dev_ready;

	nand->priv = (void *)info;
	nand->IO_ADDR_R = nand->IO_ADDR_W = (void __iomem *)NANDFC_DATA_BUF;
#ifdef CONFIG_NAND_RDA_DMA
	info->nand_data_phys = (void __iomem *)NANDFC_DATA_BUF;
#endif /* CONFIG_NAND_RDA_DMA */
	info->index = 0;
	info->write_ptr = 0;
	info->read_ptr = 0;
	info->cmd_flag = NAND_CMD_NONE;
	info->master_clk = get_master_clk_rate(_TGT_AP_CLK_APB2);
	info->clk=_TGT_AP_NAND_CLOCK;

#ifdef CONFIG_NAND_RDA_DMA
	rda_request_dma(&info->dma_ch);
#endif /* CONFIG_NAND_RDA_DMA */

	 nand_rda_init(nand, info);

	if (!nand->ecc.layout && (nand->ecc.mode != NAND_ECC_SOFT_BCH)) {
		switch (info->vir_oob_size) {
		case 64:
			nand->ecc.layout = &rda_nand_oob_64;
			break;
		case 128:
			nand->ecc.layout = &rda_nand_oob_128;
			break;
		case 256:
			nand->ecc.layout = &rda_nand_oob_256;
			break;
		case 512:
			nand->ecc.layout = &rda_nand_oob_512;
			break;
		default:
			printf("error oob size: %d \n", info->vir_oob_size);
		}
	}

	return 0;
}

#else
int rda_nand_init(struct nand_chip *nand) {return 0;}
#endif

#if 0
/* move to rda_nand_base.c */
int board_nand_init(struct nand_chip *chip) __attribute__ ((weak));

int board_nand_init(struct nand_chip *chip)
{
	return rda_nand_init(chip);
}
#endif
