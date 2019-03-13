#ifndef __PLAT_RDA_CPU_H__
#define __PLAT_RDA_CPU_H__

#define RDA8810_PROD_ID		8810
#define RDA8810_METAL_10_ID	10

unsigned short rda_get_soc_metal_id(void);
unsigned short rda_get_soc_prod_id(void);
int rda_soc_is_older_metal10(void);

#endif /* __PLAT_RDA_CPU_H__ */

