#ifndef _REG_NAND_V3_H_
#define _REG_NAND_V3_H_

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
/* bits 24-30 stores ECC error bits
** if ecc failed, bit[30:24] is 0x7f
*/
#define NANDFC_REG_IDCODE_A     (0x28)
#define NANDFC_REG_IDCODE_B     (0x2c)
//#define NANDFC_REG_DMA_REQ	  (0x30)
//#define NANDFC_REG_CMD_DEF_A    (0x34)
//#define NANDFC_REG_CMD_DEF_B    (0x38)
#define NANDFC_REG_OP_STATUS    (0x3c)
#define NANDFC_REG_IDTPYE       (0x40)
#define NANDFC_REG_TIMING_MODE  (0x48)
#define NANDFC_REG_SYNC_DELAY  (0x4c)
#define NANDFC_REG_IRBN_COUNT  (0x54)
#define NANDFC_REG_PAGE_PARA  (0x5c)
#define NANDFC_REG_COMMAND_FIFO (0x70)
#define NANDFC_REG_COMMAND_EXEC_STATUS (0x74)
#define NANDFC_REG_COMMAND_STATUS0		(0x78)
#define NANDFC_REG_COMMAND_STATUS1		(0x7c)
#define NANDFC_REG_SCRAMBLE_START_DATA	(0x80)
#define NANDFC_REG_BRICK_FSM_TIME0		(0x84)
#define NANDFC_REG_BRICK_FSM_TIME1		(0x88)
#define NANDFC_REG_BRICK_FSM_TIME2		(0x8c)
#define NANDFC_REG_BRICK_FIFO_WRITE_POINTER (0x90)
#define NADFC_REG_BCH_DATA (0x94)
#define NADFC_REG_BCH_OOB	(0x98)
#define NADFC_REG_MESSAGE_OOB_SIZE	(0x9c)

// NANDFC_REG_DCMD_ADDR
#define NANDFC_DCMD(n)	        (((n)&0xFF)<<24)
#define NANDFC_PAGE_ADDR(n)     (((n)&0x00FFFFFF)<<0)

// NANDFC_REG_CONFIG_A
#define NANDFC_CYCLE(n)         ((((n)&0x0F)<<0) | ((((n) >> 4)&0x1)<<30))
#define NANDFC_CHIP_SEL(n)      (((n)&0x0F)<<4)
#define NANDFC_TIMING(n)        (((n)&0xFF)<<8)
#define NANDFC_POLARITY_IO(n)   (((n)&0x01)<<16)
#define NANDFC_POLARITY_MEM(n)  (((n)&0x01)<<17)
#define NANDFC_CMDFULL_STA(n) (((n)&0x01)<<31)
#define NANDFC_BRICK_COMMAND(n) (((n)&0x01)<<25)
#define NANDFC_WDITH_16BIT(n)   (((n)&0x01)<<18)
#define NANDFC_DMA_ENABLE(n)     (((n)&0x01)<<22)
#define NANDFC_SCRAMBLE_ENABLE(n)   (((n)&0x01)<<24)

//NADFC_REG_BCH_DATA
#define NANDFC_BCH_KK_DATA(n) (((n)&0x1FFF) << 0)
#define NANDFC_BCH_NN_DATA(n) (((n)&0x1FFF) << 16)

//NADFC_REG_BCH_DATA
#define NANDFC_BCH_KK_OOB(n) (((n)&0x1FFF) << 0)
#define NANDFC_BCH_NN_OOB(n) (((n)&0x1FFF) << 16)

//NADFC_REG_MESSAGE_OOB_SIZE
#define NANDFC_OOB_SIZE(n) (((n)&0xFFF) << 0)
#define NANDFC_MESSAGE_SIZE(n) (((n)&0x3FFF) << 16)

//NANDFC_REG_BRICK_FIFO_WRITE_POINTER
#define NANDFC_BRICK_DATA(n) (((n)&0xFF) << 24)
#define NANDFC_BRICK_R_WIDTH(n) (((n)&0x1) << 18)
#define NANDFC_BRICK_W_WIDTH(n) (((n)&0x1) << 19)
#define NANDFC_BRICK_DATA_NUM(n) (((n)&0x3FFF) << 4)
#define NANDFC_BRICK_NEXT_ACT(n) (((n)&0xF) << 0)

typedef enum
{
    NAND_ECC_24BIT           = 0,
    NAND_ECC_96BIT_WITHOUT_CRC  = 1,
    NAND_ECC_96BIT_WITH_CRC             = 2,
    NAND_ECC_64BIT           = 3,
    NAND_ECC_56BIT           = 4,
    NAND_ECC_40BIT           = 5,
    NAND_ECC_24BIT_1       = 6,
    NAND_ECC_48BIT           = 7,
    NAND_ECC_32BIT           = 8,
    NAND_ECC_16BIT           = 9,
    NAND_ECC_8BIT             = 10,
    NAND_ECC_72BIT           = 11,
    NAND_ECC_80BIT           = 12,
    NAND_ECC_88BIT           = 13,
    NAND_ECC_INVALID           = 0xFF,
} NAND_BCHMODE_TYPE;

#define NANDFC_SCRAMBLE(n)         (((n)&0x01)<<24)

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
#define NANDFC_INT_COMMAND_FIFO_EMPTY  (1<<6)
#define NANDFC_INT_ECC_ERR_0    (1<<7)
#define NANDFC_INT_ECC_ERR_1    (1<<8)
#define NANDFC_INT_ECC_ERR_1    (1<<8)
#define NANDFC_INT_ECC_ERR_2    (1<<10)
#define NANDFC_INT_ECC_ERR_3    (1<<10)
#define NANDFC_INT_ECC_ERR_4    (1<<11)
#define NANDFC_INT_ECC_ERR_5    (1<<12)
#define NANDFC_INT_ECC_ERR_6    (1<<13)
#define NANDFC_INT_ECC_ERR_7    (1<<14)
#define NANDFC_INT_ECC_CRC_0    (1<<15)
#define NANDFC_INT_ECC_CRC_1    (1<<16)
#define NANDFC_INT_ECC_CRC_2    (1<<17)
#define NANDFC_INT_ECC_CRC_3    (1<<18)
#define NANDFC_INT_ECC_CRC_4    (1<<19)
#define NANDFC_INT_ECC_CRC_5    (1<<20)
#define NANDFC_INT_ECC_CRC_6    (1<<21)
#define NANDFC_COMMAND_FIFO_OVERFLOW (1 << 22)
#define NANDFC_INT_ERR_ALL      (0x3FFF87)
#define NANDFC_INT_CLR_MASK     (0x3FFF9F)

typedef enum
{
    NAND_TYPE_2K	= 0,   	/* support 3 byte Row Addresses only */
    NAND_TYPE_4K	= 1,
    NAND_TYPE_8K	= 2,
    NAND_TYPE_16K	= 3,
    NAND_TYPE_3K	= 4,
    NAND_TYPE_7K	= 5,
    NAND_TYPE_14K	= 6,
    NAND_TYPE_INVALID	= 0xFF
} NAND_FLASH_TYPE;
//NANDFC_REG_PAGE_PARA
#define NANDFC_PAGE_SIZE(n)         (((n)&0x07FFF)<<16)
#define NANDFC_PACKAGE_NUM(n)      (((n)&0x0F)<<0)
#endif /* _REG_NAND_V3_H_ */
