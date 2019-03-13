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

#define HX8363_MCU_CHIP_ID			0x8368

/* wngl, for FPGA */
#define HX8363_MCU_TIMING {						     \
	{.tas       =  6,                                            \
	.tah        =  6,                                            \
	.pwl        =  16,                                           \
	.pwh        =  16}}                                          \

#define HX8363_MCU_CONFIG {                                             \
	{.cs             =   GOUDA_LCD_CS_0,                         \
	.output_fmt      =   GOUDA_LCD_OUTPUT_FORMAT_16_bit_RGB565,   \
	.cs0_polarity    =   false,                                  \
	.cs1_polarity    =   false,                                  \
	.rs_polarity     =   false,                                  \
	.wr_polarity     =   false,                                  \
	.rd_polarity     =   false}}

static int hx8363_mcu_init_gpio(void)
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

static void hx8363_mcu_readid(void)
{
	u8 data[6];

	/* read id */
	LCD_MCU_CMD(0x09);
	LCD_MCU_READ(&data[0]);
	LCD_MCU_READ(&data[1]);
	LCD_MCU_READ(&data[2]);
	LCD_MCU_READ(&data[3]);
	LCD_MCU_READ(&data[4]);
	LCD_MCU_READ(&data[5]);

	printk(KERN_INFO "rda_fb: hx8363_mcu ID:"
	       "%02x %02x %02x %02x %02x %02x\n",
	       data[0], data[1], data[2], data[3], data[4], data[5]);
}

static int hx8363_mcu_open(void)
{
	hx8363_mcu_readid();

	printk("Alex.Zhou hx8363_init\n");
	LCD_MCU_CMD(0xB9); //SETEXTC: enable extention command (B9h)
	LCD_MCU_DATA(0xFF);
	LCD_MCU_DATA(0x83);
	LCD_MCU_DATA(0x63);

	LCD_MCU_CMD(0xB1);//SETPOWER: Set power (B1h)
	LCD_MCU_DATA(0x01);
	LCD_MCU_DATA(0x00);
	LCD_MCU_DATA(0x44);
	LCD_MCU_DATA(0x07);
	LCD_MCU_DATA(0x01);
	LCD_MCU_DATA(0x11);
	LCD_MCU_DATA(0x11);
	LCD_MCU_DATA(0x3A);
	LCD_MCU_DATA(0x42);
	LCD_MCU_DATA(0x3F);
	LCD_MCU_DATA(0x3F);
	LCD_MCU_DATA(0x40);
	LCD_MCU_DATA(0x32);
	LCD_MCU_DATA(0x00);
	LCD_MCU_DATA(0x00);
	LCD_MCU_DATA(0x00);
	LCD_MCU_DATA(0x00);
	LCD_MCU_DATA(0x00);
	LCD_MCU_DATA(0x00);

	LCD_MCU_CMD(0xB4);//SETCYC: Set display waveform cycle (B4h)
	//WriteComm(0xB4);  // SET CYC Line=63value*15/5.5/2
	LCD_MCU_DATA(0x00);
	LCD_MCU_DATA(0x18);
	LCD_MCU_DATA(0x9C);
	LCD_MCU_DATA(0x08);
	LCD_MCU_DATA(0x18);
	LCD_MCU_DATA(0x04);
	LCD_MCU_DATA(0x72);

	LCD_MCU_CMD(0xB2);//SETDISP: Set display related register (B2h)
	LCD_MCU_DATA(0x05);//
	LCD_MCU_DATA(0x00);//

	LCD_MCU_CMD(0xBF);  // SET PTBA for VCOM=-2.5V
	LCD_MCU_DATA(0x05);
	LCD_MCU_DATA(0x60);
	LCD_MCU_DATA(0x00);
	LCD_MCU_DATA(0x10);

	LCD_MCU_CMD(0xB6); //SETVCOM: Set VCOM voltage (B6h)
	LCD_MCU_DATA(0x2B);  //Optimal VCOM Tuning

	LCD_MCU_CMD(0xE0);  // SET Gamma
	LCD_MCU_DATA(0x00);
	LCD_MCU_DATA(0x06);
	LCD_MCU_DATA(0x0A);
	LCD_MCU_DATA(0x12);
	LCD_MCU_DATA(0x16);
	LCD_MCU_DATA(0x3F);
	LCD_MCU_DATA(0x27);
	LCD_MCU_DATA(0x37);
	LCD_MCU_DATA(0x87);
	LCD_MCU_DATA(0x8E);
	LCD_MCU_DATA(0xD3);
	LCD_MCU_DATA(0xD6);
	LCD_MCU_DATA(0xD8);
	LCD_MCU_DATA(0x16);
	LCD_MCU_DATA(0x16);
	LCD_MCU_DATA(0x13);
	LCD_MCU_DATA(0x18);
	LCD_MCU_DATA(0x00);
	LCD_MCU_DATA(0x06);
	LCD_MCU_DATA(0x0A);
	LCD_MCU_DATA(0x12);
	LCD_MCU_DATA(0x16);
	LCD_MCU_DATA(0x3F);
	LCD_MCU_DATA(0x27);
	LCD_MCU_DATA(0x37);
	LCD_MCU_DATA(0x87);
	LCD_MCU_DATA(0x8E);
	LCD_MCU_DATA(0xD3);
	LCD_MCU_DATA(0xD6);
	LCD_MCU_DATA(0xD8);
	LCD_MCU_DATA(0x16);
	LCD_MCU_DATA(0x16);
	LCD_MCU_DATA(0x13);
	LCD_MCU_DATA(0x18);
	LCD_MCU_CMD(0x3A);//
	LCD_MCU_DATA(0x55); //

	LCD_MCU_CMD(0x36);//
	LCD_MCU_DATA(0x00);//88//00

	LCD_MCU_CMD(0xCC);//SETPANEL (CCh)
	LCD_MCU_DATA(0x0B);//0E//0B

	LCD_MCU_CMD(0xC2); //SETDSPIF: Set display interface mode (C2h)
	LCD_MCU_DATA(0x02); //0x00

	LCD_MCU_CMD(0xB7);//
	LCD_MCU_DATA(0x00); //
	LCD_MCU_DATA(0x01);
	LCD_MCU_DATA(0x10);


	LCD_MCU_CMD(0xB0);//
	LCD_MCU_DATA(0x01); //
	LCD_MCU_DATA(0x05);


	LCD_MCU_CMD(0x44);//
	LCD_MCU_DATA(0x10);
	LCD_MCU_DATA(0x20);

	LCD_MCU_CMD(0x35);//
	LCD_MCU_DATA(0x00); //

	LCD_MCU_CMD(0x2A);//
	LCD_MCU_DATA(0x00); //
	LCD_MCU_DATA(0x00);
	LCD_MCU_DATA(0x01);
	LCD_MCU_DATA(0xDF);
 
	LCD_MCU_CMD(0x2B);//
	LCD_MCU_DATA(0x00);//
	LCD_MCU_DATA(0x00);
	LCD_MCU_DATA(0x03);
	LCD_MCU_DATA(0x1F);

	LCD_MCU_CMD(0x11); //
	msleep(120);
	LCD_MCU_CMD(0x29);//
	return 0;
}

