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

#define ILI9806H_MCU_CHIP_ID			0x9826
#ifdef CONFIG_PM
#include <linux/regulator/consumer.h>
#include <mach/regulator.h>
#endif /* CONFIG_PM */

//static u8 vcom=0x3b;
static struct rda_lcd_info ili9806h_mcu_info;
static struct rda_panel_id_param ili9806h_id_param;

/* wngl, for FPGA */
#define ILI9806H_MCU_TIMING {						     \
	{.tas       =  3,                                            \
	.tah        = 3,                                            \
	.pwl        =  8,                                           \
	.pwh        =  8}}                                          \

#define ILI9806H_MCU_CONFIG {                                             \
	{.cs             =   GOUDA_LCD_CS_0,                         \
	.output_fmt      =   GOUDA_LCD_OUTPUT_FORMAT_16_bit_RGB565,   \
	.cs0_polarity    =   false,                                  \
	.cs1_polarity    =   false,                                  \
	.rs_polarity     =   false,                                  \
	.wr_polarity     =   false,                                  \
	.rd_polarity     =   false,								\
	.te_en			  = 	1,									\
	.tecon2		  = 	0x100}}

static struct rda_lcd_info ili9806h_mcu_info;

static int ili9806h_mcu_init_gpio(void)
{
	int err;
	err = gpio_request(GPIO_LCD_RESET, "lcd reset");
	if(err)
		pr_info("%s gpio request fail \n",__func__);
	gpio_direction_output(GPIO_LCD_RESET, 1);
	gpio_set_value(GPIO_LCD_RESET, 1);
	msleep(5);
	gpio_set_value(GPIO_LCD_RESET, 0);
	msleep(10);
	gpio_set_value(GPIO_LCD_RESET, 1);
	msleep(120);

	return 0;
}

static int ili9806_mcu_readid_sub(void)
{
	u16 data[6] = {0};
	u32 cmd = 0xd3;

	LCD_MCU_CMD(0x11);
	msleep(20);
	/* read id */
	LCD_MCU_CMD(0xD3);
	panel_mcu_read_id(cmd,&ili9806h_id_param,data);

	printk(KERN_INFO "rda_fb: ili9806h_mcu_lyq_20131211 ID:"
	       "%02x %02x %02x %02x %02x %02x\n",
	       data[0], data[1], data[2], data[3], data[4], data[5]);

	if(data[2] == 0x98 && data[3] == 0x26)
		return 1;
	return 0;
}

static int ili9806h_mcu_readid(void)
{
	struct gouda_lcd *lcd = (void *)&(ili9806h_mcu_info.lcd);
	int ret = 0;
#ifdef CONFIG_PM
	struct regulator *lcd_reg;
#endif /* CONFIG_PM */

#ifdef CONFIG_PM
	lcd_reg = regulator_get(NULL, LDO_LCD);
	if (IS_ERR(lcd_reg)) {
		printk(KERN_ERR"rda-fb not find lcd regulator devices\n");
		return 0;
	}

	if ( regulator_enable(lcd_reg)< 0) {
		printk(KERN_ERR"rda-fb lcd could not be enabled!\n");
		return 0;
	}
#endif /* CONFIG_PM */

	rda_gouda_pre_enable_lcd(lcd,1);
	ili9806h_mcu_init_gpio();
	/* adjust timming */
	lcd->lcd_timing.mcu.pwl = 32;
	lcd->lcd_timing.mcu.pwh = 32;
	rda_gouda_configure_timing(lcd);
	ret = ili9806_mcu_readid_sub();
	/* adjust timming */
	lcd->lcd_timing.mcu.pwl = 8;
	lcd->lcd_timing.mcu.pwh = 8;

	rda_gouda_configure_timing(lcd);
	rda_gouda_pre_enable_lcd(lcd,0);

#ifdef CONFIG_PM
	if ( regulator_disable(lcd_reg)< 0) {
		printk(KERN_ERR"rda-fb lcd could not be enabled!\n");
		//return 0;
	}
#endif /* CONFIG_PM */

	return ret;
}

