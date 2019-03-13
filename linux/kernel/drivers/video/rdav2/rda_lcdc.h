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

#ifndef _RDA_LCDC_H
#define _RDA_LCDC_H
#include <linux/module.h>
#include <linux/init.h>
#include <linux/clk.h>

#include <linux/fb.h>

#include <mach/hardware.h>
#include <plat/rda_display.h>
#include <mach/rda_clk_name.h>

#include "rda_panel.h"
#include "rda_fb.h"

#define RDA8850E_WORKAROUND	1

/**
* Support max img width to 1280
* Support video layer img rotation mirror
*/

/* gd_command ,start gouda hw clear*/
#define LCDC_START		(1<<0)

/* gd_status */
#define LCDC_IA_BUSY		(1<<0)  /*GOUDA BUSY*/
#define LCDC_LCD_BUSY		(1<<1)  /*LCD BUSY*/

/* gd_eof_irq */
#define LCDC_EOF_CAUSE		(1<<0)
#define LCDC_VSYNC_RISE		(1<<2)
#define LCDC_VSYNC_FALL		(1<<3)
#define LCDC_DPI_OVER_FLOW		(1<<4)
#define LCDC_DPI_FRAME_OVER		(1<<5)
#define LCDC_MIPI_INT		(1<<6)
#define LCDC_EOF_STATUS		(1<<16)
#define LCDC_IRQ_CLEAR_ALL		(~0)

/* gd_eof_irq_mask */
#define LCDC_EOF_MASK		(1<<0)
#define LCDC_VSYNC_RISE_MASK		(1<<1)
#define LCDC_VSYNC_FALL_MASK		(1<<2)
#define LCDC_DPI_OVER_FLOW_MASK		(1<<3)
#define LCDC_DPI_FRAME_OVER_MASK		(1<<4)
#define LCDC_MIPI_INT_MASK		(1<<5)

/* gd_lcd_ctrl */
#define LCDC_DESTINATION(n)			(((n) & 0x3) << 0)
#define LCDC_DESTINATION_LCD_CS_0		(0<<0)
#define LCDC_DESTINATION_LCD_CS_1		(1<<0)
#define LCDC_DESTINATION_MEMORY_LCD_TYPE		(2<<0)
#define LCDC_DESTINATION_MEMORY_RAM		(3<<0)
#define LCDC_DESTINATION_MASK(n)		((n) & ~0x3)
#define LCDC_OUTPUT_FORMAT(n)			(((n) & 0x7) << 4)
#define LCDC_OUTPUT_FORMAT_8BIT_RGB332		(0<<4)
#define LCDC_OUTPUT_FORMAT_8BIT_RGB444		(1<<4)
#define LCDC_OUTPUT_FORMAT_8BIT_RGB565		(2<<4)
#define LCDC_OUTPUT_FORMAT_16BIT_RGB332		(4<<4)
#define LCDC_OUTPUT_FORMAT_16BIT_RGB444		(5<<4)
#define LCDC_OUTPUT_FORMAT_16BIT_RGB565		(6<<4)
#define LCDC_OUTPUT_FORMAT_32BIT_ARGB8888		(7<<4)
#define LCDC_HIGH_BYTE		(1<<7) /*OUTPUT DATA BUS 16BIT*/
#define LCDC_CS0_POLARITY		(1<<8)
#define LCDC_CS1_POLARITY		(1<<9)
#define LCDC_RS_POLARITY		(1<<10)
#define LCDC_WR_POLARITY		(1<<11)
#define LCDC_RD_POLARITY		(1<<12)
#define LCDC_BUS_SEL_16BIT		(0<<25) /*RGB565*/
#define LCDC_BUS_SEL_18BIT		(1<<25) /*RGB666*/
#define LCDC_BUS_SEL_24BIT		(2<<25) /*RGB888*/
#define LCDC_BUS_SEL(n)		(((n)&3)<<25)

#define LCDC_CTRL_FMT_SET(a, b)		a = ((a & ~0x70) | (b << 4))

/* gd_lcd_timing */
#define LCDC_TAS(n)		(((n)&7)<<0)
#define LCDC_TAH(n)		(((n)&7)<<4)
#define LCDC_PWL(n)		(((n)&0x3F)<<8)
#define LCDC_PWH(n)		(((n)&0x3F)<<16)

