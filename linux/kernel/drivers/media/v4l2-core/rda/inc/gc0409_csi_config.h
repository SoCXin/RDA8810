#ifndef _gc0409_CFG_H_
#define _gc0409_CFG_H_

#include "rda_isp_reg.h"
#include "rda_sensor.h"
#include <linux/delay.h>

#ifdef BIT
#undef BIT
#endif
#define BIT	8


static const struct isp_reg isp_8850e_gc0409[] =
{
	{0x528,0x00},
};

static const struct sensor_reg gain_gc0409_from_isp[][3] =
{
#if 0
	{{0xfd,0x01,BIT,0},{0x24,0x10,BIT,0},{0x01,0x01,BIT,0}},
	{{0xfd,0x01,BIT,0},{0x24,0x14,BIT,0},{0x01,0x01,BIT,0}},
	{{0xfd,0x01,BIT,0},{0x24,0x18,BIT,0},{0x01,0x01,BIT,0}},
	{{0xfd,0x01,BIT,0},{0x24,0x1c,BIT,0},{0x01,0x01,BIT,0}},
	{{0xfd,0x01,BIT,0},{0x24,0x20,BIT,0},{0x01,0x01,BIT,0}},
	{{0xfd,0x01,BIT,0},{0x24,0x28,BIT,0},{0x01,0x01,BIT,0}},
	{{0xfd,0x01,BIT,0},{0x24,0x30,BIT,0},{0x01,0x01,BIT,0}},
	{{0xfd,0x01,BIT,0},{0x24,0x40,BIT,0},{0x01,0x01,BIT,0}},
#endif
};


static const struct sensor_reg awb_gc0409[][6] =
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
static const struct sensor_reg vga_gc0409[] =
{
//      {0xfe,0x00,BIT,0},
};

// use this for 320x240 (QVGA) capture
static const struct sensor_reg qvga_gc0409[] =
{
//     {0xfe,0x00,BIT,0},
};


// use this for 1600*1200 (UXGA) capture
static const struct sensor_reg uxga_gc0409[] =
{
//	{0xfe,0x00,BIT,0},
};

// use this for 176x144 (QCIF) capture
static const struct sensor_reg qcif_gc0409[] =
{
//	{0xfe,0x00,BIT,0},
};

