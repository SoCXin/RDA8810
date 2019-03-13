#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/irq.h>

#include <asm/io.h>
#include <mach/hardware.h>
#include <mach/irqs.h>
#include <plat/reg_intc.h>
#include <plat/rda_debug.h>

#ifdef CONFIG_ARCH_RDA8810
/*
 * only rda8810 use rda irq contrller as irq_chip
 * other chips use gic, rda irq controller is only used for wakeup
 */
static void rda_intc_mask_irq(struct irq_data *d)
{
	void __iomem *int_base = (void __iomem *)IO_ADDRESS(RDA_INTC_BASE);

	writel((1 << d->irq), int_base + RDA_INTC_MASK_CLR);
	rda_dbg_mach("mask irq %d, mask = 0x%08x\n",
		d->irq, readl(int_base + RDA_INTC_MASK_CLR));
}

static void rda_intc_unmask_irq(struct irq_data *d)
{
	void __iomem *int_base = (void __iomem *)IO_ADDRESS(RDA_INTC_BASE);

	writel((1 << d->irq), int_base + RDA_INTC_MASK_SET);
	rda_dbg_mach("unmask irq %d, mask = 0x%08x\n",
		d->irq, readl(int_base + RDA_INTC_MASK_SET));
}

static int rda_intc_set_type(struct irq_data *data, unsigned int flow_type)
{
        if (flow_type & (IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING)) {
                irq_set_handler(data->irq, handle_edge_irq);
        }
        if (flow_type & (IRQF_TRIGGER_HIGH | IRQF_TRIGGER_LOW)) {
                irq_set_handler(data->irq, handle_level_irq);
        }
        return 0;
}

static void rda_intc_init_irq(void)
{
	void __iomem *int_base = (void __iomem *)IO_ADDRESS(RDA_INTC_BASE);

	/*
	 * Mask, and invalid all interrupt sources
	 */
	writel(RDA_IRQ_MASK_ALL, int_base + RDA_INTC_MASK_CLR);
}

static struct irq_chip rda_irq_chip = {
	.name		= "rda_intc",
	.irq_ack 	= rda_intc_mask_irq,
	.irq_mask	= rda_intc_mask_irq,
	.irq_unmask 	= rda_intc_unmask_irq,
	.irq_set_type	= rda_intc_set_type,
	.irq_disable	= rda_intc_mask_irq,
};

void rda_init_irq(void)
{
	unsigned int i;

	rda_dbg_mach("rda_init_irq: intc\n");

	rda_intc_init_irq();

	for (i = 0; i < NR_IRQS; i++) {
		irq_set_chip_and_handler(i, &rda_irq_chip, handle_level_irq);
		set_irq_flags(i, IRQF_VALID | IRQF_PROBE);
	}
}
#endif /* CONFIG_ARCH_RDA8810 */


#ifdef CONFIG_ARM_GIC

#include <linux/irqchip/arm-gic.h>


/* Rda intc is used to wakeup modem during AP suspend/sleep
 * Here is the trick to track what irq to wakeup modem
 */
static u32 intc_unmask;
static u32 intc_wakeup_mask;

/*no need to lock here, gic hold spinlock before call these functions*/

static void rda_gic_irq_unmask(struct irq_data *d)
{
	unsigned int idx = d->irq - 31 ;

	if (d->irq < 32)
		return;

	if (d->irq >= 64)
		return;

	intc_unmask |= (1 << idx);
}

static void rda_gic_irq_mask(struct irq_data *d)
{
	unsigned int idx = d->irq - 31 ;

        if (d->irq < 32)
                return;

	if (d->irq >= 64)
		return;

	intc_unmask &= ~(1 << idx);
}

static int rda_gic_irq_set_wake(struct irq_data *d, unsigned int on)
{
	unsigned int idx = d->irq - 31 ;

	/* Sanity check for SPI irq */
	if (d->irq < 32)
		return -EINVAL;

	if (d->irq >= 64)
		return -EINVAL;

	if (on)
		intc_wakeup_mask |= 1 << idx;
	else
		intc_wakeup_mask &= ~(1 << idx);

	return 0;
}

void rda_gic_init(void)
{
	gic_arch_extn.irq_mask = rda_gic_irq_mask;
	gic_arch_extn.irq_unmask = rda_gic_irq_unmask;
	gic_arch_extn.irq_set_wake = rda_gic_irq_set_wake;
}

u32 rda_gic_get_wakeup_mask(void)
{
#if 0
	this should work
	return intc_wakeup_mask | intc_unmask;
#else
	int i;
	struct irq_desc *desc;
	uint32_t temp = intc_unmask;

	/* due to lazy interrupt disable, maybe irqs are not masked
	 * the right way to do this is to make sure no irq generated
	 * when irq_disable is called by driver
	 */
	for (i = 0; i < 31; i++) {
		desc = irq_to_desc(RDA_IRQ_SPI_BASE + i);
		if (irqd_irq_disabled(&(desc->irq_data))) {
			temp &= ~(1<<(i+1));
		}
	}

	return temp | intc_unmask;
#endif
}

void rda_intc_unmask_irqs(int irq_mask)
{
	void __iomem *int_base = (void __iomem *)IO_ADDRESS(RDA_INTC_BASE);

	writel(irq_mask, int_base + RDA_INTC_MASK_SET);
}

#endif


#ifdef CONFIG_SUSPEND
/*
 * For CPU suspend/resume
 */
uint32_t rda_intc_get_mask(void)
{
	void __iomem *int_base = (void __iomem *)IO_ADDRESS(RDA_INTC_BASE);

	return readl(int_base + RDA_INTC_MASK_SET);
}

int rda_intc_set_wakeup_mask(uint32_t wakeup_mask)
{
	void __iomem *int_base = (void __iomem *)IO_ADDRESS(RDA_INTC_BASE);

	writel(wakeup_mask, int_base + RDA_INTC_WAKEUP_MASK);

	return 0;
}

int rda_intc_set_cpu_sleep(void)
{
	void __iomem *int_base = (void __iomem *)IO_ADDRESS(RDA_INTC_BASE);

	writel(1, int_base + RDA_INTC_CPU_SLEEP);

	return 0;
}

int rda_intc_get_wakeup_cause(uint32_t *wakeup_cause)
{
	void __iomem *int_base = (void __iomem *)IO_ADDRESS(RDA_INTC_BASE);

	*wakeup_cause = readl(int_base + RDA_INTC_FINALSTATUS);

	return 0;
}
#endif /* CONFIG_SUSPEND */
