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

static struct rda_lcd_info rm68172_mipi_info;

/*mipi dsi phy 300MHz*/
static const struct rda_dsi_phy_ctrl rm68172_pll_phy_300mhz = {
	{
		{0x6c,0x80},{0x10c,0x4},{0x108,0x1},{0x118,0xc},{0x11c,0x0},
		{0x120,0xd},{0x124,0x1},{0x128,0x4},{0x80,0x1a},{0x84,0xf},
		{0x130,0x11},{0x150,0xf},{0x170,0x087a},
	},
	{
		{0x64,0x5},{0x134,0x5},{0x138,0xe},{0x14c,0xe},{0x13c,0x9},
		{0x114,0x10},{0x170,0x087a},{0x140,0xff},
	},
};

static const char rm68172_cmd0[] = {0xF0, 0x55, 0xAA, 0x52, 0x08, 0x02};
static const char rm68172_cmd1[] = {0xF6, 0x60, 0x20};
static const char rm68172_cmd2[] = {0xFE, 0x01, 0x80, 0x09, 0x09};
static const char cmd_add2[] = {0xEB, 0x02};
static const char rm68172_cmd3[] = {0xF0, 0x55, 0xAA, 0x52, 0x08, 0x01};
static const char rm68172_cmd4[] = {0xB0, 0x0E};
static const char rm68172_cmd5[] = {0xB1, 0x0F};
static const char rm68172_cmd6[] = {0xB6, 0x34};
static const char rm68172_cmd7[] = {0xB7, 0x34};
static const char rm68172_cmd8[] = {0xB9, 0x34};
static const char rm68172_cmd9[] = {0xBA, 0x14};
static const char rm68172_cmd10[] = {0xBC, 0x00, 0x78, 0x00};
static const char rm68172_cmd11[] = {0xBD, 0x00, 0x78, 0x00};
static const char rm68172_cmd12[] = {0xBE, 0x00, 0x5D};
static const char rm68172_cmd13[] = {0xD1, 0x00, 0x00, 0x00, 0x10, 0x00, 0x20, 0x00, 0x30, 0x00, 0x50, 0x00, 0x60, 0x00, 0x80, 0x01, 0x1A, 0x01, 0x57, 0x01, 0x9F, 0x01, 0xCC, 0x02, 0x0B, 0x02, 0x3A, 0x02, 0x3B, 0x02, 0x5C, 0x02, 0x86, 0x02, 0x98, 0x02, 0xB7, 0x02, 0xC3, 0x02, 0xE0, 0x02, 0xE7, 0x02, 0xF8, 0x02, 0xFC, 0x03, 0x00, 0x03, 0x20, 0x03, 0xFF};
static const char rm68172_cmd14[] = {0xD2, 0x00, 0x00, 0x00, 0x10, 0x00, 0x20, 0x00, 0x30, 0x00, 0x50, 0x00, 0x60, 0x00, 0x80, 0x01, 0x1A, 0x01, 0x57, 0x01, 0x9F, 0x01, 0xCC, 0x02, 0x0B, 0x02, 0x3A, 0x02, 0x3B, 0x02, 0x5C, 0x02, 0x86, 0x02, 0x98, 0x02, 0xB7, 0x02, 0xC3, 0x02, 0xE0, 0x02, 0xE7, 0x02, 0xF8, 0x02, 0xFC, 0x03, 0x00, 0x03, 0x20, 0x03, 0xFF};
static const char rm68172_cmd15[] = {0xD3, 0x00, 0x00, 0x00, 0x10, 0x00, 0x20, 0x00, 0x30, 0x00, 0x50, 0x00, 0x60, 0x00, 0x80, 0x01, 0x1A, 0x01, 0x57, 0x01, 0x9F, 0x01, 0xCC, 0x02, 0x0B, 0x02, 0x3A, 0x02, 0x3B, 0x02, 0x5C, 0x02, 0x86, 0x02, 0x98, 0x02, 0xB7, 0x02, 0xC3, 0x02, 0xE0, 0x02, 0xE7, 0x02, 0xF8, 0x02, 0xFC, 0x03, 0x00, 0x03, 0x20, 0x03, 0xFF};
static const char rm68172_cmd16[] = {0xD4, 0x00, 0x00, 0x00, 0x10, 0x00, 0x20, 0x00, 0x30, 0x00, 0x50, 0x00, 0x60, 0x00, 0x80, 0x01, 0x1A, 0x01, 0x57, 0x01, 0x9F, 0x01, 0xCC, 0x02, 0x0B, 0x02, 0x3A, 0x02, 0x3B, 0x02, 0x5C, 0x02, 0x86, 0x02, 0x98, 0x02, 0xB7, 0x02, 0xC3, 0x02, 0xE0, 0x02, 0xE7, 0x02, 0xF8, 0x02, 0xFC, 0x03, 0x00, 0x03, 0x20, 0x03, 0xFF};
static const char rm68172_cmd17[] = {0xD5, 0x00, 0x00, 0x00, 0x10, 0x00, 0x20, 0x00, 0x30, 0x00, 0x50, 0x00, 0x60, 0x00, 0x80, 0x01, 0x1A, 0x01, 0x57, 0x01, 0x9F, 0x01, 0xCC, 0x02, 0x0B, 0x02, 0x3A, 0x02, 0x3B, 0x02, 0x5C, 0x02, 0x86, 0x02, 0x98, 0x02, 0xB7, 0x02, 0xC3, 0x02, 0xE0, 0x02, 0xE7, 0x02, 0xF8, 0x02, 0xFC, 0x03, 0x00, 0x03, 0x20, 0x03, 0xFF};
static const char rm68172_cmd18[] = {0xD6, 0x00, 0x00, 0x00, 0x10, 0x00, 0x20, 0x00, 0x30, 0x00, 0x50, 0x00, 0x60, 0x00, 0x80, 0x01, 0x1A, 0x01, 0x57, 0x01, 0x9F, 0x01, 0xCC, 0x02, 0x0B, 0x02, 0x3A, 0x02, 0x3B, 0x02, 0x5C, 0x02, 0x86, 0x02, 0x98, 0x02, 0xB7, 0x02, 0xC3, 0x02, 0xE0, 0x02, 0xE7, 0x02, 0xF8, 0x02, 0xFC, 0x03, 0x00, 0x03, 0x20, 0x03, 0xFF};
static const char rm68172_cmd19[] = {0xF0, 0x55, 0xAA, 0x52, 0x08, 0x03};
static const char rm68172_cmd20[] = {0xB0, 0x05, 0x17, 0xF9, 0x1D, 0x00, 0x00, 0x30};
static const char rm68172_cmd21[] = {0xB1, 0x05, 0x17, 0xFB, 0x1F, 0x00, 0x00, 0x30};
static const char rm68172_cmd22[] = {0xB2, 0xFC, 0xFD, 0xFE, 0xFF, 0xF0, 0x8A, 0x1A, 0xC4, 0x08};
static const char rm68172_cmd23[] = {0xB3, 0x5B, 0x00, 0xFC, 0x22, 0x22, 0x03};
static const char rm68172_cmd24[] = {0xB4, 0x00, 0x01, 0x02, 0x03, 0x00, 0x40, 0x04, 0x08, 0x8A, 0x1A, 0x00};
static const char rm68172_cmd25[] = {0xB5, 0x00, 0x44, 0xFF, 0x83, 0x28, 0x26, 0x5B, 0x5B, 0x33, 0x33, 0x55};
static const char rm68172_cmd26[] = {0xB6, 0x80, 0x00, 0x00, 0x00, 0x96, 0x81, 0x00};
static const char rm68172_cmd27[] = {0xB7, 0x00, 0x00, 0x16, 0x16, 0x16, 0x16, 0x00, 0x00};
static const char rm68172_cmd28[] = {0xB8, 0x12, 0x00, 0x00};
static const char rm68172_cmd29[] = {0xB9, 0x90};
static const char rm68172_cmd30[] = {0xBA, 0xFF, 0xF4, 0x08, 0xAC, 0xE2, 0x6F, 0xFF, 0xFF, 0xFF, 0xFF, 0xF7, 0x3F, 0xDB, 0x91, 0x5F, 0xFF};
static const char rm68172_cmd31[] = {0xBB, 0xFF, 0xF3, 0x7B, 0x9F, 0xD5, 0x1F, 0xFF, 0xFF, 0xFF, 0xFF, 0xF0, 0x4C, 0xE8, 0xA6, 0x2F, 0xFF};
static const char rm68172_cmd32[] = {0xBC, 0xE0, 0x1F, 0xF8, 0x07};
static const char rm68172_cmd33[] = {0xBD, 0xE0, 0x1F, 0xF8, 0x07};
static const char rm68172_cmd34[] = {0xF0, 0x55, 0xAA, 0x52, 0x08, 0x00};
static const char rm68172_cmd35[] = {0xB0, 0x00, 0x10};
static const char rm68172_cmd36[] = {0xB1, 0x78, 0x00};
static const char rm68172_cmd37[] = {0xB4, 0x10};
static const char rm68172_cmd38[] = {0xB5, 0x50};
static const char rm68172_cmd39[] = {0xBC, 0x02};
static const char rm68172_cmd40[] = {0x35, 0x00};
static const char rm68172_cmd41[] = {0x3A, 0x77};

