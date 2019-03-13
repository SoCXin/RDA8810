#ifndef __RDA_LCDC_H__
#define __RDA_LCDC_H__

#include <common.h>
#include <asm/arch/cs_types.h>

/*LCDC reg offset start*/
#define LCDC_COMMAND			0x0
#define LCDC_STATUS				0x04
#define LCDC_EOF_IRQ			0x08
#define LCDC_EOF_IRQ_MASK		0xC
#define LCD_CTRL				0x10
#define LCD_TIMING				0x14
#define LCDC_MEM_ADDRESS		0x18
#define LCD_STRIDE_OFFSET		0x1C
#define LCDC_SINGLE_ACCESS		0x20
#define LCDC_SPILCD_CONFIG		0x24
#define LCDC_SPILCD_RD			0x28
#define LCDC_DCT_SHIFT_UV_REG1	0x2C
#define DPI_CONFIG				0x30
#define DPI_FRAME0_ADDR			0x34
#define DPI_FRAME0_CON			0x38
#define DPI_FRAME1_ADRR			0x3C
#define DPI_FRAME1_CON			0x40
#define DPI_FRAME2_ADRR			0x44
#define DPI_FRAME2_CON			0x48
#define DPI_SIZE				0x4C
#define DPI_FIFO_CTRL			0x50
#define DPI_THROT				0x54
#define DPI_POL					0x58
#define DPI_TIME0				0x5C
#define DPI_TIME1				0x60
#define DPI_TIME2				0x64
#define DPI_TIME3				0x68
#define DPI_STATUS				0x6C
#define LCDC_DITHER_CTRL		0x70
#define LCDC_DITHER_MATRIX0_0	0x74
#define LCDC_DITHER_MATRIX0_1	0x78
#define LCDC_DITHER_MATRIX1		0x7C
#define TECON					0x80
#define TECON2					0x84
#define DSI_REG_OFFSET			0x400

/* rgb order */
#define RGB_ORDER_RGB			(0<<3)
#define RGB_ORDER_BGR			(1<<3)

/* pixel format */
#define RGB_PIX_FMT_RGB565		(0<<4)
#define RGB_PIX_FMT_RGB888		(1<<4)
#define RGB_PIX_FMT_XRGB888		(2<<4)
#define RGB_PIX_FMT_RGBX888		(3<<4)

#define MIPI_DSI_ENABLE			(1<<9)
#define MIPI_DSI_DISABLE		(0<<9)

#define MIPI_CMD_SEL_CMD		(1<<20)
#define REG_PEND_REQ(n)			(((n)&0x1F)<<21)

#define DPI_FRAME_LINE_STEP(n)	(((n)&0x1FFF)<<16)
#define DPI_FRAME_VALID			(1<<0)

//dpi size
#define VERTICAL_PIX_NUM(n)		(((n)&0x7FF)<<16)
#define HORIZONTAL_PIX_NUM(n)	(((n)&0x7FF)<<0)

//dct_shift_uv_reg1
#define DCT_SHIFT_UV_ENABLE		(1<<30)
#define LCDC_RGB_WAIT			(1<<30)

/*dpi_fifo_ctrl*/
#define DPI_DATA_FIFO_LOWTHRES(n)	(((n)&0x3FF)<<16)
#define DPI_DATA_FIFO_RST_AUTO		(1<<1)
#define DPI_DATA_FIFO_RST			(1<<0)

#define MIPI_DSI_LANE(n)		(((n)&0x3)<<0)

//gd_eof_irq
#define LCDC_EOF_CAUSE			(1 << 0)
#define LCDC_VSYNC_RISE			(1 << 2)
#define LCDC_VSYNC_FALL			(1 << 3)
#define LCDC_DPI_OVERFLOW		(1 << 4)
#define LCDC_DPI_FRAMEOVER		(1 << 5)
#define LCDC_MIPI_INT			(1 << 6)
#define LCDC_EOF_STATUS			(1 << 16)
#define LCDC_IRQ_CLEAR_ALL		(~0)

//gd_eof_irq_mask
#define LCDC_EOF_MASK			(1<<0)
#define LCDC_VSYNC_RISE_MASK	(1<<1)
#define LCDC_VSYNC_FALL_MASK	(1<<2)
#define LCDC_DPI_OVERFLOW_MASK	(1<<3)
#define LCDC_DPI_FRAMEOVER_MAS	(1<<4)
#define LCDC_MIPI_INT_MASK		(1<<5)
#define LCDC_IRQ_MASK_ALL		(0x3f)

/*dsi irq status*/
#define DSI_TX_END_FLAG			(1 << 0)
#define DSI_RX_END_FLAG			(1 << 1)

/* lcd mode */
#define LCD_DSI_MODE			(0x1)

struct dsi_timing {
	u32 off;
	u32 value;
};

struct rda_dsi_phy_ctrl {
	u32 pll[6];
	struct dsi_timing video_timing[13];
	struct dsi_timing cmd_timing[8];
};

struct mipi_panel_info {
	u8 data_lane;
	u8 mipi_mode;
	u8 pixel_format;
	u8 dsi_format;
	u8 trans_mode;
	u8 rgb_order;
	bool bllp_enable;
	u32 h_sync_active;
	u32 h_back_porch;
	u32 h_front_porch;
	u32 v_sync_active;
	u32 v_back_porch;
	u32 v_front_porch;
	u8 frame_rate;
	u8 te_sel;
	u32 dsi_pclk_rate;
	const struct rda_dsi_phy_ctrl *dsi_phy_db;
};

struct lcd_panel_info {
	u16 width;
	u16 height;
	u16 bpp;
	struct mipi_panel_info mipi_pinfo;
};

#define LCDC_OUTL(addr, data) writel((data), (addr))
#define LCDC_INL(addr) readl(addr)

#define LCDC_OR_WITH_REG(addr, data)	\
			do{ \
				LCDC_OUTL(addr,data | LCDC_INL(addr));	\
			}while(0);

#define LCDC_AND_WITH_REG(addr, data)	\
			do{ \
				LCDC_OUTL(addr,data & LCDC_INL(addr));	\
			}while(0);

PUBLIC VOID rda_write_dsi_reg(u32 offset, u32 value);
PUBLIC U32 rda_read_dsi_reg(u32 offset);
PUBLIC VOID set_lcdc_for_cmd(u32 addr, int group_num);
PUBLIC VOID set_lcdc_for_video(u32 addr, const struct lcd_panel_info *lcd);
PUBLIC VOID rda_lcdc_set(const struct rda_dsi_phy_ctrl*dsi_phy_db);
PUBLIC INT  rda_lcdc_irq_status(void);

#endif /*__RDA_LCDC_H*/

