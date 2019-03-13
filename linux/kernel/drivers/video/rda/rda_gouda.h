/*
    RDA gouda support header.

    Copyright (C) 2012  Huaping Wu <huapingwu@rdamicro.com>

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
#include <plat/reg_gouda.h>
#include <mach/rda_clk_name.h>

/* gd_command */
#define GOUDA_START 				(1<<0)

/* gd_status */
#define GOUDA_IA_BUSY				(1<<0)
#define GOUDA_LCD_BUSY				(1<<4)

/* gd_eof_irq */
#define GOUDA_EOF_CAUSE 			(1<<0)
#define GOUDA_EOF_STATUS			(1<<16)

/* gd_eof_irq_mask */
#define GOUDA_EOF_MASK				(1<<0)
#define GOUDA_EOF_IRQ				(1<<0)
#define GOUDA_FRAME_OVER_MASK			(1<<4)
#define GOUDA_FRAME_OVER_IRQ			(1<<5)

/* gd_roi_tl_ppos */
#define GOUDA_X0(n) 				(((n)&0x7FF)<<0)
#define GOUDA_Y0(n) 				(((n)&0x7FF)<<16)

/* gd_roi_br_ppos */
#define GOUDA_X1(n) 				(((n)&0x7FF)<<0)
#define GOUDA_Y1(n) 				(((n)&0x7FF)<<16)

/* gd_roi_bg_color */
#define GOUDA_B(n)				(((n)&31)<<0)
#define GOUDA_G(n)				(((n)&0x3F)<<5)
#define GOUDA_R(n)				(((n)&31)<<11)

/* gd_vl_input_fmt */
#define GOUDA_FORMAT(n) 			(((n)&3)<<0)
#define GOUDA_STRIDE(n) 			(((n)&0x1FFF)<<2)
#define GOUDA_ACTIVE				(1<<31)

/* gd_vl_extents */
#define GOUDA_MAX_LINE(n)			(((n)&0x7FF)<<0)
#define GOUDA_MAX_COL(n)			(((n)&0x7FF)<<16)

/* gd_vl_blend_opt */
#define GOUDA_CHROMA_KEY_B(n)			(((n)&31)<<0)
#define GOUDA_CHROMA_KEY_B_MASK 		(31<<0)
#define GOUDA_CHROMA_KEY_G(n)			(((n)&0x3F)<<5)
#define GOUDA_CHROMA_KEY_G_MASK 		(0x3F<<5)
#define GOUDA_CHROMA_KEY_R(n)			(((n)&31)<<11)
#define GOUDA_CHROMA_KEY_R_MASK 		(31<<11)
#define GOUDA_CHROMA_KEY_ENABLE 		(1<<16)
#define GOUDA_CHROMA_KEY_ENABLE_MASK 		(1<<16)
#define GOUDA_CHROMA_KEY_MASK(n)		(((n)&7)<<17)
#define GOUDA_CHROMA_KEY_MASK_MASK		(7<<17)
#define GOUDA_ALPHA(n)				(((n)&0xFF)<<20)
#define GOUDA_ALPHA_MASK			(0xFF<<20)
#define GOUDA_ROTATION(n)			(((n)&3)<<28)
#define GOUDA_ROTATION_MASK 			(3<<28)
#define GOUDA_DEPTH(n)				(((n)&3)<<30)
#define GOUDA_DEPTH_MASK			(3<<30)
#define GOUDA_CHROMA_KEY_COLOR(n)		(((n)&0xFFFF)<<0)
#define GOUDA_CHROMA_KEY_COLOR_MASK 		(0xFFFF<<0)
#define GOUDA_CHROMA_KEY_COLOR_SHIFT 		(0)

#define GOUDA_LCD_CTRL_CS_MASK(n)		(n & ~0x3)
#define GOUDA_LCD_CTRL_FMT_SET(a, b)		a = ((a & ~0x70) | (b << 4))

/* gd_vl_y_src */
#define GOUDA_ADDR(n)				(((n)&0xFFFFFF)<<2)

/* gd_vl_resc_ratio */
#define GOUDA_XPITCH(n) 			(((n)&0x7FF)<<0)
#define GOUDA_YPITCH(n) 			(((n)&0x7FF)<<16)
#define GOUDA_YPITCH_SCALE_ENABLE		((1<<31))

