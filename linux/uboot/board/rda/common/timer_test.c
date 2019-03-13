#include <common.h>
#include <asm/arch/cs_types.h>
#include <asm/arch/global_macros.h>
#include <asm/arch/reg_sysctrl.h>
#include <asm/arch/reg_timer.h>

#ifdef CONFIG_IRQ
#include "irq.h"
#endif

#if defined(CONFIG_MACH_RDA8850E)
static HWP_TIMER_AP_T *hwp_timer_tab[] = {hwp_apTimer,NULL,NULL};
#elif defined(CONFIG_MACH_RDA8810H)
//static HWP_TIMER_AP_T *hwp_timer_tab[] = {hwp_apTimer0,hwp_apTimer1,hwp_apTimer2};
static HWP_TIMER_AP_T *hwp_timer_tab[] = {hwp_apTimer,NULL,NULL};
#else
#error "unknown machine !"
#endif

/* Hardware timer frequency value 2MHz or 16KHz */
#define CONFIG_HW_TIMER_FAST_FREQ	2000000
#define CONFIG_HW_TIMER_SLOW_FREQ	16384

#define TIMER_BASE_TICK_US	1000000
#define TIMER_BASE_TICK_MS	1000
#define TIMER_BASE_TICK_S	1

#define d_printf	printf

#define USE_DATA_SIZE_64_BIT

#ifdef USE_DATA_SIZE_64_BIT
typedef unsigned long long int u64;
#else
typedef unsigned int u64;
#endif /* USE_DATA_SIZE_64_BIT */

typedef union
{
	volatile unsigned long long int timer;
	struct
	{
		volatile unsigned int timer_l:32;
		volatile unsigned int timer_h:32;
	}fields;
}TIMER;

/* TIMER id number */
enum TIMER_ID_TYPE
{
	TIMER0_ID = 0,
	TIMER1_ID = 1,
	TIMER2_ID = 2,
};

/* TIMER test case id number */
enum TIMER_TC_ID_TYPE
{
	HWTIMER_TC_ID_REG_DEFAULT_VAL = 0,
	HWTIMER_TC_ID_DELAY_US,
	HWTIMER_TC_ID_DELAY_MS,
	HWTIMER_TC_ID_DELAY_S,
	HWTIMER_TC_ID_INTERVAL_IRQ_125MS,
	HWTIMER_TC_ID_INTERVAL_IRQ_250MS,
	HWTIMER_TC_ID_INTERVAL_IRQ_500MS,
	HWTIMER_TC_ID_INTERVAL_IRQ_1000MS,
	HWTIMER_TC_ID_DELAY_WAVE_1MS,
	HWTIMER_TC_ID_IRQ_WAVE_125MS,
	HWTIMER_TC_ID_IRQ_WAVE_250MS,
	HWTIMER_TC_ID_IRQ_WAVE_500MS,
	HWTIMER_TC_ID_IRQ_WAVE_1000MS,
	HWTIMER_TC_ID_WRAP_IRQ,
	HWTIMER_TC_ID_LOCK_VAL,
	OSTIMER_TC_ID_REG_DEFAULT_VAL,
	OSTIMER_TC_ID_DELAY_US,
	OSTIMER_TC_ID_DELAY_MS,
	OSTIMER_TC_ID_DELAY_WAVE_1MS,
	OSTIMER_TC_ID_TEST_IRQ_STATUS,
	OSTIMER_TC_ID_IRQ_WRAP,
	OSTIMER_TC_ID_IRQ_REPEAT,
	OSTIMER_TC_ID_LOCK_VAL,
};

/*
 * TIMER LOCAL VARIBLE DEFINITION
 *********************************************************
 */

/* hardware timer interval interrupt cycle*/
enum TIMER_IT_CYCLE_TYPE
{
	IT_PERIOD_125MS = 0,
	IT_PERIOD_250MS,
	IT_PERIOD_500MS,
	IT_PERIOD_1000MS,
};

static u32 g_hwtimer_freq = CONFIG_HW_TIMER_FAST_FREQ;
static HWP_TIMER_AP_T *hwp_timer = hwp_apTimer;

void hwt_delay_us(u32 us);
static void timer_reset(void);
static u32 caculate_tick_val(u32 time_us);
static u32 caculate_time_val(u32 tick);
static void set_timer_clk_freq(int fast_mode);
static void ostimer_enable(int is_enable);
static void ostimer_clear_irq(void);
static void do_waiting_print(int *ptr_wait_cnt,int is_clear,const char *str);

/*
 * TIMER PRIVATE FUNCTION IMPLEMENTATION
 *********************************************************
 */
static void get_hwtimer_time(TIMER *ptimer)
{
	/* Read high word first */
	ptimer->fields.timer_h = (UINT32)(hwp_apTimer->HWTimer_LockVal_H);
	ptimer->fields.timer_l = (UINT32)(hwp_apTimer->HWTimer_LockVal_L);
	/* Check wrap case */
	if (ptimer->fields.timer_h != hwp_apTimer->HWTimer_LockVal_H) {
		ptimer->fields.timer_h = (UINT32)(hwp_apTimer->HWTimer_LockVal_H);
		ptimer->fields.timer_l = (UINT32)(hwp_apTimer->HWTimer_LockVal_L);
	}
}

static void hwtimer_delay_process(TIMER *start_time, TIMER *end_time,TIMER *cur_time,u64 tick)
{
	/* get current time value */
	get_hwtimer_time(cur_time);

	/* add us */
	start_time->timer = cur_time->timer;
	end_time->timer = cur_time->timer+tick;

	/* wait until timeout */
	while(1) {
		get_hwtimer_time(cur_time);
		if((cur_time->timer) > (end_time->timer))
			break;
	}
}

void hwt_delay_us(u32 us)
{
	TIMER start_time,end_time,cur_time;
	u64 temp1,temp2;

	temp1 = us * g_hwtimer_freq;
	temp2 = TIMER_BASE_TICK_US;
	temp1 = temp1 / temp2;
	hwtimer_delay_process(&start_time,&end_time,&cur_time,temp1);
}

void hwt_delay_ms(u32 ms)
{
	u32 i = 0;
	for(i = 0; i < ms; i++)
		hwt_delay_us(1000); // delay 1 ms
}

static void do_waiting_print(int *ptr_wait_cnt,int is_clear,const char *str)
{
	static int cnt = 0;

	if(is_clear) {
		cnt = 0;
		d_printf("\n");
		return;
	}
	if(ptr_wait_cnt != NULL)
		*ptr_wait_cnt = *ptr_wait_cnt + 1;
	d_printf("%s",str);
	cnt++;
	if((cnt % 80) == 0) {
		cnt = 0;
		d_printf("\n");
	}
	hwt_delay_ms(1000);
}

static u32 caculate_tick_val(u32 time_us)
{
	u32 tick = 0;
	u32 base = 0;
	if(g_hwtimer_freq >= TIMER_BASE_TICK_US) {
		  // fast freq = 2MHz, 1 us is 2 tick
		  base = g_hwtimer_freq / (u32)TIMER_BASE_TICK_US;
		  tick = time_us * base;
	} else {
		//slow freq = 16384Hz, 61 us is 1 tick
		base = (u32)TIMER_BASE_TICK_US / g_hwtimer_freq;
		tick = time_us / base;
	}
//	d_printf("cal tick value is 0x%x\n",tick);
	return tick;
}

static u32 caculate_time_val(u32 tick)
{	u32 base,time_val;
	if(g_hwtimer_freq > TIMER_BASE_TICK_US) {
		base = g_hwtimer_freq / TIMER_BASE_TICK_US;
		time_val = tick / base;
	} else {
		base = TIMER_BASE_TICK_US / g_hwtimer_freq;
		time_val = tick * base;
	}
//	d_printf("cal time value is 0x%x\n",time_val);
	return time_val;
}

static void timer_reset(void)
{
	// reset all timer
	hwp_sysCtrlAp->APB1_Rst_Set = SYS_CTRL_AP_SET_APB1_RST_TIMER;
	hwp_sysCtrlAp->APB1_Rst_Clr = SYS_CTRL_AP_SET_APB1_RST_TIMER;
}

static void set_timer_clk_freq(int fast_mode)
{
	//0 is fast mode, 1 is slow mode
	if(fast_mode) {
		hwp_sysCtrlAp->Sel_Clock &= (~SYS_CTRL_AP_TIMER_SEL_FAST_SLOW);
		g_hwtimer_freq = CONFIG_HW_TIMER_FAST_FREQ;
	} else {
		hwp_sysCtrlAp->Sel_Clock |= SYS_CTRL_AP_TIMER_SEL_FAST_SLOW;
		g_hwtimer_freq = CONFIG_HW_TIMER_SLOW_FREQ;
	}
}

