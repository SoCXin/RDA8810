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

#ifndef __PLAT_INTC_H__
#define __PLAT_INTC_H__

#ifdef CONFIG_SUSPEND
uint32_t rda_intc_get_mask(void);
int rda_intc_set_wakeup_mask(uint32_t wakeup_mask);
int rda_intc_set_cpu_sleep(void);
int rda_intc_get_wakeup_cause(uint32_t *wakeup_cause);
#endif

#ifdef CONFIG_ARM_GIC
void rda_gic_init(void);
u32 rda_gic_get_wakeup_mask(void);
void rda_intc_unmask_irqs(int irq_mask);
#endif
#endif /* __PLAT_INTC_H__ */
