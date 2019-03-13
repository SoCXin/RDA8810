#ifndef _AP_IRQ_H_
#define _AP_IRQ_H_


#include "mach/hardware.h"

// =============================================================================
//  MACROS
// =============================================================================


// =============================================================================
//  TYPES
// =============================================================================

// ============================================================================
// AP_IRQ_T
// -----------------------------------------------------------------------------
///
// =============================================================================
#define REG_AP_IRQ_BASE             0x00800000

typedef volatile struct
{
    /// If cause is not null and interrupt are enabled then the interrupt line 0
    /// is driven on the system CPU.
    /// The cause for the Irq sources, one bit for each module's irq source.
    /// The cause is the actual Irq source masked by the mask register.
    REG32                          Cause;                        //0x00000000
    /// The status for the level Irq sources, one bit for each module's irq source.
    ///
    /// The status reflect the actual Irq source.
    REG32                          Status;                       //0x00000004
    /// Writing '1' sets the corresponding bit in the mask register to '1'.
    /// Reading gives the value of the mask register.
    REG32                          Mask_Set;                     //0x00000008
    /// Writing '1' clears the corresponding bit in the mask register to '0'.
    /// Reading gives the value of the mask register.
    REG32                          Mask_Clear;                   //0x0000000C
    REG32                          NonMaskable;                  //0x00000010
    REG32                          SC;                           //0x00000014
    /// Each bit to '1' in that registers allows the correcponding interrupt to wake
    /// up the A5 CPU (i.e.: Reenable it's clock, see CLOCK_AP_ENABLE and CLOCK_AP_DISABLE
    /// registers in sys_ctrl_ap registers section)
    REG32                          WakeUp_Mask;                  //0x00000018
    REG32                          Cpu_Sleep;                    //0x0000001C
    /// Writing '1' sets the corresponding bit in the mask register to '1'.
    /// Reading gives the value of the mask register.
    REG32                          Pulse_Mask_Set;               //0x00000020
    /// Writing '1' clears the corresponding bit in the mask register to '0'.
    /// Reading gives the value of the mask register.
    REG32                          Pulse_Mask_Clr;               //0x00000024
    /// Writing '1' clears the corresponding Pulse IRQ.
    /// Pulse IRQ are set by the modules and cleared here.
    REG32                          Pulse_Clear;                  //0x00000028
    /// The status for the Pulse Irq sources, one bit for each module's irq source.
    ///
    /// The status reflect the actual Irq source.
    REG32                          Pulse_Status;                 //0x0000002C
} HWP_AP_IRQ_T;


//Cause
#define AP_IRQ_AP_IRQ_PULSE         (1<<0)
#define AP_IRQ_AP_IRQ_I2C           (1<<1)
#define AP_IRQ_AP_IRQ_NFSC          (1<<2)
#define AP_IRQ_AP_IRQ_SDMMC1        (1<<3)
#define AP_IRQ_AP_IRQ_SDMMC2        (1<<4)
#define AP_IRQ_AP_IRQ_SDMMC3        (1<<5)
#define AP_IRQ_AP_IRQ_SPI1          (1<<6)
#define AP_IRQ_AP_IRQ_SPI2          (1<<7)
#define AP_IRQ_AP_IRQ_SPI3          (1<<8)
#define AP_IRQ_AP_IRQ_UART1         (1<<9)
#define AP_IRQ_AP_IRQ_UART2         (1<<10)
#define AP_IRQ_AP_IRQ_UART3         (1<<11)
#define AP_IRQ_AP_IRQ_GPIO1         (1<<12)
#define AP_IRQ_AP_IRQ_GPIO2         (1<<13)
#define AP_IRQ_AP_IRQ_GPIO3         (1<<14)
#define AP_IRQ_AP_IRQ_KEYPAD        (1<<15)
#define AP_IRQ_AP_IRQ_TIMERS        (1<<16)
#define AP_IRQ_AP_IRQ_OSTIMER       (1<<17)
#define AP_IRQ_AP_IRQ_COM0          (1<<18)
#define AP_IRQ_AP_IRQ_COM1          (1<<19)
#define AP_IRQ_AP_IRQ_USBC          (1<<20)
#define AP_IRQ_AP_IRQ_DMC           (1<<21)
#define AP_IRQ_AP_IRQ_DMA           (1<<22)
#define AP_IRQ_AP_IRQ_CAMERA        (1<<23)
#define AP_IRQ_AP_IRQ_GOUDA         (1<<24)
#define AP_IRQ_AP_IRQ_GPU           (1<<25)
#define AP_IRQ_AP_IRQ_JPG_VPU       (1<<26)
#define AP_IRQ_AP_IRQ_HOST_VPU      (1<<27)
#define AP_IRQ_AP_IRQ_VOC           (1<<28)
#define AP_IRQ_AP_IRQ_AUIFC0        (1<<29)
#define AP_IRQ_AP_IRQ_AUIFC1        (1<<30)
#define AP_IRQ_AP_IRQ_L2CC          (1<<31)
#define AP_IRQ_CAUSE(n)             (((n)&0xFFFFFFFF)<<0)
#define AP_IRQ_CAUSE_MASK           (0xFFFFFFFF<<0)
#define AP_IRQ_CAUSE_SHIFT          (0)

