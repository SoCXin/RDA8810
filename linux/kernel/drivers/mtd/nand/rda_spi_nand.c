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

#include <mach/hardware.h>
#include <mach/irqs.h>
#include <mach/ifc.h>
#include <mach/timex.h>
#include <mach/rda_clk_name.h>
#include <plat/devices.h>
#include <plat/reg_ifc.h>
#include <plat/reg_nand.h>
#include <plat/rda_debug.h>

#ifdef CONFIG_MTD_NAND_RDA_DMA /*not working*/
#include <plat/dma.h>
#endif /* CONFIG_MTD_NAND_RDA_DMA */

//#define NAND_DEBUG
//#define NAND_DMA_POLLING
#define READ_PAGE_DATA_WITH_SW_MODE  /* verified */
//#define READ_PAGE_BY_MAPPING    /* verified */

#ifdef READ_PAGE_DATA_WITH_SW_MODE
#define SETUP_DELAY_TUNING
#endif

#define SPI_NAND_STATISTIC /*TODO - move to upper layer to be reused.*/

/*******************************************************************/
/*******************spi flash MACRO define******************************/
#define SPI_NAND_READ_DATA_BUFFER_BASE 0xc0000000
#define SPI_NAND_WRITE_DATA_BUFFER_BASE (RDA_SPIFLASH_BASE + 0x8)

/*******************spi controller register define**************************/
static u32 RDA_SPIFLASH_BASE = 0;
#define SPI_FLASH_REG_BASE RDA_SPIFLASH_BASE
#define CMD_ADDR_OFFSET 0x0
#define MODE_BLOCKSIZE_OFFSET 0x4
#define DATA_FIFO_OFFSET 0x8
#define STATUS_OFFSET 0xc
#define READBACK_REG_OFFSET 0x10
#define FLASH_CONFIG_OFFSET 0x14
#define FIFO_CTRL_OFFSET 0x18
#define DUAL_SPI_OFFSET 0x1c
#define READ_CMD_OFFSET 0x20
#define NAND_SPI_CONFIG_OFFSET 0x24
#define NAND_SPI_CONFIG2_OFFSET 0x28

#define SPI_CMD_ADDR (SPI_FLASH_REG_BASE+CMD_ADDR_OFFSET)
#define SPI_BLOCK_SIZE (SPI_FLASH_REG_BASE+MODE_BLOCKSIZE_OFFSET)
#define SPI_DATA_FIFO (SPI_FLASH_REG_BASE+DATA_FIFO_OFFSET)
#define SPI_STATUS (SPI_FLASH_REG_BASE+STATUS_OFFSET)
#define SPI_READ_BACK (SPI_FLASH_REG_BASE+READBACK_REG_OFFSET)
#define SPI_CONFIG (SPI_FLASH_REG_BASE+FLASH_CONFIG_OFFSET)
#define SPI_FIFO_CTRL (SPI_FLASH_REG_BASE+FIFO_CTRL_OFFSET)
#define SPI_CS_SIZE (SPI_FLASH_REG_BASE+DUAL_SPI_OFFSET)
#define SPI_READ_CMD (SPI_FLASH_REG_BASE+READ_CMD_OFFSET)
#define SPI_NAND_CMD (SPI_FLASH_REG_BASE+NAND_SPI_CONFIG_OFFSET)
#define SPI_NAND_CMD2 (SPI_FLASH_REG_BASE+NAND_SPI_CONFIG2_OFFSET)

/*******************spi flash command define**************************/
#define OPCODE_WRITE_ENABLE 0x06
#define OPCODE_WRITE_DISABLE 0x04
#define OPCODE_GET_FEATURE 0x0f
#define OPCODE_SET_FEATURE 0x1f
#define OPCODE_PAGE_READ_2_CACHE 0x13
#define OPCODE_READ_FROM_CACHE  0x03
#define OPCODE_READ_FROM_CACHEX4 0x6b
#define OPCODE_READ_FROM_CACHE_QUADIO 0xeb
#define OPCODE_READ_ID 0x9f
#define OPCODE_PROGRAM_LOAD_RANDOM_DATA 0x84
#define OPCODE_PROGRAM_LOAD_RANDOM_DATAX4 0xc4
#define OPCODE_PROGRAM_LOAD_RANDOM_DATA_QUADIO 0x72
#define OPCODE_PROGRAM_EXECUTE 0x10
#define OPCODE_BLOCK_ERASE 0xd8
#define OPCODE_FLASH_RESET 0Xff

//spi nand state
#define NANDFC_SPI_ECCS1 (1 << 5)
#define NANDFC_SPI_ECCS0 (1 << 4)
#define NANDFC_SPI_PROG_FAIL (1 << 3)
#define NANDFC_SPI_ERASE_FAIL (1 << 2)
#define NANDFC_SPI_OPERATE_ERROR (1 << 0)
typedef enum {
	SPINAND_TYPE_SPI2K = 16,	// SPI, 2048+64, 2Gb
	SPINAND_TYPE_SPI4K = 17,	// SPI, 4096+128, 4Gb
	SPINAND_TYPE_INVALID = 0xFF,
} SPINAND_FLASH_TYPE;

/*******************MACRO define***********************************/
#define REG_READ_UINT32( _reg_ )	(*(volatile u32 *)(_reg_))
#define REG_WRITE_UINT32( _reg_, _val_) \
		((*(volatile u32*)(_reg_)) = (u32)(_val_))

/*******************spi nand  global variables define **********************/
#define MAX_OOB_SIZE 256
static u8 g_spi_flash_oob_buffer[MAX_OOB_SIZE];

/*******************************************************************/

#define NAND_CMD_TIMEOUT_MS    ( 2000 )
#define NAND_DMA_TIMEOUT_MS    ( 100 )

#define PROCNAME "driver/nand"


/*
 * Default mtd partitions, used when mtdparts= not present in cmdline
 */
