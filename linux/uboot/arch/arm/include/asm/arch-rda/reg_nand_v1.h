#ifndef _REG_NAND_V1_H_
#define _REG_NAND_V1_H_

#include <asm/arch/hardware.h>
#include <asm/arch/rda_iomap.h>

#define REG_NANDFC_BASE     RDA_NAND_BASE

#define NANDFC_DATA_BUF         (REG_NANDFC_BASE + 0x0000)
#define NANDFC_OOB_BUF          (REG_NANDFC_BASE + 0x1000)

#if defined(CONFIG_MACH_RDA8810) || defined(CONFIG_MACH_RDA8850) || \
	defined(CONFIG_MACH_RDAARM926EJS)
#define NANDFC_REG_DCMD_ADDR    (REG_NANDFC_BASE + 0x2000)
//#define NANDFC_REG_OP_START     (REG_NANDFC_BASE + 0x2004)
//#define NANDFC_REG_CMD_PTR      (REG_NANDFC_BASE + 0x2008)
#define NANDFC_REG_COL_ADDR     (REG_NANDFC_BASE + 0x200c)
#define NANDFC_REG_CONFIG_A     (REG_NANDFC_BASE + 0x2010)
#define NANDFC_REG_CONFIG_B     (REG_NANDFC_BASE + 0x2014)
#define NANDFC_REG_BUF_CTRL     (REG_NANDFC_BASE + 0x2018)
#define NANDFC_REG_BUSY_FLAG    (REG_NANDFC_BASE + 0x201c)
#define NANDFC_REG_INT_MASK     (REG_NANDFC_BASE + 0x2020)
#define NANDFC_REG_INT_STAT     (REG_NANDFC_BASE + 0x2024)
#define NANDFC_REG_IDCODE_A     (REG_NANDFC_BASE + 0x2028)
#define NANDFC_REG_IDCODE_B     (REG_NANDFC_BASE + 0x202c)
//#define NANDFC_REG_DMA_REQ      (REG_NANDFC_BASE + 0x2030)
//#define NANDFC_REG_CMD_DEF_A    (REG_NANDFC_BASE + 0x2034)
#define NANDFC_REG_CMD_DEF_B    (REG_NANDFC_BASE + 0x2038)
#define NANDFC_REG_OP_STATUS    (REG_NANDFC_BASE + 0x203c)
#define NANDFC_REG_IDTPYE       (REG_NANDFC_BASE + 0x2040)
#define NANDFC_REG_DELAY        (REG_NANDFC_BASE + 0x2054)
#else
/* for 8810e, 8820, or later */
#define NANDFC_REG_DCMD_ADDR    (REG_NANDFC_BASE + 0x3000)
//#define NANDFC_REG_OP_START     (REG_NANDFC_BASE + 0x3004)
//#define NANDFC_REG_CMD_PTR      (REG_NANDFC_BASE + 0x3008)
#define NANDFC_REG_COL_ADDR     (REG_NANDFC_BASE + 0x300c)
#define NANDFC_REG_CONFIG_A     (REG_NANDFC_BASE + 0x3010)
#define NANDFC_REG_CONFIG_B     (REG_NANDFC_BASE + 0x3014)
#define NANDFC_REG_BUF_CTRL     (REG_NANDFC_BASE + 0x3018)
#define NANDFC_REG_BUSY_FLAG    (REG_NANDFC_BASE + 0x301c)
#define NANDFC_REG_INT_MASK     (REG_NANDFC_BASE + 0x3020)
#define NANDFC_REG_INT_STAT     (REG_NANDFC_BASE + 0x3024)
#define NANDFC_REG_IDCODE_A     (REG_NANDFC_BASE + 0x3028)
#define NANDFC_REG_IDCODE_B     (REG_NANDFC_BASE + 0x302c)
//#define NANDFC_REG_DMA_REQ      (REG_NANDFC_BASE + 0x3030)
//#define NANDFC_REG_CMD_DEF_A    (REG_NANDFC_BASE + 0x3034)
//#define NANDFC_REG_CMD_DEF_B    (REG_NANDFC_BASE + 0x3038)
#define NANDFC_REG_OP_STATUS    (REG_NANDFC_BASE + 0x303c)
#define NANDFC_REG_IDTPYE       (REG_NANDFC_BASE + 0x3040)
#define NANDFC_REG_DELAY        (REG_NANDFC_BASE + 0x3054)
#endif

