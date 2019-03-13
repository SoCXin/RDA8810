#ifndef _REG_NAND_H_
#define _REG_NAND_H_

#ifdef CONFIG_MTD_NAND_RDA_V1
#include "reg_nand_v1.h"
#elif defined(CONFIG_MTD_NAND_RDA_V2)
#include "reg_nand_v2.h"
#elif defined(CONFIG_MTD_NAND_RDA_V3)
#include "reg_nand_v3.h"
#else
#error "undefined RDA NAND Controller Version"
#endif

#define nand_assert(expr) \
	if(!(expr)) { \
	printk( "Nand assertion failed! %s,%s,%s,line=%d\n",\
	#expr,__FILE__,__func__,__LINE__); \
	BUG(); \
	}

#endif /* _REG_NAND_H_ */
