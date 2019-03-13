#ifndef _gc2355_CFG_H_
#define _gc2355_CFG_H_

#include "rda_isp_reg.h"
#include "rda_sensor.h"
#include "rda_8850e_isp_regname.h"
#include <linux/delay.h>

#ifdef BIT
#undef BIT
#endif
#define BIT	8



static const struct isp_reg isp_8850e_gc2355[] =
{
	{ISP_PAGE,0x00},
};

static const struct sensor_reg awb_gc2355[][6] =
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
static const struct sensor_reg vga_gc2355[] =
{
//      {0xfe,0x00,BIT,0},
};

// use this for 320x240 (QVGA) capture
static const struct sensor_reg qvga_gc2355[] =
{
//     {0xfe,0x00,BIT,0},
};


// use this for 1600*1200 (UXGA) capture
static const struct sensor_reg uxga_gc2355[] =
{
//	{0xfe,0x00,BIT,0},
};

// use this for 176x144 (QCIF) capture
static const struct sensor_reg qcif_gc2355[] =
{
//	{0xfe,0x00,BIT,0},
};

// use this for init sensor
static const struct sensor_reg init_gc2355[] =
{
	////////////////////////////////////////////////////
	//////////////////////	 SYS   //////////////////////
	/////////////////////////////////////////////////////
	{0xfe,0x80,BIT,0},
	{0xfe,0x80,BIT,0},
	{0xfe,0x80,BIT,0},
	{0xf2,0x00,BIT,0},
	{0xf6,0x00,BIT,0},
	{0xf7,0x31,BIT,0},
	{0xf8,0x06,BIT,0},
	{0xf9,0x0e,BIT,0},
	{0xfa,0x00,BIT,0},
	{0xfc,0x06,BIT,0},
	{0xfe,0x00,BIT,0},

	/////////////////////////////////////////////////////
	///////////////    ANALOG & CISCTL    ///////////////
	/////////////////////////////////////////////////////
	{0x03,0x07,BIT,0},
	{0x04,0xd0,BIT,0},
	{0x05,0x03,BIT,0},
	{0x06,0x4c,BIT,0},
	{0x07,0x00,BIT,0},
	{0x08,0x12,BIT,0},
	{0x0a,0x00,BIT,0},
	{0x0c,0x04,BIT,0},
	{0x0d,0x04,BIT,0},
	{0x0e,0xc0,BIT,0},
	{0x0f,0x06,BIT,0},
	{0x10,0x50,BIT,0},
	{0x17,0x17,BIT,0},
	{0x19,0x0b,BIT,0},
	{0x1b,0x48,BIT,0},
	{0x1c,0x12,BIT,0},
	{0x1d,0x10,BIT,0},
	{0x1e,0xbc,BIT,0},
	{0x1f,0xc9,BIT,0},
	{0x20,0x71,BIT,0},
	{0x21,0x20,BIT,0},
	{0x22,0xa0,BIT,0},
	{0x23,0x51,BIT,0},
	{0x24,0x19,BIT,0},
	{0x27,0x20,BIT,0},
	{0x28,0x00,BIT,0},
	{0x2b,0x80,BIT,0},
	{0x2c,0x38,BIT,0},
	{0x2e,0x16,BIT,0},
	{0x2f,0x14,BIT,0},
	{0x30,0x00,BIT,0},
	{0x31,0x01,BIT,0},
	{0x32,0x02,BIT,0},
	{0x33,0x03,BIT,0},
	{0x34,0x07,BIT,0},
	{0x35,0x0b,BIT,0},
	{0x36,0x0f,BIT,0},

	/////////////////////////////////////////////////////
	//////////////////////   MIPI   /////////////////////
	/////////////////////////////////////////////////////
	{0xfe, 0x03,BIT,0},
	{0x10, 0x81,BIT,0},
	{0x01, 0x87,BIT,0},
	{0x22, 0x03,BIT,0},
	{0x23, 0x20,BIT,0},
	{0x25, 0x10,BIT,0},
	{0x29, 0x02,BIT,0},
	{0x02, 0x00,BIT,0},
	{0x03, 0x90,BIT,0},
	{0x04, 0x01,BIT,0},
	{0x05, 0x00,BIT,0},
	{0x06, 0xa2,BIT,0},
	{0x11, 0x2b,BIT,0},
	{0x12, 0xd0,BIT,0},
	{0x13, 0x07,BIT,0},
	{0x15, 0x60,BIT,0},
	{0x21, 0x10,BIT,0},
	{0x24, 0x02,BIT,0},
	{0x26, 0x08,BIT,0},
	{0x27, 0x06,BIT,0},
	{0x2a, 0x0a,BIT,0},
	{0x2b, 0x08,BIT,0},
	{0x40, 0x00,BIT,0},
	{0x41, 0x00,BIT,0},
	{0x42, 0x40,BIT,0},
	{0x43, 0x06,BIT,0},
	{0xfe, 0x00,BIT,0},

	/////////////////////////////////////////////////////
	//////////////////////	 gain   /////////////////////
	/////////////////////////////////////////////////////
	{0xb0,0x50,BIT,0},
	{0xb1,0x01,BIT,0},
	{0xb2,0x00,BIT,0},
	{0xb3,0x40,BIT,0},
	{0xb4,0x40,BIT,0},
	{0xb5,0x40,BIT,0},
	{0xb6,0x00,BIT,0},

	/////////////////////////////////////////////////////
	//////////////////////   crop   /////////////////////
	/////////////////////////////////////////////////////
	{0x92,0x02,BIT,0},
	{0x94,0x00,BIT,0},
	{0x95,0x04,BIT,0},
	{0x96,0xb0,BIT,0},
	{0x97,0x06,BIT,0},
	{0x98,0x40,BIT,0},

	/////////////////////////////////////////////////////
	//////////////////////    BLK   /////////////////////
	/////////////////////////////////////////////////////
	{0x18,0x02,BIT,0},
	{0x1a,0x01,BIT,0},
	{0x40,0x42,BIT,0},
	{0x41,0x00,BIT,0},
	{0x44,0x00,BIT,0},
	{0x45,0x00,BIT,0},
	{0x46,0x00,BIT,0},
	{0x47,0x00,BIT,0},
	{0x48,0x00,BIT,0},
	{0x49,0x00,BIT,0},
	{0x4a,0x00,BIT,0},
	{0x4b,0x00,BIT,0},
	{0x4e,0x3c,BIT,0},
	{0x4f,0x00,BIT,0},
	{0x5e,0x00,BIT,0},
	{0x66,0x20,BIT,0},
	{0x6a,0x02,BIT,0},
	{0x6b,0x02,BIT,0},
	{0x6c,0x02,BIT,0},
	{0x6d,0x02,BIT,0},
	{0x6e,0x02,BIT,0},
	{0x6f,0x02,BIT,0},
	{0x70,0x02,BIT,0},
	{0x71,0x02,BIT,0},

	/////////////////////////////////////////////////////
	////////////////////  dark sun  /////////////////////
	/////////////////////////////////////////////////////
	{0x87,0x03,BIT,0},
	{0xe0,0xe7,BIT,0},
	{0xe3,0xc0,BIT,0},

	/////////////////////////////////////////////////////
	//////////////////////   MIPI   /////////////////////
	/////////////////////////////////////////////////////
	{0xfe, 0x03,BIT,0},
	{0x10, 0x91,BIT,0},
	{0xfe, 0x00,BIT,0},
};