#define GPIOA_SET_ADDR	0x20930030
#define GPIOA_CLR_ADDR	0x20930034

static void do_set_gpo_pin(int pin_id, int pin_val)
{
	unsigned int set_addr = GPIOA_SET_ADDR; //GPIOA 's GPO register set address
	unsigned int clr_addr = GPIOA_CLR_ADDR; // GPIOA 's GPO register reset address
	unsigned int val = 0;

	/* pin_id must be 0,1,2 */
	if((pin_id < 0) || (pin_id >= 3)) {
		d_printf("set gpo pin value failed!,pin id is %d\n",pin_id);
		return;
	}
	val = 1 << pin_id;
	if(pin_val) {
		*(volatile unsigned int *)set_addr = val;
	} else {
		*(volatile unsigned int *)clr_addr = val;
	}
#if 0
	{
		unsigned int val = *(volatile unsigned int *)set_addr;
		d_printf("set_gpo_reg value : %x\n",val);
	}
#endif

}
/*
 * TIMER PUBLIC FUNCTION IMPLEMENTATION
 *********************************************************
 */

int hwtimer_reg_default_value(u8 is_reset)
{
	unsigned int reg_val[8];
	int idx = 0;

	d_printf("\nread HWTimer register value ...\n");
	if(is_reset > 0) {
	        timer_reset();
	}
	reg_val[idx++] = hwp_timer->HWTimer_Ctrl;
	reg_val[idx++] = hwp_timer->HWTimer_CurVal_L;
	reg_val[idx++] = hwp_timer->HWTimer_CurVal_H;
	reg_val[idx++] = hwp_timer->HWTimer_LockVal_L;
	reg_val[idx++] = hwp_timer->HWTimer_LockVal_H;
	idx = 0;
	d_printf("HWTimer_Ctrl:        0x%x\n",reg_val[idx++]);
	d_printf("HWTimer_CurVal_L:    0x%x\n",reg_val[idx++]);
	d_printf("HWTimer_CurVal_H:    0x%x\n",reg_val[idx++]);
	d_printf("HWTimer_LockVal_L:   0x%x\n",reg_val[idx++]);
	d_printf("HWTimer_LockVal_H:   0x%x\n",reg_val[idx++]);
	return 0;
}

int hwtimer_delay_us(u32 us)
{
	TIMER start_time,end_time,cur_time;
	u32 tick,dly_time = 0;
	u32 real_err = 0,max_err = 5,err_diff;

	d_printf("\nHWTimer delay %d us ...\n",(int)us);

	tick = caculate_tick_val(us);
	hwtimer_delay_process(&start_time,&end_time,&cur_time,tick);

	/* get real delay value */
	//dly_time = (cur_time.fields.timer_l - start_time.fields.timer_l) / tick_1us;
	dly_time = caculate_time_val((cur_time.fields.timer_l - start_time.fields.timer_l));
	err_diff = (dly_time > us) ? (dly_time - us):(us - dly_time);
	real_err = (err_diff * 100) / us;

	d_printf("\n");
	d_printf("expect delay:   0x%x us\n",us);
	d_printf("real delay:     0x%x us\n",dly_time);
	d_printf("error value:    0x%x us\n",err_diff);
	d_printf("error percent:  0x%x\n",real_err);
	if(real_err <= max_err) {
		d_printf("timer error %d is less than %d,test success !!!\n",
				real_err,max_err);
		return 0;
	} else {
		d_printf("time error %d is greater than %d,test failed ###\n",
				real_err,max_err);
		return -1;
	}
}

int hwtimer_delay_ms(u32 ms)
{
	u32 i = 0;

	d_printf("delay %d ms ...\n",(int)ms);
	for(i = 0; i < ms; i++)
		hwt_delay_us(1000); // delay 1 ms
	d_printf("delay %d ms success !!!\n",(int)ms);
	return 0;
}

int hwtimer_delay_s(u32 s)
{
	u32 i = 0;
	d_printf("delay %d s ...\n",(int)s);
	for(i = 0; i < s*1000; i++)
		hwt_delay_us(1000); //delay 1 ms
	d_printf("delay %d s success !!!\n",(int)s);
	return 0;
}

int hwtimer_delay_wave(int period_us, int irq_times)
{
	int irq_cnt = 0;
	int half_period = period_us / 2;
	int gpo_pin = 0;

	if(irq_times <= 0)
		return -1;
	d_printf("\n");
	d_printf("HWTimer delay wave starts\n");
	d_printf("period_us is 0x%x\n",period_us);
	d_printf("irq_times is 0x%x\n",irq_times);
	d_printf("Output wave from GPO 0 ...\n");
	while(1) {
		if(irq_cnt % 2) {
			do_set_gpo_pin(gpo_pin,0);
		} else {
			do_set_gpo_pin(gpo_pin,1);
		}
		hwt_delay_us(half_period);
		irq_cnt++;
		if(irq_cnt >= irq_times)
			break;
	}
	do_set_gpo_pin(gpo_pin,0);
	d_printf("HWTimer delay wave passed !!!\n");
	return 0;
}

/******************************************************************************
 * @name:	 hwtimer_interval_irq
 * @description: to test hwtimer interval irq function
 * @parameter:
 *	cycle:	interval cycle value, this value can be:
 *		IT_PERIOD_125MS
 *		IT_PERIOD_250MS
 *		IT_PERIOD_500MS
 *		IT_PERIOD_1000MS
 *	irq_times: test times value
 * @notes:
 * 	step1: disable timer irq
 * 	step2: configure irq cycle
 * 	step3: clear irq mask
 * 	step4: enable timer irq
 * @return: 0(SUCCESS), -1(FAILED)
 ******************************************************************************
 */

int hwtimer_interval_irq_wave( enum TIMER_IT_CYCLE_TYPE cycle,int irq_times)
{
	int irq_cnt = 0;
	int gpo_pin = 0;

	d_printf("\nHWTimer interval irq wave starts\n");
	d_printf("cycle bit: 0x%x\n",cycle);
	d_printf("irq_times: 0x%x\n",irq_times);

	/* Disable interval IRQ */
	hwp_timer->HWTimer_Ctrl &= (~TIMER_AP_INTERVAL_EN);

	/* Set intterupt cycle */
	hwp_timer->HWTimer_Ctrl &= TIMER_AP_INTERVAL(0x3); //bit0,bit1
	hwp_timer->HWTimer_Ctrl |= TIMER_AP_INTERVAL(cycle);

	/* Set irq mask interval */
	hwp_timer->Timer_Irq_Mask_Set = TIMER_AP_HWTIMER_ITV_MASK;

	// clear interval irq
	hwp_timer->Timer_Irq_Clr = TIMER_AP_HWTIMER_ITV_CLR;
	while((hwp_timer->Timer_Irq_Cause & TIMER_AP_HWTIMER_ITV_CAUSE) != 0);

	d_printf("TIMER_Irq_Mask_Set: 0x%x\n",hwp_timer->Timer_Irq_Mask_Set);
	d_printf("TIMER_Irq_Mask_Clr: 0x%x\n",hwp_timer->Timer_Irq_Mask_Clr);
	d_printf("Timer_Irq_Clr:      0x%x\n",hwp_timer->Timer_Irq_Clr);
	d_printf("HWTimer_Ctrl:       0x%x\n",hwp_timer->HWTimer_Ctrl);
	d_printf("Output wave from GPO %x ...\n",gpo_pin);
	/* Enable interval IRQ */
	hwp_timer->HWTimer_Ctrl |= TIMER_AP_INTERVAL_EN;
	// Get Start time
	irq_cnt = 0;
	while(1) {
		// set gpo pin level HIGH or LOW
		if(irq_cnt % 2)
			do_set_gpo_pin(gpo_pin,0); // set low level
		else
			do_set_gpo_pin(gpo_pin,1); // set high level
		// waiting for irq status set
		while((hwp_timer->Timer_Irq_Cause & TIMER_AP_HWTIMER_ITV_CAUSE) == 0);
		//clear irq status
		hwp_timer->Timer_Irq_Clr = TIMER_AP_HWTIMER_ITV_CLR;
		while((hwp_timer->Timer_Irq_Cause & TIMER_AP_HWTIMER_ITV_CAUSE) != 0);

		irq_cnt++;
		if(irq_cnt >= irq_times) {
			break;
		}
	}
	d_printf("Output wave end\n");
	do_set_gpo_pin(gpo_pin,0);

	// disable irq
	hwp_timer->HWTimer_Ctrl &= (~TIMER_AP_INTERVAL_EN);

	// clear mask
	hwp_timer->Timer_Irq_Mask_Clr = TIMER_AP_HWTIMER_ITV_MASK;
	d_printf("HWTimer interval irq wave passed !!!\n");
	return 0;
}

