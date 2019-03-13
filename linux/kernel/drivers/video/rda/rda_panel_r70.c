#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/jiffies.h>
#include <linux/gpio.h>
#include <linux/io.h>
#include <linux/init.h>

#include <plat/devices.h>
#include <plat/rda_debug.h>
#include <mach/board.h>
#include <linux/regulator/consumer.h>
#include <mach/regulator.h>

#include "rda_panel.h"



static struct rda_spi_panel_device *rda_r70_spi_dev= NULL;
static struct regulator *g_bl_reg = NULL;

#define rda_spi_dev rda_r70_spi_dev


RDA_SPI_PARAMETERS r70_spicfg = {
	.inputEn = true,
	.clkDelay = RDA_SPI_HALF_CLK_PERIOD_0,
	.doDelay = RDA_SPI_HALF_CLK_PERIOD_0,
	.diDelay = RDA_SPI_HALF_CLK_PERIOD_1,
	.csDelay = RDA_SPI_HALF_CLK_PERIOD_0,
	.csPulse = RDA_SPI_HALF_CLK_PERIOD_0,
	.frameSize = 9,
	.oeRatio = 0,
	.rxTrigger = RDA_SPI_RX_TRIGGER_4_BYTE,
	.txTrigger = RDA_SPI_TX_TRIGGER_1_EMPTY,
	.rxMode = RDA_SPI_DIRECT_POLLING,
	.txMode = RDA_SPI_DIRECT_POLLING,
	.mask = {0, 0, 0, 0, 0},
	.handler = NULL
};

/* ************************************************
   PLEASE NOTES:
	clk_divider: 0 mean, auto-cal by driver
		     non-zero, customer can fine tune.

	example:
		GOUDA_CLK == 400MHz
		clk_divider = 14
	while by auto-cal, clk_divider = 13
   *********************************************** */

#define R70_TIMING			\
	{				\
	.lcd_freq = 33000000, /* 30MHz */\
	.clk_divider = 13,		\
	.height = WVGA_LCDD_DISP_X,	\
	.width = WVGA_LCDD_DISP_Y,	\
	.h_low=30,			\
	.v_low =13,			\
	.h_back_porch = 46,		\
	.h_front_porch = 230,		\
	.v_back_porch = 23,		\
	.v_front_porch = 12,		\
	.dot_clk_pol = true,		\
	}                               \

#define R70_CONFIG                 \
	{				\
	.frame1 = 1,			\
	.rgb_format = LCD_DATA_WIDTH,	\
	.pix_fmt = PIX_FMT,		\
	}

static int r70_init_gpio(void)
{
	gpio_request(GPIO_LCD_RESET, "lcd reset");
	gpio_direction_output(GPIO_LCD_RESET, 1);
#ifdef GPIO_LCD_PWR
	gpio_request(GPIO_LCD_PWR, "lcd power");
	gpio_direction_output(GPIO_LCD_PWR, 1);
#endif
#ifdef GPIO_LCD_EN
	gpio_request(GPIO_LCD_EN, "lcd en");
	gpio_direction_output(GPIO_LCD_EN, 1);
#endif
#ifdef GPIO_LCD_PWR
	gpio_set_value(GPIO_LCD_PWR,    1);
#endif
#ifdef GPIO_LCD_EN
	gpio_set_value(GPIO_LCD_EN,     1);
#endif

	mdelay(1);
	gpio_set_value(GPIO_LCD_RESET, 0);
	mdelay(1);
	gpio_set_value(GPIO_LCD_RESET, 1);

	mdelay(1);

	return 0;
}


static int r70_open(void)
{

	return 0;
}

static int r70_sleep(void)
{
#if 0
	/*
	 * Set LCD to sleep mode if we don't control v_lcd pin.
	 * If we are using v_lcd, we'll not use this way.
	 */
	panel_spi_send_cmd(0x10);
#endif /* #if 0 */
#ifdef GPIO_LCD_PWR
	gpio_set_value(GPIO_LCD_PWR,    0);
#endif
#ifdef GPIO_LCD_EN
	gpio_set_value(GPIO_LCD_EN,     0);
#endif
	if(g_bl_reg)
		regulator_disable(g_bl_reg);

	return 0;
}

