#ifndef __PDL_H_
#define __PDL_H_

extern int pdl_dbg_pdl;
extern int pdl_vdbg_pdl;
extern int pdl_dbg_usb_ep0;
extern int pdl_dbg_usb_serial;
extern int pdl_dbg_rw_check;
extern int pdl_dbg_factory_part;

int pdl_main(void);
int pdl_mode_get(void);
#endif
