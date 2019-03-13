#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/jiffies.h>
#include <linux/gpio.h>
#include <linux/io.h>
#include <linux/spi/spi.h>

#include <plat/devices.h>
#include <plat/rda_debug.h>
#include <mach/board.h>

#include "rda_gouda.h"
#include "rda_panel.h"

#ifdef CONFIG_PM
#include <linux/regulator/consumer.h>
#include <mach/regulator.h>
#endif /* CONFIG_PM */
#include <plat/reg_spi.h>
//#define ILI9806C_FVGA

static struct rda_spi_panel_device *rda_ili9806c_spi_dev = NULL;
static struct rda_lcd_info ili9806c_info;
static struct rda_panel_id_param ili9806c_id_param;

RDA_SPI_PARAMETERS ili9806c_spicfg;

#define rda_spi_dev  rda_ili9806c_spi_dev

static inline void panel_spi_send_data(u8 data)
{
	unsigned short AddressAndData;
	AddressAndData = ((1 << 8) | data);

	//please nots, spi write 1 slot!
	panel_spi_transfer(rda_spi_dev, (u8 *) (&AddressAndData), NULL, 1);
	mdelay(2);

}

static inline void panel_spi_send_cmd(u8 cmd)
{
	u16 AddressAndData;
	AddressAndData = cmd;
	//please nots, spi write 1 slot!
	panel_spi_transfer(rda_spi_dev, (u8 *) (&AddressAndData), NULL, 1);
	mdelay(2);
}

static inline int ili9806c_readid_sub(void)
{
	u32 cmd = 0xd3;
	u16 read_id[3] = {0};

	panel_spi_send_cmd(0xFF); // EXTC Command Set enable register
	panel_spi_send_data(0xFF);
	panel_spi_send_data(0x98);
	panel_spi_send_data(0x06);

	panel_spi_send_cmd(0xBA);
	panel_spi_send_data(0x60);

	panel_spi_send_cmd(0x11);
	msleep(20);

	panel_spi_read_id(cmd,&ili9806c_id_param,read_id);

	if(read_id[0] == 0 && read_id[1] == 0x98 && read_id[2] == 0)
		return 1;

	return 0;
}

static struct rda_panel_id_param ili9806c_id_param = {
	.lcd_info = &ili9806c_info,
	.lcd_spicfg = &ili9806c_spicfg,
	.dumy_bits = 1,
	.per_read_bytes = 3,
};

RDA_SPI_PARAMETERS ili9806c_spicfg = {
	.inputEn = true,
	.clkDelay = RDA_SPI_HALF_CLK_PERIOD_0,
	.doDelay = RDA_SPI_HALF_CLK_PERIOD_0,
	.diDelay = RDA_SPI_HALF_CLK_PERIOD_2,
	.csDelay = RDA_SPI_HALF_CLK_PERIOD_0,
	.csPulse = RDA_SPI_HALF_CLK_PERIOD_0,
	.frameSize = 9,
	.oeRatio = 9,
	.rxTrigger = RDA_SPI_RX_TRIGGER_1_BYTE,
	.txTrigger = RDA_SPI_TX_TRIGGER_1_EMPTY,
	.rxMode = RDA_SPI_DIRECT_POLLING,
	.txMode = RDA_SPI_DIRECT_POLLING,
	.mask = {0, 0, 0, 0, 0},
	.handler = NULL
};

#ifdef ILI9806C_FVGA
#define ILI9806C_TIMING			\
	{				\
	.lcd_freq = 33000000, /* 15MHz */	\
	.clk_divider = 0,			\
	.height = FWVGA_LCDD_DISP_Y,	\
	.width = FWVGA_LCDD_DISP_X,	\
	.h_low=10,			\
	.v_low =2,			\
	.h_back_porch = 50,		\
	.h_front_porch = 200,		\
	.v_back_porch = 18,		\
	.v_front_porch = 20,		\
	}                               \

#else

