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


static struct rda_spi_panel_device *rda_truly1p6365_spi_dev = NULL;

#define rda_spi_dev  rda_truly1p6365_spi_dev

static inline void panel_spi_send_data(u8 data)
{
	u8 AddressAndData[2];
	/* First Transmit */
	AddressAndData[0] = 0x40;
	AddressAndData[1] = (data & 0x00ff);
	panel_spi_transfer(rda_spi_dev, AddressAndData, NULL, 2);
	mdelay(1);
}

static inline void panel_spi_send_cmd(u16 cmd)
{
	u8 AddressAndData[2];

	/* First Transmit */
	AddressAndData[0] = 0x20;
	AddressAndData[1] = (cmd & 0xff00) >> 8;
	panel_spi_transfer(rda_spi_dev, AddressAndData, NULL, 2);
	udelay(10);

	/*second transmit */
	AddressAndData[0] = 0x00;
	AddressAndData[1] = (cmd & 0x00ff);
	panel_spi_transfer(rda_spi_dev, AddressAndData, NULL, 2);
	mdelay(1);
}

static inline void panel_spi_write_reg(u16 cmd, u8 data)
{
	u8 AddressAndData[2];
	/* First Transmit */
	AddressAndData[0] = 0x20;
	AddressAndData[1] = (cmd & 0xff00) >> 8;
	panel_spi_transfer(rda_spi_dev, AddressAndData, NULL, 2);

	/*second transmit */
	AddressAndData[0] = 0x00;
	AddressAndData[1] = (cmd & 0x00ff);
	panel_spi_transfer(rda_spi_dev, AddressAndData, NULL, 2);

	/*Third transmit */
	AddressAndData[0] = 0x40;
	AddressAndData[1] = data;
	panel_spi_transfer(rda_spi_dev, AddressAndData, NULL, 2);
}

static inline int panel_spi_read_reg(struct rda_spi_panel_device *panel_dev, u16 adrress)
{
	return 0;
}



RDA_SPI_PARAMETERS TRYLYLP6365_spicfg = {
	.inputEn = true,
	.clkDelay = RDA_SPI_HALF_CLK_PERIOD_1,
	.doDelay = RDA_SPI_HALF_CLK_PERIOD_1,
	.diDelay = RDA_SPI_HALF_CLK_PERIOD_1,
	.csDelay = RDA_SPI_HALF_CLK_PERIOD_1,
	.csPulse = RDA_SPI_HALF_CLK_PERIOD_0,
	.frameSize = 8,
	.oeRatio = 0,
	.rxTrigger = RDA_SPI_RX_TRIGGER_4_BYTE,
	.txTrigger = RDA_SPI_TX_TRIGGER_1_EMPTY,
	.rxMode = RDA_SPI_DIRECT_POLLING,
	.txMode = RDA_SPI_DIRECT_POLLING,
	.mask = { 0, 0, 0, 0, 0 },
	.handler = NULL
};

#define TRYLYLP6365_TIMING		\
	{				\
	.clk_divider = 9,		\
	.height = WVGA_LCDD_DISP_Y,			\
	.width = WVGA_LCDD_DISP_X,			\
	.h_low=50,			\
	.v_low =20,			\
	.h_back_porch = 50,		\
	.h_front_porch = 50,		\
	.v_back_porch = 20,		\
	.v_front_porch = 20,		\
	}                               \

#define TRYLYLP6365_CONFIG              \
	{				\
	.frame1 =1,			\
	.rgb_format = LCD_DATA_WIDTH,	\
	}

static int truly1p6365_init_gpio(void)
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

static void truly1p6365_readid(void)
{

}

