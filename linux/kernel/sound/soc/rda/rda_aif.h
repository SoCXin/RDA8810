/*
 * aif.h- RDA audio interface
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
#ifndef _RDA_AIF_H_
#define _RDA_AIF_H_

#ifdef CT_ASM
#error "You are trying to use in an assembly code the normal H description of 'aif'."
#endif

// =============================================================================
//  MACROS
// =============================================================================

// ============================================================================
// AIF_SAMPLING_RATE_T
// -----------------------------------------------------------------------------
/// 
// =============================================================================
typedef enum {
	AIF_8K = 0x00000000,
	AIF_11K025 = 0x00000001,
	AIF_12K = 0x00000002,
	AIF_16K = 0x00000003,
	AIF_22K05 = 0x00000004,
	AIF_24K = 0x00000005,
	AIF_32K = 0x00000006,
	AIF_44K1 = 0x00000007,
	AIF_48K = 0x00000008
} AIF_SAMPLING_RATE_T;

#define AIF_RX_FIFO_SIZE                         (4)
#define AIF_TX_FIFO_SIZE                         (4)

// =============================================================================
//  TYPES
// =============================================================================

// ============================================================================
// AIF_T
// -----------------------------------------------------------------------------
/// 
// =============================================================================
#define REG_AP_AIF_BASE             0x009E0000

typedef volatile struct {
	REG32 data;		//0x00000000
	REG32 ctrl;		//0x00000004
	REG32 serial_ctrl;	//0x00000008
	REG32 tone;		//0x0000000C
	REG32 side_tone;	//0x00000010
	REG32 Cfg_Clk_AudioBCK;	//0x00000014
	REG32 Cfg_Aif_Tx_Stb;	//0x00000018
} HWP_AIF_T;

//#define hwp_apAif                   ((HWP_AIF_T*) KSEG1(REG_AP_AIF_BASE))

//data
#define AIF_DATA0(n)                (((n)&0xFFFF)<<0)
#define AIF_DATA1(n)                (((n)&0xFFFF)<<16)

//ctrl
#define AIF_ENABLE                  (1<<0)
#define AIF_ENABLE_ENABLE           (1<<0)
#define AIF_ENABLE_DISABLE          (0<<0)
#define AIF_TX_OFF                  (1<<4)
#define AIF_TX_OFF_TX_ON            (0<<4)
#define AIF_TX_OFF_TX_OFF           (1<<4)
#define AIF_PARALLEL_OUT_SET        (1<<8)
#define AIF_PARALLEL_OUT_SET_SERL   (0<<8)
#define AIF_PARALLEL_OUT_SET_PARA   (1<<8)
#define AIF_PARALLEL_OUT_CLR        (1<<9)
#define AIF_PARALLEL_OUT_CLR_SERL   (0<<9)
#define AIF_PARALLEL_OUT_CLR_PARA   (1<<9)
#define AIF_PARALLEL_IN_SET         (1<<10)
#define AIF_PARALLEL_IN_SET_SERL    (0<<10)
#define AIF_PARALLEL_IN_SET_PARA    (1<<10)
#define AIF_PARALLEL_IN_CLR         (1<<11)
#define AIF_PARALLEL_IN_CLR_SERL    (0<<11)
#define AIF_PARALLEL_IN_CLR_PARA    (1<<11)
#define AIF_TX_STB_MODE             (1<<12)
#define AIF_PARALLEL_IN2_EN         (1<<13)
#define AIF_PARALLEL_IN2_EN_DISABLE (0<<13)
#define AIF_PARALLEL_IN2_EN_ENABLE  (1<<13)
#define AIF_OUT_UNDERFLOW           (1<<16)
#define AIF_IN_OVERFLOW             (1<<17)
#define AIF_LOOP_BACK               (1<<31)
#define AIF_LOOP_BACK_NORMAL        (0<<31)
#define AIF_LOOP_BACK_LOOPBACK      (1<<31)

//serial_ctrl
#define AIF_SERIAL_MODE(n)          (((n)&3)<<0)
#define AIF_SERIAL_MODE_I2S_PCM     (0<<0)
#define AIF_SERIAL_MODE_VOICE       (1<<0)
#define AIF_SERIAL_MODE_DAI         (2<<0)
#define AIF_I2S_IN_SEL(n)           (((n)&3)<<2)
#define AIF_I2S_IN_SEL_I2S_IN_0     (0<<2)
#define AIF_I2S_IN_SEL_I2S_IN_1     (1<<2)
#define AIF_I2S_IN_SEL_I2S_IN_2     (2<<2)
#define AIF_MASTER_MODE             (1<<4)
#define AIF_MASTER_MODE_SLAVE       (0<<4)
#define AIF_MASTER_MODE_MASTER      (1<<4)
#define AIF_LSB                     (1<<5)
#define AIF_LSB_MSB                 (0<<5)
#define AIF_LSB_LSB                 (1<<5)
#define AIF_LRCK_POL                (1<<6)
#define AIF_LRCK_POL_LEFT_H_RIGHT_L (0<<6)
#define AIF_LRCK_POL_LEFT_L_RIGHT_H (1<<6)
#define AIF_RX_DLY(n)               (((n)&3)<<8)
#define AIF_RX_DLY_ALIGN            (0<<8)
#define AIF_RX_DLY_DLY_1            (1<<8)
#define AIF_RX_DLY_DLY_2            (2<<8)
#define AIF_RX_DLY_DLY_3            (3<<8)
#define AIF_TX_DLY                  (1<<10)
#define AIF_TX_DLY_ALIGN            (0<<10)
#define AIF_TX_DLY_DLY_1            (1<<10)
#define AIF_TX_DLY_S                (1<<11)
#define AIF_TX_DLY_S_NO_DLY         (0<<11)
#define AIF_TX_DLY_S_DLY            (1<<11)
#define AIF_TX_MODE(n)              (((n)&3)<<12)
#define AIF_TX_MODE_STEREO_STEREO   (0<<12)
#define AIF_TX_MODE_MONO_STEREO_CHAN_L (1<<12)
#define AIF_TX_MODE_MONO_STEREO_DUPLI (2<<12)
#define AIF_TX_MODE_STEREO_TO_MONO  (3<<12)
#define AIF_RX_MODE                 (1<<14)
#define AIF_RX_MODE_STEREO_STEREO   (0<<14)
#define AIF_RX_MODE_STEREO_MONO_FROM_L (1<<14)
#define AIF_BCK_LRCK(n)             (((n)&31)<<16)
#define AIF_BCK_LRCK_BCK_LRCK_16    (0<<16)
#define AIF_BCK_LRCK_BCK_LRCK_17    (1<<16)
#define AIF_BCK_LRCK_BCK_LRCK_18    (2<<16)
#define AIF_BCK_LRCK_BCK_LRCK_19    (3<<16)
#define AIF_BCK_LRCK_BCK_LRCK_20    (4<<16)
#define AIF_BCK_LRCK_BCK_LRCK_21    (5<<16)
#define AIF_BCK_LRCK_BCK_LRCK_22    (6<<16)
#define AIF_BCK_LRCK_BCK_LRCK_23    (7<<16)
#define AIF_BCK_LRCK_BCK_LRCK_24    (8<<16)
#define AIF_BCK_LRCK_BCK_LRCK_25    (9<<16)
#define AIF_BCK_LRCK_BCK_LRCK_26    (10<<16)
#define AIF_BCK_LRCK_BCK_LRCK_27    (11<<16)
#define AIF_BCK_LRCK_BCK_LRCK_28    (12<<16)
#define AIF_BCK_LRCK_BCK_LRCK_29    (13<<16)
#define AIF_BCK_LRCK_BCK_LRCK_30    (14<<16)
#define AIF_BCK_LRCK_BCK_LRCK_31    (15<<16)
#define AIF_BCK_LRCK_BCK_LRCK_32    (16<<16)
#define AIF_BCK_LRCK_BCK_LRCK_33    (17<<16)
#define AIF_BCK_LRCK_BCK_LRCK_34    (18<<16)
#define AIF_BCK_LRCK_BCK_LRCK_35    (19<<16)
#define AIF_BCK_LRCK_BCK_LRCK_36    (20<<16)
#define AIF_BCK_LRCK_BCK_LRCK_37    (21<<16)
#define AIF_BCK_LRCK_BCK_LRCK_38    (22<<16)
#define AIF_BCK_LRCK_BCK_LRCK_39    (23<<16)
#define AIF_BCK_LRCK_BCK_LRCK_40    (24<<16)
#define AIF_BCK_LRCK_BCK_LRCK_41    (25<<16)
#define AIF_BCK_LRCK_BCK_LRCK_42    (26<<16)
#define AIF_BCK_LRCK_BCK_LRCK_43    (27<<16)
#define AIF_BCK_LRCK_BCK_LRCK_44    (28<<16)
#define AIF_BCK_LRCK_BCK_LRCK_45    (29<<16)
#define AIF_BCK_LRCK_BCK_LRCK_46    (30<<16)
#define AIF_BCK_LRCK_BCK_LRCK_47    (31<<16)
#define AIF_OUTPUT_HALF_CYCLE_DLY   (1<<25)
#define AIF_OUTPUT_HALF_CYCLE_DLY_NO_DLY (0<<25)
#define AIF_OUTPUT_HALF_CYCLE_DLY_DLY (1<<25)
#define AIF_INPUT_HALF_CYCLE_DLY    (1<<26)
#define AIF_INPUT_HALF_CYCLE_DLY_NO_DLY (0<<26)
#define AIF_INPUT_HALF_CYCLE_DLY_DLY (1<<26)
#define AIF_BCKOUT_GATE             (1<<28)
#define AIF_BCKOUT_GATE_NO_GATE     (0<<28)
#define AIF_BCKOUT_GATE_GATED       (1<<28)

//tone
#define AIF_ENABLE_H                (1<<0)
#define AIF_ENABLE_H_DISABLE        (0<<0)
#define AIF_ENABLE_H_ENABLE         (1<<0)
#define AIF_TONE_SELECT             (1<<1)
#define AIF_TONE_SELECT_DTMF        (0<<1)
#define AIF_TONE_SELECT_COMFORT_TONE (1<<1)
#define AIF_DTMF_FREQ_COL(n)        (((n)&3)<<4)
#define AIF_DTMF_FREQ_COL_1209_HZ   (0<<4)
#define AIF_DTMF_FREQ_COL_1336_HZ   (1<<4)
#define AIF_DTMF_FREQ_COL_1477_HZ   (2<<4)
#define AIF_DTMF_FREQ_COL_1633_HZ   (3<<4)
#define AIF_DTMF_FREQ_ROW(n)        (((n)&3)<<6)
#define AIF_DTMF_FREQ_ROW_697_HZ    (0<<6)
#define AIF_DTMF_FREQ_ROW_770_HZ    (1<<6)
#define AIF_DTMF_FREQ_ROW_852_HZ    (2<<6)
#define AIF_DTMF_FREQ_ROW_941_HZ    (3<<6)
#define AIF_COMFORT_FREQ(n)         (((n)&3)<<8)
#define AIF_COMFORT_FREQ_425_HZ     (0<<8)
#define AIF_COMFORT_FREQ_950_HZ     (1<<8)
#define AIF_COMFORT_FREQ_1400_HZ    (2<<8)
#define AIF_COMFORT_FREQ_1800_HZ    (3<<8)
#define AIF_TONE_GAIN(n)            (((n)&3)<<12)
#define AIF_TONE_GAIN_0_DB          (0<<12)
#define AIF_TONE_GAIN_M3_DB         (1<<12)
#define AIF_TONE_GAIN_M9_DB         (2<<12)
#define AIF_TONE_GAIN_M15_DB        (3<<12)

//side_tone
#define AIF_SIDE_TONE_GAIN(n)       (((n)&15)<<0)

//Cfg_Clk_AudioBCK
#define AIF_AUDIOBCK_DIVIDER(n)     (((n)&0x7FF)<<0)
#define AIF_BCK_POL                 (1<<16)
#define AIF_BCK_POL_NORMAL          (0<<16)
#define AIF_BCK_POL_INVERT          (1<<16)
#define AIF_BCK_SEL_PLL             (1<<20)
#define AIF_BCK_PLL_SOURCE          (1<<21)
#define AIF_BCK_PLL_SOURCE_PLL_150M (0<<21)
#define AIF_BCK_PLL_SOURCE_PLL_CODEC (1<<21)

//Cfg_Aif_Tx_Stb
#define AIF_AIF_TX_STB_DIV(n)       (((n)&0x3FFF)<<0)
#define AIF_AIF_TX_STB_26M_EN       (1<<30)
#define AIF_AIF_TX_STB_EN           (1<<31)

#endif
