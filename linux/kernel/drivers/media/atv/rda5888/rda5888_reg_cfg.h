/*
 * rda5888_reg_cfg.h - RDA5888 Device Driver.
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

#ifndef _RDA5888_REG_CFG_H
#define _RDA5888_REG_CFG_H

#include "rda5888.h"

static const struct rda_reg_data rda5888_rf_init_datas[] = {
	{0x002, 0x0002}, /* 9'h002,16'h0002, soft reset */
	{0x002, 0x0001}, /* 9'h002,16'h0001, */
	{0x003, 0x0920}, /* 9'h003,16'h0920, */
	{0x004, 0xbf40}, /* 9'h004,16'hbf40, */
	{0x005, 0x01aa}, /* 9'h005,16'h01aa, by 110115 for ECO1 */
	{0x002, 0x000d}, /* 9'h002,16'h000d, */
	{RDA5888_DELAY_FLAG, 0x0005},  /* wait 5ms, */
	{0x030, 0x0001}, /* 9'h030,16'h0001, soft reset interface */
	{0x100, 0x0000}, /* 9'h100,16'h0000, soft reset dsp */
	{0x100, 0x0001}, /* 9'h100,16'h0001, */
	{0x008, 0x033f}, /* 9'h008,16'h033f, */
	{0x009, 0x447f}, /* 9'h009,16'h447f, */
	{0x00a, 0x2924}, /* 9'h00a,16'h2924, for ECO1 */
	{0x00d, 0x01e1}, /* 9'h00d,16'h01e1, */
	{0x010, 0x0044}, /* 9'h010,16'h0044, */
	{0x011, 0x0c85}, /* 9'h011,16'h0c85, */
	{0x013, 0x8248}, /* 9'h013,16'h8248, */
	{0x014, 0x5018}, /* 9'h014,16'h5018, */
	{0x015, 0x102b}, /* 9'h015,16'h102b, */
	{0x017, 0x1112}, /* 9'h017,16'h1112, by 101216 */
	{0x01a, 0x1ce7}, /* 9'h01a,16'h1ce7, */
	{0x01b, 0x1ce7}, /* 9'h01b,16'h1ce7, */
	{0x01d, 0x4e10}, /* 9'h01d,16'h4e10, */
	{0x01e, 0xe33c}, /* 9'h01e,16'he33c, dac I/Q all on */
	{0x01f, 0x2042}, /* 9'h01f,16'h2042, */
	{0x020, 0x6420}, /* 9'h020,16'h6420, */
	{0x021, 0x96a0}, /* 9'h021,16'h96a0, for 27MHz DCXO */
	{0x022, 0xffff}, /* 9'h022,16'hffff, */
	{0x023, 0x3fff}, /* 9'h023,16'h3fff, */
	{0x024, 0x00f1}, /* 9'h024,16'h00f1, */
	{0x025, 0x0070}, /* 9'h025,16'h0070, */
	{0x026, 0x0010}, /* 9'h026,16'h0010, */
	{0x027, 0x2010}, /* 9'h027,16'h2010, */
	{0x028, 0x3c00}, /* 9'h028,16'h3c00, */
	{0x029, 0x3c00}, /* 9'h029,16'h3c00, */
	{0x02a, 0x3c00}, /* 9'h02a,16'h3c00, */
	{0x02b, 0x3c00}, /* 9'h02b,16'h3c00, */
	{0x02c, 0x7777}, /* 9'h02c,16'h7777, */
	{0x02d, 0x3333}, /* 9'h02d,16'h3333, for ECO1 */
	{0x02e, 0x090d}, /* 9'h02e,16'h090d, for ECO1 */
	{0x036, 0x6a85}, /* 9'h036,16'h6a85, */
	{0x037, 0x8720}, /* 9'h037,16'h8720, */
	{0x038, 0x5155}, /* 9'h038,16'h5155, */
	{0x03b, 0x07ef}, /* 9'h03b,16'h07ef, 130MHz  LNA100 ->  LNA200 */
	{0x03c, 0x1424}, /* 9'h03c,16'h1424,  330MHz  LNA200 ->  LNA800 */
	{0x045, 0x00e2}, /* 9'h045,16'h00e2, */
	{0x085, 0x0a88}, /* 9'h085,16'h0a88, */
	{0x099, 0x09A3}, /* 9'h099,16'h19a3, */
	{0x09b, 0xd488}, /* 9'h09b,16'hd488, */
	{0x09e, 0x460c}, /* 9'h09e,16'h460c, */
	{0x09f, 0x0086}, /* 9'h09f,16'h0086, FM VOL */
	{0x0a0, 0x001D}, /* 9'h0a0,16'h06d9, */
	{0x0a3, 0x2346}, /* 9'h0a3,16'h2346, */
	{0x0a4, 0x0406}, /* 9'h0a4,16'h0406, */
	{0x0a5, 0x060b}, /* 9'h0a5,16'h060b, */
	{0x0a9, 0x080f}, /* 9'h0a9,16'h080f, */
	{0x0aa, 0x0010}, /* 9'h0aa,16'h0010, */
	{0x0b3, 0x2822}, /* 9'h0b3,16'h2822, */
	{0x0b4, 0x1280}, /* 9'h0b4,16'h1280, */
	{0x082, 0x86c1}, /* 9'h082,16'h86c1, FM On */
	{0x030, 0x0005}, /* 9'h030,16'h0005, calibration */
	{RDA5888_DELAY_FLAG, 0x0005}, /* wait 5ms, */
	{0x02e, 0xe90d}, /* 9'h02e,16'he90d, for ECO1 */
	{0x030, 0x0007}, /* 9'h030,16'h0007, */
};

