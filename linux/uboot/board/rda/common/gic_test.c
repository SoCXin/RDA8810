#include <common.h>
#include <asm/arch/cs_types.h>
#include <asm/arch/global_macros.h>
#include <asm/arch/reg_sysctrl.h>
#include "rda_gic.h"

#define GIC_TEST_DEBUG

#ifdef GIC_TEST_DEBUG
#define d_printf printf
#else
#define d_printf(...) do{}while(0)
#endif

#define n_printf printf

#define RW_DELAY	10000

#define readl(addr)	(*((volatile unsigned int *)(addr)))
#define writel(val,addr) (*((volatile unsigned int *)(addr)) = (val))

static int g_irq_num = 0;
static int g_cpu_num = 0;
static int read_gic_type(void);
static void soft_dly(unsigned int);

static int read_gic_type(void)
{
	unsigned int reg,security_ext,it_num,cpu_num;

	reg = readl(RDA_GICD_BASE + GIC_DIST_CTR);
	d_printf("CPU CTR reg is %x\n",reg);
	security_ext = (reg >> 10) & 0x1;
	it_num = ((reg & 0x1F) + 1 ) * 32;
	cpu_num = ((reg >> 5) & 0x7) + 1;
	g_irq_num = it_num;
	g_cpu_num = cpu_num;
	if(security_ext)
		n_printf("have security extension,");
	else
		n_printf("no security extension,");
	n_printf("irq number: %d, cpu number: %d\n",it_num,cpu_num);
	if((it_num > 0) && (it_num < 1020))
		return 0;
	else
		return -1;
}

static void soft_dly(unsigned int n)
{
	while(n--);
}

void show_cpu_if_info(void)
{
	unsigned int reg,ver_minor,ver_major,vid;
	reg = readl(RDA_GICC_BASE + GIC_CPU_IID);
	d_printf("CPU IID reg is %x\n",reg);
	vid = 0x0FFF & reg;
	ver_minor = (reg >> 12) & 0xF;
	ver_major = (reg >> 16) & 0xF;
	if(vid != VID_ARM) {
		n_printf("unknown vendor id %x\n",vid);
		return;
	}
	n_printf("CPU INTERFACE: ARM,vendor id: %x, version number: %x.%x\n",
			vid,ver_major,ver_minor);
}

void show_dist_info(void)
{
	unsigned int reg,ver_major,ver_minor,vid;
	reg = readl(RDA_GICD_BASE + GIC_DIST_IID);
	d_printf("DIST IID reg is %x\n",reg);
	vid = 0x0FFF & reg;
	ver_minor = (reg >> 12) & 0xF;
	ver_major = (reg >> 16) & 0xF;
	if(vid != VID_ARM) {
		n_printf("unknown vendor id %x\n",vid);
		return;
	}
	n_printf("DISTRIBUTOR: ARM,vendor id: %x, version number: %x.%x\n",
			vid,ver_major,ver_minor);
}

