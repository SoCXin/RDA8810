////////////////////////////////////////////////////////////////////////////////
//                                                                            //
//            Copyright (C) 2003-2007, Coolsand Technologies, Inc.            //
//                            All Rights Reserved                             //
//                                                                            //
//      This source code is the property of Coolsand Technologies and is      //
//      confidential.  Any  modification, distribution,  reproduction or      //
//      exploitation  of  any content of this file is totally forbidden,      //
//      except  with the  written permission  of  Coolsand Technologies.      //
//                                                                            //
////////////////////////////////////////////////////////////////////////////////
//
//  $HeadURL: http://svn.rdamicro.com/svn/developing1/Sources/chip/branches/8810/hal/src/halp_sys.h $
//  $Author: huazeng $
//  $Date: 2013-08-05 11:38:03 +0800 (Mon, 05 Aug 2013) $
//  $Revision: 20799 $
//
////////////////////////////////////////////////////////////////////////////////
//                                                                            //
/// @file halp_sys.h                                                          //
/// That file provides the private interface for the System Driver.           //
//                                                                            //
////////////////////////////////////////////////////////////////////////////////

#ifndef  _HALP_SYS_H_
#define  _HALP_SYS_H_

// =============================================================================
// 
// -----------------------------------------------------------------------------
// =============================================================================


// =============================================================================
//  MACROS
// =============================================================================


// =============================================================================
// HAL_SYS_FREQ_MAX_RESOURCE_NB
// -----------------------------------------------------------------------------
/// Maximum number of resources
// =============================================================================
#define HAL_SYS_FREQ_MAX_RESOURCE_NB  64


// =============================================================================
// HAL_SYS_FREQ_WIDTH
// -----------------------------------------------------------------------------
/// Width of the bitfield holding a frequency value in bits.
/// Not sure that scales pretty well...
// =============================================================================
#define HAL_SYS_FREQ_WIDTH  8



// default clock generation params
// PCM_CLOCKS_POLARITY : INVERT_PCM_BCK | INVERT_PCM_MCK
#define PCM_CLOCKS_POLARITY 0
// ANA_CLOCKS_POLARITY : INVERT_AU_ADC_CLK | INVERT_TX_FS_CLK 
//  | INVERT_RX_FS_CLK | INVERT_PA_DAC_CLK | INVERT_AFC_DAC_CLK
#define ANA_CLOCKS_POLARITY ANA_ACCO_GPADC_CLK_POL
// RF_FS_SETTING : RX_FS_IS_13M, RX_FS_IS_6M5
#define RF_FS_SETTING RX_FS_IS_6M5
// AFC_DIVIDER_SETTING : n
#define AFC_DIVIDER_SETTING 8
#define GPADC_DIVIDER_SETTING   0x1F


#define PLL_CONFIG (SYS_CTRL_PLL_R(4-2) | SYS_CTRL_PLL_F(48-2) | SYS_CTRL_PLL_OD_DIV_BY_1)

// =============================================================================
//  TYPES
// =============================================================================

