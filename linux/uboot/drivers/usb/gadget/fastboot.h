#ifndef __FASTBOOT_H__
#define __FASTBOOT_H__

extern struct usb_endpoint_instance * fastboot_get_out_ep(void);
extern struct usb_endpoint_instance * fastboot_get_in_ep(void);
extern int fastboot_configured (void);

#endif