static int truly1p6365_open(void)
{
	truly1p6365_readid();

	mdelay(100);        // Delay 100 ms
//	test_lcd_spi();
	panel_spi_send_cmd(0xff00);  //
	panel_spi_send_data(0x80);
	panel_spi_send_cmd(0xff01);  // enable EXTC
	panel_spi_send_data(0x09);
	panel_spi_send_cmd(0xff02);  //
	panel_spi_send_data(0x01);

	panel_spi_send_cmd(0xff80);  // enable Orise mode
	panel_spi_send_data(0x80);
	panel_spi_send_cmd(0xff81); //
	panel_spi_send_data(0x09);

	panel_spi_send_cmd(0xff03);  // enable SPI+I2C cmd2 read
	panel_spi_send_data(0x01);

	panel_spi_send_cmd(0x2100); // NW2100 ; NB2000
	panel_spi_send_data(0x00);

	panel_spi_send_cmd(0xD800); //GVDD
	panel_spi_send_data(0x77);
	panel_spi_send_cmd(0xD801);  //NGVDD
	panel_spi_send_data(0x77);

	panel_spi_send_cmd(0xC582); //REG-pump23
	panel_spi_send_data(0xA3);

	panel_spi_send_cmd(0xC181); //Frame rate 65Hz//V02
	panel_spi_send_data(0x66);

// RGB I/F setting VSYNC for OTM8018 0x0e

	panel_spi_send_cmd(0xC1a1); //external Vsync(08)  /Vsync,Hsync(0c) /Vsync,Hsync,DE(0e) //V02(0e)  / all  included clk(0f)
	panel_spi_send_data(0x08);

	panel_spi_send_cmd(0xC0a3); //pre-charge //V02
	panel_spi_send_data(0x1b);

	panel_spi_send_cmd(0xC481); //source bias //V02
	panel_spi_send_data(0x83);
	panel_spi_send_cmd(0xC487);
	panel_spi_send_data(0x00);
	panel_spi_send_cmd(0xC489);
	panel_spi_send_data(0x00);


	panel_spi_send_cmd(0xC590);  //Pump setting (3x=D6)-->(2x=96)//v02 01/11
	panel_spi_send_data(0x96);

	panel_spi_send_cmd(0xC591);  //Pump setting(VGH/VGL)
	panel_spi_send_data(0xA7);

	panel_spi_send_cmd(0xC592); //Pump45
	panel_spi_send_data(0x01); //(01)

	panel_spi_send_cmd(0xC5B1);  //DC voltage setting ;[0]GVDD output, default: 0xa8
	panel_spi_send_data(0xA9);

	//VCOMDC
	panel_spi_send_cmd(0xd900);  // VCOMDC=
	panel_spi_send_data(0x2a);

/////////////////////////////////////////////////////////////////////
//Gamma2.2
	panel_spi_send_cmd(0xe100);
	panel_spi_send_data(0x06);
	panel_spi_send_cmd(0xe101);
	panel_spi_send_data(0x0e);
	panel_spi_send_cmd(0xe102);
	panel_spi_send_data(0x16);
	panel_spi_send_cmd(0xe103);
	panel_spi_send_data(0x0f);
	panel_spi_send_cmd(0xe104);
	panel_spi_send_data(0x0a);
	panel_spi_send_cmd(0xe105);
	panel_spi_send_data(0x17);
	panel_spi_send_cmd(0xe106);
	panel_spi_send_data(0x0d);
	panel_spi_send_cmd(0xe107);
	panel_spi_send_data(0x0c);
	panel_spi_send_cmd(0xe108);
	panel_spi_send_data(0x01);
	panel_spi_send_cmd(0xe109);
	panel_spi_send_data(0x05);
	panel_spi_send_cmd(0xe10a);
	panel_spi_send_data(0x04);
	panel_spi_send_cmd(0xe10b);
	panel_spi_send_data(0x08);
	panel_spi_send_cmd(0xe10c);
	panel_spi_send_data(0x10);
	panel_spi_send_cmd(0xe10d);
	panel_spi_send_data(0x26);
	panel_spi_send_cmd(0xe10e);
	panel_spi_send_data(0x21);
	panel_spi_send_cmd(0xe10f);
	panel_spi_send_data(0x04);

	panel_spi_send_cmd(0xe200);
	panel_spi_send_data(0x06);
	panel_spi_send_cmd(0xe201);
	panel_spi_send_data(0x0e);
	panel_spi_send_cmd(0xe202);
	panel_spi_send_data(0x16);
	panel_spi_send_cmd(0xe203);
	panel_spi_send_data(0x0f);
	panel_spi_send_cmd(0xe204);
	panel_spi_send_data(0x0a);
	panel_spi_send_cmd(0xe205);
	panel_spi_send_data(0x17);
	panel_spi_send_cmd(0xe206);
	panel_spi_send_data(0x0d);
	panel_spi_send_cmd(0xe207);
	panel_spi_send_data(0x0c);
	panel_spi_send_cmd(0xe208);
	panel_spi_send_data(0x01);
	panel_spi_send_cmd(0xe209);
	panel_spi_send_data(0x05);
	panel_spi_send_cmd(0xe20a);
	panel_spi_send_data(0x04);
	panel_spi_send_cmd(0xe20b);
	panel_spi_send_data(0x08);
	panel_spi_send_cmd(0xe20c);
	panel_spi_send_data(0x10);
	panel_spi_send_cmd(0xe20d);
	panel_spi_send_data(0x26);
	panel_spi_send_cmd(0xe20e);
	panel_spi_send_data(0x21);
	panel_spi_send_cmd(0xe20f);
	panel_spi_send_data(0x04);

	panel_spi_send_cmd(0x0000);
	panel_spi_send_data(0x00);

	panel_spi_send_cmd(0xb3a1);
	panel_spi_send_data(0x10);

	panel_spi_send_cmd(0xb3a6); // reg_panel_zinv, reg_panel_zinv_pixel, reg_panel_zinv_odd, reg_panel_zigzag, reg_panel_zigzag_blue,
	panel_spi_send_data(0x2B); //open zigzag

	panel_spi_send_cmd(0xb3a7); //  panel_set[0] = 1
	panel_spi_send_data(0x11);


//CE8x : vst1, vst2
	panel_spi_send_cmd(0xCE80);     // ce81[7:0] : vst1_shift[7:0]
	panel_spi_send_data(0x83);
	panel_spi_send_cmd(0xCE81);     // ce82[7:0] : 0000,		vst1_width[3:0]
	panel_spi_send_data(0x01);
	panel_spi_send_cmd(0xCE82);     // ce83[7:0] : vst1_tchop[7:0]
	panel_spi_send_data(0x00);
	panel_spi_send_cmd(0xCE83);     // ce84[7:0] : vst2_shift[7:0]
	panel_spi_send_data(0x82);
	panel_spi_send_cmd(0xCE84);     // ce85[7:0] : 0000,		vst2_width[3:0]
	panel_spi_send_data(0x01);
	panel_spi_send_cmd(0xCE85);     // ce86[7:0] : vst2_tchop[7:0]
	panel_spi_send_data(0x00);


//CEAx : clka1, clka2
	panel_spi_send_cmd(0xCEa0);     // cea1[7:0] : clka1_width[3:0], clka1_shift[11:8]
	panel_spi_send_data(0x18);
	panel_spi_send_cmd(0xCEa1);     // cea2[7:0] : clka1_shift[7:0]
	panel_spi_send_data(0x01);
	panel_spi_send_cmd(0xCEa2);     // cea3[7:0] : clka1_sw_tg, odd_high, flat_head, flat_tail, switch[11:8]
	panel_spi_send_data(0x03);
	panel_spi_send_cmd(0xCEa3);     // cea4[7:0] : clka1_switch[7:0]
	panel_spi_send_data(0x22);
	panel_spi_send_cmd(0xCEa4);     // cea5[7:0] : clka1_extend[7:0]
	panel_spi_send_data(0x00);
	panel_spi_send_cmd(0xCEa5);     // cea6[7:0] : clka1_tchop[7:0]
	panel_spi_send_data(0x00);
	panel_spi_send_cmd(0xCEa6);     // cea7[7:0] : clka1_tglue[7:0]
	panel_spi_send_data(0x00);
	panel_spi_send_cmd(0xCEa7);     // cea8[7:0] : clka2_width[3:0], clka2_shift[11:8]
	panel_spi_send_data(0x18);
	panel_spi_send_cmd(0xCEa8);     // cea9[7:0] : clka2_shift[7:0]
	panel_spi_send_data(0x00);
	panel_spi_send_cmd(0xCEa9);     // ceaa[7:0] : clka2_sw_tg, odd_high, flat_head, flat_tail, switch[11:8]
	panel_spi_send_data(0x03);
	panel_spi_send_cmd(0xCEaa);     // ceab[7:0] : clka2_switch[7:0]
	panel_spi_send_data(0x23);
	panel_spi_send_cmd(0xCEab);     // ceac[7:0] : clka2_extend
	panel_spi_send_data(0x00);
	panel_spi_send_cmd(0xCEac);     // cead[7:0] : clka2_tchop
	panel_spi_send_data(0x00);
	panel_spi_send_cmd(0xCEad);     // ceae[7:0] : clka2_tglue
	panel_spi_send_data(0x00);


//CEBx : clka3, clka4
	panel_spi_send_cmd(0xCEb0);     // ceb1[7:0] : clka3_width[3:0], clka3_shift[11:8]
	panel_spi_send_data(0x10);
	panel_spi_send_cmd(0xCEb1);     // ceb2[7:0] : clka3_shift[7:0]
	panel_spi_send_data(0x00);
	panel_spi_send_cmd(0xCEb2);     // ceb3[7:0] : clka3_sw_tg, odd_high, flat_head, flat_tail, switch[11:8]
	panel_spi_send_data(0x03);
	panel_spi_send_cmd(0xCEb3);     // ceb4[7:0] : clka3_switch[7:0]
	panel_spi_send_data(0x22);
	panel_spi_send_cmd(0xCEb4);     // ceb5[7:0] : clka3_extend[7:0]
	panel_spi_send_data(0x00);
	panel_spi_send_cmd(0xCEb5);     // ceb6[7:0] : clka3_tchop[7:0]
	panel_spi_send_data(0x00);
	panel_spi_send_cmd(0xCEb6);     // ceb7[7:0] : clka3_tglue[7:0]
	panel_spi_send_data(0x00);
	panel_spi_send_cmd(0xCEb7);     // ceb8[7:0] : clka4_width[3:0], clka2_shift[11:8]
	panel_spi_send_data(0x10);
	panel_spi_send_cmd(0xCEb8);     // ceb9[7:0] : clka4_shift[7:0]
	panel_spi_send_data(0x01);
	panel_spi_send_cmd(0xCEb9);     // ceba[7:0] : clka4_sw_tg, odd_high, flat_head, flat_tail, switch[11:8]
	panel_spi_send_data(0x03);
	panel_spi_send_cmd(0xCEba);     // cebb[7:0] : clka4_switch[7:0]
	panel_spi_send_data(0x23);
	panel_spi_send_cmd(0xCEbb);     // cebc[7:0] : clka4_extend
	panel_spi_send_data(0x00);
	panel_spi_send_cmd(0xCEbc);     // cebd[7:0] : clka4_tchop
	panel_spi_send_data(0x00);
	panel_spi_send_cmd(0xCEbd);     // cebe[7:0] : clka4_tglue
	panel_spi_send_data(0x00);

// CFCx
	panel_spi_send_cmd(0xCFc7);     // cfc8[7:0] : reg_goa_gnd_opt, reg_goa_dpgm_tail_set, reg_goa_f_gating_en, reg_goa_f_odd_gating, toggle_mod1, 2, 3,
	panel_spi_send_data(0x80);

	panel_spi_send_cmd(0xcfc9);  //Gate pre-charge
	panel_spi_send_data(0x06);


//--------------------------------------------------------------------------------
//				  initial setting 3 < Panel setting >
//--------------------------------------------------------------------------------
// cbcx
	panel_spi_send_cmd(0xCBc6);     //cbc7[7:0] : enmode H-byte of sig7  (pwrof_0, pwrof_1, norm, pwron_4 )
	panel_spi_send_data(0x04);
	panel_spi_send_cmd(0xCBc7);     //cbc8[7:0] : enmode H-byte of sig8  (pwrof_0, pwrof_1, norm, pwron_4 )
	panel_spi_send_data(0x04);
	panel_spi_send_cmd(0xCBc9);     //cbca[7:0] : enmode H-byte of sig10 (pwrof_0, pwrof_1, norm, pwron_4 )
	panel_spi_send_data(0x04);

// cbdx
	panel_spi_send_cmd(0xCBdb);     //cbdc[7:0] : enmode H-byte of sig27 (pwrof_0, pwrof_1, norm, pwron_4 )
	panel_spi_send_data(0x04);
	panel_spi_send_cmd(0xCBdc);     //cbdd[7:0] : enmode H-byte of sig28 (pwrof_0, pwrof_1, norm, pwron_4 )
	panel_spi_send_data(0x04);
	panel_spi_send_cmd(0xCBde);     //cbdf[7:0] : enmode H-byte of sig30 (pwrof_0, pwrof_1, norm, pwron_4 )
	panel_spi_send_data(0x04);

// cc8x
	panel_spi_send_cmd(0xCC86);     //cc87[7:0] : reg setting for signal07 selection with u2d mode
	panel_spi_send_data(0x0c);
	panel_spi_send_cmd(0xCC87);     //cc88[7:0] : reg setting for signal08 selection with u2d mode
	panel_spi_send_data(0x0a);
	panel_spi_send_cmd(0xCC89);     //cc8a[7:0] : reg setting for signal10 selection with u2d mode
	panel_spi_send_data(0x02);

// ccax
	panel_spi_send_cmd(0xCCa1);     //cca2[7:0] : reg setting for signal27 selection with u2d mode
	panel_spi_send_data(0x0b);
	panel_spi_send_cmd(0xCCa2);     //cca3[7:0] : reg setting for signal28 selection with u2d mode
	panel_spi_send_data(0x09);
	panel_spi_send_cmd(0xCCa4);     //cca5[7:0] : reg setting for signal30 selection with u2d mode
	panel_spi_send_data(0x01);

// ccbx
	panel_spi_send_cmd(0xCCb6);     //ccb7[7:0] : reg setting for signal07 selection with d2u mode
	panel_spi_send_data(0x09);
	panel_spi_send_cmd(0xCCb7);     //ccb8[7:0] : reg setting for signal08 selection with d2u mode
	panel_spi_send_data(0x0b);
	panel_spi_send_cmd(0xCCb9);     //ccba[7:0] : reg setting for signal10 selection with d2u mode
	panel_spi_send_data(0x01);

// ccdx
	panel_spi_send_cmd(0xCCd1);     //ccd2[7:0] : reg setting for signal27 selection with d2u mode
	panel_spi_send_data(0x0a);
	panel_spi_send_cmd(0xCCd2);     //ccd3[7:0] : reg setting for signal28 selection with d2u mode
	panel_spi_send_data(0x0c);
	panel_spi_send_cmd(0xCCd4);     //ccd5[7:0] : reg setting for signal30 selection with d2u mode
	panel_spi_send_data(0x02);


//C09x : mck_shift1/mck_shift2/mck_shift3
	panel_spi_send_cmd(0xc090);
	panel_spi_send_data(0x00);
	panel_spi_send_cmd(0xc091);
	panel_spi_send_data(0x44);
	panel_spi_send_cmd(0xc092);
	panel_spi_send_data(0x00);
	panel_spi_send_cmd(0xc093);
	panel_spi_send_data(0x00);
	panel_spi_send_cmd(0xc094);
	panel_spi_send_data(0x00);
	panel_spi_send_cmd(0xc095);
	panel_spi_send_data(0x10);


//C0Ax : hs_shift/vs_shift
	panel_spi_send_cmd(0xc1a6);
	panel_spi_send_data(0x4c);
	panel_spi_send_cmd(0xc1a7);
	panel_spi_send_data(0x00);
	panel_spi_send_cmd(0xc1a8);
	panel_spi_send_data(0x00);

	panel_spi_send_cmd(0x3A00);
	panel_spi_send_data(0x55);

	panel_spi_send_cmd(0x1100);
	mdelay(200);
	panel_spi_send_cmd(0x2900);
	mdelay(20);
	return 0;
}

