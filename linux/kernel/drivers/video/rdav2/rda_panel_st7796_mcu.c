#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/jiffies.h>
#include <linux/gpio.h>

#include <plat/devices.h>
#include <plat/rda_debug.h>
#include <mach/board.h>

#include "rda_lcdc.h"
#include "rda_panel.h"

#define ST7796_MCU_CHIP_ID			0x7796

/* timing for real platform*/
#define ST7796_MCU_TIMING {						     \
	{.tas       =  7,                                            \
		.tah        =  7,                                            \
		.pwl        =  25,                                           \
		.pwh        =  25}}                                          \

#define ST7796_MCU_CONFIG {                                             \
	{.cs             =   LCD_CS_0,                         \
		.output_fmt      =   DBI_OUTPUT_FORMAT_16_BIT_RGB565,   \
		.cs0_polarity    =   false,                                  \
		.cs1_polarity    =   false,                                  \
		.rs_polarity     =   false,                                  \
		.wr_polarity     =   false,                                  \
		.rd_polarity     =   false,				     \
		.te_en           =   1,				     	     \
		.tecon2		 = 0x100}}


#define delayms(_ms_)			msleep(_ms_)

static struct rda_lcd_info st7796_mcu_info;
static int st7796_mcu_reset_gpio(void)
{
	printk("%s\n",__func__);
	gpio_set_value(GPIO_LCD_RESET, 1);
	mdelay(10);
	gpio_set_value(GPIO_LCD_RESET, 0);
	mdelay(120);
	gpio_set_value(GPIO_LCD_RESET, 1);
	mdelay(10);

	return 0;
}

static bool st7796_mcu_match_id(void)
{
	u8 data[6];
	u32 pwl_old, pwh_old;
	struct lcd_panel_info *lcd = (void *)&(st7796_mcu_info.lcd);

	rda_lcdc_pre_enable_lcd(lcd,1);
	st7796_mcu_reset_gpio();

	/* adjust timming */
	pwl_old = lcd->mcu_pinfo.pwl;
	pwh_old = lcd->mcu_pinfo.pwh;
	lcd->mcu_pinfo.pwl =32;
	lcd->mcu_pinfo.pwh = 32;
	rda_lcdc_configure_timing(lcd);

	/* read id */
	LCD_MCU_CMD(0xd3);
	LCD_MCU_READ(&data[0]);
	LCD_MCU_READ(&data[1]);
	LCD_MCU_READ(&data[2]);
	LCD_MCU_READ(&data[3]);
	LCD_MCU_READ(&data[4]);
	LCD_MCU_READ(&data[5]);

	printk(KERN_INFO "rda_fb: st7796_mcu id:"
			"%02x %02x %02x %02x %02x %02x\n",
			data[0], data[1], data[2], data[3],data[4],data[5]);

	/* adjust timming */
	lcd->mcu_pinfo.pwl = pwl_old;
	lcd->mcu_pinfo.pwh = pwh_old;
	rda_lcdc_configure_timing(lcd);

	rda_lcdc_pre_enable_lcd(lcd,0);
	if(((data[2]==0x77)&&(data[3]==0x96)) || ((data[3]==0x77)&&(data[4]==0x96)))
		return true;
	else
		return false;
}

