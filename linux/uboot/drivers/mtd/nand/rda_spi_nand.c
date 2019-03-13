#include <common.h>
#include <malloc.h>
#include <asm/errno.h>
#include <asm/io.h>
#include <asm/types.h>
#include <nand.h>

#include <asm/arch/hardware.h>
#include <asm/arch/reg_sysctrl.h>
#include <asm/arch/hwcfg.h>
#include <mtd/nand/rda_nand.h>

#include <rda/tgt_ap_board_config.h>
#include <rda/tgt_ap_clock_config.h>

#ifdef CONFIG_SPL_BUILD
#undef CONFIG_NAND_RDA_DMA
#endif

#ifdef CONFIG_NAND_RDA_DMA
#include <asm/dma-mapping.h>
#include <asm/arch/dma.h>
#endif

#if (_TGT_NAND_TYPE_ == _SPI_NAND_USED_)
/*******************************************************************/
/*******************spi flash MACRO define******************************/
#define SPI_NAND_READ_DATA_BUFFER_BASE 0xc0000000
#define SPI_NAND_WRITE_DATA_BUFFER_BASE (RDA_SPIFLASH_BASE + 0x8)

/*******************spi controller register define**************************/
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
#define NANDFC_SPI_PROG_FAIL (1 << 3)
#define NANDFC_SPI_ERASE_FAIL (1 << 2)
#define NANDFC_SPI_OPERATE_ERROR (1 << 0)

typedef enum {
	SPINAND_TYPE_SPI2K = 16,	// SPI, 2048+64, 2Gb
	SPINAND_TYPE_SPI4K = 17,	// SPI, 4096+128, 4Gb
	SPINAND_TYPE_INVALID = 0xFF,
} SPINAND_FLASH_TYPE;

#define PLL_BUS_FREQ	(_TGT_AP_PLL_BUS_FREQ * 1000000)
/*******************MACRO define***********************************/
#define REG_READ_UINT32( _reg_ )			(*(volatile unsigned long*)(_reg_))
#define REG_WRITE_UINT32( _reg_, _val_) 	((*(volatile unsigned long*)(_reg_)) = (unsigned long)(_val_))

/*******************spi nand  global variables define **********************/
//static u8 g_spi_flash_read_temp_buffer[4352] __attribute__ ((__aligned__(64)));
/*sram space is not enough, so use sdram space as middle data buffer*/
static u8 *g_spi_flash_read_temp_buffer = (u8 *)CONFIG_SPL_NAND_MIDDLE_DATA_BUFFER;
/*******************************************************************/

//#define NAND_DEBUG
//#define NAND_DEBUG_VERBOSE
/*read page data with 4k middle buffer:  g_spi_flash_read_temp_buffer[4352],
   OOB always use middle buffer.*/
//#define READ_PAGE_DATA_USE_MIDDLE_BUFFER
#define READ_PAGE_DATA_WITH_SW_MODE

#define hal_gettime     get_ticks
#define SECOND          * CONFIG_SYS_HZ_CLOCK
#define NAND_TIMEOUT    ( 2 SECOND )

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

static void push_fifo_data(u8 data_array[], u32 data_cnt, BOOL quard_flag,
			   BOOL clr_flag)
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

	quard_flag = FALSE;
	opmode = 1;
	data_array[0] = 0xb0;
	data_size = 1;

	wait_spi_busy();
	/*clear spi rx fifo */
	spi_rx_fifo_clear();

	/*put addr into spi tx fifo */
	push_fifo_data(data_array, data_size, quard_flag, TRUE);
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
	BOOL quard_flag;

	quard_flag = FALSE;
	opmode = 3;
	data_array[1] = data;
	data_array[0] = 0xb0;
	data_size = 2;

	wait_spi_busy();
	push_fifo_data(data_array, data_size, quard_flag, TRUE);
	start_flash_operation(OPCODE_SET_FEATURE, 0, opmode, 0);
	wait_spi_busy();
	wait_spi_tx_fifo_empty();
}

static BOOL spi_nand_flash_quad_enable = FALSE;
void enable_spi_nand_flash_quad_mode(void)
{
	u8 data;

	data = get_flash_feature(0);
	data = data | 0x1;
	set_flash_feature(data);
	spi_nand_flash_quad_enable = TRUE;
}

void disable_spi_nand_flash_quad_mode(void)
{
	u8 data;

	data = get_flash_feature(0);
	data = data & 0xfe;
	set_flash_feature(data);
	spi_nand_flash_quad_enable = FALSE;
}

void enable_spi_nand_flash_ecc_mode(void)
{
	u8 data;

	data = get_flash_feature(0);
	data = data | 0x10;
	set_flash_feature(data);
}

void disable_spi_nand_flash_ecc_mode(void)
{
	u8 data;

	data = get_flash_feature(0);
	data = data & 0xef;
	set_flash_feature(data);
}

u8 get_flash_status(u32 flash_addr)
{
	u32 data_tmp_32;
	u8 opmode;
	u8 data_array[2];
	u8 data_size;
	BOOL quard_flag;

	quard_flag = FALSE;
	opmode = 1;
	data_array[0] = 0xc0;
	data_size = 1;

	wait_spi_busy();
	spi_rx_fifo_clear();

	wait_spi_busy();
	push_fifo_data(data_array, data_size, quard_flag, TRUE);
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
	push_fifo_data(data_array, data_size, FALSE, TRUE);
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
	BOOL quard_flag;
	u8 data_array[2];
	u8 data_size;
	u8 manufacturerID, device_memory_type_ID;

	opmode = 1;
	data_array[0] = 0x00;
	data_size = 1;
	quard_flag = FALSE;

	wait_spi_busy();
	spi_rx_fifo_clear();

	push_fifo_data(data_array, data_size, quard_flag, TRUE);
	start_flash_operation(OPCODE_READ_ID, flash_addr, opmode, 2);
	wait_spi_busy();

	data_tmp_32 = REG_READ_UINT32(SPI_STATUS);
	while (data_tmp_32 & 0x8)
		data_tmp_32 = REG_READ_UINT32(SPI_STATUS);

	manufacturerID = (u8) (REG_READ_UINT32(SPI_READ_BACK) & 0xff);
	device_memory_type_ID = (u8) (REG_READ_UINT32(SPI_READ_BACK) & 0xff);

	return (manufacturerID | (device_memory_type_ID << 8));
}

