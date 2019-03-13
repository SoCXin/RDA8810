/*
 * (C) Copyright 2003
 * Gerry Hamel, geh@ti.com, Texas Instruments
 *
 * (C) Copyright 2006
 * Bryan O'Donoghue, bodonoghue@codehermit.ie
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307	 USA
 *
 */

#include <common.h>
#include <config.h>
#include <asm/unaligned.h>
#include <usb/usbserial.h>
#include "usbser.h"
#include "usb_cdc_acm.h"
#include "usbdescriptors.h"

//#define DEBUG

#ifdef DEBUG
#define DBG(fmt,args...)\
	serial_printf("[%s] %s %d: "fmt, __FILE__,__FUNCTION__,__LINE__,##args)
#else
#define DBG(fmt,args...) do{}while(0)
#endif

#if 1
#define ERR(fmt,args...)\
	serial_printf("ERROR![%s] %s %d: "fmt, __FILE__,__FUNCTION__,\
	__LINE__,##args)
#else
#define ERR(fmt,args...) do{}while(0)
#endif

/*
 * Defines
 */
#define NUM_CONFIGS    1
#ifdef CONFIG_USB_ACM_TWO_CHANS
#define MAX_INTERFACES 4
#define NUM_ENDPOINTS  6
#define NUM_ACM_INTERFACES 4
#else
#define MAX_INTERFACES 2
#define NUM_ENDPOINTS  3
#define NUM_ACM_INTERFACES 2
#endif

/*
 * Buffers to hold input and output data
 */


/*
 * Instance variables
 */
static struct usb_device_instance device_instance[1];
static struct usb_bus_instance bus_instance[1];
static struct usb_configuration_instance config_instance[NUM_CONFIGS];
static struct usb_interface_instance interface_instance[MAX_INTERFACES];
static struct usb_alternate_instance alternate_instance[MAX_INTERFACES];
/* one extra for control endpoint */
static struct usb_endpoint_instance endpoint_instance[NUM_ENDPOINTS+1];

/*
 * Global flag
 */
int usbser_configured_flag = 0;


/* Indicies, References */
static unsigned short rx_endpoint[2] = {0, 0};
static unsigned short tx_endpoint[2] = {0, 0};

/* Standard USB Data Structures */
static struct usb_interface_descriptor interface_descriptors[MAX_INTERFACES];
static struct usb_endpoint_descriptor *ep_descriptor_ptrs[NUM_ENDPOINTS];
static struct usb_configuration_descriptor	*configuration_descriptor = 0;
static struct usb_device_descriptor device_descriptor = {
	.bLength = sizeof(struct usb_device_descriptor),
	.bDescriptorType =	USB_DT_DEVICE,
	.bcdUSB =		cpu_to_le16(USB_BCD_VERSION),
#ifdef CONFIG_USB_ACM_TWO_CHANS /* enable interface association */
	.bDeviceClass =		0xEF,
	.bDeviceSubClass =	0x02,
	.bDeviceProtocol =	0x01,
#else
	.bDeviceClass = 	COMMUNICATIONS_DEVICE_CLASS,
	.bDeviceSubClass =	0x00,
	.bDeviceProtocol =	0x00,
#endif
	.bMaxPacketSize0 =	EP0_MAX_PACKET_SIZE,
	.idVendor =		cpu_to_le16(CONFIG_USBD_VENDORID),
#ifdef CONFIG_USB_ACM_TWO_CHANS
	.idProduct =		cpu_to_le16(CONFIG_USBD_PRODUCTID_RDAACM),
#else
	.idProduct =		cpu_to_le16(CONFIG_USBD_PRODUCTID_CDCACM),
#endif
	.bcdDevice =		cpu_to_le16(USBTTY_BCD_DEVICE),
	.iManufacturer =	0,
	.iProduct =		0,
	.iSerialNumber =	0,
	.bNumConfigurations =	NUM_CONFIGS
};


#if defined(CONFIG_USBD_HS)
static struct usb_qualifier_descriptor qualifier_descriptor = {
	.bLength = sizeof(struct usb_qualifier_descriptor),
	.bDescriptorType =	USB_DT_QUAL,
	.bcdUSB =		cpu_to_le16(USB_BCD_VERSION),
	.bDeviceClass =		COMMUNICATIONS_DEVICE_CLASS,
	.bDeviceSubClass =	0x00,
	.bDeviceProtocol =	0x00,
	.bMaxPacketSize0 =	EP0_MAX_PACKET_SIZE,
	.bNumConfigurations =	NUM_CONFIGS
};
#endif

/*
 * Static CDC ACM specific descriptors
 */

struct acm_config_desc {
	struct usb_configuration_descriptor configuration_desc;

#ifdef CONFIG_USB_ACM_TWO_CHANS
	/* function 1 */
	struct usb_function_descriptor	function_desc;
#endif

	/* Master Interface */
	struct usb_interface_descriptor interface_desc;

	struct usb_class_header_function_descriptor usb_class_header;
	struct usb_class_call_management_descriptor usb_class_call_mgt;
	struct usb_class_abstract_control_descriptor usb_class_acm;
	struct usb_class_union_function_descriptor usb_class_union;
	struct usb_endpoint_descriptor notification_endpoint;

	/* Slave Interface */
	struct usb_interface_descriptor data_class_interface;
	struct usb_endpoint_descriptor data_endpoints[2];

#ifdef CONFIG_USB_ACM_TWO_CHANS
	/* function 2 */
	struct usb_function_descriptor	function_desc2;

	/* Master Interface 2 */
	struct usb_interface_descriptor interface_desc2;

	struct usb_class_header_function_descriptor usb_class_header2;
	struct usb_class_call_management_descriptor usb_class_call_mgt2;
	struct usb_class_abstract_control_descriptor usb_class_acm2;
	struct usb_class_union_function_descriptor usb_class_union2;
	struct usb_endpoint_descriptor notification_endpoint2;

	/* Slave Interface 2 */
	struct usb_interface_descriptor data_class_interface2;
	struct usb_endpoint_descriptor data_endpoints2[2];
#endif
} __attribute__((packed));

static struct acm_config_desc acm_configuration_descriptors[NUM_CONFIGS] = {
	{
		.configuration_desc ={
			.bLength =
				sizeof(struct usb_configuration_descriptor),
			.bDescriptorType = USB_DT_CONFIG,
			.wTotalLength =
				cpu_to_le16(sizeof(struct acm_config_desc)),
			.bNumInterfaces = NUM_ACM_INTERFACES,
			.bConfigurationValue = 1,
			.iConfiguration = 0,
			.bmAttributes =
				BMATTRIBUTE_SELF_POWERED|BMATTRIBUTE_RESERVED,
			.bMaxPower = USBTTY_MAXPOWER
		},
#ifdef CONFIG_USB_ACM_TWO_CHANS
		/* function 1 */
		.function_desc = {
			.bLength = sizeof(struct usb_function_descriptor),
			.bDescriptorType = 0x0B,
			.bFirstInterface = 0,
			.bInterfaceCount = 2,
			.bFunctionClass = COMMUNICATIONS_INTERFACE_CLASS_CONTROL,
			.bFunctionSubClass = COMMUNICATIONS_ACM_SUBCLASS,
			.bFunctionProtocol = COMMUNICATIONS_V25TER_PROTOCOL,
			.iFunction = 0,
		},
#endif
		/* Interface 1 */
		.interface_desc = {
			.bLength  = sizeof(struct usb_interface_descriptor),
			.bDescriptorType = USB_DT_INTERFACE,
			.bInterfaceNumber = 0,
			.bAlternateSetting = 0,
			.bNumEndpoints = 0x01,
			.bInterfaceClass =
				COMMUNICATIONS_INTERFACE_CLASS_CONTROL,
			.bInterfaceSubClass = COMMUNICATIONS_ACM_SUBCLASS,
			.bInterfaceProtocol = COMMUNICATIONS_V25TER_PROTOCOL,
			.iInterface = 0,
		},
		.usb_class_header = {
			.bFunctionLength	=
				sizeof(struct usb_class_header_function_descriptor),
			.bDescriptorType	= CS_INTERFACE,
			.bDescriptorSubtype	= USB_ST_HEADER,
			.bcdCDC	= cpu_to_le16(110),
		},
		.usb_class_call_mgt = {
			.bFunctionLength	=
				sizeof(struct usb_class_call_management_descriptor),
			.bDescriptorType	= CS_INTERFACE,
			.bDescriptorSubtype	= USB_ST_CMF,
			.bmCapabilities		= 0x00,
			.bDataInterface		= 0x01,
		},
		.usb_class_acm = {
			.bFunctionLength	=
				sizeof(struct usb_class_abstract_control_descriptor),
			.bDescriptorType	= CS_INTERFACE,
			.bDescriptorSubtype	= USB_ST_ACMF,
			.bmCapabilities		= 0x00,
		},
		.usb_class_union = {
			.bFunctionLength	=
				sizeof(struct usb_class_union_function_descriptor),
			.bDescriptorType	= CS_INTERFACE,
			.bDescriptorSubtype	= USB_ST_UF,
			.bMasterInterface	= 0x00,
			.bSlaveInterface0	= 0x01,
		},
		.notification_endpoint = {
			.bLength =
				sizeof(struct usb_endpoint_descriptor),
			.bDescriptorType	= USB_DT_ENDPOINT,
			.bEndpointAddress	= UDC_INT_ENDPOINT | USB_DIR_IN,
			.bmAttributes		= USB_ENDPOINT_XFER_INT,
			.wMaxPacketSize
				= cpu_to_le16(CONFIG_USBD_SERIAL_INT_PKTSIZE),
			.bInterval		= 0xb,
		},

		/* Interface 2 */
		.data_class_interface = {
			.bLength		=
				sizeof(struct usb_interface_descriptor),
			.bDescriptorType	= USB_DT_INTERFACE,
			.bInterfaceNumber	= 0x01,
			.bAlternateSetting	= 0x00,
			.bNumEndpoints		= 0x02,
			.bInterfaceClass	=
				COMMUNICATIONS_INTERFACE_CLASS_DATA,
			.bInterfaceSubClass	= DATA_INTERFACE_SUBCLASS_NONE,
			.bInterfaceProtocol	= DATA_INTERFACE_PROTOCOL_NONE,
			.iInterface		= 0,
		},
		.data_endpoints = {
			{
				.bLength		=
					sizeof(struct usb_endpoint_descriptor),
				.bDescriptorType	= USB_DT_ENDPOINT,
				.bEndpointAddress	= UDC_OUT_ENDPOINT | USB_DIR_OUT,
				.bmAttributes		=
					USB_ENDPOINT_XFER_BULK,
				.wMaxPacketSize		=
					cpu_to_le16(CONFIG_USBD_SERIAL_BULK_PKTSIZE),
				.bInterval		= 0xFF,
			},
			{
				.bLength		=
					sizeof(struct usb_endpoint_descriptor),
				.bDescriptorType	= USB_DT_ENDPOINT,
				.bEndpointAddress	= UDC_IN_ENDPOINT | USB_DIR_IN,
				.bmAttributes		=
					USB_ENDPOINT_XFER_BULK,
				.wMaxPacketSize		=
					cpu_to_le16(CONFIG_USBD_SERIAL_BULK_PKTSIZE),
				.bInterval		= 0xFF,
			},
		},
#ifdef CONFIG_USB_ACM_TWO_CHANS
		/* function 2 */
		.function_desc2 = {
			.bLength = sizeof(struct usb_function_descriptor),
			.bDescriptorType = 0x0B,
			.bFirstInterface = 2,
			.bInterfaceCount = 2,
			.bFunctionClass = COMMUNICATIONS_INTERFACE_CLASS_CONTROL,
			.bFunctionSubClass = COMMUNICATIONS_ACM_SUBCLASS,
			.bFunctionProtocol = COMMUNICATIONS_V25TER_PROTOCOL,
			.iFunction = 0,
		},
		/* Interface 3 */
		.interface_desc2 = {
			.bLength  = sizeof(struct usb_interface_descriptor),
			.bDescriptorType = USB_DT_INTERFACE,
			.bInterfaceNumber = 0x02,
			.bAlternateSetting = 0,
			.bNumEndpoints = 0x01,
			.bInterfaceClass =
				COMMUNICATIONS_INTERFACE_CLASS_CONTROL,
			.bInterfaceSubClass = COMMUNICATIONS_ACM_SUBCLASS,
			.bInterfaceProtocol = COMMUNICATIONS_V25TER_PROTOCOL,
			.iInterface = 0,
		},
		.usb_class_header2 = {
			.bFunctionLength	=
				sizeof(struct usb_class_header_function_descriptor),
			.bDescriptorType	= CS_INTERFACE,
			.bDescriptorSubtype	= USB_ST_HEADER,
			.bcdCDC	= cpu_to_le16(110),
		},
		.usb_class_call_mgt2 = {
			.bFunctionLength	=
				sizeof(struct usb_class_call_management_descriptor),
			.bDescriptorType	= CS_INTERFACE,
			.bDescriptorSubtype	= USB_ST_CMF,
			.bmCapabilities		= 0x00,
			.bDataInterface		= 0x03,
		},
		.usb_class_acm2 = {
			.bFunctionLength	=
				sizeof(struct usb_class_abstract_control_descriptor),
			.bDescriptorType	= CS_INTERFACE,
			.bDescriptorSubtype	= USB_ST_ACMF,
			.bmCapabilities		= 0x00,
		},
		.usb_class_union2 = {
			.bFunctionLength	=
				sizeof(struct usb_class_union_function_descriptor),
			.bDescriptorType	= CS_INTERFACE,
			.bDescriptorSubtype	= USB_ST_UF,
			.bMasterInterface	= 0x02,
			.bSlaveInterface0	= 0x03,
		},
		.notification_endpoint2 = {
			.bLength =
				sizeof(struct usb_endpoint_descriptor),
			.bDescriptorType	= USB_DT_ENDPOINT,
			.bEndpointAddress	= UDC_INT_ENDPOINT2 | USB_DIR_IN,
			.bmAttributes		= USB_ENDPOINT_XFER_INT,
			.wMaxPacketSize
				= cpu_to_le16(CONFIG_USBD_SERIAL_INT_PKTSIZE),
			.bInterval		= 0xb,
		},

		/* Interface 4 */
		.data_class_interface2 = {
			.bLength		=
				sizeof(struct usb_interface_descriptor),
			.bDescriptorType	= USB_DT_INTERFACE,
			.bInterfaceNumber	= 0x03,
			.bAlternateSetting	= 0x00,
			.bNumEndpoints		= 0x02,
			.bInterfaceClass	=
				COMMUNICATIONS_INTERFACE_CLASS_DATA,
			.bInterfaceSubClass	= DATA_INTERFACE_SUBCLASS_NONE,
			.bInterfaceProtocol	= DATA_INTERFACE_PROTOCOL_NONE,
			.iInterface		= 0,
		},
		.data_endpoints2 = {
			{
				.bLength		=
					sizeof(struct usb_endpoint_descriptor),
				.bDescriptorType	= USB_DT_ENDPOINT,
				.bEndpointAddress	= UDC_OUT_ENDPOINT2 | USB_DIR_OUT,
				.bmAttributes		=
					USB_ENDPOINT_XFER_BULK,
				.wMaxPacketSize		=
					cpu_to_le16(CONFIG_USBD_SERIAL_BULK_PKTSIZE),
				.bInterval		= 0xFF,
			},
			{
				.bLength		=
					sizeof(struct usb_endpoint_descriptor),
				.bDescriptorType	= USB_DT_ENDPOINT,
				.bEndpointAddress	= UDC_IN_ENDPOINT2 | USB_DIR_IN,
				.bmAttributes		=
					USB_ENDPOINT_XFER_BULK,
				.wMaxPacketSize		=
					cpu_to_le16(CONFIG_USBD_SERIAL_BULK_PKTSIZE),
				.bInterval		= 0xFF,
			},
		},
#endif //CONFIG_USB_ACM_TWO_CHANS
	},
};

static struct rs232_emu rs232_desc={
		.dter		=	115200,
		.stop_bits	=	0x00,
		.parity		=	0x00,
		.data_bits	=	0x08
};

/*
 * Static Function Prototypes
 */

static void usbser_init_instances (void);
static void usbser_init_endpoints (void);
static void usbser_init_terminal_type(void);
static void usbser_event_handler (struct usb_device_instance *device,
				usb_device_event_t event, int data);
static int usbser_cdc_setup(struct usb_device_request *request,
				struct urb *urb);
static int usbser_configured (void);


/*
 * Test whether a character is in the RX buffer
 */

int usbser_tstc (int chan)
{
	udc_irq();

	return udc_chars_in_rxfifo(rx_endpoint[chan]);
}

/*
 * Read a single byte from the usb client port. Returns 1 on success, 0
 * otherwise. When the function is succesfull, the character read is
 * written into its argument c.
 */
int usbser_getc (int chan)
{
	unsigned char c = 0;

	usbser_read(chan, &c, 1);
	return c;
}

/*
 * Output a single byte to the usb client port.
 */
void usbser_putc (int chan, const char c)
{
	usbser_write(chan, (u8 *)&c, 1);
}

int usbser_read(int chan, unsigned char *_buf, unsigned int len)
{
	int count = 0, i;
	struct usb_endpoint_instance *ep_out = NULL;
	struct urb *urb = NULL;
	unsigned char *buf = _buf;
	unsigned short ep = rx_endpoint[chan];
	u32 rx_avail;

	for(i = 1; i <= NUM_ENDPOINTS; i++) {
		if(endpoint_instance[i].endpoint_address == (ep | USB_DIR_OUT))
			ep_out = &endpoint_instance[i];
	}
	if(!ep_out)
		return -1;
	urb = ep_out->rcv_urb;

	DBG(" buf len: %d\n", len);

	rx_avail = udc_chars_in_rxfifo(ep);
	if (rx_avail > 0) {
		if (len <= rx_avail) {
			usbd_setup_urb(urb, buf, len, 0);
			udc_irq();
			if(!urb->actual_length)
				poll_rx_ep(ep);
			usbd_free_urb(urb);
			return len;
		} else {
			//first flush all chars in buffer
			usbd_setup_urb(urb, buf, rx_avail, 0);
			udc_irq();
			if(!urb->actual_length)
				poll_rx_ep(ep);
			buf += rx_avail;
			len -= rx_avail;
			count += rx_avail;
			usbd_free_urb(urb);
		}
	}
	//now get remain data...
	while (len > 0) {
		int xfer;
		int dma = 0;
		int maxPktSize = ep_out->rcv_packetSize;

		xfer = (len > maxPktSize) ? maxPktSize : len;
		if (xfer == maxPktSize)
			dma = 1;
		usbd_setup_urb(urb, buf, xfer, dma);
		udc_irq();
		if(!urb->actual_length)
			poll_rx_ep(ep);

		DBG("urb actual_len :%d\n", urb->actual_length);
		if (urb->actual_length) {
			buf += urb->actual_length;
			len -= urb->actual_length;
			count += urb->actual_length;
			usbd_free_urb(urb);
		}
	}

	DBG("read : %d\n", count);
	return count;
}

int usbser_write(int chan, const unsigned char *_buf, unsigned int len)
{
	int i;
	struct usb_endpoint_instance *ep_in = NULL;
	struct urb *current_urb = NULL;
	unsigned char *buf = (unsigned char *)_buf;
	int count = 0;
	unsigned short ep = tx_endpoint[chan];
#ifdef CONFIG_USB_ACM_TWO_CHANS
	static int acm_ch1_opened = 1;
	if(chan == USB_ACM_CHAN_1 && !acm_ch1_opened)
		return -1;
#endif

	for(i = 1; i <= NUM_ENDPOINTS; i++) {
		if(endpoint_instance[i].endpoint_address == (ep | USB_DIR_IN))
			ep_in = &endpoint_instance[i];
	}
	if(!ep_in)
		return -1;

	if (!usbser_configured ()) {
		return 0;
	}
	current_urb = ep_in->tx_urb;

	while(len > 0) {
		int xfer;
		int maxPktSize = ep_in->rcv_packetSize;

		xfer = (len > maxPktSize) ? maxPktSize : len;

		current_urb->buffer = buf;
		current_urb->actual_length = xfer;

		if(udc_endpoint_write (ep_in))
			goto oops;
		count += xfer;
		len -= xfer;
		buf += xfer;
	}

	DBG("after write urb actual_len :%d count %d\n",
		current_urb->actual_length, count);
	return count;

oops:
	serial_puts("usb write error\n");
#ifdef CONFIG_USB_ACM_TWO_CHANS
	if(chan == USB_ACM_CHAN_1)
		acm_ch1_opened = 0;
#endif
	return -1;
}

int drv_usbser_init (void)
{
	static int usb_init_flag = 0;

	if (usb_init_flag)
		return 0;
	usb_init_flag = 1;

	/* Decide on which type of UDC device to be.
	 */
	usbser_init_terminal_type();

	/* Now, set up USB controller and infrastructure */
	if (udc_is_initialized()) {
		printf("UDC software initializing...\n");
		udc_soft_init ();		/* Basic USB initialization */
	} else {
		printf("UDC hardware initializing...\n");
		udc_init ();
	}

	usbser_init_instances ();
	usbser_init_endpoints ();

	if (!udc_is_initialized()) {
		udc_startup_events (device_instance);/* Enable dev, init udc pointers */
		udc_connect ();		/* Enable pullup for host detection */
	} else {
		udc_hot_startup(device_instance);
	}

	return 0;
}


#define init_wMaxPacketSize(x)	le16_to_cpu(get_unaligned(\
			&ep_descriptor_ptrs[(x) - 1]->wMaxPacketSize));