#define ILI9806C_TIMING			\
	{				\
	.lcd_freq = 15000000, /* 15MHz */	\
	.clk_divider = 0,			\
	.height = WVGA_LCDD_DISP_Y,	\
	.width = WVGA_LCDD_DISP_X,	\
	.h_low=5,			\
	.v_low =5,			\
	.h_back_porch = 100,		\
	.h_front_porch = 100,		\
	.v_back_porch = 50,		\
	.v_front_porch = 50,		\
	}                               \

#endif

#ifdef ILI9806C_FVGA

#define ILI9806C_CONFIG                 \
	{				\
	.frame1 =1,			\
	.rgb_format = RGB_IS_24BIT,	\
	}

#else

#define ILI9806C_CONFIG                 \
	{				\
	.frame1 =1,			\
	.rgb_format = RGB_IS_16BIT,	\
	}

#endif

static int ili9806c_init_gpio(void)
{
	gpio_request(GPIO_LCD_RESET, "lcd reset");
	gpio_direction_output(GPIO_LCD_RESET, 1);
	mdelay(1);
	gpio_set_value(GPIO_LCD_RESET, 0);
	mdelay(1);
	gpio_set_value(GPIO_LCD_RESET, 1);
	mdelay(1);

	return 0;
}

static int ili9806c_spi_readid_setup(struct spi_device *spi)
{
	struct rda_spi_panel_device *panel_dev;
	int ret = 0;

	panel_dev = kzalloc(sizeof(*panel_dev), GFP_KERNEL);

	if (panel_dev == NULL) {
		dev_err(&spi->dev, "rda_fb_panel_ili9486l, out of memory\n");
		return -ENOMEM;
	}

	spi->mode = SPI_MODE_2;
	panel_dev->spi = spi;
	panel_dev->spi_xfer_num = 0;

	dev_set_drvdata(&spi->dev, panel_dev);

	spi->bits_per_word = 9;
	spi->max_speed_hz = 500000;
	spi->controller_data = &ili9806c_spicfg;

	ret = spi_setup(spi);

	if (ret < 0) {
		printk("error spi_setup failed\n");
		goto out_free_dev;
	}

	rda_ili9806c_spi_dev = panel_dev;
	ili9806c_id_param.panel_dev = rda_ili9806c_spi_dev;

	return 0;

out_free_dev:

	kfree(panel_dev);
	panel_dev = NULL;

	return ret;
}

static int ili9806c_readid(void)
{
	struct spi_device *panel_dev;
	int ret = 0;
#ifdef CONFIG_PM
	struct regulator *lcd_reg;
#endif

	panel_dev = panel_find_spidev_by_name(SPI0_0);
	if(panel_dev == NULL)
		return 0;

	ili9806c_spi_readid_setup(panel_dev);

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

	rda_gouda_pre_enable_lcd(&(ili9806c_info.lcd),1);
	ili9806c_init_gpio();
	ret = ili9806c_readid_sub();
	rda_gouda_pre_enable_lcd(&(ili9806c_info.lcd),0);

#ifdef CONFIG_PM
	if ( regulator_disable(lcd_reg)< 0) {
		printk(KERN_ERR"rda-fb lcd could not be enabled!\n");
	}
#endif /* CONFIG_PM */
	return ret;
}

