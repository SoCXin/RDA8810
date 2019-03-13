#ifndef _RDA2201_CFG_H_
#define _RDA2201_CFG_H_

#include "rda_sensor.h"
#include <linux/delay.h>

#ifdef BIT
#undef BIT
#endif
#define BIT	8

#if 0
static struct sensor_reg exp_rda2201[][3] =
{
};

static struct sensor_reg awb_rda2201[][4] =
{
};
#endif

// use this for 1600x1200 (UXGA) capture
static struct sensor_reg uxga_rda2201[] =
{
};

// use this for 640x480 (VGA) capture
static struct sensor_reg vga_rda2201[] =
{
};

#if 0
// use this for 320x240 (QVGA) capture
static struct sensor_reg qvga_rda2201[] =
{
};

// use this for 160x120 (QQVGA) capture
static struct sensor_reg qqvga_rda2201[] =
{
};

// use this for 176x144 (QCIF) capture
static struct sensor_reg qcif_rda2201[] =
{
};
#endif

// use this for init sensor
static struct sensor_reg init_rda2201[] =
{
	{0x00,0x00,BIT,0},//resetn
	{0x00,0xff,BIT,0},//de-resetn
	{0x03,0x01,BIT,0},
	{0x04,0xff,BIT,0},
	{0x00,0x06,BIT,0},//resetn dsp
	{0x00,0xff,BIT,0},//de-resetn
	{0x05,0xff,BIT,0},
	{0x06,0xa3,BIT,0},
	{0x08,0x28,BIT,0},
	{0x09,0x80,BIT,0},//dig_clk=mipi_clk
	{0x0a,0x3a,BIT,0},
	{0x0b,0x98,BIT,0},//d8 smooth, db noise //close ion_en when mipi
	{0x0d,0xbe,BIT,0},//48M ok
	{0x12,0x41,BIT,0},//41 81 c1, extend hsync gap
	{0x13,0x00,BIT,0},
	{0x14,0x08,BIT,0},
	{0x15,0x26,BIT,0},
	{0x16,0x02,BIT,0},
	{0x17,0x1c,BIT,0},
	{0x18,0xcc,BIT,0},
	{0x19,0x00,BIT,0},
	{0x1a,0x00,BIT,0},
	{0x1b,0x51,BIT,0},//pclk=div5
	{0x1c,0x60,BIT,0},//vinv
	{0x1d,0x81,BIT,0},//normal for capture, clkadc=div8
	{0x1e,0x1a,BIT,0},
	{0x20,0x11,BIT,0},//test_bar or pic
	{0x44,0xc1,BIT,0},
	{0x45,0x99,BIT,0},
	{0x42,0x01,BIT,0},
	{0x43,0x00,BIT,0},
	{0x47,0x02,BIT,0},//02 RAW
	{0x48,0x00,BIT,0},
	{0x49,0x8f,BIT,0},//out_pclk INV
	{0x4a,0x14,BIT,0},
	{0x4b,0x11,BIT,0},//left_dummy //for 4pix gap
	{0x4c,0xb0,BIT,0},//58 line_num_l default 600 = 'h258
	{0x4d,0x40,BIT,0},//20 pix_num_l  default 800 = 'h320
	{0x4e,0x64,BIT,0},//32 line_pix_h
	{0x4f,0x38,BIT,0},
	{0x50,0x0a,BIT,0},
	{0x51,0x08,BIT,0},
	{0x52,0x58,BIT,0},//SUB
	{0x53,0x20,BIT,0},//SUB
	{0x54,0x32,BIT,0},//SUB
	{0x55,0x24,BIT,0},
	{0xeb,0x76,BIT,0},//intp for color bar
	{0xa8,0x00,BIT,0},
	{0x6b,0x01,BIT,0},//ae_en=0 ana_gain=1
	{0x74,0x01,BIT,0},
	{0x72,0x80,BIT,0},//exp_init=0x180
	{0x0e,0x01,BIT,0},//enter page1
	{0x5f,0x96,BIT,0},//normal mode
	{0x60,0x7d,BIT,0},//normal mode 1600*5/4
	{0x62,0x45,BIT,0},//8'h45 8'h4a 8'h50
	{0x61,0x47,BIT,0},//8'h47 8'he7 8'h87
	{0x64,0x00,BIT,0},//8'h00 8'h01 8'h01
	{0x63,0xa7,BIT,0},//8'ha7 8'h47 8'he7
	{0x66,0x00,BIT,0},//8'h00 8'h01 8'h01
	{0x65,0xa7,BIT,0},//8'ha7 8'h47 8'he7
	{0x6e,0x44,BIT,0},//8'h44 8'h4a 8'h50
	{0x6d,0xc8,BIT,0},//8'hc8 8'h68 8'h08
	{0x70,0x00,BIT,0},//8'h00 8'h00 8'h01
	{0x6f,0x28,BIT,0},//8'h28 8'hc8 8'h68
	{0x72,0x00,BIT,0},//8'h00 8'h00 8'h01
	{0x71,0x28,BIT,0},//8'h28 8'hc8 8'h68
	{0x73,0x4b,BIT,0},//SUB mode
	{0x74,0x64,BIT,0},//SUB mode
	{0x76,0x21,BIT,0},
	{0x75,0x0c,BIT,0},
	{0x78,0x06,BIT,0},
	{0x77,0xe1,BIT,0},
	{0x7a,0x06,BIT,0},
	{0x79,0xe1,BIT,0},
	{0x7c,0x20,BIT,0},
	{0x7b,0x8e,BIT,0},
	{0x7e,0x06,BIT,0},
	{0x7d,0x63,BIT,0},
	{0x80,0x06,BIT,0},
	{0x7f,0x63,BIT,0},
	{0x55,0xac,BIT,0},//raw10 format
	{0x51,0xf3,BIT,0},//reg000_6, raw10
	{0x5b,0xc0,BIT,0},//tst_mode
	{0x57,0x18,BIT,0},//mipi enable
	{0x0e,0x80,BIT,0},//sensor enable+page0
};

