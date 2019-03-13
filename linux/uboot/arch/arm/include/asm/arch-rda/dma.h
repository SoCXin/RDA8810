/***************************************************************
*
* Copyright(C) RDA Micro Company.,2012
* All Rights Reserved. Confidential
*
****************************************************************
*
* Project: RDA8810
* File Name: arch/arm/include/asm/dma.h
*
* Author: Jason Tao
* Creation Date: 2012-11-15
*
*****************************************************************
*
* Definition of registers of DMA
*
*****************************************************************
*/


#ifndef __DMA_H__
#define __DMA_H__

#include <asm/types.h>

#define BIT(x)  (1UL << (x))

#define RDA_DMA_CHAN_REG    0x00

#define RDA_DMA_STATUS_REG  0x04
#define RDA_DMA_STA_INT      BIT(2)

#define RDA_DMA_CTL_REG     0x08
#define RDA_DMA_CTL_EN    BIT(0)
#define RDA_DMA_CTL_INT_MASK    BIT(1)
#define RDA_DMA_CTL_INT_CLE     BIT(2)
#define RDA_DMA_CTL_SRC_SEL     BIT(24)
#define RDA_DMA_CTL_DST_SEL    BIT(25)

#define RDA_DMA_SRC_REG     0x0C
#define RDA_DMA_DST_REG     0x10
#define RDA_DMA_XFER_SIZE_REG   0x18

/* In general, data will be transisted via AXI bus. */
#define RDA_DMA_NOR_MODE	 (0x00000000)
/* Fast write mode : bit25 is for control writing via sram port. */
#define RDA_DMA_FW_MODE     RDA_DMA_CTL_DST_SEL
/* Fast read mode : bit24 is for control reading via sram port. */
#define RDA_DMA_FR_MODE      RDA_DMA_CTL_SRC_SEL
#define RDA_DMA_MODE_MASK       (RDA_DMA_CTL_SRC_SEL | RDA_DMA_CTL_DST_SEL)

struct rda_dma_chan_params {
    u32 src_addr;
    u32 dst_addr;
    u32 xfer_size;
    u32 dma_mode;
};

int rda_set_dma_params(u8 ch, struct rda_dma_chan_params *params);

void rda_start_dma(u8 ch);

void rda_stop_dma(u8 ch);

void rda_poll_dma(u8 ch);

int rda_request_dma(u8 *dma_ch_out);

void rda_free_dma(u8 ch);

#endif /* __DMA_H__ */

