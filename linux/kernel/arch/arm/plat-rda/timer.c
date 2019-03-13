#include <linux/clockchips.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/platform_device.h>

#include <asm/io.h>
#include <asm/mach/time.h>

#include <mach/hardware.h>
#include <plat/reg_timer.h>
#include <plat/rda_debug.h>

static unsigned long rda_clk_rate = CLOCK_TICK_RATE;

/* in RDA8810E
 * TIMER2 2091_2000h 2091_2FFFh OStimer, general purpose timer. 4kB Slave1
 * TIMER1 2091_1000h 2091_1FFFh OStimer, general purpose timer. 4kB Slave1
 * TIMER  2091_0000h 2091_0FFFh OStimer, general purpose timer. 4kB Slave1
 * TIMER is used for cpu0
 * TIMER1 is used for cpu1
 * we don't use arch_time as it is not standard implementation and cause trouble
 * in cpuidle , which I can't solve yet..
*/
static cycle_t rda_hwtimer_read(struct clocksource *cs)
{
	unsigned int lo, hi;

	HWP_TIMER_AP_T *hwp_apTimer  = ((HWP_TIMER_AP_T*)IO_ADDRESS(RDA_TIMER_BASE));
	/* always read low 32bit first */
	lo = hwp_apTimer->HWTimer_LockVal_L;
	hi = hwp_apTimer->HWTimer_LockVal_H;

	return ((cycle_t)hi << 32) | lo;
}

static struct clocksource rda_clocksource = {
	.name           = "rda_timer",
	.rating         = 400,
	.read           = rda_hwtimer_read,
	.mask           = CLOCKSOURCE_MASK(64),
	.flags          = CLOCK_SOURCE_IS_CONTINUOUS,
};

static void __init rda_clocksource_init(void)
{
	if (clocksource_register_hz(&rda_clocksource, rda_clk_rate))
		panic("%s: can't register clocksource\n",
			rda_clocksource.name);
}

static int rda_ostimer_start(enum clock_event_mode mode,
				    unsigned long cycles)
{
	u32 ostimer_ctrl, ostimer_load_l;

	HWP_TIMER_AP_T *hwp_apTimer  =
	       	((HWP_TIMER_AP_T*)IO_ADDRESS(RDA_TIMER_BASE));
	rda_dbg_mach("cpu #%d ostimer_set_start: mode = %d, cycles = %ld\n",
		smp_processor_id(), mode, cycles);

	ostimer_load_l = (u32)cycles;
	ostimer_ctrl = TIMER_AP_OS_LOADVAL_H(0);
	ostimer_ctrl |= (TIMER_AP_ENABLE | TIMER_AP_LOAD);
	if (mode == CLOCK_EVT_MODE_PERIODIC)
		ostimer_ctrl |= TIMER_AP_REPEAT;

	/* enable ostimer interrupt first */
	hwp_apTimer->Timer_Irq_Mask_Set = TIMER_AP_OSTIMER_MASK;

	/* write low 32bit first, high 24bit are with ctrl */
	hwp_apTimer->OSTimer_LoadVal_L = ostimer_load_l;
	hwp_apTimer->OSTimer_Ctrl = ostimer_ctrl;

	return 0;
}

static int rda_ostimer_stop(void)
{
	HWP_TIMER_AP_T *hwp_apTimer  =
	       	((HWP_TIMER_AP_T*)IO_ADDRESS(RDA_TIMER_BASE));
	rda_dbg_mach("ostimer_set_stop\n");

	/* disable ostimer interrupt first */
	hwp_apTimer->Timer_Irq_Mask_Set = 0;

	hwp_apTimer->OSTimer_Ctrl = 0;

	return 0;
}

static int rda_ostimer_set_next_event(unsigned long cycles,
				       struct clock_event_device *evt)
{
	rda_dbg_mach("timer_set_next: %ld\n", cycles);

	rda_ostimer_start(evt->mode, cycles);

	return 0;
}

/*
static uint32_t rda_jiffies_per_tick = CLOCK_TICK_RATE / (HZ);
*/

static void rda_ostimer_set_mode(enum clock_event_mode mode,
				  struct clock_event_device *evt)
{
	unsigned long cycles_per_jiffy;

	rda_dbg_mach("timer_set_mode: %d, HZ = %d\n", mode, (HZ));

	rda_ostimer_stop();

	switch (mode) {
	case CLOCK_EVT_MODE_PERIODIC:
		cycles_per_jiffy =
			(((unsigned long long) NSEC_PER_SEC / HZ * evt->mult) >> evt->shift);
		rda_ostimer_start(mode, cycles_per_jiffy);
		break;

	case CLOCK_EVT_MODE_ONESHOT:
	case CLOCK_EVT_MODE_UNUSED:
	case CLOCK_EVT_MODE_SHUTDOWN:
	case CLOCK_EVT_MODE_RESUME:
		break;
	}
}

