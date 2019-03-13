#ifndef _SYS_DDR3_H_
#define _SYS_DDR3_H_

#include "clock_config.h"

/*
 * defination of ddr flags
 */
/* [Debug] DLL off mode, 0 is dll on mode */
#define DDR_FLAGS_DLLOFF               0x8000
/* [Debug] LowSpeed mode*/
#define DDR_FLAGS_RESERVE              0x4000
/* DDR LowPower mode */
#define DDR_FLAGS_LOWPWR               0x2000
/* ODT: 000--Null 001--60 010--120 011--40 100--20 101--30 */
#define DDR_FLAGS_ODT(n)               (((n)<<0)&0x0007)
#define DDR_FLAGS_ODT_SHIFT            0
#define DDR_FLAGS_ODT_MASK             0x0007
/* RON: 00--40 01--30 */
#define DDR_FLAGS_RON(n)               (((n)<<3)&0x0018)
#define DDR_FLAGS_RON_SHIFT            3
#define DDR_FLAGS_RON_MASK             0x0018

/*
 * defination of ddr parameters
 */
#define DDR_PARA_CHIP_BITS(n)          (((n)<<0)&0x00000003)
#define DDR_PARA_CHIP_BITS_SHIFT       0
#define DDR_PARA_CHIP_BITS_MASK        0x00000003
/* memory width 0--8bit 1--16bit 2--32bit */
#define DDR_PARA_MEM_BITS(n)           (((n)<<2)&0x0000000c)
#define DDR_PARA_MEM_BITS_SHIFT        2
#define DDR_PARA_MEM_BITS_MASK         0x0000000c
/* 2--4banks 3--8banks */
#define DDR_PARA_BANK_BITS(n)          (((n)<<4)&0x000000f0)
#define DDR_PARA_BANK_BITS_SHIFT       4
#define DDR_PARA_BANK_BITS_MASK        0x000000f0
/* 2--13rbs 3--14rbs 4--15rbs 5--16rbs */
#define DDR_PARA_ROW_BITS(n)           (((n)<<8)&0x00000f00)
#define DDR_PARA_ROW_BITS_SHIFT        8
#define DDR_PARA_ROW_BITS_MASK         0x00000f00
/* 0--8cbs 1--9cbs 2--10cbs 3--11cbs 4--12cbs */
#define DDR_PARA_COL_BITS(n)           (((n)<<12)&0x0000f000)
#define DDR_PARA_COL_BITS_SHIFT        12
#define DDR_PARA_COL_BITS_MASK         0x0000f000

int ddr_init(UINT16 flags, UINT32 para);

#endif /* _SYS_DDR3_H_ */

