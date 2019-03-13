#ifndef _sp0a09_CFG_H_
#define _sp0a09_CFG_H_

#include "rda_isp_reg.h"
#include "rda_sensor.h"
#include "rda_8850e_isp_regname.h"
#include <linux/delay.h>

#ifdef BIT
#undef BIT
#endif
#define BIT	8

static const struct isp_reg isp_8850e_sp0a09[] =
{

	{TOP_DUMMY_REG0,0x00},
};
static const struct sensor_reg gain_sp0a09_from_isp[][3] =
{
#if 0
	{{0xfd,0x00,BIT,0},{0x24,0x10,BIT,0},{0x01,0x01,BIT,0}},
	{{0xfd,0x00,BIT,0},{0x24,0x14,BIT,0},{0x01,0x01,BIT,0}},
	{{0xfd,0x00,BIT,0},{0x24,0x18,BIT,0},{0x01,0x01,BIT,0}},
	{{0xfd,0x00,BIT,0},{0x24,0x1c,BIT,0},{0x01,0x01,BIT,0}},
	{{0xfd,0x00,BIT,0},{0x24,0x20,BIT,0},{0x01,0x01,BIT,0}},
	{{0xfd,0x00,BIT,0},{0x24,0x28,BIT,0},{0x01,0x01,BIT,0}},
	{{0xfd,0x00,BIT,0},{0x24,0x30,BIT,0},{0x01,0x01,BIT,0}},
	{{0xfd,0x00,BIT,0},{0x24,0x40,BIT,0},{0x01,0x01,BIT,0}},
#endif
};


static const struct sensor_reg awb_sp0a09[][6] =
{
#if 0
	{{0xfd,0x02,BIT,0},{0xfd,0x00,BIT,0},{0xfd,0x00,BIT,0},{0xfd,0x00,BIT,0},{0xfd,0x00,BIT,0},{0xfd,0x00,BIT,0}},//OFF
	{{0xfd,0x02,BIT,0},{0x26,0xc8,BIT,0},{0x27,0xb6,BIT,0},{0xfd,0x01,BIT,0},{0x32,0x15,BIT,0},{0xfd,0x00,BIT,0}},//AUTO
	{{0xfd,0x01,BIT,0},{0x32,0x05,BIT,0},{0xfd,0x02,BIT,0},{0x26,0xaa,BIT,0},{0x27,0xce,BIT,0},{0xfd,0x00,BIT,0}},//INCANDESCENT
	{{0xfd,0x01,BIT,0},{0x32,0x05,BIT,0},{0xfd,0x02,BIT,0},{0x26,0x91,BIT,0},{0x27,0xc8,BIT,0},{0xfd,0x00,BIT,0}},//FLUORESCENT
	{{0xfd,0x01,BIT,0},{0x32,0x05,BIT,0},{0xfd,0x02,BIT,0},{0x26,0x75,BIT,0},{0x27,0xe2,BIT,0},{0xfd,0x00,BIT,0}},//TUNGSTEN
	{{0xfd,0x01,BIT,0},{0x32,0x05,BIT,0},{0xfd,0x02,BIT,0},{0x26,0xc8,BIT,0},{0x27,0x89,BIT,0},{0xfd,0x00,BIT,0}},//DAYLIGHT
	{{0xfd,0x01,BIT,0},{0x32,0x05,BIT,0},{0xfd,0x02,BIT,0},{0x26,0xdc,BIT,0},{0x27,0x75,BIT,0},{0xfd,0x00,BIT,0}},//CLOUD
#endif
};

// use this for 640x480 (VGA) capture
static const struct sensor_reg vga_sp0a09[] =
{
//      {0xfe,0x00,BIT,0},
};

// use this for 320x240 (QVGA) capture
static const struct sensor_reg qvga_sp0a09[] =
{
//     {0xfe,0x00,BIT,0},
};


// use this for 1600*1200 (UXGA) capture
static const struct sensor_reg uxga_sp0a09[] =
{
//	{0xfe,0x00,BIT,0},
};

// use this for 176x144 (QCIF) capture
static const struct sensor_reg qcif_sp0a09[] =
{
//	{0xfe,0x00,BIT,0},
};

