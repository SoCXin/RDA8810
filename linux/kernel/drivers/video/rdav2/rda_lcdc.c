#include <linux/ioport.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/err.h>
#include <linux/workqueue.h>
#include <linux/timer.h>
#include <linux/semaphore.h>
#include <linux/io.h>
#include <asm/sizes.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <mach/hardware.h>
#include <mach/irqs.h>
#include <plat/devices.h>
#include <plat/reg_sysctrl.h>
#include <plat/rda_display.h>
#include <plat/rda_debug.h>
#include <plat/ap_clk.h>
#include <linux/notifier.h>
#include <plat/pm_ddr.h>

#include <mach/rda_clk_name.h>

#include "tgt_ap_clock_config.h"
#include "rda_lcdc.h"
#include "rda_mipi_dsi.h"

#ifndef _TGT_AP_RGB_SCALE_LEVLEL
#define _TGT_AP_RGB_SCALE_LEVLEL 0
#endif

static struct rda_lcdc *rda_lcdc = NULL;
static unsigned char * lcdc_reg_base;
static unsigned char * tcc_reg_base;/* tv_clk_ctrl reg base*/

#define LCDC_REG_BASE lcdc_reg_base
#define TCC_REG_BASE tcc_reg_base

static void lcdc_irq_mask_clear_and_set(u32 irqMask)
{
	u32 value;

	value = LCDC_INL(LCDC_REG_BASE + LCDC_EOF_IRQ_MASK);
	value &= ~irqMask;
	LCDC_OUTL(LCDC_REG_BASE + LCDC_EOF_IRQ_MASK,value);
	value |= irqMask;
	LCDC_OUTL(LCDC_REG_BASE + LCDC_EOF_IRQ_MASK,value);
}

static void lcdc_irq_mask_clear(int irqMask)
{
	u32 value;

	value = LCDC_INL(LCDC_REG_BASE + LCDC_EOF_IRQ_MASK);
	value &= ~irqMask;
	LCDC_OUTL(LCDC_REG_BASE + LCDC_EOF_IRQ_MASK,value);

}

static void lcdc_irq_mask_set(u32 irqMask)
{
	LCDC_OUTL(LCDC_REG_BASE + LCDC_EOF_IRQ_MASK,irqMask);
}

static void lcdc_clear_irq_status(u32 statusMask)
{
	LCDC_OUTL(LCDC_REG_BASE + LCDC_EOF_IRQ,statusMask);
}

static u32 lcdc_get_irq_status(void)
{
	return LCDC_INL(LCDC_REG_BASE + LCDC_EOF_IRQ);
}

static u32 lcdc_get_dsi_irq_status(void)
{
	u32 value;
	value = LCDC_INL(LCDC_REG_BASE + DSI_REG_OFFSET + 0x90);

	return value;
}

static void lcdc_mipi_irq_clear(void)
{
	u32 reg;
	reg = rda_read_dsi_reg(0x48);
	reg |= 0x1 << 2;
	rda_write_dsi_reg(0x48,0x1 << 2);

#if RDA8850E_WORKAROUND
	/*
	* the dsi irq not pulse now,so we need to write 0 to pull down the level irq
	* should be remove in future
	*/
	rda_write_dsi_reg(0x48,reg & ~(1 << 2));
#endif
}

void enable_lcdc_clk(bool on)
{
	if(on) {
		if(rda_lcdc->lcdc_clk_on) {
			pr_warn("%s : lcdc clk already enabled.\n", __func__);
			return;
		}
		rda_dbg_lcdc("dpi clk on\n");
		clk_enable(rda_lcdc->clk_dpi);/* DBI,DPI,DSI need clk dpi*/
		if(rda_lcdc->lcd->lcd_interface == LCD_IF_DSI){
			clk_enable(rda_lcdc->clk_dsi);
		}
		rda_lcdc->lcdc_clk_on = true;
	} else {
		if(!rda_lcdc->lcdc_clk_on) {
			pr_warn("%s : lcdc clk already disabled.\n", __func__);
			return;
		}
		rda_dbg_lcdc("dpi clk off\n");
		clk_disable(rda_lcdc->clk_dpi);
		if(rda_lcdc->lcd->lcd_interface == LCD_IF_DSI){
			clk_disable(rda_lcdc->clk_dsi);
		}
		rda_lcdc->lcdc_clk_on = false;
	}
}

int rda_lcdc_dbi_write_cmd2lcd(u16 addr)
{
	u32 status;
	int err_status = 0;
	unsigned long timeout = 0;

	status = LCDC_INL(LCDC_REG_BASE + LCDC_STATUS);
	if (status != 0) {
		/* lcdc is busy */
		err_status = -1;
	} else {
		LCDC_OUTL(LCDC_REG_BASE + LCDC_SINGLE_ACCESS,
			LCDC_DBI_SGL_WRITE | LCDC_DBI_SGL_DATA(addr));

		status = LCDC_INL(LCDC_REG_BASE + LCDC_STATUS);
		timeout = jiffies + msecs_to_jiffies(DBI_SGL_TIMEOUT);
		while ((status & LCDC_LCD_BUSY)){
			status = LCDC_INL(LCDC_REG_BASE + LCDC_STATUS);
			if (time_after(jiffies, timeout)){
				pr_err("%s write cmd error\n",__func__);
				return -1;
			}
		}
		err_status = 0;
	}

	return err_status;
}

int rda_lcdc_dbi_write_data2lcd(u16 data)
{
	u32 status;
	int err_status = 0;
	unsigned long timeout = 0;

	status = LCDC_INL(LCDC_REG_BASE + LCDC_STATUS);
	if (status != 0) {
		/* lcdc is busy */
		rda_dbg_lcdc("lcdc busy\n");
		err_status = -1;
	} else {
		LCDC_OUTL(LCDC_REG_BASE + LCDC_SINGLE_ACCESS,
		LCDC_DBI_SGL_WRITE | LCDC_DBI_SGL_DATA_TYPE | LCDC_DBI_SGL_DATA(data));

		status = LCDC_INL(LCDC_REG_BASE + LCDC_STATUS);
		timeout = jiffies + msecs_to_jiffies(DBI_SGL_TIMEOUT);
		while ((status & LCDC_LCD_BUSY)){
			status = LCDC_INL(LCDC_REG_BASE + LCDC_STATUS);
			if (time_after(jiffies, timeout)){
				pr_err("%s write data error\n",__func__);
				return -1;
			}
		}
		err_status = 0;
	}

	return err_status;
}

