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
#ifndef _TGT_TYPES_H_
#define _TGT_TYPES_H_

#define KEY_MAP
#define KEY_BOOT_DOWNLOAD
#define TGT_RFD_CONFIG
#define TGT_PMD_CONFIG
#define TGT_AUD_CONFIG

typedef unsigned char BOOL;

// =============================================================================
// HAL_LCD_MODE_T
// -----------------------------------------------------------------------------
/// The LCD interface modes.
// =============================================================================
typedef enum
{
    HAL_LCD_MODE_SPI,
    HAL_LCD_MODE_PARALLEL_16BIT,
    HAL_LCD_MODE_DSI,
    HAL_LCD_MODE_RGB_16BIT,
    HAL_LCD_MODE_RGB_24BIT,

    HAL_LCD_MODE_QTY,
} HAL_LCD_MODE_T;

// =============================================================================
// HAL_CAM_MODE_T
// -----------------------------------------------------------------------------
/// The camera interface modes.
// =============================================================================
typedef enum
{
    HAL_CAM_MODE_PARALLEL,
    HAL_CAM_MODE_SPI,
    HAL_CAM_MODE_CSI,

    HAL_CAM_MODE_QTY,
} HAL_CAM_MODE_T;

// =============================================================================
// HAL_CAM_CSI_ID_T
// -----------------------------------------------------------------------------
/// The camera CSI IDs.
// =============================================================================
typedef enum
{
    HAL_CAM_CSI_NONE,
    HAL_CAM_CSI_1,
    HAL_CAM_CSI_2,

    HAL_CAM_CSI_ID_QTY,
} HAL_CAM_CSI_ID_T;

// =============================================================================
// HAL_CFG_SDMMC_T
// -----------------------------------------------------------------------------
/// This structure describes the SDMMC configuration for a given target.
// =============================================================================
typedef struct
{
    /// The sdmmc is used.
    BOOL sdmmcUsed :1;
    /// The sdmmc2 is used.
    BOOL sdmmc2Used :1;
    /// The sdmmc3 is used.
    BOOL sdmmc3Used :1;
} HAL_CFG_SDMMC_T;

// =============================================================================
// HAL_CFG_CAM_T
// -----------------------------------------------------------------------------
/// This structure describes the camera configuration for a given target.
/// The first field identify if camera is used.
/// The second and third field is the RemapFlag which identify if camera PDN/RST
/// need to be remapped to other GPIOs instead of default (GPIO_5 and GPIO_4),
/// for the first camera sensor.
/// The fourth and fifth field is the remapped pin when RemapFlag is set, for the
/// first camera sensor.
/// The sixth and seventh fields are used to describe which GPIOs are used for
/// the PDN/RST lines of the optional second camera sensor. This second camera
/// sensor can only be used on with GPIOs to control its PDN and RST lines.
// =============================================================================
typedef struct
{
    /// \c TRUE if the rear camera is used
    BOOL            camUsed :1;
    /// The polarity of the Power DowN line, for the rear sensor.
    BOOL            camPdnActiveH :1;
    /// The polarity of the Reset line, for the rear sensor.
    BOOL            camRstActiveH :1;
    /// The remapped GPIO controlling PDN (-1 if not remapped), for the rear sensor.
    INT32           camPdnRemap;
    /// The remapped GPIO controlling RST (-1 if not remapped), for the rear sensor.
    INT32           camRstRemap;
    /// The CSI ID (valid only if camera mode is CSI)
    HAL_CAM_CSI_ID_T camCsiId;
    /// \c TRUE if the front camera is used
    BOOL            cam1Used :1;
    /// The polarity of the Power DowN line, for the front sensor.
    BOOL            cam1PdnActiveH :1;
    /// The polarity of the Reset line, for the front sensor.
    BOOL            cam1RstActiveH :1;
    /// The remapped GPIO controlling PDN (-1 if not remapped), for the front sensor.
    INT32           cam1PdnRemap;
    /// The remapped GPIO controlling RST (-1 if not remapped), for the front sensor.
    INT32           cam1RstRemap;
    /// The CSI ID (valid only if camera mode is CSI)
    HAL_CAM_CSI_ID_T cam1CsiId;
    /// The camera interface mode
    HAL_CAM_MODE_T  camMode;
} HAL_CFG_CAM_T;

