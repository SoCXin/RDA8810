#ifndef _CAMERA_8850E_H_
#define _CAMERA_8850E_H_

#include <mach/hardware.h>
#include <mach/iomap.h>

#define FIFORAM_SIZE	(80)
#define RESERVE_REGS	(81)

#define REG_CAMERA_BASE	RDA_CAMERA_BASE

typedef volatile struct {
	REG32 CTRL;				/* 0x00 */
	REG32 STATUS;				/* 0x04 */
	REG32 DATA;				/* 0x08 */
	REG32 IRQ_MASK;				/* 0x0C */
	REG32 IRQ_CLEAR;			/* 0x10 */
	REG32 IRQ_CAUSE;			/* 0x14 */
	REG32 CMD_SET;				/* 0x18 */
	REG32 CMD_CLR;				/* 0x1C */
	REG32 DSTWINCOL;			/* 0x20 */
	REG32 DSTWINROW;			/* 0x24 */
	REG32 CLK_OUT;				/* 0x28 */
	REG32 SCALE_CFG;			/* 0x2C */
	REG32 CAM_SPI_REG_0;			/* 0x30 */
	REG32 CAM_SPI_REG_1;			/* 0x34 */
	REG32 CAM_SPI_REG_2;			/* 0x38 */
	REG32 CAM_SPI_REG_3;			/* 0x3C */
	REG32 CAM_SPI_REG_4;			/* 0x40 */
	REG32 CAM_SPI_REG_5;			/* 0x44 */
	REG32 CAM_SPI_REG_6;			/* 0x48 */
	REG32 CAM_SPI_OBS_0;			/* 0x4C, RO */
	REG32 CAM_SPI_OBS_1;			/* 0x50, RO */
	REG32 CAM_CSI_REG_0;			/* 0x54 */
	REG32 CAM_CSI_REG_1;			/* 0x58 */
	REG32 CAM_CSI_REG_2;			/* 0x5C */
	REG32 CAM_CSI_REG_3;			/* 0x60 */
	REG32 CAM_CSI_REG_4;			/* 0x64 */
	REG32 CAM_CSI_REG_5;			/* 0x68 */
	REG32 CAM_CSI_REG_6;			/* 0x6C */
	REG32 CAM_CSI_REG_7;			/* 0x70 */
	REG32 CAM_CSI_OBS_4;			/* 0x74 */
	REG32 CAM_CSI_OBS_5;			/* 0x78 */
	REG32 CAM_CSI_OBS_6;			/* 0x7C */
	REG32 CAM_CSI_OBS_7;			/* 0x80 */
	REG32 CAM_CSI_ENABLE;			/* 0x84 */
	REG32 DCT_SHIFTR_Y_0;			/* 0x88 */
	REG32 DCT_SHIFTR_Y_1;			/* 0x8C */
	REG32 CAM_AXI_CONFIG;			/* 0x90 */
	REG32 CAM_FRAME_START_ADDR;		/* 0x94 */
	REG32 CAM_FRAME_SIZE;			/* 0x98 */
	REG32 CAM_TC_COUNT;			/* 0x9C */
	REG32 CAM_FRAME_START_UADDR;		/* 0xA0 */
	REG32 CAM_FRAME_START_VADDR;		/* 0xA4 */
	REG32 CAM_FRAME2_START_ADDR;		/* 0xA8 */
	REG32 CAM_FRAME2_START_UADDR;		/* 0xAC */
	REG32 CAM_FRAME2_START_VADDR;		/* 0xB0 */
	REG32 CAM_CFG_CAM_C2CSE;		/* 0xB4 */

	REG32 CAM_RESERVED[RESERVE_REGS];	/* 0xB8 */

	struct {
		REG32 RAMData;			/* 0x1FC */
	} FIFORAM[FIFORAM_SIZE];
} HWP_CAMERA_T;

/* CTRL */
#define CAMERA_ENABLE			(1<<0)
#define CAMERA_DCT_ENABLE		(1<<1)
#define CAMERA_BUF_ENABLE		(1<<2)
#define CAMERA_OCG_ENABLE		(1<<3) /* Out clk gating enable */

#define CAMERA_DATAFORMAT(n)		(((n)&3)<<4)
#define CAMERA_DATAFORMAT_RGB565	(0<<4)
#define CAMERA_DATAFORMAT_YUV422	(1<<4)
#define CAMERA_DATAFORMAT_JPEG		(2<<4)
#define CAMERA_DATAFORMAT_RESERVE	(3<<4)

#define CAMERA_ISP_ENABLE		(1<<6)
#define CAMERA_RGB_RFIRST		(1<<7)
#define CAMERA_RESET_POL_INVERT		(1<<8)
#define CAMERA_PWDN_POL_INVERT		(1<<9)
#define CAMERA_VSYNC_POL_INVERT		(1<<10)
#define CAMERA_HREF_POL_INVERT		(1<<11)
#define CAMERA_LINEBUF_ISP		(1<<12)
#define CAMERA_VSYNC_DROP		(1<<14)

