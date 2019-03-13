#ifndef _SP2508_CFG_H_
#define _SP2508_CFG_H_

#include "rda_isp_reg.h"
#include "rda_sensor.h"
#include "rda_8850e_isp_regname.h"
#include <linux/delay.h>

#ifdef BIT
#undef BIT
#endif
#define BIT	8



static const struct isp_reg isp_8850e_sp2508[] =
{
	{ISP_PAGE,0x00},
};

static const struct sensor_reg awb_sp2508[][6] =
{
	{{0xfd,0x02,BIT,0},{0xfd,0x00,BIT,0},{0xfd,0x00,BIT,0},{0xfd,0x00,BIT,0},{0xfd,0x00,BIT,0},{0xfd,0x00,BIT,0}},//OFF
	{{0xfd,0x02,BIT,0},{0x26,0xc8,BIT,0},{0x27,0xb6,BIT,0},{0xfd,0x01,BIT,0},{0x32,0x15,BIT,0},{0xfd,0x00,BIT,0}},//AUTO
	{{0xfd,0x01,BIT,0},{0x32,0x05,BIT,0},{0xfd,0x02,BIT,0},{0x26,0xaa,BIT,0},{0x27,0xce,BIT,0},{0xfd,0x00,BIT,0}},//INCANDESCENT
	{{0xfd,0x01,BIT,0},{0x32,0x05,BIT,0},{0xfd,0x02,BIT,0},{0x26,0x91,BIT,0},{0x27,0xc8,BIT,0},{0xfd,0x00,BIT,0}},//FLUORESCENT
	{{0xfd,0x01,BIT,0},{0x32,0x05,BIT,0},{0xfd,0x02,BIT,0},{0x26,0x75,BIT,0},{0x27,0xe2,BIT,0},{0xfd,0x00,BIT,0}},//TUNGSTEN
	{{0xfd,0x01,BIT,0},{0x32,0x05,BIT,0},{0xfd,0x02,BIT,0},{0x26,0xc8,BIT,0},{0x27,0x89,BIT,0},{0xfd,0x00,BIT,0}},//DAYLIGHT
	{{0xfd,0x01,BIT,0},{0x32,0x05,BIT,0},{0xfd,0x02,BIT,0},{0x26,0xdc,BIT,0},{0x27,0x75,BIT,0},{0xfd,0x00,BIT,0}},//CLOUD
};

// use this for 640x480 (VGA) capture
static const struct sensor_reg vga_sp2508[] =
{
	{0xfd,0x01,BIT,0},
	{0xfd,0x00,BIT,0},
};

// use this for 320x240 (QVGA) capture
static const struct sensor_reg qvga_sp2508[] =
{
	{0xfd,0x01,BIT,0},
	{0xfd,0x00,BIT,0},
};


// use this for 1600*1200 (UXGA) capture
static const struct sensor_reg uxga_sp2508[] =
{
	{0xfd,0x01,BIT,0},
};

// use this for 176x144 (QCIF) capture
static const struct sensor_reg qcif_sp2508[] =
{
        {0xfd,0x00,BIT,0},
        {0xfd,0x01,BIT,0},
};

