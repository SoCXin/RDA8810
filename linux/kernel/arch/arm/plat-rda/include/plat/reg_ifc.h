#ifndef __RDA_REG_IFC_H
#define __RDA_REG_IFC_H

#if (defined CONFIG_ARCH_RDA8810) || (defined CONFIG_ARCH_RDA8810E)
#include "reg_ifc_v1.h"
#elif defined(CONFIG_ARCH_RDA8850E)
#include "reg_ifc_v2.h"
#elif (defined CONFIG_ARCH_RDA8810H)
#include "reg_ifc_v3.h"
#else
#error "undefined ifc Controller Version"
#endif

#endif /* _RDA_REG_FC_H_ */