#if (RDA5888_SHARE_26MCRYSTAL == 1)
static const struct rda_reg_data rda5888_26m_crystal_init_datas[] = {
	{0x021, 0x96e0}, /* 9'h021,16'h96E0; for 8.5pf 26M DCXO */
	{0x03a, 0x6590}, /* 9'h03a,16'h6590; */
	{0x163, 0x3b13}, /* 9'h163,16'h3b13; */
	{0x164, 0x0531}, /* 9'h164,16'h0531; */
};
#endif

static const struct rda_reg_data rda5888_dsp_init_datas[] = {
	{0x100, 0x0000}, /* 9'h100,16'h0000; reset dig registers */
	{0x100, 0x0001}, /* 9'h100,16'h0001; */
	{0x130, 0x0000}, /* 9'h130,16'h0000; reset dsp */
	{0x102, 0x141e}, /* 9'h102,16'h141e; */
	{0x105, 0xffff}, /* 9'h105,16'h3fff; 0x3fff afc_disable */
	{0x106, 0x003c}, /* 9'h106,16'h003c; */
	{0x107, 0x00ff}, /* 9'h107,16'h00ff; sk_rssi_rate,  sk_tmr_afc */
	{0x108, 0x0028}, /* 9'h108,16'h0028; sk_rssi_th */
	{0x10a, 0x09df}, /* 9'h10a,16'h09df; */
	{0x10b, 0x42FC}, /* 9'h10b,16'h42f7; */
	{0x10c, 0x7bdf}, /* 9'h10c,16'h7bdf; */
	{0x10d, 0x082d}, /* 9'h10d,16'h082d; */
	{0x10e, 0xff80}, /* 9'h10e,16'hff80; */
	{0x10f, 0x4444}, /* 9'h10f,16'h4444; */
	{0x111, 0x5413}, /* 9'h111,16'h5413; */
	{0x112, 0x3002}, /* 9'h112,16'h3002; */
	{0x113, 0x90d0}, /* 9'h113,16'h90d0; */
	{0x114, 0xcfe0}, /* 9'h114,16'hcfe0; */
	{0x115, 0x048a}, /* 9'h115,16'h048a; */
	{0x116, 0x1000}, /* 9'h116,16'h1000; */
	{0x117, 0x0000}, /* 9'h117,16'h0000; */
	{0x118, 0x5fdf}, /* 9'h118,16'h5fdf; auto black */
	{0x119, 0x0c84}, /* 9'h119,16'h0c84; */
	{0x11b, 0x1002}, /* 9'h11b,16'h1002; */
	{0x11c, 0xc200}, /* 9'h11c,16'hc200; */
	{0x11d, 0x6180}, /* 9'h11d,16'h6180; ha_index_var_thd & ha_value_var_thd */
	{0x11e, 0x61c2}, /* 9'h11e,16'h61c2; */
	{0x11f, 0x980e}, /* 9'h11f,16'h980e; */
	{0x120, 0x403a}, /* 9'h120,16'h403a; */
	{0x125, 0x0870}, /* 9'h125,16'h0870; gainct_fm */
	{0x129, 0x09db}, /* 9'h129,16'h09cb; framebuffer anti interference */
	{0x12f, 0x0c00}, /* 9'h12f,16'h0c00; iq swap = 1 */
	{0x133, 0x0586}, /* 9'h133,16'h0586; */
	{0x134, 0x02f2}, /* 9'h134,16'h02f2; */
	{0x135, 0x041c}, /* 9'h135,16'h041c; */
	{0x137, 0x035f}, /* 9'h137,16'h035f; */
	{0x13a, 0x038a}, /* 9'h13a,16'h038a; h_chrom_win_start */
	{0x13b, 0x03bc}, /* 9'h13b,16'h03bc; h_chrom_win_end */
	{0x140, 0x402d}, /* 9'h140,16'h402d; */
	{0x141, 0x0001}, /* 9'h141,16'h0001; */
	{0x142, 0x00a5}, /* 9'h142,16'h00a5; notch coefficients 2M */
	{0x143, 0x1525}, /* 9'h143,16'h1525; notch coefficients */
	{0x144, 0x0425}, /* 9'h144,16'h0425; notch coefficients */
	{0x145, 0x00f0}, /* 9'h145,16'h00f0; */
	{0x146, 0x1220}, /* 9'h146,16'h1220; */
	{0x147, 0x3046}, /* 9'h147,16'h3046; */
	{0x148, 0x647a}, /* 9'h148,16'h647a; */
	{0x149, 0x8808}, /* 9'h149,16'h8808; */
	{0x14a, 0x8000}, /* 9'h14a,16'h8000; rssi_check_timer */
	{0x14b, 0x0000}, /* 9'h14b,16'h0000; */
	{0x14c, 0x0000}, /* 9'h14c,16'h0000; */
	{0x14d, 0x8888}, /* 9'h14d,16'h8888; */
	{0x14e, 0x8888}, /* 9'h14e,16'h8888; */
	{0x14f, 0x0000}, /* 9'h14f,16'h0000; */
	{0x150, 0x0400}, /* 9'h150,16'h0400;  SCK pull-down by 101218 */
#if (RDA5888_IODRV_ENHANCE == 1)
	{0x151, 0x0022},
	{0x152, 0x0880},
	{0x153, 0x0002},
	{0x154, 0x0000},
	{0x155, 0x0002},
#else
	{0x151, 0x0000},
	{0x152, 0x0000},
	{0x153, 0x0000},
	{0x154, 0x0000},
	{0x155, 0x0000},
#endif
	{0x156, 0x0000}, /* 9'h156,16'h0000; */
	{0x157, 0x0000}, /* 9'h157,16'h0000; */
	{0x158, 0x0000}, /* 9'h158,16'h0000; */
	{0x159, 0x0000}, /* 9'h159,16'h0000; */
	{0x15a, 0x0124}, /* 9'h15a,16'h0124;by 110115 xu */
	{0x15b, 0x06bf}, /* 9'h15b,16'h06bf; */
	{0x15c, 0x00b3}, /* 9'h15c,16'h00b3; */
	{0x15d, 0x0017}, /* 9'h15d,16'h0017; by 110115 xu */
	{0x15e, 0x0016}, /* 9'h15e,16'h0016; change 100826 */
	{0x15f, 0x0139}, /* 9'h15f,16'h0139; change 100826 */
	{0x160, 0x0fde}, /* 9'h160,16'h0fde; pllbb setting */
	{0x161, 0x0000}, /* 9'h161,16'h0000; */
	{0x162, 0x0000}, /* 9'h162,16'h0000; */
	{0x163, 0x0000}, /* 9'h163,16'h0000; */
	{0x164, 0x0500}, /* 9'h164,16'h0500; */
	{0x165, 0x7820}, /* 9'h165,16'h7820; edited by xu 10182010 */
	{0x166, 0x7820}, /* 9'h166,16'h7820; */
	{0x167, 0x7820}, /* 9'h167,16'h7820; */
	{0x168, 0x7c10}, /* 9'h168,16'h7c10; */
	{0x169, 0x7c10}, /* 9'h169,16'h7c10; */
	{0x16a, 0x7e08}, /* 9'h16a,16'h7e08; */
	{0x16b, 0x7e86}, /* 9'h16b,16'h7e86; */
	{0x16c, 0xbe86}, /* 9'h16c,16'hbe86; */
	{0x1ca, 0x1500}, /* 9'h1ca,16'h1500; */
	{0x16d, 0x4924}, /* 9'h16d,16'h4924; */
	{0x16e, 0x4800}, /* 9'h16e,16'h4800; */
	{0x16f, 0x0000}, /* 9'h16f,16'h0000; */
	{0x170, 0x00bf}, /* 9'h170,16'h00bf; dccancel */
	{0x171, 0x0006}, /* 9'h171,16'h0006; */
	{0x172, 0x4c14}, /* 9'h172,16'h4c1d; */
	{0x173, 0x4c14}, /* 9'h173,16'h4c1d; */
	{0x174, 0x008c}, /* 9'h174,16'h008c; */
	{0x175, 0x1021}, /* 9'h175,16'h1021; */
	{0x176, 0x0030}, /* 9'h176,16'h0030; */
	{0x177, 0x01cf}, /* 9'h177,16'h01cf; */
	{0x178, 0x236d}, /* 9'h178,16'h236d; */
	{0x179, 0x0080}, /* 9'h179,16'h0080; */
	{0x17a, 0x1441}, /* 9'h17a,16'h1441; */
#if (RDA5888_HSYNC_HOLD == 1)
	{0x17b, 0x4000}, /* 9'h17b,16'h4000; // open the it601 interlace mode. */
#else
	{0x17b, 0x0000}, /* 9'h17b,16'h4000; close the it601 interlace mode. */
#endif
	{0x17c, 0x0000}, /* 9'h17c,16'h0000; */
	{0x17d, 0x0000}, /* 9'h17d,16'h0000; */
	{0x17e, 0x0000}, /* 9'h17e,16'h0000; */
	{0x17f, 0x01c0}, /* 9'h17f,16'h0000; */
	{0x180, 0x0004}, /* 9'h180,16'h0004; */
	{0x181, 0x2040}, /* 9'h181,16'h2040; */
	{0x182, 0x0138}, /* 9'h182,16'h0138;edit by tong 20100827 */
	{0x183, 0xe235}, /* 9'h183,16'he235; afc_disable_fm */
	{0x184, 0x4924}, /* 9'h184,16'h4924; */
	{0x185, 0x0000}, /* 9'h185,16'h0000; */
	{0x186, 0x0000}, /* 9'h186,16'h0000; */
	{0x187, 0x0000}, /* 9'h187,16'h0000; */
	{0x188, 0x0000}, /* 9'h188,16'h0000; */
	{0x189, 0x0000}, /* 9'h189,16'h0000; */
	{0x18a, 0x0000}, /* 9'h18a,16'h0000; */
	{0x18b, 0x0000}, /* 9'h18b,16'h0000; */
	{0x18c, 0x0000}, /* 9'h18c,16'h0000; */
	{0x18d, 0x0000}, /* 9'h18d,16'h0000; */
	{0x18e, 0x0000}, /* 9'h18e,16'h0000; */
	{0x18f, 0x051f}, /* 9'h18f,16'h051f; tmr_agc1 */
	{0x190, 0x0008}, /* 9'h190,16'h0008; */
	{0x191, 0x1ac1}, /* 9'h191,16'h1ac1; by zm 101222 */
	{0x192, 0x25d4}, /* 9'h192,16'h25d4; */
	{0x193, 0x2edd}, /* 9'h193,16'h2edd; */
	{0x194, 0x815d}, /* 9'h194,16'h815d; */
	{0x195, 0x0306}, /* 9'h195,16'h0306; */
	{0x196, 0x0306}, /* 9'h196,16'h0306; */
	{0x197, 0x0a9e}, /* 9'h197,16'h0798; */
	{0x198, 0x1434}, /* 9'h198,16'h112e; */
	{0x199, 0x6300}, /* 9'h199,16'h6300; tagt_pwr */
	{0x19a, 0x0014}, /* 9'h19a,16'h0014; */
	{0x19b, 0x0f09}, /* 9'h19B,16'h0f09; bypass rate_conv & enable notch & sk_mode */
	{0x19c, 0x00ff}, /* 9'h19C,16'h00ff; tmr_rssi1 */
	{0x19d, 0x14fe}, /* 9'h19d,16'h14fe; tmr_rssi3 */
	{0x19e, 0x0000}, /* 9'h19e,16'h0000; */
	{0x19f, 0x0000}, /* 9'h19f,16'h0000; */
	{0x1a0, 0x1f39}, /* 9'h1a0,16'h1f39;Notch filter for audio */
	{0x1a1, 0x071e}, /* 9'h1a1,16'h071e;Notch filter for audio */
	{0x1a2, 0x0070}, /* 9'h1a2,16'h0070;Notch filter for audio */
	{0x1a3, 0x0002}, /* 9'h1a3,16'h0002; */
	{0x1a4, 0x0000}, /* 9'h1a4,16'h0000; */
	{0x1a5, 0x0080}, /* 9'h1a5,16'h0080; */
	{0x1a6, 0xc120}, /* 9'h1a6,16'hc120; *added by tong 2010 0801 */
	{0x1a7, 0x0741}, /* 9'h1a7,16'h0741; */
	{0x1a8, 0x1920}, /* 9'h1a8,16'h1920; */
	{0x1a9, 0x1800}, /* 9'h1a9,16'h1800; lum_path_delay *added by tong 2010 0903 */
	{0x1aa, 0x0000}, /* 9'h1aa,16'h0000; */
	{0x1ab, 0x18c8}, /* 9'h1ab,16'h18c8; clk_dcdc=1.25m,  no dither */
	{0x1ac, 0x8800}, /* 9'h1ac,16'h8800;edit 20100127 */
	{0x1ad, 0x3000}, /* 9'h1ad,16'h3000; seek_en */
	{0x1ae, 0x4270}, /* 9'h1ae,16'h4270; change 100826 */
	{0x1af, 0x0000}, /* 9'h1af,16'h0000; */
	{0x1b0, 0x8000}, /* 9'h1b0,16'h8000; */
	{0x1b3, 0x3f20}, /* 9'h1b3,16'h3f20; */
	{0x1b5, 0x0008}, /* 9'h1b5,16'h0008; */
	{0x1b8, 0x0000}, /* 9'h1b8,16'h0000; */
	{0x1bb, 0x202f}, /* 9'h1bb,16'h202f; */
	{0x1bd, 0x129d}, /* 9'h1bd,16'h129d; */
	{0x1be, 0x05c1}, /* 9'h1be,16'h05c1; */
	{0x1c0, 0x3d08}, /* 9'h1c0,16'h3d08; */
	{0x1c1, 0x0007}, /* 9'h1c1,16'h0007; */
	{0x1c2, 0x0001}, /* 9'h1c2,16'h0001; */
	{0x1c3, 0x20c2}, /* 9'h1c3,16'h20c2; */
#if (RDA5888_HSYNC_HOLD == 1)
	{0x1c4, 0x0428}, /* 9'h1c4,16'h0248; */
#else
	{0x1c4, 0x0200}, /* 9'h1c4,16'h0248; no delay hvsync for green arise when swing ana */
#endif
	{0x1c5, 0x0001}, /* 9'h1c5,16'h0001; */
	{0x1ca, 0x1500}, /* 9'h1ca,16'h1500; */
	{0x1ce, 0x1234}, /* 9'h1ce,16'h1234; */
	{0x1cf, 0x00c2}, /* 9'h1cf,16'h00c2; */
	{0x1d0, 0x00c2}, /* 9'h1d0,16'h00c2; */
	{0x1d1, 0x0321}, /* 9'h1d1,16'h0321; */
	{0x1d2, 0x057e}, /* 9'h135,16'h041c; */
	{0x1d3, 0x0070}, /* 9'h1d3,16'h00f0; */
	{0x1d4, 0x0000}, /* 9'h1d4,16'h0000; */
	{0x1d5, 0x0000}, /* 9'h1d5,16'h0000; */
	{0x1d6, 0x0000}, /* 9'h1d6,16'h0000; */
	{0x1d7, 0x0207}, /* 9'h1d7,16'h0207; added for framebuf coef sepe */
	{0x1d8, 0x71fe}, /* 9'h1d8,16'h71fe; edited by xutao 09032010 for eco1 */
	{0x1dc, 0x6800}, /* 9'h1dc,16'h6800; */
	{0x1dd, 0x1200}, /* 9'h1dd,16'h1200; */
	{0x1de, 0x3800}, /* 9'h1de,16'h3800; dited by xutao 09032010 */
	{0x1dd, 0xd200}, /* 9'h1dd,16'hd200; bypass framebuf when async, but not reset */
	{0x1e0, 0x0c10}, /* 9'h1e0,16'h0c10; */
	{0x1e1, 0x0c10}, /* 9'h1e1,16'h0c10; */
	{0x1e2, 0x0c10}, /* 9'h1e2,16'h0c10; */
	{0x1e3, 0x0e08}, /* 9'h1e3,16'h0e08; */
	{0x1e4, 0x0e08}, /* 9'h1e4,16'h0e08; */
	{0x1e5, 0x0f04}, /* 9'h1e5,16'h0f04; */
	{0x1e6, 0x0f04}, /* 9'h1e6,16'h0f04; */
	{0x1e7, 0x0f04}, /* 9'h1e7,16'h0f04; */
	{0x1e9, 0x706c}, /* 9'h1e9,16'h706c;fm softmute by tong 04212010 */
	{0x1ed, 0x08cf}, /* 9'h1ed,16'h08cf;fm softmute by tong 04212010 */
	{0x1ee, 0x1708}, /* 9'h1ee,16'h1708; */
	{0x1f0, 0x0480}, /* 9'h1f0,16'h0480; */
	{0x1f1, 0x5c94}, /* 9'h1f1,16'h5c94; */
	{0x1f3, 0x0228}, /* 9'h1f3,16'h0228; */
	{0x1f4, 0x2244}, /* 9'h1f4,16'h2244; */
	{0x1f5, 0x3348}, /* 9'h1f7,16'h3348; */
	{0x1f8, 0x0015}, /* 9'h1f8,16'h0015; */
	{0x1f9, 0xf989}, /* 9'h1f9,16'hf989; */
	{0x1d8, 0x71f8}, /* 9'h1d8,16'h71f8; */
	{0x1a8, 0x1c40}, /* 9'h1a8,16'h1840; 0x1840 */
	{0x114, 0xcfe0}, /* 9'h114,16'hcfe0; */
	{0x1a6, 0xc020}, /* 9'h1a6,16'hc020; */
	{0x11e, 0x6182}, /* 9'h11e,16'h6182; */
	{0x11b, 0x9002}, /* 9'h11b,16'h9002; */
	{0x1de, 0x463f}, /* 9'h1de,16'h463f; */
	{0x1b2, 0x0f0f}, /* 9'h1b2,16'h0f0f; */
	{0x1c3, 0x2000}, /* 9'h1c3,16'h2000; */
	{0x1dd, 0x9200}, /* 9'h1dd,16'h9200; */
	{0x1d7, 0x0007}, /* 9'h1d7,16'h0007; */
	{0x130, 0x0010}, /* 9'h130,16'h0010; */
	{0x130, 0x0810}, /* 9'h130,16'h0810; */
	{0x1AC, 0x9800},
};