// use this for init sensor
static const struct sensor_reg init_sp0a09[] =
{
	{0x0c,0x00,BIT,0},
	{0x0f,0x3f,BIT,0},
	{0x10,0x3e,BIT,0},
	{0x11,0x00,BIT,0},
	{0x13,0x18,BIT,0},
	{0x6c,0x19,BIT,0},
	{0x6d,0x00,BIT,0},
	{0x6f,0x1a,BIT,0},
	{0x6e,0x1b,BIT,0},
	{0x69,0x1d,BIT,0},
	{0x71,0x1b,BIT,0},
	{0x14,0x01,BIT,0},
	{0x15,0x1a,BIT,0},
	{0x16,0x20,BIT,0},
	{0x17,0x20,BIT,0},
	{0x70,0x22,BIT,0},
	{0x6a,0x28,BIT,0},
	{0x72,0x2a,BIT,0},
	{0x1a,0x0b,BIT,0},
	{0x1b,0x03,BIT,0},
	{0x1e,0x13,BIT,0},
	{0x1f,0x01,BIT,0},
	{0x27,0x9b,BIT,0},
	{0x28,0x4f,BIT,0},
	{0x21,0x0c,BIT,0},
	{0x22,0x48,BIT,0},
	{0xfd,0x00,BIT,0},
	{0x24,0x28,BIT,0},
	{0x03,0x02,BIT,0},
	{0x04,0x9e,BIT,0},
	{0x0a,0x06,BIT,0},
	{0x01,0x01,BIT,0},
	{0xfd,0x01,BIT,0},
	{0xfb,0x73,BIT,0},
	{0x15,0x50,BIT,0},
	{0x16,0x2c,BIT,0},
	{0xfd,0x00,BIT,0},
	{0x01,0x01,BIT,0},
	{0xc4,0x30,BIT,0},
	{0xb1,0x01,BIT,0},
	{0xb3,0x01,BIT,0},
	{0x9d,0x85,BIT,0},
	{0x9c,0x1a,BIT,0},
	{0xcd,0x0c,BIT,0},
	{0xa4,0x01,BIT,0},
	{0xfd,0x00,BIT,0},
};

static const struct isp_reg_list sp0a09_8850e_init= {
	.size = ARRAY_ROW(isp_8850e_sp0a09),
	.val = isp_8850e_sp0a09
};

static const struct sensor_reg_list sp0a09_init = {
	.size = ARRAY_ROW(init_sp0a09),
	.val = init_sp0a09
};

static const struct sensor_reg_list sp0a09_vga = {
	.size = ARRAY_ROW(vga_sp0a09),
	.val = vga_sp0a09
};
static const struct sensor_reg_list sp0a09_qvga = {
	.size = ARRAY_ROW(qvga_sp0a09),
	.val = qvga_sp0a09
};
static const struct sensor_reg_list sp0a09_uxga = {
	.size = ARRAY_ROW(uxga_sp0a09),
	.val = uxga_sp0a09
};
static const struct sensor_reg_list sp0a09_qcif = {
	.size = ARRAY_ROW(qcif_sp0a09),
	.val = qcif_sp0a09
};
static const struct sensor_win_size sp0a09_win_size[] = {
	WIN_SIZE("UXGA", W_UXGA, H_UXGA, &sp0a09_uxga),
	WIN_SIZE("VGA", W_VGA, H_VGA, &sp0a09_vga),
	WIN_SIZE("QVGA", W_QVGA, H_QVGA, &sp0a09_qvga),
	WIN_SIZE("QCIF", W_QCIF, H_QCIF, &sp0a09_qcif),
};

static const struct sensor_win_cfg sp0a09_win_cfg = {
	.num = ARRAY_ROW(sp0a09_win_size),
	.win_size = sp0a09_win_size
};

static const struct sensor_csi_cfg sp0a09_csi_cfg = {
	.csi_en = true,
	.d_term_en = 20,
	.c_term_en = 20,
	.dhs_settle = 20,
	.chs_settle = 20,
};
static struct raw_sensor_info_data  sp0a09_raw_info = {
	.frame_line_num  = 480, //max line number of sensor output, to Camera MIPI.
	.frame_width     = 640,//frame width of sensor output to ISP.
	.frame_height    = 480, //frame height  sensor output.to ISP.
	.flicker_50      = 0x118,
	.flicker_60      = 0x104,
	.targetBV        = 0x60,
	.ae_table       = 0,
	.isp_sensor_init= &sp0a09_8850e_init
};

static const struct sensor_info sp0a09_info = {
	.name		= "sp0a09_csi",
	.chip_id	= 0x0a09,
	.mclk		= 26,
	.i2c_addr	= 0x21,
	.type_sensor    = RAW,
	.exp_def	= 0,
	.awb_def	= 1,
	.rst_act_h	= false,
	.pdn_act_h	= true,
	.raw_sensor     = &sp0a09_raw_info,
	.init		= &sp0a09_init,
	.win_cfg	= &sp0a09_win_cfg,
	.csi_cfg	= &sp0a09_csi_cfg
};

extern void sensor_power_down(bool high, bool acth, int id);
extern void sensor_reset(bool rst, bool acth);
extern void sensor_clock(bool out, int mclk);
extern void sensor_read(const u16 addr, u8 *data, u8 bits);
extern void sensor_write(const u16 addr, const u8 data, u8 bits);
extern void sensor_write_group(struct sensor_reg* reg, u32 size);

