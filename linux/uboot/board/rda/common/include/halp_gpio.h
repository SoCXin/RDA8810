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
//                                                                            //
//  $HeadURL: http://svn.rdamicro.com/svn/developing1/Sources/chip/branches/8810/hal/src/halp_gpio.h $ //
//    $Author: admin $                                                        // 
//    $Date: 2010-07-07 20:28:03 +0800 (Wed, 07 Jul 2010) $                     //   
//    $Revision: 269 $                                                          //   
//                                                                            //
////////////////////////////////////////////////////////////////////////////////
//                                                                            //
/// @file halp_gpio.h                                                         //
/// This file contains Granite's GPIO driver implementation                   //
//                                                                            //
//////////////////////////////////////////////////////////////////////////////// 


#ifndef  _HALP_GPIO_H_
#define  _HALP_GPIO_H_

// =============================================================================
// MACROS
// =============================================================================

// =============================================================================
// HAL_GPIO_BIT
// -----------------------------------------------------------------------------
/// This macro is used by internal code to convert gpio number to bit.
/// It masks the upper bit so it can be used directly with #HAL_GPIO_GPIO_ID_T.
// =============================================================================
#define HAL_GPIO_BIT(n)    (1<<((n)&0x3f))

// =============================================================================
// HAL_GPO_BIT
// -----------------------------------------------------------------------------
/// This macro is used by internal code to convert gpio number to bit.
/// It masks the upper bit so it can be used directly with #HAL_GPIO_GPO_ID_T.
// =============================================================================
#define HAL_GPO_BIT(n)    (1<<((n)&0x3f))




// =============================================================================
// hal_GpioIrqHandler
// -----------------------------------------------------------------------------
/// GPIO module IRQ handler
/// 
///     Clear the IRQ and call the IRQ handler
///     user function
///     @param interruptId The interruption ID
// =============================================================================  
PROTECTED VOID hal_GpioIrqHandler(UINT8 interruptId);


#endif //HAL_GPIO_H