static const struct sensor_reg_list gc2355_init = {
	.size = ARRAY_ROW(init_gc2355),
	.val = init_gc2355
};

static const struct isp_reg_list gc2355_8850e_init= {
	.size = ARRAY_ROW(isp_8850e_gc2355),
	.val = isp_8850e_gc2355
};

static const struct sensor_reg_list gc2355_vga = {
	.size = ARRAY_ROW(vga_gc2355),
	.val = vga_gc2355
};
static const struct sensor_reg_list gc2355_qvga = {
	.size = ARRAY_ROW(qvga_gc2355),
	.val = qvga_gc2355
};
static const struct sensor_reg_list gc2355_uxga = {
	.size = ARRAY_ROW(uxga_gc2355),
	.val = uxga_gc2355
};
static const struct sensor_reg_list gc2355_qcif = {
	.size = ARRAY_ROW(qcif_gc2355),
	.val = qcif_gc2355
};
static const struct sensor_win_size gc2355_win_size[] = {
	WIN_SIZE("UXGA", W_UXGA, H_UXGA, &gc2355_uxga),
	WIN_SIZE("VGA", W_VGA, H_VGA, &gc2355_vga),
	WIN_SIZE("QVGA", W_QVGA, H_QVGA, &gc2355_qvga),
	WIN_SIZE("QCIF", W_QCIF, H_QCIF, &gc2355_qcif),
};