// use this for init sensor
static const struct sensor_reg init_sp2508[] =
{

        {0xfd,0x00,BIT,0},
        {0xfd,0x00,BIT,0},
        {0xfd,0x00,BIT,0},
        {0x35,0x20,BIT,0},//pll bias
        {0x2f,0x08,BIT,0},//pll clk 84M
        {0x1e,0xf5,BIT,0},//driver  differential signal
   //   {0x30,0x4c,BIT,0},//clock mode buf
        {0x1c,0x03,BIT,0},//pull down parallel pad
        {0xfd,0x01,BIT,0},
        {0x03,0x04,BIT,0},//exp time, 4 base
        {0x04,0x0c,BIT,0},
        {0x06,0x10,BIT,0},//vblank
        {0x24,0x10,BIT,0},//a0 //pga gain 10x
        {0x01,0x01,BIT,0},//enable reg write
        {0x2b,0xc4,BIT,0},//readout vref
        {0x2e,0x20,BIT,0},//dclk delay
        {0x79,0x42,BIT,0},//p39 p40
        {0x85,0x0f,BIT,0},//p51
        {0x09,0x01,BIT,0},//hblank
        {0x0a,0x40,BIT,0},

        {0x21,0xef,BIT,0},//pcp tx 4.05v
        {0x25,0xf2,BIT,0},//reg dac 2.7v, enable bl_en,vbl 1.4v
        {0x26,0x00,BIT,0},//vref2 1v, disable ramp driver
        {0x2a,0xea,BIT,0},//bypass dac res, adc range 0.745, vreg counter 0.9
        {0x2c,0xf0,BIT,0},//high 8bit, pldo 2.7v
        {0x8a,0x55,BIT,0},//pixel bias 2uA
        {0x8b,0x55,BIT,0},
        {0x19,0xf3,BIT,0},//icom1 1.7u, icom2 0.6u
        {0x11,0x30,BIT,0},//rst num
        {0xd0,0x01,BIT,0},//boost2 enable
        {0xd1,0x01,BIT,0},//boost2 start point h'1do
        {0xd2,0xd0,BIT,0},
        {0x55,0x10,BIT,0},
        {0x58,0x30,BIT,0},
        {0x5d,0x15,BIT,0},
        {0x5e,0x05,BIT,0},
        {0x64,0x40,BIT,0},
        {0x65,0x00,BIT,0},
        {0x66,0x66,BIT,0},
        {0x67,0x00,BIT,0},
        {0x68,0x68,BIT,0},
        {0x72,0x70,BIT,0},
        {0xfb,0x25,BIT,0},
        {0xf0,0x00,BIT,0},//offset
        {0xf1,0x00,BIT,0},
        {0xf2,0x00,BIT,0},
        {0xf3,0x00,BIT,0},
        {0xfd,0x02,BIT,0},//raw data digital gain
        {0x00,0xc6,BIT,0},//ad
        {0x01,0xc6,BIT,0},//ad
        {0x03,0xc6,BIT,0},//ad
        {0x04,0xc6,BIT,0},//ad

        {0xfd,0x01,BIT,0},//mipi
        {0xb3,0x00,BIT,0},
        {0x93,0x01,BIT,0},
        {0x9d,0x17,BIT,0},
        {0xc5,0x01,BIT,0},
        {0xc6,0x00,BIT,0},
        {0xb1,0x01,BIT,0},
        {0x8e,0x06,BIT,0},
        {0x8f,0x50,BIT,0},
        {0x90,0x04,BIT,0},
        {0x91,0xc0,BIT,0},
        {0x92,0x01,BIT,0},
        {0xa1,0x05,BIT,0},
        {0xaa,0x01,BIT,0},
        {0xac,0x01,BIT,0},

};

static const struct sensor_reg_list sp2508_init = {
	.size = ARRAY_ROW(init_sp2508),
	.val = init_sp2508
};

static const struct isp_reg_list sp2508_8850e_init= {
	.size = ARRAY_ROW(isp_8850e_sp2508),
	.val = isp_8850e_sp2508
};

static const struct sensor_reg_list sp2508_vga = {
	.size = ARRAY_ROW(vga_sp2508),
	.val = vga_sp2508
};
static const struct sensor_reg_list sp2508_qvga = {
	.size = ARRAY_ROW(qvga_sp2508),
	.val = qvga_sp2508
};
static const struct sensor_reg_list sp2508_uxga = {
	.size = ARRAY_ROW(uxga_sp2508),
	.val = uxga_sp2508
};
static const struct sensor_reg_list sp2508_qcif = {
	.size = ARRAY_ROW(qcif_sp2508),
	.val = qcif_sp2508
};
static const struct sensor_win_size sp2508_win_size[] = {
	WIN_SIZE("UXGA", W_UXGA, H_UXGA, &sp2508_uxga),
	WIN_SIZE("VGA", W_VGA, H_VGA, &sp2508_vga),
	WIN_SIZE("QVGA", W_QVGA, H_QVGA, &sp2508_qvga),
	WIN_SIZE("QCIF", W_QCIF, H_QCIF, &sp2508_qcif),
};

static const struct sensor_win_cfg sp2508_win_cfg = {
	.num = ARRAY_ROW(sp2508_win_size),
	.win_size = sp2508_win_size
};

static const struct sensor_csi_cfg sp2508_csi_cfg = {
	.csi_en = true,
	.d_term_en = 20,
	.c_term_en = 20,
	.dhs_settle = 20,
	.chs_settle = 20
};