// use this for init sensor
static const struct sensor_reg init_gc0409[] =
{
	////////////////////////////////////////////////////
	/////////////////////	SYS 	////////////////////
	////////////////////////////////////////////////////
	{0xfe,0x80,BIT,0},
	{0xfe,0x80,BIT,0},
	{0xfe,0x80,BIT,0},
	{0xf7,0x01,BIT,0},
	{0xf8,0x05,BIT,0},
	{0xf9,0x0f,BIT,0},//[0] not_use_pll
	{0xfa,0x00,BIT,0},
	{0xfc,0x0f,BIT,0},//[0] apwd
	{0xfe,0x00,BIT,0},

	///////////////////////////////////////////////////
	////////////// ANALOG & CISCTL/////////////////////
	///////////////////////////////////////////////////
	{0xfe,0x00,BIT,0},
	{0x03,0x01,BIT,0},//01
	{0x04,0xf4,BIT,0},//e0//Exp_time
	{0x05,0x06,BIT,0},//02
	{0x06,0x08,BIT,0},//84//HB
	{0x07,0x00,BIT,0},
	{0x08,0x04,BIT,0},//24//VB
	{0x0a,0x00,BIT,0},//row_start
	{0x0c,0x04,BIT,0},
	{0x0d,0x01,BIT,0},
	{0x0e,0xe8,BIT,0},
	{0x0f,0x03,BIT,0},
	{0x10,0x30,BIT,0},//win_width
	{0x17,0x14,BIT,0},
	{0x18,0x12,BIT,0},
	{0x19,0x0b,BIT,0},//0d//AD_pipe_number
	{0x1a,0x1b,BIT,0},
	{0x1d,0x4c,BIT,0},
	{0x1e,0x50,BIT,0},
	{0x1f,0x80,BIT,0},
	{0x23,0x01,BIT,0},//00//[0]vpix_sw
	{0x24,0xc8,BIT,0},
	{0x25,0xe2,BIT,0},
	{0x27,0xaf,BIT,0},
	{0x28,0x24,BIT,0},
	{0x29,0x0f,BIT,0},//0d//[5:0]buf_EQ_post_width
	{0x2f,0x14,BIT,0},
	{0x3f,0x18,BIT,0},//tx en
	{0x72,0x98,BIT,0},//58//[7]vrefrh_en [6:4]rsgh_r
	{0x73,0x9a,BIT,0},
	{0x74,0x47,BIT,0},//49//[3]RESTH_sw [2:0]noican
	{0x76,0xb2,BIT,0},
	{0x7a,0xcb,BIT,0},//fb//[5:4]vpix_r
	{0xc2,0x0c,BIT,0},
	{0xd0,0x10,BIT,0},
	{0xdc,0x75,BIT,0},
	{0xeb,0x78,BIT,0},

	///////////////////////////////////////////////////
	////////////////////	ISP 	  //////////////////
	///////////////////////////////////////////////////
	{0xfe,0x00,BIT,0},
	{0x90,0x01,BIT,0},
	{0x92,0x01,BIT,0},
	{0x94,0x01,BIT,0},
	{0x95,0x01,BIT,0},
	{0x96,0xe0,BIT,0},
	{0x97,0x03,BIT,0},
	{0x98,0x20,BIT,0},
	{0xb0,0x79,BIT,0},//global_gain[7:3]
	{0x67,0x02,BIT,0},//global_gain[2:0]
	{0xb1,0x01,BIT,0},
	{0xb2,0x00,BIT,0},
	{0xb6,0x00,BIT,0},
	{0xb3,0x40,BIT,0},
	{0xb4,0x40,BIT,0},
	{0xb5,0x40,BIT,0},

	///////////////////////////////////////////////////
	////////////////////	 BLK		////////////////
	///////////////////////////////////////////////////
	{0x40,0x26,BIT,0},
	{0x4f,0x3c,BIT,0},

	////////////////////////////////////////////////////
	/////////////////////	dark sun	/////////////////
	////////////////////////////////////////////////////
	{0xfe,0x00,BIT,0},
	{0xe0,0x9f,BIT,0},
	{0xe4,0x0f,BIT,0},
	{0xe5,0xff,BIT,0},

	///////////////////////////////////////////////////
	////////////////////	 MIPI	////////////////////
	///////////////////////////////////////////////////
	{0xfe,0x03,BIT,0},
	{0x10,0x80,BIT,0},
	{0x01,0x03,BIT,0},
	{0x02,0x22,BIT,0},
	{0x03,0x96,BIT,0},
	{0x04,0x01,BIT,0},
	{0x05,0x00,BIT,0},
	{0x06,0x80,BIT,0},
	{0x11,0x2b,BIT,0},
	{0x12,0xe8,BIT,0},
	{0x13,0x03,BIT,0},
	{0x15,0x00,BIT,0},
	{0x21,0x10,BIT,0},
	{0x22,0x00,BIT,0},
	{0x23,0x0a,BIT,0},
	{0x24,0x10,BIT,0},
	{0x25,0x10,BIT,0},
	{0x26,0x03,BIT,0},
	{0x29,0x01,BIT,0},
	{0x2a,0x0a,BIT,0},
	{0x2b,0x03,BIT,0},
	{0x10,0x91,BIT,0},
	{0xfe,0x00,BIT,0},
	{0xf9,0x0e,BIT,0},//[0] not_use_pll
	{0xfc,0x0e,BIT,0},//[0] apwd
	{0xfe,0x00,BIT,0},
};

static const struct isp_reg_list gc0409_8850e_init= {
	.size = ARRAY_ROW(isp_8850e_gc0409),
	.val = isp_8850e_gc0409
};

static const struct sensor_reg_list gc0409_init = {
	.size = ARRAY_ROW(init_gc0409),
	.val = init_gc0409
};

static const struct sensor_reg_list gc0409_vga = {
	.size = ARRAY_ROW(vga_gc0409),
	.val = vga_gc0409
};
static const struct sensor_reg_list gc0409_qvga = {
	.size = ARRAY_ROW(qvga_gc0409),
	.val = qvga_gc0409
};
static const struct sensor_reg_list gc0409_uxga = {
	.size = ARRAY_ROW(uxga_gc0409),
	.val = uxga_gc0409
};
static const struct sensor_reg_list gc0409_qcif = {
	.size = ARRAY_ROW(qcif_gc0409),
	.val = qcif_gc0409
};
static const struct sensor_win_size gc0409_win_size[] = {
	WIN_SIZE("UXGA", W_UXGA, H_UXGA, &gc0409_uxga),
	WIN_SIZE("VGA", W_VGA, H_VGA, &gc0409_vga),
	WIN_SIZE("QVGA", W_QVGA, H_QVGA, &gc0409_qvga),
	WIN_SIZE("QCIF", W_QCIF, H_QCIF, &gc0409_qcif),
};

static const struct sensor_win_cfg gc0409_win_cfg = {
	.num = ARRAY_ROW(gc0409_win_size),
	.win_size = gc0409_win_size
};

static const struct sensor_csi_cfg gc0409_csi_cfg = {
	.csi_en = true,
	.d_term_en = 5,
	.c_term_en = 5,
	.dhs_settle = 5,
	.chs_settle = 5,
};
static struct raw_sensor_info_data  gc0409_raw_info = {
	.frame_line_num  = 488, //max line number of sensor output, to Camera MIPI.
	.frame_width     = 760,//frame width of sensor output to ISP.
	.frame_height    = 480, //frame height  sensor output.to ISP.
	.flicker_50      = 0x118,
	.flicker_60      = 0x104,
	.targetBV        = 0x60,
	.ae_table       = 0,
	.isp_sensor_init= &gc0409_8850e_init
};

