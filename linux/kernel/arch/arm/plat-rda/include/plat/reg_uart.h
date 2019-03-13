#ifndef __RDA_UART_H
#define __RDA_UART_H

#include <mach/hardware.h>
#include <mach/iomap.h>

#define UART_RX_FIFO_SIZE                        (32)
#define UART_TX_FIFO_SIZE                        (16)
#define NB_RX_FIFO_BITS                          (5)
#define NB_TX_FIFO_BITS                          (4)

typedef volatile struct
{
    REG32                          ctrl;                         //0x00000000
    REG32                          status;                       //0x00000004
    REG32                          rxtx_buffer;                  //0x00000008
    REG32                          irq_mask;                     //0x0000000C
    REG32                          irq_cause;                    //0x00000010
    REG32                          triggers;                     //0x00000014
    REG32                          CMD_Set;                      //0x00000018
    REG32                          CMD_Clr;                      //0x0000001C
} HWP_UART_T;

//ctrl
#define UART_ENABLE                 (1<<0)
#define UART_ENABLE_DISABLE         (0<<0)
#define UART_ENABLE_ENABLE          (1<<0)
#define UART_DATA_BITS              (1<<1)
#define UART_DATA_BITS_7_BITS       (0<<1)
#define UART_DATA_BITS_8_BITS       (1<<1)
#define UART_TX_STOP_BITS           (1<<2)
#define UART_TX_STOP_BITS_1_BIT     (0<<2)
#define UART_TX_STOP_BITS_2_BITS    (1<<2)
#define UART_PARITY_ENABLE          (1<<3)
#define UART_PARITY_ENABLE_NO       (0<<3)
#define UART_PARITY_ENABLE_YES      (1<<3)
#define UART_PARITY_SELECT(n)       (((n)&3)<<4)
#define UART_PARITY_SELECT_ODD      (0<<4)
#define UART_PARITY_SELECT_EVEN     (1<<4)
#define UART_PARITY_SELECT_SPACE    (2<<4)
#define UART_PARITY_SELECT_MARK     (3<<4)
#define UART_DIVISOR_MODE           (1<<20)
#define UART_IRDA_ENABLE            (1<<21)
#define UART_DMA_MODE               (1<<22)
#define UART_DMA_MODE_DISABLE       (0<<22)
#define UART_DMA_MODE_ENABLE        (1<<22)
#define UART_AUTO_FLOW_CONTROL      (1<<23)
#define UART_AUTO_FLOW_CONTROL_ENABLE (1<<23)
#define UART_AUTO_FLOW_CONTROL_DISABLE (0<<23)
#define UART_LOOP_BACK_MODE         (1<<24)
#define UART_RX_LOCK_ERR            (1<<25)
#define UART_RX_BREAK_LENGTH(n)     (((n)&15)<<28)

//status
#define UART_RX_FIFO_LEVEL(n)       (((n)&0x7F)<<0)
#define UART_RX_FIFO_LEVEL_MASK     (0x7F<<0)
#define UART_RX_FIFO_LEVEL_SHIFT    (0)
#define UART_TX_FIFO_SPACE(n)       (((n)&31)<<8)
#define UART_TX_FIFO_SPACE_MASK     (31<<8)
#define UART_TX_FIFO_SPACE_SHIFT    (8)
#define UART_TX_ACTIVE              (1<<14)
#define UART_RX_ACTIVE              (1<<15)
#define UART_RX_OVERFLOW_ERR        (1<<16)
#define UART_TX_OVERFLOW_ERR        (1<<17)
#define UART_RX_PARITY_ERR          (1<<18)
#define UART_RX_FRAMING_ERR         (1<<19)
#define UART_RX_BREAK_INT           (1<<20)
#define UART_DCTS                   (1<<24)
#define UART_CTS                    (1<<25)
#define UART_DTR                    (1<<28)
#define UART_CLK_ENABLED            (1<<31)

