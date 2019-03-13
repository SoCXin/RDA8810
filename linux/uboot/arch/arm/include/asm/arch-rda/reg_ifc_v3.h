#ifndef _REG_IFC_H_
#define _REG_IFC_H_

#include <asm/arch/hardware.h>
#include <asm/arch/rda_iomap.h>

// =============================================================================
//  MACROS
// =============================================================================
#define SYS_IFC_STD_CHAN_NB                      (8)      //(SYS_IFC_NB_STD_CHANNEL)
#define SYS_IFC_STD_CHAN_NB_SCATTER              (3)

// =============================================================================
//  TYPES
// =============================================================================

// ============================================================================
// SYS_IFC_T
// -----------------------------------------------------------------------------
/// 
// =============================================================================
typedef volatile struct
{
    REG32                          get_ch;                       //0x00000000
    REG32                          dma_status;                   //0x00000004
    REG32                          debug_status;                 //0x00000008
    REG32 Reserved_0000000C;                    //0x0000000C
    union 
    {
    struct
    {
        REG32                      control;                      //0x00000010
        REG32                      status;                       //0x00000014
        REG32                      start_addr1;                  //0x00000018
        REG32                      tc1;                          //0x0000001C
        REG32                      start_addr2;                  //0x00000020
        REG32                      tc2;                          //0x00000024
        REG32                      start_addr3;                  //0x00000028
        REG32                      tc3;                          //0x0000002C
        REG32                      start_addr4;                  //0x00000030
        REG32                      tc4;                          //0x00000034
        REG32                      start_addr5;                  //0x00000038
        REG32                      tc5;                          //0x0000003C
        REG32                      start_addr6;                  //0x00000040
        REG32                      tc6;                          //0x00000044
        REG32                      start_addr7;                  //0x00000048
        REG32                      tc7;                          //0x0000004C
        REG32                      start_addr8;                  //0x00000050
        REG32                      tc8;                          //0x00000054
        REG32 Reserved_00000048[14];            //0x00000048
    } std_ch_scatter[SYS_IFC_STD_CHAN_NB_SCATTER];
    struct
    {
        REG32                      control;                      //0x00000190
        REG32                      status;                       //0x00000194
        REG32                      start_addr;                   //0x00000198
        REG32                      tc;                           //0x0000019C
        REG32 Reserved_00000010[28];            //0x00000010
    } std_ch[SYS_IFC_STD_CHAN_NB];
    };
} HWP_SYS_IFC_T;

#define hwp_sysIfc                  ((HWP_SYS_IFC_T*)(RDA_IFC_BASE))


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
#define SYS_IFC_REQ_SRC_SYS_ID_TX_SCI1 (0<<8)
#define SYS_IFC_REQ_SRC_SYS_ID_RX_SCI1 (1<<8)
#define SYS_IFC_REQ_SRC_SYS_ID_TX_SCI2 (2<<8)
#define SYS_IFC_REQ_SRC_SYS_ID_RX_SCI2 (3<<8)
#define SYS_IFC_REQ_SRC_SYS_ID_TX_SCI3 (4<<8)
#define SYS_IFC_REQ_SRC_SYS_ID_RX_SCI3 (5<<8)
#define SYS_IFC_REQ_SRC_SYS_ID_TX_SPI1 (6<<8)
#define SYS_IFC_REQ_SRC_SYS_ID_RX_SPI1 (7<<8)
#define SYS_IFC_REQ_SRC_SYS_ID_TX_SPI2 (8<<8)
#define SYS_IFC_REQ_SRC_SYS_ID_RX_SPI2 (9<<8)
#define SYS_IFC_REQ_SRC_SYS_ID_TX_I2C (10<<8)
#define SYS_IFC_REQ_SRC_SYS_ID_RX_I2C (11<<8)
#define SYS_IFC_REQ_SRC_SYS_ID_TX_DEBUG_UART (12<<8)
#define SYS_IFC_REQ_SRC_SYS_ID_RX_DEBUG_UART (13<<8)
#define SYS_IFC_FLUSH             (1<<16)
#define SYS_IFC_ADDR_CNT(n)       (((n)&7)<<17)

//status
//#define SYS_IFC_ENABLE          (1<<0)
#define SYS_IFC_FIFO_EMPTY        (1<<4)

//start_addr1
#define SYS_IFC_START_ADDR1(n)    (((n)&0xFFFFFFFF)<<0)

//tc1
#define SYS_IFC_TC1(n)            (((n)&0x7FFFFF)<<0)