#define CAMERA_DECIM_FRM(n)		(((n)&3)<<16)
#define CAMERA_DECIM_FRM_ORG_0		(0<<16)
#define CAMERA_DECIM_FRM_DIV_2		(1<<16)
#define CAMERA_DECIM_FRM_DIV_3		(2<<16)
#define CAMERA_DECIM_FRM_DIV_4		(3<<16)
#define CAMERA_DECIM_COL(n)		(((n)&3)<<18)
#define CAMERA_DECIM_COL_ORG_0		(0<<18)
#define CAMERA_DECIM_COL_DIV_2		(1<<18)
#define CAMERA_DECIM_COL_DIV_3		(2<<18)
#define CAMERA_DECIM_COL_DIV_4		(3<<18)
#define CAMERA_DECIM_ROW(n)		(((n)&3)<<20)
#define CAMERA_DECIM_ROW_ORG_0		(0<<20)
#define CAMERA_DECIM_ROW_DIV_2		(1<<20)
#define CAMERA_DECIM_ROW_DIV_3		(2<<20)
#define CAMERA_DECIM_ROW_DIV_4		(3<<20)

#define CAMERA_REORDER(n)		(((n)&7)<<24)
#define CAMERA_REORDER_YUYV		(0<<24)
#define CAMERA_REORDER_YVYU		(1<<24)
#define CAMERA_REORDER_UYVY		(6<<24)
#define CAMERA_REORDER_VYUY		(7<<24)

#define CAMERA_CROP_ENABLE		(1<<28)
#define CAMERA_BIST_MODE		(1<<30)
#define CAMERA_TEST			(1<<31)

/* STATUS */
#define CAMERA_OVFL			(1<<0)
#define CAMERA_VSYNC_R			(1<<1)
#define CAMERA_VSYNC_F			(1<<2)
#define CAMERA_DMA_DONE			(1<<3)
#define CAMERA_FIFO_EMPTY		(1<<4)
#define CAMERA_SPI_OVFL			(1<<5)

/* DATA */
#define CAMERA_RX_DATA(n)		(((n)&0xFFFFFFFF)<<0)

/* IRQ_MASK, IRQ_CLEAR, IRQ_CAUSE use macros in STATUS */
#define IRQ_OVFL			CAMERA_OVFL
#define IRQ_VSYNC_R			CAMERA_VSYNC_R
#define IRQ_VSYNC_F			CAMERA_VSYNC_F
#define IRQ_DMADONE			CAMERA_DMA_DONE
#define IRQ_MASKALL			IRQ_OVFL |	\
					IRQ_VSYNC_R |	\
					IRQ_VSYNC_F |	\
					IRQ_DMADONE

/* CMD_SET & CMD_CLR */
#define CAMERA_PWDN			(1<<0)
#define CAMERA_RESET			(1<<4)
#define CAMERA_FIFO_RESET		(1<<8)

/* DSTWINCOL */
#define CAMERA_DSTWINCOL_START(n)	(((n)&0xFFF)<<0)
#define CAMERA_DSTWINCOL_END(n)		(((n)&0xFFF)<<16)

/* DSTWINROW */
#define CAMERA_DSTWINROW_START(n)	(((n)&0xFFF)<<0)
#define CAMERA_DSTWINROW_END(n)		(((n)&0xFFF)<<16)

/* CLK_OUT */
#define CAMERA_CLK_OUT_ENABLE		(1<<0)
#define CAMERA_CLK_SRC_DIV		(0<<4)
#define CAMERA_CLK_SRC_26M		(1<<4)
#define CAMERA_CLK_OUT_DIV(n)		(((n)&0xF)<<8)
#define CAMERA_CLK_OUT_SRC_DIV		(0<<12)
#define CAMERA_CLK_OUT_SRC_32K		(1<<12)
#define CAMERA_CLK_OUT_SRC_26M		(2<<12)
#define CAMERA_PIXCLK_POL_INVERT	(1<<28)

/* SCALE CONFIG */
#define CAMERA_SCALE_ENABLE		(1<<0)
#define CAMERA_SCALE_COL(n)		(((n)&0x3)<<8)
#define CAMERA_SCALE_ROW(n)		(((n)&0x3)<<16)

/* AXI CONFIG */
#define CAMERA_AXI_START		(1<<5)
#define CAMERA_AXI_SWDIS		(1<<29)
#define CAMERA_AXI_AUTO_DIS		(1<<28)
#define CAMERA_AXI_PINGPONG		(1<<10)
#define CAMERA_AXI_YUVPLANE		(1<<6)
#define CAMERA_AXI_COL(n)		(((n)&0xfff)<<16)
#define CAMERA_AXI_BURST(n)		((n)&0xf)

/* RAMData */
#define CAMERA_DATA(n)			(((n)&0xFFFFFFFF)<<0)
#define CAMERA_DATA_MASK		(0xFFFFFFFF<<0)
#define CAMERA_DATA_SHIFT		(0)

/*************************************************************/
/***************** SPI CAMERA CONFIGURATION ********************/
/*************************************************************/