//Status
//#define AP_IRQ_AP_IRQ_PULSE       (1<<0)
//#define AP_IRQ_AP_IRQ_I2C         (1<<1)
//#define AP_IRQ_AP_IRQ_NFSC        (1<<2)
//#define AP_IRQ_AP_IRQ_SDMMC1      (1<<3)
//#define AP_IRQ_AP_IRQ_SDMMC2      (1<<4)
//#define AP_IRQ_AP_IRQ_SDMMC3      (1<<5)
//#define AP_IRQ_AP_IRQ_SPI1        (1<<6)
//#define AP_IRQ_AP_IRQ_SPI2        (1<<7)
//#define AP_IRQ_AP_IRQ_SPI3        (1<<8)
//#define AP_IRQ_AP_IRQ_UART1       (1<<9)
//#define AP_IRQ_AP_IRQ_UART2       (1<<10)
//#define AP_IRQ_AP_IRQ_UART3       (1<<11)
//#define AP_IRQ_AP_IRQ_GPIO1       (1<<12)
//#define AP_IRQ_AP_IRQ_GPIO2       (1<<13)
//#define AP_IRQ_AP_IRQ_GPIO3       (1<<14)
//#define AP_IRQ_AP_IRQ_KEYPAD      (1<<15)
//#define AP_IRQ_AP_IRQ_TIMERS      (1<<16)
//#define AP_IRQ_AP_IRQ_OSTIMER     (1<<17)
//#define AP_IRQ_AP_IRQ_COM0        (1<<18)
//#define AP_IRQ_AP_IRQ_COM1        (1<<19)
//#define AP_IRQ_AP_IRQ_USBC        (1<<20)
//#define AP_IRQ_AP_IRQ_DMC         (1<<21)
//#define AP_IRQ_AP_IRQ_DMA         (1<<22)
//#define AP_IRQ_AP_IRQ_CAMERA      (1<<23)
//#define AP_IRQ_AP_IRQ_GOUDA       (1<<24)
//#define AP_IRQ_AP_IRQ_GPU         (1<<25)
//#define AP_IRQ_AP_IRQ_JPG_VPU     (1<<26)
//#define AP_IRQ_AP_IRQ_HOST_VPU    (1<<27)
//#define AP_IRQ_AP_IRQ_VOC         (1<<28)
//#define AP_IRQ_AP_IRQ_AUIFC0      (1<<29)
//#define AP_IRQ_AP_IRQ_AUIFC1      (1<<30)
//#define AP_IRQ_AP_IRQ_L2CC        (1<<31)
#define AP_IRQ_STATUS(n)            (((n)&0xFFFFFFFF)<<0)
#define AP_IRQ_STATUS_MASK          (0xFFFFFFFF<<0)
#define AP_IRQ_STATUS_SHIFT         (0)

