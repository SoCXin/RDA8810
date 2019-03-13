#include <common.h>
#include <asm/arch/cs_types.h>
#include <asm/arch/rda_iomap.h>
#include "rda_lcdc.h"
#include "rda_mipi_dsi.h"
#include <asm/arch/reg_spi.h>
#include <asm/arch/ispi.h>
#include <asm/io.h>

#define ALT_MUX_SELECT 0x11a09018

u32 rda_read_dsi_reg(u32 offset)
{
	u32 value;

	value = LCDC_INL(RDA_LCDC_BASE + DSI_REG_OFFSET + offset);
	return value;
}

void rda_write_dsi_reg(u32 offset, u32 value)
{
	LCDC_OUTL(RDA_LCDC_BASE + DSI_REG_OFFSET + offset,value);
}

void reset_lcdc_fifo(void)
{
	LCDC_AND_WITH_REG(RDA_LCDC_BASE + DPI_FIFO_CTRL, ~DPI_DATA_FIFO_RST_AUTO);
	LCDC_OR_WITH_REG(RDA_LCDC_BASE + DPI_FIFO_CTRL, DPI_DATA_FIFO_RST);
	LCDC_AND_WITH_REG(RDA_LCDC_BASE + DPI_FIFO_CTRL, ~DPI_DATA_FIFO_RST);
}

static void rda_dsi_irq_clear(void)
{
    u32 reg;
    reg = rda_read_dsi_reg(0x48);
    reg |= 0x1 << 2;
    rda_write_dsi_reg(0x48,0x1 << 2);
    rda_write_dsi_reg(0x48,reg & ~(1 << 2));
}

void set_lcdc_for_cmd(u32 addr, int group_num)
{
	/* fifo reset */
	reset_lcdc_fifo();
	dsi_pll_on(TRUE);

	LCDC_OUTL(RDA_LCDC_BASE + LCDC_EOF_IRQ_MASK, LCDC_MIPI_INT_MASK);
	LCDC_OUTL(RDA_LCDC_BASE + LCDC_EOF_IRQ, LCDC_IRQ_CLEAR_ALL);
	LCDC_OUTL(RDA_LCDC_BASE + DPI_FRAME0_ADDR, addr);
	LCDC_OUTL(RDA_LCDC_BASE + DPI_FRAME0_CON, DPI_FRAME_LINE_STEP(group_num * 8) | DPI_FRAME_VALID);
	LCDC_OUTL(RDA_LCDC_BASE + DPI_SIZE, VERTICAL_PIX_NUM(1) | HORIZONTAL_PIX_NUM(group_num * 2));
	LCDC_OUTL(RDA_LCDC_BASE + LCDC_DCT_SHIFT_UV_REG1, LCDC_RGB_WAIT | 0x143);
	LCDC_OUTL(RDA_LCDC_BASE + DPI_CONFIG, MIPI_CMD_SEL_CMD | MIPI_DSI_ENABLE | RGB_PIX_FMT_XRGB888);
}

