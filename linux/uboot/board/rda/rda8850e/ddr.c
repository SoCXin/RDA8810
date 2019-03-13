#include <common.h>
#include <asm/arch/rda_iomap.h>
#include <asm/io.h>
#include "tgt_ap_clock_config.h"

#define DDR_TYPE	_TGT_AP_DDR_TYPE
#if (DDR_TYPE == 2)
#include "lpddr2_init.h"
#elif (DDR_TYPE == 3)
#include "ddr3_init.h"
#else
#error "_TGT_AP_DDR_TYPE is not valid !"
#endif
#include <asm/arch/rda_sys.h>

#pragma GCC push_options
#pragma GCC optimize ("O0")

#define DF_DELAY  (0x10000)

#define DMC_REG_BASE RDA_DMC400_BASE
#define PHY_REG_BASE RDA_DDRPHY_BASE

/*To test DDR self-refresh timing */
#ifdef TGT_AP_SET_DDR_LP_PARAM
#define SEL_DDR_REG_ESR_TAB_SIZE 16
#define SEL_DDR_REG_NUMBER 4

enum DMC_STATE_VALUE
{
	DMC_STATE_CONFIG = 0X0,
	DMC_STATE_LOW_POWER = 0X1,
	DMC_STATE_PAUSED = 0X2,
	DMC_STATE_READY = 0X03,
};
enum SEL_DDR_REG_INDEX
{
	DDR_REG_IDX_ESR = 0,
	DDR_REG_IDX_XSR,
	DDR_REG_IDX_XRCKD,
	DDR_REG_IDX_CKSRD,
};
typedef struct
{
	const char *sel_str;
	UINT32 sel_val;
}sel_ddr_reg_val_item_t;

const static sel_ddr_reg_val_item_t sel_ddr_reg_esr_tab[] = {
	{"0: 0x00",0x00},{"1: 0x01",0x01},{"2: 0x02",0x02},{"3: 0x03",0x03},
	{"4: 0x04",0x04},{"5: 0x05",0x05},{"6: 0x06",0x06},{"7: 0x07",0x07},
	{"8: 0x08",0x08},{"9: 0x09",0x09},{"a: 0x0A",0x0A},{"b: 0x0B",0x0B},
	{"c: 0x0C",0x0C},{"d: 0x0D",0x0D},{"e: 0x0E",0x0E},{"f: 0x0F",0x0F},
};
const static sel_ddr_reg_val_item_t sel_ddr_reg_xsr_tab[] = {
	{"0: 0x1D8",0x1D8},
	{"1: 0x1DC",0x1DC},
	{"2: 0x1E0",0x1E0},
	{"3: 0x1E4",0x1E4},
	{"4: 0x1E8",0x1E8},
	{"5: 0x1EC",0x1EC},
	{"6: 0x1F0",0x1F0},
	{"7: 0x1F4",0x1F4},
	{"8: 0x1F8",0x1F8},
	{"9: 0x1FA",0x1FA},
	{"a: 0x1FB",0x1FB},
	{"b: 0x1FC",0x1FC},
	{"c: 0x1FD",0x1FD},
	{"d: 0x1FE",0x1FE},
	{"e: 0x1FF",0x1FF},
	{"f: 0x200",0x200},
};
const static sel_ddr_reg_val_item_t sel_ddr_reg_xrckd_tab[] = {
	{"0: 0x00",0x00},{"1: 0x01",0x01},{"2: 0x02",0x02},{"3: 0x03",0x03},
	{"4: 0x04",0x04},{"5: 0x05",0x05},{"6: 0x06",0x06},{"7: 0x07",0x07},
	{"8: 0x08",0x08},{"9: 0x09",0x09},{"a: 0x0A",0x0A},{"b: 0x0B",0x0B},
	{"c: 0x0C",0x0C},{"d: 0x0D",0x0D},{"e: 0x0E",0x0E},{"f: 0x0F",0x0F},
};

typedef struct {
	const char *name_str;
	unsigned int offset_val;
}sel_ddr_reg_info_item_t;