int hwtimer_interval_irq( enum TIMER_IT_CYCLE_TYPE cycle,int irq_times)
{
	TIMER timer_a,timer_b;
	u32 tick_diff = 0,e_period = 0,r_period,d_period = 0;
	volatile int irq_cnt = 0;
	u32 real_err = 0, max_err = 5;
	volatile u32 t1;
	volatile u32 t2;

	d_printf("\nHWTimer interval irq test starts\n");
	d_printf("cycle bit: 0x%x\n",cycle);
	d_printf("irq_times: 0x%x\n",irq_times);

	/* Disable interval IRQ */
	hwp_timer->HWTimer_Ctrl &= (~TIMER_AP_INTERVAL_EN);

	/* Set intterupt cycle */
	hwp_timer->HWTimer_Ctrl &= TIMER_AP_INTERVAL(0x3); //bit0,bit1
	hwp_timer->HWTimer_Ctrl |= TIMER_AP_INTERVAL(cycle);

	/* Set/Clear irq mask interval */
	hwp_timer->Timer_Irq_Mask_Set = TIMER_AP_HWTIMER_ITV_MASK;

	// clear interval irq
	hwp_timer->Timer_Irq_Clr = TIMER_AP_HWTIMER_ITV_CLR;
	while((hwp_timer->Timer_Irq_Cause & TIMER_AP_HWTIMER_ITV_CAUSE) != 0);

	d_printf("TIMER_Irq_Mask_Set: 0x%x\n",hwp_timer->Timer_Irq_Mask_Set);
	d_printf("TIMER_Irq_Mask_Clr: 0x%x\n",hwp_timer->Timer_Irq_Mask_Clr);
	d_printf("Timer_Irq_Clr:      0x%x\n",hwp_timer->Timer_Irq_Clr);
	d_printf("HWTimer_Ctrl:       0x%x\n",hwp_timer->HWTimer_Ctrl);

	/* Enable interval IRQ */
	hwp_timer->HWTimer_Ctrl |= TIMER_AP_INTERVAL_EN;
	// Get Start time
	get_hwtimer_time(&timer_a);
	while(1) {
		//wait irq status
		//while((hwp_timer->Timer_Irq_Cause & TIMER_AP_HWTIMER_ITV_STATUS) == 0);
		while((hwp_timer->Timer_Irq_Cause & TIMER_AP_HWTIMER_ITV_CAUSE) == 0);
		get_hwtimer_time(&timer_b);
		//clear irq status
		hwp_timer->Timer_Irq_Clr = TIMER_AP_HWTIMER_ITV_CLR;
		while((hwp_timer->Timer_Irq_Cause & TIMER_AP_HWTIMER_ITV_CAUSE) != 0);

		irq_cnt++;
		if(irq_cnt >= irq_times) {
			break;
		}
	}
	// disable irq
	hwp_timer->HWTimer_Ctrl &= (~TIMER_AP_INTERVAL_EN);
	// clear mask
	hwp_timer->Timer_Irq_Mask_Clr = TIMER_AP_HWTIMER_ITV_MASK;
	// check result
	d_printf("Timer_a value,h:%x,l:%x\n",
			timer_a.fields.timer_h,
			timer_a.fields.timer_l);
	d_printf("Timer_b value,h:%x,l:%x\n",
			timer_b.fields.timer_h,
			timer_b.fields.timer_l);
	switch(cycle) {
	case IT_PERIOD_125MS: e_period = 125;break;
	case IT_PERIOD_250MS: e_period = 250;break;
	case IT_PERIOD_500MS: e_period = 500;break;
	case IT_PERIOD_1000MS: e_period = 1000;break;
	default:d_printf("timer cycle error,exit !\n");return -1;
	}

	t1 = (u32)(timer_b.fields.timer_l);
	t2 = (u32)(timer_a.fields.timer_l);
	tick_diff = t1 - t2;
	r_period = caculate_time_val(tick_diff) / irq_times;
	r_period = r_period / 1000;
	d_period = (r_period > e_period)?(r_period - e_period):(e_period - r_period);
	real_err = (d_period * 100) / e_period;

	d_printf("\n");
	d_printf("tick_diff:       0x%x\n",tick_diff);
	d_printf("expected period: 0x%x\n",e_period);
	d_printf("real period:     0x%x\n",r_period);
	d_printf("error value:     0x%x\n",d_period);
	d_printf("error percent    0x%x\n",real_err);
	if(real_err <= max_err) {
		d_printf("timer error %d is less than %d,test success !!!\n",
				real_err,max_err);
		return 0;
	} else {
		d_printf("timer error %d is greater than %d,test failed ###\n",
				real_err,max_err);
		return -1;
	}
}

/******************************************************************************
 * @name:	 hwtimer_wrap_irq
 * @description: to test hwtimer wrap irq function
 * @parameter:
 *	irq_times: test times value
 * @notes:
 * @return: 0(SUCCESS), -1(FAILED)
 ******************************************************************************
 */

int hwtimer_wrap_irq(int irq_times)
{
	int sec_count = 0;
	u32 val_h,val_l,wait_sec = 3,wait_tick;

	d_printf("test HWTimer wrap irq starts\n");
	timer_reset();
	// disable interval irq
	hwp_timer->HWTimer_Ctrl &= (~TIMER_AP_INTERVAL_EN);
	// disable warp irq
	hwp_timer->Timer_Irq_Mask_Clr = TIMER_AP_HWTIMER_WRAP_MASK;
	// clear warp irq
	hwp_timer->Timer_Irq_Clr = TIMER_AP_HWTIMER_WRAP_CLR;
	// enable wrap irq
	hwp_timer->Timer_Irq_Mask_Set = TIMER_AP_HWTIMER_WRAP_MASK;
	wait_tick = 0xFFFFFFFF - caculate_tick_val(wait_sec * 1000000);
	sec_count = 0;
	d_printf("\nWaiting high 32 bit Counter increasing to 0x%x...\n",
			0xFFFFFFFE);
	while(1) {
		val_h = hwp_timer->HWTimer_LockVal_H;
		val_l = hwp_timer->HWTimer_LockVal_L;
		do_waiting_print(&sec_count,0,".");
		if(val_h == 0xFFFFFFFE)
			break;
	}
	d_printf("\nWaiting low 32 bit counter increasing to 0x%x\n",
			wait_tick);
	while(1) {
		val_h = hwp_timer->HWTimer_LockVal_H;
		val_l = hwp_timer->HWTimer_LockVal_L;
		do_waiting_print(&sec_count,0,".");
		if(val_l >= wait_tick)
			break;
	}
	d_printf("\nWaiting Wrap interrupt occuring...\n");
	while((hwp_timer->Timer_Irq_Cause & TIMER_AP_HWTIMER_WRAP_CAUSE) == 0);
	val_h = hwp_timer->HWTimer_LockVal_H;
	val_l = hwp_timer->HWTimer_LockVal_L;
	hwp_timer->Timer_Irq_Clr = TIMER_AP_HWTIMER_WRAP_CLR;
	while((hwp_timer->Timer_Irq_Cause & TIMER_AP_HWTIMER_WRAP_CAUSE) != 0);
	// disable warp irq
	hwp_timer->Timer_Irq_Mask_Clr = TIMER_AP_HWTIMER_WRAP_MASK;
	// clear wrap interrupt status
	hwp_timer->Timer_Irq_Clr = TIMER_AP_HWTIMER_WRAP_CLR;
	d_printf("Time elasped 0x%x us\n",sec_count);
	d_printf("When wrap irq occur, the timer tick is H:%x,L:%x\n",
			val_h,val_l);
	// after warp interrupt occur, the counter should change from maximum value to 0.
	if((val_l & 0xFFFFFFF0) != 0) {
		d_printf("HWTimer warp irq test failed ###\n");
		return -1;
	} else {
		d_printf("HWTimer warp irq test success !!!\n");
		return 0;
	}
}

/******************************************************************************
 * @name:	 hwtimer_lockval_function
 * @description: to test hwtimer lock function
 * @parameter:
 * @notes:
 * @return: 0(SUCCESS), -1(FAILED)
 ******************************************************************************
 */