int rda_lcdc_dbi_read_data(u8 * data)
{
	u32 status;
	int err_status = 0;
	unsigned long timeout = 0;

	status = LCDC_INL(LCDC_REG_BASE + LCDC_STATUS);
	if ((status & LCDC_LCD_BUSY) != 0) {
		/* lcdc is busy */
		rda_dbg_lcdc("lcdc busy\n");
		err_status = -1;
	} else {
		/* Start to read */
		LCDC_OUTL(LCDC_REG_BASE + LCDC_SINGLE_ACCESS,
			LCDC_DBI_SGL_READ | LCDC_DBI_SGL_DATA_TYPE);

		status = LCDC_INL(LCDC_REG_BASE + LCDC_STATUS);
		timeout = jiffies + msecs_to_jiffies(DBI_SGL_TIMEOUT);
		while ((status & LCDC_LCD_BUSY)){
			status = LCDC_INL(LCDC_REG_BASE + LCDC_STATUS);
			if (time_after(jiffies, timeout)){
				pr_err("%s write data error\n",__func__);
				return -1;
			}
		}
		*data = LCDC_INL(LCDC_REG_BASE + LCDC_SINGLE_ACCESS) & 0xff;
		err_status = 0;
	}

	return err_status;
}

int rda_lcdc_dbi_read_data16(u16 * data)
{
	u32 status;
	int err_status = 0;
	unsigned long timeout = 0;

	status = LCDC_INL(LCDC_REG_BASE + LCDC_STATUS);
	if ((status & LCDC_LCD_BUSY) != 0) {
		/* lcdc is busy */
		rda_dbg_lcdc("lcdc busy\n");
		err_status = -1;
	} else {
		/* Start to read */
		LCDC_OUTL(LCDC_REG_BASE + LCDC_SINGLE_ACCESS,
			LCDC_DBI_SGL_READ | LCDC_DBI_SGL_DATA_TYPE);

		status = LCDC_INL(LCDC_REG_BASE + LCDC_STATUS);
		timeout = jiffies + msecs_to_jiffies(DBI_SGL_TIMEOUT);
		while ((status & LCDC_LCD_BUSY)){
			status = LCDC_INL(LCDC_REG_BASE + LCDC_STATUS);
			if (time_after(jiffies, timeout)){
				pr_err("%s write data error\n",__func__);
				return -1;
			}
		}
		*data = LCDC_INL(LCDC_REG_BASE + LCDC_SINGLE_ACCESS) & 0xffff;
		err_status = 0;
	}

	return err_status;
}

/*
static u32 lcdc_status(void)
{
	return LCDC_INL(LCDC_REG_BASE + LCDC_STATUS);
}
*/

static u32 set_mcu_timing(struct lcd_panel_info *lcd)
{
	u32 lcdc_mcu_timing = 0;

	lcdc_mcu_timing = LCDC_TAS(lcd->mcu_pinfo.tas)
		| LCDC_TAH(lcd->mcu_pinfo.tah) | LCDC_PWL(lcd->mcu_pinfo.pwl)
		| LCDC_PWH(lcd->mcu_pinfo.pwh);

	return lcdc_mcu_timing;
}

static u32 set_mcu_lcd_contorl(struct lcd_panel_info *lcd)
{
	u32 lcdc_lcd_ctrl = 0;

	lcdc_lcd_ctrl |= LCDC_DESTINATION(lcd->mcu_pinfo.cs);
	lcdc_lcd_ctrl |= LCDC_OUTPUT_FORMAT(lcd->mcu_pinfo.output_fmt);

	if(lcd->mcu_pinfo.highByte)
		lcdc_lcd_ctrl |= LCDC_HIGH_BYTE;
	if(lcd->mcu_pinfo.cs0_polarity)
		lcdc_lcd_ctrl |= LCDC_CS0_POLARITY;
	if(lcd->mcu_pinfo.cs1_polarity)
		lcdc_lcd_ctrl |= LCDC_CS1_POLARITY;
	if(lcd->mcu_pinfo.rs_polarity)
		lcdc_lcd_ctrl |= LCDC_RS_POLARITY;
	if(lcd->mcu_pinfo.rd_polarity)
		lcdc_lcd_ctrl |= LCDC_RD_POLARITY;

	if(lcd->bpp == 16){
		lcdc_lcd_ctrl |= LCDC_BUS_SEL_16BIT;
	}else if (lcd->bpp == 18){
		lcdc_lcd_ctrl |= LCDC_BUS_SEL_18BIT;
	}else if (lcd->bpp == 24){
		lcdc_lcd_ctrl |= LCDC_BUS_SEL_24BIT;
	}

	return lcdc_lcd_ctrl;
}