static struct clock_event_device rda_clockevent = {
	.name           = "rda_timer",
	.features       = CLOCK_EVT_FEAT_PERIODIC | CLOCK_EVT_FEAT_ONESHOT,
	.rating         = 250,
	.set_next_event = rda_ostimer_set_next_event,
	.set_mode       = rda_ostimer_set_mode,
};

static irqreturn_t rda_ostimer_interrupt(int irq, void *dev_id)
{
	struct clock_event_device *evt = dev_id;

	HWP_TIMER_AP_T *hwp_apTimer  =
	       	((HWP_TIMER_AP_T*)IO_ADDRESS(RDA_TIMER_BASE));
	/* clr timer int */
	hwp_apTimer->Timer_Irq_Clr = TIMER_AP_OSTIMER_MASK;

	rda_dbg_mach("timer_interrupt\n");

	if (evt->event_handler)
		evt->event_handler(evt);
	return IRQ_HANDLED;
}

static struct irqaction rda_timer_irq = {
	.name		= "rda_ostimer_irq",
	.flags		= IRQF_DISABLED | IRQF_TIMER,
	.handler	= rda_ostimer_interrupt,
	.dev_id	= &rda_clockevent,
};

static void rda_clockevent_init(void)
{
	rda_clockevent.cpumask = cpumask_of(0),
	setup_irq(RDA_IRQ_TIMEROS, &rda_timer_irq);
	irq_force_affinity(RDA_IRQ_TIMEROS, cpumask_of(0));
	clockevents_config_and_register(&rda_clockevent,
		rda_clk_rate, 2, 0xffffffffU);
}

static void __init rda_timer_resources(void)
{
	rda_clk_rate = CLOCK_TICK_RATE;
}

#if defined(CONFIG_MACH_RDA8810E)
#define RDA_TIMER1_BASE (RDA_TIMER_BASE + 0x1000)
#endif

#if defined(CONFIG_MACH_RDA8810E) || defined(CONFIG_MACH_RDA8810H)
/*TODO - merge the functions...*/
#if 0
static cycle_t rda_hwtimer1_read(struct clocksource *c)
{
	unsigned int lo, hi;

	HWP_TIMER_AP_T *hwp_apTimer  =
	       ((HWP_TIMER_AP_T*)IO_ADDRESS(RDA_TIMER1_BASE));
	/* always read low 32bit first */
	lo = hwp_apTimer->HWTimer_LockVal_L;
	hi = hwp_apTimer->HWTimer_LockVal_H;;

	return ((cycle_t)hi << 32) | lo;
}
static struct clocksource rda_clocksource1 = {
	.name           = "rda_timer1",
	.rating         = 400,
	.read           = rda_hwtimer1_read,
	.mask           = CLOCKSOURCE_MASK(64),
	.flags          = CLOCK_SOURCE_IS_CONTINUOUS,
};
static void __init rda_clocksource1_init(void)
{
	if (clocksource_register_hz(&rda_clocksource1, rda_clk_rate))
		panic("%s: can't register clocksource\n",
			rda_clocksource1.name);
}
#endif

static int rda_ostimer1_start(enum clock_event_mode mode,
				    unsigned long cycles)
{
	u32 ostimer_ctrl, ostimer_load_l;

	HWP_TIMER_AP_T *hwp_apTimer  =
		((HWP_TIMER_AP_T*)IO_ADDRESS(RDA_TIMER1_BASE));
	rda_dbg_mach("cpu #%d ostimer1_set_start: mode = %d, cycles = %ld\n",
		smp_processor_id(), mode, cycles);

	ostimer_load_l = (u32)cycles;
	ostimer_ctrl = TIMER_AP_OS_LOADVAL_H(0);
	ostimer_ctrl |= (TIMER_AP_ENABLE | TIMER_AP_LOAD);
	if (mode == CLOCK_EVT_MODE_PERIODIC)
		ostimer_ctrl |= TIMER_AP_REPEAT;

	/* enable ostimer interrupt first */
	hwp_apTimer->Timer_Irq_Mask_Set = TIMER_AP_OSTIMER_MASK;

	/* write low 32bit first, high 24bit are with ctrl */
	hwp_apTimer->OSTimer_LoadVal_L = ostimer_load_l;
	hwp_apTimer->OSTimer_Ctrl = ostimer_ctrl;

	return 0;
}

