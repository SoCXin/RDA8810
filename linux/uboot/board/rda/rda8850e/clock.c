#include <common.h>
#include <errno.h>
#include <asm/arch/rda_iomap.h>
#include <asm/io.h>
#include <asm/arch/reg_sysctrl.h>
#include <asm/arch/hwcfg.h>
#include <asm/arch/ispi.h>
#include <asm/arch/rda_sys.h>
#include "clock_config.h"
#include "debug.h"
#include "ddr.h"
#include "tgt_ap_clock_config.h"

#if (PMU_VBUCK3_VAL < 0 || PMU_VBUCK3_VAL > 15)
#error "Invalid PMU_VBUCK3_VAL"
#endif

#ifdef PMU_VBUCK5_VAL
#if (PMU_VBUCK5_VAL < 0 || PMU_VBUCK5_VAL > 15)
#error "Invalid PMU_VBUCK5_VAL"
#endif
#endif

//#define DO_DDR_PLL_DEBUG

#define RDA_PMU_VBUCK4_BIT_ACT_SHIFT     (4)
#define RDA_PMU_VBUCK4_BIT_ACT_MASK      (0xf<<4)
#define RDA_PMU_VBUCK4_BIT_ACT(n)        (((n)&0xf)<<4)

/* This macro is used to select GPU, VPU, CPU clock value manully */
//#define TGT_AP_SELECT_CLK
/* For staying so that watching register value */
#ifdef TGT_AP_SELECT_CLK
#define DO_STAY_FOR_WATCHING
#endif
#define PMU_REG_VIBRATOR_NUM 3
#define PMU_REG_VIBRATOR_BIT 5

const static UINT32 test_pattern[] =
{

          0xa5a5a5a5, 0x5a5a5a5a,

          0x4f35b7da, 0x8e354c91,

          0x00000000, 0xFFFFFFFF,

          0x00000000, 0x00000000,

          0x0000FFFF, 0x00000000,
};

enum {
	AP_CPU_CLK_IDX = 0,
	AP_BUS_CLK_IDX,
	AP_MEM_CLK_IDX,
	AP_USB_CLK_IDX,
};

static int pll_enabled(int idx)
{
	if ((hwp_sysCtrlAp->Cfg_Pll_Ctrl[idx] &
			(SYS_CTRL_AP_AP_PLL_ENABLE_MASK |
			 SYS_CTRL_AP_AP_PLL_LOCK_RESET_MASK)) ==
			(SYS_CTRL_AP_AP_PLL_ENABLE_ENABLE |
			 SYS_CTRL_AP_AP_PLL_LOCK_RESET_NO_RESET))
		return 1;
	else
		return 0;
}

static int usb_in_use = 0;

static void check_usb_usage(void)
{
	unsigned int mask = SYS_CTRL_AP_BUS_SEL_FAST_SLOW |
		SYS_CTRL_AP_PLL_LOCKED_BUS_MASK |
		SYS_CTRL_AP_PLL_LOCKED_USB_MASK;
	unsigned int reg = SYS_CTRL_AP_BUS_SEL_FAST_FAST |
		SYS_CTRL_AP_PLL_LOCKED_BUS_LOCKED |
		SYS_CTRL_AP_PLL_LOCKED_USB_LOCKED;

	if ((hwp_sysCtrlAp->Sel_Clock & mask) == reg &&
			pll_enabled(AP_BUS_CLK_IDX) &&
			pll_enabled(AP_USB_CLK_IDX))
		usb_in_use = 1;
	else
		usb_in_use = 0;
}

#ifndef CONFIG_RDA_FPGA
struct pll_freq {
	UINT32 freq_mhz;
	UINT16 major;
	UINT16 minor;
	UINT16 with_div;
	UINT16 div;
};

struct pmu_reg {
	UINT32 reg_num;
	UINT32 reg_val;
};

static const struct pmu_reg pmu_reg_table[] = {
		{0x3f, 0x30fb},/*set nand protect battery voltage:
			0x30f3:2.4v  0x30f7:2.7v  0x30fb:3.0v  0x30ff:3.2v*/
		//{0x01, 0x000b},/*enable nand protect, 0x000a in default*/
		{0x03, 0x1853},/*shut down vbuck1, vasw, vcam, vlcd, vmic,bb1_led voltage*/
		{0x04, 0x9670},/* vmc_vsel_act = 0 */
		{0x0f, 0x1e90},/* enable bandgap chopper mode */
		{0x13, 0x3b70},
		{0x2d, 0x96ae},/*all dcdc settings are set to 0x96ae */
		{0x2e, 0x96ae},/*all dcdc settings are set to 0x96ae */
		{0x2f, 0x9244},
		{0x53, 0x9505}, /*vbuck5 val = 8, vout_sel_buck5_lp = 0 */
		{0x0d, 0x8280},/* shut down vbuck4 voltage.If backlight need, please open. */
		{0x36, 0x6e45},
		{0x2a, 0xaa45},
		{0x4a, 0x96ae},/*all dcdc settings are set to 0x96ae */
		{0x4b, 0x96ae},/*all dcdc settings are set to 0x96ae */
		{0x54, 0x96ae},/*all dcdc settings are set to 0x96ae */
};

typedef enum {
	PLL_REG_CPU_BASE = 0x00,
	PLL_REG_BUS_BASE = 0x20,
	PLL_REG_MEM_BASE = 0x60,
	PLL_REG_USB_BASE = 0x80,
} PLL_REG_BASE_INDEX_t;

typedef enum {
	PLL_REG_OFFSET_01H = 1,
	PLL_REG_OFFSET_02H = 2,
	PLL_REG_OFFSET_DIV = 3,
	PLL_REG_OFFSET_04H = 4,
	PLL_REG_OFFSET_MAJOR = 5,
	PLL_REG_OFFSET_MINOR = 6,
	PLL_REG_OFFSET_07H = 7,
} PLL_REG_OFFSET_INDEX_t;

static const struct pll_freq pll_freq_table[] = {
	/* MHz Major   Minor   div */
	{1600, 0x7B13, 0xB138, 0, 0x0000},
	{1200, 0x5C4E, 0xC4EC, 0, 0x0000},
	{1150, 0x5876, 0x2762, 0, 0x0000},
	{1100, 0x549D, 0x89D8, 0, 0x0000},
	{1050, 0x50C4, 0xEC4E, 0, 0x0000},
	{1020, 0x4E76, 0x2762, 0, 0x0000},
	{1010, 0x4DB1, 0x3B13, 0, 0x0000},
	{1000, 0x4CEC, 0x4EC4, 0, 0x0000},
	{ 988, 0x4C00, 0x0000, 0, 0x0000},
	{ 962, 0x4A00, 0x0000, 0, 0x0000},
	{ 936, 0x4800, 0x0000, 0, 0x0000},
	{ 910, 0x4600, 0x0000, 0, 0x0000},
	{ 884, 0x4400, 0x0000, 0, 0x0000},
	{ 858, 0x4200, 0x0000, 0, 0x0000},
	{ 832, 0x4000, 0x0000, 0, 0x0000},
	{ 806, 0x3E00, 0x0000, 0, 0x0000},
	{ 800, 0x3D89, 0xD89C, 0, 0x0000},
	{ 780, 0x3C00, 0x0000, 0, 0x0000},
	{ 750, 0x39B1, 0x3B13, 0, 0x0000},
	{ 600, 0x2E27, 0x6274, 0, 0x0000},
	{ 520, 0x2800, 0x0000, 0, 0x0000},
	{ 519, 0x27EC, 0x4EC4, 1, 0x0007},
	{ 500, 0x2676, 0x2762, 1, 0x0007},
	{ 480, 0x24EC, 0x4EC4, 0, 0x0000},
	{ 455, 0x2300, 0x0000, 1, 0x0007},
	{ 416, 0x2000, 0x0000, 1, 0x0007},
	{ 400, 0x1EC4, 0xEC4C, 1, 0x0007},
};
static const struct pll_freq pll_ddr_freq_table[] = {
	/* MHz Major   Minor   div */
	{ 600, 0x2E27, 0x6274, 1, 0xC497},
	{ 590, 0x2D62, 0x7624, 1, 0xC497},
	{ 573, 0x2C13, 0xB138, 1, 0xC497},
	{ 555, 0x2AB1, 0x3B10, 1, 0xC497},
	{ 538, 0x2962, 0x7624, 1, 0xC497},
	{ 519, 0x27EC, 0x4EC4, 1, 0xC497},
	{ 500, 0x2676, 0x2760, 1, 0xC397},
	{ 480, 0x24EC, 0x4EC4, 1, 0xC297},
	{ 455, 0x2300, 0x0000, 1, 0xC097},
	{ 429, 0x2100, 0x0000, 1, 0xC097},
	{ 416, 0x2000, 0x0000, 1, 0xC897},
	{ 403, 0x1F00, 0x0000, 1, 0xC897},
	{ 400, 0x1EC4, 0xEC4C, 1, 0xC897},
	{ 397, 0x1E89, 0xD89C, 1, 0xC897},
	{ 390, 0x1E00, 0x0000, 1, 0xC897},
	{ 383, 0x1D76, 0x2760, 1, 0xC897},
	{ 377, 0x1D00, 0x0000, 1, 0xD397},
	{ 370, 0x1C76, 0x2760, 1, 0xD297},
	{ 364, 0x1C00, 0x0000, 1, 0xD297},
	{ 358, 0x1B89, 0xD89C, 1, 0xD197},
	{ 355, 0x369D, 0x89D8, 0, 0xD097},
	{ 351, 0x3600, 0x0000, 0, 0xD097},
	{ 338, 0x3400, 0x0000, 0, 0xC897},
	{ 333, 0x333B, 0x13B0, 0, 0xC897},
	{ 328, 0x3276, 0x2760, 0, 0xC897},
	{ 325, 0x3200, 0x0000, 0, 0xC897},
	{ 321, 0x3162, 0x7624, 0, 0xC897},
	{ 316, 0x309D, 0x89D8, 0, 0xC897},
	{ 312, 0x3000, 0x0000, 0, 0xD897},
	{ 290, 0x2C9D, 0x89D8, 0, 0xE097},
	{ 260, 0x2800, 0x0000, 0, 0xF097},
	{ 200, 0x1EC4, 0xEC4C, 1, 0xB996},
	{ 156, 0x3000, 0x0000, 0, 0x0006},
	{ 100, 0x1EC4, 0xEC4C, 1, 0x0005},
	{  50, 0x1EC4, 0xEC4C, 1, 0x0004},
};

static const struct clock_config *g_clock_config;

#ifdef DO_DDR_PLL_DEBUG
static struct clock_config clock_debug_config;
static UINT32 ddrfreq = 400, ddr32bit = 0;
#endif

static void sys_shutdown_pll(void)
{
	int i;

	hwp_sysCtrlAp->REG_DBG = AP_CTRL_PROTECT_UNLOCK;

	if (usb_in_use) {
		hwp_sysCtrlAp->Sel_Clock = SYS_CTRL_AP_SLOW_SEL_RF_RF
			| SYS_CTRL_AP_CPU_SEL_FAST_SLOW
			| SYS_CTRL_AP_BUS_SEL_FAST_FAST
			| SYS_CTRL_AP_TIMER_SEL_FAST_FAST;
	} else {
		hwp_sysCtrlAp->Sel_Clock = SYS_CTRL_AP_SLOW_SEL_RF_RF
			| SYS_CTRL_AP_CPU_SEL_FAST_SLOW
			| SYS_CTRL_AP_BUS_SEL_FAST_SLOW
			| SYS_CTRL_AP_TIMER_SEL_FAST_FAST;
	}

	for (i = 0; i < 3; i++) {
		/* In download mode, rom code has been set ap bus*/
		if (usb_in_use) {
			if (i == AP_BUS_CLK_IDX) // ap bus
				continue;
		}
		hwp_sysCtrlAp->Cfg_Pll_Ctrl[i] =
			SYS_CTRL_AP_AP_PLL_ENABLE_POWER_DOWN |
			SYS_CTRL_AP_AP_PLL_LOCK_RESET_RESET;
	}
}

