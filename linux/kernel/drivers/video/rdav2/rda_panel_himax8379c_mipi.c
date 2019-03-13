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

#define READ_ID 1

static struct rda_lcd_info himax_8379c_mipi_info;

/*mipi dsi phy 260MHz*/
static const struct rda_dsi_phy_ctrl pll_phy_260mhz = {
	{
		{0x6C, 0x53}, {0x10C, 0x3}, {0x108,0x2}, {0x118, 0x4}, {0x11C, 0x0},
		{0x120, 0xC}, {0x124, 0x2}, {0x128, 0x3}, {0x80, 0xE}, {0x84, 0xC},
		{0x130, 0xC}, {0x150, 0x12}, {0x170, 0x87A},
	},
	{
		{0x64, 0x3}, {0x134, 0x3}, {0x138, 0xB}, {0x14C, 0x2C}, {0x13C, 0x7},
		{0x114, 0xB}, {0x170, 0x87A}, {0x140, 0xFF},
	},
};

static const char cmd0[] = {0xB9, 0xFF, 0x83, 0x79};

static const char cmd1[] = {0xB1, 0x44, 0x14, 0x14, 0x34, 0x54, 0x50, 0xD0, 0xf6, 0x54, 0x80,
	0x38, 0x38, 0xF8, 0x22, 0x22, 0x22};

static const char cmd2[] = {0x3A, 0x55};

static const char cmd3[] = {0xB2, 0x80, 0x3C, 0x0f, 0x05, 0x30, 0x50, 0x11, 0x42, 0x1D};

static const char cmd4[] = {0xB4, 0x08, 0x78, 0x08, 0x78, 0x08, 0x78, 0x22, 0x90, 0x23, 0x90};

static const char cmd5[] = {0xCC, 0x02};

static const char cmd6[] = {0xC7, 0x00, 0x00, 0x00, 0xc0};

static const char cmd7[] = {0xD2, 0x00};

static const char cmd8[] = {0xD3, 0x00, 0x07, 0x00, 0x00, 0x00, 0x08, 0x08, 0x32, 0x10, 0x08,
	0x00, 0x08, 0x03, 0x2d, 0x03, 0x2d, 0x00, 0x08, 0x00, 0x08,
	0x37, 0x33, 0x0b, 0x0b, 0x27, 0x0b, 0x0b, 0x27, 0x0d};

static const char cmd9[] = {0xD5, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x23, 0x22, 0x21, 0x20,
	0x01, 0x00, 0x03, 0x02, 0x05, 0x04, 0x07, 0x06, 0x25, 0x24,
	0x27, 0x26, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18,
	0x18, 0x18, 0x00, 0x00};

static const char cmd10[] = {0xD6, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x26, 0x27, 0x24, 0x25,
	0x02, 0x03, 0x00, 0x01, 0x06, 0x07, 0x04, 0x05, 0x22, 0x23,
	0x20, 0x21, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18,
	0x18, 0x18};

static const char cmd11[] = {0xE0, 0x00, 0x00, 0x06, 0x12, 0x13, 0x3F, 0x23, 0x35, 0x08, 0x0e,
	0x10, 0x19, 0x10, 0x15, 0x17, 0x14, 0x15, 0x07, 0x12, 0x13,
	0x17, 0x00, 0x00, 0x05, 0x11, 0x13, 0x3f, 0x23, 0x34, 0x08,
	0x0e, 0x11, 0x1a, 0x11, 0x14, 0x17, 0x14, 0x16, 0x09, 0x14,
	0x15, 0x1a};

static const char cmd12[] = {0xB6, 0x7E, 0x7E};

static const char exit_sleep[] = {0x11, 0x00};

static const char display_on[] = {0x29, 0x00};

static const char enter_sleep[] = {0x10, 0x00};

static const char display_off[] = {0x28, 0x00};

static const char read_id[] = {0x04, 0x00};

static const char max_pkt[] = {0x4, 0x00};

//static char pixel_off[] = {0x21, 0x00};

