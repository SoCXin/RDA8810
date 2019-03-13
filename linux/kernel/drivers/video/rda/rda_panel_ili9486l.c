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


static struct rda_spi_panel_device *rda_ili9486l_spi_dev= NULL;
static struct rda_lcd_info ili9486l_info;
static struct rda_panel_id_param ili9486l_id_param;
RDA_SPI_PARAMETERS ili9486l_spicfg;

#define rda_spi_dev rda_ili9486l_spi_dev

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
}

static int ili9486l_readid_sub(void)
{
	u32 cmd = 0xd3;
	u16 read_id[3] = {0};

	panel_spi_send_cmd(0xB0);
	panel_spi_send_data(0x80);

	panel_spi_send_cmd(0xFB);
	panel_spi_send_data(0x11);
	panel_spi_read_id(cmd,&ili9486l_id_param,read_id);

	panel_spi_send_cmd(0xFB);
	panel_spi_send_data(0x12);
	panel_spi_read_id(cmd,&ili9486l_id_param,&read_id[1]);

	panel_spi_send_cmd(0xFB);
	panel_spi_send_data(0x13);
	panel_spi_read_id(cmd,&ili9486l_id_param,&read_id[2]);

	panel_spi_send_cmd(0xFB);
	panel_spi_send_data(0x00);

	printk("id 0x%x 0x%x 0x%x\n",read_id[0],read_id[1],read_id[2]);
	if(read_id[0] == 0 && read_id[1] == 0x94 && read_id[2] == 0x86)
		return 1;
	return 0;
}

static struct rda_panel_id_param ili9486l_id_param = {
	.lcd_info = &ili9486l_info,
	.lcd_spicfg = &ili9486l_spicfg,
	.dumy_bits = 0,
	.per_read_bytes = 1,
};

RDA_SPI_PARAMETERS ili9486l_spicfg = {
	.inputEn = true,
	.clkDelay = RDA_SPI_HALF_CLK_PERIOD_0,
	.doDelay = RDA_SPI_HALF_CLK_PERIOD_0,
	.diDelay = RDA_SPI_HALF_CLK_PERIOD_1,
	.csDelay = RDA_SPI_HALF_CLK_PERIOD_0,
	.csPulse = RDA_SPI_HALF_CLK_PERIOD_0,
	.frameSize = 9,
	.oeRatio = 9,
	.rxTrigger = RDA_SPI_RX_TRIGGER_1_BYTE,
	.txTrigger = RDA_SPI_TX_TRIGGER_1_EMPTY,
	.rxMode = RDA_SPI_DIRECT_POLLING,
	.txMode = RDA_SPI_DIRECT_POLLING,
	.mask = {0, 0, 0, 0, 0},
	.handler = NULL,
	.spi_read_bits = 0
};

#define ILI9486L_TIMING			\
	{				\
	.lcd_freq = 15000000, /* 15MHz */	\
	.clk_divider = 0,			\
	.height = HVGA_LCDD_DISP_Y,			\
	.width = HVGA_LCDD_DISP_X,			\
	.h_low=5,			\
	.v_low =5,			\
	.h_back_porch = 100,		\
	.h_front_porch = 100,		\
	.v_back_porch = 50,		\
	.v_front_porch = 50,		\
	}                               \

#define ILI9486L_CONFIG                 \
	{				\
	.frame1 =1,			\
	.rgb_format = RGB_IS_16BIT,	\
	}

static int ili9486l_init_gpio(void)
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

static int ili9486l_spi_readid_setup(struct spi_device *spi)
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
	spi->controller_data = &ili9486l_spicfg;

	ret = spi_setup(spi);

	if (ret < 0) {
		printk("error spi_setup failed\n");
		goto out_free_dev;
	}

	rda_ili9486l_spi_dev = panel_dev;
	ili9486l_id_param.panel_dev = rda_ili9486l_spi_dev;

	return 0;

out_free_dev:

	kfree(panel_dev);
	panel_dev = NULL;

	return ret;
}

static int ili9486l_readid(void)
{
	struct spi_device *panel_dev;
	int ret = 0;
#ifdef CONFIG_PM
	struct regulator *lcd_reg;
#endif /* CONFIG_PM */

	panel_dev = panel_find_spidev_by_name(SPI0_0);
	if(panel_dev == NULL)
		return 0;

	ili9486l_spi_readid_setup(panel_dev);

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

	rda_gouda_pre_enable_lcd(&(ili9486l_info.lcd),1);
	ili9486l_init_gpio();
	ret = ili9486l_readid_sub();
	rda_gouda_pre_enable_lcd(&(ili9486l_info.lcd),0);

#ifdef CONFIG_PM
	if ( regulator_disable(lcd_reg)< 0) {
		printk(KERN_ERR"rda-fb lcd could not be enabled!\n");
	}
#endif /* CONFIG_PM */

	return ret;
}

