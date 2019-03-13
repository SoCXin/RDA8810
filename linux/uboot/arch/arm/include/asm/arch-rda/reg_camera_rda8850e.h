//==============================================================================
//
//            Copyright (C) 2012-2021,RDA, Inc.
//                            All Rights Reserved
//
//      This source code is the property of Coolsand Technologies and is
//      confidential.  Any  modification, distribution,  reproduction or
//      exploitation  of  any content of this file is totally forbidden,
//      except  with the  written permission  of  RDA.
//
//==============================================================================


#ifndef _CAMERA_H_
#define _CAMERA_H_

#include "cs_types.h"

#include "iomap_rda8850e.h"
// =============================================================================
//  MACROS
// =============================================================================
#define FIFORAM_SIZE                             (128)

// =============================================================================
//  TYPES
// =============================================================================

// ============================================================================
// CAMERA_T
// -----------------------------------------------------------------------------
///
// =============================================================================
#define REG_CAMERA_BASE             RDA_CAMERA_BASE

typedef volatile struct
{
	REG32	CTRL;                         //0x00000000
	REG32	STATUS;                       //0x00000004
	REG32	DATA;                         //0x00000008
	REG32	IRQ_MASK;                     //0x0000000C
	REG32	IRQ_CLEAR;                    //0x00000010
	REG32	IRQ_CAUSE;                    //0x00000014
	REG32	CMD_SET;                      //0x00000018
	REG32	CMD_CLR;                      //0x0000001C
	REG32	DSTWINCOL;                    //0x00000020
	REG32	DSTWINROW;                    //0x00000024
	REG32	CLK_OUT;                      //0x00000028
	REG32	SCALE_CFG;                    //0x0000002c
	REG32	CAM_SPI_REG_0;                //0x00000030
	REG32	CAM_SPI_REG_1;                //0x00000034
	REG32	CAM_SPI_REG_2;                //0x00000038
	REG32	CAM_SPI_REG_3;                //0x0000003c
	REG32	CAM_SPI_REG_4;                //0x00000040
	REG32	CAM_SPI_REG_5;                //0x00000044
	REG32	CAM_SPI_REG_6;                //0x00000048
	REG32	CAM_SPI_OBSERVE_REG_0;        //0x0000004c, read only
	REG32	CAM_SPI_OBSERVE_REG_1;        //0x00000050, read only
    	REG32	CSI_CONFIG_REG0;                         //0x00000054
	REG32	CSI_CONFIG_REG1;                       //0x00000058
	REG32	CSI_CONFIG_REG2;                         //0x0000005C
	REG32	CSI_CONFIG_REG3;                     //0x00000060
	REG32	CSI_CONFIG_REG4;                     //0x00000064
	REG32	CSI_CONFIG_REG5;                     //0x00000068
	REG32	CSI_CONFIG_REG6;                     //0x0000006C
	REG32	CSI_CONFIG_REG7;                     //0x00000070
	REG32	CSI_OBSERVE_REG4;                    //0x00000074
	REG32	CSI_OBSERVE_REG5;                    //0x00000078
	REG32	CSI_OBSERVE_REG6;                      //0x0000007C  read only
	REG32	CSI_OBSERVE_REG7;                      //0x00000080   read only
	REG32	CSI_ENABLE_PHY;				//0x00000084
	REG32	DCT_SHIFTR_Y_REG0;				//0x00000088
	REG32	DCT_SHIFTR_Y_REG1;				//0x0000008C
	REG32	CAM_AXI_CONFIG;               //0x00000090
	REG32	CAM_FRAME_START_ADDR;         //0x00000094
	REG32	CAM_FRAME_SIZE;               //0x00000098
	REG32	CAM_TC_COUNT;                 //0x0000009C
	REG32	CAM_FRAME_START_U_ADDR;         //0x000000A0
	REG32	CAM_FRAME_START_V_ADDR;         //0x000000A4
	REG32	CAM_FRAME2_START_ADDR;         //0x000000A8
	REG32	CAM_FRAME2_START_U_ADDR;         //0x000000AC
	REG32	CAM_FRAME2_START_V_ADDR;         //0x000000B0
	REG32	CFG_CAM_C2CSE;				//0x000000B4
} HWP_CAMERA_T;

