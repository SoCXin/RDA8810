#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/irqchip/arm-gic.h>
#include <linux/irqchip/chained_irq.h>

#include <linux/regulator/machine.h>
#include <asm/io.h>
#include <asm/mach-types.h>
#include <asm/mach/arch.h>
#include <asm/mach/time.h>
#include <mach/hardware.h>
#include <mach/irqs.h>
#include <mach/ifc.h>
#include <plat/rda_debug.h>
#include <plat/reg_ifc.h>
#include <plat/intc.h>
#include <plat/ispi.h>



extern void rda_init_devices(void);
extern void rda8850e_map_io(void);
extern void regulator_add_devices(void);
extern int rda_apsys_init(void);
extern void clk_rda_init(void);
extern void rda_init_shdw(void);

extern void init_dma_coherent_pool_size(unsigned long);
static int __init rda8850e_disable_coherent_pool(void)
{
	init_dma_coherent_pool_size(0);
	return 0;
}
core_initcall(rda8850e_disable_coherent_pool);

static void __init rda8850e_init(void)
{
	regulator_add_devices();
	rda_debug_init_sysfs();
	rda_init_shdw();
	rda_apsys_init();
	ispi_open();
#ifdef CONFIG_CLK_RDA
	clk_rda_init();
#endif /* CONFIG_CLK_RDA */
	rda_init_devices();
}

static void __init rda8850e_init_irq(void)
{
	void __iomem *cpu_base;
	void __iomem *dist_base;

	dist_base = (void __iomem *)(RDA_GIC_BASE + 0x1000);
	cpu_base = (void __iomem *)(RDA_GIC_BASE + 0x2000);

	gic_init_bases(0, RDA_IRQ_BASE, dist_base, cpu_base, 0, NULL);
	rda_gic_init();
}

extern struct sys_timer rda_timer;

static void __init rda8850e_init_early(void)
{
}

extern void __init rda_timer_init(void);
extern int __init arch_timer_init(void);

static void __init rda8850e_timer_init(void)
{
	//arch_timer_init();
	rda_timer_init();
}

extern struct smp_operations rda_smp_ops;

MACHINE_START(RDA8850E, "rda8850e")
	.atag_offset	= 0x00000100,
	.map_io		= rda8850e_map_io,
	.nr_irqs	= NR_IRQS,
	.init_irq	= rda8850e_init_irq,
	.init_machine	= rda8850e_init,
	.init_time	= rda8850e_timer_init,
	.init_early	= rda8850e_init_early,
	//.reserve	= rda8810e_reserve,
	//.restart	= rda8810e_restart,
MACHINE_END
