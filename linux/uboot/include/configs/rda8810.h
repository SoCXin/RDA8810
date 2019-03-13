#ifndef __CONFIG_H
#define __CONFIG_H

#include <configs/rda_config_defaults.h>

/*
 * SoC Configuration
 */
#define CONFIG_MACH_RDA8810

#define CONFIG_MACH_TYPE (5002)

/*
 * Flash & Environment
 */
/* #define CONFIG_NAND_RDA_DMA */
#define CONFIG_NAND_RDA_V1		/* V1 for rda8810, rda8850 */
#define CONFIG_SYS_NAND_MAX_CHIPS	1

// Enable RDA signed bootloader:
#define CONFIG_SIGNATURE_CHECK_IMAGE

#define CONFIG_SYS_CACHELINE_SIZE		32

#endif /* __CONFIG_H */
