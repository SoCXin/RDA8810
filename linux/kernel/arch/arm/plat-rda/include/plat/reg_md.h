/*
 * reg_md.h - A header file of modem's registers of RDA
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

#ifndef __REG_MD_H__
#define __REG_MD_H__

#include <linux/bitops.h>

#define RDA_MD_SNAPSHOT_REG     0x00000000
#define RDA_MD_SNAPSHOT_CFG_REG     0x00000004
#define RDA_MD_CAUSE_REG        0x00000008
#define RDA_MD_MASK_SET_REG     0x0000000C
#define RDA_MD_MASK_CLR_REG     0x00000010
#define RDA_MD_IT_SET_REG       0x00000014
#define RDA_MD_IT_CLR_REG       0x00000018

#define INIT_BUF_REQ_BIT   BIT(2) /* Init buffer request */
#define SLEEP_REQ_BIT      BIT(3) /* Sleep request */
#define MODEM_CRASH_BIT    BIT(4) /* A modem CPU crashed */
#define BP_SHUTDOWN_BIT    BIT(5) /* BP is power-off as low power. */
#define DBG_TRIGGER_BIT    BIT(7) /* A reserved bit used by BP for trigger AP to dump stack.*/

#define AT_READY_BIT        BIT(8)
#define AT_EMPTY_BIT        BIT(9)
#define SYS_READY_BIT       BIT(10)
#define SYS_EMPTY_BIT       BIT(11)
#define TRACE_READY_BIT         BIT(12)
#define TRACE_EMPTY_BIT         BIT(13)
#ifdef CONFIG_MACH_RDA8850E
#define PS_READY_BIT		BIT(14)
#define PS_EMPTY_BIT		BIT(15)
#endif

#define STATUS_AT_READY   AT_READY_BIT
#define STATUS_AT_EMPTY     AT_EMPTY_BIT
#define STATUS_SYS_READY      SYS_READY_BIT
#define STATUS_SYS_EMPTY        SYS_EMPTY_BIT
#define STATUS_TRACE_READY    TRACE_READY_BIT
#define STATUS_TRACE_EMPTY      TRACE_EMPTY_BIT
#ifdef CONFIG_MACH_RDA8850E
#define STATUS_PS_READY    	PS_READY_BIT
#define STATUS_PS_EMPTY  	PS_EMPTY_BIT
#endif

#define IRQ1_MASK_SET_ALL        (0xFF << 8)
#define IRQ1_MASK_CLR_ALL    (0xFF << 8)

#define IT_STA_SET_ALL      (0xFF << 8)
#define IT_STA_CLR_ALL      (0xFF << 8)

#endif /* __REG_MD_H__ */