static const char rm68172_exit_sleep[] = {0x11, 0x00};
static const char rm68172_display_on[] = {0x29, 0x00};
static const char rm68172_enter_sleep[] = {0x10, 0x00};
static const char rm68172_display_off[] = {0x28, 0x00};
//static const char all_pixel_off[] = {0x22, 0x00};

static const char rm68172_read_id[] = {0x04, 0x00};
static const char rm68172_id_max_pkt[] = {0x4, 0x00};
static const char rm68172_ic_status[] = {0x0a, 0x00};
static const char rm68172_ic_max_pkt[] = {0x1, 0x00};

static const struct rda_dsi_cmd rm68172_init_cmd_part[] = {
	{DTYPE_DCS_LWRITE,0,sizeof(rm68172_cmd0),rm68172_cmd0},
	{DTYPE_DCS_LWRITE,0,sizeof(rm68172_cmd1),rm68172_cmd1},
	{DTYPE_DCS_LWRITE,0,sizeof(rm68172_cmd2),rm68172_cmd2},
	{DTYPE_DCS_LWRITE,0,sizeof(cmd_add2),cmd_add2},
	{DTYPE_DCS_LWRITE,0,sizeof(rm68172_cmd3),rm68172_cmd3},
	{DTYPE_DCS_LWRITE,0,sizeof(rm68172_cmd4),rm68172_cmd4},
	{DTYPE_DCS_LWRITE,0,sizeof(rm68172_cmd5),rm68172_cmd5},
	{DTYPE_DCS_LWRITE,0,sizeof(rm68172_cmd6),rm68172_cmd6},
	{DTYPE_DCS_LWRITE,0,sizeof(rm68172_cmd7),rm68172_cmd7},
	{DTYPE_DCS_LWRITE,0,sizeof(rm68172_cmd8),rm68172_cmd8},
	{DTYPE_DCS_LWRITE,0,sizeof(rm68172_cmd9),rm68172_cmd9},
	{DTYPE_DCS_LWRITE,0,sizeof(rm68172_cmd10),rm68172_cmd10},
	{DTYPE_DCS_LWRITE,0,sizeof(rm68172_cmd11),rm68172_cmd11},
	{DTYPE_DCS_LWRITE,0,sizeof(rm68172_cmd12),rm68172_cmd12},
	{DTYPE_DCS_LWRITE,0,sizeof(rm68172_cmd13),rm68172_cmd13},
	{DTYPE_DCS_LWRITE,0,sizeof(rm68172_cmd14),rm68172_cmd14},
	{DTYPE_DCS_LWRITE,0,sizeof(rm68172_cmd15),rm68172_cmd15},
	{DTYPE_DCS_LWRITE,0,sizeof(rm68172_cmd16),rm68172_cmd16},
	{DTYPE_DCS_LWRITE,0,sizeof(rm68172_cmd17),rm68172_cmd17},
	{DTYPE_DCS_LWRITE,0,sizeof(rm68172_cmd18),rm68172_cmd18},
	{DTYPE_DCS_LWRITE,0,sizeof(rm68172_cmd19),rm68172_cmd19},
	{DTYPE_DCS_LWRITE,0,sizeof(rm68172_cmd20),rm68172_cmd20},
	{DTYPE_DCS_LWRITE,0,sizeof(rm68172_cmd21),rm68172_cmd21},
	{DTYPE_DCS_LWRITE,0,sizeof(rm68172_cmd22),rm68172_cmd22},
	{DTYPE_DCS_LWRITE,0,sizeof(rm68172_cmd23),rm68172_cmd23},
	{DTYPE_DCS_LWRITE,0,sizeof(rm68172_cmd24),rm68172_cmd24},
	{DTYPE_DCS_LWRITE,0,sizeof(rm68172_cmd25),rm68172_cmd25},
	{DTYPE_DCS_LWRITE,0,sizeof(rm68172_cmd26),rm68172_cmd26},
	{DTYPE_DCS_LWRITE,0,sizeof(rm68172_cmd27),rm68172_cmd27},
	{DTYPE_DCS_LWRITE,0,sizeof(rm68172_cmd28),rm68172_cmd28},
	{DTYPE_DCS_LWRITE,0,sizeof(rm68172_cmd29),rm68172_cmd29},
	{DTYPE_DCS_LWRITE,0,sizeof(rm68172_cmd30),rm68172_cmd30},
	{DTYPE_DCS_LWRITE,0,sizeof(rm68172_cmd31),rm68172_cmd31},
	{DTYPE_DCS_LWRITE,0,sizeof(rm68172_cmd32),rm68172_cmd32},
	{DTYPE_DCS_LWRITE,0,sizeof(rm68172_cmd33),rm68172_cmd33},
	{DTYPE_DCS_LWRITE,0,sizeof(rm68172_cmd34),rm68172_cmd34},
	{DTYPE_DCS_LWRITE,0,sizeof(rm68172_cmd35),rm68172_cmd35},
	{DTYPE_DCS_LWRITE,0,sizeof(rm68172_cmd36),rm68172_cmd36},
	{DTYPE_DCS_LWRITE,0,sizeof(rm68172_cmd37),rm68172_cmd37},
	{DTYPE_DCS_LWRITE,0,sizeof(rm68172_cmd38),rm68172_cmd38},
	{DTYPE_DCS_LWRITE,0,sizeof(rm68172_cmd39),rm68172_cmd39},
	{DTYPE_DCS_LWRITE,0,sizeof(rm68172_cmd40),rm68172_cmd40},
	{DTYPE_DCS_LWRITE,0,sizeof(rm68172_cmd41),rm68172_cmd41},
};

