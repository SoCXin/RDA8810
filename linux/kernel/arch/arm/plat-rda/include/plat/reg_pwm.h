#ifndef _PWM_H_
#define _PWM_H_
#include <mach/hardware.h>

typedef volatile struct
{
    REG32                          PWT_Config;                   //0x00000000
    REG32                          LPG_Config;                   //0x00000004
    REG32                          PWL0_Config;                  //0x00000008
    REG32                          PWL1_Config;                  //0x0000000C
    REG32                          Cfg_Clk_PWM;                  //0x00000010
} HWP_PWM_T;

//PWT_Config
#define PWM_PWT_ENABLE              (1<<0)
#define PWM_PWT_START               (1<<1)
#define PWM_PWT_DUTY(n)             (((n)&0x3FF)<<4)
#define PWM_PWT_PERIOD(n)           (((n)&0x7FF)<<16)

//LPG_Config
#define PWM_LPG_RESET_L             (1<<0)
#define PWM_LPG_TEST                (1<<1)
#define PWM_LPG_ONTIME(n)           (((n)&15)<<4)
#define PWM_LPG_ONTIME_MASK         (15<<4)
#define PWM_LPG_ONTIME_SHIFT        (4)
#define PWM_LPG_ONTIME_UNDEFINED    (0<<4)
#define PWM_LPG_ONTIME_15_6MS       (1<<4)
#define PWM_LPG_ONTIME_31_2MS       (2<<4)
#define PWM_LPG_ONTIME_46_8MS       (3<<4)
#define PWM_LPG_ONTIME_62MS         (4<<4)
#define PWM_LPG_ONTIME_78MS         (5<<4)
#define PWM_LPG_ONTIME_94MS         (6<<4)
#define PWM_LPG_ONTIME_110MS        (7<<4)
#define PWM_LPG_ONTIME_125MS        (8<<4)
#define PWM_LPG_ONTIME_140MS        (9<<4)
#define PWM_LPG_ONTIME_156MS        (10<<4)
#define PWM_LPG_ONTIME_172MS        (11<<4)
#define PWM_LPG_ONTIME_188MS        (12<<4)
#define PWM_LPG_ONTIME_200MS        (13<<4)
#define PWM_LPG_ONTIME_218MS        (14<<4)
#define PWM_LPG_ONTIME_234MS        (15<<4)
#define PWM_LPG_PERIOD(n)           (((n)&7)<<16)
#define PWM_LPG_PERIOD_MASK         (7<<16)
#define PWM_LPG_PERIOD_SHIFT        (16)
#define PWM_LPG_PERIOD_0_125S       (0<<16)
#define PWM_LPG_PERIOD_0_25S        (1<<16)
#define PWM_LPG_PERIOD_0_5S         (2<<16)
#define PWM_LPG_PERIOD_0_75S        (3<<16)
#define PWM_LPG_PERIOD_1S           (4<<16)
#define PWM_LPG_PERIOD_1_25S        (5<<16)
#define PWM_LPG_PERIOD_1_5S         (6<<16)
#define PWM_LPG_PERIOD_1_75S        (7<<16)

//PWL0_Config
#define PWM_PWL_MIN(n)              (((n)&0xFF)<<0)
#define PWM_PWL_MAX(n)              (((n)&0xFF)<<8)
#define PWM_PWL0_EN_H               (1<<16)
#define PWM_PWL0_FORCE_L            (1<<17)
#define PWM_PWL0_FORCE_H            (1<<18)
#define PWM_PWL_PULSE_EN            (1<<19)
#define PWM_PWL0_SET_OE             (1<<20)
#define PWM_PWL0_CLR_OE             (1<<21)
#define PWM_PWL_SET_MUX             (1<<22)
#define PWM_PWL_CLR_MUX             (1<<23)
#define PWM_PWL_PULSE_PER(n)        (((n)&0xFF)<<24)

//PWL1_Config
#define PWM_PWL1_THRESHOLD(n)       (((n)&0xFF)<<0)
#define PWM_LFSR_REG(n)             (((n)&0xFF)<<8)
#define PWM_PWL1_EN_H               (1<<16)
#define PWM_PWL1_FORCE_L            (1<<17)
#define PWM_PWL1_FORCE_H            (1<<18)
#define PWM_PWL1_SET_OE             (1<<20)
#define PWM_PWL1_CLR_OE             (1<<21)

//Cfg_Clk_PWM
#define PWM_PWM_DIVIDER(n)          (((n)&0xFF)<<0)

#endif
