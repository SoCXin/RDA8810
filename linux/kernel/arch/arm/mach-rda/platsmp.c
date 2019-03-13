#include <linux/init.h>
#include <linux/errno.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/jiffies.h>
#include <linux/smp.h>
#include <linux/io.h>

#include <asm/cacheflush.h>
#include <asm/smp_plat.h>
#include <asm/smp_scu.h>
#include <asm/firmware.h>
#include <mach/hardware.h>
#include <plat/ap_clk.h>

extern void rda_secondary_startup(void);

#define RDA_SMP_REG_BASE	(RDA_PD_CTRL_BASE + 0x100)
#define RDA_SMP_START_MAGIC	0xC8D1E000
/*
 * SMP jump flags and addresses (right now in SRAM)
 * off = 0x00, flag 1, off = 0x04, addr 1
 * off = 0x08, flag 2, off = 0x0c, addr 2
 * off = 0x10, flag 3, off = 0x14, addr 3
 */
#define RDA_SMP_REG_SIZE	8
#define RDA_SMP_REG_OFF_FLAG	0
#define RDA_SMP_REG_OFF_ADDR	4

static inline void __iomem *cpu_boot_reg_base(void)
{
	return (void __iomem *)(RDA_SMP_REG_BASE);
}

static inline void __iomem *cpu_boot_reg(int cpu)
{
	void __iomem *boot_reg;

	boot_reg = cpu_boot_reg_base();
	boot_reg += RDA_SMP_REG_SIZE * (cpu - 1);
	return boot_reg;
}

static inline void __iomem *cpu_boot_reg_addr(int cpu)
{
	return cpu_boot_reg(cpu) + RDA_SMP_REG_OFF_ADDR;
}

static inline void __iomem *cpu_boot_reg_flag(int cpu)
{
	return cpu_boot_reg(cpu) + RDA_SMP_REG_OFF_FLAG;
}

/*
 * Write pen_release in a way that is guaranteed to be visible to all
 * observers, irrespective of whether they're taking part in coherency
 * or not.  This is necessary for the hotplug code to work reliably.
 */
static void write_pen_release(int val)
{
	pen_release = val;
	smp_wmb();
	__cpuc_flush_dcache_area((void *)&pen_release, sizeof(pen_release));
	outer_clean_range(__pa(&pen_release), __pa(&pen_release + 1));
}

static DEFINE_SPINLOCK(boot_lock);

static void __cpuinit rda_secondary_init(unsigned int cpu)
{
	unsigned long phys_cpu = cpu_logical_map(cpu);

	printk(KERN_INFO "SMP: smp_secondary_init for cpu %d, phys_cpu = %d\n",
		(int)cpu, (int)phys_cpu);
	/*
	 * let the primary processor know we're out of the
	 * pen, then head off into the C entry point
	 */
	write_pen_release(-1);

	/*
	 * Synchronise with the boot thread.
	 */
	spin_lock(&boot_lock);
	spin_unlock(&boot_lock);
}

static inline void __cpuinit rda_prepare_secondary(unsigned int phys_cpu)
{

	unsigned long boot_addr;
	unsigned long boot_magic;


	boot_addr = virt_to_phys(rda_secondary_startup);
	boot_magic = RDA_SMP_START_MAGIC;

	__raw_writel(boot_addr, cpu_boot_reg_addr(phys_cpu));
	__raw_writel(boot_magic, cpu_boot_reg_flag(phys_cpu));

	smp_wmb();
}