static int ili9806h_mcu_open(void)
{
	unsigned int width =0, height=0, voltage_diff=0;

#if defined(__PANEL_IDENTIFY_BY_VOLTAGE__)
	extern  int rda_load_lcd_id_voltage(void);
	voltage_diff = rda_load_lcd_id_voltage();
	printk(KERN_ERR"[lcd_id] %s voltage_diff %d\n",__func__,voltage_diff);
#endif

	if(voltage_diff < 900){
	//jingchuangrong-lcd
		LCD_MCU_CMD(0xFF);
		LCD_MCU_DATA(0xFF);
		LCD_MCU_DATA(0x98);
		LCD_MCU_DATA(0x26);

		//LCD_MCU_CMD(0xB6);
		//LCD_MCU_DATA(0x42);//02

		LCD_MCU_CMD(0xBC);//GIP1
		LCD_MCU_DATA(0x21);
		LCD_MCU_DATA(0x06);
		LCD_MCU_DATA(0x00);
		LCD_MCU_DATA(0x00);
		LCD_MCU_DATA(0x01);
		LCD_MCU_DATA(0x05);
		LCD_MCU_DATA(0x98);
		LCD_MCU_DATA(0x02);
		LCD_MCU_DATA(0x05);
		LCD_MCU_DATA(0x00);
		LCD_MCU_DATA(0x00);
		LCD_MCU_DATA(0x00);
		LCD_MCU_DATA(0x01);
		LCD_MCU_DATA(0x01);
		LCD_MCU_DATA(0x77);
		LCD_MCU_DATA(0xF4);
		LCD_MCU_DATA(0x00);
		LCD_MCU_DATA(0x00);
		LCD_MCU_DATA(0x00);
		LCD_MCU_DATA(0xC0);
		LCD_MCU_DATA(0x08);
		LCD_MCU_DATA(0x00);
		LCD_MCU_DATA(0x00);
		LCD_MCU_DATA(0x00);
		LCD_MCU_DATA(0x00);
		LCD_MCU_DATA(0x00);

		LCD_MCU_CMD(0xBD);//GIP2
		LCD_MCU_DATA(0x01);
		LCD_MCU_DATA(0x23);
		LCD_MCU_DATA(0x45);
		LCD_MCU_DATA(0x67);
		LCD_MCU_DATA(0x01);
		LCD_MCU_DATA(0x23);
		LCD_MCU_DATA(0x45);
		LCD_MCU_DATA(0x67);

		LCD_MCU_CMD(0xBE);//GIP3
		LCD_MCU_DATA(0x03);
		LCD_MCU_DATA(0x22);
		LCD_MCU_DATA(0x11);
		LCD_MCU_DATA(0x00);
		LCD_MCU_DATA(0x86);
		LCD_MCU_DATA(0x68);
		LCD_MCU_DATA(0x22);
		LCD_MCU_DATA(0x22);
		LCD_MCU_DATA(0xDA);
		LCD_MCU_DATA(0xCB);
		LCD_MCU_DATA(0xBC);
		LCD_MCU_DATA(0xAD);
		LCD_MCU_DATA(0x22);
		LCD_MCU_DATA(0x22);
		LCD_MCU_DATA(0x22);
		LCD_MCU_DATA(0x22);
		LCD_MCU_DATA(0x22);

		LCD_MCU_CMD(0x3A);
		LCD_MCU_DATA(0x55);

		LCD_MCU_CMD(0xFA);
		LCD_MCU_DATA(0x08);
		LCD_MCU_DATA(0x00);
		LCD_MCU_DATA(0x00);
		LCD_MCU_DATA(0x02);
		LCD_MCU_DATA(0x08);

		LCD_MCU_CMD(0xB1);
		LCD_MCU_DATA(0x00);
		LCD_MCU_DATA(0x70);//51  60
		LCD_MCU_DATA(0x03);

		LCD_MCU_CMD(0xB4);
		LCD_MCU_DATA(0x02);
		LCD_MCU_DATA(0x02);
		LCD_MCU_DATA(0x02);

		LCD_MCU_CMD(0xC0);
		LCD_MCU_DATA(0x0a);//08  0a

		LCD_MCU_CMD(0xC1);
		LCD_MCU_DATA(0x15);//10   1a
		LCD_MCU_DATA(0x78);
		LCD_MCU_DATA(0x6A);

		LCD_MCU_CMD(0xC7);
		LCD_MCU_DATA(0x43);//34  43
		LCD_MCU_DATA(0x00);
		LCD_MCU_DATA(0x8F);//23
		LCD_MCU_DATA(0x00);

		LCD_MCU_CMD(0xF2);//PANEL TIMING
		LCD_MCU_DATA(0x04);
		LCD_MCU_DATA(0x08);
		LCD_MCU_DATA(0x02);
		LCD_MCU_DATA(0x8A);
		LCD_MCU_DATA(0x07);
		LCD_MCU_DATA(0x04);

		LCD_MCU_CMD(0xF7);//480*800
		LCD_MCU_DATA(0x02);

		LCD_MCU_CMD(0xED);//EN_VOLT_REG
		LCD_MCU_DATA(0x7F);
		LCD_MCU_DATA(0x0F);

		//LCD_MCU_CMD(0xD7);
		//LCD_MCU_DATA(0x10);
		//LCD_MCU_DATA(0x06);
		//LCD_MCU_DATA(0x98);
		//LCD_MCU_DATA(0xA3);

		//changnei
		LCD_MCU_CMD(0xE0);
		LCD_MCU_DATA(0x00);
		LCD_MCU_DATA(0x04);
		LCD_MCU_DATA(0x0E);
		LCD_MCU_DATA(0x0D);
		LCD_MCU_DATA(0x09);
		LCD_MCU_DATA(0x17);
		LCD_MCU_DATA(0xC9);
		LCD_MCU_DATA(0x09);
		LCD_MCU_DATA(0x02);
		LCD_MCU_DATA(0x07);
		LCD_MCU_DATA(0x08);
		LCD_MCU_DATA(0x03);
		LCD_MCU_DATA(0x09);
		LCD_MCU_DATA(0x32);
		LCD_MCU_DATA(0x2F);
		LCD_MCU_DATA(0x00);

		LCD_MCU_CMD(0xE1);
		LCD_MCU_DATA(0x00);
		LCD_MCU_DATA(0x03);
		LCD_MCU_DATA(0x0A);
		LCD_MCU_DATA(0x0A);
		LCD_MCU_DATA(0x05);
		LCD_MCU_DATA(0x14);
		LCD_MCU_DATA(0x78);
		LCD_MCU_DATA(0x07);
		LCD_MCU_DATA(0x05);
		LCD_MCU_DATA(0x08);
		LCD_MCU_DATA(0x05);
		LCD_MCU_DATA(0x02);
		LCD_MCU_DATA(0x09);
		LCD_MCU_DATA(0x32);
		LCD_MCU_DATA(0x30);
		LCD_MCU_DATA(0x00);

		/*
		LCD_MCU_CMD(0xE0);
		LCD_MCU_DATA(0x00);
		LCD_MCU_DATA(0x08);
		LCD_MCU_DATA(0x11);
		LCD_MCU_DATA(0x10);
		LCD_MCU_DATA(0x11);
		LCD_MCU_DATA(0x19);
		LCD_MCU_DATA(0xC8);
		LCD_MCU_DATA(0x07);
		LCD_MCU_DATA(0x02);
		LCD_MCU_DATA(0x07);
		LCD_MCU_DATA(0x05);
		LCD_MCU_DATA(0x0D);
		LCD_MCU_DATA(0x0B);
		LCD_MCU_DATA(0x36);
		LCD_MCU_DATA(0x32);
		LCD_MCU_DATA(0x00);

		LCD_MCU_CMD(0xE1);
		LCD_MCU_DATA(0x00);
		LCD_MCU_DATA(0x08);
		LCD_MCU_DATA(0x17);
		LCD_MCU_DATA(0x10);
		LCD_MCU_DATA(0x12);
		LCD_MCU_DATA(0x15);
		LCD_MCU_DATA(0x78);
		LCD_MCU_DATA(0x07);
		LCD_MCU_DATA(0x06);
		LCD_MCU_DATA(0x0B);
		LCD_MCU_DATA(0x06);
		LCD_MCU_DATA(0x0E);
		LCD_MCU_DATA(0x0B);
		LCD_MCU_DATA(0x20);
		LCD_MCU_DATA(0x1B);
		LCD_MCU_DATA(0x00);
		*/

		LCD_MCU_CMD(0x35);
		LCD_MCU_DATA(0x00);

		LCD_MCU_CMD(0x44);
		LCD_MCU_DATA(0x50);
		LCD_MCU_DATA(0x80);

		LCD_MCU_CMD(0x11);
		msleep(120);
		LCD_MCU_CMD(0x29);
		msleep(20);
		LCD_MCU_CMD(0x2c);
	}

	for(height=0;height<800;height++){
		for(width=0;width<480;width++){
			 LCD_MCU_DATA(0x0000);
		}
	}
	return 0;
}

