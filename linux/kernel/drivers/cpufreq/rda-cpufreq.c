/*
 * rda-cpufreq.c - RDA SoCs cpu frequency driver
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
#include <linux/module.h>
#include <linux/types.h>
#include <linux/cpufreq.h>
#include <linux/delay.h>
#include <linux/init.h>
#include <linux/err.h>
#include <linux/clk.h>
#include <linux/io.h>
#include <linux/suspend.h>
#include <mach/rda_clk_name.h>
#include <rda/tgt_ap_clock_config.h>
#include <plat/ap_clk.h>
#include <plat/md_sys.h>


#define NUM_CPUS		CONFIG_NR_CPUS

/**
 * 8810E - dual core platform, hardware limits
 * 1. CPU freq can't be set independently
 */


/* Frequency table index must be sequential starting at 0 */
static struct cpufreq_frequency_table freq_table[] = {
	/*
	 * CAUTION:
	 * CPU frequency range can NOT be too large, otherwise VCORE
	 * fluctuation might impact DDR.
	 */
	{ 0, PLL_CPU_FREQ / 3 * 1 / 1000 },
	{ 1, PLL_CPU_FREQ / 5 * 2 / 1000 },
	{ 2, PLL_CPU_FREQ / 2 * 1 / 1000 },
	{ 3, PLL_CPU_FREQ / 1 * 1 / 1000 },
	{ 4, CPUFREQ_TABLE_END },
};
#define FREQ_TABLE_SIZE ARRAY_SIZE(freq_table)

#define HIGHEST_FREQ_INDEX (FREQ_TABLE_SIZE -2)

#ifdef CONFIG_RDA_AP_PLL_FREQ_ADJUST
#if PLL_CPU_FREQ == PLL_CPU_FREQ_LOW
#undef CONFIG_RDA_AP_PLL_FREQ_ADJUST
#endif
#endif

#ifdef CONFIG_RDA_AP_PLL_FREQ_ADJUST
/*TODO - if more high temperature message keep coming, highest freq can be
 * set lower
 * */
static struct cpufreq_frequency_table low_freq_table[] = {
	/*
	 * CAUTION:
	 * CPU frequency range can NOT be too large, otherwise VCORE
	 * fluctuation might impact DDR.
	 */
	{ 0, PLL_CPU_FREQ_LOW / 3 * 1 / 1000 },
	{ 1, PLL_CPU_FREQ_LOW / 5 * 2 / 1000 },
	{ 2, PLL_CPU_FREQ_LOW / 2 * 1 / 1000 },
	{ 3, PLL_CPU_FREQ_LOW / 1 * 1 / 1000 },
	{ 4, CPUFREQ_TABLE_END },
};
#endif

static struct cpufreq_frequency_table *cur_freq_table = &(freq_table[0]);

/* ladder freq is 500M Hz */
static unsigned int ladder_index = 2;


static struct clk *cpu_clk = NULL;

static unsigned long target_cpu_index[NUM_CPUS];
static struct msys_device *cpu_msys_dev = NULL;

static DEFINE_MUTEX(rda_cpu_lock);



static int rda_verify_speed(struct cpufreq_policy *policy)
{
	return cpufreq_frequency_table_verify(policy, cur_freq_table);
}

static unsigned int rda_get_speed(unsigned int cpu)
{
	return clk_get_rate(cpu_clk) / 1000;
}

static unsigned int rda_get_index(unsigned int rate)
{
	int i;

	for (i = 0; cur_freq_table[i].frequency != CPUFREQ_TABLE_END &&
			i < FREQ_TABLE_SIZE -1; i++) {
		if (rate <= cur_freq_table[i].frequency)
			return i;
	}
	return HIGHEST_FREQ_INDEX;
}

static unsigned long rda_cpu_highest_index(void)
{
	unsigned long index = 0;
	int i;

	for_each_online_cpu(i) {
		index = max(index, target_cpu_index[i]);
	}

	return index;
}

static void rda_update_cpu_speed(struct cpufreq_policy *policy)
{
	int ret;
	struct cpufreq_freqs freqs;
	int old, new;

	old = rda_get_index(rda_get_speed(0));
	new = rda_cpu_highest_index();

	freqs.new = cur_freq_table[new].frequency;
	freqs.old = cur_freq_table[old].frequency;
	pr_debug("cpu-rda: transition: %u KHz (old %d) --> %u KHz(new %d)\n",
			freqs.old,old, freqs.new, new);

	if (old == new)
		return;


	for_each_online_cpu(freqs.cpu) {
		cpufreq_notify_transition(policy, &freqs, CPUFREQ_PRECHANGE);
	}


	if (new > old) {
		/* if boost CPU frequency, it must be step by step */
		int index = old + 1;
		for (; index <= new; index++) {
			freqs.new = cur_freq_table[index].frequency;
			ret = clk_set_rate(cpu_clk, freqs.new * 1000);
			if (ret) {
				pr_err("cpu-rda: Failed to set cpu frequency to %d kHz\n",
						freqs.new);
				return;
			}
			udelay(10);
		}
	} else {
		int index = old - 1;
		for (; index >= new; index--) {
			freqs.new = cur_freq_table[index].frequency;
			ret = clk_set_rate(cpu_clk, freqs.new * 1000);
			if (ret) {
				pr_err("cpu-rda: Failed to set cpu frequency to %d kHz\n",
						freqs.new);
				return;
			}
			udelay(10);
		}
	}

	for_each_online_cpu(freqs.cpu) {
		cpufreq_notify_transition(policy, &freqs, CPUFREQ_POSTCHANGE);
	}
}