/*gd_lcd_mem_addr*/
#define LCDC_LCD_MEM_ADDR(n)		(((n)&0xFFFFFF)<<2)

/* gd_lcd_stride_offset */
#define LCDC_STRIDE_OFFSET(n)		(((n)&0x7FF)<<0)

/* gd_lcd_single_access */
#define LCDC_DBI_SGL_DATA(n)		(((n)&0xFFFF)<<0)
#define LCDC_DBI_SGL_DATA_TYPE		(1<<16)
#define LCDC_DBI_SGL_WRITE		(1<<17)
#define LCDC_DBI_SGL_READ		(1<<18)

/* gd_spilcd_config */
#define LCDC_SPI_LCD_SEL_NORMAL		(0<<0)
#define LCDC_SPI_LCD_SEL_SPI		(1<<0)
#define LCDC_SPI_DEVICE_ID(n)		(((n)&0x3f)<<1)
#define LCDC_SPI_DEVICE_ID_MASK		(0x3f<<1)
#define LCDC_SPI_CLK_DIVIDER(n)		(((n)&0xff)<<7)
#define LCDC_SPI_CLK_DIVIDER_MASK		(0xff<<7)
#define LCDC_SPI_DUMMY_CYCLE(n)		(((n)&0x7)<<15)
#define LCDC_SPI_DUMMY_CYCLE_MASK		(0x7<<15)
#define LCDC_SPI_LINE_MASK		(0x3<<18)
#define LCDC_SPI_LINE_4		(0<<18)
#define LCDC_SPI_LINE_3		(1<<18)
#define LCDC_SPI_LINE_4_START_BYTE		(2<<18)
#define LCDC_SPI_RX_BYTE(n)		(((n)&0x7)<<20)
#define LCDC_SPI_RX_BYTE_MASK		(0x7<<20)
#define LCDC_SPI_RW_WRITE		(0<<23)
#define LCDC_SPI_RW_READ		(1<<23)

/*dct_shift_uv_reg1*/
#define LCDC_VSYNC_TOGGLE_HSYNC_CNT(n)		(((n)&0x7FF)<<0)
#define LCDC_RGB_WAIT		(1<<30)

/*dpi config*/
#define RGB_PANEL_ENABLE		(1<<0)
#define RGB_PANEL_DISABLE		(0<<0)
#define RGB_FRAME1_ENABLE		(1<<1)
#define RGB_FRAME1_DISABLE		(0<<1)
#define RGB_FRAME2_ENABLE		(1<<2)
#define RGB_FRAME2_DISABLE		(0<<2)
#define RGB_PIX_ORDER(n)		(((n)&1)<<3)
#define RGB_PIX_FMT(n)		(((n)&3)<<4)
#define RGB_OUTOFF_ALL(n)		(((n)&1)<<6)
#define RGB_OUTOFF_CLK(n)		(((n)&1)<<7)
#define RGB_OUTOFF_DATA(n)		(((n)&1)<<8)
#define MIPI_DSI_ENABLE		(1<<9)
#define MIPI_DSI_DISABLE		(0<<9)
#define RGB_DDR_ENABLE		(1<<10)
#define RGB_DDR_DISABLE		(1<<10)
#define RGB_DDR_FORMAT_8BIT_BUS		(0<<11)
#define RGB_DDR_FORMAT_12BIT_BUS		(1<<11)
#define RGB_FORMAT_24_BIT		(0<<12)
#define RGB_FORMAT_16_BIT		(1<<12)
#define RGB_FORMAT_18_BIT		(2<<12)
#define MIPI_CMD_SEL_CMD		(1<<20)
#define REG_PEND_REQ(n)		(((n)&0x1F)<<21)

/*dpi_framex_con,x=0 1 2*/
#define DPI_FRAME_LINE_STEP(n)		(((n)&0x1FFF)<<16)
#define DPI_FRAME_VALID		(1<<0)

/*dpi size*/
#define DPI_V_SIZE(n)		(((n)&0x7FF)<<16))
#define DPI_H_SIZE(n)		(((n)&0x7FF)<<0))

/*dpi_fifo_ctrl*/
#define DPI_DATA_FIFO_LOWTHRES(n)		(((n)&0x3FF)<<16)
#define DPI_DATA_FIFO_RST_AUTO		(1<<1)
#define DPI_DATA_FIFO_RST		(1<<0)

