 /*
  * Copyright (C) 2013-2014 RDA Microelectronics Inc.
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

#include <linux/kernel.h>
#include <linux/export.h>
#include <mach/iomap.h>
#include <plat/reg_mem_sc.h>

typedef enum {
    HAL_MEM_SC_AP_ISPI = 0,
    HAL_MEM_SC_WCPU_TRACE = 1,
    HAL_MEM_SC_AP_VOC = 2,

    // Add Others above ...
    // The max ID is 31

    HAL_MEM_SC_QTY = 32,
} HAL_MEM_SC_T;

#if 0

static HWP_MEM_BRIDGE_T *hwp_memBridge =
	((HWP_MEM_BRIDGE_T *)IO_ADDRESS(RDA_MODEM_EBC_BASE));

unsigned int memsc_ispi_lock(void)
{
	return hwp_memBridge->MemSC[HAL_MEM_SC_AP_ISPI];
}
EXPORT_SYMBOL(memsc_ispi_lock);

void memsc_ispi_unlock(void)
{
	hwp_memBridge->MemSC[HAL_MEM_SC_AP_ISPI] = 1;
}
EXPORT_SYMBOL(memsc_ispi_unlock);

unsigned int memsc_voc_lock(void)
{
	return hwp_memBridge->MemSC[HAL_MEM_SC_AP_VOC];
}
EXPORT_SYMBOL(memsc_voc_lock);

void memsc_voc_unlock(void)
{
	hwp_memBridge->MemSC[HAL_MEM_SC_AP_VOC] = 1;
}
EXPORT_SYMBOL(memsc_voc_unlock);

#endif