#define hwp_camera                  ((HWP_CAMERA_T*) (REG_CAMERA_BASE))


//CTRL
#define CAMERA_ENABLE               (1<<0)
#define CAMERA_ENABLE_ENABLE        (1<<0)
#define CAMERA_ENABLE_DISABLE       (0<<0)
#define CAMERA_DCT_ENABLE               (1<<1)
#define CAMERA_DCT_DISABLE			(0<<1)
#define CAMERA_1_BUFENABLE               (1<<2)
#define CAMERA_1_BUFENABLE_ENABLE        (1<<2)
#define CAMERA_1_BUFENABLE_DISABLE       (0<<2)
#define CAMERA_OUT_CLK_GATE_EN (1 << 3)
#define CAMERA_DATAFORMAT(n)        (((n)&3)<<4)
#define CAMERA_DATAFORMAT_RGB565    (0<<4)
#define CAMERA_DATAFORMAT_YUV422    (1<<4)
#define CAMERA_DATAFORMAT_JPEG      (2<<4)
#define CAMERA_DATAFORMAT_RESERVE   (3<<4)
#define CAMERA_ISP_ENABLE			(1 << 6)
#define CAMERA_RGB_RFIST		(1 << 7)
#define CAMERA_RESET_POL            (1<<8)
#define CAMERA_RESET_POL_INVERT     (1<<8)
#define CAMERA_RESET_POL_NORMAL     (0<<8)
#define CAMERA_PWDN_POL             (1<<9)
#define CAMERA_PWDN_POL_INVERT      (1<<9)
#define CAMERA_PWDN_POL_NORMAL      (0<<9)
#define CAMERA_VSYNC_POL            (1<<10)
#define CAMERA_VSYNC_POL_INVERT     (1<<10)
#define CAMERA_VSYNC_POL_NORMAL     (0<<10)
#define CAMERA_HREF_POL             (1<<11)
#define CAMERA_HREF_POL_INVERT      (1<<11)
#define CAMERA_HREF_POL_NORMAL      (0<<11)
#define CAMERA_LINEBUF_ISP_ENABLE	(1 << 12)
#define CAMERA_LINEBUF_IF	(1 << 13)
#define CAMERA_VSYNC_DROP           (1<<14)
#define CAMERA_VSYNC_DROP_DROP      (1<<14)
#define CAMERA_VSYNC_DROP_NORMAL    (0<<14)
#define CAMERA_DECIMFRM(n)          (((n)&3)<<16)
#define CAMERA_DECIMFRM_ORIGINAL    (0<<16)
#define CAMERA_DECIMFRM_DIV_2       (1<<16)
#define CAMERA_DECIMFRM_DIV_3       (2<<16)
#define CAMERA_DECIMFRM_DIV_4       (3<<16)
#define CAMERA_DECIMCOL(n)          (((n)&3)<<18)
#define CAMERA_DECIMCOL_ORIGINAL    (0<<18)
#define CAMERA_DECIMCOL_DIV_2       (1<<18)
#define CAMERA_DECIMCOL_DIV_3       (2<<18)
#define CAMERA_DECIMCOL_DIV_4       (3<<18)
#define CAMERA_DECIMROW(n)          (((n)&3)<<20)
#define CAMERA_DECIMROW_ORIGINAL    (0<<20)
#define CAMERA_DECIMROW_DIV_2       (1<<20)
#define CAMERA_DECIMROW_DIV_3       (2<<20)
#define CAMERA_DECIMROW_DIV_4       (3<<20)
#define CAMERA_REORDER(n)           (((n)&7)<<24)
#define CAMERA_CROPEN               (1<<28)
#define CAMERA_CROPEN_ENABLE        (1<<28)
#define CAMERA_CROPEN_DISABLE       (0<<28)
#define CAMERA_BIST_MODE            (1<<30)
#define CAMERA_BIST_MODE_BIST       (1<<30)
#define CAMERA_BIST_MODE_NORMAL     (0<<30)
#define CAMERA_TEST                 (1<<31)
#define CAMERA_TEST_TEST            (1<<31)
#define CAMERA_TEST_NORMAL          (0<<31)