void gic_simple_init(void)
{
	int icfg_num;
	int itarget_num;
	int ipri_num;
	int isen_num;
	int icpend_num;
	int i;
	unsigned int addr,val;

	/* get irq number and cpu number */
	if(read_gic_type()) {
		n_printf("read gic type error! irq number is %d\n",g_irq_num);
		return;
	}
	icfg_num = g_irq_num * 2 / 32;
	itarget_num = g_irq_num * 8 / 32;
	ipri_num = g_irq_num * 8 / 32;
	isen_num = g_irq_num / 32;
	icpend_num = g_irq_num / 32;

	d_printf("icfg_num = %d,itarget_num = %d,ipri_num = %d,isen_num = %d,icpend_num = %d\n",
			icfg_num,itarget_num,ipri_num,isen_num,icpend_num);

	/* disable distrabutor */
	writel(0x0,RDA_GICD_BASE + GIC_DIST_CTRL);
	d_printf("dist ctrl = %x\n",readl(RDA_GICD_BASE + GIC_DIST_CTRL));

	/* disable all PPI interrups,enable all SGI,SPI interrups */
	writel(0xFFFF0000,RDA_GICD_BASE + GIC_DIST_ENABLE_CLEAR);
	soft_dly(RW_DELAY);
	writel(0x0000FFFF,RDA_GICD_BASE + GIC_DIST_ENABLE_SET);
	soft_dly(RW_DELAY);
	val = readl(RDA_GICD_BASE + GIC_DIST_ENABLE_SET);
	d_printf("ien %x = %x\n",RDA_GICD_BASE + GIC_DIST_ENABLE_SET,val);
	for(i = 1;i < isen_num;i++) {
		addr = RDA_GICD_BASE + GIC_DIST_ENABLE_SET + i * 4;
		writel(0xFFFFFFFF,addr);
		soft_dly(RW_DELAY);
		val = readl(addr);
		d_printf("ien %x = %x\n",addr,val);
	}
	/* set SPIs irq trigger mode, 0: high level sensitive,1:edge triggered */
	for(i = 2;i < icfg_num;i++){
		addr = RDA_GICD_BASE + GIC_DIST_CONFIG + i * 4;
		writel(0,addr);
		soft_dly(RW_DELAY);
		val = readl(addr);
		d_printf("icfg %x = %x\n",addr,val);
	}
	/* set SGIs irq target cpu0 */
	for(i = 0;i < 4;i++) {
		addr = RDA_GICD_BASE + GIC_DIST_TARGET + i * 4;
		writel(0x0,addr);
		soft_dly(RW_DELAY);
		val = readl(addr);
		d_printf("itarget %x = %x\n",addr,val);
	}
	/* set PPIs irq target cpu0 */
	for(i = 4;i < 8;i++) {
		addr = RDA_GICD_BASE + GIC_DIST_TARGET + i * 4;
		writel(0x0,addr);
		soft_dly(RW_DELAY);
		val = readl(addr);
		d_printf("itarget %x = %x\n",addr,val);
	}
	/* set SPIs irq target cpu0 */
	for(i = 8;i < itarget_num;i++) {
		addr = RDA_GICD_BASE + GIC_DIST_TARGET + i * 4;
		writel(0x0,addr);
		soft_dly(RW_DELAY);
		val = readl(addr);
		d_printf("itarget %x = %x\n",addr,val);
	}
	/* set irq priority,0 is the highest priority */
	for(i = 0;i < ipri_num;i++){
		addr = RDA_GICD_BASE + GIC_DIST_PRI + i * 4;
		writel(0x0,addr);
		soft_dly(RW_DELAY);
		val = readl(addr);
		d_printf("ipri %x = %x\n",addr,val);
	}
	/* clear all pending IRQs */
	for(i = 0;i < icpend_num;i++) {
		addr = RDA_GICD_BASE + GIC_DIST_PENDING_CLEAR + i * 4;
		writel(0xFFFFFFFF,addr);
		soft_dly(RW_DELAY);
	}
	/* clear all active IRQs */
	for(i = 0;i < icpend_num;i++) {
		addr = RDA_GICD_BASE + GIC_DIST_ACTIVE_CLEAR + i * 4;
		writel(0xFFFFFFFF,addr);
		soft_dly(RW_DELAY);
	}
	/* enable cpu interface */
	writel(0x3,RDA_GICC_BASE + GIC_CPU_CTRL);
	soft_dly(RW_DELAY);
	val = readl(RDA_GICC_BASE + GIC_CPU_CTRL);
	d_printf("cpu ctrl= %x\n",val);
	/* set cpu interface priority mask,all irq can be sent to CPU */
	writel(0xFF,RDA_GICC_BASE + GIC_CPU_PRIMASK);
	soft_dly(RW_DELAY);
	val = readl(RDA_GICC_BASE + GIC_CPU_PRIMASK);
	d_printf("cpu mask = %x\n",val);
	/* enable distrabutor */
	writel(0x3,RDA_GICD_BASE + GIC_DIST_CTRL);
	soft_dly(RW_DELAY);
	val = readl(RDA_GICD_BASE + GIC_DIST_CTRL);
	d_printf("dist ctrl = %x\n",val);
	d_printf("\n\n");
}

