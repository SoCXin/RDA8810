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

static struct rda_lcd_info hx8379c_cpt45_mipi_info;

/*mipi dsi phy 260MHz*/

static const struct rda_dsi_phy_ctrl hx8379c_cpt45_pll_phy_260mhz = {
	{
		{0x6C, 0x3F}, {0x10C, 0x3}, {0x108,0x2}, {0x118, 0x4}, {0x11C, 0x0},
		{0x120, 0xC}, {0x124, 0x2}, {0x128, 0x3}, {0x80, 0xE}, {0x84, 0xC},
		{0x130, 0xC}, {0x150, 0x12}, {0x170, 0x87A},
	},
	{
		{0x64, 0x3}, {0x134, 0x3}, {0x138, 0xB}, {0x14C, 0x2C}, {0x13C, 0x7},
		{0x114, 0xB}, {0x170, 0x87A}, {0x140, 0xFF},
	},
};

static const char hx8379c_cpt45_cmd0[] = {0xB9, 0xFF, 0x83, 0x79};

static const char hx8379c_cpt45_cmd1[] = {0xB1, 0x44, 0x18, 0x18, 0x31, 0x51, 0x90, 0xD0, 0xee, 0xd4, 0x80,
	0x38, 0x38, 0xF8, 0x44, 0x44, 0x42, 0x00, 0x80, 0x30, 0x00};

static const char hx8379c_cpt45_cmd2[] = {0x3A, 0x55};

static const char hx8379c_cpt45_cmd3[] = {0xB2, 0x80, 0xfe, 0x09, 0x0c, 0x30, 0x50, 0x11, 0x42, 0x1D};

static const char hx8379c_cpt45_cmd4[] = {0xB4, 0x01, 0x28, 0x00, 0x34, 0x00, 0x34, 0x17, 0x3a, 0x17, 0x3a, 0xb0, 0x00, 0xff};

static const char hx8379c_cpt45_cmd5[] = {0xCC, 0x02};

static const char hx8379c_cpt45_cmd6[] = {0xD2, 0x33};

static const char hx8379c_cpt45_cmd7[] = {0xD3, 0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 0x06, 0x32, 0x10, 0x03,
	0x00, 0x03, 0x03, 0x5f, 0x03, 0x5f, 0x00, 0x08, 0x00, 0x08,
	0x35, 0x33, 0x07, 0x07, 0x37, 0x07, 0x07, 0x37, 0x07};

static const char hx8379c_cpt45_cmd8[] = {0xD5, 0x18, 0x18, 0x19, 0x19, 0x18, 0x18, 0x20, 0x21, 0x24, 0x25,
	0x18, 0x18, 0x18, 0x18, 0x00, 0x01, 0x04, 0x05, 0x02, 0x03,
	0x06, 0x07, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18,
	0x18, 0x18};

static const char hx8379c_cpt45_cmd9[] = {0xD6, 0x18, 0x18, 0x18, 0x18, 0x19, 0x19, 0x25, 0x24, 0x21, 0x20,
	0x18, 0x18, 0x18, 0x18, 0x05, 0x04, 0x01, 0x00, 0x03, 0x02,
	0x07, 0x06, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18,
	0x18, 0x18};

static const char hx8379c_cpt45_cmd10[] = {0xE0, 0x00, 0x03, 0x07, 0x2c, 0x35, 0x3f, 0x15, 0x36, 0x06, 0x09,
	0x0b, 0x16, 0x0e, 0x13, 0x15, 0x13, 0x14, 0x06, 0x11, 0x13,
	0x17, 0x00, 0x02, 0x06, 0x2c, 0x36, 0x3f, 0x14, 0x35, 0x06,
	0x0a, 0x0d, 0x18, 0x0e, 0x13, 0x14, 0x13, 0x13, 0x06, 0x10,
	0x11, 0x17};

static const char hx8379c_cpt45_cmd11[] = {0xB6, 0x38, 0x38};


static const char hx8379c_cpt45_exit_sleep[] = {0x11, 0x00};

static const char hx8379c_cpt45_display_on[] = {0x29, 0x00};

static const char hx8379c_cpt45_enter_sleep[] = {0x10, 0x00};

static const char hx8379c_cpt45_display_off[] = {0x28, 0x00};

static const char hx8379c_cpt45_read_id[] = {0x04, 0x00};

static const char hx8379c_cpt45_max_pkt[] = {0x4, 0x00};

//static char pixel_off[] = {0x21, 0x00};