//STATUS
#define CAMERA_OVFL                 (1<<0)
#define CAMERA_VSYNC_R              (1<<1)
#define CAMERA_VSYNC_F              (1<<2)
#define CAMERA_DMA_DONE             (1<<3)
#define CAMERA_FIFO_EMPTY           (1<<4)

//DATA
#define CAMERA_RX_DATA(n)           (((n)&0xFFFFFFFF)<<0)

//IRQ_MASK
//#define CAMERA_OVFL               (1<<0)
//#define CAMERA_VSYNC_R            (1<<1)
//#define CAMERA_VSYNC_F            (1<<2)
//#define CAMERA_DMA_DONE           (1<<3)

//IRQ_CLEAR
//#define CAMERA_OVFL               (1<<0)
//#define CAMERA_VSYNC_R            (1<<1)
//#define CAMERA_VSYNC_F            (1<<2)
//#define CAMERA_DMA_DONE           (1<<3)

//IRQ_CAUSE
//#define CAMERA_OVFL               (1<<0)
//#define CAMERA_VSYNC_R            (1<<1)
//#define CAMERA_VSYNC_F            (1<<2)
//#define CAMERA_DMA_DONE           (1<<3)

//CMD_SET,modified by xiankuiwei
#define CAMERA_RESET                (1<<0)
#define CAMERA_PWDN                 (1<<4)
#define CAMERA_FIFO_RESET           (1<<8)

//CMD_CLR
//#define CAMERA_PWDN               (1<<4)
//#define CAMERA_RESET              (1<<0)

//DSTWINCOL,modified by xiankuiwei
#define CAMERA_DSTWINCOLSTART(n)    (((n)&0xFFF)<<16)
#define CAMERA_DSTWINCOLEND(n)      (((n)&0xFFF)<<0)

//DSTWINROW
#define CAMERA_DSTWINROWSTART(n)    (((n)&0xFFF)<<16)
#define CAMERA_DSTWINROWEND(n)      (((n)&0xFFF)<<0)

// SCALE CONFIG, no use now
#define CAM_SCALE_EN            (1<<0)
#define CAM_SCALE_COL(n)      (((n)&0x3)<<8)
#define CAM_SCALE_ROW(n)     (((n)&0x3)<<16)

/*************************************************************/
/***************** SPI CAMERA CONFIGURATION ********************/
/*************************************************************/

// Here, spi master mode means the sensor works as the SPI master.
// master mode 1 means 1 data ouput with SSN, master mode 2 means the other master modes.

//--------------------------------
// camera_spi_reg_0
//--------------------------------
// the number of lines per frame
#define CAM_SPI_REG_LINE_PER_FRM(n)     (((n)&0x3ff)<<22)
// the number of words(32 bits) per line
#define CAM_SPI_REG_BLK_PER_LINE(n)     (((n)&0x3ff)<<12)
// VSYNC high effective
#define CAM_SPI_REG_VSYNC_INV_EN        (1<<11)
// HREF low effective
#define CAM_SPI_REG_HREF_INV_EN           (1<<10)
// OVFL low effective
#define CAM_SPI_REG_OVFL_INV_EN           (1<<9)
// little endian enable
#define CAM_SPI_REG_LITTLE_END_EN        (1<<8)
// module reset when ovfl enable
#define CAM_SPI_REG_OVFL_RST_EN           (1<<7)
// observe the overflow when vsync effective
#define CAM_SPI_REG_OVFL_OBS                 (1<<6)
// reset the module when overflow in vsync effective
#define CAM_SPI_REG_OVFL_RST                  (1<<5)
/************************************************
YUV data output format
        3'b000: Y0-U0-Y1-V0                3'b001: Y0-V0-Y1-U0
        3'b010: U0-Y0-V0-Y1                3'b011: U0-Y1-V0-Y0
        3'b100: V0-Y1-U0-Y0                3'b101: V0-Y0-U0-Y1
        3'b110: Y1-V0-Y0-U0                3'b111: Y1-U0-Y0-V0
*************************************************/
#define CAM_SPI_REG_YUV_OUT_FMT(n)      (((n)&0x7)<<2)
// SPI master mode 1 enable
#define CAM_SPI_REG_MASTER_EN               (1<<1)
// SPI slave mode enable
#define CAM_SPI_REG_SLAVE_EN                  (1<<0)