static const struct rda_reg_data rda5888_vstd_pal_bg_datas[] = {
	{0x104, 0x284c}, /* 9'h104,16'h284c; angle_in_if */
	{0x111, 0x5413}, /* 9'h111,16'h5413; angle_in_sub */
	{0x140, 0x4009}, /* 9'h140,16'h4009; angle_mode_if, angle_mode_fm, angle_mode_sub, line625_flag */
	{0x1a0, 0x1bc3}, /* 9'h1a0,16'h1bc3;Notch filter for audio */
	{0x1a1, 0x071e}, /* 9'h1a1,16'h071e;Notch filter for audio */
	{0x1a2, 0x0070}, /* 9'h1a2,16'h0070;Notch filter for audio */
	{0x082, 0x8541}, /* 9'h082,16'h8541;by 20101230 FM */
	{0x146, 0x0f1d}, /* 9'h146,16'h0f1d; softmute */
	{0x147, 0x293e}, /* 9'h147,16'h293e; */
	{0x148, 0x5b73}, /* 9'h148,16'h5b73; */
	{0x149, 0x8508}, /* 9'h149,16'h8508; */
	/* add for audio notch-filter. */
	{0x1a3, 0xdd52},
	{0x1bc, 0x038f},
	{0x1bf, 0x0038},
};