static void sys_setup_pll(void)
{
	int i;
	UINT32 mask;
	UINT32 locked;
	int cnt = 10; //10us, according to IC, the pll must be locked

	hwp_sysCtrlAp->REG_DBG = AP_CTRL_PROTECT_UNLOCK;

	for (i = 0; i < 3; i++) {
		/* In download mode, rom code has been set ap bus*/
		if (usb_in_use) {
			if (i == AP_BUS_CLK_IDX) // ap bus
				continue;
		}

		if (AP_MEM_CLK_IDX == i)
			hwp_sysCtrlAp->Cfg_Pll_Ctrl[i] =
				SYS_CTRL_AP_AP_PLL_ENABLE_ENABLE |
				SYS_CTRL_AP_AP_PLL_LOCK_RESET_NO_RESET |
				SYS_CTRL_AP_AP_PLL_LOCK_NUM_LOW(1)|
				SYS_CTRL_AP_AP_PLL_LOCK_NUM_HIGH(30);
		else
			hwp_sysCtrlAp->Cfg_Pll_Ctrl[i] =
				SYS_CTRL_AP_AP_PLL_ENABLE_ENABLE |
				SYS_CTRL_AP_AP_PLL_LOCK_RESET_NO_RESET |
				SYS_CTRL_AP_AP_PLL_LOCK_NUM_LOW(6)|
				SYS_CTRL_AP_AP_PLL_LOCK_NUM_HIGH(30);
	}

	mask = SYS_CTRL_AP_PLL_LOCKED_CPU_MASK
	    | SYS_CTRL_AP_PLL_LOCKED_BUS_MASK
	    | SYS_CTRL_AP_PLL_LOCKED_MEM_MASK
	    //| SYS_CTRL_AP_PLL_LOCKED_USB_MASK
	    ;
	locked = SYS_CTRL_AP_PLL_LOCKED_CPU_LOCKED
	    | SYS_CTRL_AP_PLL_LOCKED_BUS_LOCKED
	    | SYS_CTRL_AP_PLL_LOCKED_MEM_LOCKED
	    //| SYS_CTRL_AP_PLL_LOCKED_USB_LOCKED
	    ;

	while (((hwp_sysCtrlAp->Sel_Clock & mask) != locked) && cnt) {
		udelay(1);
		cnt--;
	}
	if (cnt == 0) {
		printf("WARNING, cannot lock cpu/bus/mem pll 0x%08x ",
			hwp_sysCtrlAp->Sel_Clock);
		printf("but we run anyway ...\n");
	}

	for (i = 0; i < 3; i++) {
		hwp_sysCtrlAp->Cfg_Pll_Ctrl[i] |=
		    SYS_CTRL_AP_AP_PLL_CLK_FAST_ENABLE_ENABLE;
	}

	hwp_sysCtrlAp->Sel_Clock = SYS_CTRL_AP_SLOW_SEL_RF_RF
	    | SYS_CTRL_AP_CPU_SEL_FAST_FAST
	    | SYS_CTRL_AP_BUS_SEL_FAST_FAST
	    | SYS_CTRL_AP_TIMER_SEL_FAST_FAST;
}

static void sys_setup_clk(void)
{
	// Disable some power-consuming clocks
#ifdef CONFIG_VPU_TEST
	hwp_sysCtrlAp->Clk_APO_Enable = SYS_CTRL_AP_ENABLE_APOC_VPU;
	//hwp_sysCtrlAp->Clk_MEM_Enable = SYS_CTRL_AP_ENABLE_MEM_VPU;
#else
	hwp_sysCtrlAp->Clk_APO_Disable = SYS_CTRL_AP_DISABLE_APOC_VPU;
	//hwp_sysCtrlAp->Clk_MEM_Disable = SYS_CTRL_AP_DISABLE_MEM_VPU;
#endif

	// Init clock gating mode
	hwp_sysCtrlAp->Clk_CPU_Mode = 0;
#ifdef CONFIG_VPU_TEST
	hwp_sysCtrlAp->Clk_AXI_Mode = SYS_CTRL_AP_MODE_AXI_DMA_MANUAL | SYS_CTRL_AP_MODE_APB0_CONF_MANUAL;
#else
	hwp_sysCtrlAp->Clk_AXI_Mode = SYS_CTRL_AP_MODE_AXI_DMA_MANUAL;
#endif
	hwp_sysCtrlAp->Clk_AXIDIV2_Mode = 0;
	hwp_sysCtrlAp->Clk_GCG_Mode = SYS_CTRL_AP_MODE_GCG_GOUDA_MANUAL
				| SYS_CTRL_AP_MODE_GCG_CAMERA_MANUAL;
	hwp_sysCtrlAp->Clk_AHB1_Mode = 0;
	hwp_sysCtrlAp->Clk_APB1_Mode = 0;
	hwp_sysCtrlAp->Clk_APB2_Mode = 0;
#ifdef CONFIG_VPU_TEST
	hwp_sysCtrlAp->Clk_MEM_Mode = SYS_CTRL_AP_MODE_CLK_MEM_MANUAL;
#else
	hwp_sysCtrlAp->Clk_MEM_Mode = 0;
#endif
	//hwp_sysCtrlAp->Clk_APO_Mode = SYS_CTRL_AP_MODE_APOC_VPU_MANUAL;

	// Init module frequency
	hwp_sysCtrlAp->Cfg_Clk_AP_CPU = g_clock_config->CLK_CPU;
	hwp_sysCtrlAp->Cfg_Clk_AP_AXI = g_clock_config->CLK_AXI;
	hwp_sysCtrlAp->Cfg_Clk_AP_GCG = g_clock_config->CLK_GCG;

	if (!usb_in_use) {
		hwp_sysCtrlAp->Cfg_Clk_AP_AHB1 = g_clock_config->CLK_AHB1;
	}

	hwp_sysCtrlAp->Cfg_Clk_AP_APB1 = g_clock_config->CLK_APB1;
	hwp_sysCtrlAp->Cfg_Clk_AP_APB2 = g_clock_config->CLK_APB2;
	hwp_sysCtrlAp->Cfg_Clk_AP_MEM = g_clock_config->CLK_MEM;
	hwp_sysCtrlAp->Cfg_Clk_AP_GPU = g_clock_config->CLK_GPU;
	hwp_sysCtrlAp->Cfg_Clk_AP_VPU = g_clock_config->CLK_VPU;
	hwp_sysCtrlAp->Cfg_Clk_AP_VOC = g_clock_config->CLK_VOC;
	hwp_sysCtrlAp->Cfg_Clk_AP_SFLSH = g_clock_config->CLK_SFLSH;
}

static void print_sys_reg(char *name, UINT32 value)
{
	printf("clk %s = %lx\n", name, value);
}

static void sys_dump_clk(void)
{
	print_sys_reg("CPU", hwp_sysCtrlAp->Cfg_Clk_AP_CPU);
	print_sys_reg("AXI", hwp_sysCtrlAp->Cfg_Clk_AP_AXI);
	print_sys_reg("GCG", hwp_sysCtrlAp->Cfg_Clk_AP_GCG);
	print_sys_reg("AHB1", hwp_sysCtrlAp->Cfg_Clk_AP_AHB1);
	print_sys_reg("APB1", hwp_sysCtrlAp->Cfg_Clk_AP_APB1);
	print_sys_reg("APB2", hwp_sysCtrlAp->Cfg_Clk_AP_APB2);
	print_sys_reg("MEM", hwp_sysCtrlAp->Cfg_Clk_AP_MEM);
	print_sys_reg("GPU", hwp_sysCtrlAp->Cfg_Clk_AP_GPU);
	print_sys_reg("VPU", hwp_sysCtrlAp->Cfg_Clk_AP_VPU);
	print_sys_reg("VOC", hwp_sysCtrlAp->Cfg_Clk_AP_VOC);
	print_sys_reg("SFLSH", hwp_sysCtrlAp->Cfg_Clk_AP_SFLSH);
}

static int pll_freq_set(UINT32 reg_base, UINT32 freq_mhz)
{
	int i;
	const struct pll_freq *freq;
	unsigned int major, minor;
	unsigned short value_02h;

	/* find pll_freq */
	for (i = 0; i < ARRAY_SIZE(pll_freq_table); i++) {
		if (pll_freq_table[i].freq_mhz == freq_mhz)
			break;
	}
	if (i >= ARRAY_SIZE(pll_freq_table)) {
		printf("pll_freq_set, fail to find freq\n");
		return -1;
	}

	freq = &pll_freq_table[i];
	if (freq->with_div && (reg_base == PLL_REG_MEM_BASE)) {
		if (freq_mhz >= 200)
			ispi_reg_write(reg_base + PLL_REG_OFFSET_DIV, freq->div & 0x7fff);
		mdelay(5);
		ispi_reg_write(reg_base + PLL_REG_OFFSET_DIV,
				freq->div);
		// Calculate the real MEM PLL freq
		freq_mhz *= (1 << (8 - (freq->div & 0x7)));
	}
	if (freq_mhz >= 800 || reg_base == PLL_REG_USB_BASE) {
		value_02h = 0x030B;
		// Div PLL freq by 2
		minor = ((freq->major & 0xFFFF) << 14) |
			((freq->minor >> 2) & 0x3FFF);
		minor >>= 1;
		// Recalculate the divider
		major = (minor >> 14) & 0xFFFF;
		minor = (minor << 2) & 0xFFFF;
	} else {
		value_02h = 0x020B;
		major = freq->major;
		minor = freq->minor;
	}

	if(reg_base == PLL_REG_USB_BASE)
		ispi_reg_write(reg_base + 0x9, 0x7100);
	if (reg_base == PLL_REG_USB_BASE)
		value_02h ^= (1 << 8);
#ifndef _TGT_AP_CPU_PLL_FREQ_AUTO_REGULATE
	if(reg_base == PLL_REG_CPU_BASE) {
		/* Disable pll freq divided by 2 when VCore voltage is lower than a threshold value. */
		if(rda_metal_id_get() > 1)
			ispi_reg_write(reg_base + PLL_REG_OFFSET_DIV, 0x72A2);
	}
#endif
	ispi_reg_write(reg_base + PLL_REG_OFFSET_02H, value_02h);
	ispi_reg_write(reg_base + PLL_REG_OFFSET_MAJOR, major);
	ispi_reg_write(reg_base + PLL_REG_OFFSET_MINOR, minor);
	ispi_reg_write(reg_base + PLL_REG_OFFSET_07H, 0x0012);
	mdelay(1);
	ispi_reg_write(reg_base + PLL_REG_OFFSET_07H, 0x0013);
	return 0;
}

