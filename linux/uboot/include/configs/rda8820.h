#ifndef __CONFIG_H
#define __CONFIG_H

#include <configs/rda_config_defaults.h>

/*
 * SoC Configuration
 */
#define CONFIG_MACH_RDA8820
#define CONFIG_RDA_FPGA

/*
 * Flash & Environment
 */
/* #define CONFIG_NAND_RDA_DMA */
#define CONFIG_NAND_RDA_V2		/* V2 for rda8810e, rda8820 */
#define CONFIG_SYS_NAND_MAX_CHIPS	1

#endif /* __CONFIG_H */