static int truly1p6365_sleep(void)
{
	return 0;
}

static int truly1p6365_wakeup(void)
{
	return 0;
}

static int truly1p6365_set_active_win(struct gouda_rect *r)
{
	return 0;
}

static int truly1p6365_set_rotation(int rotate)
{
	return 0;
}

static int truly1p6365_close(void)
{
	return 0;
}


static struct rda_lcd_info truly1p6365_info = {
	.ops = {
		.s_init_gpio = truly1p6365_init_gpio,
		.s_open = truly1p6365_open,
		.s_active_win = truly1p6365_set_active_win,
		.s_rotation = truly1p6365_set_rotation,
		.s_sleep = truly1p6365_sleep,
		.s_wakeup = truly1p6365_wakeup,
		.s_close = truly1p6365_close},
	.lcd = {
		.width = WVGA_LCDD_DISP_X,
		.height = WVGA_LCDD_DISP_Y,
		.lcd_interface = GOUDA_LCD_IF_DPI,
		.lcd_timing = {
			.rgb = TRYLYLP6365_TIMING},
		.lcd_cfg = {
			.rgb = TRYLYLP6365_CONFIG }
	},
	.name = TRYLYLP6365_PANEL_NAME,
};


static int rda_fb_panel_truly1p6365_probe(struct spi_device *spi)
{
	struct rda_spi_panel_device *panel_dev;
	int ret = 0;

	panel_dev = kzalloc(sizeof(*panel_dev), GFP_KERNEL);

	if (panel_dev == NULL) {
		dev_err(&spi->dev, "rda_fb_panel_truly1p6365, out of memory\n");
		return -ENOMEM;
	}

	spi->mode = SPI_MODE_2;
	panel_dev->spi = spi;
	panel_dev->spi_xfer_num = 0;

	dev_set_drvdata(&spi->dev, panel_dev);

	spi->bits_per_word = 8;
	spi->max_speed_hz = 500000;
	spi->controller_data = &TRYLYLP6365_spicfg;

	ret = spi_setup(spi);

	if (ret < 0) {
		printk("error spi_setup failed\n");
		goto out_free_dev;
	}

	rda_truly1p6365_spi_dev = panel_dev;

	rda_fb_register_panel(&truly1p6365_info);

	dev_info(&spi->dev, "rda panel truly1p6365 registered\n");
	return 0;

out_free_dev:

	kfree(panel_dev);
	panel_dev = NULL;

	return ret;
}