//rxtx_buffer
#define UART_RX_DATA(n)             (((n)&0xFF)<<0)
#define UART_TX_DATA(n)             (((n)&0xFF)<<0)

//irq_mask
#define UART_TX_MODEM_STATUS        (1<<0)
#define UART_RX_DATA_AVAILABLE      (1<<1)
#define UART_TX_DATA_NEEDED         (1<<2)
#define UART_RX_TIMEOUT             (1<<3)
#define UART_RX_LINE_ERR            (1<<4)
#define UART_TX_DMA_DONE            (1<<5)
#define UART_RX_DMA_DONE            (1<<6)
#define UART_RX_DMA_TIMEOUT         (1<<7)
#define UART_DTR_RISE               (1<<8)
#define UART_DTR_FALL               (1<<9)

//irq_cause
//#define UART_TX_MODEM_STATUS      (1<<0)
//#define UART_RX_DATA_AVAILABLE    (1<<1)
//#define UART_TX_DATA_NEEDED       (1<<2)
//#define UART_RX_TIMEOUT           (1<<3)
//#define UART_RX_LINE_ERR          (1<<4)
//#define UART_TX_DMA_DONE          (1<<5)
//#define UART_RX_DMA_DONE          (1<<6)
//#define UART_RX_DMA_TIMEOUT       (1<<7)
//#define UART_DTR_RISE             (1<<8)
//#define UART_DTR_FALL             (1<<9)
#define UART_TX_MODEM_STATUS_U      (1<<16)
#define UART_RX_DATA_AVAILABLE_U    (1<<17)
#define UART_TX_DATA_NEEDED_U       (1<<18)
#define UART_RX_TIMEOUT_U           (1<<19)
#define UART_RX_LINE_ERR_U          (1<<20)
#define UART_TX_DMA_DONE_U          (1<<21)
#define UART_RX_DMA_DONE_U          (1<<22)
#define UART_RX_DMA_TIMEOUT_U       (1<<23)
#define UART_DTR_RISE_U             (1<<24)
#define UART_DTR_FALL_U             (1<<25)

//triggers
#define UART_RX_TRIGGER(n)          (((n)&63)<<0)
#define UART_TX_TRIGGER(n)          (((n)&15)<<8)
#define UART_AFC_LEVEL(n)           (((n)&63)<<16)

//CMD_Set
#define UART_RI                     (1<<0)
#define UART_DCD                    (1<<1)
#define UART_DSR                    (1<<2)
#define UART_TX_BREAK_CONTROL       (1<<3)
#define UART_TX_FINISH_N_WAIT       (1<<4)
#define UART_RTS                    (1<<5)
#define UART_RX_FIFO_RESET          (1<<6)
#define UART_TX_FIFO_RESET          (1<<7)

//CMD_Clr
//#define UART_RI                   (1<<0)
//#define UART_DCD                  (1<<1)
//#define UART_DSR                  (1<<2)
//#define UART_TX_BREAK_CONTROL     (1<<3)
//#define UART_TX_FINISH_N_WAIT     (1<<4)
//#define UART_RTS                  (1<<5)

#define CHIP_STD_UART_QTY 2

// =============================================================================
//  MACROS
// =============================================================================

// ============================================================================
// HAL_UART_DATA_BITS_T
// -----------------------------------------------------------------------------
/// UART data length
// =============================================================================
typedef enum
{
/// Data is 7 bits
    HAL_UART_7_DATA_BITS                        = 0x00000000,
/// Data is 8 bits
    HAL_UART_8_DATA_BITS                        = 0x00000001,
    HAL_UART_DATA_BITS_QTY                      = 0x00000002
} HAL_UART_DATA_BITS_T;


