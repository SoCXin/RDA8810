#ifndef __ASM_ARCH_MTDPARTS_DEF_H
#define __ASM_ARCH_MTDPARTS_DEF_H

#include <rda/tgt_ap_flash_parts.h>


#define SEPARATOR "#"

#define SYSTEM_PKT_LEN	"4096"

#define DEFAULT_FSIMG_PKT_LEN   "4096"

#define USERDATA_IMAGE_ATTR	\
	"userdata("	\
	DEFAULT_FSIMG_PKT_LEN	\
	")"

#define CUSTOMER_IMAGE_ATTR	\
	"customer("	\
	DEFAULT_FSIMG_PKT_LEN	\
	")"

#define VENDOR_IMAGE_ATTR	\
	"vendor("	\
	DEFAULT_FSIMG_PKT_LEN	\
	")"

#define SYSTEM_IMAGE_ATTR	\
	"system("	\
	SYSTEM_PKT_LEN	\
	")"

#define CACHE_IMAGE_ATTR	\
	"cache("	\
	DEFAULT_FSIMG_PKT_LEN	\
	")"

#define IMAGE_ATTR	\
	"pdl(8)"	\
	SEPARATOR	\
	"bootloader(4096)"	\
	SEPARATOR	\
	"modem(4096)"	\
	SEPARATOR	\
	"boot(4096)"	\
	SEPARATOR	\
	SYSTEM_IMAGE_ATTR	\
	SEPARATOR	\
	CUSTOMER_IMAGE_ATTR	\
	SEPARATOR	\
	VENDOR_IMAGE_ATTR	\
	SEPARATOR	\
	USERDATA_IMAGE_ATTR	\
	SEPARATOR	\
	CACHE_IMAGE_ATTR	\
	SEPARATOR	\
	"recovery(4096)"
#endif /* __ASM_ARCH_MTDPARTS_DEF_H */
