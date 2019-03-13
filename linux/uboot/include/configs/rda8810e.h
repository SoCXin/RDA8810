#ifndef __CONFIG_H
#define __CONFIG_H

#include <configs/rda_config_defaults.h>

/*
 * SoC Configuration
 */
#define CONFIG_MACH_RDA8810E
/* #define CONFIG_RDA_FPGA */

/*
 * Flash & Environment
 */
/* #define CONFIG_NAND_RDA_DMA */
#define CONFIG_NAND_RDA_V2		/* V2 for rda8810e, rda8820 */
#define CONFIG_SYS_NAND_MAX_CHIPS	1

#define NAND_IO_RECONFIG_WORKROUND

#ifdef _TGT_AP_HW_TEST_ENABLE
/* force to run hardware test when pdl2 is running */
//#define CONFIG_PDL_FORCE_HW_TEST
/* test list */
#define CONFIG_VPU_TEST
#endif

#endif /* __CONFIG_H */
