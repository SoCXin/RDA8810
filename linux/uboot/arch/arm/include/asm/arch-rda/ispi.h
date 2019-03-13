#ifndef __ISPI_H__
#define __ISPI_H_

#define pmu_reg_write ispi_reg_write
#define pmu_reg_read ispi_reg_read

void ispi_open(int modemSpi);
void ispi_reg_write(u32 regIdx, u32 value);
u32 ispi_reg_read(u32 regIdx);
u16 rda_read_efuse(int page_index);
#endif

