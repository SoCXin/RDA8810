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

static struct rda_lcd_info st7796s_boe35_mipi_info;

/*mipi dsi phy 250MHz*/
static const struct rda_dsi_phy_ctrl st7796s_boe35_pll_phy_250mhz = {
	{
		{0x6c,0xF8},{0x10c,0x1},{0x108,0x2},{0x118,0x4},{0x11c,0x0},
		{0x120,0xC},{0x124,0x2},{0x128,0x1},{0x80,0xC},{0x84,0xC},
		{0x130,0xC},{0x150,0x12},{0x170,0x087a},
	},
	{	{0x64,0x1},{0x134,0x1},{0x138,0x5},{0x14c,0x14},{0x13c,0x3},
		{0x114,0x5},{0x170,0x087a},{0x140,0xff},
	},
};

static const char st7796s_boe35_cmd0[] = {0xF0, 0xC3};

static const char st7796s_boe35_cmd1[] = {0xF0, 0x96};

static const char st7796s_boe35_cmd2[] = {0x36, 0x48};

static const char st7796s_boe35_cmd3[] = {0x3a, 0x55};

static const char st7796s_boe35_cmd4[] = {0xB4, 0x01};

static const char st7796s_boe35_cmd5[] = {0xB7, 0xC6};

static const char st7796s_boe35_cmd6[] = {0xE8, 0x40, 0x82, 0x07, 0x18, 0x27, 0x0A, 0xB6, 0x33};

static const char st7796s_boe35_cmd7[] = {0xC2, 0xA7};

static const char st7796s_boe35_cmd8[] = {0xC5, 0x0B};

static const char st7796s_boe35_cmd9[] = {0xE0, 0xF0, 0x07, 0x11, 0x14, 0x16, 0x0C, 0x42, 0x55, 0x50, 0x0B, 0x16, 0x16, 0x20, 0x23};

static const char st7796s_boe35_cmd10[] = {0xE1, 0xF0, 0x06, 0x11, 0x13, 0x14, 0x1C, 0x42, 0x54, 0x51, 0x0B, 0x16, 0x15, 0x20, 0x22};

static const char st7796s_boe35_cmd11[] = {0xF0, 0x3C};

static const char st7796s_boe35_cmd12[] = {0xF0, 0x69};

static const char st7796s_boe35_exit_sleep[] = {0x11, 0x00};

static const char st7796s_boe35_display_on[] = {0x29, 0x00};

static const char st7796s_boe35_enter_sleep[] = {0x10, 0x00};

static const char st7796s_boe35_display_off[] = {0x28, 0x00};

static const char st7796s_boe35_read_id[] = {0xd3, 0x00};

static const char st7796s_boe35_max_pkt[] = {0x4, 0x00};

static const char st7796s_boe35_ic_status[] = {0x0a, 0x00};

static const char st7796s_boe35_ic_max_pkt[] = {0x1, 0x00};


//static char pixel_off[] = {0x21, 0x00};

static const struct rda_dsi_cmd st7796s_boe35_init_cmd_part[] = {
	{DTYPE_DCS_LWRITE,0,sizeof(st7796s_boe35_cmd0),st7796s_boe35_cmd0},
	{DTYPE_DCS_LWRITE,0,sizeof(st7796s_boe35_cmd1),st7796s_boe35_cmd1},
	{DTYPE_DCS_LWRITE,0,sizeof(st7796s_boe35_cmd2),st7796s_boe35_cmd2},
	{DTYPE_DCS_LWRITE,0,sizeof(st7796s_boe35_cmd3),st7796s_boe35_cmd3},
	{DTYPE_DCS_LWRITE,0,sizeof(st7796s_boe35_cmd4),st7796s_boe35_cmd4},
	{DTYPE_DCS_LWRITE,0,sizeof(st7796s_boe35_cmd5),st7796s_boe35_cmd5},
	{DTYPE_DCS_LWRITE,0,sizeof(st7796s_boe35_cmd6),st7796s_boe35_cmd6},
	{DTYPE_DCS_LWRITE,0,sizeof(st7796s_boe35_cmd7),st7796s_boe35_cmd7},
	{DTYPE_DCS_LWRITE,0,sizeof(st7796s_boe35_cmd8),st7796s_boe35_cmd8},
	{DTYPE_DCS_LWRITE,0,sizeof(st7796s_boe35_cmd9),st7796s_boe35_cmd9},
	{DTYPE_DCS_LWRITE,0,sizeof(st7796s_boe35_cmd10),st7796s_boe35_cmd10},
	{DTYPE_DCS_LWRITE,0,sizeof(st7796s_boe35_cmd11),st7796s_boe35_cmd11},
	{DTYPE_DCS_LWRITE,0,sizeof(st7796s_boe35_cmd12),st7796s_boe35_cmd12},
};

