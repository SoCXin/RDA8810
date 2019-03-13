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

#define ILI9488L_MCU_CHIP_ID			0x3AB6

/* wngl, for FPGA */
#define ILI9488L_MCU_TIMING {						     \
	{.tas       =  5,                                            \
	.tah        =  5,                                            \
	.pwl        =  7,                                           \
	.pwh        =  7}}                                          \

#define ILI9488L_MCU_CONFIG {                                             \
	{.cs             =   GOUDA_LCD_CS_0,                         \
	.output_fmt      =   GOUDA_LCD_OUTPUT_FORMAT_16_bit_RGB565,   \
	.cs0_polarity    =   false,                                  \
	.cs1_polarity    =   false,                                  \
	.rs_polarity     =   false,                                  \
	.wr_polarity     =   false,                                  \
	.rd_polarity     =   false}}

static int ILI9488L_mcu_init_gpio(void)
{
	gpio_request(GPIO_LCD_RESET, "lcd reset");
	gpio_direction_output(GPIO_LCD_RESET, 1);
	gpio_set_value(GPIO_LCD_RESET, 1);
	mdelay(10);
	gpio_set_value(GPIO_LCD_RESET, 0);
	mdelay(20);
	gpio_set_value(GPIO_LCD_RESET, 1);
	mdelay(100);

	return 0;
}

static void ILI9488L_mcu_readid(void)
{
	u8 data[6];

	/* read id */
	LCD_MCU_CMD(0x400);
	LCD_MCU_READ(&data[0]);
	LCD_MCU_READ(&data[1]);
	LCD_MCU_CMD(0x401);
	LCD_MCU_READ(&data[2]);
	LCD_MCU_READ(&data[3]);
	LCD_MCU_CMD(0x402);
	LCD_MCU_READ(&data[4]);
	LCD_MCU_READ(&data[5]);

	printk(KERN_INFO "rda_fb: ILI9488L_mcu ID:"
	       "%02x %02x %02x %02x %02x %02x\n",
	       data[0], data[1], data[2], data[3], data[4], data[5]);
}

static int ILI9488L_mcu_open(void)
{
	mdelay(20);
	ILI9488L_mcu_readid();
//************* Start Initial Sequence **********//

LCD_MCU_CMD(0xE0);
LCD_MCU_DATA(0x00);
LCD_MCU_DATA(0x0A);
LCD_MCU_DATA(0x1C);
LCD_MCU_DATA(0x0D);
LCD_MCU_DATA(0x1B);
LCD_MCU_DATA(0x0C);
LCD_MCU_DATA(0x48);
LCD_MCU_DATA(0x69);
LCD_MCU_DATA(0x56);
LCD_MCU_DATA(0x05);
LCD_MCU_DATA(0x0C);
LCD_MCU_DATA(0x08);
LCD_MCU_DATA(0x23);
LCD_MCU_DATA(0x26);
LCD_MCU_DATA(0x0F);
LCD_MCU_CMD(0xE1);
LCD_MCU_DATA(0x00);
LCD_MCU_DATA(0x1E);
LCD_MCU_DATA(0x22);
LCD_MCU_DATA(0x03);
LCD_MCU_DATA(0x0F);
LCD_MCU_DATA(0x03);
LCD_MCU_DATA(0x34);
LCD_MCU_DATA(0x33);
LCD_MCU_DATA(0x42);
LCD_MCU_DATA(0x03);
LCD_MCU_DATA(0x08);
LCD_MCU_DATA(0x07);
LCD_MCU_DATA(0x2E);
LCD_MCU_DATA(0x36);
LCD_MCU_DATA(0x0F);

LCD_MCU_CMD(0xC0);
LCD_MCU_DATA(0x12);
LCD_MCU_DATA(0x0C);

LCD_MCU_CMD(0xC1);
LCD_MCU_DATA(0x41);

LCD_MCU_CMD(0xC5);
LCD_MCU_DATA(0x00);
LCD_MCU_DATA(0x4D);
LCD_MCU_DATA(0x80);

LCD_MCU_CMD(0x36);
//LCD_MCU_DATA(0x48);
LCD_MCU_DATA(0x88); // to invert Y

LCD_MCU_CMD(0x3A);
LCD_MCU_DATA(0x55);

LCD_MCU_CMD(0xB0);
LCD_MCU_DATA(0x00);

LCD_MCU_CMD(0xB1);
LCD_MCU_DATA(0xA0);

LCD_MCU_CMD(0xB4);
LCD_MCU_DATA(0x02);

LCD_MCU_CMD(0xB6);
LCD_MCU_DATA(0x02);
LCD_MCU_DATA(0x02);

LCD_MCU_CMD(0xE9);
LCD_MCU_DATA(0x00);

LCD_MCU_CMD(0xF7);
LCD_MCU_DATA(0xA9);
LCD_MCU_DATA(0x51);
LCD_MCU_DATA(0x2C);
LCD_MCU_DATA(0x82);

LCD_MCU_CMD(0x11); //Sleep out
mdelay (120);
LCD_MCU_CMD(0x29); //Display on

return 0;
}

