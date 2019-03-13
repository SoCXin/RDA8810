#ifndef ISPI_H_
#define ISPI_H

#ifdef CONFIG_RDA_ISPI
void ispi_open(void);
void ispi_reg_write(u32 regIdx,u32 value);
u32 ispi_reg_read(u32 regIdx);
#else
static inline void ispi_open(void)
{
}

static inline void ispi_reg_write(u32 regIdx,u32 value)
{
}

static inline u32 ispi_reg_read(u32 regIdx)
{
	return 0;
}
#endif

#endif