// ============================================================================
// HAL_UART_STOP_BITS_QTY_T
// -----------------------------------------------------------------------------
/// Number of stop bits
// =============================================================================
typedef enum
{
/// There is 1 stop bit
    HAL_UART_1_STOP_BIT                         = 0x00000000,
/// There are 2 stop bits
    HAL_UART_2_STOP_BITS                        = 0x00000001,
    HAL_UART_STOP_BITS_QTY                      = 0x00000002
} HAL_UART_STOP_BITS_QTY_T;


// ============================================================================
// HAL_UART_PARITY_CFG_T
// -----------------------------------------------------------------------------
/// Data parity control selection If enabled, a parity check can be performed
// =============================================================================
typedef enum
{
/// No parity check
    HAL_UART_NO_PARITY                          = 0x00000000,
/// Parity check is odd
    HAL_UART_ODD_PARITY                         = 0x00000001,
/// Parity check is even
    HAL_UART_EVEN_PARITY                        = 0x00000002,
/// Parity check is always 0 (space)
    HAL_UART_SPACE_PARITY                       = 0x00000003,
/// Parity check is always 1 (mark)
    HAL_UART_MARK_PARITY                        = 0x00000004,
    HAL_UART_PARITY_QTY                         = 0x00000005
} HAL_UART_PARITY_CFG_T;


// ============================================================================
// HAL_UART_RX_TRIGGER_CFG_T
// -----------------------------------------------------------------------------
/// Reception FIFO trigger (or treshold) level The Uarts can be configured to generate
/// an interrupt when the reception FIFO is above a configurable threshold (Rx FIFO
/// trigger
// =============================================================================
typedef enum
{
/// One data received in the Rx FIFO
    HAL_UART_RX_TRIG_1                          = 0x00000000,
/// 1/4 of the Rx FIFO is full
    HAL_UART_RX_TRIG_QUARTER                    = 0x00000001,
/// 1/2 of the Rx FIFO is full
    HAL_UART_RX_TRIG_HALF                       = 0x00000002,
/// Rx FIFO is almost full
    HAL_UART_RX_TRIG_NEARFULL                   = 0x00000003,
    HAL_UART_RX_TRIG_QTY                        = 0x00000004
} HAL_UART_RX_TRIGGER_CFG_T;


// ============================================================================
// HAL_UART_TX_TRIGGER_CFG_T
// -----------------------------------------------------------------------------
/// Tranmission FIFO trigger (or treshold) level. The Uarts can be configured to
/// generate an interrupt when the emission FIFO is below a configurable threshold
/// (Tx FIFO trigger
// =============================================================================
typedef enum
{
/// Tx FIFO empty
    HAL_UART_TX_TRIG_EMPTY                      = 0x00000000,
/// Less than 1/4 of the Tx FIFO left to send
    HAL_UART_TX_TRIG_QUARTER                    = 0x00000001,
/// Less than 1/2 of the Tx FIFO left to send
    HAL_UART_TX_TRIG_HALF                       = 0x00000002,
/// Less thant 3/4 of the Tx FIFO left to send
    HAL_UART_TX_TRIG_3QUARTER                   = 0x00000003,
    HAL_UART_TX_TRIG_QTY                        = 0x00000004
} HAL_UART_TX_TRIGGER_CFG_T;


