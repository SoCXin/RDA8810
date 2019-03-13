#ifndef __ASM_ARCH_IRQS_H
#define __ASM_ARCH_IRQS_H

#ifdef	CONFIG_ARCH_RDA8810
#include "irqs-rda8810.h"
#elif defined(CONFIG_ARCH_RDA8810E)
#include "irqs-rda8810e.h"
#elif defined(CONFIG_ARCH_RDA8820)
#include "irqs-rda8820.h"
#elif defined(CONFIG_ARCH_RDA8850E)
#include "irqs-rda8850e.h"
#elif defined(CONFIG_ARCH_RDA8810H)
#include "irqs-rda8810h.h"
#else
#error "Unknown ARCH for IRQs define"
#endif

#endif /* __ASM_ARCH_IOMAP_H */
