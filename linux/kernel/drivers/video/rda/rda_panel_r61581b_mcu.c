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

#ifdef CONFIG_PM
#include <linux/regulator/consumer.h>
#include <mach/regulator.h>
#endif

#define R61581B_MCU_CHIP_ID			0x1581

/* timing for real platform*/
#define R61581B_MCU_TIMING {						     \
	{.tas       =  7,                                            \
		.tah        =  7,                                            \
		.pwl        =  16,                                           \
		.pwh        =  16}}                                          \

#define R61581B_MCU_CONFIG {                                             \
	{.cs             =   GOUDA_LCD_CS_0,                         \
		.output_fmt      =   GOUDA_LCD_OUTPUT_FORMAT_16_bit_RGB565,   \
		.cs0_polarity    =   false,                                  \
		.cs1_polarity    =   false,                                  \
		.rs_polarity     =   false,                                  \
		.wr_polarity     =   false,                                  \
		.rd_polarity     =   false,				     \
		.te_en           =   1,				     	     \
		.tecon2		 = 0x100}}

static struct rda_lcd_info R61581B_mcu_info;
static int R61581B_mcu_init_gpio(void)
{
	gpio_request(GPIO_LCD_RESET, "lcd reset");
	gpio_direction_output(GPIO_LCD_RESET, 1);
	gpio_set_value(GPIO_LCD_RESET, 1);
	mdelay(10);
	gpio_set_value(GPIO_LCD_RESET, 0);
	mdelay(120);
	gpio_set_value(GPIO_LCD_RESET, 1);
	mdelay(10);

	return 0;
}

static int R61581B_mcu_readid(void)
{
	u8 data[6];
	struct regulator *lcd_reg;
	struct gouda_lcd *lcd = (void *)&(R61581B_mcu_info.lcd);

#ifdef CONFIG_PM
	lcd_reg = regulator_get(NULL,LDO_LCD);
	if(IS_ERR(lcd_reg)){
		printk(KERN_ERR "rda-fb not find lcd regulator devices\n");
		return 0;
	}

	if(regulator_enable(lcd_reg) < 0){
		printk(KERN_ERR "rda-fb lcd could not be enable !\n");
		return 0;
	}
#endif
	rda_gouda_pre_enable_lcd(lcd,1);
	R61581B_mcu_init_gpio();

	/* adjust timming */
	lcd->lcd_timing.mcu.pwl = 32; 
	lcd->lcd_timing.mcu.pwh = 32; 
	rda_gouda_configure_timing(lcd);

	LCD_MCU_CMD(0xb0);   
	LCD_MCU_DATA(0x00);
	/* read id */
	LCD_MCU_CMD(0xbf);
	LCD_MCU_READ(&data[0]);
	LCD_MCU_READ(&data[1]);
	LCD_MCU_READ(&data[2]);
	LCD_MCU_READ(&data[3]);
	LCD_MCU_READ(&data[4]);
	LCD_MCU_READ(&data[5]);

	printk(KERN_INFO "rda_fb: R61581B_mcu ID:"
			"%02x %02x %02x %02x %02x %02x\n",
			data[0], data[1], data[2], data[3],data[4],data[5]);

	/* adjust timming */
	lcd->lcd_timing.mcu.pwl = 16; 
	lcd->lcd_timing.mcu.pwh = 16; 
	rda_gouda_configure_timing(lcd);

	rda_gouda_pre_enable_lcd(lcd,0);
#ifdef CONFIG_PM
	if(regulator_disable(lcd_reg) < 0){
		printk(KERN_ERR "rda-fb lcd could not be disabled!\n");
	}
#endif
	if((data[3]==0x15)&&(data[4]==0x81))
		return 1;
	else
		return 0;
}