static int hx8363_mcu_sleep(void)
{
	LCD_MCU_CMD(0x10);
	return 0;
}

static int hx8363_mcu_wakeup(void)
{
	LCD_MCU_CMD(0x11);
	return 0;
}

static int hx8363_mcu_set_active_win(struct gouda_rect *r)
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

static int hx8363_mcu_set_rotation(int rotate)
{
	return 0;
}

static int hx8363_mcu_close(void)
{
	return 0;
}

static struct rda_lcd_info hx8363_mcu_info = {
	.ops = {
		.s_init_gpio = hx8363_mcu_init_gpio,
		.s_open = hx8363_mcu_open,
		.s_active_win = hx8363_mcu_set_active_win,
		.s_rotation = hx8363_mcu_set_rotation,
		.s_sleep = hx8363_mcu_sleep,
		.s_wakeup = hx8363_mcu_wakeup,
		.s_close = hx8363_mcu_close},
	.lcd = {
		.width = WVGA_LCDD_DISP_X,
		.height = WVGA_LCDD_DISP_Y,
		.lcd_interface = GOUDA_LCD_IF_DBI,
		.lcd_timing = HX8363_MCU_TIMING,
		.lcd_cfg = HX8363_MCU_CONFIG},
	.name = HX8363_MCU_PANEL_NAME,
};

/*--------------------Platform Device Probe-------------------------*/

static int rda_fb_panel_hx8363_mcu_probe(struct platform_device *pdev)
{
	rda_fb_register_panel(&hx8363_mcu_info);

	dev_info(&pdev->dev, "rda panel hx8363_mcu registered\n");

	return 0;
}

static int rda_fb_panel_hx8363_mcu_remove(struct platform_device *pdev)
{
	return 0;
}

static struct platform_driver rda_fb_panel_hx8363_mcu_driver = {
	.probe = rda_fb_panel_hx8363_mcu_probe,
	.remove = rda_fb_panel_hx8363_mcu_remove,
	.driver = {
		   .name = HX8363_MCU_PANEL_NAME}
};

static struct rda_panel_driver hx8363_mcu_panel_driver = {
	.panel_type = GOUDA_LCD_IF_DBI,
	.lcd_driver_info = &hx8363_mcu_info,
	.pltaform_panel_driver = &rda_fb_panel_hx8363_mcu_driver,
};
