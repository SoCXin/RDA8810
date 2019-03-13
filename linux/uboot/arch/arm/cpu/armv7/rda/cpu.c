#include <common.h>
#include <asm/arch/hardware.h>

void enable_neon(void)
{
	/* enable cp10 and cp11 */
	uint32_t val;

	__asm__ volatile("mrc	p15, 0, %0, c1, c1, 2" : "=r" (val));
	printf("CPU:   p15-c1-c1 (NSACR): 0x%08x", val);
	val |= (3<<10);
	val &= ~(3<<14);
	__asm__ volatile("mcr	p15, 0, %0, c1, c1, 2" :: "r" (val));
	printf(" -> 0x%08x\n", val);

	__asm__ volatile("mrc	p15, 0, %0, c1, c0, 2" : "=r" (val));
	printf("CPU:   p15-c1-c0 (CPACR): 0x%08x", val);
	val |= (3<<22)|(3<<20);
	__asm__ volatile("mcr	p15, 0, %0, c1, c0, 2" :: "r" (val));
	printf(" -> 0x%08x\n", val);

	/* set enable bit in fpexc */
	val = (1<<30);
	__asm__ volatile("mcr  p10, 7, %0, c8, c0, 0" :: "r" (val));
}

#if (defined(CONFIG_MACH_RDA8810E) \
||defined(CONFIG_MACH_RDA8820) \
||defined(CONFIG_MACH_RDA8810H) \
||defined(CONFIG_MACH_RDA8850E))
/* SMP MACHs */
void smp_setup(void)
{
	asm volatile(
		"mrc    p15, 0, r0, c1, c0, 1\n"
		"orr    r0, r0, #0x40\n"
		"mcr    p15, 0, r0, c1, c0, 1\n");
}
#endif

void enable_caches(void)
{
#if (defined(CONFIG_MACH_RDA8810E) \
||defined(CONFIG_MACH_RDA8820) \
||defined(CONFIG_MACH_RDA8810H) \
||defined(CONFIG_MACH_RDA8850E))
	printf("CPU: enable smp\n");
	smp_setup();
#endif
#ifndef CONFIG_SYS_ICACHE_OFF
	printf("CPU: enable instruction caches\n");
	icache_enable();
#endif
#ifndef CONFIG_SYS_DCACHE_OFF
	printf("CPU: enable data caches\n");
	dcache_enable();
#endif
#ifndef CONFIG_SYS_NEON_OFF
	printf("CPU: enable neon\n");
	enable_neon();
#endif
}

#ifdef CONFIG_DISPLAY_CPUINFO
/* Print CPU information */
int print_cpuinfo(void)
{
#if defined(CONFIG_MACH_RDAARM926EJS)
	printf("RDAARM926EJS FPGA\n");
#elif defined(CONFIG_MACH_RDA8810)
	printf("RDA8810 SoC\n");
#elif defined(CONFIG_MACH_RDA8810E)
	printf("RDA8810E SoC\n");
#elif defined(CONFIG_MACH_RDA8820)
	printf("RDA8820 SoC\n");
#elif defined(CONFIG_MACH_RDA8850)
	printf("RDA8850 SoC\n");
#elif defined(CONFIG_MACH_RDA8850E)
	printf("RDA8850E SoC\n");
#elif defined(CONFIG_MACH_RDA8810H)
	printf("RDA8810H SoC\n");
#else
#error "Unknown RDA CPU"
#endif
	return 0;
}
#endif	/* CONFIG_DISPLAY_CPUINFO */ 
