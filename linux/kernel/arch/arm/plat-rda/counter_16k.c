/*
 * counter_16k.c - RDA platform devices
 *
 * Copyright (C) 2013 RDA Microelectronics Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/clk.h>
#include <linux/err.h>
#include <linux/io.h>
#include <linux/clocksource.h>


#include <asm/mach/time.h>
#include <asm/sched_clock.h>

#include <mach/iomap.h>

/*
 * 16KHz clocksource ... always available, it is on the modem side
 */
static void __iomem *timer_16k_base;

static u32 notrace rda_16k_read_sched_clock(void)
{
	return timer_16k_base ? __raw_readl(timer_16k_base) : 0;
}

/**
 * rda_read_persistent_clock -  Return time from a persistent clock.
 *
 * Reads the time from a source which isn't disabled during PM, the
 * 16k sync timer.  Convert the cycles elapsed since last read into
 * nsecs and adds to a monotonically increasing timespec.
 */
static struct timespec persistent_ts;
static cycles_t cycles, last_cycles;
static unsigned int persistent_mult, persistent_shift;
static DEFINE_SPINLOCK(read_persistend_clock_lock);

void rda_read_persistent_clock(struct timespec *ts)
{
	unsigned long long nsecs;
	unsigned long flags;
	cycles_t delta;
	struct timespec *tsp = &persistent_ts;

	spin_lock_irqsave(&read_persistend_clock_lock, flags);

	last_cycles = cycles;
	cycles = timer_16k_base ? __raw_readl(timer_16k_base) : 0;
	delta = cycles - last_cycles;

	nsecs = clocksource_cyc2ns(delta, persistent_mult, persistent_shift);

	timespec_add_ns(tsp, nsecs);
	*ts = *tsp;

	spin_unlock_irqrestore(&read_persistend_clock_lock, flags);
}

int __init rda_clocksource_16k_init(void)
{
	static char err[] __initdata = KERN_ERR
		"%s: can't register clocksource!\n";

	u32 pbase;
	unsigned long size = SZ_1K;
	void __iomem *base;

	pbase = RDA_MODEM_CLOCK_PHYS;
	/* For this to work we must have a static mapping in io.c for this area */
	base = ioremap(pbase, size);
	if (!base)
		return -ENODEV;

	timer_16k_base = base + 0x14;

	/*
	 * 120000 rough estimate from the calculations in
	 * __clocksource_updatefreq_scale.
	 */
	clocks_calc_mult_shift(&persistent_mult, &persistent_shift,
			16384, NSEC_PER_SEC, 120000);

	if (clocksource_mmio_init(base, "16k_counter", 16384, 250, 32,
				clocksource_mmio_readl_up))
		printk(err, "16k_counter");

	setup_sched_clock(rda_16k_read_sched_clock, 32, 16384);
	register_persistent_clock(NULL, rda_read_persistent_clock);
	pr_info("rda clocksource : 16k counter at 16384 Hz\n");

	return 0;
}
