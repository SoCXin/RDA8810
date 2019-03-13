#ifndef __RDA_MMC_H
#define __RDA_MMC_H

#include <mach/hardware.h>

typedef volatile struct
{
	REG32		SDMMC_CTRL;			//0x00000000
	REG32		Reserved_00000004;		//0x00000004
	REG32		SDMMC_FIFO_TXRX; 		//0x00000008
	REG32		Reserved_0000000C[509];	//0x0000000C
	REG32		SDMMC_CONFIG;			//0x00000800
	REG32		SDMMC_STATUS;			//0x00000804
	REG32		SDMMC_CMD_INDEX; 		//0x00000808
	REG32		SDMMC_CMD_ARG;		//0x0000080C
	REG32		SDMMC_RESP_INDEX;		//0x00000810
	REG32		SDMMC_RESP_ARG3; 		//0x00000814
	REG32		SDMMC_RESP_ARG2; 		//0x00000818
	REG32		SDMMC_RESP_ARG1; 		//0x0000081C
	REG32		SDMMC_RESP_ARG0; 		//0x00000820
	REG32		SDMMC_DATA_WIDTH;		//0x00000824
	REG32		SDMMC_BLOCK_SIZE;		//0x00000828
	REG32		SDMMC_BLOCK_CNT; 		//0x0000082C
	REG32		SDMMC_INT_STATUS;		//0x00000830
	REG32		SDMMC_INT_MASK;		//0x00000834
	REG32		SDMMC_INT_CLEAR; 		//0x00000838
	REG32		SDMMC_TRANS_SPEED;		//0x0000083C
	REG32		SDMMC_MCLK_ADJUST;		//0x00000840
#ifdef CONFIG_ARCH_RDA8850E
	REG32		SDMMC_STATUS2;		/* 0x00000844 */
	REG32		SDMMC_RSP_WAITCNT;	/* 0x00000848 */
#endif /* CONFIG_ARCH_RDA8850E */
} HWP_SDMMC_T;

//SDMMC_CTRL
#define SDMMC_L_ENDIAN(n)			(((n)&7)<<0)
#define SDMMC_SOFT_RST_L			(1<<3)

//SDMMC_FIFO_TXRX
#define SDMMC_DATA_IN(n)			(((n)&0xFFFFFFFF)<<0)
#define SDMMC_DATA_OUT(n)			(((n)&0xFFFFFFFF)<<0)

//SDMMC_CONFIG
#define SDMMC_SDMMC_SENDCMD 		(1<<0)
#define SDMMC_SDMMC_SUSPEND 		(1<<1)
#define SDMMC_RSP_EN				(1<<4)
#define SDMMC_RSP_SEL(n)			(((n)&3)<<5)
#define SDMMC_RSP_SEL_R2			(2<<5)
#define SDMMC_RSP_SEL_R3			(1<<5)
#define SDMMC_RSP_SEL_OTHER 		(0<<5)
#define SDMMC_RD_WT_EN				(1<<8)
#define SDMMC_RD_WT_SEL 			(1<<9)
#define SDMMC_RD_WT_SEL_READ		(0<<9)
#define SDMMC_RD_WT_SEL_WRITE		(1<<9)
#define SDMMC_S_M_SEL				(1<<10)
#define SDMMC_S_M_SEL_SIMPLE		(0<<10)
#define SDMMC_S_M_SEL_MULTIPLE		(1<<10)
#define SDMMC_AUTO_FLAG_EN			(1<<16)
#define SDMMC_SAMPLE_EDGE_SEL_FALL_EN   (1 << 17)

//SDMMC_STATUS
#define SDMMC_NOT_SDMMC_OVER		(1<<0)
#define SDMMC_BUSY				(1<<1)
#define SDMMC_DL_BUSY			(1<<2)
#define SDMMC_SUSPEND			(1<<3)
#define SDMMC_RSP_ERROR 			(1<<8)
#define SDMMC_NO_RSP_ERROR			(1<<9)
#define SDMMC_CRC_STATUS(n) 		(((n)&7)<<12)
#define SDMMC_DATA_ERROR(n) 		(((n)&0xFF)<<16)
#define SDMMC_DAT3_VAL			(1<<24)

//SDMMC_CMD_INDEX
#define SDMMC_COMMAND(n)			(((n)&0x3F)<<0)

//SDMMC_CMD_ARG
#define SDMMC_ARGUMENT(n)			(((n)&0xFFFFFFFF)<<0)

//SDMMC_RESP_INDEX
#define SDMMC_RESPONSE(n)			(((n)&0x3F)<<0)

//SDMMC_RESP_ARG3
#define SDMMC_ARGUMENT3(n)			(((n)&0xFFFFFFFF)<<0)

//SDMMC_RESP_ARG2
#define SDMMC_ARGUMENT2(n)			(((n)&0xFFFFFFFF)<<0)

