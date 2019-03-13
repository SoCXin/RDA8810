/*
 * arch/arm/plat-rda/cpuidle.c
 *
 * CPU idle driver for RDAmicro CPUs
 *
 * Copyright (c) 2014 RDA Micro, Inc.
 * Author: yingchunli<yingchunli@rdamicro.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/cpu.h>
#include <linux/cpuidle.h>
#include <linux/hrtimer.h>
#include <asm/proc-fns.h>
#include <linux/cpufreq.h>
#include <linux/delay.h>
#include <asm/cpuidle.h>
#include <asm/suspend.h>

#include <mach/iomap.h>
#include <plat/ap_clk.h>
#include <plat/intc.h>
#include <plat/pm_ddr.h>
#include <mach/board.h>

#if !defined(CONFIG_MACH_RDA8810E) && !defined(CONFIG_MACH_RDA8810H)
static DEFINE_PER_CPU(struct cpuidle_device, rda_idle_device);

static int rda_idle_enter_lp1(struct cpuidle_device *dev,
				struct cpuidle_driver *drv, int index);

#ifdef CONFIG_MACH_RDA8810
static int rda_idle_enter_lp2(struct cpuidle_device *dev,
				struct cpuidle_driver *drv, int index);
struct cpuidle_driver rda_idle_driver = {
	.name = "rda_idle",
	.owner = THIS_MODULE,
	.state_count = 3,
	.states = {
		[0] = ARM_CPUIDLE_WFI_STATE,
		[1] = {
			.enter			= rda_idle_enter_lp1,
			.exit_latency		= 200,
			.target_residency	= 1000,
			.power_usage		= 400,
			.flags			= CPUIDLE_FLAG_TIME_VALID,
			.name			= "LP1",
			.desc			= "CPU pll off",
		},
		[2] = {
			.disabled		= 0,
			.enter			= rda_idle_enter_lp2,
			.exit_latency		= 10000,
			.target_residency	= 20000,
			.power_usage		= 200,
			.flags			= CPUIDLE_FLAG_TIME_VALID,
			.name			= "LP2",
			.desc			= "CPU pll off, DDR off",
		},
	},
};

static int rda_idle_enter_lp2(struct cpuidle_device *dev,
	struct cpuidle_driver *drv, int index)
{
	uint32_t wakeup_mask = 0;

	if (!pm_ddr_idle_status()) {
		rda_idle_enter_lp1(dev, drv, index);
		return index;
	}

	if (!rda_gpu_idle_status()) {
		rda_idle_enter_lp1(dev, drv, index);
		return index;
	}

	wakeup_mask = rda_intc_get_mask();
	rda_intc_set_wakeup_mask(wakeup_mask);

	apsys_request_lp2(0);

	return index;
}
#endif

#ifdef CONFIG_MACH_RDA8850E
struct cpuidle_driver rda_idle_driver = {
	.name = "rda_idle",
	.owner = THIS_MODULE,
	.state_count = 2,
	.states = {
		[0] = ARM_CPUIDLE_WFI_STATE,
		[1] = {
			.enter			= rda_idle_enter_lp1,
			.exit_latency		= 200,
			.target_residency	= 1000,
			.power_usage		= 400,
			.flags			= CPUIDLE_FLAG_TIME_VALID,
			.name			= "LP1",
			.desc			= "CPU pll off",
		},
	},
};
#endif

static int rda_idle_enter_lp1(struct cpuidle_device *dev,
	struct cpuidle_driver *drv, int index)
{
	struct cpufreq_policy *policy;
	unsigned long old_freq;
	unsigned long min;

	policy = cpufreq_cpu_get(0);
	if (!policy)
		return index;

	/* setting cpu freq to the minist*/
	old_freq = policy->cur;
	min = policy->min;
	if (min != old_freq)
		apsys_adjust_cpu_clk_rate(min * 1000);

	apsys_cpupll_switch(0);
	cpu_do_idle();
	apsys_cpupll_switch(1);
	udelay(10);

	/*restore cpufreq if needed */
	if (old_freq != min)
		apsys_adjust_cpu_clk_rate(old_freq * 1000);
	cpufreq_cpu_put(policy);
	return index;
}



static int __init rda_cpuidle_init(void)
{
	int ret;
	unsigned int cpu;
	struct cpuidle_device *dev;
	struct cpuidle_driver *drv = &rda_idle_driver;

	ret = cpuidle_register_driver(&rda_idle_driver);
	if (ret) {
		pr_err("CPUidle driver registration failed\n");
		return ret;
	}

	for_each_possible_cpu(cpu) {
		dev = &per_cpu(rda_idle_device, cpu);
		dev->cpu = cpu;

		dev->state_count = drv->state_count;
		ret = cpuidle_register_device(dev);
		if (ret) {
			pr_err("CPU%u: CPUidle device registration failed\n",
				cpu);
			return ret;
		}
	}
	return 0;
}
device_initcall(rda_cpuidle_init);
#else
#include <plat/cpufreq.h>