void nand_spi_init(BOOL quard_flag, u8 clk_offset_val, u8 clkdiv_val)
{
	u32 data_tmp_32;

	wait_spi_busy();
	data_tmp_32 = ((clk_offset_val << 4) | (clkdiv_val << 8)) & 0xff70;
	REG_WRITE_UINT32(SPI_CONFIG, data_tmp_32);
	wait_spi_busy();

	if (quard_flag) {
		enable_spi_nand_flash_quad_mode();
		spi_nand_flash_operate_mode_setting(4);
	}else{
		disable_spi_nand_flash_quad_mode();
		spi_nand_flash_operate_mode_setting(1);
	}

	enable_spi_nand_flash_ecc_mode();
}

static void nand_spi_fifo_word_read_set(BOOL enable)
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

static int init_spi_nand_info(struct rda_nand_info *info,
			      int nand_type, int bus_width_16)
{
	info->type = nand_type;
	switch (nand_type) {
	case SPINAND_TYPE_SPI2K:
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
		break;
	case SPINAND_TYPE_SPI4K:
		info->hard_ecc_hec = 0;
		info->bus_width_16 = bus_width_16;
		info->page_shift = 12;
		info->oob_size = 128;
		info->page_num_per_block = 64;
		info->vir_page_size = 4096;
		info->vir_page_shift = 12;
		info->vir_oob_size = 128;
		info->vir_erase_size =
		    info->vir_page_size * info->page_num_per_block;
		break;
	default:
		printf("invalid nand type\n");
		return -EINVAL;
	}
	return 0;
}

static u8 spi_nand_rda_read_byte(struct mtd_info *mtd)
{
	u8 ret;
	struct nand_chip *chip = mtd->priv;
	struct rda_nand_info *info = chip->priv;
	ret = *((u8 *) info->byte_buf + info->index);
	info->index++;
#ifdef NAND_DEBUG
	printf("spi_nand_read_byte, ret = %02x\n", ret);
#endif
	return ret;
}

static u16 spi_nand_rda_read_word(struct mtd_info *mtd)
{
	u16 ret;
	struct nand_chip *chip = mtd->priv;
	struct rda_nand_info *info = chip->priv;
	ret = *(u16 *) ((uint8_t *) info->byte_buf + info->index);
	info->index += 2;
#ifdef NAND_DEBUG
	printf("spi_nand_read_word, ret = %04x\n", ret);
#endif
	return ret;
}

#ifdef READ_PAGE_DATA_WITH_SW_MODE
static void spi_nand_flash_page_read2cache(u32 page_addr)
{
	u32 data_tmp_32;
	u8 opmode;
	u8 data_array[3];
	u8 data_size;

#ifdef NAND_DEBUG
	printf("spi nand: OOB read to cache!page addr = %x\n", page_addr);
#endif
	opmode = 3;
	data_array[2] = (u8)(page_addr & 0xff);
	data_array[1] =  (u8)((page_addr >> 8) & 0xff);
	data_array[0] =  (u8)((page_addr >> 16) & 0xff);
	data_size = 3;

	wait_spi_busy();
	push_fifo_data(data_array, data_size, FALSE, TRUE);
	start_flash_operation(OPCODE_PAGE_READ_2_CACHE, 0, opmode, 0);
	wait_spi_busy();

       data_tmp_32 = get_flash_status(0);
       while (1)
	{
		if ((data_tmp_32 & 0x1) == 0)
		{
			break;
		}
		else
		{
			data_tmp_32 = get_flash_status(0);
		}
	}
}
#endif