static struct sensor_reg_list rda2201_init = {
	.size = ARRAY_ROW(init_rda2201),
	.val = init_rda2201
};

static struct sensor_reg_list rda2201_uxga = {
	.size = ARRAY_ROW(uxga_rda2201),
	.val = uxga_rda2201
};
static struct sensor_reg_list rda2201_vga = {
	.size = ARRAY_ROW(vga_rda2201),
	.val = vga_rda2201
};
#if 0
static struct sensor_reg_list rda2201_qvga = {
	.size = ARRAY_ROW(qvga_rda2201),
	.val = qvga_rda2201
};
static struct sensor_reg_list rda2201_qcif = {
	.size = ARRAY_ROW(qcif_rda2201),
	.val = qcif_rda2201
};
static struct sensor_reg_list rda2201_qqvga = {
	.size = ARRAY_ROW(qqvga_rda2201),
	.val = qqvga_rda2201
};
#endif
static struct sensor_win_size rda2201_win_size[] = {
	WIN_SIZE("UXGA", W_UXGA, H_UXGA, &rda2201_uxga),
	WIN_SIZE("VGA", W_VGA, H_VGA, &rda2201_vga),
#if 0
	WIN_SIZE("QVGA", W_QVGA, H_QVGA, &rda2201_qvga),
	WIN_SIZE("QCIF", W_QCIF, H_QCIF, &rda2201_qcif),
	WIN_SIZE("QQVGA", W_QQVGA, H_QQVGA, &rda2201_qqvga),
#endif
};

static struct sensor_win_cfg rda2201_win_cfg = {
	.num = ARRAY_ROW(rda2201_win_size),
	.win_size = rda2201_win_size
};

static struct sensor_csi_cfg rda2201_csi_cfg = {
	.csi_en = true,
	.d_term_en = 5,
	.c_term_en = 5,
	.dhs_settle = 5,
	.chs_settle = 5,
};

