#include <common.h>
#include <asm/arch/rda_iomap.h>

#include "ddr3.h"
#include "ddr3_init.h"
#include "tgt_ap_clock_config.h"

#pragma GCC push_options
#pragma GCC optimize ("O0")

#define DDR_TYPE _TGT_AP_DDR_TYPE

#define DF_DELAY  (0x10000)

#define DMC_REG_BASE RDA_DMC400_BASE
#define PHY_REG_BASE RDA_DDRPHY_BASE

#define  PHY_RDLAT          0
#define  PHYWRDATA          1
#define  STA                3
#define  CLKSEL             4
#define  PSSTART            5
#define  PSDONE             6
#define  LOCKED             6
#define  CTRL_DELAY         7
#define  RDELAY_SEL         8
#define  WDELAY_SEL         9
#define  PHY_RESET          10
#define  RESET_DDR3         11
#define  ODT_DELAY          12
#define  DDR3_USED          13
#define  WRITE_ENABLE_LAT   14
#define  WRITE_DATA_LAT     15
#define  DQOUT_ENABLE_LAT   16
#define  DATA_ODT_ENABLE_REG	20
#define  DATA_WRITE_ENABLE_REG  48   
#define  DMC_READY	128
#define  USE_ADDR   129

#define ARRAY_NUM(n) (sizeof(n))/(sizeof(n[0]))

