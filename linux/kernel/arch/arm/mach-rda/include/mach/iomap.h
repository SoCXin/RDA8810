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

#ifndef __ASM_ARCH_IOMAP_H
#define __ASM_ARCH_IOMAP_H

#ifdef	CONFIG_ARCH_RDA8810
#include "iomap-rda8810.h"
#elif defined(CONFIG_ARCH_RDA8810E)
#include "iomap-rda8810e.h"
#elif defined(CONFIG_ARCH_RDA8820)
#include "iomap-rda8820.h"
#elif defined(CONFIG_ARCH_RDA8850E)
#include "iomap-rda8850e.h"
#elif defined(CONFIG_ARCH_RDA8810H)
#include "iomap-rda8810h.h"
#else
#error "Unknown ARCH for IOMAP define"
#endif

#endif /* __ASM_ARCH_IOMAP_H */
