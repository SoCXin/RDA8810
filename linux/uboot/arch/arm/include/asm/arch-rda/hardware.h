#ifndef __ASM_ARCH_HARDWARE_H
#define __ASM_ARCH_HARDWARE_H

#include <config.h>
#include <asm/sizes.h>

#ifndef __ASSEMBLY__ 
typedef volatile unsigned int REG32;
typedef unsigned char               BOOL;

#define TRUE                        (1==1)
#define FALSE                       (1==0)
#endif

/* to extract bitfield from register value */
#define GET_BITFIELD(dword, bitfield) (((dword) & (bitfield ## _MASK)) >> (bitfield ## _SHIFT)) 
#define SET_BITFIELD(dword, bitfield, value) (((dword) & ~(bitfield ## _MASK)) | (bitfield(value)))

#endif /* __ASM_ARCH_HARDWARE_H */