//Mask_Set
//#define AP_IRQ_AP_IRQ_PULSE       (1<<0)
//#define AP_IRQ_AP_IRQ_I2C         (1<<1)
//#define AP_IRQ_AP_IRQ_NFSC        (1<<2)
//#define AP_IRQ_AP_IRQ_SDMMC1      (1<<3)
//#define AP_IRQ_AP_IRQ_SDMMC2      (1<<4)
//#define AP_IRQ_AP_IRQ_SDMMC3      (1<<5)
//#define AP_IRQ_AP_IRQ_SPI1        (1<<6)
//#define AP_IRQ_AP_IRQ_SPI2        (1<<7)
//#define AP_IRQ_AP_IRQ_SPI3        (1<<8)
//#define AP_IRQ_AP_IRQ_UART1       (1<<9)
//#define AP_IRQ_AP_IRQ_UART2       (1<<10)
//#define AP_IRQ_AP_IRQ_UART3       (1<<11)
//#define AP_IRQ_AP_IRQ_GPIO1       (1<<12)
//#define AP_IRQ_AP_IRQ_GPIO2       (1<<13)
//#define AP_IRQ_AP_IRQ_GPIO3       (1<<14)
//#define AP_IRQ_AP_IRQ_KEYPAD      (1<<15)
//#define AP_IRQ_AP_IRQ_TIMERS      (1<<16)
//#define AP_IRQ_AP_IRQ_OSTIMER     (1<<17)
//#define AP_IRQ_AP_IRQ_COM0        (1<<18)
//#define AP_IRQ_AP_IRQ_COM1        (1<<19)
//#define AP_IRQ_AP_IRQ_USBC        (1<<20)
//#define AP_IRQ_AP_IRQ_DMC         (1<<21)
//#define AP_IRQ_AP_IRQ_DMA         (1<<22)
//#define AP_IRQ_AP_IRQ_CAMERA      (1<<23)
//#define AP_IRQ_AP_IRQ_GOUDA       (1<<24)
//#define AP_IRQ_AP_IRQ_GPU         (1<<25)
//#define AP_IRQ_AP_IRQ_JPG_VPU     (1<<26)
//#define AP_IRQ_AP_IRQ_HOST_VPU    (1<<27)
//#define AP_IRQ_AP_IRQ_VOC         (1<<28)
//#define AP_IRQ_AP_IRQ_AUIFC0      (1<<29)
//#define AP_IRQ_AP_IRQ_AUIFC1      (1<<30)
//#define AP_IRQ_AP_IRQ_L2CC        (1<<31)
#define AP_IRQ_MASK_SET(n)          (((n)&0xFFFFFFFF)<<0)
#define AP_IRQ_MASK_SET_MASK        (0xFFFFFFFF<<0)
#define AP_IRQ_MASK_SET_SHIFT       (0)

//Mask_Clear
//#define AP_IRQ_AP_IRQ_PULSE       (1<<0)
//#define AP_IRQ_AP_IRQ_I2C         (1<<1)
//#define AP_IRQ_AP_IRQ_NFSC        (1<<2)
//#define AP_IRQ_AP_IRQ_SDMMC1      (1<<3)
//#define AP_IRQ_AP_IRQ_SDMMC2      (1<<4)
//#define AP_IRQ_AP_IRQ_SDMMC3      (1<<5)
//#define AP_IRQ_AP_IRQ_SPI1        (1<<6)
//#define AP_IRQ_AP_IRQ_SPI2        (1<<7)
//#define AP_IRQ_AP_IRQ_SPI3        (1<<8)
//#define AP_IRQ_AP_IRQ_UART1       (1<<9)
//#define AP_IRQ_AP_IRQ_UART2       (1<<10)
//#define AP_IRQ_AP_IRQ_UART3       (1<<11)
//#define AP_IRQ_AP_IRQ_GPIO1       (1<<12)
//#define AP_IRQ_AP_IRQ_GPIO2       (1<<13)
//#define AP_IRQ_AP_IRQ_GPIO3       (1<<14)
//#define AP_IRQ_AP_IRQ_KEYPAD      (1<<15)
//#define AP_IRQ_AP_IRQ_TIMERS      (1<<16)
//#define AP_IRQ_AP_IRQ_OSTIMER     (1<<17)
//#define AP_IRQ_AP_IRQ_COM0        (1<<18)
//#define AP_IRQ_AP_IRQ_COM1        (1<<19)
//#define AP_IRQ_AP_IRQ_USBC        (1<<20)
//#define AP_IRQ_AP_IRQ_DMC         (1<<21)
//#define AP_IRQ_AP_IRQ_DMA         (1<<22)
//#define AP_IRQ_AP_IRQ_CAMERA      (1<<23)
//#define AP_IRQ_AP_IRQ_GOUDA       (1<<24)
//#define AP_IRQ_AP_IRQ_GPU         (1<<25)
//#define AP_IRQ_AP_IRQ_JPG_VPU     (1<<26)
//#define AP_IRQ_AP_IRQ_HOST_VPU    (1<<27)
//#define AP_IRQ_AP_IRQ_VOC         (1<<28)
//#define AP_IRQ_AP_IRQ_AUIFC0      (1<<29)
//#define AP_IRQ_AP_IRQ_AUIFC1      (1<<30)
//#define AP_IRQ_AP_IRQ_L2CC        (1<<31)
#define AP_IRQ_MASK_CLR(n)          (((n)&0xFFFFFFFF)<<0)
#define AP_IRQ_MASK_CLR_MASK        (0xFFFFFFFF<<0)
#define AP_IRQ_MASK_CLR_SHIFT       (0)