#define __SPI_NAND_CONTROLLER_RCV_FIFO_LEVEL__  64   //bytes,  16 words
static void spi_nand_flash_read_from_cache(u8 * buffer, u32 page_column_addr,
					   u16 length)
{
	u32 data_tmp_32;
	u8 opmode, i;
	u8 data_array[3];
	u8 data_size;
	u32 data_count, data_value;
	u32 loop_div_count, loop_mod_count;
	BOOL quard_flag = FALSE;
	u8 command = OPCODE_READ_FROM_CACHE;
	u32 temp_column_addr;

#ifdef NAND_DEBUG
	printf
	    ("spi nand: OOB read from cache to buffer!buffer_addr = %x, column = %x, length = %d\n",
	     (u32) buffer, page_column_addr, length);
#endif
	opmode = 1;
	data_array[2] = 0;
	data_size = 3;

	if (spi_nand_flash_quad_enable) {
		command = OPCODE_READ_FROM_CACHE_QUADIO;
		quard_flag = TRUE;
		opmode = 0;
	}
#if  0
	for (data_count = 0; data_count < length / 16; data_count++) {
		temp_column_addr = page_column_addr + 16 * data_count;
		data_array[1] = (u8) (temp_column_addr & 0xff);
		data_array[0] = ((temp_column_addr >> 8) & 0x0f);

		wait_spi_busy();
		spi_rx_fifo_clear();

		wait_spi_busy();
		if (spi_nand_flash_quad_enable) {
			start_flash_operation(command, temp_column_addr, opmode,
					      16);
		} else {
			push_fifo_data(data_array, data_size, quard_flag, TRUE);
			start_flash_operation(command, 0, opmode, 16);
		}
		wait_spi_busy();
#ifdef NAND_DEBUG
		printf
		    ("read OOB: data [0] =  %x, data{1] = %x, data[2] = %x, quard_flag = %d, opmode = %d\n",
		     data_array[0], data_array[1], data_array[2], quard_flag,
		     opmode);
#endif
		for (i = 0; i < 16; i++) {
			data_tmp_32 = REG_READ_UINT32(SPI_STATUS);
			while (data_tmp_32 & 0x8)
				data_tmp_32 = REG_READ_UINT32(SPI_STATUS);

			buffer[data_count * 16 + i] =
			    (u8) (REG_READ_UINT32(SPI_READ_BACK) & 0xff);
#ifdef NAND_DEBUG
			printf("buffer[%d] = %x \n", data_count * 16 + i,
			       buffer[data_count * 16 + i]);
#endif
		}
	}

	if ((length % 16) > 0) {
		temp_column_addr = page_column_addr + 16 * data_count;
		data_array[1] = (u8) (temp_column_addr & 0xff);
		data_array[0] = ((temp_column_addr >> 8) & 0x0f);

		wait_spi_busy();
		spi_rx_fifo_clear();

		wait_spi_busy();
		if (spi_nand_flash_quad_enable) {
			start_flash_operation(command, temp_column_addr, opmode,
					      length % 16);
		} else {
			push_fifo_data(data_array, data_size, quard_flag, TRUE);
			start_flash_operation(command, 0, opmode, length % 16);
		}
		wait_spi_busy();

		for (i = 0; i < (length % 16); i++) {
			data_tmp_32 = REG_READ_UINT32(SPI_STATUS);
			while (data_tmp_32 & 0x8)
				data_tmp_32 = REG_READ_UINT32(SPI_STATUS);

			buffer[data_count * 16 + i] =
			    (u8) (REG_READ_UINT32(SPI_READ_BACK) & 0xff);
#ifdef NAND_DEBUG
			printf("buffer[%d] = %x \n", data_count * 16 + i,
			       buffer[data_count * 16 + i]);
#endif
		}
	}
#else
	nand_spi_fifo_word_read_set(TRUE);
	if (length  > 0) {
		loop_div_count = length / __SPI_NAND_CONTROLLER_RCV_FIFO_LEVEL__;
		loop_mod_count = length % __SPI_NAND_CONTROLLER_RCV_FIFO_LEVEL__;
		for (data_count = 0; data_count < loop_div_count; data_count++) {
			temp_column_addr = page_column_addr + __SPI_NAND_CONTROLLER_RCV_FIFO_LEVEL__ * data_count;
		data_array[1] = (u8) (temp_column_addr & 0xff);
		data_array[0] = ((temp_column_addr >> 8) & 0x0f);

		wait_spi_busy();
		spi_rx_fifo_clear();

		wait_spi_busy();
		if (spi_nand_flash_quad_enable) {
			start_flash_operation(command, temp_column_addr, opmode,
						      __SPI_NAND_CONTROLLER_RCV_FIFO_LEVEL__);
		} else {
			push_fifo_data(data_array, data_size, quard_flag, TRUE);
			start_flash_operation(command, 0, opmode, __SPI_NAND_CONTROLLER_RCV_FIFO_LEVEL__);
		}
		wait_spi_busy();

		for (i = 0; i < __SPI_NAND_CONTROLLER_RCV_FIFO_LEVEL__; i += 4) {
			data_tmp_32 = REG_READ_UINT32(SPI_STATUS);
			while (data_tmp_32 & 0x8)
				data_tmp_32 = REG_READ_UINT32(SPI_STATUS);

				data_value = REG_READ_UINT32(SPI_READ_BACK);

				buffer[i + data_count * __SPI_NAND_CONTROLLER_RCV_FIFO_LEVEL__] = (u8) (data_value & 0xff);
				buffer[i + 1 + data_count * __SPI_NAND_CONTROLLER_RCV_FIFO_LEVEL__] = (u8) ((data_value >> 8) & 0xff);
				buffer[i + 2 + data_count * __SPI_NAND_CONTROLLER_RCV_FIFO_LEVEL__] = (u8) ((data_value >> 16) & 0xff);
				buffer[i + 3 + data_count * __SPI_NAND_CONTROLLER_RCV_FIFO_LEVEL__] = (u8) ((data_value >> 24) & 0xff);
#ifdef NAND_DEBUG
				printf("buffer[%d] = %x \n", i + data_count * __SPI_NAND_CONTROLLER_RCV_FIFO_LEVEL__,
								buffer[i + data_count * __SPI_NAND_CONTROLLER_RCV_FIFO_LEVEL__]);
				printf("buffer[%d] = %x \n", i + 1 + data_count * __SPI_NAND_CONTROLLER_RCV_FIFO_LEVEL__,
								buffer[i + 1 + data_count * __SPI_NAND_CONTROLLER_RCV_FIFO_LEVEL__]);
				printf("buffer[%d] = %x \n", i + 2 + data_count * __SPI_NAND_CONTROLLER_RCV_FIFO_LEVEL__,
								buffer[i + 2 + data_count * __SPI_NAND_CONTROLLER_RCV_FIFO_LEVEL__]);
				printf("buffer[%d] = %x \n", i + 3 + data_count * __SPI_NAND_CONTROLLER_RCV_FIFO_LEVEL__,
								buffer[i + 3 + data_count * __SPI_NAND_CONTROLLER_RCV_FIFO_LEVEL__]);
#endif
			}
		}

		if(loop_mod_count){
			temp_column_addr = page_column_addr + __SPI_NAND_CONTROLLER_RCV_FIFO_LEVEL__ * data_count;
			data_array[1] = (u8) (temp_column_addr & 0xff);
			data_array[0] = ((temp_column_addr >> 8) & 0x0f);
			wait_spi_busy();
			spi_rx_fifo_clear();
			wait_spi_busy();
			if (spi_nand_flash_quad_enable) {
				start_flash_operation(command, temp_column_addr, opmode, loop_mod_count);
			} else {
				push_fifo_data(data_array, data_size, quard_flag, TRUE);
				start_flash_operation(command, 0, opmode, loop_mod_count);
			}
			wait_spi_busy();

			for (i = 0; i < loop_mod_count; i += 4) {
				data_tmp_32 = REG_READ_UINT32(SPI_STATUS);
				while (data_tmp_32 & 0x8)
					data_tmp_32 = REG_READ_UINT32(SPI_STATUS);

				data_value = REG_READ_UINT32(SPI_READ_BACK);

				buffer[i + data_count * __SPI_NAND_CONTROLLER_RCV_FIFO_LEVEL__] = (u8) (data_value & 0xff);
				buffer[i + 1 + data_count * __SPI_NAND_CONTROLLER_RCV_FIFO_LEVEL__] = (u8) ((data_value >> 8) & 0xff);
				buffer[i + 2 + data_count * __SPI_NAND_CONTROLLER_RCV_FIFO_LEVEL__] = (u8) ((data_value >> 16) & 0xff);
				buffer[i + 3 + data_count * __SPI_NAND_CONTROLLER_RCV_FIFO_LEVEL__] = (u8) ((data_value >> 24) & 0xff);
#ifdef NAND_DEBUG
				printf("buffer[%d] = %x \n", i + data_count * __SPI_NAND_CONTROLLER_RCV_FIFO_LEVEL__,
								buffer[i + data_count * __SPI_NAND_CONTROLLER_RCV_FIFO_LEVEL__]);
				printf("buffer[%d] = %x \n", i + 1 + data_count * __SPI_NAND_CONTROLLER_RCV_FIFO_LEVEL__,
								buffer[i + 1 + data_count * __SPI_NAND_CONTROLLER_RCV_FIFO_LEVEL__]);
				printf("buffer[%d] = %x \n", i + 2 + data_count * __SPI_NAND_CONTROLLER_RCV_FIFO_LEVEL__,
								buffer[i + 2 + data_count * __SPI_NAND_CONTROLLER_RCV_FIFO_LEVEL__]);
				printf("buffer[%d] = %x \n", i + 3 + data_count * __SPI_NAND_CONTROLLER_RCV_FIFO_LEVEL__,
								buffer[i + 3 + data_count * __SPI_NAND_CONTROLLER_RCV_FIFO_LEVEL__]);
#endif
				}
		}
	}
	nand_spi_fifo_word_read_set(FALSE);
#endif
}

