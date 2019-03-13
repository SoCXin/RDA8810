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
//#define JD9161BA_FVGA

static struct rda_spi_panel_device *rda_jd9161ba_spi_dev = NULL;
static struct rda_lcd_info jd9161ba_info;
static struct rda_panel_id_param jd9161ba_id_param;

RDA_SPI_PARAMETERS jd9161ba;

#define rda_spi_dev  rda_jd9161ba_spi_dev

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

static inline int jd9161ba_readid_sub(void)
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

	panel_spi_read_id(cmd,&jd9161ba_id_param,read_id);

	if(read_id[0] == 0 && read_id[1] == 0x98 && read_id[2] == 0)
		return 1;

	return 1;
}

static struct rda_panel_id_param jd9161ba_id_param = {
	.lcd_info = &jd9161ba_info,
	.lcd_spicfg = &jd9161ba,
	.dumy_bits = 1,
	.per_read_bytes = 3,
};

RDA_SPI_PARAMETERS jd9161ba = {
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

#if 0//def JD9161BA_FVGA
#define JD9161BA_TIMING			\
	{				\
	.lcd_freq = 28000000, /* 15MHz */	\
	.clk_divider = 0,			\
	.height = JD9161BA_LCDD_DISP_FVGA_Y,	\
	.width = JD9161BA_LCDD_DISP_FVGA_X,	\
	.h_low=10,			\
	.v_low =2,			\
	.h_back_porch = 50,		\
	.h_front_porch = 200,		\
	.v_back_porch = 18,		\
	.v_front_porch = 20,		\
	}                               \

#else
//FWVGA_LCDD_DISP_Y
#define JD9161BA_TIMING			\
	{				\
	.lcd_freq = 25000000, /* 28MHz */	\
	.clk_divider = 0,			\
	.height = FWVGA_LCDD_DISP_Y,	\
	.width = FWVGA_LCDD_DISP_X,	\
	.h_low=10,			\
	.v_low =4,			\
	.h_back_porch = 10,		  /*10*/\
	.h_front_porch = 10,		 /*10*/ \
	.v_back_porch = 4,		\
	.v_front_porch = 6,		\
	}                               \

#endif

#if 0//def JD9161BA_FVGA

#define JD9161BA_CONFIG                 \
	{				\
	.frame1 =1,			\
	.rgb_format = RGB_IS_24BIT,	\
	}

#else

#define JD9161BA_CONFIG                 \
	{				\
	.frame1 =1,			\
	.rgb_format = RGB_IS_16BIT,	\
	}

#endif

static int jd9161ba_init_gpio(void)
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

static int jd9161ba_spi_readid_setup(struct spi_device *spi)
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
	spi->controller_data = &jd9161ba;

	ret = spi_setup(spi);

	if (ret < 0) {
		printk("error spi_setup failed\n");
		goto out_free_dev;
	}

	rda_jd9161ba_spi_dev = panel_dev;
	jd9161ba_id_param.panel_dev = rda_jd9161ba_spi_dev;

	return 0;

out_free_dev:

	kfree(panel_dev);
	panel_dev = NULL;

	return ret;
}

static int jd9161ba_readid(void)
{
	struct spi_device *panel_dev;
	int ret = 0;
#ifdef CONFIG_PM
	struct regulator *lcd_reg;
#endif

	panel_dev = panel_find_spidev_by_name(SPI0_0);
	if(panel_dev == NULL)
		return 0;

	jd9161ba_spi_readid_setup(panel_dev);

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

	rda_gouda_pre_enable_lcd(&(jd9161ba_info.lcd),1);
	jd9161ba_init_gpio();
	ret = jd9161ba_readid_sub();
	rda_gouda_pre_enable_lcd(&(jd9161ba_info.lcd),0);

#ifdef CONFIG_PM
	if ( regulator_disable(lcd_reg)< 0) {
		printk(KERN_ERR"rda-fb lcd could not be enabled!\n");
	}
#endif /* CONFIG_PM */
	return ret;
}