#if (DDR_TYPE == 1)
static dmc_reg_t dmc_reg_cfg[] = {
	{T_REFI,	0x186,		0x186},
	{T_RFC,		0x230024,	0x230024},
	{T_RFC,		0x540024,	0x540024},
	{T_MRR,		0x2,		0x2},
	{T_MRW,		0x5,		0x5},
	{T_RCD,		0x8,		0x8},
	{T_RAS,		0x11,		0x11},
	{T_RP,		0x8,		0x8},
	{T_RPALL,	0x9,		0x9},
	{T_RRD,		0x4,		0x4},
	{T_FAW,		0x14,		0x14},
	{T_RTR,		0x4,		0x4},
	{T_RTW,		0xb,		0xb},
	{T_RTP,		0x6,		0x6},
	{T_WR,		0xe,		0xe},
	{T_WTR,		0x4000b,	0x4000b},
	{T_WTR,		0x4000b,	0x4000b},
	{T_WTW,		0x40000,	0x40000},
	{RD_LATENCY,	0x6,		0x6},
	{WR_LATENCY,	0x3,		0x3},
	{T_RDDATA_EN,	0x3,		0x3},
	{T_PHYWRLAT,	0x101,		0x101},
	{T_PHYWRLAT,	0x101,		0x101},
	{T_XP,		0x20003,	0x20003},
	{T_XP,		0x80003,	0x80003},
	{T_ESR,		0x6,		0x6},
	{T_XSR,		0x1000058,	0x1000058},
	{T_XSR,		0x580058,	0x580058},
	{T_SRCKD,	0x5,		0x5},
	{T_CKSRD,	0x5,		0x5},
	{T_ECKD,	0x5,		0x5},
	{T_XCKD,	0x5,		0x5},
	{T_EP,		0x5,		0x5},
	{RDLVL_CTRL,	0x1010,		0x1010},
	{RDLVL_CTRL,	0x1110,		0x1110},
	{RDLVL_CTRL,	0x11110,	0x11110},
	{REFRESH_CTRL,	0x0,		0x0},
	{REFRESH_CTRL,	0x0,		0x0},
	{ECC_CTRL,	0x0,		0x0},
	{ADDR_CTRL,	0x30200,	0x30200},
	{ADDR_CTRL,	0x30200,	0x30200},
	{ADDR_CTRL,	0x30200,	0x30200},
};
#elif (DDR_TYPE == 2)
static dmc_reg_t dmc_reg_cfg[] = {
	{T_REFI,	0x3f,		0x3f},
	{T_RFC,		0x230024,	0x230024},
	{T_RFC,		0x540024,	0x540024},
	{T_MRR,		0x3f,		0x3f},
	{T_MRW,		0x3f,		0x3f},
	{T_RCD,		0x8,		0x8},
	{T_RAS,		0x11,		0x11},
	{T_RP,		0x8,		0x8},
	{T_RPALL,	0x9,		0x9},
	{T_RRD,		0x4,		0x4},
	{T_FAW,		0x14,		0x14},
	{T_RTR,		0xf,		0xf},
	{T_RTW,		0x1f,		0x1f},
	{T_RTP,		0x6,		0x6},
	{T_WR,		0xe,		0xe},
	{T_WTR,		0x4001f,	0x4001f},
	{T_WTR,		0x4001f,	0x4001f},
	{T_WTW,		0x40000,	0x40000},
	{RD_LATENCY,	0x8,		0x8},
	{WR_LATENCY,	0x4,		0x4},
	{T_RDDATA_EN,	0x8,		0x8},
	{T_PHYWRLAT,	0x103,		0x103},
	{T_PHYWRLAT,	0x103,		0x103},
	{T_XP,		0x20003,	0x20003},
	{T_XP,		0x80003,	0x80003},
	{T_ESR,		0x6,		0x6},
	{T_XSR,		0x1000058,	0x1000058},
	{T_XSR,		0x580058,	0x580058},
	{T_SRCKD,	0x5,		0x5},
	{T_CKSRD,	0x5,		0x5},
	{T_ECKD,	0x5,		0x5},
	{T_XCKD,	0x5,		0x5},
	{T_EP,		0x5,		0x5},
	{RDLVL_CTRL,	0x1010,		0x1010},
	{RDLVL_CTRL,	0x1110,		0x1110},
	{RDLVL_CTRL,	0x11110,	0x11110},
	{REFRESH_CTRL,	0x1,		0x1},
	{ECC_CTRL,	0x0,		0x0},
	{QOSX_CTRL7,	0x0f, 		0x0f},
	{TURNAROUND_PRIO,0x1A, 		0x1A},
	{HIT_PRIO,	0x1A, 		0x1A},
	{ADDR_CTRL,	0x30200,	0x30200},
	{ADDR_CTRL,	0x30200,	0x30200},
	{ADDR_CTRL,	0x30200,	0x30200},
};
#elif (DDR_TYPE == 3)
static dmc_reg_t dmc_reg_cfg[] = {
	{QOSX_CTRL7,	0x08, 		0x08},
	{TURNAROUND_PRIO,0x1A, 		0x0},
	{HIT_PRIO,	0x1A, 		0x0},
	{T_REFI,	0x186,		0x3f},
	{QUEUE_CTRL,	0xffff,		0x0},
	{T_RFC,		0x23008c,	0x23008c},
	{T_RFC,		0x8c008c,	0x8c008c},
	{T_MRR,		0x4,		0x4},
	{T_MRW,		0xc,		0xc},
	{T_RCD,		0x6,		0x6},
	{T_RAS,		0xf,		0xf},
	{T_RP,		0x6,		0x6},
	{T_RPALL,	0x6,		0x5},
	{T_RRD,		0x4,		0x4},
	{T_FAW,		0x14,		0x14},
	{T_RTR,		0x6,		0x6},
//	{T_RTW,		0x7,		0x6},
	{T_RTW,		0x0c,		0x6},// turn for now
	{T_RTW,		0x1f,		0x1f},//for low speed
	{T_RTP,		0x4,		0x4},
	{T_WR,		0xf,		0x10},
	{T_WTR,		0x4000d,	0x4000d},
	{T_WTR,		0x6000d,	0x6000f},
//	{T_WTR,		0x6001f,	0x6001f},//for low speed
	{T_WTW,		0x60000,	0x60000},
	{RD_LATENCY,	0x5,		0x5},
	{WR_LATENCY,	0x5,		0x5},
	{T_RDDATA_EN,	0x8,		0x3},
	{T_PHYWRLAT,	0x102,		0x102},
	{T_PHYWRLAT,	0x102,		0x103},
	{T_EP,		0x3,		0x3},
	{T_XP,		0x20003,	0x20003},
	{T_XP,		0xa0003,	0xa0003},
	{T_ESR,		0x4,		0x4},
	{T_XSR,		0x1000090,	0x1000090},
	{T_XSR,		0x2000090,	0x2000090},
	{T_SRCKD,	0x5,		0x5},
	{T_CKSRD,	0x5,		0x5},
	{T_ECKD,	0x5,		0x5},
	{T_XCKD,	0x5,		0x5},
	{ECC_CTRL,	0x0,		0x0},
	{ADDR_CTRL,	0x30200,	0x30200},
	{ADDR_CTRL,	0x30200,	0x30200},
	{ADDR_CTRL,	0x30200,	0x30200},
};
#else
#error "Wrong DDR Type"
#endif

