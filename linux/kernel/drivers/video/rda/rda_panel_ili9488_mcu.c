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

#define ILI9488_MCU_CHIP_ID			0x3AB6

/* timing for real platform*/
#define ILI9488_MCU_TIMING {						     \
	{.tas       =  7,                                            \
		.tah        =  7,                                            \
		.pwl        =  16,                                           \
		.pwh        =  16}}                                          \

#define ILI9488_MCU_CONFIG {                                             \
	{.cs             =   GOUDA_LCD_CS_0,                         \
		.output_fmt      =   GOUDA_LCD_OUTPUT_FORMAT_16_bit_RGB565,   \
		.cs0_polarity    =   false,                                  \
		.cs1_polarity    =   false,                                  \
		.rs_polarity     =   false,                                  \
		.wr_polarity     =   false,                                  \
		.rd_polarity     =   false,				     \
		.te_en           =   1,				     	     \
		.tecon2		 = 0x100}}

static struct rda_lcd_info ILI9488_mcu_info;
static int ILI9488_mcu_init_gpio(void)
{
	gpio_request(GPIO_LCD_RESET, "lcd reset");
	gpio_direction_output(GPIO_LCD_RESET, 1);
	gpio_set_value(GPIO_LCD_RESET, 1);
	mdelay(10);
	gpio_set_value(GPIO_LCD_RESET, 0);
	mdelay(60);
	gpio_set_value(GPIO_LCD_RESET, 1);
	mdelay(60);

	return 0;
}