/*dpithrot*/
#define DPI_THROTTLE_PEROID(n)		(((n)&0x3FF)<<16)
#define DPI_THROTTLE_EN		(1<<0)

/*dpi pol*/
#define DPI_DOT_CLK_POL		(1 << 8)
#define DPI_HSYNC_POL		(1 << 9)
#define DPI_VSYNC_POL		(1 << 10)
#define DPI_DE_POL		(1 <<11)
#define DPI_CLK_ADJ(n)	(((n)&0xF) << 13)

/*dpi time0*/
#define BACK_PORCH_END_VSYNC_TIMER(n)		(((n)&0x7FF)<<0)
#define FRONT_PORCH_STATRT_TIMER(n)		(((n)&0x7FF)<<16)

/*dpi time1*/
#define VSYNC_INCLUDE_HSYNC_TH_HIGH(n)		(((n)&0x7FF)<<0)
#define VSYNC_INCLUDE_HSYNC_TH_LOW(n)		(((n)&0x7FF)<<16)

/*dpi time2*/
#define HSYNC_INCLUDE_DOTCLK_TH_HIGH(n)		(((n)&0x7FF)<<0)
#define HSYNC_INCLUDE_DOTCLK_TH_LOW(n)		(((n)&0x7FF)<<16)

/*dpi time3*/
#define RGB_DATA_ENABLE_START_TIMER(n)		(((n)&0x7FF)<<0)
#define RGB_DATA_ENABLE_END_TIMER(n)		(((n)&0x7FF)<<16)

/*dpi status*/
#define DPI_FRAME0_OVER		(0<<0)
#define DPI_FRAME1_OVER		(1<<0)
#define DPI_FRAME2_OVER		(1<<2)
#define DPI_FRAME_RUNNING		(1<<3)
#define DPI_CURRENT_FRAME		(3<<4)

/*dither ctl*/
#define DITHER_ENABLE		(1<<0)
#define DITHER_MODE_R		(1<<1)
#define DITHER_MODE_G		(1<<2)
#define DITHER_MODE_B		(1<<3)
#define DITHER_CTRL_R(n)		(((n)&3)<<4)
#define DITHER_CTRL_G(n)		(((n)&3)<<6)
#define DITHER_CTRL_B(n)		(((n)&3)<<8)

/*dpi size*/
#define VERTICAL_PIX_NUM(n)		(((n)&0x7FF)<<16)
#define HORIZONTAL_PIX_NUM(n)		(((n)&0x7FF)<<0)

/*tecon*/
#define TE_ENABLE		(1<<0)
#define TE_DISABLE		(0<<0)
#define TE_EDGE_SEL_RISE		(1<<1)
#define TE_EDGE_SEL_FALL		(0<<1)
#define TE_MODE_VSYNC_ONLY		(0<<2)
#define TE_MODE_VSYNC_HSYNC		(1<<2)
#define TE_COUNT2(n)		(((n)&0xFFF)<<16)

/*tecon2*/
#define TE_COUNT1(n)		(((n)&0x1FFFFFFF)<<0)

/*dsi irq status*/
#define DSI_TX_END_FLAG		(1 << 0)
#define DSI_RX_END_FLAG		(1 << 1)

#define MIPI_DSI_LANE(n)	(((n)&0x3)<<0)

#define DSI_REG_OFFSET		(0x400)

/*LCDC reg offset start*/
#define LCDC_COMMAND	0x0
#define LCDC_STATUS		0x04
#define LCDC_EOF_IRQ		0x08
#define LCDC_EOF_IRQ_MASK	0xC
#define LCD_CTRL		0x10
#define LCD_TIMING	0x14
#define LCDC_MEM_ADDRESS		0x18
#define LCD_STRIDE_OFFSET		0x1C
#define LCDC_SINGLE_ACCESS	0x20
#define LCDC_SPILCD_CONFIG		0x24
#define LCDC_SPILCD_RD			0x28
#define LCDC_DCT_SHIFT_UV_REG1		0x2C
#define DPI_CONFIG				0x30
#define DPI_FRAME0_ADDR			0x34
#define DPI_FRAME0_CON			0x38
#define DPI_FRAME1_ADRR			0x3C
#define DPI_FRAME1_CON			0x40
#define DPI_FRAME2_ADRR			0x44
#define DPI_FRAME2_CON			0x48
#define DPI_SIZE					0x4C
#define DPI_FIFO_CTRL			0x50
#define DPI_THROT				0x54
#define DPI_POL					0x58
#define DPI_TIME0				0x5C
#define DPI_TIME1				0x60
#define DPI_TIME2				0x64
#define DPI_TIME3				0x68
#define DPI_STATUS				0x6C
#define LCDC_DITHER_CTRL				0x70
#define LCDC_DITHER_MATRIX0_0		0x74
#define LCDC_DITHER_MATRIX0_1		0x78
#define LCDC_DITHER_MATRIX1			0x7C
#define TECON					0x80
#define TECON2					0x84
/* LCDC reg offset end */

