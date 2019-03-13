#include <linux/module.h>
#include <asm/cacheflush.h>
#include <asm/suspend.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/pm.h>
#include <linux/suspend.h>
#include <plat/rda_debug.h>
#include <plat/intc.h>
#include <plat/ap_clk.h>
#include <mach/system.h>
#include <mach/irqs.h>
#include <asm/io.h>
#include <mach/hardware.h>
#include <plat/reg_intc.h>
#include <plat/cpufreq.h>

int (*pm_cpu_sleep)(unsigned long);
#ifdef CONFIG_RDA_SLEEP_OFF_MODE
#include "sleep.h"

#define RDA_PM_WAKEUP_DEBUG
#endif

#ifdef RDA_PM_WAKEUP_DEBUG
#define RDA_PM_WAKEUP_MAGIC_ADDR (void *)(RDA_SRAM_BASE\
	       +(CONFIG_WAKEUP_JUMP_ADDR - RDA_SRAM_PHYS))
static void rda_pm_wakeup_debug_poke(void)
{
	uint32_t flag;

	flag = readl(RDA_PM_WAKEUP_MAGIC_ADDR);
	flag++;
	writel(flag, RDA_PM_WAKEUP_MAGIC_ADDR);
	dsb();
}
#else
static void rda_pm_wakeup_debug_poke(void) {}
#endif

#ifndef CONFIG_RDA_FPGA

#ifdef CONFIG_RDA_SLEEP_OFF_MODE

extern void rda_wakeup_entry(void);
extern uint32_t rda_wakeup_entry_sz;

static void rda_pm_setup_off_mode(void)
{
	uint32_t code_addr = 0;
	uint32_t flag_addr = 0;
	void __iomem *sram_base = (void __iomem *)IO_ADDRESS(RDA_SRAM_BASE);
	void __iomem *imem_base = (void __iomem *)IO_ADDRESS(RDA_IMEM_BASE);
	uint32_t entry_addr = (uint32_t) rda_wakeup_entry;

#ifdef RDA_PM_WAKEUP_DEBUG
	memset(sram_base, 0xff, RDA_SRAM_SIZE);
#endif
	/* copy wakeup code to sram*/
	code_addr = (uint32_t)sram_base +
		(CONFIG_WAKEUP_CODE_ADDR - RDA_SRAM_PHYS);
	rda_dbg_pm("copy wakeup code, to 0x%x\n", code_addr);
	memcpy((void *)code_addr, (void *) (entry_addr & 0xFFFFFFFE),
			rda_wakeup_entry_sz);
	rda_dbg_pm("copy wakeup code size %d done\n", rda_wakeup_entry_sz);
	/* setup wakeup flag */
	flag_addr = (uint32_t)sram_base +
		(CONFIG_WAKEUP_JUMP_ADDR - RDA_SRAM_PHYS);
	rda_dbg_pm("write wakeup flag, to 0x%x\n", flag_addr);
	writel(CONFIG_WAKEUP_JUMP_MAGIC, (void __iomem *)flag_addr);
	writel(virt_to_phys(rda_cpu_resume), (void __iomem *)(flag_addr + 4));
	writel(0, (void __iomem *)(flag_addr + 8)); /*dbg area*/
	rda_dbg_pm("write wakeup flag done\n");

	/* we patch ROM address 0, to program patch0 with 0
	 write jump inst into SRAM + 0, max 4 instructions */
	rda_dbg_pm("write patch0, to 0x%x\n", (uint32_t)sram_base);
#ifndef CONFIG_THUMB2_KERNEL
	/* we write following instructions
	 * 00000000 <_start>:
	 * 0:       e59f2000        ldr     r2, [pc, #0]    ; 8 <patch_addr>
	 * 4:       e1a0f002        mov     pc, r2
	 * 00000008 <patch_addr>:
	 * 8:       0010c120        .word   0x0010c120
	 */
	writel(0xe59f2000, sram_base + 0);
	writel(0xe1a0f002, sram_base + 4);
	writel(0x0010c120, sram_base + 8);
#else

	/* we write following instructions
 	 * 0:  	e59f2000 	ldr	r2, [pc]	;  <reset+0x8>
	 * 4: 	e12fff12 	bx	r2
	 * 8: 	0010c121 	.word	0x0010c121
	*/
	writel(0xe59f2000, sram_base + 0);
	writel(0xe12fff12, sram_base + 4);
	writel(0x0010c121, sram_base + 8);
#endif
	rda_dbg_pm("write patch0 done\n");

	/* finally, write patch0 register */
	rda_dbg_pm("write patch0 register, to 0x%x\n", (uint32_t)imem_base);
	writel((1<<31) | 0, imem_base + 0);
	rda_dbg_pm("write patch0 register done\n");
}
#endif /* CONFIG_RDA_SLEEP_OFF_MODE */

