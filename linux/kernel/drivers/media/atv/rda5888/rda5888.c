/*
 * rda5888.c - RDA5888 Device Driver.
 *
 * Copyright (C) 2013 RDA Microelectronics Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/device.h>
#include <linux/kdev_t.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/platform_device.h>
#include <linux/dma-mapping.h>
#include <linux/mm_types.h>
#include <linux/mm.h>
#include <linux/jiffies.h>
#include <linux/sched.h>
#include <asm/uaccess.h>
#include <asm/page.h>
#include <linux/vmalloc.h>
#include <linux/interrupt.h>
#include <mach/irqs.h>
#include <linux/wait.h>
#include <linux/spinlock.h>
#include <linux/delay.h>
#include <linux/semaphore.h>
#include <linux/i2c.h>
#include <linux/slab.h>
#include <linux/audiocontrol.h>

#include "rda5888.h"
#include "rda5888_reg_cfg.h"
#include <linux/clk.h>
#include <mach/rda_clk_name.h>
#include <mach/board.h>
#include <rda/tgt_ap_board_config.h>

#ifdef _TGT_AP_BOARD_HAS_ATV

/*
 * Definition
 */
#define RDA5888_DBG_EN

#ifdef RDA5888_DBG_EN
#define RDA5888_LOGD printk
#else
#define RDA5888_LOGD(...)
#endif
#define RDA5888_LOGE printk

/* check i2c read/write error. */
//#define RDA5888_I2C_RW_CHECK

#define RDA5888_DEVNAME "rda5888"

#define RDA5888_I2C_DEVNAME "rda5888_i2c"
#define RDA5888_I2C_ADDR	(0x62) /* 7 bits i2c addr. */

#define RDA5888_SCAN_THRESHOLD		65 /* defaul: 65 */

/*
 * The I2C Bus Channel Number.
 */
//#define RDA5888_I2C_BUS_NUM (0)

struct rda5888_pri_data {
	u16 curr_page;
	u32 freq_offset;
	bool is_625_lines;
};

struct rda5888_dev {
	struct cdev *device;
	dev_t devno;
	struct class *cls;
	int major;
	struct i2c_client *client;
	bool i2c_probed;
	bool poweron;
	struct clk *clk26m;
	struct audiocontrol_dev acdev;

	struct rda5888_pri_data pri_data;
};

struct atvvolume{
	int vol_db;
	int volume;
};

static const struct atvvolume atvvol[] = {
	{0, 0x0 },
	{600, 0x1},  // 0001, 600 means 6db
	{1200, 0x2}, // 0010
	{1556, 0x3}, //0011, 1556 means 15.56db
	{1841, 0x4}, // 0100
	{2056, 0x5}, // 0101
	{2182, 0x6}, // 0110
	{2292, 0x7}, // 0111
	{2371, 0x8}, // 1000
	{2443, 0x9}, // 1001
	{2526, 0xa}, // 1010
	{2602, 0xb}, // 1011
	{2630, 0xc}, // 1100
	{2795, 0xd}, // 1101
	{2924, 0xe}, // 1110
	{3045, 0xf}, // 1111
};

struct rda5888_dev *rda5888_d;

#if 0
static int rda5888_cust_power_on(void)
{
	return 0;
}

int rda5888_cust_power_off(void)
{
	return 0;
}
#endif

static void enable_26m_rtc_atv(void)
{
	clk_prepare_enable(rda5888_d->clk26m);
}

static void disable_26m_rtc_atv(void)
{
	clk_disable_unprepare(rda5888_d->clk26m);
}

static int rda5888_i2c_read_word(u8 cmd, u16 *data)
{
	char cmd_buf[1] = {0x00};
	char temp_data[2] = {0x00};
	int ret = 0;

	cmd_buf[0] = cmd;
	ret = i2c_master_send(rda5888_d->client, &cmd_buf[0], 1);
	if (ret < 0) {
		RDA5888_LOGE("[rdamtv]%s, error! \n", __func__);
		return 1;
	}

	ret = i2c_master_recv(rda5888_d->client, temp_data, 2);
	if (ret < 0) {
		RDA5888_LOGE("[rdamtv]%s, error!! \n", __func__);
		return 1;
	}

	*data = temp_data[0];
	*data = (*data<<8)|temp_data[1];

	return 0;
}

static int rda5888_i2c_write_word(u8 cmd, u16 data)
{
	char temp_data[3] = {0};
	int ret = 0;

	temp_data[0] = cmd;
	temp_data[1] = (data>>8)&0xff;
	temp_data[2] = data&0xff;

	ret = i2c_master_send(rda5888_d->client, temp_data, 3);
	if (ret < 0) {
		RDA5888_LOGE("[rdamtv]%s, error: 0x%x\n", __func__, cmd);
		return 1;
	}

	return 0;
}

static int rda5888_write_reg(u16 addr, const u16 data)
{
	/* do page switch. */
	if (addr < 0x100) {
		if (rda5888_d->pri_data.curr_page != 0x0) {
			rda5888_d->pri_data.curr_page = 0x0;
			rda5888_i2c_write_word(0xff, 0x0);
		}
	} else if (rda5888_d->pri_data.curr_page != 0x1){
		rda5888_d->pri_data.curr_page = 0x1;
		rda5888_i2c_write_word(0xff, 0x1);
	}

	/* start write reg. */
	addr &= 0x00ff;
	rda5888_i2c_write_word((u8)addr, data);

	return 0;
}

