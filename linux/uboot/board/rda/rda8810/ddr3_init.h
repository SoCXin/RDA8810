#ifndef _SYS_RAM_INIT_H_
#define _SYS_RAM_INIT_H_

#define MEMC_STATUS			0x000
#define MEMC_CONFIG			0x004
#define MEMC_CMD			0x008
#define ADDR_CTRL			0x010
#define DECODE_CTRL			0x014
#define FORMAT_CTRL			0x018
#define LOWPWR_CTRL			0x020
#define TURNAROUND_PRIO			0x030
#define HIT_PRIO			0x034
//QOS0~QOS15:038~078
#define QOSX_CTRL			0x038
#define QOSX_CTRL7			0x058
#define QOSX_CTRL15			0x078
#define TIMEOUT_CTRL			0x07C
#define QUEUE_CTRL			0x080
#define WR_PRIO_CTRL			0x088
#define WR_PRIO_CTRL2			0x08C
#define RD_PRIO_CTRL			0x090
#define RD_PRIO_CTRL2			0x094
#define ACCESS_ADDR_MATCH		0x098
#define ACCESS_ADDR_MATCH_63_32		0x09C
#define ACCESS_ADDR_MASK		0x0A0
#define ACCESS_ADDR_MASK_63_32		0x0A4

#define CHANNEL_STATUS			0x100
#define DIRECT_CMD			0x108
#define MR_DATA				0x110
#define REFRESH_CTRL			0x120
#define INTR_CTRL			0x128
#define INTR_CLR			0x130
#define INTR_STATUS			0x138
#define INTR_INFO			0x140
#define ECC_CTRL			0x148

#define T_REFI				0x200
#define T_RFC				0x204
#define T_MRR				0x208
#define T_MRW				0x20C
#define T_RCD				0x218
#define T_RAS				0x21C
#define T_RP				0x220
#define T_RPALL				0x224
#define T_RRD				0x228
#define T_FAW				0x22C
#define RD_LATENCY			0x230
#define T_RTR				0x234
#define T_RTW				0x238
#define T_RTP				0x23C
#define WR_LATENCY			0x240
#define T_WR				0x244
#define T_WTR				0x248
#define T_WTW				0x24C
#define T_ECKD				0x250
#define T_XCKD				0x254
#define T_EP				0x258
#define T_XP				0x25C
#define T_ESR				0x260
#define T_XSR				0x264
#define T_SRCKD				0x268
#define T_CKSRD				0x26C

#define T_RDDATA_EN			0x300
#define T_PHYWRLAT			0x304
#define RDLVL_CTRL			0x308
#define RDLVL_DIRECT			0x310
#define T_RDLVL_EN			0x318
#define T_RDLVL_RR			0x31C
#define WRLVL_CTRL			0x328
#define WRLVL_DIRECT			0x330
#define T_WRLVL_EN			0x338
#define T_WRLVL_WW			0x33C

#define PHY_PWR_CTRL			0x348
#define PHY_UPD_CTRL			0x350

#define USER_STATUS			0x400
#define USER_CONFIG0			0x404
#define USER_CONFIG1			0x408

#define INTEG_CFG			0xE00
#define INTEG_OUTPUTS			0xE08

#define PERIPH_ID_0			0xFE0
#define PERIPH_ID_1			0xFE4
#define PERIPH_ID_2			0xFE8
#define PERIPH_ID_3			0xFEC
#define COMPONENT_ID_0			0xFF0
#define COMPONENT_ID_1			0xFF4
#define COMPONENT_ID_2			0xFF8
#define COMPONENT_ID_3			0xFFC

typedef struct {
	UINT32 reg_offset;
	UINT32 dll_on_val;
	UINT32 dll_off_val;
} dmc_reg_t;

#endif /* _SYS_RAM_INIT_H_ */
