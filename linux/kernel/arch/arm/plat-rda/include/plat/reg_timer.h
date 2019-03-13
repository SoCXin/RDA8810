#ifndef __RDA_REG_TIMER_H
#define __RDA_REG_TIMER_H

#include <mach/hardware.h>
#include <mach/iomap.h>

// =============================================================================
//  MACROS
// =============================================================================
#define NB_INTERVAL                              (1)
#define INT_TIMER_NB_BITS                        (56)
#define HW_TIMER_NB_BITS                         (64)

// ============================================================================
// TIMER_AP_T
// -----------------------------------------------------------------------------
typedef volatile struct
{
    REG32                          OSTimer_LoadVal_L;            //0x00000000
    REG32                          OSTimer_Ctrl;                 //0x00000004
    REG32                          OSTimer_CurVal_L;             //0x00000008
    REG32                          OSTimer_CurVal_H;             //0x0000000C
    REG32                          OSTimer_LockVal_L;            //0x00000010
    REG32                          OSTimer_LockVal_H;            //0x00000014
    REG32                          HWTimer_Ctrl;                 //0x00000018
    REG32                          HWTimer_CurVal_L;             //0x0000001C
    REG32                          HWTimer_CurVal_H;             //0x00000020
    REG32                          HWTimer_LockVal_L;            //0x00000024
    REG32                          HWTimer_LockVal_H;            //0x00000028
    REG32                          Timer_Irq_Mask_Set;           //0x0000002C
    REG32                          Timer_Irq_Mask_Clr;           //0x00000030
    REG32                          Timer_Irq_Clr;                //0x00000034
    REG32                          Timer_Irq_Cause;              //0x00000038
} HWP_TIMER_AP_T;

//OSTimer_LoadVal_L
#define TIMER_AP_OS_LOADVAL_L(n)    (((n)&0xFFFFFFFF)<<0)

//OSTimer_Ctrl
#define TIMER_AP_OS_LOADVAL_H(n)    (((n)&0xFFFFFF)<<0)
#define TIMER_AP_OS_LOADVAL_H_MASK  (0xFFFFFF<<0)
#define TIMER_AP_OS_LOADVAL_H_SHIFT (0)
#define TIMER_AP_ENABLE             (1<<24)
#define TIMER_AP_ENABLED            (1<<25)
#define TIMER_AP_CLEARED            (1<<26)
#define TIMER_AP_REPEAT             (1<<28)
#define TIMER_AP_WRAP               (1<<29)
#define TIMER_AP_LOAD               (1<<30)

//OSTimer_CurVal_L
#define TIMER_AP_OS_CURVAL_L(n)     (((n)&0xFFFFFFFF)<<0)

//OSTimer_CurVal_H
#define TIMER_AP_OS_CURVAL_H(n)     (((n)&0xFFFFFF)<<0)
#define TIMER_AP_OS_CURVAL_H_MASK   (0xFFFFFF<<0)
#define TIMER_AP_OS_CURVAL_H_SHIFT  (0)

//OSTimer_LockVal_L
#define TIMER_AP_OS_LOCKVAL_L(n)    (((n)&0xFFFFFFFF)<<0)
#define TIMER_AP_OS_LOCKVAL_L_MASK  (0xFFFFFFFF<<0)
#define TIMER_AP_OS_LOCKVAL_L_SHIFT (0)

//OSTimer_LockVal_H
#define TIMER_AP_OS_LOCKVAL_H(n)    (((n)&0xFFFFFF)<<0)
#define TIMER_AP_OS_LOCKVAL_H_MASK  (0xFFFFFF<<0)
#define TIMER_AP_OS_LOCKVAL_H_SHIFT (0)

//HWTimer_Ctrl
#define TIMER_AP_INTERVAL_EN        (1<<8)
#define TIMER_AP_INTERVAL(n)        (((n)&3)<<0)

//HWTimer_CurVal_L
#define TIMER_AP_HW_CURVAL_L(n)     (((n)&0xFFFFFFFF)<<0)

//HWTimer_CurVal_H
#define TIMER_AP_HW_CURVAL_H(n)     (((n)&0xFFFFFFFF)<<0)

//HWTimer_LockVal_L
#define TIMER_AP_HW_LOCKVAL_L(n)    (((n)&0xFFFFFFFF)<<0)

//HWTimer_LockVal_H
#define TIMER_AP_HW_LOCKVAL_H(n)    (((n)&0xFFFFFFFF)<<0)

//Timer_Irq_Mask_Set
#define TIMER_AP_OSTIMER_MASK       (1<<0)
#define TIMER_AP_HWTIMER_WRAP_MASK  (1<<1)
#define TIMER_AP_HWTIMER_ITV_MASK   (1<<2)

//Timer_Irq_Mask_Clr
//#define TIMER_AP_OSTIMER_MASK     (1<<0)
//#define TIMER_AP_HWTIMER_WRAP_MASK (1<<1)
//#define TIMER_AP_HWTIMER_ITV_MASK (1<<2)

//Timer_Irq_Clr
#define TIMER_AP_OSTIMER_CLR        (1<<0)
#define TIMER_AP_HWTIMER_WRAP_CLR   (1<<1)
#define TIMER_AP_HWTIMER_ITV_CLR    (1<<2)

//Timer_Irq_Cause
#define TIMER_AP_OSTIMER_CAUSE      (1<<0)
#define TIMER_AP_HWTIMER_WRAP_CAUSE (1<<1)
#define TIMER_AP_HWTIMER_ITV_CAUSE  (1<<2)
#define TIMER_AP_OSTIMER_STATUS     (1<<16)
#define TIMER_AP_HWTIMER_WRAP_STATUS (1<<17)
#define TIMER_AP_HWTIMER_ITV_STATUS (1<<18)
#define TIMER_AP_OTHER_TIMS_IRQ(n)  (((n)&3)<<1)
#define TIMER_AP_OTHER_TIMS_IRQ_MASK (3<<1)
#define TIMER_AP_OTHER_TIMS_IRQ_SHIFT (1)

#endif /* __RDA_REG_TIMER_H */