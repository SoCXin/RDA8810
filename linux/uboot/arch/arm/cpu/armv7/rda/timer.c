#include <common.h>
#include <asm/io.h>
#include <asm/arch/hardware.h>
#include <asm/arch/rda_iomap.h>
#include <asm/arch/reg_timer.h>

DECLARE_GLOBAL_DATA_PTR;

typedef union
{
    unsigned long long timer;
    struct
    {
        unsigned long  timer_l     :32;
        unsigned long  timer_h     :32;
    } fields;
} RDA_TIMER;

#if (CONFIG_SYS_HZ_CLOCK == 2000000)
/* optimize for 2M Hz */
static unsigned long tick_to_ms(unsigned long long tick) __attribute__((unused));
static unsigned long tick_to_ms(unsigned long long tick)
{
    return (u32)(tick >> 11);  // divide 2048
}

static unsigned long long ms_to_tick(unsigned long ms)
{
    return (((unsigned long long)ms) << 11); // x 2048
}

static unsigned long tick_to_us(unsigned long long tick)
{
    return (unsigned long)(tick >> 1);    // divide 2
}

static unsigned long long us_to_tick(unsigned long ms)
{
    return (((unsigned long long)ms) << 1);  // x2
}
#else
/* need to calc */
#error "Timer is not 2M Hz"
#endif

static void rda_timer_get(RDA_TIMER *timer)
{
    /* always read low 32bit first */
    timer->fields.timer_l = 
        (unsigned long)(hwp_apTimer->HWTimer_LockVal_L);
    timer->fields.timer_h = 
        (unsigned long)(hwp_apTimer->HWTimer_LockVal_H);
}

static void rda_timeout_setup_ms(RDA_TIMER *timer, unsigned long ms)
	__attribute__((unused));
static void rda_timeout_setup_ms(RDA_TIMER *timer, unsigned long ms)
{
    timer->timer = get_ticks() + ms_to_tick(ms);
}

static void rda_timeout_setup_us(RDA_TIMER *timer, unsigned long us)
{
    timer->timer = get_ticks() + us_to_tick(us);
}

static int rda_timeout_check(RDA_TIMER *timer)
{
    return (get_ticks() > timer->timer);
}

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
    RDA_TIMER timer;

    rda_timer_get(&timer);
    return timer.timer;
}

ulong	usec2ticks    (unsigned long usec)
{
	return (ulong)us_to_tick(usec);
}

ulong	ticks2usec    (unsigned long ticks)
{
	return tick_to_us(ticks);
}

ulong get_timer(ulong base)
{
	unsigned long long timer_diff;

	timer_diff = get_ticks() - gd->timer_reset_value;

	return (timer_diff / (gd->timer_rate_hz / CONFIG_SYS_HZ)) - base;
}

void __udelay(unsigned long usec)
{
    RDA_TIMER timer;

    rda_timeout_setup_us(&timer, usec);
    while(!rda_timeout_check(&timer))
        ;
}

/*
 * This function is derived from PowerPC code (timebase clock frequency).
 * On ARM it returns the number of timer ticks per second.
 */
ulong get_tbclk(void)
{
	return CONFIG_SYS_HZ_CLOCK;
}
