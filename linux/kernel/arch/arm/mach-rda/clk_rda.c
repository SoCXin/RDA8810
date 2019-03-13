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
  */
#include "clk_rda.h"
#include <linux/clk-private.h>
#include <linux/clkdev.h>

extern struct clk_ops root_clk_rda_ops;
extern struct clk_ops cam_clk_rda_ops;
extern struct clk_ops gouda_clk_rda_ops;
extern struct clk_ops dpi_clk_rda_ops;
extern struct clk_ops modem_clk_rda_ops;

#define DEFINE_ROOT_CLK_RDA(_name, _flags, _id, _lock)		\
	static struct clk _name;				\
	static struct clk_rda _name##_hw = {			\
		.hw = {						\
			.clk = &_name,				\
		},						\
		.id = _id,					\
		.lock = _lock,					\
	};							\
	static struct clk _name = {				\
		.name = #_name,					\
		.ops = &root_clk_rda_ops,			\
		.hw = &_name##_hw.hw,				\
		.num_parents = 0,				\
		.flags = _flags,				\
	};

#define DEFINE_CHILD_CLK_RDA(_name, _parent_name,		\
	       _parent_ptr, _flags, _id, _lock)			\
	static struct clk _name;				\
	static const char *_name##_parent_names[] = {		\
		_parent_name,					\
	};							\
	static struct clk *_name##_parents[] = {		\
		_parent_ptr,					\
	};							\
	static struct clk_rda _name##_hw = {			\
		.hw = {						\
			.clk = &_name,				\
		},						\
		.id = _id,					\
		.lock = _lock,					\
	};							\
	static struct clk _name = {				\
		.name = #_name,					\
		.ops = &root_clk_rda_ops,			\
		.hw = &_name##_hw.hw,				\
		.parent_names = _name##_parent_names,		\
		.num_parents =					\
			ARRAY_SIZE(_name##_parent_names),	\
		.parents = _name##_parents,			\
		.flags = _flags,				\
	};

/* Static list of system clocks */

/* root clocks */
DEFINE_ROOT_CLK_RDA(cpu, CLK_IS_ROOT, CLK_RDA_CPU, NULL);
DEFINE_ROOT_CLK_RDA(bus, CLK_IS_ROOT, CLK_RDA_BUS, NULL);
DEFINE_ROOT_CLK_RDA(mem, CLK_IS_ROOT, CLK_RDA_MEM, NULL);

/* childs of BUS */
DEFINE_CHILD_CLK_RDA(usb, "bus", &bus, 0, CLK_RDA_USB, NULL);
DEFINE_CHILD_CLK_RDA(axi, "bus", &bus, 0, CLK_RDA_AXI, NULL);
DEFINE_CHILD_CLK_RDA(ahb1, "bus", &bus, 0, CLK_RDA_AHB1, NULL);
DEFINE_CHILD_CLK_RDA(apb1, "bus", &bus, 0, CLK_RDA_APB1, NULL);
DEFINE_CHILD_CLK_RDA(apb2, "bus", &bus, 0, CLK_RDA_APB2, NULL);
DEFINE_CHILD_CLK_RDA(gcg, "bus", &bus, 0, CLK_RDA_GCG, NULL);
DEFINE_CHILD_CLK_RDA(gpu, "bus", &bus, 0, CLK_RDA_GPU, NULL);
DEFINE_CHILD_CLK_RDA(vpu, "bus", &bus, 0, CLK_RDA_VPU, NULL);
DEFINE_CHILD_CLK_RDA(voc, "bus", &bus, 0, CLK_RDA_VOC, NULL);

/* childs of APB2 */
DEFINE_CHILD_CLK_RDA(uart1, "apb2", &apb2, 0, CLK_RDA_UART1, NULL);
DEFINE_CHILD_CLK_RDA(uart2, "apb2", &apb2, 0, CLK_RDA_UART2, NULL);
DEFINE_CHILD_CLK_RDA(uart3, "apb2", &apb2, 0, CLK_RDA_UART3, NULL);

/* childs of AHB1 */
DEFINE_CHILD_CLK_RDA(spiflash, "ahb1", &ahb1, 0, CLK_RDA_SPIFLASH, NULL);

/* childs of GCG */
DEFINE_CHILD_CLK_RDA(gouda, "gcg", &gcg, CLK_SET_RATE_PARENT,
		CLK_RDA_GOUDA, NULL);
DEFINE_CHILD_CLK_RDA(dpi, "gcg", &gcg, CLK_SET_RATE_PARENT,
		CLK_RDA_DPI, NULL);
DEFINE_CHILD_CLK_RDA(camera, "gcg", &gcg, CLK_SET_RATE_PARENT,
		CLK_RDA_CAMERA, NULL);

/* other clocks */
DEFINE_ROOT_CLK_RDA(bck, CLK_IS_ROOT, CLK_RDA_BCK, NULL);
DEFINE_ROOT_CLK_RDA(dsi, CLK_IS_ROOT, CLK_RDA_DSI, NULL);
DEFINE_ROOT_CLK_RDA(csi, CLK_IS_ROOT, CLK_RDA_CSI, NULL);
DEFINE_ROOT_CLK_RDA(debug, CLK_IS_ROOT, CLK_RDA_DEBUG, NULL);
DEFINE_ROOT_CLK_RDA(clk_out, CLK_IS_ROOT, CLK_RDA_CLK_OUT, NULL);
DEFINE_ROOT_CLK_RDA(aux_clk, CLK_IS_ROOT, CLK_RDA_AUX_CLK, NULL);
DEFINE_ROOT_CLK_RDA(clk_32k, CLK_IS_ROOT, CLK_RDA_CLK_32K, NULL);

static struct clk_lookup sys_clocks_lookups[] = {
	CLKDEV_INIT(NULL, "cpu", &cpu),
	CLKDEV_INIT(NULL, "bus", &bus),
	CLKDEV_INIT(NULL, "mem", &mem),
	CLKDEV_INIT(NULL, "usb", &usb),

	CLKDEV_INIT(NULL, "axi", &axi),
	CLKDEV_INIT(NULL, "ahb1", &ahb1),
	CLKDEV_INIT(NULL, "apb1", &apb1),
	CLKDEV_INIT(NULL, "apb2", &apb2),
	CLKDEV_INIT(NULL, "gcg", &gcg),
	CLKDEV_INIT(NULL, "gpu", &gpu),
	CLKDEV_INIT(NULL, "vpu", &vpu),
	CLKDEV_INIT(NULL, "voc", &voc),
	CLKDEV_INIT(NULL, "uart1", &uart1),
	CLKDEV_INIT(NULL, "uart2", &uart2),
	CLKDEV_INIT(NULL, "uart3", &uart3),

	CLKDEV_INIT(NULL, "spiflash", &spiflash),

	CLKDEV_INIT(NULL, "gouda", &gouda),
	CLKDEV_INIT(NULL, "dpi", &dpi),
	CLKDEV_INIT(NULL, "camera", &camera),

	CLKDEV_INIT(NULL, "bck", &bck),
	CLKDEV_INIT(NULL, "csi", &csi),
	CLKDEV_INIT(NULL, "debug", &debug),
	CLKDEV_INIT(NULL, "dsi", &dsi),
	CLKDEV_INIT(NULL, "clk_out", &clk_out),
	CLKDEV_INIT(NULL, "aux_clk", &aux_clk),
	CLKDEV_INIT(NULL, "clk_32k", &clk_32k),
};

void __init clk_rda_init(void)
{
	__clk_init(NULL, &cpu);
	__clk_init(NULL, &bus);
	__clk_init(NULL, &mem);
	__clk_init(NULL, &usb);
	__clk_init(NULL, &axi);
	__clk_init(NULL, &ahb1);
	__clk_init(NULL, &apb1);
	__clk_init(NULL, &apb2);
	__clk_init(NULL, &gcg);
	__clk_init(NULL, &gpu);
	__clk_init(NULL, &vpu);
	__clk_init(NULL, &voc);
	__clk_init(NULL, &uart1);
	__clk_init(NULL, &uart2);
	__clk_init(NULL, &uart3);
	__clk_init(NULL, &spiflash);

	gouda.ops = &gouda_clk_rda_ops;
	dpi.ops = &dpi_clk_rda_ops;
	camera.ops = &cam_clk_rda_ops;
	__clk_init(NULL, &gouda);
	__clk_init(NULL, &dpi);
	__clk_init(NULL, &camera);

	__clk_init(NULL, &bck);
	__clk_init(NULL, &dsi);
	__clk_init(NULL, &csi);
	__clk_init(NULL, &debug);

	clk_out.ops = &modem_clk_rda_ops;
	aux_clk.ops = &modem_clk_rda_ops;
	clk_32k.ops = &modem_clk_rda_ops;
	__clk_init(NULL, &clk_out);
	__clk_init(NULL, &aux_clk);
	__clk_init(NULL, &clk_32k);

	clkdev_add_table(sys_clocks_lookups, ARRAY_SIZE(sys_clocks_lookups));

#if 0
	clk_set_rate(&cpu, 800000000);
	clk_set_rate(&axi, 480000000);
	clk_set_rate(&ahb1, 120000000);
	clk_set_rate(&apb1, 120000000);
	clk_set_rate(&apb2, 120000000);
	clk_set_rate(&mem, 200000000);
#endif
}

