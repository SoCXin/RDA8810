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


static struct rda_spi_panel_device *rda_jb070sz03a_spi_dev = NULL;

#define rda_spi_dev  rda_jb070sz03a_spi_dev

RDA_SPI_PARAMETERS JB070SZ03A_spicfg = {
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


#define JB070SZ03A_TIMING			\
	{				\
	.lcd_freq=33000000,	\
	.clk_divider = 0,		\
	.height = WVGA_LCDD_DISP_X,			\
	.width = WGVA_LCDD_DISP_Y,			\
	.h_low=30,			\
	.v_low =13,			\
	.h_back_porch = 88,		\
	.h_front_porch = 40,		\
	.v_back_porch = 32,		\
	.v_front_porch = 13,		\
	.dot_clk_pol = true,		\
	}                               \

#define  JB070SZ03A_CONFIG               \
	{				\
	.frame1 =1,			\
	.rgb_format = RGB_IS_24BIT,	\
	.pix_fmt = RDA_FMT_RGB565,	\
	}
void dummy_backlight(void);
static int jb070sz03a_init_gpio(void)
{
#ifdef GPIO_LCD_PWR
	gpio_request(GPIO_LCD_PWR, "lcd power");
	gpio_direction_output(GPIO_LCD_PWR, 1);
	mdelay(1);
	gpio_set_value(GPIO_LCD_PWR, 1);
#endif

	gpio_request(GPIO_LCD_RESET, "lcd reset");
	gpio_direction_output(GPIO_LCD_RESET, 1);
	mdelay(1);
	gpio_set_value(GPIO_LCD_RESET, 0);
	mdelay(2);
	gpio_set_value(GPIO_LCD_RESET, 1);
	mdelay(10);

	return 0;
}


static int jb070sz03a_open(void)
{

	return 0;
}

static int jb070sz03a_sleep(void)
{
#ifdef GPIO_LCD_PWR
	gpio_set_value(GPIO_LCD_PWR, 0);
#endif
	return 0;
}

static int jb070sz03a_wakeup(void)
{
#ifdef GPIO_LCD_PWR
	gpio_set_value(GPIO_LCD_PWR, 0);
	mdelay(1);

	gpio_set_value(GPIO_LCD_PWR, 1);
#endif
	return 0;
}

static int jb070sz03a_set_active_win(struct gouda_rect *r)
{
	return 0;
}

static int jb070sz03a_set_rotation(int rotate)
{
	return 0;
}

static int jb070sz03a_close(void)
{
	return 0;
}


static struct rda_lcd_info jb070sz03a_info = {
	.ops = {
		.s_init_gpio = jb070sz03a_init_gpio,
		.s_open = jb070sz03a_open,
		.s_active_win = jb070sz03a_set_active_win,
		.s_rotation = jb070sz03a_set_rotation,
		.s_sleep = jb070sz03a_sleep,
		.s_wakeup = jb070sz03a_wakeup,
		.s_close = jb070sz03a_close},
	.lcd = {
		.width = WVGA_LCDD_DISP_Y,
		.height = WVGA_LCDD_DISP_X,
		.lcd_interface = GOUDA_LCD_IF_DPI,
		.lcd_timing = {
			       .rgb = JB070SZ03A_TIMING},
		.lcd_cfg = {
			    .rgb = JB070SZ03A_CONFIG}
		},
	.name = JB070SZ03A_PANEL_NAME,
};



static int rda_fb_panel_jb070sz03a_probe(struct spi_device *spi)
{
	struct rda_spi_panel_device *panel_dev;
	int ret = 0;

	panel_dev = kzalloc(sizeof(*panel_dev), GFP_KERNEL);

	printk(KERN_ERR "wmj rda_fb_panel_jb070sz03a_probe");
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
	spi->controller_data = &JB070SZ03A_spicfg;

	ret = spi_setup(spi);

	if (ret < 0) {
		printk("error spi_setup failed\n");
		goto out_free_dev;
	}

	rda_jb070sz03a_spi_dev = panel_dev;

	rda_fb_register_panel(&jb070sz03a_info);

	dev_info(&spi->dev, "rda panel jb070sz03a registered\n");
	return 0;

out_free_dev:
	kfree(panel_dev);
	panel_dev = NULL;

	return ret;
}

static int rda_fb_panel_jb070sz03a_remove(struct spi_device *spi)
{
	struct rda_spi_panel_device *panel_dev;

	panel_dev = dev_get_drvdata(&spi->dev);

	kfree(panel_dev);
	return 0;
}

/* The name is specific for each panel, it should be different from the
   abstract name "rda-fb-panel" */
static struct spi_driver rda_fb_panel_jb070sz03a_driver = {
	.driver = {
		   .name = JB070SZ03A_PANEL_NAME,
		   .owner = THIS_MODULE,
		   },
	.probe = rda_fb_panel_jb070sz03a_probe,
	.remove = rda_fb_panel_jb070sz03a_remove,
};

static struct rda_panel_driver jb070sz03a_mcu_panel_driver = {
	.panel_type = GOUDA_LCD_IF_DPI,
	.lcd_driver_info = &jb070sz03a_info,
	.rgb_panel_driver = &rda_fb_panel_jb070sz03a_driver,
};
