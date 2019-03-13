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

static struct rda_lcd_info jd9161ba_mipi_info;

/*mipi dsi phy 260MHz*/
static const struct rda_dsi_phy_ctrl jd9161ba_pll_phy_260mhz = {
    {
        {0x6C, 0xB3}, {0x10C, 0x1}, {0x108,0x2}, {0x118, 0x4}, {0x11C, 0x0},
        {0x120, 0xC}, {0x124, 0x2}, {0x128, 0x1}, {0x80, 0xD}, {0x84, 0xC},
        {0x130, 0xC}, {0x150, 0x12}, {0x170, 0x87A},
    },
    {
        {0x64, 0x1}, {0x134, 0x1}, {0x138, 0x5}, {0x14C, 0x14}, {0x13C, 0x3},
        {0x114, 0x5}, {0x170, 0x87A}, {0x140, 0xFF},
    },
};

static const char jd9161ba_cmd0[] = {0xBF, 0x91, 0x61, 0xF2};

//VCOM
static const char jd9161ba_cmd1[] = {0xB3, 0x00, 0xB8};

//VCOM_R
static const char jd9161ba_cmd2[] = {0xB4, 0x00, 0xB8};

//VGMP, VGSP, VGMN, VGSN
static const char jd9161ba_cmd3[] = {0xB8, 0x00, 0xB7, 0x01, 0x00, 0xB7, 0x01};

//GIP output voltage level
static const char jd9161ba_cmd4[] = {0xBA, 0x34, 0x23, 0x00};

//column
static const char jd9161ba_cmd5[] = {0xC3, 0x02};

//TCON
static const char jd9161ba_cmd6[] = {0xC4, 0x00, 0x6A};

//POWER CTRL
static const char jd9161ba_cmd7[] = {0xC7, 0x00, 0x01, 0x33, 0x05, 0x65, 0x2F,
		0x1C, 0xA5, 0xA5};

//Gamma
static const char jd9161ba_cmd8[] = {0xC8, 0x7F, 0x77, 0x64, 0x4F, 0x33, 0x18,
		0x15, 0x00, 0x1A, 0x1D, 0x21, 0x46, 0x3B, 0x4C, 0x45, 0x4C, 0x45,
		0x3A, 0x01, 0x7F, 0x77, 0x64, 0x4F, 0x33, 0x18, 0x15, 0x00, 0x1A,
		0x1D, 0x21, 0x46, 0x3B, 0x4C, 0x45, 0x4C, 0x45, 0x3A, 0x01};

//SET GIP_L
static const char jd9161ba_cmd9[] = {0xD4, 0x1F, 0x1E, 0x05, 0x07, 0x01, 0x1F,
		0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F};


//SET GIP_R
static const char jd9161ba_cmd10[] = {0xD5, 0x1F, 0x1E, 0x04, 0x06, 0x00, 0x1F,
		0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F};


//SET GIP_GS_L
static const char jd9161ba_cmd11[] = {0xD6, 0x1F, 0x1F, 0x06, 0x04, 0x00, 0x1E,
		0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F};

//SET GIP_GS_R
static const char jd9161ba_cmd12[] = {0xD7, 0x1F, 0x1F, 0x07, 0x05, 0x01, 0x1E,
		0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F};

//SET GIP1
static const char jd9161ba_cmd13[] = {0xD8, 0x20, 0x00, 0x00, 0x10, 0x03, 0x20,
		0x01, 0x02, 0x00, 0x01, 0x02, 0x5F, 0x5F, 0x00, 0x00, 0x32, 0x04,
		0x5F, 0x5F, 0x0E};

//SET GIP2
static const char jd9161ba_cmd14[] = {0xD9, 0x00, 0x0A, 0x0A, 0x88, 0x00, 0x00,
		0x06, 0x7B, 0x00, 0x00, 0x00, 0x3B, 0x2F, 0x1F, 0x00, 0x00, 0x00,
		0x03, 0x7B};

static const char jd9161ba_cmd15[] = {0xBE, 0x01};

static const char jd9161ba_cmd16[] = {0xCC, 0x34, 0x20, 0x38, 0x60, 0x11, 0x91,
		0x00, 0x40, 0x00, 0x00};

static const char jd9161ba_cmd17[] = {0xBE, 0x00};

static const char jd9161ba_cmd18[] = {0x3A, 0x55};

static const char jd9161ba_exit_sleep[] = {0x11, 0x00};

static const char jd9161ba_display_on[] = {0x29, 0x00};

static const char jd9161ba_enter_sleep[] = {0x10, 0x00};

static const char jd9161ba_display_off[] = {0x28, 0x00};

