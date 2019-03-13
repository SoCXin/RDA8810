////////////////////////////////////////////////////////////////////////////////
//                                                                            //
//            Copyright (C) 2003-2009, Coolsand Technologies, Inc.            //
//                            All Rights Reserved                             //
//                                                                            //
//      This source code is the property of Coolsand Technologies and is      //
//      confidential.  Any  modification, distribution,  reproduction or      //
//      exploitation  of  any content of this file is totally forbidden,      //
//      except  with the  written permission  of  Coolsand Technologies.      //
//                                                                            //
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
//                                                                            //
/// @file tgt_app_cfg.h                                                       //
/// That file describes the configuration of the application for this target  //
//                                                                            //
////////////////////////////////////////////////////////////////////////////////

#ifndef _TGT_APP_CFG_H_
#define _TGT_APP_CFG_H_
// Partition count.
#define DSM_PART_COUNT   2
// =============================================================================
// TGT_DSM_PART_CONFIG
// -----------------------------------------------------------------------------
// This structure describes the DSM(Data Storage Mangage) configuration.
/// Field description:
/// szPartName: Partition name string,the max size is 15 bytes.
/// eDevType: can be either DSM_MEM_DEV_FLASH for onboard flash combo or DSM_MEM_DEV_TFLASH
/// eCheckLevel: VDS Module cheking level.
//                        DSM_CHECK_LEVEL1: Check the PBD writing and PB Writing.
//                        DSM_CHECK_LEVEL2: Check the PDB writing only.
//                        DSM_CHECK_LEVEL3: Not check.
/// uSecCnt: Number of sector used by this partition (when relevant)
/// uRsvBlkCnt: Number of reseved block. When want the write speed speedy, increase this field value.
/// eModuleId: Module identification.
//                    DSM_MODULE_FS_ROOT: FS Moudle for root directory.
//                    DSM_MODULE_FS:   FS Moudle for mounting device.
//                    DSM_MODULE_SMS: SMS_DM Module.
//                    DSM_MODULE_REG: REG Module
// =============================================================================

#define TGT_DSM_PART_CONFIG                                             \
{                                                                       \
     {                                                                   \
         .szPartName  = "VDS0",                                          \
         .eDevType    = DSM_MEM_DEV_FLASH,                               \
         .eCheckLevel = DSM_CHECK_LEVEL_1,                               \
         .uSecCnt     = 3,                                               \
         .uRsvBlkCnt  = 1,                                               \
         .eModuleId   = DSM_MODULE_FS_ROOT                               \
     },                                                                  \
    {                                                                   \
        .szPartName  = "CSW",                                           \
        .eDevType    = DSM_MEM_DEV_FLASH,                               \
        .eCheckLevel = DSM_CHECK_LEVEL_1,                               \
        .uSecCnt     = 27,                                              \
        .uRsvBlkCnt  = 1,                                               \
        .eModuleId   = DSM_MODULE_CSW                                   \
    },                                                                  \
}

#define TGT_DSM_CONFIG                                                  \
{                                                                       \
    .dsmPartitionInfo = TGT_DSM_PART_CONFIG ,                           \
    .dsmPartitionNumber = DSM_PART_COUNT                                \
}


// =============================================================================
// TGT_CSW_MEM_CONFIG
// -----------------------------------------------------------------------------
/// This structure describes the user heap size
/// cswHeapSize: Size of the heap available for csw
/// cosHeapSize: Size of the heap available for mmi
// =============================================================================
#define TGT_CSW_CONFIG                                                  \
{                                                                       \
  .cswHeapSize = 400*1024,                                              \
  .cosHeapSize = 650*1024                                               \
}

// =============================================================================
// TGT_UCTLS_CONFIG
// -----------------------------------------------------------------------------
/// Default List of services
// =============================================================================
#include "uctls_tgt_params.h"


#endif //_TGT_APP_CFG_H_