/* RGB_IF_DIV*/
#define RGB_IF_DIVIDER(n)	(((n)&0xFF)<<0)
#define RGB_IF_FAST_ENABLE	1<<0
#define RGB_IF_DOT_ENABLE	1<<1

/* TV CLK CTRL  REG*/
#define RGB_IF_DIV	0x0
#define RGB_IF_EN	0x4

enum lcd_dbi_output_fmt {
	/// 8-bit - RGB3:3:2 - 1cycle/1pixel - RRRGGGBB
	DBI_OUTPUT_FORMAT_8_BIT_RGB332 = 0,
	/// 8-bit - RGB4:4:4 - 3cycle/2pixel - RRRRGGGG/BBBBRRRR/GGGGBBBB
	DBI_OUTPUT_FORMAT_8_BIT_RGB444 = 1,
	/// 8-bit - RGB5:6:5 - 2cycle/1pixel - RRRRRGGG/GGGBBBBB
	DBI_OUTPUT_FORMAT_8_BIT_RGB565 = 2,
	/// 16-bit - RGB3:3:2 - 1cycle/2pixel - RRRGGGBBRRRGGGBB
	DBI_OUTPUT_FORMAT_16_BIT_RGB332 = 4,
	/// 16-bit - RGB4:4:4 - 1cycle/1pixel - XXXXRRRRGGGGBBBB
	DBI_OUTPUT_FORMAT_16_BIT_RGB444 = 5,
	/// 16-bit - RGB5:6:5 - 1cycle/1pixel - RRRRRGGGGGGBBBBB
	DBI_OUTPUT_FORMAT_16_BIT_RGB565 = 6,
	DBI_OUTPUT_FORMAT_32_BIT_ARGB8888 = 7
};

enum lcd_dpi_fmt_bits {
	RGB_FMT_24_BIT,
	RGB_FMT_16_BIT,
	RGB_FMT_18_BIT,
};

enum lcd_dpi_fmt {
	RGB_PIX_FMT_RGB565,
	RGB_PIX_FMT_RGB888,
	RGB_PIX_FMT_XRGB8888,
	RGB_PIX_FMT_RGBX8888,
};

enum dpi_rgb_order {
	RGB_ORDER_RGB,
	RGB_ORDER_BGR,
};

struct lcdc_fb_data {
	u16 *buffer;
	u16 width;
	u16 height;
};

struct dsi_timing {
	u32 off;
	u32 value;
};

struct rda_dsi_phy_ctrl {
	struct dsi_timing video_timing[13];
	struct dsi_timing cmd_timing[8];
};

struct mipi_panel_info {
	u8 data_lane;
	u8 vc;
	u8 mipi_mode;
	u8 pixel_format;
	u8 dsi_format;
	u8 trans_mode;
	u8 data_swap;
	u8 rgb_order;
	bool bllp_enable;
	u32 h_sync_active;
	u32 h_back_porch;
	u32 h_front_porch;
	u32 v_sync_active;
	u32 v_back_porch;
	u32 v_front_porch;
	u32 vat_line;
	/* video mode */
	u8 frame_rate;
	/* command mode */
	u8 te_sel;
	u8 debug_mode;
	u32 dsi_pclk_rate;
	const struct rda_dsi_phy_ctrl *dsi_phy_db;
};

struct dbi_panel_info {
	u8 cs;
	u8 output_fmt;

	bool highByte;
	bool cs0_polarity;
	bool cs1_polarity;
	bool rs_polarity;
	bool wr_polarity;
	bool rd_polarity;

	u32 nb_cmd;
	bool start_cmd_only;

	/*TECON stuff*/
	u32 te_en;
	u32 te_edge_sel;
	u32 te_mode;
	u32 te_count2;
	u32 tecon2;

