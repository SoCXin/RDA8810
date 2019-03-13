#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/jiffies.h>
#include <linux/gpio.h>

#include <plat/devices.h>
#include <plat/rda_debug.h>
#include <mach/board.h>

#include "rda_gouda.h"
#include "rda_panel.h"

#define NT35510_MCU_CHIP_ID			0x3AB6

/* wngl, for FPGA */
#define NT35510_MCU_TIMING {						     \
	{.tas       =  5,                                            \
	.tah        =  5,                                            \
	.pwl        =  7,                                           \
	.pwh        =  7}}                                          \

#define NT35510_MCU_CONFIG {                                             \
	{.cs             =   GOUDA_LCD_CS_0,                         \
	.output_fmt      =   GOUDA_LCD_OUTPUT_FORMAT_16_bit_RGB565,   \
	.cs0_polarity    =   false,                                  \
	.cs1_polarity    =   false,                                  \
	.rs_polarity     =   false,                                  \
	.wr_polarity     =   false,                                  \
	.rd_polarity     =   false,				     \
	.te_en           =   0,				     	     \
	.tecon2		 = 0x100}}

static int nt35510_mcu_init_gpio(void)
{
	int err;
	err = gpio_request(GPIO_LCD_RESET, "lcd reset");
	if(err)
		pr_info("%s gpio req fail!\n",__func__);

	gpio_direction_output(GPIO_LCD_RESET, 1);
	gpio_set_value(GPIO_LCD_RESET, 1);
	mdelay(10);
	gpio_set_value(GPIO_LCD_RESET, 0);
	mdelay(20);
	gpio_set_value(GPIO_LCD_RESET, 1);
	mdelay(100);

	return 0;
}

static void nt35510_mcu_readid(void)
{
	u8 data[6];

	/* read id */
	LCD_MCU_CMD(0x400);
	LCD_MCU_READ(&data[0]);
	LCD_MCU_READ(&data[1]);
	LCD_MCU_CMD(0x401);
	LCD_MCU_READ(&data[2]);
	LCD_MCU_READ(&data[3]);
	LCD_MCU_CMD(0x402);
	LCD_MCU_READ(&data[4]);
	LCD_MCU_READ(&data[5]);

	printk(KERN_INFO "rda_fb: nt35510_mcu ID:"
	       "%02x %02x %02x %02x %02x %02x\n",
	       data[0], data[1], data[2], data[3], data[4], data[5]);
}