int hwtimer_lock_value(void)
{
	u32 val_h,val_l;
	u32 wait_tick = 0, wait_sec = 3,cnt_1s = 0;
	int ok = 0;

	d_printf("\nHWTimer lock value starts\n");
	wait_tick = 0xFFFFFFFF - caculate_tick_val(wait_sec * 1000000);
	timer_reset();
	val_l = hwp_timer->HWTimer_LockVal_L;
	val_h = hwp_timer->HWTimer_LockVal_H;
	if(val_h != 0) {
		d_printf("High 32 bit counter is not zero,exit\n");
		return -1;
	}
	d_printf("High 32 bit counter val: 0x%x\n",val_h);
	d_printf("Low 32 bit counter val : 0x%x\n",val_l);
	d_printf("\nWaiting for low 32 bit counter increasing to 0x%x...\n",wait_tick);
	while(1) {
		val_l = hwp_timer->HWTimer_LockVal_L;
		val_h = hwp_timer->HWTimer_LockVal_H;
		if(val_l >= wait_tick)
			break;
		do_waiting_print(NULL,0,".");
		cnt_1s++;
	}
	d_printf("\nTime has elapsed 0x%x seconds\n",cnt_1s);
	d_printf("\nWaiting for jitter occuring...\n");
	while(1) {
		val_l = hwp_timer->HWTimer_LockVal_L;
		val_h = hwp_timer->HWTimer_LockVal_H;
		if((val_l == 0) || (val_l <= 0xFF))
			break;
	}
	// when jitter occur, 		val_h = 0x00000000, val_l = 0xffffffff,
	// if okay, next should be: 	val_h = 0x00000001, val_l = 0x00000000
	// if error, maybe: 		val_h = 0x00000000, val_h = 0x00000000;
	if(val_h == 1)
		ok = 1;
	d_printf("Get last counter high 32 bit: 0x%x\n",val_h);
	d_printf("Get last counter low  32 bit: 0x%x\n",val_l);
	if(ok) {
		d_printf("HWTimer lock value test success !!!\n");
		return 0;
	} else {
		d_printf("HWTimer lock value test failed ###\n");
		return -1;
	}
}

/******************************************************************************
 * @name:	 get_ostimer_register_value
 * @description: to test ostimer register default value
 * @parameter:
 * @notes:
 * @return: 0(SUCCESS), -1(FAILED)
 ******************************************************************************
 */

int ostimer_reg_default_value(int is_reset)
{
	u32 reg_val[10];
	int idx = 0;

	d_printf("read OSTimer register value ...\n");
	if(is_reset > 0) {
	    timer_reset();
	}
	reg_val[idx++] = hwp_timer->OSTimer_Ctrl;
	reg_val[idx++] = hwp_timer->OSTimer_LoadVal_L;
	reg_val[idx++] = hwp_timer->OSTimer_CurVal_H;
	reg_val[idx++] = hwp_timer->OSTimer_CurVal_L;
	reg_val[idx++] = hwp_timer->OSTimer_LockVal_H;
	reg_val[idx++] = hwp_timer->OSTimer_LockVal_L;
	reg_val[idx++] = hwp_timer->Timer_Irq_Mask_Set;
	reg_val[idx++] = hwp_timer->Timer_Irq_Mask_Clr;
	reg_val[idx++] = hwp_timer->Timer_Irq_Cause;
	idx = 0;
	d_printf("OSTimer_Ctrl:       %x\n",reg_val[idx++]);
	d_printf("OSTimer_LoadVal_L:  %x\n",reg_val[idx++]);
	d_printf("OSTimer_CurVal_H:   %x\n",reg_val[idx++]);
	d_printf("OSTimer_CurVal_L:   %x\n",reg_val[idx++]);
	d_printf("OSTimer_LockVal_H:  %x\n",reg_val[idx++]);
	d_printf("OSTimer_LockVal_L:  %x\n",reg_val[idx++]);
	d_printf("Timer_Irq_Mask_Set: %x\n",reg_val[idx++]);
	d_printf("Timer_Irq_Mask_Clr: %x\n",reg_val[idx++]);
	d_printf("Timer_Irq_Cause:    %x\n",reg_val[idx++]);
	return 0;
}

static void ostimer_enable(int is_enable)
{
	if(is_enable) {
		hwp_timer->OSTimer_Ctrl |= TIMER_AP_ENABLE;
		while((hwp_timer->OSTimer_Ctrl & TIMER_AP_ENABLED) == 0);
	} else {
		hwp_timer->OSTimer_Ctrl &= (u32)(~TIMER_AP_ENABLE);
		while((hwp_timer->OSTimer_Ctrl & TIMER_AP_ENABLED) != 0);
	}
}

static void ostimer_clear_irq(void)
{
	// Clear IRQ status
	hwp_timer->Timer_Irq_Clr = TIMER_AP_OSTIMER_CLR;
	// Waiting until state is cleared
	//while((hwp_timer->OSTimer_Ctrl & TIMER_AP_CLEARED) != 0);
	while((hwp_timer->Timer_Irq_Cause & TIMER_AP_OSTIMER_CAUSE) != 0);
}

/******************************************************************************
 * @name:	    ostimer_test_irq_status
 * @description:    to check irq status wether is set when interrupt occuring.
 * @parameter:
 *		tick: 	   the initial value of ostimer starts
 * @notes:
 * @return: 0(SUCCESS), -1(FAILED)
 ******************************************************************************
 */

int ostimer_test_irq_status(u32 us)
{
	u64 temp;
	u32 load_tick_h,load_tick_l;
	int ok = 0;

	d_printf("\nOSTimer test irq status starts\n");
	// disable timer
	ostimer_enable(0);
	// repeat mode,not wrap
	hwp_timer->OSTimer_Ctrl &= (~TIMER_AP_WRAP);
	hwp_timer->OSTimer_Ctrl |= TIMER_AP_REPEAT;
	// write initial value
	temp = caculate_tick_val(us);
	load_tick_l = 0xFFFFFFFF & temp;
#ifdef USE_DATA_SIZE_64_BIT
	load_tick_h = 0xFFFFFF & (temp >> 32);
#else
	load_tick_h = 0;
#endif
	hwp_timer->OSTimer_LoadVal_L = load_tick_l;
	hwp_timer->OSTimer_Ctrl &= (~TIMER_AP_OS_LOADVAL_H_MASK);
	hwp_timer->OSTimer_Ctrl |= load_tick_h;
	// load to initial value
	hwp_timer->OSTimer_Ctrl |= TIMER_AP_LOAD;
	// clear irq status
	ostimer_clear_irq();
	d_printf("IRQ Mask is cleared\n");
	// clear mask
	hwp_timer->Timer_Irq_Mask_Clr = TIMER_AP_OSTIMER_MASK;
	// print value for debug
	d_printf("OSTimer_Ctrl:       0x%x\n",hwp_timer->OSTimer_Ctrl);
	d_printf("OSTimer_LoadVal_L:  0x%x\n",hwp_timer->OSTimer_LoadVal_L);
	d_printf("Timer_Irq_Mask_Set: 0x%x\n",hwp_timer->Timer_Irq_Mask_Set);
	d_printf("Timer_Irq_Mask_Clr: 0x%x\n",hwp_timer->Timer_Irq_Mask_Clr);
	d_printf("Timer_Irq_Cause:    0x%x\n",hwp_timer->Timer_Irq_Cause);
	d_printf("OSTimer_LockVal_H:  0x%x\n",hwp_timer->OSTimer_LockVal_H);
	d_printf("OSTimer_LockVal_L:  0x%x\n",hwp_timer->OSTimer_LockVal_L);
	d_printf("\n");
	d_printf("Enable timer ...\n");
	d_printf("Waiting for interrupt occuring ...\n");
	ostimer_enable(1);
	// waiting for irq cause state
	while(1) {
		if(hwp_timer->Timer_Irq_Cause & TIMER_AP_OSTIMER_STATUS) {
			// the irq cause flag should not be 1
			if((hwp_timer->Timer_Irq_Cause & TIMER_AP_OSTIMER_CAUSE) == 0)
				ok = 1;
			else
				ok = 0;
			break;
		}
	}
	d_printf("\n");
	d_printf("OSTimer_Ctrl:       0x%x\n",hwp_timer->OSTimer_Ctrl);
	d_printf("OSTimer_LoadVal_L:  0x%x\n",hwp_timer->OSTimer_LoadVal_L);
	d_printf("Timer_Irq_Mask_Set: 0x%x\n",hwp_timer->Timer_Irq_Mask_Set);
	d_printf("Timer_Irq_Mask_Clr: 0x%x\n",hwp_timer->Timer_Irq_Mask_Clr);
	d_printf("Timer_Irq_Cause:    0x%x\n",hwp_timer->Timer_Irq_Cause);
	d_printf("OSTimer_LockVal_H:  0x%x\n",hwp_timer->OSTimer_LockVal_H);
	d_printf("OSTimer_LockVal_L:  0x%x\n",hwp_timer->OSTimer_LockVal_L);
	d_printf("\n");
	d_printf("disable timer ...\n");
	ostimer_enable(0);
	ostimer_clear_irq();
	if(ok) {
		d_printf("\nOSTimer irq status test success !!!\n");
		return 0;
	} else {
		d_printf("\nOSTimer irq status test failed ###\n");
		return -1;
	}
}