/* Here, spi master mode means the sensor works as the SPI master.
   master mode 1 means 1 data ouput with SSN, master mode 2 means
   the other master modes. */

/* camera_spi_reg_0 */
/*  the number of lines per frame */
#define CAM_SPI_REG_LINE_PER_FRM(n)		(((n)&0x3ff)<<22)
/*  the number of words(32 bits) per line  */
#define CAM_SPI_REG_BLK_PER_LINE(n)		(((n)&0x3ff)<<12)
/*  VSYNC high effective  */
#define CAM_SPI_REG_VSYNC_INV_EN		(1<<11)
/*  HREF low effective */
#define CAM_SPI_REG_HREF_INV_EN			(1<<10)
/*  OVFL low effective */
#define CAM_SPI_REG_OVFL_INV_EN			(1<<9)
/*  little endian enable */
#define CAM_SPI_REG_LITTLE_END_EN		(1<<8)
/*  module reset when ovfl enable */
#define CAM_SPI_REG_OVFL_RST_EN			(1<<7)
/*  observe the overflow when vsync effective */
#define CAM_SPI_REG_OVFL_OBS			(1<<6)
/*  reset the module when overflow in vsync effective */
#define CAM_SPI_REG_OVFL_RST			(1<<5)
/************************************************
YUV data output format
        3'b000: Y0-U0-Y1-V0                3'b001: Y0-V0-Y1-U0
        3'b010: U0-Y0-V0-Y1                3'b011: U0-Y1-V0-Y0
        3'b100: V0-Y1-U0-Y0                3'b101: V0-Y0-U0-Y1
        3'b110: Y1-V0-Y0-U0                3'b111: Y1-U0-Y0-V0
*************************************************/
#define CAM_SPI_REG_YUV_OUT_FMT(n)		(((n)&0x7)<<2)
/*  SPI master mode 1 enable */
#define CAM_SPI_REG_MASTER_EN			(1<<1)
/*  SPI slave mode enable */
#define CAM_SPI_REG_SLAVE_EN			(1<<0)

/* camera_spi_reg_1 */
/* camera clock divider */
#define CAM_SPI_REG_CLK_DIV(n)			((n)&0xffff)
/* SSN high enable, only for master mode 1 */
#define CAM_SPI_REG_SSN_HIGH_EN			(1<<17)

/* camera_spi_reg_2 */
/*  only take effect in slave mode which is not supported yet */

/* camera_spi_reg_3 */
/*  only take effect in slave mode which is not supported yet */

/* camera_spi_reg_4 */
#define CAM_SPI_REG_BLK_PER_PACK(n)		(((n)&0x3ff)<<6)
#define CAM_SPI_REG_IMG_WIDTH_FROM_REG		(1<<5)
#define CAM_SPI_REG_IMG_HEIGHT_FROM_REG		(1<<4)
#define CAM_SPI_REG_PACK_SIZE_FROM_REG		(1<<3)
#define CAM_SPI_REG_LINE(n)			(((n)&0x3)<<1)
#define CAM_SPI_REG_MASTER2_EN			(1<<0)

/* camera_spi_reg_5 */
#define CAM_SPI_REG_SYNC_CODE(n)		((n)&0xffffff)

/* camera_spi_reg_6 */
/*  frame_start packet id, only in master mode 2 */
#define CAM_SPI_REG_FRM_START_PKT_ID(n)		(((n)&0xff)<<24)
/*  frame_end packet id, only in master mode 2 */
#define CAM_SPI_REG_FRM_ENDN_PKT_ID(n)		(((n)&0xff)<<16)
/*  line_start packet id, only in master mode 2 */
#define CAM_SPI_REG_LINE_START_PKT_ID(n)	(((n)&0xff)<<8)
/*  data packet id, only in master mode 2 */
#define CAM_SPI_REG_DATA_PKT_ID(n)		((n)&0xff)

/* camera_spi_observe_reg_0 (read only) */

/* camera_spi_observe_reg_1 (read only) */

#define REG_ISP_OFFS 0x400
#define REG_ISP_SIZE 0x3FF