static const struct rda_dsi_cmd himax_8379c_init_cmd[] = {
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

static const struct rda_dsi_cmd read_id_cmd[] = {
	{DTYPE_MAX_PKTSIZE,0,sizeof(max_pkt),max_pkt},
	{DTYPE_DCS_READ,5,sizeof(read_id),read_id},
};

//static struct rda_dsi_cmd pixel_off_cmd[] = {
//	{DTYPE_DCS_SWRITE,20,sizeof(pixel_off),pixel_off},
//};

static int himax_8379c_mipi_reset_gpio(void)
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

static bool himax_8379c_mipi_match_id(void)
{
	return true;
}

static int himax_8379c_mipi_init_lcd(void)
{
	struct rda_dsi_transfer *trans;
	struct dsi_cmd_list *cmdlist;
#if READ_ID
	u8 i;
	u8 data_id[8] = {0};
#endif
	printk("%s\n",__func__);
	cmdlist = kzalloc(sizeof(struct dsi_cmd_list), GFP_KERNEL);
	if (!cmdlist) {
		pr_err("%s : no memory for cmdlist\n", __func__);
		return -ENOMEM;
	}
	cmdlist->cmds = himax_8379c_init_cmd;
	cmdlist->cmds_cnt = ARRAY_SIZE(himax_8379c_init_cmd);
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
#if READ_ID
	/*read id*/
	rda_dsi_spkt_cmds_read(read_id_cmd, ARRAY_SIZE(read_id_cmd));
	rda_dsi_read_data(data_id);

	printk("%s himax 8379c id ", __func__);
	for (i = 0; i < 4; i++) {
		printk("0x%x ",data_id[i]);
	}
	printk("\n");
#endif
	kfree(cmdlist);
	kfree(trans);
	return 0;
err_no_memory:
	kfree(cmdlist);
	return -ENOMEM;
}

static int himax_8379c_mipi_sleep(void)
{
	printk("%s\n",__func__);
	mipi_dsi_switch_lp_mode();
	/*display off*/
	rda_dsi_cmd_send(display_off_cmd);
	rda_dsi_cmd_send(enter_sleep_cmd);
	return 0;
}

static int himax_8379c_mipi_wakeup(void)
{
	/* as we go entire power off, we need to re-init the LCD */
	himax_8379c_mipi_reset_gpio();
	himax_8379c_mipi_init_lcd();

	return 0;
}

static int himax_8379c_mipi_set_active_win(struct lcd_img_rect *r)
{
	return 0;
}

static int himax_8379c_mipi_set_rotation(int rotate)
{
	return 0;
}

static int himax_8379c_mipi_close(void)
{
	return 0;
}

static int rda_lcd_dbg_w(void *p,int n)
{
	return 0;
}


static struct rda_lcd_info himax_8379c_mipi_info = {
	.name = HIMAX_8379C_MIPI_PANEL_NAME,
	.ops = {
		.s_reset_gpio = himax_8379c_mipi_reset_gpio,
		.s_open = himax_8379c_mipi_init_lcd,
		.s_match_id = himax_8379c_mipi_match_id,
		.s_active_win = himax_8379c_mipi_set_active_win,
		.s_rotation = himax_8379c_mipi_set_rotation,
		.s_sleep = himax_8379c_mipi_sleep,
		.s_wakeup = himax_8379c_mipi_wakeup,
		.s_close = himax_8379c_mipi_close,
		.s_rda_lcd_dbg_w = rda_lcd_dbg_w},
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
			.rgb_order = RGB_ORDER_RGB,
			.trans_mode = DSI_BURST,
			.bllp_enable = true,
			.h_sync_active = 0x9,
			.h_back_porch = 0x3A,
			.h_front_porch = 0x3A,
			.v_sync_active = 0x4,
			.v_back_porch = 0x4,
			.v_front_porch = 0x4,
			.vat_line = 800,
			.frame_rate = 60,
			.te_sel = true,
			.dsi_pclk_rate = 260,
			.dsi_phy_db = &pll_phy_260mhz,
		}
	},
};

/*--------------------Platform Device Probe-------------------------*/

static int rda_panel_himax_8379c_mipi_probe(struct platform_device *pdev)
{
	rda_fb_register_panel(&himax_8379c_mipi_info);

	dev_info(&pdev->dev, "rda panel himax_8379c_mipi registered\n");

	return 0;
}

static int rda_panel_himax_8379c_mipi_remove(struct platform_device *pdev)
{
	return 0;
}

static struct platform_driver rda_panel_himax_8379c_mipi_driver = {
	.probe = rda_panel_himax_8379c_mipi_probe,
	.remove = rda_panel_himax_8379c_mipi_remove,
	.driver = {
		.name = HIMAX_8379C_MIPI_PANEL_NAME}
};

static struct rda_panel_driver himax8379c_mipi_panel_driver = {
	.panel_type = LCD_IF_DSI,
	.lcd_driver_info = &himax_8379c_mipi_info,
	.pltaform_panel_driver = &rda_panel_himax_8379c_mipi_driver,
};
