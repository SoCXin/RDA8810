#ifndef _SPL_BOARD_INFO_H_
#define _SPL_BOARD_INFO_H_

#include <asm/arch/rom_api_trampolin.h>

/*!!!
 *struct size must < CONFIG_SPL_BOARD_INFO_SIZE (512 bytes)
 */
typedef struct spl_bd_info {
	struct {
		unsigned long ddr_auto_cal_offs;
		u16 ddr_auto_cal_flag;
		u16 ddr_auto_cal_val[10];
	} spl_ddr_cal_info;

	struct spl_security_info {
		u32                          version;
		int                          secure_mode;
		struct chip_id               chip_id;
		struct chip_security_context chip_security_context;
		struct chip_unique_id        chip_unique_id;
		struct pubkey                pubkey;
		u8                           random[32];
	} spl_security_info;

	struct spl_emmc_info{
		u8 manufacturer_id;
		u8 reserved[3];
	} spl_emmc_info;
} spl_bd_t;

static inline spl_bd_t *get_spl_bd_info(void)
{
	spl_bd_t *p = (spl_bd_t *)CONFIG_SPL_BOARD_INFO_ADDR;

	return p;
}

#define get_bd_spl_security_info() &get_spl_bd_info()->spl_security_info
#define get_bd_spl_ddr_cal_info() &get_spl_bd_info()->spl_ddr_cal_info
#define get_bd_spl_emmc_info() &get_spl_bd_info()->spl_emmc_info
#endif	/* _SPL_BOARD_INFO_H_ */