/*
 * Hardware-limitation of rda8810e:
 *  - both CPUs clock share the same clock source and no seperate clock gating
 *  - ARM ARCH timer's freq is dynamic (just half of CPU freq) so can't
 *  be used in idle.
 *
 * For dual-core rda8810e, there is space to improve.
 * Current implementation is that CPU1 has only 1 idle state, WFI and CPU0 has
 * 3 states, WFI/LowPower1/LowPower2. The control is done by CPU0. When CPU1
 * wakeup first and found CPU0 in LP1/LP2 states, CPU1 will send IPI to CPU0
 * to wakeup the system
 *
 * Two problems with this approach:
 *   - if CPU1 goes to idle after CPU0, then only WFI state is used
 *   - CPU0 maybe wakeup more than needed and can only enter WFI after that
 *
 * Improvements that can be tried:
 *  - try coupled idle control.
 *  - dynamic control. the cpu that goes to idle secondly can set LP1 or LP2,
 *  and the first wakeup CPU can wakeup the system
 * */

typedef enum {
RDA_LP0_STATE = 0,
RDA_LP1_STATE,
RDA_LP2_STATE
}RDA_CPU_IDLE_STATES;

static DEFINE_PER_CPU(struct cpuidle_device, rda_idle_device);

static atomic_t idle_non_boot_cpus;
static volatile int cpu0_last_idle_state;

static int rda_idle_enter_lp1(struct cpuidle_device *dev,
				struct cpuidle_driver *drv, int index);
static int rda_idle_enter_lp2(struct cpuidle_device *dev,
				struct cpuidle_driver *drv, int index);

struct cpuidle_driver rda_idle_non_boot_driver = {
	.name = "rda_idle_non_boot",
	.owner = THIS_MODULE,
	.state_count = 1,
	.states = {
		[0] = ARM_CPUIDLE_WFI_STATE,
	},
};

struct cpuidle_driver rda_idle_cpu0_driver = {
	.name = "rda_idle",
	.owner = THIS_MODULE,
	.state_count = 3,
	.states = {
		[0] = ARM_CPUIDLE_WFI_STATE,
		[1] = {
			.enter			= rda_idle_enter_lp1,
			.exit_latency		= 200,
			.target_residency	= 1000,
			.power_usage		= 400,
			.flags			= CPUIDLE_FLAG_TIME_VALID,
			.name			= "LP1",
			.desc			= "CPU pll off",
		},
		[2] = {
			.disabled		= 0,
			.enter			= rda_idle_enter_lp2,
			.exit_latency		= 10000,
			.target_residency	= 20000,
			.power_usage		= 200,
			.flags			=  CPUIDLE_FLAG_TIME_VALID,
			.name			= "LP2",
			.desc			= "CPU pll off, DDR off",
		},
	},
};
//#define PL_DEBUG
#ifdef PL_DEBUG
#define rda_pm_debug rda_puts_no_irq
#else
static void rda_pm_debug(const char *fmt,...) {}
#endif


static unsigned long lp1_counter;

static void rda_idle_cpu0_do_lp0(void)
{
	cpu0_last_idle_state = RDA_LP0_STATE;
	cpu_do_idle();
}

static void rda_idle_non_boot_cpu_idle(void)
{
	atomic_inc(&idle_non_boot_cpus);
	cpu_do_idle();
	atomic_dec(&idle_non_boot_cpus);
	rmb();
	if (cpu0_last_idle_state != RDA_LP0_STATE)
		arch_send_call_function_single_ipi(0);
}

int rda_idle_lp0(struct cpuidle_device *dev)
{
	int cpu = dev->cpu;

	if (cpu == 0) {
		rda_idle_cpu0_do_lp0();
		return 0;
	}
	rda_idle_non_boot_cpu_idle();
	return 0;
}
static unsigned long lp1_old_freq;
static unsigned long lp1_min;

static int rda_idle_goto_lp1(void)
{
	struct cpufreq_policy *policy;

	policy = cpufreq_cpu_get(0);
	if (!policy) {
		lp1_old_freq = apsys_get_cpu_clk_rate();
		lp1_min = rda_get_cpufreq_min();
	} else {
		/* setting cpu freq to the minist*/
		lp1_old_freq = policy->cur * 1000;
		lp1_min = policy->min * 1000;
		cpufreq_cpu_put(policy);
	}

	if (lp1_min != lp1_old_freq)
		apsys_adjust_cpu_clk_rate(lp1_min);
	apsys_cpupll_switch(0);

	return 0;
}

static void rda_idle_wakeup_lp1(void)
{
	apsys_cpupll_switch(1);
	udelay(10);

	/*restore cpufreq if needed */
	if (lp1_old_freq != lp1_min)
		apsys_adjust_cpu_clk_rate(lp1_old_freq);
	//rda_dbg_pm("wakeup from lp1, freq is set to %ld\n", lp1_old_freq);
}