// ============================================================================
// HAL_UART_AFC_MODE_T
// -----------------------------------------------------------------------------
/// Auto Flow Control. Controls the Rx Fifo level at which the Uart_RTS Auto Flow
/// Control will be set inactive high (see UART Operation for more details on AFC).
/// The Uart_RTS Auto Flow Control will be set inactive high when quantity of data
/// in Rx Fifo &amp;gt; AFC Level
// =============================================================================
typedef enum
{
/// RTS inactive with 1 data in the Rx FIFO
    HAL_UART_AFC_MODE_RX_TRIG_1                 = 0x00000000,
/// RTS inactive with 2 data in the Rx FIFO
    HAL_UART_AFC_MODE_RX_TRIG_2                 = 0x00000001,
/// RTS inactive with 3 data in the Rx FIFO
    HAL_UART_AFC_MODE_RX_TRIG_3                 = 0x00000002,
/// RTS inactive with 4 data in the Rx FIFO
    HAL_UART_AFC_MODE_RX_TRIG_4                 = 0x00000003,
/// RTS inactive with 5 data in the Rx FIFO
    HAL_UART_AFC_MODE_RX_TRIG_5                 = 0x00000004,
/// RTS inactive with 6 data in the Rx FIFO
    HAL_UART_AFC_MODE_RX_TRIG_6                 = 0x00000005,
/// RTS inactive with 7 data in the Rx FIFO
    HAL_UART_AFC_MODE_RX_TRIG_7                 = 0x00000006,
/// RTS inactive with 8 data in the Rx FIFO
    HAL_UART_AFC_MODE_RX_TRIG_8                 = 0x00000007,
/// RTS inactive with 9 data in the Rx FIFO
    HAL_UART_AFC_MODE_RX_TRIG_9                 = 0x00000008,
/// RTS inactive with 10 data in the Rx FIFO
    HAL_UART_AFC_MODE_RX_TRIG_10                = 0x00000009,
/// RTS inactive with 11 data in the Rx FIFO
    HAL_UART_AFC_MODE_RX_TRIG_11                = 0x0000000A,
/// RTS inactive with 12 data in the Rx FIFO
    HAL_UART_AFC_MODE_RX_TRIG_12                = 0x0000000B,
/// RTS inactive with 13 data in the Rx FIFO
    HAL_UART_AFC_MODE_RX_TRIG_13                = 0x0000000C,
/// RTS inactive with 14 data in the Rx FIFO
    HAL_UART_AFC_MODE_RX_TRIG_14                = 0x0000000D,
/// RTS inactive with 15 data in the Rx FIFO
    HAL_UART_AFC_MODE_RX_TRIG_15                = 0x0000000E,
/// RTS inactive with 16 data in the Rx FIFO
    HAL_UART_AFC_MODE_RX_TRIG_16                = 0x0000000F,
/// Hardware flow control is disabled. \n &amp;lt;B&amp;gt; NEVER USE THIS MODE &amp;lt;/B&amp;gt;
    HAL_UART_AFC_MODE_DISABLE                   = 0x00000010,
    HAL_UART_AFC_MODE_QTY                       = 0x00000011,
/// AFC mode is loopback \n When set, data on the Uart_Tx line is held high, while
/// the serial output is looped back to the serial input line, internally.
    HAL_UART_AFC_LOOP_BACK                      = 0x00000020
} HAL_UART_AFC_MODE_T;


// ============================================================================
// HAL_UART_IRDA_MODE_T
// -----------------------------------------------------------------------------
/// IrDA protocole enabling IrDA SIR mode is available, and can be activated when
/// the user open the Uart
// =============================================================================
typedef enum
{
/// IrDA mode disabled
    HAL_UART_IRDA_MODE_DISABLE                  = 0x00000000,
/// IrDA mode enabled
    HAL_UART_IRDA_MODE_ENABLE                   = 0x00000001,
    HAL_UART_IRDA_MODE_QTY                      = 0x00000002
} HAL_UART_IRDA_MODE_T;