static int st7796_mcu_open(void)
{
	unsigned int width =0, height=0;

	LCD_MCU_CMD (0x11);
	mdelay(120);

	LCD_MCU_CMD(0xf0);
	LCD_MCU_DATA(0xc3);

	LCD_MCU_CMD(0xf0);
	LCD_MCU_DATA(0x96);

	LCD_MCU_CMD(0x36);
	LCD_MCU_DATA(0x48);

	LCD_MCU_CMD(0xb4);
	LCD_MCU_DATA(0x01);

	LCD_MCU_CMD(0xb7);
	LCD_MCU_DATA(0x06);

	LCD_MCU_CMD(0xe8);
	LCD_MCU_DATA(0x40);
	LCD_MCU_DATA(0x8A);
	LCD_MCU_DATA(0x00);
	LCD_MCU_DATA(0x00);
	LCD_MCU_DATA(0x29);
	LCD_MCU_DATA(0x19);
	LCD_MCU_DATA(0xA5);
	LCD_MCU_DATA(0x33);

	LCD_MCU_CMD(0xc1);
	LCD_MCU_DATA(0x06);

	LCD_MCU_CMD(0xc2);
	LCD_MCU_DATA(0xA5);

	LCD_MCU_CMD(0xc5);
	LCD_MCU_DATA(0x2D);

	LCD_MCU_CMD(0xe0);
	LCD_MCU_DATA(0xf0);
	LCD_MCU_DATA(0x09);
	LCD_MCU_DATA(0x0b);
	LCD_MCU_DATA(0x06);
	LCD_MCU_DATA(0x04);
	LCD_MCU_DATA(0x15);
	LCD_MCU_DATA(0x2f);
	LCD_MCU_DATA(0x54);
	LCD_MCU_DATA(0x42);
	LCD_MCU_DATA(0x3c);
	LCD_MCU_DATA(0x17);
	LCD_MCU_DATA(0x14);
	LCD_MCU_DATA(0x18);
	LCD_MCU_DATA(0x1B);

	LCD_MCU_CMD(0xe1);
	LCD_MCU_DATA(0xf0);
	LCD_MCU_DATA(0x09);
	LCD_MCU_DATA(0x0b);
	LCD_MCU_DATA(0x06);
	LCD_MCU_DATA(0x04);
	LCD_MCU_DATA(0x03);
	LCD_MCU_DATA(0x2d);
	LCD_MCU_DATA(0x43);
	LCD_MCU_DATA(0x42);
	LCD_MCU_DATA(0x3b);
	LCD_MCU_DATA(0x16);
	LCD_MCU_DATA(0x14);
	LCD_MCU_DATA(0x17);
	LCD_MCU_DATA(0x1b);

	LCD_MCU_CMD(0xf0);
	LCD_MCU_DATA(0x3c);

	LCD_MCU_CMD(0xf0);
	LCD_MCU_DATA(0x69);

	LCD_MCU_CMD(0x35); //Memory Access
	LCD_MCU_DATA(0x00);

	LCD_MCU_CMD(0x3a);
	LCD_MCU_DATA(0x55);

	LCD_MCU_CMD(0x29);
	mdelay(120);

	LCD_MCU_CMD(0x2c);

	for(height=0;height<480;height++)
	{
		for(width=0;width<320;width++)
			{
			 LCD_MCU_DATA(0x0000);
			}
	}
	return 0;
}

static int st7796_mcu_sleep(void)
{
	//gpio_set_value(GPIO_LCD_RESET, 0);
	LCD_MCU_CMD(0x28);
	mdelay(10);
	LCD_MCU_CMD(0x10);
	mdelay(120);
	return 0;
}

static int st7796_mcu_wakeup(void)
{
	/* as we go entire power off, we need to re-init the LCD */
	st7796_mcu_reset_gpio();
	st7796_mcu_open();

	return 0;
}

static int st7796_mcu_set_active_win(struct lcd_img_rect *r)
{
#if 0
	LCD_MCU_CMD(0x2a);	/* Set Column Address */
	LCD_MCU_DATA(r->tlX >> 8);
	LCD_MCU_DATA(r->tlX & 0x00ff);
	LCD_MCU_DATA((r->brX) >> 8);
	LCD_MCU_DATA((r->brX) & 0x00ff);

	LCD_MCU_CMD(0x2b);	/* Set Page Address */
	LCD_MCU_DATA(r->tlY >> 8);
	LCD_MCU_DATA(r->tlY & 0x00ff);
	LCD_MCU_DATA((r->brY) >> 8);
	LCD_MCU_DATA((r->brY) & 0x00ff);

	LCD_MCU_CMD(0x2c);
#endif
	return 0;
}

static int st7796_mcu_set_rotation(int rotate)
{
	return 0;
}

static int st7796_mcu_close(void)
{
	return 0;
}

static void st7796_mcu_read_fb(u8 *buffer)
{
	int i = 0;
	short x0, y0, x1, y1;
	u32 pwl_old, pwh_old;
	short h_X_start,l_X_start,h_X_end,l_X_end,h_Y_start,l_Y_start,h_Y_end,l_Y_end;
	u16 readData[3];
	struct lcd_panel_info *lcd = (void *)&(st7796_mcu_info.lcd);
	printk("st7796_mcu_read_fb\n");

	rda_lcdc_pre_wait_and_enable_clk();

	x0 = 0;
	y0 = 0;
	x1 = HVGA_LCDD_DISP_X-1;
	y1 = HVGA_LCDD_DISP_Y-1;

	h_X_start=((x0&0x0300)>>8);
	l_X_start=(x0&0x00FF);
	h_X_end=((x1&0x0300)>>8);
	l_X_end=(x1&0x00FF);

	h_Y_start=((y0&0x0300)>>8);
	l_Y_start=(y0&0x00FF);
	h_Y_end=((y1&0x0300)>>8);
	l_Y_end=(y1&0x00FF);

	/* adjust timming */
	pwl_old = lcd->mcu_pinfo.pwl;
	pwh_old = lcd->mcu_pinfo.pwh;
	lcd->mcu_pinfo.pwl = 32;
	lcd->mcu_pinfo.pwh = 32;
	rda_lcdc_configure_timing(lcd);

	LCD_MCU_CMD(0x2A);
	LCD_MCU_DATA(h_X_start);
	LCD_MCU_DATA(l_X_start);
	LCD_MCU_DATA(h_X_end);
	LCD_MCU_DATA(l_X_end);
	LCD_MCU_CMD(0x2B);
	LCD_MCU_DATA(h_Y_start);
	LCD_MCU_DATA(l_Y_start);
	LCD_MCU_DATA(h_Y_end);
	LCD_MCU_DATA(l_Y_end);
	LCD_MCU_CMD(0x2E);

	//Dummy Read
	LCD_MCU_READ16(&readData[0]);
	for(i=0; i<30; i++)
	{
		LCD_MCU_READ16(&readData[0]);
		printk("rda_factory_auto_test 0x%04x\n", readData[0]);
		((u16 *)buffer)[i]   = (readData[0]&0xF800);
	}

	/* adjust timming */
	lcd->mcu_pinfo.pwl = pwl_old;
	lcd->mcu_pinfo.pwh = pwh_old;
	rda_lcdc_configure_timing(lcd);

	rda_lcdc_post_disable_clk();

}