static int __cpuinit rda_boot_secondary(unsigned int cpu, struct task_struct *idle)
{
	unsigned long timeout;
	unsigned long phys_cpu = cpu_logical_map(cpu);
	int i;


	printk(KERN_INFO "SMP: smp_boot_secondary runnning cpu id  0x%x, cpu = %d\n",
		read_cpuid_mpidr(), (int)phys_cpu);

	/*
	 * Set synchronisation state between this boot processor
	 * and the secondary one
	 */
	spin_lock(&boot_lock);


#if 0
	if (!(__raw_readl(S5P_ARM_CORE1_STATUS) & S5P_CORE_LOCAL_PWR_EN)) {
		__raw_writel(S5P_CORE_LOCAL_PWR_EN,
			     S5P_ARM_CORE1_CONFIGURATION);

		timeout = 10;

		/* wait max 10 ms until cpu1 is on */
		while ((__raw_readl(S5P_ARM_CORE1_STATUS)
			& S5P_CORE_LOCAL_PWR_EN) != S5P_CORE_LOCAL_PWR_EN) {
			if (timeout-- == 0)
				break;

			mdelay(1);
		}

		if (timeout == 0) {
			printk(KERN_ERR "cpu1 power enable failed");
			spin_unlock(&boot_lock);
			return -ETIMEDOUT;
		}
	}
#endif

	for (i = 0; i < 2; i++) {
		/*
		 * The secondary processor is waiting to be released from
		 * the holding pen - release it, then wait for it to flag
		 * that it has been released by resetting pen_release.
		 *
		 * Note that "pen_release" is the hardware CPU ID, whereas
		 * "cpu" is Linux's internal ID.
		 */
		write_pen_release(phys_cpu);

		rda_prepare_secondary(phys_cpu);

		/* reset the core first */
		apsys_reset_cpu(phys_cpu);

		timeout = jiffies + (1 * HZ);
		while (time_before(jiffies, timeout)) {

		/*
		 * Send the secondary CPU a soft interrupt, thereby causing
		 * the boot monitor to read the system wide flags register,
		 * and branch to the address found there.
		 */
			/* The secondary CPU is in WFE state */
			//arch_send_wakeup_ipi_mask(cpumask_of(cpu));
			dsb_sev();

			smp_rmb();
			if (pen_release == -1)
				break;

			mdelay(10);
		}

		if (pen_release == -1)
			break;
	}
	/*
	 * now the secondary core is starting up let it run its
	 * calibrations, then wait for it to finish
	 */
	spin_unlock(&boot_lock);

	return pen_release != -1 ? -ENOSYS : 0;
}

/*
 * Initialise the CPU possible map early - this describes the CPUs
 * which may be present or become present in the system.
 */
static void __init rda_smp_init_cpus(void)
{
	unsigned int i, ncores;

#ifdef CONFIG_ARCH_RDA8820
	ncores = 4;
#else
	ncores = 2;
#endif

	/* sanity check */
	if (ncores > nr_cpu_ids) {
		pr_warn("SMP: %u cores greater than maximum (%u), clipping\n",
			ncores, nr_cpu_ids);
		ncores = nr_cpu_ids;
	}

	printk(KERN_INFO "SMP: .smp_init_cpus set %d CPUs to be possible\n",
		ncores);
	for (i = 0; i < ncores; i++)
		set_cpu_possible(i, true);
}

static void __init rda_smp_prepare_cpus(unsigned int max_cpus)
{
	int i;

	/*
	 * Write the address of secondary startup into the
	 * system-wide flags register. The boot monitor waits
	 * until it receives a soft interrupt, and then the
	 * secondary CPU branches to this address.
	 */
	for (i = 1; i < max_cpus; ++i) {
		unsigned long phys_cpu;
		unsigned long boot_addr;

		phys_cpu = cpu_logical_map(i);
		boot_addr = virt_to_phys(rda_secondary_startup);
		printk(KERN_INFO "SMP: smp_prepare_cpus for phys_cpu %d\n",
			(int)phys_cpu);
		__raw_writel(boot_addr, cpu_boot_reg_addr(phys_cpu));
	}
}

#ifdef CONFIG_HOTPLUG_CPU
static void __ref rda_cpu_die(unsigned int cpu)
{
	printk(KERN_INFO "SMP: smp_dpu_die for cpu %d\n",
		(int)cpu);

	/* should send msg to modem here, to reset and clock gating */

	/* goto wfi */
	cpu_do_idle();

	printk(KERN_ERR "SMP: out from smp_dpu_die, should NOT happen\n");
}
#endif

struct smp_operations rda_smp_ops __initdata = {
	.smp_init_cpus		= rda_smp_init_cpus,
	.smp_prepare_cpus	= rda_smp_prepare_cpus,
	.smp_secondary_init	= rda_secondary_init,
	.smp_boot_secondary	= rda_boot_secondary,
#ifdef CONFIG_HOTPLUG_CPU
	.cpu_die		= rda_cpu_die,
#endif
};
