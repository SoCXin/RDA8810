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
#include <asm/arch-rda/ispi.h>
#include <mtd/nand/rda_nand.h>

#include "tgt_ap_board_config.h"
#include "tgt_ap_clock_config.h"
#include "tgt_ap_flash_parts.h"

#ifdef CONFIG_SPL_BUILD
#undef CONFIG_NAND_RDA_DMA
#endif

#ifdef CONFIG_NAND_RDA_DMA
#include <asm/dma-mapping.h>
#include <asm/arch/dma.h>
#endif

/* FOR NAND TEST */
#ifdef TGT_AP_DO_NAND_TEST

unsigned int gbl_nand_test_id_val[2];
unsigned char g_nand_test_buf_write[4096] = {0};
unsigned char g_nand_test_buf_read[4096] = {0};
#endif /* TGT_AP_DO_NAND_TEST */

//#define NAND_DEBUG
//#define NAND_DEBUG_VERBOSE
#define NAND_CONTROLLER_BUFFER_LEN 9216

#define hal_gettime     get_ticks
#define SECOND          * CONFIG_SYS_HZ_CLOCK
#define NAND_TIMEOUT    ( 2 SECOND )
#define PLL_BUS_FREQ	(_TGT_AP_PLL_BUS_FREQ * 1000000)

#ifdef CONFIG_SPL_BUILD
/*sram space is not enough, so use sdram space as middle data buffer*/
static u8 *g_nand_flash_temp_buffer = (u8 *)CONFIG_SPL_NAND_MIDDLE_DATA_BUFFER;
#else
static u8 g_nand_flash_temp_buffer[32*1024];
#endif

#define ECCSIZE(n) ((n)*15/8)
static const u8 g_nand_ecc_size[] = {
	ECCSIZE(24),
	ECCSIZE(96),
	ECCSIZE(96),
	ECCSIZE(64),
	ECCSIZE(56),
	ECCSIZE(40),
	ECCSIZE(24),
	ECCSIZE(48),
	ECCSIZE(32),
	ECCSIZE(16),
	ECCSIZE(8),
	ECCSIZE(72),
	ECCSIZE(80),
	ECCSIZE(88),
};

struct flash_nand_parameter {
	unsigned int page_size;
	unsigned int ecc_msg_len;
	unsigned int spare_space_size;
	unsigned int oob_size;
	unsigned int ecc_bits;
	unsigned int spl_offset;
	unsigned int crc;
};

struct flash_nand_parameter nand_parameter;

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

static struct nand_ecclayout rda_nand_oob_2k = {
	.eccbytes = 0,
	.eccpos = {},
	.oobfree = {
		{.offset = 2,
		 .length = 30} }
};

static struct nand_ecclayout rda_nand_oob_4k = {
	.eccbytes = 0,
	.eccpos = {},
	.oobfree = {
		{.offset = 2,
		 .length = 30} }
};

static struct nand_ecclayout rda_nand_oob_8k = {
	.eccbytes = 0,
	.eccpos = {},
	.oobfree = {
		{.offset = 2,
		 .length = 30} }
};

static struct nand_ecclayout rda_nand_oob_16k = {
	.eccbytes = 0,
	.eccpos = {},
	.oobfree = {
		{.offset = 2,
		 .length = 30} }
};

extern void rda_dump_buf(char *data, size_t len);

static unsigned int checksum32( unsigned char* data, unsigned int data_size)
{
	unsigned int checksum = 0;
	unsigned int i;

	for(i = 0; i < data_size; i++)
		checksum = ((checksum << 31) | (checksum >> 1)) + (unsigned int)data[i];

	return checksum;
}

static void fill_nand_parameter(void)
{
	nand_parameter.page_size = NAND_PAGE_SIZE;
	nand_parameter.spare_space_size = NAND_SPARE_SIZE;
	nand_parameter.ecc_bits = NAND_ECCBITS;
	nand_parameter.ecc_msg_len = NAND_ECCMSGLEN;
	nand_parameter.oob_size = NAND_OOBSIZE;
	nand_parameter.spl_offset = NAND_PAGE_SIZE;
	nand_parameter.crc = checksum32((unsigned char *)&nand_parameter,
		sizeof(struct flash_nand_parameter) - 4);
}

static void hal_send_cmd_brick(unsigned char data, unsigned char io16,
			unsigned short num, unsigned char next_act)
{
	unsigned long cmd_reg;

	cmd_reg = NANDFC_BRICK_DATA(data)
		| NANDFC_BRICK_W_WIDTH(io16)
		| NANDFC_BRICK_R_WIDTH(io16)
		| NANDFC_BRICK_DATA_NUM(num)
		| NANDFC_BRICK_NEXT_ACT(next_act);
	__raw_writel(cmd_reg, NANDFC_REG_BRICK_FIFO_WRITE_POINTER);
#ifdef NAND_DEBUG
	printf("  hal_send_cmd_brick 0x%08lx\n", cmd_reg);
#endif
}

static void hal_start_cmd_data(void)
{
	unsigned long cmd_reg;

	cmd_reg = __raw_readl(NANDFC_REG_CONFIG_A);
	cmd_reg |= NANDFC_CMDFULL_STA(1);

	__raw_writel(cmd_reg, NANDFC_REG_CONFIG_A);
}

#ifdef __SCRAMBLE_ENABLE__
static void hal_set_scramble_func(BOOL enable)
{
	unsigned long cmd_reg;

	cmd_reg = __raw_readl(NANDFC_REG_CONFIG_A);
	cmd_reg &=  ~ NANDFC_SCRAMBLE_ENABLE(1);
	cmd_reg |= NANDFC_SCRAMBLE_ENABLE(enable);

	__raw_writel(cmd_reg, NANDFC_REG_CONFIG_A);
}
#endif

/*
static void hal_set_col_addr(unsigned int col_addr)
{
	__raw_writel(col_addr, NANDFC_REG_COL_ADDR);
}
*/

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

static u16 nand_cal_message_num_size(struct rda_nand_info *info,
					u16 * message_num, u16* msg_ecc_size)
{
	u16 num, mod, ecc_size, page_size, oob_size, message_len, ecc_mode;

	page_size	= info->vir_page_size;
	oob_size 	= info->oob_size;
	message_len	= info->message_len;
	ecc_mode 	= info->ecc_mode;
	
	num = page_size / message_len;
	mod = page_size % message_len;
	if (mod)
		num += 1;
	else
		mod = message_len;

	ecc_size =  g_nand_ecc_size[ecc_mode];
	if (ecc_size % 2)
		ecc_size = ecc_size + 1;

	if (ecc_size * num > (oob_size - info->vir_oob_size)){
		printf("Error:ecc used bytes has passed the oob size\n");
		//return 0;
	}

	*message_num = num;
	*msg_ecc_size = ecc_size;
	return mod;
}

static unsigned char get_nand_eccmode(unsigned short ecc_bits)
{
	unsigned char i;

	if(ecc_bits < 8 || ecc_bits > 96 || (ecc_bits % 8) != 0) {
		printf("wrong ecc_bits, nand controller not support! \n");
		return 0xff;
	}

	for(i = 0; i < ARRAY_SIZE(g_nand_ecc_size); i++) {
		if(ECCSIZE(ecc_bits) == g_nand_ecc_size[i]) {
			return i;
		}
	}

	return 0;
}

