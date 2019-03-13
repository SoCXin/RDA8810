#include <common.h>
#include <malloc.h>
#include <stdio_dev.h>
#include "pdl.h"
#include "pdl_debug.h"
#include "pdl_channel.h"

char *pdl_log_buff = NULL;
static void pdl_serial_puts(const char *s)
{
	struct pdl_channel *dbg_ch = pdl_get_debug_channel();

	/* save print string to log buffer */
	if(pdl_log_buff) {
		int a = strlen(pdl_log_buff);
		int b = strlen(s);

		if(a + b < PDL_LOG_MAX_SIZE)
			strcpy(&pdl_log_buff[a], s);
	}

	serial_puts(s);

	if(pdl_dbg_usb_serial && dbg_ch)
		dbg_ch->write((const unsigned char *)s, strlen(s)+1);
}

struct stdio_dev *search_device(int flags, const char *name);
void pdl_init_serial(void)
{
	struct stdio_dev *dev = search_device (DEV_FLAGS_OUTPUT, "serial");
	if(!dev)
		return;
	dev->puts = pdl_serial_puts;

	if(!pdl_log_buff) {
		pdl_log_buff = malloc(PDL_LOG_MAX_SIZE);
		pdl_log_buff[0] = '\0';
	}
}
void pdl_release_serial(void)
{
	struct stdio_dev *dev = search_device (DEV_FLAGS_OUTPUT, "serial");
	if(!dev)
		return;
	dev->puts = serial_puts;
}