static int rda5888_read_reg(u16 addr, u16 *data)
{
	/* do page switch. */
	if (addr < 0x100) {
		if (rda5888_d->pri_data.curr_page != 0x0) {
			rda5888_d->pri_data.curr_page = 0x0;
			rda5888_i2c_write_word(0xff, 0x0);
		}
	} else if (rda5888_d->pri_data.curr_page != 0x1){
		rda5888_d->pri_data.curr_page = 0x1;
		rda5888_i2c_write_word(0xff, 0x1);
	}

	/* start read reg. */
	addr &= 0x00ff;
	rda5888_i2c_read_word((u8)addr, data);

	return 0;
}

/*
 * Write a list of register settings;
 */
static int rda5888_write_array(const struct rda_reg_data *vals, uint size)
{
	int i, ret;
#ifdef RDA5888_I2C_RW_CHECK
	unsigned short temp_data;
#endif

	if (size == 0)
		return -EINVAL;

	for (i = 0; i < size ; i++)	{
		if (vals->reg_num == RDA5888_DELAY_FLAG) {
			mdelay(vals->value);
		} else {
			ret = rda5888_write_reg(vals->reg_num, vals->value);
			if (ret < 0) {
				RDA5888_LOGE("[rdamtv]%s, err!\n", __func__);
				return ret;
			}
			#ifdef RDA5888_I2C_RW_CHECK
				rda5888_read_reg(vals->reg_num, &temp_data);
				if (temp_data != vals->value) {
					RDA5888_LOGE("[rdamtv]%s, err: reg=0x%x, write:0x%x, read:0x%x\n",
								__func__, vals->reg_num, vals->value, temp_data);
				}
			#endif
		}
		udelay(500);
		vals++;
	}

	return 0;
}

static int rda5888_rf_init(void)
{
	RDA5888_LOGD("[rdamtv]%s.\n", __func__);

	return rda5888_write_array(rda5888_rf_init_datas,
						sizeof(rda5888_rf_init_datas)/sizeof(struct rda_reg_data));
}

static int rda5888_dsp_init(void)
{
	RDA5888_LOGD("[rdamtv]%s.\n", __func__);

	return rda5888_write_array(rda5888_dsp_init_datas,
						sizeof(rda5888_dsp_init_datas)/sizeof(struct rda_reg_data));
}

static int rda5888_26m_crystal_init(void)
{
#if (RDA5888_SHARE_26MCRYSTAL == 1)
	return rda5888_write_array(rda5888_26m_crystal_init_datas,
						sizeof(rda5888_26m_crystal_init_datas)/sizeof(struct rda_reg_data));
#else
	return 0;
#endif
}

static int rda5888_field_write_reg(u32 addr, u16 field_mask, u16 field_data)
{
	u32 rt = RDAMTV_RT_SUCCESS;
	u16 read_data;
	u16 i = 0;
	u16 val = field_mask;

	while ((!(val&0x0001))&&(i < 15)) {
		i++;
		val = (val >> 1);
	}

	rda5888_read_reg(addr, &read_data);
	read_data = (read_data & (~field_mask)) |(field_data<<i);

	rda5888_write_reg(addr, read_data);

	return rt;
}

static u16 rda5888_field_read_reg(u32 addr, u16 field_mask)
{
	u16 read_data;
	u16 i = 0;
	u16 val = field_mask;

	while ((!(val&0x0001))&&(i < 15)) {
		i++;
		val = (val >> 1);
	}

	rda5888_read_reg(addr, &read_data);
	read_data = (read_data & field_mask);

	read_data = (read_data >> i);

	return read_data;
}

static void rda5888_set_rxon(void)
{
	rda5888_field_write_reg(0x030, 0x0008, 0x0);
	mdelay(5);
	rda5888_field_write_reg(0x030, 0x0008, 0x1);
}

static void rda5888_sys_reset(void)
{
	rda5888_field_write_reg(0x130, 0x0800, 0x0);
	mdelay(5);
	rda5888_field_write_reg(0x130, 0x0800, 0x1);
}