#define init_write_wMaxPacketSize(val, x)	 (put_unaligned(le16_to_cpu(val),\
			&ep_descriptor_ptrs[(x) - 1]->wMaxPacketSize));

static struct urb urb_pool[NUM_ENDPOINTS];
static u8 int_ep_buffer[URB_BUF_SIZE] __attribute__((__aligned__(4)));

static int endpoint_is_int(struct usb_endpoint_instance *instance)
{
	if (!instance)
		return 0;

	if (instance->tx_attributes == USB_ENDPOINT_XFER_INT)
		return 1;
	else
		return 0;
}
static void usbser_init_instances (void)
{
	int i;

	/* initialize device instance */
	memset (device_instance, 0, sizeof (struct usb_device_instance));
	if (!udc_is_initialized())
		device_instance->device_state = STATE_INIT;
	else
		device_instance->device_state = STATE_CONFIGURED;
	device_instance->device_descriptor = &device_descriptor;
#if defined(CONFIG_USBD_HS)
	device_instance->qualifier_descriptor = &qualifier_descriptor;
#endif
	device_instance->event = usbser_event_handler;
	device_instance->cdc_recv_setup = usbser_cdc_setup;
	device_instance->bus = bus_instance;
	device_instance->configurations = NUM_CONFIGS;
	device_instance->configuration_instance_array = config_instance;

	/* initialize bus instance */
	memset (bus_instance, 0, sizeof (struct usb_bus_instance));
	bus_instance->device = device_instance;
	bus_instance->endpoint_array = endpoint_instance;
	bus_instance->max_endpoints = 1;
	bus_instance->maxpacketsize = 64;

	/* configuration instance */
	memset (config_instance, 0,
		sizeof (struct usb_configuration_instance));
	config_instance->interfaces = NUM_ACM_INTERFACES;
	config_instance->configuration_descriptor = configuration_descriptor;
	config_instance->interface_instance_array = interface_instance;

	/* interface instance */
	memset (interface_instance, 0,
		sizeof (struct usb_interface_instance));
	interface_instance->alternates = 1;
	interface_instance->alternates_instance_array = alternate_instance;

	/* alternates instance */
	memset (alternate_instance, 0,
		sizeof (struct usb_alternate_instance));
	alternate_instance->interface_descriptor = interface_descriptors;
	alternate_instance->endpoints = NUM_ENDPOINTS;
	alternate_instance->endpoints_descriptor_array = ep_descriptor_ptrs;

	/* endpoint instances */
	memset (&endpoint_instance[0], 0,
		sizeof (struct usb_endpoint_instance));
	endpoint_instance[0].endpoint_address = 0;
	endpoint_instance[0].rcv_packetSize = EP0_MAX_PACKET_SIZE;
	endpoint_instance[0].rcv_attributes = USB_ENDPOINT_XFER_CONTROL;
	endpoint_instance[0].tx_packetSize = EP0_MAX_PACKET_SIZE;
	endpoint_instance[0].tx_attributes = USB_ENDPOINT_XFER_CONTROL;
	udc_setup_ep (device_instance, 0, &endpoint_instance[0]);

	for (i = 1; i <= NUM_ENDPOINTS; i++) {
		memset (&endpoint_instance[i], 0,
			sizeof (struct usb_endpoint_instance));

		endpoint_instance[i].endpoint_address =
			ep_descriptor_ptrs[i - 1]->bEndpointAddress;

		endpoint_instance[i].rcv_attributes =
			ep_descriptor_ptrs[i - 1]->bmAttributes;

		endpoint_instance[i].rcv_packetSize = init_wMaxPacketSize(i);

		endpoint_instance[i].tx_attributes =
			ep_descriptor_ptrs[i - 1]->bmAttributes;

		endpoint_instance[i].tx_packetSize = init_wMaxPacketSize(i);


		urb_link_init (&endpoint_instance[i].rcv);
		urb_link_init (&endpoint_instance[i].rdy);
		urb_link_init (&endpoint_instance[i].tx);
		urb_link_init (&endpoint_instance[i].done);

		if (endpoint_instance[i].endpoint_address & USB_DIR_IN) {
			endpoint_instance[i].tx_urb = &urb_pool[i - 1];
			/* function only has one interrupt endpoint */
			if (endpoint_is_int(&endpoint_instance[i]))
				usbd_init_urb (endpoint_instance[i].tx_urb, device_instance,
					&endpoint_instance[i], int_ep_buffer, URB_BUF_SIZE);
			else // bulk in&out re-use buffer with application driver
				usbd_init_urb (endpoint_instance[i].tx_urb, device_instance,
					&endpoint_instance[i], NULL, 0);
		} else {
			endpoint_instance[i].rcv_urb = &urb_pool[i - 1];
			usbd_init_urb (endpoint_instance[i].rcv_urb, device_instance,
					&endpoint_instance[i], NULL, 0);
		}
	}
}