static int jd9161ba_open(void)
{
	//while (1)
	{
	printk("thomas 4 %s\n",__func__);
//-----initial BOE4.5_TN_Gamma2.2 480x854 DSI VDO -----//
panel_spi_send_cmd(0xBF);	//SET password, 91/61/F2 can access all page
panel_spi_send_data(0x91);
panel_spi_send_data(0x61);
panel_spi_send_data(0xF2);

//VCOM
panel_spi_send_cmd(0xB3);	//SET VCOM
panel_spi_send_data(0x00);
panel_spi_send_data(0x9D);

//VCOM_R
panel_spi_send_cmd(0xB4);	//SET VCOM_R
panel_spi_send_data(0x00);
panel_spi_send_data(0x9D);

//VGMP, VGSP, VGMN, VGSN
panel_spi_send_cmd(0xB8);	//VGMP, VGSP, VGMN, VGSN
panel_spi_send_data(0x00);  //
panel_spi_send_data(0x97);  //VGMP[7:0]
panel_spi_send_data(0x01);  //VGSP[7:0]
panel_spi_send_data(0x00);  //
panel_spi_send_data(0x97);  //VGMN[7:0]
panel_spi_send_data(0x01);  //VGSN[7:0]

//GIP output voltage level
panel_spi_send_cmd(0xBA);  //GIP output voltage level.
panel_spi_send_data(0x3E);  //VGH_REG[6:0]
panel_spi_send_data(0x23);  //VGL_REG[5:0]
panel_spi_send_data(0x00);  //

//column
panel_spi_send_cmd(0xC3);  //column
panel_spi_send_data(0x02);  //
/*panel_spi_send_data(0x15);
panel_spi_send_data(0x18);
panel_spi_send_data(0x82);
panel_spi_send_data(0x0C);
panel_spi_send_data(0x82);
panel_spi_send_data(0x4E);
Delayms(1);
*/
//TCON
panel_spi_send_cmd(0xC4);  //SET TCON
panel_spi_send_data(0x30);  //
panel_spi_send_data(0x6A);  //854 LINE


//POWER CTRL
panel_spi_send_cmd(0xC7);  //POWER CTRL
panel_spi_send_data(0x00);  //DCDCM[3:0]
panel_spi_send_data(0x02);  //
panel_spi_send_data(0x31);  //
panel_spi_send_data(0x08);  //
panel_spi_send_data(0x6A);  //
panel_spi_send_data(0x30);  //
panel_spi_send_data(0x13);  //
panel_spi_send_data(0xA5);  //
panel_spi_send_data(0xA5);  //

//Gamma
panel_spi_send_cmd(0xC8);  //
panel_spi_send_data(0x7F);  //
panel_spi_send_data(0x63);  //
panel_spi_send_data(0x51);  //
panel_spi_send_data(0x42);  //
panel_spi_send_data(0x3A);  //
panel_spi_send_data(0x2A);  //
panel_spi_send_data(0x2B);  //
panel_spi_send_data(0x16);  //
panel_spi_send_data(0x32);  //
panel_spi_send_data(0x34);  //
panel_spi_send_data(0x37);  //
panel_spi_send_data(0x5A);  //
panel_spi_send_data(0x4E);  //
panel_spi_send_data(0x5D);  //
panel_spi_send_data(0x56);  //
panel_spi_send_data(0x5B);  //
panel_spi_send_data(0x55);  //
panel_spi_send_data(0x49);  //
panel_spi_send_data(0x00);  //
panel_spi_send_data(0x7F);  //
panel_spi_send_data(0x63);  //
panel_spi_send_data(0x51);  //
panel_spi_send_data(0x42);  //
panel_spi_send_data(0x3A);  //
panel_spi_send_data(0x2A);  //
panel_spi_send_data(0x2B);  //
panel_spi_send_data(0x16);  //
panel_spi_send_data(0x32);  //
panel_spi_send_data(0x34);  //
panel_spi_send_data(0x37);  //
panel_spi_send_data(0x5A);  //
panel_spi_send_data(0x4E);  //
panel_spi_send_data(0x5D);  //
panel_spi_send_data(0x56);  //
panel_spi_send_data(0x5B);  //
panel_spi_send_data(0x55);  //
panel_spi_send_data(0x49);  //
panel_spi_send_data(0x00);  //

//SET GIP_L
panel_spi_send_cmd(0xD4);  //SETGIPL
panel_spi_send_data(0x1F);  //
panel_spi_send_data(0x1F);  //
panel_spi_send_data(0x1F);  //
panel_spi_send_data(0x03);  //
panel_spi_send_data(0x01);  //
panel_spi_send_data(0x05);  //
panel_spi_send_data(0x07);  //
panel_spi_send_data(0x09);  //
panel_spi_send_data(0x0B);  //
panel_spi_send_data(0x11);  //
panel_spi_send_data(0x13);  //
panel_spi_send_data(0x1F);  //
panel_spi_send_data(0x1F);  //
panel_spi_send_data(0x1F);  //
panel_spi_send_data(0x1F);  //
panel_spi_send_data(0x1F);  //


//SET GIP_R
panel_spi_send_cmd(0xD5);  //SETGIPL
panel_spi_send_data(0x1F);  //
panel_spi_send_data(0x1F);  //
panel_spi_send_data(0x1F);  //
panel_spi_send_data(0x02);  //
panel_spi_send_data(0x00);  //
panel_spi_send_data(0x04);  //
panel_spi_send_data(0x06);  //
panel_spi_send_data(0x08);  //
panel_spi_send_data(0x0A);  //
panel_spi_send_data(0x10);  //
panel_spi_send_data(0x12);  //
panel_spi_send_data(0x1F);  //
panel_spi_send_data(0x1F);  //
panel_spi_send_data(0x1F);  //
panel_spi_send_data(0x1F);  //
panel_spi_send_data(0x1F);  //


//SET GIP_GS_L
panel_spi_send_cmd(0xD6);  //SETGIPL
panel_spi_send_data(0x1F);  //
panel_spi_send_data(0x1F);  //
panel_spi_send_data(0x1F);  //
panel_spi_send_data(0x10);  //
panel_spi_send_data(0x12);  //
panel_spi_send_data(0x04);  //
panel_spi_send_data(0x0A);  //
panel_spi_send_data(0x08);  //
panel_spi_send_data(0x06);  //
panel_spi_send_data(0x02);  //
panel_spi_send_data(0x00);  //
panel_spi_send_data(0x1F);  //
panel_spi_send_data(0x1F);  //
panel_spi_send_data(0x1F);  //
panel_spi_send_data(0x1F);  //
panel_spi_send_data(0x1F);  //


//SET GIP_GS_R
panel_spi_send_cmd(0xD7);  //SETGIPL
panel_spi_send_data(0x1F);  //
panel_spi_send_data(0x1F);  //
panel_spi_send_data(0x1F);  //
panel_spi_send_data(0x11);  //
panel_spi_send_data(0x13);  //
panel_spi_send_data(0x05);  //
panel_spi_send_data(0x0B);  //
panel_spi_send_data(0x09);  //
panel_spi_send_data(0x07);  //
panel_spi_send_data(0x03);  //
panel_spi_send_data(0x01);  //
panel_spi_send_data(0x1F);  //
panel_spi_send_data(0x1F);  //
panel_spi_send_data(0x1F);  //
panel_spi_send_data(0x1F);  //
panel_spi_send_data(0x1F);  //


//SET GIP1
panel_spi_send_cmd(0xD8);  //SETGIP1
panel_spi_send_data(0x20);  //
panel_spi_send_data(0x00);  //
panel_spi_send_data(0x00);  //
panel_spi_send_data(0x30);  //
panel_spi_send_data(0x03);  //
panel_spi_send_data(0x30);  //
panel_spi_send_data(0x01);  //
panel_spi_send_data(0x02);  //
panel_spi_send_data(0x30);  //
panel_spi_send_data(0x01);  //
panel_spi_send_data(0x02);  //
panel_spi_send_data(0x06);  //
panel_spi_send_data(0x70);  //
panel_spi_send_data(0x73);  //
panel_spi_send_data(0x5D);  //
panel_spi_send_data(0x72);  //
panel_spi_send_data(0x04);  //
panel_spi_send_data(0x38);  //
panel_spi_send_data(0x70);  //
panel_spi_send_data(0x08);  //


//SET GIP2
panel_spi_send_cmd(0xD9);  // SETGIP2
panel_spi_send_data(0x00);  //
panel_spi_send_data(0x0A);  //
panel_spi_send_data(0x0A);  //
panel_spi_send_data(0x88);  //
panel_spi_send_data(0x00);  //
panel_spi_send_data(0x00);  //
panel_spi_send_data(0x06);  //
panel_spi_send_data(0x7B);  //
panel_spi_send_data(0x00);  //
panel_spi_send_data(0x80);  //
panel_spi_send_data(0x00);  //
panel_spi_send_data(0x3B);  //
panel_spi_send_data(0x33);  //
panel_spi_send_data(0x1F);  //
panel_spi_send_data(0x00);  //
panel_spi_send_data(0x00);  //
panel_spi_send_data(0x00);  //
panel_spi_send_data(0x06);  //
panel_spi_send_data(0x70);  //

panel_spi_send_cmd(0xBE);  // SETGIP2
panel_spi_send_data(0x01);  //
panel_spi_send_cmd(0xDD);  // SETGIP2
panel_spi_send_data(0x11);  //

panel_spi_send_cmd(0xBE);  // SETGIP2
panel_spi_send_data(0x00);  //
panel_spi_send_cmd(0x3A);  // SETGIP2
panel_spi_send_data(0x55);  //


	panel_spi_send_cmd(0x11);
	mdelay(120);

	panel_spi_send_cmd(0x29);
	mdelay(10);

panel_spi_send_cmd(0xBF);	//SET password, 91/61/F2 can access all page
panel_spi_send_data(0x09);
panel_spi_send_data(0xB1);
panel_spi_send_data(0x7F);
		}
	return 0;
}

