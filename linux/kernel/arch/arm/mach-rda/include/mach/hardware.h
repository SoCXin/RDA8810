#ifndef __ASM_ARCH_HARDWARE_H
#define __ASM_ARCH_HARDWARE_H

#include <asm/sizes.h>
#include <mach/iomap.h>

#define RDA_TTY_BASE        (RDA_UART3_BASE)
#define RDA_TTY_PHYS        (RDA_UART3_PHYS)
#define RDA_TTY_STATUS      (0x04)
#define RDA_TTY_TXRX        (0x08)

#ifndef __ASSEMBLY__
typedef volatile unsigned int REG32;
#endif

/* to extract bitfield from register value */
#define GET_BITFIELD(dword, bitfield) (((dword) & (bitfield ## _MASK)) >> (bitfield ## _SHIFT)) 

/* macro to get at IO space when running virtually */
#define IO_ADDRESS(x) (x)

#endif /* __ASM_ARCH_HARDWARE_H */
