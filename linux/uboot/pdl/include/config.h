#ifndef __PDL_CONFIG_H__
#define __PDL_CONFIG_H__

#ifdef CONFIG_SPL_BUILD
# define CONFIG_RDA_PDL1
#else
# define CONFIG_RDA_PDL2
#endif

#endif // __PDL_CONFIG_H__

