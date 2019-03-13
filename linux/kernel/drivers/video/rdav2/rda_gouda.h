/*
    RDA gouda support header.

    Copyright (C) 2015  Rdamicro.

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef _RDA_GOUDA_H
#define _RDA_GOUDA_H
#include <mach/hardware.h>
#include <plat/rda_display.h>
#include <mach/rda_clk_name.h>

/**
* Support max img width to 1280
* Support video layer img rotation mirror
*/

/* gd_command ,start gouda hw clear*/
#define GOUDA_START 				(1<<0)

/* gd_status */
#define GOUDA_IA_BUSY				(1<<0)  /*GOUDA BUSY*/
#define GOUDA_LCD_BUSY				(1<<1)  /*LCD BUSY*/

/* gd_eof_irq */
#define GOUDA_EOF_CAUSE 			(1<<0)
#define GOUDA_EOF_STATUS			(1<<16)

/* gd_eof_irq_mask */
#define GOUDA_EOF_MASK				(1<<0)

/* gd_roi_tl_ppos,gd_vl_tl_ppos,gd_ol_tl_ppos*/
#define GOUDA_X0(n) 				(((n)&0x7FF)<<0)
#define GOUDA_Y0(n) 				(((n)&0x7FF)<<16)

/* gd_roi_br_ppos,gd_vl_br_ppos,gd_ol_br_ppos*/
#define GOUDA_X1(n) 				(((n)&0x7FF)<<0)
#define GOUDA_Y1(n) 				(((n)&0x7FF)<<16)

/* gd_roi_bg_color */
#define GOUDA_B(n)				(((n)&0xFF)<<0)
#define GOUDA_G(n)				(((n)&0xFF)<<8)
#define GOUDA_R(n)				(((n)&0xFF)<<16)
#define GOUDA_A(n)				(((n)&0xFF)<<24)

/* gd_vl_input_fmt */
#define GOUDA_VL_FORMAT(n) 			(((n)&3)<<0)
#define GOUDA_VL_STRIDE(n) 			(((n)&0x1FFF)<<2)
#define GOUDA_VL_RGB_SWAP			(1<<16)
#define GOUDA_VL_BYTE_SWAP			(1<<17)
#define GOUDA_VL_FORMAT_ARGB			(1<<19)
#define GOUDA_VL_LAYER_ACTIVE			(1<<31)

/*gd_vl_extents*/
#define GOUDA_VL_MAX_LINE(n)			(((n)&0x7FF)<<0)
#define GOUDA_VL_MAX_COL(n)			(((n)&0x7FF)<<16)

/* gd_vl_blend_opt */
#define GOUDA_VL_CHROMA_KEY_EN(n)		((n)&0xffff) << 0
#define GOUDA_VL_CHROMA_KEY_MASK(n)		(~(0xffff) << 0)
#define GOUDA_VL_DEPTH(n)			(((n)&7)<<17)
#define GOUDA_VL_DEPTH_MASK 			(~(7<<17))
#define GOUDA_VL_ALPHA(n)				(((n)&0xFF)<<20)
#define GOUDA_VL_ALPHA_MASK			(~(0xFF<<20))
#define GOUDA_VL_ROTATION(n)			(((n)&3)<<28)
#define GOUDA_VL_ROTATION_MASK 			(~(3<<28))

#define GOUDA_VL_CHROMA_KEY_EN_V(n)		((n) >> 0) & 0xFFFF
#define GOUDA_VL_ALPHA_V(n)				((n) >> 20) & 0xFF
#define GOUDA_VL_ROTATION_V(n)			((n) >> 28) & 0x3
#define GOUDA_VL_DEPTH_V(n)			((n) >> 17) & 0x7

/* gd_vl_y_src */
#define GOUDA_VL_Y_ADDR(n)				(((n)&0x7FFFFFFF)<<1)

/* gd_vl_u_src */
#define GOUDA_VL_U_ADDR(n)				(((n)&0x7FFFFFFF)<<1)

/* gd_vl_u_src */
#define GOUDA_VL_V_ADDR(n)				(((n)&0x7FFFFFFF)<<1)