static void config_dbi_reg(struct rda_lcdc *lcdc)
{
	u32 addr = lcdc->frame_addr;
	int bpp = lcdc->lcd->bpp;
	struct lcd_panel_info *lcd = lcdc->lcd;
	rda_dbg_lcdc("%s \n", __func__);

	LCDC_OUTL(LCDC_REG_BASE + DPI_FRAME0_ADDR,addr);
	LCDC_OUTL(LCDC_REG_BASE + DPI_FRAME0_CON,
		(DPI_FRAME_LINE_STEP((bpp >> 3) * lcd->width) | DPI_FRAME_VALID));

	LCDC_OUTL(LCDC_REG_BASE + DPI_SIZE,
		VERTICAL_PIX_NUM(lcd->height) | HORIZONTAL_PIX_NUM(lcd->width));

	LCDC_OUTL(LCDC_REG_BASE + LCDC_DCT_SHIFT_UV_REG1,LCDC_RGB_WAIT);
	LCDC_OUTL(LCDC_REG_BASE + DPI_CONFIG,
		RGB_PIX_FMT((bpp >> 3) == 2 ? RGB_PIX_FMT_RGB565 :
		(bpp >> 3) == 3 ? RGB_PIX_FMT_RGB888 : RGB_PIX_FMT_XRGB8888)	|
		RGB_OUTOFF_DATA(1) | REG_PEND_REQ(16));

	LCDC_OUTL(LCDC_REG_BASE + LCDC_SPILCD_CONFIG,0);
	LCDC_OUTL(LCDC_REG_BASE + TECON,lcd->mcu_pinfo.te_en);
	LCDC_OUTL(LCDC_REG_BASE + TECON2,lcd->mcu_pinfo.tecon2);
	LCDC_OUTL(LCDC_REG_BASE + LCD_TIMING,set_mcu_timing(lcd));
	LCDC_OUTL(LCDC_REG_BASE + LCD_CTRL,set_mcu_lcd_contorl(lcd));

	LCDC_OUTL(LCDC_REG_BASE + DPI_FIFO_CTRL,DPI_DATA_FIFO_RST_AUTO);
	lcdc_irq_mask_clear_and_set(LCDC_EOF_MASK);
}

static void config_dpi_reg(struct rda_lcdc *lcdc)
{
	u32 val = 0;
	u32 addr = lcdc->frame_addr;
	u8 bpp = lcdc->lcd->bpp;
	unsigned long lcdc_clk = clk_get_rate(lcdc->clk_dpi);
	struct lcd_panel_info *lcd = lcdc->lcd;

	lcdc_irq_mask_set(LCDC_DPI_FRAME_OVER_MASK);

	/* Calculate a divider used by dpi lcd. */
	if (!lcd->rgb_pinfo.pclk_divider &&
		lcd->rgb_pinfo.pclk) {
		val = lcdc_clk / lcd->rgb_pinfo.pclk;
		lcd->rgb_pinfo.pclk_divider = (val > 255 ? 255 : val);
	}

	val |= ((lcd->rgb_pinfo.data_pol&1)<<11
		| (lcd->rgb_pinfo.v_pol&1)<<10
		| (lcd->rgb_pinfo.h_pol&1)<<9
		| (lcd->rgb_pinfo.dot_clk_pol&1)<<8
		| DPI_CLK_ADJ(lcd->rgb_pinfo.dpi_clk_adj));

	LCDC_OUTL(TCC_REG_BASE + RGB_IF_DIV,
		RGB_IF_DIVIDER(lcd->rgb_pinfo.pclk_divider));
	LCDC_OUTL(TCC_REG_BASE + RGB_IF_EN,RGB_IF_DOT_ENABLE);

	LCDC_OUTL(LCDC_REG_BASE + DPI_POL,val);
	LCDC_OUTL(LCDC_REG_BASE + DPI_TIME0,
		BACK_PORCH_END_VSYNC_TIMER(
		lcd->rgb_pinfo.v_low +
		lcd->rgb_pinfo.v_back_porch) |
		FRONT_PORCH_STATRT_TIMER(
		lcd->rgb_pinfo.v_low +
		lcd->rgb_pinfo.v_back_porch +
		lcd->height));

	LCDC_OUTL(LCDC_REG_BASE + DPI_TIME1,
		VSYNC_INCLUDE_HSYNC_TH_LOW(
		lcd->rgb_pinfo.v_low) |
		VSYNC_INCLUDE_HSYNC_TH_HIGH(
		lcd->rgb_pinfo.v_low +
		lcd->rgb_pinfo.v_back_porch +
		lcd->rgb_pinfo.v_front_porch - 1 +
		lcd->height));

	LCDC_OUTL(LCDC_REG_BASE + DPI_TIME2,
		HSYNC_INCLUDE_DOTCLK_TH_LOW(
		lcd->rgb_pinfo.h_low) |
		HSYNC_INCLUDE_DOTCLK_TH_HIGH(
		lcd->rgb_pinfo.h_low +
		lcd->width +
		lcd->rgb_pinfo.h_back_porch +
		lcd->rgb_pinfo.h_front_porch - 1));

	LCDC_OUTL(LCDC_REG_BASE + DPI_TIME3,
		RGB_DATA_ENABLE_START_TIMER(
		lcd->rgb_pinfo.h_low +
		lcd->rgb_pinfo.h_back_porch) |
		RGB_DATA_ENABLE_END_TIMER(
		lcd->rgb_pinfo.h_low +
		lcd->width +
		lcd->rgb_pinfo.h_back_porch));

	LCDC_OUTL(LCDC_REG_BASE + DPI_SIZE,
		VERTICAL_PIX_NUM(lcd->height) |
		HORIZONTAL_PIX_NUM(lcd->width));

	LCDC_OUTL(LCDC_REG_BASE + DPI_FRAME0_CON,
		(DPI_FRAME_LINE_STEP((bpp >> 3) * lcd->width) | DPI_FRAME_VALID));

	LCDC_OUTL(LCDC_REG_BASE + LCDC_MEM_ADDRESS,addr);
	LCDC_OUTL(LCDC_REG_BASE + DPI_FRAME0_ADDR,(UINT32)addr);
	LCDC_OUTL(LCDC_REG_BASE + LCD_CTRL,LCD_IN_RAM);

	LCDC_OUTL(LCDC_REG_BASE + LCDC_DCT_SHIFT_UV_REG1,
		HSYNC_INCLUDE_DOTCLK_TH_HIGH(
		lcd->width +
		lcd->rgb_pinfo.h_back_porch +
		lcd->rgb_pinfo.h_front_porch - 1));

	LCDC_OUTL(LCDC_REG_BASE + DPI_CONFIG,
		(lcd->rgb_pinfo.rgb_format_bits << 12)
		| (lcd->rgb_pinfo.frame1 << 1)
		| (lcd->rgb_pinfo.frame2 << 2)
		| RGB_PIX_FMT(lcd->rgb_pinfo.pixel_format)
		| RGB_PIX_ORDER(lcd->rgb_pinfo.rgb_order)
		| REG_PEND_REQ(16));

	LCDC_OR_WITH_REG(LCDC_REG_BASE + DPI_CONFIG,RGB_PANEL_ENABLE);
	LCDC_OUTL(LCDC_REG_BASE + DPI_FIFO_CTRL,DPI_DATA_FIFO_RST_AUTO);
}

