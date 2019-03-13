#ifndef __RDA_I2C_H
#define __RDA_I2C_H

#include <mach/hardware.h>

typedef volatile struct
{
    REG32                          CTRL;                         //0x00000000
    REG32                          STATUS;                       //0x00000004
    REG32                          TXRX_BUFFER;                  //0x00000008
    REG32                          CMD;                          //0x0000000C
    REG32                          IRQ_CLR;                      //0x00000010
    REG32                          CTRL1;                        //0x00000014
} HWP_I2C_MASTER_T;

//CTRL
#define I2C_MASTER_EN               (1<<0)
#define I2C_MASTER_CLEAR_FIFO               (1<<7)
#define I2C_MASTER_IRQ_MASK         (1<<8)
#define I2C_MASTER_TX_FIFO_THRESHOLD(n) (((n)&0x1F)<<1)
#define I2C_MASTER_TX_FIFO_THRESHOLD_MASK (0x1F<<1)
#define I2C_MASTER_RX_READ_NUM(n) (((n)&0x1F)<<9)
#define I2C_MASTER_RX_READ_NUM_MASK  (0x1F <<9)
#define I2C_MASTER_TIMEOUT_THRESHOLD(n)  (((n)&0x3) <<14)
#define I2C_MASTER_CLOCK_PRESCALE(n) (((n)&0xFFFF)<<16)
#define I2C_MASTER_CLOCK_PRESCALE_MASK (0xFFFF<<16)

//CTRL2
#define I2C_MASTER_DMA_MODE (1 << 0)
#define I2C_MASTER_TXFIFO_OVER_IRQ_MASK         (1<<1)
#define I2C_MASTER_TXFIFO_UNDER_IRQ_MASK         (1<<2)
#define I2C_MASTER_RXFIFO_OVER_IRQ_MASK         (1<<3)
#define I2C_MASTER_RXFIFO_UNDER_IRQ_MASK         (1<<4)
#define I2C_MASTER_TXFIFO_EMPTY_IRQ_MASK         (1<<5)
#define I2C_MASTER_RXFIFO_NUM_IRQ_MASK         (1<<6)
#define I2C_MASTER_TX_DMA_IRQ_MASK         (1<<7)
#define I2C_MASTER_RX_DMA_IRQ_MASK         (1<<8)
#define I2C_MASTER_TX_DMA_COUNTER(n)  (((n)&0x7FF)<< 9)
#define I2C_MASTER_TX_DMA_COUNTER_MASK (0x7FF<<9)
#define I2C_MASTER_RX_DMA_COUNTER(n)  (((n)&0x7FF)<< 20)
#define I2C_MASTER_RX_DMA_COUNTER_MASK (0x7FF<<20)

//STATUS
#define I2C_MASTER_IRQ_CAUSE        (1<<0)
#define I2C_MASTER_IRQ_TX_DMA        (1<<1)
#define I2C_MASTER_IRQ_RX_DMA        (1<<2)
#define I2C_MASTER_IRQ_STATUS       (1<<4)
#define I2C_MASTER_IRQ_TX_DMA_DONE        (1<<5)
#define I2C_MASTER_IRQ_RX_DMA_DONE        (1<<6)
#define I2C_MASTER_TIP              (1<<8)
#define I2C_MASTER_AL               (1<<12)
#define I2C_MASTER_BUSY             (1<<16)
#define I2C_MASTER_IRQ_RX_OVF       (1<<17)
#define I2C_MASTER_IRQ_RX_UDF       (1<<18)
#define I2C_MASTER_IRQ_TX_OVF       (1<<19)
#define I2C_MASTER_IRQ_TX_UDF       (1<<20)
//#define I2C_MASTER_RXACK            (1<<20)
#define I2C_MASTER_RX_FIFO_DATA_NUM(n) (((n)&0x1F)<<21)
#define I2C_MASTER_RX_FIFO_DATA_NUM_MASK (0x1F<<21)
#define I2C_MASTER_TX_FIFO_FREE_NUM(n) (((n)&0x1F)<<26)
#define I2C_MASTER_TX_FIFO_FREE_NUM_MASK (0x1F<<26)
#define I2C_MASTER_TIMEOUT        (1<<31)

//TXRX_BUFFER
#define I2C_MASTER_TX_DATA(n)       (((n)&0xFF)<<0)
#define I2C_MASTER_RX_DATA(n)       (((n)&0xFF)<<0)

//CMD
#define I2C_MASTER_ACK              (1<<0)
#define I2C_MASTER_RD               (1<<4)
#define I2C_MASTER_STO              (1<<8)
#define I2C_MASTER_FORCE_STO        (1<<9)
#define I2C_MASTER_WR               (1<<12)
#define I2C_MASTER_STA              (1<<16)

//IRQ_CLR
#define I2C_MASTER_IRQ_CLR          (1<<0)

#endif /* __RDA_I2C_H */