/* gd_vl_resc_ratio */
#define GOUDA_VL_XPITCH(n) 			(((n)&0x7FF)<<0)
#define GOUDA_VL_YPITCH(n) 			(((n)&0x7FF)<<16)
#define GOUDA_VL_PREFETCH_ENABLE		((1<<29))

/* gd_ol_input_fmt */
#define GOUDA_OL_FORMAT_RGB565 			(0<<0)
#define GOUDA_OL_FORMAT_ARGB8888 		(1<<0)
#define GOUDA_OL_STRIDE(n) 			(((n)&0x1FFF)<<2)
#define GOUDA_OL_RGB_SWA			(1<<16)
#define GOUDA_OL_BYTE_SWAP			(1<<17)
#define GOUDA_OL_PREFETCH_ENABLE		((1<<18))
#define GOUDA_OL_FORMAT_ARGB			(1<<19)
#define GOUDA_OL_ALPHA_SWAP			(1<<20)
#define GOUDA_OL_LAYER_ACTIVE			(1<<31)

/* gd_Ol_blend_opt */
#define GOUDA_OL_CHROMA_KEY_EN(n)		((n)&0xffff) << 0
#define GOUDA_OL_CHROMA_KEY_MASK(n)		(~(0xffff) << 0)
#define GOUDA_OL_ALPHA(n)				(((n)&0xFF)<<20)
#define GOUDA_OL_ALPHA_MASK			(~(0xFF<<20))

#define GOUDA_OL_CHROMA_KEY_EN_V(n)		((n) >> 0) & 0xFFFF
#define GOUDA_OL_ALPHA_V(n)				((n) >> 20) & 0xFF

/*gd_ol_rgb_src*/
#define GOUDA_OL_ADDR(n) 			(((n)&0x7FFFFFFF)<<1)

/* gd_lcd_ctrl */
#define GOUDA_DESTINATION_MEMORY_RAM (3<<0)
#define GOUDA_OUTPUT_FORMAT(n) (((n) & 0x7) << 4)
#define GOUDA_OUTPUT_FORMAT_MASK (~(0x7 << 4))
#define GOUDA_OUTPUT_FORMAT_16_BIT_RGB565 (6<<4)
#define GOUDA_OUTPUT_FORMAT_24_BIT_RGB888 (7<<4)
#define GOUDA_LCD_CTRL_CS_MASK(n)		(n & ~0x3)

/*gd_lcd_mem_addr*/
#define GOUDA_LCD_MEM_ADDR(n)           (((n)&0xFFFFFF)<<2)

/* gd_lcd_stride_offset */
#define GOUDA_STRIDE_OFFSET(n)			(((n)&0x7FF)<<0)

/* gd_vl_fix_ratio */
#define GOUDA_VL_XRATIO(n)			(((n)&0xff)<<0)
#define GOUDA_VL_XRATIO_MASK			(0xff<<0)
#define GOUDA_VL_YRATIO(n)			(((n)&0xff)<<8)
#define GOUDA_VL_YRATIO_MASK			(0xff<<8)
#define GOUDA_VL_XFIXEN 			(1<<16)
#define GOUDA_VL_YFIXEN 			(1<<17)
#define GOUDA_VL_MIRROR_ENABLE 			(1<<18)
#define GOUDA_VL_MIRROR_MASK			(0<<18)


//set rotation
#define ROTATION_MASK		0xCFFFFFFF
#define ROTATION_OFFSET 	28

#define GOUDA_OL_LAYER_NUM	5
#define GOUDA_VL_LAYER_BITMAP (1 << GOUDA_VID_LAYER_ID)
#define GOUDA_OL_LAYER_BITMAP ((1 << GOUDA_VID_LAYER_ID) - 1)

#define ACCELERATE_VIDOE_LAYAER_FLAG (1 << 31)

#define GD_COMMAND	0x0
#define GD_STATUS		0x4
#define GD_EOF_IRQ		0x8
#define GD_EOF_IRQ_MASK		0xC
#define GD_ROI_TL_PPOS		0x10
#define GD_ROI_BR_PPOS		0x14
#define GD_ROI_BG_COLOR		0x18

#define GD_VL_INPUT_FMT		0x1C
#define GD_VL_TL_PPOS		0x20
#define GD_VL_BR_PPOS		0x24
#define GD_VL_EXTENTS		0x28
#define GD_VL_BLEND_OPT		0x2C
#define GD_VL_Y_SRC			0x30
#define GD_VL_U_SRC			0x34
#define GD_VL_V_SRC			0x38
#define GD_VL_RESC_RATIO		0x3C