//NonMaskable
#define AP_IRQ_MAIN_IRQ             (1<<10)
#define AP_IRQ_PAGE_SPY_IRQ         (1<<13)
#define AP_IRQ_DEBUG_IRQ            (1<<14)
#define AP_IRQ_HOST_IRQ             (1<<15)
#define AP_IRQ_INTENABLE_STATUS     (1<<31)

//SC
#define AP_IRQ_INTENABLE            (1<<0)

//WakeUp_Mask
//#define AP_IRQ_AP_IRQ_PULSE       (1<<0)
//#define AP_IRQ_AP_IRQ_I2C         (1<<1)
//#define AP_IRQ_AP_IRQ_NFSC        (1<<2)
//#define AP_IRQ_AP_IRQ_SDMMC1      (1<<3)
//#define AP_IRQ_AP_IRQ_SDMMC2      (1<<4)
//#define AP_IRQ_AP_IRQ_SDMMC3      (1<<5)
//#define AP_IRQ_AP_IRQ_SPI1        (1<<6)
//#define AP_IRQ_AP_IRQ_SPI2        (1<<7)
//#define AP_IRQ_AP_IRQ_SPI3        (1<<8)
//#define AP_IRQ_AP_IRQ_UART1       (1<<9)
//#define AP_IRQ_AP_IRQ_UART2       (1<<10)
//#define AP_IRQ_AP_IRQ_UART3       (1<<11)
//#define AP_IRQ_AP_IRQ_GPIO1       (1<<12)
//#define AP_IRQ_AP_IRQ_GPIO2       (1<<13)
//#define AP_IRQ_AP_IRQ_GPIO3       (1<<14)
//#define AP_IRQ_AP_IRQ_KEYPAD      (1<<15)
//#define AP_IRQ_AP_IRQ_TIMERS      (1<<16)
//#define AP_IRQ_AP_IRQ_OSTIMER     (1<<17)
//#define AP_IRQ_AP_IRQ_COM0        (1<<18)
//#define AP_IRQ_AP_IRQ_COM1        (1<<19)
//#define AP_IRQ_AP_IRQ_USBC        (1<<20)
//#define AP_IRQ_AP_IRQ_DMC         (1<<21)
//#define AP_IRQ_AP_IRQ_DMA         (1<<22)
//#define AP_IRQ_AP_IRQ_CAMERA      (1<<23)
//#define AP_IRQ_AP_IRQ_GOUDA       (1<<24)
//#define AP_IRQ_AP_IRQ_GPU         (1<<25)
//#define AP_IRQ_AP_IRQ_JPG_VPU     (1<<26)
//#define AP_IRQ_AP_IRQ_HOST_VPU    (1<<27)
//#define AP_IRQ_AP_IRQ_VOC         (1<<28)
//#define AP_IRQ_AP_IRQ_AUIFC0      (1<<29)
//#define AP_IRQ_AP_IRQ_AUIFC1      (1<<30)
//#define AP_IRQ_AP_IRQ_L2CC        (1<<31)
#define AP_IRQ_WAKEUP_MASK(n)       (((n)&0xFFFFFFFF)<<0)
#define AP_IRQ_WAKEUP_MASK_MASK     (0xFFFFFFFF<<0)
#define AP_IRQ_WAKEUP_MASK_SHIFT    (0)

//Cpu_Sleep
#define AP_IRQ_SLEEP                (1<<0)

//Pulse_Mask_Set
#define AP_IRQ_PULSE_MASK_SET       (1<<0)

//Pulse_Mask_Clr
#define AP_IRQ_PULSE_MASK_CLR       (1<<0)

//Pulse_Clear
#define AP_IRQ_PULSE_CLR            (1<<0)

//Pulse_Status
//#define AP_IRQ_STATUS             (1<<0)





#endif

