#ifndef __CONFIG_H
#define __CONFIG_H

#include <configs/rda_config_defaults.h>

/*
 * SoC Configuration
 */
#define CONFIG_MACH_RDA8850E
/*#define CONFIG_RDA_FPGA*/

/* Enable RDA signed bootloader:*/
/* #define CONFIG_SIGNATURE_CHECK_IMAGE */

/*
 * Flash & Environment
 */
/* #define CONFIG_NAND_RDA_DMA */
#define CONFIG_NAND_RDA_V3		/* V3 for rda8850e*/
#define CONFIG_SYS_NAND_MAX_CHIPS	4

#define CONFIG_SYS_CACHELINE_SIZE		64

#ifdef _TGT_AP_HW_TEST_ENABLE
#define CONFIG_VPU_TEST
/* #define CONFIG_DDR_TEST */
/* #define CONFIG_TIMER_TEST */
/* #define CONFIG_UART_TEST */
/* #define CONFIG_GIC_TEST */
/* #define CONFIG_CACHE_TEST */
/* #define CONFIG_MIPI_LOOP_TEST */
/* #define CONFIG_I2C_TEST */
#endif

#define CONFIG_USB_ACM_TWO_CHANS 1

#endif /* __CONFIG_H */
