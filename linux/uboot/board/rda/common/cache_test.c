#include <common.h>
#include <asm/arch/global_macros.h>
#include <asm/arch/cs_types.h>
#include <asm/arch/rda_iomap.h>
#include <asm/arch/reg_timer.h>
#include "a7_cp15_reg.h"

#define d_printf printf
#define n_printf printf

#define L2C_SZ_1K	0x00000400
#define L2C_SZ_16K	0x00004000
#define L2C_SZ_256K	0x00040000

#define L2C_FILL_SIZE	256
#define L2C_WAY_NUM     16
#define L2C_WAY_SIZE	0x4000
#define L2C_LINE_SIZE	32
#define BANK_NUM	3

/* SRAM start address is 0x00100000 */
#define CONFIG_SRAM_START	0x00100000
/* SRAM size is 64KB */
#define CONFIG_SRAM_SIZE	0x00010000
/* DRAM start address is 0x80000000 */
#define CONFIG_DRAM_START	0x80000000
/* DRAM size is 256MB */
#define CONFIG_DRAM_SIZE	0x10000000
/* IO start address is 0x20000000 */
#define CONFIG_IO_START		0x20000000
/* IO size is 0x256MB */
#define CONFIG_IO_SIZE		0x10000000
/* MMU TLB address is 0x00108000 */
#define CONFIG_TEST_MMU_TLB_ADDR (CONFIG_SRAM_START + 0x8000)

#define GET_ARRAY_SIZE(array)	(sizeof(array) / sizeof((array)[0]))

/* tick / 2000 = n ms ,freq = 2MHz */
#define TICK_TO_MS(tick)	((tick) / 2000)
#define MS_TO_TICK(ms)	((ms) * 2000)

typedef struct
{
	int id;
	const char *name;
}armv7_cache_t;

typedef struct
{
	unsigned int sel;
	unsigned int num_sets;
	unsigned int size;
}armv7_cache_size_t;

static const armv7_cache_t a7_cache_tab[] =
{
	{0,"l1 d-cache"},
	{1,"l1 i-cache"},
	{2,"l2 cache"},
};

static const armv7_cache_size_t a7_cache_size_tab[] =
{
	{0,0x1F, 8,},//l1 d-cache
	{0,0x3F, 16},
	{0,0x7F, 32},
	{0,0xFF, 64},
	{1,0x7F, 8,},//l1 i-cache
	{1,0xFF, 16},
	{1,0x1FF,32},
	{1,0x3FF,64},
	{2,0xFF, 128,},//l2 cache
	{2,0x1FF,256},
	{2,0x3FF,512},
	{2,0x7FF,1024},
};

struct bank_info {
	struct {
		u32 start;
		u32 size;
	}ram[BANK_NUM];
};

#if 0
static struct bank_info ram_table;
static u32 ttb_base = 0;

static void *l2c_base = (void *)RDA_L2CC_BASE;
static u32 l2c_way_mask = 0;	/* Bitmask of active ways */
static u32 l2c_size = 0;
static u32 l2c_sets = 0;
static u32 l2c_ways = 0;
#endif

static u8 *test_phyaddr = (u8 *)(CONFIG_DRAM_START + 0x04000000);

typedef union
{
    unsigned long long timer;
    struct
    {
        unsigned int timer_l     :32;
        unsigned int timer_h     :32;
    }fields;
}TIMER;


static void timer_get(TIMER *timer)
{
	timer->fields.timer_h = (unsigned int)(hwp_apTimer->HWTimer_LockVal_H);
	timer->fields.timer_l = (unsigned int)(hwp_apTimer->HWTimer_LockVal_L);
	if(timer->fields.timer_h != hwp_apTimer->HWTimer_LockVal_H) {
		timer->fields.timer_h = (unsigned int)(hwp_apTimer->HWTimer_LockVal_H);
		timer->fields.timer_l = (unsigned int)(hwp_apTimer->HWTimer_LockVal_L);
	}
}

static void timer_start(TIMER *timer)
{
	timer_get(timer);
}

static unsigned long long timer_stop(TIMER *timer)
{
	unsigned long long start,end,elaps;
	start = timer->timer;
	timer_get(timer);
	end = timer->timer;
	if(start <= end)
		elaps = end - start;
	else
		elaps = (unsigned long long )0xFFFFFFFFFFFFFFFFULL - (start - end);
	return elaps;
}

static int is_cpu_secure(void)
{
	unsigned int is_secure;
	unsigned int scr = read_cp15_scr();
	d_printf("scr = %x\n",scr);
	scr &= 0x01;
	is_secure =  (scr == 0) ? (1) : (0);
	return is_secure;
}

static int is_l2cache_exist(void)
{
	unsigned int existed = 0;
	unsigned int clidr = read_cp15_clidr();
	d_printf("clidr = %x\n",clidr);
	clidr = (clidr >> 3) & 0x7;//bit[5:3]
	if(clidr)
		existed = 1;
	return existed;
}

/*
 *level = 0: L1 D-Cache
 *level = 1: L1 I-Cache
 *level = 2: L2 D-Cache
 */