/******************************************************************************
 * @name:	 ostimer_delay_us
 * @description: delay n us by ostimer
 * @parameter:
 * @notes:
 * @return: 0(SUCCESS), -1(FAILED)
 ******************************************************************************
 */
void ost_delay_us(u32 us)
{
	u32 load_tick_h,load_tick_l;
	u64 temp;

	// disable timer
	ostimer_enable(0);
	// repeat mode,not wrap
	hwp_timer->OSTimer_Ctrl &= (~TIMER_AP_WRAP);
	hwp_timer->OSTimer_Ctrl |= TIMER_AP_REPEAT;
	// write initial value
	temp = caculate_tick_val(us);
	load_tick_l = 0xFFFFFFFF & temp;
#ifdef USE_DATA_SIZE_64_BIT
	load_tick_h = 0xFFFFFF & (temp >> 32);
#else
	load_tick_h = 0x0;
#endif
	hwp_timer->OSTimer_LoadVal_L = load_tick_l;
	hwp_timer->OSTimer_Ctrl &= (~TIMER_AP_OS_LOADVAL_H_MASK);
	hwp_timer->OSTimer_Ctrl |= load_tick_h;
	// load to initial value
	hwp_timer->OSTimer_Ctrl |= TIMER_AP_LOAD;
	// clear irq status
	ostimer_clear_irq();
	// set mask
	hwp_timer->Timer_Irq_Mask_Set = TIMER_AP_OSTIMER_MASK;
	// enable timer
	ostimer_enable(1);
	// waiting for irq cause state
	while(1) {
		if(hwp_timer->Timer_Irq_Cause & TIMER_AP_OSTIMER_CAUSE) {
			break;
		}
	}
	ostimer_enable(0);
	ostimer_clear_irq();
	return;
}

int ostimer_delay_wave(int period_us,int times)
{
	u32 half_period = period_us / 2;
	int cnt = 0;
	int gpo_pin = 0;

	d_printf("OSTimer delay wave starts\n");
	while(1) {
		if(cnt % 2) {
			do_set_gpo_pin(gpo_pin,0);
		} else {
			do_set_gpo_pin(gpo_pin,1);
		}
		ost_delay_us(half_period);
		cnt++;
		if(cnt >= times)
			break;
	}
	d_printf("OSTimer delay wave test success !!!\n");
	return 0;
}

int ostimer_delay_us(u32 us)
{
	u32 load_tick_h,load_tick_l,real_dly,err_dly,max_err = 5,real_err;
	u64 temp;
	int ok = 0;
	TIMER timer_a,timer_b;

	d_printf("\nOSTimer delay %d us ...\n",us);
	// disable timer
	ostimer_enable(0);
	// repeat mode,not wrap
	hwp_timer->OSTimer_Ctrl &= (~TIMER_AP_WRAP);
	hwp_timer->OSTimer_Ctrl |= TIMER_AP_REPEAT;
	// write initial value
	temp = caculate_tick_val(us);
	load_tick_l = 0xFFFFFFFF & temp;
#ifdef USE_DATA_SIZE_64_BIT
	load_tick_h = 0xFFFFFF & (temp >> 32);
#else
	load_tick_h = 0x0;
#endif
	hwp_timer->OSTimer_LoadVal_L = load_tick_l;
	hwp_timer->OSTimer_Ctrl &= (~TIMER_AP_OS_LOADVAL_H_MASK);
	hwp_timer->OSTimer_Ctrl |= load_tick_h;
	// load to initial value
	hwp_timer->OSTimer_Ctrl |= TIMER_AP_LOAD;
	// clear irq status
	ostimer_clear_irq();
	// set mask
	hwp_timer->Timer_Irq_Mask_Set = TIMER_AP_OSTIMER_MASK;
	// print value for debug
	d_printf("OSTimer_Ctrl:       0x%x\n",hwp_timer->OSTimer_Ctrl);
	d_printf("OSTimer_LoadVal_L:  0x%x\n",hwp_timer->OSTimer_LoadVal_L);
	d_printf("Timer_Irq_Mask_Set: 0x%x\n",hwp_timer->Timer_Irq_Mask_Set);
	d_printf("Timer_Irq_Mask_Clr: 0x%x\n",hwp_timer->Timer_Irq_Mask_Clr);
	d_printf("Timer_Irq_Cause:    0x%x\n",hwp_timer->Timer_Irq_Cause);
	d_printf("OSTimer_LockVal_H:  0x%x\n",hwp_timer->OSTimer_LockVal_H);
	d_printf("OSTimer_LockVal_L:  0x%x\n",hwp_timer->OSTimer_LockVal_L);
	d_printf("Enable timer...\n");
	d_printf("Waiting for interrupt occuring...\n");
	ostimer_enable(1);
	get_hwtimer_time(&timer_a);
	// waiting for irq cause state
	while(1) {
		if(hwp_timer->Timer_Irq_Cause & TIMER_AP_OSTIMER_CAUSE) {
			get_hwtimer_time(&timer_b);
			break;
		}
	}
	real_dly = caculate_time_val((timer_b.fields.timer_l - timer_a.fields.timer_l));
	err_dly = (real_dly > us) ? (real_dly - us):(us - real_dly);
	real_err = (err_dly * 100) / real_dly;
	if(real_err <= max_err)
		ok = 1;
	else
		ok = 0;

	d_printf("\n");
	d_printf("Expected delay: 0x%x\n",us);
	d_printf("Real delay:     0x%x\n",real_dly);
	d_printf("Error value:    0x%x\n",err_dly);
	d_printf("Error percent:  0x%x\n",real_err);
	d_printf("\n");
	d_printf("OSTimer_Ctrl:       0x%x\n",hwp_timer->OSTimer_Ctrl);
	d_printf("OSTimer_LoadVal_L:  0x%x\n",hwp_timer->OSTimer_LoadVal_L);
	d_printf("Timer_Irq_Mask_Set: 0x%x\n",hwp_timer->Timer_Irq_Mask_Set);
	d_printf("Timer_Irq_Mask_Clr: 0x%x\n",hwp_timer->Timer_Irq_Mask_Clr);
	d_printf("Timer_Irq_Cause:    0x%x\n",hwp_timer->Timer_Irq_Cause);
	d_printf("OSTimer_LockVal_H:  0x%x\n",hwp_timer->OSTimer_LockVal_H);
	d_printf("OSTimer_LockVal_L:  0x%x\n",hwp_timer->OSTimer_LockVal_L);
	d_printf("\n");
	d_printf("disable timer\n");
	ostimer_enable(0);
	ostimer_clear_irq();
	if(ok) {
		d_printf("\nerror %d is less than %d,test success !!!\n"
				,real_err,max_err);
		return 0;
	} else {
		d_printf("\nerror %d is greater than %d,test failed ####\n"
				,real_err,max_err);
		return -1;
	}
}

/******************************************************************************
 * @name:	 ostimer_delay_ms
 * @description: delay n ms by ostimer
 * @parameter:
 * @notes:
 * @return: 0(SUCCESS), -1(FAILED)
 ******************************************************************************
 */

int ostimer_delay_ms(u32 ms)
{
	d_printf("OSTimer delay %d ms \n",ms);
	ostimer_delay_us(ms * 1000);
	d_printf("OSTimer delay ms test success !!!\n");
	return 0;
}

/******************************************************************************
 * @name:	    ostimer_repeat_irq
 * @description:    to generate irq status when ostimer work at repeat mode
 * @parameter:
 *		period: 	the interrupt period us time value
 *		irq_times: the irq occuring times when ostimer is running.
 * @notes:
 * @return: 0(SUCCESS), -1(FAILED)
 ******************************************************************************
 */