static int rda_ostimer1_stop(void)
{
	HWP_TIMER_AP_T *hwp_apTimer  =
		((HWP_TIMER_AP_T*)IO_ADDRESS(RDA_TIMER1_BASE));
	rda_dbg_mach("ostimer1_set_stop\n");

	/* disable ostimer interrupt first */
	hwp_apTimer->Timer_Irq_Mask_Set = 0;

	hwp_apTimer->OSTimer_Ctrl = 0;

	return 0;
}

static int rda_ostimer1_set_next_event(unsigned long cycles,
				       struct clock_event_device *evt)
{
	rda_dbg_mach("timer1_set_next: %ld\n", cycles);

	rda_ostimer1_start(evt->mode, cycles);

	return 0;
}

/*
static uint32_t rda_jiffies_per_tick = CLOCK_TICK_RATE / (HZ);
*/

static void rda_ostimer1_set_mode(enum clock_event_mode mode,
				  struct clock_event_device *evt)
{
	unsigned long cycles_per_jiffy;

	rda_dbg_mach("timer1_set_mode: %d, HZ = %d cpu #%d\n", mode, (HZ), smp_processor_id());

	rda_ostimer1_stop();

	switch (mode) {
	case CLOCK_EVT_MODE_PERIODIC:
		cycles_per_jiffy =
			(((unsigned long long) NSEC_PER_SEC / HZ * evt->mult) >> evt->shift);
		rda_ostimer1_start(mode, cycles_per_jiffy);
		break;

	case CLOCK_EVT_MODE_ONESHOT:
	case CLOCK_EVT_MODE_UNUSED:
	case CLOCK_EVT_MODE_SHUTDOWN:
	case CLOCK_EVT_MODE_RESUME:
		break;
	}
}

static struct clock_event_device rda_clockevent1 = {
	.name           = "rda_timer1",
	.features       = CLOCK_EVT_FEAT_ONESHOT,
	.rating         = 250,
	.set_next_event = rda_ostimer1_set_next_event,
	.set_mode       = rda_ostimer1_set_mode,
};

#include <asm/cputype.h>
static irqreturn_t rda_ostimer1_interrupt(int irq, void *dev_id)
{
	struct clock_event_device *evt = dev_id;

	HWP_TIMER_AP_T *hwp_apTimer  =
		((HWP_TIMER_AP_T*)IO_ADDRESS(RDA_TIMER1_BASE));
	/* clr timer int */
	hwp_apTimer->Timer_Irq_Clr = TIMER_AP_OSTIMER_MASK;

	rda_dbg_mach("timer1_interrupt, #%u\n", read_cpuid_mpidr());

	if (evt->event_handler)
		evt->event_handler(evt);
	return IRQ_HANDLED;
}

static struct irqaction rda_timer1_irq = {
	.name		= "rda_ostimer1_irq",
	.flags		= IRQF_DISABLED | IRQF_TIMER,
	.handler	= rda_ostimer1_interrupt,
	.dev_id	= &rda_clockevent1,
};

static void rda_clockevent1_init(void)
{
	int cpu;

	cpu = smp_processor_id();
	rda_clockevent1.cpumask = cpumask_of(cpu);
	if (irq_force_affinity(RDA_IRQ_TIMEROS2, cpumask_of(cpu))) {
		panic("%s can't set up irq affinity for cpu #%d\n",
			__func__, smp_processor_id());
	}
	clockevents_config_and_register(&rda_clockevent1,
		rda_clk_rate, 2, 0xffffffffU);
}

#include <linux/cpu.h>
/* These function needs to be called from CPU1 */
static int __cpuinit rda_timer_cpu_notify(struct notifier_block *self,
					   unsigned long action, void *hcpu)
{
	/*
	 * Grab cpu pointer in each case to avoid spurious
	 * preemptible warnings
	 */
	switch (action & ~CPU_TASKS_FROZEN) {
	case CPU_STARTING:
		/*TODO - test only..*/
		//rda_clocksource1_init();
		rda_clockevent1_init();
		break;
	case CPU_DYING:
		break;
	}

	return NOTIFY_OK;
}

static struct notifier_block rda_timer_cpu_nb __cpuinitdata = {
	.notifier_call = rda_timer_cpu_notify,
};
#endif

extern int __init rda_clocksource_16k_init(void);
void __init rda_timer_init(void)
{
	rda_dbg_mach("rda_timer_init\n");

	rda_timer_resources();
	rda_clocksource_init();
	rda_clockevent_init();
#if defined(CONFIG_MACH_RDA8810E) || defined(CONFIG_MACH_RDA8810H)
	setup_irq(RDA_IRQ_TIMEROS2, &rda_timer1_irq);
	register_cpu_notifier(&rda_timer_cpu_nb);
#endif
	rda_clocksource_16k_init();
}


/*
struct sys_timer rda_timer = {
	.init		= rda_timer_init,
};
*/
