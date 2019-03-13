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

#ifndef __REG_PD_CTRL_H__
#define __REG_PD_CTRL_H__

#include <mach/hardware.h>
#include <mach/iomap.h>

/* ISOLATE register */
#define PDC_ISOLATE_REG	0x00
#define 	PDC_ISOLATE_CPU		(1UL << 0)
#define 	PDC_ISOLATE_GPU		(1UL << 1)
#define 	PDC_ISOLATE_VPU		(1UL << 2)

/* CPU_CLK_ON register */
#define PDC_CPU_CLK_REG	0x04
#define 	PDC_CPU_CLK_ENABLE		(1UL << 0)

/* CPU_RESETN register */
#define PDC_CPU_RESET_REG	0x08
#define 	PDC_CPU_RESET		(1UL << 0)

/* GPU_CLK_ON register */
#define PDC_GPU_CLK_REG	0x0C
#define 	PDC_GPU_CLK_ENABLE	(1UL << 0)

/* GPU_RESETN register */
#define PDC_GPU_RESET_REG	0x10
#define 	PDC_GPU_RESET		(1UL << 0)

/* VPU_CLK_ON register */
#define PDC_VPU_CLK_REG	0x14
#define 	PDC_VPU_CLK_ENABLE		(1UL << 0)

/* VPU_RESETN register */
#define PDC_VPU_RESET_REG	0x18
#define 	PDC_VPU_RESET		(1UL << 0)

/* GPU_STATUS register */
#define PDC_GPU_STATUS_REG	0x1C
#define 	PDC_GPU_IDLE_L2C		(1UL << 0)
#define 	PDC_GPU_IDLE_GPMMU	(1UL << 1)
#define 	PDC_GPU_IDLE_GP		(1UL << 2)
#define 	PDC_GPU_IDLE_PPMMU1	(1UL << 3)
#define 	PDC_GPU_IDLE_PP1		(1UL << 4)
#define 	PDC_GPU_IDLE_PPMU0	(1UL << 5)
#define 	PDC_GPU_IDLE_PP0		(1UL << 6)
#define 	PDC_GPU_STATUS_MASK		(0x7F)

/* CPU_PD_CTRL register */
#define PDC_CPU_PD_CTRL_REG	0x20
#define 	PDC_CPU_PD_CTRL_MASK		(0x1FF)

/* CPU_HS_EMA register */
#define PDC_CPU_HS_EMA_REG	0x24
#define	PDC_HS_EMA_MASK		(0x7 << 0)
#define 	PDC_HS_EMA(n)		(((n) & 0x7) << 0)
#define 	PDC_HS_EMAW_MASK		(0x3 << 3)
#define 	PDC_HS_EMAW(n)	(((n) & 0x3) << 3)
#define 	PDC_HS_EMAS		(1UL << 5)
#define 	PDC_HD_EMA_MASK		(0x7 << 6)
#define 	PDC_HD_EMA(n)		(((n) & 0x7) << 6)
#define 	PDC_HD_EMAW_MASK		(0x3 << 9)
#define 	PDC_HD_EMAW(n)	(((n) & 0x3) << 9)
#define 	PDC_HD_EMAS		(1UL << 11)

/* CPU_PD_STATUS0 register */
#define PDC_CPU_PD_STA0_REG	0x28
#define 	PDC_CPU_STA_IRQ0_MASK		(0xFFFFFFFF)

/* CPU_PD_STATUS1 register */
#define PDC_CPU_PD_STA1_REG	0x2C
#define	PDC_CPU_STA_IRQ1_MASK		(0x7FFFFF)

/* CPU_POWER_SWITCH register */
#define PDC_CPU_SWITCH_REG	0x30
#define 	PDC_CPU_SLEEP0		(1UL << 0)
#define 	PDC_CPU_SLEEP1		(1UL << 1)
#define 	PDC_CPU_SLEEP1_SEL	(1UL << 2)
#define	PDC_CPU_SLEEP1_CNT(n)	(((n) & 0xFF) << 8)

/* GPU_POWER_SWITCH register */
#define PDC_GPU_SWITCH_REG	0x34
#define	PDC_GPU_SLEEP0		(1UL << 0)

/* VPU_POWER_SWITCH register */
#define PDC_VPU_SWITCH_REG	0x38
#define 	PDC_VPU_SLEEP0		(1UL << 0)

/* POWER_SWITCH_STATUS register */
#define PDC_SWITCH_STA_REG	0x3C
#define	PDC_CPU_ACK0	(1UL << 0)
#define	PDC_CPU_ACK1	(1UL << 1)
#define	PDC_GPU_ACK0	(1UL << 2)
#define	PDC_VPU_ACK0	(1UL << 4)

#endif /* __REG_PD_CTRL_H__ */