static const struct ae_control ae_table_sp2508[] = {
//   Exp     ana_gain
	{0x0001,  0x10},     //<1E
	{0x0002,  0x10},     //<1E
	{0x0003,  0x10},     //<1E
	{0x0004,  0x10},     //<1E
	{0x0005,  0x10},     //<1E
	{0x0006,  0x10},     //<1E
	{0x0007,  0x10},     //<1E
	{0x0008,  0x10},     //<1E
	{0x0009,  0x10},     //<1E
	{0x000a,  0x10},     //<1E
	{0x000b,  0x10},     //<1E
	{0x000c,  0x10},     //<1E
	{0x000d,  0x10},     //<1E
	{0x000e,  0x10},     //<1E
	{0x000f,  0x10},     //<1E
	{0x0010,  0x10},     //<1E
	{0x0014,  0x10},     //<1E
	{0x0018,  0x10},     //<1E
	{0x0020,  0x10},     //<1E
	{0x0028,  0x10},     //<1E
	{0x0030,  0x10},     //<1E
	{0x0038,  0x10},     //<1E
	{0x0046,  0x10},     //<1E
	{0x0054,  0x10},     //<1E
	{0x0062,  0x10},     //<1E
	{0x0070,  0x10},     //<1E
	{0x007E,  0x10},     //<1E
	{0x008C,  0x10},     //<1E
	{0x009A,  0x10},     //<1E
	{0x00A8,  0x10},     //<1E
	{0x00B6,  0x10},     //<1E
	{0x00C4,  0x10},     //<1E
	{0x00D2,  0x10},     //<1E
	{0x00E0,  0x10},     //<1E
	{0x00EE,  0x10},     //<1E
	{0x00FC,  0x10},     //<1E
	{0x010A,  0x10},     //<1E
	{0x01,    0x10},     //1E
	{0x01,    0x12},     //1E
	{0x01,    0x14},     //1E
	{0x01,    0x16},     //1E
	{0x01,    0x19},     //1E
	{0x01,    0x1b},     //1E
	{0x01,    0x1e},     //1E
	{0x02,    0x10},     //2E
	{0x02,    0x12},     //2E
	{0x02,    0x14},     //2E
	{0x02,    0x16},     //2E
	{0x03,    0x10},     //3E
	{0x03,    0x12},     //3E
	{0x03,    0x14},     //3E
	{0x04,    0x10},     //4E
	{0x04,    0x12},     //4E
	{0x04,    0x14},     //4E
	{0x04,    0x16},     //4E
	{0x04,    0x18},     //4E
	{0x04,    0x1a},     //4E
	{0x04,    0x1c},     //4E
	{0x04,    0x1e},     //4E/
	{0x04,    0x20},     //4E
	{0x05,    0x1a},     //5E
	{0x05,    0x1c},     //5E
	{0x05,    0x1e},     //5E
	{0x05,    0x20},     //5E
	{0x05,    0x22},     //5E
	{0x05,    0x24},     //5E
	{0x05,    0x26},     //5E
	{0x05,    0x2a},     //5E
	{0x05,    0x2e},     //5E
	{0x05,    0x32},     //5E
	{0x06,    0x2e},     //6E
	{0x06,    0x32},     //6E
	{0x06,    0x36},     //6E
	{0x06,    0x3a},     //6E
	{0x06,    0x3e},     //6E
	{0x07,    0x3a},     //7E
	{0x07,    0x3e},     //7E
	{0x07,    0x44},     //7E
	{0x08,    0x40},     //8E
	{0x08,    0x48},     //8E
	{0x08,    0x50},     //8E
	{0x09,    0x4c},     //9E
	{0x09,    0x54},     //9E
	{0x09,    0x5c},     //9E
	{0x0a,    0x58},     //AE
	{0x0a,    0x60},     //AE
	{0x0a,    0x68},     //AE
	{0x0b,    0x64},     //BE
	{0x0b,    0x6c},     //BE
	{0x0b,    0x74},     //BE
	{0x0c,    0x70},     //CE
	{0x0c,    0x78},     //CE
	{0x0c,    0x80},     //CE
	{0x0c,    0x90},     //CE
	{0x0c,    0xa0},     //CE
	{0x0c,    0xb0},     //CE
	{0x0c,    0xc0},     //CE
};

static const struct ae_control_list sp2508_ae_table= {
	.size = ARRAY_ROW(ae_table_sp2508),
	.val = ae_table_sp2508
};

static struct raw_sensor_info_data  sp2508_raw_info = {
	.frame_line_num =  1208, //max line number of sensor output, to Camera MIPI.
	.frame_width     = 1608,//frame width of sensor output to ISP.
	.frame_height    = 1208, //frame height  sensor output.to ISP.
	.flicker_50      = 0x118,
	.flicker_60      = 0x104,
	.targetBV        = 0x60,
	.ae_table       = &sp2508_ae_table,
	.isp_sensor_init= &sp2508_8850e_init
};
static const struct sensor_info sp2508_info = {
	.name		= "sp2508",
	.chip_id	= 0x25,
	.type_sensor    = RAW,
	.mclk		= 25,
	.i2c_addr	= 0x3C,
	.exp_def	= 0,
	.awb_def	= 1,
	.rst_act_h	= false,
	.pdn_act_h	= true,
	.raw_sensor     = &sp2508_raw_info,
        .init		= &sp2508_init,
	.win_cfg	= &sp2508_win_cfg,
	.csi_cfg	= &sp2508_csi_cfg
};