static const struct rda_reg_data rda5888_vstd_pal_dk_datas[] = {
	{0x140, 0x402d}, /* 9'h140,16'h402d; angle_mode_if, angle_mode_fm, angle_mode_sub, line625_flag */
	{0x146, 0x1220}, /* 9'h146,16'h1220; */
	{0x147, 0x3046}, /* 9'h147,16'h3046; */
	{0x148, 0x647a}, /* 9'h148,16'h647a; */
	{0x149, 0x8808}, /* 9'h149,16'h8808; */
	{0x111, 0x5413}, /* 9'h111,16'h5413; angle_in_sub */
	{0x1a0, 0x1f39}, /* 9'h1a0,16'h1f39;Notch filter for audio */
	{0x1a1, 0x071e}, /* 9'h1a1,16'h071e;Notch filter for audio */
	{0x1a2, 0x0070}, /* 9'h1a2,16'h0070;Notch filter for audio */
	{0x082, 0x86c1}, /* 9'h082,16'h86c1;by 20101230 FM */
	/* add for audio notch-filter. */
	{0x1a3, 0xf8f2},
	{0x1bc, 0x038f},
	{0x1bf, 0x0038},
};

static const struct rda_reg_data rda5888_vstd_pal_i_datas[] = {
	{0x111, 0x5413}, /* 9'h111,16'h5413; angle_in_sub */
	{0x140, 0x4024}, /* 9'h140,16'h4024; angle_mode_if, angle_mode_fm, angle_mode_sub, line625_flag */
	{0x146, 0x1220}, /* 9'h146,16'h1220; */
	{0x147, 0x3046}, /* 9'h147,16'h3046; */
	{0x148, 0x647a}, /* 9'h148,16'h647a; */
	{0x149, 0x8808}, /* 9'h149,16'h8808; */
	{0x1a0, 0x1d7a}, /* 9'h1a0,16'h1d7a;Notch filter for audio */
	{0x1a1, 0x071e}, /* 9'h1a1,16'h071e;Notch filter for audio */
	{0x1a2, 0x0070}, /* 9'h1a2,16'h0070;Notch filter for audio */
	{0x082, 0x8641}, /* 9'h082,16'h8641;by 20101230 FM */
	/* add for audio notch-filter. */
	{0x1a3, 0xeb02},
	{0x1bc, 0x038f},
	{0x1bf, 0x0038},
};

