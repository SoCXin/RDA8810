#ifndef __IOMAP_H
#define __IOMAP_H

#include <common.h>

#ifdef CONFIG_MACH_RDAARM926EJS
#include "iomap_rdaarm926ejs.h"
#elif defined(CONFIG_MACH_RDA8810)
#include "iomap_rda8810.h"
#elif defined(CONFIG_MACH_RDA8810E)
#include "iomap_rda8810e.h"
#elif defined(CONFIG_MACH_RDA8820)
#include "iomap_rda8820.h"
#elif defined(CONFIG_MACH_RDA8850)
#include "iomap_rda8850.h"
#elif defined(CONFIG_MACH_RDA8850E)
#include "iomap_rda8850e.h"
#elif defined(CONFIG_MACH_RDA8810H)
#include "iomap_rda8810h.h"
#else
#error "No MACH defined"
#endif

#endif // __IOMAP_H

