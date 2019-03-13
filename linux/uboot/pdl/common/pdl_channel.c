
#include "pdl_channel.h"
#include "pdl_debug.h"

static struct pdl_channel *serial_channel = NULL;

void pdl_channel_register(struct pdl_channel *channel)
{
	if (channel) {
		serial_channel = channel;
	} else {
		pdl_error("channel cannot register\n");
	}

	return;
}

struct pdl_channel *pdl_get_channel(void)
{
	return serial_channel;
}

static struct pdl_channel *debug_channel = NULL;

void pdl_debug_channel_register(struct pdl_channel *channel)
{
	if (channel) {
		debug_channel = channel;
	} else {
		pdl_error("debug channel cannot register\n");
	}

	return;
}

struct pdl_channel *pdl_get_debug_channel(void)
{
	return debug_channel;
}
