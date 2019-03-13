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

#define ILI9327_CHIP_ID			0x9327

/* wngl, for FPGA */
#define ILI9327_TIMING {						     \
	{.tas       =  6,                                            \
	.tah        =  6,                                            \
	.pwl        =  16,                                           \
	.pwh        =  16}}                                          \

#define ILI9327_CONFIG {                                             \
	{.cs             =   GOUDA_LCD_CS_0,                         \
	.output_fmt      =   GOUDA_LCD_OUTPUT_FORMAT_8_bit_RGB565,   \
	.cs0_polarity    =   false,                                  \
	.cs1_polarity    =   false,                                  \
	.rs_polarity     =   false,                                  \
	.wr_polarity     =   false,                                  \
	.rd_polarity     =   false}}

static int ili9327_init_gpio(void)
{
	gpio_request(GPIO_LCD_RESET, "lcd reset");
	gpio_direction_output(GPIO_LCD_RESET, 1);
	msleep(1);
	gpio_set_value(GPIO_LCD_RESET, 0);
	msleep(100);
	gpio_set_value(GPIO_LCD_RESET, 1);
	msleep(50);

	return 0;
}

static void ili9327_readid(void)
{
	u8 data[6];

	/* read id */
	LCD_MCU_CMD(0xEF);
	LCD_MCU_READ(&data[0]);
	LCD_MCU_READ(&data[1]);
	LCD_MCU_READ(&data[2]);
	LCD_MCU_READ(&data[3]);
	LCD_MCU_READ(&data[4]);
	LCD_MCU_READ(&data[5]);

	printk(KERN_INFO "rda_fb: ili9327 ID:"
	       "%02x %02x %02x %02x %02x %02x\n",
	       data[0], data[1], data[2], data[3], data[4], data[5]);
}

static int ili9327_open(void)
{
	ili9327_readid();

	//************* Start Initial Sequence **********//
	LCD_MCU_CMD(0xE9);
	LCD_MCU_DATA(0x20);

	LCD_MCU_CMD(0x11);	//Exit Sleep
	msleep(100);

	LCD_MCU_CMD(0x3A);
	LCD_MCU_DATA(0x55);

	LCD_MCU_CMD(0xD1);
	LCD_MCU_DATA(0x00);
	LCD_MCU_DATA(0x48);
	LCD_MCU_DATA(0x10);

	LCD_MCU_CMD(0xD0);
	LCD_MCU_DATA(0x07);
	LCD_MCU_DATA(0x02);
	LCD_MCU_DATA(0x09);

	LCD_MCU_CMD(0x36);
	//LCD_MCU_DATA (0x08);
	LCD_MCU_DATA(0x48);	// fix inversion issue

	LCD_MCU_CMD(0xC1);
	LCD_MCU_DATA(0x10);
	LCD_MCU_DATA(0x10);
	LCD_MCU_DATA(0x02);
	LCD_MCU_DATA(0x02);

	LCD_MCU_CMD(0xC0);	//Set Default Gamma
	LCD_MCU_DATA(0x00);
	LCD_MCU_DATA(0x35);
	LCD_MCU_DATA(0x00);
	LCD_MCU_DATA(0x00);
	LCD_MCU_DATA(0x01);
	LCD_MCU_DATA(0x02);

	LCD_MCU_CMD(0xC5);	//Set frame rate
	LCD_MCU_DATA(0x03);

	LCD_MCU_CMD(0xD2);	//power setting
	LCD_MCU_DATA(0x01);
	LCD_MCU_DATA(0x22);

	LCD_MCU_CMD(0xC8);	//Set Gamma
	LCD_MCU_DATA(0x00);
	LCD_MCU_DATA(0x41);
	LCD_MCU_DATA(0x25);
	LCD_MCU_DATA(0x33);
	LCD_MCU_DATA(0x0F);
	LCD_MCU_DATA(0x06);
	LCD_MCU_DATA(0x25);
	LCD_MCU_DATA(0x63);
	LCD_MCU_DATA(0x77);
	LCD_MCU_DATA(0x33);
	LCD_MCU_DATA(0x06);
	LCD_MCU_DATA(0x0F);
	LCD_MCU_DATA(0x08);
	LCD_MCU_DATA(0x80);
	LCD_MCU_DATA(0x00);

	LCD_MCU_CMD(0xB3);
	LCD_MCU_DATA(0x02);
	LCD_MCU_DATA(0x00);
	LCD_MCU_DATA(0x00);
	LCD_MCU_DATA(0x30);
	LCD_MCU_CMD(0xEA);
	LCD_MCU_DATA(0x00);
	LCD_MCU_DATA(0xC0);
	LCD_MCU_CMD(0xE5);
	LCD_MCU_DATA(0x91);
	LCD_MCU_DATA(0xD1);

	LCD_MCU_CMD(0x29);	//display on
	return 0;
}

static int ili9327_sleep(void)
{
	LCD_MCU_CMD(0x10);
	return 0;
}

static int ili9327_wakeup(void)
{
	LCD_MCU_CMD(0x11);
	return 0;
}

static int ili9327_set_active_win(struct gouda_rect *r)
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

static int ili9327_set_rotation(int rotate)
{
	return 0;
}

static int ili9327_close(void)
{
	return 0;
}

static struct rda_lcd_info ili9327_info = {
	.ops = {
		.s_init_gpio = ili9327_init_gpio,
		.s_open = ili9327_open,
		.s_active_win = ili9327_set_active_win,
		.s_rotation = ili9327_set_rotation,
		.s_sleep = ili9327_sleep,
		.s_wakeup = ili9327_wakeup,
		.s_close = ili9327_close},
	.lcd = {
		.width = FWQVGA_LCDD_DISP_X,
		.height = FWQVGA_LCDD_DISP_Y,
		.lcd_interface = GOUDA_LCD_IF_DBI,
		.lcd_timing = ILI9327_TIMING,
		.lcd_cfg = ILI9327_CONFIG},
	.name = ILI9327_PANEL_NAME,
};

/*--------------------Platform Device Probe-------------------------*/

static int rda_fb_panel_ili9327_probe(struct platform_device *pdev)
{
	rda_fb_register_panel(&ili9327_info);

	dev_info(&pdev->dev, "rda panel ili9327 registered\n");

	return 0;
}

static int rda_fb_panel_ili9327_remove(struct platform_device *pdev)
{
	return 0;
}

static struct platform_driver rda_fb_panel_ili9327_driver = {
	.probe = rda_fb_panel_ili9327_probe,
	.remove = rda_fb_panel_ili9327_remove,
	.driver = {
		   .name = ILI9327_PANEL_NAME}
};

static struct rda_panel_driver ili9327_mcu_panel_driver = {
	.panel_type = GOUDA_LCD_IF_DBI,
	.lcd_driver_info = &ili9327_info,
	.pltaform_panel_driver = &rda_fb_panel_ili9327_driver,
};