typedef volatile struct {
	REG32 Y_ANA_OFFSET;			/* 0x82=0x608 */
	REG32 Y_LMT_OFFSET;			/* 0x83=0x60C */
	REG32 Y_CTRL_MISC;			/* 0x84=0x610 */

	REG32 AE_CTRL2;				/* 0x85=0x614 */
	REG32 AE_CTRL3;				/* 0x86=0x618 */
	REG32 AE_CTRL4;				/* 0x87=0x61C */
	REG32 AE_EXP_OFFSET_H;			/* 0x88=0x620 */
	REG32 G3_EXP_POFST;			/* 0x89=0x624 */
	REG32 G3_EXP_NOFST;			/* 0x8A=0x628 */
	REG32 AE_XGAP1;				/* 0x8B=0x62C */
	REG32 AE_XGAP2;				/* 0x8C=0x630 */
	REG32 AE_YGAP1;				/* 0x8D=0x634 */
	REG32 AE_YGAP2;				/* 0x8E=0x638 */
	REG32 AE_HIST_TOPLEFT;			/* 0x8F=0x63C */
	REG32 AE_DK_HIST_THR;			/* 0x90=0x640 */
	REG32 AE_BR_HIST_THR;			/* 0x91=0x644 */
	REG32 IMAGE_EFF;			/* 0x92=0x648 */
	REG32 LUMA_OFFSET;			/* 0x93=0x64C */
	REG32 SEPIA_CR;				/* 0x94=0x650 */
	REG32 SEPIA_CB;				/* 0x95=0x654 */
	REG32 CSUP_Y_MIN_H;			/* 0x96=0x658 */
	REG32 CSUP_GAIN_H;			/* 0x97=0x65C */
	REG32 CSUP_Y_MAX_L;			/* 0x98=0x660 */
	REG32 CSUP_GAIN_L;			/* 0x99=0x664 */
	REG32 AE_DRC_GAIN_IN;			/* 0x9A=0x668 */
	REG32 AE_DARK_HIST;			/* 0x9B=0x66C */
	REG32 AE_BRIGHT_HIST;			/* 0x9C=0x670 */
	REG32 YWAVE_OUT;			/* 0x9D=0x674 */
	REG32 YAVE_OUT;				/* 0x9E=0x678 */
	REG32 EXP_OUT;				/* 0x9F=0x67C */
	REG32 GAIN_EXP_OUT;			/* 0xA0=0x680 */
	REG32 AWB_DEBUG_OUT;			/* 0xA1=0x684 */
	REG32 AE_DRC_GAIN;			/* 0xA2=0x688 */
	REG32 R_AWB_GAIN;			/* 0xA3=0x68C */
	REG32 B_AWB_GAIN;			/* 0xA4=0x690 */
	REG32 AWB_R_PEDEST;			/* 0xA5=0x694 */
	REG32 AWB_G_PEDEST;			/* 0xA6=0x698 */
	REG32 AWB_B_PEDEST;			/* 0xA7=0x69C */

	REG32 LSC_CTRL;				/* 0xA8=0x6A0 */
	REG32 LSC_X_CENT;			/* 0xA9=0x6A4 */
	REG32 LSC_Y_CENT;			/* 0xAA=0x6A8 */
	REG32 BLC_CTRL;				/* 0xAB=0x6AC */
	REG32 BLC_INIT;				/* 0xAC=0x6B0 */
	REG32 BLC_OFFSET;			/* 0xAD=0x6B4 */
	REG32 BLC_THR;				/* 0xAE=0x6B8 */
	REG32 DRC_CLAMP_CTRL;			/* 0xAF=0x6BC */
	REG32 DRC_R_CLP_VALUE;			/* 0xB0=0x6C0 */
	REG32 DRC_GR_CLP_VALUE;			/* 0xB1=0x6C4 */
	REG32 DRC_GB_CLP_VALUE;			/* 0xB2=0x6C8 */
	REG32 DRC_B_CLP_VALUE;			/* 0xB3=0x6CC */

	REG32 GAMMA_CTRL;			/* 0xB4=0x6D0 */
	REG32 BAYER_GAMMA_B0;			/* 0xB5=0x6D4 */
	REG32 BAYER_GAMMA_B1;			/* 0xB6=0x6D8 */
	REG32 BAYER_GAMMA_B2;			/* 0xB7=0x6DC */
	REG32 BAYER_GAMMA_B3;			/* 0xB8=0x6E0 */
	REG32 BAYER_GAMMA_B4;			/* 0xB9=0x6E4 */
	REG32 BAYER_GAMMA_B5;			/* 0xBA=0x6E8 */
	REG32 BAYER_GAMMA_B6;			/* 0xBB=0x6EC */
	REG32 BAYER_GAMMA_B8;			/* 0xBC=0x6F0 */
	REG32 BAYER_GAMMA_B10;			/* 0xBD=0x6F4 */
	REG32 BAYER_GAMMA_B12;			/* 0xBE=0x6F8 */
	REG32 BAYER_GAMMA_B14;			/* 0xBF=0x6FC */
	REG32 BAYER_GAMMA_B16;			/* 0xC0=0x700 */
	REG32 BAYER_GAMMA_B18;			/* 0xC1=0x704 */
	REG32 BAYER_GAMMA_B20;			/* 0xC2=0x708 */
	REG32 BAYER_GAMMA_B24;			/* 0xC3=0x70C */
	REG32 BAYER_GAMMA_B28;			/* 0xC4=0x710 */
	REG32 BAYER_GAMMA_B32;			/* 0xC5=0x714 */
	REG32 LSC_P2_UP_R;			/* 0xC6=0x718 */
	REG32 LSC_P2_UP_G;			/* 0xC7=0x71C */
	REG32 LSC_P2_UP_B;			/* 0xC8=0x720 */
	REG32 LSC_P2_DOWN_R;			/* 0xC9=0x724 */
	REG32 LSC_P2_DOWN_G;			/* 0xCA=0x728 */
	REG32 LSC_P2_DOWN_B;			/* 0xCB=0x72C */
	REG32 LSC_P2_LEFT_R;			/* 0xCC=0x730 */
	REG32 LSC_P2_LEFT_G;			/* 0xCD=0x734 */
	REG32 LSC_P2_LEFT_B;			/* 0xCE=0x738 */
	REG32 LSC_P2_RIGHT_R;			/* 0xCF=0x73C */
	REG32 LSC_P2_RIGHT_G;			/* 0xD0=0x740 */
	REG32 LSC_P2_RIGHT_B;			/* 0xD1=0x744 */
	REG32 LSC_P4_Q1;			/* 0xD2=0x748 */
	REG32 LSC_P4_Q2;			/* 0xD3=0x74C */
	REG32 LSC_P4_Q3;			/* 0xD4=0x750 */
	REG32 LSC_P4_Q4;			/* 0xD5=0x754 */
	REG32 BLC_OUT0;				/* 0xD6=0x758 */
	REG32 BLC_OUT1;				/* 0xD7=0x75C */
	REG32 DPC_CTRL0;			/* 0xD8=0x760 */
	REG32 DPC_CTRL1;			/* 0xD9=0x764 */
	REG32 DPC_DEAD_THR;			/* 0xDA=0x768 */
	REG32 DPC_HOT_THR;			/* 0xDB=0x76C */
	REG32 DPC_LUM_THR;			/* 0xDC=0x770 */
	REG32 DPC_NUM_DEFECT;			/* 0xDD=0x774 */
	REG32 DPC_Y_THR_DATA;			/* 0xDE=0x778 */
	REG32 DPC_AU1;				/* 0xDF=0x77C */
	REG32 DPC_AU2;				/* 0xE0=0x780 */
	REG32 DPC_AU3;				/* 0xE1=0x784 */
	REG32 DPC_INT_THR;			/* 0xE2=0x788 */
	REG32 DPC_NRF_THR1;			/* 0xE3=0x78C */
	REG32 DPC_NRF_THR2;			/* 0xE4=0x790 */
	REG32 DPC_NRF_WEI_HI;			/* 0xE5=0x794 */
	REG32 DPC_NRF_WEI_MID;			/* 0xE6=0x798 */
	REG32 DPC_NRF_WEI_LOW;			/* 0xE7=0x79C */
	REG32 DPC_NR_LF_STR;			/* 0xE8=0x7A0 */
	REG32 DPC_NR_HF_STR;			/* 0xE9=0x7A4 */
	REG32 DPC_NR_AREA_THR;			/* 0xEA=0x7A8 */
	REG32 INTP_CTRL;			/* 0xEB=0x7AC */
	REG32 INTP_CFA_H_THR;			/* 0xEC=0x7B0 */
	REG32 INTP_CFA_V_THR;			/* 0xED=0x7B4 */
	REG32 INTP_GRGB_SEL_LMT;		/* 0xEE=0x7B8 */
	REG32 INTP_GF_LMT_THR;			/* 0xEF=0x7BC */
	REG32 CC_R_OFFSET;			/* 0xF0=0x7C0 */
	REG32 CC_G_OFFSET;			/* 0xF1=0x7C4 */
	REG32 CC_B_OFFSET;			/* 0xF2=0x7C8 */
	REG32 CC_00;				/* 0xF3=0x7CC */
	REG32 CC_01;				/* 0xF4=0x7D0 */
	REG32 CC_02;				/* 0xF5=0x7D4 */
	REG32 CC_10;				/* 0xF6=0x7D8 */
	REG32 CC_11;				/* 0xF7=0x7DC */
	REG32 CC_12;				/* 0xF8=0x7E0 */
	REG32 CC_20;				/* 0xF9=0x7E4 */
	REG32 CC_21;				/* 0xFA=0x7E8 */
	REG32 CC_22;				/* 0xFB=0x7EC */
	REG32 INTP_CFA_HV;			/* 0xFC=0x7F0 */
	REG32 AE_VDARK_HIST;			/* 0xFD=0x7F4 */
	REG32 AE_VBRIGHT_HIST;			/* 0xFE=0x7F8 */
	REG32 Y_THR7;				/* 0xFF=0x7FC */
} ISP_PAGE0_T;