static const struct rda_dsi_cmd rm68172_exit_sleep_cmd[] = {
	{DTYPE_DCS_SWRITE,120,sizeof(rm68172_exit_sleep),rm68172_exit_sleep},
};

static const struct rda_dsi_cmd rm68172_display_on_cmd[] = {
	{DTYPE_DCS_SWRITE,20,sizeof(rm68172_display_on),rm68172_display_on},
};

static const struct rda_dsi_cmd rm68172_enter_sleep_cmd[] = {
	{DTYPE_DCS_SWRITE,120,sizeof(rm68172_enter_sleep),rm68172_enter_sleep},
};

static const struct rda_dsi_cmd rm68172_display_off_cmd[] = {
	{DTYPE_DCS_SWRITE,20,sizeof(rm68172_display_off),rm68172_display_off},
};

static const struct rda_dsi_cmd rm68172_read_id_cmd[] = {
	{DTYPE_MAX_PKTSIZE,0,sizeof(rm68172_id_max_pkt),rm68172_id_max_pkt},
	{DTYPE_DCS_READ,5,sizeof(rm68172_read_id),rm68172_read_id},
};

static const struct rda_dsi_cmd rm68172_ic_status_cmd[] = {
	{DTYPE_MAX_PKTSIZE,0,sizeof(rm68172_ic_max_pkt),rm68172_ic_max_pkt},
	{DTYPE_DCS_READ,5,sizeof(rm68172_ic_status),rm68172_ic_status},
};