// NANDFC_REG_DCMD_ADDR
#define NANDFC_DCMD(n)          (((n)&0xFF)<<24)
#define NANDFC_PAGE_ADDR(n)     (((n)&0x000FFFFF)<<0)

// NANDFC_REG_CONFIG_A
#define NANDFC_CYCLE(n)         (((n)&0x0F)<<0)
#define NANDFC_CHIP_SEL(n)      (((n)&0x0F)<<4)
#define NANDFC_TIMING(n)        (((n)&0xFF)<<8)
#define NANDFC_POLARITY_IO(n)   (((n)&0x01)<<16)
#define NANDFC_POLARITY_MEM(n)  (((n)&0x01)<<17)
#define NANDFC_WDITH_16BIT(n)   (((n)&0x01)<<18)

// NANDFC_REG_CONFIG_B
#define NANDFC_HWECC(n)         (((n)&0x01)<<7)
#define NANDFC_ECC_MODE(n)      (((n)&0x0F)<<0)

// NANDFC_REG_BUF_CTRL
#define NANDFC_BUF_DIRECT(n)    (((n)&0x01)<<2)

// NANDFC_REG_INT_STAT
#define NANDFC_INT_ECC_ERR      (1<<0)
#define NANDFC_INT_PROG_ERR     (1<<1)
#define NANDFC_INT_ERASE_ERR    (1<<2)
#define NANDFC_INT_TIMEOUT      (1<<3)
#define NANDFC_INT_DONE         (1<<4)
#define NANDFC_INT_CRC_FAIL     (1<<5)
#define NANDFC_INT_ECC_ERR_0    (1<<6)
#define NANDFC_INT_ECC_ERR_1    (1<<7)
#define NANDFC_INT_ECC_ERR_2    (1<<8)
#define NANDFC_INT_ECC_CRC_0    (1<<9)
#define NANDFC_INT_ECC_CRC_1    (1<<10)
#define NANDFC_INT_ECC_CRC_2    (1<<11)
#define NANDFC_INT_ERR_ALL      (0x0FEF)
#define NANDFC_INT_CLR_MASK     (0x0FDE)

typedef enum
{
    NAND_TYPE_512S         = 0,   // 512+16, 64Mb, 128Mb, 256Mb
    NAND_TYPE_512L         = 1,   // 512+16, 512Mb, 1Gb, 2Gb
    NAND_TYPE_2KS          = 2,   // 2048+64, 1Gb (2 byte Row addresses)
    NAND_TYPE_2KL          = 3,   // 2048+64, 2Gb, 4Gb, 8Gb, 16Gb (3 byte Row Addresses)
    NAND_TYPE_MLC2K        = 4,   // MLC, 2048+64
    NAND_TYPE_MLC4K        = 5,   // MLC, 4096+224, 16Gb, 32Gb
    NAND_TYPE_SLC4K        = 6,   // SLC, 4096+224, 16Gb, 32Gb
    NAND_TYPE_MLC8K        = 7,   // MLC, 8192+224, 16Gb, 32Gb
    NAND_TYPE_SLC8K        = 8,   // SLC, 8192+224, 16Gb, 32Gb
    NAND_TYPE_MLC16K        = 9,   // MLC,
    NAND_TYPE_SLC16K        = 10,   // SLC,
    NAND_TYPE_INVALID      = 0xff
} NAND_FLASH_TYPE;

#endif /* _REG_NAND_V1_H_ */

