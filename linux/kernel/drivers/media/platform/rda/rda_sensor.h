/*
 * rda_sensor.h
 *
 * Copyright (C) 2014 Rda electronics, Inc.
 *
 * Contact: Xing Wei <xingwei@rdamicro.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 */

#ifndef _RDA_SENSOR_H_
#define _RDA_SENSOR_H_

/* Macro */
#define WIN_SIZE(n, w, h, r)	\
	{			\
		.name = n,	\
		.width = w,	\
		.height = h,	\
		.win_val = r	\
	}


enum sensor_w {
	W_QQVGA	= 160,
	W_QCIF	= 176,
	W_QVGA	= 320,
	W_CIF	= 352,
	W_VGA	= 640,
	W_SVGA	= 800,
	W_XGA	= 1024,
	W_SXGA	= 1280,
	W_720P	= 1280,
	W_UXGA	= 1600,
};
enum sensor_h {
	H_QQVGA	= 120,
	H_QCIF	= 144,
	H_QVGA	= 240,
	H_CIF	= 288,
	H_VGA	= 480,
	H_SVGA	= 600,
	H_720P	= 720,
	H_XGA	= 768,
	H_SXGA	= 960,
	H_UXGA	= 1200,
};

enum sensor_type {
	YUV= 0,
	RAW
};

struct sensor_reg {
	u16 addr; //for 8bits regs addr, only use the low 8bits
	u8 data;
	u8 bits;
	u32 wait;
};
struct sensor_reg_list {
	u32 size;
	struct sensor_reg *val;
};

#define ARRAY_ROW(a)    (ARRAY_SIZE(a))
#define ARRAY_COL(a)    (ARRAY_SIZE(a[0]))

struct sensor_win_size {
	char *name;
	enum sensor_w width;
	enum sensor_h height;
	struct sensor_reg_list *win_val;
};

struct sensor_win_cfg {
	int num;
	struct sensor_win_size *win_size;
};

struct sensor_csi_cfg {
	bool csi_en;
	u8 d_term_en;
	u8 c_term_en;
	u8 dhs_settle;
	u8 chs_settle;
};

//static const struct sensor_reg_list *exp_val;
//static const struct sensor_reg_list *wb_val;

struct sensor_info {
	char name[32];
	u16 chip_id;
	u8 mclk;
	u8 i2c_addr;
	int exp_def;
	int awb_def;
	int bri_def;
	int con_def;
	int sha_def;
	int sat_def;
	int af_def;
	bool rst_act_h;
	bool pdn_act_h;
	enum sensor_type type_sensor; //raw or yuv sensor, 0->YUV, 1->RAW.
	struct raw_sensor_info_data *raw_sensor;
	struct sensor_reg_list *init;
	struct sensor_reg_list *test;
	struct sensor_win_cfg *win_cfg;
	struct sensor_csi_cfg *csi_cfg;
};

struct sensor_ops {
	u32(*power) (int id, int mclk, bool rst_h, bool pdn_h);
	u32(*get_chipid) (void);
	u32(*get_lum) (void);
	void(*set_flip) (int hv, int flip);
	void(*set_exp) (int exp);
	void(*set_awb) (int awb);
	void(*set_bri) (int bri);
	void(*set_con) (int con);
	void(*set_sha) (int sha);
	void(*set_sat) (int sat);
	void(*set_af)(int af);
	void(*upd_gain_isp)(int val);
	void(*upd_exp_isp)(int val);
	void(*sensor_test) (void);
	void(*start) (void);
	void(*stop) (void);
};

struct sensor_callback_ops {
	void(*cb_pdn) (bool pdn, bool acth, int id);
	void(*cb_rst) (bool rst, bool acth);
	void(*cb_clk) (bool out, int mclk);
	void(*cb_i2c_r) (const u16 addr, u8 *data, u8 bits);
	void(*cb_i2c_w) (const u16 addr, const u8 data, u8 bits);
	void(*cb_isp_reg_write) (const u32 addr_offset, const u8 data);
	void(*cb_isp_set_exp) (int exp);
	void(*cb_isp_set_awb) (int awb);
	void(*cb_isp_set_af)(int af);
};

struct sensor_dev {
	struct list_head list;
	struct sensor_info *info;
	struct sensor_ops *ops;
	struct sensor_callback_ops *cb;
};

/* Function */
extern int rda_sensor_adapt(struct sensor_callback_ops *callback,
		struct list_head *sdl, u32 id);
#endif