/* gd_lcd_ctrl */
#define GOUDA_DESTINATION_LCD_CS_0		(0<<0)
#define GOUDA_DESTINATION_LCD_CS_1		(1<<0)
#define GOUDA_DESTINATION_MEMORY_LCD_TYPE 	(2<<0)
#define GOUDA_DESTINATION_MEMORY_RAM 		(3<<0)
#define GOUDA_OUTPUT_FORMAT_8_BIT_RGB332 	(0<<4)
#define GOUDA_OUTPUT_FORMAT_8_BIT_RGB444 	(1<<4)
#define GOUDA_OUTPUT_FORMAT_8_BIT_RGB565 	(2<<4)
#define GOUDA_OUTPUT_FORMAT_16_BIT_RGB332 	(4<<4)
#define GOUDA_OUTPUT_FORMAT_16_BIT_RGB444 	(5<<4)
#define GOUDA_OUTPUT_FORMAT_16_BIT_RGB565 	(6<<4)
#define GOUDA_CS0_POLARITY			(1<<8)
#define GOUDA_CS1_POLARITY			(1<<9)
#define GOUDA_RS_POLARITY			(1<<10)
#define GOUDA_WR_POLARITY			(1<<11)
#define GOUDA_RD_POLARITY			(1<<12)
#define GOUDA_NB_COMMAND(n) 			(((n)&31)<<16)
#define GOUDA_START_COMMAND 			(1<<24)

/* gd_lcd_timing */
#define GOUDA_TAS(n)				(((n)&7)<<0)
#define GOUDA_TAH(n)				(((n)&7)<<4)
#define GOUDA_PWL(n)				(((n)&0x3F)<<8)
#define GOUDA_PWH(n)				(((n)&0x3F)<<16)

/* gd_lcd_mem_address */
#define GOUDA_ADDR_DST(n)			(((n)&0xFFFFFF)<<2)

/* gd_lcd_stride_offset */
#define GOUDA_STRIDE_OFFSET(n)			(((n)&0x3FF)<<0)

/* gd_lcd_single_access */
#define GOUDA_LCD_DATA(n)			(((n)&0xFFFF)<<0)
#define GOUDA_TYPE				(1<<16)
#define GOUDA_START_WRITE			(1<<17)
#define GOUDA_START_READ			(1<<18)

/* gd_spilcd_config */
#define GOUDA_SPI_LCD_SEL_NORMAL		(0<<0)
#define GOUDA_SPI_LCD_SEL_SPI			(1<<0)
#define GOUDA_SPI_DEVICE_ID(n)			(((n)&0x3f)<<1)
#define GOUDA_SPI_DEVICE_ID_MASK		(0x3f<<1)
#define GOUDA_SPI_DEVICE_ID_SHIFT		(1)
#define GOUDA_SPI_CLK_DIVIDER(n)		(((n)&0xff)<<7)
#define GOUDA_SPI_CLK_DIVIDER_MASK		(0xff<<7)
#define GOUDA_SPI_CLK_DIVIDER_SHIFT 		(7)
#define GOUDA_SPI_DUMMY_CYCLE(n)		(((n)&0x7)<<15)
#define GOUDA_SPI_DUMMY_CYCLE_MASK		(0x7<<15)
#define GOUDA_SPI_DUMMY_CYCLE_SHIFT 		(15)
#define GOUDA_SPI_LINE_MASK 			(0x3<<18)
#define GOUDA_SPI_LINE_4			(0<<18)
#define GOUDA_SPI_LINE_3			(1<<18)
#define GOUDA_SPI_LINE_4_START_BYTE 		(2<<18)
#define GOUDA_SPI_RX_BYTE(n)			(((n)&0x7)<<20)
#define GOUDA_SPI_RX_BYTE_MASK			(0x7<<20)
#define GOUDA_SPI_RX_BYTE_SHIFT 		(20)
#define GOUDA_SPI_RW_WRITE			(0<<23)
#define GOUDA_SPI_RW_READ			(1<<23)

/* gd_vl_fix_ratio */
#define GOUDA_VL_XRATIO(n)			(((n)&0xff)<<0)
#define GOUDA_VL_XRATIO_MASK			(0xff<<0)
#define GOUDA_VL_XRATIO_SHIFT			(0)
#define GOUDA_VL_YRATIO(n)			(((n)&0xff)<<8)
#define GOUDA_VL_YRATIO_MASK			(0xff<<8)
#define GOUDA_VL_YRATIO_SHIFT			(8)
#define GOUDA_VL_XFIXEN 			(1<<16)
#define GOUDA_VL_YFIXEN 			(1<<17)