static void spi_nand_read_flash_data2buffer(struct mtd_info *mtd, u32 page_addr)
{
#ifndef READ_PAGE_DATA_WITH_SW_MODE
	u32 flash_addr =
	    SPI_NAND_READ_DATA_BUFFER_BASE + page_addr * mtd->writesize;
#endif

#ifdef NAND_DEBUG
	printf("read flash : page addr = %x, flash_addr = %x\n", page_addr,
	       flash_addr);
#endif /* NAND_DEBUG */

#if 1// ndef CONFIG_NAND_RDA_DMA
#ifdef READ_PAGE_DATA_WITH_SW_MODE
	spi_nand_flash_page_read2cache(page_addr);
	spi_nand_flash_read_from_cache(&g_spi_flash_read_temp_buffer[0],
				0, mtd->writesize);
#else
	memcpy((void *)(&g_spi_flash_read_temp_buffer[0]), (void *)flash_addr,
	       mtd->writesize);
#endif
#ifdef NAND_DEBUG
	printf("read flash: dst_addr = %x, src_addr = %x, length = %x\n",
	       (u32) (&g_spi_flash_read_temp_buffer[0]), flash_addr,
	       mtd->writesize);
#endif /* NAND_DEBUG */
#else
	struct nand_chip *chip = mtd->priv;
	struct rda_dma_chan_params dma_param;
	dma_addr_t phys_addr;
	void *addr = (void *)(&g_spi_flash_read_temp_buffer[0]);
	int ret = 0;
	struct rda_nand_info *info = chip->priv;

	phys_addr = dma_map_single(addr, mtd->writesize, DMA_FROM_DEVICE);

	dma_param.src_addr = flash_addr;
	dma_param.dst_addr = phys_addr;
	dma_param.xfer_size = mtd->writesize;
	//dma_param.dma_mode = RDA_DMA_FR_MODE;
	dma_param.dma_mode = RDA_DMA_NOR_MODE;

	ret = rda_set_dma_params(info->dma_ch, &dma_param);
	if (ret < 0) {
		printf("rda spi nand : Failed to set parameter\n");
		dma_unmap_single(addr, mtd->writesize, DMA_FROM_DEVICE);
		return;
	}

	/* use flush to avoid annoying unaligned warning */
	/* however, invalidate after the dma it the right thing to do */
	flush_dcache_range((u32) addr, (u32) (addr + mtd->writesize));

	rda_start_dma(info->dma_ch);
	rda_poll_dma(info->dma_ch);
	rda_stop_dma(info->dma_ch);

	/* use flush to avoid annoying unaligned warning */
	//invalidate_dcache_range((u32)addr, (u32)(addr + len));

	/* Free the specified physical address */
	dma_unmap_single(addr, mtd->writesize, DMA_FROM_DEVICE);
#ifdef NAND_DEBUG
	printf("read flash, middle buf-data:\n");
	rda_dump_buf((char *)addr, mtd->writesize);
#endif
#endif
}

