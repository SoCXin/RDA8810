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


static struct rda_spi_panel_device *rda_hx8664_spi_dev = NULL;

#define rda_spi_dev  rda_hx8664_spi_dev

static inline void panel_spi_send_data(u8 data)
{
	unsigned short AddressAndData;
	AddressAndData = ((1 << 8) | data);

	//please nots, spi write 1 slot!
	panel_spi_transfer(rda_spi_dev, (u8 *) (&AddressAndData), NULL, 1);

}

static inline void panel_spi_send_cmd(u8 cmd)
{
	u16 AddressAndData;
	AddressAndData = cmd;
	//please nots, spi write 1 slot!
	panel_spi_transfer(rda_spi_dev, (u8 *) (&AddressAndData), NULL, 1);
	mdelay(2);

}

static inline int panel_spi_read_reg(struct rda_spi_panel_device *panel_dev, u16 adrress)
{
#if 0
	u8 receivebuffer[12];
	u8 AddressAndData[2];

	/* First Transmit */
	AddressAndData[0] = 0x20;
	AddressAndData[1] = (adrress & 0xff00) >> 8;
	//rda_lcd_write(rdalcd, AddressAndData, 2);
	panel_spi_transfer(panel_dev, AddressAndData, receivebuffer, 2);

	//serial_puts(" get data 1 ");
	/*second transmit */
	AddressAndData[0] = 0x00;
	AddressAndData[1] = (adrress & 0x00ff);
	//rda_lcd_write(rdalcd, AddressAndData, 2);
	panel_spi_transfer(panel_dev, AddressAndData, receivebuffer, 2);

	/*Third transmit */
	AddressAndData[0] = 0xc0;
	AddressAndData[1] = 0xff;
	panel_spi_transfer(panel_dev, AddressAndData, receivebuffer, 2);

	return receivebuffer[1];
#endif
	return 0;
}


RDA_SPI_PARAMETERS HX8664_spicfg = {
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

#define HX8664_TIMING			\
	{				\
	.clk_divider = 4,		\
	.height = WVGA_LCDD_DISP_X,			\
	.width = WVGA_LCDD_DISP_Y,			\
	.h_low=20,			\
	.v_low =10,			\
	.h_back_porch = 100,		\
	.h_front_porch = 100,		\
	.v_back_porch = 30,		\
	.v_front_porch = 30,		\
	}                               \

#define HX8664_CONFIG                     \
	{				\
	.frame1 =1,			\
	.rgb_format =1,			\
	}

static int hx8664_init_gpio(void)
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

static void hx8664_readid(void)
{
#if 0
	u8 id = 0;

	while (1) {
		id = panel_spi_read_reg(rda_spi_dev, 0x0401);
		printk(KERN_INFO "panel_spi_test: id is 0x%x\n", id);
		if (id == 0x80) {
			break;
		}
	}
#endif
}

static int hx8664_open(void)
{
	hx8664_readid();

	mdelay(100);		// Delay 100 ms

	panel_spi_send_cmd(0x11);

	mdelay(20);

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
	mdelay(120);
	panel_spi_send_cmd(0x29);
	mdelay(20);

	return 0;
}

static int hx8664_sleep(void)
{
	/* Set LCD to sleep mode */
	//panel_spi_send_cmd(0x10);
	gpio_request(GPIO_A3, "lcd reset");
	gpio_direction_output(GPIO_A3, 1);
	mdelay(1);
	gpio_set_value(GPIO_A3, 0);
	mdelay(20);
	return 0;
}

static int hx8664_wakeup(void)
{
	/* Exist from sleep mode */
	//panel_spi_send_cmd(0x11);
	gpio_request(GPIO_A3, "lcd reset");
	gpio_direction_output(GPIO_A3, 1);
	mdelay(1);
	gpio_set_value(GPIO_A3, 1);
	mdelay(20);
	return 0;
}

static int hx8664_set_active_win(struct gouda_rect *r)
{
	return 0;
}

static int hx8664_set_rotation(int rotate)
{
	return 0;
}

static int hx8664_close(void)
{
	return 0;
}


static struct rda_lcd_info hx8664_info = {
	.ops = {
		.s_init_gpio = hx8664_init_gpio,
		.s_open = hx8664_open,
		.s_active_win = hx8664_set_active_win,
		.s_rotation = hx8664_set_rotation,
		.s_sleep = hx8664_sleep,
		.s_wakeup = hx8664_wakeup,
		.s_close = hx8664_close},
	.lcd = {
		.width = WVGA_LCDD_DISP_Y,
		.height = WVGA_LCDD_DISP_X,
		.lcd_interface = GOUDA_LCD_IF_DPI,
		.lcd_timing = {
			       .rgb = HX8664_TIMING},
		.lcd_cfg = {
			    .rgb = HX8664_CONFIG}
		},
	.name = HX8664_PANEL_NAME,
};



static int rda_fb_panel_hx8664_probe(struct spi_device *spi)
{
	struct rda_spi_panel_device *panel_dev;
	int ret = 0;

	panel_dev = kzalloc(sizeof(*panel_dev), GFP_KERNEL);

	if (panel_dev == NULL) {
		dev_err(&spi->dev, "rda_fb_panel_hx8664, out of memory\n");
		return -ENOMEM;
	}

	spi->mode = SPI_MODE_2;
	panel_dev->spi = spi;
	panel_dev->spi_xfer_num = 0;

	dev_set_drvdata(&spi->dev, panel_dev);

	spi->bits_per_word = 9;
	spi->max_speed_hz = 500000;
	spi->controller_data = &HX8664_spicfg;

	ret = spi_setup(spi);

	if (ret < 0) {
		printk("error spi_setup failed\n");
		goto out_free_dev;
	}

	rda_hx8664_spi_dev = panel_dev;

	rda_fb_register_panel(&hx8664_info);

	dev_info(&spi->dev, "rda panel hx8664 registered\n");
	return 0;

out_free_dev:

	kfree(panel_dev);
	panel_dev = NULL;

	return ret;
}

static int rda_fb_panel_hx8664_remove(struct spi_device *spi)
{
	struct rda_spi_panel_device *panel_dev;

	panel_dev = dev_get_drvdata(&spi->dev);

	kfree(panel_dev);
	return 0;
}

#ifdef CONFIG_PM
static int rda_fb_panel_hx8664_suspend(struct spi_device *spi, pm_message_t mesg)
{
	return 0;
}

static int rda_fb_panel_hx8664_resume(struct spi_device *spi)
{
	return 0;
}
#else
#define rda_fb_panel_hx8664_suspend NULL
#define rda_fb_panel_hx8664_resume NULL
#endif /* CONFIG_PM */

/* The name is specific for each panel, it should be different from the
   abstract name "rda-fb-panel" */
static struct spi_driver rda_fb_panel_hx8664_driver = {
	.driver = {
		   .name = HX8664_PANEL_NAME,
		   .owner = THIS_MODULE,
		   },
	.probe = rda_fb_panel_hx8664_probe,
	.remove = rda_fb_panel_hx8664_remove,
	.suspend = rda_fb_panel_hx8664_suspend,
	.resume = rda_fb_panel_hx8664_resume,
};

static struct rda_panel_driver hx8664_mcu_panel_driver = {
	.panel_type = GOUDA_LCD_IF_DPI,
	.lcd_driver_info = &hx8664_info,
	.rgb_panel_driver = &rda_fb_panel_hx8664_driver,
};