void test_enable_irq(void)
{
	unsigned int mask,val,addr;
	int i,j = 0;

	n_printf("\n\n");
	/* disable dist */
	writel(0x0,RDA_GICD_BASE + GIC_DIST_CTRL);
	/* disable all irq */
	for(addr = 0x180;addr < 0x200; addr +=4) {
		writel(0xFFFFFFFF,RDA_GICD_BASE + addr);
	}
	/* enable irq */
	for(addr = 0x100;addr < 0x180; addr +=4) {
		n_printf("reg %x enable test starts.\n",addr);
		for(i = 0;i < 32;i++) {
			val = 1 << i;
			mask = val;
			writel(val,RDA_GICD_BASE + addr);
			soft_dly(RW_DELAY);
			val = readl(RDA_GICD_BASE + addr);
			if(val & mask) {
				n_printf("bit %d enable operation success !!!\n",i);
				j++;
			}
		}
		n_printf("reg %x enable test finished.\n\n",addr);
	}
	/* disable all irq */
	for(addr = 0x180;addr < 0x200; addr +=4) {
		writel(val,RDA_GICD_BASE + addr);
	}
	n_printf(" %d IRQs enable operation test success !!!\n",j);
}

void test_disable_irq(void)
{
	unsigned int mask,val,addr;
	int i,j = 0;

	n_printf("\n\n");
	writel(0x0,RDA_GICD_BASE + GIC_DIST_CTRL);

	/* 0~15 SGIs is allways enabled */
	addr = 0x180;
	n_printf("reg %x disable test starts\n",addr);
	for(i = 0;i < 16;i++) {
		val = 1 << i;
		mask = val;
		writel(val,RDA_GICD_BASE + addr);
		soft_dly(RW_DELAY);
		val = readl(RDA_GICD_BASE + addr);
		if((val & mask) != 0) {
			n_printf("bit %d disable operation success !!!\n",i);
		} else {
			n_printf("bit %d disable operation failed ###\n",i);
			j++;
		}
	}
	/* 16~24 is unused, 25~31 PPIs is normally used */
	addr = 0x180;
	n_printf("reg %x disable test starts\n",addr);
	for(i = 25;i < 32;i++) {
		val = 1 << i;
		mask = val;
		writel(val,RDA_GICD_BASE + addr);
		soft_dly(RW_DELAY);
		val = readl(RDA_GICD_BASE + addr);
		if((val & mask) == 0) {
			n_printf("bit %d disable operation success !!!\n",i);
		} else {
			n_printf("bit %d disable operation failed ###\n",i);
			j++;
		}
	}
	/* 32~95 SGIs is normally used */
	for(addr = 0x184;addr < 0x200; addr +=4) {
		n_printf("reg %x disable test starts\n",addr);
		for(i = 0;i < 32;i++) {
			val = 1 << i;
			mask = val;
			writel(val,RDA_GICD_BASE + addr);
			soft_dly(RW_DELAY);
			val = readl(RDA_GICD_BASE + addr);
			if((val & mask) == 0) {
				if(addr < 0x18C)
					n_printf("bit %d disable operation success !!!\n",i);
			} else {
				n_printf("bit %d disable operation failed ###\n",i);
				j++;
			}
		}
		n_printf("reg %x disable test finished\n\n",addr);
	}
	n_printf(" %d IRQs disable operation test failed.\n",j);
}

/*******************************************************************************
 *	@name:		gic_simple_test
 *	@param:		void
 *	@return:	void
 *	@descipt:
 *		This simple test would pend some interrupts and GIC forward
 *		these signals to CPU, once CPU recognize it,a IRQ is occured
 *		and it jump into do_irq() to process this event;
 *		This simple test will pend all SPIs interrupts and PPIs
 *		interrupts.
 ******************************************************************************
 */