//start_addr2
#define SYS_IFC_START_ADDR2(n)    (((n)&0xFFFFFFFF)<<0)

//tc2
#define SYS_IFC_TC2(n)            (((n)&0x7FFFFF)<<0)

//start_addr3
#define SYS_IFC_START_ADDR3(n)    (((n)&0xFFFFFFFF)<<0)

//tc3
#define SYS_IFC_TC3(n)            (((n)&0x7FFFFF)<<0)

//start_addr4
#define SYS_IFC_START_ADDR4(n)    (((n)&0xFFFFFFFF)<<0)

//tc4
#define SYS_IFC_TC4(n)            (((n)&0x7FFFFF)<<0)

//start_addr5
#define SYS_IFC_START_ADDR5(n)    (((n)&0xFFFFFFFF)<<0)

//tc5
#define SYS_IFC_TC5(n)            (((n)&0x7FFFFF)<<0)

//start_addr6
#define SYS_IFC_START_ADDR6(n)    (((n)&0xFFFFFFFF)<<0)

//tc6
#define SYS_IFC_TC6(n)            (((n)&0x7FFFFF)<<0)

//start_addr7
#define SYS_IFC_START_ADDR7(n)    (((n)&0xFFFFFFFF)<<0)

//tc7
#define SYS_IFC_TC7(n)            (((n)&0x7FFFFF)<<0)

//start_addr8
#define SYS_IFC_START_ADDR8(n)    (((n)&0xFFFFFFFF)<<0)

//tc8
#define SYS_IFC_TC8(n)            (((n)&0x7FFFFF)<<0)

//control
//#define SYS_IFC_ENABLE          (1<<0)
//#define SYS_IFC_DISABLE         (1<<1)
//#define SYS_IFC_CH_RD_HW_EXCH   (1<<2)
//#define SYS_IFC_CH_WR_HW_EXCH   (1<<3)
//#define SYS_IFC_AUTODISABLE     (1<<4)
//#define SYS_IFC_SIZE(n)         (((n)&3)<<5)
//#define SYS_IFC_SIZE_MASK       (3<<5)
//#define SYS_IFC_SIZE_SHIFT      (5)
//#define SYS_IFC_SIZE_BYTE       (0<<5)
//#define SYS_IFC_SIZE_HALF_WORD  (1<<5)
//#define SYS_IFC_SIZE_WORD       (2<<5)
//#define SYS_IFC_REQ_SRC(n)      (((n)&31)<<8)
//#define SYS_IFC_REQ_SRC_MASK    (31<<8)
//#define SYS_IFC_REQ_SRC_SHIFT   (8)
//#define SYS_IFC_REQ_SRC_SYS_ID_TX_SCI1 (0<<8)
//#define SYS_IFC_REQ_SRC_SYS_ID_RX_SCI1 (1<<8)
//#define SYS_IFC_REQ_SRC_SYS_ID_TX_SCI2 (2<<8)
//#define SYS_IFC_REQ_SRC_SYS_ID_RX_SCI2 (3<<8)
//#define SYS_IFC_REQ_SRC_SYS_ID_TX_SCI3 (4<<8)
//#define SYS_IFC_REQ_SRC_SYS_ID_RX_SCI3 (5<<8)
//#define SYS_IFC_REQ_SRC_SYS_ID_TX_SPI1 (6<<8)
//#define SYS_IFC_REQ_SRC_SYS_ID_RX_SPI1 (7<<8)
//#define SYS_IFC_REQ_SRC_SYS_ID_TX_SPI2 (8<<8)
//#define SYS_IFC_REQ_SRC_SYS_ID_RX_SPI2 (9<<8)
//#define SYS_IFC_REQ_SRC_SYS_ID_TX_I2C (10<<8)
//#define SYS_IFC_REQ_SRC_SYS_ID_RX_I2C (11<<8)
//#define SYS_IFC_REQ_SRC_SYS_ID_TX_DEBUG_UART (12<<8)
//#define SYS_IFC_REQ_SRC_SYS_ID_RX_DEBUG_UART (13<<8)
//#define SYS_IFC_FLUSH           (1<<16)

//status
//#define SYS_IFC_ENABLE          (1<<0)
//#define SYS_IFC_FIFO_EMPTY      (1<<4)

//start_addr
#define SYS_IFC_START_ADDR(n)     (((n)&0xFFFFFFFF)<<0)

//tc
#define SYS_IFC_TC(n)             (((n)&0x7FFFFF)<<0)




#endif /* _REG_IFC_H_ */