#endif /* CONFIG_RDA_FPGA */

#ifndef CONFIG_RDA_FPGA

static int rda_pm_enter(suspend_state_t state)
{
	uint32_t wakeup_mask = 0;
	uint32_t cpu_freq;
	uint32_t min;

#ifdef CONFIG_MACH_RDA8810
	wakeup_mask = rda_intc_get_mask();
#elif defined(CONFIG_ARM_GIC)
	wakeup_mask = rda_gic_get_wakeup_mask();
#else
#error "Unknown MACH"
#endif

	rda_dbg_pm("enter suspend, wakeup_mask = 0x%08x\n",
			wakeup_mask);

#ifdef CONFIG_ARM_GIC
	/* need to unmask first, since irq is no longer used as irq_chip */
	rda_intc_unmask_irqs(wakeup_mask);
#endif
	rda_intc_set_wakeup_mask(wakeup_mask);


#ifdef CONFIG_RDA_SLEEP_OFF_MODE
	rda_pm_setup_off_mode();

	/* ChenGang - we do it just before suspend.
	 * flush cache back to ram
	 * */
	//flush_cache_all();
#endif

	cpu_freq = apsys_get_cpu_clk_rate();
	min = rda_get_cpufreq_min();
	if (cpu_freq != min)
		apsys_adjust_cpu_clk_rate(min);

	if (0 == cpu_suspend(0, pm_cpu_sleep)) {

		rda_pm_wakeup_debug_poke();
#ifdef CONFIG_RDA_SLEEP_OFF_MODE
		apsys_acknowledge_sleep_off();
#endif
	} else {
		rda_dbg_pm("cpu suspend failed\n");
	}

	rda_pm_wakeup_debug_poke();
	if (cpu_freq != min)
		apsys_adjust_cpu_clk_rate(cpu_freq);

	rda_pm_wakeup_debug_poke();
	rda_dbg_pm("exit suspend, min/restoring  cpu freq is %d/%d\n",
			min, cpu_freq);
	return 0;
}

static void rda_pm_wake(void)
{
	uint32_t wakeup_cause = 0;

	rda_intc_get_wakeup_cause(&wakeup_cause);
	rda_dbg_pm("wakeup, wakeup_cause = 0x%08x\n",
			wakeup_cause);
}
#else
static int rda_pm_enter(suspend_state_t state)
{
	rda_dbg_pm("enter suspend, FPGA call arch_idle\n");
	arch_idle();
	return 0;
}

static void rda_pm_wake(void)
{
	rda_dbg_pm("wakeup, FPGA\n");
}
#endif

static struct platform_suspend_ops rda_pm_ops = {
	.enter		= rda_pm_enter,
	.wake		= rda_pm_wake,
	.valid		= suspend_valid_only_mem,
};

static int rda_cpu_sleep(unsigned long arg)
{
#ifdef CONFIG_RDA_SLEEP_OFF_MODE
	flush_cache_all();
#ifdef CONFIG_CACHE_L2X0
	outer_flush_all();
#endif
	return apsys_request_power_off(arg);
#else
	return apsys_request_sleep(arg);
#endif
}

static int __init rda_pm_init(void)
{
	suspend_set_ops(&rda_pm_ops);
	pm_cpu_sleep = rda_cpu_sleep;

	return 0;
}

__initcall(rda_pm_init);