//--------------------------------
// camera_spi_reg_1
//--------------------------------
#define CAM_SPI_REG_CLK_DIV(n)                ((n)&0xffff)      // camera clock divider
#define CAM_SPI_REG_SSN_HIGH_EN             (1<<17) // SSN high enable, only for master mode 1

//--------------------------------
// camera_spi_reg_2
//--------------------------------
// only take effect in slave mode which is not supported yet

//--------------------------------
// camera_spi_reg_3
//--------------------------------
// only take effect in slave mode which is not supported yet

//--------------------------------
// camera_spi_reg_4
//--------------------------------
#define CAM_SPI_REG_BLK_PER_PACK(n)              (((n)&0x3ff)<<6)
#define CAM_SPI_REG_IMG_WIDTH_FROM_REG      (1<<5)
#define CAM_SPI_REG_IMG_HEIGHT_FROM_REG     (1<<4)
#define CAM_SPI_REG_PACK_SIZE_FROM_REG       (1<<3)
#define CAM_SPI_REG_LINE(n)                             (((n)&0x3)<<1)
#define CAM_SPI_REG_MASTER2_EN                      (1<<0)

//--------------------------------
// camera_spi_reg_5
//--------------------------------
#define CAM_SPI_REG_SYNC_CODE(n)                ((n)&0xffffff)

//--------------------------------
// camera_spi_reg_6
//--------------------------------
// frame_start packet id, only in master mode 2
#define CAM_SPI_REG_FRM_START_PKT_ID(n)            (((n)&0xff)<<24)
// frame_end packet id, only in master mode 2
#define CAM_SPI_REG_FRM_ENDN_PKT_ID(n)             (((n)&0xff)<<16)
// line_start packet id, only in master mode 2
#define CAM_SPI_REG_LINE_START_PKT_ID(n)           (((n)&0xff)<<8)
// data packet id, only in master mode 2
#define CAM_SPI_REG_DATA_PKT_ID(n)               ((n)&0xff)

//--------------------------------
// camera_spi_observe_reg_0 (read only)
//--------------------------------

//--------------------------------
// camera_spi_observe_reg_1 (read only)
//--------------------------------

//--------------------------------
// csi_config_reg0
//--------------------------------
#define CSI_NUM_D_TERM_EN(n)          (((n)&0XFF)<<0)
#define CSI_CUR_FRAME_LINE_NUM(n)          (((n)&0X3FF)<<8)
#define CSI_VC_ID_SET(n)          (((n)&3)<<18)
#define CSI_DATA_LP_IN_CHOOSE(n)          (((n)&3)<<20)
#define CSI_DATA_LP_INV           (1<<22)
#define CSI_CLK_LP_INV           (1<<23)
#define CSI_TRAIL_DATA_WRONG_CHOOSE    (1<<24)
#define CSI_SYNC_BYPASS       (1<<25)
#define CSI_RDATA_BIT_INV_EN               (1<<26)
#define CSI_HS_SYNC_FIND_EN        (1<<27)
#define CSI_LINE_PACKET_ENABLE       (1<<28)
#define CSI_ECC_BYPASS                 (1<<29)
#define CSI_DATA_LANE_CHOOSE            (1<<30)
#define CSI_MODULE_ENABLE          (1<<31)

//--------------------------------
// csi_config_reg01
//--------------------------------
#define CSI_NUM_HS_SETTLE(n)          (((n)&0XFF)<<0)
#define CSI_LP_DATA_LENGTH_CHOSSE(n)          (((n)&7)<<8)
#define CSI_DATA_CLK_LP_POSEEDGE_CHOSSE(n)          (((n)&7)<<11)
#define CSI_CLK_LP_CK_INV        (1<<14)
#define CSI_RCLR_MASK_EN        (1<<15)
#define CSI_RINC_MASK_EN        (1<<16)
#define CSI_HS_ENALBE_MASK_EN        (1<<17)
#define CSI_DEN_INV_BIT        (1<<18)
#define CSI_HSYNC_INV_BIT        (1<<19)
#define CSI_VSYNC_INV_BIT        (1<<20)
#define CSI_HS_DATA2_ENABLE_REG  (1<<21)

