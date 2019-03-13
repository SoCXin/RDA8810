/* linux/arch/arm/plat-rda/gpio.c
 *
 * Copyright (c) 2012-, RDA micro inc. All rights reserved.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <linux/bitops.h>
#include <linux/gpio.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/irq.h>
#include <linux/module.h>
#include <linux/irqchip/chained_irq.h>
#include <asm/mach/irq.h>
#include "gpio_hw.h"

#define RDA_GPIO_BANK(bank, io_type, first, last)			\
	{								\
		.regs = {						\
			.oen_val =  RDA_GPIO_OEN_VAL_##bank,		\
			.oen_set_out = RDA_GPIO_OEN_SET_OUT_##bank,	\
			.oen_set_in =  RDA_GPIO_SET_IN_##bank,	\
			.val =   RDA_GPIO_VAL_##bank,	\
			.set =      RDA_GPIO_SET_##bank,		\
			.clr =    RDA_GPIO_CLR_##bank,	\
			.int_ctrl_set =     RDA_GPIO_INT_CTRL_SET_##bank,	\
			.int_ctrl_clr =     RDA_GPIO_INT_CTRL_CLR_##bank,	\
			.int_clr  =	RDA_GPIO_INT_CLR_##bank,		\
			.int_status = RDA_GPIO_INT_STATUS_##bank,		\
		},							\
		.chip = {						\
			.base = (first),				\
			.ngpio = (last) - (first) + 1,			\
			.get = rda_gpio_get,				\
			.set = rda_gpio_set,				\
			.direction_input = rda_gpio_direction_input,	\
			.direction_output = rda_gpio_direction_output,	\
			.set_debounce = rda_set_debounce,		\
			.to_irq = rda_gpio_to_irq,			\
			.request = rda_gpio_request,			\
			.free = rda_gpio_free,				\
		},							\
		.type = io_type,					\
	}

struct rda_gpio_regs {
	void __iomem *oen_val;
	void __iomem *oen_set_out;
	void __iomem *oen_set_in;
	void __iomem *val;
	void __iomem *set;
	void __iomem *clr;
	void __iomem *int_ctrl_set;
	void __iomem *int_ctrl_clr;
	void __iomem *int_clr;
	void __iomem *int_status;
	void __iomem *chg_ctrl;
	void __iomem *chg_cmd;
};

enum {
	GPIO,
	GPO
};

struct rda_gpio_chip {
	spinlock_t		lock;
	struct gpio_chip	chip;
	struct rda_gpio_regs	regs;
	int type;
};

static int  __set_gpio_irq_type(struct rda_gpio_chip *rda_chip,
		unsigned offset, unsigned int flow_type);

static int rda_gpio_direction_input(struct gpio_chip *chip, unsigned offset)
{
	struct rda_gpio_chip *rda_chip;

	rda_chip = container_of(chip, struct rda_gpio_chip, chip);
	if (rda_chip->type == GPO) {
		pr_warning("can not set gpo with input");
		return -EINVAL;
	}

//	pr_debug("set gpio %d input \n", chip->base + offset);
	writel(BIT(offset), rda_chip->regs.oen_set_in);
	return 0;
}

static int
rda_gpio_direction_output(struct gpio_chip *chip, unsigned offset, int value)
{
	struct rda_gpio_chip *rda_chip;

	rda_chip = container_of(chip, struct rda_gpio_chip, chip);
	if (rda_chip->type == GPIO) {
		pr_debug("set gpio %d output value:%d\n",
				chip->base + offset,  value);
		writel(BIT(offset), rda_chip->regs.oen_set_out);
	}

	if (value) 
		writel(BIT(offset), rda_chip->regs.set);
	else 
		writel(BIT(offset), rda_chip->regs.clr);

	return 0;
}

static int rda_gpio_get(struct gpio_chip *chip, unsigned offset)
{
	struct rda_gpio_chip *rda_chip;
	int val;

	pr_debug("get gpio %d value:\n", chip->base + offset);
	rda_chip = container_of(chip, struct rda_gpio_chip, chip);
	if (rda_chip->type == GPO) { //gpo 
		return (readl(rda_chip->regs.set) & (BIT(offset))) ? 1 : 0;
	}

	if (readl(rda_chip->regs.oen_val) & BIT(offset))   //gpio input
		val = (readl(rda_chip->regs.val) & BIT(offset)) ? 1 : 0;
	
	else 	//gpio output
		val = (readl(rda_chip->regs.set) & BIT(offset)) ? 1 : 0;

	return val;
}

static void rda_gpio_set(struct gpio_chip *chip, unsigned offset, int value)
{
	struct rda_gpio_chip *rda_chip;

	rda_chip = container_of(chip, struct rda_gpio_chip, chip);
	//pr_debug("set gpio value:%d\n", value);
	//pr_debug("set gpio regs:%p to %lx\n",
	//			rda_chip->regs.set, BIT(offset));
	if (value) 
		writel(BIT(offset), rda_chip->regs.set);
	else 
		writel(BIT(offset), rda_chip->regs.clr);
}

/*
  GPIO as interrupt, for every gpio bank, only the gpio refer to register
  GPIO_OEN_VAL bits 0~7 can be interrupt input pins.
   ____________________________________________________________________________
  |      31~24       |      23~16       |     8~15      |     0~7              |
  |__________________|__________________|_______________|______________________|
  |                            normal gpios             |   input interrupt    |
  |_____________________________________________________|______________________|

 */