static int rm68172_mipi_reset_gpio(void)
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

static bool rm68172_mipi_match_id(void)
{
	struct lcd_panel_info *lcd = (void *)&(rm68172_mipi_info.lcd);
	u8 id[4];

	rda_lcdc_pre_enable_lcd(lcd, 1);
	rm68172_mipi_reset_gpio();
	rda_dsi_spkt_cmds_read(rm68172_read_id_cmd, ARRAY_SIZE(rm68172_read_id_cmd));
	rda_dsi_read_data(id, 4);
	rda_lcdc_reset();

	printk("get id 0x%02x%02x%02x\n", id[0], id[1], id[2]);

	rda_lcdc_pre_enable_lcd(lcd, 0);

	if ((id[0] == 0x11) && (id[1] == 0x80) && (id[2] == 0x72))
		return true;

	return false;

}

static int rm68172_mipi_init_lcd(void)
{
	struct rda_dsi_transfer *trans;
	struct dsi_cmd_list *cmdlist;

	printk("%s\n",__func__);
	cmdlist = kzalloc(sizeof(struct dsi_cmd_list), GFP_KERNEL);
	if (!cmdlist) {
		pr_err("%s : no memory for cmdlist\n", __func__);
		return -ENOMEM;
	}
	cmdlist->cmds = rm68172_init_cmd_part;
	cmdlist->cmds_cnt = ARRAY_SIZE(rm68172_init_cmd_part);
	cmdlist->flags = DSI_LP_MODE;
	cmdlist->rxlen = 0;

	trans = kzalloc(sizeof(struct rda_dsi_transfer), GFP_KERNEL);
	if (!trans) {
		pr_err("%s : no memory for trans\n", __func__);
		goto err_no_memory;
	}
	trans->cmdlist = cmdlist;
	rda_dsi_cmdlist_send_lpkt(trans);

	rda_dsi_cmd_send(rm68172_exit_sleep_cmd);
	rda_dsi_cmd_send(rm68172_display_on_cmd);

	kfree(cmdlist);
	kfree(trans);

	return 0;
err_no_memory:
	kfree(cmdlist);
	return -ENOMEM;

}

