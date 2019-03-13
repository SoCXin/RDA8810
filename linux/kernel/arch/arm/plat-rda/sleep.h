#ifndef _RDA_PLAT_SLEEP_H_
#define _RDA_PLAT_SLEEP_H_

#define CONFIG_WAKEUP_JUMP_ADDR         0x0010c100
#define CONFIG_WAKEUP_JUMP_MAGIC        0xD8E1A000

#define CONFIG_WAKEUP_CODE_ADDR         0x0010c120

#ifndef __ASSEMBLY__ 
void copy_wakeup_code(unsigned long code_address);
void rda_cpu_resume(void);
#endif

#endif /* _RDA_PLAT_SLEEP_H_ */