static int pll_ddr_freq_set(UINT32 reg_base, UINT32 freq_mhz)
{
	int i;
	const struct pll_freq *freq;
	unsigned int major, minor;
	unsigned short value_02h;

	/* find pll_freq */
	for (i = 0; i < ARRAY_SIZE(pll_ddr_freq_table); i++) {
		if (pll_ddr_freq_table[i].freq_mhz == freq_mhz)
			break;
	}
	if (i >= ARRAY_SIZE(pll_ddr_freq_table)) {
		printf("pll_freq_set, fail to find freq\n");
		return -1;
	}

	freq = &pll_ddr_freq_table[i];
	if (freq->with_div)
		value_02h = 0x030B;
	else
		value_02h = 0x020B;
	major = freq->major;
	minor = freq->minor;

	printf("major = %x, minor = %x --\n", major, minor);
	ispi_reg_write(reg_base + PLL_REG_OFFSET_02H, value_02h);
	ispi_reg_write(reg_base + PLL_REG_OFFSET_MAJOR, major);
	ispi_reg_write(reg_base + PLL_REG_OFFSET_MINOR, minor);
	ispi_reg_write(reg_base + PLL_REG_OFFSET_07H, 0x0012);
	mdelay(1);
	ispi_reg_write(reg_base + PLL_REG_OFFSET_07H, 0x0013);
	mdelay(1);
	ispi_reg_write(reg_base + PLL_REG_OFFSET_DIV, freq->div & 0x7fff);
	mdelay(1);
	ispi_reg_write(reg_base + PLL_REG_OFFSET_DIV, freq->div);
	return 0;
}

void modem_RfspiInit_624M(void);
void modem_8850eeco2_RfspiInit_624M(void);

static void pll_setup_freq(void)
{
	u16 metal_id = rda_metal_id_get();

	pll_freq_set(PLL_REG_CPU_BASE, g_clock_config->PLL_FREQ_CPU);
	// Always configure BUS PLL, even when it is being used
	pll_freq_set(PLL_REG_BUS_BASE, g_clock_config->PLL_FREQ_BUS);
	pll_ddr_freq_set(PLL_REG_MEM_BASE, g_clock_config->PLL_FREQ_MEM);
	pll_freq_set(PLL_REG_USB_BASE, g_clock_config->PLL_FREQ_USB);

	if (metal_id < 2)
		modem_RfspiInit_624M();
	else
		modem_8850eeco2_RfspiInit_624M();
}

static void print_pll_freq(char *name, UINT32 value)
{
	printf("pll freq %s = %d\n", name, (int)value);
}

static void sys_dump_pll_freq(void)
{
	print_pll_freq("CPU", g_clock_config->PLL_FREQ_CPU);
	print_pll_freq("BUS", g_clock_config->PLL_FREQ_BUS);
	print_pll_freq("MEM", g_clock_config->PLL_FREQ_MEM);
	//print_pll_freq("USB", g_clock_config->PLL_FREQ_USB);
}

static void pmu_setup_init(void)
{
	int i;
	u16 metal_id = rda_metal_id_get();
	u32 reg_val = 0;

	rda_nand_iodrive_set();

	ispi_open(1);
	for (i = 0; i < ARRAY_SIZE(pmu_reg_table); i++){
		/*Fix is used for vibrator shutdown for eco2 and later chip.
		  If 8850e eco1 is not used, then we should change reg3 value 
		  in pmu_reg_table and remove the redundant code*/
		reg_val = pmu_reg_table[i].reg_val;
		if ((metal_id >= 2) && (pmu_reg_table[i].reg_num == PMU_REG_VIBRATOR_NUM))
			reg_val &= ~(1 << PMU_REG_VIBRATOR_NUM);
		if (pmu_reg_table[i].reg_num == 0x53)
			reg_val = (reg_val & ~(0xf << 5)) | (PMU_VBUCK5_VAL << 5);
		ispi_reg_write(pmu_reg_table[i].reg_num, reg_val);
	}
	ispi_open(0);

#ifdef CONFIG_RDA_PDL
	enable_charger(0);
#endif
}

/*
 * we initialize usb clock, but this won't cause the usb clock jitter,
 * because wo don't setup the usb pll
 */
static void usb_clock_pre_init(void)
{
#if 0
	u16 val = ispi_reg_read(0x83);
	printf("%s: reg 0x83 = %#x\n", __func__, val);
	val = ispi_reg_read(0x89);
	printf("%s: reg 0x89 = %#x\n", __func__, val);
	val = ispi_reg_read(0x8d);
	printf("%s: reg 0x8d = %#x\n", __func__, val);
#endif
	ispi_reg_write(0x83, 0x72ff); /* default 0x72e3, update bit 1~4, b0001 -> b1111 */
	ispi_reg_write(0x89, 0x7180); /* default 0x7100, update bit 7, b0 -> b1 */
	ispi_reg_write(0x8d, 0x0302); /* default 0x0304, update bit 0~2, b100 -> b010 */
}

static void pll_setup_init(void)
{
	if (g_clock_config->DDR_CHAN_1_VALID) {
		ispi_reg_write(0x101, g_clock_config->DDR_TIMING_101H);
		ispi_reg_write(0x102, g_clock_config->DDR_TIMING_102H);
		ispi_reg_write(0x103, g_clock_config->DDR_TIMING_103H);
		ispi_reg_write(0x104, g_clock_config->DDR_TIMING_104H);
		ispi_reg_write(0x105, g_clock_config->DDR_TIMING_105H);
		ispi_reg_write(0x106, g_clock_config->DDR_TIMING_106H);
		ispi_reg_write(0x107, g_clock_config->DDR_TIMING_107H);
		ispi_reg_write(0x108, g_clock_config->DDR_TIMING_108H);
		ispi_reg_write(0x109, g_clock_config->DDR_TIMING_109H);
		ispi_reg_write(0x10A, g_clock_config->DDR_TIMING_10AH);
		ispi_reg_write(0x10B, g_clock_config->DDR_TIMING_10BH);
		ispi_reg_write(0x10C, g_clock_config->DDR_TIMING_10CH);
		ispi_reg_write(0x10D, g_clock_config->DDR_TIMING_10DH);
		ispi_reg_write(0x10E, g_clock_config->DDR_TIMING_10EH);
		ispi_reg_write(0x10F, g_clock_config->DDR_TIMING_10FH);
	}

	if (g_clock_config->DDR_CHAN_2_VALID) {
		ispi_reg_write(0x121, g_clock_config->DDR_TIMING_121H);
		ispi_reg_write(0x122, g_clock_config->DDR_TIMING_122H);
		ispi_reg_write(0x123, g_clock_config->DDR_TIMING_123H);
		ispi_reg_write(0x124, g_clock_config->DDR_TIMING_124H);
		ispi_reg_write(0x125, g_clock_config->DDR_TIMING_125H);
		ispi_reg_write(0x126, g_clock_config->DDR_TIMING_126H);
		ispi_reg_write(0x127, g_clock_config->DDR_TIMING_127H);
		ispi_reg_write(0x128, g_clock_config->DDR_TIMING_128H);
		ispi_reg_write(0x129, g_clock_config->DDR_TIMING_129H);
		ispi_reg_write(0x12A, g_clock_config->DDR_TIMING_12AH);
		ispi_reg_write(0x12B, g_clock_config->DDR_TIMING_12BH);
		ispi_reg_write(0x12C, g_clock_config->DDR_TIMING_12CH);
		ispi_reg_write(0x12D, g_clock_config->DDR_TIMING_12DH);
		ispi_reg_write(0x12E, g_clock_config->DDR_TIMING_12EH);
		ispi_reg_write(0x12F, g_clock_config->DDR_TIMING_12FH);
	}

	if (g_clock_config->DDR_CHAN_3_VALID) {
		ispi_reg_write(0x141, g_clock_config->DDR_TIMING_141H);
		ispi_reg_write(0x142, g_clock_config->DDR_TIMING_142H);
		ispi_reg_write(0x143, g_clock_config->DDR_TIMING_143H);
		ispi_reg_write(0x144, g_clock_config->DDR_TIMING_144H);
		ispi_reg_write(0x145, g_clock_config->DDR_TIMING_145H);
		ispi_reg_write(0x146, g_clock_config->DDR_TIMING_146H);
		ispi_reg_write(0x147, g_clock_config->DDR_TIMING_147H);
		ispi_reg_write(0x148, g_clock_config->DDR_TIMING_148H);
		ispi_reg_write(0x149, g_clock_config->DDR_TIMING_149H);
		ispi_reg_write(0x14A, g_clock_config->DDR_TIMING_14AH);
		ispi_reg_write(0x14B, g_clock_config->DDR_TIMING_14BH);
		ispi_reg_write(0x14C, g_clock_config->DDR_TIMING_14CH);
		ispi_reg_write(0x14D, g_clock_config->DDR_TIMING_14DH);
		ispi_reg_write(0x14E, g_clock_config->DDR_TIMING_14EH);
		ispi_reg_write(0x14F, g_clock_config->DDR_TIMING_14FH);
	}

	if (g_clock_config->DDR_CHAN_4_VALID) {
		ispi_reg_write(0x161, g_clock_config->DDR_TIMING_161H);
		ispi_reg_write(0x162, g_clock_config->DDR_TIMING_162H);
		ispi_reg_write(0x163, g_clock_config->DDR_TIMING_163H);
		ispi_reg_write(0x164, g_clock_config->DDR_TIMING_164H);
		ispi_reg_write(0x165, g_clock_config->DDR_TIMING_165H);
		ispi_reg_write(0x166, g_clock_config->DDR_TIMING_166H);
		ispi_reg_write(0x167, g_clock_config->DDR_TIMING_167H);
		ispi_reg_write(0x168, g_clock_config->DDR_TIMING_168H);
		ispi_reg_write(0x169, g_clock_config->DDR_TIMING_169H);
		ispi_reg_write(0x16A, g_clock_config->DDR_TIMING_16AH);
		ispi_reg_write(0x16B, g_clock_config->DDR_TIMING_16BH);
		ispi_reg_write(0x16C, g_clock_config->DDR_TIMING_16CH);
		ispi_reg_write(0x16D, g_clock_config->DDR_TIMING_16DH);
		ispi_reg_write(0x16E, g_clock_config->DDR_TIMING_16EH);
		ispi_reg_write(0x16F, g_clock_config->DDR_TIMING_16FH);
	}

	ispi_reg_write(0x160, g_clock_config->DDR_TIMING_160H);
	ispi_reg_write(0x180, g_clock_config->DDR_TIMING_180H);
	ispi_reg_write(0x181, g_clock_config->DDR_TIMING_181H);
	ispi_reg_write(0x182, g_clock_config->DDR_TIMING_182H);
	ispi_reg_write(0x183, g_clock_config->DDR_TIMING_183H);
	ispi_reg_write(0x184, g_clock_config->DDR_TIMING_184H);
	ispi_reg_write(0x185, g_clock_config->DDR_TIMING_185H);
	ispi_reg_write(0x186, g_clock_config->DDR_TIMING_186H);
	ispi_reg_write(0x187, g_clock_config->DDR_TIMING_187H);
	ispi_reg_write(0x188, g_clock_config->DDR_TIMING_188H);
	ispi_reg_write(0x189, g_clock_config->DDR_TIMING_189H);
	ispi_reg_write(0x18A, g_clock_config->DDR_TIMING_18AH);
	ispi_reg_write(0x18B, g_clock_config->DDR_TIMING_18BH);
	ispi_reg_write(0x18C, g_clock_config->DDR_TIMING_18CH);
	ispi_reg_write(0x18D, g_clock_config->DDR_TIMING_18DH);
	ispi_reg_write(0x18E, g_clock_config->DDR_TIMING_18EH);
	ispi_reg_write(0x18F, g_clock_config->DDR_TIMING_18FH);

	ispi_reg_write(0x69, g_clock_config->DDR_TIMING_69H);
	usb_clock_pre_init();

	udelay(5000);
}