static const struct rda_reg_data rda5888_vstd_pal_m_datas[] = {
	{0x111, 0x43ce}, /* 9'h111,16'h43ce; angle_in_sub */
	{0x140, 0x0040}, /* 9'h140,16'h0040; angle_mode_if, angle_mode_fm, angle_mode_sub, line625_flag */
	{0x1a0, 0x1872}, /* 9'h1a0,16'h1872; Notch filter for audio */
	{0x1a1, 0x0749}, /* 9'h1a1,16'h0749; Notch filter for audio */
	{0x1a2, 0x005b}, /* 9'h1a2,16'h005b; Notch filter for audio */
	{0x1a9, 0x1c00}, /* 9'h1a9,16'h1c00; lum_path_delay */
	{0x1ba, 0x06b3}, /* 9'h1ba,16'h06b3; comb_constant */
	{0x082, 0x8441}, /* 9'h082,16'h8441;by 20101230 FM */
	{0x15a, 0x010c}, /* 9'h15A,16'h010c; added by xu 09272010 for snow */
	{0x15b, 0x06b3}, /* 9'h15B,16'h06B3; */
	{0x1ae, 0x420c}, /* 9'h1AE,16'h420c; */
	{0x146, 0x0f1c}, /* 9'h146,16'h0f1c; softmute added 10182010 by xu */
	{0x147, 0x283a}, /* 9'h147,16'h283a; */
	{0x148, 0x5367}, /* 9'h148,16'h5367; */
	{0x149, 0x7007}, /* 9'h149,16'h7007; */
	/* add for audio notch-filter. */
	{0x1a3, 0xc382},
	{0x1bc, 0x038f},
	{0x1bf, 0x0038},
};