#define GD_OL_INPUT_FMT_0	0x40
#define GD_OL_TL_PPOS_0		0x44
#define GD_OL_BR_PPOS_0		0x48
#define GD_OL_BLEND_OPT_0	0x4C
#define GD_OL_RGB_SRC_0		0x50

#define GD_OL_INPUT_FMT_1	0x54
#define GD_OL_TL_PPOS_1		0x58
#define GD_OL_BR_PPOS_1		0x5C
#define GD_OL_BLEND_OPT_1	0x60
#define GD_OL_RGB_SRC_1		0x64

#define GD_OL_INPUT_FMT_2	0x68
#define GD_OL_TL_PPOS_2		0x6C
#define GD_OL_BR_PPOS_2		0x70
#define GD_OL_BLEND_OPT_2	0x74
#define GD_OL_RGB_SRC_2		0x78

#define GD_LCD_CTRL				0x7C
#define GD_LCD_MEM_ADDRESS		0x84
#define GD_LCD_STRIDE_OFFSET		0x88
#define GD_VL_FIX_RATIO			0x98
#define DCT_SHIFTR_Y_REG1		0xA0
#define DCT_CHOOSE_Y_REG0		0xA4
#define DCT_CHOOSE_Y_REG1		0xA8
#define DCT_SHIFT_UV_REG0		0xAC
#define DCT_SHIFT_UV_REG1		0xB0
#define OUT_COLOR_CFG			0x118

#define GD_OL_INPUT_FMT_3		0x11C
#define GD_OL_TL_PPOS_3			0x120
#define GD_OL_BR_PPOS_3			0x124
#define GD_OL_BLEND_OPT_3		0x128
#define GD_OL_RGB_SRC_3			0x12C

#define DITHER_CTRL				0x130
#define DITHER_MATRIX0_0			0x134
#define DITHER_MATRIX0_1			0x138
#define DITHER_MATRIX1			0x13C

#define GD_OL_INPUT_FMT_4		0x148
#define GD_OL_TL_PPOS_4			0x14C
#define GD_OL_BR_PPOS_4			0x150
#define GD_OL_BLEND_OPT_4		0x154
#define GD_OL_RGB_SRC_4			0x158

enum gouda_layer_ol_offset{
	GD_OL_INPUT_FMT = 0,
	GD_OL_TL_PPOS = 0x4,
	GD_OL_BR_PPOS = 0x8,
	GD_OL_BLEND_OPT = 0xC,
	GD_OL_RGB_SRC = 0x10,
};

#define GOUDA_OUTL(addr, data) writel((data), (addr))

#define GOUDA_INL(addr) readl(addr)

#define GOUDA_OR_WITH_REG(addr, data)	\
	do{	\
		GOUDA_OUTL(addr,data | GOUDA_INL(addr));	\
	}while(0);

#define GOUDA_AND_WITH_REG(addr, data)	\
	do{	\
		GOUDA_OUTL(addr,data & GOUDA_INL(addr));	\
	}while(0);

struct rda_gd {
	void __iomem *reg_base;
	int irq;
	int enabled;
	int layer_open_mask;
	bool is_active;

	spinlock_t lock;
	wait_queue_head_t wait_eof;
	struct clk *mclk;
	bool gouda_clk_on;
};

/* 2D functions */
void rda_gouda_stretch_blit(struct gouda_vl_input *src,
				   struct gouda_vl_output *dst);

void rda_gouda_set_ovl_var(struct gouda_blend_var *var, int id);
void rda_gouda_get_ovl_var(struct gouda_blend_var *var, int id);
void rda_gouda_set_osd_layer(struct gouda_ovl_layer_var *buf);
int rda_gouda_ovl_close(int layerId);
int rda_gouda_ovl_blend(struct gouda_layer_blend_var *layer);
void rda_gouda_set_vid_layer(struct gouda_vl_input *src,
				   struct gouda_vl_output *dst);
int rda_gouda_enable_clk(bool enable);
void rda_gouda_reset(void);

#endif /* _RDA_GOUDA_H */