static void rda5888_set_notch_filter(u16 mode, u32 v_freq)
{
	v_freq = v_freq/1000;

	RDA5888_LOGD("[rdamtv]%s, v_freq = %d\n", __func__, v_freq);

	switch (v_freq)	{
	case 49750: /* 49.75 MHZ */
		rda5888_write_reg(0x1A0, 0x1743);
		rda5888_write_reg(0x1A1, 0x07e3);
		rda5888_write_reg(0x1A2, 0xe);
		rda5888_write_reg(0x1A3, 0xBA12);
		rda5888_write_reg(0x1BC, 0x03F1);
		rda5888_write_reg(0x1BF, 0x07);
		rda5888_write_reg(0x1AC, 0x9800);
		break;
	case 55250: /* 55.25 MHZ */
		rda5888_write_reg(0x1A0, 0x10CC);
		rda5888_write_reg(0x1A1, 0x7DE);
		rda5888_write_reg(0x1A2, 0x10);
		break;
	case 57750: /* 57.75 MHZ */
		rda5888_write_reg(0x1A0, 0x15CD);
		rda5888_write_reg(0x1A1, 0x7DE);
		rda5888_write_reg(0x1A2, 0x10);
		break;
	case 61250: /* 61.25 MHZ */
		rda5888_write_reg(0x1A0, 0x1E29);
		rda5888_write_reg(0x1A1, 0x7DE);
		rda5888_write_reg(0x1A2, 0x10);
		break;
	case 62250: /* 62.25 MHZ */
		rda5888_write_reg(0x1A0, 0x1A90);
		rda5888_write_reg(0x1A1, 0x7E8);
		rda5888_write_reg(0x1A2, 0xB);
		break;
	case 65750: /* 65.75 MHZ */
		rda5888_write_reg(0x1A0, 0x116D);
		rda5888_write_reg(0x1A1, 0x7DE);
		rda5888_write_reg(0x1A2, 0x10);
		break;
	case 77250: /* 77.25 MHZ */
		rda5888_write_reg(0x1A0, 0x15CD);
		rda5888_write_reg(0x1A1, 0x7DE);
		rda5888_write_reg(0x1A2, 0x10);
		rda5888_write_reg(0x1A3, 0x8242);
		rda5888_write_reg(0x1BC, 0x03FB);
		rda5888_write_reg(0x1BF, 0x02);
		rda5888_write_reg(0x1AC, 0x9800);
		break;
	case 77750: /* 77.75 MHZ */
		rda5888_write_reg(0x1A0, 0x1745);
		rda5888_write_reg(0x1A1, 0x7e3);
		rda5888_write_reg(0x1A2, 0xe);
		break;
	case 83250: /* 83.25 MHZ */
		rda5888_write_reg(0x1A0, 0x124E);
		rda5888_write_reg(0x1A1, 0x7D0);
		rda5888_write_reg(0x1A2, 0x17);
		break;
	case 85250: /* 85.25 MHZ */
		rda5888_write_reg(0x1A0, 0x1744);
		rda5888_write_reg(0x1A1, 0x7e3);
		rda5888_write_reg(0x1A2, 0xe);
		break;
	case 176250: //176.25 MHZ */
		rda5888_write_reg(0x1A0, 0x1c57);
		rda5888_write_reg(0x1A1, 0x7de);
		rda5888_write_reg(0x1A2, 0x10);
		break;
	case 183250: /* 183.25 MHZ */
		rda5888_write_reg(0x1A0, 0x1c57);
		rda5888_write_reg(0x1A1, 0x7de);
		rda5888_write_reg(0x1A2, 0x10);
		break;
	case 184250: /* 184.25 MHZ */
		rda5888_write_reg(0x1A0, 0x18df);
		rda5888_write_reg(0x1A1, 0x7e3);
		rda5888_write_reg(0x1A2, 0xe);
		break;
	case 187250: /* 187.25 MHZ */
		rda5888_write_reg(0x1A0, 0x116D);
		rda5888_write_reg(0x1A1, 0x7DE);
		rda5888_write_reg(0x1A2, 0x10);
		break;
	case 192250: /* 192.25 MHZ */
		rda5888_write_reg(0x1A0, 0x1475);
		rda5888_write_reg(0x1A1, 0x7DE);
		rda5888_write_reg(0x1A2, 0x10);
		break;
	case 199250: /* 199.25 MHZ */
		rda5888_write_reg(0x1A0, 0x1470);
		rda5888_write_reg(0x1A1, 0x7e3);
		rda5888_write_reg(0x1A2, 0xe);
		break;
	case 200250: /* 200.25 MHZ */
		rda5888_write_reg(0x1A3, 0xc022);
		rda5888_write_reg(0x1BC, 0x03FB);
		rda5888_write_reg(0x1BF, 0x02);
		rda5888_write_reg(0x1AC, 0x9800);
		break;
	case 210250: /* 210.25 MHZ */
		rda5888_write_reg(0x1A0, 0x1C54);
		rda5888_write_reg(0x1A1, 0x7ED);
		rda5888_write_reg(0x1A2, 0x9);
		break;
	case 211250: /* 211.25 MHZ */
		rda5888_write_reg(0x1A0, 0x18E1);
		rda5888_write_reg(0x1A1, 0x7DE);
		rda5888_write_reg(0x1A2, 0x10);
		break;
	case 215250: /* 215.25 MHZ */
		rda5888_write_reg(0x1A0, 0x1055);
		rda5888_write_reg(0x1A1, 0x7e8);
		rda5888_write_reg(0x1A2, 0xb);
		break;
	case 216250: /* 216.25 MHZ */
		rda5888_write_reg(0x1A0, 0x1023);
		rda5888_write_reg(0x1A1, 0x7e4);
		rda5888_write_reg(0x1A2, 0xe);
		break;
	case 217250: /* 217.25 MHZ */
		rda5888_write_reg(0x1A0, 0x10C3);
		rda5888_write_reg(0x1A1, 0x7E8);
		rda5888_write_reg(0x1A2, 0xB);
		break;
	case 535250: /* 535.25 MHZ */
		rda5888_write_reg(0x1A0, 0x18df);
		rda5888_write_reg(0x1A1, 0x7e3);
		rda5888_write_reg(0x1A2, 0xe);
		break;
	case 631250: /* 631.25 MHZ */
		rda5888_write_reg(0x1A0, 0x1471);
		rda5888_write_reg(0x1A1, 0x7e3);
		rda5888_write_reg(0x1A2, 0xe);
		break;
	case 671250: /* 671.25 MHZ */
		rda5888_write_reg(0x1A0, 0x15c8);
		rda5888_write_reg(0x1A1, 0x7e3);
		rda5888_write_reg(0x1A2, 0xe);
		break;
	case 711250: /* 711.25 MHZ */
		rda5888_write_reg(0x1A0, 0x1744);
		rda5888_write_reg(0x1A1, 0x7e3);
		rda5888_write_reg(0x1A2, 0xe);
		break;
	case 751250: /* 751.25 MHZ */
		rda5888_write_reg(0x1A0, 0x18df);
		rda5888_write_reg(0x1A1, 0x7e3);
		rda5888_write_reg(0x1A2, 0xe);
		break;
	case 775250: /* 775.25 MHZ */
		rda5888_write_reg(0x1A0, 0x18da);
		rda5888_write_reg(0x1A1, 0x7ed);
		rda5888_write_reg(0x1A2, 0x9);
		break;
	case 777250: /* 777.25 MHZ */
		rda5888_write_reg(0x1A0, 0x1341);
		rda5888_write_reg(0x1A1, 0x7e3);
		rda5888_write_reg(0x1A2, 0xe);
		break;
	case 781250: /* 781.25 MHZ */
		rda5888_write_reg(0x1A0, 0x10be);
		rda5888_write_reg(0x1A1, 0x7ed);
		rda5888_write_reg(0x1A2, 0x9);
		break;
	case 927250: /* 927.25 MHZ */
		rda5888_write_reg(0x1A0, 0x1744);
		rda5888_write_reg(0x1A1, 0x7e3);
		rda5888_write_reg(0x1A2, 0xe);
		break;
	default:
		rda5888_write_reg(0x1AC, 0x9800);
		return; /* return before sys reset. */
	}

	rda5888_sys_reset();
}

