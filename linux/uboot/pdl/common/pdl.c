#include <common.h>
#include <asm/arch/hwcfg.h>
#include <asm/arch/rda_sys.h>
#include <pdl.h>

int pdl_dbg_pdl = 0;
int pdl_vdbg_pdl = 0;
int pdl_dbg_usb_ep0 = 0;
int pdl_dbg_usb_serial = 0;
int pdl_dbg_rw_check = 0;
int pdl_dbg_factory_part = 0;

/*
  check if should start pdl utilities
*/
int pdl_mode_get(void)
{
	u16 swcfg = rda_swcfg_get();

	/*
	if hwconfig is in force download and swcofing is not
	in calibration mode
	*/
	if (rda_bm_is_download() && !(swcfg & RDA_SW_CFG_BIT_4))
		return 1;
	else
		return 0;
}
