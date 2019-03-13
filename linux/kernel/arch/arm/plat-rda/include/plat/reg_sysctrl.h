#ifndef _REG_SYSCTRL_H_
#define _REG_SYSCTRL_H_

#if defined CONFIG_ARCH_RDA8810
#include "reg_sysctrl_8810.h"
#elif defined CONFIG_ARCH_RDA8850E
#include "reg_sysctrl_8850e.h"
#elif defined CONFIG_ARCH_RDA8810H
#include "reg_sysctrl_8810h.h"
#endif /* CONFIG_ARCH_RDA8810 */

#endif /* _REG_SYSCTRL_H_ */