void set_lcdc_for_video(u32 addr, const struct lcd_panel_info *lcd)
{
	u32 dsi_bpp;

	reset_lcdc_fifo();
	dsi_enable(FALSE);
	rda_write_dsi_reg(0x144, 0x14);
	dsi_pll_on(TRUE);
	LCDC_OUTL(RDA_LCDC_BASE + DPI_FRAME0_ADDR, addr);
	LCDC_OUTL(RDA_LCDC_BASE + DPI_FRAME0_CON, DPI_FRAME_LINE_STEP(lcd->width \
		* (lcd->bpp >> 3)) | DPI_FRAME_VALID);
	LCDC_OUTL(RDA_LCDC_BASE + DPI_SIZE, VERTICAL_PIX_NUM(lcd->height)
		| HORIZONTAL_PIX_NUM(lcd->width));
	LCDC_OUTL(RDA_LCDC_BASE + LCDC_DCT_SHIFT_UV_REG1, LCDC_RGB_WAIT | 0x143);
	LCDC_OUTL(RDA_LCDC_BASE + DPI_CONFIG, MIPI_DSI_ENABLE | lcd->mipi_pinfo.pixel_format \
		| RGB_ORDER_BGR | REG_PEND_REQ(16));
	if (lcd->mipi_pinfo.pixel_format == RGB_PIX_FMT_RGB565)
		LCDC_OUTL(RDA_LCDC_BASE + LCDC_DCT_SHIFT_UV_REG1, 0x143);

	LCDC_OUTL(RDA_LCDC_BASE + DPI_FIFO_CTRL, DPI_DATA_FIFO_RST_AUTO);
	rda_write_dsi_reg(0x8,lcd->mipi_pinfo.data_lane >> 1);

	/*rgb num, width bytes	+ header + crc*/
	dsi_bpp = lcd->bpp != 32 ? lcd->bpp : 24;
	rda_write_dsi_reg(0x2c,(lcd->width * (dsi_bpp >> 3)+ 6) >> (lcd->mipi_pinfo.data_lane >> 1));
	/*mipi tx mode*/
	dsi_op_mode(DSI_BURST);
	dsi_enable(TRUE);

}

static void set_dsi_ap_pll(const struct rda_dsi_phy_ctrl *pll_phy)
{
	ispi_open(0);
	dsi_pll_on(TRUE);

	ispi_reg_write(0xA5, pll_phy->pll[0]);
	ispi_reg_write(0xA6, pll_phy->pll[1]);
	ispi_reg_write(0xA7, pll_phy->pll[2]);
	ispi_reg_write(0xA3, pll_phy->pll[3]);
	ispi_reg_write(0xA2, pll_phy->pll[4]);
	udelay(100);
	ispi_reg_write(0xA2, pll_phy->pll[5]);
}

void rda_lcdc_set(const struct rda_dsi_phy_ctrl *dsi_phy_db)
{
	unsigned long temp;

	LCDC_OUTL(RDA_SYSCTRL_BASE + 0x34, 0x10);
	LCDC_OUTL(RDA_SYSCTRL_BASE + 0x38, 0x10);
	temp = readl(ALT_MUX_SELECT);
	temp |= LCD_DSI_MODE;
	writel(temp, ALT_MUX_SELECT);
	LCDC_OUTL(RDA_LCDC_BASE + DPI_FIFO_CTRL, DPI_DATA_FIFO_RST_AUTO);
	set_dsi_ap_pll(dsi_phy_db);
	LCDC_OUTL(RDA_LCDC_BASE + LCDC_EOF_IRQ_MASK, LCDC_MIPI_INT_MASK);
	LCDC_OUTL(RDA_LCDC_BASE + LCDC_EOF_IRQ, LCDC_IRQ_CLEAR_ALL);
}

int rda_lcdc_irq_status(void)
{
	u8 time_out = 0;
	u32 status;
	do {
		status = LCDC_INL(RDA_LCDC_BASE + LCDC_EOF_IRQ);
		printf("irq status: %x\n",status);

		if (status & LCDC_DPI_FRAMEOVER) {
			printf("irq: LCDC_DPI_FRAMEOVER\n");
			LCDC_OUTL(RDA_LCDC_BASE + LCDC_EOF_IRQ, LCDC_IRQ_CLEAR_ALL);
		}
		if (status & LCDC_MIPI_INT) {
			status = rda_read_dsi_reg(0x90);
			printf("irq: LCDC_MIPI_INT\r\n");
			if(status & DSI_TX_END_FLAG || status & DSI_RX_END_FLAG){
				rda_dsi_irq_clear();
				LCDC_OUTL(RDA_LCDC_BASE + LCDC_EOF_IRQ, LCDC_IRQ_CLEAR_ALL);
				return 0;
			}
		}
		udelay(100);
	} while (time_out++ < 10);
	return -1;
}