static const char jd9161ba_read_id[] = {0x04, 0x00};

static const char jd9161ba_max_pkt[] = {0x4, 0x00};

static const struct rda_dsi_cmd jd9161ba_init_cmd[] = {
	{DTYPE_DCS_LWRITE,0,sizeof(jd9161ba_cmd0),jd9161ba_cmd0},
	{DTYPE_DCS_LWRITE,0,sizeof(jd9161ba_cmd1),jd9161ba_cmd1},
	{DTYPE_DCS_LWRITE,0,sizeof(jd9161ba_cmd2),jd9161ba_cmd2},
	{DTYPE_DCS_LWRITE,0,sizeof(jd9161ba_cmd3),jd9161ba_cmd3},
	{DTYPE_DCS_LWRITE,0,sizeof(jd9161ba_cmd4),jd9161ba_cmd4},
	{DTYPE_DCS_LWRITE,0,sizeof(jd9161ba_cmd5),jd9161ba_cmd5},
	{DTYPE_DCS_LWRITE,0,sizeof(jd9161ba_cmd6),jd9161ba_cmd6},
	{DTYPE_DCS_LWRITE,0,sizeof(jd9161ba_cmd7),jd9161ba_cmd7},
	{DTYPE_DCS_LWRITE,0,sizeof(jd9161ba_cmd8),jd9161ba_cmd8},
	{DTYPE_DCS_LWRITE,0,sizeof(jd9161ba_cmd9),jd9161ba_cmd9},
	{DTYPE_DCS_LWRITE,0,sizeof(jd9161ba_cmd10),jd9161ba_cmd10},
	{DTYPE_DCS_LWRITE,0,sizeof(jd9161ba_cmd11),jd9161ba_cmd11},
	{DTYPE_DCS_LWRITE,0,sizeof(jd9161ba_cmd12),jd9161ba_cmd12},
	{DTYPE_DCS_LWRITE,0,sizeof(jd9161ba_cmd13),jd9161ba_cmd13},
	{DTYPE_DCS_LWRITE,0,sizeof(jd9161ba_cmd14),jd9161ba_cmd14},
	{DTYPE_DCS_LWRITE,0,sizeof(jd9161ba_cmd15),jd9161ba_cmd15},
	{DTYPE_DCS_LWRITE,0,sizeof(jd9161ba_cmd16),jd9161ba_cmd16},
	{DTYPE_DCS_LWRITE,0,sizeof(jd9161ba_cmd17),jd9161ba_cmd17},
	{DTYPE_DCS_LWRITE,0,sizeof(jd9161ba_cmd18),jd9161ba_cmd18},
};


static const struct rda_dsi_cmd jd9161ba_exit_sleep_cmd[] = {
	{DTYPE_DCS_SWRITE,120,sizeof(jd9161ba_exit_sleep),jd9161ba_exit_sleep},
};

static const struct rda_dsi_cmd jd9161ba_display_on_cmd[] = {
	{DTYPE_DCS_SWRITE,20,sizeof(jd9161ba_display_on),jd9161ba_display_on},
};

static const struct rda_dsi_cmd jd9161ba_enter_sleep_cmd[] = {
	{DTYPE_DCS_SWRITE,120,sizeof(jd9161ba_enter_sleep),jd9161ba_enter_sleep},
};

static const struct rda_dsi_cmd jd9161ba_display_off_cmd[] = {
	{DTYPE_DCS_SWRITE,20,sizeof(jd9161ba_display_off),jd9161ba_display_off},
};

static const struct rda_dsi_cmd jd9161ba_read_id_cmd[] = {
	{DTYPE_MAX_PKTSIZE,0,sizeof(jd9161ba_max_pkt),jd9161ba_max_pkt},
	{DTYPE_DCS_READ,5,sizeof(jd9161ba_read_id),jd9161ba_read_id},
};

//static struct rda_dsi_cmd pixel_off_cmd[] = {
//	{DTYPE_DCS_SWRITE,20,sizeof(pixel_off),pixel_off},
//};

static int jd9161ba_mipi_reset_gpio(void)
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

static bool jd9161ba_mipi_match_id(void)
{
	struct lcd_panel_info *lcd = (void *)&(jd9161ba_mipi_info.lcd);
	u8 id[4];

	rda_lcdc_pre_enable_lcd(lcd, 1);
	jd9161ba_mipi_reset_gpio();
	rda_dsi_spkt_cmds_read(jd9161ba_read_id_cmd, ARRAY_SIZE(jd9161ba_read_id_cmd));
	rda_dsi_read_data(id, 4);
	rda_lcdc_reset();

	printk("jd9161ba get id 0x%02x%02x%02x\n", id[0], id[1], id[2]);

	rda_lcdc_pre_enable_lcd(lcd, 0);

	if ((id[0] == 0x83) && (id[1] == 0x79) && (id[2] == 0x0c))
		return true;

	return true;//false;

}