static const struct rda_reg_data rda5888_vstd_pal_n_datas[] = {
	{0x140, 0x4100}, /* 9'h140,16'h4100; angle_mode_if, angle_mode_fm, angle_mode_sub, line625_flag */
	{0x111, 0x43ed}, /* 9'h111,16'h43ed; angle_in_sub */
	{0x1a0, 0x1872}, /* 9'h1a0,16'h1872;Notch filter for audio */
	{0x1a1, 0x0749}, /* 9'h1a1,16'h0749;Notch filter for audio */
	{0x1a2, 0x005b}, /* 9'h1a2,16'h005b;Notch filter for audio */
	{0x082, 0x8441}, /* 9'h082,16'h8441;by 20101230 FM */
	{0x146, 0x0f1c}, /* 9'h146,16'h0f1c */
	{0x147, 0x283a}, /* 9'h147,16'h283a */
	{0x148, 0x5367}, /* 9'h148,16'h5367 */
	{0x149, 0x7007}, /* 9'h149,16'h7007 */
	/* add for audio notch-filter. */
	{0x1a3, 0xc382},
	{0x1bc, 0x038f},
	{0x1bf, 0x0038},
};

static const struct rda_reg_data rda5888_vstd_secam_bg_datas[] = {
	{0x111, 0x5142}, /* 9'h111,16'h5142; */
	{0x116, 0x008c}, /* 9'h116,16'h008c; u_cb_dis, k1a */
	{0x117, 0x0fd2}, /* 9'h117,16'h0fd2; k1b */
	{0x118, 0x3fbf}, /* 9'h118,16'h3fbf; */
	{0x140, 0x40c9}, /* 9'h140,16'h40c9; angle_mode_if, angle_mode_fm, angle_mode_sub, line625_flag */
	{0x145, 0x00d0}, /* 9'h145,16'h00d0; */
	{0x146, 0x4550}, /* 9'h146,16'h4550; */
	{0x147, 0x5e67}, /* 9'h147,16'h5e67; */
	{0x148, 0x7178}, /* 9'h148,16'h7178; */
	{0x149, 0x7c02}, /* 9'h149,16'h7c02; */
	{0x1a0, 0x1bc3}, /* 9'h1a0,16'h1bc3; */
	{0x1a1, 0x071e}, /* 9'h1a1,16'h071e; */
	{0x1a2, 0x0070}, /* 9'h1a2,16'h0070; */
	{0x082, 0x8541}, /* 9'h082,16'h8541;by 20101230 FM */
	{0x1e8, 0x4000}, /* 9'h1e8,16'h4000;by 20101214 */
	/* add for audio notch-filter. */
	{0x1a3, 0xdd52},
	{0x1bc, 0x038f},
	{0x1bf, 0x0038},
};

