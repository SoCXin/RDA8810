#ifdef CONFIG_MACH_RDA8810
#include "reg_md_sysctrl_rda8810.h"
#elif defined(CONFIG_MACH_RDA8810E)
#include "reg_md_sysctrl_rda8810e.h"
#elif defined(CONFIG_MACH_RDA8820)
#include "reg_md_sysctrl_rda8820.h"
#elif defined(CONFIG_MACH_RDA8850)
#include "reg_md_sysctrl_rda8850.h"
#elif defined(CONFIG_MACH_RDA8850E)
#include "reg_md_sysctrl_rda8850e.h"
#elif defined(CONFIG_MACH_RDA8810H)
#include "reg_md_sysctrl_rda8810h.h"
#else
#error "unknown MACH"
#endif