static const struct sensor_win_cfg gc2355_win_cfg = {
	.num = ARRAY_ROW(gc2355_win_size),
	.win_size = gc2355_win_size
};

static const struct sensor_csi_cfg gc2355_csi_cfg = {
	.csi_en = true,
	.d_term_en = 5,
	.c_term_en = 5,
	.dhs_settle = 5,
	.chs_settle = 5,
};

static const struct ae_control ae_table_gc2355[] = {
//       Exp            ana_gain
//{p1:0x03,p1:0x04}     p1:0xb6,0xb1,0xb2
//
	{0x0001, 64},     //<1E
	{0x0002, 64},     //<1E
	{0x0003, 64},     //<1E
	{0x0004, 64},     //<1E
	{0x0005, 64},     //<1E
	{0x0006, 64},     //<1E
	{0x0007, 64},     //<1E
	{0x0008, 64},     //<1E
	{0x0009, 64},     //<1E
	{0x000a, 64},     //<1E
	{0x000b, 64},     //<1E
	{0x000c, 64},     //<1E
	{0x000d, 64},     //<1E
	{0x000e, 64},     //<1E
	{0x000f, 64},     //<1E
	{0x0010, 64},     //<1E
	{0x0014, 64},     //<1E
	{0x0018, 64},     //<1E
	{0x0020, 64},     //<1E
	{0x0028, 64},     //<1E
	{0x0030, 64},     //<1E
	{0x0038, 64},     //<1E
	{0x0046, 64},     //<1E
	{0x0054, 64},     //<1E
	{0x0062, 64},     //<1E
	{0x0070, 64},     //<1E
	{0x007E, 64},     //<1E
	{0x008C, 64},     //<1E
	{0x009A, 64},     //<1E
	{0x00A8, 64},     //<1E
	{0x00B6, 64},     //<1E
	{0x00C4, 64},     //<1E
	{0x00D2, 64},     //<1E
	{0x00E0, 64},     //<1E
	{0x00EE, 64},     //<1E
	{0x00FC, 64},     //<1E
	{1,      64},     //1E
	{1,      70},     //1E
	{1,      76},     //1E
	{1,      82},     //1E
	{1,      88},     //1E
	{1,      96},     //1E
	{1,     104},     //1E
	{1,     112},     //2E
	{1,     120},     //2E
	{2,      70},   //2E
	{2,      76},   //2E
	{2,      82},   //2E
	{2,      88},   //2E
	{2,      96},   //2E
	{3,      70},     //3E
	{3,      76},     //3E
	{3,      82},     //3E
	{3,      88},     //3E
	{4,      70},     //1E
	{4,      76},     //1E
	{4,      82},     //1E
	{4,      88},     //1E
	{4,      96},     //1E
	{4,     104},     //1E
	{4,     112},     //2E
	{4,     120},     //2E
	{4,     128},     //4E
	{5,     104},     //5E
	{5,     112},     //5E
	{5,     120},     //5E
	{5,     128},     //5E
	{5,     140},     //5E
	{5,     152},     //5E
	{5,     164},     //5E
	{5,     177},     //5E
	{5,     195},     //5E
	{5,     213},     //5E
	{6,     186},     //5E
	{6,     204},     //5E
	{6,     222},     //5E
	{6,     240},     //5E
	{6,     275},     //5E
	{7,     257},     //6E
	{7,     293},     //6E
	{7,     329},     //6E
	{8,     311},     //6E
	{8,     347},     //6E
	{8,     383},     //6E
	{9,     347},  //6E
	{9,     383},  //6E
	{9,     419},  //6E
	{0x0a,  383},   //6E
	{0x0a,  419},   //6E
	{0x0a,  455},   //6E
	{0x0b,  419},  //7E
	{0x0b,  455},  //7E
	{0x0b,  491},  //7E
	{0x0c,  455},  //7E
	{0x0c,  491},  //7E
	{0x0c,  527},  //7E
	{0x0c,  563},  //7E
	{0x0c,  599},  //7E
};

