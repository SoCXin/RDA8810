/* arch/arm/mach-goldfish/include/mach/debug-macro.S
 *
 * Copyright (C) 2009 Google, Inc.
 * Author: David Turner <digit@google.com>
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

#include <mach/hardware.h>

	.macro addruart, rp, rv, tmp
	ldr	\rp, = RDA_TTY_PHYS
	ldr	\rv, = RDA_TTY_BASE
	.endm

	.macro	senduart,rd,rx
	strb 	\rd, [\rx, #(RDA_TTY_TXRX)]
	.endm

	.macro	waituart,rd,rx
1001:	ldr	\rd, [\rx, #(RDA_TTY_STATUS)]
	and	\rd, \rd, #0x1f00
	cmp	\rd, #0x100
	beq	1001b
	.endm

	.macro	busyuart,rd,rx
	.endm
