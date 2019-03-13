#include <common.h>
#include <asm/io.h>
#include <asm/arch/hardware.h>
#include <asm/arch/rda_iomap.h>

DECLARE_GLOBAL_DATA_PTR;

typedef volatile struct
{
    REG32                          OSTimer_Ctrl;                 //0x00000000
    REG32                          OSTimer_CurVal;               //0x00000004
    REG32                          WDTimer_Ctrl;                 //0x00000008
    REG32                          WDTimer_LoadVal;              //0x0000000C
    REG32                          HWTimer_Ctrl;                 //0x00000010
    REG32                          HWTimer_CurVal;               //0x00000014
    REG32                          Timer_Irq_Mask_Set;           //0x00000018
    REG32                          Timer_Irq_Mask_Clr;           //0x0000001C
    REG32                          Timer_Irq_Clr;                //0x00000020
    REG32                          Timer_Irq_Cause;              //0x00000024
} HWP_TIMER_T;

#define hwp_timer                   ((HWP_TIMER_T*)(RDA_TIMER_BASE))

int timer_init(void)
{
	gd->timer_rate_hz = CONFIG_SYS_HZ_CLOCK;
	gd->timer_reset_value = 0;

	/* We are using timer34 in unchained 32-bit mode, full speed */
	return(0);
}

void reset_timer(void)
{
	gd->timer_reset_value = get_ticks();
}

/*
 * Get the current 64 bit timer tick count
 */
unsigned long long get_ticks(void)
{
	unsigned long now = hwp_timer->HWTimer_CurVal;

	/* increment tbu if tbl has rolled over */
	if (now < gd->tbl)
		gd->tbu++;
	gd->tbl = now;

	return (((unsigned long long)gd->tbu) << 32) | gd->tbl;
}

ulong get_timer(ulong base)
{
	unsigned long long timer_diff;

	timer_diff = get_ticks() - gd->timer_reset_value;

	return (timer_diff / (gd->timer_rate_hz / CONFIG_SYS_HZ)) - base;
}

#pragma GCC push_options
#pragma GCC optimize ("O0")

void __udelay(unsigned long usec)
{
#if 0 /* our timer is 16kHz, can NOT support udelay */
	unsigned long long endtime;

	endtime = ((unsigned long long)usec * gd->timer_rate_hz) / 1000000UL;
	endtime += get_ticks();

	while (get_ticks() < endtime)
		;
#else /* use loop instead */
#define USEC_LOOP (1)
	int i, j;
	for (i=0;i<(usec);i++)
		for (j=0;j<USEC_LOOP;j++)
			;
#endif
}

#pragma GCC pop_options

/*
 * This function is derived from PowerPC code (timebase clock frequency).
 * On ARM it returns the number of timer ticks per second.
 */
ulong get_tbclk(void)
{
	return CONFIG_SYS_HZ;
}
