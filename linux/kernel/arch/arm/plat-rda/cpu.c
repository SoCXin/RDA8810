/*
 * cpu.c - Getting information of CPU
 *
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

#include <linux/init.h>
#include <asm/io.h>

#include <plat/cpu.h>
#include <plat/reg_cfg_regs.h>

struct rda_socinfo {
	unsigned short prod_id;
	unsigned short metal_id;
	unsigned short bond_id;
};

#define RDA8810_ID	8810
#define RDA8810_U06_ID	10

static struct rda_socinfo rda_soc_info;

unsigned short rda_get_soc_metal_id(void)
{
	return rda_soc_info.metal_id;
}

unsigned short rda_get_soc_prod_id(void)
{
	return rda_soc_info.prod_id;
}

int rda_soc_is_older_metal10(void)
{
	if (rda_soc_info.prod_id ==0) {
		return 1;
	}

	if (rda_soc_info.prod_id == RDA8810_ID &&
		rda_soc_info.metal_id < RDA8810_U06_ID) {
		return 1;
	}

	return 0;
}

static int __init rda_soc_init(void)
{
	HWP_CFG_REGS_T __iomem *hwp_configRegs = NULL;

	hwp_configRegs = ioremap(RDA_CONFIG_REGS_PHYS, RDA_CONFIG_REGS_SIZE);
	if (hwp_configRegs) {
		rda_soc_info.prod_id = GET_BITFIELD(hwp_configRegs->CHIP_ID, CFG_REGS_PROD_ID);
		rda_soc_info.metal_id = GET_BITFIELD(hwp_configRegs->CHIP_ID, CFG_REGS_METAL_ID);
		iounmap(hwp_configRegs);
	}

	return 0;
}
arch_initcall(rda_soc_init);