static void pll_setup_mem(void)
{
}

static void pll_setup_mem_cal(void)
{
}

static void print_pll_reg(UINT32 index, UINT32 value)
{
	printf("pll reg %lx = %lx\n", index, value);
}

static void pll_dump_reg(void)
{
	print_pll_reg(0x005, ispi_reg_read(0x005));
	print_pll_reg(0x006, ispi_reg_read(0x006));
	print_pll_reg(0x063, ispi_reg_read(0x063));
	print_pll_reg(0x065, ispi_reg_read(0x065));
	print_pll_reg(0x066, ispi_reg_read(0x066));

	if (g_clock_config->DDR_CHAN_1_VALID)
	{
		print_pll_reg(0x100, ispi_reg_read(0x100));
		print_pll_reg(0x101, ispi_reg_read(0x101));
		print_pll_reg(0x102, ispi_reg_read(0x102));
		print_pll_reg(0x103, ispi_reg_read(0x103));
		print_pll_reg(0x104, ispi_reg_read(0x104));
		print_pll_reg(0x105, ispi_reg_read(0x105));
		print_pll_reg(0x106, ispi_reg_read(0x106));
		print_pll_reg(0x107, ispi_reg_read(0x107));
		print_pll_reg(0x108, ispi_reg_read(0x108));
		print_pll_reg(0x109, ispi_reg_read(0x109));
		print_pll_reg(0x10A, ispi_reg_read(0x10A));
		print_pll_reg(0x10B, ispi_reg_read(0x10B));
		print_pll_reg(0x10C, ispi_reg_read(0x10C));
		print_pll_reg(0x10D, ispi_reg_read(0x10D));
		print_pll_reg(0x10E, ispi_reg_read(0x10E));
		print_pll_reg(0x10F, ispi_reg_read(0x10F));
	}

	if (g_clock_config->DDR_CHAN_2_VALID)
	{
		print_pll_reg(0x120, ispi_reg_read(0x120));
		print_pll_reg(0x121, ispi_reg_read(0x121));
		print_pll_reg(0x122, ispi_reg_read(0x122));
		print_pll_reg(0x123, ispi_reg_read(0x123));
		print_pll_reg(0x124, ispi_reg_read(0x124));
		print_pll_reg(0x125, ispi_reg_read(0x125));
		print_pll_reg(0x126, ispi_reg_read(0x126));
		print_pll_reg(0x127, ispi_reg_read(0x127));
		print_pll_reg(0x128, ispi_reg_read(0x128));
		print_pll_reg(0x129, ispi_reg_read(0x129));
		print_pll_reg(0x12A, ispi_reg_read(0x12A));
		print_pll_reg(0x12B, ispi_reg_read(0x12B));
		print_pll_reg(0x12C, ispi_reg_read(0x12C));
		print_pll_reg(0x12D, ispi_reg_read(0x12D));
		print_pll_reg(0x12E, ispi_reg_read(0x12E));
		print_pll_reg(0x12F, ispi_reg_read(0x12F));
	}

	if (g_clock_config->DDR_CHAN_3_VALID)
	{
		print_pll_reg(0x140, ispi_reg_read(0x140));
		print_pll_reg(0x141, ispi_reg_read(0x141));
		print_pll_reg(0x142, ispi_reg_read(0x142));
		print_pll_reg(0x143, ispi_reg_read(0x143));
		print_pll_reg(0x144, ispi_reg_read(0x144));
		print_pll_reg(0x145, ispi_reg_read(0x145));
		print_pll_reg(0x146, ispi_reg_read(0x146));
		print_pll_reg(0x147, ispi_reg_read(0x147));
		print_pll_reg(0x148, ispi_reg_read(0x148));
		print_pll_reg(0x149, ispi_reg_read(0x149));
		print_pll_reg(0x14A, ispi_reg_read(0x14A));
		print_pll_reg(0x14B, ispi_reg_read(0x14B));
		print_pll_reg(0x14C, ispi_reg_read(0x14C));
		print_pll_reg(0x14D, ispi_reg_read(0x14D));
		print_pll_reg(0x14E, ispi_reg_read(0x14E));
		print_pll_reg(0x14F, ispi_reg_read(0x14F));
	}

	if (g_clock_config->DDR_CHAN_4_VALID)
	{
		print_pll_reg(0x161, ispi_reg_read(0x161));
		print_pll_reg(0x162, ispi_reg_read(0x162));
		print_pll_reg(0x163, ispi_reg_read(0x163));
		print_pll_reg(0x164, ispi_reg_read(0x164));
		print_pll_reg(0x165, ispi_reg_read(0x165));
		print_pll_reg(0x166, ispi_reg_read(0x166));
		print_pll_reg(0x167, ispi_reg_read(0x167));
		print_pll_reg(0x168, ispi_reg_read(0x168));
		print_pll_reg(0x169, ispi_reg_read(0x169));
		print_pll_reg(0x16A, ispi_reg_read(0x16A));
		print_pll_reg(0x16B, ispi_reg_read(0x16B));
		print_pll_reg(0x16C, ispi_reg_read(0x16C));
		print_pll_reg(0x16D, ispi_reg_read(0x16D));
		print_pll_reg(0x16E, ispi_reg_read(0x16E));
		print_pll_reg(0x16F, ispi_reg_read(0x16F));
	}

	print_pll_reg(0x160, ispi_reg_read(0x160));
	print_pll_reg(0x180, ispi_reg_read(0x180));
	print_pll_reg(0x181, ispi_reg_read(0x181));
	print_pll_reg(0x182, ispi_reg_read(0x182));
	print_pll_reg(0x183, ispi_reg_read(0x183));
	print_pll_reg(0x184, ispi_reg_read(0x184));
	print_pll_reg(0x185, ispi_reg_read(0x185));
	print_pll_reg(0x186, ispi_reg_read(0x186));
	print_pll_reg(0x187, ispi_reg_read(0x187));
	print_pll_reg(0x188, ispi_reg_read(0x188));
	print_pll_reg(0x189, ispi_reg_read(0x189));
	print_pll_reg(0x18A, ispi_reg_read(0x18A));
	print_pll_reg(0x18B, ispi_reg_read(0x18B));
	print_pll_reg(0x18C, ispi_reg_read(0x18C));
	print_pll_reg(0x18D, ispi_reg_read(0x18D));
	print_pll_reg(0x18E, ispi_reg_read(0x18E));
	print_pll_reg(0x18F, ispi_reg_read(0x18F));
}

static int clock_save_config(void)
{
	/* save config to nand */
	return 1;
}

#ifdef DO_DDR_PLL_DEBUG

static int ddr_get_freq(UINT8 chioce)
{
	switch(chioce)
	{
		case 1:
			return 200;
		case 2:
			return 290;
		case 3:
			return 312;
		case 4:
			return 316;
		case 5:
			return 321;
		case 6:
			return 325;
		case 7:
			return 328;
		case 8:
			return 333;
		case 9:
			return 338;
		case 10:
			return 351;
		case 11:
			return 355;
		case 12:
			return 358;
		case 13:
			return 364;
		case 14:
			return 370;
		case 15:
			return 377;
		case 16:
			return 383;
		case 17:
			return 390;
		case 18:
			return 397;
		case 19:
			return 400;
		case 20:
			return 403;
		case 21:
			return 416;
		case 22:
			return 429;
		case 23:
			return 455;
		case 24:
			return 480;
		case 25:
			return 100;
		case 26:
			return 500;
		case 27:
			return 519;
		case 28:
			return 538;
		case 29:
			return 555;
		case 30:
			return 573;
		case 31:
			return 590;
		case 32:
			return 600;

		default:
			return -1;
	}
}

static void freq_choose(void)
{
	UINT8 i = 0, buf[3] = {0}, choice = 0;
	INT32 freq_temp = 0;

	printf("\nPlese choose the ddr Freq:");
	printf("\n 1.200M  2.290M  3.312M  4.316M  5.321M ");
	printf("\n 6.325M  7.328M  8.333M  9.338M 10.351M ");
	printf("\n11.355M 12.358M 13.364M 14.370M 15.377M ");
	printf("\n11.355M 12.358M 13.364M 14.370M 15.377M ");
	printf("\n16.383M 17.390M 18.397M 19.400M 20.403M ");
	printf("\n21.416M 22.429M 23.455M 24.480M 25.100M ");
	printf("\n26.500M 27.519M 28.538M 29.555M 30.573M ");
	printf("\n31.590M 32.600M ");
	printf("\nThe number is:");

	while(1)
	{
		if (i > 2)
		{
			printf("\n Sorry, you input is wrong. Please input again:");
			i = 0;
		}
		buf[i] = serial_getc();
		serial_putc(buf[i]);

		if ( ('\r' == buf[i]) || ('\n' == buf[i]))
		{
			serial_puts("\n");
			break;
		}

		i++;
	}

	if (1 == i)
		choice = buf[0] - 0x30;
	else if (2 == i)
		choice = 10*(buf[0] - 0x30) + (buf[1] - 0x30);
	else
		return;

	freq_temp = ddr_get_freq(choice);
	if (-1 == freq_temp )
		printf("\n Sorry, the fre you choose is wrong");
	ddrfreq = freq_temp;
}

static void data_bits_choose(void)
{
	UINT8 i = 0, buf[2] = {0};

	printf("\nPlese choose the ddr data bits:");
	printf("\n1.16  2.32");
	printf("\nThe number is:");

	while(1)
	{
		buf[i] =serial_getc();

		if (i == 1)
		{
			if (('\r' == buf[i]) || ('\n' == buf[i]))
			{
				if (1 == (buf[0] - 0x30))
					ddr32bit = 0;
				else
					ddr32bit = 1;
				return;
			}
			else
			{
				printf("\n Sorry, you input is wrong. Please input again:");
				i = 0;
				continue;
			}
		}

		serial_putc(buf[i]);

		if ((buf[i] - 0x30) > 2 || 0 == (buf[i] - 0x30))
			printf("\n Sorry, you input is wrong. Please input again:");
		else
			i++;
	}
}

