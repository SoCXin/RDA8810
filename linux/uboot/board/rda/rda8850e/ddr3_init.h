#ifndef _SYS_DDR_INIT_H_
#define _SYS_DDR_INIT_H_

#include <common.h>
#include "ddr.h"
#include "ddr_init.h"

#define  PHY_RDLAT          0
#define  PHYWRDATA          1
#define  STA                3
#define  CLKSEL             4
#define  PSSTART            5
#define  PSDONE             6
#define  LOCKED             6
#define  CTRL_DELAY         7
#define  RDELAY_SEL         8
#define  WDELAY_SEL         9
#define  PHY_RESET          10
#define  RESET_DDR3         11
#define  ODT_DELAY          12
#define  DDR3_USED          13
#define  WRITE_ENABLE_LAT   14
#define  WRITE_DATA_LAT     15
#define  DQOUT_ENABLE_LAT   16
#define  DATA_ODT_ENABLE_REG	20
#define  DATA_WRITE_ENABLE_REG  48
#define  DMC_READY	128
#define  USE_ADDR	129

const dmc_reg_t dmc_reg_cfg[] = {
		{QOSX_CTRL7,	0x08,		0x08},
		{TURNAROUND_PRIO,0x1A,		0x0},
		{HIT_PRIO,	0x1A,		0x0},
		{T_REFI,	0x186,		0x3f},
		{QUEUE_CTRL,	0xffff, 	0x0},
		{T_RFC, 	0x23008c,	0x23008c},
		{T_RFC, 	0x8c008c,	0x8c008c},
		{T_MRR, 	0x4,		0x4},
		{T_MRW, 	0xc,		0xc},
		{T_RCD, 	0x6,		0x6},
		{T_RAS, 	0xf,		0xf},
		{T_RP,		0x6,		0x6},
		{T_RPALL,	0x6,		0x5},
		{T_RRD, 	0x4,		0x4},
		{T_FAW, 	0x14,		0x14},
		{T_RTR, 	0x6,		0x6},
	//	{T_RTW, 	0x7,		0x6},
		{T_RTW, 	0x0c,		0x6},// turn for now
		{T_RTW, 	0x1f,		0x1f},//for low speed
		{T_RTP, 	0x4,		0x4},
		{T_WR,		0xf,		0x10},
		{T_WTR, 	0x4000d,	0x4000d},
		{T_WTR, 	0x6000d,	0x6000f},
	//	{T_WTR, 	0x6001f,	0x6001f},//for low speed
		{T_WTW, 	0x60000,	0x60000},
		{RD_LATENCY,	0x5,		0x5},
		{WR_LATENCY,	0x5,		0x5},
		{T_RDDATA_EN,	0x8,		0x3},
		{T_PHYWRLAT,	0x102,		0x102},
		{T_PHYWRLAT,	0x102,		0x103},
		{T_EP,		0x3,		0x3},
		{T_XP,		0x20003,	0x20003},
		{T_XP,		0xa0003,	0xa0003},
		{T_ESR, 	0x4,		0x4},
		{T_XSR, 	0x1000090,	0x1000090},
		{T_XSR, 	0x2000090,	0x2000090},
		{T_SRCKD,	0x5,		0x5},
		{T_CKSRD,	0x5,		0x5},
		{T_ECKD,	0x5,		0x5},
		{T_XCKD,	0x5,		0x5},
		{ECC_CTRL,	0x0,		0x0},
		{ADDR_CTRL, 0x30200,	0x30200},
		{ADDR_CTRL, 0x30200,	0x30200},
		{ADDR_CTRL, 0x30200,	0x30200},

};

const dmc_reg_t dmc_reg_low_power_cfg[] = {
	{LOWPWR_CTRL,	0x48,	0x48},
	{T_ESR,		0xa,	0xa},
	{T_XSR,		0x200,	0x200},
	{T_SRCKD,	0xa,	0xa},
	{T_CKSRD,	0xa,	0xa},
};

const dmc_reg_t ddr_config[] = {
	{DIRECT_CMD,	0x0,		0x0},
	{DIRECT_CMD,	0x10020000,	0x10020008},
	{DIRECT_CMD,	0x10030000,	0x10030000},
	{DIRECT_CMD,	0x10010004,	0x10010005},
	{DIRECT_CMD,	0x10000520,	0x10000520},
	{DIRECT_CMD,	0x50000400,	0x50000400},
	{DIRECT_CMD,	0x30000000,	0x30000000},
};

const dmc_reg_t dmc_digphy_config[] = {
	{DMC_READY*4,			0x0,	0x0},
	{RESET_DDR3*4,			0x1,	0x1},
	{WRITE_DATA_LAT*4,		0x3,	0x1},
	{DDR3_USED*4,			0x1,	0x1},
	{WDELAY_SEL*4,			0x3,	0x3},
//	{RDELAY_SEL*4,			0x1,	0x1},
	{PHY_RDLAT*4,			0x7,	0x7},
	{CTRL_DELAY*4,			0x2,	0x2},
	{WRITE_ENABLE_LAT*4,		0x2,	0x2},
	{DQOUT_ENABLE_LAT*4,		0x3,	0x1},
	{DATA_WRITE_ENABLE_REG*4,	0xf,	0xf},
	{DATA_ODT_ENABLE_REG*4,		0xe,	0xe},

};

const dmc_reg_t dmc_digphy_eco2_config[] = {
	{DMC_READY*4,			0x0,	0x0},
	{RESET_DDR3*4,			0x1,	0x1},
	{WRITE_DATA_LAT*4,		0x3,	0x1},
	{DDR3_USED*4,			0x1,	0x1},
	{WDELAY_SEL*4,			0x3,	0x3},
//	{RDELAY_SEL*4,			0x1,	0x1},
	{PHY_RDLAT*4,			0x7,	0x7},
	{CTRL_DELAY*4,			0x2,	0x2},
	{WRITE_ENABLE_LAT*4,		0x2,	0x2},
	{DQOUT_ENABLE_LAT*4,		0x3,	0x1},
	{DATA_WRITE_ENABLE_REG*4,	0xf,	0xf},
	{DATA_ODT_ENABLE_REG*4,		0xe,	0xe},

};
#endif