static int ILI9488L_mcu_sleep(void)
{
	//LCD_MCU_CMD(0x10);
	gpio_set_value(GPIO_LCD_RESET, 0);
	
	return 0;
}

static int ILI9488L_mcu_wakeup(void)
{
	/* as we go entire power off, we need to re-init the LCD */
	gpio_set_value(GPIO_LCD_RESET, 1);
	mdelay(10);
	gpio_set_value(GPIO_LCD_RESET, 0);
	mdelay(1);
	gpio_set_value(GPIO_LCD_RESET, 1);
	mdelay(1);

	ILI9488L_mcu_open();

	return 0;
}

static int ILI9488L_mcu_set_active_win(struct gouda_rect *r)
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

static int ILI9488L_mcu_set_rotation(int rotate)
{
	return 0;
}

static int ILI9488L_mcu_close(void)
{
	return 0;
}

static struct rda_lcd_info ILI9488L_mcu_info = {
	.ops = {
		.s_init_gpio = ILI9488L_mcu_init_gpio,
		.s_open = ILI9488L_mcu_open,
		.s_active_win = ILI9488L_mcu_set_active_win,
		.s_rotation = ILI9488L_mcu_set_rotation,
		.s_sleep = ILI9488L_mcu_sleep,
		.s_wakeup = ILI9488L_mcu_wakeup,
		.s_close = ILI9488L_mcu_close},
	.lcd = {
		.width = HVGA_LCDD_DISP_X,
		.height = HVGA_LCDD_DISP_Y,
		.lcd_interface = GOUDA_LCD_IF_DBI,
		.lcd_timing = ILI9488L_MCU_TIMING,
		.lcd_cfg = ILI9488L_MCU_CONFIG},
	.name = ILI9488L_MCU_PANEL_NAME,
};

/*--------------------Platform Device Probe-------------------------*/

static int rda_fb_panel_ILI9488L_mcu_probe(struct platform_device *pdev)
{
	rda_fb_register_panel(&ILI9488L_mcu_info);

	dev_info(&pdev->dev, "rda panel ILI9488L_mcu registered\n");

	return 0;
}

static int rda_fb_panel_ILI9488L_mcu_remove(struct platform_device *pdev)
{
	return 0;
}

static struct platform_driver rda_fb_panel_ILI9488L_mcu_driver = {
	.probe = rda_fb_panel_ILI9488L_mcu_probe,
	.remove = rda_fb_panel_ILI9488L_mcu_remove,
	.driver = {
		   .name = ILI9488L_MCU_PANEL_NAME}
};

static struct rda_panel_driver ili9488l_mcu_panel_driver = {
	.panel_type = GOUDA_LCD_IF_DBI,
	.lcd_driver_info = &ILI9488L_mcu_info,
	.pltaform_panel_driver = &rda_fb_panel_ILI9488L_mcu_driver,
};