// =============================================================================
// HAL_SYS_FREQ_SCAL_USERS_T
// -----------------------------------------------------------------------------
/// That type lists all the possible entities (software / drivers / peripherals)
/// that need a specific minimum clock.
/// This is the internal type for HAL use only. Specific resource identifiers
/// are exported through the interface for use in upper layer or by upper 
/// layers.
/// The list attempts to be exhaustive, but could contain useless fields
/// and lack some others. Don't forget to adjust #HAL_SYS_TENS_QTY consequently.
// =============================================================================
typedef enum
{
    HAL_SYS_FREQ_PWM, // 0
    HAL_SYS_FREQ_SIM,
    HAL_SYS_FREQ_SPI,
    HAL_SYS_FREQ_TRACE,
    HAL_SYS_FREQ_UART,
    HAL_SYS_FREQ_I2C, // 5
    HAL_SYS_FREQ_RF_SPI,
    HAL_SYS_FREQ_DEBUG_HOST,
    HAL_SYS_FREQ_AIF,
    HAL_SYS_FREQ_AUDIO,
    HAL_SYS_FREQ_HAL_NULL_0, // 10
    HAL_SYS_FREQ_SDMMC,
    HAL_SYS_FREQ_SDMMC2,
    HAL_SYS_FREQ_LCD,
    HAL_SYS_FREQ_CLK_OUT,
    HAL_SYS_FREQ_CAMCLK, // 15
    HAL_SYS_FREQ_USB,
    HAL_SYS_FREQ_DMA,
    HAL_SYS_FREQ_VOC,
    HAL_SYS_FREQ_AP,
    HAL_SYS_FREQ_BTCPU, // 20
    // !!! Add new sys users above !!!
    HAL_SYS_FREQ_SYS_USER_END,
    HAL_SYS_FREQ_SYS_LAST_USER = HAL_SYS_FREQ_SYS_USER_END-1,

// The last sys user ID should be less than the first platform user
// ID, which is 25 in hal_sys.h at present.
// This is checked in hal_SysSetupSystemClock, PROTECTED function
// called only once during hal_Open().
    HAL_SYS_USER_START = HAL_SYS_FREQ_PLATFORM_FIRST_USER,

    HAL_SYS_TOTAL_USERS_QTY = HAL_SYS_FREQ_USER_QTY

} HAL_SYS_FREQ_SCAL_USERS_T;


// =============================================================================
// HAL_SYS_FREQ_SCAL_FREQ_T
// -----------------------------------------------------------------------------
/// This list the possible value for the frequency required by the frequence 
/// scaling users. This type differs from the one in the interface because
/// the value here have to be stored into the #HAL_SYS_FREQ_WIDTH (8 bits) wide
/// field. It is only used in the managing of the frequency scaling.
// =============================================================================
typedef enum
{
    HAL_SYS_FREQ_SCAL_32K   = 0,
    HAL_SYS_FREQ_SCAL_13M   = 1,
    HAL_SYS_FREQ_SCAL_26M   = 2,
    HAL_SYS_FREQ_SCAL_39M   = 3,
    HAL_SYS_FREQ_SCAL_52M   = 4,
    HAL_SYS_FREQ_SCAL_78M   = 5,
    HAL_SYS_FREQ_SCAL_104M  = 6,
    HAL_SYS_FREQ_SCAL_156M  = 7,
#if (CHIP_HAS_ASYNC_TCU)
    HAL_SYS_FREQ_SCAL_208M  = 8,
    HAL_SYS_FREQ_SCAL_250M  = 9,
    HAL_SYS_FREQ_SCAL_312M  = 10,
#endif
    HAL_SYS_FREQ_SCAL_QTY

} HAL_SYS_FREQ_SCAL_FREQ_T;


