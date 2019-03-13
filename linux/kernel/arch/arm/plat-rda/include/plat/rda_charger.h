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
#ifndef __RDA_CHARGER_H__
#define __RDA_CHARGER_H__

/* Status of charger that is as same as BP. */
#define PM_CHARGER_DISCONNECTED      0
#define PM_CHARGER_CONNECTED         1
#define PM_CHARGER_CHARGING           2
#define PM_CHARGER_FINISHED          3
#define PM_CHARGER_ERROR_TEMPERATURE 4
#define PM_CHARGER_ERROR_VOLTAGE     5
#define PM_CHARGER_ERROR_UNKNOWN     9

#endif /* __RDA_CHARGER_H__ */
