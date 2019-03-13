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

#include "tgt_ap_board_config.h"
#include "tgt_ap_clock_config.h"

#ifdef CONFIG_SPL_BUILD
#undef CONFIG_NAND_RDA_DMA
#endif

#ifdef CONFIG_NAND_RDA_DMA
#include <asm/dma-mapping.h>
#include <asm/arch/dma.h>
#endif

//#define NAND_DEBUG
//#define NAND_DEBUG_VERBOSE

#define NAND_NEED_OOB_OFF_FIX
#ifdef NAND_NEED_OOB_OFF_FIX
static uint8_t oob_ff_pattern[] = { 0xff, 0xff }; 
#define NAND_4K_86_OOB_OFF      0x10B8
#define NAND_8K_86_OOB_OFF      0x2170
#define NAND_8K_83_OOB_OFF      0x23C0
#endif

#define hal_gettime     get_ticks
#define SECOND          * CONFIG_SYS_HZ_CLOCK
#define NAND_TIMEOUT    ( 2 SECOND )
#define PLL_BUS_FREQ	(_TGT_AP_PLL_BUS_FREQ * 1000000)

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

static struct nand_ecclayout rda_nand_oob_40 = {
	.eccbytes = 0,
	.eccpos = {},
	.oobfree = {
		{.offset = 2,
		 .length = 38} }
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
	/*
	 * Use NANDFC_INT_STAT_IDLE instead of NANDFC_INT_DONE
	 * this is HW bug, NANDFC_INT_DONE comes too early
	 * however, NANDFC_INT_STAT_IDLE is not an interrupt source
	 * need HW fix anyway
	 */
	do {
		int_stat = __raw_readl(NANDFC_REG_INT_STAT);
		if (hal_gettime() - start_time >= wait_time) {
			timeout = 1;
		}
#ifdef CONFIG_MACH_RDA8810E
	} while (!(int_stat & NANDFC_INT_STAT_IDLE) && !timeout);
#else
	} while (!(int_stat & NANDFC_INT_DONE) && !timeout);
