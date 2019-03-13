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

#ifndef __GPIO_RDA8810H_H__
#define __GPIO_RDA8810H_H__

#ifndef __ASM_ARCH_BOARD_H
#error  "Don't include this file directly, include <mach/board.h>"
#endif

#include "gpio_id.h"
#include "tgt_ap_gpio_setting.h"

#define GPIO_TOUCH_RESET	_TGT_AP_GPIO_TOUCH_RESET
#define GPIO_TOUCH_IRQ		_TGT_AP_GPIO_TOUCH_IRQ

#ifdef _TGT_AP_GPIO_CAM_RESET
#define GPIO_CAM_RESET		_TGT_AP_GPIO_CAM_RESET
#endif

#ifdef _TGT_AP_GPIO_CAM_PWDN0
#define GPIO_CAM_PWDN0		_TGT_AP_GPIO_CAM_PWDN0
#endif

#ifdef _TGT_AP_GPIO_CAM_PWDN1
#define GPIO_CAM_PWDN1		_TGT_AP_GPIO_CAM_PWDN1
#endif

#ifdef _TGT_AP_GPIO_CAM_FLASH
#define GPIO_CAM_FLASH		_TGT_AP_GPIO_CAM_FLASH
#endif

#define GPIO_LCD_RESET		_TGT_AP_GPIO_LCD_RESET

#ifdef _TGT_AP_GPIO_LCD_EN
#define GPIO_LCD_EN         _TGT_AP_GPIO_LCD_EN
#endif
#ifdef _TGT_AP_GPIO_LCD_PWR
#define GPIO_LCD_PWR		_TGT_AP_GPIO_LCD_PWR
#endif

#define GPIO_HEADSET_DETECT	_TGT_AP_GPIO_HEADSET_DETECT

/* gpio settings for usb otg */
#define GPIO_OTG_DETECT		_TGT_AP_GPIO_OTG_DETECT
#define GPIO_USB_VBUS_SWITCH	_TGT_AP_GPIO_USB_VBUS_SWITCH
#ifdef _TGT_AP_GPIO_USBID_CTRL
#define GPIO_USBID_CTRL		_TGT_AP_GPIO_USBID_CTRL
#else
#define GPIO_USBID_CTRL		0
#endif
#ifdef _TGT_AP_GPIO_PLUGIN_CTRL
#define GPIO_PLUGIN_CTRL	_TGT_AP_GPIO_PLUGIN_CTRL
#else
#define GPIO_PLUGIN_CTRL	0
#endif

#define GPIO_VOLUME_UP		_TGT_AP_GPIO_VOLUME_UP
#define GPIO_VOLUME_DOWN	_TGT_AP_GPIO_VOLUME_DOWN

/*GPIO_WIFI*/
#ifdef _TGT_AP_GPIO_WIFI
#define GPIO_WIFI		_TGT_AP_GPIO_WIFI
#else
#define GPIO_WIFI		GPIO_MAX_NUM
#endif /*_TGT_AP_GPIO_WIFI*/

/*GPIO_BT_HOST_WAKE*/
#ifdef _TGT_AP_GPIO_BT_HOST_WAKE
#define GPIO_BT_HOST_WAKE	_TGT_AP_GPIO_BT_HOST_WAKE
#else
#define GPIO_BT_HOST_WAKE	GPIO_MAX_NUM
#endif /*_TGT_AP_GPIO_BT_HOST_WAKE*/

/*GPIO setting for external audio PA*/
#ifdef _TGT_AP_GPIO_AUDIO_EXTPA
#define GPIO_AUDIO_EXTPA _TGT_AP_GPIO_AUDIO_EXTPA
#else
#define GPIO_AUDIO_EXTPA 0
#endif

#ifdef _TGT_AP_GPIO_AUDIO_EXTPA_1
#define GPIO_AUDIO_EXTPA_1 _TGT_AP_GPIO_AUDIO_EXTPA_1
#else
#define GPIO_AUDIO_EXTPA_1 0
#endif

#endif
