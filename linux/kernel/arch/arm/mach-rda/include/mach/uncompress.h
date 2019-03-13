#ifndef __ASM_ARCH_UNCOMPRESS_H
#define __ASM_ARCH_UNCOMPRESS_H

#include <mach/hardware.h>

#define RDA_UART_STATUS (*(volatile unsigned int *)(RDA_TTY_PHYS + RDA_TTY_STATUS))
#define RDA_UART_RXTX   (*(volatile unsigned int *)(RDA_TTY_PHYS + RDA_TTY_TXRX))

/*
 * This does not append a newline
 */
static void putc(int c)
{
	while (!(RDA_UART_STATUS& 0x1F00))
		;
	RDA_UART_RXTX = (unsigned int)c;
}

static inline void flush(void)
{
}

/*
 * nothing to do
 */
#define arch_decomp_setup()

#define arch_decomp_wdog()

#endif
