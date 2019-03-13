#include <common.h>
#include <asm/arch/ispi.h>

//#define VPU_TEST_DEBUG
#ifdef VPU_TEST_DEBUG
#define debug_printf	printf
#else
#define debug_printf(...)	do{}while(0)
#endif

//#define DO_WRITE_RESULT_TO_SRAM
#ifdef DO_WRITE_RESULT_TO_SRAM
#include <asm/io.h>
/* VPU & MD5 test result storage address */
#define VPU_TEST_DATA_BASE	0X11C010E4
#define VPU_TEST_RESULT_ADDR (VPU_TEST_DATA_BASE+0X00)
#define VPU_TEST_COUNT_ADDR	(VPU_TEST_DATA_BASE+0X04)
#define MD5_TEST_RESULT_ADDR	(VPU_TEST_DATA_BASE+0X08)
#define MD5_TEST_COUNT_ADDR	(VPU_TEST_DATA_BASE+0X0C)
#define VPU_MD5_TEST_RESULT_OK	0XA55A6666
#define VPU_MD5_TEST_RESULT_ERROR	0XDEADDEAD
#endif

#ifdef CONFIG_MACH_RDA8850E
#define PMU_VCORE_VOLTAGE_REG	(u32)0x53
#define PMU_DDR_VOLTAGE_VAL_REG	(u32)0x2A
#elif defined(CONFIG_MACH_RDA8810E) || defined(CONFIG_MACH_RDA8810H) || defined(CONFIG_MACH_RDA8810)
#define PMU_VCORE_VOLTAGE_REG	(u32)0x2F
#define PMU_DDR_VOLTAGE_VAL_REG	(u32)0x2A
#else
#error "define pmu vcore,ddr register address error for vpu test!"
#endif

#if defined(CONFIG_MACH_RDA8810E) || defined(CONFIG_MACH_RDA8810H) || defined(CONFIG_MACH_RDA8850E)
#include "vpu_test_coda7l.c"
#elif defined(CONFIG_MACH_RDA8810)
#include "vpu_test_coda960.c"
#else
static int _vpu_test(int md5_check)
{
	return 0;
}
#endif
static void sys_get_pmu_values(unsigned int *vcore_value, unsigned int *ddr_value)
{
	ispi_open(1);
	*vcore_value = ispi_reg_read(PMU_VCORE_VOLTAGE_REG);
	*ddr_value = ispi_reg_read(PMU_DDR_VOLTAGE_VAL_REG);
	ispi_open(0);
}
static void sys_set_pmu_values(unsigned int vcore_value, unsigned int ddr_value)
{
	ispi_open(1);
	ispi_reg_write(PMU_VCORE_VOLTAGE_REG,vcore_value);
	ispi_reg_write(PMU_DDR_VOLTAGE_VAL_REG,ddr_value);
	ispi_open(0);
}
static void sys_setup_vpu_clk(unsigned int div)
{
	*((unsigned long *)0x209000E8) = div;
}
void vpu_sta_test(int times)
{
	unsigned int vcore_value, ddr_value,t_vcore=0,t_ddr=0;
	int i,ret;
	int err_buf[3] = {0,0,0};

	printf("\n%s: current ticks = %llu\n",__func__,get_ticks());
	sys_get_pmu_values(&vcore_value, &ddr_value);
	debug_printf("\n%s: before test: vcore_value = %#x , ddr_value = %#x\n", __func__,vcore_value,ddr_value);
#ifdef CONFIG_MACH_RDA8850E
	sys_set_pmu_values(0x9525, 0xba45);//vcore=9,ddr=11
#else
	//sys_set_pmu_values(0xb444, 0xbab5); //vcore=11, ddr=11
	//sys_set_pmu_values(0xb444, 0xaab5); //vcore=11, ddr=10
	//sys_set_pmu_values(0xa444, 0x9ab5); //vcore=10, ddr=9
	//sys_set_pmu_values(0x9444, 0x9ab5); //vcore=9, ddr=9
	//sys_set_pmu_values(0xa444, 0xaab5); //vcore=10, ddr=10
	sys_set_pmu_values(0xa444, 0xbab5); //vcore=10, ddr=11
#endif
	sys_get_pmu_values(&t_vcore, &t_ddr);
	debug_printf("\n%s: when test: vcore_value = %#x, ddr_value = %#x\n", __func__, t_vcore, t_ddr);
	printf("\n%s: %d times, test start\n",__func__,times);
	for(i = 1;i <= times;i++) {
		printf("\n%s: times %d / %d\n", __func__, i, times);
		if(i % 2 == 1) {
			sys_setup_vpu_clk(0x16);
		} else {
			sys_setup_vpu_clk(0x1c);
		}
		ret = _vpu_test(1);
		/*
		 * store test result to error buffer:
		 * err_buf[0]: md5 check error
		 * err_buf[1]: vpu decode error
		 * err_buf[2]: success
		 */
		ret += 2;
		if((ret >= 0) && (ret <= 2)) {
			err_buf[ret] += 1;
		}
		printf("\n");
	}
	sys_set_pmu_values(vcore_value, ddr_value);
	printf("\n%s: current ticks = %llu\n", __func__, get_ticks());
	printf("\n%s: end seq\n",__func__);
	printf("\n md5 error: %d, vpu error %d, success: %d, sum: %d\n\n",
			err_buf[0],err_buf[1],err_buf[2],times);

#ifdef DO_WRITE_RESULT_TO_SRAM
	writel(i-1,VPU_TEST_COUNT_ADDR);
	if(err_buf[1] != 0)
		writel(VPU_MD5_TEST_RESULT_ERROR, VPU_TEST_RESULT_ADDR);
	else
		writel(VPU_MD5_TEST_RESULT_OK, VPU_TEST_RESULT_ADDR);

	writel(i-1,MD5_TEST_COUNT_ADDR);
	if(err_buf[0] != 0)
		writel(VPU_MD5_TEST_RESULT_ERROR,MD5_TEST_RESULT_ADDR);
	else
		writel(VPU_MD5_TEST_RESULT_OK,MD5_TEST_RESULT_ADDR);
#endif /* DO_WRITE_RESULT_TO_SRAM */
	sys_get_pmu_values(&t_vcore, &t_ddr);
	debug_printf("\n%s: after test: vcore_value = %#x, ddr_value = %#x\n", __func__, t_vcore, t_ddr);
	printf("\n%s: finish successfully\n",__func__);
}

int vpu_md5_test(int times)
{
	unsigned int vcore_value, ddr_value;
	int i, ret = 0;
	debug_printf("\n%s: current ticks = %llu\n", __func__, get_ticks());
#if 0
	sys_get_pmu_values(&vcore_value, &ddr_value);
	sys_set_pmu_values(0xa444, 0xbab5); //vcore=10, ddr=11
#endif
	for(i=1; i<=times; i++) {
		printf("\n%s: times %d / %d\n", __func__, i, times);
		ret = _vpu_test(1);
		printf("\n");
		if(ret)
			break;
	}
#if 0
	sys_set_pmu_values(vcore_value, ddr_value);
#endif
	debug_printf("\n%s: current ticks = %llu\n", __func__, get_ticks());
	return ret;
}
