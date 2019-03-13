/***************************************************************
*
* Copyright(C) RDA Micro Company.,2012
* All Rights Reserved. Confidential
*
****************************************************************
*
* Project: RDA8810
* File Name: drivers/dma/rda_dma.c
*
* Author: Jason Tao
* Creation Date: 2012-11-15
*
*****************************************************************
*
* Implementation of functions of DMA
*
*****************************************************************
*/

#include <common.h>
#include <asm/arch/rda_iomap.h>
#include <asm/arch/dma.h>
#include <asm/io.h>
#include <asm/errno.h>
#include <linux/compiler.h>

/* 0x20820000 */
static void __iomem *dma_base = (void __iomem *)RDA_DMA_BASE;

static inline void __dma_write(u32 val, u32 offset, u8 ch)
{
	__raw_writel(val, dma_base + offset);
}

static inline u32 __dma_read(u32 offset, u8 ch)
{
	u32 val;

	val = __raw_readl(dma_base + offset);
	return val;
}

int rda_set_dma_params(u8 ch, struct rda_dma_chan_params *params)
{
	dma_addr_t src;
	dma_addr_t dst;
	u32 size;
	u32 mode;

	if (!params) {
		printf("rda8810 dma : Invalid parameter!\n");
		return -EINVAL;
	}

	src = params->src_addr;
	dst = params->dst_addr;
	size = params->xfer_size;
	mode = (params->dma_mode & RDA_DMA_MODE_MASK);
#if 0
	printf("[rda_dma] : src = 0x%08x, dst = 0x%08x, size = 0x%x\n", src,
	       dst, size);
#endif /* #if 0 */
	switch (mode) {
	case RDA_DMA_FW_MODE:
		/*
		 * DMA transits data by a word(4 bytes).
		 * The mask is only for NANDFC that supports no more than 2048 words(8192 bytes),
		 * but our address's unit is byte. So we have to mask 0x1FFF.
		 */
		dst &= 0x1FFF;
		break;

	case RDA_DMA_FR_MODE:
		src &= 0x1FFF;
		break;

	case RDA_DMA_NOR_MODE:
		/* Nothing to do */
		break;

	default:
		printf("[rda8810 dma] : Invalid mode of dma!\r\n");
		return -EINVAL;
	}

	/* Fill src address */
	__dma_write(src, RDA_DMA_SRC_REG, ch);
	/* Fill dst address */
	__dma_write(dst, RDA_DMA_DST_REG, ch);
	/* Fill size */
	__dma_write(size, RDA_DMA_XFER_SIZE_REG, ch);

	/* Set ctl flag */
	__dma_write(mode, RDA_DMA_CTL_REG, ch);

	return 0;
}

void rda_start_dma(u8 ch)
{
	u32 reg = 0;

	reg = __dma_read(RDA_DMA_CTL_REG, ch);

	reg |= RDA_DMA_CTL_EN;
	__dma_write(reg, RDA_DMA_CTL_REG, ch);

	return;
}

void rda_stop_dma(u8 ch)
{
	u32 reg = RDA_DMA_CTL_INT_CLE;

	__dma_write(reg, RDA_DMA_CTL_REG, ch);
	return;
}

void rda_poll_dma(u8 ch)
{
	u32 reg = 0;

	while (!((reg = __dma_read(RDA_DMA_STATUS_REG, ch)) & RDA_DMA_STA_INT)) {
		asm("nop");
	}

	return;
}

int rda_request_dma(u8 * dma_ch_out)
{
	/* Clear interrupt flag and disble dma. */
	rda_stop_dma(0);
	*dma_ch_out = 0;

	return 0;
}

void rda_free_dma(u8 ch)
{
	return;
}