typedef volatile struct {
	REG32 EE_CTRL;				/* 0x82=0x608 */
	REG32 EE_HL_THR;			/* 0x83=0x60C */
	REG32 EE_LL_THR;			/* 0x84=0x610 */
	REG32 EE_EDGE_GAIN;			/* 0x85=0x614 */
	REG32 EE_H_LF_STR;			/* 0x86=0x618 */
	REG32 EE_H_AREA_THR;			/* 0x87=0x61C */
	REG32 EE_L_LF_STR;			/* 0x88=0x620 */
	REG32 EE_L_AREA_THR;			/* 0x89=0x624 */
	REG32 EE_H_MINUS_STR;			/* 0x8A=0x628 */
	REG32 EE_H_PLUS_STR;			/* 0x8B=0x62C */
	REG32 EE_M_MINUS_STR;			/* 0x8C=0x630 */
	REG32 EE_M_PLUS_STR;			/* 0x8D=0x634 */
	REG32 EE_L_MINUS_STR;			/* 0x8E=0x638 */
	REG32 EE_L_PLUS_STR;			/* 0x8F=0x63C */
	REG32 EE_B0;				/* 0x90=0x640 */
	REG32 EE_B1;				/* 0x91=0x644 */
	REG32 EE_B2;				/* 0x92=0x648 */
	REG32 EE_B3;				/* 0x93=0x64C */
	REG32 EE_B4;				/* 0x94=0x650 */
	REG32 EE_B5;				/* 0x95=0x654 */
	REG32 EE_B6;				/* 0x96=0x658 */
	REG32 EE_B7;				/* 0x97=0x65C */
	REG32 EE_B8;				/* 0x98=0x660 */
	REG32 EE_B9;				/* 0x99=0x664 */
	REG32 EE_B10;				/* 0x9A=0x668 */
	REG32 EE_B11;				/* 0x9B=0x66C */
	REG32 EE_B12;				/* 0x9C=0x670 */
	REG32 EE_B13;				/* 0x9D=0x674 */
	REG32 EE_B14;				/* 0x9E=0x678 */
	REG32 EE_B15;				/* 0x9F=0x67C */
	REG32 EE_B16;				/* 0xA0=0x680 */
	REG32 EE_INT_Y_THR;			/* 0xA1=0x684 */

	REG32 YCNR_CTRL;			/* 0xA2=0x688 */
	REG32 CNR_2ST_STR;			/* 0xA3=0x68C */
	REG32 YNR_LL_AREA_THR;			/* 0xA4=0x690 */
	REG32 YNR_LL_HF_STR;			/* 0xA5=0x694 */
	REG32 YNR_LL_LF_STR;			/* 0xA6=0x698 */
	REG32 YNR_LL_LF_METHOD_STR;		/* 0xA7=0x69C */
	REG32 YNR_HL_AREA_THR;			/* 0xA8=0x6A0 */
	REG32 YNR_HL_HF_STR;			/* 0xA9=0x6A4 */
	REG32 YNR_HL_LF_STR;			/* 0xAA=0x6A8 */
	REG32 YNR_HL_LF_METHOD_STR;		/* 0xAB=0x6AC */
	REG32 YNR_1ST_LL_THR;			/* 0xAC=0x6B0 */
	REG32 YNR_1ST_HL_THR;			/* 0xAD=0x6B4 */
	REG32 YNR_2ND_AREA_THR;			/* 0xAE=0x6B8 */
	REG32 YNR_2ND_HF_STR;			/* 0xAF=0x6BC */
	REG32 YNR_2ND_LF_STR;			/* 0xB0=0x6C0 */
	REG32 YNR_2ND_LF_METHOD_STR;		/* 0xB1=0x6C4 */
	REG32 YNR_2ND_HF_METHOD_STR;		/* 0xB2=0x6C8 */

	REG32 RGB_GAMMA_B0;			/* 0xB3=0x6CC */
	REG32 RGB_GAMMA_B1;			/* 0xB4=0x6D0 */
	REG32 RGB_GAMMA_B2;			/* 0xB5=0x6D4 */
	REG32 RGB_GAMMA_B3;			/* 0xB6=0x6D8 */
	REG32 RGB_GAMMA_B4;			/* 0xB7=0x6DC */
	REG32 RGB_GAMMA_B6;			/* 0xB8=0x6E0 */
	REG32 RGB_GAMMA_B8;			/* 0xB9=0x6E4 */
	REG32 RGB_GAMMA_B10;			/* 0xBA=0x6E8 */
	REG32 RGB_GAMMA_B12;			/* 0xBB=0x6EC */
	REG32 RGB_GAMMA_B16;			/* 0xBC=0x6F0 */
	REG32 RGB_GAMMA_B20;			/* 0xBD=0x6F4 */
	REG32 RGB_GAMMA_B24;			/* 0xBE=0x6F8 */
	REG32 RGB_GAMMA_B28;			/* 0xBF=0x6FC */
	REG32 RGB_GAMMA_B32;			/* 0xC0=0x700 */
	REG32 RGB_GAMMA_B40;			/* 0xC1=0x704 */
	REG32 RGB_GAMMA_B48;			/* 0xC2=0x708 */
	REG32 RGB_GAMMA_B56;			/* 0xC3=0x70C */
	REG32 RGB_GAMMA_B64;			/* 0xC4=0x710 */
	REG32 RGB_GAMMA_B80;			/* 0xC5=0x714 */
	REG32 RGB_GAMMA_B96;			/* 0xC6=0x718 */
	REG32 RGB_GAMMA_B112;			/* 0xC7=0x71C */
	REG32 RGB_GAMMA_B128;			/* 0xC8=0x720 */

	REG32 CONST_REG;			/* 0xC9=0x724 */
	REG32 SATUR_REG;			/* 0xCA=0x728 */
	REG32 CONST_SATUR_OFF;			/* 0xCB=0x72C */
	REG32 EXP_CHG_0;			/* 0xCC=0x730 */
	REG32 EXP_CHG_1;			/* 0xCD=0x734 */
	REG32 HIST_DP_LEVEL;			/* 0xCE=0x738 */
	REG32 HIST_BP_LEVEL;			/* 0xCF=0x73C */
	REG32 HIST_DP_NUM;			/* 0xD0=0x740 */
	REG32 HIST_BP_NUM;			/* 0xD1=0x744 */
	REG32 HIST_VDP_LEVEL;			/* 0xD2=0x748 */
	REG32 HIST_VBP_LEVEL;			/* 0xD3=0x74C */
	REG32 HIST_VDP_NUM;			/* 0xD4=0x750 */
	REG32 HIST_VBP_NUM;			/* 0xD5=0x754 */
	REG32 EE_INT_CTRL;			/* 0xD6=0x758 */
	REG32 YNR_INT_Y_THR;			/* 0xD7=0x75C */
	REG32 YNR_INT_CTRL;			/* 0xD8=0x760 */
	REG32 EXP_CTRL;				/* 0xD9=0x764 */
	REG32 CNR_1D_CTRL;			/* 0xDA=0x768 */
	REG32 CNR_1D_STR;			/* 0xDB=0x76C */
	REG32 CHECK_G_DIFF;			/* 0xDC=0x770 */
	REG32 CC_R_OFFSET;			/* 0xDD=0x774 */
	REG32 CC_G_OFFSET;			/* 0xDE=0x778 */
	REG32 CC_B_OFFSET;			/* 0xDF=0x77C */
	REG32 G0_EXP_LOW;			/* 0xE0=0x780 */
	REG32 G0_EXP_HIGH;			/* 0xE1=0x784 */
	REG32 G0_EXP_HIGH_LOW;			/* 0xE2=0x788 */
	REG32 G1_EXP_LOW;			/* 0xE3=0x78C */
	REG32 G1_EXP_HIGH;			/* 0xE4=0x790 */
	REG32 G1_EXP_HIGH_LOW;			/* 0xE5=0x794 */
	REG32 G2_EXP_LOW;			/* 0xE6=0x798 */
	REG32 G2_EXP_HIGH;			/* 0xE7=0x79C */
	REG32 G2_EXP_HIGH_LOW;			/* 0xE8=0x7A0 */
	REG32 G3_EXP_LOW;			/* 0xE9=0x7A4 */
	REG32 G3_EXP_HIGH;			/* 0xEA=0x7A8 */
	REG32 G3_EXP_HIGH_LOW;			/* 0xEB=0x7AC */
	REG32 G4_EXP_LOW;			/* 0xEC=0x7B0 */
	REG32 G4_EXP_HIGH;			/* 0xED=0x7B4 */
	REG32 G4_EXP_HIGH_LOW;			/* 0xEE=0x7B8 */
	REG32 G1_EXP_INIT_M1;			/* 0xEF=0x7BC */
	REG32 G1_EXP_INIT_P1;			/* 0xF0=0x7C0 */
	REG32 G1_EXP_INIT;			/* 0xF1=0x7C4 */
	REG32 G2_EXP_INIT_M1;			/* 0xF2=0x7C8 */
	REG32 G2_EXP_INIT_P1;			/* 0xF3=0x7CC */
	REG32 G2_EXP_INIT;			/* 0xF4=0x7D0 */
	REG32 G3_EXP_INIT_M1;			/* 0xF5=0x7D4 */
	REG32 G3_EXP_INIT_P1;			/* 0xF6=0x7D8 */
	REG32 G3_EXP_INIT;			/* 0xF7=0x7DC */
	REG32 G0_EXP_INIT_M1;			/* 0xF8=0x7E0 */
	REG32 G4_EXP_INIT_P1;			/* 0xF9=0x7E4 */
	REG32 G04_EXP_INIT;			/* 0xFA=0x7E8 */
	REG32 SHAPE_DIFF_THR;			/* 0xFB=0x7EC */
	REG32 CHECK_ROUND_DIFF;			/* 0xFC=0x7F0 */
	REG32 Y_THR6_REG;			/* 0xFD=0x7F4 */
	REG32 AE_CTRL_5;			/* 0xFE=0x7F8 */
	REG32 RESERVE;				/* 0xFF=0x7FC */
} ISP_PAGE1_T;