static const struct rda_dsi_cmd st7796s_boe35_exit_sleep_cmd[] = {
	{DTYPE_DCS_SWRITE,120,sizeof(st7796s_boe35_exit_sleep),st7796s_boe35_exit_sleep},
};

static const struct rda_dsi_cmd st7796s_boe35_display_on_cmd[] = {
	{DTYPE_DCS_SWRITE,20,sizeof(st7796s_boe35_display_on),st7796s_boe35_display_on},
};

static const struct rda_dsi_cmd st7796s_boe35_enter_sleep_cmd[] = {
	{DTYPE_DCS_SWRITE,120,sizeof(st7796s_boe35_enter_sleep),st7796s_boe35_enter_sleep},
};

static const struct rda_dsi_cmd st7796s_boe35_display_off_cmd[] = {
	{DTYPE_DCS_SWRITE,20,sizeof(st7796s_boe35_display_off),st7796s_boe35_display_off},
};

static const struct rda_dsi_cmd st7796s_boe35_read_id_cmd[] = {
	{DTYPE_MAX_PKTSIZE,0,sizeof(st7796s_boe35_max_pkt),st7796s_boe35_max_pkt},
	{DTYPE_DCS_READ,5,sizeof(st7796s_boe35_read_id),st7796s_boe35_read_id},
};

static const struct rda_dsi_cmd st7796s_boe35_ic_status_cmd[] = {
	{DTYPE_MAX_PKTSIZE,0,sizeof(st7796s_boe35_ic_max_pkt),st7796s_boe35_ic_max_pkt},
	{DTYPE_DCS_READ,5,sizeof(st7796s_boe35_ic_status),st7796s_boe35_ic_status},
};


static int st7796s_boe35_mipi_reset_gpio(void)
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

static bool st7796s_boe35_mipi_match_id(void)
{
	struct lcd_panel_info *lcd = (void *)&(st7796s_boe35_mipi_info.lcd);
	u8 id[4];

	rda_lcdc_pre_enable_lcd(lcd, 1);
	st7796s_boe35_mipi_reset_gpio();
	rda_dsi_spkt_cmds_read(st7796s_boe35_read_id_cmd, ARRAY_SIZE(st7796s_boe35_read_id_cmd));
	rda_dsi_read_data(id, 4);
	rda_lcdc_reset();

	printk("get id 0x%02x%02x%02x%02x\n", id[0],id[1],id[2],id[3]);

	rda_lcdc_pre_enable_lcd(lcd, 0);

	if (id[1] == 0x77 && id[2] == 0x96)
		return true;

	return false;

}

static int st7796s_boe35_mipi_init_lcd(void)
{
	struct rda_dsi_transfer *trans;
	struct dsi_cmd_list *cmdlist;

	printk("%s\n",__func__);

	cmdlist = kzalloc(sizeof(struct dsi_cmd_list), GFP_KERNEL);
	if (!cmdlist) {
		pr_err("%s : no memory for cmdlist\n", __func__);
		return -ENOMEM;
	}
	cmdlist->cmds = st7796s_boe35_init_cmd_part;
	cmdlist->cmds_cnt = ARRAY_SIZE(st7796s_boe35_init_cmd_part);
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
	rda_dsi_cmd_send(st7796s_boe35_exit_sleep_cmd);
	rda_dsi_cmd_send(st7796s_boe35_display_on_cmd);

	kfree(cmdlist);
	kfree(trans);

	return 0;
err_no_memory:
	kfree(cmdlist);
	return -ENOMEM;
}

static int st7796s_boe35_mipi_sleep(void)
{
	//struct rda_lcd_info *info = &st7796s_boe35_mipi_info;
	//info->lcd.mipi_pinfo.debug_mode = DBG_NEED_CHK;

	printk("%s\n",__func__);

	mipi_dsi_switch_lp_mode();

	/*display off*/
	rda_dsi_cmd_send(st7796s_boe35_display_off_cmd);
	rda_dsi_cmd_send(st7796s_boe35_enter_sleep_cmd);

	return 0;
}