static void pmu_setup_calibration_voltage(UINT8 vcoreselect)
{
	UINT8 buf[3] = {0}, voltage = 0;
	int i = 0;
	UINT32 reg_value,temp, regid = 0;

	ispi_open(1);
	switch(vcoreselect) {
	case 1:
		regid = 0x53;
		printf("\nPlese input vcore voltage(0 ~ 15):");
		break;
	case 2:
		regid = 0x2a;
		printf("\nPlese input DDR buck3 voltage(0 ~ 15):");
		break;
	case 3:
		regid = 0x2a;
		printf("\nPlese input DDR buck4 voltage(0 ~ 15):");
		break;
	default:
		printf("\ninvalid vcoreselect,exit\n");
		return;
	}

	while(1) {
		if (i > 2) {
			printf("\n Sorry, you input is wrong. Please input again:");
			i = 0;
		}
		buf[i] = serial_getc();
		serial_putc(buf[i]);

		if ( ('\r' == buf[i]) || ('\n' == buf[i])) {
			printf("\n");
			break;
		}
		i++;
	}
	if (1 == i)
		voltage = buf[0] - 0x30;
	else if (2 == i)
		voltage = 10*(buf[0] - 0x30) + (buf[1] - 0x30);
	else
		return;
	printf("voltage is: 0x%x\n",(unsigned int)voltage);

	if (vcoreselect == 1){
		reg_value = 0x9415;
		reg_value &= ~(0xf << 5);
		reg_value |= ((voltage & 0xf) << 5);
		ispi_reg_write(regid, reg_value);//write vbuc5 act voltage bit5~bit8
	} else if (vcoreselect == 2) {
		temp = (UINT32)voltage;
		temp = (temp & 0xf) << 12;
		reg_value = ispi_reg_read(regid);
		reg_value &= ~(0xf << 12);
		reg_value |= temp ;
		ispi_reg_write(regid, reg_value);//write vbuck3 act voltage,bit12~bit15
		mdelay(100);
	} else {
		temp = (UINT32)voltage;
		temp = (temp & 0xf) << 4;
		reg_value = ispi_reg_read(regid);
		reg_value &= ~(0xf << 4);
		reg_value |= temp ;
		ispi_reg_write(regid, reg_value);//write vbuck4 act voltage,bit4~bit7
		mdelay(100);
	}
	printf("write reg: 0x%x,val: 0x%x\n",(unsigned int)regid,(unsigned int)reg_value);
}
#if 0
static void pmu_buck4_buck3_choose(void)
{
	UINT8 i = 0, buf[2] = {0};
	UINT8 ddr_voltage_source = 0;

	printf("\nPlese choose the ddr voltage:");
	printf("\n1.DDR3L  2.DDR3");
	printf("\nThe number is:");

	while(1)
	{
		buf[i] =serial_getc();

		if (i == 1)
		{
			if (('\r' == buf[i]) || ('\n' == buf[i]))
			{
				if (1 == (buf[0] - 0x30))
					ddr_voltage_source =2;
				else{
					ddr_voltage_source = 3;
					pmu_setup_calibration_voltage(2);
				}
				pmu_setup_calibration_voltage(ddr_voltage_source);
				return;
			}
			else
			{
				printf("\n Sorry, you input is wrong. Please input again:");
				i = 0;
				continue;
			}
		}

		serial_putc(buf[i]);

		if ((buf[i] - 0x30) > 2 || 0 == (buf[i] - 0x30))
			printf("\n Sorry, you input is wrong. Please input again:");
		else
			i++;
	}
}
#endif

void clock_load_ddr_cal_config(void)
{
	clock_debug_config.PLL_FREQ_MEM = ddrfreq;
	if (ddrfreq < 200)
		clock_debug_config.DDR_FLAGS |= DDR_FLAGS_DLLOFF;
	else
		clock_debug_config.DDR_FLAGS &= ~DDR_FLAGS_DLLOFF;

	if (0 == ddr32bit)
	{
		clock_debug_config.DDR_CHAN_3_VALID = 0;
		clock_debug_config.DDR_CHAN_4_VALID = 0;
	}

	clock_debug_config.DDR_PARA &= ~DDR_PARA_MEM_BITS_MASK;
	if (0 == ddr32bit)
		clock_debug_config.DDR_PARA |= DDR_PARA_MEM_BITS(1);
	else
		clock_debug_config.DDR_PARA |= DDR_PARA_MEM_BITS(2);
}

static int serial_gets(UINT8 *pstr)
{
    UINT32 length;

    length = 0;
    while(1) {
        pstr[length] = serial_getc();
        if(pstr[length] == '\r') {
            pstr[length] = 0x00;
            break;
        }
        else if( pstr[length] == '\b' ) {
            if(length>0) {
                length --;
                printf("\b");
            }
        }
        else {
            serial_putc(pstr[length]);
            length ++;
        }

        if(length > 32)
            return -1;
    }
    return length;
}

UINT32 asc2hex(UINT8 *pstr, UINT8 len)
{
	UINT8 i,ch,mylen;
	UINT32 hexvalue;

	for(mylen=0,i=0; i<8; i++)
	{
		if( pstr[i] == 0 )
			break;
		mylen ++;
	}
	if( len != 0 )
	{
		if(mylen>len)
			mylen = len;
	}
	if(mylen>8)
		mylen = 8;

	hexvalue = 0;
	for (i = 0; i < mylen; i++)
	{
		hexvalue <<= 4;
		ch = *(pstr+i);
		if((ch>='0') && (ch<='9'))
			hexvalue |= ch - '0';
		else if((ch>='A') && (ch<='F'))
			hexvalue |= ch - ('A' - 10);
		else if((ch>='a') && (ch<='f'))
			hexvalue |= ch - ('a' - 10);
		else
			;
	}
	return(hexvalue);
}

static int process_cmd(char * pname, char * cmd, UINT32 reb_base)
{
	char cmd_element[3][16] = {{0}};
	char * cmd_temp = cmd;
	UINT8 i = 0, cmd_element_num = 0, former_space = 1;
	UINT32 reg = 0, reg_value = 0;

	if (NULL == cmd)
		return -1;

	while(('\0' != *cmd_temp) && ('\r' != *cmd_temp) && ('\n' != *cmd_temp))
	{
		if (' ' == * cmd_temp)
		{
			if (0 == former_space)
			{
				former_space = 1;
				cmd_element[cmd_element_num][i] = '\0';
				cmd_element_num++;
				if (cmd_element_num > 2)
					return -1;
				i = 0;
			}
		}
		else
		{
			former_space = 0;
			cmd_element[cmd_element_num][i] = *cmd_temp;
			i++;
			if (i > 10)
				return -1;
		}

		cmd_temp++;
	}

	cmd_element[cmd_element_num][i] = '\0';

	if (!strcmp(cmd_element[0], "read"))
	{
		if (cmd_element_num == 2)
			return -1;
		if (('0' != cmd_element[1][0]) || ('x' != cmd_element[1][1]))
			return -1;

		reg = asc2hex((UINT8 *)&cmd_element[1][2], 8);
		if (reb_base)
			reg_value = *(UINT32 *)(reb_base + reg);
		else
			reg_value = ispi_reg_read(reg);

		printf("value = 0x%x", (unsigned int)reg_value);
		printf("\n%s#", pname);
	}
	else if (!strcmp(cmd_element[0], "write"))
	{
		if (cmd_element_num != 2)
			return -1;
		if (('0' != cmd_element[1][0]) || ('x' != cmd_element[1][1])
		   || ('0' != cmd_element[2][0]) || ('x' != cmd_element[2][1]))
			return -1;

		reg = asc2hex((UINT8 *)&cmd_element[1][2], 8);
		reg_value  = asc2hex((UINT8 *)&cmd_element[2][2], 8);
		if (reb_base)
			*(UINT32 *)(reb_base + reg) = reg_value;
		else
			ispi_reg_write(reg, reg_value);
	}
	else if (!strcmp(cmd_element[0], "finish"))
	{
		return 1;
	}
	else if (!strcmp(cmd_element[0], "dump"))
	{
		if (reb_base == 0)
			pll_dump_reg();
		printf("\n%s#", pname);
	}
	else
	{
		return -1;
	}

	return 0;
}

void cmd_input(char * pname, UINT32 reg_base)
{
	char cmd[48] = {0};
	int len = 0;

	ispi_open(0);
	printf("\n%s#", pname);
	while(1)
	{
		len = serial_gets((UINT8 *)cmd);
		printf("\n%s#", pname);
		if (len > 0)
		{
			int result = 0;

			result =  process_cmd(pname, cmd, reg_base);
			if (-1 == result)
				printf("command error! \n%s#", pname);
			else if (1 == result)
				break;
			else
				continue;
		}
	}
	printf("Command is finished .\n");
	return;
}

#endif /* DO_DDR_PLL_DEBUG */


#ifdef TGT_AP_SELECT_CLK

/* clock divider table size */
#define CLOCK_DIV_TAB_SIZE 60
const static UINT16 clock_div_tab[CLOCK_DIV_TAB_SIZE][2] = {
	{10, 0x1F},{11, 0x1F},{12, 0x1F},{13, 0x1F},{14, 0x1F},
	{15, 0x1F},{16, 0x1F},{17, 0x1F},{18, 0x1F},{19, 0x1F},
	{20, 0x1E},{21, 0x1E},{22, 0x1E},{23, 0x1E},{24, 0x1E},
	{25, 0x1D},{26, 0x1D},{27, 0x1D},{28, 0x1D},{29, 0x1D},
	{30, 0x1C},{31, 0x1C},{32, 0x1C},{33, 0x1C},{34, 0x1C},
	{35, 0x1B},{36, 0x1B},{37, 0x1B},{38, 0x1B},{39, 0x1B},
	{40, 0x1A},{41, 0x1A},{42, 0x1A},{43, 0x1A},{44, 0x1A},
	{45, 0x19},{46, 0x19},{47, 0x19},{48, 0x19},{49, 0x19},
	{50, 0x18},{51, 0x18},{52, 0x18},{53, 0x18},{54, 0x18},
	{55, 0x17},{56, 0x17},{57, 0x17},{58, 0x17},{59, 0x17},
	{60, 0x16},{61, 0x16},{62, 0x16},{63, 0x16},{64, 0x16},
	{65, 0x15},{66, 0x15},{67, 0x15},{68, 0x15},{69, 0x15},
};

static struct clock_config clock_select_config;