static void usbser_init_endpoints (void)
{
	int i;

	bus_instance->max_endpoints = NUM_ENDPOINTS + 1;

	for (i = 0; i < NUM_ENDPOINTS; i++) {
		if (((ep_descriptor_ptrs[i]->bmAttributes &
			USB_ENDPOINT_XFERTYPE_MASK) ==
			USB_ENDPOINT_XFER_BULK)
			&& is_usbd_high_speed()) {

			init_write_wMaxPacketSize(CONFIG_USBD_SERIAL_BULK_HS_PKTSIZE, i+1);
		}

		endpoint_instance[i + 1].tx_packetSize =  init_wMaxPacketSize(i + 1);
		endpoint_instance[i + 1].rcv_packetSize =  init_wMaxPacketSize(i + 1);

		udc_setup_ep (device_instance, i + 1, &endpoint_instance[i+1]);
	}
}

/* usbser_init_terminal_type
 *
 * Do some late binding for our device type.
 */
static void usbser_init_terminal_type(void)
{
	/* CDC ACM */
	/* Assign endpoint descriptors */
	ep_descriptor_ptrs[0] =
		&acm_configuration_descriptors[0].notification_endpoint;
	ep_descriptor_ptrs[1] =
		&acm_configuration_descriptors[0].data_endpoints[0];
	ep_descriptor_ptrs[2] =
		&acm_configuration_descriptors[0].data_endpoints[1];
#ifdef CONFIG_USB_ACM_TWO_CHANS
	ep_descriptor_ptrs[3] =
		&acm_configuration_descriptors[0].notification_endpoint2;
	ep_descriptor_ptrs[4] =
		&acm_configuration_descriptors[0].data_endpoints2[0];
	ep_descriptor_ptrs[5] =
		&acm_configuration_descriptors[0].data_endpoints2[1];
#endif

#if defined(CONFIG_USBD_HS)
	qualifier_descriptor.bDeviceClass =
		COMMUNICATIONS_DEVICE_CLASS;
#endif
	/* Assign endpoint indices */
	tx_endpoint[0] = UDC_IN_ENDPOINT;
	rx_endpoint[0] = UDC_OUT_ENDPOINT;
#ifdef CONFIG_USB_ACM_TWO_CHANS
	tx_endpoint[1] = UDC_IN_ENDPOINT2;
	rx_endpoint[1] = UDC_OUT_ENDPOINT2;
#endif

	/* Configuration Descriptor */
	configuration_descriptor =
		(struct usb_configuration_descriptor*)
		&acm_configuration_descriptors;

}

