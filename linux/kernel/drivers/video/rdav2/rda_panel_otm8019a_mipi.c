#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/jiffies.h>
#include <linux/gpio.h>

#include <plat/devices.h>
#include <plat/rda_debug.h>
#include <mach/board.h>

#include "rda_panel.h"
#include "rda_lcdc.h"
#include "rda_mipi_dsi.h"

static struct rda_lcd_info otm8019a_mipi_info;

/*mipi dsi phy 260MHz*/
static const struct rda_dsi_phy_ctrl otm8019a_pll_phy_260mhz = {
	{
		{0x6C, 0x8D}, {0x10C, 0x3}, {0x108,0x2}, {0x118, 0x4}, {0x11C, 0x0},
		{0x120, 0xC}, {0x124, 0x2}, {0x128, 0x3}, {0x80, 0xE}, {0x84, 0xC},
		{0x130, 0xC}, {0x150, 0x12}, {0x170, 0x87A},
	},
	{
		{0x64, 0x3}, {0x134, 0x3}, {0x138, 0xB}, {0x14C, 0x2C}, {0x13C, 0x7},
		{0x114, 0xB}, {0x170, 0x87A}, {0x140, 0xFF},
	},
};

static const char cmd0[]  = {0x00,0x00};
static const char cmd1[]  = {0xFF,0x80,0x19,0x01};
static const char cmd2[]  = {0x00,0x80};
static const char cmd3[]  = {0xFF,0x80,0x19};
static const char cmd4[]  = {0x00,0xB4};
static const char cmd5[]  = {0xC0,0x70};
//static const char cmd0[]  = {0x00,0xB5};
//static const char cmd0[]  = {0xC0,0x68};
static const char cmd6[]  = {0x00,0x81};
static const char cmd7[]  = {0xC5,0x66};
static const char cmd8[]  = {0x00,0x82};
static const char cmd9[]  = {0xC5,0xB0};
static const char cmd10[]  = {0x00,0x90};
static const char cmd11[]  = {0xC5,0x4E,0x76};
static const char cmd12[]  = {0x00,0x00};
static const char cmd13[]  = {0xD8,0x6F,0x6f};
static const char cmd14[]  = {0x00,0x00};
static const char cmd15[]  = {0xD9,0x6b};
static const char cmd17[]  = {0x00,0x81};
static const char cmd18[]  = {0xC1,0x33};
static const char cmd19[]  = {0x00,0xA1};
static const char cmd20[]  = {0xC1,0x08};
static const char cmd21[]  = {0x00,0x81};
static const char cmd22[]  = {0xC4,0x80};
static const char cmd23[]  = {0x00,0xB1};
static const char cmd24[]  = {0xC5,0xA9};
static const char cmd25[]  = {0x00,0x93};
static const char cmd26[]  = {0xC5,0x03};
static const char cmd27[]  = {0x00,0x92};
static const char cmd28[]  = {0xB3,0x40};
static const char cmd29[]  = {0x00,0x90};
static const char cmd30[]  = {0xB3,0x02};
static const char cmd31[]  = {0x00,0x80};
static const char cmd32[]  = {0xC0,0x00,0x58,0x00,0x15,0x15,0x00,
	0x58,0x15,0x15};
static const char cmd33[]  = {0x00,0x90};
static const char cmd34[]  = {0xC0,0x00,0x15,0x00,0x00,0x00,0x03};
static const char cmd35[]  = {0x00,0x80};
static const char cmd36[]  = {0xCE,0x8B,0x03,0x00,0x8A,0x03,0x00,
	0x89,0x03,0x00,0x88,0x03,0x00};
static const char cmd37[]  = {0x00,0xA0};
static const char cmd38[]  = {0xCE,0x38,0x07,0x03,0x54,0x00,0x02,
	0x00,0x38,0x06,0x03,0x55,0x00,0x02,0x00};
static const char cmd39[]  = {0x00,0xB0};
static const char cmd40[]  = {0xCE,0x38,0x05,0x03,0x56,0x00,0x02,
	0x00,0x38,0x04,0x03,0x57,0x00,0x02,0x00};
static const char cmd41[]  = {0x00,0xC0};
static const char cmd42[]  = {0xCE,0x38,0x03,0x03,0x58,0x00,0x02,
	0x00,0x38,0x02,0x03,0x59,0x00,0x02,0x00};