static uint8_t nand_eccmode_autoselect(struct rda_nand_info *info)
{
	u16 num, mod, ecc_size, ecc_bits, max_ecc_parity_num;

	ecc_bits = 96;
	max_ecc_parity_num = info->oob_size - info->vir_oob_size;

	num = info->vir_page_size / info->message_len;
	mod = info->vir_page_size % info->message_len;
	if (mod)
		num += 1;

	while(ecc_bits >= 8) {
		ecc_size = ECCSIZE(ecc_bits);
		if (ecc_size % 2)
			ecc_size = ecc_size + 1;

		if (ecc_size * num > max_ecc_parity_num){
			ecc_bits -= 8;
		} else
			break;
	}

	return get_nand_eccmode(ecc_bits);
}

static int init_nand_info(struct rda_nand_info *info)
{
	u16 message_num = 0, message_mod;
	u16 msg_ecc_size = 0, page_ecc_size;
	u16 bch_kk = 0, bch_nn = 0, bch_oob_kk = 0, bch_oob_nn = 0;

	info->hard_ecc_hec = 0;
	info->nand_use_type = 1;
	info->oob_size = NAND_SPARE_SIZE;
	info->vir_erase_size = NAND_BLOCK_SIZE;

	if (info->parameter_mode_select  == NAND_PARAMETER_BY_GPIO) {
		switch (info->type) {
		case NAND_TYPE_2K:
			info->vir_page_size = 2048;
			info->vir_page_shift = 11;
			break;
		case NAND_TYPE_4K:
			info->vir_page_size = 4096;
			info->vir_page_shift = 12;
			break;
		case NAND_TYPE_8K:
			info->vir_page_size = 8192;
			info->vir_page_shift = 13;
			break;
		case NAND_TYPE_16K:
			info->vir_page_size = 16384;
			info->vir_page_shift = 14;
			break;
		default:
			printf("invalid nand type %d\n", info->type);
			return -EINVAL;
		}

		info->max_eccmode_support = nand_eccmode_autoselect(info);
	} else {
		info->vir_page_size = nand_parameter.page_size;
		if (is_power_of_2(info->vir_page_size))
			info->vir_page_shift = ffs(info->vir_page_size) - 1;
		else
			info->vir_page_shift = 0;

		if (nand_parameter.spare_space_size) {
			info->ecc_mode = nand_eccmode_autoselect(info);
		} else {
			info->ecc_mode = get_nand_eccmode(nand_parameter.ecc_bits);
		}
	}
	info->ecc_mode_bak = info->ecc_mode;
	info->type_bak = info->type;
	info->cmd_flag = 0;
	info->dump_debug_flag = 0;
	message_mod = nand_cal_message_num_size(info, &message_num, &msg_ecc_size);
	page_ecc_size = msg_ecc_size * message_num;

	info->page_total_num = info->vir_page_size + info->vir_oob_size + page_ecc_size;
	info->flash_oob_off = info->vir_page_size + page_ecc_size - msg_ecc_size;

	bch_kk =  info->message_len;
	bch_nn = bch_kk + g_nand_ecc_size[info->ecc_mode];
	info->bch_data= NANDFC_BCH_KK_DATA(bch_kk) | NANDFC_BCH_NN_DATA(bch_nn);

	bch_oob_kk = message_mod + info->vir_oob_size;
	bch_oob_nn = bch_oob_kk + g_nand_ecc_size[info->ecc_mode];
	info->bch_oob = NANDFC_BCH_KK_OOB(bch_oob_kk) | NANDFC_BCH_NN_OOB(bch_oob_nn) ;

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

#ifndef CONFIG_RDA_FPGA
static unsigned long rda_nand_clk_div_tbl[] = {
	2, 3, 4, 5, 6, 7, 8, 9, 10,
	12, 14, 16, 18, 20, 22, 24,26, 28, 30, 32, 40
};
#endif

static unsigned long hal_calc_divider(struct rda_nand_info *info)
{
#ifdef CONFIG_RDA_FPGA
	return 7;
#else
	unsigned long mclk = info->master_clk;
	unsigned long div, clk_div;
	int i;
	int size = ARRAY_SIZE(rda_nand_clk_div_tbl);

	div = mclk / info->clk;
	if (mclk % info->clk)
		div += 1;

	for (i = 0; i < size; i++) {
		if(div <= rda_nand_clk_div_tbl[i]) {
			clk_div = i;
			break;
		}
	}

	if (i >= size) {
		clk_div = size - 1;
	}

	printf("NAND: max clk: %ld, bus clk: %ld\n", info->clk, mclk);
	printf("NAND: div: %ld, clk_div: %ld\n", div, clk_div);
	return clk_div;
#endif
}

static int hal_init(struct rda_nand_info *info)
{
	unsigned long config_a, config_b,page_para,message_oob;
	unsigned long clk_div = hal_calc_divider(info);
	uint16_t msg_package_num, msg_package_mod;

	__raw_writel((1 << 23), NANDFC_REG_CONFIG_A);
	__raw_writel(0, NANDFC_REG_CONFIG_A);

	/* setup config_a and config_b */
	config_a = NANDFC_CYCLE(clk_div);
	config_a |= NANDFC_CHIP_SEL(0x0e);
	config_a |= NANDFC_POLARITY_IO(0);	// set 0 invert IO
	config_a |= NANDFC_POLARITY_MEM(1);	// set 1 invert MEM
	config_a |= NANDFC_BRICK_COMMAND(1);

	/* enable the 16bit mode; */
	if (info->bus_width_16)
		config_a |= NANDFC_WDITH_16BIT(1);

	config_a |= (1 << 20);

	config_b = NANDFC_HWECC(1) | NANDFC_ECC_MODE(info->ecc_mode);

	msg_package_num = info->vir_page_size/info->message_len;
	msg_package_mod = info->vir_page_size%info->message_len;
	if(msg_package_mod)
		msg_package_num += 1;
	else
		msg_package_mod = info->vir_page_size;
       /*16k page nand need program twice one page*/
	/*package num need change when program if not time of 2*/
	if (info->type == NAND_TYPE_16K || info->type == NAND_TYPE_14K)
		msg_package_num >>= 1;

	message_oob = NANDFC_OOB_SIZE(info->vir_oob_size) |
		NANDFC_MESSAGE_SIZE(msg_package_num * info->message_len);
	/*page size is not used, but package num is used*/
	page_para = NANDFC_PAGE_SIZE(info->page_total_num) |
		NANDFC_PACKAGE_NUM(msg_package_num);

	__raw_writel(config_a, NANDFC_REG_CONFIG_A);
	__raw_writel(config_b, NANDFC_REG_CONFIG_B);
	__raw_writel(page_para, NANDFC_REG_PAGE_PARA);
	if (msg_package_num == 1)
		info->bch_data = info->bch_oob;
	__raw_writel(info->bch_data, NANDFC_REG_BCH_DATA);
	__raw_writel(info->bch_oob, NANDFC_REG_BCH_OOB);
	__raw_writel(message_oob, NANDFC_REG_MESSAGE_OOB_SIZE);
	__raw_writel(0x80008000, NANDFC_REG_IRBN_COUNT);
	__raw_writel(0x20202020, NANDFC_REG_BRICK_FSM_TIME0);
	__raw_writel(0x00006020, NANDFC_REG_BRICK_FSM_TIME1);
	__raw_writel(0x80000000, NANDFC_REG_BRICK_FSM_TIME2);
	printf("bch_data=0x%x, bch_oob=0x%x, page_total=0x%x, oob=0x%x \n",
		info->bch_data, info->bch_oob, info->page_total_num, info->vir_oob_size);
	printf("page_para=0x%08lx, message_oob=0x%08lx\n", page_para, message_oob);
	printf("NAND: nand init done, config=0x%08lx configb=0x%08lx\n", config_a, config_b);

	return 0;
}

static int nand_eccmode_setting(struct rda_nand_info *info,
	NAND_BCHMODE_TYPE eccmode)
{
	u16 message_num = 0, message_mod;
	u16 msg_ecc_size = 0, page_ecc_size;
	u16 bch_kk = 0, bch_nn = 0, bch_oob_kk = 0, bch_oob_nn = 0;
	unsigned long config_b,page_para,message_oob;

	info->ecc_mode = eccmode;

	message_mod = nand_cal_message_num_size(info, &message_num, &msg_ecc_size);
	page_ecc_size = msg_ecc_size * message_num;

	info->page_total_num = info->vir_page_size + info->vir_oob_size + page_ecc_size;
	info->flash_oob_off = info->vir_page_size + page_ecc_size - msg_ecc_size;

	bch_kk =  info->message_len;
	bch_nn = bch_kk + g_nand_ecc_size[eccmode];
	info->bch_data= NANDFC_BCH_KK_DATA(bch_kk) | NANDFC_BCH_NN_DATA(bch_nn);

	bch_oob_kk = message_mod + info->vir_oob_size;
	bch_oob_nn = bch_oob_kk + g_nand_ecc_size[eccmode];
	info->bch_oob = NANDFC_BCH_KK_OOB(bch_oob_kk) | NANDFC_BCH_NN_OOB(bch_oob_nn) ;

	config_b = NANDFC_HWECC(1) | NANDFC_ECC_MODE(info->ecc_mode);

       /*16k page nand need program twice one page*/
	/*package num need change when program if not time of 2*/
	if (info->type == NAND_TYPE_16K || info->type == NAND_TYPE_14K)
		message_num >>= 1;

	message_oob = NANDFC_OOB_SIZE(info->vir_oob_size) |
		NANDFC_MESSAGE_SIZE(info->message_len * message_num);
	/*page size is not used, but package num is used*/
	page_para = NANDFC_PAGE_SIZE(info->page_total_num) |
		NANDFC_PACKAGE_NUM(message_num);

	__raw_writel(config_b, NANDFC_REG_CONFIG_B);
	__raw_writel(page_para, NANDFC_REG_PAGE_PARA);
	if (message_num == 1)
		info->bch_data = info->bch_oob;
	__raw_writel(info->bch_data, NANDFC_REG_BCH_DATA);
	__raw_writel(info->bch_oob, NANDFC_REG_BCH_OOB);
	__raw_writel(message_oob, NANDFC_REG_MESSAGE_OOB_SIZE);

#ifdef NAND_DEBUG
	printf("nand_eccmode_setting:bch_data=0x%x, bch_oob=0x%x, page_total=0x%x, oob=0x%x\n",
		info->bch_data, info->bch_oob, info->page_total_num, info->vir_oob_size);
	printf("nand_eccmode_setting:page_para=0x%08lx, message_oob=0x%08lx\n",
		page_para, message_oob);
#endif
	printf("nand ecc setting done, configA=0x%x, configb=0x%08lx, type=%d\n",
		__raw_readl(NANDFC_REG_CONFIG_A), config_b, info->type);
	return 0;
}

static void nand_eccmode_convert(unsigned int page_addr, struct mtd_info *mtd)
{
	register struct nand_chip *chip = mtd->priv;
	struct rda_nand_info *info = chip->priv;
	u8 ecc_mode;

	if(page_addr < info->boot_logic_end_pageaddr) {
		ecc_mode = info->ecc_mode_bak;
	} else {
		ecc_mode = info->max_eccmode_support;
	}

	if (ecc_mode != info->ecc_mode)
		nand_eccmode_setting(info, ecc_mode);
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
	u8 * u_nand_temp_buffer = &g_nand_flash_temp_buffer[info->read_ptr];

#ifdef NAND_DEBUG
	printf("read : buf addr = 0x%x, len = %d, read_ptr = %d, u_nand_temp_buffer = 0x%x \n",
			(u32) buf, (u32)len,
	      (u32) info->read_ptr, (u32)u_nand_temp_buffer);
#endif /* NAND_DEBUG */

#ifndef CONFIG_NAND_RDA_DMA
	if (info->type == NAND_TYPE_16K || info->type == NAND_TYPE_14K) {
		memcpy((void *)buf, (void *)u_nand_temp_buffer, len);
	} else {
		if (info->page_addr == 0 && info->read_ptr == 0 &&
			info->parameter_mode_select == NAND_PARAMETER_BY_NAND) {
			/* specially do for nand when read 1KB of first page. */
			if (info->dump_debug_flag == 0)
				len = info->vir_page_size;/*no ecc bytes*/
			else
				len = 2048;/*ecc bytes needed*/
			memset(buf, 0xff, info->page_size);
		}
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

#ifdef NAND_DEBUG
	printf("write : buf addr = 0x%p, len = %d, write_ptr = %d\n", buf, len,
	       info->write_ptr);
#endif /* NAND_DEBUG */

#ifdef NAND_DEBUG_VERBOSE
	rda_dump_buf((char *)buf, len);
#endif

#ifndef CONFIG_NAND_RDA_DMA
	if (info->type == NAND_TYPE_16K || info->type == NAND_TYPE_14K)
		memcpy((void *)u_nand_temp_buffer, (void *)buf, len);
	else
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
	unsigned int page_addr = info->page_addr;

	__raw_writel(0xfff, NANDFC_REG_INT_STAT);
	switch (info->cmd) {
	case NAND_CMD_SEQIN:
		hal_flush_buf(mtd);
		info->write_ptr = 0;
		break;
	case NAND_CMD_READ0:
		hal_flush_buf(mtd);
		//info->read_ptr = 0;
		if(info->parameter_mode_select == NAND_PARAMETER_BY_NAND &&
			info->dump_debug_flag == 0) {
			if (page_addr < info->boot_logic_end_pageaddr) {
				page_addr += info->spl_logic_pageaddr;
				info->page_addr = page_addr;
			}
		}
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
		printf("post_cmd read status 0x3c = %x\n", info->byte_buf[0]);
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

static int nand_rda_dev_ready(struct mtd_info *mtd)
{
	return 1;
}

int nand_cmd_reset_flash(void)
{
	int ret = 0;

	hal_send_cmd_brick(NAND_CMD_RESET,0,0,5);
	hal_send_cmd_brick(0,0,0,0);
	hal_start_cmd_data();
	ret = hal_wait_cmd_complete();
	if (ret)
		printf("nand_reset_flash failed\n");

	return ret;
}

int nand_cmd_read_id(void)
{
	int ret = 0;

	hal_send_cmd_brick(NAND_CMD_READID,0,0,2);
	hal_send_cmd_brick(0,0,0,6);
	hal_send_cmd_brick(0,0,0,0xd);
	hal_send_cmd_brick(0,0,7,0);
	hal_start_cmd_data();
	ret = hal_wait_cmd_complete();
	if (ret)
		printf("nand_cmd_read_id failed\n");

	return ret;
}

void nand_cmd_write_page(struct rda_nand_info *info, unsigned int page_addr,
	u16 page_size, u16 col_addr)
{
	u8 addr_temp = 0;

	hal_send_cmd_brick(NAND_CMD_SEQIN,0,0,2);

	addr_temp = col_addr & 0xff;
	hal_send_cmd_brick(addr_temp,0,0,2);
	addr_temp = (col_addr >> 8) & 0xff;
	hal_send_cmd_brick(addr_temp,0,0,2);

	if (NAND_BLOCK_SIZE == 0x300000) {
		/*page 0x180 ~ 0x1ff are invalid, so need skip to next block.*/
		page_addr += 0x80 * (page_addr / 0x180);
		/*block 0xab0 ~ 0xfff are invalid, so need skip to next LUN.*/
		page_addr += (0x550 << 9) * ((page_addr >> 9) / 0xab0);
	}

	addr_temp = page_addr & 0xff;
	hal_send_cmd_brick(addr_temp,0,0,2);
	addr_temp = (page_addr >> 8) & 0xff;
	hal_send_cmd_brick(addr_temp,0,0,2);
	addr_temp = (page_addr >> 16) & 0xff;
	hal_send_cmd_brick(addr_temp,0,0,7);

	hal_send_cmd_brick(0,0,page_size,3);

	hal_send_cmd_brick(0,info->bus_width_16,page_size, 1);

	hal_send_cmd_brick(NAND_CMD_PAGEPROG,0,0,5);
	
	hal_send_cmd_brick(0,0,0,0);

	return;
}

void nand_cmd_read_page(struct rda_nand_info *info, unsigned int page_addr,
	unsigned short num, unsigned short col_addr)
{
	u8 addr_temp = 0;
	
	hal_send_cmd_brick(NAND_CMD_READ0,0,0,2);

	addr_temp = col_addr & 0xff;
	hal_send_cmd_brick(addr_temp,0,0,2);
	addr_temp = (col_addr >> 8)& 0xff;
	hal_send_cmd_brick(addr_temp,0,0,2);

	if (NAND_BLOCK_SIZE == 0x300000) {
		/*page 0x180 ~ 0x1ff are invalid, so need skip to next block.*/
		page_addr += 0x80 * (page_addr / 0x180);
		/*block 0xab0 ~ 0xfff are invalid, so need skip to next LUN.*/
		page_addr += (0x550 << 9) * ((page_addr >> 9) / 0xab0);
	}

	addr_temp = page_addr & 0xff;
	hal_send_cmd_brick(addr_temp,0,0,2);
	addr_temp = (page_addr >> 8) & 0xff;
	hal_send_cmd_brick(addr_temp,0,0,2);
	addr_temp = (page_addr >> 16) & 0xff;
	hal_send_cmd_brick(addr_temp,0,0,1);

	hal_send_cmd_brick(NAND_CMD_READSTART,0,0,5);
	hal_send_cmd_brick(0,0,0,0xa);
	hal_send_cmd_brick(0,0,0,0x4);

	hal_send_cmd_brick(0,info->bus_width_16,num, 0);

	return;
}

void nand_cmd_read_random(struct rda_nand_info *info, unsigned short num,
	unsigned short col_addr)
{
	u8 addr_temp = 0;
	
	hal_send_cmd_brick(NAND_CMD_RNDOUT,0,0,2);

	addr_temp = col_addr & 0xff;
	hal_send_cmd_brick(addr_temp,0,0,2);
	addr_temp = (col_addr >> 8)& 0xff;
	hal_send_cmd_brick(addr_temp,0,0,1);
	hal_send_cmd_brick(NAND_CMD_RNDOUTSTART,0,0,9);
	hal_send_cmd_brick(0,0,0,0x4);

	hal_send_cmd_brick(0,info->bus_width_16,num, 0);

	return;
}
void nand_cmd_block_erase(unsigned int page_addr)
{
	u8 addr_temp = 0;
	int ret = 0;
	
	hal_send_cmd_brick(NAND_CMD_ERASE1,0,0,2);

	if (NAND_BLOCK_SIZE == 0x300000) {
		/*page 0x180 ~ 0x1ff are invalid, so need skip to next block.*/
		page_addr += 0x80 * (page_addr / 0x180);
		/*block 0xab0 ~ 0xfff are invalid, so need skip to next LUN.*/
		page_addr += (0x550 << 9) * ((page_addr >> 9) / 0xab0);
	}

	addr_temp = page_addr & 0xff;
	hal_send_cmd_brick(addr_temp,0,0,2);
	addr_temp = (page_addr >> 8) & 0xff;
	hal_send_cmd_brick(addr_temp,0,0,2);
	addr_temp = (page_addr >> 16) & 0xff;
	hal_send_cmd_brick(addr_temp,0,0,1);

	hal_send_cmd_brick(NAND_CMD_ERASE2,0,0,5);
	hal_send_cmd_brick(0,0,0,0);

	hal_start_cmd_data();
	ret = hal_wait_cmd_complete();
	if (ret) {
		printf("nand_cmd_block_erase fail\n");
		return;
	}
}

void nand_cmd_read_status(void)
{
	int ret = 0;

	hal_send_cmd_brick(NAND_CMD_STATUS,0,0,6);
	hal_send_cmd_brick(0,0,0,0xd);
	hal_send_cmd_brick(0,0,1,0);

	hal_start_cmd_data();
	ret = hal_wait_cmd_complete();
	if (ret) {
		printf("nand_cmd_read_status failed\n");
		return;
	}
}

void nandfc_page_write_first(struct rda_nand_info *info, unsigned int page_addr,
	u16 page_size, u16 col_addr)
{
        u8 addr_temp = 0;

        hal_send_cmd_brick(NAND_CMD_SEQIN,0,0,2);

        addr_temp = col_addr & 0xff;
        hal_send_cmd_brick(addr_temp,0,0,2);
        addr_temp = (col_addr >> 8) & 0xff;
        hal_send_cmd_brick(addr_temp,0,0,2);

	if (NAND_BLOCK_SIZE == 0x300000) {
		/*page 0x180 ~ 0x1ff are invalid, so need skip to next block.*/
		page_addr += 0x80 * (page_addr / 0x180);
		/*block 0xab0 ~ 0xfff are invalid, so need skip to next LUN.*/
		page_addr += (0x550 << 9) * ((page_addr >> 9) / 0xab0);
	}

        addr_temp = page_addr & 0xff;
        hal_send_cmd_brick(addr_temp,0,0,2);
        addr_temp = (page_addr >> 8) & 0xff;
        hal_send_cmd_brick(addr_temp,0,0,2);
        addr_temp = (page_addr >> 16) & 0xff;
        hal_send_cmd_brick(addr_temp,0,0,7);

        hal_send_cmd_brick(0,0,0,3);

        hal_send_cmd_brick(0,info->bus_width_16,page_size, 0);

        return;
}

void nandfc_page_write_random(struct rda_nand_info *info, u16 col_addr,
	int data_num, char cmd_2nd)
{
        u8 addr_temp = 0;

        hal_send_cmd_brick(NAND_CMD_RNDIN,0,0,2);

        addr_temp = col_addr & 0xff;
        hal_send_cmd_brick(addr_temp,0,0,2);
        addr_temp = (col_addr >> 8) & 0xff;
        hal_send_cmd_brick(addr_temp,0,0,9);

        hal_send_cmd_brick(0,0,data_num, 3);
        hal_send_cmd_brick(0,info->bus_width_16,data_num, 1);

        hal_send_cmd_brick(cmd_2nd,0,0,5);

        hal_send_cmd_brick(0,0,0,0);
}

int nand_page_write_all(unsigned int page_addr, struct mtd_info *mtd)
{
	int ret = 0;
	u16 page_size = 0;
	u16 col_addr = 0;
	u32 message_oob = 0, page_para;
	register struct nand_chip *chip = mtd->priv;
	struct rda_nand_info *info = chip->priv;
	u8 *nand_ptr = (u8 *) chip->IO_ADDR_W;
	u8 *u_nand_temp_buffer = &g_nand_flash_temp_buffer[0];
	u16 message_num = 0, message_mod = 0;
	u16 first_wr_ecc_len = 0, first_wr_data_len = 0;

	if(info->parameter_mode_select == NAND_PARAMETER_BY_GPIO)
		nand_eccmode_convert(page_addr, mtd);

	if (info->type == NAND_TYPE_16K || info->type == NAND_TYPE_14K) {
		message_num = info->vir_page_size / info->message_len;
		message_mod = info->vir_page_size % info->message_len;
		if(message_mod)
			message_num += 1;
		else
			message_mod = info->message_len;

		first_wr_data_len = (message_num >> 1) * info->message_len;

		message_oob = NANDFC_OOB_SIZE(0) |
			NANDFC_MESSAGE_SIZE(first_wr_data_len);

		__raw_writel(info->bch_data, NANDFC_REG_BCH_OOB);
		__raw_writel(message_oob, NANDFC_REG_MESSAGE_OOB_SIZE);

		first_wr_ecc_len = (info->page_total_num - info->vir_page_size - \
			info->vir_oob_size) / message_num * (message_num >> 1);
		page_size = first_wr_data_len + first_wr_ecc_len;
		col_addr = page_size;

		if (info->bus_width_16)
			page_size = page_size - 2;
		else
			page_size = page_size - 1;

		page_para = NANDFC_PAGE_SIZE(page_size) |
			NANDFC_PACKAGE_NUM(message_num >> 1);
		__raw_writel(page_para, NANDFC_REG_PAGE_PARA);

		hal_flush_buf(mtd);
		memcpy((void *)nand_ptr, (void *)u_nand_temp_buffer, first_wr_data_len);
	}else{
		if (info->bus_width_16)
			page_size = info->page_total_num - 2;
		else
			page_size = info->page_total_num - 1;
	}

	if (info->type == NAND_TYPE_16K || info->type == NAND_TYPE_14K)
		nandfc_page_write_first(info, page_addr, page_size, 0);
	else
		nand_cmd_write_page(info, page_addr, page_size, 0);
	hal_start_cmd_data();
	ret = hal_wait_cmd_complete();
	if (ret) {
		if (info->type == NAND_TYPE_16K || info->type == NAND_TYPE_14K)
			printf ("nandfc_page_write 16k first half page failed: page_addr = 0x%x\n",
					page_addr);
		else
			printf ("nandfc_page_write failed:  page_addr = 0x%x\n", page_addr);
		return ret;
	}

	if (info->type == NAND_TYPE_16K || info->type == NAND_TYPE_14K) {
		message_oob = NANDFC_OOB_SIZE(info->vir_oob_size) |
			NANDFC_MESSAGE_SIZE(info->vir_page_size - first_wr_data_len);

		__raw_writel(info->bch_oob, NANDFC_REG_BCH_OOB);
		__raw_writel(message_oob, NANDFC_REG_MESSAGE_OOB_SIZE);
		page_size = info->page_total_num - col_addr;

		if (info->bus_width_16){
			page_size = page_size - 2;
			col_addr = col_addr >> 1;
		} else
			page_size = page_size - 1;

		page_para = NANDFC_PAGE_SIZE(page_size) |
			NANDFC_PACKAGE_NUM(message_num - (message_num >> 1));
		__raw_writel(page_para, NANDFC_REG_PAGE_PARA);

		hal_flush_buf(mtd);
		memcpy((void *)nand_ptr, (void *)(u_nand_temp_buffer + first_wr_data_len),
			info->vir_page_size + info->vir_oob_size - first_wr_data_len);

		//nand_cmd_write_page(page_addr, page_size, col_addr);
		nandfc_page_write_random(info, col_addr, page_size, NAND_CMD_PAGEPROG);
		hal_start_cmd_data();
		ret = hal_wait_cmd_complete();
		if (ret) {
			printf ("nandfc_page_write 16k second half page failed: page_addr = 0x%x\n",
					page_addr);
			return ret;
		}
	}

	return ret;
}

int nand_page_read_all(unsigned int page_addr, struct mtd_info *mtd)
{
	int ret = 0;
	unsigned short num = 0,col_addr = 0;
	u32 message_oob = 0, page_para;
	register struct nand_chip *chip = mtd->priv;
	struct rda_nand_info *info = chip->priv;
	u8 *nand_ptr = (u8 *) chip->IO_ADDR_R;
	u8 *u_nand_temp_buffer = &g_nand_flash_temp_buffer[0];
	u16 message_num = 0, message_mod = 0;
	u16 first_rd_ecc_len = 0, first_rd_data_len = 0;

	if (info->parameter_mode_select == NAND_PARAMETER_BY_GPIO)
		nand_eccmode_convert(page_addr, mtd);

	if (info->type == NAND_TYPE_16K || info->type == NAND_TYPE_14K){
		message_num = info->vir_page_size / info->message_len;
		message_mod = info->vir_page_size % info->message_len;
		if(message_mod)
			message_num += 1;
		else
			message_mod = info->message_len;

		first_rd_data_len =  info->message_len * (message_num >> 1);
		message_oob = NANDFC_OOB_SIZE(0) | NANDFC_MESSAGE_SIZE(first_rd_data_len);

		__raw_writel(info->bch_data, NANDFC_REG_BCH_OOB);
		__raw_writel(message_oob, NANDFC_REG_MESSAGE_OOB_SIZE);

		first_rd_ecc_len = (info->page_total_num - info->vir_page_size - \
			info->vir_oob_size) / message_num * (message_num >> 1);
		num = first_rd_data_len+ first_rd_ecc_len;

		page_para = NANDFC_PAGE_SIZE(num) |
			NANDFC_PACKAGE_NUM(message_num >> 1);
		__raw_writel(page_para, NANDFC_REG_PAGE_PARA);
	}else
		num = info->page_total_num;

	nand_cmd_read_page(info, page_addr, num, col_addr);
	hal_start_cmd_data();
	ret = hal_wait_cmd_complete();
	if (ret) {
		if (info->type != NAND_TYPE_16K && info->type != NAND_TYPE_14K) {
			printf ("nandfc_page_read failed : page_addr = 0x%x\n", page_addr);
			return ret;
		} else
			printf ("nandfc_page_read 16k first half page failed: page_addr = 0x%x\n",
					page_addr);
	}

	if (info->type == NAND_TYPE_16K || info->type == NAND_TYPE_14K)
		memcpy((void *)u_nand_temp_buffer, (void *)nand_ptr, first_rd_data_len);

	if (info->type == NAND_TYPE_16K || info->type == NAND_TYPE_14K) {
		message_oob = NANDFC_OOB_SIZE(info->vir_oob_size) |
			NANDFC_MESSAGE_SIZE(info->vir_page_size - first_rd_data_len);

		__raw_writel(info->bch_oob, NANDFC_REG_BCH_OOB);
		__raw_writel(message_oob, NANDFC_REG_MESSAGE_OOB_SIZE);

		col_addr = num;
		num = info->page_total_num - col_addr;

		page_para = NANDFC_PAGE_SIZE(num) |
			NANDFC_PACKAGE_NUM(message_num - (message_num >> 1));
		__raw_writel(page_para, NANDFC_REG_PAGE_PARA);

		nand_cmd_read_random(info, num, col_addr);
		hal_start_cmd_data();
		ret = hal_wait_cmd_complete();
		memcpy((void *)(u_nand_temp_buffer + first_rd_data_len), (void *)nand_ptr,
			(info->vir_page_size + info->vir_oob_size - first_rd_data_len));
		if (ret) {
			printf ("nandfc_page_read 16k second half page failed: page_addr = 0x%x\n",
					page_addr);
			return ret;
		}
	}

	return ret;
}

static void nand_parameter_info_wr(unsigned int page_addr, struct mtd_info *mtd)
{
	register struct nand_chip *chip = mtd->priv;
	struct rda_nand_info *info = chip->priv;
	u8 *nand_ptr = (u8 *) (chip->IO_ADDR_W);
	u8 ecc_mode;

	if(page_addr < info->boot_logic_end_pageaddr && info->cmd_flag == 0) {
		ecc_mode = NAND_ECC_96BIT_WITHOUT_CRC;
		info->vir_page_size = 1024;
		info->vir_page_shift = 10;
		info->vir_oob_size = 32;
		info->message_len = 1024;
		info->type = NAND_TYPE_2K;
		info->cmd_flag = 1;
		nand_eccmode_setting(info, ecc_mode);

		memset(nand_ptr, 0xff, 2048);
		memcpy(nand_ptr, (unsigned char*)&nand_parameter,
			sizeof(struct flash_nand_parameter));
		nand_page_write_all(0, mtd);

		ecc_mode = info->ecc_mode_bak;
		info->vir_page_size = nand_parameter.page_size;
		if (is_power_of_2(info->vir_page_size))
			info->vir_page_shift = ffs(info->vir_page_size) - 1;
		else
			info->vir_page_shift = 0;
		info->vir_oob_size = nand_parameter.oob_size;
		info->message_len = nand_parameter.ecc_msg_len;
		info->type = info->type_bak;
		nand_eccmode_setting(info, ecc_mode);
	}
}

static void nand_parameter_info_rd(unsigned int page_addr, struct mtd_info *mtd)
{
	register struct nand_chip *chip = mtd->priv;
	struct rda_nand_info *info = chip->priv;
	u8 ecc_mode;

	if(page_addr < info->spl_logic_pageaddr) {
		ecc_mode = NAND_ECC_96BIT_WITHOUT_CRC;
		if (ecc_mode != info->ecc_mode) {
			info->vir_page_size = 1024;
			info->vir_page_shift = 10;
			info->vir_oob_size = 32;
			info->message_len = 1024;
			info->type = NAND_TYPE_2K;
		}
	} else {
		ecc_mode = info->ecc_mode_bak;
		if (ecc_mode != info->ecc_mode) {
			info->vir_page_size = nand_parameter.page_size;
			if (is_power_of_2(info->vir_page_size))
				info->vir_page_shift = ffs(info->vir_page_size) - 1;
			else
				info->vir_page_shift = 0;
			info->vir_oob_size = nand_parameter.oob_size;
			info->message_len = nand_parameter.ecc_msg_len;
			info->type = info->type_bak;
		}
	}

	if (ecc_mode != info->ecc_mode) {
		nand_eccmode_setting(info, ecc_mode);
	}
}

static void nand_send_cmd_all(struct mtd_info *mtd)
{
	register struct nand_chip *chip = mtd->priv;
	struct rda_nand_info *info = chip->priv;
	unsigned int page_addr = info->page_addr;

	switch(info->cmd){
		case NAND_CMD_READ0:
			if(info->parameter_mode_select == NAND_PARAMETER_BY_NAND)
				nand_parameter_info_rd(page_addr, mtd);
			nand_page_read_all(page_addr, mtd);
			break;
		case NAND_CMD_RESET:
			nand_cmd_reset_flash();
			break;
		case NAND_CMD_READID:
			nand_cmd_read_id();
			break;
		case NAND_CMD_ERASE1:
			nand_cmd_block_erase(page_addr);
			if (info->parameter_mode_select == NAND_PARAMETER_BY_NAND) {
				if (page_addr < info->boot_logic_end_pageaddr) {
					info->cmd_flag = 0;
					//nand_cmd_block_erase(0);
				}
			}
			break;
		case NAND_CMD_PAGEPROG:
		case NAND_CMD_SEQIN:
			nand_page_write_all(page_addr, mtd);
			if(info->parameter_mode_select == NAND_PARAMETER_BY_NAND)
				nand_parameter_info_wr(page_addr, mtd);
			break;
		case NAND_CMD_STATUS:
			nand_cmd_read_status();
			break;
		default:
			return;
	}

	//hal_start_cmd_data();
}

static void nand_rda_cmdfunc(struct mtd_info *mtd, unsigned int command,
			     int column, int page_addr)
{
	register struct nand_chip *chip = mtd->priv;
	struct rda_nand_info *info = chip->priv;

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
		if(info->parameter_mode_select == NAND_PARAMETER_BY_NAND) {
			if (page_addr < info->boot_logic_end_pageaddr) {
				page_addr += info->spl_logic_pageaddr;
				info->page_addr = page_addr;
			}
			nand_parameter_info_rd(page_addr, mtd);
		}
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

#ifdef NAND_DEBUG
	printf("after cmd remap, cmd = %x, page_addr = %x, col_addr = %x\n",
	       info->cmd, info->page_addr, info->col_addr);
#endif
/*
	if (info->col_addr != -1) {
		hal_set_col_addr(info->col_addr);
	}
*/
	if (info->page_addr == -1)
		info->page_addr = 0;

	nand_rda_do_cmd_pre(mtd);

	nand_send_cmd_all(mtd);

	nand_rda_do_cmd_post(mtd);

	nand_wait_ready(mtd);
}

static int nand_reset_flash(void)
{
	int ret = 0;
#ifdef NAND_DEBUG
	printf("Enter function nand_rda_init\n");
#endif
	nand_cmd_reset_flash();

	return ret;
}

static int nand_read_id(unsigned int id[2])
{
	int ret = 0;

	nand_cmd_read_id();

	id[0] = __raw_readl(NANDFC_REG_IDCODE_A);
	id[1] = __raw_readl(NANDFC_REG_IDCODE_B);

	printf("NAND: Nand ID: %08x %08x\n", id[0], id[1]);

#ifdef TGT_AP_DO_NAND_TEST
	gbl_nand_test_id_val[0] = id[0];
	gbl_nand_test_id_val[1] = id[1];
#endif /* TGT_AP_DO_NAND_TEST */
	return ret;
}

static void read_ID(struct mtd_info *mtd)
{
	unsigned int id[2];

	nand_read_id(id);
}

static void nand_rda_select_chip(struct mtd_info *mtd, int chip)
{
	unsigned long config_a = __raw_readl(NANDFC_REG_CONFIG_A);

	if ((chip == -1) || (chip > 3))
		return;

	config_a |= NANDFC_CHIP_SEL(0x0f);
	config_a &= ~ NANDFC_CHIP_SEL(1 << chip);

	//printf("**********config_a = 0x%lx", config_a);
	__raw_writel(config_a, NANDFC_REG_CONFIG_A);
}

static int nand_get_type(struct rda_nand_info *info, unsigned int id[2])
{
	u16 hwcfg_nand_type;
	int metal_id;

	info->spl_adjust_ratio = 2;
	metal_id = rda_metal_id_get();

#if defined(CONFIG_MACH_RDA8850E)
#define RDA_NAND_TYPE_BIT_BUS  (1 << 1)    /* 1: 8-bit; 0: 16-bit */
/*bit7 bit0 00 menas 2k, 01 means 4k, 10 means 8k, 11 means 16k*/
#define RDA_NAND_TYPE_BIT_7       (1 << 7)
#define RDA_NAND_TYPE_BIT_0       (1 << 0)
	printf("NAND: RDA8850E chip, metal %d\n", metal_id);
	hwcfg_nand_type = rda_hwcfg_get() & 0x83;
	if (hwcfg_nand_type & RDA_NAND_TYPE_BIT_BUS)
		info->bus_width_16 = 0;
	else
		info->bus_width_16 = 1;

	info->vir_oob_size = NAND_OOBSIZE;
	info->parameter_mode_select = NAND_PARAMETER_BY_GPIO;

	u16 nand_type = ((hwcfg_nand_type & RDA_NAND_TYPE_BIT_7) >> 6)
			| (hwcfg_nand_type & RDA_NAND_TYPE_BIT_0);
	if(nand_type == 0) {
		info->type = NAND_TYPE_2K;
		info->parameter_mode_select = NAND_PARAMETER_BY_NAND;
	}
	else if(nand_type == 1)
		info->type = NAND_TYPE_4K;
	else
		info->type = nand_type;

	if (info->parameter_mode_select  == NAND_PARAMETER_BY_GPIO) {
		switch(info->type){
		case NAND_TYPE_2K:
			info->page_size = 2048;
			info->page_shift = 11;
			info->ecc_mode = NAND_ECC_16BIT;
			info->message_len = 2048;
			break;
		case NAND_TYPE_4K:
			info->page_size = 4096;
			info->page_shift = 12;
			info->ecc_mode = NAND_ECC_24BIT;
			info->message_len = 1024;
			break;
		case NAND_TYPE_8K:
			info->page_size = 8192;
			info->page_shift = 13;
			info->ecc_mode = NAND_ECC_24BIT;
			info->message_len = 1024;
			break;
		case NAND_TYPE_16K:
			info->page_size = 16384;
			info->page_shift = 14;
			info->ecc_mode = NAND_ECC_32BIT;
			info->message_len = 1024;
			break;
		default:
			printf("Error nand type \n");
			break;
		}
	}else {
		fill_nand_parameter();
		info->page_size = nand_parameter.page_size;
		if (is_power_of_2(info->page_size))
			info->page_shift = ffs(info->page_size) - 1;
		else
			info->page_shift = 0;
		info->message_len = nand_parameter.ecc_msg_len;
		info->vir_oob_size = nand_parameter.oob_size;
		switch(info->page_size) {
		case 2048:
			info->type = NAND_TYPE_2K;
			break;
		case 4096:
			info->type = NAND_TYPE_4K;
			break;
		case 8192:
			info->type = NAND_TYPE_8K;
			break;
		case 16384:
			info->type = NAND_TYPE_16K;
			break;
		case 3072:
			info->type = NAND_TYPE_3K;
			break;
		case 7168:
			info->type = NAND_TYPE_7K;
			break;
		case 14336:
			info->type = NAND_TYPE_14K;
			break;
		default:
			printf("Error nand type \n");
			break;
		}
	}
#else
#if defined(CONFIG_MACH_RDA8810H)
	printf("NAND: RDA8810H chip, metal %d\n", metal_id);
	hwcfg_nand_type = rda_hwcfg_get();

	if (RDA_HW_CFG_GET_BM_IDX(hwcfg_nand_type) == RDA_MODE_NAND_8BIT)
		info->bus_width_16 = 0;
	else
	if (RDA_HW_CFG_GET_BM_IDX(hwcfg_nand_type) == RDA_MODE_NAND_16BIT)
		info->bus_width_16 = 1;
	else
		printf("nand: invalid nand type \n");

	info->parameter_mode_select = NAND_PARAMETER_BY_NAND;

	fill_nand_parameter();
	info->page_size = nand_parameter.page_size;
	if (is_power_of_2(info->page_size))
		info->page_shift = ffs(info->page_size) - 1;
	else
		info->page_shift = 0;
	info->message_len = nand_parameter.ecc_msg_len;
	info->vir_oob_size = nand_parameter.oob_size;
	switch(info->page_size) {
	case 2048:
		info->type = NAND_TYPE_2K;
		break;
	case 4096:
		info->type = NAND_TYPE_4K;
		break;
	case 8192:
		info->type = NAND_TYPE_8K;
		break;
	case 16384:
		info->type = NAND_TYPE_16K;
		break;
	case 3072:
		info->type = NAND_TYPE_3K;
		break;
	case 7168:
		info->type = NAND_TYPE_7K;
		break;
	case 14336:
		info->type = NAND_TYPE_14K;
		break;
	default:
		printf("Error nand type \n");
		break;
	}
#else
#error "WRONG MACH"
#endif
#endif

	printf("NAND: actually ratio * 2, SPL adjust ratio is %d. \n",
		info->spl_adjust_ratio);
	return 0;
}

static int nand_rda_init(struct nand_chip *this, struct rda_nand_info *info)
{
	unsigned int id[2];
	int ret = 0;

#ifdef NAND_DEBUG
	printf("Enter function nand_rda_init\n");
#endif

	ret = nand_get_type(info, id);
	if (ret)
		return ret;

	ret = init_nand_info(info);
	if (ret)
		return ret;
	if (info->bus_width_16)
		this->options |= NAND_BUSWIDTH_16;

	(void)hal_init(info);

	ret = nand_reset_flash();
	if (ret)
		return ret;

	ret = nand_read_id(id);
	if (ret)
		return ret;
	
	return ret;
}

static int nand_rda_init_size(struct mtd_info *mtd, struct nand_chip *this,
			u8 *id_data)
{
	struct rda_nand_info *info = this->priv;

	/* TODO: this is not always right
	 * assuming nand is 2k/4k SLC for now
	 * for other types of nand, 64 pages per block is not always true
	 */
	mtd->erasesize = info->vir_erase_size;
	mtd->writesize = info->vir_page_size;
	mtd->oobsize = info->vir_oob_size;

	return (info->bus_width_16) ? NAND_BUSWIDTH_16 : 0;
}

int rda_nand_init(struct nand_chip *nand)
{
	struct rda_nand_info *info;
	static struct rda_nand_info rda_nand_info;
	int i;
	u32 bootloader_end_addr = 0x400000;//4Mbytes

	info = &rda_nand_info;
#if 0
	if (g_nand_flash_temp_buffer)
		g_nand_flash_temp_buffer = (u8 *)malloc(18432);
#endif
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
	nand->dev_ready = nand_rda_dev_ready;
	nand->select_chip = nand_rda_select_chip;
	nand->read_nand_ID = read_ID;
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

	if (info->parameter_mode_select == NAND_PARAMETER_BY_NAND) {
		switch (info->type) {
		case NAND_TYPE_2K:
			bootloader_end_addr = 0x200000;//2Mbytes
			break;
		case NAND_TYPE_4K:
			bootloader_end_addr = 0x400000;//4Mbytes
			break;
		case NAND_TYPE_8K:
			if (is_power_of_2(info->vir_erase_size))
				bootloader_end_addr = 0x400000;//4Mbytes
			else
				bootloader_end_addr = 0x300000;//3Mbytes
			break;
		case NAND_TYPE_16K:
			bootloader_end_addr = 0x400000;//4Mbytes
			break;
		case NAND_TYPE_3K:
			bootloader_end_addr = 0x300000;//3Mbytes
			break;
		case NAND_TYPE_7K:
			bootloader_end_addr = 0x700000;//7Mbytes
			break;
		case NAND_TYPE_14K:
			bootloader_end_addr = 0x700000;//7Mbytes
			break;
		default:
			printf("error nand type: %d \n", info->type);
		}

		bootloader_end_addr -= nand_parameter.spl_offset;

		if (info->page_shift) {
			info->boot_logic_end_pageaddr = bootloader_end_addr >> info->page_shift;
			info->spl_logic_pageaddr = nand_parameter.spl_offset >> info->page_shift;
		} else {
			info->boot_logic_end_pageaddr = bootloader_end_addr / info->page_size;
			info->spl_logic_pageaddr = nand_parameter.spl_offset / info->page_size;
		}
	}

	if (!nand->ecc.layout && (nand->ecc.mode != NAND_ECC_SOFT_BCH)) {
		switch (info->type) {
		case NAND_TYPE_2K:
			nand->ecc.layout = &rda_nand_oob_2k;
			break;
		case NAND_TYPE_4K:
			nand->ecc.layout = &rda_nand_oob_4k;
			break;
		case NAND_TYPE_8K:
			nand->ecc.layout = &rda_nand_oob_8k;
			break;
		case NAND_TYPE_16K:
			nand->ecc.layout = &rda_nand_oob_16k;
			break;
		case NAND_TYPE_3K:
			nand->ecc.layout = &rda_nand_oob_4k;
			break;
		case NAND_TYPE_7K:
			nand->ecc.layout = &rda_nand_oob_8k;
			break;
		case NAND_TYPE_14K:
			nand->ecc.layout = &rda_nand_oob_16k;
			break;
		default:
			printf("error oob size: %d \n", info->vir_oob_size);
		}
		nand->ecc.layout->oobfree[0].length = info->vir_oob_size - 2;
		for(i=1; i<MTD_MAX_OOBFREE_ENTRIES; i++)
			nand->ecc.layout->oobfree[i].length = 0;
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

#ifdef TGT_AP_DO_NAND_TEST

#define NAND_TEST_DATA_BASE	0x11c01080
#define NAND_RESULT_DATA_ADDR	(NAND_TEST_DATA_BASE+0x00)
#define NAND_TYPE_DATA_ADDR 	(NAND_TEST_DATA_BASE+0x04)
#define NAND_PAGE_SIZE_ADDR     (NAND_TEST_DATA_BASE+0x08)
#define NAND_VID_B1_ADDR	(NAND_TEST_DATA_BASE+0x0C)
#define NAND_VID_B2_ADDR	(NAND_TEST_DATA_BASE+0x10)
#define NAND_NAME_ADDR		(NAND_TEST_DATA_BASE+0x14)
#define NAND_NAME_SIZE		0x08
#define NAND_BUS_WIDTH_ADDR	(NAND_TEST_DATA_BASE+0x1C)

#define NAND_TEST_OK_VAL	0xA55A6666
#define NAND_TEST_ERROR_VAL	0xDEADDEAD

static void print_nand_info(int dev_idx)
{
	nand_info_t *nand = &nand_info[dev_idx];
	struct nand_chip *chip = nand->priv;
	struct rda_nand_info *info = chip->priv;
	unsigned int id[2],type_val = 0,bus_width = 8;
	int man_id,j,k;
	char *pname ;

	id[0] = gbl_nand_test_id_val[0];
	id[1] = gbl_nand_test_id_val[1];
	man_id = id[0] & 0xFF;
	pname = NULL;
	for(j=0; nand_manuf_ids[j].id != 0x0;j++) {
		if(nand_manuf_ids[j].id  == man_id) {
			pname = nand_manuf_ids[j].name;
			break;
		}
	}
	printf("nand device info:\n");
	if(pname != NULL)
		printf("Nand name: %s\n",pname);
	switch(info->type) {
	case NAND_TYPE_4K: printf("Nand type: 4K\n"); type_val = 4;break;
	case NAND_TYPE_2K: printf("Nand type: 2K\n"); type_val = 2;break;
	case NAND_TYPE_8K: printf("Nand type: 8K\n"); type_val = 8;break;
	case NAND_TYPE_16K: printf("Nand type: 16K\n"); type_val = 16;break;
	default: printf("Nand type error\n"); type_val = 0;break;
	}
	//intf("Nand type: 0x%x\n",info->type);
	//printf("Nand_used_type: 0x%x\n",info->nand_use_type);
	printf("Nand page size: 0x%x\n",info->page_size);
	printf("Nand ID: %08x %08x\n", id[0], id[1]);
	printf("Nand page total num: 0x%x\n",info->page_total_num);

	switch(info->bus_width_16) {
	case 0:printf("Nand bus width: 8 bit\n"); bus_width = 8;break;
	case 1:printf("Nand bus width: 16 bit\n"); bus_width = 16;break;
	default:printf("Nand bus width error\n"); bus_width = 0;break;
	}
	*(unsigned int *)NAND_TYPE_DATA_ADDR = type_val;
	*(unsigned int *)NAND_PAGE_SIZE_ADDR = info->page_size;
	*(unsigned int *)NAND_VID_B1_ADDR = id[0];
	*(unsigned int *)NAND_VID_B2_ADDR = id[1];
	if(pname != NULL) {
		for(k = 0;k<8;k++) {
			*(unsigned char *)(NAND_NAME_ADDR + k) = '\0';
		}
		for(k = 0;k<8;k++) {
			*(unsigned char *)(NAND_NAME_ADDR + k) = *(pname+k);
		}
	}
	*(unsigned int *)NAND_BUS_WIDTH_ADDR = bus_width;
}

void test_nand_easily(void)
{
	nand_info_t *nand;
	unsigned int i = 0,j = 0;
	unsigned long addr =  0x1000000;
	size_t len = 0;
	int ret = 0;
	nand_erase_options_t opts;

	j = j;
	ret = ret;
	nand = &nand_info[0];

	/* check test result */
	if(*(unsigned int *)NAND_RESULT_DATA_ADDR == NAND_TEST_ERROR_VAL) {
		printf("nand error!!!\n");
		return;
	}

	printf("\n%s: current ticks = %llu\n",__func__,get_ticks());
	printf("-------------Begin to test internal nand flash memory.............. \n");
	len = nand->writesize;
	for (i = 0; i < len; i++){
		g_nand_test_buf_write[i] = i & 0xff;
		g_nand_test_buf_read[i] =0;
	}

	/* set nand erasing parameters */
	memset(&opts,0,sizeof(opts));
	opts.offset = addr;
	opts.length = len;
	opts.jffs2 = 0;
	opts.quiet = 0;
	opts.spread = 0;

	/* step0: read nand flash information data */
	print_nand_info(0);
	/* step1: erase flash */
	printf("erase flash memory,addr = 0x%x,len = %d\n",(int)addr,(int)len);
	//nand_erase(nand,addr,len);
	ret = nand_erase_opts(nand, &opts);
	if(ret != 0) {
		printf("erase flash error, returned value: ret = %d\n",ret);
		goto L_ERROR_END;
	}
	/* step2: write test data to flash */
	printf("write test data to flash memory, addr = 0x%x, len = %d\n",(int)addr, (int)len);
	/*
	nand_write_skip_bad(nand,
			addr,
			&len,
			g_nand_test_buf_write,
			0);
	*/
	nand_write(nand, addr, &len,g_nand_test_buf_write);
	printf("real writing test data len = %d\n",(int)len);
	/* step3: read test data back */
	printf("read test data from flash memory,addr = 0x%x, len = %d\n",(int)addr,(int)len);
	//nand_read_skip_bad(nand,addr,&len,g_nand_test_buf_read);
	nand_read(nand, addr,&len,g_nand_test_buf_read);
	printf("real reading test data len = %d\n",(int)len);
	#if 0
	printf("display rx data\n");
	for(i = 0; i < len; i++) {
		printf("%x\t",g_nand_test_buf_read[i]);
		j++;
		if(j > 15) {
		j = 0;
		printf("\n");
		}
	}
	#endif

	/* step4: check test data for testing result */
	printf("compare data\n");
	for (i = 0; i < len; i++) {
		if (g_nand_test_buf_write[i] != g_nand_test_buf_read[i]){
			printf("nand operate error!!\n");
			*(unsigned int *)NAND_RESULT_DATA_ADDR = NAND_TEST_ERROR_VAL;
			return;
		}
	}
//L_OK_END:

	*(unsigned int *)NAND_RESULT_DATA_ADDR = NAND_TEST_OK_VAL;
	printf("Write result data: addr = 0x%x,value = 0x%x\n",(int)NAND_RESULT_DATA_ADDR, NAND_TEST_OK_VAL);
	printf("-------------Test internal nand flash memory OK.............. \n");
	printf("\n%s: current ticks = %llu\n",__func__,get_ticks());
	return;
L_ERROR_END:
	printf("-------------Test internal nand flash memory error.............. \n");
	printf("\n%s: current ticks = %llu\n",__func__,get_ticks());
	return;
}
#endif /* TGT_AP_DO_NAND_TEST */