static const struct rda_dsi_cmd hx8379c_cpt45_init_cmd[] = {
	{DTYPE_DCS_LWRITE,0,sizeof(hx8379c_cpt45_cmd0),hx8379c_cpt45_cmd0},
	{DTYPE_DCS_LWRITE,0,sizeof(hx8379c_cpt45_cmd1),hx8379c_cpt45_cmd1},
	{DTYPE_DCS_LWRITE,0,sizeof(hx8379c_cpt45_cmd2),hx8379c_cpt45_cmd2},
	{DTYPE_DCS_LWRITE,0,sizeof(hx8379c_cpt45_cmd3),hx8379c_cpt45_cmd3},
	{DTYPE_DCS_LWRITE,0,sizeof(hx8379c_cpt45_cmd4),hx8379c_cpt45_cmd4},
	{DTYPE_DCS_LWRITE,0,sizeof(hx8379c_cpt45_cmd5),hx8379c_cpt45_cmd5},
	{DTYPE_DCS_LWRITE,0,sizeof(hx8379c_cpt45_cmd6),hx8379c_cpt45_cmd6},
	{DTYPE_DCS_LWRITE,0,sizeof(hx8379c_cpt45_cmd7),hx8379c_cpt45_cmd7},
	{DTYPE_DCS_LWRITE,0,sizeof(hx8379c_cpt45_cmd8),hx8379c_cpt45_cmd8},
	{DTYPE_DCS_LWRITE,0,sizeof(hx8379c_cpt45_cmd9),hx8379c_cpt45_cmd9},
	{DTYPE_DCS_LWRITE,0,sizeof(hx8379c_cpt45_cmd10),hx8379c_cpt45_cmd10},
	{DTYPE_DCS_LWRITE,0,sizeof(hx8379c_cpt45_cmd11),hx8379c_cpt45_cmd11},
};

static const struct rda_dsi_cmd hx8379c_cpt45_exit_sleep_cmd[] = {
	{DTYPE_DCS_SWRITE,120,sizeof(hx8379c_cpt45_exit_sleep),hx8379c_cpt45_exit_sleep},
};

static const struct rda_dsi_cmd hx8379c_cpt45_display_on_cmd[] = {
	{DTYPE_DCS_SWRITE,20,sizeof(hx8379c_cpt45_display_on),hx8379c_cpt45_display_on},
};

static const struct rda_dsi_cmd hx8379c_cpt45_enter_sleep_cmd[] = {
	{DTYPE_DCS_SWRITE,120,sizeof(hx8379c_cpt45_enter_sleep),hx8379c_cpt45_enter_sleep},
};

static const struct rda_dsi_cmd hx8379c_cpt45_display_off_cmd[] = {
	{DTYPE_DCS_SWRITE,20,sizeof(hx8379c_cpt45_display_off),hx8379c_cpt45_display_off},
};

static const struct rda_dsi_cmd hx8379c_cpt45_read_id_cmd[] = {
	{DTYPE_MAX_PKTSIZE,0,sizeof(hx8379c_cpt45_max_pkt),hx8379c_cpt45_max_pkt},
	{DTYPE_DCS_READ,5,sizeof(hx8379c_cpt45_read_id),hx8379c_cpt45_read_id},
};

//static struct rda_dsi_cmd pixel_off_cmd[] = {
//	{DTYPE_DCS_SWRITE,20,sizeof(pixel_off),pixel_off},
//};

static int hx8379c_cpt45_mipi_reset_gpio(void)
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

static bool hx8379c_cpt45_mipi_match_id(void)
{
	struct lcd_panel_info *lcd = (void *)&(hx8379c_cpt45_mipi_info.lcd);
	u8 id[4];

	rda_lcdc_pre_enable_lcd(lcd, 1);
	hx8379c_cpt45_mipi_reset_gpio();
	rda_dsi_spkt_cmds_read(hx8379c_cpt45_read_id_cmd, ARRAY_SIZE(hx8379c_cpt45_read_id_cmd));
	rda_dsi_read_data(id, 4);
	rda_lcdc_reset();

	printk("get id 0x%02x%02x%02x\n", id[0], id[1], id[2]);

	rda_lcdc_pre_enable_lcd(lcd, 0);

	if ((id[0] == 0x83) && (id[1] == 0x79) && (id[2] == 0x0c))
		return true;

	return false;

}

