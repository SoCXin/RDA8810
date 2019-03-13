#include <common.h>

#ifdef CONFIG_NAND_RDA_V1
#include "rda_nand_v1.c"
#elif defined(CONFIG_NAND_RDA_V2)
#include "rda_nand_v2.c"
#elif defined(CONFIG_NAND_RDA_V3)
#include "rda_nand_v3.c"
#else
#error "undefined RDA NAND Controller Version"
#endif