static void rda5888_dsp_sys_reset(enum sys_mode mode)
{
	switch (mode) {
	case ATV_SYS_RESET_MODE0:
		rda5888_write_reg(0x130, 0x0010);
		mdelay(5);
		rda5888_write_reg(0x130, 0x0810);
		break;
	case ATV_SYS_RESET_MODE1:
		rda5888_write_reg(0x130, 0x0011);
		mdelay(5);
		rda5888_write_reg(0x130, 0x0811);
		break;
	case ATV_SYS_RESET_MODE2:
		rda5888_write_reg(0x130, 0x0012);
		mdelay(5);
		rda5888_write_reg(0x130, 0x0812);
		break;
	default:
		break;
	}
}

static void rda5888_calc_freq_offset(enum rdamtv_vstd vid_std)
{
	switch (vid_std) {
	case RDAMTV_VSTD_NTSC_M:
	case RDAMTV_VSTD_PAL_M:
	case RDAMTV_VSTD_PAL_N:
	case RDAMTV_VSTD_PAL_NC:
		rda5888_d->pri_data.freq_offset = 1725;
		break;
	case RDAMTV_VSTD_PAL_B:
	case RDAMTV_VSTD_PAL_B1:
	case RDAMTV_VSTD_PAL_G:
	case RDAMTV_VSTD_PAL_I:
	case RDAMTV_VSTD_SECAM_B:
	case RDAMTV_VSTD_SECAM_B1:
	case RDAMTV_VSTD_SECAM_G:
	case RDAMTV_VSTD_NTSC_B_G:
	case RDAMTV_VSTD_NTSC_I:
		rda5888_d->pri_data.freq_offset = 2125;
		break;
	case RDAMTV_VSTD_PAL_D:
	case RDAMTV_VSTD_PAL_D1:
	case RDAMTV_VSTD_PAL_K:
	case RDAMTV_VSTD_PAL_H:
	case RDAMTV_VSTD_SECAM_D:
	case RDAMTV_VSTD_SECAM_D1:
	case RDAMTV_VSTD_SECAM_K:
	case RDAMTV_VSTD_NTSC_D_K:
		rda5888_d->pri_data.freq_offset = 2625;
		break;
	case RDAMTV_VSTD_SECAM_L:
		break;
	default:
		break;
	}

    RDA5888_LOGD("[rdamtv]%s, vid_std = %d, freq_offset = %d\n",
				__func__, vid_std, rda5888_d->pri_data.freq_offset);
}

u32 rda5888_freq_set(u32 v_freq)
{
	u32 rt = 0;
	u16 rega, regb;
	u32 c_freq;

	/* convert to center carrier freq. */
	c_freq = v_freq/1000 + rda5888_d->pri_data.freq_offset;

	RDA5888_LOGD("[rdamtv]%s, c_freq = %d KHZ\n", __func__, c_freq);

	/* set center freq */
	c_freq = (c_freq << 10);
	regb = (u16)((c_freq&0xffff0000)>>16);
	rega = (u16)((c_freq&0x0000ffff));

	RDA5888_LOGD("[rdamtv]%s, 0x32 = %x, 0x33 = %x\n", __func__, regb, rega);
	rda5888_write_reg(0x033, (rega|0x0001));
	rda5888_write_reg(0x032, regb);

	return rt;
}

