/*
 * Copyright (C) 2013 RDA Microelectronics Inc.
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
 *
 */
#include "clk_rda.h"
#include <linux/clk-provider.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/io.h>
#include <linux/err.h>
#include <linux/string.h>
#include "plat/ap_clk.h"

#define to_clk_rda(_hw) container_of(_hw, struct clk_rda, hw)

static void __clk_rda_prepare(struct clk_rda *clk_ptr, int on)
{
#ifndef CONFIG_RDA_CLK_SIMU
	unsigned long flags = 0;

	if (clk_ptr->lock)
		spin_lock_irqsave(clk_ptr->lock, flags);

	switch (clk_ptr->id) {
	case CLK_RDA_USB:
		apsys_enable_usb_clk(on);
		break;
	case CLK_RDA_GOUDA:
	case CLK_RDA_DPI:
	case CLK_RDA_DSI:
	case CLK_RDA_CAMERA:
	case CLK_RDA_GPU:
	case CLK_RDA_VPU:
	case CLK_RDA_VOC:
	case CLK_RDA_SPIFLASH:
	case CLK_RDA_UART1:
	case CLK_RDA_UART2:
	case CLK_RDA_UART3:
	case CLK_RDA_BCK:
	case CLK_RDA_CSI:
	case CLK_RDA_DEBUG:
	case CLK_RDA_CLK_OUT:
	case CLK_RDA_AUX_CLK:
	case CLK_RDA_CLK_32K:
	case CLK_RDA_BUS:
	case CLK_RDA_CPU:
	case CLK_RDA_MEM:
	case CLK_RDA_AXI:
	case CLK_RDA_AHB1:
	case CLK_RDA_APB1:
	case CLK_RDA_APB2:
	case CLK_RDA_GCG:
		break;
	default:
		pr_warn("%s Invalid clk id: %d\n",__func__, clk_ptr->id);
		break;
	}

	if (clk_ptr->lock)
		spin_unlock_irqrestore(clk_ptr->lock, flags);
#endif
}
static void __clk_rda_enable(struct clk_rda *clk_ptr, int on)
{
#ifndef CONFIG_RDA_CLK_SIMU
	unsigned long flags = 0;

	if (clk_ptr->lock)
		spin_lock_irqsave(clk_ptr->lock, flags);

	switch (clk_ptr->id) {
	case CLK_RDA_GOUDA:
		apsys_enable_gouda_clk(on);
		break;
	case CLK_RDA_DPI:
		apsys_enable_dpi_clk(on);
		break;
	case CLK_RDA_DSI:
		apsys_enable_dsi_clk(on);
		break;
	case CLK_RDA_CAMERA:
		apsys_enable_camera_clk(on);
		break;
	case CLK_RDA_GPU:
		apsys_enable_gpu_clk(on);
		break;
	case CLK_RDA_VPU:
		apsys_enable_vpu_clk(on);
		break;
	case CLK_RDA_VOC:
		apsys_enable_voc_clk(on);
		break;
	case CLK_RDA_SPIFLASH:
		apsys_enable_spiflash_clk(on);
		break;
	case CLK_RDA_UART1:
		apsys_enable_uart1_clk(on);
		break;
	case CLK_RDA_UART2:
		apsys_enable_uart2_clk(on);
		break;
	case CLK_RDA_UART3:
		apsys_enable_uart3_clk(on);
		break;
	case CLK_RDA_BCK:
		apsys_enable_bck_clk(on);
		break;
	case CLK_RDA_CSI:
		apsys_enable_csi_clk(on);
		break;
	case CLK_RDA_DEBUG:
		apsys_enable_debug_clk(on);
		break;

	case CLK_RDA_CLK_OUT:
		apsys_enable_clk_out(on);
		break;

	case CLK_RDA_AUX_CLK:
		apsys_enable_aux_clk(on);
		break;

	case CLK_RDA_CLK_32K:
		apsys_enable_clk_32k(on);
		break;

	case CLK_RDA_BUS:
		apsys_enable_bus_clk(on);
		break;

	case CLK_RDA_USB:
	case CLK_RDA_CPU:
	case CLK_RDA_MEM:
	case CLK_RDA_AXI:
	case CLK_RDA_AHB1:
	case CLK_RDA_APB1:
	case CLK_RDA_APB2:
	case CLK_RDA_GCG:
		break;
	default:
		pr_warn("%s Invalid clk id: %d\n",__func__, clk_ptr->id);
		break;
	}

	if (clk_ptr->lock)
		spin_unlock_irqrestore(clk_ptr->lock, flags);
#endif
}

