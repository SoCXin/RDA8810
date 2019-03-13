/*
 * reg_rtc.h - A header file of definition of rtc of RDA
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

#ifndef __REG_RTC_H__
#define __REG_RTC_H__

#include <linux/bitops.h>

#define RDA_RTC_CTRL_REG        0x00000000
#define RDA_RTC_CMD_REG         0x00000004
#define RDA_RTC_STA_REG         0x00000008
#define RDA_RTC_CAL_LOAD_LOW_REG        0x0000000C
#define RDA_RTC_CAL_LOAD_HIGH_REG       0x00000010
#define RDA_RTC_CUR_LOAD_LOW_REG        0x00000014
#define RDA_RTC_CUR_LOAD_HIGH_REG       0x00000018
#define RDA_RTC_ALARM_LOW_REG       0x0000001C
#define RDA_RTC_ALARM_HIGH_REG      0x00000020

#define RDA_RTC_CMD_CAL_LOAD    BIT(0)
#define RDA_RTC_CMD_ALARM_LOAD      BIT(4)
#define RDA_RTC_CMD_ALARM_ENABLE       BIT(5)
#define RDA_RTC_CMD_ALARM_DISABLE       BIT(6)
#define RDA_RTC_CMD_INVALID     BIT(31)

#define RDA_RTC_STA_ALARM_ENABLE        BIT(20)
#define RDA_RTC_STA_NOT_PROG        BIT(31)

#define SET_SEC(n)      (((n) & 0x3F) << 0)
#define GET_SEC(n)    ((n) & 0x3F)

#define SET_MIN(n)      (((n) & 0x3F) << 8)
#define GET_MIN(n)    (((n) >> 8) & 0x3F)

#define SET_HOUR(n)     (((n) & 0x1F) << 16)
#define GET_HOUR(n)     (((n) >> 16) & 0x1F)

#define SET_MDAY(n)      (((n) & 0x1F) << 0)
#define GET_MDAY(n)     ((n) & 0x1F)

#define SET_MON(n)      (((n) & 0xF) << 8)
#define GET_MON(n)      (((n) >> 8) & 0xF)

#define SET_YEAR(n)     (((n) & 0x7F) << 16)
#define GET_YEAR(n)     (((n) >> 16) & 0x7F)

#define SET_WDAY(n)     (((n) & 0x7) << 24)
#define GET_WDAY(n)     (((n) >> 24) & 0x7)

#endif /* __REG_RTC_H__ */

