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

#define ILI9806G_MCU_CHIP_ID			0x9806
#ifdef CONFIG_PM
#include <linux/regulator/consumer.h>
#include <mach/regulator.h>
#endif /* CONFIG_PM */

//static u8 vcom=0x3b;
static struct rda_lcd_info ILI9806g_mcu_info;
static struct rda_panel_id_param ILI9806g_id_param;

/* wngl, for FPGA */

#if 1//def ILI9806G_MCU_397_XCX_K_20150813
#define ILI9806G_MCU_TIMING {\
	{\
		.tas = 7,\
		.tah = 7,\
		.pwl = 7,\
		.pwh = 7\
	}\
}
#else
#define ILI9806G_MCU_TIMING {\
	{\
		.tas = 15,\
		.tah = 15,\
		.pwl = 16,\
		.pwh = 16\
	}\
}
#endif
 
#define ILI9806G_MCU_CONFIG {\
	{\
		.cs = GOUDA_LCD_CS_0,\
		.output_fmt = GOUDA_LCD_OUTPUT_FORMAT_16_bit_RGB565,\
		.cs0_polarity = false,\
		.cs1_polarity = false,\
		.rs_polarity = false,\
		.wr_polarity = false,\
		.rd_polarity = false,\
		.te_en       =   1,\
		.tecon2	     = 0x100\
	}\
}

#define delayms(_ms_) msleep(_ms_)

static struct rda_lcd_info ILI9806g_mcu_info;

static int ILI9806g_mcu_init_gpio(void)
{

	gpio_request(GPIO_LCD_RESET, "lcd reset");
	gpio_direction_output(GPIO_LCD_RESET, 1);
	mdelay(5);
	gpio_set_value(GPIO_LCD_RESET, 0);
	mdelay(10);
	gpio_set_value(GPIO_LCD_RESET, 1);
	mdelay(120);

	return 0;
}

static int ili9806g_mcu_readid_sub(void)
{
	u16 data[6] = {0};
	u32 cmd = 0xd3;

	//LCD_MCU_CMD(0x11);
	//mdelay(20);
	/* read id */
	//while(1) 
//{ 
	LCD_MCU_CMD(0xD3);
	panel_mcu_read_id(cmd,&ILI9806g_id_param,data);

	printk(KERN_INFO "rda_fb: ILI9806g_mcu_lyq_20131211 ID:"
	       "%02x %02x %02x %02x %02x %02x\n",
	       data[0], data[1], data[2], data[3], data[4], data[5]);
//}
	if(data[2] == 0x98 && data[3] == 0x06)
		return 1;
	return 0;

}

static int ILI9806g_mcu_readid(void)
{
	struct regulator *lcd_reg;
	struct gouda_lcd *lcd = (void *)&(ILI9806g_mcu_info.lcd);
	int ret = 0;

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

	rda_gouda_pre_enable_lcd(lcd,1);
	ILI9806g_mcu_init_gpio();
	ret = ili9806g_mcu_readid_sub();
	rda_gouda_pre_enable_lcd(lcd,0);

#ifdef CONFIG_PM
	if ( regulator_disable(lcd_reg)< 0) {
		printk(KERN_ERR"rda-fb lcd could not be enabled!\n");
		//return 0;
	}
#endif /* CONFIG_PM */

	return ret;
}



