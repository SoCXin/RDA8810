/*
 * Copyright (C) 2012 RDA Micro Inc.
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

#ifndef __ASM_ARCH_BOARD_H
#define __ASM_ARCH_BOARD_H

#ifdef	CONFIG_MACH_RDA8810
#include <mach/board-rda8810.h>
#endif

#ifdef	CONFIG_MACH_RDA8810E
#include <mach/board-rda8810e.h>
#endif

#ifdef	CONFIG_MACH_RDA8810
#include <mach/gpio-rda8810.h>
#elif defined(CONFIG_MACH_RDA8810E)
#include <mach/gpio-rda8810e.h>
#elif defined(CONFIG_MACH_RDA8820)
#include <mach/gpio-rda8820.h>
#elif defined(CONFIG_MACH_RDA8850E)
#include <mach/gpio-rda8850e.h>
#elif defined(CONFIG_MACH_RDA8810H)
#include <mach/gpio-rda8810h.h>
#include <mach/board-rda8810h.h>
#endif

#endif
