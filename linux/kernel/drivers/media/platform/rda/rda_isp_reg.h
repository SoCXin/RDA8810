/*
 * rda_isp.h
 *
 * Copyright (C) 2014 Rda electronics, Inc.
 *
 * Contact: Yongxia Xie <yongxiaxie@rdamicro.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 */

#ifndef _RDA_ISP_H_
#define _RDA_ISP_H_

/* Macro */

/* Structure */
struct isp_reg {
	u16 addr_offset; //ISP address are all 32 bits
	u8 data;
};

struct isp_reg_list {
	u32 size;
	struct isp_reg *val;
};


struct sensor_status {
	u16  exp;
	u16  gain;
	u16  targetBV;
	u8   sensor_stable;
	u8   tBV_dec;//number of target BrightValue need to decrease
};

struct ae_control {
	u16 expo;
	u16 gain;
};

struct ae_control_list {
	u32 size;
	struct ae_control *val;
};


struct raw_sensor_info_data {
	u16  frame_line_num; //max line number of sensor output, to Camera MIPI.
	u16  frame_width; //frame width of sensor output to ISP.
	u16  frame_height; //frame height  sensor output.to ISP.
	u16  flicker_50;
	u16  flicker_60;
	u16  targetBV;
        struct ae_control_list *ae_table;
        struct isp_reg_list *isp_sensor_init;
};
/* Function */
//extern int rda_sensor_adapt(struct sensor_callback_ops *callback,
#endif