#endif

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
			int nand_type, int nand_ecc, int bus_width_16)
{
	info->type = nand_type;
	info->ecc_mode = nand_ecc;
	info->bus_width_16 = bus_width_16;
	switch (nand_type) {
	case NAND_TYPE_2K:
		info->page_size = 2048;
		info->page_shift = 11;
		info->oob_size = 64;
		break;
	case NAND_TYPE_4K:
		info->page_size = 4096;
		info->page_shift = 12;
		info->oob_size = 224;
		info->vir_oob_size = 40;
		break;
	case NAND_TYPE_8K:
		info->page_size = 8192;
		info->page_shift = 13;
		info->oob_size = 744;
		break;
	default:
		printf("invalid nand type %d\n", nand_type);
		return -EINVAL;
	}
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

	if (div < 5) {
		/* 5 is minimal divider by V2 hardware */
		div = 5;
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

	printf("NAND: max clk: %ld, bus clk: %ld\n", info->clk, mclk);
	printf("NAND: div: %ld, clk_div: %ld\n", div, clk_div);
	return clk_div;
}

static int hal_init(struct rda_nand_info *info)
{
	unsigned long config_a, config_b;
#ifdef CONFIG_RDA_FPGA
	unsigned long clk_div = 4;
#else
	unsigned long clk_div = hal_calc_divider(info);
#endif

	/* setup config_a and config_b */
	config_a = NANDFC_CYCLE(clk_div);
	config_a |= NANDFC_CHIP_SEL(0x0e);
	config_a |= NANDFC_POLARITY_IO(0);	// set 0 invert IO
	config_a |= NANDFC_POLARITY_MEM(1);	// set 1 invert MEM

	switch (info->type) {
	case NAND_TYPE_2K:
		config_a |= NANDFC_TIMING(0x8B);
		break;
	case NAND_TYPE_4K:
		config_a |= NANDFC_TIMING(0x8C);
		break;
	case NAND_TYPE_8K:
		config_a |= NANDFC_TIMING(0x8D);
		break;
	default:
		printf("invalid nand type %d\n", info->type);
		return -EINVAL;
	}

	/* enable the 16bit mode; */
	if (info->bus_width_16)
		config_a |= NANDFC_WDITH_16BIT(1);

	config_b = NANDFC_HWECC(1) | NANDFC_ECC_MODE(info->ecc_mode);

	__raw_writel(config_a, NANDFC_REG_CONFIG_A);
	__raw_writel(config_b, NANDFC_REG_CONFIG_B);

#if 1
	/* Set an interval of filter for erasing operation. */
	unsigned long delay;
	delay = __raw_readl(NANDFC_REG_DELAY);
	delay |= 0x10;
	__raw_writel(delay, NANDFC_REG_DELAY);
#endif /* #if 0 */

	printf("NAND: nand init done, %08lx %08lx\n", config_a, config_b);

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

static void nand_rda_read_buf(struct mtd_info *mtd, uint8_t * buf, int len)
{
	struct nand_chip *chip = mtd->priv;
	struct rda_nand_info *info = chip->priv;
	u8 *nand_ptr = (u8 *) (chip->IO_ADDR_R + info->read_ptr);

#ifdef NAND_DEBUG
	printf("read : buf addr = 0x%p, len = 0x%x, read_ptr = 0x%x \n", buf, len,
	       info->read_ptr);
#endif /* NAND_DEBUG */

#ifndef CONFIG_NAND_RDA_DMA
	memcpy((void *)buf, (void *)nand_ptr, len);
	if(info->ecc_mode == NAND_ECC_1K24BIT)
		info->read_ptr += 184;
	info->read_ptr += len;

#ifdef NAND_DEBUG_VERBOSE
	rda_dump_buf((char *)buf, 2);
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

#ifdef NAND_DEBUG
	printf("write : buf addr = 0x%p, len = %d, write_ptr = %d\n", buf, len,
	       info->write_ptr);
#endif /* NAND_DEBUG */

#ifdef NAND_DEBUG_VERBOSE
	rda_dump_buf((char *)buf, len);
#endif

#ifdef NAND_NEED_OOB_OFF_FIX
	u8 *nand_oob_ptr;
	switch (info->type) {
	case NAND_TYPE_2K:
		break;
	case NAND_TYPE_4K:
		if(info->write_ptr > 0 && info->ecc_mode == NAND_ECC_1K24BIT){
			nand_ptr = (u8 *)(chip->IO_ADDR_W + NAND_4K_86_OOB_OFF);
			len = 40;
		}
		break;
	case NAND_TYPE_8K:
		if (info->ecc_mode == NAND_ECC_1K24BIT)
			nand_oob_ptr = (u8 *)(chip->IO_ADDR_W + 
				NAND_8K_86_OOB_OFF);
		else
			nand_oob_ptr = (u8 *)(chip->IO_ADDR_W + 
				NAND_8K_83_OOB_OFF);
		memcpy((void *)nand_oob_ptr, (void *)oob_ff_pattern, 2);
		break;
	default:
		break;
	}
#endif

#ifndef CONFIG_NAND_RDA_DMA
	memcpy((void *)nand_ptr, (void *)buf, len);
	info->write_ptr += len;

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

static void nand_rda_do_cmd_pre(struct mtd_info *mtd)
{
	struct nand_chip *this = mtd->priv;
	struct rda_nand_info *info = this->priv;

	__raw_writel(0xfff, NANDFC_REG_INT_STAT);
	switch (info->cmd) {
	case NAND_CMD_SEQIN:
		hal_flush_buf(mtd);
		info->write_ptr = 0;
		break;
	case NAND_CMD_READ0:
		hal_flush_buf(mtd);
		//info->read_ptr = 0;
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
		break;
#if 0
/* read back after write, for err checking */
	case NAND_CMD_SEQIN:
		{
			int cmd_ret;
			/* read back instantly and check fcs/crc result */
			hal_send_cmd((unsigned char)NAND_CMD_READ0,
				     info->page_addr);
			cmd_ret = hal_wait_cmd_complete();
			if (cmd_ret) {
				printf
				    ("readback cmd fail, page_addr = %x, ret = %x\n",
				     info->page_addr, cmd_ret);
			}
		}
		break;
#endif
#if 0
/* delay 10ms, for erase to complete */
	case NAND_CMD_ERASE1:
		udelay(10000);
		break;
#endif
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
#ifdef NAND_NEED_OOB_OFF_FIX
		switch (info->type) {
		case NAND_TYPE_2K:
			break;
		case NAND_TYPE_4K:
			//info->col_addr = NAND_4K_86_OOB_OFF;
			info->read_ptr = NAND_4K_86_OOB_OFF;
			break;
		case NAND_TYPE_8K:
			if (info->ecc_mode == NAND_ECC_1K24BIT) {
				info->col_addr = NAND_8K_86_OOB_OFF;
				info->read_ptr = NAND_8K_86_OOB_OFF;
			}
			else {
				info->col_addr = NAND_8K_83_OOB_OFF;
				info->read_ptr = NAND_8K_83_OOB_OFF;
			}
			break;
		default:
			break;
		}
#endif
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
		printf("nand_reset_flash failed\n");
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

	printf("NAND: Nand ID: %08x %08x\n", id[0], id[1]);

	return ret;
}

static void read_ID(struct mtd_info *mtd)
{
	unsigned int id[2];

	nand_read_id(id);
}

static int nand_get_type(struct rda_nand_info *info, unsigned int id[2],
		int *rda_nand_type, int *rda_nand_ecc, int *bus_width_16)
{
	u16 hwcfg_nand_type;
	int metal_id;

	info->spl_adjust_ratio = 2;
	metal_id = rda_metal_id_get();

#if defined(CONFIG_MACH_RDA8810) || defined(CONFIG_MACH_RDA8850)
#error "RDA8810/RDA8850 does not match RDA NAND Controller V2"
#elif defined(CONFIG_MACH_RDA8810E) || defined(CONFIG_MACH_RDA8820)
	printf("NAND: RDA8810E/RDA8820 get type, metal %d\n", metal_id);
	if (1) {
#define RDA_NAND_TYPE_BIT_7       (1 << 7)    /* 1: 8K NAND, 0 - 2K/4K NAND */
#define RDA_NAND_TYPE_BIT_BUS     (1 << 1)    /* 1: 8-bit; 0: 16-bit */
#define RDA_NAND_TYPE_BIT_0       (1 << 0)    /* if BIT_7 == 1 (8K NAND)  : 1 - 1K64bit ECC, 0 - 1K24bit ECC */
                                              /* if BIT_7 == 0 (4K/2K)    : 1 - 4K(1K24bit), 0 - 2K(2K24bit) */
		hwcfg_nand_type = rda_hwcfg_get() & 0x83;
		if (hwcfg_nand_type & RDA_NAND_TYPE_BIT_BUS)
			*bus_width_16 = 0;
		else
			*bus_width_16 = 1;
		if (hwcfg_nand_type & RDA_NAND_TYPE_BIT_7) {
			/* 8K */
			*rda_nand_type = NAND_TYPE_8K;
			if (hwcfg_nand_type & RDA_NAND_TYPE_BIT_0) {
				printf("NAND: 8K page, 1K64BIT ECC, %s bus\n",
					(*bus_width_16)?"16bit":"8bit");
				*rda_nand_ecc = NAND_ECC_1K64BIT;
			} else {
				printf("NAND: 8K page, 1K24BIT ECC, %s bus\n",
					(*bus_width_16)?"16bit":"8bit");
				*rda_nand_ecc = NAND_ECC_1K24BIT;
			}
		} else {
			/* 4K/2K */
			if (hwcfg_nand_type & RDA_NAND_TYPE_BIT_0) {
				printf("NAND: 4K page, 1K24BIT ECC, %s bus\n",
					(*bus_width_16)?"16bit":"8bit");
				*rda_nand_type = NAND_TYPE_4K;
				*rda_nand_ecc = NAND_ECC_1K24BIT;
			} else {
				printf("NAND: 2K page, 2K24BIT ECC, %s bus\n",
					(*bus_width_16)?"16bit":"8bit");
				*rda_nand_type = NAND_TYPE_2K;
				*rda_nand_ecc = NAND_ECC_2K24BIT;
			}
		}
	}
#else
#error "unknown MACH"
#endif

	printf("NAND: actually ratio * 2, SPL adjust ratio is %d. \n",
		info->spl_adjust_ratio);
	return 0;
}

static int nand_rda_init(struct nand_chip *this, struct rda_nand_info *info)
{
	unsigned int id[2];
	int ret = 0;
	int rda_nand_type = NAND_TYPE_INVALID;
	int rda_nand_ecc = NAND_ECC_INVALID;
	int bus_width_16 = 0;

	ret = nand_reset_flash();
	if (ret)
		return ret;
	ret = nand_read_id(id);
	if (ret)
		return ret;
	ret = nand_get_type(info, id, &rda_nand_type, &rda_nand_ecc, &bus_width_16);
	if (ret)
		return ret;
	ret = init_nand_info(info, rda_nand_type, rda_nand_ecc, bus_width_16);
	if (ret)
		return ret;
	if (info->bus_width_16)
		this->options |= NAND_BUSWIDTH_16;

	return hal_init(info);
}

#if defined(CONFIG_SPL_BUILD)
/* this is for u-boot-spl only */
static int nand_rda_init_size(struct mtd_info *mtd, struct nand_chip *this,
			u8 *id_data)
{
	struct rda_nand_info *info = this->priv;

	/* TODO: this is not always right
	 * assuming nand is 2k/4k SLC for now
	 * for other types of nand, 64 pages per block is not always true
	 */
	mtd->erasesize = info->page_size * 64;
	mtd->writesize = info->page_size;
	mtd->oobsize = (info->oob_size > 256)?256:info->oob_size;

	return (info->bus_width_16) ? NAND_BUSWIDTH_16 : 0;
}
#endif

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
#if defined(CONFIG_SPL_BUILD)
	nand->init_size = nand_rda_init_size;
#else
	nand->init_size = NULL;
#endif
	nand->cmdfunc = nand_rda_cmdfunc;

	nand->read_nand_ID = read_ID;
	nand->read_byte = nand_rda_read_byte;
	nand->read_word = nand_rda_read_word;
	nand->read_buf = nand_rda_read_buf;
	nand->write_buf = nand_rda_write_buf;

	nand->dev_ready = nand_rda_dev_ready;

	nand->priv = (void *)info;
	nand->IO_ADDR_R = nand->IO_ADDR_W = (void __iomem *)NANDFC_DATA_BUF;
#ifdef CONFIG_NAND_RDA_DMA
	info->nand_data_phys = (void __iomem *)NANDFC_DATA_BUF;
#endif /* CONFIG_NAND_RDA_DMA */
	info->index = 0;
	info->write_ptr = 0;
	info->read_ptr = 0;
	info->master_clk = get_master_clk_rate(_TGT_AP_CLK_APB2);
	info->clk=_TGT_AP_NAND_CLOCK;

#ifdef CONFIG_NAND_RDA_DMA
	rda_request_dma(&info->dma_ch);
#endif /* CONFIG_NAND_RDA_DMA */

	 nand_rda_init(nand, info);

	if (!nand->ecc.layout && (nand->ecc.mode != NAND_ECC_SOFT_BCH)) {
		switch (info->vir_oob_size) {
		case 40:
			nand->ecc.layout = &rda_nand_oob_40;
			break;
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

#if 0
/* move to rda_nand_base.c */
int board_nand_init(struct nand_chip *chip) __attribute__ ((weak));

int board_nand_init(struct nand_chip *chip)
{
	return rda_nand_init(chip);
}
#endif