int ostimer_repeat_irq(u32 period,int irq_times)
{
	u32 load_tick_h,load_tick_l,real_period,err_period,err_percent,max_err_percent = 5;
	int irq_cnt = 0,ok = 0;
	u64 temp;
	TIMER timer_a,timer_b;

	d_printf("\nOSTimer repeat irq %d times with period %d\n",irq_times,period);
	// disable timer
	ostimer_enable(0);
	// repeat mode,not wrap
	hwp_timer->OSTimer_Ctrl |= TIMER_AP_REPEAT;
	hwp_timer->OSTimer_Ctrl &= (~TIMER_AP_WRAP);
	// write initial value
	temp = caculate_tick_val(period);
	load_tick_l = 0xFFFFFFFF & (temp);
#ifdef USE_DATA_SIZE_64_BIT
	load_tick_h = 0xFFFFFF & (temp >> 32);
#else
	load_tick_h = 0x0;
#endif
	hwp_timer->OSTimer_LoadVal_L = load_tick_l;
	hwp_timer->OSTimer_Ctrl &= (~TIMER_AP_OS_LOADVAL_H_MASK);
	hwp_timer->OSTimer_Ctrl |= load_tick_h;
	// load to initial value
	hwp_timer->OSTimer_Ctrl |= TIMER_AP_LOAD;
	// clear irq status
	ostimer_clear_irq();
	// set mask
	hwp_timer->Timer_Irq_Mask_Set = TIMER_AP_OSTIMER_MASK;
	// print value for debug
	d_printf("OSTimer_Ctrl:       0x%x\n",hwp_timer->OSTimer_Ctrl);
	d_printf("OSTimer_LoadVal_L:  0x%x\n",hwp_timer->OSTimer_LoadVal_L);
	d_printf("Timer_Irq_Mask_Set: 0x%x\n",hwp_timer->Timer_Irq_Mask_Set);
	d_printf("Timer_Irq_Mask_Clr: 0x%x\n",hwp_timer->Timer_Irq_Mask_Clr);
	d_printf("Timer_Irq_Cause:    0x%x\n",hwp_timer->Timer_Irq_Cause);
	d_printf("OSTimer_LockVal_H:  0x%x\n",hwp_timer->OSTimer_LockVal_H);
	d_printf("OSTimer_LockVal_L:  0x%x\n",hwp_timer->OSTimer_LockVal_L);
	d_printf("\n");
	d_printf("Enable timer...\n");
	d_printf("Waiting for interrupt occuring...\n");
	ostimer_enable(1);
	get_hwtimer_time(&timer_a);
	// waiting for irq cause state
	irq_cnt = 0;
	while(1) {
		if(hwp_timer->Timer_Irq_Cause & TIMER_AP_OSTIMER_CAUSE) {
			d_printf(".");
			ostimer_clear_irq();
			irq_cnt++;
		}
		if(irq_cnt >= irq_times) {
			get_hwtimer_time(&timer_b);
			break;
		}
	}
	real_period = caculate_time_val((timer_b.fields.timer_l - timer_a.fields.timer_l)) / irq_times;
	err_period = (real_period > period) ? (real_period - period):(period - real_period);
	err_percent = (err_period * 100) / period;
	if(err_percent <= max_err_percent)
		ok = 1;
	else
		ok = 0;
	// disable timer
	ostimer_enable(0);
	// clear irq status
	ostimer_clear_irq();
	d_printf("\n");
	d_printf("Expected period: 0x%x\n",period);
	d_printf("Real period:     0x%x\n",real_period);
	d_printf("Error value:     0x%x\n",err_period);
	d_printf("Error percent:   0x%x\n",err_percent);
	d_printf("\n");
	if(ok) {
		d_printf("\nerror %d is less than %d,test success !!!\n"
				,err_percent,max_err_percent);
		return 0;
	} else {
		d_printf("\nerr %d is greater than %d,test failed ###\n"
				,err_percent,max_err_percent);
		return -1;
	}
}

/******************************************************************************
 * @name:	    ostimer_wrap_irq
 * @description:    to generate irq status when os timer work at wrap mode
 * @parameter:
 *		tick: 	   the initial value of ostimer starts
 *		irq_times: the irq occuring times when ostimer is running.
 * @notes:
 * @return: 0(SUCCESS), -1(FAILED)
 ******************************************************************************
 */

int ostimer_wrap_irq(u64 tick, int irq_times)
{
	u32 load_tick_h,load_tick_l;
	int irq_cnt = 0,print_cnt = 0;
	u32 tick_val_h,tick_val_l;
	u32 tick_val_hh,tick_val_ll;

	print_cnt = print_cnt;
	d_printf("\nOSTimer wrap irq starts\n");
	// disable timer
	ostimer_enable(0);
	// wrap mode,not repeat
	hwp_timer->OSTimer_Ctrl |= TIMER_AP_WRAP;
	hwp_timer->OSTimer_Ctrl &= (~TIMER_AP_REPEAT);
	// write initial value
	load_tick_l = 0xFFFFFFFF & tick;
#ifdef USE_DATA_SIZE_64_BIT
	load_tick_h = 0xFFFFFF & (tick >> 32);
#else
	load_tick_h = 0x0;
#endif
	hwp_timer->OSTimer_LoadVal_L = load_tick_l;
	hwp_timer->OSTimer_Ctrl &= (~TIMER_AP_OS_LOADVAL_H_MASK);
	hwp_timer->OSTimer_Ctrl |= load_tick_h;
	// load to initial value
	hwp_timer->OSTimer_Ctrl |= TIMER_AP_LOAD;
	// clear irq status
	ostimer_clear_irq();
	// set mask
	hwp_timer->Timer_Irq_Mask_Set = TIMER_AP_OSTIMER_MASK;
	// print value for debug
	d_printf("OSTimer_Ctrl:       0x%x\n",hwp_timer->OSTimer_Ctrl);
	d_printf("OSTimer_LoadVal_L:  0x%x\n",hwp_timer->OSTimer_LoadVal_L);
	d_printf("Timer_Irq_Mask_Set: 0x%x\n",hwp_timer->Timer_Irq_Mask_Set);
	d_printf("Timer_Irq_Mask_Clr: 0x%x\n",hwp_timer->Timer_Irq_Mask_Clr);
	d_printf("Timer_Irq_Cause:    0x%x\n",hwp_timer->Timer_Irq_Cause);
	d_printf("OSTimer_LockVal_H:  0x%x\n",hwp_timer->OSTimer_LockVal_H);
	d_printf("OSTimer_LockVal_L:  0x%x\n",hwp_timer->OSTimer_LockVal_L);
	d_printf("\n");
	d_printf("Enable timer...\n");
	d_printf("Waiting for interrupt occuring...\n");
	// enable  timer
	ostimer_enable(1);
	// waiting for irq cause state
	irq_cnt = 0;
	while(1) {
		if(hwp_timer->Timer_Irq_Cause & TIMER_AP_OSTIMER_CAUSE) {
			// clear irq status
			ostimer_clear_irq();
			irq_cnt++;
		}
		if(irq_cnt >= irq_times) {
			// delay sometime, then to check lock value which should be the maximum value
			//hwt_delay_us(1000);
			tick_val_hh = hwp_timer->OSTimer_LockVal_H;
			tick_val_ll = hwp_timer->OSTimer_LockVal_L;
			break;
		}
	}
	hwt_delay_us(10);
	tick_val_h = hwp_timer->OSTimer_LockVal_H;
	tick_val_l = hwp_timer->OSTimer_LockVal_L;
	// disable timer
	ostimer_enable(0);
	// clear irq status
	ostimer_clear_irq();
	d_printf("OSTimer_LockVal_HH: 0x%x\n",tick_val_hh);
	d_printf("OSTimer_LockVal_LL: 0x%x\n",tick_val_ll);
	d_printf("OSTimer_LockVal_H: 0x%x\n",tick_val_h);
	d_printf("OSTimer_LockVal_L: 0x%x\n",tick_val_l);
	if(tick_val_h == 0xFFFFFFFF) {
		d_printf("\nOSTimer wrap irq test success !!!\n");
		return 0;
	} else {
		d_printf("\nOSTimer wrap irq test failed ###\n");
		return -1;
	}
}

/******************************************************************************
 * @name:	    ostimer_lock_value
 * @description:    to check lock function if worked when counter's value is
 *		    changing.
 * @parameter:
 *		tick: 	   the initial value of ostimer starts
 * @notes:
 * @return: 0(SUCCESS), -1(FAILED)
 ******************************************************************************
 */

int ostimer_lock_value(void)
{
	int ok = 0;
	u32 read_val_h,read_val_l;
	u32 val_h,val_l;

	d_printf("\nOSTimer lock value test starts\n");
	ostimer_enable(0);
	// wrap mode,not repeat mode
	hwp_timer->OSTimer_Ctrl |= TIMER_AP_WRAP;
	hwp_timer->OSTimer_Ctrl &= (~TIMER_AP_REPEAT);
	// write load value
	val_h = 0x00000001;
	val_l = 0x00000000;
	hwp_timer->OSTimer_LoadVal_L = TIMER_AP_OS_LOADVAL_L(val_l);
	hwp_timer->OSTimer_Ctrl &= ~(TIMER_AP_OS_LOADVAL_H_MASK);
	hwp_timer->OSTimer_Ctrl |= TIMER_AP_OS_LOADVAL_H(val_h);
	// load to initial value
	hwp_timer->OSTimer_Ctrl |= TIMER_AP_LOAD;
	// clear irq status
	ostimer_clear_irq();
	// set mask
	hwp_timer->Timer_Irq_Mask_Set = TIMER_AP_OSTIMER_MASK;
	d_printf("Waiting for jitter occur...\n");
	// enable timer
	ostimer_enable(1);
	while(1) {
			read_val_h = hwp_timer->OSTimer_LockVal_H;
			read_val_l = hwp_timer->OSTimer_LockVal_L;
			if(read_val_h == 0)
				break;
	}
	// disable timer
	ostimer_enable(0);
	// clear irq status
	ostimer_clear_irq();
	/* When jitter occur,	read_val_h = 0x00000001,read_val_l = 0x00000000
		if okay,	read_val_h = 0x00000000,read_val_l = 0xFFFFFFFF
		if error,	read_val_h = 0x00000000,read_val_l = 0x00000000
	*/
	if((read_val_l & 0xFFFFFF00) == 0xFFFFFF00)
		ok = 1;
	else
		ok = 0;
	// read value
	d_printf("Last lock value h: 0x%x\n",read_val_h);
	d_printf("Last lock value l: 0x%x\n",read_val_l);
	if(ok) {
		d_printf("OSTimer lock value test success!!!\n");
		return 0;
	} else {
		d_printf("OSTimer lock value test failed ###\n");
		return -1;
	}
}