u32 rda_read_dsi_reg(u32 offset)
{
	u32 value;

	value = LCDC_INL(LCDC_REG_BASE + DSI_REG_OFFSET + offset);
	return value;
}

void rda_write_dsi_reg(u32 offset,u32 value)
{
	LCDC_OUTL(LCDC_REG_BASE + DSI_REG_OFFSET + offset,value);
}

int rda_mipi_dsi_txrx_status(void)
{
	u8 time_out = 0;
	do{
		if((rda_lcdc->dsi_txrx_status & DSI_TX_END_FLAG) ||
			(rda_lcdc->dsi_txrx_status & DSI_RX_END_FLAG)){
			rda_lcdc->dsi_txrx_status = 0;
			return 0;
		}
		udelay(100);
	}while(time_out++ < 10);/* wait 1 ms */

	return -1;
}

void rda_dsi_enable(bool on)
{
	rda_write_dsi_reg(0x04,on ? 0x1 : 0);
}

static void mipi_dsi_pll_on(bool enable)
{
	if(enable)
		rda_write_dsi_reg(0x0,0x3);/*enable pll*/
	else
		rda_write_dsi_reg(0x0,0x0);/*disable pll*/
}

static void set_dsi_set_rate(struct clk *clk,unsigned long rate)
{
	rda_dbg_lcdc("set_dsi_set_rate\n");

	/*enable mipi pll*/
	mipi_dsi_pll_on(true);
	clk_set_rate(clk,rate);
}

void adjust_dsi_phy_phase(u32 value)
{
	int i;
	u32 phase = value & 0xf;

	for(i = 0;i < 9;i++){
		rda_write_dsi_reg(0x144,((phase + i) % 8)|(value & 0xf0));
		udelay(1);
	}
}

static void reset_lcdc_fifo(void)
{
	LCDC_AND_WITH_REG(LCDC_REG_BASE + DPI_FIFO_CTRL,~DPI_DATA_FIFO_RST_AUTO);
	LCDC_OR_WITH_REG(LCDC_REG_BASE + DPI_FIFO_CTRL,DPI_DATA_FIFO_RST);
	LCDC_AND_WITH_REG(LCDC_REG_BASE + DPI_FIFO_CTRL,~DPI_DATA_FIFO_RST);
}

static void set_dsi_timing(const struct rda_dsi_phy_ctrl *dsi_phy_db)
{
	int i;
	u32 off,value;

	reset_lcdc_fifo();

	rda_dsi_enable(false);
	rda_dsi_op_mode_config(DSI_CMD);

	/* cmd mode */
	for(i = 0;i < ARRAY_SIZE(dsi_phy_db->cmd_timing);i++){
		off = dsi_phy_db->cmd_timing[i].off;
		value = dsi_phy_db->cmd_timing[i].value;
		rda_write_dsi_reg(off,value);
	}
	/* video mode */
	for(i = 0;i < ARRAY_SIZE(dsi_phy_db->video_timing);i++){
		off = dsi_phy_db->video_timing[i].off;
		value = dsi_phy_db->video_timing[i].value;
		rda_write_dsi_reg(off,value);
	}
}

static void rda_set_dsi_parameter(struct rda_lcdc *lcdc)
{
	/*pixel num*/
	rda_write_dsi_reg(0xc,lcdc->lcd->width);

	/*pixel type*/
	rda_write_dsi_reg(0x10,lcdc->lcd->mipi_pinfo.dsi_format);

	/*vc,bllp enable*/
	if(lcdc->lcd->mipi_pinfo.bllp_enable)
		rda_write_dsi_reg(0x18,0x1 << 2);

	/*
	* h/v b/f porch,the same calculate method to rgb num,
	* we use the precalculated value other than the way of set rgb num
	*/
	rda_write_dsi_reg(0x20,lcdc->lcd->mipi_pinfo.h_sync_active);
	rda_write_dsi_reg(0x24,lcdc->lcd->mipi_pinfo.h_back_porch);
	rda_write_dsi_reg(0x28,lcdc->lcd->mipi_pinfo.h_front_porch);

	rda_write_dsi_reg(0x30,lcdc->lcd->mipi_pinfo.v_sync_active - 1);
	rda_write_dsi_reg(0x34,lcdc->lcd->mipi_pinfo.v_back_porch - 1);
	rda_write_dsi_reg(0x38,lcdc->lcd->mipi_pinfo.v_front_porch - 1);
	rda_write_dsi_reg(0x3c,lcdc->lcd->height - 1);/*vat line */

	/*
	* sync data swap
	*/
	rda_write_dsi_reg(0x104,0x2);
}

void rda_set_lcdc_for_dsi_cmdlist_tx(u32 cmdlist_addr,int cmd_cnt)
{
	rda_dbg_lcdc("rda_set_lcdc_for_dsi_cmdlist_tx\n");

	/*
	* we put the cmds on ddr when dsi send long cmds,a cmd takes only 1 byte,but we
	* use 8 bytes and the low 1 byte is cmd,all the cmds send by mipi as a line,
	* a cmd takes 2 pix num(pix  fmt xrgb8888).
	*/

	if(cmdlist_addr && cmd_cnt) {
		LCDC_OUTL(LCDC_REG_BASE + DPI_FRAME0_ADDR,cmdlist_addr);
		LCDC_OUTL(LCDC_REG_BASE + DPI_FRAME0_CON,(DPI_FRAME_LINE_STEP(cmd_cnt * 8) | DPI_FRAME_VALID));
		LCDC_OUTL(LCDC_REG_BASE + DPI_SIZE,VERTICAL_PIX_NUM(1) | HORIZONTAL_PIX_NUM(cmd_cnt * 2));
	}
	LCDC_OUTL(LCDC_REG_BASE + LCDC_DCT_SHIFT_UV_REG1,LCDC_RGB_WAIT | 0x143);/* 0x143 is a random value <  0x7ff  */
	LCDC_OUTL(LCDC_REG_BASE + DPI_CONFIG,MIPI_CMD_SEL_CMD | MIPI_DSI_ENABLE | RGB_PIX_FMT(RGB_PIX_FMT_XRGB8888));
}

