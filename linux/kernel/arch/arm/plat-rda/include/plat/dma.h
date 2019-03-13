/***************************************************************
*
* Copyright(C) RDA Micro Company.,2012
* All Rights Reserved. Confidential
*
****************************************************************
*
* Project: RDA
* File Name: arch/arm/plat-rda/include/plat/dma.h
*
* Author: Jason Tao
* Creation Date: 2012-10-15
*
*****************************************************************
*
* Definition of registers of DMA
*
*****************************************************************
*/


#ifndef __DMA_H__
#define __DMA_H__

#include <linux/dma-direction.h>

#define RDA_DMA_CHAN_REG        0x00

#define RDA_DMA_STATUS_REG      0x04
#define RDA_DMA_STA_INT         BIT(1)
#define RDA_DMA_STA_TRANS_DONE	BIT(2)

#define RDA_DMA_CTL_REG         0x08
#define RDA_DMA_CTL_EN          BIT(0)
#define RDA_DMA_CTL_INT_MASK    BIT(1)
#define RDA_DMA_CTL_INT_CLE     BIT(2)
#define RDA_DMA_CTL_SRC_SEL     BIT(24)
#define RDA_DMA_CTL_DST_SEL     BIT(25)

#define RDA_DMA_SRC_REG         0x0C
#define RDA_DMA_DST_REG         0x10
#define RDA_DMA_XFER_SIZE_REG   0x18

// FROM RDA8850E, scatter-gather DMA is supported
//channel 10
#define DMA_LLI_CNT_REG   48

#define DMA_LLI1_SRC_REG  64
#define DMA_LLI1_DST_REG  68
#define DMA_LLI1_SIZE_REG 72

#define DMA_LLI2_SRC_REG  76
#define DMA_LLI2_DST_REG  80
#define DMA_LLI2_SIZE_REG 84

#define DMA_LLI3_SRC_REG  88
#define DMA_LLI3_DST_REG  92
#define DMA_LLI3_SIZE_REG 96

#define DMA_LLI4_SRC_REG  100
#define DMA_LLI4_DST_REG  104
#define DMA_LLI4_SIZE_REG 108

#define DMA_LLI5_SRC_REG  112
#define DMA_LLI5_DST_REG  116
#define DMA_LLI5_SIZE_REG 120

#define DMA_LLI6_SRC_REG  124
#define DMA_LLI6_DST_REG  128
#define DMA_LLI6_SIZE_REG 132

#define DMA_LLI7_SRC_REG  136
#define DMA_LLI7_DST_REG  140
#define DMA_LLI7_SIZE_REG 144

#define DMA_LLI8_SRC_REG  148
#define DMA_LLI8_DST_REG  152
#define DMA_LLI8_SIZE_REG 156

#define DMA_LLI9_SRC_REG  160
#define DMA_LLI9_DST_REG  164
#define DMA_LLI9_SIZE_REG 168

#define DMA_LLI10_SRC_REG  172
#define DMA_LLI10_DST_REG  176
#define DMA_LLI10_SIZE_REG 180

#define DMA_LLI11_SRC_REG  184
#define DMA_LLI11_DST_REG  188
#define DMA_LLI11_SIZE_REG 192

#define DMA_LLI12_SRC_REG  196
#define DMA_LLI12_DST_REG  200
#define DMA_LLI12_SIZE_REG 204

#define DMA_LLI13_SRC_REG  208
#define DMA_LLI13_DST_REG  212
#define DMA_LLI13_SIZE_REG 216

#define DMA_LLI14_SRC_REG  220
#define DMA_LLI14_DST_REG  224
#define DMA_LLI14_SIZE_REG 228

#define DMA_LLI15_SRC_REG  232
#define DMA_LLI15_DST_REG  236
#define DMA_LLI15_SIZE_REG 240

/* In general, data will be transisted via AXI bus. */
#define RDA_DMA_NOR_MODE        (0x00000000)
/* Fast write mode : bit25 is for control writing via sram port. */
#define RDA_DMA_FW_MODE         RDA_DMA_CTL_DST_SEL
/* Fast read mode : bit24 is for control reading via sram port. */
#define RDA_DMA_FR_MODE         RDA_DMA_CTL_SRC_SEL
#define RDA_DMA_MODE_MASK       (RDA_DMA_CTL_SRC_SEL | RDA_DMA_CTL_DST_SEL)

#ifdef CONFIG_ARCH_RDA8850E
#define RDA_SCATTER_GATHER_DMA
#endif

#ifdef RDA_SCATTER_GATHER_DMA
#define RDA_SCATTER_GATHER_DMA_LIST 0x10
struct rda_dma_trans_addr {
	dma_addr_t src_addr;
	dma_addr_t dst_addr;
	unsigned long xfer_size;
};

struct rda_dma_chan_list_params {
	struct rda_dma_trans_addr addrs[RDA_SCATTER_GATHER_DMA_LIST];
	u32 nr_dma_lists;
	unsigned long dma_mode;
	unsigned int enable_int;
};

void rda_dma_map_list_params(struct rda_dma_chan_list_params *params,
		enum dma_data_direction dir);

void rda_dma_unmap_list_params(struct rda_dma_chan_list_params *params,
		enum dma_data_direction dir);

int rda_set_dma_list_params(u8 ch, struct rda_dma_chan_list_params *param);
#endif

struct rda_dma_chan_params {
	dma_addr_t src_addr;
	dma_addr_t dst_addr;
	unsigned long xfer_size;
	unsigned long dma_mode;
	unsigned int enable_int;
};
int rda_set_dma_params(u8 ch, struct rda_dma_chan_params *params);
void rda_start_dma(u8 ch);
void rda_stop_dma(u8 ch);
void rda_poll_dma(u8 ch);
int rda_request_dma(int dev_id, const char *dev_name,
	void (*callback)(u8 ch, void *data), void *data, u8 *dma_ch_out);
void rda_free_dma(u8 ch);

#endif /* __DMA_H__ */