static int __get_gpio_bank(int base)
{
	/*
	  * bank = base / 32;
	  */
	return (base >> 5);
}
static int rda_gpio_to_irq(struct gpio_chip *chip, unsigned offset)
{
	int bank = __get_gpio_bank(chip->base);
	int irq;

	if (unlikely(BIT(offset) & GPIO_IRQ_MASK) == 0) {
		pr_warning("the gpio %d can not be used as irq\n", chip->base + offset);
		return -EINVAL;
	}
	irq = bank * NR_GPIO_BANK_IRQS + offset;
	irq += RDA_GPIO_IRQ_START;
	return irq;
}

#define RISING_SHIFT	(0)
#define FALLING_SHIFT	(8)
#define DEBOUCE_SHIFT	(16)
#define LEVEL_MODE_SHIFT	(24)

static int rda_set_debounce(struct gpio_chip *chip,unsigned offset,
		unsigned debounce)
{
	struct rda_gpio_chip *rda_chip;

	if ((BIT(offset) & GPIO_IRQ_MASK) == 0) {
		return -EINVAL;
	}

	rda_chip = container_of(chip, struct rda_gpio_chip, chip);
	if (debounce > 0)
		writel(BIT(offset) << DEBOUCE_SHIFT, rda_chip->regs.int_ctrl_set);
	else
		writel(BIT(offset) << DEBOUCE_SHIFT, rda_chip->regs.int_ctrl_clr);
	return 0;
}

static int rda_gpio_request(struct gpio_chip *chip, unsigned offset)
{
	//return rda_gpiomux_get(chip->base + offset);
	return 0;
}

static void rda_gpio_free(struct gpio_chip *chip, unsigned offset)
{
	//rda_gpiomux_put(chip->base + offset);
}

struct rda_gpio_chip rda_gpio_chips[] = {
	RDA_GPIO_BANK(A, GPIO,   GPIO_A0,  GPIO_A31),
	RDA_GPIO_BANK(B, GPIO,   GPIO_B0,  GPIO_B31),
	RDA_GPIO_BANK(D, GPIO,   GPIO_D0,  GPIO_D31),
#if defined(CONFIG_ARCH_RDA8850E) || defined(CONFIG_ARCH_RDA8810H)
	RDA_GPIO_BANK(E, GPIO,   GPIO_E0,  GPIO_E31),
#endif
	RDA_GPIO_BANK(C, GPIO,   GPIO_C0,  GPIO_C31),
	RDA_GPIO_BANK(GPO1, GPO, GPO_0,  GPO_4),
};