static int nt35510_mcu_open(void)
{
	mdelay(100);
	nt35510_mcu_readid();
#if 1
	LCD_MCU_CMD(0xff00); LCD_MCU_DATA(0x80);
	LCD_MCU_CMD(0xff01); LCD_MCU_DATA(0x09);
	LCD_MCU_CMD(0xff02); LCD_MCU_DATA(0x01);
	LCD_MCU_CMD(0xff80); LCD_MCU_DATA(0x80);
	LCD_MCU_CMD(0xff81); LCD_MCU_DATA(0x09);
	LCD_MCU_CMD(0xff03); LCD_MCU_DATA(0x01);
	LCD_MCU_CMD(0xc5b1); LCD_MCU_DATA(0xA9);
	LCD_MCU_CMD(0xc591); LCD_MCU_DATA(0x0F);
	LCD_MCU_CMD(0xc0B4); LCD_MCU_DATA(0x50);

	LCD_MCU_CMD(0xE100); LCD_MCU_DATA(0x00);
	LCD_MCU_CMD(0xE101); LCD_MCU_DATA(0x09);
	LCD_MCU_CMD(0xE102); LCD_MCU_DATA(0x0F);
	LCD_MCU_CMD(0xE103); LCD_MCU_DATA(0x0E);
	LCD_MCU_CMD(0xE104); LCD_MCU_DATA(0x07);
	LCD_MCU_CMD(0xE105); LCD_MCU_DATA(0x10);
	LCD_MCU_CMD(0xE106); LCD_MCU_DATA(0x0B);
	LCD_MCU_CMD(0xE107); LCD_MCU_DATA(0x0A);
	LCD_MCU_CMD(0xE108); LCD_MCU_DATA(0x04);
	LCD_MCU_CMD(0xE109); LCD_MCU_DATA(0x07);
	LCD_MCU_CMD(0xE10A); LCD_MCU_DATA(0x0B);
	LCD_MCU_CMD(0xE10B); LCD_MCU_DATA(0x08);
	LCD_MCU_CMD(0xE10C); LCD_MCU_DATA(0x0F);
	LCD_MCU_CMD(0xE10D); LCD_MCU_DATA(0x10);
	LCD_MCU_CMD(0xE10E); LCD_MCU_DATA(0x0A);
	LCD_MCU_CMD(0xE10F); LCD_MCU_DATA(0x01);

	LCD_MCU_CMD(0xE200); LCD_MCU_DATA(0x00);
	LCD_MCU_CMD(0xE201); LCD_MCU_DATA(0x09);
	LCD_MCU_CMD(0xE202); LCD_MCU_DATA(0x0F);
	LCD_MCU_CMD(0xE203); LCD_MCU_DATA(0x0E);
	LCD_MCU_CMD(0xE204); LCD_MCU_DATA(0x07);
	LCD_MCU_CMD(0xE205); LCD_MCU_DATA(0x10);
	LCD_MCU_CMD(0xE206); LCD_MCU_DATA(0x0B);
	LCD_MCU_CMD(0xE207); LCD_MCU_DATA(0x0A);
	LCD_MCU_CMD(0xE208); LCD_MCU_DATA(0x04);
	LCD_MCU_CMD(0xE209); LCD_MCU_DATA(0x07);
	LCD_MCU_CMD(0xE20A); LCD_MCU_DATA(0x0B);
	LCD_MCU_CMD(0xE20B); LCD_MCU_DATA(0x08);
	LCD_MCU_CMD(0xE20C); LCD_MCU_DATA(0x0F);
	LCD_MCU_CMD(0xE20D); LCD_MCU_DATA(0x10);
	LCD_MCU_CMD(0xE20E); LCD_MCU_DATA(0x0A);
	LCD_MCU_CMD(0xE20F); LCD_MCU_DATA(0x01);

	LCD_MCU_CMD(0xD900); LCD_MCU_DATA(0x4E);
	LCD_MCU_CMD(0xc181); LCD_MCU_DATA(0x66);
	LCD_MCU_CMD(0xc1a1); LCD_MCU_DATA(0x08);
	LCD_MCU_CMD(0xc592); LCD_MCU_DATA(0x01);
	LCD_MCU_CMD(0xc595); LCD_MCU_DATA(0x34);
	LCD_MCU_CMD(0xd800); LCD_MCU_DATA(0x95);
	LCD_MCU_CMD(0xd801); LCD_MCU_DATA(0x95);
	LCD_MCU_CMD(0xc594); LCD_MCU_DATA(0x33);
	LCD_MCU_CMD(0xc0a3); LCD_MCU_DATA(0x1B);
	LCD_MCU_CMD(0xc582); LCD_MCU_DATA(0x83);
	LCD_MCU_CMD(0xc481); LCD_MCU_DATA(0x83);
	LCD_MCU_CMD(0xc1a1); LCD_MCU_DATA(0x0E);
	LCD_MCU_CMD(0xb3a6); LCD_MCU_DATA(0x20);
	LCD_MCU_CMD(0xb3a7); LCD_MCU_DATA(0x01);
	LCD_MCU_CMD(0xce80); LCD_MCU_DATA(0x85);
	LCD_MCU_CMD(0xce81); LCD_MCU_DATA(0x01);
	LCD_MCU_CMD(0xce82); LCD_MCU_DATA(0x00);
	LCD_MCU_CMD(0xce83); LCD_MCU_DATA(0x84);
	LCD_MCU_CMD(0xce84); LCD_MCU_DATA(0x01);
	LCD_MCU_CMD(0xce85); LCD_MCU_DATA(0x00);
	LCD_MCU_CMD(0xcea0); LCD_MCU_DATA(0x18);
	LCD_MCU_CMD(0xcea1); LCD_MCU_DATA(0x04);
	LCD_MCU_CMD(0xcea2); LCD_MCU_DATA(0x03);
	LCD_MCU_CMD(0xcea3); LCD_MCU_DATA(0x39);
	LCD_MCU_CMD(0xcea4); LCD_MCU_DATA(0x00);
	LCD_MCU_CMD(0xcea5); LCD_MCU_DATA(0x00);
	LCD_MCU_CMD(0xcea6); LCD_MCU_DATA(0x00);
	LCD_MCU_CMD(0xcea7); LCD_MCU_DATA(0x18);
	LCD_MCU_CMD(0xcea8); LCD_MCU_DATA(0x03);
	LCD_MCU_CMD(0xcea9); LCD_MCU_DATA(0x03);
	LCD_MCU_CMD(0xceaa); LCD_MCU_DATA(0x3a);
	LCD_MCU_CMD(0xceab); LCD_MCU_DATA(0x00);
	LCD_MCU_CMD(0xceac); LCD_MCU_DATA(0x00);
	LCD_MCU_CMD(0xcead); LCD_MCU_DATA(0x00);
	LCD_MCU_CMD(0xceb0); LCD_MCU_DATA(0x18);
	LCD_MCU_CMD(0xceb1); LCD_MCU_DATA(0x02);
	LCD_MCU_CMD(0xceb2); LCD_MCU_DATA(0x03);
	LCD_MCU_CMD(0xceb3); LCD_MCU_DATA(0x3b);
	LCD_MCU_CMD(0xceb4); LCD_MCU_DATA(0x00);
	LCD_MCU_CMD(0xceb5); LCD_MCU_DATA(0x00);
	LCD_MCU_CMD(0xceb6); LCD_MCU_DATA(0x00);
	LCD_MCU_CMD(0xceb7); LCD_MCU_DATA(0x18);
	LCD_MCU_CMD(0xceb8); LCD_MCU_DATA(0x01);
	LCD_MCU_CMD(0xceb9); LCD_MCU_DATA(0x03);
	LCD_MCU_CMD(0xceba); LCD_MCU_DATA(0x3c);
	LCD_MCU_CMD(0xcebb); LCD_MCU_DATA(0x00);
	LCD_MCU_CMD(0xcebc); LCD_MCU_DATA(0x00);
	LCD_MCU_CMD(0xcebd); LCD_MCU_DATA(0x00);
	LCD_MCU_CMD(0xcfc0); LCD_MCU_DATA(0x01);
	LCD_MCU_CMD(0xcfc1); LCD_MCU_DATA(0x01);
	LCD_MCU_CMD(0xcfc2); LCD_MCU_DATA(0x20);
	LCD_MCU_CMD(0xcfc3); LCD_MCU_DATA(0x20);
	LCD_MCU_CMD(0xcfc4); LCD_MCU_DATA(0x00);
	LCD_MCU_CMD(0xcfc5); LCD_MCU_DATA(0x00);
	LCD_MCU_CMD(0xcfc6); LCD_MCU_DATA(0x01);
	LCD_MCU_CMD(0xcfc7); LCD_MCU_DATA(0x00);
	LCD_MCU_CMD(0xcfc8); LCD_MCU_DATA(0x00);
	LCD_MCU_CMD(0xcfc9); LCD_MCU_DATA(0x00);
	LCD_MCU_CMD(0xcfd0); LCD_MCU_DATA(0x00);
	LCD_MCU_CMD(0xcb80); LCD_MCU_DATA(0x00);
	LCD_MCU_CMD(0xcb81); LCD_MCU_DATA(0x00);
	LCD_MCU_CMD(0xcb82); LCD_MCU_DATA(0x00);
	LCD_MCU_CMD(0xcb83); LCD_MCU_DATA(0x00);
	LCD_MCU_CMD(0xcb84); LCD_MCU_DATA(0x00);
	LCD_MCU_CMD(0xcb85); LCD_MCU_DATA(0x00);
	LCD_MCU_CMD(0xcb86); LCD_MCU_DATA(0x00);
	LCD_MCU_CMD(0xcb87); LCD_MCU_DATA(0x00);
	LCD_MCU_CMD(0xcb88); LCD_MCU_DATA(0x00);
	LCD_MCU_CMD(0xcb89); LCD_MCU_DATA(0x00);
	LCD_MCU_CMD(0xcb90); LCD_MCU_DATA(0x00);
	LCD_MCU_CMD(0xcb91); LCD_MCU_DATA(0x00);
	LCD_MCU_CMD(0xcb92); LCD_MCU_DATA(0x00);
	LCD_MCU_CMD(0xcb93); LCD_MCU_DATA(0x00);
	LCD_MCU_CMD(0xcb94); LCD_MCU_DATA(0x00);
	LCD_MCU_CMD(0xcb95); LCD_MCU_DATA(0x00);
	LCD_MCU_CMD(0xcb96); LCD_MCU_DATA(0x00);
	LCD_MCU_CMD(0xcb97); LCD_MCU_DATA(0x00);
	LCD_MCU_CMD(0xcb98); LCD_MCU_DATA(0x00);
	LCD_MCU_CMD(0xcb99); LCD_MCU_DATA(0x00);
	LCD_MCU_CMD(0xcb9a); LCD_MCU_DATA(0x00);
	LCD_MCU_CMD(0xcb9b); LCD_MCU_DATA(0x00);
	LCD_MCU_CMD(0xcb9c); LCD_MCU_DATA(0x00);
	LCD_MCU_CMD(0xcb9d); LCD_MCU_DATA(0x00);
	LCD_MCU_CMD(0xcb9e); LCD_MCU_DATA(0x00);
	LCD_MCU_CMD(0xcba0); LCD_MCU_DATA(0x00);
	LCD_MCU_CMD(0xcba1); LCD_MCU_DATA(0x00);
	LCD_MCU_CMD(0xcba2); LCD_MCU_DATA(0x00);
	LCD_MCU_CMD(0xcba3); LCD_MCU_DATA(0x00);
	LCD_MCU_CMD(0xcba4); LCD_MCU_DATA(0x00);
	LCD_MCU_CMD(0xcba5); LCD_MCU_DATA(0x00);
	LCD_MCU_CMD(0xcba6); LCD_MCU_DATA(0x00);
	LCD_MCU_CMD(0xcba7); LCD_MCU_DATA(0x00);
	LCD_MCU_CMD(0xcba8); LCD_MCU_DATA(0x00);
	LCD_MCU_CMD(0xcba9); LCD_MCU_DATA(0x00);
	LCD_MCU_CMD(0xcbaa); LCD_MCU_DATA(0x00);
	LCD_MCU_CMD(0xcbab); LCD_MCU_DATA(0x00);
	LCD_MCU_CMD(0xcbac); LCD_MCU_DATA(0x00);
	LCD_MCU_CMD(0xcbad); LCD_MCU_DATA(0x00);
	LCD_MCU_CMD(0xcbae); LCD_MCU_DATA(0x00);
	LCD_MCU_CMD(0xcbb0); LCD_MCU_DATA(0x00);
	LCD_MCU_CMD(0xcbb1); LCD_MCU_DATA(0x00);
	LCD_MCU_CMD(0xcbb2); LCD_MCU_DATA(0x00);
	LCD_MCU_CMD(0xcbb3); LCD_MCU_DATA(0x00);
	LCD_MCU_CMD(0xcbb4); LCD_MCU_DATA(0x00);
	LCD_MCU_CMD(0xcbb5); LCD_MCU_DATA(0x00);
	LCD_MCU_CMD(0xcbb6); LCD_MCU_DATA(0x00);
	LCD_MCU_CMD(0xcbb7); LCD_MCU_DATA(0x00);
	LCD_MCU_CMD(0xcbb8); LCD_MCU_DATA(0x00);
	LCD_MCU_CMD(0xcbb9); LCD_MCU_DATA(0x00);
	LCD_MCU_CMD(0xcbc0); LCD_MCU_DATA(0x00);
	LCD_MCU_CMD(0xcbc1); LCD_MCU_DATA(0x04);
	LCD_MCU_CMD(0xcbc2); LCD_MCU_DATA(0x04);
	LCD_MCU_CMD(0xcbc3); LCD_MCU_DATA(0x04);
	LCD_MCU_CMD(0xcbc4); LCD_MCU_DATA(0x04);
	LCD_MCU_CMD(0xcbc5); LCD_MCU_DATA(0x04);
	LCD_MCU_CMD(0xcbc6); LCD_MCU_DATA(0x00);
	LCD_MCU_CMD(0xcbc7); LCD_MCU_DATA(0x00);
	LCD_MCU_CMD(0xcbc8); LCD_MCU_DATA(0x00);
	LCD_MCU_CMD(0xcbc9); LCD_MCU_DATA(0x00);
	LCD_MCU_CMD(0xcbca); LCD_MCU_DATA(0x00);
	LCD_MCU_CMD(0xcbcb); LCD_MCU_DATA(0x00);
	LCD_MCU_CMD(0xcbcc); LCD_MCU_DATA(0x00);
	LCD_MCU_CMD(0xcbcd); LCD_MCU_DATA(0x00);
	LCD_MCU_CMD(0xcbce); LCD_MCU_DATA(0x00);
	LCD_MCU_CMD(0xcbd0); LCD_MCU_DATA(0x00);
	LCD_MCU_CMD(0xcbd1); LCD_MCU_DATA(0x00);
	LCD_MCU_CMD(0xcbd2); LCD_MCU_DATA(0x00);
	LCD_MCU_CMD(0xcbd3); LCD_MCU_DATA(0x00);
	LCD_MCU_CMD(0xcbd4); LCD_MCU_DATA(0x00);
	LCD_MCU_CMD(0xcbd5); LCD_MCU_DATA(0x00);
	LCD_MCU_CMD(0xcbd6); LCD_MCU_DATA(0x04);
	LCD_MCU_CMD(0xcbd7); LCD_MCU_DATA(0x04);
	LCD_MCU_CMD(0xcbd8); LCD_MCU_DATA(0x04);
	LCD_MCU_CMD(0xcbd9); LCD_MCU_DATA(0x04);
	LCD_MCU_CMD(0xcbda); LCD_MCU_DATA(0x04);
	LCD_MCU_CMD(0xcbdb); LCD_MCU_DATA(0x00);
	LCD_MCU_CMD(0xcbdc); LCD_MCU_DATA(0x00);
	LCD_MCU_CMD(0xcbdd); LCD_MCU_DATA(0x00);
	LCD_MCU_CMD(0xcbde); LCD_MCU_DATA(0x00);
	LCD_MCU_CMD(0xcbe0); LCD_MCU_DATA(0x00);
	LCD_MCU_CMD(0xcbe1); LCD_MCU_DATA(0x00);
	LCD_MCU_CMD(0xcbe2); LCD_MCU_DATA(0x00);
	LCD_MCU_CMD(0xcbe3); LCD_MCU_DATA(0x00);
	LCD_MCU_CMD(0xcbe4); LCD_MCU_DATA(0x00);
	LCD_MCU_CMD(0xcbe5); LCD_MCU_DATA(0x00);
	LCD_MCU_CMD(0xcbe6); LCD_MCU_DATA(0x00);
	LCD_MCU_CMD(0xcbe7); LCD_MCU_DATA(0x00);
	LCD_MCU_CMD(0xcbe8); LCD_MCU_DATA(0x00);
	LCD_MCU_CMD(0xcbe9); LCD_MCU_DATA(0x00);
	LCD_MCU_CMD(0xcbf0); LCD_MCU_DATA(0xFF);
	LCD_MCU_CMD(0xcbf1); LCD_MCU_DATA(0xFF);
	LCD_MCU_CMD(0xcbf2); LCD_MCU_DATA(0xFF);
	LCD_MCU_CMD(0xcbf3); LCD_MCU_DATA(0xFF);
	LCD_MCU_CMD(0xcbf4); LCD_MCU_DATA(0xFF);
	LCD_MCU_CMD(0xcbf5); LCD_MCU_DATA(0xFF);
	LCD_MCU_CMD(0xcbf6); LCD_MCU_DATA(0xFF);
	LCD_MCU_CMD(0xcbf7); LCD_MCU_DATA(0xFF);
	LCD_MCU_CMD(0xcbf8); LCD_MCU_DATA(0xFF);
	LCD_MCU_CMD(0xcbf9); LCD_MCU_DATA(0xFF);
	LCD_MCU_CMD(0xcc80); LCD_MCU_DATA(0x00);
	LCD_MCU_CMD(0xcc81); LCD_MCU_DATA(0x26);
	LCD_MCU_CMD(0xcc82); LCD_MCU_DATA(0x09);
	LCD_MCU_CMD(0xcc83); LCD_MCU_DATA(0x0B);
	LCD_MCU_CMD(0xcc84); LCD_MCU_DATA(0x01);
	LCD_MCU_CMD(0xcc85); LCD_MCU_DATA(0x25);
	LCD_MCU_CMD(0xcc86); LCD_MCU_DATA(0x00);
	LCD_MCU_CMD(0xcc87); LCD_MCU_DATA(0x00);
	LCD_MCU_CMD(0xcc88); LCD_MCU_DATA(0x00);
	LCD_MCU_CMD(0xcc89); LCD_MCU_DATA(0x00);
	LCD_MCU_CMD(0xcc90); LCD_MCU_DATA(0x00);
	LCD_MCU_CMD(0xcc91); LCD_MCU_DATA(0x00);
	LCD_MCU_CMD(0xcc92); LCD_MCU_DATA(0x00);
	LCD_MCU_CMD(0xcc93); LCD_MCU_DATA(0x00);
	LCD_MCU_CMD(0xcc94); LCD_MCU_DATA(0x00);
	LCD_MCU_CMD(0xcc95); LCD_MCU_DATA(0x00);
	LCD_MCU_CMD(0xcc96); LCD_MCU_DATA(0x00);
	LCD_MCU_CMD(0xcc97); LCD_MCU_DATA(0x00);
	LCD_MCU_CMD(0xcc98); LCD_MCU_DATA(0x00);
	LCD_MCU_CMD(0xcc99); LCD_MCU_DATA(0x00);
	LCD_MCU_CMD(0xcc9a); LCD_MCU_DATA(0x00);
	LCD_MCU_CMD(0xcc9b); LCD_MCU_DATA(0x26);
	LCD_MCU_CMD(0xcc9c); LCD_MCU_DATA(0x0A);
	LCD_MCU_CMD(0xcc9d); LCD_MCU_DATA(0x0C);
	LCD_MCU_CMD(0xcc9e); LCD_MCU_DATA(0x02);
	LCD_MCU_CMD(0xcca0); LCD_MCU_DATA(0x25);
	LCD_MCU_CMD(0xcca1); LCD_MCU_DATA(0x00);
	LCD_MCU_CMD(0xcca2); LCD_MCU_DATA(0x00);
	LCD_MCU_CMD(0xcca3); LCD_MCU_DATA(0x00);
	LCD_MCU_CMD(0xcca4); LCD_MCU_DATA(0x00);
	LCD_MCU_CMD(0xcca5); LCD_MCU_DATA(0x00);
	LCD_MCU_CMD(0xcca6); LCD_MCU_DATA(0x00);
	LCD_MCU_CMD(0xcca7); LCD_MCU_DATA(0x00);
	LCD_MCU_CMD(0xcca8); LCD_MCU_DATA(0x00);
	LCD_MCU_CMD(0xcca9); LCD_MCU_DATA(0x00);
	LCD_MCU_CMD(0xccaa); LCD_MCU_DATA(0x00);
	LCD_MCU_CMD(0xccab); LCD_MCU_DATA(0x00);
	LCD_MCU_CMD(0xccac); LCD_MCU_DATA(0x00);
	LCD_MCU_CMD(0xccad); LCD_MCU_DATA(0x00);
	LCD_MCU_CMD(0xccae); LCD_MCU_DATA(0x00);
	LCD_MCU_CMD(0xccb0); LCD_MCU_DATA(0x00);
	LCD_MCU_CMD(0xccb1); LCD_MCU_DATA(0x25);
	LCD_MCU_CMD(0xccb2); LCD_MCU_DATA(0x0C);
	LCD_MCU_CMD(0xccb3); LCD_MCU_DATA(0x0A);
	LCD_MCU_CMD(0xccb4); LCD_MCU_DATA(0x02);
	LCD_MCU_CMD(0xccb5); LCD_MCU_DATA(0x26);
	LCD_MCU_CMD(0xccb6); LCD_MCU_DATA(0x00);
	LCD_MCU_CMD(0xccb7); LCD_MCU_DATA(0x00);
	LCD_MCU_CMD(0xccb8); LCD_MCU_DATA(0x00);
	LCD_MCU_CMD(0xccb9); LCD_MCU_DATA(0x00);
	LCD_MCU_CMD(0xccc0); LCD_MCU_DATA(0x00);
	LCD_MCU_CMD(0xccc1); LCD_MCU_DATA(0x00);
	LCD_MCU_CMD(0xccc2); LCD_MCU_DATA(0x00);
	LCD_MCU_CMD(0xccc3); LCD_MCU_DATA(0x00);
	LCD_MCU_CMD(0xccc4); LCD_MCU_DATA(0x00);
	LCD_MCU_CMD(0xccc5); LCD_MCU_DATA(0x00);
	LCD_MCU_CMD(0xccc6); LCD_MCU_DATA(0x00);
	LCD_MCU_CMD(0xccc7); LCD_MCU_DATA(0x00);
	LCD_MCU_CMD(0xccc8); LCD_MCU_DATA(0x00);
	LCD_MCU_CMD(0xccc9); LCD_MCU_DATA(0x00);
	LCD_MCU_CMD(0xccca); LCD_MCU_DATA(0x00);
	LCD_MCU_CMD(0xcccb); LCD_MCU_DATA(0x25);
	LCD_MCU_CMD(0xcccc); LCD_MCU_DATA(0x0B);
	LCD_MCU_CMD(0xcccd); LCD_MCU_DATA(0x09);
	LCD_MCU_CMD(0xccce); LCD_MCU_DATA(0x01);
	LCD_MCU_CMD(0xccd0); LCD_MCU_DATA(0x26);
	LCD_MCU_CMD(0xccd1); LCD_MCU_DATA(0x00);
	LCD_MCU_CMD(0xccd2); LCD_MCU_DATA(0x00);
	LCD_MCU_CMD(0xccd3); LCD_MCU_DATA(0x00);
	LCD_MCU_CMD(0xccd4); LCD_MCU_DATA(0x00);
	LCD_MCU_CMD(0xccd5); LCD_MCU_DATA(0x00);
	LCD_MCU_CMD(0xccd6); LCD_MCU_DATA(0x00);
	LCD_MCU_CMD(0xccd7); LCD_MCU_DATA(0x00);
	LCD_MCU_CMD(0xccd8); LCD_MCU_DATA(0x00);
	LCD_MCU_CMD(0xccd9); LCD_MCU_DATA(0x00);
	LCD_MCU_CMD(0xccda); LCD_MCU_DATA(0x00);
	LCD_MCU_CMD(0xccdb); LCD_MCU_DATA(0x00);
	LCD_MCU_CMD(0xccdc); LCD_MCU_DATA(0x00);
	LCD_MCU_CMD(0xccdd); LCD_MCU_DATA(0x00);
	LCD_MCU_CMD(0xccde); LCD_MCU_DATA(0x00);

	LCD_MCU_CMD(0xff00); LCD_MCU_DATA(0xff);
	LCD_MCU_CMD(0xff01); LCD_MCU_DATA(0xff);
	LCD_MCU_CMD(0xff02); LCD_MCU_DATA(0xff);

#ifdef DISPLAY_DIRECTION_0_MODE
	LCD_MCU_CMD(0x3600);  LCD_MCU_DATA(0x00);// Display Direction 0
	LCD_MCU_CMD(0x3500);  LCD_MCU_DATA(0x00);// TE( Fmark ) Signal On
	LCD_MCU_CMD(0x4400);  LCD_MCU_DATA(0x01);
	LCD_MCU_CMD(0x4401);  LCD_MCU_DATA(0x22);// TE( Fmark ) Signal Output Position
#endif

#ifdef DISPLAY_DIRECTION_180_MODE
	LCD_MCU_CMD(0x3600);  LCD_MCU_DATA(0xD0);// Display Direction 180
	LCD_MCU_CMD(0x3500);  LCD_MCU_DATA(0x00);// TE( Fmark ) Signal On
	LCD_MCU_CMD(0x4400);  LCD_MCU_DATA(0x01);
	LCD_MCU_CMD(0x4401);  LCD_MCU_DATA(0xFF);// TE( Fmark ) Signal Output Position
#endif

#ifdef LCD_BACKLIGHT_CONTROL_MODE
	LCD_MCU_CMD(0x5100);  LCD_MCU_DATA(0xFF);// Backlight Level Control
	LCD_MCU_CMD(0x5300);  LCD_MCU_DATA(0x2C);// Backlight On
	LCD_MCU_CMD(0x5500);  LCD_MCU_DATA(0x00);// CABC Function Off
#endif

	LCD_MCU_CMD(0x3A00); LCD_MCU_DATA(0x55);

#endif

	LCD_MCU_CMD( 0x1100);
	mdelay (20);
	LCD_MCU_CMD( 0x2900);

	mdelay(100);

	LCD_MCU_CMD(0x2A00); LCD_MCU_DATA(0x00);
	LCD_MCU_CMD(0x2A01); LCD_MCU_DATA(0x00);
	LCD_MCU_CMD(0x2A02); LCD_MCU_DATA(0x01);
	LCD_MCU_CMD(0x2A03); LCD_MCU_DATA(0xDF);
	LCD_MCU_CMD(0x2B00); LCD_MCU_DATA(0x00);
	LCD_MCU_CMD(0x2B01); LCD_MCU_DATA(0x00);
	LCD_MCU_CMD(0x2B02); LCD_MCU_DATA(0x03);
	LCD_MCU_CMD(0x2B03); LCD_MCU_DATA(0x1F);

	LCD_MCU_CMD(0x2C00);
	return 0;
}