static int rda_target(struct cpufreq_policy *policy,
		       unsigned int target_freq,
		       unsigned int relation)
{
	unsigned int idx;
	int ret = 0;

	mutex_lock(&rda_cpu_lock);

	ret = cpufreq_frequency_table_target(policy, cur_freq_table, target_freq,
		relation, &idx);
	if (ret) {
		pr_err("cpu%d: no freq match for %d(ret=%d)\n",
			policy->cpu, target_freq, ret);
		goto out;
	}

	target_cpu_index[policy->cpu] = idx;

	rda_update_cpu_speed(policy);
out:
	mutex_unlock(&rda_cpu_lock);

	return ret;
}

static int rda_cpu_init(struct cpufreq_policy *policy)
{
	int ret = 0;
	unsigned int index;

	if (policy->cpu >= NUM_CPUS)
		return -EINVAL;

	pr_info("initialize cpu frequency for cpu %u\n", policy->cpu);

	if (!cpu_clk) {
		cpu_clk = clk_get(NULL, RDA_CLK_CPU);
		if (IS_ERR(cpu_clk)) {
			ret = PTR_ERR(cpu_clk);
			cpu_clk = NULL;
			pr_info("cpu clock get error %d\n", ret);
			goto out;
		}
		clk_prepare_enable(cpu_clk);

		cpufreq_frequency_table_cpuinfo(policy, cur_freq_table);
		cpufreq_frequency_table_get_attr(cur_freq_table, policy->cpu);
	}

	index = rda_get_index(rda_get_speed(0));
	target_cpu_index[policy->cpu] = index;
	policy->cur = cur_freq_table[index].frequency;
	/* FIXME: what's the actual transition time? */
	policy->cpuinfo.transition_latency = 300 * 1000;

	policy->shared_type = CPUFREQ_SHARED_TYPE_ALL;
	cpumask_copy(policy->related_cpus, cpu_possible_mask);

out:
	if (ret) {
		if (cpu_clk) {
			clk_put(cpu_clk);
			cpu_clk = NULL;
		}
	}

	return ret;
}

static int rda_cpu_exit(struct cpufreq_policy *policy)
{
	if (cpu_clk) {
		clk_put(cpu_clk);
		cpu_clk = NULL;
	}
	return 0;
}

static struct freq_attr *rda_cpufreq_attr[] = {
	&cpufreq_freq_attr_scaling_available_freqs,
	NULL,
};

static struct cpufreq_driver rda_cpufreq_driver = {
	.flags 		= CPUFREQ_STICKY,
	.verify		= rda_verify_speed,
	.target		= rda_target,
	.get		= rda_get_speed,
	.init		= rda_cpu_init,
	.exit		= rda_cpu_exit,
	.name		= "rda",
	.attr		= rda_cpufreq_attr,
};

unsigned long rda_get_cpufreq_min(void)
{
	static unsigned long min;

	if (unlikely(!min))
		min = cur_freq_table[0].frequency * 1000;
	return min;
}

unsigned long rda_get_cpufreq_ladder(void)
{
	static unsigned long ladder;

	if (unlikely(!ladder)) {
		BUG_ON(ladder_index >= (FREQ_TABLE_SIZE - 1));
		ladder = cur_freq_table[ladder_index].frequency * 1000;
	}
	return ladder;
}

#ifdef CONFIG_RDA_AP_PLL_FREQ_ADJUST
/*
 * adjust ap PLL to use lower freq
 */