static int R61581B_mcu_open(void)
{


	mdelay(10);
	LCD_MCU_CMD(0xFF);
	LCD_MCU_CMD(0xFF);
	mdelay(5);
	LCD_MCU_CMD(0xFF);
	LCD_MCU_CMD(0xFF);
	LCD_MCU_CMD(0xFF);
	LCD_MCU_CMD(0xFF);
	mdelay(10);

	LCD_MCU_CMD(0xB0);
	LCD_MCU_DATA(0x00);

	LCD_MCU_CMD(0xB3);
	LCD_MCU_DATA(0x02);
	LCD_MCU_DATA(0x00);
	LCD_MCU_DATA(0x00);
	LCD_MCU_DATA(0x00);

	LCD_MCU_CMD(0xC0);
	LCD_MCU_DATA(0x13);
	LCD_MCU_DATA(0x3B);//480
	LCD_MCU_DATA(0x00);
	LCD_MCU_DATA(0x02);
	LCD_MCU_DATA(0x00);
	LCD_MCU_DATA(0x01);
	LCD_MCU_DATA(0x00);//NW
	LCD_MCU_DATA(0x43);

	LCD_MCU_CMD(0xC1);
	LCD_MCU_DATA(0x08);
	LCD_MCU_DATA(0x12);//CLOCK
	LCD_MCU_DATA(0x08);
	LCD_MCU_DATA(0x08);

	LCD_MCU_CMD(0xC4);
	LCD_MCU_DATA(0x11);
	LCD_MCU_DATA(0x07);
	LCD_MCU_DATA(0x03);
	LCD_MCU_DATA(0x03);

	LCD_MCU_CMD(0xC6);//dataenable,VSYNC,HSYNC,PCKL??
	LCD_MCU_DATA(0x02);

	LCD_MCU_CMD(0xC8);//GAMMA
	LCD_MCU_DATA(0x03);
	LCD_MCU_DATA(0x03);
	LCD_MCU_DATA(0x13);
	LCD_MCU_DATA(0x5C);
	LCD_MCU_DATA(0x03);
	LCD_MCU_DATA(0x07);
	LCD_MCU_DATA(0x14);
	LCD_MCU_DATA(0x08);
	LCD_MCU_DATA(0x00);
	LCD_MCU_DATA(0x21);

	LCD_MCU_DATA(0x08);
	LCD_MCU_DATA(0x14);
	LCD_MCU_DATA(0x07);
	LCD_MCU_DATA(0x53);
	LCD_MCU_DATA(0x0C);
	LCD_MCU_DATA(0x13);
	LCD_MCU_DATA(0x03);
	LCD_MCU_DATA(0x03);
	LCD_MCU_DATA(0x21);
	LCD_MCU_DATA(0x00);

	LCD_MCU_CMD(0x35);
	LCD_MCU_DATA(0x00);

	LCD_MCU_CMD(0x36);
	LCD_MCU_DATA(0x00);

	LCD_MCU_CMD(0x3A);
	LCD_MCU_DATA(0x55);//24bit=77,18bit=66,16bit=55

	LCD_MCU_CMD(0x44);
	LCD_MCU_DATA(0x00);
	LCD_MCU_DATA(0x01);

	LCD_MCU_CMD(0xD0);
	LCD_MCU_DATA(0x07);
	LCD_MCU_DATA(0x07);//VCI1
	LCD_MCU_DATA(0x1D);//VRH
	LCD_MCU_DATA(0x02);//BT

	LCD_MCU_CMD(0xD1);
	LCD_MCU_DATA(0x03);
	LCD_MCU_DATA(0x1A);//VCM
	LCD_MCU_DATA(0x10);//VDV

	LCD_MCU_CMD(0xD2);
	LCD_MCU_DATA(0x03);
	LCD_MCU_DATA(0x14);
	LCD_MCU_DATA(0x04);

	LCD_MCU_CMD(0x29);
	mdelay(30);

	LCD_MCU_CMD(0x2A);
	LCD_MCU_DATA(0x00);
	LCD_MCU_DATA(0x00);
	LCD_MCU_DATA(0x01);
	LCD_MCU_DATA(0x3F);//320

	LCD_MCU_CMD(0x2B);
	LCD_MCU_DATA(0x00);
	LCD_MCU_DATA(0x00);
	LCD_MCU_DATA(0x01);
	LCD_MCU_DATA(0xDF);//480

	LCD_MCU_CMD(0xB4);
	LCD_MCU_DATA(0x00);//RGB???10,MCU?00
	mdelay(100);

	LCD_MCU_CMD(0xB0);
	LCD_MCU_DATA(0x03);

	LCD_MCU_CMD(0x11);
	mdelay(150);

	LCD_MCU_CMD(0x2C);


	return 0;
}

static int R61581B_mcu_sleep(void)
{
	gpio_set_value(GPIO_LCD_RESET, 0);

	return 0;
}

static int R61581B_mcu_wakeup(void)
{
	/* as we go entire power off, we need to re-init the LCD */
	gpio_set_value(GPIO_LCD_RESET, 1);
	mdelay(10);
	gpio_set_value(GPIO_LCD_RESET, 0);
	mdelay(1);
	gpio_set_value(GPIO_LCD_RESET, 1);
	mdelay(1);

	R61581B_mcu_open();

	return 0;
}

static int R61581B_mcu_set_active_win(struct gouda_rect *r)
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

static int R61581B_mcu_set_rotation(int rotate)
{
	return 0;
}

static int R61581B_mcu_close(void)
{
	return 0;
}

static struct rda_lcd_info R61581B_mcu_info = {
	.ops = {
		.s_init_gpio = R61581B_mcu_init_gpio,
		.s_open = R61581B_mcu_open,
		.s_readid = R61581B_mcu_readid,
		.s_active_win = R61581B_mcu_set_active_win,
		.s_rotation = R61581B_mcu_set_rotation,
		.s_sleep = R61581B_mcu_sleep,
		.s_wakeup = R61581B_mcu_wakeup,
		.s_close = R61581B_mcu_close},
	.lcd = {
		.width = HVGA_LCDD_DISP_X,
		.height = HVGA_LCDD_DISP_Y,
		.lcd_interface = GOUDA_LCD_IF_DBI,
		.lcd_timing = R61581B_MCU_TIMING,
		.lcd_cfg = R61581B_MCU_CONFIG},
	.name = R61581B_MCU_PANEL_NAME,
};

/*--------------------Platform Device Probe-------------------------*/

static int rda_fb_panel_R61581B_mcu_probe(struct platform_device *pdev)
{
	rda_fb_register_panel(&R61581B_mcu_info);

	dev_info(&pdev->dev, "rda panel R61581B_mcu registered\n");

	return 0;
}

static int rda_fb_panel_R61581B_mcu_remove(struct platform_device *pdev)
{
	return 0;
}

static struct platform_driver rda_fb_panel_R61581B_mcu_driver = {
	.probe = rda_fb_panel_R61581B_mcu_probe,
	.remove = rda_fb_panel_R61581B_mcu_remove,
	.driver = {
		.name = R61581B_MCU_PANEL_NAME}
};

static struct rda_panel_driver r6158b_mcu_panel_driver = {
	.panel_type = GOUDA_LCD_IF_DBI,
	.lcd_driver_info = &R61581B_mcu_info,
	.pltaform_panel_driver = &rda_fb_panel_R61581B_mcu_driver,
};