static void do_clock_selection(void)
{
	char choice = 'n';

	UINT32 set_cpu_clk = _TGT_AP_PLL_CPU_FREQ;
	UINT16 set_vpu_clk = 133,vpu_clk_div = _TGT_AP_CLK_VPU; //divided 6
	UINT16 set_gpu_clk = 200,gpu_clk_div = _TGT_AP_CLK_GPU; //divided 4
	UINT16 set_axi_clk = 400,axi_clk_div = _TGT_AP_CLK_AXI; //divided 2
	UINT32 real_vpu_clk = 0;
	UINT32 real_gpu_clk = 0;
	UINT32 real_axi_clk = 0;
	UINT16 clk_div;
	int i;

	printf("If you want to configure clock manully?(y=yes,n=no)\n");
	choice = serial_getc();
	if((choice != 'y') && (choice != 'Y')) {
		printf("Use default value as CPU,VPU,GPU AXI clock\n");
		return;
	}
	printf("\nYou are trying to set CPU,VPU,GPU AXI clock.\n"
		"Press <Enter> will use default value.\n\n");
	/* CPU clock selection process */
	choice = '1';
	printf("Set CPU clock:\n"
		"\t 1: 800 MHz\n"
		"\t 2: 910 MHz\n"
		"\t 3: 962 MHz\n");
	choice = serial_getc();
	switch(choice) {
	case '1':
		set_cpu_clk = 800;
		break;
	case '2':
		set_cpu_clk = 910;
		break;
	case '3':
		set_cpu_clk = 962;
		break;
	default:
		printf("Use default value: %d\n",(int)set_cpu_clk);
		break;
	}
	printf("CPU clock value is %d MHz\n",(int)set_cpu_clk);
	/* GPU clock selection process */
	choice = '1';
	printf("Set GPU clock:\n"
		"\t 1: 400 MHz\n"
		"\t 2: 320 MHz\n"
		"\t 3: 266 MHz\n");
	choice = serial_getc();
	switch(choice) {
	case '1':
		set_gpu_clk = 400;
		break;
	case '2':
		set_gpu_clk = 320;
		break;
	case '3':
		set_gpu_clk = 266;
		break;
	default:
		printf("Use default value: %d\n",(int)set_gpu_clk);
		break;
	}
	printf("GPU clock value is %d MHz\n",(int)set_gpu_clk);

	/* VPU clock selection process */
	choice = '1';
	printf("Set VPU clock:\n"
		"\t 1: 228 MHz\n"
		"\t 2: 200 MHz\n");
	choice = serial_getc();
	switch(choice) {
	case '1':
		set_vpu_clk = 228;
		break;
	case '2':
		set_vpu_clk = 200;
		break;
	default:
		printf("Use default value: %d\n",(int)set_vpu_clk);
		break;
	}
	printf("VPU clock value is %d MHz\n",(int)set_vpu_clk);
	choice = '1';
	printf("Set AXI clock:\n"
		"\t 1: 400 MHz\n"
		"\t 2: 320 MHz\n");
	choice = serial_getc();
	switch(choice) {
	case '1':
		set_axi_clk = 400;
		break;
	case '2':
		set_axi_clk = 320;
		break;
	case '3':
		set_axi_clk = 266;
		break;
	default:
		printf("Use default value: %d\n",(int)set_axi_clk);
		break;
	}
	printf("AXI clock value is %d MHz\n",(int)set_axi_clk);

	/* Get GPU clock div value */
	clk_div = (10 * set_cpu_clk) / set_gpu_clk;
	for ( i = 0; i < CLOCK_DIV_TAB_SIZE; i++) {
		if(clock_div_tab[i][0] == clk_div) {
			break;
		}
	}
	gpu_clk_div = clock_div_tab[i][1];
	real_gpu_clk = (10 * set_cpu_clk) / clk_div;

	/* Get VPU clock div value */
	clk_div = (10 * set_cpu_clk) / set_vpu_clk;
	for ( i = 0; i < CLOCK_DIV_TAB_SIZE; i++) {
		if(clock_div_tab[i][0] == clk_div) {
			break;
		}
	}
	vpu_clk_div = clock_div_tab[i][1];
	real_vpu_clk = (10 * set_cpu_clk) / clk_div;

	/* Get AXI clock div value */
	clk_div = (10 * set_cpu_clk) / set_axi_clk;
	for ( i = 0; i < CLOCK_DIV_TAB_SIZE; i++) {
		if(clock_div_tab[i][0] == clk_div) {
			break;
		}
	}
	axi_clk_div = clock_div_tab[i][1];
	real_axi_clk = (10 * set_cpu_clk) / clk_div;

	printf("\nAll clock value:\n");
	printf("CPU clk: %d\n",(int)set_cpu_clk);
	printf("GPU max clk: %d MHz, div val: 0x%x, real clk: %d MHz\n",(int)set_gpu_clk,(int)gpu_clk_div,(int)real_gpu_clk);
	printf("VPU max clk: %d MHz, div val: 0x%x, real clk: %d MHz\n",(int)set_vpu_clk,(int)vpu_clk_div,(int)real_vpu_clk);
	printf("AXI max clk: %d MHz, div val: 0x%x, real clk: %d MHz\n",(int)set_axi_clk,(int)axi_clk_div,(int)real_axi_clk);

	printf("Write to register ? (y=yes,n=no)\n");
	choice = 'y';
	choice = serial_getc();
	if((choice != 'y')&&(choice != 'Y')) {
		printf("Clock selection canceled\n");
		return;
	}
	/* Set CPU, GPU, VPU AXI clock value */
	memcpy(&clock_select_config,g_clock_config,
		sizeof(clock_select_config));
	clock_select_config.PLL_FREQ_CPU = set_cpu_clk;
	clock_select_config.CLK_GPU = gpu_clk_div;
	clock_select_config.CLK_VPU = vpu_clk_div;
	clock_select_config.CLK_AXI = axi_clk_div;
	g_clock_config = &clock_select_config;
	printf("Set Clock successful !!!\n");
}

#endif /* TGT_AP_SELECT_CLK */

#ifdef DO_STAY_FOR_WATCHING

void do_stay_for_watching_clock_reg(void)
{
	char ch;
	printf("Press <Enter> to continue\n");
	ch = serial_getc();
	ch = ch;
}

#endif /* DO_STAY_FOR_WATCHING */


#ifdef TGT_AP_DO_DDR_TEST

/* DDR test case list */
#define DDR_T_C_GET_INFO
#define DDR_T_C_FUNCTION
#define DDR_T_C_MEMORY_COPY
#define DDR_T_C_BOUNDARY_SCAN
#define DDR_T_C_PLL_SWITCH

/* DDR test result storage base address */
#define DDR_TEST_DATA_BASE 0x11C010C0
#define DDR_T_MANU_ID_ADDR	(DDR_TEST_DATA_BASE+0X00)
#define DDR_T_CAPACITY_ADDR	(DDR_TEST_DATA_BASE+0X04)
#define DDR_T_BAND_WIDTH_ADDR	(DDR_TEST_DATA_BASE+0X08)
#define DDR_T_PLL_SWITCH_ADDR	(DDR_TEST_DATA_BASE+0X0C)
#define DDR_T_FUNCTION_ADDR	(DDR_TEST_DATA_BASE+0X10)
#define DDR_T_BOUNDARY_ADDR	(DDR_TEST_DATA_BASE+0X14)
#define DDR_T_MEM_COPY_ADDR	(DDR_TEST_DATA_BASE+0X18)
#define DDR_T_MANU_NAME_ADDR	(DDR_TEST_DATA_BASE+0X1C)
#define DDR_T_MANU_NAME_SIZE	8

#define DDR_TEST_RESULT_OK	0XA55A6666
#define DDR_TEST_RESULT_ERROR	0XDEADDEAD

typedef unsigned int uint32;

typedef struct ddr_manufacture {
    UINT32 value;
    const char* name;
}DDR_MANUFACTURE;

const static DDR_MANUFACTURE g_ddr_type[] = {
	{0x0,"S4 SDRAM"},
	{0x1,"S2 SDRAM"},
	{0x2,"N NVM"},
};

const static DDR_MANUFACTURE g_ddr_manuacture[] ={
    {0x3, "Elpida"},
    {0x5, "Nanya"},
    {0xff, "Micron"}
};

const static UINT16 g_ddr_capacity_value[] = {
    8,
    16,
    32,
    64,
    128,
    256,
    512,
    1024,
    2048,
    4096
};

const static UINT16 g_ddr_io_width[]={
    32,
    16,
    8
};

extern unsigned int g_ddr_manufacture_id;
extern unsigned int g_ddr_capacity_id;
extern unsigned int con_dram_mrdata(unsigned int val);

static void write_ddr_test_result( unsigned int addr , unsigned int val);
static void do_set_gpo_pin(int pin_id, int pin_val);

int get_dram_info(void)
{
	int i = 0;
	UINT8 ddr_cap_id = 0;
	UINT8 ddr_band_id = 0;
	UINT8 ddr_type_id = 0;
	char *ptr_addr;
	const char *ptr_str;

	printf("Read dram info... ");
	ddr_cap_id = (g_ddr_capacity_id & 0x3C)>>2; //bit 2 3 4 5
	ddr_band_id = (g_ddr_capacity_id & 0xC0)>>6; // bit 6 7
	if((ddr_band_id >=0x1) || (ddr_cap_id < 0x4)) {
		g_ddr_manufacture_id = con_dram_mrdata(g_ddr_manufacture_id);
		g_ddr_capacity_id = con_dram_mrdata(g_ddr_capacity_id);
	}
	for(i = 0; i < 3; i++) {
		if((g_ddr_manufacture_id & 0xFF) == g_ddr_manuacture[i].value) {
			break;
		}
	}
	if(i > 2) {
		printf("unknown manufacture id: 0x%x\n",g_ddr_manufacture_id);
		return -1;
	}

	ddr_cap_id = (g_ddr_capacity_id & 0x3C)>>2; //bit 2 3 4 5
	ddr_band_id = (g_ddr_capacity_id & 0xC0)>>6; // bit 6 7
	ddr_type_id = (g_ddr_capacity_id & 0x03); //bit 0 1
	if(ddr_type_id > 2) {
		printf("unknown type id 0x%x\n",ddr_type_id);
		return -1;
	}
	if(ddr_band_id > 2) {
		printf("unknown band id 0x%x\n",ddr_band_id);
		return -1;
	}
	if(ddr_cap_id > 9) {
		printf("unknown capacity id 0x%x\n",ddr_cap_id);
		return -1;
	}
	printf("%s, ",g_ddr_type[ddr_type_id].name);
	printf("%s, ", g_ddr_manuacture[i].name);
	printf("id: 0x%x, ", (int)g_ddr_manuacture[i].value);
	printf("bandwidth: %d bit, ", g_ddr_io_width[ddr_band_id]);
	printf("capacity: %d MB\n", g_ddr_capacity_value[ddr_cap_id]);

	writel((int)(g_ddr_io_width[ddr_band_id]),DDR_T_BAND_WIDTH_ADDR);
	writel((int)(g_ddr_capacity_value[ddr_cap_id]),DDR_T_CAPACITY_ADDR);
	writel((int)(g_ddr_manuacture[i].value),DDR_T_MANU_ID_ADDR);

	ptr_str = (g_ddr_manuacture[i].name);
	ptr_addr = (char *)(DDR_T_MANU_NAME_ADDR);
	for(i = 0;i < DDR_T_MANU_NAME_SIZE; i++)
		*ptr_addr++ = '\0';
	ptr_addr = (char *)(DDR_T_MANU_NAME_ADDR);
	for(i = 0;i < DDR_T_MANU_NAME_SIZE; i++)
		*ptr_addr++ = *ptr_str++;
	return 0;
}

