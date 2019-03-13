#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/jiffies.h>
#include <linux/gpio.h>

#include <plat/devices.h>
#include <plat/rda_debug.h>
#include <mach/board.h>

#include "rda_gouda.h"
#include "rda_panel.h"

#define HX8357_MCU_CHIP_ID			0x8357

/* wngl, for FPGA */
#define HX8357_MCU_TIMING {						     \
	{.tas       =  6,                                            \
	.tah        =  6,                                            \
	.pwl        =  16,                                           \
	.pwh        =  16}}                                          \

#define HX8357_MCU_CONFIG {                                             \
	{.cs             =   GOUDA_LCD_CS_0,                         \
	.output_fmt      =   GOUDA_LCD_OUTPUT_FORMAT_16_bit_RGB565,   \
	.cs0_polarity    =   false,                                  \
	.cs1_polarity    =   false,                                  \
	.rs_polarity     =   false,                                  \
	.wr_polarity     =   false,                                  \
	.rd_polarity     =   false,									\
	.te_en           =   1,				     	     			\
	.tecon2		 = 0x100}}

static int hx8357_mcu_init_gpio(void)
{


	gpio_request(GPIO_LCD_RESET, "lcd reset");
	gpio_direction_output(GPIO_LCD_RESET, 1);
	mdelay(1);
	gpio_set_value(GPIO_LCD_RESET, 0);
	mdelay(100);
	gpio_set_value(GPIO_LCD_RESET, 1);
	mdelay(50);

	return 0;
}

static int hx8357_mcu_open(void)
{
		msleep(50);
		LCD_MCU_CMD(0x11);			   // SLPOUT
		msleep(120);
		LCD_MCU_CMD(0xB9);			   //EXTC
		LCD_MCU_DATA(0xFF); 			//EXTC
		LCD_MCU_DATA(0x83); 			//EXTC
		LCD_MCU_DATA(0x57); 			//EXTC

		LCD_MCU_CMD(0xCC);			   //
		LCD_MCU_DATA(0x00); 			//Set Panel

		LCD_MCU_CMD(0xB6);			   //
		LCD_MCU_DATA(0x0e); 			//VCOMDC

		LCD_MCU_CMD(0x36);
		LCD_MCU_DATA(0x48);

		LCD_MCU_CMD(0x3A);
		LCD_MCU_DATA(0x55);  //16bit/pixel

		LCD_MCU_CMD(0xB0);				  //
		LCD_MCU_DATA(0x08); 			//08

		LCD_MCU_CMD(0xB1);			   //
		LCD_MCU_DATA(0x00); 			//
		LCD_MCU_DATA(0x24); 			//BT  //15
		LCD_MCU_DATA(0x25); 			//VSNR
		LCD_MCU_DATA(0x25); 			//AP
		LCD_MCU_DATA(0x83); 			//FS
		LCD_MCU_DATA(0xAA);

		LCD_MCU_CMD(0xB4);			   //
		LCD_MCU_DATA(0x02); 			//NW
		LCD_MCU_DATA(0x40); 			//RTN
		LCD_MCU_DATA(0x00); 			//DIV
		LCD_MCU_DATA(0x2A); 			//DUM
		LCD_MCU_DATA(0x2A); 			//DUM
		LCD_MCU_DATA(0x0D); 			//GDON
		LCD_MCU_DATA(0x78); 			//GDOFF

		LCD_MCU_CMD(0xB5);
		LCD_MCU_DATA(0x00);//03
		LCD_MCU_DATA(0x00);
		LCD_MCU_DATA(0x66);

		LCD_MCU_CMD(0xC0);			   //STBA
		LCD_MCU_DATA(0x70); 			//OPON
		LCD_MCU_DATA(0x70); 			//OPON
		LCD_MCU_DATA(0x01); 			//
		LCD_MCU_DATA(0x3C); 			//
		LCD_MCU_DATA(0x1c); 			//
		LCD_MCU_DATA(0x08); 			//GEN

		LCD_MCU_CMD(0x2A);		   //  column address set
		LCD_MCU_DATA(0x00); 			//
		LCD_MCU_DATA(0x00); 			//
		LCD_MCU_DATA(0x01); 			//
		LCD_MCU_DATA(0x3F); 			//
		LCD_MCU_CMD(0x2B);			//	page adress set
		LCD_MCU_DATA(0x00); 			//
		LCD_MCU_DATA(0x00); 			//
		LCD_MCU_DATA(0x01); 			//
		LCD_MCU_DATA(0xdF); 			//

		LCD_MCU_CMD(0xE0);			   //
		LCD_MCU_DATA(0x00);
		LCD_MCU_DATA(0x04);
		LCD_MCU_DATA(0x0C);
		LCD_MCU_DATA(0x1A);
		LCD_MCU_DATA(0x28);
		LCD_MCU_DATA(0x44);
		LCD_MCU_DATA(0x4F);
		LCD_MCU_DATA(0x5E);
		LCD_MCU_DATA(0x40);
		LCD_MCU_DATA(0x39);
		LCD_MCU_DATA(0x34);
		LCD_MCU_DATA(0x2C);
		LCD_MCU_DATA(0x24);
		LCD_MCU_DATA(0x20);
		LCD_MCU_DATA(0x1E);
		LCD_MCU_DATA(0x04);
		LCD_MCU_DATA(0x00);
		LCD_MCU_DATA(0x04);
		LCD_MCU_DATA(0x0C);
		LCD_MCU_DATA(0x1A);
		LCD_MCU_DATA(0x28);
		LCD_MCU_DATA(0x44);
		LCD_MCU_DATA(0x4F);
		LCD_MCU_DATA(0x5E);
		LCD_MCU_DATA(0x40);
		LCD_MCU_DATA(0x39);
		LCD_MCU_DATA(0x34);
		LCD_MCU_DATA(0x2C);
		LCD_MCU_DATA(0x24);
		LCD_MCU_DATA(0x20);
		LCD_MCU_DATA(0x1E);
		LCD_MCU_DATA(0x1D);


		LCD_MCU_CMD(0x44);			   //TE
		LCD_MCU_DATA(0x00);
		LCD_MCU_DATA(0x80);

		LCD_MCU_CMD(0x35);				//	TE ON
		LCD_MCU_DATA(0x00);

		LCD_MCU_CMD(0x29);	   // Display On
		msleep(5);
	return 0;
}

