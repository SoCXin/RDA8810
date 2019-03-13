/* include/asm-arm/arch-goldfish/entry-macro.S
**
** Copyright (C) 2007 Google, Inc.
**
** This software is licensed under the terms of the GNU General Public
** License version 2, as published by the Free Software Foundation, and
** may be copied, distributed, and modified under those terms.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
*/

#include <mach/hardware.h>
#include <plat/reg_intc.h>

	.macro  disable_fiq
	.endm

	.macro  get_irqnr_preamble, base, tmp
	.endm

	.macro  arch_ret_to_user, tmp1, tmp2
	.endm

	@ this macro checks which interrupt occured
	@ and returns its number in irqnr
	@ and returns if an interrupt occured in irqstat 
	.macro  get_irqnr_and_base, irqnr, irqstat, base, tmp
		ldr	\base, =IO_ADDRESS(RDA_INTC_BASE)
		ldr	\irqstat, [\base, #RDA_INTC_FINALSTATUS]	@ get masked status
		rsb	\irqnr, \irqstat, #0
		and	\irqnr, \irqnr, \irqstat
		clz	\irqnr, \irqnr
		rsb	\irqnr, \irqnr, #31
		teq	\irqstat, #0
	.endm