static int jd9161ba_sleep(void)
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

static int jd9161ba_wakeup(void)
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

	jd9161ba_open();

	return 0;
}

static int jd9161ba_set_active_win(struct gouda_rect *r)
{
	return 0;
}

static int jd9161ba_set_rotation(int rotate)
{
	return 0;
}

static int jd9161ba_close(void)
{
	return 0;
}

static struct rda_lcd_info jd9161ba_info = {
	.ops = {
		.s_init_gpio = jd9161ba_init_gpio,
		.s_open = jd9161ba_open,
		.s_readid = jd9161ba_readid,
		.s_active_win = jd9161ba_set_active_win,
		.s_rotation = jd9161ba_set_rotation,
		.s_sleep = jd9161ba_sleep,
		.s_wakeup = jd9161ba_wakeup,
		.s_close = jd9161ba_close},
	.lcd = {
#if 0//def JD9161BA_FVGA
		.width = JD9161BA_LCDD_DISP_FVGA_X,
		.height = JD9161BA_LCDD_DISP_FVGA_Y,
#else
		.width = FWVGA_LCDD_DISP_X,
		.height = FWVGA_LCDD_DISP_Y,
#endif
		.lcd_interface = GOUDA_LCD_IF_DPI,
		.lcd_timing = {
			       .rgb = JD9161BA_TIMING},
		.lcd_cfg = {
			    .rgb = JD9161BA_CONFIG}
		},
	.name = JD9161BA_PANEL_NAME,
};


