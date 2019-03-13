#include <linux/module.h>
#include <linux/init.h>
#include <linux/ioport.h>
#include <linux/platform_device.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/dma-mapping.h>
#include <linux/clk.h>
#include <linux/err.h>
#include <linux/workqueue.h>
#include <linux/timer.h>
#include <linux/semaphore.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/nand.h>
#include <linux/mtd/partitions.h>
#include <linux/io.h>
#include <linux/slab.h>
#include <asm/sizes.h>
#include <asm/memory.h>

#include <mach/hardware.h>
#include <mach/irqs.h>
#include <mach/ifc.h>
#include <mach/timex.h>
#include <mach/rda_clk_name.h>
#include <plat/devices.h>
#include <plat/reg_ifc.h>
#include <plat/reg_nand.h>
#include <plat/rda_debug.h>

#ifdef CONFIG_MTD_NAND_RDA_DMA
#include <plat/dma.h>
#endif /* CONFIG_MTD_NAND_RDA_DMA */

#define NAND_CMD_TIMEOUT_MS    ( 2000 )
#define NAND_DMA_TIMEOUT_MS    ( 100 )
//#define NAND_IRQ_POLL
#define PROCNAME "driver/nand"
#define NAND_STATISTIC
/*
 * Default mtd partitions, used when mtdparts= not present in cmdline
 */
