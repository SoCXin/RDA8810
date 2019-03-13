#ifndef _REG_NAND_V1_H_
#define _REG_NAND_V1_H_

#include <mach/hardware.h>

#define NANDFC_DATA_OFFSET      (0x0000)
#define NANDFC_REG_OFFSET       (0x2000)

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

#define NANDFC_REG_CMD_DEF_B    (0x38)
/* bit [31:25] stores the flip bits of ECC of each HW ECC
 * if val == 127, ECC failure
 * otherwize, the real flipped bits of 1 ECC caculation
 * Hardware supports two mode of ECC
 * 1. 96bits in 1K   - this is called HEC
 * 2. 24bits in 2K
 * if method2 is used for 4K page, the real flipped bits can
 * be estimated as 2*value
 *
 * The above feaure is available when metal is bigger than 0xA.
 * (not including)
 * */

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
#define NANDFC_INT_ERR_ALL      (0x0FEF)
#define NANDFC_INT_CLR_MASK     (0x0FDE)

typedef enum
{
    NAND_TYPE_512S		= 0,   // 512+16, 64Mb, 128Mb, 256Mb
    NAND_TYPE_512L		= 1,   // 512+16, 512Mb, 1Gb, 2Gb
    NAND_TYPE_2KS		= 2,   // 2048+64, 1Gb (2 byte Row addresses)
    NAND_TYPE_2KL		= 3,   // 2048+64, 2Gb, 4Gb, 8Gb, 16Gb (3 byte Row Addresses)
    NAND_TYPE_MLC2K		= 4,   // MLC, 2048+64
    NAND_TYPE_MLC4K		= 5,   // MLC, 4096+224, 16Gb, 32Gb
    NAND_TYPE_SLC4K		= 6,   // SLC, 4096+224, 16Gb, 32Gb
    NAND_TYPE_MLC8K	 	= 7,   // MLC, 8192+??
    NAND_TYPE_SLC8K		= 8,   // SLC, 8192+224, 16Gb, 32Gb
    NAND_TYPE_MLC16K	= 9,   // MLC,
    NAND_TYPE_SLC16K	= 10,  // SLC,
    NAND_TYPE_INVALID		= 0xFF,
} NAND_FLASH_TYPE;

#endif /* _REG_NAND_V1_H_ */
