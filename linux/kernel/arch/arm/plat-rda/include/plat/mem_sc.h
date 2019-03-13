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

#ifndef __MEM_SC_H__
#define __MEM_SC_H__

unsigned int memsc_ispi_lock(void);
void memsc_ispi_unlock(void);

unsigned int memsc_voc_lock(void);
void memsc_voc_unlock(void);

#endif /* __MEM_SC_H__ */