static int ILI9806g_mcu_open(void)
{
		unsigned int i;
		LCD_MCU_CMD(0xB9); //Set_EXTC
		LCD_MCU_DATA(0xFF);
		LCD_MCU_DATA(0x83);
		LCD_MCU_DATA(0x69);
		LCD_MCU_CMD(0xB1); //Set Power
		LCD_MCU_DATA(0x01);
		LCD_MCU_DATA(0x00);
		LCD_MCU_DATA(0x34);
		LCD_MCU_DATA(0x0A);
		LCD_MCU_DATA(0x00);
		LCD_MCU_DATA(0x11);
		LCD_MCU_DATA(0x12);
		LCD_MCU_DATA(0x1e);//
		LCD_MCU_DATA(0x1f);//
		LCD_MCU_DATA(0x3F);
		LCD_MCU_DATA(0x3F);
		LCD_MCU_DATA(0x01);
		LCD_MCU_DATA(0x1A);
		LCD_MCU_DATA(0x01);
		LCD_MCU_DATA(0xE6);
		LCD_MCU_DATA(0xE6);
		LCD_MCU_DATA(0xE6);
		LCD_MCU_DATA(0xE6);
		LCD_MCU_DATA(0xE6);

		LCD_MCU_CMD(0xB2); // SET Display 480x800
		LCD_MCU_DATA(0x00);
		LCD_MCU_DATA(0x20);  //0x2b;0x20-MCU;0x29-DPI;RM,DM; RM=0:DPI IF;  RM=1:RGB IF;
		LCD_MCU_DATA(0x03);
		LCD_MCU_DATA(0x03);
		LCD_MCU_DATA(0x70);
		LCD_MCU_DATA(0x00);
		LCD_MCU_DATA(0xFF);
		LCD_MCU_DATA(0x00);
		LCD_MCU_DATA(0x00);
		LCD_MCU_DATA(0x00);
		LCD_MCU_DATA(0x00);
		LCD_MCU_DATA(0x03);
		LCD_MCU_DATA(0x03);
		LCD_MCU_DATA(0x00);
		LCD_MCU_DATA(0x01);

		LCD_MCU_CMD(0xB4); // SET Display column inversion
		LCD_MCU_DATA(0x00);         // 2Dot inversion
		LCD_MCU_DATA(0x18);
		LCD_MCU_DATA(0x70);
		LCD_MCU_DATA(0x13);
		LCD_MCU_DATA(0x05);
		LCD_MCU_CMD(0xB6); // SET VCOM
		LCD_MCU_DATA(0x50);
		LCD_MCU_DATA(0x50);

		LCD_MCU_CMD(0xD5); //SET GIP
		LCD_MCU_DATA(0x00);  //SHR 8-11 
		LCD_MCU_DATA(0x01);  //SHR 0-7    6
		LCD_MCU_DATA(0x03);    //SHR1 8-11 
		LCD_MCU_DATA(0x25);  //SHR1 0-7    //reset 808
		LCD_MCU_DATA(0x01); //SPD    stv delay
		LCD_MCU_DATA(0x02); //CHR         8
		LCD_MCU_DATA(0x28); //CON     ck delay
		LCD_MCU_DATA(0x70); //COFF /////////
		LCD_MCU_DATA(0x11); //SHP  SCP  stv high 1 hsync  stv 周期
		LCD_MCU_DATA(0x13); //CHP  CCP  CK HIGH 1 HSYNC CK周期3
		LCD_MCU_DATA(0x00); //CGOUT10_L CGOUT9_L  ML=0 
		LCD_MCU_DATA(0x00); //CGOUT10_R   
		LCD_MCU_DATA(0x40); //CGOUT6_L CGOUT5_L    //40 
		LCD_MCU_DATA(0xe6); //CGOUT8_L CGOUT7_L   //26 
		LCD_MCU_DATA(0x51); //CGOUT6_R CGOUT5_R   //51 
		LCD_MCU_DATA(0xf7); //CGOUT8_R CGOUT7_R   //37 
		LCD_MCU_DATA(0x00); //CGOUT10_L CGOUT9_L ML=1   
		LCD_MCU_DATA(0x00); //CGOUT10_R CGOUT9_R   
		LCD_MCU_DATA(0x71); //CGOUT6_L CGOUT5_L   
		LCD_MCU_DATA(0x35); //CGOUT8_L CGOUT7_L   
		LCD_MCU_DATA(0x60); //CGOUT6_R CGOUT5_R   
		LCD_MCU_DATA(0x24); //CGOUT8_R CGOUT7_R 
		LCD_MCU_DATA(0x07);   // GTO
		LCD_MCU_DATA(0x0F);    // GNO
		LCD_MCU_DATA(0x04);    // EQ DELAY
		LCD_MCU_DATA(0x04);  // GIP 

		LCD_MCU_CMD(0xE0); //SET GAMMA
		LCD_MCU_DATA(0x00);
		LCD_MCU_DATA(0x02);
		LCD_MCU_DATA(0x0b);
		LCD_MCU_DATA(0x0a);
		LCD_MCU_DATA(0x09);
		LCD_MCU_DATA(0x18);
		LCD_MCU_DATA(0x1d);
		LCD_MCU_DATA(0x2a);
		LCD_MCU_DATA(0x08);
		LCD_MCU_DATA(0x11);
		LCD_MCU_DATA(0x0d);
		LCD_MCU_DATA(0x13);
		LCD_MCU_DATA(0x15);
		LCD_MCU_DATA(0x14);
		LCD_MCU_DATA(0x15);
		LCD_MCU_DATA(0x0f);
		LCD_MCU_DATA(0x14);
		LCD_MCU_DATA(0x00);
		LCD_MCU_DATA(0x02);
		LCD_MCU_DATA(0x0b);
		LCD_MCU_DATA(0x0a);
		LCD_MCU_DATA(0x09);
		LCD_MCU_DATA(0x18);
		LCD_MCU_DATA(0x1d);
		LCD_MCU_DATA(0x2a);
		LCD_MCU_DATA(0x08);
		LCD_MCU_DATA(0x11);
		LCD_MCU_DATA(0x0d);
		LCD_MCU_DATA(0x13);
		LCD_MCU_DATA(0x15);
		LCD_MCU_DATA(0x14);
		LCD_MCU_DATA(0x15);
		LCD_MCU_DATA(0x0f);
		LCD_MCU_DATA(0x14);

		LCD_MCU_CMD(0x35);
		LCD_MCU_DATA(0x00);

		LCD_MCU_CMD(0x36);
		LCD_MCU_DATA(0xC0);

		LCD_MCU_CMD(0x3A); //Set COLMOD
		LCD_MCU_DATA(0x55);



		LCD_MCU_CMD(0x2D); 
		for (i=0; i<=63; i++)               

		LCD_MCU_DATA(i*8);                

		for (i=0; i<=63; i++)              

		LCD_MCU_DATA(i*4);   

		for (i=0; i<=63; i++)          

		LCD_MCU_DATA(i*8);   

	    LCD_MCU_CMD(0x21);

		LCD_MCU_CMD(0x11); //Sleep Out
		mdelay(120);

		LCD_MCU_CMD(0x29); //Display On

		LCD_MCU_CMD(0x2C);
	
	return 0;
}