const static sel_ddr_reg_info_item_t sel_ddr_reg_tab[] = {
	{"enter self-refresh timing",0x260,},
	{"exit self-refresh timing",0x264,},
	{"self-refresh to DRAM clock disable timing",0x268,},
	{"DRAM clock enable to exist self-refresh timing",0x26C,},
};
typedef struct
{
	unsigned int stop_mem_clock_idle:1; //bit0
	unsigned int stop_mem_clock_sref:1; //bit1
	unsigned int auto_power_down	:1; //bit2
	unsigned int auto_self_refresh	:1; //bit3
	unsigned int asr_period		:4; //bit4-bit7
} ddr_lp_cntl_reg_bit_t;
typedef union
{
	unsigned int value;
	ddr_lp_cntl_reg_bit_t bit;
} ddr_lp_cntl_reg_t;

static int _ddr_config_low_power_mode(void)
{
	char choice = 'n';
	int wait_idle = 0;
	unsigned int dmc_state = 0,temp;
	ddr_lp_cntl_reg_t lp_reg,lp_reg_set;

	/* Print message for selection */
	printf("\nConfigure LOWPWR_CTRL register manully?(y=yes,n=no)\n");
	choice = serial_getc();
	if((choice != 'y') && (choice != 'Y')) {
		printf("Using default value for DDR configuration\n");
		return -1;
	}
LAB_CONFIG:
	/* initiale value */
	dmc_state = 0;
	temp = 0;
	lp_reg.value = 0;
	lp_reg_set.value = 0;

	/* check dmc current state */
	dmc_state = readl(DMC_REG_BASE + MEMC_STATUS);
	if(dmc_state != DMC_STATE_CONFIG) {
		printf("DMC is not in CONFIG state, current state value: 0x%x,exit\n",dmc_state);
		return -1;
	}
	/* read current low power configuration value */
	lp_reg_set.value = 0;
	lp_reg.value = readl(DMC_REG_BASE + LOWPWR_CTRL);
	printf("Current LOWPWR_CTRL register value: 0x%x\n",lp_reg.value);
	/* set stop_mem_clock_idle */
#if 0
	/* In RDA8850E, the DMC400 do not support auto power down mode,
	   so
		stop_mem_clock_idle = 0,
		stop_mem_clock_sref = 0,
		auto_power_down = 0
	*/
	lp_reg_set.bit.stop_mem_clock_idle = 0;
	lp_reg_set.bit.stop_mem_clock_sref = 0;
	lp_reg_set.bit.auto_power_down = 0;
	lp_reg_set.bit.auto_self_refresh = 1; //enable auto self refresh mode
#else
	printf("Enable or disable stop_mem_clock_idle ? (e/d)\n");
	choice = serial_getc();
	printf("Your choice is: %c\n",choice);
	if((choice == 'e')||(choice == 'E')) {
		lp_reg_set.bit.stop_mem_clock_idle = 1;
	} else {
		lp_reg_set.bit.stop_mem_clock_idle = 0;
	}
	/* set stop_mem_clock_sref */
	printf("Enable or disable stop_mem_clock_sref ? (e/d)\n");
	choice = serial_getc();
	printf("Your choice is: %c\n",choice);
	if((choice == 'e')||(choice == 'E')) {
		lp_reg_set.bit.stop_mem_clock_sref = 1;
	} else {
		lp_reg_set.bit.stop_mem_clock_sref = 0;
	}
	/* set auto_power_down */
	printf("Enable or disable auto_power_down ? (e/d)\n");
	choice = serial_getc();
	printf("Your choice is: %c\n",choice);
	if((choice == 'e')||(choice == 'E')) {
		lp_reg_set.bit.auto_power_down = 1;
	} else {
		lp_reg_set.bit.auto_power_down = 0;
	}
	/* set auto_self_refresh */
	printf("Enable or disable auto_self_refresh ? (e/d)\n");
	choice = serial_getc();
	printf("Your choice is: %c\n",choice);
	if((choice == 'e')||(choice == 'E')) {
		lp_reg_set.bit.auto_self_refresh = 1;
	} else {
		lp_reg_set.bit.auto_self_refresh = 0;
	}
#endif
	/* set asr period */
	printf("Set asr_period: 0,1,2,3,4,5,6,7,8,9,a,b,c,d,e,f\n");
	choice = serial_getc();
	printf("Your choice is: %c\n",choice);
	if((choice >= 'a')&&(choice <= 'f'))
		choice -= 0x20;
	if((choice >= 'A') && (choice <= 'F'))
		choice -=0x37;
	else
		choice -=0x30;

	temp = (unsigned int )choice;
	printf("temp: 0x%x\n",temp);
	if(temp > 0x0F) {
		printf("Input data error,use default value 0x04\n");
		temp = 0x04;
	}
	lp_reg_set.bit.asr_period = temp;
	/* be sure to write */
	printf("LOWPWR_CTRL register's setting value: 0x%x\n",lp_reg_set.value);
	printf("Write to register ? (y/n)\n");
	choice = serial_getc();
	if((choice != 'y')&&(choice != 'Y')) {
		printf("Writing cancelled,exit\n");
		goto LAB_RECONFIG;
	}
	/* write register */
	printf("Writing register...");
	writel(lp_reg_set.value, DMC_REG_BASE + LOWPWR_CTRL);
	wait_idle = DF_DELAY + 0x10;
	while (wait_idle--);
	printf("address is 0x%x, value is 0x%x\n",LOWPWR_CTRL,lp_reg_set.value);
	/* read resigter */
	printf("Reading register...");
	temp = readl(DMC_REG_BASE + LOWPWR_CTRL);
	printf("address is 0x%x, value is 0x%x\n",LOWPWR_CTRL,temp);
LAB_RECONFIG:
	printf("\nRe-configure LOWPWR_CTRL register ? (y/n)\n");
	choice = serial_getc();
	if((choice == 'y') || (choice == 'Y')) {
		goto LAB_CONFIG;
	}
	printf("Configure LOWPWR_CTRL register success !!!\n");
	return 0;
}
static int _ddr_set_refresh_timing(void)
{
	char choice = 'n';
	int try_times = 0,i,idx = 0,wait_idle = 0;
	unsigned int temp,reg_val[SEL_DDR_REG_NUMBER] = {0};
	unsigned int dmc_status = 0;

	dmc_status = dmc_status;
	/* Print message for selection */
	printf("Configure DDR parameters manully?(y=yes,n=no)\n");
	choice = serial_getc();
	if((choice != 'y') && (choice != 'Y')) {
		printf("Using default value for DDR configuration\n");
		return 0;
	}
	idx = 0;
LAB_INPUT:
	try_times = 0;
	while(1) {
		/* get current register value */
		temp = readl(DMC_REG_BASE + sel_ddr_reg_tab[idx].offset_val);
		printf("\nConfigure %s parameter, current value is: 0x%x\n",
			sel_ddr_reg_tab[idx].name_str, temp);
		/* print selection table */
		switch(idx){
		case DDR_REG_IDX_ESR:
			for( i = 0; i < SEL_DDR_REG_ESR_TAB_SIZE; i++)
				printf("\t%s\n",sel_ddr_reg_esr_tab[i].sel_str);
		break;
		case DDR_REG_IDX_XSR:
			for( i = 0; i < SEL_DDR_REG_ESR_TAB_SIZE; i++)
				printf("\t%s\n",sel_ddr_reg_xsr_tab[i].sel_str);
		break;
		case DDR_REG_IDX_XRCKD:
		case DDR_REG_IDX_CKSRD:
			for( i = 0; i < SEL_DDR_REG_ESR_TAB_SIZE; i++)
				printf("\t%s\n",sel_ddr_reg_xrckd_tab[i].sel_str);
		break;
		default:
			printf("print selection table error,idx = %d\n",idx);
			return -1;
		}
		/* input selection :[0,9],[A,F],[a,f] */
		choice = serial_getc();
		try_times++;
		if((choice < '0')
			||((choice > '9')&&(choice < 'A'))
			||((choice > 'F')&&(choice < 'a'))
			||(choice > 'f')) {
			printf("Your input:%c is invalid,asc is 0x%x\n",choice,choice);
			if(try_times > 3)
				break;
			else
				continue;
		}
		break;
	}
	if(try_times > 3) {
		printf("Invalid input over 3 times ,exit\n");
		return -1;
	}
	printf("debug: choice is 0x%x\n",choice);
	if((choice >= 'a')&&(choice <= 'f'))
		choice -= 0x20;
	if((choice >= 'A') && (choice <= 'F'))
		choice -=0x37;
	else
		choice -=0x30;

	if(choice >= SEL_DDR_REG_ESR_TAB_SIZE) {
		printf("Input error choice 0x%x, exit\n",choice);
		return -1;
	}
	i = (int)(choice);
	switch(idx) {
	case DDR_REG_IDX_ESR:
		reg_val[idx] = sel_ddr_reg_esr_tab[i].sel_val;
	break;
	case DDR_REG_IDX_XSR:
		reg_val[idx] = sel_ddr_reg_xsr_tab[i].sel_val;
	break;
	case DDR_REG_IDX_XRCKD:
	case DDR_REG_IDX_CKSRD:
		reg_val[idx] = sel_ddr_reg_xrckd_tab[i].sel_val;
	break;
	default:
		printf("read selection table error,idx = %d\n",idx);
		return -1;
	break;
	}
	printf("Register offset: 0x%x, value: 0x%x\n",
		sel_ddr_reg_tab[idx].offset_val,reg_val[idx]);
	idx++;
	if(idx < SEL_DDR_REG_NUMBER)
		goto LAB_INPUT;
	/* List all selection */
	printf("\nList your selection:\n");
	for(i = 0; i < SEL_DDR_REG_NUMBER; i++)
		printf("\t%s, value: 0x%x\n",sel_ddr_reg_tab[i].name_str, reg_val[i]);

	/* Request writing decision */
	printf("\nWrite to register ?(y=yes,n=no)\n");
	choice = serial_getc();
	if((choice != 'y') && (choice != 'Y')) {
		printf("Using default value for DDR configuration\n");
		return 0;
	}
	/* write to register */
	printf("Writing DMC register...\n");
	for(i = 0; i < SEL_DDR_REG_NUMBER; i++){
		writel(reg_val[i],DMC_REG_BASE + sel_ddr_reg_tab[i].offset_val);
		wait_idle = DF_DELAY + 0x10;
		while (wait_idle--);
		printf("Address: 0x%x, value: 0x%x\n",
			sel_ddr_reg_tab[i].offset_val, reg_val[i]);
	}
	/* read from register */
	printf("Reading DMC register...\n");
	for( i = 0;i < SEL_DDR_REG_NUMBER; i++) {
		temp = readl(DMC_REG_BASE + sel_ddr_reg_tab[i].offset_val);
		wait_idle = DF_DELAY + 0x10;
		while (wait_idle--);
		printf("Address: 0x%x, value: 0x%x\n",
			sel_ddr_reg_tab[i].offset_val, temp);
	}
	/* Finish configuration */
	printf("Configure DDR parameter successful !!!.\n");
	return 0;
}