static void rda_idle_cpu0_do_lp1(void)
{

	rda_idle_goto_lp1();
	cpu0_last_idle_state = RDA_LP1_STATE;
	cpu_do_idle();
	cpu0_last_idle_state = RDA_LP0_STATE;
	wmb(); /*let the other CPU get this write*/
	rda_idle_wakeup_lp1();

	lp1_counter++;
	if ((lp1_counter & 0xff) == 0)
		rda_pm_debug("do lp1 %d times\n", lp1_counter);
}


static int rda_idle_enter_lp1(struct cpuidle_device *dev,
	struct cpuidle_driver *drv, int index)
{

	int nr_idle_non_boot_cpus;
	int ret;
	int cpu = smp_processor_id();

	if (cpu != dev->cpu)
		printk("what is going on?? cpu is %d, dev->cpu is %d",
				cpu, dev->cpu);

	nr_idle_non_boot_cpus = atomic_read(&idle_non_boot_cpus);
	if (nr_idle_non_boot_cpus != CONFIG_NR_CPUS -1) {
		rda_idle_cpu0_do_lp0();
		ret = 0;
	} else {
		rda_idle_cpu0_do_lp1();
		/* wakeup other cpus?
		arch_send_call_function_single_ipi(1);
		*/
		ret = 1;
	}
	return ret;
}


static void rda_idle_cpu0_do_lp2(struct cpuidle_device *dev)
{

	uint32_t wakeup_mask = 0;
	static unsigned long lp2_counter;
	wakeup_mask = rda_intc_get_mask();
	/* TODO - at least keypad can wakeup
	 * but we need to find all valid interrupt source
	 */
#if 0
	wakeup_mask |= 1<<RDA_WAKEUP_IRQ_KEYPAD |
			1<<RDA_SYS_IRQ_OS_TIMER |
			1<<RDA_IRQ_UART3;
#endif
	/*any irq should wakeup cpu*/
	wakeup_mask = 0xFFFFFFFF;
	rda_intc_set_wakeup_mask(wakeup_mask);
	cpu0_last_idle_state = RDA_LP2_STATE;
	wmb();
#ifndef CONFIG_RDA_FPGA
#ifdef CONFIG_MACH_RDA8810
	apsys_request_lp2(0);
#endif
#endif
	cpu0_last_idle_state = RDA_LP0_STATE;
	wmb(); /*let the other CPU get this write*/
	lp2_counter++;
	if ((lp2_counter & 0xff) == 0)
		printk(KERN_INFO"do lp2 %lu times\n", lp2_counter);
}

static int rda_idle_enter_lp2(struct cpuidle_device *dev,
	struct cpuidle_driver *drv, int index)
{
	int nr_idle_non_boot_cpus;
	int ret;
	int cpu = smp_processor_id();

	if (cpu != dev->cpu)
		printk("what is going on?? cpu is %d, dev->cpu is %d",
				cpu, dev->cpu);
	nr_idle_non_boot_cpus = atomic_read(&idle_non_boot_cpus);
	if (nr_idle_non_boot_cpus != CONFIG_NR_CPUS - 1) {
		rda_idle_cpu0_do_lp0();
		ret = 0;
	} else {
		if (!pm_ddr_idle_status() || !rda_gpu_idle_status()) {
			rda_idle_cpu0_do_lp1();
			ret = 1;
		} else {
			rda_idle_cpu0_do_lp2(dev);
			ret = 2;
		}
	}

	return ret;
}

static int __init rda_cpuidle_init(void)
{
	int ret;
	unsigned int cpu;
	struct cpuidle_device *dev;
#if 0
	struct cpuidle_driver *drv = &rda_idle_driver;
	ret = cpuidle_register_driver(&rda_idle_driver);
	if (ret) {
		pr_err("CPUidle driver registration failed\n");
		return ret;
	}
#endif
	atomic_set(&idle_non_boot_cpus, 0);
	for_each_possible_cpu(cpu) {
		struct cpuidle_driver *drv;
		if (cpu == 0) {
			drv = &rda_idle_cpu0_driver;
		} else {
			drv = &rda_idle_non_boot_driver;
		}
		ret = cpuidle_register_cpu_driver(drv, cpu);
		if (ret) {
			pr_err("CPU%u: CPUidle driver registration failed %d\n",
				cpu, ret);
			return ret;
		}
		dev = &per_cpu(rda_idle_device, cpu);
		dev->cpu = cpu;

		dev->state_count = drv->state_count;
		if (cpu != 0)
			dev->state_count = 1;
		ret = cpuidle_register_device(dev);
		if (ret) {
			pr_err("CPU%u: CPUidle device registration failed\n",
				cpu);
			return ret;
		}

	}
	return 0;
}
device_initcall(rda_cpuidle_init);
#endif