static const struct rda_reg_data rda5888_vstd_secam_dk_datas[] = {
	{0x10b, 0x8708}, /* 9'h10b,16'h8708; */
	{0x111, 0x5142}, /* 9'h111,16'h5142; */
	{0x112, 0x3000}, /* 9'h112,16'h3000; */
	{0x116, 0x008c}, /* 9'h116,16'h008c; u_cb_dis, k1a */
	{0x117, 0x0fd2}, /* 9'h117,16'h0fd2; k1b */
	{0x118, 0x3fbf}, /* 9'h118,16'h3fbf; */
	{0x140, 0x40ed}, /* 9'h140,16'h40ed; angle_mode_if, angle_mode_fm, angle_mode_sub, line625_flag */
	{0x145, 0x00d0}, /* 9'h145,16'h00d0; */
	{0x146, 0x4550}, /* 9'h146,16'h4550; */
	{0x147, 0x5e67}, /* 9'h147,16'h5e67; */
	{0x148, 0x7178}, /* 9'h148,16'h7178; */
	{0x149, 0x7c01}, /* 9'h149,16'h7c01; */
	{0x1a0, 0x1f39}, /* 9'h1a0,16'h1f39; */
	{0x1a1, 0x071e}, /* 9'h1a1,16'h071e; */
	{0x1a2, 0x0070}, /* 9'h1a2,16'h0070; */
	{0x082, 0x86c1}, /* 9'h082,16'h86c1;by 20101230 FM */
	{0x1e8, 0x4000}, /* 9'h1e8,16'h4000;by 20101214 */
	/* add for audio notch-filter. */
	{0x1a3, 0xf8f2},
	{0x1bc, 0x038f},
	{0x1bf, 0x0038},
};