static int ILI9488_mcu_readid(void)
{
	u8 data[6];
	struct regulator *lcd_reg;
	struct gouda_lcd *lcd = (void *)&(ILI9488_mcu_info.lcd);

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
	ILI9488_mcu_init_gpio();

	/* adjust timming */
	lcd->lcd_timing.mcu.pwl = 32; 
	lcd->lcd_timing.mcu.pwh = 32; 
	rda_gouda_configure_timing(lcd);

	/* read id */
	LCD_MCU_CMD(0xd3);
	LCD_MCU_READ(&data[0]);
	LCD_MCU_READ(&data[1]);
	LCD_MCU_READ(&data[2]);
	LCD_MCU_READ(&data[3]);
	LCD_MCU_READ(&data[4]);
	LCD_MCU_READ(&data[5]);

	printk(KERN_INFO "rda_fb: ILI9488_mcu ID:"
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
	if(((data[2]==0x94)&&(data[3]==0x88)) || ((data[3]==0x94)&&(data[4]==0x88))
		|| ((data[2]==0x77)&&(data[3]==0x96)))
		return 1;
	else
		return 0;
}

static int ILI9488_mcu_open(void)
{
	int i;

	mdelay(120);
	LCD_MCU_CMD(0xE0);   //P-Gamma
	LCD_MCU_DATA(0x00);
	LCD_MCU_DATA(0x04);
	LCD_MCU_DATA(0x0D);
	LCD_MCU_DATA(0x07);
	LCD_MCU_DATA(0x15);
	LCD_MCU_DATA(0x0A);
	LCD_MCU_DATA(0x3A);
	LCD_MCU_DATA(0x88);
	LCD_MCU_DATA(0x48);
	LCD_MCU_DATA(0x08);
	LCD_MCU_DATA(0x0E);
	LCD_MCU_DATA(0x0B);
	LCD_MCU_DATA(0x17);
	LCD_MCU_DATA(0x1B);
	LCD_MCU_DATA(0x0F);
	LCD_MCU_CMD(0XE1);    //N-Gamma
	LCD_MCU_DATA(0x00);
	LCD_MCU_DATA(0x1A);
	LCD_MCU_DATA(0x1D);
	LCD_MCU_DATA(0x03);
	LCD_MCU_DATA(0x10);
	LCD_MCU_DATA(0x06);
	LCD_MCU_DATA(0x31);
	LCD_MCU_DATA(0x34);
	LCD_MCU_DATA(0x43);
	LCD_MCU_DATA(0x02);
	LCD_MCU_DATA(0x09);
	LCD_MCU_DATA(0x08);
	LCD_MCU_DATA(0x30);
	LCD_MCU_DATA(0x36);
	LCD_MCU_DATA(0x0F);

	LCD_MCU_CMD(0XC0);   //Power Control 1
	LCD_MCU_DATA(0x10);   //Vreg1out
	LCD_MCU_DATA(0x10);   //Verg2out

	LCD_MCU_CMD(0xC1);  //Power Control 2
	LCD_MCU_DATA(0x41); //VGH,VGL

	LCD_MCU_CMD(0xC5); //Power Control 3
	LCD_MCU_DATA(0x00);
	LCD_MCU_DATA(0x2c); //Vcom
	LCD_MCU_DATA(0x80);

	LCD_MCU_CMD(0x35); //
	LCD_MCU_DATA(0x00);

	LCD_MCU_CMD(0x36); //Memory Access
	LCD_MCU_DATA(0x48);

	LCD_MCU_CMD(0x3A); // Interface Pixel Format
	LCD_MCU_DATA(0x55);

	//LCD_MCU_CMD(0XB0); // Interface Mode Control
	//LCD_MCU_DATA(0x00);

	LCD_MCU_CMD(0xB1); //Frame rate
	LCD_MCU_DATA(0xA0);
	LCD_MCU_DATA(0x11);

	LCD_MCU_CMD(0xB4); //Display Inversion Control
	LCD_MCU_DATA(0x02);

	LCD_MCU_CMD(0XB6); //RGB/MCU Interface Control
	LCD_MCU_DATA(0x02); //MCU
	LCD_MCU_DATA(0x02); //Source,Gate scan dieection
	LCD_MCU_DATA(0x3B);

	//LCD_MCU_CMD(0XB7);
	//LCD_MCU_DATA(0xC6);

	LCD_MCU_CMD(0xF7);
	LCD_MCU_DATA(0xA9);
	LCD_MCU_DATA(0x51);
	LCD_MCU_DATA(0x2C);
	LCD_MCU_DATA(0x82);

	LCD_MCU_CMD(0xE9);
	LCD_MCU_DATA(0x00);

	LCD_MCU_CMD(0x11); //Sleep out

	mdelay(60);

	// Add for flash in poweron;
	LCD_MCU_CMD(0x2c);
	for (i = 0; i < 320 * 480; i++)
	{
		LCD_MCU_DATA(0x0000); // Black;
	}
	LCD_MCU_CMD(0x29);

	return 0;
}

static int ILI9488_mcu_display_off(void)
{
	return 0;
}

static int ILI9488_mcu_display_on(void)
{
	return 0;
}

static int ILI9488_mcu_sleep(void)
{
	gpio_set_value(GPIO_LCD_RESET, 0);

	return 0;
}

static int ILI9488_mcu_wakeup(void)
{
	/* as we go entire power off, we need to re-init the LCD */
	gpio_set_value(GPIO_LCD_RESET, 1);
	mdelay(10);
	gpio_set_value(GPIO_LCD_RESET, 0);
	mdelay(1);
	gpio_set_value(GPIO_LCD_RESET, 1);
	mdelay(1);

	ILI9488_mcu_open();

	return 0;
}

static int ILI9488_mcu_set_active_win(struct gouda_rect *r)
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

static int ILI9488_mcu_set_rotation(int rotate)
{
	return 0;
}

static int ILI9488_mcu_close(void)
{
	return 0;
}

static void ILI9488_mcu_read_fb(u8 *buffer)
{
	int i = 0;
	short x0, y0, x1, y1;
	short h_X_start,l_X_start,h_X_end,l_X_end,h_Y_start,l_Y_start,h_Y_end,l_Y_end;
	u16 readData[3];
	struct gouda_lcd *lcd = (void *)&(ILI9488_mcu_info.lcd);
	printk("ILI9488_mcu_read_fb\n");

	rda_gouda_stretch_pre_wait_and_enable_clk();

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
	lcd->lcd_timing.mcu.pwl = 32;
	lcd->lcd_timing.mcu.pwh = 32;
	rda_gouda_configure_timing(lcd);

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
#ifdef LCD_IF_MCU_8BIT_FORMAT
	for(i=0; i<30; i++)
	{
		LCD_MCU_READ16(&readData[0]);
		LCD_MCU_READ16(&readData[1]);
		LCD_MCU_READ16(&readData[2]);
		printk("rda_factory_auto_test 0x%04x 0x%04x 0x%04x\n", readData[0], readData[1], readData[2]);
		((u16 *)buffer)[i]	 = (((readData[0]&0x00FF)>>3)<<11)|(((readData[1]&0x00FF)>>2)<<5)|((readData[2]&0x00FF)>>3);
		printk("rda_factory_auto_test 0x%04x\n", ((u16 *)buffer)[i]);
	}
#else
	for(i=0; i<30; i+=2)
	{
		LCD_MCU_READ16(&readData[0]);
		LCD_MCU_READ16(&readData[1]);
		LCD_MCU_READ16(&readData[2]);
		printk("rda_factory_auto_test 0x%04x 0x%04x 0x%04x\n", readData[0], readData[1], readData[2]);
		((u16 *)buffer)[i]   = (readData[0]&0xF800)|((readData[0]&0xFC)<<3)|((readData[1]&0xF800)>>11);
		((u16 *)buffer)[i+1] = ((readData[1]&0x00F8)<<8)|((readData[2]>>10)<<5)|((readData[2]&0xF8)>>3);
		printk("rda_factory_auto_test 0x%04x 0x%04x\n", ((u16 *)buffer)[i], ((u16 *)buffer)[i+1]);
	}
#endif
	/* adjust timming */
	lcd->lcd_timing.mcu.pwl = 16;
	lcd->lcd_timing.mcu.pwh = 16;
	rda_gouda_configure_timing(lcd);

	rda_gouda_stretch_post_disable_clk();

}

static int ILI9488_rda_lcd_dbg_w(void *p,int n)
{
	int i;
	int *buf = (int *)p;

	LCD_MCU_CMD(buf[0]);
	for(i = 1;i< n;i++)
		LCD_MCU_DATA(buf[i]);

	return 0;
}


static struct rda_lcd_info ILI9488_mcu_info = {
	.ops = {
		.s_init_gpio = ILI9488_mcu_init_gpio,
		.s_open = ILI9488_mcu_open,
		.s_readid = ILI9488_mcu_readid,
		.s_active_win = ILI9488_mcu_set_active_win,
		.s_rotation = ILI9488_mcu_set_rotation,
		.s_sleep = ILI9488_mcu_sleep,
		.s_wakeup = ILI9488_mcu_wakeup,
		.s_display_on = ILI9488_mcu_display_on,
		.s_display_off = ILI9488_mcu_display_off,
		.s_close = ILI9488_mcu_close,
		.s_read_fb = ILI9488_mcu_read_fb,
		.s_rda_lcd_dbg_w = ILI9488_rda_lcd_dbg_w},
	.lcd = {
		.width = HVGA_LCDD_DISP_X,
		.height = HVGA_LCDD_DISP_Y,
		.lcd_interface = GOUDA_LCD_IF_DBI,
		.lcd_timing = ILI9488_MCU_TIMING,
		.lcd_cfg = ILI9488_MCU_CONFIG},
	.name = ILI9488_MCU_PANEL_NAME,
};

/*--------------------Platform Device Probe-------------------------*/

static int rda_fb_panel_ILI9488_mcu_probe(struct platform_device *pdev)
{
	rda_fb_register_panel(&ILI9488_mcu_info);

	dev_info(&pdev->dev, "rda panel ILI9488_mcu registered\n");

	return 0;
}

static int rda_fb_panel_ILI9488_mcu_remove(struct platform_device *pdev)
{
	return 0;
}

static struct platform_driver rda_fb_panel_ILI9488_mcu_driver = {
	.probe = rda_fb_panel_ILI9488_mcu_probe,
	.remove = rda_fb_panel_ILI9488_mcu_remove,
	.driver = {
		.name = ILI9488_MCU_PANEL_NAME}
};

static struct rda_panel_driver ili9488_mcu_panel_driver = {
	.panel_type = GOUDA_LCD_IF_DBI,
	.lcd_driver_info = &ILI9488_mcu_info,
	.pltaform_panel_driver = &rda_fb_panel_ILI9488_mcu_driver,
};