static u32 rda5888_vid_std_set(enum rdamtv_vstd vid_std)
{
	RDA5888_LOGD("[rdamtv]%s, mode = %d\n", __func__, vid_std);

	switch (vid_std) {
	case RDAMTV_VSTD_PAL_B:
	case RDAMTV_VSTD_PAL_B1:
	case RDAMTV_VSTD_PAL_G:
		rda5888_write_array(rda5888_vstd_pal_bg_datas,
				sizeof(rda5888_vstd_pal_bg_datas)/sizeof(struct rda_reg_data));
		rda5888_dsp_sys_reset(ATV_SYS_RESET_MODE0);
		rda5888_d->pri_data.is_625_lines = true;
		break;
	case RDAMTV_VSTD_PAL_D:
	case RDAMTV_VSTD_PAL_D1:
	case RDAMTV_VSTD_PAL_K:
		rda5888_write_array(rda5888_vstd_pal_dk_datas,
				sizeof(rda5888_vstd_pal_dk_datas)/sizeof(struct rda_reg_data));
		rda5888_dsp_sys_reset(ATV_SYS_RESET_MODE0);
		rda5888_d->pri_data.is_625_lines = true;
		break;
	case RDAMTV_VSTD_PAL_I:
		rda5888_write_array(rda5888_vstd_pal_i_datas,
				sizeof(rda5888_vstd_pal_i_datas)/sizeof(struct rda_reg_data));
		rda5888_dsp_sys_reset(ATV_SYS_RESET_MODE0);
		rda5888_d->pri_data.is_625_lines = true;
		break;
	case RDAMTV_VSTD_PAL_M:
		rda5888_write_array(rda5888_vstd_pal_m_datas,
				sizeof(rda5888_vstd_pal_m_datas)/sizeof(struct rda_reg_data));
		rda5888_dsp_sys_reset(ATV_SYS_RESET_MODE0);
		rda5888_d->pri_data.is_625_lines = false;
		break;
	case RDAMTV_VSTD_PAL_H:
		break;
	case RDAMTV_VSTD_PAL_N:
	case RDAMTV_VSTD_PAL_NC:
		rda5888_write_array(rda5888_vstd_pal_n_datas,
				sizeof(rda5888_vstd_pal_n_datas)/sizeof(struct rda_reg_data));
		rda5888_dsp_sys_reset(ATV_SYS_RESET_MODE0);
		rda5888_d->pri_data.is_625_lines = true;
		break;
	case RDAMTV_VSTD_SECAM_B:
	case RDAMTV_VSTD_SECAM_B1:
	case RDAMTV_VSTD_SECAM_G:
		rda5888_write_array(rda5888_vstd_secam_bg_datas,
				sizeof(rda5888_vstd_secam_bg_datas)/sizeof(struct rda_reg_data));
		rda5888_dsp_sys_reset(ATV_SYS_RESET_MODE2);
		rda5888_d->pri_data.is_625_lines = true;
		break;
	case RDAMTV_VSTD_SECAM_D:
	case RDAMTV_VSTD_SECAM_D1:
	case RDAMTV_VSTD_SECAM_K:
		rda5888_write_array(rda5888_vstd_secam_dk_datas,
				sizeof(rda5888_vstd_secam_dk_datas)/sizeof(struct rda_reg_data));
		rda5888_dsp_sys_reset(ATV_SYS_RESET_MODE2);
		rda5888_d->pri_data.is_625_lines = true;
		break;
	case RDAMTV_VSTD_SECAM_L:
		break;
	case RDAMTV_VSTD_NTSC_B_G:
		rda5888_dsp_sys_reset(ATV_SYS_RESET_MODE1);
		rda5888_d->pri_data.is_625_lines = false;
		break;
	case RDAMTV_VSTD_NTSC_D_K:
		rda5888_dsp_sys_reset(ATV_SYS_RESET_MODE1);
		rda5888_d->pri_data.is_625_lines = false;
	    break;
	case RDAMTV_VSTD_NTSC_I:
		rda5888_dsp_sys_reset(ATV_SYS_RESET_MODE1);
		rda5888_d->pri_data.is_625_lines = false;
		break;
	case RDAMTV_VSTD_NTSC_M:
		rda5888_write_array(rda5888_vstd_ntsc_m_datas,
			sizeof(rda5888_vstd_ntsc_m_datas)/sizeof(struct rda_reg_data));
		rda5888_dsp_sys_reset(ATV_SYS_RESET_MODE1);
		rda5888_d->pri_data.is_625_lines = false;
		break;
	default:
		break;
	}

	return (RDAMTV_RT_SUCCESS);
}

static void rda5888_cust_for_rda8810(void)
{
	rda5888_write_reg(0x120,0x423a);

	if (rda5888_d->pri_data.is_625_lines) {
	    rda5888_write_reg(0x15e,0x0018);
	    rda5888_write_reg(0x15f,0x0147);
	    rda5888_write_reg(0x15a,0x0100);
		rda5888_field_write_reg(0x129, 0x0010, 1);
	} else {
		rda5888_field_write_reg(0x129, 0x0010, 0);
	}

	rda5888_write_reg(0x134, 0x02c6);
	rda5888_write_reg(0x135, 0x03cc); /* 9'h135,16'h03cc; h_blank_end, cell without shell */
	rda5888_write_reg(0x1d1, 0x0311); /* 9'h1d1,16'h0311; for scaler */
	rda5888_write_reg(0x1d2, 0x05a0); /* 9'h1d2,16'h05a0; for frame buf */
}

static int rda5888_scan_ch(u32 v_freq, enum rdamtv_vstd vid_std)
{
	u16 var, reg;
	u32 m1, m2, m3, rt;
	u32 k1 = 1;
	u32 k2 = 2;

#if 0 /* set fixed freq, for test. */
	v_freq = 711250000;
	vid_std = RDAMTV_VSTD_PAL_D;
#endif

	rda5888_calc_freq_offset(vid_std);
	rda5888_freq_set(v_freq);

	rda5888_set_rxon();
	mdelay(20);

	rda5888_vid_std_set(vid_std);
	rda5888_set_notch_filter(vid_std, v_freq);
	rda5888_cust_for_rda8810();
	mdelay(20);

	/* vsb_dem reset */
	rda5888_write_reg(0x103, 0x020d);
	rda5888_write_reg(0x103, 0x000d);
	mdelay(60);

	/* read noise */
	m1 = rda5888_field_read_reg(0x086, 0x1fc0);

	rda5888_write_reg(0x12f, 0x0c35);
	rda5888_read_reg(0x117, &reg);
	m2 = (reg & 0x00fe) >> 1;
	m3 = (reg & 0x7f00) >> 8;

	var = k1*m1 + k2*(m3-m2);

	if (var < RDA5888_SCAN_THRESHOLD) {
	    rt = RDAMTV_RT_SCAN_DONE;
		RDA5888_LOGD("[rdamtv]%s Done, var = %d\n", __func__, var);
	}
	else {
	    rt = RDAMTV_RT_SCAN_FAIL;
	}

	//rda5888_sys_reset();

	RDA5888_LOGD("[rdamtv]%s End, seek_done = %d\n", __func__, rt);

	return rt;
}