static int rda_ap_clock_adjust(int high_freq)
{
	struct cpufreq_freqs freqs;
	struct cpufreq_policy *policy = cpufreq_cpu_get(0);

	if ((cur_freq_table ==  freq_table) && (high_freq))
		return 0;

	if ((cur_freq_table == low_freq_table) && (high_freq == 0))
		return 0;

	pr_debug("%s, change to %s pll", __func__, high_freq?"high":"low");
	freqs.old = rda_get_speed(0);
	if (high_freq == 0) {
		apsys_ap_pll_adjust(0);
		cur_freq_table = low_freq_table;
	} else {
		apsys_ap_pll_adjust(1);
		cur_freq_table = freq_table;
	}
	policy->user_policy.max = cur_freq_table[HIGHEST_FREQ_INDEX].frequency;
	policy->user_policy.min = cur_freq_table[0].frequency;
	freqs.new = policy->cur = rda_get_speed(0);

	cpufreq_frequency_table_cpuinfo(policy, cur_freq_table);

	for_each_online_cpu(freqs.cpu) {
		cpufreq_frequency_table_get_attr(cur_freq_table, freqs.cpu);
		cpufreq_notify_transition(policy, &freqs, CPUFREQ_POSTCHANGE);
	}

	(void)cpufreq_update_policy(0);
	return 0;

}
#else
/*
 * Can't adjust ap pll, so limit the highest freq
 * */
static int rda_ap_clock_adjust(int high_freq)
{
	struct cpufreq_policy *policy = cpufreq_cpu_get(0);
	if (high_freq == 0)
		policy->user_policy.max = freq_table[HIGHEST_FREQ_INDEX - 1].frequency;
	else
		policy->user_policy.max = freq_table[HIGHEST_FREQ_INDEX].frequency;

	(void)cpufreq_update_policy(0);
	return 0;
}
#endif


static int rda_set_cpufreq_max(struct notifier_block *nb, unsigned long val,
								void *data)
{
	struct client_mesg *pmesg = (struct client_mesg *)data;
	int temperature = 0, high_temperature = 0;
	u32 pm_data = 0;

	if (pmesg->mod_id != SYS_PM_MOD) {
		//pr_err("%s: The mod id is error \n", __FUNCTION__);
		return NOTIFY_DONE;
	}

	switch (val) {
	case SYS_PM_MESG_CHIP_TEMP_STATUS:
		pm_data  = *((unsigned int*)&(pmesg->param));
		temperature = (int)(pm_data & 0xFFFF);
		high_temperature = ((int)(pm_data & (0xFFFF << 16))) >> 16;
		//pr_info("%s: Temperature = %d, hight_temperature = %d\n",__FUNCTION__,temperature,high_temperature);

		printk(KERN_WARNING"%s got %s temperature message", __func__,
					high_temperature?"high":"low");
		rda_ap_clock_adjust(high_temperature == 0);
		break;

	default:
		return NOTIFY_DONE;
	}


	return NOTIFY_OK;
}

static ssize_t rda_ap_pll_freq_show(struct kobject *kobj,
			struct kobj_attribute *attr, char *buf)
{
	return sprintf(buf, "%s\n", cur_freq_table == freq_table ? "high" : "low");
}

static ssize_t rda_ap_pll_freq_store(struct kobject *kobj,
		struct kobj_attribute *attr, const char *buf, size_t n)
{
	int value;

	if (sscanf(buf, "%d", &value) != 1)
		return -EINVAL;

	if (value)
		value = 1;
	else
		value = 0;

	rda_ap_clock_adjust(value);
	return n;
}

static struct kobj_attribute rda_ap_pll_freq_attr =
	__ATTR(rda_ap_pll_freq, 0664, rda_ap_pll_freq_show, rda_ap_pll_freq_store);

int __init rda_cpu_freq_init_sysfs(void)
{
	int error = 0;
	error = sysfs_create_file(kernel_kobj, &rda_ap_pll_freq_attr.attr);
	if (error)
		printk(KERN_ERR "sysfs_create_file rda_ap_pll_freq failed: %d\n", error);
	return error;
}
static int __init rda_cpufreq_init(void)
{

	// ap <---> modem temperature controll
	cpu_msys_dev = rda_msys_alloc_device();
	if (!cpu_msys_dev) {
		return -EINVAL;;
	}

	//pr_info("%s: register ap bp communication\n",__FUNCTION__);
	cpu_msys_dev->module = SYS_PM_MOD;
	cpu_msys_dev->name = "rda-cpu-freq";
	cpu_msys_dev->notifier.notifier_call = rda_set_cpufreq_max;

	rda_msys_register_device(cpu_msys_dev);
	rda_cpu_freq_init_sysfs();
	return cpufreq_register_driver(&rda_cpufreq_driver);
}

static void __exit rda_cpufreq_exit(void)
{
	rda_msys_unregister_device(cpu_msys_dev);
	rda_msys_free_device(cpu_msys_dev);
	cpufreq_unregister_driver(&rda_cpufreq_driver);
}

MODULE_AUTHOR("Yingchun Li <yingchunli@rdamicro.com>");
MODULE_DESCRIPTION("cpufreq driver for RDA SoCs");
MODULE_LICENSE("GPL");
module_init(rda_cpufreq_init);
module_exit(rda_cpufreq_exit);