static const char cmd43[]  = {0x00,0xD0};
static const char cmd44[]  = {0xCE,0x38,0x01,0x03,0x5A,0x00,0x02,
	0x00,0x38,0x00,0x03,0x5B,0x00,0x02,0x00};
static const char cmd45[]  = {0x00,0xC0};
static const char cmd46[]  = {0xCF,0x02,0x02,0x00,0x00,0x00,0x00,
	0x01,0x00,0x00,0x00};
static const char cmd47[]  = {0x00,0x90};
static const char cmd48[]  = {0xCB,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
static const char cmd49[]  = {0x00,0xA0};
static const char cmd50[]  = {0xCB,0x00};
static const char cmd51[]  = {0x00,0xA5};
static const char cmd52[]  = {0xCB,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00};
static const char cmd53[]  = {0x00,0xB0};
static const char cmd54[]  = {0xCB,0x00,0x00,0x00,0x00,0x00,0x00};
static const char cmd55[]  = {0x00,0xC0};
static const char cmd56[]  = {0xCB,0x01,0x01,0x01,0x01,0x01,0x01,
	0x01,0x01,0x01,0x01,0x00,0x00,0x00,0x00,0x00};
static const char cmd57[]  = {0x00,0xD0};
static const char cmd58[]  = {0xCB,0x00};
static const char cmd59[]  = {0x00,0xD5};
static const char cmd60[]  = {0xCB,0x00,0x00,0x00,0x00,0x00,0x00,
	0x01,0x01,0x01,0x01};
static const char cmd61[]  = {0x00,0xE0};
static const char cmd62[]  = {0xCB,0x01,0x01,0x01,0x01,0x01,0x01};
static const char cmd63[]  = {0x00,0x80};
static const char cmd64[]  = {0xCC,0x26,0x25,0x21,0x22,0x0C,0x0A,
	0x10,0x0E,0x02,0x04};
static const char cmd65[]  = {0x00,0x90};
static const char cmd66[]  = {0xCC,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
static const char cmd67[]  = {0x00,0xA0};
static const char cmd68[]  = {0xCC,0x00,0x03,0x01,0x0D,0x0F,0x09,
	0x0B,0x22,0x21,0x25,0x26};
static const char cmd69[]  = {0x00,0xB0};
static const char cmd70[]  = {0xCC,0x25,0x26,0x21,0x22,0x10,0x0A,
	0x0C,0x0E,0x04,0x02};
static const char cmd71[]  = {0x00,0xC0};
static const char cmd72[]  = {0xCC,0x00,0x00,0x00,0x00,0x00,0x00};
static const char cmd73[]  = {0x00,0xCA};
static const char cmd74[]  = {0xCC,0x00,0x00,0x00,0x00,0x00};
static const char cmd75[]  = {0x00,0xD0};
static const char cmd76[]  = {0xCC,0x00,0x01,0x03,0x0D,0x0B,0x09,
	0x0F,0x22,0x21,0x26,0x25};
static const char cmd77[]  = {0x00,0x00};
static const char cmd78[]  = {0xE1,0x00,0x06,0x13,0x1D,0x2C,0x39,
	0x3B,0x6A,0x5B,0x77};
static const char cmd79[]  = {0xE1,0x94,0x7B,0x8A,0x60,0x5A,0x49,
	0x38,0x2A,0x19,0x00};
static const char cmd80[]  = {0x00,0x00};
static const char cmd81[]  = {0xE2,0x00,0x06,0x13,0x1D,0x2C,0x39,
	0x3B,0x6A,0x5B,0x77};
static const char cmd82[]  = {0xE2,0x94,0x7C,0x8A,0x60,0x5A,0x49,
	0x38,0x2A,0x19,0x00};
static const char cmd83[]  = {0x00,0x80};
static const char cmd84[]  = {0xC4,0x30};
static const char cmd85[]  = {0x00,0x98};
static const char cmd86[]  = {0xC0,0x00};
static const char cmd87[]  = {0x00,0xa9};
static const char cmd88[]  = {0xC0,0x06};
static const char cmd89[]  = {0x00,0xb0};
static const char cmd90[]  = {0xC1,0x20,0x00,0x00};
static const char cmd91[]  = {0x00,0xe1};
static const char cmd92[]  = {0xC0,0x40,0x18};
static const char cmd93[]  = {0x00,0x80};
static const char cmd94[]  = {0xC1,0x03,0x33};
static const char cmd95[]  = {0x00,0xA0};
static const char cmd96[]  = {0xC1,0xe8};
static const char cmd97[]  = {0x00,0x90};
static const char cmd98[]  = {0xb6,0xb4};
static const char cmd99[]  = {0x00,0x00};
static const char cmd100[]  = {0xfb,0x01};
static const char cmd101[]  = {0x00,0x00};
static const char cmd102[]  = {0x3A,0x55};
static const char cmd103[]  = {0x00,0x00};
static const char cmd104[]  = {0xFF,0xFF,0xFF,0xFF};

static const char exit_sleep[] = {0x11, 0x00};
static const char display_on[] = {0x29, 0x00};
static const char enter_sleep[] = {0x10, 0x00};
static const char display_off[] = {0x28, 0x00};

static const char ic_status[] = {0x0A, 0x00};
static const char max_pkt[] = {0x1, 0x00};

static const struct rda_dsi_cmd otm8019a_init_cmd_part[] = {
	{DTYPE_DCS_LWRITE,0,sizeof(cmd0),cmd0},
	{DTYPE_DCS_LWRITE,0,sizeof(cmd1),cmd1},
	{DTYPE_DCS_LWRITE,0,sizeof(cmd2),cmd2},
	{DTYPE_DCS_LWRITE,0,sizeof(cmd3),cmd3},
	{DTYPE_DCS_LWRITE,0,sizeof(cmd4),cmd4},
	{DTYPE_DCS_LWRITE,0,sizeof(cmd5),cmd5},
	{DTYPE_DCS_LWRITE,0,sizeof(cmd6),cmd6},
	{DTYPE_DCS_LWRITE,0,sizeof(cmd7),cmd7},
	{DTYPE_DCS_LWRITE,0,sizeof(cmd8),cmd8},
	{DTYPE_DCS_LWRITE,0,sizeof(cmd9),cmd9},
	{DTYPE_DCS_LWRITE,0,sizeof(cmd10),cmd10},
	{DTYPE_DCS_LWRITE,0,sizeof(cmd11),cmd11},
	{DTYPE_DCS_LWRITE,0,sizeof(cmd12),cmd12},
	{DTYPE_DCS_LWRITE,0,sizeof(cmd13),cmd13},
	{DTYPE_DCS_LWRITE,0,sizeof(cmd14),cmd14},
	{DTYPE_DCS_LWRITE,0,sizeof(cmd15),cmd15},
	//{DTYPE_DCS_LWRITE,0,sizeof(cmd16),cmd16},
	{DTYPE_DCS_LWRITE,0,sizeof(cmd17),cmd17},
	{DTYPE_DCS_LWRITE,0,sizeof(cmd18),cmd18},
	{DTYPE_DCS_LWRITE,0,sizeof(cmd19),cmd19},
	{DTYPE_DCS_LWRITE,0,sizeof(cmd20),cmd20},
	{DTYPE_DCS_LWRITE,0,sizeof(cmd21),cmd21},
	{DTYPE_DCS_LWRITE,0,sizeof(cmd22),cmd22},
	{DTYPE_DCS_LWRITE,0,sizeof(cmd23),cmd23},
	{DTYPE_DCS_LWRITE,0,sizeof(cmd24),cmd24},
	{DTYPE_DCS_LWRITE,0,sizeof(cmd25),cmd25},
	{DTYPE_DCS_LWRITE,0,sizeof(cmd26),cmd26},
	{DTYPE_DCS_LWRITE,0,sizeof(cmd27),cmd27},
	{DTYPE_DCS_LWRITE,0,sizeof(cmd28),cmd28},
	{DTYPE_DCS_LWRITE,0,sizeof(cmd29),cmd29},
	{DTYPE_DCS_LWRITE,0,sizeof(cmd30),cmd30},
	{DTYPE_DCS_LWRITE,0,sizeof(cmd31),cmd31},
	{DTYPE_DCS_LWRITE,0,sizeof(cmd32),cmd32},
	{DTYPE_DCS_LWRITE,0,sizeof(cmd33),cmd33},
	{DTYPE_DCS_LWRITE,0,sizeof(cmd34),cmd34},
	{DTYPE_DCS_LWRITE,0,sizeof(cmd35),cmd35},
	{DTYPE_DCS_LWRITE,0,sizeof(cmd36),cmd36},
	{DTYPE_DCS_LWRITE,0,sizeof(cmd37),cmd37},
	{DTYPE_DCS_LWRITE,0,sizeof(cmd38),cmd38},
	{DTYPE_DCS_LWRITE,0,sizeof(cmd39),cmd39},
	{DTYPE_DCS_LWRITE,0,sizeof(cmd40),cmd40},
	{DTYPE_DCS_LWRITE,0,sizeof(cmd41),cmd41},
	{DTYPE_DCS_LWRITE,0,sizeof(cmd42),cmd42},
	{DTYPE_DCS_LWRITE,0,sizeof(cmd43),cmd43},
	{DTYPE_DCS_LWRITE,0,sizeof(cmd44),cmd44},
	{DTYPE_DCS_LWRITE,0,sizeof(cmd45),cmd45},
	{DTYPE_DCS_LWRITE,0,sizeof(cmd46),cmd46},
	{DTYPE_DCS_LWRITE,0,sizeof(cmd47),cmd47},
	{DTYPE_DCS_LWRITE,0,sizeof(cmd48),cmd48},
	{DTYPE_DCS_LWRITE,0,sizeof(cmd49),cmd49},
	{DTYPE_DCS_LWRITE,0,sizeof(cmd50),cmd50},
	{DTYPE_DCS_LWRITE,0,sizeof(cmd51),cmd51},
	{DTYPE_DCS_LWRITE,0,sizeof(cmd52),cmd52},
	{DTYPE_DCS_LWRITE,0,sizeof(cmd53),cmd53},
	{DTYPE_DCS_LWRITE,0,sizeof(cmd54),cmd54},
	{DTYPE_DCS_LWRITE,0,sizeof(cmd55),cmd55},
	{DTYPE_DCS_LWRITE,0,sizeof(cmd56),cmd56},
	{DTYPE_DCS_LWRITE,0,sizeof(cmd57),cmd57},
	{DTYPE_DCS_LWRITE,0,sizeof(cmd58),cmd58},
	{DTYPE_DCS_LWRITE,0,sizeof(cmd59),cmd59},
	{DTYPE_DCS_LWRITE,0,sizeof(cmd60),cmd60},
	{DTYPE_DCS_LWRITE,0,sizeof(cmd61),cmd61},
	{DTYPE_DCS_LWRITE,0,sizeof(cmd62),cmd62},
	{DTYPE_DCS_LWRITE,0,sizeof(cmd63),cmd63},
	{DTYPE_DCS_LWRITE,0,sizeof(cmd64),cmd64},
	{DTYPE_DCS_LWRITE,0,sizeof(cmd65),cmd65},
	{DTYPE_DCS_LWRITE,0,sizeof(cmd66),cmd66},
	{DTYPE_DCS_LWRITE,0,sizeof(cmd67),cmd67},
	{DTYPE_DCS_LWRITE,0,sizeof(cmd68),cmd68},
	{DTYPE_DCS_LWRITE,0,sizeof(cmd69),cmd69},
	{DTYPE_DCS_LWRITE,0,sizeof(cmd70),cmd70},
	{DTYPE_DCS_LWRITE,0,sizeof(cmd71),cmd71},
	{DTYPE_DCS_LWRITE,0,sizeof(cmd72),cmd72},
	{DTYPE_DCS_LWRITE,0,sizeof(cmd73),cmd73},
	{DTYPE_DCS_LWRITE,0,sizeof(cmd74),cmd74},
	{DTYPE_DCS_LWRITE,0,sizeof(cmd75),cmd75},
	{DTYPE_DCS_LWRITE,0,sizeof(cmd76),cmd76},
	{DTYPE_DCS_LWRITE,0,sizeof(cmd77),cmd77},
	{DTYPE_DCS_LWRITE,0,sizeof(cmd78),cmd78},
	{DTYPE_DCS_LWRITE,0,sizeof(cmd79),cmd79},
	{DTYPE_DCS_LWRITE,0,sizeof(cmd80),cmd80},
	{DTYPE_DCS_LWRITE,0,sizeof(cmd81),cmd81},
	{DTYPE_DCS_LWRITE,0,sizeof(cmd82),cmd82},
	{DTYPE_DCS_LWRITE,0,sizeof(cmd83),cmd83},
	{DTYPE_DCS_LWRITE,0,sizeof(cmd84),cmd84},
	{DTYPE_DCS_LWRITE,0,sizeof(cmd85),cmd85},
	{DTYPE_DCS_LWRITE,0,sizeof(cmd86),cmd86},
	{DTYPE_DCS_LWRITE,0,sizeof(cmd87),cmd87},
	{DTYPE_DCS_LWRITE,0,sizeof(cmd88),cmd88},
	{DTYPE_DCS_LWRITE,0,sizeof(cmd89),cmd89},
	{DTYPE_DCS_LWRITE,0,sizeof(cmd90),cmd90},
	{DTYPE_DCS_LWRITE,0,sizeof(cmd91),cmd91},
	{DTYPE_DCS_LWRITE,0,sizeof(cmd92),cmd92},
	{DTYPE_DCS_LWRITE,0,sizeof(cmd93),cmd93},
	{DTYPE_DCS_LWRITE,0,sizeof(cmd94),cmd94},
	{DTYPE_DCS_LWRITE,0,sizeof(cmd95),cmd95},
	{DTYPE_DCS_LWRITE,0,sizeof(cmd96),cmd96},
	{DTYPE_DCS_LWRITE,0,sizeof(cmd97),cmd97},
	{DTYPE_DCS_LWRITE,0,sizeof(cmd98),cmd98},
	{DTYPE_DCS_LWRITE,0,sizeof(cmd99),cmd99},
	{DTYPE_DCS_LWRITE,0,sizeof(cmd100),cmd100},
	{DTYPE_DCS_LWRITE,0,sizeof(cmd101),cmd101},
	{DTYPE_DCS_LWRITE,0,sizeof(cmd102),cmd102},
	{DTYPE_DCS_LWRITE,0,sizeof(cmd103),cmd103},
	{DTYPE_DCS_LWRITE,0,sizeof(cmd104),cmd104},
};

static const struct rda_dsi_cmd exit_sleep_cmd[] = {
	{DTYPE_DCS_SWRITE,120,sizeof(exit_sleep),exit_sleep},
};

static const struct rda_dsi_cmd display_on_cmd[] = {
	{DTYPE_DCS_SWRITE,20,sizeof(display_on),display_on},
};

static const struct rda_dsi_cmd enter_sleep_cmd[] = {
	{DTYPE_DCS_SWRITE,120,sizeof(enter_sleep),enter_sleep},
};

static const struct rda_dsi_cmd display_off_cmd[] = {
	{DTYPE_DCS_SWRITE,20,sizeof(display_off),display_off},
};

static int otm8019a_mipi_reset_gpio(void)
{
	printk("%s\n",__func__);
	gpio_set_value(GPIO_LCD_RESET, 1);
	mdelay(5);
	gpio_set_value(GPIO_LCD_RESET, 0);
	mdelay(10);
	gpio_set_value(GPIO_LCD_RESET, 1);
	msleep(120);

	return 0;
}

static bool otm8019a_mipi_match_id(void)
{
	return true;
}

static int otm8019a_mipi_init_lcd(void)
{
	struct rda_dsi_transfer *trans;
	struct dsi_cmd_list *cmdlist;

	printk("%s\n",__func__);

	cmdlist = kzalloc(sizeof(struct dsi_cmd_list), GFP_KERNEL);
	if (!cmdlist) {
		pr_err("%s : no memory for cmdlist\n", __func__);
		return -ENOMEM;
	}
	cmdlist->cmds = otm8019a_init_cmd_part;
	cmdlist->cmds_cnt = ARRAY_SIZE(otm8019a_init_cmd_part);
	cmdlist->flags = DSI_LP_MODE;
	cmdlist->rxlen = 0;

	trans = kzalloc(sizeof(struct rda_dsi_transfer), GFP_KERNEL);
	if (!trans) {
		pr_err("%s : no memory for trans\n", __func__);
		goto err_no_memory;
	}
	trans->cmdlist = cmdlist;

	rda_dsi_cmdlist_send_lpkt(trans);

	/*display on*/
	rda_dsi_cmd_send(exit_sleep_cmd);
	rda_dsi_cmd_send(display_on_cmd);

	kfree(cmdlist);
	kfree(trans);

	return 0;
err_no_memory:
	kfree(cmdlist);
	return -ENOMEM;
}

static int otm8019a_mipi_sleep(void)
{
	printk("%s\n",__func__);

	mipi_dsi_switch_lp_mode();

	/*display off*/
	rda_dsi_cmd_send(display_off_cmd);
	rda_dsi_cmd_send(enter_sleep_cmd);

	return 0;
}

static int otm8019a_mipi_wakeup(void)
{
	printk("%s\n",__func__);

	/* as we go entire power off, we need to re-init the LCD */
	otm8019a_mipi_reset_gpio();
	otm8019a_mipi_init_lcd();

	return 0;
}

static int otm8019a_mipi_set_active_win(struct lcd_img_rect *r)
{
	return 0;
}

static int otm8019a_mipi_set_rotation(int rotate)
{
	return 0;
}

static int otm8019a_mipi_close(void)
{
	return 0;
}

static int rda_lcd_dbg_w(void *p,int n)
{
	return 0;
}


static struct rda_lcd_info otm8019a_mipi_info = {
	.name = OTM8019A_MIPI_PANEL_NAME,
	.ops = {
		.s_reset_gpio = otm8019a_mipi_reset_gpio,
		.s_open = otm8019a_mipi_init_lcd,
		.s_match_id = otm8019a_mipi_match_id,
		.s_active_win = otm8019a_mipi_set_active_win,
		.s_rotation = otm8019a_mipi_set_rotation,
		.s_sleep = otm8019a_mipi_sleep,
		.s_wakeup = otm8019a_mipi_wakeup,
		.s_close = otm8019a_mipi_close,
		.s_rda_lcd_dbg_w = rda_lcd_dbg_w},
	.lcd = {
		.width = FWVGA_LCDD_DISP_X,
		.height = FWVGA_LCDD_DISP_Y,
		.bpp = 16,
		.lcd_interface = LCD_IF_DSI,
		.mipi_pinfo = {
			.data_lane = 2,
			.vc = 0,
			.mipi_mode = DSI_VIDEO_MODE,
			.pixel_format = RGB_PIX_FMT_RGB565,
			.dsi_format = DSI_FMT_RGB565,
			.rgb_order = RGB_ORDER_BGR,
			.trans_mode = DSI_BURST,
			.bllp_enable = true,
			.h_sync_active = 0x6,
			.h_back_porch = 0x1c,
			.h_front_porch = 0x1e,
			.v_sync_active = 2,
			.v_back_porch = 0x10,
			.v_front_porch = 0xf,
			.vat_line = 854,
			.frame_rate = 60,
			.te_sel = true,
			.dsi_pclk_rate = 260,
			.debug_mode = 0,
			.dsi_phy_db = &otm8019a_pll_phy_260mhz,
		}
	},
};

/*--------------------Platform Device Probe-------------------------*/

static int rda_panel_otm8019a_mipi_probe(struct platform_device *pdev)
{
	rda_fb_register_panel(&otm8019a_mipi_info);

	dev_info(&pdev->dev, "rda panel otm8019a_mipi registered\n");

	return 0;
}

static int rda_panel_otm8019a_mipi_remove(struct platform_device *pdev)
{
	return 0;
}

static struct platform_driver rda_panel_otm8019a_mipi_driver = {
	.probe = rda_panel_otm8019a_mipi_probe,
	.remove = rda_panel_otm8019a_mipi_remove,
	.driver = {
		.name = OTM8019A_MIPI_PANEL_NAME}
};

static struct rda_panel_driver otm8019a_mipi_panel_driver = {
	.panel_type = LCD_IF_DSI,
	.lcd_driver_info = &otm8019a_mipi_info,
	.pltaform_panel_driver = &rda_panel_otm8019a_mipi_driver,
};

