/*
 * devices.c - RDA5888 Device Driver.
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

#ifndef _RDA5888_H
#define _RDA5888_H

#define RDA5888_TEST_SET

#define RDA5888_IOC_MAGIC    'a'

/* control message */
#define RDA5888_CTL_TEST           _IO(RDA5888_IOC_MAGIC,  0x00)
#define RDA5888_READ               _IOW(RDA5888_IOC_MAGIC, 0x01, unsigned int)
#define RDA5888_WRITE              _IOW(RDA5888_IOC_MAGIC, 0x02, unsigned int)
#define RDA5888_POWER_ON           _IOW(RDA5888_IOC_MAGIC, 0x03, unsigned int)
#define RDA5888_POWER_OFF          _IOW(RDA5888_IOC_MAGIC, 0x04, unsigned int)
#define RDA5888_SCAN_CH            _IOW(RDA5888_IOC_MAGIC, 0x05, unsigned int)
#ifdef RDA5888_TEST_SET
#define RDA5888_SET_CROP           _IOW(RDA5888_IOC_MAGIC, 0x10, unsigned int)
#define RDA5888_ISP_READ           _IOW(RDA5888_IOC_MAGIC, 0x11, unsigned int)
#define RDA5888_ISP_WRITE          _IOW(RDA5888_IOC_MAGIC, 0x12, unsigned int)
#endif

#define RDA5888_DELAY_FLAG			0xffff

/* rda5888 chip config. */
#define RDA5888_IODRV_ENHANCE		1
#define RDA5888_HSYNC_HOLD			0
#define RDA5888_SHARE_26MCRYSTAL	1

struct rda_reg_data {
	unsigned short reg_num;
	unsigned short value;
};

struct rdamtv_scan_data {
	unsigned int vfreq;
	unsigned int vid_std;
	unsigned int ret;
};

#ifdef RDA5888_TEST_SET
struct rda_set_crop_data {
	int x;
	int y;
	int width;
	int height;
};

struct isp_reg_data {
	unsigned int reg_num;
	unsigned int value;
};
#endif

/*
 * Commonly used video formats are included here.
 */
enum rdamtv_vstd {
	RDAMTV_VSTD_PAL_M       = 1,
	RDAMTV_VSTD_PAL_B       = 2,
	RDAMTV_VSTD_PAL_B1      = 3,
	RDAMTV_VSTD_PAL_D       = 4,
	RDAMTV_VSTD_PAL_D1      = 5,
	RDAMTV_VSTD_PAL_G       = 6,
	RDAMTV_VSTD_PAL_H       = 7,
	RDAMTV_VSTD_PAL_K       = 8,
	RDAMTV_VSTD_PAL_N       = 9,
	RDAMTV_VSTD_PAL_I       = 10,
	RDAMTV_VSTD_PAL_NC      = 11,

	RDAMTV_VSTD_NTSC_M      = 21,
	RDAMTV_VSTD_NTSC_B_G    = 22,
	RDAMTV_VSTD_NTSC_D_K    = 23,
	RDAMTV_VSTD_NTSC_I      = 24,

	RDAMTV_VSTD_SECAM_M     = 31,
	RDAMTV_VSTD_SECAM_B     = 32,
	RDAMTV_VSTD_SECAM_B1    = 33,
	RDAMTV_VSTD_SECAM_D     = 34,
	RDAMTV_VSTD_SECAM_D1    = 35,
	RDAMTV_VSTD_SECAM_G     = 36,
	RDAMTV_VSTD_SECAM_H     = 37,
	RDAMTV_VSTD_SECAM_K     = 38,
	RDAMTV_VSTD_SECAM_N     = 39,
	RDAMTV_VSTD_SECAM_I     = 40,
	RDAMTV_VSTD_SECAM_NC    = 41,
	RDAMTV_VSTD_SECAM_L     = 42,

	RDAMTV_VSTD_NONE        = 255
};

enum sys_mode
{
	ATV_SYS_RESET_MODE0,
	ATV_SYS_RESET_MODE1,
	ATV_SYS_RESET_MODE2,
	ATV_SYS_RESET_INVALID
};

enum rdamtv_rt
{
	RDAMTV_RT_SUCCESS = 0,
	RDAMTV_RT_SCAN_FAIL,
	RDAMTV_RT_SCAN_DONE,
	RDAMTV_RT_ERROR
};

#endif /* end of _RDA5888_H */