static const struct mtd_partition partition_info[] = {
	{
	 .name = "reserved",
	 .offset = 0,.size = 24 * SZ_1M},
	{
	 .name = "customer",
	 .offset = 24 * SZ_1M,.size = SZ_8M},
	{
	 .name = "system",
	 .offset = SZ_32M,.size = 192 * SZ_1M},
	{
	 .name = "cache",
	 .offset = 224 * SZ_1M,.size = SZ_8M},
	{
	 .name = "userdata",
	 .offset = 232 * SZ_1M,.size = 280 * SZ_1M},
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

struct rda_nand_info {
	struct nand_chip nand_chip;
	struct mtd_info mtd;

	struct rda_nand_device_data *plat_data;
	struct device *dev;
	int type;
	int ecc_mode;
	int bus_width_16;
	int vir_page_size;
	int oob_size;
	int message_len;
	int page_total_num;
	int bch_data;
	int bch_oob;
	int flash_oob_off;

	int cmd;
	int col_addr;
	int page_addr;
	int read_ptr;
	u32 byte_buf[4];	// for read_id, status, etc
	int ecc_bits;
	int index;
	int write_ptr;
	u16 first_data_len_16k;
	u16 first_ecc_len_16k;

	void __iomem *base;
	void __iomem *reg_base;
	void __iomem *data_base;
#ifdef CONFIG_MTD_NAND_RDA_DMA
	dma_addr_t phy_addr;
	/* For waiting for dma */
	struct completion comp;
	u8 dma_ch;
#endif
	u8 *databuf;
	u32 nand_status;
#ifndef NAND_IRQ_POLL
	struct completion nand_comp;
#endif /* NAND_IRQ_POLL */

#ifdef NAND_STATISTIC
	u64 bytes_read;
	u64 bytes_write;
	u64 ecc_flips;
	u64 read_fails;
	u64 write_fails;
#endif
	struct clk *master_clk;
	unsigned long clk;

	u8 read_retry_cmd_bak;
	u8 read_retry_time;
	u8 read_retry_error;
	u8 nand_controller_init_divider;
	u32 nand_id[2];
};

struct read_retry_cmd {
	u8 addr;
	u8 data;
};

const static u8 g_read_retry_cmd_enter_19nm_sequence[] = {0x5c, 0xc5};
const static u8 g_read_retry_cmd_startops_19nm_sequence[] = {0x26, 0x5d};
const static struct read_retry_cmd g_read_retry_cmd_19nm_sequence[][5] = {
	{{0x4, 0x04}, {0x5, 0x04}, {0x6, 0x7c}, {0x7, 0x7e}, {0xd, 0x00}},/*first*/
	{{0x4, 0x00}, {0x5, 0x7c}, {0x6, 0x78}, {0x7, 0x78}, {0xd, 0x00}},/*second*/
	{{0x4, 0x7c}, {0x5, 0x76}, {0x6, 0x74}, {0x7, 0x72}, {0xd, 0x00}},/*third*/
	{{0x4, 0x08}, {0x5, 0x08}, {0x6, 0x00}, {0x7, 0x00}, {0xd, 0x00}},/*fourth*/
	{{0x4, 0x0b}, {0x5, 0x7e}, {0x6, 0x76}, {0x7, 0x74}, {0xd, 0x00}},/*fifth*/
	{{0x4, 0x10}, {0x5, 0x76}, {0x6, 0x72}, {0x7, 0x70}, {0xd, 0x00}},/*sixth*/
	{{0x4, 0x02}, {0x5, 0x7c}, {0x6, 0x7e}, {0x7, 0x70}, {0xd, 0x00}},/*seventh*/
	{{0x4, 0x14}, {0x5, 0x74}, {0x6, 0x00}, {0x7, 0x6e}, {0xd, 0x00}},/*eighth*/
	{{0x4, 0x05}, {0x5, 0x78}, {0x6, 0x76}, {0x7, 0x74}, {0xd, 0x00}},/*ninth*/
	{{0x4, 0x0a}, {0x5, 0x0b}, {0x6, 0x04}, {0x7, 0x04}, {0xd, 0x00}},/*tenth*/
	{{0x4, 0x00}, {0x5, 0x00}, {0x6, 0x00}, {0x7, 0x00}, {0xd, 0x00}},/*exit*/
};
const static u8 g_read_retry_cmd_special_19nm_sequence[] = {0xb3};

const static u8 g_read_retry_cmd_startops_15nm_sequence[] = {0x26};
const static u8 g_read_retry_cmd_15nm_sequence[][4] = {
	{0x00,0x00,0x00,0x00},/*first*/
	{0x02,0x04,0x02,0x00},/*second*/
	{0x7c,0x00,0x7c,0x7c},/*third*/
	{0x7a,0x7c,0x7a,0x7c},/*fourth*/
	{0x78,0x00,0x78,0x7a},/*fifth*/
	{0x7e,0x02,0x7e,0x7a},/*sixth*/
	{0x76,0x04,0x76,0x02},/*seventh*/
	{0x04,0x00,0x04,0x78},/*eighth*/
	{0x06,0x00,0x06,0x76},/*ninth*/
	{0x74,0x7c,0x74,0x76},/*tenth*/
	{0x00,0x00,0x00,0x00}/*exit*/
};


static const u8 g_nand_ecc_bits[] = {
	24,
	96,
	96,
	64,
	56,
	40,
	24,
	48,
	32,
	16,
	8,
	72,
	80,
	88,
};

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

static void hal_send_cmd_brick(struct rda_nand_info *info,unsigned char data, unsigned char io16,
	unsigned short num, unsigned char next_act)
{
	unsigned int cmd_reg;

	cmd_reg = NANDFC_BRICK_DATA(data)
		| NANDFC_BRICK_R_WIDTH(io16)
		| NANDFC_BRICK_W_WIDTH(io16)
		| NANDFC_BRICK_DATA_NUM(num)
		| NANDFC_BRICK_NEXT_ACT(next_act);
	__raw_writel(cmd_reg, info->reg_base + NANDFC_REG_BRICK_FIFO_WRITE_POINTER);
}

static inline void hal_enable_dma(struct rda_nand_info *info)
{

	unsigned int cmd_reg;

	cmd_reg = __raw_readl(info->reg_base + NANDFC_REG_CONFIG_A);
	cmd_reg |= NANDFC_DMA_ENABLE(1);

	__raw_writel(cmd_reg,  info->reg_base + NANDFC_REG_CONFIG_A);
}

static inline void hal_disable_dma(struct rda_nand_info *info)
{

	unsigned int cmd_reg;

	cmd_reg = __raw_readl(info->reg_base + NANDFC_REG_CONFIG_A);
	cmd_reg &= ~ NANDFC_DMA_ENABLE(1);

	__raw_writel(cmd_reg,  info->reg_base + NANDFC_REG_CONFIG_A);
}

static void hal_start_cmd_data(struct rda_nand_info *info)
{
	unsigned int cmd_reg;

	cmd_reg = __raw_readl(info->reg_base + NANDFC_REG_CONFIG_A);
	cmd_reg |= NANDFC_CMDFULL_STA(1);

	__raw_writel(cmd_reg,  info->reg_base + NANDFC_REG_CONFIG_A);
}

#ifdef __SCRAMBLE_ENABLE__
static void hal_set_scramble_func(struct rda_nand_info *info, bool enable)
{
	unsigned int cmd_reg;

	cmd_reg = __raw_readl(info->reg_base + NANDFC_REG_CONFIG_A);
	cmd_reg &=  ~ NANDFC_SCRAMBLE_ENABLE(1);
	cmd_reg |= NANDFC_SCRAMBLE_ENABLE(enable);
	__raw_writel(cmd_reg,  info->reg_base + NANDFC_REG_CONFIG_A);
}
#endif

/*
static void hal_set_col_addr(struct rda_nand_info *info, unsigned int col_addr)
{
	__raw_writel(col_addr, info->reg_base + NANDFC_REG_COL_ADDR);
}
*/

static void hal_flush_buf(struct rda_nand_info *info)
{
	/*
	 there is no reg NANDFC_REG_BUF_CTRL
	 */
	__raw_writel(0x7, info->reg_base + NANDFC_REG_BUF_CTRL);
}

static void hal_read_normal_mode_timing_set(struct rda_nand_info *info)
{
	if (info->type == NAND_TYPE_16K || info->type == NAND_TYPE_14K) {
		__raw_writel(0x20202020, info->reg_base + NANDFC_REG_BRICK_FSM_TIME0);
		__raw_writel(0x00008020, info->reg_base + NANDFC_REG_BRICK_FSM_TIME1);
		__raw_writel(0xffffffff, info->reg_base + NANDFC_REG_IRBN_COUNT);
	} else {
		__raw_writel(0x06140405, info->reg_base + NANDFC_REG_BRICK_FSM_TIME0);
		__raw_writel(0x0000600f, info->reg_base + NANDFC_REG_BRICK_FSM_TIME1);
		__raw_writel(0x00200020, info->reg_base + NANDFC_REG_IRBN_COUNT);
	}
}

static void hal_read_retry_mode_timing_set(struct rda_nand_info *info)
{
	__raw_writel(0x01010101, info->reg_base + NANDFC_REG_BRICK_FSM_TIME0);
	__raw_writel(0x00000101, info->reg_base + NANDFC_REG_BRICK_FSM_TIME1);
	__raw_writel(0x00010001, info->reg_base + NANDFC_REG_IRBN_COUNT);
}

#ifdef NAND_IRQ_POLL
static u32 hal_wait_cmd_complete(struct rda_nand_info *info)
{
	unsigned int int_stat;
	unsigned long timeout;

	timeout = jiffies + msecs_to_jiffies(NAND_CMD_TIMEOUT_MS);
	do {
		int_stat = __raw_readl(info->reg_base + NANDFC_REG_INT_STAT);
	} while (!(int_stat & NANDFC_INT_DONE) && time_before(jiffies, timeout));

	/* to clear */
	__raw_writel((int_stat & NANDFC_INT_CLR_MASK),
			info->reg_base + NANDFC_REG_INT_STAT);

	if (time_after(jiffies, timeout)) {
		dev_err(info->dev, "cmd timeout\n");
		return -ETIME;
	}

	if (int_stat & NANDFC_INT_ERR_ALL) {
		dev_err(info->dev, "int error, int_stat = %x\n", int_stat);
		return (int_stat & NANDFC_INT_ERR_ALL);
	}

	return 0;
}
#else
static void hal_irq_enable(struct rda_nand_info *info)
{
	__raw_writel(0x12, info->reg_base + NANDFC_REG_INT_MASK);
	return;
}

static void hal_irq_disable(struct rda_nand_info *info)
{
	__raw_writel(0x0, info->reg_base + NANDFC_REG_INT_MASK);
	return;
}

static void hal_irq_clear(struct rda_nand_info *info, u32 int_status)
{
	__raw_writel((int_status & NANDFC_INT_CLR_MASK),
			info->reg_base + NANDFC_REG_INT_STAT);
	return;
}

static u32 hal_irq_get_status(struct rda_nand_info *info)
{
	return __raw_readl(info->reg_base + NANDFC_REG_INT_STAT);
}
#endif /* NAND_IRQ_POLL */

static u32 hal_get_flipbits(struct rda_nand_info *info)
{
	u32 raw;
	raw = __raw_readl(info->reg_base + NANDFC_REG_INT_STAT);
	return (raw & 0x7F000000) >> 24;
}

static u16 nand_cal_message_num_size(struct rda_nand_info *info,
	u8 * message_num, u16* ecc_total_size)
{
	u16 num, mod, ecc_size, page_size, msg_len, ecc_mode;

	page_size = info->vir_page_size;
	msg_len	= info->message_len;
	ecc_mode = info->ecc_mode;

	num = page_size / msg_len;
	mod = page_size % msg_len;
	if(mod)
		num += 1;
	else
		mod = msg_len;

	ecc_size =  g_nand_ecc_size[ecc_mode];
	if (ecc_size % 2)
		ecc_size = ecc_size + 1;

	*message_num = num;
	*ecc_total_size = ecc_size * num;

	return mod;
}

static int nand_get_ecc_strength(struct rda_nand_info *info)
{
	u8 message_num;
	u16 ecc_total_size;
	u16 temp;

	temp = nand_cal_message_num_size(info, &message_num, &ecc_total_size);
	if (info->type == NAND_TYPE_16K || info->type == NAND_TYPE_14K) {
		/*
		** Just the last 8K buff ECC is returned to upper layer
		** so lower the strength for 16K page size is given
		*/
		return message_num * g_nand_ecc_bits[info->ecc_mode] * 10 /20;
	}
	return message_num * g_nand_ecc_bits[info->ecc_mode];
}

static int nand_get_ecc_bytes(struct rda_nand_info *info)
{
	u8 message_num;
	u16 ecc_total_size;
	u16 temp;

	temp = nand_cal_message_num_size(info, &message_num, &ecc_total_size);
	return ecc_total_size;
}

static unsigned char get_nand_eccmode(unsigned short ecc_bits)
{
	unsigned char i;

	if(ecc_bits < 8 || ecc_bits > 96 || (ecc_bits % 8) != 0) {
		printk("wrong ecc_bits, nand controller not support! \n");
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
	max_ecc_parity_num = NAND_SPARE_SIZE - info->oob_size;

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

#ifndef CONFIG_RDA_FPGA
static const unsigned long rda_nand_clk_div_tbl[] = {
	2, 3, 4, 5, 6, 7, 8, 9, 10,
	12, 14, 16, 18, 20, 22, 24, 26, 28, 30, 32, 40
};

/*set nand controller divider to 40 for read retry mode.*/
static void hal_set_controller_divider(struct rda_nand_info *info, u8 value)
{
	unsigned int cmd_reg;

	cmd_reg = __raw_readl(info->reg_base + NANDFC_REG_CONFIG_A);
	cmd_reg &= ~ NANDFC_CYCLE(0x1f);
	cmd_reg |= NANDFC_CYCLE(value);
	__raw_writel(cmd_reg,  info->reg_base + NANDFC_REG_CONFIG_A);
}

static unsigned long hal_calc_divider(struct rda_nand_info *info)
{
	unsigned long mclk = clk_get_rate(info->master_clk);
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
		dev_warn(info->dev, "clk %d is too low, use max_div = 40\n",
			(int)info->clk);
		clk_div = size - 1;
	}

	dev_info(info->dev, "set clk %d, bus_clk = %d, divider = %d\n",
		(int)info->clk, (int)mclk, (int)clk_div);

	return clk_div;
}
#endif

static int nand_rda_map_type(struct rda_nand_info *info)
{
	u8 msg_num = 0;
	u16 ecc_total_size = 0, msg_mod;
	u16 bch_kk = 0, bch_nn = 0, bch_oob_kk = 0, bch_oob_nn = 0;

	rda_dbg_nand(
		"nand_rda_map_type, page_size = %d,"
		" chip_size = %lld, options = %x\n",
		info->mtd.writesize, info->nand_chip.chipsize,
		info->nand_chip.options);

	info->message_len = NAND_ECCMSGLEN;
	if (info->message_len != 1024 && info->message_len != 2048 &&
		info->message_len != 4096 && info->message_len != 1152)
		dev_err(info->dev, "nand ecc message len config error, "
			"should be 1024 or 2048 or 4096 or 1152, but is %d, "
			"need reconfig in file target/config.mk.\n",
			info->message_len);

	if (info->mtd.erasesize != NAND_BLOCK_SIZE)
		dev_err(info->dev, "nand block size config error, is 0x%x, "
			"conflict with 0x%x, please recheck it with flash datasheet!\n",
			NAND_BLOCK_SIZE, info->mtd.erasesize);

	info->vir_page_size = info->mtd.writesize;
	if (info->vir_page_size != NAND_PAGE_SIZE)
		dev_err(info->dev, "nand page size len config error, is 0x%x, "
			"conflict with 0x%x, please recheck it with flash datasheet!\n",
			NAND_PAGE_SIZE, info->vir_page_size);

	switch (info->mtd.writesize) {
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
		dev_err(info->dev, "invalid pagesize %d\n",
				info->mtd.writesize);
		info->type = NAND_TYPE_INVALID;
		return -EINVAL;
	}

	if (info->nand_chip.options & NAND_BUSWIDTH_16)
		info->bus_width_16 = 1;
	else
		info->bus_width_16 = 0;

	if (NAND_SPARE_SIZE) {
		info->ecc_mode = nand_eccmode_autoselect(info);
	} else {
		info->ecc_mode = get_nand_eccmode(NAND_ECCBITS);
	}

	msg_mod = nand_cal_message_num_size(info, &msg_num, &ecc_total_size);
	info->page_total_num = info->vir_page_size + info->oob_size + ecc_total_size;
	info->flash_oob_off = info->page_total_num - ecc_total_size/msg_num - info->oob_size;

	bch_kk = info->message_len;
	bch_nn = bch_kk + g_nand_ecc_size[info->ecc_mode];
	info->bch_data= NANDFC_BCH_KK_DATA(bch_kk) | NANDFC_BCH_NN_DATA(bch_nn);

	bch_oob_kk = msg_mod  + info->oob_size;
	bch_oob_nn = bch_oob_kk + g_nand_ecc_size[info->ecc_mode];
	info->bch_oob = NANDFC_BCH_KK_OOB(bch_oob_kk) | NANDFC_BCH_NN_OOB(bch_oob_nn);

	rda_dbg_nand("nand_rda_map_type, map result, type = %d\n", info->type);

	return 0;
}

static int hal_init(struct rda_nand_info *info, int nand_type)
{
	unsigned int config_a = 0, config_b = 0,page_para = 0,msg_oob = 0;
	int msg_pkg_num, msg_pkg_mod;
#ifdef CONFIG_RDA_FPGA
	unsigned long clk_div = 7;
#else
	unsigned long clk_div = hal_calc_divider(info);
#endif

	rda_dbg_nand("hal_init, nand_type = %d\n", nand_type);

	__raw_writel((1 << 23), info->reg_base + NANDFC_REG_CONFIG_A);
	__raw_writel(0, info->reg_base + NANDFC_REG_CONFIG_A);

	/* setup config_a and config_b */
	info->nand_controller_init_divider = clk_div;
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

	msg_pkg_num = info->vir_page_size / info->message_len;
	msg_pkg_mod = info->vir_page_size % info->message_len;
	if (msg_pkg_mod)
		msg_pkg_num += 1;
	else
		msg_pkg_mod = info->message_len;
	/*16k page nand need program twice one page*/
	/*package num need change when program if not time of 2*/
	if (info->type == NAND_TYPE_16K || info->type == NAND_TYPE_14K) {
		info->first_data_len_16k = info->message_len * (msg_pkg_num >> 1);
		info->first_ecc_len_16k = (info->page_total_num - info->vir_page_size - \
			info->oob_size) / msg_pkg_num * (msg_pkg_num >> 1);
		msg_pkg_num = msg_pkg_num >> 1;
	}

	msg_oob = NANDFC_OOB_SIZE(info->oob_size) | \
		NANDFC_MESSAGE_SIZE(info->message_len * msg_pkg_num);
	page_para = NANDFC_PAGE_SIZE(info->page_total_num) | \
		NANDFC_PACKAGE_NUM(msg_pkg_num);

	__raw_writel(config_a, info->reg_base + NANDFC_REG_CONFIG_A);
	__raw_writel(config_b, info->reg_base + NANDFC_REG_CONFIG_B);
	__raw_writel(page_para, info->reg_base + NANDFC_REG_PAGE_PARA);
	if (msg_pkg_num == 1)
		info->bch_data = info->bch_oob;
	__raw_writel(info->bch_data, info->reg_base + NADFC_REG_BCH_DATA);
	__raw_writel(info->bch_oob, info->reg_base + NADFC_REG_BCH_OOB);
	__raw_writel(msg_oob, info->reg_base + NADFC_REG_MESSAGE_OOB_SIZE);
	hal_read_normal_mode_timing_set(info);
	__raw_writel(0x80000000,info->reg_base +  NANDFC_REG_BRICK_FSM_TIME2);

	dev_info(info->dev,"bch_data = %x, bch_oob = %x,page_total = %x,oob =%x \n",
			info->bch_data, info->bch_oob,info->page_total_num,info->oob_size);
	dev_info(info->dev,"NAND: nand init done, %08x %08x\n", config_a, config_b);
	dev_info(info->dev,"page_para = %08x,message_oob =  %08x\n", page_para, msg_oob);
	dev_info(info->dev,"bch_oob = %08x,bch_data=  %08x\n", info->bch_oob, info->bch_data);

	return 0;
}

static int nand_rda_init(struct rda_nand_info *info)
{
	int ret;

	ret = nand_rda_map_type(info);
	if (ret) {
		return ret;
	}

	return hal_init(info, info->type);
}

static u8 nand_rda_read_byte(struct mtd_info *mtd)
{
	u8 ret;
	struct nand_chip *chip = mtd->priv;
	struct rda_nand_info *info = chip->priv;
	ret = *((u8 *) info->byte_buf + info->index);
	info->index++;

	rda_dbg_nand("nand_read_byte, ret = %02x\n", ret);
	return ret;
}

static u16 nand_rda_read_word(struct mtd_info *mtd)
{
	u16 ret;
	struct nand_chip *chip = mtd->priv;
	struct rda_nand_info *info = chip->priv;
	ret = *(u16 *) ((uint8_t *) info->byte_buf + info->index);
	info->index += 2;

	rda_dbg_nand("nand_read_word, ret = %04x\n", ret);
	return ret;
}


#ifdef CONFIG_MTD_NAND_RDA_DMA
static int vmalloc_addr_to_phy(u32 addr)
{

	unsigned long pfn;
	pfn = vmalloc_to_pfn((const void *)addr);
	return __pfn_to_phys(pfn) + ( addr & ~PAGE_MASK);
}

static int nand_rda_dma_setup_list_param(u32 virt_addr, u32 nand_phy_addr, int len,
		struct rda_dma_chan_list_params *params)
{
	u32 phy_addr;
	u32 seg_len;
	int i = 0;

	memset(params , 0 ,sizeof(*params));
	while (len > 0) {
		seg_len = PAGE_SIZE - offset_in_page(virt_addr);
		if (seg_len > len)
			seg_len = len;
		phy_addr = vmalloc_addr_to_phy(virt_addr);
		params->addrs[i].src_addr = nand_phy_addr;
		params->addrs[i].dst_addr = phy_addr;
		params->addrs[i].xfer_size = seg_len;
	        virt_addr += seg_len;
		nand_phy_addr += seg_len;
		len -= seg_len;
		i++;
	}
	params->dma_mode = RDA_DMA_FR_MODE;
	params->nr_dma_lists = i;
#ifdef NAND_DMA_POLLING
	params->enable_int = 0;
#else
	params->enable_int = 1;
#endif /* NAND_DMA_POLLING */
	return 0;

}

static int nand_rda_dma_setup_param(u32 dst_phys_addr, u32 src_phys_addr, int len,
		struct rda_dma_chan_params *dma_param)
{
	dma_param->src_addr = src_phys_addr;
	dma_param->dst_addr = dst_phys_addr;
	dma_param->xfer_size = len;
	dma_param->dma_mode = RDA_DMA_FR_MODE;
#ifdef NAND_DMA_POLLING
	dma_param->enable_int = 0;
#else
	dma_param->enable_int = 1;
#endif /* NAND_DMA_POLLING */
	return 0;
}

static int nand_rda_dma_to_ram(u32 nand_phy_addr, void *addr, int len,
		                      struct rda_nand_info *info)
{
	//TODO - is this too big in stack?
	struct rda_dma_chan_params dma_params;
	struct rda_dma_chan_list_params list_dma_params;
	int use_list_param = 0;
	int ret = 0;
	u32 phy_addr;
	/*
	 * Check if the address of buffer is an address allocated by vmalloc.
	 * If so, we don't directly use these address, it might be un-continous
	 * in physical address.
	 */
	if (addr >= high_memory) {
		if (((size_t) addr & PAGE_MASK) !=
		    ((size_t) (addr + len - 1) & PAGE_MASK)) {
			use_list_param = 1;
			ret = nand_rda_dma_setup_list_param((u32)addr, (u32)nand_phy_addr,
				       len, &list_dma_params);
			if (ret < 0)
				return -1;
		} else {
			phy_addr = vmalloc_addr_to_phy((u32)addr);
		}
	} else {
		phy_addr = virt_to_phys(addr);
	}

	if (use_list_param) {
		rda_dma_map_list_params(&list_dma_params, DMA_FROM_DEVICE);
		ret = rda_set_dma_list_params(info->dma_ch, &list_dma_params);
	} else {
		dma_sync_single_for_device(info->dev, phy_addr, len, DMA_FROM_DEVICE);
		nand_rda_dma_setup_param(phy_addr, nand_phy_addr, len, &dma_params);
		ret = rda_set_dma_params(info->dma_ch, &dma_params);
	}
	if (ret < 0) {
		/*TODO - do we really need this if DMA is not started yet??*/
		if (use_list_param) {
			rda_dma_unmap_list_params(&list_dma_params, DMA_FROM_DEVICE);
		} else {
			dma_sync_single_for_cpu(info->dev, phy_addr, len, DMA_FROM_DEVICE);
		}
		dev_err(info->dev, "rda nand : Failed to set parameter\n");
		return -1;
	}

	INIT_COMPLETION(info->comp);

	rda_start_dma(info->dma_ch);
#ifndef NAND_DMA_POLLING
	ret = wait_for_completion_timeout(&info->comp,
			msecs_to_jiffies(NAND_DMA_TIMEOUT_MS));
	if (ret <= 0) {
		dev_err(info->dev, "dma read timeout, ret = 0x%08x\n", ret);
		if (use_list_param) {
			rda_dma_unmap_list_params(&list_dma_params, DMA_FROM_DEVICE);
		} else {
			dma_sync_single_for_cpu(info->dev, phy_addr, len, DMA_FROM_DEVICE);
		}
		return -1;
	}
#else
	rda_poll_dma(info->dma_ch);
	rda_stop_dma(info->dma_ch);
#endif

	if (use_list_param) {
		rda_dma_unmap_list_params(&list_dma_params, DMA_FROM_DEVICE);
	} else {
		dma_sync_single_for_cpu(info->dev, phy_addr, len, DMA_FROM_DEVICE);
	}
	info->read_ptr += len;
	return 0;
}
#endif

static void nand_rda_read_buf(struct mtd_info *mtd, uint8_t * buf, int len)
{
	struct nand_chip *chip = mtd->priv;
	struct rda_nand_info *info = chip->priv;
	u8 *nand_ptr = (u8 *) (chip->IO_ADDR_R + info->read_ptr);
	u8 * u_nand_temp_buffer = &info->databuf[info->read_ptr];

#ifndef CONFIG_MTD_NAND_RDA_DMA
	rda_dbg_nand("nand_rda_read_buf, len = %d, ptr = %d\n", len, info->read_ptr);
	if (info->type == NAND_TYPE_16K || info->type == NAND_TYPE_14K)
		memcpy(buf, u_nand_temp_buffer, len);
	else
		memcpy(buf, nand_ptr, len);
	info->read_ptr += len;
#else

	dma_addr_t nand_phys_addr ;

	rda_dbg_nand("nand_rda_read_buf -- dma, len = %d, ptr = %d\n", len, info->col_addr);
	nand_phys_addr = info->phy_addr + info->read_ptr;
	/*
	 * If size is less than the size of oob,
	 * we copy directly them to mapping buffer.
	 */
	if (len <= mtd->oobsize) {
		if (info->type == NAND_TYPE_16K || info->type == NAND_TYPE_14K)
			memcpy(buf, u_nand_temp_buffer, len);
		else
			memcpy(buf, nand_ptr, len);
		info->read_ptr = 0;
		return;
	}

	//hal_enable_dma(info); hardware bugs, no need to enable this
	if(nand_rda_dma_to_ram(nand_phys_addr, buf, len, info)) {
		if (info->type == NAND_TYPE_16K || info->type == NAND_TYPE_14K)
			memcpy(buf, u_nand_temp_buffer, len);
		else
			memcpy(buf, nand_ptr, len);
		info->read_ptr += len;
	}
#endif
#ifdef NAND_STATISTIC
	info->bytes_read += len;
	if (info->nand_status == 0)
		info->ecc_flips += info->ecc_bits;
	else
		info->read_fails++;
#endif
	return;
}

static void nand_rda_write_buf(struct mtd_info *mtd, const uint8_t * buf,
			       int len)
{
	struct nand_chip *chip = mtd->priv;
	struct rda_nand_info *info = chip->priv;
	u8 *nand_ptr = (u8 *) (chip->IO_ADDR_W + info->write_ptr);
	u8 *u_nand_temp_buffer = &info->databuf[info->write_ptr];

	rda_dbg_nand("nand_rda_write_buf, len = %d, ptr = %d\n", len,
		     info->write_ptr);

#ifndef CONFIG_MTD_NAND_RDA_DMA

	//hal_disable_dma(info); hardware bug, no need to use..
	if (info->type == NAND_TYPE_16K || info->type == NAND_TYPE_14K)
		memcpy(u_nand_temp_buffer, buf, len);
	else
		memcpy(nand_ptr, buf, len);
	info->write_ptr += len;

#else
	struct rda_dma_chan_params dma_param;
	dma_addr_t phys_addr;
	void *addr = (void *)buf;
	dma_addr_t nand_phys_addr = virt_to_phys((void *)nand_ptr);
	int ret = 0;
	struct page *p1;

	/*
	 * If size is less than the size of oob,
	 * we copy directly them to mapping buffer.
	 */
	if (len <= mtd->oobsize) {
		if (info->type == NAND_TYPE_16K || info->type == NAND_TYPE_14K)
			memcpy(u_nand_temp_buffer, buf, len);
		else
			memcpy(nand_ptr, buf, len);
		info->write_ptr += len;
		return;
	}

	/*
	 * Check if the address of buffer is an address allocated by vmalloc.
	 * If so, we don't directly use these address, it might be un-continous
	 * in physical address. We have to copy them via memcpy.
	 */
	if (addr >= high_memory) {
		if (((size_t) addr & PAGE_MASK) !=
		    ((size_t) (addr + len - 1) & PAGE_MASK)) {
			if (info->type == NAND_TYPE_16K || info->type == NAND_TYPE_14K)
				memcpy(u_nand_temp_buffer, buf, len);
			else
				memcpy(nand_ptr, buf, len);
			info->write_ptr += len;
			return;
		}

		p1 = vmalloc_to_page(addr);
		if (!p1) {
			if (info->type == NAND_TYPE_16K || info->type == NAND_TYPE_14K)
				memcpy(u_nand_temp_buffer, buf, len);
			else
				memcpy(nand_ptr, buf, len);
			info->write_ptr += len;
			return;
		}
		addr = page_address(p1) + ((size_t) addr & ~PAGE_MASK);
	}

	if (info->type == NAND_TYPE_16K || info->type == NAND_TYPE_14K)
		nand_phys_addr = virt_to_phys((void *)u_nand_temp_buffer);

	phys_addr = dma_map_single(info->dev, addr, len, DMA_TO_DEVICE);

	dma_param.src_addr = phys_addr;
	dma_param.dst_addr = (u32)nand_phys_addr;
	dma_param.xfer_size = len;
	dma_param.dma_mode = RDA_DMA_FW_MODE;
#ifdef NAND_DMA_POLLING
		dma_param.enable_int = 0;
#else
		dma_param.enable_int = 1;
#endif /* NAND_DMA_POLLING */

	ret = rda_set_dma_params(info->dma_ch, &dma_param);
	if (ret < 0) {
		pr_err("rda nand : Failed to set parameter\n");
		dma_unmap_single(info->dev, addr, len, DMA_TO_DEVICE);
		return;
	}

	INIT_COMPLETION(info->comp);

	rda_start_dma(info->dma_ch);
#ifndef NAND_DMA_POLLING
	ret = wait_for_completion_timeout(&info->comp,
			msecs_to_jiffies(NAND_DMA_TIMEOUT_MS));
	if (ret <= 0) {
		dev_err(info->dev, "dma write timeout, ret = 0x%08x\n", ret);
		dma_unmap_single(info->dev, phys_addr, len, DMA_TO_DEVICE);
		return;
	}
#else
	rda_poll_dma(info->dma_ch);
	rda_stop_dma(info->dma_ch);
#endif /* #if 0 */

	/* Free the specified physical address */
	dma_unmap_single(info->dev, addr, len, DMA_TO_DEVICE);

	/* use flush to avoid annoying unaligned warning */
	/* however, invalidate after the dma it the right thing to do */
	flush_dcache_range((u32)addr, (u32)(addr + len));

	info->write_ptr += len;

#endif
#ifdef NAND_STATISTIC
	info->bytes_write += len;
#endif
	return;
}

static void nand_rda_do_cmd_post(struct mtd_info *mtd, u32 org_cmd)
{
	struct nand_chip *this = mtd->priv;
	struct rda_nand_info *info = this->priv;
	u32 temp;
	u8 *nand_ptr = (u8 *) this->IO_ADDR_R;
	u8 *u_nand_temp_buffer = info->databuf;

	switch (info->cmd) {
	case NAND_CMD_RESET:
		return;
	case NAND_CMD_READID:
		temp = __raw_readl(info->reg_base + NANDFC_REG_IDCODE_A);
		info->byte_buf[0] = temp;
		if (info->nand_id[0] == 0)
			info->nand_id[0] = temp;
		temp = __raw_readl(info->reg_base + NANDFC_REG_IDCODE_B);
		info->byte_buf[1] = temp;
		if (info->nand_id[1] == 0)
			info->nand_id[1] = temp;
		info->index = 0;
		printk("nand id: %x  %x \n", info->nand_id[0], info->nand_id[1]);
		break;
	case NAND_CMD_STATUS:
		temp = __raw_readl(info->reg_base + NANDFC_REG_OP_STATUS);
		info->byte_buf[0] = (temp & 0xFF);
		info->index = 0;
		if (info->byte_buf[0] & 0x3) {
			/* lower 2 bits are indicator of error
			 * see NAND_STATUS_FAIL
			 * */
			dev_err(info->dev, "nand error, op status = %x\n",
			       info->byte_buf[0]);
		}
		break;
	case NAND_CMD_SEQIN:
#ifdef NAND_STATISTIC
		if (info->nand_status != 0)
			info->write_fails++;
#endif
		break;
	case NAND_CMD_READ0:
		if(info->type == NAND_TYPE_16K || info->type == NAND_TYPE_14K) {
			memcpy((u_nand_temp_buffer + info->first_data_len_16k),
				nand_ptr,
				(info->vir_page_size + info->oob_size - info->first_data_len_16k));
		}

		if (info->nand_status == 0) {
			info->ecc_bits = hal_get_flipbits(info);
		} else {
			info->ecc_bits = -EBADMSG;
		}
		break;
	default:
		break;
	}

	return;
}

static void nand_rda_hwcontrol(struct mtd_info *mtd, int cmd, unsigned int ctrl)
{
	rda_dbg_nand("nand_rda_hwcontrol, cmd = %x, ctrl = %x, not use anymore\n",
				cmd, ctrl);
}

static int nand_rda_dev_ready(struct mtd_info *mtd)
{
	return 1;
}

void nand_cmd_reset_flash(struct rda_nand_info *info)
{
	hal_send_cmd_brick(info,NAND_CMD_RESET,0,0,5);
	hal_send_cmd_brick(info,0,0,0,0);
	hal_start_cmd_data(info);
}

void nand_cmd_read_id(struct rda_nand_info *info)
{
	hal_send_cmd_brick(info, NAND_CMD_READID,0,0,2);
	hal_send_cmd_brick(info,0,0,0,6);
	hal_send_cmd_brick(info,0,0,0,0xd);
	hal_send_cmd_brick(info,0,0,7,0);
	hal_start_cmd_data(info);
}

void nand_cmd_write_page(struct rda_nand_info *info,unsigned int page_addr,u16 page_size, u16 col_addr)
{
	u8 addr_temp = 0;

	hal_send_cmd_brick(info,NAND_CMD_SEQIN,0,0,2);

	addr_temp = col_addr & 0xff;
	hal_send_cmd_brick(info,addr_temp,0,0,2);
	addr_temp = (col_addr >> 8) & 0xff;
	hal_send_cmd_brick(info,addr_temp,0,0,2);

	if (NAND_BLOCK_SIZE == 0x300000) {
		/*page 0x180 ~ 0x1ff are invalid, so need skip to next block.*/
		page_addr += 0x80 * (page_addr / 0x180);
		/*block 0xab0 ~ 0xfff are invalid, so need skip to next LUN.*/
		page_addr += (0x550 << 9) * ((page_addr >> 9) / 0xab0);
	}

	addr_temp = page_addr & 0xff;
	hal_send_cmd_brick(info,addr_temp,0,0,2);
	addr_temp = (page_addr >> 8) & 0xff;
	hal_send_cmd_brick(info,addr_temp,0,0,2);
	addr_temp = (page_addr >> 16) & 0xff;
	hal_send_cmd_brick(info,addr_temp,0,0,7);

	hal_send_cmd_brick(info,0,0,page_size,3);

	hal_send_cmd_brick(info,0,info->bus_width_16,page_size, 1);

	hal_send_cmd_brick(info,NAND_CMD_PAGEPROG,0,0,5);

	hal_send_cmd_brick(info,0,0,0,0);

	return;
}

void nand_cmd_read_page(struct rda_nand_info *info,unsigned int page_addr,
							unsigned short num, unsigned short col_addr)
{
	u8 addr_temp = 0;

	hal_send_cmd_brick(info,NAND_CMD_READ0,0,0,2);

	addr_temp = col_addr & 0xff;
	hal_send_cmd_brick(info,addr_temp,0,0,2);
	addr_temp = (col_addr >> 8)& 0xff;
	hal_send_cmd_brick(info,addr_temp,0,0,2);

	if (NAND_BLOCK_SIZE == 0x300000) {
		/*page 0x180 ~ 0x1ff are invalid, so need skip to next block.*/
		page_addr += 0x80 * (page_addr / 0x180);
		/*block 0xab0 ~ 0xfff are invalid, so need skip to next LUN.*/
		page_addr += (0x550 << 9) * ((page_addr >> 9) / 0xab0);
	}

	addr_temp = page_addr & 0xff;
	hal_send_cmd_brick(info,addr_temp,0,0,2);
	addr_temp = (page_addr >> 8) & 0xff;
	hal_send_cmd_brick(info,addr_temp,0,0,2);
	addr_temp = (page_addr >> 16) & 0xff;
	hal_send_cmd_brick(info,addr_temp,0,0,1);

	hal_send_cmd_brick(info,NAND_CMD_READSTART,0,0,5);
	hal_send_cmd_brick(info,0,0,0,0xa);
	hal_send_cmd_brick(info,0,0,0,0x4);

	hal_send_cmd_brick(info,0,info->bus_width_16,num, 0);

	return;
}

void nand_cmd_read_random(struct rda_nand_info *info, unsigned short num,
								unsigned short col_addr)
{
	u8 addr_temp = 0;

	hal_send_cmd_brick(info,NAND_CMD_RNDOUT,0,0,2);

	addr_temp = col_addr & 0xff;
	hal_send_cmd_brick(info,addr_temp,0,0,2);
	addr_temp = (col_addr >> 8)& 0xff;
	hal_send_cmd_brick(info,addr_temp,0,0,1);
	hal_send_cmd_brick(info,NAND_CMD_RNDOUTSTART,0,0,9);
	hal_send_cmd_brick(info,0,0,0,0x4);

	hal_send_cmd_brick(info,0,info->bus_width_16,num, 0);

	return;
}
void nand_cmd_block_erase(struct rda_nand_info *info, unsigned int page_addr)
{
	u8 addr_temp = 0;

	hal_send_cmd_brick(info,NAND_CMD_ERASE1,0,0,2);

	if (NAND_BLOCK_SIZE == 0x300000) {
		/*page 0x180 ~ 0x1ff are invalid, so need skip to next block.*/
		page_addr += 0x80 * (page_addr / 0x180);
		/*block 0xab0 ~ 0xfff are invalid, so need skip to next LUN.*/
		page_addr += (0x550 << 9) * ((page_addr >> 9) / 0xab0);
	}

	addr_temp = page_addr & 0xff;
	hal_send_cmd_brick(info,addr_temp,0,0,2);
	addr_temp = (page_addr >> 8) & 0xff;
	hal_send_cmd_brick(info,addr_temp,0,0,2);
	addr_temp = (page_addr >> 16) & 0xff;
	hal_send_cmd_brick(info,addr_temp,0,0,1);

	hal_send_cmd_brick(info,NAND_CMD_ERASE2,0,0,5);
	hal_send_cmd_brick(info,0,0,0,0);

	hal_start_cmd_data(info);
}

void nand_cmd_read_status(struct rda_nand_info *info)
{
	hal_send_cmd_brick(info,NAND_CMD_STATUS,0,0,6);
	hal_send_cmd_brick(info,0,0,0,0xd);
	hal_send_cmd_brick(info,0,0,1,0);

	hal_start_cmd_data(info);
}

void nand_toshiba_read_retry_scmd(struct rda_nand_info *info, u8 cmd)
{
#ifdef NAND_IRQ_POLL
	unsigned long cmd_ret;
#else
	unsigned long timeout;
#endif /* NAND_IRQ_POLL */

#ifndef NAND_IRQ_POLL
	INIT_COMPLETION(info->nand_comp);
#endif

	hal_send_cmd_brick(info,cmd,0,0,5);
	hal_send_cmd_brick(info,0,0,0,0xb);
	hal_send_cmd_brick(info,0,0,0,0);
	hal_start_cmd_data(info);

#ifdef NAND_IRQ_POLL
	cmd_ret = hal_wait_cmd_complete(info);
	if (cmd_ret) {
		dev_err(info->dev, "read retry scmd fail! cmd = 0x%x \n", cmd);
	}
#else
	timeout = wait_for_completion_timeout(&info->nand_comp,
					      msecs_to_jiffies(NAND_CMD_TIMEOUT_MS));
	if (timeout == 0) {
		dev_err(info->dev, "read retry scmd time out! cmd = 0x%x\n", cmd);
	}

	if (info->nand_status) {
		dev_err(info->dev, "read retry scmd fail! cmd = 0x%x, status = 0x%x\n",
			cmd, info->nand_status);
	}
#endif /* NAND_IRQ_POLL */
}

void nand_toshiba_19nm_read_retry_sprm_set(struct rda_nand_info *info, u8 addr, u8 data)
{
#ifdef NAND_IRQ_POLL
	unsigned long cmd_ret;
#else
	unsigned long timeout;
#endif /* NAND_IRQ_POLL */

#ifndef NAND_IRQ_POLL
	INIT_COMPLETION(info->nand_comp);
#endif

	hal_send_cmd_brick(info,0x55,0,0,2);
	hal_send_cmd_brick(info,addr,0,0,7);
	hal_send_cmd_brick(info,0,0,0,0xc);
	hal_send_cmd_brick(info,data,0,0,5);
	hal_send_cmd_brick(info,0,0,0,0);
	hal_send_cmd_brick(info,0,0,0,0);

	hal_start_cmd_data(info);

#ifdef NAND_IRQ_POLL
	cmd_ret = hal_wait_cmd_complete(info);
	if (cmd_ret) {
		dev_err(info->dev, "read retry sprm fail! addr = 0x%x data = 0x%x\n", addr, data);
	}
#else
	timeout = wait_for_completion_timeout(&info->nand_comp,
					      msecs_to_jiffies(NAND_CMD_TIMEOUT_MS));
	if (timeout == 0) {
		dev_err(info->dev, "read retry sprm time out! addr = 0x%x data = 0x%x\n", addr, data);
	}

	if (info->nand_status) {
		dev_err(info->dev, "read retry sprm fail! addr = 0x%x data = 0x%x status = 0x%x\n",
			addr, data, info->nand_status);
	}
#endif /* NAND_IRQ_POLL */
}
void nand_toshiba_15nm_read_retry_sprm_set(struct rda_nand_info *info, u8 times)
{
#ifdef NAND_IRQ_POLL
	unsigned long cmd_ret;
#else
	unsigned long timeout;
#endif /* NAND_IRQ_POLL */

#ifndef NAND_IRQ_POLL
	INIT_COMPLETION(info->nand_comp);
#endif

	hal_send_cmd_brick(info,0xd5,0,0,2);
	hal_send_cmd_brick(info,0x00,0,0,2);
	hal_send_cmd_brick(info,0x89,0,0,7);
	hal_send_cmd_brick(info,0,0,0,0xc);
	hal_send_cmd_brick(info,g_read_retry_cmd_15nm_sequence[times][0],0,3,0x0c);
	hal_send_cmd_brick(info,g_read_retry_cmd_15nm_sequence[times][1],0,3,0x0c);
	hal_send_cmd_brick(info,g_read_retry_cmd_15nm_sequence[times][2],0,3,0x0c);
	hal_send_cmd_brick(info,g_read_retry_cmd_15nm_sequence[times][3],0,3,5);
	hal_send_cmd_brick(info,0,0,0,0);

	hal_start_cmd_data(info);

#ifdef NAND_IRQ_POLL
	cmd_ret = hal_wait_cmd_complete(info);
	if (cmd_ret) {
		dev_err(info->dev, "read retry sprm fail! data = 0x%x\n",g_read_retry_cmd_15nm_sequence[times][0] );
	}
#else
	timeout = wait_for_completion_timeout(&info->nand_comp,
					      msecs_to_jiffies(NAND_CMD_TIMEOUT_MS));
	if (timeout == 0) {
		dev_err(info->dev, "read retry sprm time out!data = 0x%x\n",g_read_retry_cmd_15nm_sequence[times][0]);
	}

	if (info->nand_status) {
		dev_err(info->dev, "read retry sprm fail! data = 0x%x status = 0x%x\n",
			 g_read_retry_cmd_15nm_sequence[times][0], info->nand_status);
	}
#endif /* NAND_IRQ_POLL */
}

void nand_toshiba_read_retry_reset(struct rda_nand_info *info)
{
#ifdef NAND_IRQ_POLL
	unsigned long cmd_ret;
#else
	unsigned long timeout;
#endif /* NAND_IRQ_POLL */

#ifndef NAND_IRQ_POLL
	INIT_COMPLETION(info->nand_comp);
#endif

	hal_send_cmd_brick(info,0xff,0,0,5);
	hal_send_cmd_brick(info,0,0,0,0);
	hal_start_cmd_data(info);

#ifdef NAND_IRQ_POLL
	cmd_ret = hal_wait_cmd_complete(info);
	if (cmd_ret) {
		dev_err(info->dev, "read retry cmd fail! cmd = 0xff \n");
	}
#else
	timeout = wait_for_completion_timeout(&info->nand_comp,
					      msecs_to_jiffies(NAND_CMD_TIMEOUT_MS));
	if (timeout == 0) {
		dev_err(info->dev, "read retry cmd time out! cmd = 0xff\n");
	}

	if (info->nand_status) {
		dev_err(info->dev, "read retry cmd fail! cmd = 0xff, status = 0x%x\n",
			info->nand_status);
	}
#endif /* NAND_IRQ_POLL */
}

void nand_toshiba_read_retry_enter(struct rda_nand_info *info)
{
	u8 i;
	hal_set_controller_divider(info, 20);

	for (i = 0; i < ARRAY_SIZE(g_read_retry_cmd_enter_19nm_sequence); i++)
		nand_toshiba_read_retry_scmd(info, g_read_retry_cmd_enter_19nm_sequence[i]);
}

void nand_toshiba_19nm_read_retry_startops(struct rda_nand_info *info)
{
	u8 i;
	for (i = 0; i < ARRAY_SIZE(g_read_retry_cmd_startops_19nm_sequence); i++)
		nand_toshiba_read_retry_scmd(info, g_read_retry_cmd_startops_19nm_sequence[i]);
}

void nand_toshiba_15nm_read_retry_startops(struct rda_nand_info *info)
{
	u8 i;
	for (i = 0; i < ARRAY_SIZE(g_read_retry_cmd_startops_15nm_sequence); i++)
		nand_toshiba_read_retry_scmd(info, g_read_retry_cmd_startops_15nm_sequence[i]);
}

void nand_toshiba_19nm_read_retry_exit(struct rda_nand_info *info)
{
	u8 i;
	u8 count = ARRAY_SIZE(g_read_retry_cmd_19nm_sequence);
	u8 num = ARRAY_SIZE(g_read_retry_cmd_19nm_sequence[count - 1]);

	hal_read_retry_mode_timing_set(info);
	for (i = 0; i < num; i++)
		nand_toshiba_19nm_read_retry_sprm_set(info,
			g_read_retry_cmd_19nm_sequence[count - 1][i].addr,
			g_read_retry_cmd_19nm_sequence[count - 1][i].data);
	hal_read_normal_mode_timing_set(info);
	nand_toshiba_read_retry_reset(info);
	hal_set_controller_divider(info, info->nand_controller_init_divider);
	udelay(20);
}

void nand_toshiba_15nm_read_retry_exit(struct rda_nand_info *info)
{
	u8 count = ARRAY_SIZE(g_read_retry_cmd_15nm_sequence);

	nand_toshiba_15nm_read_retry_sprm_set(info, (count - 1));

	nand_toshiba_read_retry_reset(info);
	hal_set_controller_divider(info, info->nand_controller_init_divider);
	udelay(20);
}
void nandfc_page_write_first(struct rda_nand_info *info,unsigned int page_addr,
							u16 page_size, u16 col_addr)
{
        u8 addr_temp = 0;

        hal_send_cmd_brick(info,NAND_CMD_SEQIN,0,0,2);

        addr_temp = col_addr & 0xff;
        hal_send_cmd_brick(info,addr_temp,0,0,2);
        addr_temp = (col_addr >> 8) & 0xff;
        hal_send_cmd_brick(info,addr_temp,0,0,2);

	if (NAND_BLOCK_SIZE == 0x300000) {
		/*page 0x180 ~ 0x1ff are invalid, so need skip to next block.*/
		page_addr += 0x80 * (page_addr / 0x180);
		/*block 0xab0 ~ 0xfff are invalid, so need skip to next LUN.*/
		page_addr += (0x550 << 9) * ((page_addr >> 9) / 0xab0);
	}

        addr_temp = page_addr & 0xff;
        hal_send_cmd_brick(info,addr_temp,0,0,2);
        addr_temp = (page_addr >> 8) & 0xff;
        hal_send_cmd_brick(info,addr_temp,0,0,2);
        addr_temp = (page_addr >> 16) & 0xff;
        hal_send_cmd_brick(info,addr_temp,0,0,7);

        hal_send_cmd_brick(info,0,0,0,3);

        hal_send_cmd_brick(info,0,info->bus_width_16,page_size, 0);

        return;
}

void nandfc_page_write_random(struct rda_nand_info *info,u16 col_addr,
									int data_num, char cmd_2nd)
{
        u8 addr_temp = 0;

        hal_send_cmd_brick(info,NAND_CMD_RNDIN,0,0,2);

        addr_temp = col_addr & 0xff;
        hal_send_cmd_brick(info,addr_temp,0,0,2);
        addr_temp = (col_addr >> 8) & 0xff;
        hal_send_cmd_brick(info,addr_temp,0,0,9);

        hal_send_cmd_brick(info,0,0,data_num, 3);
        hal_send_cmd_brick(info,0,info->bus_width_16,data_num, 1);

        hal_send_cmd_brick(info,cmd_2nd,0,0,5);

        hal_send_cmd_brick(info,0,0,0,0);
}

static void nand_page_write_all(unsigned int page_addr, struct mtd_info *mtd)
{
	u16 page_size = 0;
	u16 col_addr = 0;
	u32 msg_oob = 0;
	register struct nand_chip *chip = mtd->priv;
	struct rda_nand_info *info = chip->priv;
	u8 *nand_ptr = (u8 *) chip->IO_ADDR_W;
	u8 *u_nand_temp_buffer = info->databuf;

	if (info->type == NAND_TYPE_16K || info->type == NAND_TYPE_14K) {
		msg_oob = NANDFC_OOB_SIZE(0) | \
			NANDFC_MESSAGE_SIZE(info->first_data_len_16k);

		__raw_writel(info->bch_data, info->reg_base + NADFC_REG_BCH_OOB);
		__raw_writel(msg_oob, info->reg_base + NADFC_REG_MESSAGE_OOB_SIZE);

		page_size = info->first_data_len_16k + info->first_ecc_len_16k;
		col_addr = page_size;

		if (info->bus_width_16)
			page_size = page_size - 2;
		else
			page_size = page_size - 1;

		hal_flush_buf(info);
		memcpy(nand_ptr, u_nand_temp_buffer, info->first_data_len_16k);
	} else {
		if (info->bus_width_16)
			page_size = info->page_total_num - 2;
		else
			page_size = info->page_total_num - 1;
	}

	if (info->type == NAND_TYPE_16K || info->type == NAND_TYPE_14K)
		nandfc_page_write_first(info,page_addr, page_size, 0);
	else
		nand_cmd_write_page(info,page_addr, page_size, 0);
	hal_start_cmd_data(info);

	if (info->type == NAND_TYPE_16K || info->type == NAND_TYPE_14K) {
#ifdef NAND_IRQ_POLL
		unsigned long ret = hal_wait_cmd_complete(info);
		if (ret) {
			pr_err("%s send write cmd first failed\n",__FUNCTION__);
		}
#else
		unsigned long timeout = wait_for_completion_timeout(&info->nand_comp,
					      msecs_to_jiffies
					      (NAND_CMD_TIMEOUT_MS));
		if (timeout == 0) {
			dev_err(info->dev, "cmd write timeout, first half page: cmd = 0x%x,"
				" page_addr = 0x%x, col_addr = 0x%x\n",
				info->cmd, info->page_addr, info->col_addr);
		}

		if (info->nand_status) {
			dev_err(info->dev, "cmd write fail, first half page: cmd = 0x%x,"
				" page_addr = 0x%x, col_addr = 0x%x, status = 0x%x\n",
				info->cmd, info->page_addr, info->col_addr,
				info->nand_status);
		}
#endif

#ifndef NAND_IRQ_POLL
		INIT_COMPLETION(info->nand_comp);
#endif
		msg_oob = NANDFC_OOB_SIZE(info->oob_size) |
			NANDFC_MESSAGE_SIZE(info->vir_page_size - info->first_data_len_16k);

		__raw_writel(info->bch_oob, info->reg_base + NADFC_REG_BCH_OOB);
		__raw_writel(msg_oob, info->reg_base + NADFC_REG_MESSAGE_OOB_SIZE);
		page_size = info->page_total_num - col_addr;

		if (info->bus_width_16){
			page_size = page_size - 2;
			col_addr = col_addr >> 1;
		} else
			page_size = page_size - 1;

		hal_flush_buf(info);

		memcpy(nand_ptr, (u_nand_temp_buffer + info->first_data_len_16k),
				info->vir_page_size + info->oob_size - info->first_data_len_16k);

		//nand_cmd_write_page(page_addr, page_size, col_addr);
		nandfc_page_write_random(info, col_addr, page_size, NAND_CMD_PAGEPROG);
		hal_start_cmd_data(info);
	}
}

static void nand_page_read_all(unsigned int page_addr, struct mtd_info *mtd)
{
	unsigned short num = 0, col_addr = 0;
	u32 msg_oob = 0;
	register struct nand_chip *chip = mtd->priv;
	struct rda_nand_info *info = chip->priv;
	u8 *nand_ptr = (u8 *) chip->IO_ADDR_R;
	u8 *u_nand_temp_buffer = info->databuf;

	if (info->type == NAND_TYPE_16K || info->type == NAND_TYPE_14K) {
		msg_oob = NANDFC_OOB_SIZE(0) | \
			NANDFC_MESSAGE_SIZE(info->first_data_len_16k);

		__raw_writel(info->bch_data, info->reg_base + NADFC_REG_BCH_OOB);
		__raw_writel(msg_oob, info->reg_base + NADFC_REG_MESSAGE_OOB_SIZE);

		num = info->first_data_len_16k + info->first_ecc_len_16k;
	} else
		num = info->page_total_num;

	nand_cmd_read_page(info, page_addr, num, col_addr);
	hal_start_cmd_data(info);

	if (info->type == NAND_TYPE_16K || info->type == NAND_TYPE_14K) {
#ifdef NAND_IRQ_POLL
		unsigned long ret = hal_wait_cmd_complete(info);
		if (ret){
			pr_err("%s send read cmd first failed\n",__FUNCTION__);
			info->read_retry_error = 1;
		}
#else
		unsigned long timeout = wait_for_completion_timeout(&info->nand_comp,
					      msecs_to_jiffies
					      (NAND_CMD_TIMEOUT_MS));
		if (timeout == 0) {
			dev_err(info->dev, "cmd read timeout, first half page: cmd = 0x%x,"
				" page_addr = 0x%x, col_addr = 0x%x\n",
				info->cmd, info->page_addr, info->col_addr);
		}

		if (info->nand_status) {
			dev_err(info->dev, "cmd read fail, first half page: cmd = 0x%x,"
				" page_addr = 0x%x, col_addr = 0x%x, status = 0x%x\n",
				info->cmd, info->page_addr, info->col_addr,
				info->nand_status);
			info->read_retry_error = 1;
		}
#endif

		memcpy(u_nand_temp_buffer, nand_ptr, info->first_data_len_16k);

#ifndef NAND_IRQ_POLL
		INIT_COMPLETION(info->nand_comp);
#endif
		msg_oob = NANDFC_OOB_SIZE(info->oob_size) |
			NANDFC_MESSAGE_SIZE(info->vir_page_size - info->first_data_len_16k);

		__raw_writel(info->bch_oob, info->reg_base + NADFC_REG_BCH_OOB);
		__raw_writel(msg_oob, info->reg_base + NADFC_REG_MESSAGE_OOB_SIZE);

		col_addr = num;
		num = info->page_total_num - col_addr;

		nand_cmd_read_random(info, num, col_addr);
		hal_start_cmd_data(info);
	}
}

static void nand_send_cmd_all(struct mtd_info *mtd)
{
	register struct nand_chip *chip = mtd->priv;
	struct rda_nand_info *info = chip->priv;
	unsigned int page_addr = info->page_addr;

	switch(info->cmd){
		case NAND_CMD_READ0:
			nand_page_read_all(page_addr, mtd);
			break;
		case NAND_CMD_RESET:
			nand_cmd_reset_flash(info);
			break;
		case NAND_CMD_READID:
			nand_cmd_read_id(info);
			break;
		case NAND_CMD_ERASE1:
			nand_cmd_block_erase(info,page_addr);
			break;
		case NAND_CMD_PAGEPROG:
		case NAND_CMD_SEQIN:
			nand_page_write_all(page_addr, mtd);
			break;
		case NAND_CMD_STATUS:
			nand_cmd_read_status(info);
			break;
		default:
			return;
	}
}

static void nand_rda_cmdfunc(struct mtd_info *mtd, unsigned int command,
			     int column, int page_addr)
{
	u8 i;
	register struct nand_chip *chip = mtd->priv;
	struct rda_nand_info *info = chip->priv;
#ifdef NAND_IRQ_POLL
	unsigned long cmd_ret;
#else
	unsigned long timeout;
#endif /* NAND_IRQ_POLL */

	rda_dbg_nand("nand_rda_cmdfunc, cmd = %x, page_addr = %x, col_addr = %x\n",
	     				command, page_addr, column);
	info->read_retry_cmd_bak = command;

	/* remap command */
	switch (command) {
	case NAND_CMD_SEQIN:	/* 0x80 do nothing, wait for 0x10 */
		info->page_addr = page_addr;
		info->col_addr = column;
		info->cmd = NAND_CMD_NONE;
		info->write_ptr= column;
		hal_flush_buf(info);
		break;
	case NAND_CMD_READSTART:	/* hw auto gen 0x30 for read */
	case NAND_CMD_ERASE2:	/* hw auto gen 0xd0 for erase */
		rda_dbg_nand("erase block, erase_size = %x\n", mtd->erasesize);
		info->cmd = NAND_CMD_NONE;
		break;
	case NAND_CMD_PAGEPROG:	/* 0x10 do the real program */
		info->cmd = NAND_CMD_SEQIN;
		/*
		 * As facing only oob case,
		 * we'll modify column address to zero to fake a writing page.
		 */
		if (info->col_addr == mtd->writesize ) {
			info->col_addr = 0;
		}
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
		hal_flush_buf(info);
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

	rda_dbg_nand("  after remap, cmd = %x, page_addr = %x, col_addr = %x\n",
		     info->cmd, info->page_addr, info->col_addr);
/*
	if (info->col_addr != -1) {
		hal_set_col_addr(info,info->col_addr);
	}
*/
	if (info->page_addr == -1)
		info->page_addr = 0;

loop_read:
#ifndef NAND_IRQ_POLL
	INIT_COMPLETION(info->nand_comp);
#endif

	nand_send_cmd_all(mtd);
#ifdef NAND_IRQ_POLL
	cmd_ret = hal_wait_cmd_complete(info);
	if (cmd_ret) {
		dev_err(info->dev, "cmd fail, cmd = %x,"
			" page_addr = %x, col_addr = %x\n",
			info->cmd, info->page_addr, info->col_addr);
		info->read_retry_error |= 1;
	}
#else
	timeout = wait_for_completion_timeout(&info->nand_comp,
					      msecs_to_jiffies
					      (NAND_CMD_TIMEOUT_MS));
	if (timeout == 0) {
		dev_err(info->dev, "cmd timeout : cmd = 0x%x,"
			" page_addr = 0x%x, col_addr = 0x%x\n",
			command, info->page_addr, info->col_addr);
	}

	if (info->nand_status) {
		dev_err(info->dev, "cmd fail : cmd = 0x%x,"
			" page_addr = 0x%x, col_addr = 0x%x, status = 0x%x\n",
			command, info->page_addr, info->col_addr,
			info->nand_status);
		info->read_retry_error |= 1;
	}
#endif /* NAND_IRQ_POLL */

	nand_rda_do_cmd_post(mtd, command);

	if ((info->type == NAND_TYPE_16K || info->type == NAND_TYPE_14K) &&
		info->read_retry_error &&
		info->read_retry_cmd_bak == NAND_CMD_READ0) {
		if (((info->nand_id[0] & 0xff) == NAND_MFR_TOSHIBA) && (((info->nand_id[1] >> 8) & 0xff) == 0x50)) {
			u8 count = ARRAY_SIZE(g_read_retry_cmd_19nm_sequence);
			u8 num = ARRAY_SIZE(g_read_retry_cmd_19nm_sequence[0]);
			if (info->read_retry_time >= (count - 1))
				goto read_error;

			hal_read_retry_mode_timing_set(info);

			if (info->read_retry_time == 0) {
				nand_toshiba_read_retry_enter(info);
			}

			for (i = 0; i < num; i++)
				nand_toshiba_19nm_read_retry_sprm_set(info,
					g_read_retry_cmd_19nm_sequence[info->read_retry_time][i].addr,
					g_read_retry_cmd_19nm_sequence[info->read_retry_time][i].data);

			if (info->read_retry_time == 3)
				nand_toshiba_read_retry_scmd(info, g_read_retry_cmd_special_19nm_sequence[0]);

			nand_toshiba_19nm_read_retry_startops(info);
			hal_read_normal_mode_timing_set(info);
		} else if(((info->nand_id[0] & 0xff) == NAND_MFR_TOSHIBA) && (((info->nand_id[1] >> 8) & 0xff) == 0x51)){
				u8 count = ARRAY_SIZE(g_read_retry_cmd_15nm_sequence);
			if (info->read_retry_time >= (count - 1))
				goto read_error;


			nand_toshiba_15nm_read_retry_sprm_set(info,info->read_retry_time);
			nand_toshiba_15nm_read_retry_startops(info);

		}else if ((info->nand_id[0] & 0xff) == NAND_MFR_SAMSUNG) {
			//debug later
		} else {
			//other nand need debug or not support read retry mode
		}

		info->read_retry_time += 1;
		info->read_retry_error = 0;
		dev_info(info->dev, "nand read retry time : %d \n", info->read_retry_time);

		goto loop_read;

read_error:
		dev_err(info->dev, "nand read fail, could not be read retry mode corrected,"
				" data will be lost, please change nand chip!!!!!\n");
		info->read_retry_time = 0xff ;
		nand_assert(0);
	}

	if (info->read_retry_time > 0) {
		u8 count=0;
		if (((info->nand_id[0] & 0xff) == NAND_MFR_TOSHIBA) && (((info->nand_id[1] >> 8) & 0xff) == 0x50))
			count = ARRAY_SIZE(g_read_retry_cmd_19nm_sequence);
		else if(((info->nand_id[0] & 0xff) == NAND_MFR_TOSHIBA) && (((info->nand_id[1] >> 8) & 0xff) == 0x51))
			count = ARRAY_SIZE(g_read_retry_cmd_15nm_sequence);

		if (info->read_retry_time < count)
			dev_info(info->dev, "nand read retry success!\n");
		else
			dev_info(info->dev, "nand read retry failed!\n");

		dev_info(info->dev, "nand exit read retry mode!\n");
		if (((info->nand_id[0] & 0xff) == NAND_MFR_TOSHIBA) && (((info->nand_id[1] >> 8) & 0xff) == 0x50))
			nand_toshiba_19nm_read_retry_exit(info);
		else if(((info->nand_id[0] & 0xff) == NAND_MFR_TOSHIBA) && (((info->nand_id[1] >> 8) & 0xff) == 0x51))
			nand_toshiba_15nm_read_retry_exit(info);
		info->read_retry_time = 0;
		info->read_retry_error = 0;
	}

	return;
}

static void nand_rda_select_chip(struct mtd_info *mtd, int chip_num)
{
	register struct nand_chip *chip = mtd->priv;
	struct rda_nand_info *info = chip->priv;

	unsigned int config_a = __raw_readl(info->reg_base + NANDFC_REG_CONFIG_A);

	if ((chip_num == -1) || (chip_num > 3))
		return;

	config_a |= NANDFC_CHIP_SEL(0x0f);
	config_a &= ~ NANDFC_CHIP_SEL(1 << chip_num);

	__raw_writel(config_a, info->reg_base + NANDFC_REG_CONFIG_A);
	rda_dbg_nand("%s:config_a = %x \n",__FUNCTION__, config_a);
}

static int nand_rda_init_size(struct mtd_info *mtd, struct nand_chip *this,
			u8 *id_data)
{
	struct rda_nand_info *info = this->priv;

	info->oob_size = NAND_OOBSIZE;
	mtd->oobsize = info->oob_size;
	if (mtd->oobsize != 32 && mtd->oobsize != 16)
		dev_err(info->dev, "nand oob size config error, should be 32 or 16, "
		"but config is %x, reconfig in file target/config.mk\n", info->oob_size);

	return (info->bus_width_16) ? NAND_BUSWIDTH_16 : 0;
}

#ifndef NAND_IRQ_POLL
static irqreturn_t nand_rda_irq_handler(int irq, void *data)
{
	struct rda_nand_info *info = (struct rda_nand_info *)data;
	u32 int_status;

	/* get and clear interrupts */
	int_status = hal_irq_get_status(info);
	hal_irq_clear(info, int_status);

	if (int_status & NANDFC_INT_ERR_ALL) {
		info->nand_status = int_status;
	} else if (int_status & NANDFC_INT_DONE) {
		info->nand_status = 0;
	} else {
		/* Nothing to do */
	}

	complete(&info->nand_comp);

	return IRQ_HANDLED;
}
#endif /* NAND_IRQ_POLL */

#ifdef CONFIG_PROC_FS
static int nand_rda_reg_dump(struct seq_file *m, struct rda_nand_info *info)
{
	int len = 0;

	len += seq_printf(m ,"config A : 0x%x\n",
			(u32)__raw_readl(info->reg_base + NANDFC_REG_CONFIG_A));

	len += seq_printf(m ,"config B : 0x%x\n",
			(u32)__raw_readl(info->reg_base + NANDFC_REG_CONFIG_B));


	len += seq_printf(m ,"int status : 0x%x\n",
			(u32)__raw_readl(info->reg_base + NANDFC_REG_INT_STAT));

	return len;
}

static int nand_rda_proc_show(struct seq_file *m,void *v)
{
	int len;
	struct rda_nand_info *info = m->private;

	len = seq_printf(m, "Total Size: %dMiB\n", (int)(info->nand_chip.chipsize/SZ_1M));
	len += seq_printf(m, "Page Size : %dB\n",  info->mtd.writesize);

#ifdef NAND_STATISTIC
	len += seq_printf(m, "total read size: %llu KB\n", info->bytes_read/SZ_1K);
	len += seq_printf(m, "total write size: %llu KB\n", info->bytes_write/SZ_1K);
	len += seq_printf(m, "total flip bits: %llu \n", info->ecc_flips);
	len += seq_printf(m, "total read fails: %llu \n", info->read_fails);
	len += seq_printf(m, "total write fails: %llu \n", info->write_fails);
#endif
	len += nand_rda_reg_dump(m, info);
	return len;
}

static int nand_rda_proc_open(struct inode *inode, struct file *file)
{
	struct rda_nand_info *info = PDE_DATA(inode);
	return single_open(file, nand_rda_proc_show, info);
}

static const struct file_operations nand_rda_proc_fops = {
	.open		= nand_rda_proc_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= single_release
};

static void nand_rda_create_proc(struct rda_nand_info *info)
{
	struct proc_dir_entry *pde;
	pde = proc_create_data(PROCNAME, 0664, NULL, &nand_rda_proc_fops, info);
	if (pde == NULL) {
		pr_err("%s failed\n", __func__);
	}
}

static void  nand_rda_delete_proc(void)
{
	remove_proc_entry(PROCNAME, NULL);
}
#else

static void spi_nand_rda_create_proc(struct rda_nand_info *info) {}

static void spi_nand_rda_delete_proc(void) {}
#endif

/**
 * nand_rda_wait - wait until the command is done
 * @mtd: MTD device structure
 * @chip: NAND chip structure
 *
 * Wait for command done. This applies to erase and program only. Erase can
 * take up to 400ms and program up to 20ms according to general NAND and
 * SmartMedia specs.
 */
static int nand_rda_wait(struct mtd_info *mtd, struct nand_chip *chip)
{
#ifndef NAND_IRQ_POLL
	struct rda_nand_info *info = chip->priv;
#endif
	int status;
	unsigned long timeo = jiffies;
	int state = chip->state;

	if (state == FL_ERASING) {
		timeo += (HZ * 400) / 1000;
	} else {
		timeo += (HZ * 20) / 1000;
	}

	/*
	 * Apply this short delay always to ensure that we do wait tWB in any
	 * case on any machine.
	 */
	ndelay(100);

	chip->cmdfunc(mtd, NAND_CMD_STATUS, -1, -1);

	while (time_before(jiffies, timeo)) {
		if (chip->dev_ready) {
			if (chip->dev_ready(mtd)) {
				break;
			}
		} else {
			if (chip->read_byte(mtd) & NAND_STATUS_READY) {
				break;
			}
		}
		cond_resched();
	}

	status = (int)chip->read_byte(mtd);
#ifndef NAND_IRQ_POLL
	/* Check if there are any errors be reported by interrupt. */
	if (info->nand_status) {
		status |= NAND_STATUS_FAIL;
	}
#endif

	return status;
}

#ifdef CONFIG_MTD_NAND_RDA_DMA
/*
 * nand_rda_dma_cb: callback on the completion of dma transfer
 * @ch: logical channel
 * @data: pointer to completion data structure
 */
static void nand_rda_dma_cb(u8 ch, void *data)
{
	complete((struct completion *)data);
}
#endif /* CONFIG_MTD_NAND_RDA_DMA */

static int rda_nand_write_page(struct mtd_info *mtd, struct nand_chip *chip,
			const uint8_t *buf, int oob_required)
{
	/* upper layer(nand_write_page  in nand_base.c)
	 * will send SEQIN before write page is called,
	 * and will send PAGEPROG after this write page function is called
	 * */
	/* Nand controller will do ECC automatically, no sw control needed
	 * */
	rda_dbg_nand("%s \n", __func__);
	chip->write_buf(mtd, buf, mtd->writesize);
	if (oob_required)
		chip->write_buf(mtd, chip->oob_poi, mtd->oobsize);
	/* how to return status of write_page? we won't be able to return
	 * any status here as no staus avaiable unitl PAGEPROG done,
	 * but wait_func will return the status of write to upperlayer
	 * */
	return 0;
}
static int rda_nand_read_page(struct mtd_info *mtd, struct nand_chip *chip,
				uint8_t *buf, int oob_required, int page)
{

	struct rda_nand_info *info = chip->priv;

	/* Upperlayer(nand_do_read_ops in nand_base.c) will send READ0
	 * before this function got called, so we have status before this
	 * */
	/* Nand controller will do ECC automatically, no sw control needed
	 * */
	rda_dbg_nand("%s for page %d\n", __func__, page);
	chip->read_buf(mtd, buf, mtd->writesize);
	if (oob_required)
		chip->read_buf(mtd, chip->oob_poi, mtd->oobsize);
	return info->ecc_bits;

}

static int __init rda_nand_probe(struct platform_device *pdev)
{
	struct rda_nand_info *info;
	struct mtd_info *mtd;
	struct nand_chip *nand_chip;
	struct resource *mem;
	int res = 0;
	int irq, i = 0;

	mem = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!mem) {
		dev_err(&pdev->dev, "can not get resource mem\n");
		return -ENXIO;
	}

	irq = platform_get_irq(pdev, 0);
	if (irq < 0) {
		return irq;
	}

	/* Allocate memory for the device structure (and zero it) */
	info = kzalloc(sizeof(struct rda_nand_info), GFP_KERNEL);
	if (!info) {
		dev_err(&pdev->dev, "failed to allocate device structure.\n");
		return -ENOMEM;
	}
#ifdef CONFIG_MTD_NAND_RDA_DMA
	info->phy_addr = mem->start;
#endif

	info->databuf = kmalloc((SZ_16K + SZ_1K), GFP_KERNEL);
	if (!info->databuf) {
		dev_err(&pdev->dev, "failed to allocate databuf\n");
		kfree(info);
		return -ENOMEM;
	}

	info->base = ioremap(mem->start, resource_size(mem));
	if (info->base == NULL) {
		dev_err(&pdev->dev, "ioremap failed\n");
		res = -EIO;
		goto err_nand_ioremap;
	}
	info->reg_base = info->base + NANDFC_REG_OFFSET;
	info->data_base = info->base + NANDFC_DATA_OFFSET;
#ifndef NAND_IRQ_POLL
	init_completion(&info->nand_comp);
	res = request_irq(irq, nand_rda_irq_handler,
			  IRQF_DISABLED | IRQF_ONESHOT, "nand_rda",
			  (void *)info);
	if (res < 0) {
		dev_err(&pdev->dev, "%s: request irq fail\n", __func__);
		res = -EPERM;
		goto err_nand_irq;
	}

	info->nand_status = 0;
#endif /* NAND_IRQ_POLL */

	mtd = &info->mtd;
	nand_chip = &info->nand_chip;
	info->plat_data = pdev->dev.platform_data;
	info->dev = &pdev->dev;

	info->master_clk = clk_get(NULL, RDA_CLK_APB2);
	if (!info->master_clk) {
		dev_err(&pdev->dev, "no handler of clock\n");
		res = -EINVAL;
		goto err_nand_get_clk;
	}
	info->clk = info->plat_data->max_clock;

	nand_chip->priv = info;	/* link the private data structures */
	mtd->priv = nand_chip;
	mtd->owner = THIS_MODULE;

	/* Set address of NAND IO lines */
	nand_chip->IO_ADDR_R = info->data_base;
	nand_chip->IO_ADDR_W = info->data_base;
	nand_chip->cmd_ctrl = nand_rda_hwcontrol;
	nand_chip->cmdfunc = nand_rda_cmdfunc;
	nand_chip->dev_ready = nand_rda_dev_ready;
	nand_chip->init_size = nand_rda_init_size;
#ifdef CONFIG_MTD_NAND_RDA_DMA
	init_completion(&info->comp);
	rda_request_dma(0, "rda-dma", nand_rda_dma_cb, &info->comp,
			&info->dma_ch);
#endif /* CONFIG_MTD_NAND_RDA_DMA */

	nand_chip->chip_delay = 20;	/* 20us command delay time */
	nand_chip->read_buf = nand_rda_read_buf;
	nand_chip->write_buf = nand_rda_write_buf;
	nand_chip->read_byte = nand_rda_read_byte;
	nand_chip->read_word = nand_rda_read_word;
	nand_chip->waitfunc = nand_rda_wait;
	nand_chip->select_chip = nand_rda_select_chip;
	/* we do use flash based bad block table, created by u-boot */
	nand_chip->bbt_options = NAND_BBT_USE_FLASH;

	info->write_ptr = 0;
	info->read_ptr = 0;
	info->index = 0;

	info->read_retry_cmd_bak = 0;
	info->read_retry_error = 0;
	info->read_retry_time = 0;
	info->nand_id[0] = 0;
	info->nand_id[1] = 0;

	platform_set_drvdata(pdev, info);

#ifndef NAND_IRQ_POLL
	hal_irq_enable(info);
#endif /* NAND_IRQ_POLL */

	/* first scan to find the device and get the page size */
	if (nand_scan_ident(mtd, 4, NULL)) {
		res = -ENXIO;
		goto err_scan_ident;
	}

	nand_rda_init(info);

	if (!nand_chip->ecc.layout && (nand_chip->ecc.mode != NAND_ECC_SOFT_BCH)) {
		switch (info->type) {
		case NAND_TYPE_2K:
			nand_chip->ecc.layout = &rda_nand_oob_2k;
			break;
		case NAND_TYPE_4K:
			nand_chip->ecc.layout = &rda_nand_oob_4k;
			break;
		case NAND_TYPE_8K:
			nand_chip->ecc.layout = &rda_nand_oob_8k;
			break;
		case NAND_TYPE_16K:
			nand_chip->ecc.layout = &rda_nand_oob_16k;
			break;
		case NAND_TYPE_3K:
			nand_chip->ecc.layout = &rda_nand_oob_4k;
			break;
		case NAND_TYPE_7K:
			nand_chip->ecc.layout = &rda_nand_oob_8k;
			break;
		case NAND_TYPE_14K:
			nand_chip->ecc.layout = &rda_nand_oob_16k;
			break;
		default:
			pr_err("error oob size: %d \n", info->oob_size);
		}

		nand_chip->ecc.layout->oobfree[0].length = info->oob_size - 2;
		for(i=1; i<MTD_MAX_OOBFREE_ENTRIES; i++)
			nand_chip->ecc.layout->oobfree[i].length = 0;
	}

	nand_chip->ecc.mode = NAND_ECC_HW;
	nand_chip->ecc.size = mtd->writesize;
	nand_chip->ecc.bytes = nand_get_ecc_bytes(info);
	nand_chip->ecc.strength = nand_get_ecc_strength(info) * 4/5;
	/* hardware just return the flip bits for last message
	 * so we have to lower our ecc strength here
	 */
	nand_chip->ecc.read_page = rda_nand_read_page;
	nand_chip->ecc.write_page = rda_nand_write_page;

	/* second phase scan */
	if (nand_scan_tail(mtd)) {
		res = -ENXIO;
		goto err_scan_tail;
	}

	mtd->name = "rda_nand";
	res = mtd_device_parse_register(&info->mtd, NULL, NULL,
					partition_info,
					ARRAY_SIZE(partition_info));
	if (res) {
		goto err_parse_register;
	}

	dev_info(&pdev->dev, "rda_nand initialized\n");
	nand_rda_create_proc(info);

	return 0;

err_parse_register:
	nand_release(mtd);

err_scan_tail:
#ifndef NAND_IRQ_POLL
	hal_irq_disable(info);
#endif
err_scan_ident:

	//rda_nand_disable(host);
	platform_set_drvdata(pdev, NULL);

#ifdef CONFIG_MTD_NAND_RDA_DMA
	rda_free_dma(info->dma_ch);
#endif /* CONFIG_MTD_NAND_RDA_DMA */

	clk_put(info->master_clk);

err_nand_get_clk:
#ifndef NAND_IRQ_POLL
	free_irq(irq, (void *)info);

err_nand_irq:
#endif /* NAND_IRQ_POLL */
	iounmap(info->base);

err_nand_ioremap:
#ifndef CONFIG_MTD_NAND_RDA_DMA
	if (info->databuf) {
		kfree(info->databuf);
	}
#endif /* CONFIG_MTD_NAND_RDA_DMA */

	kfree(info);

	return res;
}

/*
 * Remove a NAND device.
 */
static int __exit rda_nand_remove(struct platform_device *pdev)
{
	struct rda_nand_info *info = platform_get_drvdata(pdev);
	struct mtd_info *mtd = &info->mtd;
#ifndef NAND_IRQ_POLL
	int irq;
#endif

	nand_rda_delete_proc();
#ifndef NAND_IRQ_POLL
	hal_irq_disable(info);
#endif

	nand_release(mtd);

	//rda_nand_disable(host);
#ifdef CONFIG_MTD_NAND_RDA_DMA
	rda_free_dma(info->dma_ch);
#endif /* CONFIG_MTD_NAND_RDA_DMA */

#ifndef NAND_IRQ_POLL
	irq = platform_get_irq(pdev, 0);
	free_irq(irq, (void *)info);
#endif /* NAND_IRQ_POLL */

	clk_put(info->master_clk);

	iounmap(info->base);

#ifndef CONFIG_MTD_NAND_RDA_DMA
	if (info->databuf) {
		kfree(info->databuf);
	}
#endif /* CONFIG_MTD_NAND_RDA_DMA */

	kfree(info);

	return 0;
}

static struct platform_driver rda_nand_driver = {
	.remove = __exit_p(rda_nand_remove),
	.driver = {
		   .name = RDA_NAND_DRV_NAME,
		   .owner = THIS_MODULE,
		   },
};

static int __init rda_nand_init(void)
{
	return platform_driver_probe(&rda_nand_driver, rda_nand_probe);
}

static void __exit rda_nand_exit(void)
{
	platform_driver_unregister(&rda_nand_driver);
}

module_init(rda_nand_init);
module_exit(rda_nand_exit);

MODULE_DESCRIPTION("RDA NAND Driver");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:rda_nand");

