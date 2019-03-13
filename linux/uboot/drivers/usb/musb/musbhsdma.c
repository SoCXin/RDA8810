#include <common.h>
#include "musbhsdma.h"
#include "rda.h"


static u32 start_addr = 0;
static u32 dma_req_len = 0;
static int complete = 0;

int configure_dma_channel(u32 epnum, u16 packet_sz, u8 mode,
				u32 dma_addr, u32 len, int tx)
{
	void *mbase = (void *)MUSB_BASE;
	u8 bchannel = 0;
	u16 csr = 0;

	complete = 0;

	/* check alignment for 32(0x20) */
	if ((dma_addr % 32) != 0) {
		//printf("%s: dma addr %#x is not aligned.\n", __func__, dma_addr);
		return -1;
	}
	if ((len % 32) != 0) {
		//printf("%s: dma xfer size %#x not aligned.\n", __func__, len);
		return -1;
	}

	if (mode) {
		csr |= 1 << MUSB_HSDMA_MODE1_SHIFT;
		BUG_ON(len < packet_sz);
	}
	csr |= MUSB_HSDMA_BURSTMODE_INCR16
				<< MUSB_HSDMA_BURSTMODE_SHIFT;

	csr |= (epnum << MUSB_HSDMA_ENDPOINT_SHIFT)
		| (1 << MUSB_HSDMA_ENABLE_SHIFT)
		| (1 << MUSB_HSDMA_IRQENABLE_SHIFT)
		| (tx ? (1 << MUSB_HSDMA_TRANSMIT_SHIFT)
				: 0);

	start_addr = dma_addr;
	dma_req_len = len;
	/* address/count */
	musb_write_hsdma_addr(mbase, bchannel, dma_addr);
	musb_write_hsdma_count(mbase, bchannel, len);

	if(tx)
		flush_dcache_range(dma_addr, dma_addr+len);
	else
		invalidate_dcache_range(dma_addr, dma_addr+len);

	/* control (this should start things) */
	musb_writew(mbase,
		MUSB_HSDMA_CHANNEL_OFFSET(bchannel, MUSB_HSDMA_CONTROL),
		csr);

	return 0;
}

static int dma_controller_irq(void)
{
	void *mbase = (void *)MUSB_BASE;
	u8 bchannel = 0;
	u8 int_hsdma;

	u32 addr;
	u16 csr;
	u32 actual_len;


	int_hsdma = musb_readb(mbase, MUSB_HSDMA_INTR);

	if (int_hsdma & (1 << bchannel)) {

		csr = musb_readw(mbase,
				MUSB_HSDMA_CHANNEL_OFFSET(bchannel,
					MUSB_HSDMA_CONTROL));

		if (csr & (1 << MUSB_HSDMA_BUSERROR_SHIFT)) {
			serial_printf("dma bus error\n");
		} else {
			addr = musb_read_hsdma_addr(mbase,
					bchannel);
			actual_len = addr
				- start_addr;
			/*
			   serial_printf("0x%x -> 0x%x (%zu / %d) %s\n",
			   start_addr,
			   addr, actual_len,
			   dma_req_len,
			   (actual_len
			   < dma_req_len) ?
			   "=> reconfig 0" : "=> complete");
			 */
			if (actual_len == dma_req_len)
				complete = 1;
		}
	}

	return 0;
}

int wait_dma_xfer_done(void)
{
	int timeout = 100000;

	while(!complete && timeout) {
		dma_controller_irq();
		timeout--;
	}
	if (timeout <= 0) {
		serial_printf("ERROR: dma xfer timeout\n");
		return -1;
	}

	return 0;
}