static int r70_wakeup(void)
{
	int ret = 0;

	/* 
	 * Note:
	 * If we use v_lcd pin, we'll not controll lcd to sleep mode.
	 */
#if 0
	/* Exist from sleep mode */
	panel_spi_send_cmd(0x11);
#endif /* #if 0 */
#ifdef GPIO_LCD_PWR
	gpio_set_value(GPIO_LCD_PWR,    1);
#endif
#ifdef GPIO_LCD_EN
	gpio_set_value(GPIO_LCD_EN,     1);
#endif
	if (g_bl_reg) {
		ret = regulator_enable(g_bl_reg);
	}

	gpio_set_value(GPIO_LCD_RESET, 0);
	mdelay(1);
	gpio_set_value(GPIO_LCD_RESET, 1);
	mdelay(1);

	r70_open();

	return 0;
}

static int r70_set_active_win(struct gouda_rect *r)
{
	return 0;
}

static int r70_set_rotation(int rotate)
{
	return 0;
}

static int r70_close(void)
{
	return 0;
}


static struct rda_lcd_info r70_info = {
	.ops = {
		.s_init_gpio = r70_init_gpio,
		.s_open = r70_open,
		.s_active_win = r70_set_active_win,
		.s_rotation = r70_set_rotation,
		.s_sleep = r70_sleep,
		.s_wakeup = r70_wakeup,
		.s_close = r70_close},
	.lcd = {
		.width = WVGA_LCDD_DISP_Y,
		.height = WVGA_LCDD_DISP_X,
		.lcd_interface = GOUDA_LCD_IF_DPI,
		.lcd_timing = {
			       .rgb = R70_TIMING},
		.lcd_cfg = {
			    .rgb = R70_CONFIG}
		},
	.name = R70_PANEL_NAME,
};



static int rda_fb_panel_r70_probe(struct spi_device *spi)
{
	struct rda_spi_panel_device *panel_dev;
	int ret = 0;
	
	printk("rda_fb_panel_r70_probe\n");
	panel_dev = kzalloc(sizeof(*panel_dev), GFP_KERNEL);

	if (panel_dev == NULL) {
		dev_err(&spi->dev, "rda_fb_panel_r70, out of memory\n");
		return -ENOMEM;
	}

	spi->mode = SPI_MODE_2;
	panel_dev->spi = spi;
	panel_dev->spi_xfer_num = 0;


	dev_set_drvdata(&spi->dev, panel_dev);

	spi->bits_per_word = 9;
	spi->max_speed_hz = 500000;
	spi->controller_data = &r70_spicfg;

	ret = spi_setup(spi);

	if (ret < 0) {
		printk("error spi_setup failed\n");
		goto out_free_dev;
	}

	g_bl_reg = regulator_get(NULL, LDO_BACKLIGHT);
	if (IS_ERR(g_bl_reg)) {
		printk("lcd r70 : could not find regulator devices\n");
		ret = PTR_ERR(g_bl_reg);
		goto out_free_dev;
	}
	regulator_set_voltage(g_bl_reg, 2560000, 2560000);
	ret = regulator_enable(g_bl_reg);

	rda_r70_spi_dev = panel_dev;

	rda_fb_register_panel(&r70_info);


	dev_info(&spi->dev, "rda panel r70 registered\n");
	return 0;

out_free_dev:

	kfree(panel_dev);
	panel_dev = NULL;

	return ret;
}

static int rda_fb_panel_r70_remove(struct spi_device *spi)
{
	struct rda_spi_panel_device *panel_dev;
	panel_dev = dev_get_drvdata(&spi->dev);

	if(g_bl_reg) {
		regulator_disable(g_bl_reg);
		regulator_put(g_bl_reg);
	}

	kfree(panel_dev);
	return 0;
}

#ifdef CONFIG_PM
static int rda_fb_panel_r70_suspend(struct spi_device *spi, pm_message_t mesg)
{
	return 0;
}

static int rda_fb_panel_r70_resume(struct spi_device *spi)
{
	return 0;
}
#else
#define rda_fb_panel_r70_suspend NULL
#define rda_fb_panel_r70_resume NULL
#endif /* CONFIG_PM */

/* The name is specific for each panel, it should be different from the
   abstract name "rda-fb-panel" */
static struct spi_driver rda_fb_panel_r70_driver = {
	.driver = {
		   .name = R70_PANEL_NAME,
		   .owner = THIS_MODULE,
		   },
	.probe = rda_fb_panel_r70_probe,
	.remove = rda_fb_panel_r70_remove,
	.suspend = rda_fb_panel_r70_suspend,
	.resume = rda_fb_panel_r70_resume,
};

static struct rda_panel_driver r70_mcu_panel_driver = {
	.panel_type = GOUDA_LCD_IF_DPI,
	.lcd_driver_info = &r70_info,
	.rgb_panel_driver = &rda_fb_panel_r70_driver,
};