static struct sensor_info rda2201_info = {
	.name		= "rda2201_csi",
	.chip_id	= 0x21,
	.mclk		= 26,
	.i2c_addr	= 0x30,
	.exp_def	= 0,
	.awb_def	= 1,
	.rst_act_h	= false,
	.pdn_act_h	= true,
	.init		= &rda2201_init,
	.win_cfg	= &rda2201_win_cfg,
	.csi_cfg	= &rda2201_csi_cfg
};

extern void sensor_power_down(bool high, bool acth, int id);
extern void sensor_reset(bool rst, bool acth);
extern void sensor_clock(bool out, int mclk);
extern void sensor_read(const u16 addr, u8 *data, u8 bits);
extern void sensor_write(const u16 addr, const u8 data, u8 bits);
extern void sensor_write_group(struct sensor_reg* reg, u32 size);

static u32 rda2201_power(int id, int mclk, bool rst_h, bool pdn_h)
{
	/* set state to power off */
	sensor_power_down(true, pdn_h, 0);
	mdelay(1);
	sensor_power_down(true, pdn_h, 1);
	mdelay(1);
	sensor_reset(true, rst_h);
	mdelay(1);

	/* power on sequence */
	sensor_clock(true, mclk);
	mdelay(1);
	sensor_power_down(false, pdn_h, id);
	mdelay(1);
	sensor_reset(false, rst_h);
	mdelay(10);

	return 0;
}

static u32 rda2201_get_chipid(void)
{
	u16 chip_id = 0;
	u8 tmp;

	sensor_read(0x01, &tmp, BIT);
	chip_id = (tmp & 0xff);

	return chip_id;
}

static u32 rda2201_get_lum(void)
{
#if 0
	u8 val = 0;
	u32 ret = 0;

	sensor_write(0xfe, 0x01, BIT);
	sensor_read(0x14, &val, BIT);
	sensor_write(0xfe, 0x00, BIT);

	if (val < 0x50)
		ret = 1;

	return ret;
#endif
}

#define GC2035_FLIP_BASE	0x17
#define GC2035_H_FLIP_BIT	0
#define GC2035_V_FLIP_BIT	1
static void rda2201_set_flip(int hv, int flip)
{
#if 0
	u8 tmp = 0;

	sensor_read(GC2035_FLIP_BASE, &tmp, BIT);

	if (hv) {
		if (flip)
			tmp |= (0x1 << GC2035_V_FLIP_BIT);
		else
			tmp &= ~(0x1 << GC2035_V_FLIP_BIT);
	}
	else {
		if (flip)
			tmp |= (0x1 << GC2035_H_FLIP_BIT);
		else
			tmp &= ~(0x1 << GC2035_H_FLIP_BIT);
	}

	sensor_write(GC2035_FLIP_BASE, tmp, BIT);
#endif
}

#if 0
#define GC2035_EXP_ROW		ARRAY_ROW(exp_rda2201)
#define GC2035_EXP_COL		ARRAY_COL(exp_rda2201)
#endif
static void rda2201_set_exp(int exp)
{
#if 0
	int key = exp + (GC2035_EXP_ROW / 2);
	if ((key < 0) || (key > (GC2035_EXP_ROW - 1)))
		return;

	sensor_write_group(exp_rda2201[key], GC2035_EXP_COL);
#endif
}

#if 0
#define GC2035_AWB_ROW		ARRAY_ROW(awb_rda2201)
#define GC2035_AWB_COL		ARRAY_COL(awb_rda2201)
#endif
static void rda2201_set_awb(int awb)
{
#if 0
	if ((awb < 0) || (awb > (GC2035_AWB_ROW - 1)))
		return;

	sensor_write_group(awb_rda2201[awb], GC2035_AWB_COL);
#endif
}

static struct sensor_ops rda2201_ops = {
	.power		= rda2201_power,
	.get_chipid	= rda2201_get_chipid,
	.get_lum	= rda2201_get_lum,
	.set_flip	= rda2201_set_flip,
	.set_exp	= rda2201_set_exp,
	.set_awb	= rda2201_set_awb,
	.start		= NULL,
	.stop		= NULL
};

struct sensor_dev rda2201_dev = {
	.info	= &rda2201_info,
	.ops	= &rda2201_ops,
};

#undef BIT
#endif