static int rm68172_mipi_sleep(void)
{
	//struct rda_lcd_info *info = &rm68172_mipi_info;
	//info->lcd.mipi_pinfo.debug_mode = DBG_NEED_CHK;
	printk("%s\n",__func__);
	mipi_dsi_switch_lp_mode();
	/*display off*/
	rda_dsi_cmd_send(rm68172_enter_sleep_cmd);
	rda_dsi_cmd_send(rm68172_display_off_cmd);

	return 0;
}

static int rm68172_mipi_wakeup(void)
{
	/* as we go entire power off, we need to re-init the LCD */
	rm68172_mipi_reset_gpio();
	rm68172_mipi_init_lcd();

	return 0;
}

static int rm68172_mipi_set_active_win(struct lcd_img_rect *r)
{
	return 0;
}

static int rm68172_mipi_set_rotation(int rotate)
{
	return 0;
}

static int rm68172_mipi_close(void)
{
	return 0;
}

static int rm68172_lcd_dbg_w(void *p,int n)
{
	return 0;
}

static int rm68172_check_lcd(void *p)
{
	u8 i;
	u8 data[8] = {0};
	struct rda_lcd_info *info = (struct rda_lcd_info *)p;
	u32 len =1;

	mipi_dsi_switch_lp_mode();

	/*read id*/
	rda_dsi_spkt_cmds_read(rm68172_ic_status_cmd, ARRAY_SIZE(rm68172_ic_status_cmd));
	rda_dsi_read_data(data, len);
	printk("%s rm68172 ic_status ", __func__);

	for (i = 0; i < 8; i++) {
		printk("0x%x ",data[i]);
	}
	printk("\n");
	if (data[1] == 0x21) {
		info->lcd.mipi_pinfo.debug_mode = DBG_FIXED;
		return 0;
	} else if (data[0] == 0x02) {
		printk("MIPI DSI report ERR 0x02\n");
		adjust_dsi_phy_phase(0x14);
		return 1;
	}

	return 1;
}

static struct rda_lcd_info rm68172_mipi_info = {
	.name = RM68172_MIPI_PANEL_NAME,
	.ops = {
		.s_reset_gpio = rm68172_mipi_reset_gpio,
		.s_open = rm68172_mipi_init_lcd,
		.s_match_id = rm68172_mipi_match_id,
		.s_active_win = rm68172_mipi_set_active_win,
		.s_rotation = rm68172_mipi_set_rotation,
		.s_sleep = rm68172_mipi_sleep,
		.s_wakeup = rm68172_mipi_wakeup,
		.s_close = rm68172_mipi_close,
		.s_rda_lcd_dbg_w = rm68172_lcd_dbg_w,
		.s_check_lcd_state = rm68172_check_lcd},
	.lcd = {
		.width = WVGA_LCDD_DISP_X,
		.height = WVGA_LCDD_DISP_Y,
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
			.h_sync_active = 0xc,
			.h_back_porch = 0x1f,
			.h_front_porch = 0x2e,

			.v_sync_active = 2,
			.v_back_porch = 14,
			.v_front_porch = 16,
			.vat_line = 800,
			.frame_rate = 60,
			.te_sel = true,
			.dsi_pclk_rate = 300,
			.debug_mode = DBG_NO_USE,
			.dsi_phy_db = &rm68172_pll_phy_300mhz,
		}
	},
};

/*--------------------Platform Device Probe-------------------------*/

static int rda_panel_rm68172_mipi_probe(struct platform_device *pdev)
{
	rda_fb_register_panel(&rm68172_mipi_info);

	dev_info(&pdev->dev, "rda panel rm68172_mipi registered\n");

	return 0;
}

static int rda_panel_rm68172_mipi_remove(struct platform_device *pdev)
{
	return 0;
}

static struct platform_driver rda_panel_rm68172_mipi_driver = {
	.probe = rda_panel_rm68172_mipi_probe,
	.remove = rda_panel_rm68172_mipi_remove,
	.driver = {
		.name = RM68172_MIPI_PANEL_NAME}
};

static struct rda_panel_driver rm68172_mipi_panel_driver = {
	.panel_type = LCD_IF_DSI,
	.lcd_driver_info = &rm68172_mipi_info,
	.pltaform_panel_driver = &rda_panel_rm68172_mipi_driver,
};