static int hx8379c_cpt45_mipi_init_lcd(void)
{
	struct rda_dsi_transfer *trans;
	struct dsi_cmd_list *cmdlist;

	printk("%s\n",__func__);
	cmdlist = kzalloc(sizeof(struct dsi_cmd_list), GFP_KERNEL);
	if (!cmdlist) {
		pr_err("%s : no memory for cmdlist\n", __func__);
		return -ENOMEM;
	}
	cmdlist->cmds = hx8379c_cpt45_init_cmd;
	cmdlist->cmds_cnt = ARRAY_SIZE(hx8379c_cpt45_init_cmd);
	cmdlist->rxlen = 0;
	trans = kzalloc(sizeof(struct rda_dsi_transfer), GFP_KERNEL);
	if (!trans) {
		pr_err("%s : no memory for trans\n", __func__);
		goto err_no_memory;
	}
	trans->cmdlist = cmdlist;
	rda_dsi_cmdlist_send_lpkt(trans);
	/*display on*/
	rda_dsi_cmd_send(hx8379c_cpt45_exit_sleep_cmd);
	rda_dsi_cmd_send(hx8379c_cpt45_display_on_cmd);

	kfree(cmdlist);
	kfree(trans);

	return 0;
err_no_memory:
	kfree(cmdlist);
	return -ENOMEM;
}

static int hx8379c_cpt45_mipi_sleep(void)
{
	printk("%s\n",__func__);
	mipi_dsi_switch_lp_mode();
	/*display off*/
	rda_dsi_cmd_send(hx8379c_cpt45_display_off_cmd);
	rda_dsi_cmd_send(hx8379c_cpt45_enter_sleep_cmd);
	return 0;
}

static int hx8379c_cpt45_mipi_wakeup(void)
{
	/* as we go entire power off, we need to re-init the LCD */
	hx8379c_cpt45_mipi_reset_gpio();
	hx8379c_cpt45_mipi_init_lcd();

	return 0;
}

static int hx8379c_cpt45_mipi_set_active_win(struct lcd_img_rect *r)
{
	return 0;
}

static int hx8379c_cpt45_mipi_set_rotation(int rotate)
{
	return 0;
}

static int hx8379c_cpt45_mipi_close(void)
{
	return 0;
}

static int hx8379c_cpt45_lcd_dbg_w(void *p,int n)
{
	return 0;
}


static struct rda_lcd_info hx8379c_cpt45_mipi_info = {
	.name = HX8379C_CPT45_MIPI_PANEL_NAME,
	.ops = {
		.s_reset_gpio = hx8379c_cpt45_mipi_reset_gpio,
		.s_open = hx8379c_cpt45_mipi_init_lcd,
		.s_match_id = hx8379c_cpt45_mipi_match_id,
		.s_active_win = hx8379c_cpt45_mipi_set_active_win,
		.s_rotation = hx8379c_cpt45_mipi_set_rotation,
		.s_sleep = hx8379c_cpt45_mipi_sleep,
		.s_wakeup = hx8379c_cpt45_mipi_wakeup,
		.s_close = hx8379c_cpt45_mipi_close,
		.s_rda_lcd_dbg_w = hx8379c_cpt45_lcd_dbg_w},
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
			.h_sync_active = 0x14,
			.h_back_porch = 0x1D,
			.h_front_porch = 0x2A,
			.v_sync_active = 0x8,
			.v_back_porch = 0x8,
			.v_front_porch = 0x10,
			.vat_line = 854,
			.frame_rate = 60,
			.te_sel = true,
			.dsi_pclk_rate = 260,
			.dsi_phy_db = &hx8379c_cpt45_pll_phy_260mhz,
		}
	},
};

/*--------------------Platform Device Probe-------------------------*/

static int rda_panel_hx8379c_cpt45_mipi_probe(struct platform_device *pdev)
{
	rda_fb_register_panel(&hx8379c_cpt45_mipi_info);

	dev_info(&pdev->dev, "rda panel hx_8379c_cpt45_mipi registered\n");

	return 0;
}

static int rda_panel_hx8379c_cpt45_mipi_remove(struct platform_device *pdev)
{
	return 0;
}

static struct platform_driver rda_panel_hx8379c_cpt45_mipi_driver = {
	.probe = rda_panel_hx8379c_cpt45_mipi_probe,
	.remove = rda_panel_hx8379c_cpt45_mipi_remove,
	.driver = {
		.name = HX8379C_CPT45_MIPI_PANEL_NAME}
};

static struct rda_panel_driver hx8379c_cpt45_mipi_panel_driver = {
	.panel_type = LCD_IF_DSI,
	.lcd_driver_info = &hx8379c_cpt45_mipi_info,
	.pltaform_panel_driver = &rda_panel_hx8379c_cpt45_mipi_driver,
};

