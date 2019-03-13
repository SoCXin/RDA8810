#ifndef _REG_NAND_V2_H_
#define _REG_NAND_V2_H_

#include <mach/hardware.h>

#define NANDFC_DATA_OFFSET      (0x0000)
#define NANDFC_REG_OFFSET       (0x3000)

#define NANDFC_REG_DCMD_ADDR    (0x00)
//#define NANDFC_REG_OP_START     (0x04)
//#define NANDFC_REG_CMD_PTR	  (0x08)
#define NANDFC_REG_COL_ADDR     (0x0c)
#define NANDFC_REG_CONFIG_A     (0x10)
#define NANDFC_REG_CONFIG_B     (0x14)
#define NANDFC_REG_BUF_CTRL     (0x18)
#define NANDFC_REG_BUSY_FLAG    (0x1c)
#define NANDFC_REG_INT_MASK     (0x20)
#define NANDFC_REG_INT_STAT     (0x24)
#define NANDFC_REG_IDCODE_A     (0x28)
#define NANDFC_REG_IDCODE_B     (0x2c)
//#define NANDFC_REG_DMA_REQ	  (0x30)
//#define NANDFC_REG_CMD_DEF_A    (0x34)
//#define NANDFC_REG_CMD_DEF_B    (0x38)
#define NANDFC_REG_OP_STATUS    (0x3c)
#define NANDFC_REG_IDTPYE       (0x40)
#define NANDFC_REG_DELAY        (0x54)

// NANDFC_REG_DCMD_ADDR
#define NANDFC_DCMD(n)	        (((n)&0xFF)<<24)
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
#define NANDFC_INT_ECC_ERR	(1<<0)
#define NANDFC_INT_PROG_ERR     (1<<1)
#define NANDFC_INT_ERASE_ERR    (1<<2)
#define NANDFC_INT_TIMEOUT	(1<<3)
#define NANDFC_INT_DONE	        (1<<4)
#define NANDFC_INT_CRC_FAIL     (1<<5)
#define NANDFC_INT_ECC_ERR_0    (1<<6)
#define NANDFC_INT_ECC_ERR_1    (1<<7)
#define NANDFC_INT_ECC_ERR_2    (1<<8)
#define NANDFC_INT_ECC_CRC_0    (1<<9)
#define NANDFC_INT_ECC_CRC_1    (1<<10)
#define NANDFC_INT_ECC_CRC_2    (1<<11)
#define NANDFC_INT_STAT_IDLE    (1<<21)
#define NANDFC_INT_ERR_ALL      (0x01EF)  /* CRC bits do not work anymore */
#define NANDFC_INT_CLR_MASK     (0x0FDE)

typedef enum
{
    NAND_TYPE_2K	= 0,   	/* support 3 byte Row Addresses only */
    NAND_TYPE_4K	= 1,
    NAND_TYPE_8K	= 2,
    NAND_TYPE_INVALID	= 0xFF,
} NAND_FLASH_TYPE;

typedef enum
{
    NAND_ECC_2K24BIT	= 0,   // support 2K nand only
    NAND_ECC_RESERVE	= 1,
    NAND_ECC_1K96BIT	= 2,   // legacy, 4K use as 3K or 8K use as 6K
    NAND_ECC_1K64BIT	= 3,   // 8K only, use as 8K, need 7xx spare
    NAND_ECC_1K56BIT	= 4,   // 8K only, use as 8K, need 6xx spare
    NAND_ECC_1K40BIT	= 5,   // 8K only, use as 8K, need 5xx spare
    NAND_ECC_1K24BIT	= 6,   // 4K/8K, 4K need 224 spare
    NAND_ECC_INVALID	= 0xFF,
} NAND_ECC_TYPE;

#endif /* _REG_NAND_V2_H_ */