// =============================================================================
// HAL_CFG_PWM_T
// -----------------------------------------------------------------------------
/// This structure describes the PWM configuration for a given target.
/// The fields tell wether the pin corresponding to PWM output
/// is actually used as PWM output and not as something else (for
/// instance as a GPIO).
// =============================================================================
typedef struct
{
    /// \c TRUE if the PWL0 is used
    BOOL pwl0Used :1;
    /// \c TRUE if the PWL1 is used
    BOOL pwl1Used :1;
    /// \c TRUE if the PWT is used
    BOOL pwtUsed :1;
    /// \c TRUE if the LPG is used
    BOOL lpgUsed :1;
} HAL_CFG_PWM_T;

// =============================================================================
// HAL_CFG_I2C_T
// -----------------------------------------------------------------------------
/// This structure describes the I2C configuration for a given target. The
/// fields tell wether the corresponding I2C pins are actually used
/// for I2C and not as something else (for instance as a GPIO).
// =============================================================================
typedef struct
{
    /// \c TRUE if the I2C pins are used
    BOOL i2cUsed :1;
    /// \c TRUE if the I2C2 pins are used
    BOOL i2c2Used :1;
    /// \c TRUE if the I2C2 pins are used from cam pins
    BOOL i2c2PinsCam :1;
    /// \c TRUE if the I2C3 pins are used
    BOOL i2c3Used :1;
} HAL_CFG_I2C_T;

// =============================================================================
// HAL_CFG_I2S_T
// -----------------------------------------------------------------------------
/// This structure describes the I2S configuration for a given target. The
/// fields tell wether the corresponding I2S pin is actually used
/// for I2S and not as something else (for instance as a GPIO).
// =============================================================================
typedef struct
{
    /// \c TRUE if the data out pin is used
    BOOL doUsed :1;
    BOOL :3;
    /// \c TRUE if corresponding input is used
    BOOL di0Used :1;
    BOOL di1Used :1;
    BOOL di2Used :1;
} HAL_CFG_I2S_T;

// =============================================================================
// HAL_UART_CONFIG_T
// -----------------------------------------------------------------------------
/// Used to describes a configuration for used pin by an UART for a given target.
// =============================================================================
typedef enum
{
    /// invalid
    HAL_UART_CONFIG_INVALID = 0,

    /// UART is not used
    HAL_UART_CONFIG_NONE,

    /// UART use only data lines (TXD & RXD)
    HAL_UART_CONFIG_DATA,

    /// UART use data and flow control lines (TXD, RXD, RTS & CTS)
    HAL_UART_CONFIG_FLOWCONTROL,

    /// UART use all lines (TXD, RXD, RTS, CTS, RI, DSR, DCD & DTR)
    HAL_UART_CONFIG_MODEM,

    HAL_UART_CONFIG_QTY
} HAL_UART_CONFIG_T;

// =============================================================================
// HAL_CFG_SPI_T
// -----------------------------------------------------------------------------
/// This structure describes the SPI configuration for a given target. The first
/// fields tell wether the pin corresponding to chip select is actually used
/// as a chip select and not as something else (for instance as a GPIO).
/// Then, the polarity of the Chip Select is given. It is only relevant
/// if the corresponding Chip Select is used as a Chip Select.
/// Finally which pin is used as input, Can be none, one or the other.
/// On most chip configuration the input 0 (di0) is on the output pin: SPI_DIO
// =============================================================================
typedef struct
{
    /// \c TRUE if the corresponding pin is used as a Chip Select.
    BOOL cs0Used :1;
    BOOL cs1Used :1;
    BOOL cs2Used :1;
    BOOL cs3Used :1;
    /// \c TRUE if the first edge is falling
    BOOL cs0ActiveLow :1;
    BOOL cs1ActiveLow :1;
    BOOL cs2ActiveLow :1;
    BOOL cs3ActiveLow :1;
    /// \c TRUE if corresponding input is used
    BOOL di0Used :1;
    BOOL di1Used :1;
} HAL_CFG_SPI_T;

// =============================================================================
// HAL_CFG_GOUDA_T
// -----------------------------------------------------------------------------
/// This structure describes the GOUDA configuration for a given target.
/// The first fields tell wether the pin corresponding to chip select is
/// actually used as a chip select and not as something else (for instance
/// as a GPIO). If none are used, the GOUDA is considered unused.
// =============================================================================
typedef struct
{
    /// \c TRUE if the corresponding pin is used as a Chip Select.
    BOOL cs0Used :1;
    BOOL cs1Used :1;
    /// \c TRUE if LCD 16-23 bits are from camera, FALSE from NAND
    BOOL lcd16_23Cam :1;
    /// \c LCD interface mode
    HAL_LCD_MODE_T lcdMode;
} HAL_CFG_GOUDA_T;

#endif // _TGT_TYPES_H_