static const struct rda_reg_data rda5888_vstd_ntsc_m_datas[] = {
	{0x10b, 0x42f9}, /* 9'h10B,16'h42f9; */
	{0x111, 0x43e1}, /* 9'h111,16'h43e1; angle_in_sub */
	{0x137, 0x034c}, /* 9'h137,16'h034c; h_sync_win_end */
	{0x139, 0x03ac}, /* 9'h139,16'h03ac; h_blank_win_end */
	{0x140, 0x0080}, /* 9'h140,16'h0080; angle_mode_if, angle_mode_fm, angle_mode_sub, line625_flag */
	{0x1a0, 0x1872}, /* 9'h1a0,16'h1872; Notch filter for audio */
	{0x1a1, 0x0749}, /* 9'h1a1,16'h0749; Notch filter for audio */
	{0x1a2, 0x005b}, /* 9'h1a2,16'h005b; Notch filter for audio */
	{0x1ba, 0x06b3}, /* 9'h1ba,16'h06b3; comb_constant */
	{0x082, 0x8441}, /* 9'h082,16'h8441;by 20101230 FM */
	{0x15a, 0x010c}, /* 9'h15A,16'h010c; added by xu 09272010 for snow */
	{0x15b, 0x06b3}, /* 9'h15B,16'h06B3; */
	{0x1ae, 0x420c}, /* 9'h1AE,16'h420c;  */
	{0x146, 0x0f16}, /* 9'h146,16'h0f16;  softmute added 10182010 by xu */
	{0x147, 0x2634}, /* 9'h147,16'h2634;  */
	{0x148, 0x4b61}, /* 9'h148,16'h4b61;  */
	{0x149, 0x6e07}, /* 9'h149,16'h6e07;  */
	/* add for audio notch-filter. */
	{0x1a3, 0xc382},
	{0x1bc, 0x038f},
	{0x1bf, 0x0038},
};

#endif /* end of _RDA5888_REG_CFG_H */