#define DMC_STAT_CONFIG 0
#define DMC_STAT_READY	3
#define DMC_REG_BASE	RDA_DMC400_BASE
#define DRAM_RW_TEST_DEBUG 0
#include "ddr_init.h"
static u32 get_dram_paramter(unsigned int offset_addr)
{
	unsigned int reg = 0;
	unsigned status;

	writel(DMC_STAT_CONFIG, DMC_REG_BASE + MEMC_CMD);
	do {
		status = readl(DMC_REG_BASE + MEMC_STATUS);
#if DRAM_RW_TEST_DEBUG
		printf("dmc status is %d\n",(int)status);
#endif
	} while (status != DMC_STAT_CONFIG);

	reg = readl(RDA_DMC400_BASE+offset_addr);
#if DRAM_RW_TEST_DEBUG
	printf("read dmc reg 0x%x = 0x%x\n",offset_addr, reg);
#endif
	writel(DMC_STAT_READY, DMC_REG_BASE + MEMC_CMD);
	do {
		status = readl(DMC_REG_BASE + MEMC_STATUS);
#if DRAM_RW_TEST_DEBUG
		printf("dmc status is %d\n",(int)status);
#endif
	} while (status != DMC_STAT_READY);

	return reg;
}
static int parse_dram_address(u32 *row_bits, u32 *col_bits, u32 *bank_bits, u32 *channel_bits,u32 *chip_bits,u32 *addr_order)
{
	u32 reg_val = 0,t;

	// Get ADDR_CTRL register value
	reg_val = get_dram_paramter(ADDR_CTRL);

	/* Get column bits number */
	t = reg_val & 0x0F;
	if(t < 5)
		*col_bits = 8 + t;
	else
		return -1;

	/* Get row bits number */
	t = (reg_val & 0x0F00)>>8;
	if((t >= 2)&&(t <= 5))
		*row_bits = 11 + t;
	else
		return -1;

	/* Get bank bits number */
	t = (reg_val & 0xF0000)>>16;
	if((t == 2)||(t == 3))
		*bank_bits = t;
	else
		return -1;

	/* Get chip bits number */
	t = (reg_val & 0x03000000)>>24;//bit24, bit25
	if((t == 0)||(t == 1))
		*chip_bits = t;
	else
		return -1;

	/* Get channel bits number */
	t = (reg_val & 0x30000000)>>28;//bit28, bit29
	if((t == 0)||(t == 1))
		*channel_bits = t;
	else
		return -1;

	/* Get decode control register value */
	reg_val = get_dram_paramter(DECODE_CTRL);
	*addr_order = reg_val & 0x03;
	return 0;
}
/*
 ****************************************************************************************
 *	@name:	ddr_rw_data_test
 *	@description:	DRAM reading and writing row,bank,column address test.
 *	@parameter
 *		dram_offset_addr: the DRAM's offset address
 * 		reverse_data: 	0, data will be reversed,such as 0xfe,0xfd,0xfc...
 *		 		1, adta will not be reversed,such as 0x1,0x2,0x3...
 * 		is_addr32: 	0, 8 bit address reading or writing
 *	      			1, 32 bit address reading or writing
 *	@return:	0(OK), -1(FAILED)
 ****************************************************************************************
*/
int ddr_rw_data_test(unsigned int dram_offset_addr, unsigned int reverse_data, unsigned int is_addr32)
{
	unsigned int dat,rd_dat;
	unsigned int temp_addr = 0,row_addr = 0,col_addr = 0,bank_addr = 0, end_addr = 0,dram_base_addr = 0x80000000;
	unsigned int row_bits,col_bits,bank_bits,channel_bits,chip_bits,addr_order;
	int i,is_err = 0,err_cnt = 0;
	/* Parse DRAM address parameters */
	i = parse_dram_address(&row_bits, &col_bits, &bank_bits, &channel_bits, &chip_bits, &addr_order);
	if( i != 0) {
		printf("Parse DRAM address parameter failed.\n");
		return -1;
	}
	dram_base_addr += dram_offset_addr;
	printf("\nDRAM RW test,offset addr 0x%x,",dram_offset_addr);
	if(is_addr32)
		printf("32 bit,");
	else
		printf("8 bit,");
	if(reverse_data)
		printf("reversed data\n");
	else
		printf("no reversed data\n");

	/* Row address 8 bit or 32 bit data writing */
	printf("\nStart row address RW test\n");
	printf("Writing row data...\n");
	row_addr = 0;
	dat = 0;
	i = 0;
	end_addr = 0x100; //writing ending address
	while(i < end_addr){
		if(reverse_data)
			dat = (unsigned int)~i;
		else
			dat = i;
		row_addr = (unsigned int )i;
		if(addr_order == 0x00)
			temp_addr = (row_addr)<<(col_bits+bank_bits);
		else if(addr_order == 0x02)
			temp_addr = (row_addr)<<(col_bits+channel_bits);
		else
			temp_addr = (row_addr)<<(channel_bits + chip_bits + col_bits + bank_bits);

		if(is_addr32){
			//32bit data address
			*(unsigned int *)(dram_base_addr + temp_addr) = (unsigned int )dat;
			i += 4;
		} else {
			//8bit data address
			dat = dat & 0xFF;
			*(unsigned char *)(dram_base_addr + temp_addr) = (unsigned char)dat;
			i += 1;
		}
#if DRAM_RW_TEST_DEBUG
		printf("Row addr: 0x%x, data: 0x%x\n", (dram_base_addr + temp_addr), dat);
#endif
	}
	/* Row address 8 bit or 32 bit data reading */
	printf("Reading row data...\n");
	row_addr = 0;
	dat = 0;
	i = 0;
	is_err = 0;
	err_cnt = 0;
	while(i < end_addr){
		if(reverse_data)
			dat = (unsigned int)~i;
		else
			dat = i;
		row_addr = i;
		if(addr_order == 0x00)
			temp_addr = (row_addr)<<(col_bits+bank_bits);
		else if(addr_order == 0x02)
			temp_addr = (row_addr)<<(col_bits+channel_bits);
		else
			temp_addr = (row_addr)<<(channel_bits + chip_bits + col_bits + bank_bits);

		if(is_addr32) {
			//32bit data address
			rd_dat = *(unsigned int *)(dram_base_addr + temp_addr);
			i += 4;
		} else {
			//8bit data address
			dat = dat & 0xFF;
			rd_dat = *(unsigned char *)(dram_base_addr + temp_addr);
			i += 1;
		}
#if DRAM_RW_TEST_DEBUG
		printf("Row addr: 0x%x, data: 0x%x\n", (dram_base_addr + temp_addr), rd_dat);
#endif
		if ( rd_dat != dat) {
			is_err = 1;
			err_cnt++;
		}
	}
	if(!is_err) {
		printf("Row address RW test passed !!!\n");
	}else{
		printf("Row address RW test failed ###\t, error count is 0x%d\n",err_cnt);
		return -1;
	}
	/* Start to test column address reading ,writing */
	printf("\nStart column address RW test\n");
	printf("Writing column data...\n");
	col_addr = 0;
	dat = 0;
	i = 0;
	end_addr = 0x100;
	while(i < end_addr) {
		if(reverse_data)
			dat = (unsigned int)~i;
		else
			dat = i;
		col_addr = (unsigned int)i;
		temp_addr = col_addr;
		if(is_addr32){
			*(unsigned int *)(dram_base_addr + temp_addr) = dat;
			i += 4;
		} else {
			dat = dat & 0xFF;
			*(unsigned char *)(dram_base_addr + temp_addr) = (unsigned char)dat;
			i += 1;
		}
#if DRAM_RW_TEST_DEBUG
		printf("Column addr: 0x%x, data: 0x%x\n", (dram_base_addr + temp_addr), dat);
#endif
	}
	printf("Reading column data...\n");
	col_addr = 0;
	dat = 0;
	i = 0;
	err_cnt = 0;
	is_err = 0;
	while(i < end_addr) {
		if(reverse_data)
			dat = (unsigned int)~i;
		else
			dat = i;
		col_addr = (unsigned int)i;
		temp_addr = col_addr;
		if(is_addr32){
			rd_dat = *(unsigned int *)(dram_base_addr + temp_addr);
			i += 4;
		} else {
			dat = dat & 0xFF;
			rd_dat = *(unsigned char *)(dram_base_addr + temp_addr);
			i += 1;
		}
#if DRAM_RW_TEST_DEBUG
		printf("Column addr: 0x%x, data: 0x%x\n", (dram_base_addr + temp_addr), rd_dat);
#endif
		if ( rd_dat != dat) {
			is_err = 1;
			err_cnt++;
		}
	}
	if(!is_err) {
		printf("Column address RW test passed !!!\n");
	}else{
		printf("Column address RW test failed ###\t, error count is 0x%d\n",err_cnt);
		return -1;
	}
	/* Start to test bank address reading and writing */
	printf("\nStart bank address RW test\n");
	printf("Writing bank data...\n");
	bank_addr = 0;
	dat = 0;
	i = 0;
	end_addr = 0x8;
	while(i < end_addr) {
		if(reverse_data)
			dat = (unsigned int)~i;
		else
			dat = i;
		bank_addr = (unsigned int)i;
		if(addr_order == 0x02)
			temp_addr = (bank_addr)<<(col_bits + channel_bits + row_bits);
		else if(addr_order == 0x03)
			temp_addr = (bank_addr)<<(col_bits+channel_bits);
		else
			temp_addr = (bank_addr)<<(col_bits);

		if(is_addr32){
			*(unsigned int *)(dram_base_addr + temp_addr) = dat;
			i += 4;
		} else {
			dat = dat & 0xFF;
			*(unsigned char *)(dram_base_addr + temp_addr) = (unsigned char)dat;
			i += 1;
		}
#if DRAM_RW_TEST_DEBUG
		printf("Bank addr: 0x%x, data: 0x%x\n", (dram_base_addr + temp_addr), dat);
#endif
	}
	printf("Reading bank data...\n");
	bank_addr = 0;
	dat = 0;
	i = 0;
	err_cnt = 0;
	is_err = 0;
	while(i < end_addr) {
		if(reverse_data)
			dat = (unsigned int)~i;
		else
			dat = i;
		bank_addr = (unsigned int)i;
		if(addr_order == 0x02)
			temp_addr = (bank_addr)<<(col_bits + channel_bits + row_bits);
		else if(addr_order == 0x03)
			temp_addr = (bank_addr)<<(col_bits+channel_bits);
		else
			temp_addr = (bank_addr)<<(col_bits);

		if(is_addr32){
			rd_dat = *(unsigned int *)(dram_base_addr + temp_addr);
			i += 4;
		} else {
			dat = dat & 0xFF;
			rd_dat = *(unsigned char *)(dram_base_addr + temp_addr);
			i += 1;
		}
#if DRAM_RW_TEST_DEBUG
		printf("Bank addr: 0x%x, data: 0x%x\n", (dram_base_addr + temp_addr), rd_dat);
#endif
		if ( rd_dat != dat) {
			is_err = 1;
			err_cnt++;
		}
	}
	if(!is_err) {
		printf("Bank address RW test passed !!!\n");
	}else{
		printf("Bank address RW test failed ###\t, error count is 0x%d\n",err_cnt);
		return -1;
	}
	return 0;
}

int ddr_memory_copy_test(uint32 src_addr, uint32 des_addr, uint32 nwords)
{
	int ret = 0;
	unsigned int dat = 0x00,t = 0;;
	uint32 count;
	volatile unsigned int *psrc, *pdes;

	printf("ddr memory copy test: from  0x%x to 0x%x,0x%x words\n",
		src_addr, des_addr, nwords);
	/* write n bytes to src address */
	psrc = (volatile unsigned int *)src_addr;
	pdes = (volatile unsigned int *)des_addr;
	count = nwords;
	dat = 0;
	while(count--)  {
		*psrc++ = dat++;
	}
	/* copy bytes from src addr to des addr */
	psrc = (volatile unsigned int *)src_addr;
	pdes = (volatile unsigned int *)des_addr;
	count = nwords;
	while(count--) {
		dat = *psrc++;
		*pdes++ = dat;
	}
	/* compare data */
	psrc = (volatile unsigned int *)src_addr;
	pdes = (volatile unsigned int *)des_addr;
	count = nwords;
	dat = 0;
	while(count--) {
		t = *psrc++;
		if(dat != t) {
			printf("compare data error,position = %d\n",(int)count);
			ret = -1;
			break;
		}
		dat++;
	}
	return ret;
}

int ddr_boundscan_test(uint32 addr, uint32 nwords)
{
	unsigned int dat = 0;
	uint32 count = nwords;
	volatile unsigned int *pdes;

	printf("ddr boundary scan test: addr: 0x%x, nwords: 0x%x\n",
		addr,nwords);
	if(nwords == 0)
		return -1;
	/* read data from boundary address */
	pdes = (volatile unsigned int *)addr;
	while(count--) {
		dat = *pdes;
		dat = dat;
		printf("0x%x\n",dat);
		pdes++;
	}
	return 0;
}

int ddr_pll_switch_test(int times)
{
	printf("ddr pll switch test times: %d\n",times);
	while(times--) {
		sys_shutdown_pll();
		//ispi_open(0);
		//pll_setup_init();
		//pll_setup_freq();
		sys_setup_pll();
	}
	return 0;
}

