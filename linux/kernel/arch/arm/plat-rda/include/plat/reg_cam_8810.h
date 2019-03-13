#ifndef _CAMERA_8810_H_
#define _CAMERA_8810_H_

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
	REG32 DCT_CHOOSE_Y_0;			/* 0x90 */
	REG32 DCT_CHOOSE_Y_1;			/* 0x94 */
	REG32 DCT_SHIFTR_UV_0;			/* 0x98 */
	REG32 DCT_SHIFTR_UV_1;			/* 0x9C */
	REG32 DCT_CHOOSE_UV_0;			/* 0xA0 */
	REG32 DCT_CHOOSE_UV_1;			/* 0xA4 */
	REG32 CAM_DCT_CONFIG;			/* 0xA8 */
	REG32 CAM_AXI_CONFIG;			/* 0xAC */
	REG32 CAM_FRAME_START_ADDR;		/* 0xB0 */
	REG32 CAM_FRAME_SIZE;			/* 0xB4 */
	REG32 CAM_TC_COUNT;			/* 0xB8 */
	REG32 CAM_RESERVED[RESERVE_REGS];	/* 0xBC */

	struct {
		REG32 RAMData;			/* 0x200 */
	} FIFORAM[FIFORAM_SIZE];
} HWP_CAMERA_T;

/* CTRL */
#define CAMERA_ENABLE			(1<<0)
#define CAMERA_DCT_ENABLE		(1<<1)
#define CAMERA_BUF_ENABLE		(1<<2)

#define CAMERA_DATAFORMAT(n)		(((n)&3)<<4)
#define CAMERA_DATAFORMAT_RGB565	(0<<4)
#define CAMERA_DATAFORMAT_YUV422	(1<<4)
#define CAMERA_DATAFORMAT_JPEG		(2<<4)
#define CAMERA_DATAFORMAT_RESERVE	(3<<4)

#define CAMERA_RESET_POL_INVERT		(1<<8)
#define CAMERA_PWDN_POL_INVERT		(1<<9)
#define CAMERA_VSYNC_POL_INVERT		(1<<10)
#define CAMERA_HREF_POL_INVERT		(1<<11)
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

#endif