static int rda_fb_panel_jd9161ba_probe(struct spi_device *spi)
{
	struct rda_spi_panel_device *panel_dev;
	int ret = 0;

	panel_dev = kzalloc(sizeof(*panel_dev), GFP_KERNEL);
	pr_info("%s \n",__func__);
	if (panel_dev == NULL) {
		dev_err(&spi->dev, "rda_fb_panel_jd9161ba, out of memory\n");
		return -ENOMEM;
	}

	spi->mode = SPI_MODE_2;
	panel_dev->spi = spi;
	panel_dev->spi_xfer_num = 0;

	dev_set_drvdata(&spi->dev, panel_dev);

	spi->bits_per_word = 9;
	spi->max_speed_hz = 500000;
	spi->controller_data = &jd9161ba;

	ret = spi_setup(spi);

	if (ret < 0) {
		printk("error spi_setup failed\n");
		goto out_free_dev;
	}

	rda_jd9161ba_spi_dev = panel_dev;

	rda_fb_register_panel(&jd9161ba_info);
	pr_info("%s end\n",__func__);
	dev_info(&spi->dev, "rda panel jd9161ba registered\n");
	return 0;

out_free_dev:

	kfree(panel_dev);
	panel_dev = NULL;

	return ret;
}

static int rda_fb_panel_jd9161ba_remove(struct spi_device *spi)
{
	struct rda_spi_panel_device *panel_dev;

	panel_dev = dev_get_drvdata(&spi->dev);

	kfree(panel_dev);
	return 0;
}

#ifdef CONFIG_PM
static int rda_fb_panel_jd9161ba_suspend(struct spi_device *spi, pm_message_t mesg)
{
	return 0;
}

static int rda_fb_panel_jd9161ba_resume(struct spi_device *spi)
{
	return 0;
}
#else
#define rda_fb_panel_jd9161ba_suspend NULL
#define rda_fb_panel_jd9161ba_resume NULL
#endif /* CONFIG_PM */

/* The name is specific for each panel, it should be different from the
   abstract name "rda-fb-panel" */
static struct spi_driver rda_fb_panel_jd9161ba_driver = {
	.driver = {
		   .name = JD9161BA_PANEL_NAME,
		   .owner = THIS_MODULE,
		   },
	.probe = rda_fb_panel_jd9161ba_probe,
	.remove = rda_fb_panel_jd9161ba_remove,
	.suspend = rda_fb_panel_jd9161ba_suspend,
	.resume = rda_fb_panel_jd9161ba_resume,
};

static struct rda_panel_driver jd9161ba_rgb_panel_driver = {
	.panel_type = GOUDA_LCD_IF_DPI,
	.lcd_driver_info = &jd9161ba_info,
	.rgb_panel_driver = &rda_fb_panel_jd9161ba_driver,
};