static void do_set_gpo_pin(int pin_id, int pin_val)
{
	unsigned int set_addr = 0x20930030; //GPIOA 's GPO register set address
	unsigned int clr_addr = 0x20930034; // GPIOA 's GPO register reset address

	switch(pin_id) {
	case 0:
		if(pin_val)
			*(volatile unsigned int *)set_addr = 0x01;//gpo 0 = 1
		else
			*(volatile unsigned int *)clr_addr = 0x01;//gpo 0 = 0
	break;
	case 1:
		if(pin_val)
			*(volatile unsigned int *)set_addr = 0x02;//gpo 1 = 1
		else
			*(volatile unsigned int *)clr_addr = 0x02;//gpo 1 = 0
	break;
	case 2:
		if(pin_val)
			*(volatile unsigned int *)set_addr = 0x04;//gpo 2 = 1
		else
			*(volatile unsigned int *)clr_addr = 0x04;
	break;
	default:
	break;
	}
#if 0
	{
		unsigned int val = *(volatile unsigned int *)set_addr;
		printf("set_gpo_reg value : %x\n",val);
	}
#endif

}

static void write_ddr_test_result( unsigned int addr , unsigned int val)
{
	writel(val,addr);
#if 0
	printf("Write ddr test result: addr =  0x%x,val = 0x%x\n",addr,val);
#endif
}

static void ddr_test_entry(void)
{
	int res[7]= {0},j = 0,err = 0;
	uint32 ddr_start_addr = 0x80000000, ddr_end_addr = 0xbfffffff, scan_len = 6;

	printf("\n%s: current ticks = %llu\n",__func__,get_ticks());
	printf("--------------------DDR test starts-------------------------\n");
	printf("MP:DDR test bootloader 4.0\n");
	printf("SV:0x10\n");

	/* ddr test starts */
	do_set_gpo_pin(1,0);

	/* reset gpo0 to low level */
	do_set_gpo_pin(0,0);

	/* avoid generating building warnning*/
	for(j = 0;j<7;j++)
		res[j] = 0;

	ddr_start_addr = ddr_start_addr;
	ddr_end_addr = ddr_end_addr;
	scan_len = scan_len;

#ifdef DDR_T_C_PLL_SWITCH
	/* ddr pll switch test */
	res[0] = ddr_pll_switch_test(100);
#endif

#ifdef DDR_T_C_GET_INFO
	/* get ddr information */
	res[1] = get_dram_info();
#endif

#ifdef DDR_T_C_FUNCTION
	/* ddr function test */
	res[2]  = ddr_rw_data_test(0x01000000,0,0);
	res[2] += ddr_rw_data_test(0x02000000,0,1);
	res[3]  = ddr_rw_data_test(0x03000000,1,0);
	res[3] += ddr_rw_data_test(0x04000000,1,1);
	if((res[2] == 0) && (res[3] == 0)) {
		printf("ddr function test OK\n");
		write_ddr_test_result(DDR_T_FUNCTION_ADDR,DDR_TEST_RESULT_OK);
	} else {
		printf("ddr function test error,res1 = %d,res2 = %d\n",res[2],res[3]);
		write_ddr_test_result(DDR_T_FUNCTION_ADDR,DDR_TEST_RESULT_ERROR);
	}
#endif

#ifdef DDR_T_C_BOUNDARY_SCAN
	/* ddr boundary scan test from 0x80000000 to 0xBFFFFFFF */
	scan_len = 3;
	ddr_start_addr = 0x80000000;
	ddr_end_addr = 0xBFFFFFFF;
	res[4] = ddr_boundscan_test(ddr_start_addr, scan_len);
	res[5] = ddr_boundscan_test((ddr_end_addr-scan_len), scan_len);
	if((res[4] == 0) && (res[5] == 0)) {
		printf("ddr boundary scan test OK\n");
		write_ddr_test_result(DDR_T_BOUNDARY_ADDR,DDR_TEST_RESULT_OK);
	} else {
		printf("ddr boudary scan test error, res = 0x%x\n",res[4]);
		write_ddr_test_result(DDR_T_BOUNDARY_ADDR,DDR_TEST_RESULT_ERROR);
	}
#endif

#ifdef DDR_T_C_MEMORY_COPY
	/* ddr memory copy test */
	res[6] = ddr_memory_copy_test(0x84000000, 0x85000000, 0x1000);
	if(res[6] == 0) {
		printf("ddr memory copy test OK\n");
		write_ddr_test_result(DDR_T_MEM_COPY_ADDR,DDR_TEST_RESULT_OK);
	} else {
		printf("ddr memory copy test error, res = 0x%x\n",res[6]);
		write_ddr_test_result(DDR_T_MEM_COPY_ADDR,DDR_TEST_RESULT_ERROR);
	}
#endif

#ifdef DDR_T_C_PLL_SWITCH
	printf("ddr pll switch test OK\n");
	write_ddr_test_result(DDR_T_PLL_SWITCH_ADDR, DDR_TEST_RESULT_OK);
#endif
	/*check all test case result */
	err = 0;
	for(j = 0; j<7;j++){
		if(res[j] != 0){
			err = -1;
			break;
		}
	}
	if(!err) {
		do_set_gpo_pin(0,1);//test result ok
		printf("\nDDR test successfully !!!\n");
	}
	/* ddr test ends */
	do_set_gpo_pin(1,1);//gpo1 set high level
	printf("--------------------DDR test ends-------------------------\n");
	printf("\n%s: current ticks = %llu\n",__func__,get_ticks());
}
#endif /* TGT_AP_DO_DDR_TEST */

#ifdef CONFIG_MEM_WR_TEST
void mem_test_write(void)
{
        volatile unsigned int *addr;
        int pos = 0, cnt = 0;

        printf("Start writing DDR test!!!!!!\n");
        addr = (volatile unsigned int *)(0x83000000);
        while((u32)addr < 0x83008000) {
                pos = cnt % 10;
                *addr = test_pattern[pos];
                addr++;
                cnt++;
        }
}

int mem_test_read(void)
{
        volatile unsigned int *addr;
        unsigned int temp= 0;
        int pos = 0, cnt = 0, i = 0;

        printf("Start reading DDR test!!!!\n");

        addr = (volatile unsigned int *)(0x83000000);

        while((u32)addr < 0x83008000) {

                pos = cnt % 10;
                temp = test_pattern[pos];
                if ( temp != *addr) {
			printf("test error!!!!!, addr = %x\n", (unsigned int)addr);
			//printf("GPIO output,addr=0x20930030,value=0x01\n");
			//*(volatile unsigned int *)0x20930030 = 0x01;

                        for (i = 0; i < 80;i++){
                                printf("%x ", *(addr + i));
                                if ((i % 4) == 0)
                                        printf("\n");
                        }

                        return -1;
                }

                addr++;
                cnt++;
        }
        serial_puts("DDR test OK********************************!\n");
	//printf("GPIO output,addr = 0x20930030,value=0x02;\n");
	//*(volatile unsigned int *)0x20930030 = 0x02;
        return 0;
}
#endif /* CONFIG_MEM_WR_TEST */

void sys_config_dcache_ema(unsigned int ema_val)
{
	unsigned int val;
	unsigned int addr = RDA_APDEBUG_BASE + 0x24;
	val = readl(addr);
	/* clear bit 8,7,6 */
	val &= (~(0x7 << 6));
	val |= ((ema_val & 0x7) << 6);
	writel(val,addr);
}

void sys_config_dram_qos(unsigned int idx)
{
	unsigned int val;
	//L2cc_Ctrl register address
	unsigned int addr = RDA_SYSCTRL_BASE + 0x100;
	val = readl(addr);
	val &= 0xFFFF00FF;
	val |= (idx << 8);
	val |= (idx << 12);
	writel(val,addr);
}

int clock_init(void)
{
#ifdef DO_DDR_PLL_DEBUG
	char choice = 'n';
#endif

	/* First check current usb usage */
	check_usb_usage();

	printf("Init Clock ...\n");
	g_clock_config = get_default_clock_config();

	printf("Clock config ver: %d.%d\n",
		g_clock_config->VERSION_MAJOR, g_clock_config->VERSION_MINOR);

	pmu_setup_init();

#ifdef TGT_AP_SELECT_CLK
	do_clock_selection();
#endif /* TGT_AP_SELECT_CLK */

#ifdef DO_DDR_PLL_DEBUG
	printf("If you want to config the ddr para manully ?(y = yes, n = no) \n");
	choice = serial_getc();
	if (choice == 'y') {
		memcpy(&clock_debug_config, g_clock_config,
				sizeof(clock_debug_config));
		g_clock_config = &clock_debug_config;
		pmu_setup_calibration_voltage(1);
		pmu_setup_calibration_voltage(2);
		//pmu_buck4_buck3_choose();
		freq_choose();
		data_bits_choose();
		clock_load_ddr_cal_config();
	}
#endif

	sys_shutdown_pll();
	ispi_open(0);
	pll_setup_init();
#ifdef DO_DDR_PLL_DEBUG
	if (choice == 'y')
		cmd_input("ddrPll", 0);
#endif
	pll_setup_freq();
	sys_setup_pll();
	sys_setup_clk();
	/* modem use qos6_ctrl register */
	sys_config_dram_qos(6);
	/* set l2 d-cache ema is 0 */
	sys_config_dcache_ema(0);

	if (g_clock_config->DDR_CAL) {
		printf("Init DDR for ddr_cal\n");
		ddr_init(g_clock_config->DDR_FLAGS, g_clock_config->DDR_PARA);
		printf("Done\n");
		pll_setup_mem_cal();
		clock_save_config();
	} else {
		pll_setup_mem();
	}

	sys_dump_pll_freq();
	sys_dump_clk();
	pll_dump_reg();

	printf("Init DDR, flag = 0x%04x, para = 0x%08lx\n",
		g_clock_config->DDR_FLAGS, g_clock_config->DDR_PARA);
	ddr_init(g_clock_config->DDR_FLAGS, g_clock_config->DDR_PARA);
	printf("Done\n");

#ifdef TGT_AP_DO_DDR_TEST
	ddr_test_entry();
#endif /* TGT_AP_DO_DDR_TEST */

#ifdef CONFIG_MEM_WR_TEST
	mem_test_write();
	mem_test_read();
#endif /* CONFIG_MEM_WR_TEST */

#ifdef DO_STAY_FOR_WATCHING
	do_stay_for_watching_clock_reg();
#endif /* DO_STAY_FOR_WATCHING */

	return 0;
}
#else /* CONFIG_RDA_FPGA */

int clock_init(void)
{
	u16 ddr_flags = DDR_FLAGS_DLLOFF
		| DDR_FLAGS_ODT(1)
		| DDR_FLAGS_RON(0);
	//16bit
	u32 ddr_para = DDR_PARA_MEM_BITS(2)
		| DDR_PARA_BANK_BITS(3)
		| DDR_PARA_ROW_BITS(4)
		| DDR_PARA_COL_BITS(1);

	printf("Init DDR\n");
	ddr_init(ddr_flags, ddr_para);
	printf("Done\n");

#ifdef TGT_AP_DO_DDR_TEST
	ddr_test_entry();
#endif /* TGT_AP_DO_DDR_TEST */

#ifdef CONFIG_MEM_WR_TEST
	mem_test_write();
	mem_test_read();
#endif /* CONFIG_MEM_WR_TEST */

	return 0;
}

#endif /* CONFIG_RDA_FPGA */