static long rda5888_ioctl_read(unsigned long arg)
{
	void *user_addr;
	long err = 0;
	struct rda_reg_data atv_reg = {0,0};

	do {
		user_addr = (void __user*)arg;
		if (user_addr == NULL) {
			err = -EINVAL;
			break;
		}
		if (copy_from_user(&atv_reg, user_addr, sizeof(struct rda_reg_data))) {
			err = -EFAULT;
			break;
		}
		rda5888_read_reg(atv_reg.reg_num, &atv_reg.value);
		if (copy_to_user(user_addr, &atv_reg, sizeof(struct rda_reg_data))) {
			err = -EFAULT;
			break;
		}
	} while (0);

	RDA5888_LOGD("[rdamtv]%s, reg = 0x%x, data = 0x%x\n",
				__func__, atv_reg.reg_num, atv_reg.value);

	return err;
}

static long rda5888_ioctl_write(unsigned long arg)
{
	void *user_addr;
	long err = 0;
	struct rda_reg_data atv_reg = {0,0};

	do {
		user_addr = (void __user*)arg;
		if (user_addr == NULL) {
			err = -EINVAL;
			break;
		}
		if (copy_from_user(&atv_reg, user_addr, sizeof(struct rda_reg_data))) {
			err = -EFAULT;
			break;
		}
		rda5888_write_reg(atv_reg.reg_num, atv_reg.value);
	} while (0);

	RDA5888_LOGD("[rdamtv]%s, reg = 0x%x, data = 0x%x\n",
				__func__, atv_reg.reg_num, atv_reg.value);

	return err;
}

static int rda5888_ioctl_poweron(unsigned long arg)
{
	u16 chipid = 0;

	RDA5888_LOGD("[rdamtv]%s, %d.\n", __func__, rda5888_d->poweron);

	enable_26m_rtc_atv();
	msleep(8);

	/* read chipid. */
	rda5888_read_reg(0x0, &chipid);
	RDA5888_LOGD("[rdamtv]%s, chipid: 0x%x\n", __func__, chipid);

	if (!rda5888_d->poweron) {
		//rda5888_cust_power_off();
		//rda5888_cust_power_on();

		rda5888_rf_init();
		mdelay(300);
		rda5888_dsp_init();
		rda5888_26m_crystal_init();
		rda5888_d->poweron = true;
	}

	return 0;
}

static int rda5888_ioctl_poweroff(unsigned long arg)
{
	RDA5888_LOGD("[rdamtv]%s.\n", __func__);

	rda5888_write_reg(0x002, 0x10);
	rda5888_write_reg(0x002, 0x0); /* power donw all */

	disable_26m_rtc_atv();

	//rda5888_cust_power_off();

	rda5888_d->poweron = false;

	return 0;
}

static long rda5888_ioctl_scan(unsigned long arg)
{
	void *user_addr;
	long err = 0;
	struct rdamtv_scan_data scan_data = {0,};

	do {
		user_addr = (void __user*)arg;
		if (user_addr == NULL) {
			err = -EINVAL;
			break;
		}
		if (copy_from_user(&scan_data, user_addr, sizeof(struct rdamtv_scan_data))) {
			err = -EFAULT;
			break;
		}
		scan_data.ret = rda5888_scan_ch(scan_data.vfreq, scan_data.vid_std);
		if (copy_to_user(user_addr, &scan_data, sizeof(struct rdamtv_scan_data))) {
			err = -EFAULT;
			break;
		}
	} while (0);

	RDA5888_LOGD("[rdamtv]%s, vfreq = %d, vstd = %d, ret = %d\n",
				__func__, scan_data.vfreq, scan_data.vid_std, scan_data.ret);

	return err;
}

#ifdef RDA5888_TEST_SET
extern void RDA5888SensorSetCrop(int x, int y, int width, int height);
static long rda5888_ioctl_set_crop(unsigned long arg)
{
	void *user_addr;
	long err = 0;
	struct rda_set_crop_data crop_d = {0,0};

	do {
		user_addr = (void __user*)arg;
		if (user_addr == NULL) {
			err = -EINVAL;
			break;
		}
		if (copy_from_user(&crop_d, user_addr, sizeof(struct rda_set_crop_data))) {
			err = -EFAULT;
			break;
		}
		//RDA5888SensorSetCrop(crop_d.x, crop_d.y, crop_d.width, crop_d.height);
	} while (0);

	RDA5888_LOGD("[rdamtv]rda5888_ioctl_set_crop, (%d, %d, %d, %d)\n",
				crop_d.x, crop_d.y, crop_d.width, crop_d.height);

	return err;
}

unsigned int rda5888_isp_read_reg(unsigned int reg);
unsigned int rda5888_isp_write_reg(unsigned int reg, unsigned int val);

static long rda5888_ioctl_isp_read(unsigned long arg)
{
	void *user_addr;
	long err = 0;
	struct isp_reg_data isp_reg = {0,0};

	do {
		user_addr = (void __user*)arg;
		if (user_addr == NULL) {
			err = -EINVAL;
			break;
		}
		if (copy_from_user(&isp_reg, user_addr, sizeof(struct isp_reg_data))) {
			err = -EFAULT;
			break;
		}
		//isp_reg.value = rda5888_isp_read_reg(isp_reg.reg_num);
		if (copy_to_user(user_addr, &isp_reg, sizeof(struct isp_reg_data))) {
			err = -EFAULT;
			break;
		}
	} while (0);

	RDA5888_LOGD("[rdamtv]%s, reg = 0x%x, data = 0x%x\n",
				__func__, isp_reg.reg_num, isp_reg.value);

	return err;
}

