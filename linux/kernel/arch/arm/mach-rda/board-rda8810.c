#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>

#include <linux/regulator/machine.h>
#include <asm/hardware/cache-l2x0.h>
#include <asm/io.h>
#include <asm/mach-types.h>
#include <asm/mach/arch.h>
#include <asm/mach/time.h>
#include <mach/hardware.h>
#include <mach/irqs.h>
#include <mach/ifc.h>
#include <plat/rda_debug.h>
#include <plat/reg_ifc.h>



extern void rda_init_irq(void);
extern void rda_init_devices(void);
extern void rda8810_map_io(void);
extern void regulator_add_devices(void);
extern int rda_apsys_init(void);
extern void clk_rda_init(void);
extern void rda_init_shdw(void);

static void __iomem * gpu_base;
int rda_gpu_idle_status(void)
{
	void __iomem * gpu_idle_reg;
	int val;
#define GPU_IDLE_REG_MASK	(0xfff)

	gpu_idle_reg = gpu_base + 0x4;
	val = readl(gpu_idle_reg);
	if ((val & GPU_IDLE_REG_MASK) == GPU_IDLE_REG_MASK)
		return 1;
	else
		return 0;
}

static int rda_vivante_gpu_reset(void)
{
	void __iomem *gpu_rst_reg = gpu_base + 0x1028;

	writel(0x1, gpu_rst_reg);
	return 0;
}

static void __init rda8810_init(void)
{
	regulator_add_devices();
	rda_debug_init_sysfs();
	rda_init_shdw();
	rda_apsys_init();
#ifdef CONFIG_CLK_RDA
	clk_rda_init();
#endif /* CONFIG_CLK_RDA */
	rda_init_devices();
	gpu_base = ioremap(RDA_GPU_PHYS, SZ_8K);
	if(!gpu_base) {
		pr_err("cannot remap for gpu\n");
		return;
	}

	rda_vivante_gpu_reset();
}
extern void init_dma_coherent_pool_size(unsigned long);
static int __init rda8810_disable_coherent_pool(void)
{
	init_dma_coherent_pool_size(0);
	return 0;
}
core_initcall(rda8810_disable_coherent_pool);

static void __init rda8810_init_irq(void)
{
	rda_init_irq();
}

extern struct sys_timer rda_timer;

static void __init rda8810_init_l2cache(void)
{
#ifdef CONFIG_CACHE_L2X0
	void __iomem *p = (void *)(RDA_L2CC_BASE);
	u32 aux_ctrl = 0x30030000;
	u32 aux_mask = 0xFDFFFFFF;
	/*
	* Config Aux Register
	* bit29 : Instruction prefetch enable
	* bit28 : Data prefetch enable
	* bit25 : replacement policy as Pseudo-random
	* bit19-17 : 0b001 : way size is 16KB.
	* bit16 : Associativity is 16-way.
	*/
	l2x0_init(p, aux_ctrl, aux_mask);
#endif
}

static void __init rda8810_init_early(void)
{
	rda8810_init_l2cache();
}

extern void rda_timer_init(void);

MACHINE_START(RDA8810, "rda8810")
	.atag_offset	= 0x00000100,
	.map_io		= rda8810_map_io,
	.nr_irqs	= NR_IRQS,
	.init_irq	= rda8810_init_irq,
	.init_machine	= rda8810_init,
	.init_time	= rda_timer_init,
	.init_early	= rda8810_init_early,
	//.reserve	= rda8810_reserve,
	//.restart	= rda8810_restart,
MACHINE_END