static unsigned long __clk_rda_get_rate(struct clk_rda *clk_ptr)
{
	unsigned long rate = 0;
#ifndef CONFIG_RDA_CLK_SIMU
	unsigned long flags = 0;

	if (clk_ptr->lock)
		spin_lock_irqsave(clk_ptr->lock, flags);

	switch (clk_ptr->id) {
	case CLK_RDA_CPU:
		rate = apsys_get_cpu_clk_rate();
		break;
	case CLK_RDA_BUS:
		rate = apsys_get_bus_clk_rate();
		break;
	case CLK_RDA_MEM:
		rate = apsys_get_mem_clk_rate();
		break;
	case CLK_RDA_USB:
		rate = apsys_get_usb_clk_rate();
		break;
	case CLK_RDA_AXI:
		rate = apsys_get_axi_clk_rate();
		break;
	case CLK_RDA_AHB1:
		rate = apsys_get_ahb1_clk_rate();
		break;
	case CLK_RDA_APB1:
		rate = apsys_get_apb1_clk_rate();
		break;
	case CLK_RDA_APB2:
		rate = apsys_get_apb2_clk_rate();
		break;
	case CLK_RDA_GPU:
		rate = apsys_get_gpu_clk_rate();
		break;
	case CLK_RDA_VPU:
		rate = apsys_get_vpu_clk_rate();
		break;
	case CLK_RDA_VOC:
		rate = apsys_get_voc_clk_rate();
		break;
	case CLK_RDA_SPIFLASH:
		rate = apsys_get_spiflash_clk_rate();
		break;
	case CLK_RDA_UART1:
	case CLK_RDA_UART2:
	case CLK_RDA_UART3:
		rate = apsys_get_uart_clk_rate(clk_ptr->id - CLK_RDA_UART1);
		break;
	case CLK_RDA_BCK:
		rate = apsys_get_bck_clk_rate();
		break;
	case CLK_RDA_GCG:
		rate = apsys_get_gcg_clk_rate();
		break;
	case CLK_RDA_CSI:
	case CLK_RDA_DEBUG:
	case CLK_RDA_GOUDA:
	case CLK_RDA_DPI:
	case CLK_RDA_DSI:
	case CLK_RDA_CAMERA:
	case CLK_RDA_CLK_OUT:
	case CLK_RDA_AUX_CLK:
	case CLK_RDA_CLK_32K:
		break;
	default:
		pr_warn("%s Invalid clk id: %d\n",__func__, clk_ptr->id);
		break;
	}

	if (clk_ptr->lock)
		spin_unlock_irqrestore(clk_ptr->lock, flags);
#endif
	return rate;
}

static void __clk_rda_set_rate(struct clk_rda *clk_ptr,
				unsigned long rate)
{
#ifndef CONFIG_RDA_CLK_SIMU
	unsigned long flags = 0;

	if (clk_ptr->lock)
		spin_lock_irqsave(clk_ptr->lock, flags);

	switch (clk_ptr->id) {
	case CLK_RDA_CPU:
		apsys_set_cpu_clk_rate(rate);
		break;
	case CLK_RDA_BUS:
		apsys_set_bus_clk_rate(rate);
		break;
	case CLK_RDA_MEM:
		apsys_set_mem_clk_rate(rate);
		break;
	case CLK_RDA_USB:
		apsys_set_usb_clk_rate(rate);
		break;
	case CLK_RDA_AXI:
		apsys_set_axi_clk_rate(rate);
		break;
	case CLK_RDA_AHB1:
		apsys_set_ahb1_clk_rate(rate);
		break;
	case CLK_RDA_APB1:
		apsys_set_apb1_clk_rate(rate);
		break;
	case CLK_RDA_APB2:
		apsys_set_apb2_clk_rate(rate);
		break;
	case CLK_RDA_GPU:
		apsys_set_gpu_clk_rate(rate);
		break;
	case CLK_RDA_VPU:
		apsys_set_vpu_clk_rate(rate);
		break;
	case CLK_RDA_VOC:
		apsys_set_voc_clk_rate(rate);
		break;
	case CLK_RDA_SPIFLASH:
		apsys_set_spiflash_clk_rate(rate);
		break;
	case CLK_RDA_UART1:
	case CLK_RDA_UART2:
	case CLK_RDA_UART3:
		apsys_set_uart_clk_rate(clk_ptr->id - CLK_RDA_UART1,
				rate);
		break;
	case CLK_RDA_BCK:
		apsys_set_bck_clk_rate(rate);
		break;
	case CLK_RDA_GCG:
		apsys_set_gcg_clk_rate(rate);
		break;
	case CLK_RDA_DSI:
		apsys_set_dsi_clk_rate(rate);
		break;
	case CLK_RDA_CSI:
	case CLK_RDA_DEBUG:
	case CLK_RDA_GOUDA:
	case CLK_RDA_DPI:
	case CLK_RDA_CAMERA:
	case CLK_RDA_CLK_OUT:
	case CLK_RDA_AUX_CLK:
	case CLK_RDA_CLK_32K:
		break;
	default:
		pr_warn("%s Invalid clk id: %d\n",__func__, clk_ptr->id);
		break;
	}

	if (clk_ptr->lock)
		spin_unlock_irqrestore(clk_ptr->lock, flags);
#endif
}

