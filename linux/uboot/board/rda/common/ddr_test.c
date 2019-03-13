#include <common.h>

#if defined(CONFIG_MACH_RDA8810E) || defined(CONFIG_MACH_RDA8810H) || defined(CONFIG_MACH_RDA8850E)

#define DDR_TEST_DEBUG

#ifdef DDR_TEST_DEBUG
#define d_printf	printf
#else
#define d_printf(...)	do{}while(0)
#endif

/* HWTIMER frequency is 2M,so 1 tick is 0.5 us */
#define TIME_TICK_TO_US(t)	(t/2)
#define DDR_MEM_START_ADDR	0X80000000
#define DDR_MEM_END_ADDR	0XBFFFFFFF

int ddr_mem_copy_test(int times,unsigned int src_addr, unsigned int des_addr, unsigned int nword)
{
	unsigned int err = 0;
	unsigned int *ptr_src,*ptr_des,n = 0;
	int count = 0;
	unsigned long long tick1 = 0,tick2 = 0;

	/* checkt test times value */
	if((times <= 0)
		||(src_addr<DDR_MEM_START_ADDR)
		||(src_addr>DDR_MEM_END_ADDR)
		||(des_addr<DDR_MEM_START_ADDR)
		||(des_addr>DDR_MEM_END_ADDR)) {

		printf("\nInvalid parameters,times: %d\t,src_addr: 0x%x\t,des_addr: 0x%x\t, nword: 0x%x\n",
			times,src_addr, des_addr,nword);
		return -1;
	}
	printf("\n_ddr_mem_copy_test,times:%d\t,src_addr: 0x%x\t,des_addr: 0x%x\t, nword: 0x%x\n",
		times,src_addr, des_addr,nword);
LAB_COPY:
	tick1 = get_ticks();
	d_printf("tick1: %llu\n",tick1);
	/* copy data from src to des */
	ptr_src = (unsigned int *)src_addr;
	ptr_des = (unsigned int *)des_addr;
	n = nword;
	d_printf("data copying...\n");
	while(n--) {
		*ptr_des++ = *ptr_src++;
	}
	/* compare src and des */
	ptr_src = (unsigned int *)src_addr;
	ptr_des = (unsigned int *)des_addr;
	n = nword;
	d_printf("data checking...\n");
	while(n--) {
		if(*ptr_des != *ptr_src) {
			err = 1;
			break;
		}
		ptr_des++;
		ptr_src++;
	}
	tick2 = get_ticks();
	d_printf("tick2: %llu\n",tick2);
	tick1 = (tick2 > tick1)?(tick2 - tick1):(tick1 - tick2);
	d_printf("tick2 - tick1: %llu\n",tick1);
	printf("\n\ncost time: %llu us\n",TIME_TICK_TO_US(tick1));
	count++;
	/* print result message */
	if(err != 0) {
		printf("ddr memory copy test error.	times: %d\t,position: %d\n",count,nword-n);
		return -1;
	} else {
		printf("ddr memory copy test success.	times: %d\t\n",count);
	}
	/* check loop */
	if(count < times)
		goto LAB_COPY;
	return 0;
}
#endif
