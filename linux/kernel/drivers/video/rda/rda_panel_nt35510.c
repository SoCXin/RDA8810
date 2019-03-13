#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/jiffies.h>
#include <linux/gpio.h>
#include <linux/spi/spi.h>

#include <plat/devices.h>
#include <plat/rda_debug.h>
#include <mach/board.h>

#include "rda_gouda.h"
#include "rda_panel.h"

static struct rda_spi_panel_device *rda_nt35510_spi_dev = NULL;

#define rda_spi_dev rda_nt35510_spi_dev 

static void nt35510_spi_write_reg(u16 cmd, u8 data)
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

static inline void panel_spi_send_cmd(u16 cmd)
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
}

static inline int panel_spi_read_reg(u16 adrress)
{
	u8 receivebuffer[12];
	u8 AddressAndData[2];

	/* First Transmit */
	AddressAndData[0] = 0x20;
	AddressAndData[1] = (adrress & 0xff00) >> 8;
	//rda_lcd_write(rdalcd, AddressAndData, 2);
	panel_spi_transfer(rda_spi_dev, AddressAndData, NULL, 2);

	//serial_puts(" get data 1 ");
	/*second transmit */
	AddressAndData[0] = 0x00;
	AddressAndData[1] = (adrress & 0x00ff);
	//rda_lcd_write(rdalcd, AddressAndData, 2);
	panel_spi_transfer(rda_spi_dev, AddressAndData, NULL, 2);

	/*Third transmit */
	AddressAndData[0] = 0xc0;
	AddressAndData[1] = 0xff;
	panel_spi_transfer(rda_spi_dev, AddressAndData, NULL, 2);

	panel_spi_transfer(rda_spi_dev, NULL, receivebuffer, 6);
	return receivebuffer[5];
}

RDA_SPI_PARAMETERS rgblcd_spicfg = {
	//.inputEn    = 1,
	.clkDelay = RDA_SPI_HALF_CLK_PERIOD_1,
	.doDelay = RDA_SPI_HALF_CLK_PERIOD_1,
	.diDelay = RDA_SPI_HALF_CLK_PERIOD_1,
	.csDelay = RDA_SPI_HALF_CLK_PERIOD_1,
	.csPulse = RDA_SPI_HALF_CLK_PERIOD_0,
	.frameSize = 8,
	.oeRatio = 0,
	.rxTrigger = RDA_SPI_RX_TRIGGER_4_BYTE,
	.txTrigger = RDA_SPI_TX_TRIGGER_1_EMPTY,
};

#define NT35510_TIMING			\
	{				\
	.clk_divider = 2,		\
	.height = WVGA_LCDD_DISP_Y,			\
	.width = WVAG_LCDD_DISP_X,			\
	.h_low=3,			\
	.v_low =3,			\
	.h_back_porch = 30,		\
	.h_front_porch = 30,		\
	.v_back_porch = 30,		\
	.v_front_porch = 30,		\
	}                               \

#define NT35510_CONFIG                  \
	{				\
	.frame1 =1,			\
	.rgb_format = RGB_IS_24BIT,	\
	}

static int nt35510_init_gpio(void)
{
	gpio_request(GPIO_LCD_RESET, "lcd reset");
	gpio_direction_output(GPIO_LCD_RESET, 1);
	msleep(1);
	gpio_set_value(GPIO_LCD_RESET, 0);
	msleep(100);
	gpio_set_value(GPIO_LCD_RESET, 1);
	msleep(50);


	return 0;
}

static void nt35510_readid(void)
{
#if 0
	u8 id = 0;

	while (1) {
		id = panel_spi_read_reg(0x0401);
		printk(KERN_INFO "panel_spi_test: id is 0x%x\n", id);
		if (id == 0x80) {
			break;
		}
	}
#endif
}