static int st7796s_boe35_mipi_wakeup(void)
{
	printk("%s\n",__func__);

	/* as we go entire power off, we need to re-init the LCD */
	st7796s_boe35_mipi_reset_gpio();
	st7796s_boe35_mipi_init_lcd();

	return 0;
}

static int st7796s_boe35_mipi_set_active_win(struct lcd_img_rect *r)
{
	return 0;
}

static int st7796s_boe35_mipi_set_rotation(int rotate)
{
	return 0;
}

static int st7796s_boe35_mipi_close(void)
{
	return 0;
}

static int st7796s_boe35_lcd_dbg_w(void *p,int n)
{
	return 0;
}

static int st7796s_boe35_check_lcd(void *p)
{
	u8 i;
	u8 data[8] = {0};
	struct rda_lcd_info *info = (struct rda_lcd_info *)p;
	u32 len =1;

	mipi_dsi_switch_lp_mode();

	/*read id*/
	rda_dsi_spkt_cmds_read(st7796s_boe35_ic_status_cmd, ARRAY_SIZE(st7796s_boe35_ic_status_cmd));
	mdelay(1);
	rda_dsi_read_data(data, len);
	printk("%s st7796s boe35 ic_status ", __func__);

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

static struct rda_lcd_info st7796s_boe35_mipi_info = {
	.name = ST7796S_BOE35_MIPI_PANEL_NAME,
	.ops = {
		.s_reset_gpio = st7796s_boe35_mipi_reset_gpio,
		.s_open = st7796s_boe35_mipi_init_lcd,
		.s_match_id = st7796s_boe35_mipi_match_id,
		.s_active_win = st7796s_boe35_mipi_set_active_win,
		.s_rotation = st7796s_boe35_mipi_set_rotation,
		.s_sleep = st7796s_boe35_mipi_sleep,
		.s_wakeup = st7796s_boe35_mipi_wakeup,
		.s_close = st7796s_boe35_mipi_close,
		.s_rda_lcd_dbg_w = st7796s_boe35_lcd_dbg_w,
		.s_check_lcd_state = st7796s_boe35_check_lcd},
	.lcd = {
		.width = HVGA_LCDD_DISP_X,
		.height = HVGA_LCDD_DISP_Y,
		.bpp = 16,
		.lcd_interface = LCD_IF_DSI,
		.mipi_pinfo = {
			.data_lane = 1,
			.vc = 0,
			.mipi_mode = DSI_VIDEO_MODE,
			.pixel_format = RGB_PIX_FMT_RGB565,
			.dsi_format = DSI_FMT_RGB565,
			.rgb_order = RGB_ORDER_BGR,
			.trans_mode = DSI_BURST,
			.bllp_enable = true,
			.h_sync_active = 0x31,
			.h_back_porch = 0x3C,
			.h_front_porch = 0x6C,
			.v_sync_active = 4,
			.v_back_porch = 4,
			.v_front_porch = 8,

			.vat_line = 480,
			.frame_rate = 60,
			.te_sel = true,
			.dsi_pclk_rate = 250,
			.debug_mode = DBG_NO_USE,
			.dsi_phy_db = &st7796s_boe35_pll_phy_250mhz,
		}
	},
};

/*--------------------Platform Device Probe-------------------------*/

static int rda_panel_st7796s_boe35_mipi_probe(struct platform_device *pdev)
{
	rda_fb_register_panel(&st7796s_boe35_mipi_info);

	dev_info(&pdev->dev, "rda panel st7796s_boe35_mipi registered\n");

	return 0;
}

static int rda_panel_st7796s_boe35_mipi_remove(struct platform_device *pdev)
{
	return 0;
}

static struct platform_driver rda_panel_st7796s_boe35_mipi_driver = {
	.probe = rda_panel_st7796s_boe35_mipi_probe,
	.remove = rda_panel_st7796s_boe35_mipi_remove,
	.driver = {
		.name = ST7796S_BOE35_MIPI_PANEL_NAME}
};

static struct rda_panel_driver st7796s_boe35_mipi_panel_driver = {
	.panel_type = LCD_IF_DSI,
	.lcd_driver_info = &st7796s_boe35_mipi_info,
	.pltaform_panel_driver = &rda_panel_st7796s_boe35_mipi_driver,
};

