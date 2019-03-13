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


static struct rda_spi_panel_device *rda_otm8019_spi_dev = NULL;

#define rda_spi_dev  rda_otm8019_spi_dev


static inline void panel_spi_send_data(u8 data)
{
	u8 AddressAndData[2];
	/* First Transmit */
	AddressAndData[0] = 0x40;
	AddressAndData[1] = (data & 0x00ff);
	panel_spi_transfer(rda_spi_dev, AddressAndData, NULL, 2);
	udelay(250);
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
	udelay(250);
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

RDA_SPI_PARAMETERS otm8019_spicfg = {
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



#define otm8019_TIMING			\
	{				\
	.lcd_freq=28000000,	\
	.clk_divider = 15,		\
	.height = 860,			\
	.width = 480,			\
	.h_low=20,			\
	.v_low =18,			\
	.h_back_porch = 44,		\
	.h_front_porch = 46,		\
	.v_back_porch = 16,		\
	.v_front_porch = 20,		\
	.dot_clk_pol = false,		\
	}                               \

#define  otm8019_CONFIG               \
	{				\
	.frame1 =1,			\
	.rgb_format = RGB_IS_16BIT,	\
	.pix_fmt = PIX_FMT,		\
	}

static int otm8019_init_gpio(void)
{
//printk("otm8019_init_gpio !");
	gpio_request(GPIO_LCD_RESET, "lcd reset");
	gpio_direction_output(GPIO_LCD_RESET, 1);
	mdelay(1);
	gpio_set_value(GPIO_LCD_RESET, 0);
	mdelay(1);
	gpio_set_value(GPIO_LCD_RESET, 1);
	udelay(100);

	return 0;
}



static int otm8019_open(void)
{

	printk("%s\n",__func__);
	panel_spi_send_cmd(0xFF00); panel_spi_send_data(0x80);
	panel_spi_send_cmd(0xFF01); panel_spi_send_data(0x19);
	panel_spi_send_cmd(0xFF02); panel_spi_send_data(0x01);

	panel_spi_send_cmd(0xFF80); panel_spi_send_data(0x80);
	panel_spi_send_cmd(0xFF81); panel_spi_send_data(0x19);

	panel_spi_send_cmd(0xB390); panel_spi_send_data(0x02);
	panel_spi_send_cmd(0xB391); panel_spi_send_data(0x00);
	panel_spi_send_cmd(0xB392); panel_spi_send_data(0x45);

	panel_spi_send_cmd(0xC0A2); panel_spi_send_data(0x04);
	panel_spi_send_cmd(0xC0A3); panel_spi_send_data(0x00);
	panel_spi_send_cmd(0xC0A4); panel_spi_send_data(0x02);

	panel_spi_send_cmd(0xC080); panel_spi_send_data(0x00);
	panel_spi_send_cmd(0xC081); panel_spi_send_data(0x58);
	panel_spi_send_cmd(0xC082); panel_spi_send_data(0x00);
	panel_spi_send_cmd(0xC083); panel_spi_send_data(0x14);
	panel_spi_send_cmd(0xC084); panel_spi_send_data(0x16);

	panel_spi_send_cmd(0xC090); panel_spi_send_data(0x00);
	panel_spi_send_cmd(0xC091); panel_spi_send_data(0x15);
	panel_spi_send_cmd(0xC092); panel_spi_send_data(0x00);
	panel_spi_send_cmd(0xC093); panel_spi_send_data(0x00);
	panel_spi_send_cmd(0xC094); panel_spi_send_data(0x00);
	panel_spi_send_cmd(0xC095); panel_spi_send_data(0x03);

	panel_spi_send_cmd(0xC0B4); panel_spi_send_data(0x70);
	panel_spi_send_cmd(0xC0B5); panel_spi_send_data(0x18);

	panel_spi_send_cmd(0xC181); panel_spi_send_data(0x33);

	panel_spi_send_cmd(0xC480); panel_spi_send_data(0x30);
	panel_spi_send_cmd(0xC481); panel_spi_send_data(0x83);

	panel_spi_send_cmd(0xC489); panel_spi_send_data(0x08);

	panel_spi_send_cmd(0xC582); panel_spi_send_data(0xB0);

	panel_spi_send_cmd(0xC590); panel_spi_send_data(0x4E);
	panel_spi_send_cmd(0xC591); panel_spi_send_data(0x87);
	panel_spi_send_cmd(0xC592); panel_spi_send_data(0x01);
	panel_spi_send_cmd(0xC593); panel_spi_send_data(0x03);

	panel_spi_send_cmd(0xC5B1); panel_spi_send_data(0xA9);

	//C08x
	panel_spi_send_cmd(0xC080);	panel_spi_send_data(0x00);
	panel_spi_send_cmd(0xC081);	panel_spi_send_data(0x58);
	panel_spi_send_cmd(0xC082);	panel_spi_send_data(0x00);
	panel_spi_send_cmd(0xC083);	panel_spi_send_data(0x14);
	panel_spi_send_cmd(0xC084);	panel_spi_send_data(0x16);

	//C09X
	panel_spi_send_cmd(0xC090);	panel_spi_send_data(0x00);
	panel_spi_send_cmd(0xC091);	panel_spi_send_data(0x15);
	panel_spi_send_cmd(0xC092);	panel_spi_send_data(0x00);
	panel_spi_send_cmd(0xC093);	panel_spi_send_data(0x00);
	panel_spi_send_cmd(0xC094);	panel_spi_send_data(0x00);
	panel_spi_send_cmd(0xC095);	panel_spi_send_data(0x04);

	panel_spi_send_cmd(0xC1A6);	panel_spi_send_data(0x01);
	panel_spi_send_cmd(0xC1A7);	panel_spi_send_data(0x00);
	panel_spi_send_cmd(0xC1A8);	panel_spi_send_data(0x01);

	//CE8X <TCON_GAVE>
	panel_spi_send_cmd(0xCE80);	panel_spi_send_data(0x8B);
	panel_spi_send_cmd(0xCE81);	panel_spi_send_data(0x03);
	panel_spi_send_cmd(0xCE82);	panel_spi_send_data(0x0A);
	panel_spi_send_cmd(0xCE83);	panel_spi_send_data(0x8A);
	panel_spi_send_cmd(0xCE84);	panel_spi_send_data(0x03);
	panel_spi_send_cmd(0xCE85);	panel_spi_send_data(0x0A);
	panel_spi_send_cmd(0xCE86);	panel_spi_send_data(0x89);
	panel_spi_send_cmd(0xCE87);	panel_spi_send_data(0x03);
	panel_spi_send_cmd(0xCE88);	panel_spi_send_data(0x0A);
	panel_spi_send_cmd(0xCE89);	panel_spi_send_data(0x88);
	panel_spi_send_cmd(0xCE8A);	panel_spi_send_data(0x03);
	panel_spi_send_cmd(0xCE8B);	panel_spi_send_data(0x0A);

	//CEAx
	panel_spi_send_cmd(0xCEA0);	panel_spi_send_data(0x38);
	panel_spi_send_cmd(0xCEA1);	panel_spi_send_data(0x07);
	panel_spi_send_cmd(0xCEA2);	panel_spi_send_data(0x83);
	panel_spi_send_cmd(0xCEA3);	panel_spi_send_data(0x54);
	panel_spi_send_cmd(0xCEA4);	panel_spi_send_data(0x8C);
	panel_spi_send_cmd(0xCEA5);	panel_spi_send_data(0x10);
	panel_spi_send_cmd(0xCEA6);	panel_spi_send_data(0x00);
	panel_spi_send_cmd(0xCEA7);	panel_spi_send_data(0x38);
	panel_spi_send_cmd(0xCEA8);	panel_spi_send_data(0x06);
	panel_spi_send_cmd(0xCEA9);	panel_spi_send_data(0x83);
	panel_spi_send_cmd(0xCEAA);	panel_spi_send_data(0x55);
	panel_spi_send_cmd(0xCEAB);	panel_spi_send_data(0x8C);
	panel_spi_send_cmd(0xCEAC);	panel_spi_send_data(0x10);
	panel_spi_send_cmd(0xCEAD);	panel_spi_send_data(0x00);

	//CEBx
	panel_spi_send_cmd(0xCEB0);	panel_spi_send_data(0x38);
	panel_spi_send_cmd(0xCEB1);	panel_spi_send_data(0x05);
	panel_spi_send_cmd(0xCEB2);	panel_spi_send_data(0x83);
	panel_spi_send_cmd(0xCEB3);	panel_spi_send_data(0x56);
	panel_spi_send_cmd(0xCEB4);	panel_spi_send_data(0x8C);
	panel_spi_send_cmd(0xCEB5);	panel_spi_send_data(0x10);
	panel_spi_send_cmd(0xCEB6);	panel_spi_send_data(0x00);
	panel_spi_send_cmd(0xCEB7);	panel_spi_send_data(0x38);
	panel_spi_send_cmd(0xCEB8);	panel_spi_send_data(0x04);
	panel_spi_send_cmd(0xCEB9);	panel_spi_send_data(0x83);
	panel_spi_send_cmd(0xCEBA);	panel_spi_send_data(0x57);
	panel_spi_send_cmd(0xCEBB);	panel_spi_send_data(0x8C);
	panel_spi_send_cmd(0xCEBC);	panel_spi_send_data(0x10);
	panel_spi_send_cmd(0xCEBD);	panel_spi_send_data(0x00);


	//CECx
	panel_spi_send_cmd(0xCEC0);	panel_spi_send_data(0x38);
	panel_spi_send_cmd(0xCEC1);	panel_spi_send_data(0x03);
	panel_spi_send_cmd(0xCEC2);	panel_spi_send_data(0x83);
	panel_spi_send_cmd(0xCEC3);	panel_spi_send_data(0x58);
	panel_spi_send_cmd(0xCEC4);	panel_spi_send_data(0x8C);
	panel_spi_send_cmd(0xCEC5);	panel_spi_send_data(0x10);
	panel_spi_send_cmd(0xCEC6);	panel_spi_send_data(0x00);
	panel_spi_send_cmd(0xCEC7);	panel_spi_send_data(0x38);
	panel_spi_send_cmd(0xCEC8);	panel_spi_send_data(0x02);
	panel_spi_send_cmd(0xCEC9);	panel_spi_send_data(0x83);
	panel_spi_send_cmd(0xCECA);	panel_spi_send_data(0x59);
	panel_spi_send_cmd(0xCECB);	panel_spi_send_data(0x8C);
	panel_spi_send_cmd(0xCECC);	panel_spi_send_data(0x10);
	panel_spi_send_cmd(0xCECD);	panel_spi_send_data(0x00);

	//CEDx
	panel_spi_send_cmd(0xCED0);	panel_spi_send_data(0x38);
	panel_spi_send_cmd(0xCED1);	panel_spi_send_data(0x01);
	panel_spi_send_cmd(0xCED2);	panel_spi_send_data(0x83);
	panel_spi_send_cmd(0xCED3);	panel_spi_send_data(0x5A);
	panel_spi_send_cmd(0xCED4);	panel_spi_send_data(0x8C);
	panel_spi_send_cmd(0xCED5);	panel_spi_send_data(0x10);
	panel_spi_send_cmd(0xCED6);	panel_spi_send_data(0x00);
	panel_spi_send_cmd(0xCED7);	panel_spi_send_data(0x38);
	panel_spi_send_cmd(0xCED8);	panel_spi_send_data(0x00);
	panel_spi_send_cmd(0xCED9);	panel_spi_send_data(0x83);
	panel_spi_send_cmd(0xCEDA);	panel_spi_send_data(0x5B);
	panel_spi_send_cmd(0xCEDB);	panel_spi_send_data(0x8C);
	panel_spi_send_cmd(0xCEDC);	panel_spi_send_data(0x10);
	panel_spi_send_cmd(0xCEDD);	panel_spi_send_data(0x00);

	//CFCx
	panel_spi_send_cmd(0xCFC0);	panel_spi_send_data(0x3D);
	panel_spi_send_cmd(0xCFC1);	panel_spi_send_data(0x3D);
	panel_spi_send_cmd(0xCFC2);	panel_spi_send_data(0x0D);//GPW1&GPW2 space
	panel_spi_send_cmd(0xCFC3);	panel_spi_send_data(0x20);
	panel_spi_send_cmd(0xCFC4);	panel_spi_send_data(0x00);
	panel_spi_send_cmd(0xCFC5);	panel_spi_send_data(0x00);
	panel_spi_send_cmd(0xCFC6);	panel_spi_send_data(0x01);
	panel_spi_send_cmd(0xCFC7);	panel_spi_send_data(0x80);
	panel_spi_send_cmd(0xCFC8);	panel_spi_send_data(0x00);
	panel_spi_send_cmd(0xCFC9);	panel_spi_send_data(0x0A);


	//CBCx
	panel_spi_send_cmd(0xCBC0);	panel_spi_send_data(0x15);
	panel_spi_send_cmd(0xCBC1);	panel_spi_send_data(0x15);
	panel_spi_send_cmd(0xCBC2);	panel_spi_send_data(0x15);
	panel_spi_send_cmd(0xCBC3);	panel_spi_send_data(0x15);
	panel_spi_send_cmd(0xCBC4);	panel_spi_send_data(0x15);
	panel_spi_send_cmd(0xCBC5);	panel_spi_send_data(0x15);
	panel_spi_send_cmd(0xCBC6);	panel_spi_send_data(0x15);
	panel_spi_send_cmd(0xCBC7);	panel_spi_send_data(0x15);
	panel_spi_send_cmd(0xCBC8);	panel_spi_send_data(0x15);
	panel_spi_send_cmd(0xCBC9);	panel_spi_send_data(0x15);
	panel_spi_send_cmd(0xCBCA);	panel_spi_send_data(0x00);
	panel_spi_send_cmd(0xCBCB);	panel_spi_send_data(0x00);
	panel_spi_send_cmd(0xCBCC);	panel_spi_send_data(0x00);
	panel_spi_send_cmd(0xCBCD);	panel_spi_send_data(0x00);
	panel_spi_send_cmd(0xCBCE);	panel_spi_send_data(0x00);

	//CBDX
	panel_spi_send_cmd(0xCBD0);	panel_spi_send_data(0x00);
	panel_spi_send_cmd(0xCBD1);	panel_spi_send_data(0x00);
	panel_spi_send_cmd(0xCBD2);	panel_spi_send_data(0x00);
	panel_spi_send_cmd(0xCBD3);	panel_spi_send_data(0x00);
	panel_spi_send_cmd(0xCBD4);	panel_spi_send_data(0x00);
	panel_spi_send_cmd(0xCBD5);	panel_spi_send_data(0x00);
	panel_spi_send_cmd(0xCBD6);	panel_spi_send_data(0x00);
	panel_spi_send_cmd(0xCBD7);	panel_spi_send_data(0x00);
	panel_spi_send_cmd(0xCBD8);	panel_spi_send_data(0x00);
	panel_spi_send_cmd(0xCBD9);	panel_spi_send_data(0x00);
	panel_spi_send_cmd(0xCBDA);	panel_spi_send_data(0x00);
	panel_spi_send_cmd(0xCBDB);	panel_spi_send_data(0x15);
	panel_spi_send_cmd(0xCBDC);	panel_spi_send_data(0x15);
	panel_spi_send_cmd(0xCBDD);	panel_spi_send_data(0x15);
	panel_spi_send_cmd(0xCBDE);	panel_spi_send_data(0x15);


	//CBEx
	panel_spi_send_cmd(0xCBE0);	panel_spi_send_data(0x15);
	panel_spi_send_cmd(0xCBE1);	panel_spi_send_data(0x15);
	panel_spi_send_cmd(0xCBE2);	panel_spi_send_data(0x15);
	panel_spi_send_cmd(0xCBE3);	panel_spi_send_data(0x15);
	panel_spi_send_cmd(0xCBE4);	panel_spi_send_data(0x15);
	panel_spi_send_cmd(0xCBE5);	panel_spi_send_data(0x15);
	panel_spi_send_cmd(0xCBE6);	panel_spi_send_data(0x00);
	panel_spi_send_cmd(0xCBE7);	panel_spi_send_data(0x00);
	panel_spi_send_cmd(0xCBE8);	panel_spi_send_data(0x00);
	panel_spi_send_cmd(0xCBE9);	panel_spi_send_data(0x00);


	//CC8x
	panel_spi_send_cmd(0xCC80);	panel_spi_send_data(0x26);
	panel_spi_send_cmd(0xCC81);	panel_spi_send_data(0x25);
	panel_spi_send_cmd(0xCC82);	panel_spi_send_data(0x21);
	panel_spi_send_cmd(0xCC83);	panel_spi_send_data(0x22);
	panel_spi_send_cmd(0xCC84);	panel_spi_send_data(0x0C);
	panel_spi_send_cmd(0xCC85);	panel_spi_send_data(0x0A);
	panel_spi_send_cmd(0xCC86);	panel_spi_send_data(0x10);
	panel_spi_send_cmd(0xCC87);	panel_spi_send_data(0x0E);
	panel_spi_send_cmd(0xCC88);	panel_spi_send_data(0x02);
	panel_spi_send_cmd(0xCC89);	panel_spi_send_data(0x04);


	//CCAx
	panel_spi_send_cmd(0xCCA0);	panel_spi_send_data(0x00);
	panel_spi_send_cmd(0xCCA1);	panel_spi_send_data(0x03);
	panel_spi_send_cmd(0xCCA2);	panel_spi_send_data(0x01);
	panel_spi_send_cmd(0xCCA3);	panel_spi_send_data(0x0D);
	panel_spi_send_cmd(0xCCA4);	panel_spi_send_data(0x0F);
	panel_spi_send_cmd(0xCCA5);	panel_spi_send_data(0x09);
	panel_spi_send_cmd(0xCCA6);	panel_spi_send_data(0x0B);
	panel_spi_send_cmd(0xCCA7);	panel_spi_send_data(0x22);
	panel_spi_send_cmd(0xCCA8);	panel_spi_send_data(0x21);
	panel_spi_send_cmd(0xCCA9);	panel_spi_send_data(0x25);
	panel_spi_send_cmd(0xCCAA);	panel_spi_send_data(0x26);
	panel_spi_send_cmd(0xCCAB);	panel_spi_send_data(0x00);
	panel_spi_send_cmd(0xCCAC);	panel_spi_send_data(0x00);
	panel_spi_send_cmd(0xCCAD);	panel_spi_send_data(0x00);
	panel_spi_send_cmd(0xCCAE);	panel_spi_send_data(0x00);

	//CCBx
	panel_spi_send_cmd(0xCCB0);	panel_spi_send_data(0x25);
	panel_spi_send_cmd(0xCCB1);	panel_spi_send_data(0x26);
	panel_spi_send_cmd(0xCCB2);	panel_spi_send_data(0x21);
	panel_spi_send_cmd(0xCCB3);	panel_spi_send_data(0x22);
	panel_spi_send_cmd(0xCCB4);	panel_spi_send_data(0x0B);
	panel_spi_send_cmd(0xCCB5);	panel_spi_send_data(0x0D);
	panel_spi_send_cmd(0xCCB6);	panel_spi_send_data(0x0F);
	panel_spi_send_cmd(0xCCB7);	panel_spi_send_data(0x09);
	panel_spi_send_cmd(0xCCB8);	panel_spi_send_data(0x03);
	panel_spi_send_cmd(0xCCB9);	panel_spi_send_data(0x01);


	//CCDx
	panel_spi_send_cmd(0xCCD0);	panel_spi_send_data(0x00);
	panel_spi_send_cmd(0xCCD1);	panel_spi_send_data(0x02);
	panel_spi_send_cmd(0xCCD2);	panel_spi_send_data(0x04);
	panel_spi_send_cmd(0xCCD3);	panel_spi_send_data(0x0A);
	panel_spi_send_cmd(0xCCD4);	panel_spi_send_data(0x10);
	panel_spi_send_cmd(0xCCD5);	panel_spi_send_data(0x0E);
	panel_spi_send_cmd(0xCCD6);	panel_spi_send_data(0x0C);
	panel_spi_send_cmd(0xCCD7);	panel_spi_send_data(0x22);
	panel_spi_send_cmd(0xCCD8);	panel_spi_send_data(0x21);
	panel_spi_send_cmd(0xCCD9);	panel_spi_send_data(0x26);
	panel_spi_send_cmd(0xCCDA);	panel_spi_send_data(0x25);
	panel_spi_send_cmd(0xCCDB);	panel_spi_send_data(0x00);
	panel_spi_send_cmd(0xCCDC);	panel_spi_send_data(0x00);
	panel_spi_send_cmd(0xCCDD);	panel_spi_send_data(0x00);
	panel_spi_send_cmd(0xCCDE);	panel_spi_send_data(0x00);
	panel_spi_send_cmd(0xD800); panel_spi_send_data(0x70);
	panel_spi_send_cmd(0xD801); panel_spi_send_data(0x70);

	panel_spi_send_cmd(0xD900); panel_spi_send_data(0x62);

	panel_spi_send_cmd(0xE100); panel_spi_send_data(0x00);
	panel_spi_send_cmd(0xE101); panel_spi_send_data(0x01);
	panel_spi_send_cmd(0xE102); panel_spi_send_data(0x03);
	panel_spi_send_cmd(0xE103); panel_spi_send_data(0x07);
	panel_spi_send_cmd(0xE104); panel_spi_send_data(0x14);
	panel_spi_send_cmd(0xE105); panel_spi_send_data(0x26);
	panel_spi_send_cmd(0xE106); panel_spi_send_data(0x30);
	panel_spi_send_cmd(0xE107); panel_spi_send_data(0x79);
	panel_spi_send_cmd(0xE108); panel_spi_send_data(0x6C);
	panel_spi_send_cmd(0xE109); panel_spi_send_data(0x86);
	panel_spi_send_cmd(0xE10A); panel_spi_send_data(0x7F);
	panel_spi_send_cmd(0xE10B); panel_spi_send_data(0x6D);
	panel_spi_send_cmd(0xE10C); panel_spi_send_data(0x85);
	panel_spi_send_cmd(0xE10D); panel_spi_send_data(0x70);
	panel_spi_send_cmd(0xE10E); panel_spi_send_data(0x75);
	panel_spi_send_cmd(0xE10F); panel_spi_send_data(0x6D);
	panel_spi_send_cmd(0xE110); panel_spi_send_data(0x66);
	panel_spi_send_cmd(0xE111); panel_spi_send_data(0x57);
	panel_spi_send_cmd(0xE112); panel_spi_send_data(0x50);
	panel_spi_send_cmd(0xE113); panel_spi_send_data(0x00);

	panel_spi_send_cmd(0xE200); panel_spi_send_data(0x00);
	panel_spi_send_cmd(0xE201); panel_spi_send_data(0x01);
	panel_spi_send_cmd(0xE202); panel_spi_send_data(0x03);
	panel_spi_send_cmd(0xE203); panel_spi_send_data(0x07);
	panel_spi_send_cmd(0xE204); panel_spi_send_data(0x14);
	panel_spi_send_cmd(0xE205); panel_spi_send_data(0x26);
	panel_spi_send_cmd(0xE206); panel_spi_send_data(0x30);
	panel_spi_send_cmd(0xE207); panel_spi_send_data(0x79);
	panel_spi_send_cmd(0xE208); panel_spi_send_data(0x6C);
	panel_spi_send_cmd(0xE209); panel_spi_send_data(0x86);
	panel_spi_send_cmd(0xE20A); panel_spi_send_data(0x7F);
	panel_spi_send_cmd(0xE20B); panel_spi_send_data(0x6D);
	panel_spi_send_cmd(0xE20C); panel_spi_send_data(0x85);
	panel_spi_send_cmd(0xE20D); panel_spi_send_data(0x70);
	panel_spi_send_cmd(0xE20E); panel_spi_send_data(0x75);
	panel_spi_send_cmd(0xE20F); panel_spi_send_data(0x6E);
	panel_spi_send_cmd(0xE210); panel_spi_send_data(0x67);
	panel_spi_send_cmd(0xE211); panel_spi_send_data(0x56);
	panel_spi_send_cmd(0xE212); panel_spi_send_data(0x50);
	panel_spi_send_cmd(0xE213); panel_spi_send_data(0x00);

	panel_spi_send_cmd(0xC480); panel_spi_send_data(0x30);

	panel_spi_send_cmd(0xC098); panel_spi_send_data(0x00);

	panel_spi_send_cmd(0xC0A9); panel_spi_send_data(0x0A);

	panel_spi_send_cmd(0xC1B0); panel_spi_send_data(0x20);
	panel_spi_send_cmd(0xC1B1); panel_spi_send_data(0x00);
	panel_spi_send_cmd(0xC1B2); panel_spi_send_data(0x00);

	panel_spi_send_cmd(0xC0E1); panel_spi_send_data(0x40);
	panel_spi_send_cmd(0xC0E2); panel_spi_send_data(0x30);

	panel_spi_send_cmd(0xC180); panel_spi_send_data(0x03);
	panel_spi_send_cmd(0xC181); panel_spi_send_data(0x33);

	panel_spi_send_cmd(0xC1A0); panel_spi_send_data(0xE8);

	panel_spi_send_cmd(0xB690); panel_spi_send_data(0xB4);

	panel_spi_send_cmd(0xFF91); panel_spi_send_data(0x01);
	panel_spi_send_cmd(0xFF93); panel_spi_send_data(0x10);

	mdelay(10);

	panel_spi_send_cmd(0xFB00); panel_spi_send_data(0x01);
	mdelay(10);
	panel_spi_send_cmd(0xC0A9); panel_spi_send_data(0x0A);
	panel_spi_send_cmd(0xC0E2); panel_spi_send_data(0x30);
	panel_spi_send_cmd(0xD691); panel_spi_send_data(0xCF);
	panel_spi_send_cmd(0xC095); panel_spi_send_data(0x08);

	panel_spi_send_cmd(0xFF00); panel_spi_send_data(0xFF);
	panel_spi_send_cmd(0xFF01); panel_spi_send_data(0xFF);
	panel_spi_send_cmd(0xFF02); panel_spi_send_data(0xFF);
	panel_spi_send_cmd(0x3A00); panel_spi_send_data(0x55);

	panel_spi_send_cmd(0x1100);
	mdelay(120);

	panel_spi_send_cmd(0x2900);
	mdelay(10);

	return 0;
}

static int otm8019_sleep(void)
{
	printk("otm8019_sleep in!\n");
	//		gpio_set_value(GPIO_LCD_RESET, 0);

	panel_spi_send_cmd(0x2800);
	mdelay(120);

	panel_spi_send_cmd(0x1000);
	mdelay(50);

	return 0;
}

static int otm8019_wakeup(void)
{
	/* Exist from sleep mode */
	//Sleep Out
	printk("otm8019_wakeup out!\n");
	otm8019_init_gpio();
	gpio_set_value(GPIO_LCD_RESET, 1);
	otm8019_open();
//	printk("otm8019_wakeup out  adsfaf    !\n");

	return 0;
}

static int otm8019_set_active_win(struct gouda_rect *r)
{
	return 0;
}

static int otm8019_set_rotation(int rotate)
{
	return 0;
}

static int otm8019_close(void)
{
//	printk("otm8019_close !");

	return 0;
}


static struct rda_lcd_info otm8019_info = {
	.ops = {
		.s_init_gpio = otm8019_init_gpio,
		.s_open = otm8019_open,
		.s_active_win = otm8019_set_active_win,
		.s_rotation = otm8019_set_rotation,
		.s_sleep = otm8019_sleep,
		.s_wakeup = otm8019_wakeup,
		.s_close = otm8019_close},
	.lcd = {
		.width = FWVGA_LCDD_DISP_X,
		.height = FWVGA_LCDD_DISP_Y,
		.lcd_interface = GOUDA_LCD_IF_DPI,
		.lcd_timing = {
			       .rgb = otm8019_TIMING},
		.lcd_cfg = {
			    .rgb = otm8019_CONFIG}
		},
	.name = OTM8019A_PANEL_NAME,
};



static int rda_fb_panel_otm8019_probe(struct spi_device *spi)
{
	struct rda_spi_panel_device *panel_dev;
	int ret = 0;

	panel_dev = kzalloc(sizeof(*panel_dev), GFP_KERNEL);

	printk(KERN_INFO "rda_fb_panel_otm8019_probe   panel_spi_test: id is \n");

	if (panel_dev == NULL) {
		dev_err(&spi->dev, "rda_fb_panel_otm8019, out of memory\n");
		return -ENOMEM;
	}

	spi->mode = SPI_MODE_2;
	panel_dev->spi = spi;
	panel_dev->spi_xfer_num = 0;

	dev_set_drvdata(&spi->dev, panel_dev);

	spi->bits_per_word = 8; //9;
	spi->max_speed_hz = 20000000;//1000000; //500000;
	spi->controller_data = &otm8019_spicfg;

	ret = spi_setup(spi);

	if (ret < 0) {
		printk("error spi_setup failed\n");
		goto out_free_dev;
	}

	rda_otm8019_spi_dev = panel_dev;

	rda_fb_register_panel(&otm8019_info);

	dev_info(&spi->dev, "rda panel otm8019 registered\n");
	return 0;

out_free_dev:

	kfree(panel_dev);
	panel_dev = NULL;

	return ret;
}

static int rda_fb_panel_otm8019_remove(struct spi_device *spi)
{
	struct rda_spi_panel_device *panel_dev;

	panel_dev = dev_get_drvdata(&spi->dev);

	kfree(panel_dev);
	return 0;
}

#ifdef CONFIG_PM
static int rda_fb_panel_otm8019_suspend(struct spi_device *spi, pm_message_t mesg)
{
	return 0;
}

static int rda_fb_panel_otm8019_resume(struct spi_device *spi)
{
	return 0;
}
#else
#define rda_fb_panel_otm8019_suspend NULL
#define rda_fb_panel_otm8019_resume NULL
#endif /* CONFIG_PM */

/* The name is specific for each panel, it should be different from the
   abstract name "rda-fb-panel" */
static struct spi_driver rda_fb_panel_otm8019_driver = {
	.driver = {
		   .name = OTM8019A_PANEL_NAME,
		   .owner = THIS_MODULE,
		   },
	.probe = rda_fb_panel_otm8019_probe,
	.remove = rda_fb_panel_otm8019_remove,
	.suspend = rda_fb_panel_otm8019_suspend,
	.resume = rda_fb_panel_otm8019_resume,
};

static struct rda_panel_driver otm8019a_mcu_panel_driver = {
	.panel_type = GOUDA_LCD_IF_DPI,
	.lcd_driver_info = &otm8019_info,
	.rgb_panel_driver = &rda_fb_panel_otm8019_driver,
};