static int jd9161ba_mipi_init_lcd(void)
{
	struct rda_dsi_transfer *trans;
	struct dsi_cmd_list *cmdlist;

	printk("%s\n",__func__);
	cmdlist = kzalloc(sizeof(struct dsi_cmd_list), GFP_KERNEL);
	if (!cmdlist) {
		pr_err("%s : no memory for cmdlist\n", __func__);
		return -ENOMEM;
	}
	cmdlist->cmds = jd9161ba_init_cmd;
	cmdlist->cmds_cnt = ARRAY_SIZE(jd9161ba_init_cmd);
	cmdlist->rxlen = 0;
	trans = kzalloc(sizeof(struct rda_dsi_transfer), GFP_KERNEL);
	if (!trans) {
		pr_err("%s : no memory for trans\n", __func__);
		goto err_no_memory;
	}
	trans->cmdlist = cmdlist;
	rda_dsi_cmdlist_send_lpkt(trans);
	/*display on*/
	rda_dsi_cmd_send(jd9161ba_exit_sleep_cmd);
	rda_dsi_cmd_send(jd9161ba_display_on_cmd);

	kfree(cmdlist);
	kfree(trans);

	return 0;
err_no_memory:
	kfree(cmdlist);
	return -ENOMEM;
}

static int jd9161ba_mipi_sleep(void)
{
	printk("%s\n",__func__);
	mipi_dsi_switch_lp_mode();
	/*display off*/
	rda_dsi_cmd_send(jd9161ba_display_off_cmd);
	rda_dsi_cmd_send(jd9161ba_enter_sleep_cmd);
	return 0;
}

static int jd9161ba_mipi_wakeup(void)
{
	/* as we go entire power off, we need to re-init the LCD */
	jd9161ba_mipi_reset_gpio();
	jd9161ba_mipi_init_lcd();

	return 0;
}

static int jd9161ba_mipi_set_active_win(struct lcd_img_rect *r)
{
	return 0;
}

static int jd9161ba_mipi_set_rotation(int rotate)
{
	return 0;
}

static int jd9161ba_mipi_close(void)
{
	return 0;
}

static int jd9161ba_lcd_dbg_w(void *p,int n)
{
	return 0;
}


static struct rda_lcd_info jd9161ba_mipi_info = {
	.name = JD9161BA_MIPI_PANEL_NAME,
	.ops = {
		.s_reset_gpio = jd9161ba_mipi_reset_gpio,
		.s_open = jd9161ba_mipi_init_lcd,
		.s_match_id = jd9161ba_mipi_match_id,
		.s_active_win = jd9161ba_mipi_set_active_win,
		.s_rotation = jd9161ba_mipi_set_rotation,
		.s_sleep = jd9161ba_mipi_sleep,
		.s_wakeup = jd9161ba_mipi_wakeup,
		.s_close = jd9161ba_mipi_close,
		.s_rda_lcd_dbg_w = jd9161ba_lcd_dbg_w},
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
			.h_sync_active = 0xA,
			.h_back_porch = 0xA,
			.h_front_porch = 0xA,
			.v_sync_active = 0x4,
			.v_back_porch = 0x6,
			.v_front_porch = 0x4,
			.vat_line = 800,
			.frame_rate = 60,
			.te_sel = true,
			.dsi_pclk_rate = 260,
			.dsi_phy_db = &jd9161ba_pll_phy_260mhz,
		}
	},
};

/*--------------------Platform Device Probe-------------------------*/

static int rda_panel_jd9161ba_mipi_probe(struct platform_device *pdev)
{
	rda_fb_register_panel(&jd9161ba_mipi_info);

	dev_info(&pdev->dev, "rda panel jd9161ba_mipi registered\n");

	return 0;
}

static int rda_panel_jd9161ba_mipi_remove(struct platform_device *pdev)
{
	return 0;
}

static struct platform_driver rda_panel_jd9161ba_mipi_driver = {
	.probe = rda_panel_jd9161ba_mipi_probe,
	.remove = rda_panel_jd9161ba_mipi_remove,
	.driver = {
		.name = JD9161BA_MIPI_PANEL_NAME}
};

static struct rda_panel_driver jd9161ba_mipi_panel_driver = {
	.panel_type = LCD_IF_DSI,
	.lcd_driver_info = &jd9161ba_mipi_info,
	.pltaform_panel_driver = &rda_panel_jd9161ba_mipi_driver,
};