	/*mcu timing*/
	u32 tas;
	u32 tah;
	u32 pwl;
	u32 pwh;
};

struct dpi_panel_info {
	u32 pclk;
	u16 pclk_divider;
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
	bool rgb_if_fast_enable;

	bool frame1;
	bool frame2;
	bool rgb_enable;

	u16 dpi_clk_adj;
	u16 pixel_format;
	u16 rgb_format_bits;
	u16 rgb_order;
};

struct lcd_panel_info {
	u16 width;
	u16 height;
	u16 bpp;
	u16 lcd_interface;

	struct mipi_panel_info mipi_pinfo;
	struct dbi_panel_info mcu_pinfo;
	struct dpi_panel_info rgb_pinfo;
};

struct rda_lcd_info {
	char name[32];
	struct lcd_panel_info lcd;
	struct rda_lcd_ops ops;
};

struct rda_lcd_info;

#ifdef CONFIG_FB_RDA_USE_HWC
struct lcdc_vsync {
	ktime_t timestamp;
	int vsync_irq_enabled;
	struct completion vsync_wait;
};
#endif

struct rda_lcdc {
	void __iomem *reg_base;
	void __iomem *tv_clk_ctrl;
	struct clk * clk_dpi;
	struct clk *clk_dsi;
	bool lcdc_clk_on;

	spinlock_t lock;
	wait_queue_head_t wait_eof;
	wait_queue_head_t wait_frmover;
	int irq;

	u32 frame_addr;
	struct lcd_panel_info *lcd;
	bool is_active;
	bool enabled;
	u32 dsi_txrx_status;

#ifdef CONFIG_FB_RDA_USE_HWC
	bool vsync_enabled;
	struct lcdc_vsync vsync_info;
#endif
};

#define DBI_SGL_TIMEOUT 500
#define LCD_MCU_CMD(Cmd)		\
		{while(rda_lcdc_dbi_write_cmd2lcd(Cmd) != 0);}
#define LCD_MCU_DATA(Data)	\
		{while(rda_lcdc_dbi_write_data2lcd(Data) != 0);}
#define LCD_MCU_READ(pData)		\
		{while(rda_lcdc_dbi_read_data(pData) != 0);}
#define LCD_MCU_READ16(pData)		\
		{while(rda_lcdc_dbi_read_data16(pData) != 0);}

#define LCDC_OUTL(addr, data) writel((data), (addr))

#define LCDC_INL(addr) readl(addr)

#define LCDC_OR_WITH_REG(addr, data)	\
	do{	\
		LCDC_OUTL(addr,data | LCDC_INL(addr));	\
	}while(0);

#define LCDC_AND_WITH_REG(addr, data)	\
	do{	\
		LCDC_OUTL(addr,data & LCDC_INL(addr));	\
	}while(0);

extern void * rda_fb_get_device(void);

/* LCD interface */
void enable_lcdc_clk(bool onoff);
void rda_lcdc_pre_enable_lcd(struct lcd_panel_info *lcd,int onoff);
int rda_lcdc_pre_wait_and_enable_clk(void);
void rda_lcdc_post_disable_clk(void);
void rda_lcdc_configure_timing(struct lcd_panel_info *lcd);
void rda_lcdc_open_lcd(struct rda_fb *rdafb);
void rda_lcdc_display(struct rda_fb *rdafb);
void rda_lcdc_close_lcd(struct lcd_panel_info *lcd);
int rda_lcdc_dbi_write_cmd2lcd(u16 addr);
int rda_lcdc_dbi_write_data2lcd(u16 data);
int rda_lcdc_dbi_read_data(u8 * data);
int rda_lcdc_dbi_read_data16(u16 * data);
void rda_reset_lcdc_for_dsi_tx(void);
void rda_lcdc_reset(void);
void rda_lcdc_set_vsync(int enable);
/*mipi dsi*/
void rda_dsi_enable(bool on);
u32 rda_read_dsi_reg(u32 offsets);
void rda_write_dsi_reg(u32 offset,u32 value);
int rda_mipi_dsi_txrx_status(void);
void rda_set_lcdc_for_dsi_cmdlist_tx(u32 cmdlist_addr,int cmd_cnt);
void rda_lcdc_enable_mipi(struct rda_fb *rdafb);
void adjust_dsi_phy_phase(u32 value);

#endif /* _RDA_GOUDA_H */