static u32 sp0a09_power(int id, int mclk, bool rst_h, bool pdn_h)
{
	/* set state to power off */
	sensor_power_down(true, pdn_h, 0);
	mdelay(10);
	sensor_power_down(true, pdn_h, 1);
	mdelay(20);
	sensor_reset(true, rst_h);
	mdelay(10);
	sensor_clock(false, mclk);
	mdelay(5);

	/* power on sequence */
	sensor_clock(true, mclk);
	mdelay(10);
	sensor_power_down(false, pdn_h, id);
	mdelay(5);

	sensor_power_down(true, pdn_h, id);
	mdelay(5);
	sensor_power_down(false, pdn_h, id);
	mdelay(5);

	sensor_reset(true, rst_h);
	mdelay(5);
	sensor_reset(false, rst_h);
	mdelay(10);

	return 0;
}

static u32 sp0a09_get_chipid(void)
{
	u16 chip_id = 0;
	u8 tmp;


	sensor_write(0xfd, 0x00, BIT);
	sensor_read(0xa0, &tmp, BIT);
	chip_id = (tmp << 8) & 0xff00;
	sensor_read(0xb0, &tmp, BIT);
	chip_id |= (tmp & 0xff);
	rda_dbg_camera("%s: chip_id = %x\n", __func__,chip_id);
	return chip_id;
}

static u32 sp0a09_get_lum(void)
{
	u8 val = 0;
	u32 ret = 0;

	sensor_write(0xfd, 0x00, BIT);
	sensor_read(0x23, &val, BIT);
	sensor_write(0xfd, 0x00, BIT);

	if (val > 0x50)
		ret = 1;
	return ret;


}

#define sp0a09_FLIP_BASE	0x31
#define sp0a09_H_FLIP_BIT	0
#define sp0a09_V_FLIP_BIT	1
static void sp0a09_set_flip(int hv, int flip)
{

	u8 tmp = 0;
	sensor_write(0xfd, 0x00, BIT);
	sensor_read(sp0a09_FLIP_BASE, &tmp, BIT);


	if (hv) {
		if (flip)
			tmp |= (0x1 << sp0a09_V_FLIP_BIT);
		else
			tmp &= ~(0x1 << sp0a09_V_FLIP_BIT);
	}
	else {
		if (flip)
			tmp |= (0x1 << sp0a09_H_FLIP_BIT);
		else
			tmp &= ~(0x1 << sp0a09_H_FLIP_BIT);
	}

	sensor_write(sp0a09_FLIP_BASE, tmp, BIT);
}

static void sp0a09_upd_exp_isp(int val)
{

	u8 tmp3, tmp4 = 0;
	tmp3 = (val>>8) & 0xff;
	tmp4 = (val) & 0xff;
	sensor_write(0xfd, 0x01, BIT);
	sensor_write(0x03, tmp3, BIT);
	sensor_write(0x04, tmp4, BIT);
	sensor_write(0x01, 0x01, BIT);
}

#define sp0a09_GAIN_ISP_ROW		ARRAY_ROW(gain_sp0a09_from_isp)
#define sp0a09_GAIN_ISP_COL		ARRAY_COL(gain_sp0a09_from_isp)
static void sp0a09_upd_gain_isp(int val)
{
        sensor_write(0xfd, 0x01, BIT);
        sensor_write(0x24, (u8)val, BIT);
        sensor_write(0x01, 0x01, BIT);
}

/*
#define sp0a09_EXP_ROW		ARRAY_ROW(exp_sp0a09)
#define sp0a09_EXP_COL		ARRAY_COL(exp_sp0a09)
static void sp0a09_set_exp(int exp)
{
	return; // need to be done by ISP
	int key = exp + (sp0a09_EXP_ROW / 2);
	if ((key < 0) || (key > (sp0a09_EXP_ROW - 1)))
		return;

	sensor_write_group(exp_sp0a09[key], sp0a09_EXP_COL);
}
*/
#define sp0a09_AWB_ROW		ARRAY_ROW(awb_sp0a09)
#define sp0a09_AWB_COL		ARRAY_COL(awb_sp0a09)
static void sp0a09_set_awb(int awb)
{
	return; // need to be done by ISP
	if ((awb < 0) || (awb > (sp0a09_AWB_ROW - 1)))
		return;

	sensor_write_group(awb_sp0a09[awb], sp0a09_AWB_COL);
}

static struct sensor_ops sp0a09_ops = {
	.power		= sp0a09_power,
	.get_chipid	= sp0a09_get_chipid,
	.get_lum	= sp0a09_get_lum,
	.set_flip	= sp0a09_set_flip,
	.set_exp	= NULL,//sp0a09_set_exp,
	.set_awb	= NULL,//sp0a09_set_awb,
	.upd_gain_isp   =  sp0a09_upd_gain_isp,
	.upd_exp_isp    = sp0a09_upd_exp_isp,
	.start		= NULL,
	.stop		= NULL
};

struct sensor_dev sp0a09_dev = {
	.info	= &sp0a09_info,
	.ops	= &sp0a09_ops,
};

#undef BIT
#endif