static int select_cache(int level)
{
	unsigned int csselr = 0;

	if((level < 0) || (level > 2)) {
		d_printf("select cache error,level is %d\n",level);
		return -1;
	}
	d_printf("select %s\n",a7_cache_tab[level].name);
	csselr = read_cp15_csselr();
	d_printf("\ncsselr = %x\n",csselr);

	csselr &= ~(0xF << 0);//bit[3:0]
	csselr |= level;
	write_cp15_csselr(csselr);
	return 0;
}

static int parse_cache_info(int level,unsigned int val)
{
	unsigned int line_size,asso,ways,num_sets,wa,ra,wb,wt;
	int i,num;

	d_printf("\nparse value %x\n",val);

	line_size = val & 0x7;
	asso = (val >> 3) & 0x3FF;
	num_sets = (val >> 13) & 0x7FFF;
	wa = (val >> 28) & 0x1;
	ra = (val >> 29) & 0x1;
	wb = (val >> 30) & 0x1;
	wt = (val >> 31) & 0x1;

	if(wt)
		n_printf("Write-Through:    Yes\n");
	else
		n_printf("Write-Through:    No\n");
	if(wb)
		n_printf("Write-Back:       Yes\n");
	else
		n_printf("Write-Back:       No\n");
	if(ra)
		n_printf("Read-Allocation:  Yes\n");
	else
		n_printf("Read-Allocation:  No\n");
	if(wa)
		n_printf("Write-Allocation: Yes\n");
	else
		n_printf("Write-Allocation: No\n");

	ways = asso + 1;
	n_printf("Associativity is %d-ways\n",ways);

	line_size = line_size * 8;
	n_printf("%d words per line\n\n",line_size);

	num = GET_ARRAY_SIZE(a7_cache_size_tab);
	for(i = 0;i < num;i++) {
		if((level == a7_cache_size_tab[i].sel)
			&& (num_sets == a7_cache_size_tab[i].num_sets))
			break;
	}
	if(i >= num) {
		n_printf("parse cache size error\n");
		return -1;
	}
	n_printf("%s size is %d KB\n",
			a7_cache_tab[level].name,
			a7_cache_size_tab[i].size);
	return 0;
}

static int print_cache_info(int level)
{
	unsigned int ccsidr;
	if(select_cache(level)) {
		n_printf("select cache failed\n");
		return -1;
	}
	ccsidr = read_cp15_ccsidr();
	parse_cache_info(level,ccsidr);
	return 0;
}

static void read_sys_ctrl_reg(void)
{
	unsigned int val = read_cp15_sctlr();
	n_printf("sctlr is %x\n",val);
}

static int init_test_buffer(void)
{
	int i;
	for(i = 0;i < L2C_FILL_SIZE;i++)
		test_phyaddr[i] = i;
	return 0;
}
#if 0
static void init_banks(void)
{
	/*
	 * Page table is at end of DRAM, and 16K align.
	 */
	ttb_base = CONFIG_TEST_MMU_TLB_ADDR;

	/* SRAM address */
	ram_table.ram[0].start = CONFIG_SRAM_START;
	ram_table.ram[0].size  = CONFIG_SRAM_SIZE;
	/* IOMAP address */
	ram_table.ram[1].start = CONFIG_IO_START;
	ram_table.ram[1].size = CONFIG_IO_SIZE;
	/* DDRAM address */
	ram_table.ram[2].start = CONFIG_DRAM_START;
	ram_table.ram[2].size  = CONFIG_DRAM_SIZE;
}

unsigned int init_pgtable(void)
{
	int i;
	unsigned int *page_table = (unsigned int *)ttb_base;
	unsigned int val = 0xFFFFFFFF;

	/* Set up an identity-mapping for all 4GB, rw for everyone */
	for (i = 0; i < 4096; i++) {
		/*
		 * Section description definition
		 *
		 * TEX[2:0]----AP[1:0]----CB
		 *    000              11               00
		 */
		page_table[i] = (i << 20) | (3 << 10) | 0x2;
	}
	/*
	 * Update memory attribute
	 * TEX[2:0]----AP[1:0]----CB
	 *    001              11               11
	 */
	page_table[0] = (0 << 20) | (1 << 12) | (3 << 10) | 0xE;
	page_table[1] = (1 << 20) | (1 << 12) | (3 << 10) | 0xE;
	for (i = (ram_table.ram[1].start >> 20);
		i < ((ram_table.ram[1].start + ram_table.ram[1].size) >> 20);
		i++) {
		/*
		 * Update memory attribute
		 * TEX[2:0]----AP[1:0]----CB
		 *    000              11               00
		 */
		page_table[i] = (i << 20) | (3 << 10) | 0x2;
	}
	for (i = (ram_table.ram[2].start >> 20);
		i < ((ram_table.ram[2].start + ram_table.ram[2].size) >> 20);
		i++) {
		/*
		 * Update memory attribute
		 * TEX[2:0]----AP[1:0]----CB
		 *    001              11               11
		 */
		page_table[i] = (i << 20) | (1 << 12) | (3 << 10) | 0xE;
	}
	// set TTBR0
	write_cp15_ttbr0((unsigned int)page_table);
	for(i = 0;i < 0x1000;i++)
		__nop_dly();

	// TODO: set TTBR1
	//write_cp15_ttbr1((unsigned int)page_table);

	// TODO: set TTBCR
	//write_cp15_ttbcr(0);

	// TODO: set HTCR
	//write_cp15_htcr(0);

	// write access domain register
	write_cp15_dacr(val);
	for(i = 0;i < 0x1000;i++)
		__nop_dly();

	d_printf("page table address is %x\n",(unsigned int)page_table);
	d_printf("dommain access control register value is %x\n",val);
	return 0;
}