static int ILI9806g_mcu_sleep(void)
{

	return 0;
}



static int ILI9806g_mcu_wakeup(void)
{
	gpio_set_value(GPIO_LCD_RESET, 1);
	mdelay(5);
	gpio_set_value(GPIO_LCD_RESET, 0);
	mdelay(15);
	gpio_set_value(GPIO_LCD_RESET, 1);
	mdelay(120);
	ILI9806g_mcu_open();

	return 0;
}



static int ILI9806g_mcu_set_active_win(struct gouda_rect *r)
{
	    LCD_MCU_CMD(0x2a);  /* Set Column Address */
        LCD_MCU_DATA(r->tlX >> 8);
	    LCD_MCU_DATA(r->tlX & 0x00ff);
	    LCD_MCU_DATA((r->brX) >> 8);
	    LCD_MCU_DATA((r->brX) & 0x00ff);
	    LCD_MCU_CMD(0x2b);  /* Set Page Address */
	    LCD_MCU_DATA(r->tlY >> 8);
	    LCD_MCU_DATA(r->tlY & 0x00ff);
	    LCD_MCU_DATA((r->brY) >> 8);
	    LCD_MCU_DATA((r->brY) & 0x00ff);
	    LCD_MCU_CMD(0x2c);

	return 0;
}



static int ILI9806g_mcu_set_rotation(int rotate)
{
	return 0;
}

static int ILI9806g_mcu_close(void)
{
	return 0;
}

static struct rda_panel_id_param ILI9806g_id_param = {
	.lcd_info = &ILI9806g_mcu_info,
	.per_read_bytes = 4,
};

static struct rda_lcd_info ILI9806g_mcu_info = {
	.ops = {
		.s_init_gpio = ILI9806g_mcu_init_gpio,
		.s_open = ILI9806g_mcu_open,
		.s_readid = ILI9806g_mcu_readid,
		.s_active_win = ILI9806g_mcu_set_active_win,
		.s_rotation = ILI9806g_mcu_set_rotation,
		.s_sleep = ILI9806g_mcu_sleep,
		.s_wakeup = ILI9806g_mcu_wakeup,
		.s_close = ILI9806g_mcu_close
	},
	.lcd = {
		.width = WVGA_LCDD_DISP_X,
		.height = WVGA_LCDD_DISP_Y,
		.lcd_interface = GOUDA_LCD_IF_DBI,
		.lcd_timing = ILI9806G_MCU_TIMING,
		.lcd_cfg = ILI9806G_MCU_CONFIG
	},
	.name = ILI9806G_MCU_PANEL_NAME,
};

/*--------------------Platform Device Probe-------------------------*/

static int rda_fb_panel_ILI9806g_mcu_probe(struct platform_device *pdev)
{
	rda_fb_register_panel(&ILI9806g_mcu_info);

	dev_info(&pdev->dev, "rda panel ILI9806g_mcu registered\n");

	return 0;
}

static int rda_fb_panel_ILI9806g_mcu_remove(struct platform_device *pdev)
{
	return 0;
}

static struct platform_driver rda_fb_panel_ILI9806g_mcu_driver = {
	.probe = rda_fb_panel_ILI9806g_mcu_probe,
	.remove = rda_fb_panel_ILI9806g_mcu_remove,
	.driver = {
		.name = ILI9806G_MCU_PANEL_NAME
	}
};

static struct rda_panel_driver ili9806g_mcu_panel_driver = {
	.panel_type = GOUDA_LCD_IF_DBI,
	.lcd_driver_info = &ILI9806g_mcu_info,
	.pltaform_panel_driver = &rda_fb_panel_ILI9806g_mcu_driver,
};

static int __init rda_fb_panel_ILI9806g_mcu_init(void)
{
	rda_fb_probe_panel(&ILI9806g_mcu_info, &rda_fb_panel_ILI9806g_mcu_driver);
	return platform_driver_register(&rda_fb_panel_ILI9806g_mcu_driver);
}

static void __exit rda_fb_panel_ILI9806g_mcu_exit(void)
{
	platform_driver_unregister(&rda_fb_panel_ILI9806g_mcu_driver);
}

module_init(rda_fb_panel_ILI9806g_mcu_init);
module_exit(rda_fb_panel_ILI9806g_mcu_exit);