static void __clk_rda_ops_init(struct clk_rda *clk_ptr)
{
	unsigned long flags = 0;

	if (clk_ptr->lock)
		spin_lock_irqsave(clk_ptr->lock, flags);

	switch (clk_ptr->id) {
	case CLK_RDA_GOUDA:
		__clk_rda_enable(clk_ptr,0);
		break;
	default:
		break;
	}

	if (clk_ptr->lock)
		spin_unlock_irqrestore(clk_ptr->lock, flags);
}

static void clk_rda_ops_init(struct clk_hw *hw)
{
	struct clk_rda *clk_ptr = to_clk_rda(hw);
	__clk_rda_ops_init(clk_ptr);
}

static int clk_rda_prepare(struct clk_hw *hw)
{
	struct clk_rda *clk_ptr = to_clk_rda(hw);
	__clk_rda_prepare(clk_ptr, 1);
	return 0;
}

static void clk_rda_unprepare(struct clk_hw *hw)
{
	struct clk_rda *clk_ptr = to_clk_rda(hw);
	__clk_rda_prepare(clk_ptr, 0);
}

static int clk_rda_enable(struct clk_hw *hw)
{
	struct clk_rda *clk_ptr = to_clk_rda(hw);
	__clk_rda_enable(clk_ptr, 1);
	return 0;
}

static void clk_rda_disable(struct clk_hw *hw)
{
	struct clk_rda *clk_ptr = to_clk_rda(hw);
	__clk_rda_enable(clk_ptr, 0);
}

static int clk_rda_set_rate(struct clk_hw *hw, unsigned long rate,
		unsigned long parent_rate)
{
	struct clk_rda *clk_ptr = to_clk_rda(hw);
	__clk_rda_set_rate(clk_ptr, rate);
	return 0;
}

static long clk_rda_round_rate(struct clk_hw *hw, unsigned long rate, unsigned long *unused)
{
	return (long)rate;
}

static unsigned long clk_rda_recalc_rate(struct clk_hw *hw, unsigned long parent_rate)
{
	struct clk_rda *clk_ptr = to_clk_rda(hw);
	return __clk_rda_get_rate(clk_ptr);
}

struct clk_ops root_clk_rda_ops = {
	.prepare = clk_rda_prepare,
	.unprepare = clk_rda_unprepare,
	.enable = clk_rda_enable,
	.disable = clk_rda_disable,
	.set_rate = clk_rda_set_rate,
	.round_rate = clk_rda_round_rate,
	.recalc_rate = clk_rda_recalc_rate,
};

struct clk_ops gouda_clk_rda_ops = {
	.enable = clk_rda_enable,
	.disable = clk_rda_disable,
	/* take parent rate */
	.set_rate = NULL,
	.round_rate = NULL,
	.recalc_rate = NULL,
	.init = clk_rda_ops_init,
};

struct clk_ops dpi_clk_rda_ops = {
	.enable = clk_rda_enable,
	.disable = clk_rda_disable,
	/* take parent rate */
	.set_rate = NULL,
	.round_rate = NULL,
	.recalc_rate = NULL,
};

struct clk_ops cam_clk_rda_ops = {
	.enable = clk_rda_enable,
	.disable = clk_rda_disable,
	/* take parent rate */
	.set_rate = NULL,
	.round_rate = NULL,
	.recalc_rate = NULL,
};

struct clk_ops modem_clk_rda_ops = {
	.prepare = clk_rda_enable,
	.unprepare = clk_rda_disable,
	.enable	= NULL,
	.disable = NULL,
	.set_rate = clk_rda_set_rate,
	.round_rate = clk_rda_round_rate,
	.recalc_rate = clk_rda_recalc_rate,
};