void set_lcdc_for_dsi_video(struct rda_lcdc *lcdc)
{
	rda_dbg_lcdc("set_lcdc_for_dsi_video\n");

	reset_lcdc_fifo();

	rda_write_dsi_reg(0x04,0);
	rda_write_dsi_reg(0x144,0x14);

	/*dsi phy init*/
	mipi_dsi_pll_on(true);

	LCDC_OUTL(LCDC_REG_BASE + DPI_FRAME0_ADDR,lcdc->frame_addr);
	LCDC_OUTL(LCDC_REG_BASE + DPI_FRAME0_CON,(DPI_FRAME_LINE_STEP(lcdc->lcd->width * (lcdc->lcd->bpp >> 3)) | DPI_FRAME_VALID));

	LCDC_OUTL(LCDC_REG_BASE + DPI_SIZE,
		VERTICAL_PIX_NUM(lcdc->lcd->height) | HORIZONTAL_PIX_NUM(lcdc->lcd->width));
	LCDC_OUTL(LCDC_REG_BASE + LCDC_DCT_SHIFT_UV_REG1,LCDC_RGB_WAIT | 0x143);/* 0x143 is a random value <  0x7ff  */
	LCDC_OUTL(LCDC_REG_BASE + DPI_CONFIG, MIPI_DSI_ENABLE | RGB_PIX_FMT(lcdc->lcd->mipi_pinfo.pixel_format)
		| RGB_PIX_ORDER(lcdc->lcd->mipi_pinfo.rgb_order) | REG_PEND_REQ(16));

	if(lcdc->lcd->mipi_pinfo.pixel_format == RGB_PIX_FMT_RGB565)
		LCDC_OUTL(LCDC_REG_BASE + LCDC_DCT_SHIFT_UV_REG1,0x143);/* 0x143 is a random value <  0x7ff  */

	LCDC_OUTL(LCDC_REG_BASE + DPI_FIFO_CTRL,DPI_DATA_FIFO_RST_AUTO);
}

static void enable_dsi_video(struct rda_lcdc *lcdc)
{
	u32 dsi_config = 0;
	u32 dsi_bpp;
	rda_dbg_lcdc("enable_dsi_video\n");

	/*lane config*/
	dsi_config = MIPI_DSI_LANE(lcdc->lcd->mipi_pinfo.data_lane >> 1);
	rda_write_dsi_reg(0x8,dsi_config);

	/*rgb num, width bytes	+ header + crc*/
	dsi_bpp = lcdc->lcd->bpp != 32 ? lcdc->lcd->bpp : 24;
	rda_write_dsi_reg(0x2c,(lcdc->lcd->width * (dsi_bpp >> 3)+ 6) >>
			(lcdc->lcd->mipi_pinfo.data_lane >> 1));

	/*mipi tx mode*/
	rda_dsi_op_mode_config(lcdc->lcd->mipi_pinfo.trans_mode);
	rda_dsi_enable(true);
}

static void config_dsi_reg(struct rda_lcdc *lcdc)
{
	struct lcd_panel_info *lcd = lcdc->lcd;
	unsigned long rate;

	const struct rda_dsi_phy_ctrl *dsi_phy_db = lcd->mipi_pinfo.dsi_phy_db;
	rate = lcd->mipi_pinfo.dsi_pclk_rate;
	rda_mipi_dsi_init(&lcd->mipi_pinfo);
	set_dsi_set_rate(rda_lcdc->clk_dsi,rate * 1000000);
	set_dsi_timing(dsi_phy_db);
	rda_set_dsi_parameter(rda_lcdc);
	lcdc_irq_mask_clear_and_set(LCDC_MIPI_INT_MASK);/*enable mipi_int*/
}

static int lcdc_wait_done(struct rda_lcdc *lcdc)
{
	wait_event_timeout(lcdc->wait_eof, !lcdc->is_active, HZ / 2);
	if (lcdc->is_active) {
		pr_err("%s : timeout\n", __func__);
		return -ETIMEDOUT;
	}

	return 0;
}

static int lcdc_wait_frameover(struct rda_lcdc *lcdc)
{
	rda_dbg_lcdc("%s \n",__func__);
	wait_event_timeout(lcdc->wait_frmover, !lcdc->is_active, HZ / 2);
	if (lcdc->is_active) {
		pr_err("%s : timeout\n", __func__);
		return -ETIMEDOUT;
	}
	return 0;
}

static void lcdc_update_img_dsi(UINT32 addr, bool sync)
{
	rda_dbg_lcdc("%s \n",__func__);

	rda_lcdc->frame_addr = addr;
	LCDC_OUTL(LCDC_REG_BASE + DPI_FRAME0_ADDR,addr);
	rda_lcdc->is_active = true;
	if (sync) {
		lcdc_wait_frameover(rda_lcdc);
	}
}

static void lcdc_update_img_dpi(UINT32 addr, bool sync)
{
	rda_dbg_lcdc("%s \n",__func__);

	rda_lcdc->frame_addr = addr;
	LCDC_OUTL(LCDC_REG_BASE + DPI_FRAME0_ADDR,addr);
	rda_lcdc->is_active = true;
	if (sync) {
		lcdc_wait_frameover(rda_lcdc);
	}
}

