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

extern void rda_init_devices(void);
extern void rda8820_map_io(void);
extern void regulator_add_devices(void);
extern int rda_apsys_init(void);
extern void clk_rda_init(void);
extern void rda_init_shdw(void);

static void __init rda8820_init(void)
{
	regulator_add_devices();
	rda_debug_init_sysfs();
	rda_init_shdw();
	rda_apsys_init();
#ifdef CONFIG_CLK_RDA
	clk_rda_init();
#endif
	rda_init_devices();
}

static void __init rda8820_init_irq(void)
{
	void __iomem *cpu_base;
	void __iomem *dist_base;

	dist_base = (void __iomem *)(RDA_GIC_BASE + 0x1000);
	cpu_base = (void __iomem *)(RDA_GIC_BASE + 0x2000);

	gic_init_bases(0, RDA_IRQ_BASE, dist_base, cpu_base, 0, NULL);
}

extern struct sys_timer rda_timer;

static void __init rda8820_init_early(void)
{
}

extern void rda_timer_init(void);
extern struct smp_operations rda_smp_ops;

MACHINE_START(RDA8820, "rda8820")
	.atag_offset	= 0x00000100,
	.smp		= smp_ops(rda_smp_ops),
	.map_io		= rda8820_map_io,
	.nr_irqs	= NR_IRQS,
	.init_irq	= rda8820_init_irq,
	.init_machine	= rda8820_init,
	.init_time	= rda_timer_init,
	.init_early	= rda8820_init_early,
	//.reserve	= rda8820_reserve,
	//.restart	= rda8820_restart,
MACHINE_END
