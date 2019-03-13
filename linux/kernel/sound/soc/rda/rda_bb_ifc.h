//==============================================================================
//                                                                              
//            Copyright (C) 2003-2007, Coolsand Technologies, Inc.              
//                            All Rights Reserved                               
//                                                                              
//      This source code is the property of Coolsand Technologies and is        
//      confidential.  Any  modification, distribution,  reproduction or        
//      exploitation  of  any content of this file is totally forbidden,        
//      except  with the  written permission  of  Coolsand Technologies.        
//                                                                              
//==============================================================================
//                                                                              
//    THIS FILE WAS GENERATED FROM ITS CORRESPONDING XML VERSION WITH COOLXML.  
//                                                                              
//                       !!! PLEASE DO NOT EDIT !!!                             
//                                                                              
//  $HeadURL$                                                                   
//  $Author$                                                                    
//  $Date$                                                                      
//  $Revision$                                                                  
//                                                                              
//==============================================================================
//
/// @file
//
//==============================================================================

#ifndef _RDA_BB_IFC_H_
#define _RDA_BB_IFC_H_

#ifdef CT_ASM
#error "You are trying to use in an assembly code the normal H description of 'bb_ifc'."
#endif

//#include "globals.h"

// =============================================================================
//  MACROS
// =============================================================================
#define BB_IFC_ADDR_LEN                          (15)
#define BB_IFC_ADDR_ALIGN                        (2)
#define BB_IFC_TC_LEN                            (8)

// =============================================================================
//  TYPES
// =============================================================================

// ============================================================================
// BB_IFC_T
// -----------------------------------------------------------------------------
/// 
// =============================================================================
#define REG_AU_IFC_BASE             0x009F0000
#define REG_BB_IFC_BASE             0x01901000

typedef volatile struct {
	/// The Channel 0 conveys data from the AIF to the memory.
	///  The Channel 1 conveys data from the memory to the AIF. 
	/// These Channels only exist with Voice Option.
	struct {
		REG32 control;	//0x00000000
		REG32 status;	//0x00000004
		REG32 start_addr;	//0x00000008
		REG32 Fifo_Size;	//0x0000000C
		REG32 Reserved_00000010;	//0x00000010
		REG32 int_mask;	//0x00000014
		REG32 int_clear;	//0x00000018
		REG32 cur_ahb_addr;	//0x0000001C
	} ch[2];
	REG32 ch2_control;	//0x00000040
	REG32 ch2_status;	//0x00000044
	REG32 ch2_start_addr;	//0x00000048
	REG32 ch2_end_addr;	//0x0000004C
	REG32 ch2_tc;		//0x00000050
	REG32 ch2_int_mask;	//0x00000054
	REG32 ch2_int_clear;	//0x00000058
	REG32 ch2_cur_ahb_addr;	//0x0000005C
	REG32 ch3_control;	//0x00000060
	REG32 ch3_status;	//0x00000064
	REG32 ch3_start_addr;	//0x00000068
	REG32 Reserved_0000006C;	//0x0000006C
	REG32 ch3_tc;		//0x00000070
	REG32 ch3_int_mask;	//0x00000074
	REG32 ch3_int_clear;	//0x00000078
	REG32 ch3_cur_ahb_addr;	//0x0000007C
} HWP_BB_IFC_T;

//control
#define BB_IFC_ENABLE               (1<<0)
#define BB_IFC_DISABLE              (1<<1)
#define BB_IFC_AUTO_DISABLE         (1<<4)

//status
//#define BB_IFC_ENABLE             (1<<0)
#define BB_IFC_FIFO_EMPTY           (1<<4)
#define BB_IFC_CAUSE_IEF            (1<<8)
#define BB_IFC_CAUSE_IHF            (1<<9)
#define BB_IFC_CAUSE_I4F            (1<<10)
#define BB_IFC_CAUSE_I3_4F          (1<<11)
#define BB_IFC_IEF                  (1<<16)
#define BB_IFC_IHF                  (1<<17)
#define BB_IFC_I4F                  (1<<18)
#define BB_IFC_I3_4F                (1<<19)

//start_addr
#define BB_IFC_START_ADDR(n)        (((n)&0xFFFFFF)<<2)

//Fifo_Size
#define BB_IFC_FIFO_SIZE(n)         (((n)&0x7FF)<<4)

//int_mask
#define BB_IFC_END_FIFO             (1<<8)
#define BB_IFC_HALF_FIFO            (1<<9)
#define BB_IFC_QUARTER_FIFO         (1<<10)
#define BB_IFC_THREE_QUARTER_FIFO   (1<<11)

//cur_ahb_addr
#define BB_IFC_CUR_AHB_ADDR(n)      (((n)&0x3FFFFFF)<<0)

#endif