static void lcdc_update_img_dbi(UINT32 addr, bool sync)
{
	rda_dbg_lcdc("%s \n",__func__);

	rda_lcdc->frame_addr = addr;
	LCDC_OUTL(LCDC_REG_BASE + DPI_FRAME0_ADDR,addr);

	lcdc_clear_irq_status(LCDC_IRQ_CLEAR_ALL);
	LCDC_OUTL(LCDC_REG_BASE + LCDC_COMMAND,LCDC_START);/*start dbi*/

	rda_lcdc->is_active = true;
	if (sync) {
		lcdc_wait_done(rda_lcdc);
	}
}

void rda_lcdc_display(struct rda_fb *rdafb)
{
	struct rda_lcd_info *info = rdafb->lcd_info;
	rda_dbg_lcdc("%s \n",__func__);

	if(info->lcd.lcd_interface == LCD_IF_DBI){
		rda_lcdc_pre_wait_and_enable_clk();
		lcdc_update_img_dbi((UINT32)rdafb->fb_addr, true);
	}else if (info->lcd.lcd_interface == LCD_IF_DPI){
		lcdc_update_img_dpi((UINT32)rdafb->fb_addr, true);
	}else if (info->lcd.lcd_interface == LCD_IF_DSI){
		lcdc_update_img_dsi((UINT32)rdafb->fb_addr, true);
	}else
		pr_err("%s : error lcdc interface.\n", __func__);
	return;
}

int rda_lcdc_pre_wait_and_enable_clk(void)
{
	long ret;

	if (rda_lcdc->lcd->lcd_interface == LCD_IF_DBI) {
		ret = wait_event_timeout(rda_lcdc->wait_eof,  !rda_lcdc->is_active, HZ / 2);
		if (ret == 0 && rda_lcdc->is_active) {
			pr_err("%s : lcdc is active.\n", __func__);
			return -ETIMEDOUT;
		}
		enable_lcdc_clk(true);
	}

	return 0;
}

void rda_lcdc_post_disable_clk(void)
{
	if (rda_lcdc->lcd->lcd_interface == LCD_IF_DBI) {
		enable_lcdc_clk(false);
	}
}

void rda_lcdc_pre_enable_lcd(struct lcd_panel_info *lcd,int onoff)
{
	struct clk *pre_clk = NULL;
	u32 status;

	pre_clk = clk_get(NULL, RDA_CLK_DPI);
	if (!pre_clk) {
		rda_dbg_lcdc("failed to get lcdc clk controller\n");
		return;
	}

	if(onoff){
		clk_enable(pre_clk);
		if (lcd->lcd_interface == LCD_IF_DBI) {
			status = LCDC_INL(LCDC_REG_BASE + LCDC_STATUS);
			rda_dbg_lcdc("hwp_lcdc->gd_status 0x%x \n",status);
			LCDC_OUTL(LCDC_REG_BASE + LCDC_SPILCD_CONFIG,0);
			LCDC_OUTL(LCDC_REG_BASE + TECON,lcd->mcu_pinfo.te_en);
			LCDC_OUTL(LCDC_REG_BASE + TECON2,lcd->mcu_pinfo.tecon2);
			LCDC_OUTL(LCDC_REG_BASE + LCD_TIMING,set_mcu_timing(lcd));
			LCDC_OUTL(LCDC_REG_BASE + LCD_CTRL,set_mcu_lcd_contorl(lcd));
		} else if (lcd->lcd_interface == LCD_IF_DSI) {
			unsigned long rate;
			int i;
			u32 off,value;
			const struct rda_dsi_phy_ctrl *dsi_phy_db = lcd->mipi_pinfo.dsi_phy_db;
			clk_enable(rda_lcdc->clk_dsi);
			rate = lcd->mipi_pinfo.dsi_pclk_rate;
			rda_mipi_dsi_init(&lcd->mipi_pinfo);
			set_dsi_set_rate(rda_lcdc->clk_dsi,rate * 1000000);

			for (i = 0; i < ARRAY_SIZE(dsi_phy_db->cmd_timing); i++) {
				off = dsi_phy_db->cmd_timing[i].off;
				value = dsi_phy_db->cmd_timing[i].value;
				rda_write_dsi_reg(off, value);
			}
			lcdc_irq_mask_set(LCDC_MIPI_INT_MASK);
		}
	} else {
		if (lcd->lcd_interface == LCD_IF_DSI) {
			mipi_dsi_pll_on(false);
			lcdc_irq_mask_clear(LCDC_MIPI_INT_MASK);
			rda_mipi_dsi_deinit(&lcd->mipi_pinfo);
			clk_disable(rda_lcdc->clk_dsi);
		}
		clk_disable(pre_clk);
	}
}

void rda_lcdc_configure_timing(struct lcd_panel_info *lcd)
{
	if (lcd->lcd_interface == LCD_IF_DBI) {
		LCDC_OUTL(LCDC_REG_BASE + LCD_TIMING,set_mcu_timing(lcd));
	} else {
		printk("info: to do add spi reconfiguration timing here\n");
	}
}

void rda_reset_lcdc_for_dsi_tx(void)
{
	reset_lcdc_fifo();
	lcdc_mipi_irq_clear();
	lcdc_irq_mask_clear_and_set(LCDC_MIPI_INT_MASK);/*enable mipi_int*/
	lcdc_clear_irq_status(LCDC_IRQ_CLEAR_ALL);
}

void rda_lcdc_open_lcd(struct rda_fb *rdafb)
{
	struct lcd_panel_info *lcd = &rdafb->lcd_info->lcd;
	rda_dbg_lcdc("rda_lcdc_open_lcd\n");

	rda_lcdc->lcd = lcd;
	rda_lcdc->frame_addr = rdafb->fb_addr;

	enable_lcdc_clk(true);
	if (lcd->lcd_interface == LCD_IF_DBI) {
		config_dbi_reg(rda_lcdc);
	} else if (lcd->lcd_interface == LCD_IF_DPI) {
		config_dpi_reg(rda_lcdc);
	} else if (lcd->lcd_interface == LCD_IF_DSI){
		config_dsi_reg(rda_lcdc);
	}

	lcdc_clear_irq_status(LCDC_IRQ_CLEAR_ALL);
}

