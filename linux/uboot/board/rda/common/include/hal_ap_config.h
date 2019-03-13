/*
* devices.c - RDA platform devices
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

#ifndef _HAL_AP_AP_CONFIG_H_
#define _HAL_AP_AP_CONFIG_H_

#define CHIP_AP_STD_UART_QTY            3
#define CHIP_AP_STD_SPI_QTY             3

#define CHIP_AP_GOUDA_MAGIC_NUM         0x12345678
#define CHIP_AP_LCD_MAGIC_NUM           0x81234567
#define CHIP_AP_USB_MAGIC_NUM           0x78123456
#define CHIP_AP_BT_MAGIC_NUM            0x67812345
#define CHIP_AP_CAM_MAGIC_NUM           0x56781234
#define CHIP_AP_LED_MAGIC_NUM           0x45678123
#define CHIP_AP_MMC_MAGIC_NUM           0x34567812
#define CHIP_AP_EARPHONE_MAGIC_NUM      0x23456781
#define CHIP_AP_LOUDSPK_MAGIC_NUM       0x12345678

/* Synced with iomap @ap side */
#define CHIP_AP_SHARED_MEMORY_SIZE      0x00010000
#define CHIP_AP_SHARED_MEMORY_BASE      0x00100000

/* **********************************************************
*   gouda cofig
*
*   bankId    GPIO_A ,GPIO_B,GPIO_D??
*   goudaResetPin
*
*************************************************************/
struct HAL_AP_GOUDA_T{
    int magic;
    enum gpio_id reset_pin;
};

/* **********************************************************
*   lcd cofig
*
*   bankId    GPIO_A ,GPIO_B,GPIO_D??
*   lcdResetPin
*
*************************************************************/
struct HAL_AP_LCD_T{
    int magic;
    enum gpio_id reset_pin;
};

/* **********************************************************
*   usb cofig
*
*   bankId    GPIO_A ,GPIO_B,GPIO_D??
*   usbResetPin
*
*************************************************************/
struct HAL_AP_USB_T
{
    int magic;
    enum gpio_id reset_pin;
};

/* **********************************************************
*   bt cofig
*
*   bankId    GPIO_A ,GPIO_B,GPIO_D??
*   btResetPin
*
*************************************************************/
struct HAL_AP_BT_T{
    int magic;
    enum gpio_id reset_pin;
};

/* **********************************************************
*   cam cofig
*
*   bankId    GPIO_A ,GPIO_B,GPIO_D??
*   camResetPin
*
*************************************************************/
struct HAL_AP_CAM_T{
    int magic;
    enum gpio_id reset_pin;
};

/* **********************************************************
*   led cofig
*
*   bankId    GPIO_A ,GPIO_B,GPIO_D??
*   ledResetPin
*
*************************************************************/
struct HAL_AP_LED_T{
    int magic;
    enum gpio_id reset_pin;
};

/* **********************************************************
*   mmc cofig
*
*   bankId    GPIO_A ,GPIO_B,GPIO_D??
*   mmcResetPin
*
*************************************************************/
struct HAL_AP_MMC_T{
    int magic;
    enum gpio_id reset_pin;
};

/* **********************************************************
*   earphone cofig
*
*   bankId    GPIO_A ,GPIO_B,GPIO_D??
*   earphoneResetPin
*
*************************************************************/
struct HAL_AP_EARPHONE_T{
    int magic;
    enum gpio_id reset_pin;
};

/* **********************************************************
*   loudspk cofig
*
*   bankId    GPIO_A ,GPIO_B,GPIO_D??
*   loudSpkResetPin
*
*************************************************************/
struct HAL_AP_LOUDSPK_T{
    int magic;
    enum gpio_id reset_pin;
};

/* **********************************************************
*   configure gpio used by ap
*
*
*************************************************************/
struct HAL_AP_CFG_CONFIG_T{
    struct HAL_AP_GOUDA_T       gouda_cfg;
    struct HAL_AP_LCD_T         lcd_cfg;
    struct HAL_AP_USB_T         usb_cfg;
    struct HAL_AP_BT_T          bt_cfg;
    struct HAL_AP_CAM_T         cam_cfg;
    struct HAL_AP_LED_T         led_cfg;
    struct HAL_AP_MMC_T         mmc_cfg;
    struct HAL_AP_LOUDSPK_T     loudspk_cfg;
    struct HAL_AP_EARPHONE_T    earphone_cfg;

    int noConnectGpio_A;
    int usedGpio_A;
    int usedGpo_A;
    int noConnectGpio_B;
    int usedGpio_B;
    int noConnectGpio_D;
    int usedGpio_D;
};


#endif // _HAL_AP_AP_CONFIG_H_