static const struct ae_control_list gc2355_ae_table= {
	.size = ARRAY_ROW(ae_table_gc2355),
	.val = ae_table_gc2355
};

static struct raw_sensor_info_data  gc2355_raw_info = {
	.frame_line_num = 1200, //max line number of sensor output, to Camera MIPI.
	.frame_width     = 1600,//frame width of sensor output to ISP.
	.frame_height    = 1200, //frame height  sensor output.to ISP.
	.flicker_50      = 0x118,
	.flicker_60      = 0x104,
	.targetBV        = 0x60,
	.ae_table       = &gc2355_ae_table,
	.isp_sensor_init= &gc2355_8850e_init
};
static const struct sensor_info gc2355_info = {
	.name		= "gc2355",
	.chip_id	= 0x2355,
	.type_sensor    = RAW,
	.mclk		= 26,
	.i2c_addr	= 0x3C,
	.exp_def	= 0,
	.awb_def	= 1,
	.rst_act_h	= false,
	.pdn_act_h	= true,
	.raw_sensor     = &gc2355_raw_info,
	.init		= &gc2355_init,
	.win_cfg	= &gc2355_win_cfg,
	.csi_cfg	= &gc2355_csi_cfg
};

extern void sensor_power_down(bool high, bool acth, int id);
extern void sensor_reset(bool rst, bool acth);
extern void sensor_clock(bool out, int mclk);
extern void sensor_read(const u16 addr, u8 *data, u8 bits);
extern void sensor_write(const u16 addr, const u8 data, u8 bits);
extern void sensor_write_group(struct sensor_reg* reg, u32 size);

static u32 gc2355_power(int id, int mclk, bool rst_h, bool pdn_h)
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

static u32 gc2355_get_chipid(void)
{
	u16 chip_id = 0;
	u8 tmp;

	sensor_read(0xf0, &tmp, BIT);
	chip_id = (tmp << 8) & 0xff00;
	sensor_read(0xf1, &tmp, BIT);
	chip_id |= (tmp & 0xff);

	return chip_id;
}

static u32 gc2355_get_lum(void)
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



#define gc2355_FLIP_BASE	0x17
#define gc2355_H_FLIP_BIT	0
#define gc2355_V_FLIP_BIT	1
static void gc2355_set_flip(int hv, int flip)
{
	u8 tmp = 0;

	sensor_read(gc2355_FLIP_BASE, &tmp, BIT);

	if (hv) {
		if (flip)
			tmp |= (0x1 << gc2355_V_FLIP_BIT);
		else
			tmp &= ~(0x1 << gc2355_V_FLIP_BIT);
	}
	else {
		if (flip)
			tmp |= (0x1 << gc2355_H_FLIP_BIT);
		else
			tmp &= ~(0x1 << gc2355_H_FLIP_BIT);
	}

	sensor_write(gc2355_FLIP_BASE, tmp, BIT);
}