void rda_lcdc_enable_mipi(struct rda_fb *rdafb)
{
	struct lcd_panel_info *lcd = &rdafb->lcd_info->lcd;

	rda_dbg_lcdc("rda_lcdc_enable_mipi\n");

	lcdc_mipi_irq_clear();
	lcdc_clear_irq_status(LCDC_IRQ_CLEAR_ALL);/*clear all irq*/
	lcdc_irq_mask_set(LCDC_DPI_FRAME_OVER_MASK);/*enable frameover irq*/
	if (lcd->lcd_interface == LCD_IF_DSI){
		set_lcdc_for_dsi_video(rda_lcdc);
		enable_dsi_video(rda_lcdc);
	}
}

void rda_lcdc_close_lcd(struct lcd_panel_info *lcd)
{
	if (lcd->lcd_interface == LCD_IF_DBI){
		LCDC_OUTL(LCDC_REG_BASE + LCDC_SPILCD_CONFIG,0);
		LCDC_OUTL(LCDC_REG_BASE + LCD_CTRL,0);
	} else if (lcd->lcd_interface == LCD_IF_DPI){
	}
}

void rda_lcdc_reset(void)
{
	apsys_reset_lcdc();
}

#ifdef CONFIG_FB_RDA_USE_HWC
/*
static struct rda_fb * get_rda_fb_handle(struct rda_lcdc *rlcdc)
{
	struct rda_lcd_info *lcdinfo;
	struct rda_fb *rdafb;

	lcdinfo = container_of(rlcdc->lcd,struct rda_lcd_info,lcd);
	rdafb = container_of(&lcdinfo,struct rda_fb,lcd_info);

	return rdafb;
}
*/

static void vsync_handler(struct rda_lcdc *lcdc)
{
	lcdc->vsync_info.timestamp = ktime_get();
	complete_all(&lcdc->vsync_info.vsync_wait);
}

void rda_lcdc_set_vsync(int enable)
{
	rda_lcdc->vsync_enabled = !!enable;

	return;
}
#endif

static irqreturn_t rda_lcdc_interrupt(int irq, void *dev_id)
{
	unsigned long irq_flags;
	struct rda_lcdc *rlcdc = dev_id;
	uint32_t status;

	spin_lock_irqsave(&rlcdc->lock, irq_flags);
	status = lcdc_get_irq_status();

	rda_dbg_lcdc("rda_lcdc_interrupt status 0x%x \n",status);
	if (status & LCDC_EOF_CAUSE) {
		rda_dbg_lcdc("rda_lcdc_interrupt status LCDC_EOF_CAUSE \n");
		lcdc_clear_irq_status(LCDC_EOF_CAUSE);
		rlcdc->is_active = false;

#if RDA8850E_WORKAROUND
		if(rlcdc->lcd->lcd_interface == LCD_IF_DBI){
			rda_lcdc_reset();
			config_dbi_reg(rlcdc);
		}
#endif
		lcdc_irq_mask_clear_and_set(LCDC_EOF_MASK);
		lcdc_clear_irq_status(LCDC_IRQ_CLEAR_ALL);

		if(rlcdc->lcd->lcd_interface == LCD_IF_DBI){
			enable_lcdc_clk(false);
		}
		wake_up(&rlcdc->wait_eof);
	}

	if (status & LCDC_DPI_FRAME_OVER){
		rda_dbg_lcdc("rda_lcdc_interrupt status LCDC_DPI_FRAME_OVER \n");
		if(rlcdc->lcd->lcd_interface != LCD_IF_DBI){
			lcdc_clear_irq_status(LCDC_DPI_FRAME_OVER);
			rlcdc->is_active = false;
			lcdc_clear_irq_status(LCDC_IRQ_CLEAR_ALL);
			wake_up(&rlcdc->wait_frmover);
		}
#ifdef CONFIG_FB_RDA_USE_HWC
		if(rlcdc->vsync_enabled)
			vsync_handler(rlcdc);
#endif
	}

	if(status & LCDC_MIPI_INT){
		status = lcdc_get_dsi_irq_status();
		rda_dbg_lcdc("dsi  irq status 0x%x\n",status);
		rlcdc->dsi_txrx_status = 0;
		if(status & DSI_TX_END_FLAG){
			rlcdc->dsi_txrx_status |= DSI_TX_END_FLAG;
		}

		if(status & DSI_RX_END_FLAG){
			rlcdc->dsi_txrx_status |= DSI_RX_END_FLAG;
		}

		lcdc_mipi_irq_clear();
		lcdc_clear_irq_status(LCDC_IRQ_CLEAR_ALL);
	}
	spin_unlock_irqrestore(&rlcdc->lock, irq_flags);

	return status ? IRQ_HANDLED : IRQ_NONE;
}

static int rda_lcdc_suspend(struct device *dev)
{
	struct rda_lcdc *lcdc = dev_get_drvdata(dev);
	printk("%s \n",__func__);
	if(lcdc->lcd->lcd_interface == LCD_IF_DSI){
		rda_dsi_enable(false);
		mipi_dsi_switch_lp_mode();
	}

	return 0;
}

static int rda_lcdc_resume(struct device *dev)
{
	printk("%s \n",__func__);
	return 0;
}

#ifdef CONFIG_FB_RDA_USE_HWC
static ssize_t lcdc_vsync_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct rda_lcdc *lcdc = dev_get_drvdata(dev);
	ssize_t ret = 0;

	INIT_COMPLETION(lcdc->vsync_info.vsync_wait);

	wait_for_completion(&lcdc->vsync_info.vsync_wait);
	ret = snprintf(buf, PAGE_SIZE, "vsync_time=%llu",
			ktime_to_ns(lcdc->vsync_info.timestamp));
	buf[strlen(buf) + 1] = '\0';

	return ret;
}
static DEVICE_ATTR(vsync, S_IWUSR | S_IWGRP | S_IRUGO,
	lcdc_vsync_show, NULL);
#endif

static ssize_t rda_lcdc_enable_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct rda_lcdc *lcdc = dev_get_drvdata(dev);

	return sprintf(buf, "%d\n", lcdc->enabled);
}

