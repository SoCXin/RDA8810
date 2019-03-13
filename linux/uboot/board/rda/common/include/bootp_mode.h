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
//  $HeadURL: http://svn.rdamicro.com/svn/developing1/Sources/chip/branches/gallite441/boot/8809/src/bootp_mode.h $ //
//    $Author: huazeng $                                                        // 
//    $Date: 2011-11-30 18:24:46 +0800 (Wed, 30 Nov 2011) $                     //   
//    $Revision: 12233 $                                                          //   
//                                                                            //
////////////////////////////////////////////////////////////////////////////////
//                                                                            //
/// @file halp_boot_asm.h                                                     //
/// That file provides defines used by the assembly boot code.                //
//                                                                            //
////////////////////////////////////////////////////////////////////////////////

#ifndef _HALP_BOOT_MODE_H_
#define _HALP_BOOT_MODE_H_


#define BOOT_MODE_STD                       (0)

// O: SLC
#define BOOT_MODE_NAND_MLC                  (1<<0)
// 0: 16 bits
#define BOOT_MODE_NAND_8BIT                 (1<<1)
// 0: boot from nand
#define BOOT_MODE_BOOT_EMMC                 (1<<2)
// 0: boot from emmc or nand
#define BOOT_MODE_BOOT_SPI                  (1<<3)
// 0: boot from nor spi
#define BOOT_MODE_BOOT_SPI_NAND             (1<<4)
// 0: spi flash from camera pins
#define BOOT_MODE_SPI_PIN_NAND              (1<<5)
// 0: nand flash high 8 bits from lcd pins
#define BOOT_MODE_NAND_HIGH_PIN_CAM         (1<<6)
// 0: nand page size small, 2K-SLC/4K-MLC; 1: 4K-SLC/8K-MLC
#define BOOT_MODE_NAND_PAGE_SIZE_L          (1<<7)

#define BOOT_MODE_JTAG_ENABLE               (1<<8)

#define BOOT_MODE_AP_MEM_CHECK_DISABLE      (1<<9)

#define BOOT_MODE_DRIVER_TEST               (1<<10)

#define BOOT_MODE_FORCE_DOWNLOAD            (1<<11)

#define BOOT_MODE_KEY_ON                    (1<<12)
// If with KeyOn, driver test;
// If with long KeyOn, hardware reset
#define BOOT_MODE_VOL_DN                    (1<<13)
// If with KeyOn, force download
#define BOOT_MODE_VOL_UP                    (1<<14)
// Force download
#define BOOT_MODE_PLUG_IN                   (1<<15)


#endif // _HALP_BOOT_MODE_H_