typedef volatile struct {
	REG32 ISP_SOFT_RST;			/* 0x00=0x400 */
	REG32 ISP_RESERVE0[13];			/* 0x01 ~ 0x0D */
	REG32 ISP_PAGE;				/* 0x0E=0x438 */
	REG32 ISP_RESERVE1[14];			/* 0x0F ~ 0x1C */
	REG32 ISP_SUB_MODE;			/* 0x1D=0x474 */
	REG32 ISP_RESERVE2[41];			/* 0x1E ~ 0x46 */
	REG32 ISP_CTRL0;			/* 0x47=0x51C */
	REG32 ISP_CTRL1;			/* 0x48=0x520 */
	REG32 ISP_CTRL2;			/* 0x49=0x524 */

	REG32 TOP_DUMMY_REG0;			/* 0x4A=0x528 */
	REG32 LEFT_DUMMY_REG0;			/* 0x4B=0x52C */
	REG32 LINE_NUM_1_REG0;			/* 0x4C=0x530 */
	REG32 PIX_NUM_1_REG0;			/* 0x4D=0x534 */
	REG32 LINE_PIX_H_REG0;			/* 0x4E=0x538 */
	REG32 V_DUMMY_REG0;			/* 0x4F=0x53C */

	REG32 TOP_DUMMY_REG1;			/* 0x50=0x540 */
	REG32 LEFT_DUMMY_REG1;			/* 0x51=0x544 */
	REG32 LINE_NUM_1_REG1;			/* 0x52=0x548 */
	REG32 PIX_NUM_1_REG1;			/* 0x53=0x54C */
	REG32 LINE_PIX_H_REG1;			/* 0x54=0x550 */
	REG32 V_DUMMY_REG1;			/* 0x55=0x554 */

	REG32 SCG_REG;				/* 0x56=0x558 */
	REG32 Y_GAMMA_B0_REG;			/* 0x57=0x55C */
	REG32 Y_GAMMA_B1_REG;			/* 0x58=0x560 */
	REG32 Y_GAMMA_B2_REG;			/* 0x59=0x564 */
	REG32 Y_GAMMA_B4_REG;			/* 0x5A=0x568 */
	REG32 Y_GAMMA_B6_REG;			/* 0x5B=0x56C */
	REG32 Y_GAMMA_B8_REG;			/* 0x5C=0x570 */
	REG32 Y_GAMMA_B10_REG;			/* 0x5D=0x574 */
	REG32 Y_GAMMA_B12_REG;			/* 0x5E=0x578 */
	REG32 Y_GAMMA_B16_REG;			/* 0x5F=0x57C */
	REG32 Y_GAMMA_B20_REG;			/* 0x60=0x580 */
	REG32 Y_GAMMA_B24_REG;			/* 0x61=0x584 */
	REG32 Y_GAMMA_B28_REG;			/* 0x62=0x588 */
	REG32 Y_GAMMA_B32_REG;			/* 0x63=0x58C */

	REG32 R_AWB_GAIN_IN_REG;		/* 0x64=0x590 */
	REG32 G_AWB_GAIN_IN_REG;		/* 0x65=0x594 */
	REG32 B_AWB_GAIN_IN_REG;		/* 0x66=0x598 */
	REG32 R_DRC_GAIN_IN_REG;		/* 0x67=0x59C */
	REG32 GR_DRC_GAIN_IN_REG;		/* 0x68=0x5A0 */
	REG32 GB_DRC_GAIN_IN_REG;		/* 0x69=0x5A4 */
	REG32 B_DRC_GAIN_IN_REG;		/* 0x6A=0x5A8 */

	REG32 AE_CTRL_REG;			/* 0x6B=0x5AC */
	REG32 AE_ANA_GAIN_H_L_REG;		/* 0x6C=0x5B0 */
	REG32 AE_LCNT_TOP_REG;			/* 0x6D=0x5B4 */
	REG32 AE_PCNT_TOP_REG;			/* 0x6E=0x5B8 */
	REG32 AE_EXP_LOW_REG;			/* 0x6F=0x5BC */
	REG32 AE_EXP_HIGH_REG;			/* 0x70=0x5C0 */
	REG32 AE_EXP_HIGH_LOW_REG;		/* 0x71=0x5C4 */
	REG32 AE_EXP_INIT_REG;			/* 0x72=0x5C8 */
	REG32 AE_EXP_CEIL_REG;			/* 0x73=0x5CC */
	REG32 AE_EXP_CEIL_INIT_REG;		/* 0x74=0x5D0 */
	REG32 AE_EXP_POFFSET_L;			/* 0x75=0x5D4 */
	REG32 AE_EXP_NOFFSET_L;			/* 0x76=0x5D8 */

	REG32 AWB_CTRL_REG;			/* 0x77=0x5DC */
	REG32 AWB_DRC_REG;			/* 0x78=0x5E0 */
	REG32 AWB_STOP_REG;			/* 0x79=0x5E4 */
	REG32 AE_EXP_BTHR_REG;			/* 0x7A=0x5E8 */
	REG32 AWB_ALGO_REG;			/* 0x7B=0x5EC */
	REG32 AWB_DRC_GAIN_H_REG;		/* 0x7C=0x5F0 */
	REG32 AWB_DRC_GAIN_L_REG;		/* 0x7D=0x5F4 */
	REG32 AWB_GAIN_H_REG;			/* 0x7E=0x5F8 */
	REG32 AWB_GAIN_L_REG;			/* 0x7F=0x5FC */

	REG32 Y_AVE_TARGET_REG;			/* 0x80=0x600 */
	REG32 Y_DRC_OFFSET_REG;			/* 0x81=0x604 */

	union {
		ISP_PAGE0_T ISP_PAGE0;
		ISP_PAGE1_T ISP_PAGE1;
	} PAGE_X;
} ISP_CAMERA_T;

#define ISP_START_AE		(0x1<<6)
#endif