int timer_simple_test_in(unsigned int period)
{
	int cnt = 0;
	d_printf("print message with period %d ms\n",(int)period);
	do {
		hwt_delay_ms(period); // delay one second
		d_printf("Hello world,0x%x\n",cnt);
		cnt++;
	}while(cnt<5);
	d_printf("test finished\n");
	return 0;
}

/*
 * TIMER FULL TEST
 *********************************************************
 */
int timer_full_test_in(int timer_id, enum TIMER_TC_ID_TYPE case_id)
{
	int ret = 0;
	// reset timer
	timer_reset();
	hwt_delay_us(10);

	switch(case_id) {
	case HWTIMER_TC_ID_REG_DEFAULT_VAL:
		ret = hwtimer_reg_default_value(1);
		break;
	case HWTIMER_TC_ID_DELAY_US:
		//ret  = hwtimer_delay_us(61);
		//ret += hwtimer_delay_us(610);
		ret += hwtimer_delay_us(1000);
		break;
	case HWTIMER_TC_ID_DELAY_MS:
		ret  = hwtimer_delay_ms(50);
		ret += hwtimer_delay_ms(100);
		ret += hwtimer_delay_ms(1000);
		break;
	case HWTIMER_TC_ID_DELAY_S:
		ret  = hwtimer_delay_s(1);
		ret += hwtimer_delay_s(2);
		break;
	case HWTIMER_TC_ID_DELAY_WAVE_1MS:
		// output 10000 wave which period is 1000 us,
		ret = hwtimer_delay_wave(1000,10000);
		break;
	case HWTIMER_TC_ID_INTERVAL_IRQ_125MS:
		ret  = hwtimer_interval_irq(IT_PERIOD_125MS,4);
		break;
	case HWTIMER_TC_ID_INTERVAL_IRQ_250MS:
		ret = hwtimer_interval_irq(IT_PERIOD_250MS,4);
		break;
	case HWTIMER_TC_ID_INTERVAL_IRQ_500MS:
		ret = hwtimer_interval_irq(IT_PERIOD_500MS,2);
		break;
	case HWTIMER_TC_ID_INTERVAL_IRQ_1000MS:
		ret = hwtimer_interval_irq(IT_PERIOD_1000MS,2);
		break;
	case HWTIMER_TC_ID_IRQ_WAVE_125MS:
		ret = hwtimer_interval_irq_wave(IT_PERIOD_125MS,80);
		break;
	case HWTIMER_TC_ID_IRQ_WAVE_250MS:
		ret = hwtimer_interval_irq_wave(IT_PERIOD_250MS,40);
		break;
	case HWTIMER_TC_ID_IRQ_WAVE_500MS:
		ret = hwtimer_interval_irq_wave(IT_PERIOD_500MS,20);
		break;
	case HWTIMER_TC_ID_IRQ_WAVE_1000MS:
		ret = hwtimer_interval_irq_wave(IT_PERIOD_1000MS,10);
		break;
	case HWTIMER_TC_ID_WRAP_IRQ:
		ret = hwtimer_wrap_irq(1);//test only once
		break;
	case HWTIMER_TC_ID_LOCK_VAL:
		ret = hwtimer_lock_value();
		break;
	case OSTIMER_TC_ID_REG_DEFAULT_VAL:
		ret = ostimer_reg_default_value(1);
		break;
	case OSTIMER_TC_ID_DELAY_US:
		ret = ostimer_delay_us(500); // delay 500 us
		break;
	case OSTIMER_TC_ID_DELAY_MS:
		ret = ostimer_delay_ms(5); // delay 500 ms
		break;
	case OSTIMER_TC_ID_DELAY_WAVE_1MS:
		ret = ostimer_delay_wave(1000,10000);
		break;
	case OSTIMER_TC_ID_TEST_IRQ_STATUS:
		ret = ostimer_test_irq_status(500); // delay 500 us for checking irq status and irq cause
		break;
	case OSTIMER_TC_ID_IRQ_REPEAT:
		ret = ostimer_repeat_irq(10000,10); // period = 10 ms, 10 times
		break;
	case OSTIMER_TC_ID_IRQ_WRAP:
		ret = ostimer_wrap_irq(0x1000,1); //initial tick = 0x1000, test only once
		break;
	case OSTIMER_TC_ID_LOCK_VAL:
		ret = ostimer_lock_value();
		break;
	default:
		d_printf("Test case is not found\n");
		ret = -1;
		break;
	}
	return ret;
}

void config_clock_source(void)
{
	char ch;
	while(1) {
		d_printf("\nSelect TIMER clock source\n"
			"\t 1: 2MHz clock source\n"
			"\t 2: 16KHz clock source\n"
			"\t q: exit\n");
		ch = serial_getc();
		if(ch == 'q') {
			d_printf("Exit configuration\n");
			return;
		}
		d_printf("Input cmd: ");
		serial_putc(ch);
		d_printf("\n");
		switch(ch) {
		case '1':
			set_timer_clk_freq(1);
			d_printf("Set 2MHz clock source OK\n");
			break;
		case '2':
			set_timer_clk_freq(0);
			d_printf("Set 16KHz clock source OK\n");
			break;
		default:
			d_printf("Invalid cmd\n");
			break;
		}
	}
}

void timer_single_test_in(int timer_id)
{
	char choice;
	int case_id = 0xff;
	d_printf("**************************************************************"
		"\n\tThis program is used to output wave from GPO 0\n"
		"\tAll waves are generated by HWTimer and OSTimer\n"
		"*************************************************************");
	while(1) {
		d_printf("\nInput your choice:\n"
			"\t1: wave period 1 ms by hwtimer\n"
			"\t2: wave period 250ms\n"
			"\t3: wave period 500ms\n"
			"\t4: wave period 1000ms\n"
			"\t5: wave period 2000ms\n"
			"\t6: wave period 1 ms by ostimer\n"
			"\tq: exit\n");
		choice = serial_getc();
		if(choice == 'q') {
			d_printf("Have a nice day !\n");
			return;
		}
		d_printf("Input cmd: ");
		serial_putc(choice);
		d_printf("\n");
		switch(choice) {
			case '1': case_id = HWTIMER_TC_ID_DELAY_WAVE_1MS;break;
			case '2': case_id = HWTIMER_TC_ID_IRQ_WAVE_125MS;break;
			case '3': case_id = HWTIMER_TC_ID_IRQ_WAVE_250MS;break;
			case '4': case_id = HWTIMER_TC_ID_IRQ_WAVE_500MS;break;
			case '5': case_id = HWTIMER_TC_ID_IRQ_WAVE_1000MS;break;
			case '6': case_id = OSTIMER_TC_ID_DELAY_WAVE_1MS;break;
			default: d_printf("\nInvalied input\n"); case_id = 0xFF;break;
		}
		if(case_id != 0xFF)
			timer_full_test_in(timer_id, case_id);
	}
}