extern void sensor_power_down(bool high, bool acth, int id);
extern void sensor_reset(bool rst, bool acth);
extern void sensor_clock(bool out, int mclk);
extern void sensor_read(const u16 addr, u8 *data, u8 bits);
extern void sensor_write(const u16 addr, const u8 data, u8 bits);
extern void sensor_write_group(struct sensor_reg* reg, u32 size);

static u32 sp2508_power(int id, int mclk, bool rst_h, bool pdn_h)
{
	/* set state to power off */
	sensor_power_down(true, pdn_h, 0);
	sensor_power_down(true, pdn_h, 1);
	mdelay(10);
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
	msleep(350);

	return 0;
}

static u32 sp2508_get_chipid(void)
{
	u32 chip_id = 0;

	sensor_write(0xfd, 0x00, BIT);
	mdelay(10);
	sensor_read(0x02, &chip_id, BIT);
	rda_dbg_camera("%s: chip_id = %x\n", __func__,chip_id);

	return chip_id;
}

static u32 sp2508_get_lum(void)
{
	u8 val = 0;
	u32 ret = 0;

	sensor_write(0xfd, 0x01, BIT);
	sensor_read(0x23, &val, BIT);
	sensor_write(0xfd, 0x00, BIT);

	if (val > 0x50)
		ret = 1;

	return ret;
}



#define SP2508_FLIP_BASE	0x3F
#define SP2508_H_FLIP_BIT	0
#define SP2508_V_FLIP_BIT		1
static void sp2508_set_flip(int hv, int flip)
{
	u8 tmp = 0;

	sensor_read(SP2508_FLIP_BASE, &tmp, BIT);

	if (hv) {
		if (flip)
			tmp |= (0x1 << SP2508_V_FLIP_BIT);
		else
			tmp &= ~(0x1 << SP2508_V_FLIP_BIT);
	}
	else {
		if (flip)
			tmp |= (0x1 << SP2508_H_FLIP_BIT);
		else
			tmp &= ~(0x1 << SP2508_H_FLIP_BIT);
	}

	sensor_write(0xfd, 0x01, BIT);
	sensor_write(SP2508_FLIP_BASE, tmp, BIT);
	sensor_write(0xfd, 0x00, BIT);
}

static void sp2508_upd_exp_isp(int val)
{
	u8 tmp3, tmp4 = 0;
	tmp3 = (val>>8) & 0xff;
	tmp4 = (val) & 0xff;
	sensor_write(0xfd, 0x01, BIT);
	sensor_write(0x03, tmp3, BIT);
	sensor_write(0x04, tmp4, BIT);
	sensor_write(0x01, 0x01, BIT);
}

#define SP2508_GAIN_ISP_ROW		ARRAY_ROW(gain_sp2508_from_isp)
#define SP2508_GAIN_ISP_COL		ARRAY_COL(gain_sp2508_from_isp)
static void sp2508_upd_gain_isp(int val)
{
        sensor_write(0xfd, 0x01, BIT);
        sensor_write(0x24, (u8)val, BIT);
        sensor_write(0x01, 0x01, BIT);
}

/*
#define SP2508_EXP_ROW		ARRAY_ROW(exp_sp2508)
#define SP2508_EXP_COL		ARRAY_COL(exp_sp2508)
static void sp2508_set_exp(int exp)
{
	return; // need to be done by ISP
	int key = exp + (SP2508_EXP_ROW / 2);
	if ((key < 0) || (key > (SP2508_EXP_ROW - 1)))
		return;

	sensor_write_group(exp_sp2508[key], SP2508_EXP_COL);
}
*/
#define SP2508_AWB_ROW		ARRAY_ROW(awb_sp2508)
#define SP2508_AWB_COL		ARRAY_COL(awb_sp2508)
static void sp2508_set_awb(int awb)
{
	return; // need to be done by ISP
	if ((awb < 0) || (awb > (SP2508_AWB_ROW - 1)))
		return;

	sensor_write_group(awb_sp2508[awb], SP2508_AWB_COL);
}

static struct sensor_ops sp2508_ops = {
	.power		= sp2508_power,
	.get_chipid	= sp2508_get_chipid,
	.get_lum	= sp2508_get_lum,
	.set_flip	= sp2508_set_flip,
	.set_exp	= NULL, //sp2508_set_exp,
	.set_awb	= sp2508_set_awb,
	.upd_gain_isp   = sp2508_upd_gain_isp,
	.upd_exp_isp    = sp2508_upd_exp_isp,
	.start		= NULL,
	.stop		= NULL
};

struct sensor_dev sp2508_dev = {
	.info	= &sp2508_info,
	.ops	= &sp2508_ops,
};

#undef BIT
#endif
