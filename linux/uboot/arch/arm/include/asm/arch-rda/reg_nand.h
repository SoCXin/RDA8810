#ifndef _REG_NAND_H_
#define _REG_NAND_H_

#include <common.h>

#ifdef CONFIG_NAND_RDA_V1
#include "reg_nand_v1.h"
#elif defined(CONFIG_NAND_RDA_V2)
#include "reg_nand_v2.h"
#elif defined(CONFIG_NAND_RDA_V3)
#include "reg_nand_v3.h"
#else
#error "undefined RDA NAND Controller Version"
#endif

#endif /* _REG_NAND_H_ */

