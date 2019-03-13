/*
 * Copyright (C) 2012 RDA Communications Inc.
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


#ifndef __REGULATOR_H__
#define __REGULATOR_H__

/*ldo regs*/
#define LDO_CAM			"v_cam"
#define LDO_USB			"v_usb"
#define LDO_SDMMC		"v_sdmmc"
#define LDO_FM			"v_fm"
#define LDO_BT			"v_bt"
#define LDO_LCD			"v_lcd"
#define LDO_CAM_FLASH	"v_camflash"
#define LDO_I2C			"v_i2c"
#define LDO_KEYPAD		"v_keypad"
#define LDO_BACKLIGHT	"v_backlight"
#define LDO_LEDR			"v_ledr"
#define LDO_LEDG			"v_ledg"
#define LDO_LEDB			"v_ledb"
#define LDO_VIBRATOR		"v_vibrator"


enum rda_ldo_src{
	rda_ldo_cam = 0,
	rda_ldo_usb,
	rda_ldo_sdmmc,
	rda_ldo_fm,
	rda_ldo_bt,
	rda_ldo_lcd,
	rda_ldo_cam_flash,
	rda_ldo_i2c,
	rda_ldo_keypad,
	rda_ldo_backlight,
	rda_ldo_ledr,
	rda_ldo_ledg,
	rda_ldo_ledb,
	rda_ldo_vibrator,
	rda_ldo_max,
};

/* Number of backlight levels */
#define POWER_BL_NUM	256
/* Number of led levels */
#define POWER_LED_NUM	256

#define POWER_NONE	0
#define POWER_OFF	0
#define POWER_ON		1

#endif /* __REGULATOR_H__ */

