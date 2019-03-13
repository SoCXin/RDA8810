////////////////////////////////////////////////////////////////////////////////
//                                                                            //
//            Copyright (C) 2013, RDA Microeletronics.                        //
//                            All Rights Reserved                             //
//                                                                            //
//      This source code is the property of RDA Microeletronics and is        //
//      confidential.  Any  modification, distribution,  reproduction or      //
//      exploitation  of  any content of this file is totally forbidden,      //
//      except  with the  written permission  of  RDA Microeletronics.        //
//                                                                            //
////////////////////////////////////////////////////////////////////////////////
//                                                                            //
//  $HeadURL: http://svn.rdamicro.com/svn/developing1/Sources/chip/branches/8810/hal/8810/src/hal_config.c $ //
//    $Author: huazeng $                                                        //
//    $Date: 2013-08-31 17:48:58 +0800 (Sat, 31 Aug 2013) $                     //
//    $Revision: 21005 $                                                          //
//                                                                            //
////////////////////////////////////////////////////////////////////////////////
//                                                                            //
/// @file hal_config.c
/// Implementation of HAL initialization related with the particular instance
/// of the IP.
//                                                                            //
////////////////////////////////////////////////////////////////////////////////

/* uboot */
#include <common.h>

/* common */
#include <asm/arch/rda_iomap.h>
#include <asm/arch/hwcfg.h>
#include <asm/arch/chip_id.h>
#include <asm/arch/cs_types.h>
#include <asm/arch/global_macros.h>
#include <asm/arch/reg_gpio.h>

/* device */
#include "hal_sys.h"
#include "halp_sys.h"
#include "halp_gpio.h"
#include "bootp_mode.h"
#include "hal_config.h"
#include "hal_ap_gpio.h"
#include "hal_ap_config.h"
#include <asm/arch/reg_cfg_regs.h>