// ============================================================================
// HAL_UART_BAUD_RATE_T
// -----------------------------------------------------------------------------
/// Baudrate available with the modifiable system clock UARTs are able to run at
/// a wide selection of baud rates. This must be configured at the UART opening
// =============================================================================
typedef enum
{
/// 2.4 KBaud (Serial and IrDA)
    HAL_UART_BAUD_RATE_2400                     = 0x00000960,
/// 4.8 KBaud (Serial and IrDA)
    HAL_UART_BAUD_RATE_4800                     = 0x000012C0,
/// 9.6 KBaud (Serial and IrDA)
    HAL_UART_BAUD_RATE_9600                     = 0x00002580,
/// 14.4 KBaud (Serial and IrDA)
    HAL_UART_BAUD_RATE_14400                    = 0x00003840,
/// 19.2 KBaud (Serial and IrDA)
    HAL_UART_BAUD_RATE_19200                    = 0x00004B00,
/// 28.8 KBaud (Serial and IrDA)
    HAL_UART_BAUD_RATE_28800                    = 0x00007080,
/// 33.6 KBaud (Serial and IrDA)
    HAL_UART_BAUD_RATE_33600                    = 0x00008340,
/// 38.4 KBaud (Serial and IrDA)
    HAL_UART_BAUD_RATE_38400                    = 0x00009600,
/// 57.6 KBaud (Serial and IrDA)
    HAL_UART_BAUD_RATE_57600                    = 0x0000E100,
/// 115.2 KBaud (Serial and IrDA)
    HAL_UART_BAUD_RATE_115200                   = 0x0001C200,
/// 230.4 KBaud (Available only in serial mode)
    HAL_UART_BAUD_RATE_230400                   = 0x00038400,
/// 460.8 KBaud (Available only in serial mode)
    HAL_UART_BAUD_RATE_460800                   = 0x00070800,
/// 921.6 KBaud (Available only in serial mode)
    HAL_UART_BAUD_RATE_921600                   = 0x000E1000,
/// 1843.2 KBaud (Available only in serial mode)
    HAL_UART_BAUD_RATE_1843200                  = 0x001C2000,
    HAL_UART_BAUD_RATE_QTY                      = 0x001C2001
} HAL_UART_BAUD_RATE_T;


// ============================================================================
// HAL_UART_TRANSFERT_MODE_T
// -----------------------------------------------------------------------------
/// Data transfert mode: via DMA or direct. To allow for an easy use of the Uart
/// modules, a non blocking hardware abstraction layer interface is provided
// =============================================================================
typedef enum
{
/// Direct polling: The application sends/receives the data directly to/from the
/// hardware module. The number of bytes actually sent/received is returned. No IRQ
/// is generated.
    HAL_UART_TRANSFERT_MODE_DIRECT_POLLING      = 0x00000000,
/// Direct Irq: The application sends/receives the data directly to/from the hardware
/// module. The number of bytes actually sent/received is returned.An irq can be
/// generated when the Tx/Rx FIFO reaches the pre-programmed level.
    HAL_UART_TRANSFERT_MODE_DIRECT_IRQ          = 0x00000001,
/// DMA polling: The application sends/receives the data through a DMA to the hardware
/// module. When no DMA channel is available, the function returns 0. No byte is
/// sent. When a DMA resource is available, the function returns the number of bytes
/// to send. They will all be sent. A function allows to check if the previous DMA
/// transfer is finished. No new DMA transfer for the same Uart and in the same direction
/// is allowed until the previous transfer is finished.
    HAL_UART_TRANSFERT_MODE_DMA_POLLING         = 0x00000002,
/// The application sends/receives the data through a DMA to the hardware module.
/// When no DMA channel is available, the function returns 0. No byte is sent. When
/// a DMA resource is available, the function returns the number of bytes to send.
/// They will all be sent. An Irq is generated when the current transfer is finished.
/// No new DMA transfer for the same Uart and in the same direction is allowed until
/// the previous transfer is finished.
    HAL_UART_TRANSFERT_MODE_DMA_IRQ             = 0x00000003,
/// The transfert is off.
    HAL_UART_TRANSFERT_MODE_OFF                 = 0x00000004,
    HAL_UART_TRANSFERT_MODE_QTY                 = 0x00000005
} HAL_UART_TRANSFERT_MODE_T;


// =============================================================================
//  TYPES
// =============================================================================

