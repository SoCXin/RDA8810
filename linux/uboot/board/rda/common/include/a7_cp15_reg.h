#ifndef __A7_CP15_REG_H__
#define __A7_CP15_REG_H__

unsigned int read_cp15_midr(void);
unsigned int read_cp15_mpidr(void);
unsigned int read_cp15_revidr(void);
unsigned int read_cp15_ccsidr(void);
unsigned int read_cp15_clidr(void);
unsigned int read_cp15_l2ctlr(void);

unsigned int read_cp15_csselr(void);
void write_cp15_csselr(unsigned int val);

unsigned int read_cp15_sctlr(void);
void write_cp15_sctlr(unsigned int val);

unsigned int read_cp15_scr(void);
void write_cp15_scr(unsigned int val);

unsigned int read_cp15_ttbr0(void);
void write_cp15_ttbr0(unsigned int val);

unsigned int read_cp15_ttbr1(void);
void write_cp15_ttbr1(unsigned int val);

unsigned int read_cp15_ttbcr(void);
void write_cp15_ttbcr(unsigned int val);

unsigned int read_cp15_htcr(void);
void write_cp15_htcr(unsigned int val);
void write_cp15_dacr(unsigned int val);

void __copy_bytes(unsigned char *des,unsigned char *src,unsigned int size);
void __nop_dly(void);

unsigned int read_cpu_cpsr(void);
void write_cpu_cpsr(unsigned int val);

void cp15_disable_all_cache(void);
void cp15_enable_dcache(void);
void cp15_enable_icache(void);
void cp15_enable_mmu(void);
void cp15_disable_dcache(void);
void cp15_disable_icache(void);
void cp15_disable_mmu(void);
#endif
