/*
 * Copyright (C) 2013-2014 RDA Microelectronics Inc.
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
#ifndef __CLK_RDA_H__
#define __CLK_RDA_H__

#ifdef CONFIG_CLK_RDA

#include <linux/clk-provider.h>

/*
 * Clock enum list
 */
enum CLK_RDA {
	/* root clocks */
	CLK_RDA_CPU,
	CLK_RDA_BUS,
	CLK_RDA_MEM,

	/* childs of BUS */
	CLK_RDA_USB,
	CLK_RDA_AXI,
	CLK_RDA_AHB1,
	CLK_RDA_APB1,
	CLK_RDA_APB2,
	CLK_RDA_GCG,
	CLK_RDA_GPU,
	CLK_RDA_VPU,
	CLK_RDA_VOC,

	/* childs of APB2 */
	CLK_RDA_UART1,
	CLK_RDA_UART2,
	CLK_RDA_UART3,

	/* childs of AHB1 */
	CLK_RDA_SPIFLASH,

	/* childs of GCG */
	CLK_RDA_GOUDA,
	CLK_RDA_DPI,
	CLK_RDA_CAMERA,

	/* other */
	CLK_RDA_BCK,
	CLK_RDA_DSI,
	CLK_RDA_CSI,
	CLK_RDA_DEBUG,
	CLK_RDA_CLK_OUT,
	CLK_RDA_AUX_CLK,
	CLK_RDA_CLK_32K,

	CLK_RDA_END,
};

/*
 * struct clk_rda - rda clock
 */
struct clk_rda {
	struct clk_hw	hw;
	unsigned int	id;
	spinlock_t	*lock;
};

#endif /* CONFIG_CLK_RDA */
#endif /* __CLK_RDA_H__ */
