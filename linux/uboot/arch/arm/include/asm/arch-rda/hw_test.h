#ifndef __RDA_HW_TEST_H__
#define __RDA_HW_TEST_H__

#ifdef CONFIG_VPU_TEST
extern void vpu_sta_test(int times);
extern int  vpu_md5_test(int times);
#else
static inline void vpu_sta_test(int times){};
static inline int  vpu_md5_test(int times){return 0;};
#endif /* CONFIG_VPU_TEST */

#ifdef CONFIG_CPU_TEST
extern void cpu_pll_test(int times);
#else
static inline void cpu_pll_test(int times){};
#endif /*CONFIG_CPU_TEST */

#ifdef CONFIG_DDR_TEST
extern int ddr_mem_copy_test(int times,unsigned int src_addr, unsigned int des_addr, unsigned int nword);
#else
static inline int ddr_mem_copy_test(int times,unsigned int src_addr, unsigned int des_addr, unsigned int nword){return 0;}
#endif /* CONFIG_DDR_TEST */

#ifdef CONFIG_TIMER_TEST
extern int tim_test(int timer_id,int times);
#else
static inline int tim_test(int timer_id,int times){ return 0;}
#endif /* CONFIG_TIMER_TEST */

#ifdef CONFIG_UART_TEST
extern int uart_test(int uart_id,int times);
#else
static inline int uart_test(int uart_id,int times){ return 0;}
#endif /* CONFIG_UART_TEST */

#ifdef CONFIG_GIC_TEST
extern int gic_test(int times);
#else
static inline int gic_test(int times){ return 0;}
#endif /* CONFIG_GIC_TEST */

#ifdef CONFIG_CACHE_TEST
extern int cpu_cache_test(int times);
#else
static inline int cpu_cache_test(int times){ return 0;}
#endif /* CONFIG_CACHE_TEST */

#ifdef CONFIG_MIPI_LOOP_TEST
extern int test_dsi_csi_loop(int times);
#else
static inline int test_dsi_csi_loop(int times) {return 0;};
#endif /* CONFIG_MIPI_LOOP_TEST */

#ifdef CONFIG_I2C_TEST
extern int test_i2c(int id,int times);
#else
static inline int test_i2c(int id,int times){ return 0;}
#endif /* CONFIG_I2C_TEST */


#endif /*__RDA_HW_TEST_H__*/
