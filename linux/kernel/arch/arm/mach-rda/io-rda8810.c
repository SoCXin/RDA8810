/*
 * Copyright (C) 2012 RDA Communications Inc.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/bug.h>

#include <asm/io.h>
#include <asm/page.h>
#include <asm/mach/map.h>
#include <mach/hardware.h>

#define RDA_DEVICE(name) { \
	.virtual = RDA_##name##_BASE, \
	.pfn = __phys_to_pfn(RDA_##name##_PHYS), \
	.length = RDA_##name##_SIZE, \
	.type = MT_DEVICE_NONSHARED, \
	}

static struct map_desc rda_io_desc[] __initdata = {
	RDA_DEVICE(SRAM),
	RDA_DEVICE(MODEM_EBC),
	RDA_DEVICE(MODEM_BCPU),
	RDA_DEVICE(MODEM_XCPU),
	RDA_DEVICE(L2CC),
	RDA_DEVICE(INTC),
	RDA_DEVICE(IMEM),
	RDA_DEVICE(SYSCTRL),
	RDA_DEVICE(TIMER),
	RDA_DEVICE(GPIOA),
	RDA_DEVICE(GPIOB),
	RDA_DEVICE(GPIOD),
	RDA_DEVICE(GPIOC),
	RDA_DEVICE(PWM),
	RDA_DEVICE(COMREGS),
	RDA_DEVICE(DBGAPB),
	RDA_DEVICE(AIF),
	RDA_DEVICE(AUIFC),
	RDA_DEVICE(UART3),
	RDA_DEVICE(IFC),
};

void __init rda8810_map_io(void)
{
	iotable_init(rda_io_desc, ARRAY_SIZE(rda_io_desc));
}