static int hx8357_mcu_sleep(void)
{
	gpio_set_value(GPIO_LCD_RESET, 0);
	mdelay(10);
	return 0;
}

static int hx8357_mcu_wakeup(void)
{
	gpio_direction_output(GPIO_LCD_RESET, 1);
	mdelay(1);
	gpio_set_value(GPIO_LCD_RESET, 0);
	mdelay(10);
	gpio_set_value(GPIO_LCD_RESET, 1);
	mdelay(5);
	hx8357_mcu_open();
	return 0;
}

static int hx8357_mcu_set_active_win(struct gouda_rect *r)
{
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

	return 0;
}

static int hx8357_mcu_display_on(void)
{
	return 0;
}

static int hx8357_mcu_display_off(void)
{
	return 0;
}

static int hx8357_mcu_set_rotation(int rotate)
{
	return 0;
}

static int hx8357_mcu_close(void)
{
	return 0;
}

static struct rda_lcd_info hx8357_mcu_info = {
	.ops = {
		.s_init_gpio = hx8357_mcu_init_gpio,
		.s_open = hx8357_mcu_open,
		.s_active_win = hx8357_mcu_set_active_win,
		.s_rotation = hx8357_mcu_set_rotation,
		.s_sleep = hx8357_mcu_sleep,
		.s_wakeup = hx8357_mcu_wakeup,
		.s_close = hx8357_mcu_close,
		.s_display_on = hx8357_mcu_display_on,
		.s_display_off = hx8357_mcu_display_off
	},
	.lcd = {
		.width = HVGA_LCDD_DISP_X,
		.height = HVGA_LCDD_DISP_Y,
		.lcd_interface = GOUDA_LCD_IF_DBI,
		.lcd_timing = HX8357_MCU_TIMING,
		.lcd_cfg = HX8357_MCU_CONFIG},
	.name = HX8357_MCU_PANEL_NAME,
};

/*--------------------Platform Device Probe-------------------------*/

static int rda_fb_panel_hx8357_mcu_probe(struct platform_device *pdev)
{
	rda_fb_register_panel(&hx8357_mcu_info);

	dev_info(&pdev->dev, "rda panel hx8357_mcu registered\n");

	return 0;
}

static int rda_fb_panel_hx8357_mcu_remove(struct platform_device *pdev)
{
	return 0;
}

static struct platform_driver rda_fb_panel_hx8357_mcu_driver = {
	.probe = rda_fb_panel_hx8357_mcu_probe,
	.remove = rda_fb_panel_hx8357_mcu_remove,
	.driver = {
		   .name = HX8357_MCU_PANEL_NAME}
};

static struct rda_panel_driver hx8357_mcu_panel_driver = {
	.panel_type = GOUDA_LCD_IF_DBI,
	.lcd_driver_info = &hx8357_mcu_info,
	.pltaform_panel_driver = &rda_fb_panel_hx8357_mcu_driver,
};