static int usbser_configured (void)
{
	return usbser_configured_flag;
}

/******************************************************************************/

static void usbser_event_handler (struct usb_device_instance *device,
				  usb_device_event_t event, int data)
{
	switch (event) {
	case DEVICE_RESET:
	case DEVICE_BUS_INACTIVE:
		usbser_configured_flag = 0;
		break;
	case DEVICE_CONFIGURED:
		usbser_configured_flag = 1;
		break;

	case DEVICE_ADDRESS_ASSIGNED:
		usbser_init_endpoints ();

	default:
		break;
	}
}

/******************************************************************************/

int usbser_cdc_setup(struct usb_device_request *request, struct urb *urb)
{
	switch (request->bRequest){

		case ACM_SET_CONTROL_LINE_STATE:	/* Implies DTE ready */
			break;
		case ACM_SEND_ENCAPSULATED_COMMAND :	/* Required */
			break;
		case ACM_SET_LINE_ENCODING :		/* DTE stop/parity bits
							 * per character */
			break;
		case ACM_GET_ENCAPSULATED_RESPONSE :	/* request response */
			break;
		case ACM_GET_LINE_ENCODING :		/* request DTE rate,
							 * stop/parity bits */
			memcpy (urb->buffer , &rs232_desc, sizeof(rs232_desc));
			urb->actual_length = sizeof(rs232_desc);

			break;
		default:
			return 1;
	}
	return 0;
}

/******************************************************************************/
#include <command.h>
int usbser_test(cmd_tbl_t * cmdtp, int flag, int argc, char * const argv[])
{
	char c;

	drv_usbser_init();
	serial_puts("usb tty test\n");
	while (1) {
		c = usbser_getc(USB_ACM_CHAN_0);
		serial_putc(c);
		//usbser_puts("from usb ");
		usbser_putc(USB_ACM_CHAN_0, c);
		//usbser_puts("\n");
		if (c == 'Q' || c == 'q')
			break;
	}
	return 0;
}

U_BOOT_CMD(usbser_test, 1, 1, usbser_test,
           "rda usbserial test", "loopback everything to user");