void gic_simple_test(void)
{
	unsigned int reg_idx,reg_pos,reg_active,reg_pend;
	int i,j,err,irq_id;

	/* Start to test SPIs IRQs */
	mdelay(10);
	n_printf("\nstart %d SPIs interrupts test ...\n\n",(g_irq_num-32));
	j = 0;
	err = 0;
	/* enable all SPIs */
	writel(0xFFFFFFFF,RDA_GICD_BASE + GIC_DIST_ENABLE_SET + 4);
	writel(0xFFFFFFFF,RDA_GICD_BASE + GIC_DIST_ENABLE_SET + 8);
	/* pending SPIs */
	for(i = 32;i < g_irq_num;i++) {
		j = 1 << (i % 32);
		reg_idx = (i / 32 + 1) * 4;
		reg_pos = j;
		irq_id = i + 32;
		if(irq_id == g_irq_num)
			break;
		reg_active = readl(RDA_GICD_BASE + GIC_DIST_ACTIVE_SET + reg_idx);
		reg_pend   = readl(RDA_GICD_BASE + GIC_DIST_PENDING_SET + reg_idx);
		n_printf("pending before, pend state: %x, active state: %x\n",
				(reg_active & reg_pos),(reg_pend & reg_pos));

		n_printf("pending irq and waiting for it occur ... id is %d\n",irq_id);
		writel(j,RDA_GICD_BASE + GIC_DIST_PENDING_SET + reg_idx);
		mdelay(200);

		reg_active = readl(RDA_GICD_BASE + GIC_DIST_ACTIVE_SET + reg_idx);
		reg_pend   = readl(RDA_GICD_BASE + GIC_DIST_PENDING_SET + reg_idx);
		n_printf("pending after, pend state: %x, active state: %x\n",
				(reg_active & reg_pos),(reg_pend & reg_pos));
		if(reg_pend & reg_pos) {
			writel(j,RDA_GICD_BASE + GIC_DIST_PENDING_CLEAR + reg_idx);
			n_printf("error! CPU do not response the IRQ which id is %d\n",irq_id);
			err++;
		}
		n_printf("\n");
	}
	if(err)
		n_printf("all SPI irq test finished, have %d irq errors\n",err);
	else
		n_printf("all SPI irq test success !\n");

	/* Start to test PPIs IRQs */
	mdelay(10);
	n_printf("\nstart %d PPIs interrupts test ...\n\n",15);
	j = 0;
	err = 0;
	/* enable PPIs */
	writel(0xFFFF0000,RDA_GICD_BASE + GIC_DIST_ENABLE_SET);
	/* pending PPIs */
	for(i = 16;i < 32;i++) {
		j = 1 << (i % 32);
		reg_idx = (i / 32 + 0) * 4;
		reg_pos = j;
		irq_id = i;
		/* the PPI 16 ~ 24 is not available */
		if((irq_id >= 16) && ( irq_id <= 24)) {
			continue;
		}
		reg_active = readl(RDA_GICD_BASE + GIC_DIST_ACTIVE_SET + reg_idx);
		reg_pend   = readl(RDA_GICD_BASE + GIC_DIST_PENDING_SET + reg_idx);
		n_printf("pending before, pend state: %x, active state: %x\n",
				(reg_active & reg_pos),(reg_pend & reg_pos));

		n_printf("pending irq and waiting for it occur ... id is %d\n",irq_id);
		writel(j,RDA_GICD_BASE + GIC_DIST_PENDING_SET + reg_idx);
		mdelay(200);

		reg_active = readl(RDA_GICD_BASE + GIC_DIST_ACTIVE_SET + reg_idx);
		reg_pend   = readl(RDA_GICD_BASE + GIC_DIST_PENDING_SET + reg_idx);
		n_printf("pending after, pend state: %x, active state: %x\n",
				(reg_active & reg_pos),(reg_pend & reg_pos));
		if(reg_pend & reg_pos) {
			writel(j,RDA_GICD_BASE + GIC_DIST_PENDING_CLEAR + reg_idx);
			n_printf("error! CPU do not response the IRQ which id is %d\n",irq_id);
			err++;
		}
		d_printf("\n");
	}
	/* disable PPIs */
	writel(0xFFFF0000,RDA_GICD_BASE + GIC_DIST_ENABLE_CLEAR);
	if(err)
		n_printf("all PPI irq test finished, have %d irq errors\n",err);
	else
		n_printf("all PPI irq test success !\n");
}

void gic_test_entry(void)
{
	n_printf("entry gic test\n");
	show_dist_info();
	show_cpu_if_info();
	gic_simple_init();
	gic_simple_test();
	test_enable_irq();
	test_disable_irq();
	mdelay(1000);
	n_printf("delay 1 second ...\n");
	mdelay(1000);
}

int gic_test(int times)
{
	int n = 0;
	while(n < times) {
		n++;
		n_printf("\ngic_test: %d / %d\n",n,times);
		gic_test_entry();
		n_printf("\ngic_test end\n");
	}
	return 0;
}
