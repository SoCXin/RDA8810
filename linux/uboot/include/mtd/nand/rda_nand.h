#ifndef __RDA_NAND_H__
#define __RDA_NAND_H__

#include <asm/arch-rda/rda_sys.h>

typedef enum
{
    NAND_MSG_LEN_1K        = 0,
    NAND_MSG_LEN_2K        = 1,
    NAND_MSG_LEN_4K        = 2,
    NAND_MSG_LEN_1152      = 3
} NAND_ECC_MSG_LEN_TYPE;

typedef enum
{
    NAND_OOB_LEN_32         = 0,
    NAND_OOB_LEN_16         = 1
} NAND_OOB_LEN_TYPE;

typedef enum
{
    NAND_BUS_WIDTH_8BIT     = 0,
    NAND_BUS_WIDTH_16BIT    = 1
} NAND_BUS_WIDTH_TYPE;

typedef union
{
    struct{
        unsigned short pagesize:2;
        unsigned short eccmode:4;
        unsigned short eccmsglen:2;
        unsigned short ooblen:1;
    };
    unsigned short reg;
}EFUSE_NAND_INFO_T;

typedef enum
{
    NAND_PARAMETER_BY_GPIO         = 0,
    NAND_PARAMETER_BY_NAND        = 1
} NAND_PARAMETER_MODE_TYPE;

/*************************************************************/

#define RDA_EFUSE_INDEX_NAND_INFO        (4)

/*
  Fuse layout:low 9 bit
	O.MM.VVVV.NN
  NN: NAND_PAGE_TYPE
  VVVV:NAND_ECC_TYPE
  MM:NAND_ECC_MSG_LEN
  O:OOB LEN
*/
/* define bit for rda nand info config */
#define RDA_NAND_PAGESIZE_INDEX(n)         (((n)&0x3)<<0)
#define RDA_NAND_GET_PAGESIZE(r)     (((r)>>0)&0x3)
#define RDA_NAND_ECCMODE(n)         (((n)&0xF)<<2)
#define RDA_NAND_GET_ECCMODE(r)     (((r)>>2)&0xF)
#define RDA_NAND_ECCMSGLEN(n)         (((n)&0x3)<<6)
#define RDA_NAND_GET_ECCMSGLEN(r)     (((r)>>6)&0x3)
#define RDA_NAND_OOBLEN(n)         (((n)&0x1)<<8)
#define RDA_NAND_GET_OOBLEN(r)     (((r)>>8)&0x1)
/**************************************************************/

struct rda_nand_info {
	int type;
	int hard_ecc_hec;
	int ecc_mode;
	int bus_width_16;
	int page_num_per_block;

	int page_size;
	int erase_size;
	int page_shift;
	int oob_size;

	int vir_page_size;
	int vir_erase_size;
	int vir_page_shift;
	int vir_oob_size;

	int cmd;
	int col_addr;
	int page_addr;
	int read_ptr;
	u32 byte_buf[4];	// for read_id, status, etc
	int index;
	int write_ptr;
	void __iomem *nand_data_phys;
	u8 dma_ch;
	unsigned long master_clk;
	unsigned long clk;
	int cmd_flag;

	u8 spl_adjust_ratio;
	/*specially for nand dump command to read first 16K of nand v3.*/
	u8 dump_debug_flag;
	/*8810:8K/16k setting*/
	/*nand_use_type = 1: 8K used as 2 4k or 3K*/
	/*nand_use_type = 3: 16K used as 4 4k or 3K*/
	/*nand_use_type = 0: 2K or  4k*/
	u8  nand_use_type;
	int logic_page_size;
	int logic_oob_size;
	int logic_operate_time;
	/* rda8850e use*/
	int page_total_num;
	unsigned short flash_oob_off;
	unsigned short message_len;
	u32	bch_data;
	u32	bch_oob;
	u16 ecc_mode_bak;
	u16 max_eccmode_support;
	u8 type_bak;
	NAND_PARAMETER_MODE_TYPE parameter_mode_select;
	u32 spl_logic_pageaddr;
	u32 boot_logic_end_pageaddr;
};
#endif