//dpi time0
#define FRONT_PORCH_STATRT_TIMER(n)              (((n)&0x7FF)<<16)
#define BACK_PORCH_END_VSYNC_TIMER(n)              (((n)&0x7FF)<<0)

//dpi time1
#define VSYNC_INCLUDE_HSYNC_TH_LOW(n)              (((n)&0x7FF)<<16)
#define VSYNC_INCLUDE_HSYNC_TH_HIGH(n)              (((n)&0x7FF)<<0)

//dpi time2
#define HSYNC_INCLUDE_DOTCLK_TH_LOW(n)              (((n)&0x7FF)<<16)
#define HSYNC_INCLUDE_DOTCLK_TH_HIGH(n)              (((n)&0x7FF)<<0)

//dpi time3
#define RGB_DATA_ENABLE_END_TIMER(n)              (((n)&0x7FF)<<16)
#define RGB_DATA_ENABLE_START_TIMER(n)              (((n)&0x7FF)<<0)

//dpi config
#define RGB_FORMAT_16_BIT                                             (1<<12)
#define RGB_FORMAT_24_BIT                                             (0<<12)

#define RGB_PANEL_ENABLE    (1<<0)
#define RGB_PANEL_DISABLE    (0<<0)

#define DPI_WAIT_UNDERFLOW	(1 << 30)
#define DPI_REQUEST_2_OUTSTANDING (1 << 18)
#define ACCELERATE_VIDOE_LAYAER_FLAG (1 << 31)
#define GOUDA_READ_ACCELERATE	(1 << 29)
#define DPI_CLK_AUTO		(1 << 17)

//dpi size
#define VERTICAL_PIX_NUM(n)              (((n)&0x7FF)<<16)
#define HORIZONTAL_PIX_NUM(n)              (((n)&0x7FF)<<0)

//set rotation
#define ROTATION_MASK		0xCFFFFFFF
#define ROTATION_OFFSET 	28

typedef volatile struct {
	REG32 gd_command;	/* 0x00000000 */
	REG32 gd_status;	/* 0x00000004 */
	REG32 gd_eof_irq;	/* 0x00000008 */
	REG32 gd_eof_irq_mask;	/* 0x0000000C */
	REG32 gd_roi_tl_ppos;	/* 0x00000010 */
	REG32 gd_roi_br_ppos;	/* 0x00000014 */
	REG32 gd_roi_bg_color;	/* 0x00000018 */
	REG32 gd_vl_input_fmt;	/* 0x0000001C */
	REG32 gd_vl_tl_ppos;	/* 0x00000020 */
	REG32 gd_vl_br_ppos;	/* 0x00000024 */
	REG32 gd_vl_extents;	/* 0x00000028 */
	REG32 gd_vl_blend_opt;	/* 0x0000002C */
	REG32 gd_vl_y_src;	/* 0x00000030 */
	REG32 gd_vl_u_src;	/* 0x00000034 */
	REG32 gd_vl_v_src;	/* 0x00000038 */
	REG32 gd_vl_resc_ratio;	/* 0x0000003C */
	/*
	 *The Overlay layers have a fixed depth relative to their index. Overlay layer
	 * 0 is the first to be drawn (thus the deepest), overlay layer 2 is the last
	 * to be drawn.
	 */
	struct {
		REG32 gd_ol_input_fmt;	/* 0x00000040 */
		REG32 gd_ol_tl_ppos;	/* 0x00000044 */
		REG32 gd_ol_br_ppos;	/* 0x00000048 */
		REG32 gd_ol_blend_opt;	/* 0x0000004C */
		REG32 gd_ol_rgb_src;	/* 0x00000050 */
	} overlay_layer[3];
	REG32 gd_lcd_ctrl;	/* 0x0000007C */
	/* All value are in cycle number of system clock */
	REG32 gd_lcd_timing;	/* 0x00000080 */
	REG32 gd_lcd_mem_address;	/* 0x00000084 */
	REG32 gd_lcd_stride_offset;	/* 0x00000088 */
	REG32 gd_lcd_single_access;	/* 0x0000008C */
	REG32 gd_spilcd_config;	/* 0x00000090 */
	REG32 gd_spilcd_rd;	/* 0x00000094 */
	REG32 gd_vl_fix_ratio;	/* 0x00000098 */
	REG32 dct_shiftr_y_reg0;	//0x0
	REG32 dct_shiftr_y_reg1;	//0x0
	REG32 dct_choose_y_reg0;	//0x
	REG32 dct_choose_y_reg1;	//0x0
	REG32 dct_shift_uv_reg0;	//off
	REG32 dct_shift_uv_reg1;	//off
	REG32 dct_choose_uv_reg0;	// off
	REG32 dct_choose_uv_reg1;	//offs
	REG32 dct_config;	// offset=47*4
	REG32 dpi_config;	//offset =48*4
	REG32 dpi_frame0_adrr;	//offset =49*4
	REG32 dpi_frame0_con;	// offset=50*4
	REG32 dpi_frame1_adrr;	// offset=51*4
	REG32 dpi_frame1_con;	// offset=52*4
	REG32 dpi_frame2_adrr;	// offset=53*
	REG32 dpi_frame2_con;	//offset =54*4
	REG32 dpi_size;		// offset=55*4
	REG32 dpi_fifo_ctrl;	// offset=56*4
	REG32 dpi_throt;	// offset=57*4
	REG32 dpi_pol;		// offset=58*4
	REG32 dpi_time0;	// offset=59*4
	REG32 dpi_time1;	// offset=60*
	REG32 dpi_time2;	// offset=61*4
	REG32 dpi_time3;	// offset=62*4
	REG32 dpi_status;	// offset=63*4
	REG32 d2_ctrl;		//4offset=64*
	REG32 d2_rop_ctrl;	//offset =65*4
	REG32 d2_draw_color;	//offset =66*
	REG32 d2_draw_p0;	//offset=67*4
	REG32 d2_draw_p1;	//offset =68*4
	REG32 d2_draw_p2;	// offset=69*4
	REG32 out_color_cfg;	// offset=70*4
	REG32 GD_OL_INPUT_FMT_3;	// offset=7
	REG32 GD_OL_TL_PPOS_3;	// offset=72*
	REG32 GD_OL_BR_PPOS_3;	// offset=73*
	REG32 GD_OL_BLEND_OPT_3;	// offset=7
	REG32 GD_OL_RGB_SRC_3;	// offset=75*
	REG32 DITHER_CTRL;	// offset=76*4
	REG32 DITHER_MATRIX0_0;	// offset=77
	REG32 DITHER_MATRIX0_1;	// offset=78
	REG32 DITHER_MATRIX1;	//offset =79*4
	REG32 TECON;		// offset=80*4
	REG32 TECON2;		//offset =81*4
} HWP_GOUDA_T;