//SDMMC_RESP_ARG1
#define SDMMC_ARGUMENT1(n)			(((n)&0xFFFFFFFF)<<0)

//SDMMC_RESP_ARG0
#define SDMMC_ARGUMENT0(n)			(((n)&0xFFFFFFFF)<<0)

//SDMMC_DATA_WIDTH
#define SDMMC_SDMMC_DATA_WIDTH(n)		(((n)&15)<<0)

//SDMMC_BLOCK_SIZE
#define SDMMC_SDMMC_BLOCK_SIZE(n)		(((n)&15)<<0)

//SDMMC_BLOCK_CNT
#define SDMMC_SDMMC_BLOCK_CNT(n)		(((n)&0xFFFF)<<0)

//SDMMC_INT_STATUS
#define SDMMC_NO_RSP_INT			(1<<0)
#define SDMMC_RSP_ERR_INT			(1<<1)
#define SDMMC_RD_ERR_INT			(1<<2)
#define SDMMC_WR_ERR_INT			(1<<3)
#define SDMMC_DAT_OVER_INT		(1<<4)
#define SDMMC_TXDMA_DONE_INT		(1<<5)
#define SDMMC_RXDMA_DONE_INT		(1<<6)
#define SDMMC_SDIO_INT                    (1<<7)
#define SDMMC_NO_RSP_SC 			(1<<8)
#define SDMMC_RSP_ERR_SC			(1<<9)
#define SDMMC_RD_ERR_SC 			(1<<10)
#define SDMMC_WR_ERR_SC 			(1<<11)
#define SDMMC_DAT_OVER_SC			(1<<12)
#define SDMMC_TXDMA_DONE_SC 		(1<<13)
#define SDMMC_RXDMA_DONE_SC 		(1<<14)

//SDMMC_INT_MASK
#define SDMMC_NO_RSP_MK 			(1<<0)
#define SDMMC_RSP_ERR_MK			(1<<1)
#define SDMMC_RD_ERR_MK 			(1<<2)
#define SDMMC_WR_ERR_MK 			(1<<3)
#define SDMMC_DAT_OVER_MK			(1<<4)
#define SDMMC_TXDMA_DONE_MK 		(1<<5)
#define SDMMC_RXDMA_DONE_MK 		(1<<6)
#define SDMMC_SDIO_INT_MK              (1<<7)

//SDMMC_INT_CLEAR
#define SDMMC_NO_RSP_CL 			(1<<0)
#define SDMMC_RSP_ERR_CL			(1<<1)
#define SDMMC_RD_ERR_CL 			(1<<2)
#define SDMMC_WR_ERR_CL 			(1<<3)
#define SDMMC_DAT_OVER_CL			(1<<4)
#define SDMMC_TXDMA_DONE_CL 		(1<<5)
#define SDMMC_RXDMA_DONE_CL 		(1<<6)

//SDMMC_TRANS_SPEED
#define SDMMC_TRANS_SPEED(n)		(((n)&0xFF)<<0)

//SDMMC_MCLK_ADJUST
#define SDMMC_MCLK_ADJUST(n)		(((n)&15)<<0)
#define SDMMC_CLK_INV			(1<<4)
#define SDMMC_CLK_DISA			(1<<5)

typedef union
{
	u32 reg;
	struct
	{
		u32 operationNotOver	 	:1;
		/// Module is busy ?
		u32 busy		 	:1;
		u32 dataLineBusy	 	:1;
		/// '1' means the controller will not perform the new command when SDMMC_SENDCMD= '1'.
		u32 suspend 		 	:1;
		u32 			 	:4;
		u32 responseCrcError	 	:1;
		/// 1 as long as no reponse to a command has been received.
		u32 noResponseReceived	:1;
		u32 			 	:2;
		/// CRC check for SD/MMC write operation
		/// "101" transmission error
		/// "010" transmission right
		/// "111" flash programming error
		u32 crcStatus			:3;
		u32 				:1;
		/// 8 bits data CRC check, "00000000" means no data error,
		/// "00000001" means DATA0 CRC check error, "10000000" means
		/// DATA7 CRC check error, each bit match one data line.
		u32 dataError			:8;
	} fields;
} HAL_SDMMC_OP_STATUS_T;

typedef enum
{
	HAL_SDMMC_DIRECTION_READ,
	HAL_SDMMC_DIRECTION_WRITE,
	HAL_SDMMC_DIRECTION_QTY
} HAL_SDMMC_DIRECTION_T;

typedef enum
{
	HAL_SDMMC_DATA_BUS_WIDTH_1 = 0x0,
	HAL_SDMMC_DATA_BUS_WIDTH_4 = 0x2
} HAL_SDMMC_DATA_BUS_WIDTH_T;

#endif /* __RDA_MMC_H */