static int ili9806h_mcu_sleep(void)
{
	//gpio_set_value(GPIO_LCD_RESET, 0);
	LCD_MCU_CMD(0X28);
	msleep(50);
	LCD_MCU_CMD(0X10);
	msleep(120);
	printk(" %s  \n",__func__);
	return 0;
}

static int ili9806h_mcu_wakeup(void)
{
	gpio_set_value(GPIO_LCD_RESET, 1);
	msleep(5);
	gpio_set_value(GPIO_LCD_RESET, 0);
	msleep(10);
	gpio_set_value(GPIO_LCD_RESET, 1);
	msleep(120);
	ili9806h_mcu_open();

	return 0;
}

static int ili9806h_mcu_set_active_win(struct gouda_rect *r)
{
#if 0 // remove by xugang
	LCD_MCU_CMD(0x2a);
	LCD_MCU_DATA(r->tlX >> 8);
	LCD_MCU_DATA(r->tlX & 0x00ff);
	LCD_MCU_DATA(r->brX>>8);
	LCD_MCU_DATA(r->brX & 0x00ff);

	LCD_MCU_CMD(0x2b);
	LCD_MCU_DATA(r->tlY >> 8);
	LCD_MCU_DATA(r->tlY & 0x00ff);
	LCD_MCU_DATA(r->brY>>8);
	LCD_MCU_DATA(r->brY & 0x00ff);

	LCD_MCU_CMD(0x2c);
#endif
	return 0;
}