static long rda5888_ioctl_isp_write(unsigned long arg)
{
	void *user_addr;
	long err = 0;
	struct isp_reg_data isp_reg = {0,0};

	do {
		user_addr = (void __user*)arg;
		if (user_addr == NULL) {
			err = -EINVAL;
			break;
		}
		if (copy_from_user(&isp_reg, user_addr, sizeof(struct isp_reg_data))) {
			err = -EFAULT;
			break;
		}
		//rda5888_isp_write_reg(isp_reg.reg_num, isp_reg.value);
	} while (0);

	RDA5888_LOGD("[rdamtv]%s, reg = 0x%x, data = 0x%x\n",
				__func__, isp_reg.reg_num, isp_reg.value);

	return err;
}

#endif

static long rda5888_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	long err = 0;

	RDA5888_LOGD("[rdamtv]%s, cmd = %d\n", __func__, cmd);

	switch (cmd) {
	case RDA5888_READ:
		err = rda5888_ioctl_read(arg);
        break;
	case RDA5888_WRITE:
		err = rda5888_ioctl_write(arg);
        break;
	case RDA5888_POWER_ON:
		err = rda5888_ioctl_poweron(arg);
		break;
	case RDA5888_POWER_OFF:
		err = rda5888_ioctl_poweroff(arg);
		break;
	case RDA5888_SCAN_CH:
		err = rda5888_ioctl_scan(arg);
		break;
#ifdef RDA5888_TEST_SET
	case RDA5888_SET_CROP:
		err = rda5888_ioctl_set_crop(arg);
		break;
	case RDA5888_ISP_READ:
		err = rda5888_ioctl_isp_read(arg);
        break;
	case RDA5888_ISP_WRITE:
		err = rda5888_ioctl_isp_write(arg);
        break;
#endif
	default:
	    break;
    }

    return 0;
}

static int rda5888_open(struct inode *inode, struct file *file)
{
	RDA5888_LOGD("[rdamtv]%s.\n", __func__);

	/*if (!rda5888_d->opened) {
		rda5888_cust_power_off();
		rda5888_cust_power_on();

		rda5888_rf_init();
		mdelay(200);
		rda5888_dsp_init();
		rda5888_26m_crystal_init();
		rda5888_d->opened = true;
	}*/

	return 0;
}

static int rda5888_release(struct inode *inode, struct file *file)
{
	RDA5888_LOGD("[rdamtv]%s.\n", __func__);

	//rda5888_d->opened = false;

	return 0;
}

static struct file_operations rda5888_fops = {
	.owner		= THIS_MODULE,
	.unlocked_ioctl		= rda5888_ioctl,
	.open		= rda5888_open,
	.release	= rda5888_release,
};

static int atv_audiocontrol_get_volume(struct audiocontrol_dev* acdev)
{
	struct rda5888_dev *atv_d =
		container_of(acdev, struct rda5888_dev, acdev);

	if(!atv_d)
		return -1;

	return acdev->volume;
}
static int atv_audiocontrol_set_volume(struct audiocontrol_dev* acdev, int volume)
{
	int i = 0, volume_reg = -1;
	struct rda5888_dev *atv_d =
		container_of(acdev, struct rda5888_dev, acdev);

	if(!atv_d)
		return -1;

	for(i = 0; i < sizeof(atvvol)/sizeof(struct atvvolume); ++i) {
		if(volume == atvvol[i].vol_db) {
			volume_reg  = atvvol[i].volume;
			printk(KERN_INFO"atv set volume : found volume_reg %d\n", volume_reg);
			break;
		}
	}

	if(volume_reg == -1) {
		printk(KERN_INFO"atv set volume fail: %d\n", volume);
		return -EFAULT;
	}

	rda5888_field_write_reg(0x0a8, 0x01e0, volume_reg);

	acdev->volume = volume;

	return 0;
}

static int atv_audiocontrol_get_mute(struct audiocontrol_dev* acdev)
{
	struct rda5888_dev *atv_d =
		container_of(acdev, struct rda5888_dev, acdev);

	if(!atv_d) {
		printk(KERN_INFO"%s : atv_d is NULL.", __func__);
		return -1;
	}

	return acdev->mute;
}
static int atv_audiocontrol_set_mute(struct audiocontrol_dev* acdev, int mute)
{
	struct rda5888_dev *atv_d =
		container_of(acdev, struct rda5888_dev, acdev);


	if(!atv_d) {
		printk(KERN_INFO"%s : atv_d is NULL.", __func__);
		return -1;
	}

	printk(KERN_INFO"%s : mute - %d", __func__, mute);

	if(mute) {
		rda5888_field_write_reg(0x082, 0x4000, 1);
		acdev->mute = 1;
	}
	else {
		rda5888_field_write_reg(0x082, 0x4000, 0);
		acdev->mute = 0;
	}

	return 0;
}