static void spi_nand_rda_read_buf(struct mtd_info *mtd, uint8_t * buf, int len)
{
	struct nand_chip *chip = mtd->priv;
	struct rda_nand_info *info = chip->priv;
	u8 *nand_ptr = (u8 *) (g_spi_flash_read_temp_buffer + info->read_ptr);
#ifndef READ_PAGE_DATA_USE_MIDDLE_BUFFER
#ifndef READ_PAGE_DATA_WITH_SW_MODE
	u32 flash_addr =
	    SPI_NAND_READ_DATA_BUFFER_BASE + info->page_addr * mtd->writesize;
#endif/*READ_PAGE_DATA_WITH_SW_MODE*/
#endif/*READ_PAGE_DATA_USE_MIDDLE_BUFFER*/

#ifdef NAND_DEBUG
	printf
	    ("read buf: buf addr = %x, len = %d,  nand_ptr = %x, read_ptr = %d\n",
	     (u32) buf, len, (u32) nand_ptr, info->read_ptr);
#endif /* NAND_DEBUG */

#if 1//ndef CONFIG_NAND_RDA_DMA
#ifdef READ_PAGE_DATA_USE_MIDDLE_BUFFER
	memcpy((void *)buf, (void *)nand_ptr, len);
#else
	if(info->read_ptr == 0){
#ifndef READ_PAGE_DATA_WITH_SW_MODE		
		memcpy((void *)buf, (void *)flash_addr, mtd->writesize);
#else
		spi_nand_flash_page_read2cache(info->page_addr);
		spi_nand_flash_read_from_cache(buf, 0, mtd->writesize);
#endif
		spi_nand_flash_read_from_cache(&g_spi_flash_read_temp_buffer[mtd->writesize],
				mtd->writesize, mtd->oobsize);
		/*gigadevice flash hardware reserved block, add to BBT, 0xffe0000, 0x1ffc0000 */
		if ((0xffe0000 == info->page_addr * mtd->writesize && info->type == SPINAND_TYPE_SPI2K)
		|| (0x1ffc0000 == info->page_addr * mtd->writesize && info->type == SPINAND_TYPE_SPI4K)){
			memset((void *)buf, 0, len);
			memset(&g_spi_flash_read_temp_buffer[0], 0,
			       mtd->writesize + mtd->oobsize);
		}
	}else{
		memcpy((void *)buf, (void *)nand_ptr, len);
	}
#endif/*READ_PAGE_DATA_USE_MIDDLE_BUFFER*/

	info->read_ptr += len;

#ifdef NAND_DEBUG
	printf("read buf, dst buf:\n");
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
#ifdef NAND_DEBUG
		printf("read buf, dst buf-oob:\n");
		rda_dump_buf((char *)buf, len);
#endif
		return;
	}

	phys_addr = dma_map_single(addr, len, DMA_BIDIRECTIONAL);

	dma_param.src_addr = (u32) nand_ptr;
	dma_param.dst_addr = phys_addr;
	dma_param.xfer_size = len;
	//dma_param.dma_mode = RDA_DMA_FR_MODE;
	dma_param.dma_mode = RDA_DMA_NOR_MODE;

	ret = rda_set_dma_params(info->dma_ch, &dma_param);
	if (ret < 0) {
		printf("rda spi nand : Failed to set parameter\n");
		dma_unmap_single(addr, len, DMA_BIDIRECTIONAL);
		return;
	}

	/* use flush to avoid annoying unaligned warning */
	/* however, invalidate after the dma it the right thing to do */
	flush_dcache_range((u32) addr, (u32) (addr + len));

	rda_start_dma(info->dma_ch);
	rda_poll_dma(info->dma_ch);
	rda_stop_dma(info->dma_ch);

	/* use flush to avoid annoying unaligned warning */
	//invalidate_dcache_range((u32)addr, (u32)(addr + len));

	/* Free the specified physical address */
	dma_unmap_single(addr, len, DMA_BIDIRECTIONAL);
	info->read_ptr += len;

#ifdef NAND_DEBUG
	printf("read buf, dst buf-data:\n");
	rda_dump_buf((char *)addr, len);
#endif

	return;
#endif /* CONFIG_NAND_RDA_DMA */
}

static void spi_nand_flash_write_data2cache_random(u8 * buffer,
						   u32 page_column_addr,
						   u32 length,
						   BOOL bug_mode)
{
	u8 opmode;
	u8 data_array[3];
	u8 data_size;
	BOOL quard_flag;
	u8 opcommand;

	if(bug_mode == FALSE){
		if (spi_nand_flash_quad_enable) {
			quard_flag = TRUE;
			opcommand = OPCODE_PROGRAM_LOAD_RANDOM_DATA_QUADIO;
		} else {
			quard_flag = FALSE;
			opcommand = OPCODE_PROGRAM_LOAD_RANDOM_DATA;
		}

		opmode = 3;
		data_array[1] = (u8) (page_column_addr & 0xff);
		data_array[0] = ((page_column_addr >> 8) & 0x0f);
		data_size = 2;

		wait_spi_busy();
		push_fifo_data(data_array, data_size, quard_flag, TRUE);
		push_fifo_data(buffer, length, quard_flag, FALSE);
		start_flash_operation(opcommand, 0, opmode, 0);
	}
	else{/*0x84 standard mode for spi controller bug*/
		wait_spi_busy();
		push_fifo_data(buffer, length, FALSE, TRUE);
		start_flash_operation(OPCODE_PROGRAM_LOAD_RANDOM_DATA, page_column_addr,
							0, 0);
	}

	wait_spi_busy();
	wait_spi_tx_fifo_empty();
	wait_spi_busy();
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
								       SPI_NAND_OPERATE_LENGTH_ONCE * i,
								       SPI_NAND_OPERATE_LENGTH_ONCE * i + write_pos,
								       SPI_NAND_OPERATE_LENGTH_ONCE,
								       FALSE);
			}
		}

		if (n > 0)
			spi_nand_flash_write_data2cache_random(w_buffer +
							       SPI_NAND_OPERATE_LENGTH_ONCE * i,
							       SPI_NAND_OPERATE_LENGTH_ONCE * i + write_pos,
							       n,
							       TRUE);