#ifdef CONFIG_IRQ
int start_ostimer(u32 us)
{
	u64 temp;
	u32 load_tick_h,load_tick_l;

	d_printf("OSTimer runs at repeat irq mode,period is %d us\n",us);
	// disable timer
	ostimer_enable(0);
	// repeat mode,not wrap
	hwp_timer->OSTimer_Ctrl &= (~TIMER_AP_WRAP);
	hwp_timer->OSTimer_Ctrl |= TIMER_AP_REPEAT;
	// write initial value
	temp = caculate_tick_val(us);
	load_tick_l = 0xFFFFFFFF & temp;
#ifdef USE_DATA_SIZE_64_BIT
	load_tick_h = 0xFFFFFF & (temp >> 32);
#else
	load_tick_h = 0;
#endif
	hwp_timer->OSTimer_LoadVal_L = load_tick_l;
	hwp_timer->OSTimer_Ctrl &= (~TIMER_AP_OS_LOADVAL_H_MASK);
	hwp_timer->OSTimer_Ctrl |= load_tick_h;
	// load to initial value
	hwp_timer->OSTimer_Ctrl |= TIMER_AP_LOAD;
	// clear irq status
	ostimer_clear_irq();
	// set mask
	hwp_timer->Timer_Irq_Mask_Set |= TIMER_AP_OSTIMER_MASK;
	// print value for debug
#if 0
	d_printf("OSTimer_Ctrl:       0x%x\n",hwp_timer->OSTimer_Ctrl);
	d_printf("OSTimer_LoadVal_L:  0x%x\n",hwp_timer->OSTimer_LoadVal_L);
	d_printf("Timer_Irq_Mask_Set: 0x%x\n",hwp_timer->Timer_Irq_Mask_Set);
	d_printf("Timer_Irq_Mask_Clr: 0x%x\n",hwp_timer->Timer_Irq_Mask_Clr);
	d_printf("Timer_Irq_Cause:    0x%x\n",hwp_timer->Timer_Irq_Cause);
	d_printf("OSTimer_LockVal_H:  0x%x\n",hwp_timer->OSTimer_LockVal_H);
	d_printf("OSTimer_LockVal_L:  0x%x\n",hwp_timer->OSTimer_LockVal_L);
	d_printf("\n");
	d_printf("Enable timer ...\n");
	d_printf("Waiting for interrupt occuring ...\n");
#endif
	d_printf("waiting for irq occuring ...\n");
	ostimer_enable(1);
	return 0;
}

int stop_ostimer(void)
{
	ostimer_enable(0);
//	timer_reset();
	return 0;
}

static volatile int ostimer_cb_tick = 0;
void ostimer_callback(void)
{
	d_printf("ostimer tick is %d\n",++ostimer_cb_tick);
	//clear OSTimer irq status
	ostimer_clear_irq();
}

void ostimer_interrupt_test(int count)
{
	/* handler OSTIMER pointer */
	hwp_timer = hwp_apTimer;
	/* to stop timer firstly */
	stop_ostimer();
	/* request a irq for OSTIMER */
	irq_request(RDA_IRQ_TIMEROS,ostimer_callback);
	/* enable interrupt */
	irq_unmask(RDA_IRQ_TIMEROS);
	/* clear callback tick value */
	ostimer_cb_tick = 0;
	/* start OSTIMER and set IRQs period is 1 second */
	start_ostimer(1000000);
	d_printf("waiting ...\n");
	while(1) {
		if(ostimer_cb_tick > count) {
			d_printf("stop\n");
			stop_ostimer();
			irq_mask(RDA_IRQ_TIMEROS);
			irq_free(RDA_IRQ_TIMEROS);
			break;
		}
//		d_printf(".");
	}
	d_printf("ostimer irq test finished \n");
}
#endif /* CONFIG_IRQ */

/*
 * TIMER TEST ENTRY
 *********************************************************
 */
int timer_test_entry(int timer_id)
{
	int ret = 0;

	if(timer_id > 2) {
		d_printf("handle timer failed,timer id is %d\n",timer_id);
		return -1;
	}
	hwp_timer = hwp_timer_tab[timer_id];
	if(hwp_timer == NULL) {
		d_printf("timer do not exist,timer id is %d\n",timer_id);
		return -1;
	}
	d_printf("\nstart to test timer %d ...\n",timer_id);
/* This is used to test OSTIMER's IRQ */
#ifdef CONFIG_IRQ
	d_printf("interrupts test ...\n");
	set_timer_clk_freq(1);
	hwt_delay_ms(1000);
	ostimer_interrupt_test(5);
#endif
/* simple test */
#if 1
	d_printf("simple test ...\n");
	set_timer_clk_freq(1);
	hwt_delay_ms(1000);
	ret = timer_simple_test_in(1000);
#endif
/* fast mode test */
#if 1
	d_printf("fast mode test ... clock frequency is 2MHz\n");
	set_timer_clk_freq(1);
	hwt_delay_ms(1000);
	hwt_delay_ms(1000);

	ret = 0;
	ret += timer_full_test_in(timer_id,HWTIMER_TC_ID_REG_DEFAULT_VAL);
	ret += timer_full_test_in(timer_id,HWTIMER_TC_ID_DELAY_US);
	ret += timer_full_test_in(timer_id,HWTIMER_TC_ID_DELAY_MS);
	ret += timer_full_test_in(timer_id,HWTIMER_TC_ID_DELAY_S);

	ret += timer_full_test_in(timer_id,HWTIMER_TC_ID_INTERVAL_IRQ_125MS);
	ret += timer_full_test_in(timer_id,HWTIMER_TC_ID_INTERVAL_IRQ_250MS);
	ret += timer_full_test_in(timer_id,HWTIMER_TC_ID_INTERVAL_IRQ_500MS);
	ret += timer_full_test_in(timer_id,HWTIMER_TC_ID_INTERVAL_IRQ_1000MS);

	ret += timer_full_test_in(timer_id,OSTIMER_TC_ID_REG_DEFAULT_VAL);
	ret += timer_full_test_in(timer_id,OSTIMER_TC_ID_DELAY_US);
	ret += timer_full_test_in(timer_id,OSTIMER_TC_ID_DELAY_MS);
	ret += timer_full_test_in(timer_id,OSTIMER_TC_ID_TEST_IRQ_STATUS);
	ret += timer_full_test_in(timer_id,OSTIMER_TC_ID_IRQ_REPEAT);
	ret += timer_full_test_in(timer_id,OSTIMER_TC_ID_IRQ_WRAP);
	ret += timer_full_test_in(timer_id,OSTIMER_TC_ID_LOCK_VAL);
#endif
/* slow mode test */
#if 1
	d_printf("slow mode test ... clock frequency is 16384Hz\n");
	set_timer_clk_freq(0);
	hwt_delay_ms(1000);
	hwt_delay_ms(1000);

	ret += timer_full_test_in(timer_id,HWTIMER_TC_ID_REG_DEFAULT_VAL);
	ret += timer_full_test_in(timer_id,HWTIMER_TC_ID_DELAY_US);
	ret += timer_full_test_in(timer_id,HWTIMER_TC_ID_DELAY_MS);
	ret += timer_full_test_in(timer_id,HWTIMER_TC_ID_DELAY_S);

/*	ret += timer_full_test_in(timer_id,HWTIMER_TC_ID_INTERVAL_IRQ_125MS);
	ret += timer_full_test_in(timer_id,HWTIMER_TC_ID_INTERVAL_IRQ_250MS);
	ret += timer_full_test_in(timer_id,HWTIMER_TC_ID_INTERVAL_IRQ_500MS);
	ret += timer_full_test_in(timer_id,HWTIMER_TC_ID_INTERVAL_IRQ_1000MS);
*/
	ret += timer_full_test_in(timer_id,OSTIMER_TC_ID_REG_DEFAULT_VAL);
	ret += timer_full_test_in(timer_id,OSTIMER_TC_ID_DELAY_US);
	ret += timer_full_test_in(timer_id,OSTIMER_TC_ID_DELAY_MS);
	ret += timer_full_test_in(timer_id,OSTIMER_TC_ID_TEST_IRQ_STATUS);
	ret += timer_full_test_in(timer_id,OSTIMER_TC_ID_IRQ_REPEAT);
	ret += timer_full_test_in(timer_id,OSTIMER_TC_ID_IRQ_WRAP);
	ret += timer_full_test_in(timer_id,OSTIMER_TC_ID_LOCK_VAL);
#endif
/* lock & wrap test in fast mode */
#if 0
	d_printf("lock and wrap test ... clock frequency is 2MHz\n");
	set_timer_clk_freq(1);
	hwt_delay_ms(1000);
	hwt_delay_ms(1000);

	ret += timer_full_test_in(timer_id,HWTIMER_TC_ID_LOCK_VAL);
	ret += timer_full_test_in(timer_id,HWTIMER_TC_ID_WRAP_IRQ);
#endif
	if(ret != 0)
		d_printf("\nreturn value is %x, test failed !!!\n",(unsigned int)ret);
	else
		d_printf("\nreturn value is %x, test successful!!!\n",(unsigned int)ret);
	hwt_delay_ms(1000);
	hwt_delay_ms(1000);
	d_printf("timer test finished\n\n");
	return ret;
}

int tim_test(int timer_id,int times)
{
	int ret = 0,n = 0;
	while(n < times) {
		n++;
		printf("\ntim_test: %d / %d\n",n,times);
		ret += timer_test_entry(timer_id);
		printf("\ntim_test end,ret is %d\n",ret);
	}
	return ret;
}