static int rda5888_i2c_probe(struct i2c_client *client,
	const struct i2c_device_id *id)
{
	struct rda5888_dev *atv_d;
	struct class_device *class_dev = NULL;
	int ret = 0;

	RDA5888_LOGD("[rdamtv]%s, device_id:%s.\n ", __func__, id->name);
	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		RDA5888_LOGE("[rdamtv]client is not i2c capable.\n");
		ret = -ENODEV;
		goto i2c_functioality_failed;
	}

	atv_d = kzalloc(sizeof(struct rda5888_dev), GFP_KERNEL);
	if (atv_d == NULL) {
		ret = -ENOMEM;
		goto err_atv_alloc_failed;
	}
	i2c_set_clientdata(client, atv_d);

	rda5888_d = atv_d;

	ret = alloc_chrdev_region(&atv_d->devno, 0, 1, RDA5888_DEVNAME);
	if (ret)
		RDA5888_LOGE("[rdamtv]alloc_chrdev_region error.\n");

	atv_d->device = cdev_alloc();
	atv_d->device->owner = THIS_MODULE;
	atv_d->device->ops = &rda5888_fops;
	ret = cdev_add(atv_d->device, atv_d->devno, 1);
	if(ret)
	    RDA5888_LOGE("[rdamtv]cdev_add error.\n");

	atv_d->major = MAJOR(atv_d->devno);
	atv_d->cls = class_create(THIS_MODULE, RDA5888_DEVNAME);
	class_dev = (struct class_device *)device_create(atv_d->cls,
			NULL,
			atv_d->devno,
			NULL,
			RDA5888_DEVNAME);

	atv_d->clk26m = clk_get(NULL, RDA_CLK_AUX);

	atv_d->client = client;
	atv_d->client->addr = atv_d->client->addr; // & I2C_MASK_FLAG;
	//atv_d->client->timing = 50;
	atv_d->i2c_probed = true;

	i2c_set_clientdata(atv_d->client, atv_d);

	// register audiocontrol
	atv_d->acdev.name = "atvaudio";
	atv_d->acdev.mute = 0;
	atv_d->acdev.volume = 0;
	atv_d->acdev.get_volume = atv_audiocontrol_get_volume;
	atv_d->acdev.set_volume = atv_audiocontrol_set_volume;
	atv_d->acdev.get_mute = atv_audiocontrol_get_mute;
	atv_d->acdev.set_mute = atv_audiocontrol_set_mute;
	audiocontrol_dev_register(&atv_d->acdev);

	RDA5888_LOGD("[rdamtv]%s, done.\n", __func__);

	return 0;

err_atv_alloc_failed:
i2c_functioality_failed:
	RDA5888_LOGD("[rdamtv]%s, fail.\n", __func__);
	return ret;
}

static int rda5888_i2c_remove(struct i2c_client *client)
{
	struct rda5888_dev *atv_d = i2c_get_clientdata(client);

	if (atv_d->cls)
		class_destroy(atv_d->cls);

	RDA5888_LOGD("[rdamtv]%s\n", __func__);
	clk_put(atv_d->clk26m);

	kfree(atv_d);
	rda5888_d = NULL;

	return 0;
}

static const struct i2c_device_id rda5888_i2c_id[] =
{
	{RDA5888_I2C_DEVNAME, 0},
	{}
};

struct i2c_driver rda5888_i2c_driver = {
	.probe = rda5888_i2c_probe,
	.remove = rda5888_i2c_remove,
	.driver = {
		.name = RDA5888_I2C_DEVNAME,
	},
	.id_table = rda5888_i2c_id,
};

int rda5888_test(void)
{
	unsigned short chipid = 0;

	if (!rda5888_d->i2c_probed)
		return -EBUSY;

	RDA5888_LOGD("[rdamtv]rda5888_test 111, begin.\n");

	/* read chipid. */
	rda5888_read_reg(0x0, &chipid);
	RDA5888_LOGD("[rdamtv]rda5888_test, chipid: 0x%x\n", chipid);

	rda5888_rf_init();
	mdelay(200);
	rda5888_dsp_init();
	rda5888_26m_crystal_init();

	//rda5888_set_rxon();
	rda5888_scan_ch(527250000, RDAMTV_VSTD_PAL_D);

	RDA5888_LOGD("[rdamtv]rda5888_test, end.\n");

	return 0;
}
EXPORT_SYMBOL(rda5888_test);

bool rda5888_is_opend(void)
{
	return rda5888_d->poweron;
}
EXPORT_SYMBOL(rda5888_is_opend);

bool rda5888_is_625lines(void)
{
	if (rda5888_d != NULL) {
		return rda5888_d->pri_data.is_625_lines;
	} else {
		return true;
	}
}
EXPORT_SYMBOL(rda5888_is_625lines);

/*static struct i2c_board_info __initdata i2c_rda5888_info = {
	I2C_BOARD_INFO(RDA5888_I2C_DEVNAME, RDA5888_I2C_ADDR)
};*/

static int __init rda5888_init(void)
{
	int ret;

	RDA5888_LOGD("[rdamtv]rda5888_init.\n");

	//i2c_register_board_info(RDA5888_I2C_BUS_NUM, &i2c_rda5888_info, 1);
	if (i2c_add_driver(&rda5888_i2c_driver)) {
		RDA5888_LOGE("[rdamtv]fail to add device into i2c\n");
		ret = -ENODEV;
		return ret;
	}

	return 0;
}

static void __exit rda5888_exit(void)
{
	RDA5888_LOGD("[rdamtv]rda5888_exit.\n");

	i2c_del_driver(&rda5888_i2c_driver);
}

module_init(rda5888_init);
module_exit(rda5888_exit);

MODULE_AUTHOR("Huaping Wu <huapingwu@rdamicro.com>");
MODULE_DESCRIPTION("The RDA5888 driver for Linux");
MODULE_LICENSE("GPL");

#endif