static int ili9486l_open(void)
{
	//ili9486l_readid();
	panel_spi_send_cmd(0x11);
	mdelay(1);

	panel_spi_send_cmd(0xF2);
	panel_spi_send_data(0x18);
	panel_spi_send_data(0xA3);
	panel_spi_send_data(0x12);
	panel_spi_send_data(0x02);
	panel_spi_send_data(0XB2);
	panel_spi_send_data(0x12);
	panel_spi_send_data(0xFF);
	panel_spi_send_data(0x10);
	panel_spi_send_data(0x00);

	panel_spi_send_cmd(0xF8);
	panel_spi_send_data(0x21);
	panel_spi_send_data(0x04);

	panel_spi_send_cmd(0xF9);
	panel_spi_send_data(0x00);
	panel_spi_send_data(0x08);

	panel_spi_send_cmd(0x36);	//?
	panel_spi_send_data(0x08);	//0x88

	panel_spi_send_cmd(0xB4);
	panel_spi_send_data(0x00);

	panel_spi_send_cmd(0xB6);
	panel_spi_send_data(0x32);	//rgblcd_send_data(0x02);
	panel_spi_send_data(0x22);

	panel_spi_send_cmd(0xC1);
	panel_spi_send_data(0x41);

	panel_spi_send_cmd(0xC5);
	panel_spi_send_data(0x00);
	panel_spi_send_data(0x18);

	panel_spi_send_cmd(0xE0);
	panel_spi_send_data(0x0F);
	panel_spi_send_data(0x1F);
	panel_spi_send_data(0x1C);
	panel_spi_send_data(0x0C);
	panel_spi_send_data(0x0F);
	panel_spi_send_data(0x08);
	panel_spi_send_data(0x48);
	panel_spi_send_data(0x98);
	panel_spi_send_data(0x37);
	panel_spi_send_data(0x0A);
	panel_spi_send_data(0x13);
	panel_spi_send_data(0x04);
	panel_spi_send_data(0x11);
	panel_spi_send_data(0x0D);
	panel_spi_send_data(0x00);

	panel_spi_send_cmd(0xE1);
	panel_spi_send_data(0x0F);
	panel_spi_send_data(0x32);
	panel_spi_send_data(0x2E);
	panel_spi_send_data(0x0B);
	panel_spi_send_data(0x0D);
	panel_spi_send_data(0x05);
	panel_spi_send_data(0x47);
	panel_spi_send_data(0x75);
	panel_spi_send_data(0x37);
	panel_spi_send_data(0x06);
	panel_spi_send_data(0x10);
	panel_spi_send_data(0x03);
	panel_spi_send_data(0x24);
	panel_spi_send_data(0x20);
	panel_spi_send_data(0x00);

	panel_spi_send_cmd(0x3A);	//set rgb565 or rgb666
	panel_spi_send_data(0x66);	//0x05//CHANGE FOR MYSELF//rgblcd_send_data(0x0055);//
	panel_spi_send_cmd(0x20);	//CHANGE FOR MYSELF

	panel_spi_send_cmd(0x11);
	mdelay(1);
	panel_spi_send_cmd(0x29);

	return 0;
}

static int ili9486l_sleep(void)
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

static int ili9486l_wakeup(void)
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

	ili9486l_open();

	return 0;
}

static int ili9486l_set_active_win(struct gouda_rect *r)
{
	return 0;
}

static int ili9486l_set_rotation(int rotate)
{
	return 0;
}

static int ili9486l_close(void)
{
	return 0;
}

static struct rda_lcd_info ili9486l_info = {
	.ops = {
		.s_init_gpio = ili9486l_init_gpio,
		.s_open = ili9486l_open,
		.s_readid = ili9486l_readid,
		.s_active_win = ili9486l_set_active_win,
		.s_rotation = ili9486l_set_rotation,
		.s_sleep = ili9486l_sleep,
		.s_wakeup = ili9486l_wakeup,
		.s_close = ili9486l_close},
	.lcd = {
		.width = HVGA_LCDD_DISP_X,
		.height = HVGA_LCDD_DISP_Y,
		.lcd_interface = GOUDA_LCD_IF_DPI,
		.lcd_timing = {
			       .rgb = ILI9486L_TIMING},
		.lcd_cfg = {
			    .rgb = ILI9486L_CONFIG}
		},
	.name = ILI9486L_PANEL_NAME,
};


static int rda_fb_panel_ili9486l_probe(struct spi_device *spi)
{
	struct rda_spi_panel_device *panel_dev;
	int ret = 0;

	panel_dev = kzalloc(sizeof(*panel_dev), GFP_KERNEL);
	pr_info("%s start \n",__func__);
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
	spi->controller_data = &ili9486l_spicfg;

	ret = spi_setup(spi);

	if (ret < 0) {
		printk("error spi_setup failed\n");
		goto out_free_dev;
	}
	rda_ili9486l_spi_dev = panel_dev;

	rda_fb_register_panel(&ili9486l_info);
	pr_info("%s end \n",__func__);
	dev_info(&spi->dev, "rda panel ili9486l registered\n");
	return 0;

out_free_dev:


	kfree(panel_dev);
	panel_dev = NULL;

	return ret;
}

static int rda_fb_panel_ili9486l_remove(struct spi_device *spi)
{
	struct rda_spi_panel_device *panel_dev;

	panel_dev = dev_get_drvdata(&spi->dev);


	kfree(panel_dev);
	return 0;
}

#ifdef CONFIG_PM
static int rda_fb_panel_ili9486l_suspend(struct spi_device *spi, pm_message_t mesg)
{
	return 0;
}

static int rda_fb_panel_ili9486l_resume(struct spi_device *spi)
{
	return 0;
}
#else
#define rda_fb_panel_ili9486l_suspend NULL
#define rda_fb_panel_ili9486l_resume NULL
#endif /* CONFIG_PM */

/* The name is specific for each panel, it should be different from the
   abstract name "rda-fb-panel" */
static struct spi_driver rda_fb_panel_ili9486l_driver = {
	.driver = {
		   .name = ILI9486L_PANEL_NAME,
		   .owner = THIS_MODULE,
		   },
	.probe = rda_fb_panel_ili9486l_probe,
	.remove = rda_fb_panel_ili9486l_remove,
	.suspend = rda_fb_panel_ili9486l_suspend,
	.resume = rda_fb_panel_ili9486l_resume,
};

static struct rda_panel_driver ili9486l_mcu_panel_driver = {
	.panel_type = GOUDA_LCD_IF_DPI,
	.lcd_driver_info = &ili9486l_info,
	.rgb_panel_driver = &rda_fb_panel_ili9486l_driver,
};