//// =============================================================================
////  EXPORTED VARIABLES
//// =============================================================================
//
//#if (CHIP_HAS_ASYNC_TCU)
//// ============================================================================
//// g_bootSysTcuRunningAt26M
//// ----------------------------------------------------------------------------
/////  Whether TCU is running at 26M
//// ============================================================================
//PUBLIC EXPORT BOOL g_bootSysTcuRunningAt26M;
//#endif
//
//// =============================================================================
//// g_halSysFreqScalRegistry
//// -----------------------------------------------------------------------------
///// This array is used to store the frequency required by each frequency scaling
///// user, a dime a tens.
//// =============================================================================
//PROTECTED EXPORT UINT32 g_halSysFreqScalRegistry[HAL_SYS_FREQ_MAX_RESOURCE_NB/4];
//
//// ============================================================================
//// g_halSysSystemFreq
//// ----------------------------------------------------------------------------
/////  Global var to have a view of the system frequency
//// ============================================================================
//PROTECTED EXPORT HAL_SYS_FREQ_T g_halSysSystemFreq;
//
//// ============================================================================
//// g_halNumberOfUserPerFreq
//// ----------------------------------------------------------------------------
///// This array is used to calculate the minimum system frequency to set
///// (that is the maximum required by a module among all the module) in a 
///// constant time (relative to the number of users).
///// 
//// ============================================================================
//PROTECTED EXPORT UINT32 g_halNumberOfUserPerFreq[HAL_SYS_FREQ_SCAL_QTY];
//
//
//// =============================================================================
////  FUNCTIONS
//// =============================================================================
//
//// =============================================================================
//// hal_SysFreqScalSet
//// -----------------------------------------------------------------------------
///// Set the required frequency for a given user in the frequency scaling
///// registry.
///// @param user Frequency scaling user Id.
///// @param freq Minimum frequency required by this user.
//// =============================================================================
//INLINE VOID hal_SysFreqScalSet(HAL_SYS_FREQ_SCAL_USERS_T user, HAL_SYS_FREQ_SCAL_FREQ_T freq)
//{
//    UINT8* registry = (UINT8*)g_halSysFreqScalRegistry;
//
//    // We exit the previous frequency class
//    g_halNumberOfUserPerFreq[registry[user]] -=1;
//
//    // To enter the new one
//    g_halNumberOfUserPerFreq[freq] += 1;
//
//    // Update the registry
//    registry[user] = freq;
//}
//
//
//
//// =============================================================================
//// hal_SysFreqScalGet
//// -----------------------------------------------------------------------------
///// Get the required frequency for a given user.
///// @return The frequency required by a given user. 
//// =============================================================================
//INLINE HAL_SYS_FREQ_SCAL_FREQ_T hal_SysFreqScalGet(HAL_SYS_FREQ_SCAL_USERS_T user)
//{
//    UINT8* registry = (UINT8*)g_halSysFreqScalRegistry;
//    return (registry[user]);
//}
//
//// =============================================================================
//// hal_SysSF2FSF
//// -----------------------------------------------------------------------------
///// Convert a System Frequency into a Frequency Scaling Frequency
//// =============================================================================
//PROTECTED HAL_SYS_FREQ_SCAL_FREQ_T hal_SysSF2FSF(HAL_SYS_FREQ_T freq);
//
//
//
//// =============================================================================
//// hal_SysFSF2SF
//// -----------------------------------------------------------------------------
///// Convert a Frequency Scaling Frequency into a System Frequency
//// =============================================================================
//PROTECTED HAL_SYS_FREQ_T hal_SysFSF2SF(HAL_SYS_FREQ_SCAL_FREQ_T fsfFreq);
//
//
//
//// =============================================================================
//// hal_SysSetupSystemClock
//// -----------------------------------------------------------------------------
///// Configure the initial settings of the system clock.
///// This function is to be called only by hal_init.
///// @param fastClockSetup Initial System Clock.
//// =============================================================================
//PROTECTED VOID hal_SysSetupSystemClock(HAL_SYS_FREQ_T fastClockSetup);
//
//
//
//// =============================================================================
//// hal_SysXcpuSleep
//// -----------------------------------------------------------------------------
///// Put the XCPU to sleep
///// @param wakeUpMask Set the WakeUp mask, a bitfield where a bit to '1'
///// means that the corresponding IRQ can wake up the CPU.
//// =============================================================================
//INLINE VOID hal_SysXcpuSleep(VOID)
//{
//    hwp_sysIrq->Cpu_Sleep = SYS_IRQ_SLEEP;
//    // flush the write buffer to ensure we sleep before exiting this function
//    UINT32 clkSysDisable __attribute__((unused)) = hwp_sysIrq->Cpu_Sleep;
//}
//
//// =============================================================================
//// hal_SysEnablePLL
//// -----------------------------------------------------------------------------
///// Enable PLL
//// =============================================================================
//PROTECTED VOID hal_SysEnablePLL(VOID);
//
//// =============================================================================
//// hal_SysDisablePLL
//// -----------------------------------------------------------------------------
///// Disable PLL and switch on low clock
//// =============================================================================
//PROTECTED VOID hal_SysDisablePLL(VOID);
//
//// =============================================================================
//// hal_SysGetPLLLock
//// -----------------------------------------------------------------------------
///// Return PLL lock 
///// @return PLL status
/////         If \c TRUE, PLL locked.
/////         If \c FALSE, PLL not locked.
//// =============================================================================
//PROTECTED BOOL hal_SysGetPLLLock(VOID);
//
//
//#ifdef CHIP_IS_MODEM
//// =============================================================================
//// hal_SysApHostEnable
//// -----------------------------------------------------------------------------
///// Enable or disable usb host.
///// This is useful to inform hal_SysProcessHostMonitor() that the AP Host functions
///// must be called to process the host commands.
///// @param enable \c TRUE to enable, \c FALSE to disable.
//// =============================================================================
//PROTECTED VOID hal_SysApHostEnable(BOOL enable);
//
//
//// =============================================================================
//// hal_SysEnableApClock
//// -----------------------------------------------------------------------------
///// Enable or disable AP clock.
///// @param enable \c TRUE to enable, \c FALSE to disable.
//// =============================================================================
//PROTECTED VOID hal_SysEnableApClock(BOOL enable);
//
//
//// =============================================================================
//// hal_SysEnableApIrq
//// -----------------------------------------------------------------------------
///// Enable or disable AP irq.
///// @param enable \c TRUE to enable, \c FALSE to disable.
//// =============================================================================
//PROTECTED VOID hal_SysEnableApIrq(BOOL enable);
//
//
//// =============================================================================
//// hal_SysApIrqActive
//// -----------------------------------------------------------------------------
///// Check AP irq status.
///// @return TRUE if AP irq is active; FALSE otherwise
//// =============================================================================
//PROTECTED BOOL hal_SysApIrqActive(VOID);
//
//
//#else // !CHIP_IS_MODEM
//// =============================================================================
//// hal_SysSetupClkUart
//// -----------------------------------------------------------------------------
///// Setup the uart clock
///// Be careful not to use it with a uart2 which doesn't exist here (yet)
///// or any improper parameter.
///// @param Uart_id Id of the uart whose clock we want to setup
///// @param divider Divider of the clock, from 2 to 1025
//// =============================================================================
//INLINE VOID hal_SysSetupClkUart(HAL_UART_ID_T uartId, UINT16 divider)
//{
//    hwp_sysCtrl->Cfg_Clk_Uart[uartId] = divider-2;
//}
//
//
//// =============================================================================
//// hal_SysSetupClkPwm
//// -----------------------------------------------------------------------------
///// Setup the PWM clock
///// The PWM clock is got by dividing the system clock    
///// @param divider The divider used
//// =============================================================================
//INLINE VOID hal_SysSetupClkPwm(UINT8 divider)
//{
//    hwp_sysCtrl->Cfg_Clk_PWM = divider;
//}
//#endif // !CHIP_IS_MODEM
//
//
//// =============================================================================
//// hal_SysSetupSystemClock
//// -----------------------------------------------------------------------------
///// Configure the initial settings of the system clocks.
///// This function is to be called only by hal_init.
///// It also checks the validity of the public enum for HAL_SYS_FREQ_T and 
///// the internal one HAL_SYS_FREQ_SCAL_USERS_T use for frequency scaling.
///// @param fastClockSetup Initial System Clock.
//// =============================================================================
//PROTECTED VOID hal_SysSetupSystemClock(HAL_SYS_FREQ_T fastClockSetup);
//
//
//
//// =============================================================================
//// hal_SysUpdateSystemFrequency
//// -----------------------------------------------------------------------------
///// Set the system frequency to the highest minimal frequency required 
///// by the user of the system.
//// =============================================================================
//PROTECTED VOID hal_SysUpdateSystemFrequency();
//
//
//// =============================================================================
//// hal_SysUsbHostEnable
//// -----------------------------------------------------------------------------
///// Enable or disable usb host.
///// This is useful to inform hal_SysProcessIdle() that the USB Host functions
///// must be called to process the host commands.
///// @param enable \c TRUE to enable, \c FALSE to disable.
//// =============================================================================
//PROTECTED VOID hal_SysUsbHostEnable(BOOL enable);


#endif //  HAL_SYS_H