#endif /* TGT_AP_SET_DDR_LP_PARAM */

/* This macro is used to test DDR */
#ifdef TGT_AP_DO_DDR_TEST
unsigned int g_ddr_manufacture_id = 0;
unsigned int g_ddr_capacity_id = 0;
#endif /* TGT_AP_DO_DDR_TEST */

typedef struct
{
	unsigned char val;
	const char *str;
}dram_info_t;

const static dram_info_t dram_manu[] =
{
	{0x01,"Samsung"},
	{0x03,"Elpida"},
	{0x05,"Nanya"},
	{0x06,"Hynix"},
	{0x08,"Winbond"},
	{0x0E,"Intel"},
	{0xFF,"Micron"},
};

const static char *dram_density[] =
{
	"64Mb",
	"128Mb",
	"256Mb",
	"512Mb",
	"1Gb",
	"2Gb",
	"4Gb",
	"8Gb",
	"16Gb",
	"32Gb",
};

const static char *dram_type[] =
{
	"S4 SDRAM",
	"S2 SDRAM",
	"N NVM",
	"Reserved",
};

const static int dram_io_width[] =
{
	32,
	16,
	8,
	0,
};

unsigned int con_dram_mrdata(unsigned int val)
{
	unsigned int t = 0;
	/* order: 7 6 5 4 3 2 1 0 -> 7 3 1 6 5 4 2 0 */
	t |= (val & 0x80);
	t |= (val & 0x40)>>2;
	t |= (val & 0x20)>>2;
	t |= (val & 0x10)>>2;
	t |= (val & 0x08)<<3;
	t |= (val & 0x04)>>1;
	t |= (val & 0x02)<<4;
	t |= (val & 0x01);
	return t;
}

