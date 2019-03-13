/*
 * Copyright (C) 2014 RDA Microelectronics Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/err.h>
#include <linux/spinlock.h>
#include <asm/io.h>

#include <plat/reg_pd_ctrl.h>
#include <plat/pd_ctrl.h>

static __iomem void *rda_pdc_base = NULL;
static spinlock_t pdc_lock;

static  u32 power_count = 1;

int rda_pdc_gpu_power_on()
{
	u32 val = 0;

	/* Power up GPU. */
	__raw_writel(0, rda_pdc_base + PDC_GPU_SWITCH_REG);
	/* Check the status of gpu switch until it is cleared. */
	do {
		val = __raw_readl(rda_pdc_base + PDC_SWITCH_STA_REG);
	} while (val & PDC_GPU_ACK0);
	/* Enable clock of GPU. */
	__raw_writel(PDC_GPU_CLK_ENABLE, rda_pdc_base + PDC_GPU_CLK_REG);

	spin_lock(&pdc_lock);
	/* Disable the isolate of GPU. */
	val = __raw_readl(rda_pdc_base + PDC_ISOLATE_REG);
	__raw_writel((val & (~PDC_ISOLATE_GPU)), rda_pdc_base + PDC_ISOLATE_REG);
	spin_unlock(&pdc_lock);

	/* Set active flag of GPU. */
	__raw_writel(PDC_GPU_RESET, rda_pdc_base + PDC_GPU_RESET_REG);

	return 0;
}
EXPORT_SYMBOL_GPL(rda_pdc_gpu_power_on);

int rda_pdc_gpu_power_off()
{
	u32 val = 0;

	/* Disable clock of GPU. */
	__raw_writel(0, rda_pdc_base + PDC_GPU_CLK_REG);

	spin_lock(&pdc_lock);
	/* Enable the isolate of GPU. */
	val = __raw_readl(rda_pdc_base + PDC_ISOLATE_REG);
	__raw_writel((val | PDC_ISOLATE_GPU), rda_pdc_base + PDC_ISOLATE_REG);
	spin_unlock(&pdc_lock);

	/* Power off GPU. */
	__raw_writel(PDC_GPU_SLEEP0, rda_pdc_base + PDC_GPU_SWITCH_REG);
	/* Check if the status of gpu switch until it is set. */
	do {
		val = __raw_readl(rda_pdc_base + PDC_SWITCH_STA_REG);
	} while (!(val & PDC_GPU_ACK0));
	/* Clear active flag of GPU. */
	__raw_writel(0, rda_pdc_base + PDC_GPU_RESET_REG);

	return 0;
}
EXPORT_SYMBOL_GPL(rda_pdc_gpu_power_off);

int rda_pdc_vpu_power_on()
{
	u32 val = 0;

	if (power_count == 0) {
		/* Power up VPU. */
		__raw_writel(0, rda_pdc_base + PDC_VPU_SWITCH_REG);
		/* Check the status of vpu switch until it is cleared. */
		do {
			val = __raw_readl(rda_pdc_base + PDC_SWITCH_STA_REG);
		} while (val & PDC_VPU_ACK0);
		/* Enable clock of VPU. */
		__raw_writel(PDC_VPU_CLK_ENABLE, rda_pdc_base + PDC_VPU_CLK_REG);

		spin_lock(&pdc_lock);
		/* Disable the isolate of VPU. */
		val = __raw_readl(rda_pdc_base + PDC_ISOLATE_REG);
		__raw_writel((val & (~PDC_ISOLATE_VPU)), rda_pdc_base + PDC_ISOLATE_REG);
		spin_unlock(&pdc_lock);

		/* Set active flag of VPU. */
		__raw_writel(PDC_VPU_RESET, rda_pdc_base + PDC_VPU_RESET_REG);
	}

	power_count++;

	return 0;
}
EXPORT_SYMBOL_GPL(rda_pdc_vpu_power_on);

int rda_pdc_vpu_power_off()
{
	u32 val = 0;

	if (power_count <= 0)
		return -1;
	if (power_count == 1) {
		/* Disable clock of VPU. */
		__raw_writel(0, rda_pdc_base + PDC_VPU_CLK_REG);

		spin_lock(&pdc_lock);
		/* Enable the isolate of VPU. */
		val = __raw_readl(rda_pdc_base + PDC_ISOLATE_REG);
		__raw_writel((val | PDC_ISOLATE_VPU), rda_pdc_base + PDC_ISOLATE_REG);
		spin_unlock(&pdc_lock);

		/* Power off VPU. */
		__raw_writel(PDC_VPU_SLEEP0, rda_pdc_base + PDC_VPU_SWITCH_REG);
		/* Check if the status of vpu switch until it is set. */
		do {
			val = __raw_readl(rda_pdc_base + PDC_SWITCH_STA_REG);
		} while (!(val & PDC_VPU_ACK0));
		/* Clear active flag of VPU. */
		__raw_writel(0, rda_pdc_base + PDC_VPU_RESET_REG);
	}

	power_count--;

	return 0;
}
EXPORT_SYMBOL_GPL(rda_pdc_vpu_power_off);

static int __init rda_pd_ctrl_init(void)
{
	int ret = 0;

	rda_pdc_base = ioremap(RDA_PD_CTRL_PHYS, RDA_PD_CTRL_SIZE);
	if (rda_pdc_base == NULL) {
		pr_err("Failed to ioremap  power domain register\n");
		return -ENOMEM;
	}
	spin_lock_init(&pdc_lock);

	pr_info("rda power domain controller is initialized!\n");
	return ret;
}
module_init(rda_pd_ctrl_init);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("RDA Special Power Domain Controler Driver");
MODULE_AUTHOR("Tao Lei<leitao@rdamicro.com>");