static void gc2355_upd_exp_isp(int val)
{
	u8 tmp3, tmp4 = 0;
	tmp3 = (val>>8) & 0x0f;
	tmp4 = (val) & 0xff;
	sensor_write(0x03, tmp3, BIT);
	sensor_write(0x04, tmp4, BIT);
}


#define ANALOG_GAIN_1 64  // 1.00x
#define ANALOG_GAIN_2 88  // 1.375x
#define ANALOG_GAIN_3 122  // 1.90x
#define ANALOG_GAIN_4 168  // 2.625x
#define ANALOG_GAIN_5 239  // 3.738x
#define ANALOG_GAIN_6 330  // 5.163x
#define ANALOG_GAIN_7 470  // 7.350x
static void gc2355_upd_gain_isp(int val)
{
//0xB6 is analog gain
//0xB1,0xB2 is digital gain, MSB 6 bit is integral part, LSB 6 bit is floating part.
        int gain ;
	int temp;
        int  dig_gain_b0, dig_gain_b1;

	sensor_write(0xb1, 0x01, BIT);
	sensor_write(0xb2, 0x00, BIT);

        gain = val;
        if (gain < 0x40)
                gain = 0x40;

        if  (( ANALOG_GAIN_1 <= gain ) && (gain < ANALOG_GAIN_2)){
	        sensor_write(0xb6, 0x00, BIT);
                temp = val;
        } else if  (( ANALOG_GAIN_2 <= gain ) && (gain < ANALOG_GAIN_3)){
	        sensor_write(0xb6, 0x01, BIT);
                temp = val* 64 / ANALOG_GAIN_2;
        } else if  (( ANALOG_GAIN_3 <= gain ) && (gain < ANALOG_GAIN_4)){

	        sensor_write(0xb6, 0x02, BIT);
                temp = val * 64 / ANALOG_GAIN_3;
        } else if  ( ANALOG_GAIN_4 <= gain ){

	        sensor_write(0xb6, 0x03, BIT);
                temp = val * 64 / ANALOG_GAIN_4;
        }

        dig_gain_b0 = (temp>>6);
        dig_gain_b1 = (temp<<2) & 0xfc;
	sensor_write(0xb1, (dig_gain_b0), BIT);
	sensor_write(0xb2, (dig_gain_b1), BIT);

}

/*
#define gc2355_EXP_ROW		ARRAY_ROW(exp_gc2355)
#define gc2355_EXP_COL		ARRAY_COL(exp_gc2355)
static void gc2355_set_exp(int exp)
{
	return; // need to be done by ISP
	int key = exp + (gc2355_EXP_ROW / 2);
	if ((key < 0) || (key > (gc2355_EXP_ROW - 1)))
		return;

	sensor_write_group(exp_gc2355[key], gc2355_EXP_COL);
}
*/
#define gc2355_AWB_ROW		ARRAY_ROW(awb_gc2355)
#define gc2355_AWB_COL		ARRAY_COL(awb_gc2355)
static void gc2355_set_awb(int awb)
{
	return; // need to be done by ISP
	if ((awb < 0) || (awb > (gc2355_AWB_ROW - 1)))
		return;

	sensor_write_group(awb_gc2355[awb], gc2355_AWB_COL);
}

static struct sensor_ops gc2355_ops = {
	.power		= gc2355_power,
	.get_chipid	= gc2355_get_chipid,
	.get_lum	= gc2355_get_lum,
	.set_flip	= gc2355_set_flip,
	.set_exp	= NULL, //gc2355_set_exp,
	.set_awb	= gc2355_set_awb,
	.upd_gain_isp   = gc2355_upd_gain_isp,
	.upd_exp_isp    = gc2355_upd_exp_isp,
	.start		= NULL,
	.stop		= NULL
};

struct sensor_dev gc2355_dev = {
	.info	= &gc2355_info,
	.ops	= &gc2355_ops,
};

#undef BIT
#endif