void config_ddr_phy(UINT16 flag)
{
	int wait_idle;
	UINT16 __attribute__((unused)) dll_off;

	wait_idle = DF_DELAY + 16;
	dll_off = flag & DDR_FLAGS_DLLOFF;

	while (wait_idle--);

#if (DDR_TYPE == 1)
	*((volatile UINT32*)(PHY_REG_BASE + CTRL_DELAY * 4)) = 1;
	printf("ddr1 phy init done!\n");
#elif (DDR_TYPE ==2)
	*((volatile UINT32*)(PHY_REG_BASE + USE_ADDR * 4)) = 0;
	*((volatile UINT32*)(PHY_REG_BASE + DATA_WRITE_ENABLE_REG *4))  = 0xf;
	*((volatile UINT32*)(PHY_REG_BASE + DATA_ODT_ENABLE_REG *4))  = 0xf;
	*((volatile UINT32*)(PHY_REG_BASE + WRITE_DATA_LAT *4))  = 0x0;
	*((volatile UINT32*)(PHY_REG_BASE + WRITE_ENABLE_LAT *4))  = 0x0;
	*((volatile UINT32*)(PHY_REG_BASE + WDELAY_SEL *4))  = 0x7;
	*((volatile UINT32*)(PHY_REG_BASE + RDELAY_SEL *4))  = 0x6;
	*((volatile UINT32*)(PHY_REG_BASE + DQOUT_ENABLE_LAT *4))  = 0x1;
	printf("ddr2 phy init done!\n");
#else
	*((volatile UINT32*)(PHY_REG_BASE + DMC_READY *4))  = 0x0;
	*((volatile UINT32*)(PHY_REG_BASE + RESET_DDR3 * 4)) = 1;

	if (dll_off)
		*((volatile UINT32*)(PHY_REG_BASE + WRITE_DATA_LAT * 4)) = 1;

	*((volatile UINT32*)(PHY_REG_BASE + DDR3_USED * 4)) = 1;
	*((volatile UINT32*)(PHY_REG_BASE + WDELAY_SEL * 4)) = 3;

	if (!dll_off){
		*((volatile UINT32*)(PHY_REG_BASE + PHY_RDLAT * 4)) = 7;
		*((volatile UINT32*)(PHY_REG_BASE + CTRL_DELAY * 4)) =2;
		*((volatile UINT32*)(PHY_REG_BASE + WRITE_ENABLE_LAT * 4)) =2;
		*((volatile UINT32*)(PHY_REG_BASE + WRITE_DATA_LAT * 4)) = 3;
		*((volatile UINT32*)(PHY_REG_BASE + DQOUT_ENABLE_LAT * 4)) = 3;
	}else
		*((volatile UINT32*)(PHY_REG_BASE + DQOUT_ENABLE_LAT * 4)) = 1;

	*((volatile UINT32*)(PHY_REG_BASE + DATA_ODT_ENABLE_REG *4))  = 0xf;
	*((volatile UINT32*)(PHY_REG_BASE + DATA_WRITE_ENABLE_REG *4))  = 0xf;
	
	serial_puts("ddr3 phy init done!\n");
#endif
}