static int ili9806c_open(void)
{
	//ili9806c_readid();
#ifdef ILI9806C_FVGA
	panel_spi_send_cmd(0xFF); // EXTC Command Set enable register
	panel_spi_send_data(0xFF);
	panel_spi_send_data(0x98);
	panel_spi_send_data(0x16);
	
	panel_spi_send_cmd(0xBA); // SPI Interface Setting
	panel_spi_send_data(0x60);
	
	panel_spi_send_cmd(0xB6); //data enable
	panel_spi_send_data(0x20);
	
	panel_spi_send_cmd(0x3a); //format
	panel_spi_send_data(0x77);
	
	panel_spi_send_cmd(0XB0); // Interface Mode Control
	panel_spi_send_data(0x01);
	
	panel_spi_send_cmd(0xBC); // GIP 1
	panel_spi_send_data(0x03);
	panel_spi_send_data(0x0D);
	panel_spi_send_data(0x03);
	panel_spi_send_data(0x63);
	panel_spi_send_data(0x01);
	panel_spi_send_data(0x01);
	panel_spi_send_data(0x1B);
	panel_spi_send_data(0x11);
	panel_spi_send_data(0x6E);
	panel_spi_send_data(0x00);
	panel_spi_send_data(0x00);
	panel_spi_send_data(0x00);
	panel_spi_send_data(0x01);
	panel_spi_send_data(0x01);
	panel_spi_send_data(0x16);
	panel_spi_send_data(0x00);
	panel_spi_send_data(0xFF);
	panel_spi_send_data(0XF2);
	
	panel_spi_send_cmd(0xBD); // GIP 2
	panel_spi_send_data(0x02);
	panel_spi_send_data(0x13);
	panel_spi_send_data(0x45);
	panel_spi_send_data(0x67);
	panel_spi_send_data(0x45);
	panel_spi_send_data(0x67);
	panel_spi_send_data(0x01);
	panel_spi_send_data(0x23);
	
	panel_spi_send_cmd(0xBE); // GIP 3
	panel_spi_send_data(0x03);
	panel_spi_send_data(0x22);
	panel_spi_send_data(0x22);
	panel_spi_send_data(0x22);
	panel_spi_send_data(0x22);
	panel_spi_send_data(0xDD);
	panel_spi_send_data(0xCC);
	panel_spi_send_data(0xBB);
	panel_spi_send_data(0xAA);
	panel_spi_send_data(0x66);
	panel_spi_send_data(0x77);
	panel_spi_send_data(0x22);
	panel_spi_send_data(0x22);
	panel_spi_send_data(0x22);
	panel_spi_send_data(0x22);
	panel_spi_send_data(0x22);
	panel_spi_send_data(0x22);
	
	panel_spi_send_cmd(0xED); // en_volt_reg measure VGMP
	panel_spi_send_data(0x7F);
	panel_spi_send_data(0x0F);
	
	panel_spi_send_cmd(0xF3);
	panel_spi_send_data(0x70);
	
	panel_spi_send_cmd(0XB4); // Display Inversion Control
	panel_spi_send_data(0x02);
	
	panel_spi_send_cmd(0XC0); // Power Control 1
	panel_spi_send_data(0x0F);
	panel_spi_send_data(0x0B);
	panel_spi_send_data(0x0A);
	
	panel_spi_send_cmd(0XC1); // Power Control 2
	panel_spi_send_data(0x17);
	panel_spi_send_data(0x88);
	panel_spi_send_data(0x70);
	panel_spi_send_data(0x20);
	
	panel_spi_send_cmd(0XD8); // VGLO Selection
	panel_spi_send_data(0x50);
	
	
	panel_spi_send_cmd(0XFC); 
	panel_spi_send_data(0x07);
	
	panel_spi_send_cmd(0XE0); // Positive Gamma Control
	panel_spi_send_data(0x00);
	panel_spi_send_data(0x05);
	panel_spi_send_data(0x11);
	panel_spi_send_data(0x0D);
	panel_spi_send_data(0x0F);
	panel_spi_send_data(0x1C);
	panel_spi_send_data(0XCA);
	panel_spi_send_data(0x09);
	panel_spi_send_data(0x02);
	panel_spi_send_data(0x05);
	panel_spi_send_data(0x01);
	panel_spi_send_data(0x14);
	panel_spi_send_data(0x1B);
	panel_spi_send_data(0x2D);
	panel_spi_send_data(0x28);
	panel_spi_send_data(0x00);
	
	panel_spi_send_cmd(0XE1); // Negative Gamma Control
	panel_spi_send_data(0x00);
	panel_spi_send_data(0x00);
	panel_spi_send_data(0x00);
	panel_spi_send_data(0x07);
	panel_spi_send_data(0x0D);
	panel_spi_send_data(0x0E);
	panel_spi_send_data(0X7B);
	panel_spi_send_data(0x09);
	panel_spi_send_data(0x03);
	panel_spi_send_data(0x0B);
	panel_spi_send_data(0x0B);
	panel_spi_send_data(0x0B);
	panel_spi_send_data(0x06);
	panel_spi_send_data(0x2E);
	panel_spi_send_data(0x29);
	panel_spi_send_data(0x00);
	
	panel_spi_send_cmd(0XD5); // Source Timing Adjust
	panel_spi_send_data(0x0D);
	panel_spi_send_data(0x08);
	panel_spi_send_data(0x08);
	panel_spi_send_data(0x09);
	panel_spi_send_data(0xCB);
	panel_spi_send_data(0XA5);
	panel_spi_send_data(0x01);
	panel_spi_send_data(0x04);
	
	panel_spi_send_cmd(0XF7); // Resolution
	panel_spi_send_data(0x89);
	
	panel_spi_send_cmd(0XC7); // Vcom
	panel_spi_send_data(0x88);//8e
	
	panel_spi_send_cmd(0X11); // Exit Sleep
	mdelay(120) ;
	panel_spi_send_cmd(0X29); // Display On

#else

	panel_spi_send_cmd(0xFF); // EXTC Command Set enable register 
	panel_spi_send_data(0xFF); 
	panel_spi_send_data(0x98); 
	panel_spi_send_data(0x06); 

	panel_spi_send_cmd(0xBA); // SPI Interface Setting 
	panel_spi_send_data(0x60); 
	panel_spi_send_cmd(0xBC); // GIP 1 
	panel_spi_send_data(0x01); 
	panel_spi_send_data(0x0E); 
	panel_spi_send_data(0x61); 
	panel_spi_send_data(0xFF); 
	panel_spi_send_data(0x01); 
	panel_spi_send_data(0x01); 
	panel_spi_send_data(0x1B); 
	panel_spi_send_data(0x10); 
	panel_spi_send_data(0x3B); 
	panel_spi_send_data(0x63); 
	panel_spi_send_data(0xFF); 
	panel_spi_send_data(0xFF); 
	panel_spi_send_data(0x05); 
	panel_spi_send_data(0x05); 
	panel_spi_send_data(0x02); 
	panel_spi_send_data(0x00); 
	panel_spi_send_data(0x55); 
	panel_spi_send_data(0XD0); 
	panel_spi_send_data(0x01); 
	panel_spi_send_data(0x00); 
	panel_spi_send_data(0X40); 

	panel_spi_send_cmd(0xBD); // GIP 2 
	panel_spi_send_data(0x01); 
	panel_spi_send_data(0x23); 
	panel_spi_send_data(0x45); 
	panel_spi_send_data(0x67); 
	panel_spi_send_data(0x01); 
	panel_spi_send_data(0x23); 
	panel_spi_send_data(0x45); 
	panel_spi_send_data(0x67); 

	panel_spi_send_cmd(0xBE); // GIP 3 
	panel_spi_send_data(0x01); 
	panel_spi_send_data(0x2D); 
	panel_spi_send_data(0xCB); 
	panel_spi_send_data(0xA2); 
	panel_spi_send_data(0x62); 
	panel_spi_send_data(0xF2); 
	panel_spi_send_data(0xE2); 
	panel_spi_send_data(0x22); 
	panel_spi_send_data(0x22); 

	panel_spi_send_cmd(0xC7); 
	panel_spi_send_data(0x75); 

	panel_spi_send_cmd(0xED); 
	panel_spi_send_data(0x7F); 
	panel_spi_send_data(0x0F); 
	panel_spi_send_data(0x00); 

	panel_spi_send_cmd(0XC0); // Power Control 1 
	panel_spi_send_data(0x03); 
	panel_spi_send_data(0x0B); 
	panel_spi_send_data(0x00); 

	panel_spi_send_cmd(0XFC); 
	panel_spi_send_data(0x08); 

	panel_spi_send_cmd(0XDF); 
	panel_spi_send_data(0x00); 
	panel_spi_send_data(0x00);
	panel_spi_send_data(0x00);
	panel_spi_send_data(0x00);
	panel_spi_send_data(0x00);
	panel_spi_send_data(0x20);

	panel_spi_send_cmd(0XF3); 
	panel_spi_send_data(0x74); 

	panel_spi_send_cmd(0XF9); 
	panel_spi_send_data(0x00); 
	panel_spi_send_data(0xFD); 
	panel_spi_send_data(0x80); 
	panel_spi_send_data(0xC0); 

	panel_spi_send_cmd(0XB4); // Display Inversion Control 
	panel_spi_send_data(0x02); 
	panel_spi_send_data(0x02);
	panel_spi_send_data(0x02);

	panel_spi_send_cmd(0XF7); 
	panel_spi_send_data(0x82); 

	panel_spi_send_cmd(0XB1); 
	panel_spi_send_data(0x00); 
	panel_spi_send_data(0x13); 
	panel_spi_send_data(0x13); 

	panel_spi_send_cmd(0XF2); 
	panel_spi_send_data(0x80); 
	panel_spi_send_data(0x01); 
	panel_spi_send_data(0x40); 
	panel_spi_send_data(0x28); 

	panel_spi_send_cmd(0XC1); // Power Control 2 
	panel_spi_send_data(0x17); 
	panel_spi_send_data(0x86); 
	panel_spi_send_data(0xA0); 
	panel_spi_send_data(0x20); 

	panel_spi_send_cmd(0XE0); // Positive Gamma Control 
	panel_spi_send_data(0x00); 
	panel_spi_send_data(0x09); 
	panel_spi_send_data(0x15); 
	panel_spi_send_data(0x10); 
	panel_spi_send_data(0x12); 
	panel_spi_send_data(0x1B); 
	panel_spi_send_data(0XCB); 
	panel_spi_send_data(0x09); 
	panel_spi_send_data(0x02); 
	panel_spi_send_data(0x08); 
	panel_spi_send_data(0x05); 
	panel_spi_send_data(0x0E); 
	panel_spi_send_data(0x0D); 
	panel_spi_send_data(0x2F); 
	panel_spi_send_data(0x2A); 
	panel_spi_send_data(0x00); 

	panel_spi_send_cmd(0XE1); // Negative Gamma Control 
	panel_spi_send_data(0x00); 
	panel_spi_send_data(0x07); 
	panel_spi_send_data(0x13); 
	panel_spi_send_data(0x11); 
	panel_spi_send_data(0x13); 
	panel_spi_send_data(0x18); 
	panel_spi_send_data(0X7A); 
	panel_spi_send_data(0x09); 
	panel_spi_send_data(0x03); 
	panel_spi_send_data(0x08); 
	panel_spi_send_data(0x05); 
	panel_spi_send_data(0x0A); 
	panel_spi_send_data(0x0A); 
	panel_spi_send_data(0x2C); 
	panel_spi_send_data(0x27); 
	panel_spi_send_data(0x00); 

	panel_spi_send_cmd(0X3A); 
	panel_spi_send_data(0x66); 

	panel_spi_send_cmd(0X35); 
	panel_spi_send_data(0x00); 

	panel_spi_send_cmd(0XB6); // Display On
	panel_spi_send_data(0X22); // Display On

	panel_spi_send_cmd(0XB0);
	panel_spi_send_data(0X01);//00,01,--0f     00-0b  0f
	panel_spi_send_cmd(0X11); // Exit Sleep 
	mdelay(120); 
	panel_spi_send_cmd(0X29); // Display On 
#endif
	return 0;
}

