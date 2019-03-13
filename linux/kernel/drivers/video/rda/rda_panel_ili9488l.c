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



static struct rda_spi_panel_device *rda_ili9488l_spi_dev = NULL;

#define rda_spi_dev rda_ili9488l_spi_dev

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

RDA_SPI_PARAMETERS ili9488l_spicfg = {
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

#define ILI9488L_TIMING			\
	{				\
	.lcd_freq = 15000000, /* 15MHz */	\
	.clk_divider = 0,			\
	.height = HVGA_LCDD_DISP_Y,			\
	.width = HVGA_LCDD_DISP_X,			\
	.h_low=5,			\
	.v_low =5,			\
	.h_back_porch = 15,		\
	.h_front_porch = 164,	\
	.v_back_porch = 7,		\
	.v_front_porch = 7,		\
	}                     

#define ILI9488L_CONFIG                     \
	{				\
	.frame1 =1,			\
	.rgb_format = RGB_IS_16BIT,	\
	}

static int ili9488l_init_gpio(void)
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

static void ili9488l_readid(void)
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

static int ili9488l_open(void)
{
	ili9488l_readid();

	/*ill9488 Alex.Zhou*/
	panel_spi_send_cmd(0xE0); 
	panel_spi_send_data(0x00); 
	panel_spi_send_data(0x01); 
	panel_spi_send_data(0x02); 
	panel_spi_send_data(0x04); 
	panel_spi_send_data(0x14); 
	panel_spi_send_data(0x09); 
	panel_spi_send_data(0x3F); 
	panel_spi_send_data(0x57); 
	panel_spi_send_data(0x4D); 
	panel_spi_send_data(0x05); 
	panel_spi_send_data(0x0B); 
	panel_spi_send_data(0x09); 
	panel_spi_send_data(0x1A); 
	panel_spi_send_data(0x1D); 
	panel_spi_send_data(0x0F);  
	 
	panel_spi_send_cmd(0xE1); 
	panel_spi_send_data(0x00); 
	panel_spi_send_data(0x1D); 
	panel_spi_send_data(0x20); 
	panel_spi_send_data(0x02); 
	panel_spi_send_data(0x0E); 
	panel_spi_send_data(0x03); 
	panel_spi_send_data(0x35); 
	panel_spi_send_data(0x12); 
	panel_spi_send_data(0x47); 
	panel_spi_send_data(0x02); 
	panel_spi_send_data(0x0D); 
	panel_spi_send_data(0x0C); 
	panel_spi_send_data(0x38); 
	panel_spi_send_data(0x39); 
	panel_spi_send_data(0x0F); 

	panel_spi_send_cmd(0xC0); 
	panel_spi_send_data(0x18); 
	panel_spi_send_data(0x16); 
	 
	panel_spi_send_cmd(0xC1); 
	panel_spi_send_data(0x41); 

	panel_spi_send_cmd(0xC5); 
	panel_spi_send_data(0x00); 
	panel_spi_send_data(0x21); 
	panel_spi_send_data(0x80); 

	panel_spi_send_cmd(0x36); 
	panel_spi_send_data(0x08); 

	panel_spi_send_cmd(0x3A); //Interface Mode Control
	panel_spi_send_data(0x66);
	//panel_spi_send_data(0x55);


	panel_spi_send_cmd(0XB0);  //Interface Mode Control  
	panel_spi_send_data(0x00); /*Alex.Zhou*/
	//panel_spi_send_data(0x0C); 
	panel_spi_send_cmd(0xB1);   //Frame rate 70HZ  
	panel_spi_send_data(0xB0); 

	panel_spi_send_cmd(0xB4); 
	panel_spi_send_data(0x02);   

	panel_spi_send_cmd(0xB6); //RGB/MCU Interface Control
	//panel_spi_send_data(0x02); /*Alex.Zhou*/
	panel_spi_send_data(0xB2);
	//panel_spi_send_data(0x62);
	panel_spi_send_data(0x22); 

	panel_spi_send_cmd(0xE9); 
	panel_spi_send_data(0x00);
	 
	panel_spi_send_cmd(0XF7);    
	panel_spi_send_data(0xA9); 
	panel_spi_send_data(0x51); 
	panel_spi_send_data(0x2C); 
	panel_spi_send_data(0x82);

	panel_spi_send_cmd(0x11); 
	mdelay(120); 
	panel_spi_send_cmd(0x29); 

	return 0;
}

static int ili9488l_sleep(void)
{
	/* Set LCD to sleep mode */
	panel_spi_send_cmd(0x10);

	return 0;
}

static int ili9488l_wakeup(void)
{
	/* Exist from sleep mode */
	panel_spi_send_cmd(0x11);

	return 0;
}

static int ili9488l_set_active_win(struct gouda_rect *r)
{
	return 0;
}

static int ili9488l_set_rotation(int rotate)
{
	return 0;
}

static int ili9488l_close(void)
{
	return 0;
}

static struct rda_lcd_info ili9488l_info = {
	.ops = {
		.s_init_gpio = ili9488l_init_gpio,
		.s_open = ili9488l_open,
		.s_active_win = ili9488l_set_active_win,
		.s_rotation = ili9488l_set_rotation,
		.s_sleep = ili9488l_sleep,
		.s_wakeup = ili9488l_wakeup,
		.s_close = ili9488l_close},
	.lcd = {
		.width = HVGA_LCDD_DISP_X,
		.height = HVGA_LCDD_DISP_Y,
		.lcd_interface = GOUDA_LCD_IF_DPI,
		.lcd_timing = {
			       .rgb = ILI9488L_TIMING},
		.lcd_cfg = {
			    .rgb = ILI9488L_CONFIG}
		},
	.name = ILI9488L_PANEL_NAME,
};


static int rda_fb_panel_ili9488l_probe(struct spi_device *spi)
{
	struct rda_spi_panel_device *panel_dev;
	int ret = 0;

	panel_dev = kzalloc(sizeof(*panel_dev), GFP_KERNEL);

	if (panel_dev == NULL) {
		dev_err(&spi->dev, "rda_fb_panel_ili9488l, out of memory\n");
		return -ENOMEM;
	}

	spi->mode = SPI_MODE_2;
	panel_dev->spi = spi;
	panel_dev->spi_xfer_num = 0;


	dev_set_drvdata(&spi->dev, panel_dev);

	spi->bits_per_word = 9;
	spi->max_speed_hz = 500000;
	spi->controller_data = &ili9488l_spicfg;

	ret = spi_setup(spi);

	if (ret < 0) {
		printk("error spi_setup failed\n");
		goto out_free_dev;
	}

	rda_ili9488l_spi_dev = panel_dev;

	rda_fb_register_panel(&ili9488l_info);

	dev_info(&spi->dev, "rda panel ili9488l registered\n");
	return 0;

out_free_dev:

	kfree(panel_dev);
	panel_dev = NULL;

	return ret;
}

static int rda_fb_panel_ili9488l_remove(struct spi_device *spi)
{
	struct rda_spi_panel_device *panel_dev;

	panel_dev = dev_get_drvdata(&spi->dev);

	kfree(panel_dev);
	return 0;
}

#ifdef CONFIG_PM
static int rda_fb_panel_ili9488l_suspend(struct spi_device *spi, pm_message_t mesg)
{
	return 0;
}

static int rda_fb_panel_ili9488l_resume(struct spi_device *spi)
{
	return 0;
}
#else
#define rda_fb_panel_ili9488l_suspend NULL
#define rda_fb_panel_ili9488l_resume NULL
#endif /* CONFIG_PM */

static struct spi_driver rda_fb_panel_ili9488l_driver = {
	.driver = {
		   .name = ILI9488L_PANEL_NAME,
		   .owner = THIS_MODULE,
		   },
	.probe = rda_fb_panel_ili9488l_probe,
	.remove = rda_fb_panel_ili9488l_remove,
	.suspend = rda_fb_panel_ili9488l_suspend,
	.resume = rda_fb_panel_ili9488l_resume,
};

static struct rda_panel_driver ili9488l_mcu_panel_driver = {
	.panel_type = GOUDA_LCD_IF_DPI,
	.lcd_driver_info = &ili9488l_info,
	.rgb_panel_driver = &rda_fb_panel_ili9488l_driver,
};
