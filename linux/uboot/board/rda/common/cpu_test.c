#include <common.h>
#include <asm/io.h>
#include <asm/arch/reg_sysctrl.h>

#ifdef CONFIG_MACH_RDA8810
static void apsys_cpupll_switch(int on)
{
	volatile u32 *clock_reg = &hwp_sysCtrlAp->Sel_Clock;
	volatile u32 *pll_ctrl_reg = &hwp_sysCtrlAp->Cfg_Pll_Ctrl[0];

	if (on) {
		int val;
		u32 clk_value = readl(clock_reg);

		val  = SYS_CTRL_AP_AP_PLL_ENABLE_ENABLE |
			SYS_CTRL_AP_AP_PLL_LOCK_RESET_NO_RESET |
			SYS_CTRL_AP_AP_PLL_LOCK_NUM_LOW(6)|
			SYS_CTRL_AP_AP_PLL_LOCK_NUM_HIGH(30);
		writel(val, pll_ctrl_reg);
		val = readl(pll_ctrl_reg);
		val |= SYS_CTRL_AP_AP_PLL_CLK_FAST_ENABLE_ENABLE;
		writel(val, pll_ctrl_reg);

		clk_value &= ~(1 << 4);
		writel(clk_value, clock_reg);
	} else {
		int val;
		u32 clk_value = readl(clock_reg);

		clk_value |= SYS_CTRL_AP_CPU_SEL_FAST_SLOW;
		writel(clk_value, clock_reg);
		val = SYS_CTRL_AP_AP_PLL_ENABLE_POWER_DOWN |
			SYS_CTRL_AP_AP_PLL_LOCK_RESET_RESET;
		writel(val, pll_ctrl_reg);
	}
}
#else
static void apsys_cpupll_switch(int on)
{
}
#endif

void cpu_pll_test(int times)
{
	int i;

	printf("cpu pll switch test....\n");
	for(i = 0; i < times; i++) {
		printf("on/off %d times\r", i+1);
		apsys_cpupll_switch(0);
		udelay(100);
		apsys_cpupll_switch(1);
		udelay(100);
	}
	printf("\n cpu pll finished\n");

}

