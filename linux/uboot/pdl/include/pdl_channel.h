#ifndef _PDL_CHANNEL_H_
#define _PDL_CHANNEL_H_

struct pdl_channel {
	int (*read) (unsigned char *buf, unsigned int len);
	int (*getchar) (void);
	int (*tstc) (void);
	int (*write) (const unsigned char *buf, unsigned int len);
	void (*putchar) (const char ch);
	void   *priv;
};

/* get the current pdl data channel */
struct pdl_channel *pdl_get_channel(void);
/* register the pdl data channel */
void pdl_channel_register(struct pdl_channel *channel);

/* get the current pdl debug channel */
struct pdl_channel *pdl_get_debug_channel(void);
/* register the pdl debug channel */
void pdl_debug_channel_register(struct pdl_channel *channel);

/* register the usb serial channel 0 as the pdl data channel.
   if defined, register the usb serial channel 1 as the pdl debug channel. */
void pdl_usb_channel_register(void);

#endif

