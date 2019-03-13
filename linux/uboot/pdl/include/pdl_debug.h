#ifndef _PDL_DEBUG_H_
#define _PDL_DEBUG_H_

#include <serial.h>

#define PDL_LOG_MAX_SIZE		(512*1024)

extern char *pdl_log_buff;
void pdl_init_serial(void);
void pdl_release_serial(void);

#define pdl_error printf
#define pdl_info printf

extern int pdl_dbg_pdl;
#define pdl_dbg(str...) \
	do {	\
	if (pdl_dbg_pdl) \
		printf(str); \
	} while(0)

extern int pdl_vdbg_pdl;
#define pdl_vdbg(str...) \
	do {	\
	if (pdl_vdbg_pdl) \
		printf(str); \
	} while(0)
#endif