#define CSI_HS_TATA1_ENALBE_REG   (1<<22)
#define CSI_HS_DATA1_ENABLE_CHOSSE           (1<<23)
#define CSI_HS_DATA1_ENALBE_DRE    (1<<24)
#define CSI_DATA2_TER_ENABLE_REG         (1<<25)
#define CSI_DATAL_TER_ENABLE_REG           (1<<26)
#define CSI_DATAL_TER_ENABLE_DR    (1<<27)
#define CSI_LP_DATA_INTERRUPT_CLR  (1<<28)
#define CSI_LP_CMD_INTERRUPT_CLR       (1<<29)
#define CSI_LP_DATA_CLR            (1<<30)
#define CSI_LP_CMD_CLR          (1<<31)

//--------------------------------
// csi_config_reg02
//--------------------------------
#define CSI_NUM_C_TERM_EN(n)          (((n)&0XFFFF)<<16)
#define CSI_NUM_HS_SETTLE_CLK(n)          (((n)&0XFFFF)<<0)

//--------------------------------
// csi_config_reg03
//--------------------------------
#define CSI_DLY_SEL_REG(n)          (((n)&3)<<0)
#define CSI_DLY_SEL_DATA2_REG(n)          (((n)&3)<<2)
#define CSI_DLY_SEL_DATA1_REG(n)          (((n)&3)<<4)
#define CSI_CLK_LP_IN_CHOOSE_BIT(n)          (((n)&3)<<6)
#define CSI_PU_LPRX_REG        (1<<8)
#define CSI_PU_HSRX_REG        (1<<9)
#define CSI_PU_DR        (1<<10)
#define CSI_AVDD1V8_2V8_SEL_REG        (1<<11)
#define CSI_HS_CLK_ENABLE_REG        (1<<12)
#define CSI_HS_CLK_ENABLE_CHOOSE_BIT        (1<<13)
#define CSI_HS_CLK_ENALBE_DR        (1<<14)
#define CSI_CLK_TERMINAL_ENABLE_REG        (1<<15)
#define CSI_CLK_TERMINAL_ENABLE_DR        (1<<16)
#define CSI_OBSERVE_REG_5_LOW8_CHOOSE	(1 << 17)
#define CSI_ECC_ERROR_FLAG_REG	(1 << 18)
#define CSI_ECC_ERROR_DR (1 << 19)
#define CSI_CHANNEL_SEL (1 << 20)
#define CSI_TWO_LANE_BYTE_REVERSE (1 << 21)
#define CSI_DATA2_LANE_BIT_REVERSE (1 << 22)
#define CSI_DATA1_LANE_BIT_REVERSE (1 << 23)
#define CSI_DATA2_HAS_NO_MASK (1 << 24)
#define CSI_DATA1_HAS_NO_MASK (1 << 25)
#define CSI_PU_LRX_D2_REG (1 << 26)
#define CSI_PU_LRX_D1_REG (1 << 27)
#define CSI_PU_LRX_CLK_REG (1 << 28)
#define CSI_CLK_EDGE_SEL (1 << 29)
#define CSI_CLK_X2_SEL (1 << 30)
#define CSI_SINGLE_DATALANE_EN (1 << 31)

//--------------------------------
// csi_config_reg04,modified in 8850e by xiankuiwei
//--------------------------------
#define CSI_NUM_HS_CLK_USEFUL(n)          (((n)&0X7FFFFFFF)<<0)
#define CSI_NUM_HS_CLK_USEFUL_EN         (1<<31)

//--------------------------------
// csi_config_reg05
//--------------------------------
#define CSI_LP_CMD_OUT(n)          (((n)&0XFF)<<0)
#define CSI_PHY_CLK_STATE(n)          (((n)&0X1FF)<<10)
#define CSI_HS_DATA_ERROR_FLAG        (1<<28)
#define CSI_ERR_ECC_CORRECTED_FLAG        (1<<29)
#define CSI_ERR_DATA_CORRECTED_FLAG        (1<<30)
#define CSI_ERR_DATA_ZERO_FLAG        (1<<31)

//--------------------------------
// csi_observe_reg6 (read only)
//--------------------------------

//--------------------------------
// csi_observe_reg6 (read only)
//--------------------------------
#endif

