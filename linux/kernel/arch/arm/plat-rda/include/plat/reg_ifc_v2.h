#ifndef __RDA_REG_IFC_V2_H
#define __RDA_REG_IFC_V2_H

#include <mach/hardware.h>

// =============================================================================
//  MACROS
// =============================================================================
#define SYS_IFC_STD_CHAN_NB		(8)      //(SYS_IFC_NB_STD_CHANNEL)
/* Number of scatter/gather channels */
#define SYS_IFC_SG_CHAN_NUM		(3)
/* Number of items per sg channel */
#define SYS_IFC_SG_MAX			(8)

typedef volatile struct {
	REG32	start_addr;	/* 0x00000018 + slot * 4 */
	REG32	tc;			/* 0x0000001C + slot * 4 */
} HWP_SYS_SG_T;

/*Notice: Just the former 3 channels support scatter gather funciton.Define all the 
    channels the same struct to keep the struct simple and easy to use. So you cannot 
    use the rese five channels when using scatter function. */
typedef volatile struct
{
	REG32			get_ch;                           //0x00000000
	REG32			dma_status;                   //0x00000004
	REG32			debug_status;                //0x00000008
	REG32			reserved_0000000C;     //0x0000000C
	struct
	{
		REG32			control;                      //0x00000010
		REG32			status;                       //0x00000014
		HWP_SYS_SG_T	sg_table[SYS_IFC_SG_MAX];
		REG32			reserve[14];
	} std_ch[SYS_IFC_STD_CHAN_NB];
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
#define SYS_IFC_SG_NUM(n)	(((n) & 0x7) << 17)


//status
//#define SYS_IFC_ENABLE            (1<<0)
#define SYS_IFC_FIFO_EMPTY          (1<<4)

#define SYS_IFC_START_ADDR(n)       (((n)&0xFFFFFFFF)<<0)


//tc
#define SYS_IFC_TC(n)               (((n)&0x7FFFFF)<<0)

//ch_rfspi_control
//#define SYS_IFC_ENABLE            (1<<0)
//#define SYS_IFC_DISABLE           (1<<1)

//ch_rfspi_status
//#define SYS_IFC_ENABLE            (1<<0)
//#define SYS_IFC_FIFO_EMPTY        (1<<4)
#define SYS_IFC_FIFO_LEVEL(n)       (((n)&31)<<8)
#endif /* __RDA_REG_IFC_H */