static int nt35510_mcu_display_off(void)
{
	LCD_MCU_CMD( 0x2800);
	return 0;
}

static int nt35510_mcu_display_on(void)
{
	LCD_MCU_CMD( 0x2900);
	LCD_MCU_CMD(0x2C00);
	return 0;
}

static int nt35510_mcu_set_active_win(struct gouda_rect *r)
{
	return 0;
}

static int nt35510_mcu_set_rotation(int rotate)
{
	return 0;
}

static int nt35510_mcu_sleep(void)
{
	//LCD_MCU_CMD(0x10);
	gpio_set_value(GPIO_LCD_RESET, 0);
	return 0;
}

static int nt35510_mcu_wakeup(void)
{
	/* re-init */
	gpio_set_value(GPIO_LCD_RESET, 0);
	mdelay(2);
	gpio_set_value(GPIO_LCD_RESET, 1);
	mdelay(1);

	nt35510_mcu_open();

	return 0;
}


static int nt35510_mcu_close(void)
{
	return 0;
}

static void nt35510_mcu_read_fb(u8 *buffer)
{
	int i = 0;
	short x0, y0, x1, y1;
	short h_X_start,l_X_start,h_X_end,l_X_end,h_Y_start,l_Y_start,h_Y_end,l_Y_end;
	u8 readData[2];

	rda_gouda_stretch_pre_wait_and_enable_clk();

	x0 = 0;
	y0 = 0;
	x1 = WVGA_LCDD_DISP_X-1;
	y1 = WVGA_LCDD_DISP_Y-1;

	h_X_start=((x0&0x0300)>>8);
	l_X_start=(x0&0x00FF);
	h_X_end=((x1&0x0300)>>8);
	l_X_end=(x1&0x00FF);

	h_Y_start=((y0&0x0300)>>8);
	l_Y_start=(y0&0x00FF);
	h_Y_end=((y1&0x0300)>>8);
	l_Y_end=(y1&0x00FF);

	LCD_MCU_CMD(0x2A00);
	LCD_MCU_DATA(h_X_start);
	LCD_MCU_CMD(0x2A01);
	LCD_MCU_DATA(l_X_start);
	LCD_MCU_CMD(0x2A02);
	LCD_MCU_DATA(h_X_end);
	LCD_MCU_CMD(0x2A03);
	LCD_MCU_DATA(l_X_end);
	LCD_MCU_CMD(0x2B00);
	LCD_MCU_DATA(h_Y_start);
	LCD_MCU_CMD(0x2B01);
	LCD_MCU_DATA(l_Y_start);
	LCD_MCU_CMD(0x2B02);
	LCD_MCU_DATA(h_Y_end);
	LCD_MCU_CMD(0x2B03);
	LCD_MCU_DATA(l_Y_end);
	LCD_MCU_CMD(0x2E00);

	mdelay(20);

	for(i=0; i<60; i++)
	{
		LCD_MCU_READ(&readData[0]);
		LCD_MCU_READ(&readData[1]);
	}

	LCD_MCU_CMD(0x2A00);
	LCD_MCU_DATA(h_X_start);
	LCD_MCU_CMD(0x2A01);
	LCD_MCU_DATA(l_X_start);
	LCD_MCU_CMD(0x2A02);
	LCD_MCU_DATA(h_X_end);
	LCD_MCU_CMD(0x2A03);
	LCD_MCU_DATA(l_X_end);
	LCD_MCU_CMD(0x2B00);
	LCD_MCU_DATA(h_Y_start);
	LCD_MCU_CMD(0x2B01);
	LCD_MCU_DATA(l_Y_start);
	LCD_MCU_CMD(0x2B02);
	LCD_MCU_DATA(h_Y_end);
	LCD_MCU_CMD(0x2B03);
	LCD_MCU_DATA(l_Y_end);
	LCD_MCU_CMD(0x2C00);

	rda_gouda_stretch_post_disable_clk();
}
static struct rda_lcd_info nt35510_mcu_info = {
	.ops = {
		.s_init_gpio = nt35510_mcu_init_gpio,
		.s_open = nt35510_mcu_open,
		.s_active_win = nt35510_mcu_set_active_win,
		.s_rotation = nt35510_mcu_set_rotation,
		.s_sleep = nt35510_mcu_sleep,
		.s_wakeup = nt35510_mcu_wakeup,
		.s_display_on = nt35510_mcu_display_on,
		.s_display_off = nt35510_mcu_display_off,
		.s_close = nt35510_mcu_close,
		.s_read_fb = nt35510_mcu_read_fb},
	.lcd = {
		.width = WVGA_LCDD_DISP_X,
		.height = WVGA_LCDD_DISP_Y,
		.lcd_interface = GOUDA_LCD_IF_DBI,
		.lcd_timing = NT35510_MCU_TIMING,
		.lcd_cfg = NT35510_MCU_CONFIG},
	.name = NT35510_MCU_PANEL_NAME,
};

/*--------------------Platform Device Probe-------------------------*/

static int rda_fb_panel_nt35510_mcu_probe(struct platform_device *pdev)
{
	rda_fb_register_panel(&nt35510_mcu_info);
	dev_info(&pdev->dev, "rda panel nt35510_mcu registered\n");
	pr_info("%s \n",__func__);

	return 0;
}

static int rda_fb_panel_nt35510_mcu_remove(struct platform_device *pdev)
{
	return 0;
}

static struct platform_driver rda_fb_panel_nt35510_mcu_driver = {
	.probe = rda_fb_panel_nt35510_mcu_probe,
	.remove = rda_fb_panel_nt35510_mcu_remove,
	.driver = {
		   .name = NT35510_MCU_PANEL_NAME}
};

static struct rda_panel_driver nt35510_mcu_panel_driver = {
	.panel_type = GOUDA_LCD_IF_DBI,
	.lcd_driver_info = &nt35510_mcu_info,
	.pltaform_panel_driver = &rda_fb_panel_nt35510_mcu_driver,
};
