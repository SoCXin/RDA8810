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
	{T_REFI,	0x90,		0x90},
	{T_RFC,		0x2F0016,	0x2F0016},
	{T_MRR,		0x2,		0x2},
	{T_MRW,		0x5,		0x5},
	{T_RCD,		0x8,		0x8},
	{T_RAS,		0x11,		0x11},
	{T_RP,		0x8,		0x8},
	{T_RPALL,	0x9,		0x9},
	{T_RRD,		0x4,		0x4},
	{T_FAW,		0x14,		0x14},
	{T_RTR,		0x4,		0x4},
	{T_RTW,		0x0F,		0x0F},
	{T_RTP,		0x6,		0x6},
	{T_WR,		0xE,		0xE},
	{T_WTR,		0x1F000F,	0x1F000F},
	{T_WTW,		0x40000,	0x40000},
	{RD_LATENCY,	0x8,		0x8},
	{WR_LATENCY,	0x4,		0x4},
	{T_RDDATA_EN,	0x9,		0x9},
	{T_PHYWRLAT,	0x103,		0x103},
	{T_XP,		0x20003,	0x20003},
	{T_XP,		0x80003,	0x80003},
	{T_ESR,		0x6,		0x6},
	{T_XSR,		0x580032,	0x580032},
	{T_SRCKD,	0x5,		0x5},
	{T_CKSRD,	0x5,		0x5},
	{T_ECKD,	0x5,		0x5},
	{T_XCKD,	0x5,		0x5},
	{T_EP,		0x5,		0x5},
	{RDLVL_CTRL,	0x1010,		0x1010},
	{RDLVL_CTRL,	0x1110,		0x1110},
	{RDLVL_CTRL,	0x11110,	0x11110},
	{REFRESH_CTRL,	0x1,		0x1},
	{ECC_CTRL,	0x0,		0x0},
	{ADDR_CTRL,	0x30200,	0x30200},
	{ECC_CTRL,	0x0,		0x0},
	{QOSX_CTRL6,	0x9,		0x9}, //set qos priority 9 for modem
	{QUEUE_CTRL,	0x10,		0x10},//reserve 1 queue for modem
};

const dmc_reg_t dmc_reg_low_power_cfg[] = {
	{LOWPWR_CTRL,	0x48,	0x48},
	{T_ESR,		0x6,	0x6},
	{T_XSR,		0x32,	0x32},
	{T_SRCKD,	0xa,	0xa},
	{T_CKSRD,	0xa,	0x5},
};

const dmc_reg_t ddr_config[] = {
	{DIRECT_CMD,	0x0,		0x0},
	{DIRECT_CMD,	0x1000003f,	0x1000003f},
	{DIRECT_CMD,	0x1000ff0a,	0x1000ff0a},
	{DIRECT_CMD,	0x10008301,	0x10008301},
	{DIRECT_CMD,	0x10000602,	0x10000602},
	{DIRECT_CMD,	0x10000103,	0x10000103},
	{DIRECT_CMD,	0x30000000,	0x30000000},
};

const dmc_reg_t dmc_digphy_config[] = {
	{USE_ADDR*4,			0x0,	0x0},
	{WDELAY_SEL*4,			0x3,	0x3},
	{RDELAY_SEL*4,			0x6,	0x6},
	{DQOUT_ENABLE_LAT*4,		0x1,	0x1},
	{DATA_WRITE_ENABLE_REG*4,	0xf,	0xf},
#ifdef CONFIG_RDA_FPGA
	{DATA_ODT_ENABLE_REG*4,		0xe,	0xe},
#else
	{DATA_ODT_ENABLE_REG*4,		0xf,	0xf},
#endif
};

const dmc_reg_t dmc_digphy_eco2_config[] = {
	{USE_ADDR*4,			0x0,	0x0},
	{WDELAY_SEL*4,			0x7,	0x7},
	{RDELAY_SEL*4,			0x6,	0x6},
	{DQOUT_ENABLE_LAT*4,            0x3,    0x3},
	{DATA_WRITE_ENABLE_REG*4,	0xf,	0xf},
#ifdef CONFIG_RDA_FPGA
	{DATA_ODT_ENABLE_REG*4,		0xe,	0xe},
#else
	{DATA_ODT_ENABLE_REG*4,		0xf,	0xf},
#endif
};
#endif