union gouda_lcd_timing {
	struct {
		/* Address setup time (RS to WR, RS to RD)in clock number */
		u32 tas:3;
		 u32:1;
		/* Adress hold time in clock number */
		u32 tah:3;
		 u32:1;
		/* Control  pulse width low, in clock number */
		u32 pwl:6;
		 u32:2;
		/* Control pulse width high, in clock number */
		u32 pwh:6;
		 u32:10;
	} mcu;
	u32 reg;
	struct {
		u32 lcd_freq;
		u16 clk_divider;
		u16 width;
		u16 height;
		u16 v_back_porch;
		u16 v_front_porch;
		u16 h_back_porch;
		u16 h_front_porch;
		u16 v_low;
		u16 h_low;
		bool v_pol;
		bool h_pol;
		bool data_pol;
		bool dot_clk_pol;
	} rgb;
};

enum gouda_lcd_cs {
	GOUDA_LCD_CS_0 = 0,
	GOUDA_LCD_CS_1 = 1,
	GOUDA_LCD_CS_QTY,
	GOUDA_LCD_MEMORY_IF = 2,
	GOUDA_LCD_IN_RAM = 3
};


enum RGB_Panel_Mode {
	RGB_IS_24BIT = 0,
	RGB_IS_16BIT
};

enum GOUDA_RGB_FMT {
	RDA_FMT_RGB565 = 0,
	RDA_FMT_RGB888,
	RDA_FMT_XRGB,
	RDA_FMT_RGBX
};

#define MAX_INT_NUM 3
union gouda_lcd_cfg {
	struct {
		enum gouda_lcd_cs cs:2;
		 u32:2;
		enum gouda_lcd_output_fmt output_fmt:3;
		bool highByte:1;

		/* \c FALSE is active low, \c TRUE is active high. */
		bool cs0_polarity:1;
		/* \c FALSE is active low, \c TRUE is active high. */
		bool cs1_polarity:1;

		/* \c FALSE is active low, \c TRUE is active high. */
		bool rs_polarity:1;

		/* \c FALSE is active low, \c TRUE is active high. */
		bool wr_polarity:1;