static ssize_t rda_lcdc_enable_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	struct rda_lcdc *lcdc = dev_get_drvdata(dev);
	int ret;
	int set;

	printk("rda_lcdc_enable_store in\n");
	ret = kstrtoint(buf, 0, &set);
	if (ret < 0) {
		return ret;
	}

	set = !!set;

	if (lcdc->enabled == set) {
		return count;
	}

	lcdc->enabled = set;
	if (set) {
		ret = rda_lcdc_resume(dev);
	} else {
		ret = rda_lcdc_suspend(dev);
	}

	printk("rda_lcdc_enable_store out\n");
	return count;
}

static DEVICE_ATTR(enabled, S_IWUSR | S_IWGRP | S_IRUGO,
	rda_lcdc_enable_show, rda_lcdc_enable_store);

static struct attribute *rda_lcdc_attrs[] = {
	&dev_attr_enabled.attr,
#ifdef CONFIG_FB_RDA_USE_HWC
	&dev_attr_vsync.attr,
#endif
	NULL
};

static const struct attribute_group rda_lcdc_attr_group = {
	.attrs = rda_lcdc_attrs,
};

extern int rda_gouda_enable;
static int rda_lcdc_probe(struct platform_device *pdev)
{
	int ret;
	struct resource *mem;
	struct rda_lcdc *rlcdc;

	mem = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!mem) {
		dev_err(&pdev->dev, "no mem resource?\n");
		return -ENODEV;
	}

	rlcdc = kzalloc(sizeof(*rlcdc), GFP_KERNEL);
	if (rlcdc == NULL) {
		ret = -ENOMEM;
		goto err_rlcdc_alloc_failed;
	}

	rda_lcdc = rlcdc;
	rda_lcdc->is_active = false;
	rda_lcdc->dsi_txrx_status = 0;

	rlcdc->reg_base = ioremap(mem->start, resource_size(mem));
	lcdc_reg_base = rlcdc->reg_base;
	if (!rlcdc->reg_base) {
		dev_err(&pdev->dev, "rda lcdc ioremap fail\n");
		ret = -ENOMEM;
		goto err_iomap_fail;
	}

	rda_lcdc->tv_clk_ctrl = (void __iomem *)IO_ADDRESS(RDA_TV_CTRL_BASE);
	tcc_reg_base = rda_lcdc->tv_clk_ctrl;
	if (!rlcdc->tv_clk_ctrl) {
		dev_err(&pdev->dev, "rda tcc ioremap fail\n");
		ret = -ENOMEM;
		goto err_tv_ctrl_fail;
	}

	rlcdc->irq = platform_get_irq(pdev, 0);
	if (rlcdc->irq < 0) {
		dev_err(&pdev->dev, "fail to get irq\n");
		ret = -ENODEV;
		goto err_request_irq_failed;
	}
	ret = request_irq(rlcdc->irq, rda_lcdc_interrupt,
			  /*IRQF_SHARED*/0, pdev->name, rlcdc);
	if (ret)
		goto err_request_irq_failed;

	rlcdc->clk_dpi = clk_get(NULL, RDA_CLK_DPI);
	if (IS_ERR(rlcdc->clk_dpi)) {
		ret = PTR_ERR(rlcdc->clk_dpi);
		goto err_clk_failed;
	}
	clk_prepare(rlcdc->clk_dpi);

	rlcdc->clk_dsi = clk_get(NULL, RDA_CLK_DSI);
	if (IS_ERR(rlcdc->clk_dsi)) {
		ret = PTR_ERR(rlcdc->clk_dsi);
		goto err_clk_failed;
	}
	clk_prepare(rlcdc->clk_dsi);
	spin_lock_init(&rlcdc->lock);
	init_waitqueue_head(&rlcdc->wait_eof);
	init_waitqueue_head(&rlcdc->wait_frmover);

#ifdef CONFIG_FB_RDA_USE_HWC
	rlcdc->vsync_enabled = false;
	init_completion(&rlcdc->vsync_info.vsync_wait);
#endif
	platform_set_drvdata(pdev,rlcdc);

	ret = sysfs_create_group(&pdev->dev.kobj, &rda_lcdc_attr_group);
	if (ret < 0) {
		goto err_sysfs_create_group;
	}

	rda_dbg_lcdc("rda_lcdc_probe, reg_base = 0x%08x, irq = %d\n",
		      mem->start, rlcdc->irq);

	return 0;

err_sysfs_create_group:
err_clk_failed:
	free_irq(rlcdc->irq, rlcdc);
err_request_irq_failed:
err_tv_ctrl_fail:
	iounmap(rlcdc->reg_base);
err_iomap_fail:
	kfree(rlcdc);
err_rlcdc_alloc_failed:
	return ret;
}

static int rda_lcdc_remove(struct platform_device *pdev)
{
	struct rda_lcdc *lcdc = platform_get_drvdata(pdev);
	printk("lcdc remove\n");

	if (!lcdc) {
		return -EINVAL;
	}

	clk_unprepare(lcdc->clk_dpi);
	clk_put(lcdc->clk_dpi);

	clk_unprepare(lcdc->clk_dsi);
	clk_put(lcdc->clk_dsi);

	free_irq(lcdc->irq, lcdc);

	kfree(lcdc);
	rda_lcdc = NULL;

	iounmap(rda_lcdc->reg_base);

	return 0;
}

static struct platform_driver rda_lcdc_driver = {
	.probe = rda_lcdc_probe,
	.remove = rda_lcdc_remove,
	.driver = {
		   .name = RDA_LCDC_DRV_NAME,
	}
};

static int __init rda_lcdc_init_module(void)
{
	return platform_driver_register(&rda_lcdc_driver);
}

static void __exit rda_lcdc_exit_module(void)
{
	platform_driver_unregister(&rda_lcdc_driver);
}

module_init(rda_lcdc_init_module);
module_exit(rda_lcdc_exit_module);

MODULE_AUTHOR("RDA lcdc@rdamicro.com");
MODULE_DESCRIPTION("The lcd controller driver for RDA Linux");