// ============================================================================
// HAL_UART_CFG_T
// -----------------------------------------------------------------------------
/// UART Configuration Structure This structure defines the Uart behavior
// =============================================================================
typedef struct
{
    /// Data format
    HAL_UART_DATA_BITS_T           data;                         //0x00000000
    /// Number of stop bits
    HAL_UART_STOP_BITS_QTY_T       stop;                         //0x00000004
    /// Parity check
    HAL_UART_PARITY_CFG_T          parity;                       //0x00000008
    /// Trigger for the Rx FIFO
    HAL_UART_RX_TRIGGER_CFG_T      rx_trigger;                   //0x0000000C
    /// Trigger for the Tx FIFO
    HAL_UART_TX_TRIGGER_CFG_T      tx_trigger;                   //0x00000010
    /// Hardware Flow control
    HAL_UART_AFC_MODE_T            afc;                          //0x00000014
    /// IrDA mode
    HAL_UART_IRDA_MODE_T           irda;                         //0x00000018
    /// Baud Rate
    HAL_UART_BAUD_RATE_T           rate;                         //0x0000001C
    /// Reception transfer mode
    HAL_UART_TRANSFERT_MODE_T      rx_mode;                      //0x00000020
    /// Transmission transfer mode
    HAL_UART_TRANSFERT_MODE_T      tx_mode;                      //0x00000024
} HAL_UART_CFG_T; //Size : 0x28



// ============================================================================
// HAL_UART_IRQ_STATUS_T
// -----------------------------------------------------------------------------
/// 
// =============================================================================
typedef struct
{
    u32                            txModemStatus:1;
    u32                            rxDataAvailable:1;
    u32                            txDataNeeded:1;
    u32                            rxTimeout:1;
    u32                            rxLineErr:1;
    u32                            txDmaDone:1;
    u32                            rxDmaDone:1;
    u32                            rxDmaTimeout:1;
} HAL_UART_IRQ_STATUS_T;
//unused
#define HAL_UART_IRQ_STATUS_TXMODEMSTATUS (1<<0)
#define HAL_UART_IRQ_STATUS_RXDATAAVAILABLE (1<<1)
#define HAL_UART_IRQ_STATUS_TXDATANEEDED (1<<2)
#define HAL_UART_IRQ_STATUS_RXTIMEOUT (1<<3)
#define HAL_UART_IRQ_STATUS_RXLINEERR (1<<4)
#define HAL_UART_IRQ_STATUS_TXDMADONE (1<<5)
#define HAL_UART_IRQ_STATUS_RXDMADONE (1<<6)
#define HAL_UART_IRQ_STATUS_RXDMATIMEOUT (1<<7)



// ============================================================================
// HAL_UART_ERROR_STATUS_T
// -----------------------------------------------------------------------------
/// 
// =============================================================================
typedef struct
{
    u32                            _:4;
    u32                            rxOvflErr:1;
    u32                            txOvflErr:1;
    u32                            rxParityErr:1;
    u32                            rxFramingErr:1;
    u32                            rxBreakInt:1;
} HAL_UART_ERROR_STATUS_T;
//unused
#define HAL_UART_ERROR_STATUS__(n)  (((n)&15)<<0)
#define HAL_UART_ERROR_STATUS_RXOVFLERR (1<<4)
#define HAL_UART_ERROR_STATUS_TXOVFLERR (1<<5)
#define HAL_UART_ERROR_STATUS_RXPARITYERR (1<<6)
#define HAL_UART_ERROR_STATUS_RXFRAMINGERR (1<<7)
#define HAL_UART_ERROR_STATUS_RXBREAKINT (1<<8)


/// Uart 0 is the trace uart and is unavailable for this driver
/// The numbering starts at 1 for consistency.
/// The HAL_UART_QTY value is defined as the number of UARTS
/// avalaible for the chip on which the driver is running, and
/// can therefore be used for consistency checks
typedef enum {
    HAL_UART_1                                  = 0x00000001,
    HAL_UART_2                                  = 0x00000002,
    HAL_UART_QTY                                = CHIP_STD_UART_QTY+1
} HAL_UART_ID_T;   

#endif /* __RDA_UART_H_ */