void show_dram_info(void)
{
	int wait_idle,i,num;
	unsigned int mr5,mr8,status;
	unsigned char manu_idx,den_idx,width_idx,type_idx;
	/* read dmc status */
	status = readl(DMC_REG_BASE+MEMC_STATUS);
	if(status != 0) {
		return;
	}
	/* read MR5, manufacture id */
	writel(0x60000005,DMC_REG_BASE+DIRECT_CMD);
	wait_idle = DF_DELAY+0xFF;
	while(wait_idle--);
	mr5 = readl(DMC_REG_BASE + MR_DATA);
	/* read MR8, capacity id */
	writel(0x60000008,DMC_REG_BASE+DIRECT_CMD);
	wait_idle = DF_DELAY+0xFF;
	while(wait_idle--);
	mr8 = readl(DMC_REG_BASE + MR_DATA);
#ifdef TGT_AP_DO_DDR_TEST
	g_ddr_manufacture_id = mr5;
	g_ddr_capacity_id = mr8;
#endif /* TGT_AP_DO_DDR_TEST */
	/* to determine wether to transform bit order */
	width_idx = (mr8 >> 6) & 0x3;
	den_idx = (mr8 >> 2) & 0xF;
	if((width_idx >= 0x1) || (den_idx < 0x4)) {
		mr5 = con_dram_mrdata(mr5);
		mr8 = con_dram_mrdata(mr8);
	}
	/* parse dram info */
	manu_idx = mr5 & 0xFF;
	num = ARRAY_SIZE(dram_manu);
	for(i = 0;i < num;i++) {
		if(dram_manu[i].val == manu_idx)
			break;
	}
	if(i >= num) {
		printf("unknown manu id %x\n",mr5);
		return;
	}
	manu_idx = i;
	type_idx = mr8 & 0x3;
	den_idx = (mr8 >> 2) & 0xF;
	width_idx = (mr8 >> 6) & 0x3;
	num = ARRAY_SIZE(dram_density);
	if(den_idx >= num) {
		printf("unknown density %x\n",mr8);
		return;
	}
	printf("dram info: %s, %s, %s, x%d\n",
		dram_manu[manu_idx].str,
		dram_type[type_idx],
		dram_density[den_idx],
		dram_io_width[width_idx]);
}