/* target */
#include "tgt_board_cfg.h"
#include "tgt_ap_gpio_setting.h"
#define HAL_ASSERT(cond, fmt, ...) \
    if(!(cond)) { \
        hal_assert(fmt, ##__VA_ARGS__); \
        g_halConfigError = TRUE; \
    }

//// =============================================================================
////  GLOBAL VARIABLES
//// =============================================================================
PRIVATE CONST UINT8 g_halCfgSimOrder[] = { TGT_SIM_ORDER };
PRIVATE HAL_CFG_CONFIG_T hal_cfg = TGT_HAL_CONFIG;
PRIVATE UINT32 g_bootBootMode = 0;
PRIVATE BOOL g_halConfigError = FALSE;


// =============================================================================
//  FUNCTIONS
// =============================================================================
// HAL_ASSERT
PRIVATE VOID hal_assert(const char *fmt, ...)
{
    va_list args;
    puts("\n#-------Board Mux config found error-------#\n");
    va_start(args, fmt);
    vprintf(fmt, args);
    putc('\n');
    puts("--------------------------------------------\n\n");
    va_end(args);
}

// =============================================================================
// hal_BoardSimUsed
// -----------------------------------------------------------------------------
/// Check whether a SIM card interface is used.
// ============================================================================
PRIVATE BOOL hal_BoardSimUsed(UINT32 simId)
{
    UINT32 i;
    for (i=0; i<ARRAY_SIZE(g_halCfgSimOrder) && i<NUMBER_OF_SIM; i++)
    {
        if (g_halCfgSimOrder[i] == simId)
        {
            return TRUE;
        }
    }

    return FALSE;
}

#if 0
// =============================================================================
// hal_BoardConfigClk32k
// -----------------------------------------------------------------------------
/// Configure CLK_32K output.
/// @param enable TRUE to configure, and FALSE to disable.
// ============================================================================
PROTECTED VOID hal_BoardConfigClk32k(BOOL enable)
{
    HAL_ASSERT(g_halCfg->useClk32k == TRUE, "32K clock is not configured");

    if (enable)
    {
        // Setup the pin as 32K clock output
        hwp_configRegs->Alt_mux_select =
            (hwp_configRegs->Alt_mux_select & ~CFG_REGS_GPO_2_MASK) |
            CFG_REGS_GPO_2_CLK_32K;
    }
    else
    {
        // Setup the pin as GPO (and low ouput has been
        // configured in hal_BoardSetup())
        hwp_configRegs->Alt_mux_select =
            (hwp_configRegs->Alt_mux_select & ~CFG_REGS_GPO_2_MASK) |
            CFG_REGS_GPO_2_GPO_2;
    }
}
#endif

// =============================================================================
// hal_BoardSetupGeneral
// -----------------------------------------------------------------------------
/// Apply board dependent configuration to HAL for general purpose
/// @param halCfg Pointer to HAL configuration structure (from the target
/// module).
// ============================================================================
PROTECTED VOID hal_BoardSetupGeneral(CONST HAL_CFG_CONFIG_T* halCfg)
{
    UINT32 altMux = 0;
    UINT32 altMux2 = 0;

    UINT32 availableGpo_A = 0;

    UINT32 availableGpio_C = 0;
    UINT32 availableGpio_A = 0;
    UINT32 availableGpio_B = 0;
    UINT32 availableGpio_D = 0;
    UINT32 availableGpio_E = 0;

    UINT32 bootModeGpio_C = 0;
    UINT32 bootModeGpio_A = 0;
    UINT32 bootModeGpio_B = 0;
    UINT32 bootModeGpio_D = 0;
    UINT32 bootModeGpio_E = 0;

    UINT32 noConnectMask_C = 0;
    UINT32 noConnectMask_A = 0;
    UINT32 noConnectMask_B = 0;
    UINT32 noConnectMask_D = 0;
    UINT32 noConnectMask_E = 0;

    UINT32 gpoClr_A = 0;

#ifdef FPGA

    // no muxing, do nothing

#else // !FPGA

    // GPIOs as boot mode pins
    bootModeGpio_C = HAL_GPIO_BIT(1)
                   | HAL_GPIO_BIT(2)
                   | HAL_GPIO_BIT(3)
                   | HAL_GPIO_BIT(4)
                   | HAL_GPIO_BIT(5);
    bootModeGpio_A = HAL_GPIO_BIT(4)
                   | HAL_GPIO_BIT(5)
                   | HAL_GPIO_BIT(6)
                   | HAL_GPIO_BIT(7);
    bootModeGpio_B = HAL_GPIO_BIT(0);
    bootModeGpio_D = 0;
    bootModeGpio_E = 0;

    // Available GPIOs
    availableGpio_C |= HAL_GPIO_BIT(0)
                     | HAL_GPIO_BIT(1)
                     | HAL_GPIO_BIT(2)
                     | HAL_GPIO_BIT(3)
                     | HAL_GPIO_BIT(4)
                     | HAL_GPIO_BIT(5);
    // Volume down/up keys
    availableGpio_D |= HAL_GPIO_BIT(5)
                     | HAL_GPIO_BIT(6);

    // Boot modes
    BOOL spiFlashCam = FALSE;
    BOOL spiFlashNand = FALSE;
    if (g_bootBootMode & BOOT_MODE_BOOT_SPI)
    {
        if (g_bootBootMode & BOOT_MODE_SPI_PIN_NAND)
        {
            HAL_ASSERT((g_bootBootMode & BOOT_MODE_BOOT_EMMC),
                "Emmc boot pin should be pulled up to boot on SPI flash over NAND pins");
            spiFlashNand = TRUE;
        }
        else
        {
            spiFlashCam = TRUE;
        }
    }

    //UINT32 metalId = rda_metal_id_get();
    UINT32 emmcBootModeMask;
    if (1) //metalId < 0x02)
    {
        emmcBootModeMask = BOOT_MODE_BOOT_EMMC
                         | BOOT_MODE_BOOT_SPI
                         | BOOT_MODE_BOOT_SPI_NAND;
    }
    else
    {
        emmcBootModeMask = BOOT_MODE_BOOT_EMMC
                         | BOOT_MODE_BOOT_SPI;
    }
    if ((g_bootBootMode & emmcBootModeMask) == BOOT_MODE_BOOT_EMMC)
    {
        // Boot from EMMC
        HAL_ASSERT(halCfg->sdmmcCfg.sdmmc3Used, "SDMMC3 (EMMC) should be used");
    }

    if (1) //metalId < 0x02)
    {
        if ( (g_bootBootMode & (BOOT_MODE_BOOT_SPI|BOOT_MODE_BOOT_SPI_NAND))
             == BOOT_MODE_BOOT_SPI_NAND ||
             (g_bootBootMode & (BOOT_MODE_BOOT_SPI|BOOT_MODE_NAND_PAGE_SIZE_L))
             == (BOOT_MODE_BOOT_SPI|BOOT_MODE_NAND_PAGE_SIZE_L)
           )
        {
            // Boot from SDMMC1
            HAL_ASSERT(halCfg->sdmmcCfg.sdmmcUsed, "SDMMC1 should be used");
        }
    }

    // For convenience, we do not consider spiFlashCam at present.
    // We assume that SPI flash always uses parallel nand pins.
#if 0
    BOOL parallelNandUsed = halCfg->parallelNandUsed;
#else
    BOOL parallelNandUsed = !spiFlashNand && !halCfg->sdmmcCfg.sdmmc3Used;
#endif
    BOOL nand8_15Cam = FALSE;
    BOOL nand8_15Lcd = FALSE;
    if ((g_bootBootMode & (BOOT_MODE_BOOT_SPI|BOOT_MODE_BOOT_EMMC)) == 0)
    {
        HAL_ASSERT(parallelNandUsed, "Parallel NAND should be used");
    }
    if (parallelNandUsed)
    {
        HAL_ASSERT(!spiFlashNand, "SPI flash uses parallel NAND pins");
        HAL_ASSERT(!halCfg->sdmmcCfg.sdmmc3Used,
            "SDMMC3 (EMMC) uses parallel NAND pins");
        if ((g_bootBootMode & BOOT_MODE_NAND_8BIT) == 0)
        {
            if (g_bootBootMode & BOOT_MODE_NAND_HIGH_PIN_CAM)
            {
                HAL_ASSERT(!spiFlashCam, "SPI flash uses NAND 8-15 bits on cam pins");
                nand8_15Cam = TRUE;
            }
            else
            {
                nand8_15Lcd = TRUE;
            }
        }
    }

    if (!parallelNandUsed)
    {
        availableGpio_D |= HAL_GPIO_BIT(9);

#ifdef NAND_IO_RECONFIG_WORKROUND
        if (spiFlashNand)
        {/*GPIO_B:25,26,27,28,29; GPIO_D:4  the output level is inverted*/
            availableGpio_B |= HAL_GPIO_BIT(25)
                             | HAL_GPIO_BIT(26)
                             | HAL_GPIO_BIT(27)
                             | HAL_GPIO_BIT(28)
                             | HAL_GPIO_BIT(29);
            availableGpio_D |= HAL_GPIO_BIT(4);
        }else{
	     availableGpio_D |= HAL_GPIO_BIT(8);
	}
#else
        if (!spiFlashNand)
        {
            availableGpio_B |= HAL_GPIO_BIT(25)
                             | HAL_GPIO_BIT(26)
                             | HAL_GPIO_BIT(27)
                             | HAL_GPIO_BIT(28)
                             | HAL_GPIO_BIT(29);
            availableGpio_D |= HAL_GPIO_BIT(4)
                             | HAL_GPIO_BIT(8);
        }
#endif

        if (!halCfg->sdmmcCfg.sdmmc3Used)
        {
            availableGpio_D |= HAL_GPIO_BIT(10);
        }
    	}
#ifdef NAND_IO_RECONFIG_WORKROUND
	else
	{/*GPIO_B:25,26,27,28,29; GPIO_D:4  the output level is inverted*/
            availableGpio_B |= HAL_GPIO_BIT(25)
                             | HAL_GPIO_BIT(26)
                             | HAL_GPIO_BIT(27)
                             | HAL_GPIO_BIT(28)
                             | HAL_GPIO_BIT(29);
            availableGpio_D |= HAL_GPIO_BIT(4);
        }
#endif

    CONST HAL_CFG_CAM_T *camCfg = &halCfg->camCfg;
    CONST HAL_CFG_CAM_T *cam2Cfg = &halCfg->cam2Cfg;
    CONST HAL_LCD_MODE_T lcdMode = halCfg->goudaCfg.lcdMode;

    // LCD
    if (lcdMode == HAL_LCD_MODE_PARALLEL_16BIT)
    {
        /* Check LCD 8-15 bits */
        HAL_ASSERT(!nand8_15Lcd, "NAND uses LCD 8-15 bits");
        HAL_ASSERT(!halCfg->goudaCfg.lcd8_23Cam2, "Parallel LCD 8-5 bits should use NAND pins only");
        altMux |= CFG_REGS_LCD_MODE_PARALLEL_16BIT;
    }
    else if (lcdMode == HAL_LCD_MODE_DSI)
    {
        altMux |= CFG_REGS_LCD_MODE_DSI;
        availableGpio_A |= HAL_GPIO_BIT(22)
                         | HAL_GPIO_BIT(23);
    }
    else if (lcdMode == HAL_LCD_MODE_RGB_16BIT ||
             lcdMode == HAL_LCD_MODE_RGB_18BIT ||
             lcdMode == HAL_LCD_MODE_RGB_24BIT)
    {
        /* Check LCD 8-15 bits */
        if (halCfg->goudaCfg.lcd8_23Cam2)
        {
            HAL_ASSERT(!(cam2Cfg->camUsed || cam2Cfg->cam1Used),
                "Camera2 uses LCD 8-15 bits");
            altMux2 |= CFG_REGS_RGB_CAM_2_ENABLE;
        }
        else
        {
            HAL_ASSERT(!nand8_15Lcd, "NAND uses LCD 8-15 bits");
        }
        if (lcdMode == HAL_LCD_MODE_RGB_18BIT ||
            lcdMode == HAL_LCD_MODE_RGB_24BIT)
        {
            /* Check LCD 16-23 bits */
            if (lcdMode == HAL_LCD_MODE_RGB_18BIT && halCfg->goudaCfg.lcd16_17Cs)
            {
                HAL_ASSERT(!halCfg->goudaCfg.lcd16_23Cam,
                    "LCD 16-17 bits have been configured on camera pins");
                HAL_ASSERT(!(halCfg->goudaCfg.cs0Used || halCfg->goudaCfg.cs1Used),
                    "LCD CS 0/1 use LCD 16-17 bits");
                altMux2 |= CFG_REGS_LCD_RGB_17_16_LCD_DATA;
            }
            else if (halCfg->goudaCfg.lcd8_23Cam2)
            {
                HAL_ASSERT(!halCfg->goudaCfg.lcd16_23Cam,
                    "LCD 16-23 bits have been configured on camera pins");
                HAL_ASSERT(halCfg->goudaCfg.lcd16_17Cs,
                    "LCD 16-17 bits should use LCD CS 0/1");
                HAL_ASSERT(!(halCfg->goudaCfg.cs0Used || halCfg->goudaCfg.cs1Used),
                    "LCD CS 0/1 use LCD 16-17 bits");
                HAL_ASSERT(!(cam2Cfg->camUsed || cam2Cfg->cam1Used),
                    "Camera2 uses LCD 18-23 bits");
                altMux2 |= CFG_REGS_LCD_RGB_17_16_LCD_DATA;
                altMux2 |= CFG_REGS_RGB_CAM_2_ENABLE;
            }
            else if (halCfg->goudaCfg.lcd16_23Cam)
            {
                HAL_ASSERT(!(halCfg->goudaCfg.lcd16_17Cs || halCfg->goudaCfg.lcd8_23Cam2),
                    "LCD 16-23 bits have been configured on LCD CS 0/1 and camera2 pins");
                HAL_ASSERT(
                    !(camCfg->camMode == HAL_CAM_MODE_PARALLEL &&
                      (camCfg->camUsed || camCfg->cam1Used)),
                    "Parallel camera uses LCD 16-23 bits");
                HAL_ASSERT(
                    !(halCfg->i2cCfg.i2c2Used && halCfg->i2cCfg.i2c2PinsCam),
                    "I2C2 uses LCD 22-23 bits");
                HAL_ASSERT(!spiFlashCam, "SPI flash uses LCD 16-21 bits");
                HAL_ASSERT(!nand8_15Cam, "NAND uses LCD 16-23 bits");
                HAL_ASSERT(
                    !(camCfg->camMode == HAL_CAM_MODE_CSI &&
                      ((camCfg->camUsed && camCfg->camCsiId == HAL_CAM_CSI_1) ||
                       (camCfg->cam1Used && camCfg->cam1CsiId == HAL_CAM_CSI_1))),
                    "Camera CSI1 uses LCD 16-21 bits");
                altMux |= CFG_REGS_RGB_CAM_ENABLE;
            }
            else
            {
                HAL_ASSERT(!parallelNandUsed, "NAND uses LCD 16-23 bits");
                HAL_ASSERT(!halCfg->sdmmcCfg.sdmmc3Used, "SDMMC3 uses LCD 16-23 bits");
                altMux |= CFG_REGS_RGB_CAM_DISABLE;
            }
        }
        altMux |= CFG_REGS_LCD_MODE_RGB_24BIT;
    }
    else if (lcdMode == HAL_LCD_MODE_SPI)
    {
        HAL_ASSERT(!(halCfg->goudaCfg.cs0Used || halCfg->goudaCfg.cs1Used),
            "LCD CS 0/1 use SPI LCD pins");
        HAL_ASSERT((halCfg->usedGpo_A & (HAL_GPO_BIT(3)|HAL_GPO_BIT(4))) == 0,
            "GPO 3/4 use SPI LCD pins");
        altMux |= CFG_REGS_SPI_LCD_SPI_LCD;
        availableGpio_A |= HAL_GPIO_BIT(18)
                         | HAL_GPIO_BIT(19)
                         | HAL_GPIO_BIT(20)
                         | HAL_GPIO_BIT(21);
    }
    else
    {
        HAL_ASSERT(FALSE, "Lcd mode not defined!");
    }

    if (lcdMode == HAL_LCD_MODE_DSI ||
        lcdMode == HAL_LCD_MODE_SPI ||
        ((lcdMode == HAL_LCD_MODE_RGB_16BIT ||
          lcdMode == HAL_LCD_MODE_RGB_18BIT ||
          lcdMode == HAL_LCD_MODE_RGB_24BIT) &&
         halCfg->goudaCfg.lcd8_23Cam2))
    {
        if (!nand8_15Lcd)
        {
            availableGpio_A |= HAL_GPIO_BIT(24)
                             | HAL_GPIO_BIT(25)
                             | HAL_GPIO_BIT(26)
                             | HAL_GPIO_BIT(27)
                             | HAL_GPIO_BIT(28)
                             | HAL_GPIO_BIT(29)
                             | HAL_GPIO_BIT(30)
                             | HAL_GPIO_BIT(31);
        }
    }

    if (halCfg->goudaCfg.cs0Used)
    {
        altMux |= CFG_REGS_GPO_4_LCD_CS_0;
    }
    else
    {
        altMux |= CFG_REGS_GPO_4_GPO_4;
        availableGpo_A |= HAL_GPO_BIT(4);
    }

    if (halCfg->goudaCfg.cs1Used)
    {
        altMux |= CFG_REGS_GPO_3_LCD_CS_1;
    }
    else
    {
        altMux |= CFG_REGS_GPO_3_GPO_3;
        availableGpo_A |= HAL_GPO_BIT(3);
    }

    // I2C
    if (!halCfg->i2cCfg.i2cUsed)
    {
        availableGpio_B |= HAL_GPIO_BIT(30)
                         | HAL_GPIO_BIT(31);
    }

    if (halCfg->i2cCfg.i2c2Used)
    {
        if (halCfg->i2cCfg.i2c2PinsCam)
        {
            if (camCfg->camUsed)
            {
                HAL_ASSERT(camCfg->camPdnRemap == GPIO_NONE &&
                    camCfg->camRstRemap == GPIO_NONE,
                    "Cam uses PDN/RST pins");
            }
            if (camCfg->cam1Used)
            {
                HAL_ASSERT(camCfg->cam1PdnRemap == GPIO_NONE &&
                    camCfg->cam1RstRemap == GPIO_NONE,
                    "Cam1 uses PDN/RST pins");
            }
            HAL_ASSERT(!nand8_15Cam, "NAND uses cam PDN/RST pins");
            availableGpio_A |= HAL_GPIO_BIT(0)
                             | HAL_GPIO_BIT(1);
            altMux |= CFG_REGS_CAM_I2C2_I2C2;
        }
        else
        {
            // I2C2 does not use cam pins
            altMux |= CFG_REGS_CAM_I2C2_CAM;
        }
    }
    else
    {
        // I2C2 does not use cam pins
        altMux |= CFG_REGS_CAM_I2C2_CAM;
        availableGpio_A |= HAL_GPIO_BIT(0)
                         | HAL_GPIO_BIT(1);
    }

    if (halCfg->i2cCfg.i2c3Used)
    {
        HAL_ASSERT((halCfg->keyOutMask & 0x18) == 0, "Keyout 3/4 use I2C3 pins");
        altMux |= CFG_REGS_KEYOUT_3_4_I2C3;
    }
    else
    {
        altMux |= CFG_REGS_KEYOUT_3_4_KEYOUT_3_4;
        if ((halCfg->keyOutMask & 0x8) == 0)
        {
            availableGpio_B |= HAL_GPIO_BIT(6);
        }
        if ((halCfg->keyOutMask & 0x10) == 0)
        {
            availableGpio_B |= HAL_GPIO_BIT(7);
        }
    }

    if (halCfg->i2cCfg.modemI2cUsed)
    {
        HAL_ASSERT(!halCfg->emacUsed, "EMAC uses modem I2C pins");
    }
    else
    {
        if (!halCfg->emacUsed)
        {
            availableGpio_E |= HAL_GPIO_BIT(0)
                             | HAL_GPIO_BIT(1);
        }
    }

    // Camera
    if (camCfg->camUsed || camCfg->cam1Used)
    {
        if(camCfg->camMode == HAL_CAM_MODE_PARALLEL)
        {
            HAL_ASSERT(!spiFlashCam, "SPI flash uses cam pins");
            HAL_ASSERT(!nand8_15Cam, "NAND 8-15 bits use cam pins");
            altMux |= CFG_REGS_CSI2_PARALLEL_CAM;
            availableGpio_B |= HAL_GPIO_BIT(24);
        }
        else if (camCfg->camMode == HAL_CAM_MODE_SPI)
        {
            altMux |= CFG_REGS_CSI2_SPI_CAM;
            availableGpio_B |= HAL_GPIO_BIT(14);
            if (!(spiFlashCam || nand8_15Cam ||
                  (lcdMode == HAL_LCD_MODE_RGB_24BIT &&
                   halCfg->goudaCfg.lcd16_23Cam)
                 )
               )
            {
                availableGpio_B |= HAL_GPIO_BIT(13)
                                 | HAL_GPIO_BIT(20)
                                 | HAL_GPIO_BIT(21)
                                 | HAL_GPIO_BIT(22)
                                 | HAL_GPIO_BIT(23)
                                 | HAL_GPIO_BIT(24);
            }
        }
        else if (camCfg->camMode == HAL_CAM_MODE_CSI)
        {
            if ((camCfg->camUsed && camCfg->camCsiId == HAL_CAM_CSI_1) ||
                (camCfg->cam1Used && camCfg->cam1CsiId == HAL_CAM_CSI_1))
            {
                HAL_ASSERT(!spiFlashCam, "SPI flash uses CSI1 pins");
                HAL_ASSERT(!nand8_15Cam, "NAND 8-15 bits use CSI1 pins");
            }
            else
            {
                // CSI1 is unused
                if (!(spiFlashCam || nand8_15Cam ||
                      (lcdMode == HAL_LCD_MODE_RGB_24BIT &&
                       halCfg->goudaCfg.lcd16_23Cam)
                     )
                   )
                {
                    availableGpio_B |= HAL_GPIO_BIT(13)
                                     | HAL_GPIO_BIT(20)
                                     | HAL_GPIO_BIT(21)
                                     | HAL_GPIO_BIT(22)
                                     | HAL_GPIO_BIT(23)
                                     | HAL_GPIO_BIT(24);
                }
            }
            altMux |= CFG_REGS_CSI2_CSI2;
            // If CSI2 is unused
            if (!((camCfg->camUsed && camCfg->camCsiId == HAL_CAM_CSI_2) ||
                  (camCfg->cam1Used && camCfg->cam1CsiId == HAL_CAM_CSI_2)))
            {
                availableGpio_B |= HAL_GPIO_BIT(14)
                                 | HAL_GPIO_BIT(15)
                                 | HAL_GPIO_BIT(16)
                                 | HAL_GPIO_BIT(17)
                                 | HAL_GPIO_BIT(18)
                                 | HAL_GPIO_BIT(19);
            }
        }
        else
        {
            HAL_ASSERT(FALSE, "Invalid cam mode: %d", camCfg->camMode);
        }

        // Cam PDN/RST pins
        if (!(nand8_15Cam ||
              (lcdMode == HAL_LCD_MODE_RGB_24BIT && halCfg->goudaCfg.lcd16_23Cam) ||
              (halCfg->i2cCfg.i2c2Used && halCfg->i2cCfg.i2c2PinsCam)
             )
           )
        {
            if (!((camCfg->camUsed && camCfg->camRstRemap == GPIO_NONE) ||
                  (camCfg->cam1Used && camCfg->cam1RstRemap == GPIO_NONE)))
            {
                availableGpio_B |= HAL_GPIO_BIT(10);
            }
            if (!((camCfg->camUsed && camCfg->camPdnRemap == GPIO_NONE) ||
                  (camCfg->cam1Used && camCfg->cam1PdnRemap == GPIO_NONE)))
            {
                availableGpio_B |= HAL_GPIO_BIT(11);
            }
        }
    }
    else // cam is unused
    {
        availableGpio_B |= HAL_GPIO_BIT(12)
                         | HAL_GPIO_BIT(14)
                         | HAL_GPIO_BIT(15)
                         | HAL_GPIO_BIT(16)
                         | HAL_GPIO_BIT(17)
                         | HAL_GPIO_BIT(18)
                         | HAL_GPIO_BIT(19);
        if (!(nand8_15Cam ||
              (lcdMode == HAL_LCD_MODE_RGB_24BIT && halCfg->goudaCfg.lcd16_23Cam) ||
              (halCfg->i2cCfg.i2c2Used && halCfg->i2cCfg.i2c2PinsCam)
             )
           )
        {
            availableGpio_B |= HAL_GPIO_BIT(10)
                             | HAL_GPIO_BIT(11);
        }
        if (!(spiFlashCam || nand8_15Cam ||
              (lcdMode == HAL_LCD_MODE_RGB_24BIT && halCfg->goudaCfg.lcd16_23Cam)
             )
           )
        {
            availableGpio_B |= HAL_GPIO_BIT(13)
                             | HAL_GPIO_BIT(20)
                             | HAL_GPIO_BIT(21)
                             | HAL_GPIO_BIT(22)
                             | HAL_GPIO_BIT(23)
                             | HAL_GPIO_BIT(24);
        }
    }

    // Camera2
    if (cam2Cfg->camUsed || cam2Cfg->cam1Used)
    {
        if(cam2Cfg->camMode == HAL_CAM_MODE_PARALLEL)
        {
            altMux2 |= CFG_REGS_CSI2_2_PARALLEL_CAM;
        }
        else if (cam2Cfg->camMode == HAL_CAM_MODE_SPI)
        {
            altMux2 |= CFG_REGS_CSI2_2_SPI_CAM;
            availableGpio_E |= HAL_GPIO_BIT(4)
                             | HAL_GPIO_BIT(5)
                             | HAL_GPIO_BIT(6)
                             | HAL_GPIO_BIT(12)
                             | HAL_GPIO_BIT(13)
                             | HAL_GPIO_BIT(14)
                             | HAL_GPIO_BIT(15);
        }
        else if (cam2Cfg->camMode == HAL_CAM_MODE_CSI)
        {
            if ((cam2Cfg->camUsed && (cam2Cfg->camCsiId == HAL_CAM_CSI_1 ||
                                      cam2Cfg->camCsiId == HAL_CAM_CSI_2)) ||
                (cam2Cfg->cam1Used && (cam2Cfg->cam1CsiId == HAL_CAM_CSI_1 ||
                                       cam2Cfg->cam1CsiId == HAL_CAM_CSI_2)))
            {
                altMux2 |= CFG_REGS_CSI2_2_CSI2;
            }
            else
            {
                availableGpio_E |= HAL_GPIO_BIT(4)
                                 | HAL_GPIO_BIT(5)
                                 | HAL_GPIO_BIT(6)
                                 | HAL_GPIO_BIT(7)
                                 | HAL_GPIO_BIT(8)
                                 | HAL_GPIO_BIT(9)
                                 | HAL_GPIO_BIT(10)
                                 | HAL_GPIO_BIT(11)
                                 | HAL_GPIO_BIT(12)
                                 | HAL_GPIO_BIT(13)
                                 | HAL_GPIO_BIT(14)
                                 | HAL_GPIO_BIT(15);
            }
        }
        else
        {
            HAL_ASSERT(FALSE, "Invalid cam2 mode: %d", cam2Cfg->camMode);
        }

        // Cam2 PDN/RST pins
        if (!((cam2Cfg->camUsed && cam2Cfg->camRstRemap == GPIO_NONE) ||
              (cam2Cfg->cam1Used && cam2Cfg->cam1RstRemap == GPIO_NONE)))
        {
            availableGpio_E |= HAL_GPIO_BIT(2);
        }
        if (!((cam2Cfg->camUsed && cam2Cfg->camPdnRemap == GPIO_NONE) ||
              (cam2Cfg->cam1Used && cam2Cfg->cam1PdnRemap == GPIO_NONE) ||
              (altMux2 & CFG_REGS_CSI2_2_CSI2)))
        {
            availableGpio_E |= HAL_GPIO_BIT(3);
        }
    }
    else // cam2 is unused
    {
        if (!((lcdMode == HAL_LCD_MODE_RGB_16BIT ||
               lcdMode == HAL_LCD_MODE_RGB_18BIT ||
               lcdMode == HAL_LCD_MODE_RGB_24BIT) &&
              halCfg->goudaCfg.lcd8_23Cam2))
        {
                availableGpio_E |= HAL_GPIO_BIT(2)
                                 | HAL_GPIO_BIT(3)
                                 | HAL_GPIO_BIT(4)
                                 | HAL_GPIO_BIT(5)
                                 | HAL_GPIO_BIT(6)
                                 | HAL_GPIO_BIT(7)
                                 | HAL_GPIO_BIT(8)
                                 | HAL_GPIO_BIT(9)
                                 | HAL_GPIO_BIT(10)
                                 | HAL_GPIO_BIT(11)
                                 | HAL_GPIO_BIT(12)
                                 | HAL_GPIO_BIT(13)
                                 | HAL_GPIO_BIT(14)
                                 | HAL_GPIO_BIT(15);
        }
    }

    // UART 1 Pin configuration
    switch (halCfg->uartCfg[0])
    {
        case HAL_UART_CONFIG_NONE:
            altMux |= CFG_REGS_KEYOUT_7_KEYOUT_7;
            availableGpio_C |= HAL_GPIO_BIT(6);
            availableGpio_A |= HAL_GPIO_BIT(14);
            if ((halCfg->keyInMask & 0x80) == 0)
            {
                availableGpio_A |= HAL_GPIO_BIT(15);
            }
            if ((halCfg->keyInMask & 0x80) == 0)
            {
                availableGpio_A |= HAL_GPIO_BIT(16);
            }
            break;
        case HAL_UART_CONFIG_DATA:
             // use UART1 TXD, RXD
            altMux |= CFG_REGS_KEYOUT_7_KEYOUT_7;
            if ((halCfg->keyInMask & 0x80) == 0)
            {
                availableGpio_A |= HAL_GPIO_BIT(15);
            }
            if ((halCfg->keyInMask & 0x80) == 0)
            {
                availableGpio_A |= HAL_GPIO_BIT(16);
            }
            break;
        case HAL_UART_CONFIG_FLOWCONTROL:
             // use UART1 TXD, RXD, CTS, RTS
            HAL_ASSERT((halCfg->keyInMask & 0x80) == 0, "Keyin 7 uses UART1 CTS");
            HAL_ASSERT((halCfg->keyOutMask & 0x80) == 0, "Keyout 7 uses UART1 RTS");
            altMux |= CFG_REGS_KEYOUT_7_UART1_RTS;
            break;
        case HAL_UART_CONFIG_MODEM:
            // use UART1 TXD, RXD, CTS, RTS, RI, DSR, DCD, DTR
            HAL_ASSERT((halCfg->keyInMask & 0x80) == 0, "Keyin 7 uses UART1 CTS");
            HAL_ASSERT((halCfg->keyOutMask & 0x80) == 0, "Keyout 7 uses UART1 RTS");
            HAL_ASSERT(!halCfg->hostUartUsed, "Host UART uses UART1 DCD/DTR");
            HAL_ASSERT(halCfg->uartCfg[1] == HAL_UART_CONFIG_NONE,
                    "UART 2 must be unused to use UART1 Modem lines.");
            HAL_ASSERT((halCfg->keyInMask & 0x40) == 0, "Keyin 6 uses UART1 DSR");
            HAL_ASSERT((halCfg->keyOutMask & 0x40) == 0, "Keyout 6 uses UART1 RI");
            altMux |= CFG_REGS_KEYOUT_7_UART1_RTS
                    | CFG_REGS_UART1_8LINE_UART1_8_LINE;
            break;
        default:
            HAL_ASSERT(FALSE,
                    "Invalid Uart 1 Configuration (%d).",
                    halCfg->uartCfg[0]);
            break;
    }

    // Host UART
    if (halCfg->hostUartUsed)
    {
        HAL_ASSERT(halCfg->uartCfg[1] == HAL_UART_CONFIG_NONE,
                "UART2 uses host UART RXD/TXD.");
    }

    // UART 2 Pin configuration
    switch (halCfg->uartCfg[1])
    {
        case HAL_UART_CONFIG_NONE:
            altMux |= CFG_REGS_UART2_HOST_UART
                    | CFG_REGS_KEYOUT_6_KEYOUT_6;
            if (halCfg->uartCfg[0] != HAL_UART_CONFIG_MODEM)
            {
                availableGpio_C |= HAL_GPIO_BIT(7)
                                 | HAL_GPIO_BIT(8);
            }
            if ((halCfg->keyInMask & 0x40) == 0)
            {
                availableGpio_B |= HAL_GPIO_BIT(8);
            }
            if ((halCfg->keyOutMask & 0x40) == 0)
            {
                availableGpio_B |= HAL_GPIO_BIT(9);
            }
            break;
        case HAL_UART_CONFIG_DATA:
            // use UART2 TXD, RXD
            altMux |= CFG_REGS_UART2_UART2
                    | CFG_REGS_KEYOUT_6_KEYOUT_6;
            if ((halCfg->keyInMask & 0x40) == 0)
            {
                availableGpio_B |= HAL_GPIO_BIT(8);
            }
            if ((halCfg->keyOutMask & 0x40) == 0)
            {
                availableGpio_B |= HAL_GPIO_BIT(9);
            }
            break;
        case HAL_UART_CONFIG_FLOWCONTROL:
             // use UART2 TXD, RXD, CTS, RTS
            HAL_ASSERT((halCfg->keyInMask & 0x40) == 0, "Keyin 6 uses UART2 CTS");
            HAL_ASSERT((halCfg->keyOutMask & 0x40) == 0, "Keyout 6 uses UART2 RTS");
            altMux |= CFG_REGS_UART2_UART2
                    | CFG_REGS_KEYOUT_6_UART2_RTS;
            break;
        case HAL_UART_CONFIG_MODEM:
        default:
            HAL_ASSERT(FALSE,
                    "Invalid Uart2 Configuration (%d).",
                    halCfg->uartCfg[1]);
            break;
    }

    // UART 3 Pin configuration
    switch (halCfg->uartCfg[2])
    {
        case HAL_UART_CONFIG_NONE:
            availableGpio_D |= HAL_GPIO_BIT(0)
                             | HAL_GPIO_BIT(1)
                             | HAL_GPIO_BIT(2)
                             | HAL_GPIO_BIT(3);
            break;
        case HAL_UART_CONFIG_DATA:
            // use UART3 TXD, RXD
            availableGpio_D |= HAL_GPIO_BIT(2)
                             | HAL_GPIO_BIT(3);
            break;
        case HAL_UART_CONFIG_FLOWCONTROL:
             // use UART3 TXD, RXD, CTS, RTS
            break;
        case HAL_UART_CONFIG_MODEM:
        default:
            HAL_ASSERT(FALSE,
                    "Invalid Uart3 Configuration (%d).",
                    halCfg->uartCfg[2]);
            break;
    }

    // I2S
    altMux |= CFG_REGS_DAI_I2S;
    if (halCfg->i2sCfg.di0Used || halCfg->i2sCfg.di1Used ||
        halCfg->i2sCfg.di2Used || halCfg->i2sCfg.doUsed)
    {
        if (!halCfg->i2sCfg.di0Used)
        {
            availableGpio_A |= HAL_GPIO_BIT(11);
        }
        if (!halCfg->i2sCfg.di1Used)
        {
            availableGpio_A |= HAL_GPIO_BIT(12);
        }
        if (halCfg->i2sCfg.di2Used)
        {
            HAL_ASSERT((halCfg->keyInMask & 0x4) == 0, "Keyin 2 uses I2S DI2");
            altMux |= CFG_REGS_I2S_DI_2_I2S_DI_2;
        }
        else
        {
            altMux |= CFG_REGS_I2S_DI_2_KEYIN_2;
            if ((halCfg->keyInMask & 0x4) == 0)
            {
                availableGpio_B |= HAL_GPIO_BIT(2);
            }
        }
        if (!halCfg->i2sCfg.doUsed)
        {
            availableGpio_A |= HAL_GPIO_BIT(13);
        }
    }
    else
    {
        availableGpio_A |= HAL_GPIO_BIT(9)
                         | HAL_GPIO_BIT(10)
                         | HAL_GPIO_BIT(11)
                         | HAL_GPIO_BIT(12)
                         | HAL_GPIO_BIT(13);
    }

    // I2S2
    if (halCfg->i2s2Cfg.di0Used || halCfg->i2s2Cfg.di1Used ||
        halCfg->i2s2Cfg.di2Used || halCfg->i2s2Cfg.doUsed)
    {
        HAL_ASSERT(!halCfg->emacUsed, "EMAC uses I2S2 pins");
        if (!halCfg->i2s2Cfg.di0Used)
        {
            availableGpio_D |= HAL_GPIO_BIT(13);
        }
        if (!halCfg->i2s2Cfg.di1Used)
        {
            availableGpio_D |= HAL_GPIO_BIT(14);
        }
        if (!halCfg->i2s2Cfg.doUsed)
        {
            availableGpio_D |= HAL_GPIO_BIT(15);
        }
    }
    else
    {
        availableGpio_D |= HAL_GPIO_BIT(11)
                         | HAL_GPIO_BIT(12)
                         | HAL_GPIO_BIT(13)
                         | HAL_GPIO_BIT(14)
                         | HAL_GPIO_BIT(15);
    }

    // LPS CO1
    if (halCfg->useLpsCo1)
    {
        HAL_ASSERT((halCfg->keyInMask & 0x10) == 0, "Keyin 4 uses LPS_CO1");
        altMux |= CFG_REGS_LPSCO_1_LPSCO_1;
    }
    else
    {
        altMux |= CFG_REGS_LPSCO_1_KEYIN_4;
        if ((halCfg->keyInMask & 0x10) == 0)
        {
            availableGpio_A |= HAL_GPIO_BIT(7);
        }
    }

    // TCO
    if (halCfg->usedTco & 0x1)
    {
        HAL_ASSERT((halCfg->keyOutMask & 0x1) == 0, "Keyout 0 uses TCO0");
        altMux |= CFG_REGS_TCO_0_TCO_0;
    }
    else
    {
        altMux |= CFG_REGS_TCO_0_KEYOUT_0;
        if ((halCfg->keyOutMask & 0x1) == 0)
        {
            availableGpio_B |= HAL_GPIO_BIT(3);
        }
    }

    if (halCfg->usedTco & 0x2)
    {
        HAL_ASSERT((halCfg->keyOutMask & 0x2) == 0, "Keyout 1 uses TCO1");
        altMux |= CFG_REGS_TCO_1_TCO_1;
    }
    else
    {
        altMux |= CFG_REGS_TCO_1_KEYOUT_1;
        if ((halCfg->keyOutMask & 0x2) == 0)
        {
            availableGpio_B |= HAL_GPIO_BIT(4);
        }
    }

    if (halCfg->usedTco & 0x4)
    {
        HAL_ASSERT((halCfg->keyOutMask & 0x4) == 0, "Keyout 2 uses TCO2");
        altMux |= CFG_REGS_TCO_2_TCO_2;
    }
    else
    {
        altMux |= CFG_REGS_TCO_2_KEYOUT_2;
        if ((halCfg->keyOutMask & 0x4) == 0)
        {
            availableGpio_B |= HAL_GPIO_BIT(5);
        }
    }

    // PWM
    if (halCfg->pwmCfg.pwtUsed)
    {
        HAL_ASSERT((halCfg->keyInMask & 0x20) == 0, "Keyin 5 uses PWT");
        altMux |= CFG_REGS_GPO_0_PWT;
    }
    else if (halCfg->keyInMask & 0x20)
    {
        altMux |= CFG_REGS_GPO_0_KEYIN_5;
    }
    else
    {
        altMux |= CFG_REGS_GPO_0_GPO_0;
        availableGpo_A |= HAL_GPO_BIT(0);
    }

    if (halCfg->pwmCfg.lpgUsed)
    {
        HAL_ASSERT((halCfg->keyOutMask & 0x20) == 0, "Keyout 5 uses LPG");
        altMux |= CFG_REGS_GPO_1_LPG;
    }
    else if (halCfg->keyOutMask & 0x20)
    {
        altMux |= CFG_REGS_GPO_1_KEYOUT_5;
    }
    else
    {
        altMux |= CFG_REGS_GPO_1_GPO_1;
        availableGpo_A |= HAL_GPO_BIT(1);
    }

    if (halCfg->pwmCfg.pwl1Used)
    {
        HAL_ASSERT(!halCfg->useClk32k, "Clk 32k uses PWL1");
        altMux |= CFG_REGS_GPO_2_PWL_1;
    }
    else if (halCfg->useClk32k)
    {
        // Default to GPO low output
        // 32K clock will be enabled per request
        //altMux |= CFG_REGS_GPO_2_CLK_32K;
        altMux |= CFG_REGS_GPO_2_GPO_2;
    }
    else
    {
        altMux |= CFG_REGS_GPO_2_GPO_2;
        availableGpo_A |= HAL_GPO_BIT(2);
    }

    // Clock out and host clock
    if (halCfg->clkOutUsed)
    {
        HAL_ASSERT(!halCfg->hostClkUsed, "HST_CLK uses CLK_OUT pin");
        altMux |= CFG_REGS_CLK_OUT_CLK_OUT;
    }
    else if (halCfg->hostClkUsed)
    {
        altMux |= CFG_REGS_CLK_OUT_HST_CLK;
    }
    else
    {
        altMux |= CFG_REGS_CLK_OUT_HST_CLK;
        availableGpo_A |= HAL_GPO_BIT(8);
    }

    // SPI
    if (halCfg->spiCfg[0].cs0Used || halCfg->spiCfg[0].cs1Used ||
        halCfg->spiCfg[0].cs2Used)
    {
        HAL_ASSERT(!(halCfg->modemSpiCfg[0].cs0Used ||
                     halCfg->modemSpiCfg[0].cs1Used ||
                     halCfg->modemSpiCfg[0].cs2Used), "Modem SPI1 is in use");
        altMux |= CFG_REGS_AP_SPI1_AP_SPI1;
        if (!halCfg->spiCfg[0].cs0Used)
        {
            availableGpio_C |= HAL_GPIO_BIT(22);
        }
        if (!halCfg->spiCfg[0].cs1Used)
        {
            availableGpio_A |= HAL_GPIO_BIT(17);
        }
        if (halCfg->spiCfg[0].cs2Used)
        {
            HAL_ASSERT((halCfg->keyInMask & 0x2) == 0, "Keyin 1 uses SPI1 CS2");
            altMux |= CFG_REGS_SPI1_CS_2_SPI1_CS_2;
        }
        else
        {
            altMux |= CFG_REGS_SPI1_CS_2_KEYIN_1;
            if ((halCfg->keyInMask & 0x2) == 0)
            {
                availableGpio_B |= HAL_GPIO_BIT(1);
            }
        }
        if (!halCfg->spiCfg[0].di0Used)
        {
            availableGpio_C |= HAL_GPIO_BIT(23);
        }
        if (!halCfg->spiCfg[0].di1Used)
        {
            availableGpio_C |= HAL_GPIO_BIT(24);
        }
    }
    else // AP SPI1 unused
    {
        altMux |= CFG_REGS_AP_SPI1_BB_SPI1;
        if (halCfg->modemSpiCfg[0].cs0Used || halCfg->modemSpiCfg[0].cs1Used ||
            halCfg->modemSpiCfg[0].cs2Used)
        {
            if (!halCfg->modemSpiCfg[0].cs0Used)
            {
                availableGpio_C |= HAL_GPIO_BIT(22);
            }
            if (!halCfg->modemSpiCfg[0].cs1Used)
            {
                availableGpio_A |= HAL_GPIO_BIT(17);
            }
            if (!halCfg->modemSpiCfg[0].di0Used)
            {
                availableGpio_C |= HAL_GPIO_BIT(23);
            }
            if (!halCfg->modemSpiCfg[0].di1Used)
            {
                availableGpio_C |= HAL_GPIO_BIT(24);
            }
        }
        else
        {
            availableGpio_A |= HAL_GPIO_BIT(17);
            availableGpio_C |= HAL_GPIO_BIT(21)
                             | HAL_GPIO_BIT(22)
                             | HAL_GPIO_BIT(23)
                             | HAL_GPIO_BIT(24);
            if ((halCfg->keyInMask & 0x2) == 0)
            {
                availableGpio_B |= HAL_GPIO_BIT(1);
            }
        }
    }
    if (halCfg->spiCfg[1].cs0Used || halCfg->spiCfg[1].cs1Used)
    {
        if (!halCfg->spiCfg[1].cs0Used)
        {
            availableGpio_A |= HAL_GPIO_BIT(5);
        }
        if (halCfg->spiCfg[1].cs1Used)
        {
            HAL_ASSERT((halCfg->keyInMask & 0x8) == 0, "Keyin 3 uses SPI2 CS1");
            altMux |= CFG_REGS_KEYIN_3_SPI2_CS_1;
        }
        else
        {
            altMux |= CFG_REGS_KEYIN_3_KEYIN_3;
            if ((halCfg->keyInMask & 0x8) == 0)
            {
                availableGpio_A |= HAL_GPIO_BIT(6);
            }
        }
        if (!halCfg->spiCfg[1].di0Used)
        {
            availableGpio_A |= HAL_GPIO_BIT(3);
        }
        if (!halCfg->spiCfg[1].di1Used)
        {
            availableGpio_A |= HAL_GPIO_BIT(4);
        }
    }
    else // AP SPI2 unused
    {
        availableGpio_A |= HAL_GPIO_BIT(2)
                         | HAL_GPIO_BIT(3)
                         | HAL_GPIO_BIT(4)
                         | HAL_GPIO_BIT(5);
        if ((halCfg->keyInMask & 0x8) == 0)
        {
            availableGpio_A |= HAL_GPIO_BIT(6);
        }
    }

    // Standalone keyin/out
    if ((halCfg->keyInMask & 0x1) == 0)
    {
        availableGpio_B |= HAL_GPIO_BIT(0);
    }

    // SDMMC
    if (!halCfg->sdmmcCfg.sdmmcUsed)
    {
        availableGpio_C |= HAL_GPIO_BIT(9)
                         | HAL_GPIO_BIT(10)
                         | HAL_GPIO_BIT(11)
                         | HAL_GPIO_BIT(12)
                         | HAL_GPIO_BIT(13)
                         | HAL_GPIO_BIT(14);
    }
    if (!halCfg->sdmmcCfg.sdmmc2Used)
    {
        availableGpio_C |= HAL_GPIO_BIT(15)
                         | HAL_GPIO_BIT(16);
        if (!halCfg->sdio2Uart1Used)
        {
            availableGpio_C |= HAL_GPIO_BIT(17)
                             | HAL_GPIO_BIT(18)
                             | HAL_GPIO_BIT(19)
                             | HAL_GPIO_BIT(20);
        }
    }
    HAL_ASSERT(!(parallelNandUsed && halCfg->sdmmcCfg.sdmmc3Used),
        "Parallel NAND uses SDMMC3 pins");

    // SDIO2_UART1
    if (halCfg->sdio2Uart1Used)
    {
        HAL_ASSERT(halCfg->uartCfg[0] == HAL_UART_CONFIG_NONE,
            "UART1 has been configured on UART1 pins");
        altMux2 |= CFG_REGS_UART1_SDIO2_UART1;
    }
    else
    {
        altMux2 |= CFG_REGS_UART1_SDIO2_SDIO2;
    }

    // EMAC
    if (halCfg->emacUsed)
    {
        altMux2 |= CFG_REGS_MAC_EN_ENABLE;
    }
    else
    {
        altMux2 |= CFG_REGS_MAC_EN_DISABLE;
    }

    // SIM
    if (!hal_BoardSimUsed(2))
    {
        availableGpio_C |= HAL_GPIO_BIT(25)
                         | HAL_GPIO_BIT(26)
                         | HAL_GPIO_BIT(27);
    }

    if (hal_BoardSimUsed(3))
    {
        HAL_ASSERT(!halCfg->emacUsed, "EMAC uses SIM3 pins");
    }
    else
    {
        if (!halCfg->emacUsed)
        {
            availableGpio_C |= HAL_GPIO_BIT(28)
                             | HAL_GPIO_BIT(29)
                             | HAL_GPIO_BIT(30);
        }
    }

    UINT32 gpioMask;
    // GPIO_C mask check
    gpioMask = ~availableGpio_C & halCfg->usedGpio_C;
    HAL_ASSERT(gpioMask == 0,
               "Some used GPIO C are not available (0x%x)",
               gpioMask);
    gpioMask = ~availableGpio_C & halCfg->noConnectGpio_C;
    HAL_ASSERT(gpioMask == 0,
               "Some GPIO C declared as not connected are not available (0x%x)",
               gpioMask);
    gpioMask = halCfg->usedGpio_C & halCfg->noConnectGpio_C;
    HAL_ASSERT(gpioMask == 0,
               "Some GPIO C declared as not connected are used (0x%x)",
               gpioMask);
#ifdef CHECK_GPIO_ALT_FUNC
    gpioMask = availableGpio_C &
               (~halCfg->usedGpio_C & ~halCfg->noConnectGpio_C);
    HAL_ASSERT(gpioMask == 0,
               "Some available GPIO C declared as alt function (0x%x)",
               gpioMask);
#endif

    // GPIO_A mask check
    gpioMask = ~availableGpio_A & halCfg->usedGpio_A;
    HAL_ASSERT(gpioMask == 0,
               "Some used GPIO A are not available (0x%x)",
               gpioMask);
    gpioMask = ~availableGpio_A & halCfg->noConnectGpio_A;
    HAL_ASSERT(gpioMask == 0,
               "Some GPIO A declared as not connected are not available (0x%x)",
               gpioMask);
    gpioMask = halCfg->usedGpio_A & halCfg->noConnectGpio_A;
    HAL_ASSERT(gpioMask == 0,
               "Some GPIO A declared as not connected are used (0x%x)",
               gpioMask);
#ifdef CHECK_GPIO_ALT_FUNC
    gpioMask = availableGpio_A &
               (~halCfg->usedGpio_A & ~halCfg->noConnectGpio_A);
    HAL_ASSERT(gpioMask == 0,
               "Some available GPIO A declared as alt function (0x%x)",
               gpioMask);
#endif

    // GPIO_B mask check
    gpioMask = ~availableGpio_B & halCfg->usedGpio_B;
    HAL_ASSERT(gpioMask == 0,
               "Some used GPIO B are not available (0x%x)",
               gpioMask);
    gpioMask = ~availableGpio_B & halCfg->noConnectGpio_B;
    HAL_ASSERT(gpioMask == 0,
               "Some GPIO B declared as not connected are not available (0x%x)",
               gpioMask);
    gpioMask = halCfg->usedGpio_B & halCfg->noConnectGpio_B;
    HAL_ASSERT(gpioMask == 0,
               "Some GPIO B declared as not connected are used (0x%x)",
               gpioMask);
#ifdef CHECK_GPIO_ALT_FUNC
    gpioMask = availableGpio_B &
               (~halCfg->usedGpio_B & ~halCfg->noConnectGpio_B);
    HAL_ASSERT(gpioMask == 0,
               "Some available GPIO B declared as alt function (0x%x)",
               gpioMask);
#endif

    // GPIO_D mask check
    gpioMask = ~availableGpio_D & halCfg->usedGpio_D;
    HAL_ASSERT(gpioMask == 0,
               "Some used GPIO D are not available (0x%x)",
               gpioMask);
    gpioMask = ~availableGpio_D & halCfg->noConnectGpio_D;
    HAL_ASSERT(gpioMask == 0,
               "Some GPIO D declared as not connected are not available (0x%x)",
               gpioMask);
    gpioMask = halCfg->usedGpio_D & halCfg->noConnectGpio_D;
    HAL_ASSERT(gpioMask == 0,
               "Some GPIO D declared as not connected are used (0x%x)",
               gpioMask);
#ifdef CHECK_GPIO_ALT_FUNC
    gpioMask = availableGpio_D &
               (~halCfg->usedGpio_D & ~halCfg->noConnectGpio_D);
    HAL_ASSERT(gpioMask == 0,
               "Some available GPIO D declared as alt function (0x%x)",
               gpioMask);
#endif

    // GPIO_E mask check
    gpioMask = ~availableGpio_E & halCfg->usedGpio_E;
    HAL_ASSERT(gpioMask == 0,
               "Some used GPIO E are not available (0x%x)",
               gpioMask);
    gpioMask = ~availableGpio_E & halCfg->noConnectGpio_E;
    HAL_ASSERT(gpioMask == 0,
               "Some GPIO E declared as not connected are not available (0x%x)",
               gpioMask);
    gpioMask = halCfg->usedGpio_E & halCfg->noConnectGpio_E;
    HAL_ASSERT(gpioMask == 0,
               "Some GPIO E declared as not connected are used (0x%x)",
               gpioMask);
#ifdef CHECK_GPIO_ALT_FUNC
    gpioMask = availableGpio_E &
               (~halCfg->usedGpio_E & ~halCfg->noConnectGpio_E);
    HAL_ASSERT(gpioMask == 0,
               "Some available GPIO E declared as alt function (0x%x)",
               gpioMask);
#endif

    // GPO_A mask check
    gpioMask = ~availableGpo_A & halCfg->usedGpo_A;
    HAL_ASSERT(gpioMask == 0,
               "Some used GPO A are not available (0x%x)",
               gpioMask);

#endif // !FPGA

    // Set IO drive
    hwp_configRegs->IO_Drive1_Select = halCfg->ioDrive.select1;
    hwp_configRegs->IO_Drive2_Select = halCfg->ioDrive.select2;

    // Set the not connected ones as output and drive 0
    // But keep boot mode GPIOs as input to avoid current leakage
    noConnectMask_C = halCfg->noConnectGpio_C & ~bootModeGpio_C;
    noConnectMask_A = halCfg->noConnectGpio_A & ~bootModeGpio_A;
    noConnectMask_B = halCfg->noConnectGpio_B & ~bootModeGpio_B;
    noConnectMask_D = halCfg->noConnectGpio_D & ~bootModeGpio_D;
    noConnectMask_E = halCfg->noConnectGpio_E & ~bootModeGpio_E;

    hwp_gpio->gpio_clr = noConnectMask_C;
    hwp_gpio->gpio_oen_set_out = noConnectMask_C;

    hwp_apGpioA->gpio_clr = noConnectMask_A;
    hwp_apGpioA->gpio_oen_set_out = noConnectMask_A;

    hwp_apGpioB->gpio_clr = noConnectMask_B;
    hwp_apGpioB->gpio_oen_set_out = noConnectMask_B;

    hwp_apGpioD->gpio_clr = noConnectMask_D;
    hwp_apGpioD->gpio_oen_set_out = noConnectMask_D;

    hwp_apGpioE->gpio_clr = noConnectMask_E;
    hwp_apGpioE->gpio_oen_set_out = noConnectMask_E;

    // Init GPO_A values
    gpoClr_A = availableGpo_A;
    if (halCfg->useClk32k)
    {
        // Init 32K clock pin to low (by setting up GPO)
        gpoClr_A |= HAL_GPO_BIT(2);
    }
#ifdef _TGT_AP_GPIO_USBID_CTRL
    // if GPO_1 used for usb id, it should be high
    gpoClr_A &= ~(HAL_GPO_BIT(1));
#endif
    hwp_apGpioA->gpo_clr = gpoClr_A;

    // Configure MUX after initializing all the GPO pins
    // (GPIO pins are in input mode by default)

    // Set GPIO mode
    hwp_configRegs->BB_GPIO_Mode = availableGpio_C;
    hwp_configRegs->AP_GPIO_A_Mode = availableGpio_A;
    hwp_configRegs->AP_GPIO_B_Mode = availableGpio_B;
    hwp_configRegs->AP_GPIO_D_Mode = availableGpio_D;
    hwp_configRegs->AP_GPIO_E_Mode = availableGpio_E;
    // Set Alt Mux configuration
    hwp_configRegs->Alt_mux_select = altMux;
    hwp_configRegs->Alt_mux_select2 = altMux2;

    // --------------------------------------------------
    // As of now, all connected GPIOs, which are as GPIO
    // or not used at all but connected are configured as
    // GPIOs in input mode, except for Gouda reset pins
    // --------------------------------------------------

}


// =============================================================================
// hal_BoardSetup
// -----------------------------------------------------------------------------
/// Apply board dependent configuration to HAL
/// @param halCfg Pointer to HAL configuration structure (from the target
/// module).
// ============================================================================
//PROTECTED VOID hal_BoardSetup(CONST HAL_CFG_CONFIG_T* halCfg)
PUBLIC INT32 hal_BoardSetup(INT8 run_mode)
{
    g_bootBootMode = rda_hwcfg_get();
    hal_BoardSetupGeneral(&hal_cfg);

    if(g_halConfigError)
        return CMD_RET_FAILURE;
    else
        return CMD_RET_SUCCESS;
}