static int ili9806h_mcu_set_rotation(int rotate)
{
	return 0;
}

static int ili9806h_mcu_close(void)
{
	return 0;
}
static struct rda_panel_id_param ili9806h_id_param = {
	.lcd_info = &ili9806h_mcu_info,
	.per_read_bytes = 4,
};


static struct rda_lcd_info ili9806h_mcu_info = {
	.ops = {
		.s_init_gpio = ili9806h_mcu_init_gpio,
		.s_open = ili9806h_mcu_open,
		.s_readid = ili9806h_mcu_readid,
		.s_active_win = ili9806h_mcu_set_active_win,
		.s_rotation = ili9806h_mcu_set_rotation,
		.s_sleep = ili9806h_mcu_sleep,
		.s_wakeup = ili9806h_mcu_wakeup,
		.s_close = ili9806h_mcu_close},
	.lcd = {
		.width = WVGA_LCDD_DISP_X,
		.height = WVGA_LCDD_DISP_Y,
		.lcd_interface = GOUDA_LCD_IF_DBI,
		.lcd_timing = ILI9806H_MCU_TIMING,
		.lcd_cfg = ILI9806H_MCU_CONFIG},
	.name = ILI9806H_MCU_PANEL_NAME,
};

/*--------------------Platform Device Probe-------------------------*/

static int rda_fb_panel_ili9806h_mcu_probe(struct platform_device *pdev)
{
	pr_info("%s \n",__func__);
	rda_fb_register_panel(&ili9806h_mcu_info);

	dev_info(&pdev->dev, "rda panel ili9806h_mcu registered\n");

	return 0;
}

static int rda_fb_panel_ili9806h_mcu_remove(struct platform_device *pdev)
{
	return 0;
}

static struct platform_driver rda_fb_panel_ili9806h_mcu_driver = {
	.probe = rda_fb_panel_ili9806h_mcu_probe,
	.remove = rda_fb_panel_ili9806h_mcu_remove,
	.driver = {
		   .name = ILI9806H_MCU_PANEL_NAME}
};

static struct rda_panel_driver ili9806h_mcu_panel_driver = {
	.panel_type = GOUDA_LCD_IF_DBI,
	.lcd_driver_info = &ili9806h_mcu_info,
	.pltaform_panel_driver = &rda_fb_panel_ili9806h_mcu_driver,
};
static int __init rda_fb_panel_ili9806h_mcu_init(void)
{
	rda_fb_probe_panel(&ili9806h_mcu_info, &rda_fb_panel_ili9806h_mcu_driver);
	return platform_driver_register(&rda_fb_panel_ili9806h_mcu_driver);
}

static void __exit rda_fb_panel_ili9806h_mcu_exit(void)
{
	platform_driver_unregister(&rda_fb_panel_ili9806h_mcu_driver);
}

module_init(rda_fb_panel_ili9806h_mcu_init);
module_exit(rda_fb_panel_ili9806h_mcu_exit);