#ifdef DO_DDR_PLL_DEBUG
extern void cmd_input(char * pname, UINT32 reg_base);
#endif /* DO_DDR_PLL_DEBUG */

void config_ddr_phy(UINT16 flag)
{
	UINT16 __attribute__((unused)) dll_off;
	dll_off = flag & DDR_FLAGS_DLLOFF;

#if (DDR_TYPE == 2) /* LPDDR2 */
	int wait_idle, i, num;
	UINT32 val;
	u16 metal_id = rda_metal_id_get();
	dmc_reg_t *dmc_digphy_config_ptr;

	wait_idle = DF_DELAY + 16;
	while (wait_idle--);

	if (metal_id < 2) {
		dmc_digphy_config_ptr = (dmc_reg_t *)dmc_digphy_config;
		num = ARRAY_SIZE(dmc_digphy_config);
	} else {
		dmc_digphy_config_ptr = (dmc_reg_t *)dmc_digphy_eco2_config;
		num = ARRAY_SIZE(dmc_digphy_eco2_config);
	}

	for (i = 0; i < num; i++) {
		if (dll_off)
			val = dmc_digphy_config_ptr[i].dll_off_val;
		else
			val = dmc_digphy_config_ptr[i].dll_on_val;

		writel(val, PHY_REG_BASE + dmc_digphy_config_ptr[i].reg_offset);
		//printf("reg = %x, val = %x \n",PHY_REG_BASE + dmc_digphy_config_ptr[i].reg_offset, val );
	}
#else /* DDR3 */
	writel(0x00,PHY_REG_BASE + DMC_READY *4);
	writel(0x01,PHY_REG_BASE + RESET_DDR3 *4);

	if (dll_off)
		writel(0x01,PHY_REG_BASE + WRITE_DATA_LAT *4);

	writel(0x01,PHY_REG_BASE + DDR3_USED *4);
	writel(0x03,PHY_REG_BASE + WDELAY_SEL *4);

	if (!dll_off){
		writel(0x07,PHY_REG_BASE + PHY_RDLAT *4);
		writel(0x02,PHY_REG_BASE + CTRL_DELAY *4);
		writel(0x02,PHY_REG_BASE + WRITE_ENABLE_LAT *4);
		writel(0x03,PHY_REG_BASE + WRITE_DATA_LAT *4);
		writel(0x03,PHY_REG_BASE + DQOUT_ENABLE_LAT *4);
	}else
		writel(0x01,PHY_REG_BASE + DQOUT_ENABLE_LAT *4);

	writel(0x0e,PHY_REG_BASE + DATA_ODT_ENABLE_REG *4);
	writel(0x0f,PHY_REG_BASE + DATA_WRITE_ENABLE_REG *4);

#endif
	printf("ddr %d phy init done!\n",(int)DDR_TYPE);
}