void cpu_setup_mmu(int enabled)
{
	int i;
	if(enabled) {
		init_banks();
		init_pgtable();

		cp15_enable_mmu();
		for(i = 0;i < 0x1000;i++)
			__nop_dly();
		d_printf("MMU enabled\n");
	} else {
		cp15_disable_mmu();
		for(i = 0;i < 0x1000;i++)
			__nop_dly();
		d_printf("MMU disabled\n");
	}
}
#endif
static int gen_fibonacci_num(int n)
{
	volatile int a = 0,b = 1,s = 0;
	while(n--) {
		s += a;
		s += b;
		a += b;
		b += b;
	}
	return s;
}

static int test_icache(unsigned int test_num)
{
	volatile int i = test_num;
	TIMER timer;
	unsigned int tick;
	int cost;

	d_printf("test icache\n");
	timer_start(&timer);
	d_printf("timer started\n");

	while (i--) {
		gen_fibonacci_num(10);
	};

	tick = timer_stop(&timer);
	cost = TICK_TO_MS(tick);
	d_printf("<<<<<<<< Ins total time = %d ms\n",cost);
	return cost;
}

static int test_dcache(unsigned int test_num)
{
	volatile u8 value = 0;
	u32 i = 0;
	u32 j = 0;
	TIMER timer;
	unsigned int tick;
	int cost;

	d_printf("test dcache\n");
	timer_start(&timer);
	d_printf("timer started\n");

	for (i = 0; i < test_num; i++) {
		for (j = 0; j < L2C_FILL_SIZE; j++) {
			value = test_phyaddr[j];
		}
	}
	value = value;
	tick = timer_stop(&timer);
	cost = TICK_TO_MS(tick);
	d_printf("<<<<<<<< Data total time = %d ms\n",cost);
	return cost;
}

void l2cache_test(void)
{
	unsigned int cost_time[4];

	n_printf("\n\n");
	n_printf("====== L2 cache test ======\r\n");

	n_printf("\n========> check cpu secure state\n");
	if(is_cpu_secure()) {
		n_printf("cpu is in secure state\n");
	} else {
		n_printf("cpu is in none secure state\n");
	}

	n_printf("\n========> check l2 cache existing\n");
	if(!is_l2cache_exist()) {
		n_printf("l2 cache do not exist\n");
		return;
	} else {
		n_printf("exist l2 cache\n");
	}

	n_printf("\n========> print L1 data cache info\n");
	print_cache_info(0);

	n_printf("\n========> print L1 instruction cache info\n");
	print_cache_info(1);

	n_printf("\n========> print L2 data cache info\n");
	print_cache_info(2);

	n_printf("\n========> test cache: disable L1,L2 cache\n");

	n_printf("read sys ctrl register before disable cache\n");
	read_sys_ctrl_reg();
	icache_disable();
	dcache_disable();
	n_printf("read sys ctrl register after disable cache\n");
	read_sys_ctrl_reg();

	init_test_buffer();
	cost_time[0] = test_icache(0x10000);
	cost_time[1] = test_dcache(0x1000);

	n_printf("\n========> test cache: enable  L1,L2 cache\n");
	/*
	 * set TTBR0,TTBR1,DACR,HTCR
	 * enable MMU,I-cache,D-cache
	 */
	icache_enable();
	dcache_enable();
	n_printf("read sys ctrl register after enable cache\n");
	read_sys_ctrl_reg();

	n_printf("delay n ms ...\n");
	mdelay(10);
	/* run test program */
	cost_time[2] = test_icache(0x10000);
	cost_time[3] = test_dcache(0x1000);

	read_sys_ctrl_reg();

	/* print result */
	n_printf("\n========> test cache result\n");

	n_printf("\n========> L1,L2 Disabled\n"
			"I-Cache tests cost time: %8d ms "
			"D-Cache tests cost time: %8d ms ",
			cost_time[0],cost_time[1]);

	n_printf("\n========> L1,L2 Enabled\n"
			"I-Cache tests cost time: %8d ms "
			"D-Cache tests cost time: %8d ms ",
			cost_time[2],cost_time[3]);
	n_printf("\n========> test cache finished !\n");
}

int cpu_cache_test(int times)
{
	int n = 0;
	while(n < times) {
		n++;
		n_printf("\ncpu_cache_test: %d / %d \n",n,times);
		l2cache_test();
		n_printf("\ncpu_cache_test end\n");
	}
	return 0;
}