//	} else {
//		spi_nand_flash_write_data2cache_random(w_buffer, write_pos,
//						       length, TRUE);
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

	push_fifo_data(data_array, data_size, FALSE, TRUE);
	start_flash_operation(OPCODE_PROGRAM_EXECUTE, 0, opmode, 0);
	wait_spi_busy();

	data_tmp_32 = get_flash_status(0);
	while (1) {
		if ((data_tmp_32 & 0x1) == 0) {
			if ((data_tmp_32 & 0x8) == 0x8) {
				printf("!!program failed page addr  = %x\n ",
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

static void spi_nand_rda_write_buf(struct mtd_info *mtd, const uint8_t * buf,
				   int len)
{
	struct nand_chip *chip = mtd->priv;
	struct rda_nand_info *info = chip->priv;
	u8 *pbuf = (u8 *) buf;

#ifdef NAND_DEBUG
	printf("spi_nand_rda_write_buf : buf addr = %x, len = %d, write_pos = %d\n", (u32) pbuf,
	       len, info->write_ptr);
#endif /* NAND_DEBUG */

#ifdef NAND_DEBUG
	rda_dump_buf((char *)pbuf, len);
#endif

	if (len > mtd->writesize) {
		printf("error: write size is out of page size!\n");
	}

	spi_nand_flash_write_one_page_cache_random(pbuf, len, info->write_ptr);
	info->write_ptr += len;
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
	data_array[0] = ((block_page_addr >> 16) & 0x0f);
	data_size = 3;

	wait_spi_busy();
	push_fifo_data(data_array, data_size, 0, TRUE);
	start_flash_operation(OPCODE_BLOCK_ERASE, 0, opmode, 0);
	wait_spi_busy();

	data_tmp_32 = get_flash_status(0);
	while (1) {
		if ((data_tmp_32 & 0x1) == 0) {
			if ((data_tmp_32 & 0x4) == 0x4) {
				printf("!!earse failed page addr  = %x \n",
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

static void nand_rda_do_cmd_post(struct mtd_info *mtd)
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
		printf("nand_rda_do_cmd_post: flash status = %x\n ", temp);
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

#ifdef NAND_DEBUG
	printf("nand_rda_cmdfunc, cmd = %x, page_addr = %x, col_addr = %x\n",
	       command, page_addr, column);
#endif

	/* remap command */
	switch (command) {
	case NAND_CMD_RESET:
		spi_nand_reset_flash();
		return;

	case NAND_CMD_READID:
		info->byte_buf[0] = get_flash_ID(0);
		info->byte_buf[1] = get_flash_ID(1);
		info->index = 0;
#ifdef NAND_DEBUG
		printf("READID: id = %x \n", info->byte_buf[0]);
#endif
		break;

	case NAND_CMD_SEQIN:	/* 0x80 do nothing, just only transfer write addr */
		info->page_addr = page_addr;
		info->col_addr = column;
		info->cmd = NAND_CMD_NONE;
		info->write_ptr = column;
		break;
	case NAND_CMD_READSTART:	/* hw auto gen 0x30 for read */
		break;

	case NAND_CMD_ERASE1:
		disable_flash_protection(0);
		if (NANDFC_SPI_ERASE_FAIL ==
		    spi_nand_flash_block_erase(page_addr))
			printf("spi nand: erase failed!page addr = %x\n",
			       page_addr);
		break;

	case NAND_CMD_ERASE2:
#ifdef NAND_DEBUG
		printf("erase block complete!\n");
#endif
		info->cmd_flag = NAND_CMD_ERASE2;
		info->cmd = NAND_CMD_NONE;
		break;

	case NAND_CMD_PAGEPROG:	/* 0x10 do the real program */
		disable_flash_protection(0);
		if (NANDFC_SPI_PROG_FAIL ==
		    spi_nand_flash_program_execute(info->page_addr))
			printf("spi nand: program failed!page addr = %x\n",
			       page_addr);
		info->cmd_flag = NAND_CMD_PAGEPROG;
		info->cmd = NAND_CMD_NONE;
		info->col_addr = 0;
		info->write_ptr = 0;

		//spi_nand_flash_cache_flush(mtd, info->page_addr);
		break;

	case NAND_CMD_READOOB:	/* Emulate NAND_CMD_READOOB */
		info->page_addr = page_addr;
		info->col_addr = column + mtd->writesize;
		info->cmd = NAND_CMD_READ0;
		/* Offset of oob */
		info->read_ptr = info->col_addr;
		/*read page data to cache */
		spi_nand_read_flash_data2buffer(mtd, page_addr);
		//spi_nand_flash_page_read2cache(page_addr);
		spi_nand_flash_read_from_cache(&g_spi_flash_read_temp_buffer
					       [mtd->writesize], mtd->writesize,
					       mtd->oobsize);

		/*gigadevice flash hardware reserved block, add to BBT, 0xffe0000, 0x1ffc0000 */
		if ((0xffe0000 == page_addr * mtd->writesize && info->type == SPINAND_TYPE_SPI2K)
		|| (0x1ffc0000 == info->page_addr * mtd->writesize && info->type == SPINAND_TYPE_SPI4K))
			memset(&g_spi_flash_read_temp_buffer[0], 0,
			       mtd->writesize + mtd->oobsize);

		info->byte_buf[0] =
		    g_spi_flash_read_temp_buffer[mtd->writesize];
		info->index = 0;

#ifdef NAND_DEBUG
		printf("read flash, middle buf-oob0:\n");
		rda_dump_buf((char *)
			     &g_spi_flash_read_temp_buffer[mtd->writesize],
			     mtd->oobsize);
#endif
		break;

	case NAND_CMD_READ0:	/* Emulate NAND_CMD_READOOB */
		info->read_ptr = 0;
		info->page_addr = page_addr;
#ifdef READ_PAGE_DATA_USE_MIDDLE_BUFFER
		/*read page data to cache */
		spi_nand_read_flash_data2buffer(mtd, page_addr);
		/*read OOB to cache */
		//spi_nand_flash_page_read2cache(page_addr);
		spi_nand_flash_read_from_cache(&g_spi_flash_read_temp_buffer
					       [mtd->writesize], mtd->writesize,
					       mtd->oobsize);

		/*gigadevice flash hardware reserved block, add to BBT, 0xffe0000, 0x1ffc0000 */
		if ((0xffe0000 == page_addr * mtd->writesize && info->type == SPINAND_TYPE_SPI2K)
		|| (0x1ffc0000 == info->page_addr * mtd->writesize && info->type == SPINAND_TYPE_SPI4K))
			memset(&g_spi_flash_read_temp_buffer[0], 0,
			       mtd->writesize + mtd->oobsize);
#endif/*READ_PAGE_DATA_USE_MIDDLE_BUFFER*/

#ifdef NAND_DEBUG
		printf("read flash, middle buf-oob1:\n");
		rda_dump_buf((char *)
			     &g_spi_flash_read_temp_buffer[mtd->writesize],
			     mtd->oobsize);
#endif
		break;

	default:
		info->page_addr = page_addr;
		info->col_addr = column;
		info->cmd = command;
		break;
	}

	if (info->cmd == NAND_CMD_NONE) {
		return;
	}

	nand_rda_do_cmd_post(mtd);
}

static int spi_nand_rda_dev_ready(struct mtd_info *mtd)
{
	return 1;
}

static void nand_rda_select_chip(struct mtd_info *mtd, int chip){}

static int spi_nand_rda_wait(struct mtd_info *mtd, struct nand_chip *chip)
{
#ifdef NAND_DEBUG
	printf("function: spi_nand_rda_wait\n");
#endif
	chip->cmdfunc(mtd, NAND_CMD_STATUS, -1, -1);

	return (int)chip->read_byte(mtd);
}

static int spi_nand_read_id(unsigned int id[2])
{
	id[0] = get_flash_ID(0);
	id[1] = get_flash_ID(0);

	printf("Nand ID: %08x %08x\n", id[0], id[1]);

	return 0;
}

static void read_ID(struct mtd_info *mtd)
{
	unsigned int id[2];

	spi_nand_read_id(id);
}

#define RDA_HW_CFG_BIT_7     (1 << 7)
#define RDA_HW_CFG_BIT_4     (1 << 4)
#define RDA_HW_CFG_BIT_3     (1 << 3)

#if defined(CONFIG_MACH_RDA8810)
static int spi_nand_get_type(struct rda_nand_info *info, unsigned int id[2],
			     int *rda_nand_type, int *bus_width_16)
{
	int metal_id;

	info->spl_adjust_ratio = 2;
	metal_id = rda_metal_id_get();
	printf("get type, metal %d\n", metal_id);

	if (metal_id >= 7) {
		if(id[0] == 0xc4c8 || id[0] == 0xd4c8){
			*rda_nand_type = SPINAND_TYPE_SPI4K;
			info->spl_adjust_ratio = 4; //actually ratio * 2
		}else{
			*rda_nand_type = SPINAND_TYPE_SPI2K;
		}

		*bus_width_16 = 0;
	} else {
		printf("invalid metal id %d\n", metal_id);
		return -ENODEV;
	}

	printf("SPL adjust ratio is %d. \n", info->spl_adjust_ratio);
	return 0;
}
#else /* 8810E, 8820, 8850 */
static int spi_nand_get_type(struct rda_nand_info *info, unsigned int id[2],
			     int *rda_nand_type, int *bus_width_16)
{
	int metal_id;

	info->spl_adjust_ratio = 2;
	metal_id = rda_metal_id_get();
	printf("get type, metal %d\n", metal_id);

	if(id[0] == 0xc4c8 || id[0] == 0xd4c8){
		*rda_nand_type = SPINAND_TYPE_SPI4K;
		info->spl_adjust_ratio = 4; //actually ratio * 2
	}else{
		*rda_nand_type = SPINAND_TYPE_SPI2K;
	}

	*bus_width_16 = 0;

	printf("SPL adjust ratio is %d. \n", info->spl_adjust_ratio);
	return 0;
}
#endif

static u32 spi_nand_cal_freq_by_divreg(u32 basefreq, u32 reg, u32 div2)
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

static unsigned long spi_nand_get_master_clk_rate(u32 reg)
{
	u32 div2;
	unsigned long rate;

	div2 = reg & SYS_CTRL_AP_AP_SFLSH_SRC_DIV2;
	reg = GET_BITFIELD(reg, SYS_CTRL_AP_AP_SFLSH_FREQ);
	rate = spi_nand_cal_freq_by_divreg(PLL_BUS_FREQ, reg, div2);

	return rate;
}

static unsigned char spi_nand_calc_divider(struct rda_nand_info *info)
{
	unsigned long mclk = info->master_clk;
	unsigned char div;
	unsigned long real_clk;

	div = mclk / info->clk;
	if (mclk % info->clk)
		div += 1;

	if (div < 2) {
		/* 2 is minimal divider by hardware */
		div = 2;
	}

	if (div > 255) {
		/* 255 is max divider by hardware */
		div = 255;
	}

	real_clk = mclk / div;
	printf("Spi nand real clock is %ld\n", real_clk);
	return div;
}

static unsigned char spi_nand_calc_delay(struct rda_nand_info *info)
{/*the value need check in future!*/
	unsigned char delay;

#ifndef _TGT_AP_SPI_NAND_READDELAY
	unsigned long mclk = info->master_clk;
	unsigned short clk_val;

	clk_val = mclk / 1000000;
	if (clk_val <= 220)
		delay = 2;
	else 	if (clk_val <= 280)
		delay = 3;
	else 	if (clk_val <= 350)
		delay = 4;
	else 	if (clk_val <= 420)
		delay = 5;
	else
		delay = 6;
#else
		delay = _TGT_AP_SPI_NAND_READDELAY;
#endif

	printf("read delay:%d\n", delay);
	return delay;
}

static int spi_nand_rda_init(struct nand_chip *this, struct rda_nand_info *info)
{
	unsigned int id[2];
	int ret = 0;
	int rda_nand_type = SPINAND_TYPE_INVALID;
	int bus_width_16 = 0;
	BOOL spi_nand_quad_mode = TRUE;
	unsigned char spi_read_delay = 2; /*AP_CLK_SFLSH clock cycles*/
	unsigned char spi_controller_div = 4; /*2~8*/

	ret = spi_nand_reset_flash();
	if (ret)
		return ret;
	ret = spi_nand_read_id(id);
	if (ret)
		return ret;
	ret = spi_nand_get_type(info, id, &rda_nand_type, &bus_width_16);
	if (ret)
		return ret;
	ret = init_spi_nand_info(info, rda_nand_type, bus_width_16);
	if (ret)
		return ret;
	if (info->bus_width_16)
		this->options |= NAND_BUSWIDTH_16;

	spi_controller_div = spi_nand_calc_divider(info);
	spi_read_delay = spi_nand_calc_delay(info);
	//nand_spi_init(1, 4, 3);
	//nand_spi_init(1, 2, 4);
	nand_spi_init(spi_nand_quad_mode, spi_read_delay, spi_controller_div);


	return 0;
}

static int spi_nand_rda_init_size(struct mtd_info *mtd, struct nand_chip *this,
				  u8 * id_data)
{
	struct rda_nand_info *info = this->priv;

	mtd->erasesize = info->vir_erase_size;
	mtd->writesize = info->vir_page_size;
	mtd->oobsize = info->vir_oob_size;

	return (info->bus_width_16) ? NAND_BUSWIDTH_16 : 0;
}

int rda_spi_nand_init(struct nand_chip *nand)
{
	struct rda_nand_info *info;
	static struct rda_nand_info rda_nand_info;

	info = &rda_nand_info;

	nand->chip_delay = 0;
#ifdef CONFIG_SYS_NAND_USE_FLASH_BBT
	nand->options |= NAND_USE_FLASH_BBT;
#endif
#ifdef NAND_DEBUG
	printf("SPINAND: nand_options = %x\n", nand->options);
#endif
	/*
	 * in fact, nand controler we do hardware ECC silently, and
	 * we don't need tell mtd layer, and will simple nand operations
	 */
	nand->ecc.mode = NAND_ECC_NONE;
	/* Set address of hardware control function */
	nand->cmd_ctrl = spi_nand_rda_hwcontrol;
	nand->init_size = spi_nand_rda_init_size;
	nand->cmdfunc = spi_nand_rda_cmdfunc;
	nand->read_byte = spi_nand_rda_read_byte;
	nand->read_word = spi_nand_rda_read_word;
	nand->read_buf = spi_nand_rda_read_buf;
	nand->write_buf = spi_nand_rda_write_buf;
	nand->dev_ready = spi_nand_rda_dev_ready;
	nand->waitfunc = spi_nand_rda_wait;
	nand->select_chip = nand_rda_select_chip;
	nand->read_nand_ID = read_ID;
	nand->priv = (void *)info;
	nand->IO_ADDR_R = (void __iomem *)SPI_NAND_READ_DATA_BUFFER_BASE;
	nand->IO_ADDR_W = (void __iomem *)SPI_NAND_WRITE_DATA_BUFFER_BASE;
	info->index = 0;
	info->write_ptr = 0;
	info->read_ptr = 0;
	info->cmd_flag = NAND_CMD_NONE;
	info->master_clk = spi_nand_get_master_clk_rate(_TGT_AP_CLK_SFLSH);
	info->clk = _TGT_AP_SPI_NAND_CLOCK;

#ifdef CONFIG_NAND_RDA_DMA
	rda_request_dma(&info->dma_ch);
#endif /* CONFIG_NAND_RDA_DMA */

	spi_nand_rda_init(nand, info);

	if (!nand->ecc.layout && (nand->ecc.mode != NAND_ECC_SOFT_BCH)) {
		switch (info->vir_oob_size) {
		case 64:
			nand->ecc.layout = &spi_nand_oob_64;
			break;
		case 128:
			nand->ecc.layout = &spi_nand_oob_128;
			break;
		default:
			printf("error oobsize: %d\n",
			       info->vir_oob_size);
		}
	}

	if (nand->options & NAND_USE_FLASH_BBT) {
		/* Use the default pattern descriptors */
		if (!nand->bbt_td) {
			nand->bbt_td = &spi_bbt_main_descr;
			nand->bbt_md = &spi_bbt_mirror_descr;
		}
	}

	printf("SPINAND: Nand Init Done\n");
	return 0;
}

#else
int rda_spi_nand_init(struct nand_chip *nand){return 0;}
#endif

#if 0
/* move to rda_nand_base.c */
int board_nand_init(struct nand_chip *chip) __attribute__ ((weak));

int board_nand_init(struct nand_chip *chip)
{
	return rda_spi_nand_init(chip);
}
#endif