void config_dmc400(UINT16 flag, UINT32 para)
{
	int i, wait_idle;
	UINT32 val, tmp;
	UINT16 __attribute__((unused)) dll_off, lpw;
	UINT32 mem_width, col_bits, row_bits, bank_bits;
#if (DDR_TYPE == 3)
	UINT16 odt,ron;

	odt = (flag & DDR_FLAGS_ODT_MASK) >> DDR_FLAGS_ODT_SHIFT;
	ron = (flag & DDR_FLAGS_RON_MASK) >> DDR_FLAGS_RON_SHIFT;
#endif
	// parse parameter
	dll_off = flag & DDR_FLAGS_DLLOFF;
	lpw = flag & DDR_FLAGS_LOWPWR;
	mem_width = (para & DDR_PARA_MEM_BITS_MASK) >> DDR_PARA_MEM_BITS_SHIFT;
	bank_bits = (para & DDR_PARA_BANK_BITS_MASK) >> DDR_PARA_BANK_BITS_SHIFT;
	row_bits = (para & DDR_PARA_ROW_BITS_MASK) >> DDR_PARA_ROW_BITS_SHIFT;
	col_bits = (para & DDR_PARA_COL_BITS_MASK) >> DDR_PARA_COL_BITS_SHIFT;

	wait_idle = 0x4e;
	while (wait_idle--);
	//config MEMC CMD
	writel(0x0, DMC_REG_BASE + MEMC_CMD);

	//READ MEMC STATUS
	val = 0;
	do {
		val = readl(DMC_REG_BASE + MEMC_STATUS);
		//printf("MEMC_STATUS value: %x\n", val);
	} while (val != 0);

	for (i = 0; i < ARRAY_SIZE(dmc_reg_cfg); i++) {
		if (dll_off)
			val = dmc_reg_cfg[i].dll_off_val;
		else
			val = dmc_reg_cfg[i].dll_on_val;

		writel(val, DMC_REG_BASE + dmc_reg_cfg[i].reg_offset);
		wait_idle = 0x5;
		while (wait_idle--);
		/*printf("dmc reg 0x%x = 0x%x\n",
			(unsigned int)(dmc_reg_cfg[i].reg_offset),(unsigned int)val);*/
	}
	//config low power control register
	if (lpw) {
		printf("enter low power mode \n");
		for (i = 0; i < ARRAY_SIZE(dmc_reg_low_power_cfg); i++) {
			writel(dmc_reg_low_power_cfg[i].dll_off_val,
				DMC_REG_BASE + dmc_reg_low_power_cfg[i].reg_offset);
			wait_idle = 0x5;
			while (wait_idle--);
		}
	}
#ifdef DO_DDR_PLL_DEBUG
	cmd_input("Dmc400",DMC_REG_BASE);
#endif
	//config format control register
	val = readl(DMC_REG_BASE + FORMAT_CTRL);
	val &= ~0xf;
	if (mem_width == 2)
		val |= 0x2; //for 32bit DDR
	else if ((mem_width == 1) || (mem_width == 0))
		val |= 0x1; //for 16bit & 8bit DDR
	else
		printf("unsupported DDR width: %lx\n", mem_width);
	writel(val, DMC_REG_BASE + FORMAT_CTRL);

	printf("format ctrl value: %lx\n", val);

	wait_idle = 0x1d;
	while (wait_idle--);

	//config address control register
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
	writel(val, DMC_REG_BASE + ADDR_CTRL);

	printf("address ctrl value: %lx\n", val);

	wait_idle = 0x5;
	while (wait_idle--);

	//config decode control register
	tmp = 12 - (8 + col_bits + mem_width);
	if (tmp < 8)
		val = tmp << 4;
	else
		printf("unsupported stripe decode: %lx\n", tmp);
	writel(val, DMC_REG_BASE + DECODE_CTRL);

	printf("decode ctrl value: %lx\n", val);

	wait_idle = 0x6;
	while (wait_idle--);
#if (DDR_TYPE == 2)
	//config direct command
	for (i = 0; i < ARRAY_SIZE(ddr_config); i++) {
		mdelay(1);
		wait_idle = DF_DELAY + 0x10;
		while (wait_idle--);
		writel(ddr_config[i].dll_off_val, DMC_REG_BASE + ddr_config[i].reg_offset);
/*		printf("reg 0x%x = 0x%x\n",
			(unsigned int)(ddr_config[i].reg_offset),
			(unsigned int)(ddr_config[i].dll_off_val));*/
	}
#else
	//direct cmd ddr3
	for (i = 0; i < ARRAY_SIZE(ddr_config); i++) {
		mdelay(1);
		wait_idle = DF_DELAY + 0x10;
		while (wait_idle--);
		if(i == 3) {
			//config MR1 ODT [9,6,2] RON [5,1] DLLOFF [0]
			val = 0x10010000;
			val |= (odt & 0x4)<<7 | (odt & 0x2)<<5 | (odt & 0x1)<<2;
			val |= (ron & 0x2)<<4 | (ron & 0x1)<<1;
			if (dll_off)
				val |= 0x1;
			writel(val, DMC_REG_BASE + ddr_config[i].reg_offset);
		/*	printf("MR1 is %x\n", (unsigned int)val);*/
		} else {
			writel(ddr_config[i].dll_off_val, DMC_REG_BASE + ddr_config[i].reg_offset);
		/*	printf("reg 0x%x = 0x%x\n",
				(unsigned int)(ddr_config[i].reg_offset),
				(unsigned int)(ddr_config[i].dll_off_val));*/
		}
	}
#endif
#if (DDR_TYPE == 2)
	show_dram_info();
#ifdef TGT_AP_SET_DDR_LP_PARAM
	_ddr_config_low_power_mode();
	_ddr_set_refresh_timing();
#endif
#endif
	//config MEMC CMD to READY state
	writel(0x3, DMC_REG_BASE + MEMC_CMD);

	//READ MEMC STATUS
	val = 0;
	do {
		val = readl(DMC_REG_BASE + MEMC_STATUS);
		//printf("MEMC_STATUS value: %x\n", val);
	} while (val != 3);
}

void axi_prio_init(void)
{
	writel(0xaa0000aa, 0x20900100);
	writel(0x00000008, 0x21054100);
	writel(0x00000008, 0x21054104);
}

void axi_outstandings_init(void)
{
	writel(0x00000060, 0x2104210c);
	writel(0x00000300, 0x21042110);
}

int ddr_init(UINT16 flags, UINT32 para)
{
	UINT16 dll_off;
	UINT32 mem_width;

	//axi_prio_init();
	axi_outstandings_init();

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
#ifdef DO_DDR_PLL_DEBUG
	cmd_input("DigPhy", PHY_REG_BASE);
#endif
	config_dmc400(flags, para);
	printf("dram init done ...\n");

	return 0;
}
