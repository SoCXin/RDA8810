/*
 * rda_calib.h For Calib Driver
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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * Copyright (C) RDA Corporation, 2014
 *
 */

#ifndef __RDA_CALIB_H__
#define __RDA_CALIB_H__

#include <linux/ioctl.h>

#define CALIB_AUDIO_IIR_GAIN_NUM 10
// params
struct get_iir_param_param
{
	unsigned char itf;

	// like CALIB_AUDIO_IIR_PARAM_ITF_T
	signed char               gain[CALIB_AUDIO_IIR_GAIN_NUM];
	unsigned char             qual[CALIB_AUDIO_IIR_GAIN_NUM];
	unsigned short            freq[CALIB_AUDIO_IIR_GAIN_NUM];
};
struct set_iir_param_param
{
	unsigned char itf;

	// like CALIB_AUDIO_IIR_PARAM_ITF_T
	signed char               gain[CALIB_AUDIO_IIR_GAIN_NUM];
	unsigned char             qual[CALIB_AUDIO_IIR_GAIN_NUM];
	unsigned short            freq[CALIB_AUDIO_IIR_GAIN_NUM];
};

//Add for Audio Calibration -----start-----
//eq params
struct eq_band_param
{
	short num[3];
	short den[3];
	short gain;
	short type;
	short frep;
	union {
		short qual;
		short slop;
		short bw;
	} param;
	short reserved[2];
};

struct eq_param
{
	short eq_en;
	struct eq_band_param band[7];
};

struct eq_data
{
	unsigned char itf;
	struct eq_param eq_param;
};

// drc params
struct drc_calib_param
{
	short drc_enable;
//actually for drc
	short thres;
	short width;
	short R;
	short R1;
	short R2;
	short limit;
	short M;
	short alpha1;
	short alpha2;
	short noise_gate;
	short alpha_max;
	short Thr_dB;
	short mm_gain;
	short channel;
	short reserved;
};

struct drc_data
{
	unsigned char itf;
	struct drc_calib_param drc_calib_param;
};
//Add for Audio Calibration -----end -----

// params
struct get_mic_gains_param
{
	unsigned char itf;

	// like CALIB_AUDIO_IN_GAINS_T
	char ana;
	char adc;
	char alg;
	char reserv;
};
struct set_mic_gains_param
{
	unsigned char itf;

	// like CALIB_AUDIO_IN_GAINS_T
	char ana;
	char adc;
	char alg;
	char reserv;
};
struct get_all_gains_param
{
	unsigned char itf;

	// like CALIB_AUDIO_GAINS_T
};
struct set_all_gains_param
{
	unsigned char itf;

	// like CALIB_AUDIO_GAINS_T
};

enum calib_type
{
	EQ = 0,
	DRC = 1
};

// cmds
#define CALIBDATA_IOC_MAGIC        0xc4 // FIXME: any conflict?

#define CALIBDATA_IOCTL_GET_MIC_GAINS   _IOWR(CALIBDATA_IOC_MAGIC, 0, struct get_mic_gains_param*)
#define CALIBDATA_IOCTL_SET_MIC_GAINS   _IOWR(CALIBDATA_IOC_MAGIC, 1, struct set_mic_gains_param*)

#define CALIBDATA_IOCTL_GET_ALL_GAINS   _IOWR(CALIBDATA_IOC_MAGIC, 2, struct get_all_gains_param*)
#define CALIBDATA_IOCTL_SET_SET_GAINS   _IOWR(CALIBDATA_IOC_MAGIC, 3, struct set_mic_gains_param*)

#define CALIBDATA_IOCTL_GET_IIR_PARAM   _IOWR(CALIBDATA_IOC_MAGIC, 4, struct get_iir_param_param*)
#define CALIBDATA_IOCTL_SET_IIR_PARAM   _IOWR(CALIBDATA_IOC_MAGIC, 5, struct set_iir_param_param*)

/*Add for Audio Calibration -----start-----*/
#define CALIBDATA_IOCTL_GET_EQ_PARAM   _IOWR(CALIBDATA_IOC_MAGIC, 6, struct eq_data*)
#define CALIBDATA_IOCTL_GET_DRC_PARAM   _IOWR(CALIBDATA_IOC_MAGIC, 7, struct drc_data*)
#define CALIBDATA_IOCTL_GET_INGAINS    _IOWR(CALIBDATA_IOC_MAGIC, 8, struct get_mic_gains_param*)
/*Add for Audio Calibration -----end -----*/
#endif