static void rda_gpio_irq_ack(struct irq_data *d)
{
	struct rda_gpio_chip *rda_chip = irq_data_get_irq_chip_data(d);
	unsigned offset = d->irq - gpio_to_irq(rda_chip->chip.base);

	pr_debug("%s, gpio %d\n",__func__, rda_chip->chip.base + offset);
	writel(BIT(offset), rda_chip->regs.int_clr);
}

static void rda_gpio_irq_mask(struct irq_data *d)
{
	struct rda_gpio_chip *rda_chip = irq_data_get_irq_chip_data(d);
	unsigned offset = d->irq - gpio_to_irq(rda_chip->chip.base);
	unsigned value;

	pr_debug("%s, gpio %d\n", __func__, rda_chip->chip.base + offset);
	value = BIT(offset) << RISING_SHIFT;
	value |= BIT(offset) << FALLING_SHIFT;
	writel(value, rda_chip->regs.int_ctrl_clr);
}

static void rda_gpio_irq_unmask(struct irq_data *d)
{
	struct rda_gpio_chip *rda_chip = irq_data_get_irq_chip_data(d);
	unsigned offset = d->irq - gpio_to_irq(rda_chip->chip.base);
	u32 trigger = irqd_get_trigger_type(d);

	pr_debug("%s, gpio %d\n", __func__, rda_chip->chip.base + offset);
	__set_gpio_irq_type(rda_chip, offset, trigger);
}

static int rda_gpio_irq_set_wake(struct irq_data *d, unsigned int on)
{
	return 0;
}

static int  __set_gpio_irq_type(struct rda_gpio_chip *rda_chip,
		unsigned offset, unsigned int flow_type)
{
	unsigned long irq_flags;
	u32 value;
	int ret = 0;

	spin_lock_irqsave(&rda_chip->lock, irq_flags);
	switch (flow_type & IRQ_TYPE_SENSE_MASK) {
	case IRQ_TYPE_EDGE_RISING:
		value = BIT(offset) << RISING_SHIFT;
		writel(value, rda_chip->regs.int_ctrl_set);
		value = BIT(offset) << LEVEL_MODE_SHIFT;
		writel(value, rda_chip->regs.int_ctrl_clr);
		break;

	case IRQ_TYPE_EDGE_FALLING:
		value = BIT(offset) << FALLING_SHIFT;
		writel(value, rda_chip->regs.int_ctrl_set);
		value = BIT(offset) << LEVEL_MODE_SHIFT;
		writel(value, rda_chip->regs.int_ctrl_clr);
		break;

	case IRQ_TYPE_EDGE_BOTH:
		value = BIT(offset) << RISING_SHIFT;
		value |= BIT(offset) << FALLING_SHIFT;
		writel(value, rda_chip->regs.int_ctrl_set);

		value = BIT(offset) << LEVEL_MODE_SHIFT;
		writel(value, rda_chip->regs.int_ctrl_clr);
		break;

	case IRQ_TYPE_LEVEL_HIGH:
		value = BIT(offset) << RISING_SHIFT;
		value |= BIT(offset) << LEVEL_MODE_SHIFT;
		writel(value, rda_chip->regs.int_ctrl_set);
		break;

	case IRQ_TYPE_LEVEL_LOW:
		value = BIT(offset) << FALLING_SHIFT;
		value |= BIT(offset) << LEVEL_MODE_SHIFT;
		writel(value, rda_chip->regs.int_ctrl_set);
		break;

	default:
		ret =  -EINVAL;
		break;
	}
	spin_unlock_irqrestore(&rda_chip->lock, irq_flags);
	return ret;
}

