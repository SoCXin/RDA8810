#ifndef __RDA_REG_IFC_V1_H
#define __RDA_REG_IFC_V1_H

#include <mach/hardware.h>

// =============================================================================
//  MACROS
// =============================================================================
#define SYS_IFC_ADDR_ALIGN		(0)
#define SYS_IFC_TC_LEN			(23)

#ifdef CONFIG_ARCH_RDA8810
#define SYS_IFC_STD_CHAN_NB		(7)
#else
#define SYS_IFC_STD_CHAN_NB		(11)      //(SYS_IFC_NB_STD_CHANNEL)

#endif /* CONFIG_ARCH_RDA8810*/

#ifdef CONFIG_MMC_RDA_IFC_SG
/* Number of sg channels */
#define SYS_IFC_SG_CHAN_NUM		(3)
/* Number of items per a sg channel*/
#define SYS_IFC_SG_MAX			(8)
#else
#define SYS_IFC_SG_CHAN_NUM		(1)
#define SYS_IFC_SG_MAX			(1)
#endif /* CONFIG_MMC_RDA_IFC_SG */

#define SYS_IFC_RFSPI_CHAN		(1)

typedef volatile struct {
	REG32	start_addr;	/* 0x00000018 + slot * 4 */
	REG32	tc;			/* 0x0000001C + slot * 4 */
} HWP_SYS_SG_T;

typedef volatile struct
{
    REG32                          get_ch;                       //0x00000000
    REG32                          dma_status;                   //0x00000004
    REG32                          debug_status;                 //0x00000008
    REG32 Reserved_0000000C;                    //0x0000000C
    struct
    {
        REG32                      control;                      //0x00000010
        REG32                      status;                       //0x00000014
        HWP_SYS_SG_T	sg_table[SYS_IFC_SG_MAX];
    } std_ch[SYS_IFC_STD_CHAN_NB];
    // REG32 Reserved_00000080;                 //0x00000080
    REG32                          ch_rfspi_control;             //0x00000080
    REG32                          ch_rfspi_status;              //0x00000084
    REG32                          ch_rfspi_start_addr;          //0x00000088
    REG32                          ch_rfspi_end_addr;            //0x0000008C
    REG32                          ch_rfspi_tc;                  //0x00000090
} HWP_SYS_IFC_T;

//get_ch
#define SYS_IFC_CH_TO_USE(n)        (((n)&15)<<0)
#define SYS_IFC_CH_TO_USE_MASK      (15<<0)
#define SYS_IFC_CH_TO_USE_SHIFT     (0)

//dma_status
#define SYS_IFC_CH_ENABLE(n)        (((n)&0xFF)<<0)
#define SYS_IFC_CH_BUSY(n)          (((n)&0x7F)<<16)

//debug_status
#define SYS_IFC_DBG_STATUS          (1<<0)

//control
#define SYS_IFC_ENABLE              (1<<0)
#define SYS_IFC_DISABLE             (1<<1)
#define SYS_IFC_CH_RD_HW_EXCH       (1<<2)
#define SYS_IFC_CH_WR_HW_EXCH       (1<<3)
#define SYS_IFC_AUTODISABLE         (1<<4)
#define SYS_IFC_SIZE(n)             (((n)&3)<<5)
#define SYS_IFC_SIZE_MASK           (3<<5)
#define SYS_IFC_SIZE_SHIFT          (5)
#define SYS_IFC_SIZE_BYTE           (0<<5)
#define SYS_IFC_SIZE_HALF_WORD      (1<<5)
#define SYS_IFC_SIZE_WORD           (2<<5)
#define SYS_IFC_REQ_SRC(n)          (((n)&31)<<8)
#define SYS_IFC_REQ_SRC_MASK        (31<<8)
#define SYS_IFC_REQ_SRC_SHIFT       (8)
#define SYS_IFC_FLUSH               (1<<16)

#ifdef CONFIG_ARCH_RDA8810
#define SYS_IFC_SG_NUM(n)	(0)
#else
#define SYS_IFC_SG_NUM(n)	(((n) & 0x7) << 17)
#endif /* CONFIG_ARCH_RDA8810 */

//status
//#define SYS_IFC_ENABLE            (1<<0)
#define SYS_IFC_FIFO_EMPTY          (1<<4)

#ifdef CONFIG_ARCH_RDA8810
//start_addr
#define SYS_IFC_START_ADDR(n)       (((n)&0x3FFFFFF)<<0)
#else
#define SYS_IFC_START_ADDR(n)
#endif /* CONFIG_ARCH_RDA8810 */

//tc
#define SYS_IFC_TC(n)               (((n)&0x7FFFFF)<<0)

//ch_rfspi_control
//#define SYS_IFC_ENABLE            (1<<0)
//#define SYS_IFC_DISABLE           (1<<1)

//ch_rfspi_status
//#define SYS_IFC_ENABLE            (1<<0)
//#define SYS_IFC_FIFO_EMPTY        (1<<4)
#define SYS_IFC_FIFO_LEVEL(n)       (((n)&31)<<8)

//ch_rfspi_start_addr
#define SYS_IFC_START_AHB_ADDR(n)   (((n)&0x3FFFFFF)<<0)

//ch_rfspi_end_addr
#define SYS_IFC_END_AHB_ADDR(n)     (((n)&0x3FFFFFF)<<0)

//ch_rfspi_tc
//#define SYS_IFC_TC(n)             (((n)&0x3FFF)<<0)

#endif /* __RDA_REG_IFC_H */