static const struct sensor_info gc0409_info = {
	.name		= "gc0409",
	.chip_id	= 0x0409,
	.mclk		= 26,
	.i2c_addr	= 0x21,
	.type_sensor    = RAW,
	.exp_def	= 0,
	.awb_def	= 1,
	.rst_act_h	= false,
	.pdn_act_h	= true,
	.raw_sensor     = &gc0409_raw_info,
	.init		= &gc0409_init,
	.win_cfg	= &gc0409_win_cfg,
	.csi_cfg	= &gc0409_csi_cfg
};

extern void sensor_power_down(bool high, bool acth, int id);
extern void sensor_reset(bool rst, bool acth);
extern void sensor_clock(bool out, int mclk);
extern void sensor_read(const u16 addr, u8 *data, u8 bits);
extern void sensor_write(const u16 addr, const u8 data, u8 bits);
extern void sensor_write_group(struct sensor_reg* reg, u32 size);

static u32 gc0409_power(int id, int mclk, bool rst_h, bool pdn_h)
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

static u32 gc0409_get_chipid(void)
{
	u16 chip_id = 0;
	u8 tmp;

	sensor_read(0xf0, &tmp, BIT);
	chip_id = (tmp << 8) & 0xff00;
	sensor_read(0xf1, &tmp, BIT);
	chip_id |= (tmp & 0xff);

	return chip_id;
}

static u32 gc0409_get_lum(void)
{
	u8 val = 0;
	u32 ret = 0;

	sensor_write(0xfe, 0x01, BIT);
	sensor_read(0x14, &val, BIT);
	sensor_write(0xfe, 0x00, BIT);

	if (val < 0x50)
		ret = 1;

	return ret;
}

#define gc0409_FLIP_BASE	0x17
#define gc0409_H_FLIP_BIT	0
#define gc0409_V_FLIP_BIT	1
static void gc0409_set_flip(int hv, int flip)
{
	u8 tmp = 0;

	sensor_read(gc0409_FLIP_BASE, &tmp, BIT);

	if (hv) {
		if (flip)
			tmp |= (0x1 << gc0409_V_FLIP_BIT);
		else
			tmp &= ~(0x1 << gc0409_V_FLIP_BIT);
	}
	else {
		if (flip)
			tmp |= (0x1 << gc0409_H_FLIP_BIT);
		else
			tmp &= ~(0x1 << gc0409_H_FLIP_BIT);
	}

	sensor_write(gc0409_FLIP_BASE, tmp, BIT);
}

static void gc0409_upd_exp_isp(int val)
{
	return;
/*	u8 tmp3, tmp4 = 0;
	tmp3 = (val>>8) & 0xff;
	tmp4 = (val) & 0xff;
	sensor_write(0xfd, 0x01, BIT);
	sensor_write(0x03, tmp3, BIT);
	sensor_write(0x04, tmp4, BIT);
	sensor_write(0x01, 0x01, BIT);
*/
}

#define gc0409_GAIN_ISP_ROW		ARRAY_ROW(gain_gc0409_from_isp)
#define gc0409_GAIN_ISP_COL		ARRAY_COL(gain_gc0409_from_isp)
static void gc0409_upd_gain_isp(int val)
{
	sensor_write(0xfd, 0x01, BIT);
	sensor_write(0x24, (u8)val, BIT);
	sensor_write(0x01, 0x01, BIT);
}

/*
#define gc0409_EXP_ROW		ARRAY_ROW(exp_gc0409)
#define gc0409_EXP_COL		ARRAY_COL(exp_gc0409)
static void gc0409_set_exp(int exp)
{
	return; // need to be done by ISP
	int key = exp + (gc0409_EXP_ROW / 2);
	if ((key < 0) || (key > (gc0409_EXP_ROW - 1)))
		return;

	sensor_write_group(exp_gc0409[key], gc0409_EXP_COL);
}
*/
#define gc0409_AWB_ROW		ARRAY_ROW(awb_gc0409)
#define gc0409_AWB_COL		ARRAY_COL(awb_gc0409)
static void gc0409_set_awb(int awb)
{
	return; // need to be done by ISP
	if ((awb < 0) || (awb > (gc0409_AWB_ROW - 1)))
		return;

	sensor_write_group(awb_gc0409[awb], gc0409_AWB_COL);
}

static struct sensor_ops gc0409_ops = {
	.power		= gc0409_power,
	.get_chipid	= gc0409_get_chipid,
	.get_lum	= gc0409_get_lum,
	.set_flip	= gc0409_set_flip,
	.set_exp	= NULL, //gc0409_set_exp,
	.set_awb	= gc0409_set_awb,
	.upd_gain_isp   = gc0409_upd_gain_isp,
	.upd_exp_isp    = gc0409_upd_exp_isp,
	.start		= NULL,
	.stop		= NULL
};

struct sensor_dev gc0409_dev = {
	.info	= &gc0409_info,
	.ops	= &gc0409_ops,
};

#undef BIT
#endif