static int ili9806c_sleep(void)
{
#if 0
	/*
	 * Set LCD to sleep mode if we don't control v_lcd pin.
	 * If we are using v_lcd, we'll not use this way.
	 */
	panel_spi_send_cmd(0x10);
#endif /* #if 0 */
	return 0;
}

static int ili9806c_wakeup(void)
{
	/* 
	 * Note:
	 * If we use v_lcd pin, we'll not controll lcd to sleep mode.
	 */
#if 0
	/* Exist from sleep mode */
	panel_spi_send_cmd(0x11);
#endif /* #if 0 */

	gpio_set_value(GPIO_LCD_RESET, 0);
	mdelay(1);
	gpio_set_value(GPIO_LCD_RESET, 1);
	mdelay(1);

	ili9806c_open();

	return 0;
}

static int ili9806c_set_active_win(struct gouda_rect *r)
{
	return 0;
}

static int ili9806c_set_rotation(int rotate)
{
	return 0;
}

static int ili9806c_close(void)
{
	return 0;
}

static struct rda_lcd_info ili9806c_info = {
	.ops = {
		.s_init_gpio = ili9806c_init_gpio,
		.s_open = ili9806c_open,
		.s_readid = ili9806c_readid,
		.s_active_win = ili9806c_set_active_win,
		.s_rotation = ili9806c_set_rotation,
		.s_sleep = ili9806c_sleep,
		.s_wakeup = ili9806c_wakeup,
		.s_close = ili9806c_close},
	.lcd = {
#ifdef ILI9806C_FVGA
		.width = FWVGA_LCDD_DISP_X,
		.height = FWVGA_LCDD_DISP_Y,
#else
		.width = WVGA_LCDD_DISP_X,
		.height = WVGA_LCDD_DISP_Y,
#endif
		.lcd_interface = GOUDA_LCD_IF_DPI,
		.lcd_timing = {
			       .rgb = ILI9806C_TIMING},
		.lcd_cfg = {
			    .rgb = ILI9806C_CONFIG}
		},
	.name = ILI9806C_PANEL_NAME,
};