static int st7796_rda_lcd_dbg_w(void *p,int n)
{
	int i;
	int *buf = (int *)p;

	printk(KERN_ERR "lcd_dbg_w:cmd:0x%x\n",buf[0]);
	if (buf[0] == 0xff)
	{
		mdelay(buf[1]);
		return 0;
	}

	LCD_MCU_CMD(buf[0]);
	for(i = 1;i< n;i++)
	{
		LCD_MCU_DATA(buf[i]);
		printk(KERN_ERR "lcd_dbg_w:data:0x%x\n",buf[i]);
	}
	return 0;
}

static int st7796_mcu_display_on(void)
{
	return 0;
}

static int st7796_mcu_display_off(void)
{
	return 0;
}

static struct rda_lcd_info st7796_mcu_info = {
	.name = ST7796_MCU_PANEL_NAME,
	.ops = {
		.s_reset_gpio = st7796_mcu_reset_gpio,
		.s_open = st7796_mcu_open,
		.s_match_id = st7796_mcu_match_id,
		.s_active_win = st7796_mcu_set_active_win,
		.s_rotation = st7796_mcu_set_rotation,
		.s_sleep = st7796_mcu_sleep,
		.s_wakeup = st7796_mcu_wakeup,
		.s_close = st7796_mcu_close,
		.s_read_fb = st7796_mcu_read_fb,
		.s_rda_lcd_dbg_w = st7796_rda_lcd_dbg_w,
		.s_display_on = st7796_mcu_display_on,
		.s_display_off = st7796_mcu_display_off
	},
	.lcd = {
		.width = HVGA_LCDD_DISP_X,
		.height = HVGA_LCDD_DISP_Y,
		.bpp = 16,
		.lcd_interface = LCD_IF_DBI,
		.mcu_pinfo = {
			.tas = 5,
			.tah = 5,
			.pwl = 12,
			.pwh = 12,

			.cs = LCD_CS_0,
			.output_fmt = DBI_OUTPUT_FORMAT_16_BIT_RGB565,
			.cs0_polarity = false,
			.cs1_polarity = false,
			.rs_polarity = false,
			.wr_polarity = false,
			.rd_polarity = false,
			.te_en  = 1,
			.tecon2 = 0x100
		}
	}
//		.width = HVGA_LCDD_DISP_X,
//		.height = HVGA_LCDD_DISP_Y,
//		.lcd_interface = LCD_IF_DBI,
//		.lcd_timing = ST7796_MCU_TIMING,
//		.lcd_cfg = ST7796_MCU_CONFIG},
//	    .name = ST7796_MCU_PANEL_NAME,
};

/*--------------------Platform Device Probe-------------------------*/

static int rda_fb_panel_st7796_mcu_probe(struct platform_device *pdev)
{
	rda_fb_register_panel(&st7796_mcu_info);

	dev_info(&pdev->dev, "rda panel st7796_mcu registered\n");

	return 0;
}

static int rda_fb_panel_st7796_mcu_remove(struct platform_device *pdev)
{
	return 0;
}

static struct platform_driver rda_fb_panel_st7796_mcu_driver = {
	.probe = rda_fb_panel_st7796_mcu_probe,
	.remove = rda_fb_panel_st7796_mcu_remove,
	.driver = {
		.name = ST7796_MCU_PANEL_NAME}
};
static struct rda_panel_driver st7796_mcu_panel_driver = {
	.panel_type = LCD_IF_DBI,
	.lcd_driver_info = &st7796_mcu_info,
	.pltaform_panel_driver = &rda_fb_panel_st7796_mcu_driver,
};
