#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/jiffies.h>
#include <linux/gpio.h>

#include <plat/devices.h>
#include <plat/rda_debug.h>
#include <mach/board.h>

#include "rda_panel.h"
#include "rda_lcdc.h"

#define HX8357_MCU_CHIP_ID			0x8357
static struct rda_lcd_info hx8357_mcu_info;

static int hx8357_mcu_reset_gpio(void)
{
	printk("%s\n",__func__);
	gpio_set_value(GPIO_LCD_RESET, 1);
	mdelay(5);
	gpio_set_value(GPIO_LCD_RESET, 0);
	mdelay(10);
	gpio_set_value(GPIO_LCD_RESET, 1);
	msleep(120);

	return 0;
}

static bool himax_8357_mcu_match_id(void)
{
	u8 data[6];
	u32 pwl_old, pwh_old;
	struct lcd_panel_info *lcd = (void *)&(hx8357_mcu_info.lcd);

	rda_lcdc_pre_enable_lcd(lcd,1);
	hx8357_mcu_reset_gpio();

	/* adjust timming */
	pwl_old = lcd->mcu_pinfo.pwl;
	pwh_old = lcd->mcu_pinfo.pwh;
	lcd->mcu_pinfo.pwl = 32;
	lcd->mcu_pinfo.pwh = 32;
	rda_lcdc_configure_timing(lcd);

	LCD_MCU_CMD(0x11);
	msleep(120);
	LCD_MCU_CMD(0xb9);
	LCD_MCU_DATA(0xff);
	LCD_MCU_DATA(0x83);
	LCD_MCU_DATA(0x57);
	msleep(15);

	/* read id */
	LCD_MCU_CMD(0xd0);
	LCD_MCU_READ(&data[0]);
	LCD_MCU_READ(&data[1]);
	LCD_MCU_READ(&data[2]);
	LCD_MCU_READ(&data[3]);
	LCD_MCU_READ(&data[4]);
	LCD_MCU_READ(&data[5]);

	printk(KERN_INFO "rda_fb: himax_8357 ID:"
			"%02x %02x %02x %02x %02x %02x\n",
			data[0], data[1], data[2], data[3],data[4],data[5]);

	/* adjust timming */
	lcd->mcu_pinfo.pwl = pwl_old;
	lcd->mcu_pinfo.pwh = pwh_old;
	rda_lcdc_configure_timing(lcd);

	rda_lcdc_pre_enable_lcd(lcd,0);

	if(data[1]== 0x90)
		return true;
	else
		return false;
}

static int hx8357_mcu_open(void)
{
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
	msleep(10);
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
	hx8357_mcu_reset_gpio();
	return 0;
}

static int hx8357_mcu_set_active_win(struct lcd_img_rect *r)
{
	LCD_MCU_CMD(0x2a);	/* Set Column Address */
	LCD_MCU_DATA(r->tlx >> 8);
	LCD_MCU_DATA(r->tlx & 0x00ff);
	LCD_MCU_DATA((r->brx) >> 8);
	LCD_MCU_DATA((r->brx) & 0x00ff);

	LCD_MCU_CMD(0x2b);	/* Set Page Address */
	LCD_MCU_DATA(r->tly >> 8);
	LCD_MCU_DATA(r->tly & 0x00ff);
	LCD_MCU_DATA((r->bry) >> 8);
	LCD_MCU_DATA((r->bry) & 0x00ff);

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
	.name = HX8357_MCU_PANEL_NAME,
	.ops = {
		.s_reset_gpio = hx8357_mcu_reset_gpio,
		.s_match_id = himax_8357_mcu_match_id,
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
			.te_en = 1,
			.tecon2 = 0x100
		}
	}
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
	.panel_type = LCD_IF_DBI,
	.lcd_driver_info = &hx8357_mcu_info,
	.pltaform_panel_driver = &rda_fb_panel_hx8357_mcu_driver,
};