static struct mtd_partition partition_info[] = {
	{
	 .name = "reserved",
	 .offset = 0,.size = 24 * SZ_1M},
	{
	 .name = "vendor",
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

struct rda_nand_info {
	struct nand_chip nand_chip;
	struct mtd_info mtd;

	struct rda_nand_device_data *plat_data;
	struct device *dev;
	SPINAND_FLASH_TYPE type;

	int cmd;
	int col_addr;
	int page_addr;
	int read_ptr;           // in read_buf function for data or oob
	u32 byte_buf[4];	// for read_id, status, etc
	int index;
	int write_ptr;

	void __iomem *base;
	void __iomem *reg_base;
	void __iomem *data_base;
#ifdef CONFIG_MTD_NAND_RDA_DMA
	dma_addr_t phy_addr;
	/* For waiting for dma */
	struct completion comp;
	u8 dma_ch;
#else
	u8 *databuf;
#endif /* CONFIG_MTD_NAND_RDA_DMA */

#ifndef NAND_IRQ_POLL
	u32 nand_status;
	struct completion nand_comp;
#endif /* NAND_IRQ_POLL */

	struct clk *master_clk;
	unsigned long clk;
	int cmd_flag;
#ifdef SETUP_DELAY_TUNING
	unsigned long setup_delay_min;
	unsigned long setup_delay_max;
#endif

#ifdef SPI_NAND_STATISTIC
	u64 bytes_read;
	u64 bytes_write;
	u64 read_ecc_error_pages;
	u64 read_ecc_flip_pages;
#endif
};

static struct nand_ecclayout spi_nand_oob_64 = {
	.eccbytes = 16,
	.eccpos = {
		   12, 13, 14, 15, 28, 29, 30, 31,
		   44, 45, 46, 47, 60, 61, 62, 63},
	.oobfree = {
		    {.offset = 2,
		     .length = 10},
		    {.offset = 16,
		     .length = 12},
		    {.offset = 32,
		     .length = 12},
		    {.offset = 48,
		     .length = 12}}
};

static struct nand_ecclayout spi_nand_oob_128 = {
	.eccbytes = 0,
	.eccpos = {},
	.oobfree = {
		    {.offset = 2,
		     .length = 126}}
};

static struct nand_ecclayout spi_nand_oob_256 = {
	.eccbytes = 128,
	.eccpos = {128, 129, 130,
	131, 132, 133, 134, 135, 136, 137, 138, 139, 140,
	141, 142, 143, 144, 145, 146, 147, 148, 149, 150,
	151, 152, 153, 154, 155, 156, 157, 158, 159, 160,
	161, 162, 163, 164, 165, 166, 167, 168, 169, 170,
	171, 172, 173, 174, 175, 176, 177, 178, 179, 180,
	181, 182, 183, 184, 185, 186, 187, 188, 189, 190,
	191, 192, 193, 194, 195, 196, 197, 198, 199, 200,
	201, 202, 203, 204, 205, 206, 207, 208, 209, 210,
	211, 212, 213, 214, 215, 216, 217, 218, 219, 220,
	221, 222, 223, 224, 225, 226, 227, 228, 229, 230,
	231, 232, 233, 234, 235, 236, 237, 238, 239, 240,
	241, 242, 243, 244, 245, 246, 247, 248, 249, 250,
	251, 252, 253, 254, 255},
	.oobfree = {
		    {.offset = 2,
		     .length = 126}}
};

/* Generic flash bbt decriptors
*/
static uint8_t bbt_pattern[] = { 'B', 'b', 't', '0' };
static uint8_t mirror_pattern[] = { '1', 't', 'b', 'B' };

static struct nand_bbt_descr spi_bbt_main_descr = {
	.options = NAND_BBT_LASTBLOCK | NAND_BBT_CREATE | NAND_BBT_WRITE
	    | NAND_BBT_2BIT | NAND_BBT_VERSION | NAND_BBT_PERCHIP,
	.offs = 8,
	.len = 4,
	.veroffs = 16,
	.maxblocks = 4,
	.pattern = bbt_pattern
};

static struct nand_bbt_descr spi_bbt_mirror_descr = {
	.options = NAND_BBT_LASTBLOCK | NAND_BBT_CREATE | NAND_BBT_WRITE
	    | NAND_BBT_2BIT | NAND_BBT_VERSION | NAND_BBT_PERCHIP,
	.offs = 8,
	.len = 4,
	.veroffs = 16,
	.maxblocks = 4,
	.pattern = mirror_pattern
};

void wait_spi_busy(void)
{				/*spi is busy,wait until idle */
	u32 data_tmp_32;

	data_tmp_32 = REG_READ_UINT32(SPI_STATUS);
	while (data_tmp_32 & 0x1) {
		data_tmp_32 = REG_READ_UINT32(SPI_STATUS);
	}
}

void wait_spi_tx_fifo_empty(void)
{				/*spi tx fifo not empty, wait until empty */
	u32 data_tmp_32;

	data_tmp_32 = REG_READ_UINT32(SPI_STATUS);
	while (!(data_tmp_32 & 0x2)) {
		data_tmp_32 = REG_READ_UINT32(SPI_STATUS);
	}
}

void wait_spi_rx_fifo_empty(void)
{				/*spi rx fifo not empty, wait until empty */
	u32 data_tmp_32;

	data_tmp_32 = REG_READ_UINT32(SPI_STATUS);
	while (!(data_tmp_32 & 0x8)) {
		data_tmp_32 = REG_READ_UINT32(SPI_STATUS);
	}
}

void spi_tx_fifo_clear(void)
{				/*clear spi tx fifo */
	REG_WRITE_UINT32(SPI_FIFO_CTRL, 0x2);
	wait_spi_tx_fifo_empty();
}

void spi_rx_fifo_clear(void)
{				/*clear spi rx fifo */
	REG_WRITE_UINT32(SPI_FIFO_CTRL, 0x1);
	wait_spi_rx_fifo_empty();
}

static void push_fifo_data(u8 data_array[], u32 data_cnt, bool quard_flag,
			   bool clr_flag)
{
	u32 data_tmp_32;
	u16 i = 0;

	if (clr_flag)
		spi_tx_fifo_clear();

	/*if tx fifo full, wait */
	data_tmp_32 = REG_READ_UINT32(SPI_STATUS);
	while (data_tmp_32 & 0x4)
		data_tmp_32 = REG_READ_UINT32(SPI_STATUS);

	/*put data into fifo */
	for (i = 0; i < data_cnt; i++) {
		data_tmp_32 = (u32) data_array[i];
		if (quard_flag)
			data_tmp_32 = data_tmp_32 | 0x100;
		REG_WRITE_UINT32(SPI_DATA_FIFO, data_tmp_32);
	}
}

/********************************************************************/
/*	opmode = 0:	normal operation																		*/
/*	opmode = 1:	extended serial read																*/
/*	opmode = 2:	extended quard read																	*/
/*	opmode = 3:	extended write		(OPCODE_WRITE_ENABLE, flash_addr, opmode, 0);																*/
/********************************************************************/
static void start_flash_operation(u8 cmd, u32 addr, u8 opmode, u16 blocksize)
{
	u32 data_tmp_32;

	wait_spi_busy();

	if (blocksize != 0) {
		data_tmp_32 = REG_READ_UINT32(SPI_BLOCK_SIZE);
		data_tmp_32 = (data_tmp_32 & (~0x1ff00)) | (blocksize << 8);
		REG_WRITE_UINT32(SPI_BLOCK_SIZE, data_tmp_32);
		wait_spi_busy();
	}

	if (opmode == 0) {
		data_tmp_32 = ((addr << 8) & 0xffffff00) | cmd;
		REG_WRITE_UINT32(SPI_CMD_ADDR, data_tmp_32);
	} else if (opmode == 1) {
		data_tmp_32 = (((cmd << 8) | 0x10000) & 0x000fff00);
		REG_WRITE_UINT32(SPI_CMD_ADDR, data_tmp_32);
	} else if (opmode == 2) {
		data_tmp_32 = (((cmd << 8) | 0x30000) & 0x000fff00);
		REG_WRITE_UINT32(SPI_CMD_ADDR, data_tmp_32);
	} else if (opmode == 3) {
		data_tmp_32 = ((cmd << 8) & 0x000fff00);
		REG_WRITE_UINT32(SPI_CMD_ADDR, data_tmp_32);
	} else {
		data_tmp_32 = ((cmd << 8) & 0x000fff00);
		REG_WRITE_UINT32(SPI_CMD_ADDR, data_tmp_32);
	}
}

u8 get_flash_feature(u32 flash_addr)
{
	u32 data_tmp_32;
	u8 opmode;
	u8 data_array[2];
	u8 data_size;
	u8 quard_flag;

	quard_flag = false;
	opmode = 1;
	data_array[0] = 0xb0;
	data_size = 1;

	wait_spi_busy();
	/*clear spi rx fifo */
	spi_rx_fifo_clear();

	/*put addr into spi tx fifo */
	push_fifo_data(data_array, data_size, quard_flag, true);
	/*send command */
	start_flash_operation(OPCODE_GET_FEATURE, flash_addr, opmode, 1);
	wait_spi_busy();

	/*wait until rx fifo not empty */
	data_tmp_32 = REG_READ_UINT32(SPI_STATUS);
	while (data_tmp_32 & 0x8)
		data_tmp_32 = REG_READ_UINT32(SPI_STATUS);

	return (REG_READ_UINT32(SPI_READ_BACK) & 0xff);
}

void set_flash_feature(u8 data)
{
	u8 opmode;
	u8 data_array[2];
	u8 data_size;
	bool quard_flag;

	quard_flag = false;
	opmode = 3;
	data_array[1] = data;
	data_array[0] = 0xb0;
	data_size = 2;

	wait_spi_busy();
	push_fifo_data(data_array, data_size, quard_flag, true);
	start_flash_operation(OPCODE_SET_FEATURE, 0, opmode, 0);
	wait_spi_busy();
	wait_spi_tx_fifo_empty();
}

static bool spi_nand_flash_quad_enable = false;
void enable_spi_nand_flash_quad_mode(void)
{
	u8 data;

	data = get_flash_feature(0);
	data = data | 0x1;
	set_flash_feature(data);
	spi_nand_flash_quad_enable = true;
}

void disable_spi_nand_flash_quad_mode(void)
{
	u8 data;

	data = get_flash_feature(0);
	data = data & 0xfe;
	set_flash_feature(data);
	spi_nand_flash_quad_enable = false;
}

static bool get_spi_nand_flash_mode(void)
{
	u8 data;

	data = get_flash_feature(0);
	if(data & 0x1)
		return true;
	else
		return false;
}

u8 get_flash_status(u32 flash_addr)
{
	u32 data_tmp_32;
	u8 opmode;
	u8 data_array[2];
	u8 data_size;
	bool quard_flag;

	quard_flag = false;
	opmode = 1;
	data_array[0] = 0xc0;
	data_size = 1;

	wait_spi_busy();
	spi_rx_fifo_clear();

	wait_spi_busy();
	push_fifo_data(data_array, data_size, quard_flag, true);
	start_flash_operation(OPCODE_GET_FEATURE, flash_addr, opmode, 1);
	wait_spi_busy();

	data_tmp_32 = REG_READ_UINT32(SPI_STATUS);
	while (data_tmp_32 & 0x8)
		data_tmp_32 = REG_READ_UINT32(SPI_STATUS);

	return (REG_READ_UINT32(SPI_READ_BACK) & 0xff);
}

void disable_flash_protection(u32 flash_addr)
{
	u8 opmode;
	u8 data_array[2];
	u8 data_size;

	opmode = 3;
	data_array[1] = 0;
	data_array[0] = 0xa0;
	data_size = 2;

	wait_spi_busy();
	push_fifo_data(data_array, data_size, false, true);
	start_flash_operation(OPCODE_SET_FEATURE, flash_addr, opmode, 0);
	wait_spi_busy();
	wait_spi_tx_fifo_empty();
}

void spi_nand_flash_operate_mode_setting(u8 mode)
{
	u32 data_tmp_32;

	data_tmp_32 = REG_READ_UINT32(SPI_NAND_CMD);
	if (mode == 1)		// 1 command-1 addr- 1 data
	{
		data_tmp_32 |= OPCODE_READ_FROM_CACHE << 24;
	} else if (mode == 2)	// 1 command- 1 addr- 4 data
	{
		data_tmp_32 |= OPCODE_READ_FROM_CACHEX4 << 24;
	} else if (mode == 4)	// 1 command- 4 addr- 4 data
	{
		data_tmp_32 |= OPCODE_READ_FROM_CACHE_QUADIO << 24;
	} else			// default:1 command-1 addr- 1 data
	{
		data_tmp_32 |= OPCODE_READ_FROM_CACHE << 24;
	}

	REG_WRITE_UINT32(SPI_NAND_CMD, data_tmp_32);
}

u16 get_flash_ID(u32 flash_addr)
{
	u32 data_tmp_32;
	u8 opmode;
	bool quard_flag;
	u8 data_array[2];
	u8 data_size;
	u8 manufacturerID, device_memory_type_ID;

	opmode = 1;
	data_array[0] = 0x00;
	data_size = 1;
	quard_flag = false;

	wait_spi_busy();
	spi_rx_fifo_clear();

	push_fifo_data(data_array, data_size, quard_flag, true);
	start_flash_operation(OPCODE_READ_ID, flash_addr, opmode, 2);
	wait_spi_busy();

	data_tmp_32 = REG_READ_UINT32(SPI_STATUS);
	while (data_tmp_32 & 0x8)
		data_tmp_32 = REG_READ_UINT32(SPI_STATUS);

	manufacturerID = (u8) (REG_READ_UINT32(SPI_READ_BACK) & 0xff);
	device_memory_type_ID = (u8) (REG_READ_UINT32(SPI_READ_BACK) & 0xff);

	return (manufacturerID | (device_memory_type_ID << 8));
}

void nand_spi_init(void)
{
	spi_nand_flash_quad_enable = get_spi_nand_flash_mode();
}

static void nand_spi_fifo_word_read_set(bool enable)
{
	u32 data_tmp_32;

	wait_spi_busy();
	data_tmp_32 = REG_READ_UINT32(SPI_CONFIG);
	if(enable)
		data_tmp_32 = (data_tmp_32 & (~(3 << 17))) |(2 << 17);
	else
		data_tmp_32 = data_tmp_32 & (~(3 << 17));
	REG_WRITE_UINT32(SPI_CONFIG, data_tmp_32);
	wait_spi_busy();
}

static u8 spi_nand_rda_read_byte(struct mtd_info *mtd)
{
	u8 ret;
	struct nand_chip *chip = mtd->priv;
	struct rda_nand_info *info = chip->priv;
	u8 *ptr = (u8 *) info->byte_buf;

	ret = ptr[info->index];
	info->index++;
	return ret;
}

static u16 spi_nand_rda_read_word(struct mtd_info *mtd)
{
	u16 ret;
	struct nand_chip *chip = mtd->priv;
	struct rda_nand_info *info = chip->priv;
	u16 *ptr = (u16 *) info->byte_buf;

	ret = ptr[info->index];
	info->index += 2;
	return ret;
}

static inline void __spi_nand_rda_copyfrom_iomem(void *dst_buf, void *io_mem,
						 size_t size)
{
	memcpy(dst_buf, io_mem, size);
	return;
}
#ifdef SETUP_DELAY_TUNING
static void spi_nand_flash_page_read2cache_busywait(u32 page_addr)
{
	u8 opmode;
	u8 data_array[5];
	u8 data_size;

	opmode = 3;
	data_array[2] = (u8) (page_addr & 0xff);
	data_array[1] = (u8) ((page_addr >> 8) & 0xff);
	data_array[0] = (u8) ((page_addr >> 16) & 0xff);
	data_size = 3;

	wait_spi_busy();
	push_fifo_data(data_array, data_size, false, true);
	start_flash_operation(OPCODE_PAGE_READ_2_CACHE, 0, opmode, 0);
	//wait_spi_busy();


	/*
	 * According to Spec, time to data ready in cache will be max 120us
	 * sleep here to give other thread some chance to run
	 * */
	while ((get_flash_status(0)& 0x1) != 0)
		;
}

#ifdef CONFIG_CPU_V7
/*TODO move these performance counter related function to other header file
 * so it can be easily reused
 * */
#include <rda/tgt_ap_clock_config.h>

enum CYCLE_COUNTER_STATE {
	CYCLE_COUNTER_STATE_DISABLED,
	CYCLE_COUNTER_STATE_ENABLED_64,
	CYCLE_COUNTER_STATE_ENABLED_1
};

/* return previous cycle counter status,
 * if not enabled, enable it with 64 divider on
 * */
static enum CYCLE_COUNTER_STATE raw_armv7_perf_cycles_init(void)
{
	int val;
	enum CYCLE_COUNTER_STATE status;
	int enable_cycle = 0x80000000;

	asm volatile("mrc p15, 0, %0, c9, c12, 0" : "=r"(val));
	if ((val & 0x1) == 0)
		status = CYCLE_COUNTER_STATE_DISABLED;
	else if ((val & 0x8) == 0)
		status = CYCLE_COUNTER_STATE_ENABLED_1;
	else
		status = CYCLE_COUNTER_STATE_ENABLED_64;

	if ((val & 0x1) == 0) {
		val |= 0x9;
		asm volatile("mcr p15, 0, %0, c9, c12, 0" : : "r"(val));
	}

	asm volatile("mcr p15, 0, %0, c9, c12, 1" : : "r"(enable_cycle));
	return status;
}
static void raw_armv7_perf_cycles_restore(enum CYCLE_COUNTER_STATE state)
{
	int val;

	if ( state == CYCLE_COUNTER_STATE_DISABLED){
		asm volatile("mrc p15, 0, %0, c9, c12, 0" : "=r"(val));
		val &= 0xFFFFFFFE;
		asm volatile("mcr p15, 0, %0, c9, c12, 0" : : "r"(val));
	}
}

static uint32_t armv7_perf_cycles(enum CYCLE_COUNTER_STATE state)
{
	int value;
	asm volatile("mrc p15, 0, %0, c9, c13, 0" : "=r" (value));
	if (state == CYCLE_COUNTER_STATE_ENABLED_1)
		value = value/64;
	return value;
}

#define CYCLES_64_TO_US(a)    (a*64/_TGT_AP_PLL_CPU_FREQ)
static unsigned long spi_nand_setup_delay_tuning(u32 page_addr, enum CYCLE_COUNTER_STATE state)
{
	int start, end ; /*cycles/64*/

	/*TODO should we disable IRQ to by more accurate? no need for now */
	start = armv7_perf_cycles(state);
	spi_nand_flash_page_read2cache_busywait(page_addr);
	end = armv7_perf_cycles(state);
	return CYCLES_64_TO_US((end-start));
}
static void spi_nand_do_setup_delay_tuning(struct rda_nand_info *info)
{

	int i;
	unsigned long delay_min, delay_max, delay;
	enum CYCLE_COUNTER_STATE state;
	u32 page_addr = 4096;

	state = raw_armv7_perf_cycles_init();
	delay_min = delay_max = spi_nand_setup_delay_tuning(page_addr, state);
	for (i = 0; i < 5; i++){
		page_addr += page_addr;
		delay = spi_nand_setup_delay_tuning(page_addr, state);
		if (delay > delay_max)
			delay_max = delay;
		if (delay < delay_min)
			delay_min = delay;
		pr_info("%s delay round %d : %lu US\n", __func__, i, delay);
	}
	raw_armv7_perf_cycles_restore(state);
	if (delay_max > 120)
		delay_max = 120;
	info->setup_delay_min = delay_min;
	info->setup_delay_max = delay_max;
	pr_info("%s delay_min is %lu us, delay_max is %lu US \n", __func__,
				delay_min, delay_max);
}

#else

static void spi_nand_do_setup_delay_tuning(struct rda_nand_info *info)
{
	/*according to spec, 120 is the max setup delay for read to cache...*/
	info->setup_delay_min = 80;
	info->setup_delay_max = 85;
}
#endif
#endif

static void spi_nand_flash_page_read2cache(struct rda_nand_info *info, u32 page_addr)
{
	u8 opmode;
	u8 data_array[5];
	u8 data_size;

	opmode = 3;
	data_array[2] = (u8) (page_addr & 0xff);
	data_array[1] = (u8) ((page_addr >> 8) & 0xff);
	data_array[0] = (u8) ((page_addr >> 16) & 0xff);
	data_size = 3;

	wait_spi_busy();
	push_fifo_data(data_array, data_size, false, true);
	start_flash_operation(OPCODE_PAGE_READ_2_CACHE, 0, opmode, 0);
	//wait_spi_busy();


	/*
	 * According to some spi nand Spec, time to data ready in cache will be max 120us
	 * sleep here to give other thread some chance to run
	 * */
#ifdef 	SETUP_DELAY_TUNING
	usleep_range(info->setup_delay_min, info->setup_delay_max);
#endif
	while ((get_flash_status(0)& 0x1) != 0)
		;
}

#define __SPI_NAND_CONTROLLER_RCV_FIFO_LEVEL__  64   //bytes,  16 words
#define __SPI_NAND_CONTROLLER_MAX_BURST_SIZE    64
static void spi_nand_flash_read_from_cache(u8 * buffer, u32 page_column_addr,
					   u16 length)
{
	u32 i, data_size;
	u8 opmode;
	u8 data_array[3];
	u32 data_count;
	u32 loop_div_count, loop_mod_count;
	bool quard_flag = false;
	u8 command = OPCODE_READ_FROM_CACHE;
	u32 temp_column_addr;


	opmode = 1;
	data_array[2] = 0;
	data_size = 3;

	if (length == 0)
		return ;

	wait_spi_busy();
	spi_rx_fifo_clear();
	wait_spi_busy();

	if (spi_nand_flash_quad_enable) {
		command = OPCODE_READ_FROM_CACHE_QUADIO;
		quard_flag = true;
		opmode = 0;
	}

	nand_spi_fifo_word_read_set(true);
	loop_div_count = length / __SPI_NAND_CONTROLLER_MAX_BURST_SIZE;
	loop_mod_count = length % __SPI_NAND_CONTROLLER_MAX_BURST_SIZE;

	if (1) {
		for (data_count = 0; data_count < loop_div_count; data_count++) {
			temp_column_addr = page_column_addr +
				data_count * __SPI_NAND_CONTROLLER_MAX_BURST_SIZE;
			data_array[1] = (u8) (temp_column_addr & 0xff);
			data_array[0] = ((temp_column_addr >> 8) & 0x0f);

			if (spi_nand_flash_quad_enable) {
				start_flash_operation(command, temp_column_addr, opmode,
						__SPI_NAND_CONTROLLER_MAX_BURST_SIZE);
			} else {
				/* TODO - what is this? */
				push_fifo_data(data_array, data_size, quard_flag, true);
				start_flash_operation(command, 0 ,
						opmode, __SPI_NAND_CONTROLLER_MAX_BURST_SIZE);
			}


			for (i = data_count *__SPI_NAND_CONTROLLER_MAX_BURST_SIZE;
				i < (data_count + 1) * __SPI_NAND_CONTROLLER_MAX_BURST_SIZE;
				      i += 4) {

				while (REG_READ_UINT32(SPI_STATUS)& 0x8)
					;

				*(u32 *)(buffer + i) =  REG_READ_UINT32(SPI_READ_BACK);
			}
		}

		if(loop_mod_count){
			temp_column_addr = page_column_addr + __SPI_NAND_CONTROLLER_MAX_BURST_SIZE* data_count;
			data_array[1] = (u8) (temp_column_addr & 0xff);
			data_array[0] = ((temp_column_addr >> 8) & 0x0f);
			wait_spi_busy();
			spi_rx_fifo_clear();
			wait_spi_busy();
			if (spi_nand_flash_quad_enable) {
				start_flash_operation(command, temp_column_addr, opmode, loop_mod_count);
			} else {
				push_fifo_data(data_array, data_size, quard_flag, true);
				start_flash_operation(command, 0, opmode, loop_mod_count);
			}


			for (i =__SPI_NAND_CONTROLLER_MAX_BURST_SIZE * data_count;
			       	i < __SPI_NAND_CONTROLLER_MAX_BURST_SIZE* data_count + loop_mod_count;
				     i += 4) {
				/* TODO - Potential of overwrite
				 * input buffer tail when it is not a multiple of 4
				 * */

				while (REG_READ_UINT32(SPI_STATUS)& 0x8)
					;

				* (u32*)(buffer + i) = REG_READ_UINT32(SPI_READ_BACK);
			}
		}
	}
	nand_spi_fifo_word_read_set(false);
}


/* NOTE this must be called after data is in chip cache,
 * oob is read into oob buffer
 * */
static int spi_nand_read_oob(struct mtd_info *mtd)
{
	struct nand_chip *chip = mtd->priv;
	struct rda_nand_info *info = chip->priv;
	u8* buf = g_spi_flash_oob_buffer;

	spi_nand_flash_read_from_cache(buf, mtd->writesize, mtd->oobsize);

	/*gigadevice flash hardware reserved block, add to BBT, 0xffe0000, 0x1ffc0000*/
	if (((0xffe0000 == info->page_addr * mtd->writesize) && (info->type == SPINAND_TYPE_SPI2K))
		|| ((0x1ffc0000 == info->page_addr * mtd->writesize) &&
			(info->type == SPINAND_TYPE_SPI4K))){
		memset((void *)buf, 0, mtd->oobsize);
		return 0;
	}
	return mtd->oobsize;
}



static int spi_nand_read_page_oob(struct mtd_info *mtd,  u32 page_addr)
{
	struct nand_chip *chip = mtd->priv;
	struct rda_nand_info *info = chip->priv;

	info->page_addr = page_addr;
	spi_nand_flash_page_read2cache(info, info->page_addr);
	return spi_nand_read_oob(mtd);
}


static int spi_nand_read_page_data(struct mtd_info *mtd, uint8_t *buf, int len)
{

	struct nand_chip *chip = mtd->priv;
	struct rda_nand_info *info = chip->priv;
	int oob_read_result;

	if (len > mtd->writesize)
		len = mtd->writesize;
#ifdef READ_PAGE_BY_MAPPING
	{
		u32 flash_addr =
		    SPI_NAND_READ_DATA_BUFFER_BASE + info->page_addr * mtd->writesize;
		void __iomem *flash_virt_addr;
		flash_virt_addr = ioremap(flash_addr, mtd->writesize);
		if (flash_virt_addr == NULL) {
			rda_dbg_nand("spi flash ioremap failed!!!!\n");
			return 0;
		}

		memcpy((void *)buf, (void *)(flash_virt_addr+info->col_addr), len);
		iounmap(flash_virt_addr);
		oob_read_result = spi_nand_read_oob(mtd);
		if(oob_read_result == 0)
			memset(buf, 0, len);
	}
#endif

#ifdef READ_PAGE_DATA_WITH_SW_MODE
	spi_nand_flash_page_read2cache(info, info->page_addr);
	spi_nand_flash_read_from_cache(buf, info->col_addr, len);
	oob_read_result = spi_nand_read_oob(mtd);
	if(oob_read_result == 0)
		memset(buf, 0, len);
#endif


#if 0 //def CONFIG_MTD_NAND_RDA_DMA
	struct rda_dma_chan_params dma_param;
	dma_addr_t phys_addr_src, phys_addr_dst;
	void *addr = (void *)buf;
	int ret = 0;
	struct page *p1;

	pr_info
	    ("spi_nand_rda_read_buf-dma, buf = %x, nand_ptr = %x, len = %d, ptr = %d\n",
	     (u32) buf, (u32) nand_ptr, len, info->read_ptr);

	/*
	 * Check if the address of buffer is an address allocated by vmalloc.
	 * If so, we don't directly use these address, it might be un-continous
	 * in physical address. We have to copy them via memcpy.
	 */
	if (addr >= high_memory) {
		if (((size_t) addr & PAGE_MASK) !=
		    ((size_t) (addr + len - 1) & PAGE_MASK)) {
			goto out_copy;
		}

		p1 = vmalloc_to_page(addr);
		if (!p1) {
			goto out_copy;
		}
		addr = page_address(p1) + ((size_t) addr & ~PAGE_MASK);
	}

	phys_addr_dst = dma_map_single(info->dev, addr, len, DMA_BIDIRECTIONAL);
	if (dma_mapping_error(info->dev, phys_addr_dst)) {
		dev_err(info->dev, "dst addr failed to dma_map_single\n");
		return;
	}

	phys_addr_src =
	    dma_map_single(info->dev, (void *)nand_ptr, len, DMA_BIDIRECTIONAL);
	if (dma_mapping_error(info->dev, phys_addr_src)) {
		dev_err(info->dev, "src addr failed to dma_map_single\n");
		return;
	}

	dma_param.src_addr = phys_addr_src;
	dma_param.dst_addr = phys_addr_dst;
	dma_param.xfer_size = len;
	dma_param.dma_mode = RDA_DMA_NOR_MODE;
#ifdef NAND_DMA_POLLING
	dma_param.enable_int = 0;
#else
	dma_param.enable_int = 1;
#endif /* NAND_DMA_POLLING */

	ret = rda_set_dma_params(info->dma_ch, &dma_param);
	if (ret < 0) {
		dev_err(info->dev, "failed to set parameter\n");
		dma_unmap_single(info->dev, phys_addr_src, len,
				 DMA_BIDIRECTIONAL);
		dma_unmap_single(info->dev, phys_addr_dst, len,
				 DMA_BIDIRECTIONAL);
		return;
	}

	init_completion(&info->comp);

	rda_start_dma(info->dma_ch);
#ifndef NAND_DMA_POLLING
	ret = wait_for_completion_timeout(&info->comp,
					  msecs_to_jiffies
					  (NAND_DMA_TIMEOUT_MS));
	if (ret <= 0) {
		dev_err(info->dev, "dma read timeout, ret = 0x%08x\n", ret);
		dma_unmap_single(info->dev, phys_addr_src, len,
				 DMA_BIDIRECTIONAL);
		dma_unmap_single(info->dev, phys_addr_dst, len,
				 DMA_BIDIRECTIONAL);
		return;
	}
#else
	rda_poll_dma(info->dma_ch);
	rda_stop_dma(info->dma_ch);
#endif /* #if 0 */

	/* Free the specified physical address */
	dma_unmap_single(info->dev, phys_addr_src, len, DMA_BIDIRECTIONAL);
	dma_unmap_single(info->dev, phys_addr_dst, len, DMA_BIDIRECTIONAL);
	info->read_ptr += len;

	pr_info("data read from middle buf:\n");
	rda_dump_buf((char *)nand_ptr, len);

	pr_info("data read to dst buf:\n");
	rda_dump_buf((char *)buf, len);

	return;

out_copy:
	__spi_nand_rda_copyfrom_iomem(buf, (void *)nand_ptr, len);
	info->read_ptr += len;
	return;

#endif /* CONFIG_MTD_NAND_RDA_DMA */

	rda_dbg_nand("spi_nand_rda_read_buf, buf = %x, len = %dn", (u32) buf, len);
	return len;
}
/*
 *read page data or read page oob
 * */
static void spi_nand_rda_read_buf(struct mtd_info *mtd, uint8_t * buf, int len)
{
	struct nand_chip *chip = mtd->priv;
	struct rda_nand_info *info = chip->priv;
	int length;

	if(info->read_ptr == 0) {
		/* read the page */
		length = spi_nand_read_page_data(mtd, buf, len);
		info->read_ptr += length;
	} else {
		length = len;
		if (length > mtd->oobsize)
			length = mtd->oobsize;
		memcpy(buf, g_spi_flash_oob_buffer, length);
	}

#ifdef SPI_NAND_STATISTIC
	info->bytes_read += length;
#endif
	return;
}

static void spi_nand_flash_write_data2cache_random(u8 * buffer,
						   u32 page_column_addr,
						   u32 length,
						   bool bug_mode)
{
	u8 opmode;
	u8 data_array[3];
	u8 data_size;
	bool quard_flag;
	u8 opcommand;

#ifdef NAND_DEBUG
	u8 temp_data_cache[200];
	u8 *temp_pointer = buffer;
#endif

	if(false == bug_mode){
		if (spi_nand_flash_quad_enable) {
			quard_flag = true;
			opcommand = OPCODE_PROGRAM_LOAD_RANDOM_DATA_QUADIO;
		} else {
			quard_flag = false;
			opcommand = OPCODE_PROGRAM_LOAD_RANDOM_DATA;
		}

		opmode = 3;
		data_array[1] = (u8) (page_column_addr & 0xff);
		data_array[0] = ((page_column_addr >> 8) & 0x0f);
		data_size = 2;
#ifdef NAND_DEBUG
		pr_info
		    ("spi_nand_flash_write_data2cache_random: data_addr[0] = %x, data_addr[1] = %x \n",
		     data_array[0], data_array[1]);
#endif
		wait_spi_busy();
		push_fifo_data(data_array, data_size, quard_flag, true);
		push_fifo_data(buffer, length, quard_flag, false);
		start_flash_operation(opcommand, 0, opmode, 0);
	}
	else{/*0x84 standard mode for spi controller bug*/
		wait_spi_busy();
		push_fifo_data(buffer, length, false, true);
		start_flash_operation(OPCODE_PROGRAM_LOAD_RANDOM_DATA, page_column_addr,
							0, 0);
	}

	wait_spi_busy();
	wait_spi_tx_fifo_empty();
	wait_spi_busy();

#ifdef NAND_DEBUG
	rda_dump_buf((char *)temp_pointer, length);
	spi_nand_flash_read_from_cache(&temp_data_cache[0], page_column_addr,
				       length);
	rda_dump_buf((char *)&temp_data_cache[0], length);
#endif
}

void spi_nand_flash_write_one_page_cache_random(u8 * buffer, u32 length,
						u32 write_pos)
{
	u32 i = 0, m, n;
	u8 *w_buffer = buffer;

#define SPI_NAND_OPERATE_LENGTH_ONCE  200

//	if (write_pos == 0) {
		m = length / SPI_NAND_OPERATE_LENGTH_ONCE;
		n = length % SPI_NAND_OPERATE_LENGTH_ONCE;

		if(n == 0){
			m = m - 1;
			n = SPI_NAND_OPERATE_LENGTH_ONCE;
		}

		if (m > 0) {
			for (i = 0; i < m; i++) {
				spi_nand_flash_write_data2cache_random(w_buffer
								       +
								       SPI_NAND_OPERATE_LENGTH_ONCE
								       * i,
								       SPI_NAND_OPERATE_LENGTH_ONCE
								       * i + write_pos,
								       SPI_NAND_OPERATE_LENGTH_ONCE,
								       false);
			}
		}

		if (n > 0)
			spi_nand_flash_write_data2cache_random(w_buffer +
							       SPI_NAND_OPERATE_LENGTH_ONCE
							       * i,
							       SPI_NAND_OPERATE_LENGTH_ONCE
							       * i + write_pos, n, true);
//	} else {
//		spi_nand_flash_write_data2cache_random(w_buffer, write_pos,
//						       length, true);
//	}
}

static int spi_nand_flash_program_execute(u32 block_page_addr)
{
	u32 data_tmp_32;
	u8 opmode;
	u8 data_array[5];
	u8 data_size;

	start_flash_operation(OPCODE_WRITE_ENABLE, 0, 0, 0);
	wait_spi_busy();

	opmode = 3;
	data_array[2] = (u8) (block_page_addr & 0xff);
	data_array[1] = (u8) ((block_page_addr >> 8) & 0xff);
	data_array[0] = (u8) ((block_page_addr >> 16) & 0xff);
	data_size = 3;

	push_fifo_data(data_array, data_size, false, true);
	start_flash_operation(OPCODE_PROGRAM_EXECUTE, 0, opmode, 0);
	wait_spi_busy();

	data_tmp_32 = get_flash_status(0);
	while (1) {
		if ((data_tmp_32 & 0x1) == 0) {
			if ((data_tmp_32 & 0x8) == 0x8) {
				pr_info("!!program failed page addr  = %x\n ",
					block_page_addr);
				return NANDFC_SPI_PROG_FAIL;
			}
			break;
		} else {
			data_tmp_32 = get_flash_status(0);
		}
	}

	return 0;
}

static inline void __spi_nand_rda_copyto_iomem(void *io_mem, void *src_buf,
					       size_t size)
{
	memcpy(io_mem, src_buf, size);
	return;
}

static void spi_nand_rda_write_buf(struct mtd_info *mtd, const uint8_t * buf,
				   int len)
{
	struct nand_chip *chip = mtd->priv;
	struct rda_nand_info *info = chip->priv;
	u8 *pbuf = (u8 *) buf;

	rda_dbg_nand("nand_rda_write_buf, buf addr = %x, len = %d, write_pos = %d\n",
		(u32) pbuf, len, info->write_ptr);
	if (len > mtd->writesize) {
		pr_err("error: write size is out of page size!\n");
	}

	spi_nand_flash_write_one_page_cache_random(pbuf, len, info->write_ptr);

	info->write_ptr += len;

#ifdef SPI_NAND_STATISTIC
	info->bytes_write += len;
#endif
}

u32 spi_nand_flash_block_erase(u32 block_page_addr)
{
	u32 data_tmp_32;
	u8 opmode;
	u8 data_array[3];
	u8 data_size;

	start_flash_operation(OPCODE_WRITE_ENABLE, 0, 0, 0);

	opmode = 3;
	data_array[2] = (u8) (block_page_addr & 0xff);
	data_array[1] = ((block_page_addr >> 8) & 0xff);
	data_array[0] = ((block_page_addr >> 16) & 0xff);
	data_size = 3;

	wait_spi_busy();
	push_fifo_data(data_array, data_size, 0, true);
	start_flash_operation(OPCODE_BLOCK_ERASE, 0, opmode, 0);
	wait_spi_busy();

	data_tmp_32 = get_flash_status(0);
	while (1) {
		if ((data_tmp_32 & 0x1) == 0) {
			if ((data_tmp_32 & 0x4) == 0x4) {
				pr_info("!!earse failed page addr  = %x \n",
					block_page_addr);
				return NANDFC_SPI_ERASE_FAIL;
			}
			break;
		} else {
			data_tmp_32 = get_flash_status(0);
		}
	}
	return 0;
}

static void spi_nand_rda_do_cmd_post(struct mtd_info *mtd)
{
	struct nand_chip *this = mtd->priv;
	struct rda_nand_info *info = this->priv;
	u8 temp;

	switch (info->cmd) {
	case NAND_CMD_READID:
		info->index = 0;
		break;

	case NAND_CMD_STATUS:
		temp = get_flash_status(0);
#ifdef NAND_DEBUG
		printk("nand_rda_do_cmd_post: flash status = %x\n ", temp);
#endif
		if ((temp & NANDFC_SPI_ERASE_FAIL)
		    && (info->cmd_flag == NAND_CMD_ERASE2))
			info->byte_buf[0] = 0xE0 | NAND_STATUS_FAIL;
		else if ((temp & NANDFC_SPI_PROG_FAIL)
			 && (info->cmd_flag == NAND_CMD_PAGEPROG))
			info->byte_buf[0] = 0xE0 | NAND_STATUS_FAIL;
		else if (temp & NANDFC_SPI_OPERATE_ERROR)
			info->byte_buf[0] = 0xE0 | NAND_STATUS_FAIL;
		else
			info->byte_buf[0] = 0xE0;

		info->index = 0;
		info->cmd_flag = 0;
		break;
	default:
		break;
	}
}

static void spi_nand_rda_hwcontrol(struct mtd_info *mtd, int cmd,
				   unsigned int ctrl)
{

}

static int spi_nand_reset_flash(void)
{
	u32 data_tmp_32;

	start_flash_operation(OPCODE_FLASH_RESET, 0, 0, 0);

	/*delay 10us for reset process */
	udelay(10);

	data_tmp_32 = get_flash_status(0);
	while (1) {
		if ((data_tmp_32 & 0x1) == 0) {
			break;
		} else {
			data_tmp_32 = get_flash_status(0);
		}
	}

	return 0;
}

static void spi_nand_rda_cmdfunc(struct mtd_info *mtd, unsigned int command,
				 int column, int page_addr)
{
	register struct nand_chip *chip = mtd->priv;
	struct rda_nand_info *info = chip->priv;

	rda_dbg_nand("nand_rda_cmdfunc, cmd = %x, page_addr = %x, col_addr = %x\n",
		command, page_addr, column);

	/* remap command */
	switch (command) {
	case NAND_CMD_RESET:
		spi_nand_reset_flash();
		return;

	case NAND_CMD_READID:
		info->byte_buf[0] = get_flash_ID(0);
		info->byte_buf[1] = get_flash_ID(1);
		info->index = 0;
		break;

	case NAND_CMD_SEQIN:	/* 0x80 do nothing, wait for 0x10 */
		info->page_addr = page_addr;
		info->col_addr = column;
		info->cmd = NAND_CMD_NONE;
		/* Hold the offset we want to write. */
		info->write_ptr = column;
		break;

	case NAND_CMD_READSTART:	/* hw auto gen 0x30 for read */
		break;

	case NAND_CMD_ERASE1:
		disable_flash_protection(0);
		if (NANDFC_SPI_ERASE_FAIL ==
		    spi_nand_flash_block_erase(page_addr))
			pr_info("spi nand: erase failed!page addr = %x\n",
				page_addr);
		break;

	case NAND_CMD_ERASE2:	/* hw auto gen 0xd0 for erase */
		info->cmd_flag = NAND_CMD_ERASE2;
		info->cmd = NAND_CMD_NONE;
		break;

	case NAND_CMD_PAGEPROG:	/* 0x10 do the real program */
		disable_flash_protection(0);
		if (NANDFC_SPI_PROG_FAIL ==
			    spi_nand_flash_program_execute(info->page_addr))
			pr_err("spi nand: program failed!page addr = %x\n", page_addr);
		info->cmd_flag = NAND_CMD_PAGEPROG;
		info->cmd = NAND_CMD_NONE;
		info->col_addr = 0;
		info->write_ptr = 0;

		//spi_nand_flash_cache_flush(mtd, info->page_addr);
		break;

	case NAND_CMD_READOOB:	/* Emulate NAND_CMD_READOOB */
		info->cmd = NAND_CMD_NONE;
		spi_nand_read_page_oob(mtd, page_addr);

		info->byte_buf[0] = g_spi_flash_oob_buffer[0];
		info->index = 0;
		break;

	case NAND_CMD_READ0:
		info->read_ptr = 0;
		info->page_addr = page_addr;
		info->col_addr = column;
		break;

	default:
		info->page_addr = page_addr;
		info->col_addr = column;
		info->cmd = command;
		break;
	}

	spi_nand_rda_do_cmd_post(mtd);
}

static int spi_nand_rda_dev_ready(struct mtd_info *mtd)
{
	return 1;
}


static int spi_nand_rda_map_type(struct rda_nand_info *info)
{
	switch (info->mtd.writesize) {
	case 2048:
		info->type = SPINAND_TYPE_SPI2K;
		break;
	case 4096:
		info->type = SPINAND_TYPE_SPI4K;
		break;
	default:
		dev_err(info->dev, "invalid pagesize %d\n",
			info->mtd.writesize);
		info->type = SPINAND_TYPE_INVALID;
		return -EINVAL;
	}

	return 0;
}

static int spi_nand_read_page(struct mtd_info *mtd, struct nand_chip *chip,
				uint8_t *buf, int oob_required, int page)
{

	struct rda_nand_info *info = chip->priv;
	u32 status;

	rda_dbg_nand("%s for page %d\n", __func__, page);
	chip->read_buf(mtd, buf, mtd->writesize);
	if (oob_required)
		chip->read_buf(mtd, chip->oob_poi, mtd->oobsize);
	status = get_flash_status(0);
	
	/* ECC is done inside NAND CHIP automaticlly, SW just need
	 * to read the ECC status
	 * from spec
	** 00b = No bit errors were detected during the previous read algorithm.
	** 01b = bit error was detected and corrected, error bit number = 1~7.
	** 10b = bit error was detected and not corrected.
	** 11b = bit error was detected and corrected, error bit number = 8.
	*/

	if (0x10 == (status & (NANDFC_SPI_ECCS0 | NANDFC_SPI_ECCS1))) {
#ifdef SPI_NAND_STATISTIC
		info->read_ecc_flip_pages++;
#endif
		return 3; /* 1 to 7 flipped bits, is not critical */
	}

	if (0x20 == (status & (NANDFC_SPI_ECCS0 | NANDFC_SPI_ECCS1))) {
#ifdef SPI_NAND_STATISTIC
		info->read_ecc_error_pages++;
#endif
		return -EIO;
	}

	if (0x30 == (status & (NANDFC_SPI_ECCS0 | NANDFC_SPI_ECCS1))) {
#ifdef SPI_NAND_STATISTIC
		info->read_ecc_flip_pages++;
#endif
		return 8; /* Maximum number of bits corrected, critical! */
	}
	return 0;
}

static int spi_nand_write_page(struct mtd_info *mtd, struct nand_chip *chip,
			const uint8_t *buf, int oob_required)
{
	rda_dbg_nand("%s \n", __func__);
	chip->write_buf(mtd, buf, mtd->writesize);
	if (oob_required)
		chip->write_buf(mtd, chip->oob_poi, mtd->oobsize);
	/* It is not possible to return status until PAGEPROG done.
	 * Status of the write is returned by wait_func()
	 */
	return 0;
}

#ifdef CONFIG_PROC_FS
static int nand_rda_proc_show(struct seq_file *m,void *v)
{
	int len;
	struct rda_nand_info *info = m->private;

	len = seq_printf(m, "Total Size: %dMiB\n",(int)(info->nand_chip.chipsize/SZ_1M));
	len += seq_printf(m, "page size: %dB\n", info->mtd.writesize);

#ifdef SPI_NAND_STATISTIC
	len += seq_printf(m, "total read size: %llu KB\n", info->bytes_read/SZ_1K);
	len += seq_printf(m, "total write size: %llu KB\n", info->bytes_write/SZ_1K);
	len += seq_printf(m, "total read error pages %llu\n", info->read_ecc_error_pages);
	len += seq_printf(m, "total read flip pages %llu\n", info->read_ecc_flip_pages);
#endif
#ifdef SETUP_DELAY_TUNING
	len += seq_printf(m, "delay_min : %lu US\n", info->setup_delay_min);
	len += seq_printf(m, "delay_max : %lu US\n", info->setup_delay_max);
#endif
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

static void spi_nand_rda_create_proc(struct rda_nand_info *info)
{
	struct proc_dir_entry *pde;

	pde = proc_create_data(PROCNAME, 0664, NULL, &nand_rda_proc_fops, info);
	if (pde == NULL) {
		pr_err("%s failed\n", __func__);
	}
}

static void spi_nand_rda_delete_proc(void)
{
	remove_proc_entry(PROCNAME, NULL);
}
#else

static void spi_nand_rda_create_proc(struct rda_nand_info *info) {}

static void spi_nand_rda_delete_proc(void) {}
#endif

static int spi_nand_rda_init(struct rda_nand_info *info)
{
	int ret;

	ret = spi_nand_rda_map_type(info);
	if (ret) {
		return ret;
	}

	nand_spi_init();

	return 0;
}

/**
 * nand_rda_wait - wait until the command is done
 * @mtd: MTD device structure
 * @chip: NAND chip structure
 *
 * Wait for command done. This applies to erase and program only. Erase can
 * take up to 400ms and program up to 20ms according to general NAND and
 * SmartMedia specs.
 */
static int spi_nand_rda_wait(struct mtd_info *mtd, struct nand_chip *chip)
{
	chip->cmdfunc(mtd, NAND_CMD_STATUS, -1, -1);

	return (int)chip->read_byte(mtd);
}

#ifdef CONFIG_MTD_NAND_RDA_DMA
/*
 * nand_rda_dma_cb: callback on the completion of dma transfer
 * @ch: logical channel
 * @data: pointer to completion data structure
 */
static void spi_nand_rda_dma_cb(u8 ch, void *data)
{
	complete((struct completion *)data);
}
#endif /* CONFIG_MTD_NAND_RDA_DMA */

#ifdef CONFIG_MTD_PARTITIONS
static const char *part_probes[] = { "cmdlinepart", NULL };
#endif

/*
 * Probe for the NAND device.
 */
static int __init rda_spi_nand_probe(struct platform_device *pdev)
{
	struct rda_nand_info *info;
	struct mtd_info *mtd;
	struct nand_chip *nand_chip;
	struct resource *mem;
	int res = 0;

	mem = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!mem) {
		dev_err(&pdev->dev, "can not get resource mem\n");
		return -ENXIO;
	}

	/* Allocate memory for the device structure (and zero it) */
	info = kzalloc(sizeof(struct rda_nand_info), GFP_KERNEL);
	if (!info) {
		dev_err(&pdev->dev, "failed to allocate device structure.\n");
		return -ENOMEM;
	}
#ifdef CONFIG_MTD_NAND_RDA_DMA
	info->phy_addr = mem->start;
#endif /* CONFIG_MTD_NAND_RDA_DMA */

	info->base = ioremap(mem->start, resource_size(mem));
	if (info->base == NULL) {
		dev_err(&pdev->dev, "ioremap failed\n");
		res = -EIO;
		goto err_nand_ioremap;
	}
	info->reg_base = info->base + 0;
	info->data_base = info->base + 8;
	RDA_SPIFLASH_BASE = (u32) info->reg_base;

	mtd = &info->mtd;
	nand_chip = &info->nand_chip;
	info->plat_data = pdev->dev.platform_data;
	info->dev = &pdev->dev;

	info->master_clk = clk_get(NULL, RDA_CLK_SPIFLASH);
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
#ifdef CONFIG_MTD_NAND_RDA_DMA
	nand_chip->IO_ADDR_R = info->data_base;
	nand_chip->IO_ADDR_W = info->data_base;
#endif /* CONFIG_MTD_NAND_RDA_DMA */
	nand_chip->cmd_ctrl = spi_nand_rda_hwcontrol;
	nand_chip->cmdfunc = spi_nand_rda_cmdfunc;
	nand_chip->dev_ready = spi_nand_rda_dev_ready;

#ifdef CONFIG_MTD_NAND_RDA_DMA
	rda_request_dma(0, "rda-dma", spi_nand_rda_dma_cb, &info->comp,
			&info->dma_ch);
#endif /* CONFIG_MTD_NAND_RDA_DMA */

	nand_chip->chip_delay = 20;	/* 20us command delay time */
	nand_chip->read_buf = spi_nand_rda_read_buf;
	nand_chip->write_buf = spi_nand_rda_write_buf;
	nand_chip->read_byte = spi_nand_rda_read_byte;
	nand_chip->read_word = spi_nand_rda_read_word;
	nand_chip->waitfunc = spi_nand_rda_wait;
	/* we do use flash based bad block table, created by u-boot */
	nand_chip->bbt_options = NAND_BBT_USE_FLASH;
	info->write_ptr = 0;
	info->read_ptr = 0;
	info->index = 0;

	platform_set_drvdata(pdev, info);

	/* first scan to find the device and get the page size */
	if (nand_scan_ident(mtd, 1, NULL)) {
		res = -ENXIO;
		goto err_scan_ident;
	}

	spi_nand_rda_init(info);


	nand_chip->ecc.mode = NAND_ECC_HW;
	nand_chip->ecc.size = mtd->writesize;
	nand_chip->ecc.bytes = 0;
	nand_chip->ecc.strength = 8;
	nand_chip->ecc.read_page = spi_nand_read_page;
	nand_chip->ecc.write_page = spi_nand_write_page;

	if (!nand_chip->ecc.layout
	    && (nand_chip->ecc.mode != NAND_ECC_SOFT_BCH)) {
		pr_info("kernel OOB layout:%d\n", mtd->oobsize);
		switch (mtd->oobsize) {
		case 8:
			break;
		case 16:
			break;
		case 64:
			nand_chip->ecc.layout = &spi_nand_oob_64;
			break;
		case 128:
			nand_chip->ecc.layout = &spi_nand_oob_128;
			break;
		case 256:
			nand_chip->ecc.layout = &spi_nand_oob_256;
			break;
		default:
			pr_info("No oob layout defined for oobsize %d\n",
				mtd->oobsize);
		}
	}

	if (nand_chip->bbt_options & NAND_BBT_USE_FLASH) {
		/* Use the default pattern descriptors */
		if (!nand_chip->bbt_td) {
			pr_info("kernel BBT ver: spi\n");
			nand_chip->bbt_td = &spi_bbt_main_descr;
			nand_chip->bbt_md = &spi_bbt_mirror_descr;
		}
	}

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

	spi_nand_do_setup_delay_tuning(info);

	spi_nand_rda_create_proc(info);

	return 0;

err_parse_register:
	nand_release(mtd);

err_scan_tail:

err_scan_ident:

	//rda_nand_disable(host);
	platform_set_drvdata(pdev, NULL);

#ifdef CONFIG_MTD_NAND_RDA_DMA
	rda_free_dma(info->dma_ch);
#endif /* CONFIG_MTD_NAND_RDA_DMA */

	clk_put(info->master_clk);

err_nand_get_clk:
	iounmap(info->base);

err_nand_ioremap:

	kfree(info);

	return res;
}

/*
 * Remove a NAND device.
 */
static int __exit rda_spi_nand_remove(struct platform_device *pdev)
{
	struct rda_nand_info *info = platform_get_drvdata(pdev);
	struct mtd_info *mtd = &info->mtd;

	nand_release(mtd);
	spi_nand_rda_delete_proc();
	//rda_nand_disable(host);
#ifdef CONFIG_MTD_NAND_RDA_DMA
	rda_free_dma(info->dma_ch);
#endif /* CONFIG_MTD_NAND_RDA_DMA */

	clk_put(info->master_clk);

	iounmap(info->base);

	kfree(info);

	return 0;
}

static struct platform_driver rda_spi_nand_driver = {
	.remove = __exit_p(rda_spi_nand_remove),
	.driver = {
		   .name = RDA_SPI_NAND_DRV_NAME,
		   .owner = THIS_MODULE,
		   },
};

static int __init rda_spi_nand_init(void)
{
	return platform_driver_probe(&rda_spi_nand_driver, rda_spi_nand_probe);
}

static void __exit rda_spi_nand_exit(void)
{
	platform_driver_unregister(&rda_spi_nand_driver);
}

module_init(rda_spi_nand_init);
module_exit(rda_spi_nand_exit);

MODULE_DESCRIPTION("RDA spi NAND Driver");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:rda_spinand");
