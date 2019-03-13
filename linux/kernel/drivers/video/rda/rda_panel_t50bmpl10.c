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

#include "rda_panel.h"


static struct rda_spi_panel_device *rda_t50bmpl10_spi_dev = NULL;

#define rda_spi_dev  rda_t50bmpl10_spi_dev



RDA_SPI_PARAMETERS T50BMPL10_spicfg = {
	.inputEn = true,
	.clkDelay = RDA_SPI_HALF_CLK_PERIOD_0,
	.doDelay = RDA_SPI_HALF_CLK_PERIOD_0,
	.diDelay = RDA_SPI_HALF_CLK_PERIOD_1,
	.csDelay = RDA_SPI_HALF_CLK_PERIOD_0,
	.csPulse = RDA_SPI_HALF_CLK_PERIOD_0,
	.frameSize = 8, //9
	.oeRatio = 0,
	.rxTrigger = RDA_SPI_RX_TRIGGER_4_BYTE,
	.txTrigger = RDA_SPI_TX_TRIGGER_1_EMPTY,
	.rxMode = RDA_SPI_DIRECT_POLLING,
	.txMode = RDA_SPI_DIRECT_POLLING,
	.mask = {0, 0, 0, 0, 0},
	.handler = NULL
};


#define T50BMPL10_TIMING			\
	{				\
	.lcd_freq=33000000,	\
	.clk_divider = 0,		\
	.height = WVGA_LCDD_DISP_X,			\
	.width = WVGA_LCDD_DISP_Y,			\
	.h_low=54,			\
	.v_low =3,			\
	.h_back_porch = 82,		\
	.h_front_porch = 120,		\
	.v_back_porch = 29,		\
	.v_front_porch = 13,		\
	.dot_clk_pol = true,		\
	}                               \

#define  T50BMPL10_CONFIG               \
	{				\
	.frame1 =1,			\
	.rgb_format = LCD_DATA_WIDTH,	\
	.pix_fmt = PIX_FMT,		\
	}

static int t50bmpl10_init_gpio(void)
{
	gpio_request(GPIO_LCD_RESET, "lcd reset");
	gpio_direction_output(GPIO_LCD_RESET, 1);
	mdelay(1);
	gpio_set_value(GPIO_LCD_RESET, 0);
	mdelay(150);
	gpio_set_value(GPIO_LCD_RESET, 1);
	mdelay(150);

	return 0;
}


static int t50bmpl10_open(void)
{

	return 0;
}

static int t50bmpl10_sleep(void)
{
	return 0;
}

static int t50bmpl10_wakeup(void)
{
	return 0;
}

static int t50bmpl10_set_active_win(struct gouda_rect *r)
{
	return 0;
}

static int t50bmpl10_set_rotation(int rotate)
{
	return 0;
}

static int t50bmpl10_close(void)
{
	return 0;
}


static struct rda_lcd_info t50bmpl10_info = {
	.ops = {
		.s_init_gpio = t50bmpl10_init_gpio,
		.s_open = t50bmpl10_open,
		.s_active_win = t50bmpl10_set_active_win,
		.s_rotation = t50bmpl10_set_rotation,
		.s_sleep = t50bmpl10_sleep,
		.s_wakeup = t50bmpl10_wakeup,
		.s_close = t50bmpl10_close},
	.lcd = {
		.width = WVGA_LCDD_DISP_Y,
		.height = WVGA_LCDD_DISP_X,
		.lcd_interface = GOUDA_LCD_IF_DPI,
		.lcd_timing = {
			       .rgb = T50BMPL10_TIMING},
		.lcd_cfg = {
			    .rgb = T50BMPL10_CONFIG}
		},
	.name = T50BMPL10_PANEL_NAME,
};



static int rda_fb_panel_t50bmpl10_probe(struct spi_device *spi)
{
	struct rda_spi_panel_device *panel_dev;
	int ret = 0;

	panel_dev = kzalloc(sizeof(*panel_dev), GFP_KERNEL);

	printk(KERN_ERR "wmj rda_fb_panel_t50bmpl10_probe");
	if (panel_dev == NULL) {
		dev_err(&spi->dev, "rda_fb_panel_hx8664, out of memory\n");
		return -ENOMEM;
	}

	spi->mode = SPI_MODE_2;
	panel_dev->spi = spi;
	panel_dev->spi_xfer_num = 0;

	dev_set_drvdata(&spi->dev, panel_dev);

	spi->bits_per_word = 8; //9;
	spi->max_speed_hz = 1000000; //500000;
	spi->controller_data = &T50BMPL10_spicfg;

	ret = spi_setup(spi);

	if (ret < 0) {
		printk("error spi_setup failed\n");
		goto out_free_dev;
	}

	rda_t50bmpl10_spi_dev = panel_dev;

	rda_fb_register_panel(&t50bmpl10_info);

	dev_info(&spi->dev, "rda panel t50bmpl10 registered\n");
	return 0;

out_free_dev:

	kfree(panel_dev);
	panel_dev = NULL;

	return ret;
}

static int rda_fb_panel_t50bmpl10_remove(struct spi_device *spi)
{
	struct rda_spi_panel_device *panel_dev;

	panel_dev = dev_get_drvdata(&spi->dev);

	kfree(panel_dev);
	return 0;
}

#ifdef CONFIG_PM
static int rda_fb_panel_t50bmpl10_suspend(struct spi_device *spi, pm_message_t mesg)
{
	return 0;
}

static int rda_fb_panel_t50bmpl10_resume(struct spi_device *spi)
{
	return 0;
}
#else
#define rda_fb_panel_t50bmpl10_suspend NULL
#define rda_fb_panel_t50bmpl10_resume NULL
#endif /* CONFIG_PM */

/* The name is specific for each panel, it should be different from the
   abstract name "rda-fb-panel" */
static struct spi_driver rda_fb_panel_t50bmpl10_driver = {
	.driver = {
		   .name = T50BMPL10_PANEL_NAME,
		   .owner = THIS_MODULE,
		   },
	.probe = rda_fb_panel_t50bmpl10_probe,
	.remove = rda_fb_panel_t50bmpl10_remove,
	.suspend = rda_fb_panel_t50bmpl10_suspend,
	.resume = rda_fb_panel_t50bmpl10_resume,
};

static struct rda_panel_driver t50bmpl10_mcu_panel_driver = {
	.panel_type = GOUDA_LCD_IF_DPI,
	.lcd_driver_info = &t50bmpl10_info,
	.rgb_panel_driver = &rda_fb_panel_t50bmpl10_driver,
};