static int rda_fb_panel_ili9806c_probe(struct spi_device *spi)
{
	struct rda_spi_panel_device *panel_dev;
	int ret = 0;

	panel_dev = kzalloc(sizeof(*panel_dev), GFP_KERNEL);
	pr_info("%s \n",__func__);
	if (panel_dev == NULL) {
		dev_err(&spi->dev, "rda_fb_panel_ili9806c, out of memory\n");
		return -ENOMEM;
	}

	spi->mode = SPI_MODE_2;
	panel_dev->spi = spi;
	panel_dev->spi_xfer_num = 0;

	dev_set_drvdata(&spi->dev, panel_dev);

	spi->bits_per_word = 9;
	spi->max_speed_hz = 500000;
	spi->controller_data = &ili9806c_spicfg;

	ret = spi_setup(spi);

	if (ret < 0) {
		printk("error spi_setup failed\n");
		goto out_free_dev;
	}

	rda_ili9806c_spi_dev = panel_dev;

	rda_fb_register_panel(&ili9806c_info);
	pr_info("%s end\n",__func__);
	dev_info(&spi->dev, "rda panel ili9806c registered\n");
	return 0;

out_free_dev:

	kfree(panel_dev);
	panel_dev = NULL;

	return ret;
}

static int rda_fb_panel_ili9806c_remove(struct spi_device *spi)
{
	struct rda_spi_panel_device *panel_dev;

	panel_dev = dev_get_drvdata(&spi->dev);

	kfree(panel_dev);
	return 0;
}

#ifdef CONFIG_PM
static int rda_fb_panel_ili9806c_suspend(struct spi_device *spi, pm_message_t mesg)
{
	return 0;
}

static int rda_fb_panel_ili9806c_resume(struct spi_device *spi)
{
	return 0;
}
#else
#define rda_fb_panel_ili9806c_suspend NULL
#define rda_fb_panel_ili9806c_resume NULL
#endif /* CONFIG_PM */

/* The name is specific for each panel, it should be different from the
   abstract name "rda-fb-panel" */
static struct spi_driver rda_fb_panel_ili9806c_driver = {
	.driver = {
		   .name = ILI9806C_PANEL_NAME,
		   .owner = THIS_MODULE,
		   },
	.probe = rda_fb_panel_ili9806c_probe,
	.remove = rda_fb_panel_ili9806c_remove,
	.suspend = rda_fb_panel_ili9806c_suspend,
	.resume = rda_fb_panel_ili9806c_resume,
};

static struct rda_panel_driver ili9806c_mcu_panel_driver = {
	.panel_type = GOUDA_LCD_IF_DPI,
	.lcd_driver_info = &ili9806c_info,
	.rgb_panel_driver = &rda_fb_panel_ili9806c_driver,
};
