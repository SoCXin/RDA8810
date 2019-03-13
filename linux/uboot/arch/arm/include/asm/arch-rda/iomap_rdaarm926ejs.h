#ifndef __IOMAP_RDAARM926EJS_H
#define __IOMAP_RDAARM926EJS_H

#include <asm/sizes.h>

#define RDA_INTC_BASE         0x01AF0000
#define RDA_INTC_SIZE         SZ_4K

#define RDA_TIMER_BASE        0x01A02000
#define RDA_TIMER_SIZE        SZ_4K

#define RDA_GPIO_BASE         0x01A03000
#define RDA_GPIO_SIZE         SZ_4K

#define RDA_KEYPAD_BASE       0x01A05000
#define RDA_KEYPAD_SIZE       SZ_4K

#define RDA_I2C1_BASE         0x01A07000
#define RDA_I2C1_SIZE         SZ_4K

#define RDA_I2C2_BASE         0x01A22000
#define RDA_I2C2_SIZE         SZ_4K

#define RDA_I2C3_BASE         0x01A23000
#define RDA_I2C3_SIZE         SZ_4K

#define RDA_IFC_BASE          0x01A09000
#define RDA_IFC_SIZE          SZ_4K

#define RDA_UART1_BASE        0x01A15000
#define RDA_UART1_SIZE        SZ_4K

#define RDA_UART2_BASE        0x01A16000
#define RDA_UART2_SIZE        SZ_4K

#define RDA_SDMMC1_BASE       0x01A17000
#define RDA_SDMMC1_SIZE       SZ_4K

#define RDA_GOUDA_BASE        0x01A21000
#define RDA_GOUDA_SIZE        SZ_4K

#define RDA_NAND_BASE         0x01A26000
#define RDA_NAND_SIZE         SZ_16K

#define RDA_USB_BASE          0x01A80000
#define RDA_USB_SIZE          SZ_64K

#endif /* __IOMAP_RDAARM926EJS_H */