		/* \c FALSE is active low, \c TRUE is active high. */
		bool rd_polarity:1;
		 u32:3;

		/* Number of command to be send to the LCD command (up to 31) */
		u32 nb_cmd:5;
		 u32:3;

		/* Start command transfer only. Autoreset */
		bool start_cmd_only:1;
		 u32:7;

		//TECON stuff
		u32 te_en:1;
		u32 te_edge_sel:1;
		u32 te_mode:1;
		u32 :13;
		u32 te_count2:12;
		u32 :4;

		//TECON2
		u32 tecon2;
	};
	struct {
		u16 rgb_format:1;
		 u16:2;
		u16 mipi_enable:1;
		 u16:3;
		u16 pix_fmt:2;
		u16 rgb_order:1;
		u16 frame1:1;
		u16 frame2:1;
		u16 rgb_enable:1;
	} rgb;
	u32 reg[MAX_INT_NUM];
};
struct gouda_fb {
	u16 *buffer;
	u16 width;
	u16 height;
	enum gouda_lcd_output_fmt lcd_output_fmt;
};

struct gouda_image {
	u16 *buffer;
	u16 width;
	u16 height;
	enum gouda_img_fmt image_fmt;
};

struct gouda_lcd {
	u16 width;
	u16 height;
	enum gouda_lcd_interface lcd_interface;
	union gouda_lcd_timing lcd_timing;
	union gouda_lcd_cfg lcd_cfg;
};
struct gouda_ovl_layer_def {
	/* @todo check with u16* (?) */
	u32 *addr;
	enum gouda_img_fmt fmt;
	/* buffer width in bytes including optional padding, when 0 will be calculated */
	u16 stride;
	struct gouda_rect pos;
	//u8 alpha;
	//bool ckey_en;
	//u16 ckey_color;
	//enum gouda_ckey_mask ckey_mask;
};
enum gouda_vid_layer_depth {
	/* Video layer behind all Overlay layers */
	GOUDA_VID_LAYER_BEHIND_ALL = 0,
	/* Video layer between Overlay layers 1 and 0 */
	GOUDA_VID_LAYER_BETWEEN_1_0,
	/* Video layer between Overlay layers 2 and 1 */
	GOUDA_VID_LAYER_BETWEEN_2_1,
	/* Video layer on top of all Overlay layers */
	GOUDA_VID_LAYER_OVER_ALL
};
struct gouda_vid_layer_def {
	u32 *addr_y;
	u32 *addr_u;
	u32 *addr_v;
	enum gouda_img_fmt fmt;
	u16 height;
	u16 width;
	u16 stride;
	struct gouda_rect pos;
};
struct rda_lcd_info;

extern void * rda_fb_get_device(void);

/* LCD interface */
void rda_gouda_pre_enable_lcd(struct gouda_lcd *lcd,int onoff);

void rda_gouda_open_lcd(struct gouda_lcd *lcd, unsigned int addr);

void rda_gouda_display(struct gouda_image *src,
			      struct gouda_rect *src_rect, u16 x, u16 y);

void rda_gouda_close_lcd(struct gouda_lcd *lcd);

void rda_gouda_flash_fifo(void);

int rda_gouda_dbi_write_cmd2lcd(u16 addr);

int rda_gouda_dbi_write_data2lcd(u16 data);

int rda_gouda_dbi_read_data(u8 * data);

int rda_gouda_dbi_read_data16(u16 * data);

/* 2D functions */
void rda_gouda_stretch_blit(struct gouda_input *src,
				   struct gouda_output *dst,
				   bool block);

void rda_gouda_set_ovl_var(struct gouda_ovl_blend *var, int id);

void rda_gouda_get_ovl_var(struct gouda_ovl_blend *var, int id);

void rda_gouda_osd_display(struct gouda_ovl_buf_var *buf);

int rda_gouda_ovl_close(int layerId);
int rda_gouda_ovl_open(struct gouda_ovl_open_var *layer, bool block);

void rda_gouda_vid_display(struct gouda_input *src,
				   struct gouda_output *dst);

int rda_gouda_stretch_pre_wait_and_enable_clk(void);

void rda_gouda_stretch_post_disable_clk(void);
extern int rda_gouda_save_outfmt(void);
extern void rda_gouda_restore_outfmt(int);

void rda_gouda_configure_timing(struct gouda_lcd *lcd);
void rda_gouda_configure_te(struct gouda_lcd *lcd,int te_mode);
#endif /* _RDA_GOUDA_H */