static int nt35510_open(void)
{
	nt35510_readid();

	nt35510_spi_write_reg(0xF000, 0x55);
	nt35510_spi_write_reg(0xF001, 0xAA);
	nt35510_spi_write_reg(0xF002, 0x52);
	nt35510_spi_write_reg(0xF003, 0x08);
	nt35510_spi_write_reg(0xF004, 0x01);	//goto page1

	nt35510_spi_write_reg(0xB000, 0x0D);
	nt35510_spi_write_reg(0xB001, 0x0D);
	nt35510_spi_write_reg(0xB002, 0x0D);

	nt35510_spi_write_reg(0xB600, 0x34);
	nt35510_spi_write_reg(0xB601, 0x34);
	nt35510_spi_write_reg(0xB602, 0x34);

	nt35510_spi_write_reg(0xB100, 0x0D);
	nt35510_spi_write_reg(0xB101, 0x0D);
	nt35510_spi_write_reg(0xB102, 0x0D);

	nt35510_spi_write_reg(0xB700, 0x34);
	nt35510_spi_write_reg(0xB701, 0x34);
	nt35510_spi_write_reg(0xB702, 0x34);

	nt35510_spi_write_reg(0xB200, 0x00);
	nt35510_spi_write_reg(0xB201, 0x00);
	nt35510_spi_write_reg(0xB202, 0x00);

	nt35510_spi_write_reg(0xB800, 0x24);
	nt35510_spi_write_reg(0xB801, 0x24);
	nt35510_spi_write_reg(0xB802, 0x24);

	nt35510_spi_write_reg(0xBF00, 0x01);

	nt35510_spi_write_reg(0xB300, 0x0F);
	nt35510_spi_write_reg(0xB301, 0x0F);
	nt35510_spi_write_reg(0xB302, 0x0F);

	nt35510_spi_write_reg(0xB900, 0x34);
	nt35510_spi_write_reg(0xB901, 0x34);
	nt35510_spi_write_reg(0xB902, 0x34);

	nt35510_spi_write_reg(0xB500, 0x08);
	nt35510_spi_write_reg(0xB501, 0x08);
	nt35510_spi_write_reg(0xB502, 0x08);

	nt35510_spi_write_reg(0xC200, 0x03);

	nt35510_spi_write_reg(0xBA00, 0x24);
	nt35510_spi_write_reg(0xBA01, 0x24);
	nt35510_spi_write_reg(0xBA02, 0x24);

	nt35510_spi_write_reg(0xBC00, 0x00);
	nt35510_spi_write_reg(0xBC01, 0x78);
	nt35510_spi_write_reg(0xBC02, 0x00);

	nt35510_spi_write_reg(0xBD00, 0x00);
	nt35510_spi_write_reg(0xBD01, 0x70);
	nt35510_spi_write_reg(0xBD02, 0x00);

	nt35510_spi_write_reg(0xBE00, 0x00);	//vcom offset for flick display
	nt35510_spi_write_reg(0xBE01, 0x80);

	nt35510_spi_write_reg(0xD100, 0x00);
	nt35510_spi_write_reg(0xD101, 0x06);
	nt35510_spi_write_reg(0xD102, 0x00);
	nt35510_spi_write_reg(0xD103, 0x07);
	nt35510_spi_write_reg(0xD104, 0x00);
	nt35510_spi_write_reg(0xD105, 0x0E);
	nt35510_spi_write_reg(0xD106, 0x00);
	nt35510_spi_write_reg(0xD107, 0x22);
	nt35510_spi_write_reg(0xD108, 0x00);
	nt35510_spi_write_reg(0xD109, 0x3B);
	nt35510_spi_write_reg(0xD10A, 0x00);
	nt35510_spi_write_reg(0xD10B, 0x71);
	nt35510_spi_write_reg(0xD10C, 0x00);
	nt35510_spi_write_reg(0xD10D, 0x9F);
	nt35510_spi_write_reg(0xD10E, 0x00);
	nt35510_spi_write_reg(0xD10F, 0xE2);
	nt35510_spi_write_reg(0xD110, 0x01);
	nt35510_spi_write_reg(0xD111, 0x12);
	nt35510_spi_write_reg(0xD112, 0x01);
	nt35510_spi_write_reg(0xD113, 0x57);
	nt35510_spi_write_reg(0xD114, 0x01);
	nt35510_spi_write_reg(0xD115, 0x88);
	nt35510_spi_write_reg(0xD116, 0x01);
	nt35510_spi_write_reg(0xD117, 0xCE);
	nt35510_spi_write_reg(0xD118, 0x02);
	nt35510_spi_write_reg(0xD119, 0x07);
	nt35510_spi_write_reg(0xD11A, 0x02);
	nt35510_spi_write_reg(0xD11B, 0x08);
	nt35510_spi_write_reg(0xD11C, 0x02);
	nt35510_spi_write_reg(0xD11D, 0x39);
	nt35510_spi_write_reg(0xD11E, 0x02);
	nt35510_spi_write_reg(0xD11F, 0x6C);
	nt35510_spi_write_reg(0xD120, 0x02);
	nt35510_spi_write_reg(0xD121, 0x87);
	nt35510_spi_write_reg(0xD122, 0x02);
	nt35510_spi_write_reg(0xD123, 0xA6);
	nt35510_spi_write_reg(0xD124, 0x02);
	nt35510_spi_write_reg(0xD125, 0xBA);
	nt35510_spi_write_reg(0xD126, 0x02);
	nt35510_spi_write_reg(0xD127, 0xD2);
	nt35510_spi_write_reg(0xD128, 0x02);
	nt35510_spi_write_reg(0xD129, 0xE2);
	nt35510_spi_write_reg(0xD12A, 0x02);
	nt35510_spi_write_reg(0xD12B, 0xF7);
	nt35510_spi_write_reg(0xD12C, 0x03);
	nt35510_spi_write_reg(0xD12D, 0x06);
	nt35510_spi_write_reg(0xD12E, 0x03);
	nt35510_spi_write_reg(0xD12F, 0x1E);
	nt35510_spi_write_reg(0xD130, 0x03);
	nt35510_spi_write_reg(0xD131, 0x55);
	nt35510_spi_write_reg(0xD132, 0x03);
	nt35510_spi_write_reg(0xD133, 0xFF);

	nt35510_spi_write_reg(0xD200, 0x00);
	nt35510_spi_write_reg(0xD201, 0x06);
	nt35510_spi_write_reg(0xD202, 0x00);
	nt35510_spi_write_reg(0xD203, 0x07);
	nt35510_spi_write_reg(0xD204, 0x00);
	nt35510_spi_write_reg(0xD205, 0x0E);
	nt35510_spi_write_reg(0xD206, 0x00);
	nt35510_spi_write_reg(0xD207, 0x22);
	nt35510_spi_write_reg(0xD208, 0x00);
	nt35510_spi_write_reg(0xD209, 0x3B);
	nt35510_spi_write_reg(0xD20A, 0x00);
	nt35510_spi_write_reg(0xD20B, 0x71);
	nt35510_spi_write_reg(0xD20C, 0x00);
	nt35510_spi_write_reg(0xD20D, 0x9F);
	nt35510_spi_write_reg(0xD20E, 0x00);
	nt35510_spi_write_reg(0xD20F, 0xE2);
	nt35510_spi_write_reg(0xD210, 0x01);
	nt35510_spi_write_reg(0xD211, 0x12);
	nt35510_spi_write_reg(0xD212, 0x01);
	nt35510_spi_write_reg(0xD213, 0x57);
	nt35510_spi_write_reg(0xD214, 0x01);
	nt35510_spi_write_reg(0xD215, 0x88);
	nt35510_spi_write_reg(0xD216, 0x01);
	nt35510_spi_write_reg(0xD217, 0xCE);
	nt35510_spi_write_reg(0xD218, 0x02);
	nt35510_spi_write_reg(0xD219, 0x07);
	nt35510_spi_write_reg(0xD21A, 0x02);
	nt35510_spi_write_reg(0xD21B, 0x08);
	nt35510_spi_write_reg(0xD21C, 0x02);
	nt35510_spi_write_reg(0xD21D, 0x39);
	nt35510_spi_write_reg(0xD21E, 0x02);
	nt35510_spi_write_reg(0xD21F, 0x6C);
	nt35510_spi_write_reg(0xD220, 0x02);
	nt35510_spi_write_reg(0xD221, 0x87);
	nt35510_spi_write_reg(0xD222, 0x02);
	nt35510_spi_write_reg(0xD223, 0xA6);
	nt35510_spi_write_reg(0xD224, 0x02);
	nt35510_spi_write_reg(0xD225, 0xBA);
	nt35510_spi_write_reg(0xD226, 0x02);
	nt35510_spi_write_reg(0xD227, 0xD2);
	nt35510_spi_write_reg(0xD228, 0x02);
	nt35510_spi_write_reg(0xD229, 0xE2);
	nt35510_spi_write_reg(0xD22A, 0x02);
	nt35510_spi_write_reg(0xD22B, 0xF7);
	nt35510_spi_write_reg(0xD22C, 0x03);
	nt35510_spi_write_reg(0xD22D, 0x06);
	nt35510_spi_write_reg(0xD22E, 0x03);
	nt35510_spi_write_reg(0xD22F, 0x1E);
	nt35510_spi_write_reg(0xD230, 0x03);
	nt35510_spi_write_reg(0xD231, 0x55);
	nt35510_spi_write_reg(0xD232, 0x03);
	nt35510_spi_write_reg(0xD233, 0xFF);

	nt35510_spi_write_reg(0xD300, 0x00);
	nt35510_spi_write_reg(0xD301, 0x06);
	nt35510_spi_write_reg(0xD302, 0x00);
	nt35510_spi_write_reg(0xD303, 0x07);
	nt35510_spi_write_reg(0xD304, 0x00);
	nt35510_spi_write_reg(0xD305, 0x0E);
	nt35510_spi_write_reg(0xD306, 0x00);
	nt35510_spi_write_reg(0xD307, 0x22);
	nt35510_spi_write_reg(0xD308, 0x00);
	nt35510_spi_write_reg(0xD309, 0x3B);
	nt35510_spi_write_reg(0xD30A, 0x00);
	nt35510_spi_write_reg(0xD30B, 0x71);
	nt35510_spi_write_reg(0xD30C, 0x00);
	nt35510_spi_write_reg(0xD30D, 0x9F);
	nt35510_spi_write_reg(0xD30E, 0x00);
	nt35510_spi_write_reg(0xD30F, 0xE2);
	nt35510_spi_write_reg(0xD310, 0x01);
	nt35510_spi_write_reg(0xD311, 0x12);
	nt35510_spi_write_reg(0xD312, 0x01);
	nt35510_spi_write_reg(0xD313, 0x57);
	nt35510_spi_write_reg(0xD314, 0x01);
	nt35510_spi_write_reg(0xD315, 0x88);
	nt35510_spi_write_reg(0xD316, 0x01);
	nt35510_spi_write_reg(0xD317, 0xCE);
	nt35510_spi_write_reg(0xD318, 0x02);
	nt35510_spi_write_reg(0xD319, 0x07);
	nt35510_spi_write_reg(0xD31A, 0x02);
	nt35510_spi_write_reg(0xD31B, 0x08);
	nt35510_spi_write_reg(0xD31C, 0x02);
	nt35510_spi_write_reg(0xD31D, 0x39);
	nt35510_spi_write_reg(0xD31E, 0x02);
	nt35510_spi_write_reg(0xD31F, 0x6C);
	nt35510_spi_write_reg(0xD320, 0x02);
	nt35510_spi_write_reg(0xD321, 0x87);
	nt35510_spi_write_reg(0xD322, 0x02);
	nt35510_spi_write_reg(0xD323, 0xA6);
	nt35510_spi_write_reg(0xD324, 0x02);
	nt35510_spi_write_reg(0xD325, 0xBA);
	nt35510_spi_write_reg(0xD326, 0x02);
	nt35510_spi_write_reg(0xD327, 0xD2);
	nt35510_spi_write_reg(0xD328, 0x02);
	nt35510_spi_write_reg(0xD329, 0xE2);
	nt35510_spi_write_reg(0xD32A, 0x02);
	nt35510_spi_write_reg(0xD32B, 0xF7);
	nt35510_spi_write_reg(0xD32C, 0x03);
	nt35510_spi_write_reg(0xD32D, 0x06);
	nt35510_spi_write_reg(0xD32E, 0x03);
	nt35510_spi_write_reg(0xD32F, 0x1E);
	nt35510_spi_write_reg(0xD330, 0x03);
	nt35510_spi_write_reg(0xD331, 0x55);
	nt35510_spi_write_reg(0xD332, 0x03);
	nt35510_spi_write_reg(0xD333, 0xFF);

	nt35510_spi_write_reg(0xD400, 0x00);
	nt35510_spi_write_reg(0xD401, 0x06);
	nt35510_spi_write_reg(0xD402, 0x00);
	nt35510_spi_write_reg(0xD403, 0x07);
	nt35510_spi_write_reg(0xD404, 0x00);
	nt35510_spi_write_reg(0xD405, 0x0E);
	nt35510_spi_write_reg(0xD406, 0x00);
	nt35510_spi_write_reg(0xD407, 0x22);
	nt35510_spi_write_reg(0xD408, 0x00);
	nt35510_spi_write_reg(0xD409, 0x3B);
	nt35510_spi_write_reg(0xD40A, 0x00);
	nt35510_spi_write_reg(0xD40B, 0x71);
	nt35510_spi_write_reg(0xD40C, 0x00);
	nt35510_spi_write_reg(0xD40D, 0x9F);
	nt35510_spi_write_reg(0xD40E, 0x00);
	nt35510_spi_write_reg(0xD40F, 0xE2);
	nt35510_spi_write_reg(0xD410, 0x01);
	nt35510_spi_write_reg(0xD411, 0x12);
	nt35510_spi_write_reg(0xD412, 0x01);
	nt35510_spi_write_reg(0xD413, 0x57);
	nt35510_spi_write_reg(0xD414, 0x01);
	nt35510_spi_write_reg(0xD415, 0x88);
	nt35510_spi_write_reg(0xD416, 0x01);
	nt35510_spi_write_reg(0xD417, 0xCE);
	nt35510_spi_write_reg(0xD418, 0x02);
	nt35510_spi_write_reg(0xD419, 0x07);
	nt35510_spi_write_reg(0xD41A, 0x02);
	nt35510_spi_write_reg(0xD41B, 0x08);
	nt35510_spi_write_reg(0xD41C, 0x02);
	nt35510_spi_write_reg(0xD41D, 0x39);
	nt35510_spi_write_reg(0xD41E, 0x02);
	nt35510_spi_write_reg(0xD41F, 0x6C);
	nt35510_spi_write_reg(0xD420, 0x02);
	nt35510_spi_write_reg(0xD421, 0x87);
	nt35510_spi_write_reg(0xD422, 0x02);
	nt35510_spi_write_reg(0xD423, 0xA6);
	nt35510_spi_write_reg(0xD424, 0x02);
	nt35510_spi_write_reg(0xD425, 0xBA);
	nt35510_spi_write_reg(0xD426, 0x02);
	nt35510_spi_write_reg(0xD427, 0xD2);
	nt35510_spi_write_reg(0xD428, 0x02);
	nt35510_spi_write_reg(0xD429, 0xE2);
	nt35510_spi_write_reg(0xD42A, 0x02);
	nt35510_spi_write_reg(0xD42B, 0xF7);
	nt35510_spi_write_reg(0xD42C, 0x03);
	nt35510_spi_write_reg(0xD42D, 0x06);
	nt35510_spi_write_reg(0xD42E, 0x03);
	nt35510_spi_write_reg(0xD42F, 0x1E);
	nt35510_spi_write_reg(0xD430, 0x03);
	nt35510_spi_write_reg(0xD431, 0x55);
	nt35510_spi_write_reg(0xD432, 0x03);
	nt35510_spi_write_reg(0xD433, 0xFF);

	nt35510_spi_write_reg(0xD500, 0x00);
	nt35510_spi_write_reg(0xD501, 0x06);
	nt35510_spi_write_reg(0xD502, 0x00);
	nt35510_spi_write_reg(0xD503, 0x07);
	nt35510_spi_write_reg(0xD504, 0x00);
	nt35510_spi_write_reg(0xD505, 0x0E);
	nt35510_spi_write_reg(0xD506, 0x00);
	nt35510_spi_write_reg(0xD507, 0x22);
	nt35510_spi_write_reg(0xD508, 0x00);
	nt35510_spi_write_reg(0xD509, 0x3B);
	nt35510_spi_write_reg(0xD50A, 0x00);
	nt35510_spi_write_reg(0xD50B, 0x71);
	nt35510_spi_write_reg(0xD50C, 0x00);
	nt35510_spi_write_reg(0xD50D, 0x9F);
	nt35510_spi_write_reg(0xD50E, 0x00);
	nt35510_spi_write_reg(0xD50F, 0xE2);
	nt35510_spi_write_reg(0xD510, 0x01);
	nt35510_spi_write_reg(0xD511, 0x12);
	nt35510_spi_write_reg(0xD512, 0x01);
	nt35510_spi_write_reg(0xD513, 0x57);
	nt35510_spi_write_reg(0xD514, 0x01);
	nt35510_spi_write_reg(0xD515, 0x88);
	nt35510_spi_write_reg(0xD516, 0x01);
	nt35510_spi_write_reg(0xD517, 0xCE);
	nt35510_spi_write_reg(0xD518, 0x02);
	nt35510_spi_write_reg(0xD519, 0x07);
	nt35510_spi_write_reg(0xD51A, 0x02);
	nt35510_spi_write_reg(0xD51B, 0x08);
	nt35510_spi_write_reg(0xD51C, 0x02);
	nt35510_spi_write_reg(0xD51D, 0x39);
	nt35510_spi_write_reg(0xD51E, 0x02);
	nt35510_spi_write_reg(0xD51F, 0x6C);
	nt35510_spi_write_reg(0xD520, 0x02);
	nt35510_spi_write_reg(0xD521, 0x87);
	nt35510_spi_write_reg(0xD522, 0x02);
	nt35510_spi_write_reg(0xD523, 0xA6);
	nt35510_spi_write_reg(0xD524, 0x02);
	nt35510_spi_write_reg(0xD525, 0xBA);
	nt35510_spi_write_reg(0xD526, 0x02);
	nt35510_spi_write_reg(0xD527, 0xD2);
	nt35510_spi_write_reg(0xD528, 0x02);
	nt35510_spi_write_reg(0xD529, 0xE2);
	nt35510_spi_write_reg(0xD52A, 0x02);
	nt35510_spi_write_reg(0xD52B, 0xF7);
	nt35510_spi_write_reg(0xD52C, 0x03);
	nt35510_spi_write_reg(0xD52D, 0x06);
	nt35510_spi_write_reg(0xD52E, 0x03);
	nt35510_spi_write_reg(0xD52F, 0x1E);
	nt35510_spi_write_reg(0xD530, 0x03);
	nt35510_spi_write_reg(0xD531, 0x55);
	nt35510_spi_write_reg(0xD532, 0x03);
	nt35510_spi_write_reg(0xD533, 0xFF);

	nt35510_spi_write_reg(0xD600, 0x00);
	nt35510_spi_write_reg(0xD601, 0x06);
	nt35510_spi_write_reg(0xD602, 0x00);
	nt35510_spi_write_reg(0xD603, 0x07);
	nt35510_spi_write_reg(0xD604, 0x00);
	nt35510_spi_write_reg(0xD605, 0x0E);
	nt35510_spi_write_reg(0xD606, 0x00);
	nt35510_spi_write_reg(0xD607, 0x22);
	nt35510_spi_write_reg(0xD608, 0x00);
	nt35510_spi_write_reg(0xD609, 0x3B);
	nt35510_spi_write_reg(0xD60A, 0x00);
	nt35510_spi_write_reg(0xD60B, 0x71);
	nt35510_spi_write_reg(0xD60C, 0x00);
	nt35510_spi_write_reg(0xD60D, 0x9F);
	nt35510_spi_write_reg(0xD60E, 0x00);
	nt35510_spi_write_reg(0xD60F, 0xE2);
	nt35510_spi_write_reg(0xD610, 0x01);
	nt35510_spi_write_reg(0xD611, 0x12);
	nt35510_spi_write_reg(0xD612, 0x01);
	nt35510_spi_write_reg(0xD613, 0x57);
	nt35510_spi_write_reg(0xD614, 0x01);
	nt35510_spi_write_reg(0xD615, 0x88);
	nt35510_spi_write_reg(0xD616, 0x01);
	nt35510_spi_write_reg(0xD617, 0xCE);
	nt35510_spi_write_reg(0xD618, 0x02);
	nt35510_spi_write_reg(0xD619, 0x07);
	nt35510_spi_write_reg(0xD61A, 0x02);
	nt35510_spi_write_reg(0xD61B, 0x08);
	nt35510_spi_write_reg(0xD61C, 0x02);
	nt35510_spi_write_reg(0xD61D, 0x39);
	nt35510_spi_write_reg(0xD61E, 0x02);
	nt35510_spi_write_reg(0xD61F, 0x6C);
	nt35510_spi_write_reg(0xD620, 0x02);
	nt35510_spi_write_reg(0xD621, 0x87);
	nt35510_spi_write_reg(0xD622, 0x02);
	nt35510_spi_write_reg(0xD623, 0xA6);
	nt35510_spi_write_reg(0xD624, 0x02);
	nt35510_spi_write_reg(0xD625, 0xBA);
	nt35510_spi_write_reg(0xD626, 0x02);
	nt35510_spi_write_reg(0xD627, 0xD2);
	nt35510_spi_write_reg(0xD628, 0x02);
	nt35510_spi_write_reg(0xD629, 0xE2);
	nt35510_spi_write_reg(0xD62A, 0x02);
	nt35510_spi_write_reg(0xD62B, 0xF7);
	nt35510_spi_write_reg(0xD62C, 0x03);
	nt35510_spi_write_reg(0xD62D, 0x06);
	nt35510_spi_write_reg(0xD62E, 0x03);
	nt35510_spi_write_reg(0xD62F, 0x1E);
	nt35510_spi_write_reg(0xD630, 0x03);
	nt35510_spi_write_reg(0xD631, 0x55);
	nt35510_spi_write_reg(0xD632, 0x03);
	nt35510_spi_write_reg(0xD633, 0xFF);

	nt35510_spi_write_reg(0xF000, 0x55);	//goto page0
	nt35510_spi_write_reg(0xF001, 0xAA);
	nt35510_spi_write_reg(0xF002, 0x52);
	nt35510_spi_write_reg(0xF003, 0x08);
	nt35510_spi_write_reg(0xF004, 0x00);

	nt35510_spi_write_reg(0xB100, 0xCC);
	nt35510_spi_write_reg(0xB101, 0x00);

	nt35510_spi_write_reg(0xB600, 0x05);

	nt35510_spi_write_reg(0xB700, 0x70);
	nt35510_spi_write_reg(0xB701, 0x70);

	nt35510_spi_write_reg(0xB800, 0x01);
	nt35510_spi_write_reg(0xB801, 0x03);
	nt35510_spi_write_reg(0xB802, 0x03);
	nt35510_spi_write_reg(0xB803, 0x03);

	nt35510_spi_write_reg(0xBC00, 0x02);
	nt35510_spi_write_reg(0xBC01, 0x00);
	nt35510_spi_write_reg(0xBC02, 0x00);

	nt35510_spi_write_reg(0xC900, 0xD0);
	nt35510_spi_write_reg(0xC901, 0x02);
	nt35510_spi_write_reg(0xC902, 0x50);
	nt35510_spi_write_reg(0xC903, 0x50);
	nt35510_spi_write_reg(0xC904, 0x50);

	nt35510_spi_write_reg(0x3500, 0x00);
	nt35510_spi_write_reg(0x3A00, 0x55);
	nt35510_spi_write_reg(0x3600, 0x00);

	nt35510_spi_write_reg(0xF000, 0x55);	//start color enchange
	nt35510_spi_write_reg(0xF001, 0xAA);
	nt35510_spi_write_reg(0xF002, 0x52);
	nt35510_spi_write_reg(0xF003, 0x08);
	nt35510_spi_write_reg(0xF004, 0x00);

	nt35510_spi_write_reg(0xB400, 0x10);

	nt35510_spi_write_reg(0xFF00, 0xAA);
	nt35510_spi_write_reg(0xFF01, 0x55);
	nt35510_spi_write_reg(0xFF02, 0x25);
	nt35510_spi_write_reg(0xFF03, 0x01);

	nt35510_spi_write_reg(0XF900, 0x14);
	nt35510_spi_write_reg(0XF901, 0x00);
	nt35510_spi_write_reg(0XF902, 0x0D);
	nt35510_spi_write_reg(0XF903, 0x1A);
	nt35510_spi_write_reg(0XF904, 0x26);
	nt35510_spi_write_reg(0XF905, 0x33);
	nt35510_spi_write_reg(0XF906, 0x40);
	nt35510_spi_write_reg(0XF907, 0x4D);
	nt35510_spi_write_reg(0XF908, 0x5A);
	nt35510_spi_write_reg(0XF909, 0x66);
	nt35510_spi_write_reg(0XF90A, 0x73);	// end of color enchange
	panel_spi_send_cmd(0x1100);
	mdelay(200);
	panel_spi_send_cmd(0x2900);
	return 0;
}