static int rda_fb_panel_truly1p6365_remove(struct spi_device *spi)
{
	struct rda_spi_panel_device *panel_dev;

	panel_dev = dev_get_drvdata(&spi->dev);

	kfree(panel_dev);
	return 0;
}

#ifdef CONFIG_PM
static int rda_fb_panel_truly1p6365_suspend(struct spi_device *spi, pm_message_t mesg)
{
	return 0;
}

static int rda_fb_panel_truly1p6365_resume(struct spi_device *spi)
{
	return 0;
}
#else
#define rda_fb_panel_truly1p6365_suspend NULL
#define rda_fb_panel_truly1p6365_resume NULL
#endif /* CONFIG_PM */

/* The name is specific for each panel, it should be different from the
   abstract name "rda-fb-panel" */
static struct spi_driver rda_fb_panel_truly1p6365_driver = {
	.driver = {
		.name = TRYLYLP6365_PANEL_NAME,
		.owner = THIS_MODULE,
	},
	.probe = rda_fb_panel_truly1p6365_probe,
	.remove = rda_fb_panel_truly1p6365_remove,
	.suspend = rda_fb_panel_truly1p6365_suspend,
	.resume = rda_fb_panel_truly1p6365_resume,
};

static struct rda_panel_driver truly1p6365_mcu_panel_driver = {
	.panel_type = GOUDA_LCD_IF_DPI,
	.lcd_driver_info = &truly1p6365_info,
	.rgb_panel_driver = &rda_fb_panel_truly1p6365_driver,
};