void config_dmc400(UINT16 flag, UINT32 para)
{
	int i, wait_idle;
	UINT32 val, tmp;
	volatile UINT32 *addr;
	UINT16 __attribute__((unused)) dll_off, lpw, odt, ron;
	UINT32 mem_width, col_bits, row_bits, bank_bits;

	dll_off = flag & DDR_FLAGS_DLLOFF;
	lpw = flag & DDR_FLAGS_LOWPWR;
	odt = (flag & DDR_FLAGS_ODT_MASK) >> DDR_FLAGS_ODT_SHIFT;
	ron = (flag & DDR_FLAGS_RON_MASK) >> DDR_FLAGS_RON_SHIFT;
	mem_width = (para & DDR_PARA_MEM_BITS_MASK) >> DDR_PARA_MEM_BITS_SHIFT;
	bank_bits = (para & DDR_PARA_BANK_BITS_MASK) >> DDR_PARA_BANK_BITS_SHIFT;
	row_bits = (para & DDR_PARA_ROW_BITS_MASK) >> DDR_PARA_ROW_BITS_SHIFT;
	col_bits = (para & DDR_PARA_COL_BITS_MASK) >> DDR_PARA_COL_BITS_SHIFT;


	wait_idle = 0x4e;
	while (wait_idle--);

	for (i=0; i<ARRAY_NUM(dmc_reg_cfg); i++) {
		addr = (volatile UINT32*)(DMC_REG_BASE + dmc_reg_cfg[i].reg_offset);
		if (dll_off)
			val = dmc_reg_cfg[i].dll_off_val;
		else
			val = dmc_reg_cfg[i].dll_on_val;

		*addr = val;
		wait_idle = 0x5;
		while (wait_idle--);
	}

#if (DDR_TYPE == 3)
	//config low power control register
	if (lpw) {
		addr = (volatile UINT32*)(DMC_REG_BASE + LOWPWR_CTRL);//0x20
		*addr = 0x48;
		wait_idle = 0x5;
		while (wait_idle--);

		addr = (volatile UINT32*)(DMC_REG_BASE + T_ESR);//0x260
		*addr = 0xa;
		wait_idle = 0x5;
		while (wait_idle--);

		addr = (volatile UINT32*)(DMC_REG_BASE + T_XSR);//0x264
		*addr = 0x200;
		wait_idle = 0x5;
		while (wait_idle--);

		addr = (volatile UINT32*)(DMC_REG_BASE + T_SRCKD);//0x268
		*addr = 0xa;
		wait_idle = 0x5;
		while (wait_idle--);

		addr = (volatile UINT32*)(DMC_REG_BASE + T_CKSRD);//0x26C
		*addr = 0xa;
		wait_idle = 0x5;
		while (wait_idle--);	
	}
#elif (DDR_TYPE == 2)
	addr = (volatile UINT32*)(DMC_REG_BASE + LOWPWR_CTRL);//0x20
	*addr = 0x38;
	wait_idle = 0x5;
	while (wait_idle--);
#endif

	//config format control register
	addr = (volatile UINT32*)(DMC_REG_BASE + FORMAT_CTRL);
	val = *addr;
	val &= ~0xf;
	if (mem_width == 2)
		val |= 0x2; //for 32bit DDR
	else if ((mem_width == 1) || (mem_width == 0))
		val |= 0x1; //for 16bit & 8bit DDR
	else
		printf("unsupported DDR width: %lx\n", mem_width);
	*addr = val;

	printf("format ctrl value: %lx\n", val);

	wait_idle = 0x1d;
	while (wait_idle--);

	//config address control register
	addr = (volatile UINT32*)(DMC_REG_BASE + ADDR_CTRL);
	if (mem_width == 2)
		tmp = col_bits; //for 32bit
	else if (mem_width == 1)
		tmp = col_bits - 1; //for 16bit
	else if (mem_width == 0)
		tmp = col_bits - 2; //for 8bit
	else {
		printf("unsupported DDR width: %lx\n", mem_width);
		tmp = col_bits;
	}
	val = (bank_bits << 16) | (row_bits << 8) | tmp;

	*addr = val;

	printf("address ctrl value: %lx\n", val);
	wait_idle = 0x5;
	while (wait_idle--);

	//config decode control register
	addr = (volatile UINT32*)(DMC_REG_BASE + DECODE_CTRL);
	tmp = 12 - (8 + col_bits + mem_width);
	if (tmp < 8)
		val = tmp << 4;
	else
		printf("unsupported stripe decode: %lx\n", tmp);
	*addr = val;

	printf("decode ctrl value: %lx\n", val);
	wait_idle = 0x6;
	while (wait_idle--);

	wait_idle = DF_DELAY;
	while (wait_idle--);
	addr = (volatile UINT32*)(DMC_REG_BASE + DIRECT_CMD);
	*addr = 0x0;
	wait_idle = DF_DELAY + 0x10;
	while (wait_idle--);

	addr = (volatile UINT32*)(DMC_REG_BASE + DIRECT_CMD);
#if (DDR_TYPE == 1)
	val = 0;
#elif (DDR_TYPE == 2)
	val = 0;
#else
	//config MR2
	if (dll_off)
		val = 0x10020008;
	else
		val = 0x10020000;
#endif
	*addr = val;
	wait_idle = DF_DELAY + 0x10;
	while (wait_idle--);

	addr = (volatile UINT32*)(DMC_REG_BASE + DIRECT_CMD);
#if (DDR_TYPE == 1)
	val = 0x20000000;
#elif (DDR_TYPE == 2)
	val = 0x1000003f;
#else
	//config MR3
	val = 0x10030000;
#endif
	*addr = val;
	wait_idle = DF_DELAY + 0x10;
	while (wait_idle--);

	addr = (volatile UINT32*)(DMC_REG_BASE + DIRECT_CMD);
#if (DDR_TYPE == 1)
	val = 0x30000000;
#elif (DDR_TYPE == 2)
	val = 0x1000ff0a;
#else
	//config MR1 ODT [9,6,2] RON [5,1] DLLOFF [0]
	val = 0x10010000;
	val |= (odt & 0x4)<<7 | (odt & 0x2)<<5 | (odt & 0x1)<<2;
	val |= (ron & 0x2)<<4 | (ron & 0x1)<<1;
	if (dll_off)
		val |= 0x1;
#endif
	*addr = val;
	wait_idle = DF_DELAY + 0x10;
	while (wait_idle--);

	addr = (volatile UINT32*)(DMC_REG_BASE + DIRECT_CMD);
#if (DDR_TYPE == 1)
	val = 0x30000000;
#elif (DDR_TYPE == 2)
	val = 0x10008301;
#else
	//config MR0
	val = 0x10000520;
#endif
	*addr = val;
	wait_idle = DF_DELAY + 0x10;
	while (wait_idle--);

	addr = (volatile UINT32*)(DMC_REG_BASE + DIRECT_CMD);
#if (DDR_TYPE == 1)
	val = 0x10000033;
#elif (DDR_TYPE == 2)
	val = 0x10000602;//0ld 0x10000102
#else
	val = 0x50000400;
#endif
	*addr = val;
	wait_idle = DF_DELAY + 0x10;
	while (wait_idle--);

#if (DDR_TYPE == 2)
	addr = (volatile UINT32*)(DMC_REG_BASE + DIRECT_CMD);
	val = 0x10000103;//0ld 0x10000102
	*addr = val;
	wait_idle = DF_DELAY + 0x10;
	while (wait_idle--);
#endif

	addr = (volatile UINT32*)(DMC_REG_BASE + DIRECT_CMD);
#if (DDR_TYPE == 1)
	val = 0x10020000;
#elif (DDR_TYPE == 2)
	val = 0x30000000;
#else
	val = 0x30000000;
#endif
	*addr = val;
	wait_idle = DF_DELAY + 0x10;
	while (wait_idle--);

#if (DDR_TYPE == 2)
/* zhangli for MCP */
	addr = (volatile UINT32*)(DMC_REG_BASE + DIRECT_CMD);
	*addr = 0x10000103;
	wait_idle = DF_DELAY + 0x9;
	while (wait_idle--);
#endif

#if (DDR_TYPE == 2)
	addr = (volatile UINT32*)(DMC_REG_BASE + MEMC_CMD);
	val = 0x4;
	*addr = val;
#endif

	//config ECC control
	wait_idle = 0x6;
	while (wait_idle--);
	addr = (volatile UINT32*)(DMC_REG_BASE + ECC_CTRL);
	val = 0x0;
	*addr = val;
	wait_idle = 0x8;
	while (wait_idle--);

	//config MEMC CMD
	addr = (volatile UINT32*)(DMC_REG_BASE + MEMC_CMD);
	val = 0x3;
	*addr = val;

	//READ MEMC STATUS
	addr = (volatile UINT32*)(DMC_REG_BASE + MEMC_STATUS);
	val = 0;
	do {
		val = *addr;
		//printf("MEMC_STATUS value: %x\n", val);
	} while (val != 3);
}

void axi_prio_init(void)
{
	*(volatile UINT32*)(0x20900100) = 0xaa0000aa;
	*(volatile UINT32*)(0x21054100) = 0x00000008;
	*(volatile UINT32*)(0x21054104) = 0x00000008;
}

void axi_outstandings_init(void)
{
	*(volatile UINT32*)(0x2104210C) = 0x00000060;
	*(volatile UINT32*)(0x21042110) = 0x00000300;
}

int ddr_init(UINT16 flags, UINT32 para)
{
	UINT16 dll_off;
	UINT32 mem_width;

	axi_prio_init();
	//axi_outstandings_init();

	dll_off = flags & DDR_FLAGS_DLLOFF;
	mem_width = (para & DDR_PARA_MEM_BITS_MASK) >> DDR_PARA_MEM_BITS_SHIFT;

	switch (mem_width) {
		case 0:
			printf("8bit ");
			break;
		case 1:
			printf("16bit ");
			break;
		case 2:
			printf("32bit ");
			break;
	}
	if (dll_off)
		printf("dll-off Mode ...\n");
	else
		printf("dll-on Mode ...\n");

	config_ddr_phy(flags);
	config_dmc400(flags, para);
	printf("dram init done ...\n");

	return 0;
}