static int nt35510_sleep(void)
{
	return 0;
}

static int nt35510_wakeup(void)
{
	return 0;
}

static int nt35510_set_active_win(struct gouda_rect *r)
{
	return 0;
}

static int nt35510_set_rotation(int rotate)
{
	return 0;
}

static int nt35510_close(void)
{
	return 0;
}

static struct rda_lcd_info nt35510_info = {
	.ops = {
		.s_init_gpio = nt35510_init_gpio,
		.s_open = nt35510_open,
		.s_active_win = nt35510_set_active_win,
		.s_rotation = nt35510_set_rotation,
		.s_sleep = nt35510_sleep,
		.s_wakeup = nt35510_wakeup,
		.s_close = nt35510_close},
	.lcd = {
		.width = WVGA_LCDD_DISP_X,
		.height = WVGA_LCDD_DISP_Y,
		.lcd_interface = GOUDA_LCD_IF_DPI,
		.lcd_timing = {
			       .rgb = NT35510_TIMING},
		.lcd_cfg = {
			    .rgb = NT35510_CONFIG}
		},
	.name = NT35510_PANEL_NAME,
};



static int rda_fb_panel_nt35510_probe(struct spi_device *spi)
{
	struct rda_spi_panel_device *panel_dev;
	int ret = 0;

	pr_info("%s \n",__func__);
	panel_dev = kzalloc(sizeof(*panel_dev), GFP_KERNEL);
	if (panel_dev == NULL) {
		dev_err(&spi->dev, "rda_fb_panel_nt35510, out of memory\n");
		return -ENOMEM;
	}

	spi->mode = SPI_MODE_0;
	panel_dev->spi = spi;
	panel_dev->spi_xfer_num = 0;
	dev_set_drvdata(&spi->dev, panel_dev);

	rda_nt35510_spi_dev = panel_dev;

	spi->bits_per_word = 8;
	spi->max_speed_hz = 1000000;
	spi->controller_data = &rgblcd_spicfg;

	ret = spi_setup(spi);

	if (ret < 0) {
		printk("error spi_setup failed\n");
		goto out_free_dev;
	}

	rda_fb_register_panel(&nt35510_info);
	pr_info("%s end\n",__func__);
	dev_info(&spi->dev, "rda panel nt35510 registered\n");
	return 0;
out_free_dev:
	kfree(panel_dev);
	panel_dev = NULL;
	return ret;
}

static int rda_fb_panel_nt35510_remove(struct spi_device *spi)
{
	struct rda_spi_panel_device *panel_dev;
	panel_dev = dev_get_drvdata(&spi->dev);
	kfree(panel_dev);
	return 0;
}

static struct spi_driver rda_fb_panel_nt35510_driver = {
	.driver = {
		   .name = NT35510_PANEL_NAME,
		   .owner = THIS_MODULE,
		   },
	.probe = rda_fb_panel_nt35510_probe,
	.remove = rda_fb_panel_nt35510_remove,
};

static struct rda_panel_driver nt35510_mcu_panel_driver = {
	.panel_type = GOUDA_LCD_IF_DPI,
	.lcd_driver_info = &nt35510_info,
	.rgb_panel_driver = &rda_fb_panel_nt35510_driver,
};
