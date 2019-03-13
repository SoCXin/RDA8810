#include <usb/usbserial.h>
#include "pdl_channel.h"
#include "pdl_debug.h"

static int usbser_ch0_tstc (void)
{
	return usbser_tstc(USB_ACM_CHAN_0);
}

static int usbser_ch0_getc (void)
{
	return usbser_getc(USB_ACM_CHAN_0);
}

static void usbser_ch0_putc (const char c)
{
	usbser_putc(USB_ACM_CHAN_0, c);
}

static int usbser_ch0_read(unsigned char *_buf, unsigned int len)
{
	return usbser_read(USB_ACM_CHAN_0, _buf, len);
}

static int usbser_ch0_write(const unsigned char *_buf, unsigned int len)
{
	return usbser_write(USB_ACM_CHAN_0, _buf, len);
}

static struct pdl_channel usb_channel = {
	.getchar = usbser_ch0_getc,
	.putchar = usbser_ch0_putc,
	.tstc = usbser_ch0_tstc,
	.read = usbser_ch0_read,
	.write = usbser_ch0_write,
	.priv = NULL
};

#ifdef CONFIG_USB_ACM_TWO_CHANS

static int usbser_ch1_tstc (void)
{
	return usbser_tstc(USB_ACM_CHAN_1);
}

static int usbser_ch1_getc (void)
{
	return usbser_getc(USB_ACM_CHAN_1);
}

static void usbser_ch1_putc (const char c)
{
	usbser_putc(USB_ACM_CHAN_1, c);
}

static int usbser_ch1_read(unsigned char *_buf, unsigned int len)
{
	return usbser_read(USB_ACM_CHAN_1, _buf, len);
}

static int usbser_ch1_write(const unsigned char *_buf, unsigned int len)
{
	return usbser_write(USB_ACM_CHAN_1, _buf, len);
}

static struct pdl_channel usb_channel2 = {
	.getchar = usbser_ch1_getc,
	.putchar = usbser_ch1_putc,
	.tstc = usbser_ch1_tstc,
	.read = usbser_ch1_read,
	.write = usbser_ch1_write,
	.priv = NULL,
};

#endif //CONFIG_USB_ACM_TWO_CHANS

void pdl_usb_channel_register(void)
{
#ifdef CONFIG_USB_ACM_TWO_CHANS
	pdl_debug_channel_register(&usb_channel2);
#endif

	pdl_channel_register(&usb_channel);
}