static int rda_gpio_irq_set_type(struct irq_data *d, unsigned int flow_type)
{
	struct rda_gpio_chip *rda_chip = irq_data_get_irq_chip_data(d);
	unsigned offset = d->irq - gpio_to_irq(rda_chip->chip.base);
	int ret;

	if ((BIT(offset) & GPIO_IRQ_MASK) == 0) {
		pr_warning("only bits %x in the gpio bank can be interrupt line\n",
				GPIO_IRQ_MASK);
		return -EINVAL;
	}
	pr_debug("%s, gpio %d\n",__func__, rda_chip->chip.base + offset);

	ret = __set_gpio_irq_type(rda_chip, offset, flow_type);
	if (ret)
		return ret;

	if (flow_type & (IRQ_TYPE_LEVEL_LOW | IRQ_TYPE_LEVEL_HIGH))
		__irq_set_handler_locked(d->irq, handle_level_irq);
	else if (flow_type & (IRQ_TYPE_EDGE_FALLING | IRQ_TYPE_EDGE_RISING))
		__irq_set_handler_locked(d->irq, handle_edge_irq);
	return 0;
}

#define FIRST_GPIO_IRQ RDA_GPIO_IRQ_START

static void rda_gpio_irq_handler(unsigned int irq, struct irq_desc *desc)
{
	int i, j;
	unsigned val;
	struct irq_chip *chip = irq_desc_get_chip(desc);
	int gpio_irq;
	int banks;

	chained_irq_enter(chip, desc);

	/*
	 * only gpio banks have interrupt, gpo ang gpio_c doesn't suport irq;
	 */
	banks = ARRAY_SIZE(rda_gpio_chips) - 2;
	for (i = 0; i < banks; i++) {
		struct rda_gpio_chip *rda_chip = &rda_gpio_chips[i];

		val = readl(rda_chip->regs.int_status);
		val &= GPIO_IRQ_MASK;//only 8 bit;
		while (val) {
			j = __ffs(val);
			val &= ~(1 << j);
			/* printk("%s %08x %08x bit %d gpio %d irq %d\n",
				__func__, v, m, j, rda_chip->chip.start + j,
				FIRST_GPIO_IRQ + rda_chip->chip.start + j); */
			gpio_irq = rda_gpio_to_irq(&rda_chip->chip, j);
			generic_handle_irq(gpio_irq);
		}
	}
	chained_irq_exit(chip, desc);
}

static struct irq_chip rda_gpio_irq_chip = {
	.name          = "rdagpio",
	.irq_ack       = rda_gpio_irq_ack,
	.irq_mask      = rda_gpio_irq_mask,
	.irq_unmask    = rda_gpio_irq_unmask,
	.irq_set_wake  = rda_gpio_irq_set_wake,
	.irq_set_type  = rda_gpio_irq_set_type,
	.irq_disable = rda_gpio_irq_mask,
};

static int __init rda_init_gpio(void)
{
	int i, j = 0;
	unsigned offset;

	for (i = FIRST_GPIO_IRQ; i < FIRST_GPIO_IRQ + NR_GPIO_IRQS; i++) {
		j = (i - FIRST_GPIO_IRQ) / NR_GPIO_BANK_IRQS;
		pr_debug("map irq %d with bank %d\n", i, j);
		irq_set_chip_data(i, &rda_gpio_chips[j]);
		irq_set_chip_and_handler(i, &rda_gpio_irq_chip,
					 handle_level_irq);
		set_irq_flags(i, IRQF_VALID);
		offset = i % NR_GPIO_BANK_IRQS;
		writel(BIT(offset), rda_gpio_chips[j].regs.int_clr);
	}

	for (i = 0; i < ARRAY_SIZE(rda_gpio_chips); i++) {
		spin_lock_init(&rda_gpio_chips[i].lock);
		//writel(0, rda_gpio_chips[i].regs.int_en);
		gpiochip_add(&rda_gpio_chips[i].chip);
	}

	irq_set_chained_handler(RDA_IRQ_GPIO1, rda_gpio_irq_handler);
	irq_set_chained_handler(RDA_IRQ_GPIO2, rda_gpio_irq_handler);
	irq_set_chained_handler(RDA_IRQ_GPIO3, rda_gpio_irq_handler);
#if defined(CONFIG_ARCH_RDA8850E) || defined(CONFIG_ARCH_RDA8810H)
	irq_set_chained_handler(RDA_IRQ_GPIOE4, rda_gpio_irq_handler);
#endif
	return 0;
}

postcore_initcall(rda_init_gpio);

