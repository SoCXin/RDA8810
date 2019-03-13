/*
 * ether.c -- Ethernet gadget driver, with CDC and non-CDC options
 *
 * Copyright (C) 2003-2005,2008 David Brownell
 * Copyright (C) 2003-2004 Robert Schwebel, Benedikt Spranger
 * Copyright (C) 2008 Nokia Corporation
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#define DEBUG

#include <common.h>
#include <config.h>
#include <asm/unaligned.h>
#include "usbdescriptors.h"
#include <usbdevice.h>

#if defined(CONFIG_MUSB_UDC)
#include <usb/musb_udc.h>
#else
#error "no usb device controller"
#endif

#define DEV_CONFIG_CDC	   1

/* defined and used by gadget/ep0.c */
extern struct usb_string_descriptor **usb_strings;

/*-------------------------------------------------------------------------*/

/*
 * Thanks to NetChip Technologies for donating this product ID.
 * It's for devices with only CDC Ethernet configurations.
 */
#define FASTBOOT_VENDOR_NUM		0x18D1	/* Goolge VID */
#define FASTBOOT_PRODUCT_NUM		0x4EE0	/* Bootloader Product ID */

#define FASTBOOT_MANUFACTURER			"RDAmicro"
#define FASTBOOT_PRODUCT_NAME			"RDA Droid"
#define FASTBOOT_CONFIGURATION_STR 	"fastboot"
#define FASTBOOT_DATA_INTERFACE_STR 	"fastboot data intf"

/*-------------------------------------------------------------------------*/

/*
 * DESCRIPTORS ... most are static, but strings and (full) configuration
 * descriptors are built on demand.  For now we do either full CDC, or
 * our simple subset.
 */

#define STRING_MANUFACTURER		1
#define STRING_PRODUCT				2
#define STRING_DATA				3
#define STRING_CFG					4
#define STRING_SERIALNUMBER		5

#define NUM_ENDPOINTS 2

static struct usb_device_instance fastboot_dev_instance[1];
static struct usb_bus_instance fastboot_bus_instance[1];
static struct usb_configuration_instance fastboot_cfg_instance;
static struct usb_interface_instance fastboot_intf_instance[1];
static struct usb_alternate_instance fastboot_alternate_instance[1];
/* one extra for control endpoint */
static struct usb_endpoint_instance fastboot_ep_instance[NUM_ENDPOINTS + 1];

static char serial_number[] = "dragon2012";

static struct usb_endpoint_descriptor *ep_desc_ptrs[NUM_ENDPOINTS];

static struct usb_device_descriptor
 device_desc = {
	.bLength = sizeof device_desc,
	.bDescriptorType = USB_DT_DEVICE,

	.bcdUSB = __constant_cpu_to_le16(0x0200),

	.bDeviceClass = USB_CLASS_VENDOR_SPEC,
	.bDeviceSubClass = 0,
	.bDeviceProtocol = 0,
	.bMaxPacketSize0 = 64,
	.idVendor = __constant_cpu_to_le16(FASTBOOT_VENDOR_NUM),
	.idProduct = __constant_cpu_to_le16(FASTBOOT_PRODUCT_NUM),
	.iManufacturer = STRING_MANUFACTURER,
	.iProduct = STRING_PRODUCT,
	.iSerialNumber = STRING_SERIALNUMBER,
	.bNumConfigurations = 1,
};

static struct usb_interface_descriptor
 data_intf = {
	.bLength = sizeof data_intf,
	.bDescriptorType = USB_DT_INTERFACE,

	.bInterfaceNumber = 0,
	.bAlternateSetting = 0,
	.bNumEndpoints = 2,
	.bInterfaceClass = USB_CLASS_VENDOR_SPEC,
	.bInterfaceSubClass = 0x42,
	.bInterfaceProtocol = 3,
	.iInterface = STRING_DATA,
};

static struct usb_qualifier_descriptor
 dev_qualifier = {
	.bLength = sizeof dev_qualifier,
	.bDescriptorType = USB_DT_QUAL,

	.bcdUSB = __constant_cpu_to_le16(0x0200),
	.bDeviceClass = USB_CLASS_VENDOR_SPEC,

	.bMaxPacketSize0 = 64,
	.bNumConfigurations = 1,
};

struct fastboot_config_desc {
	struct usb_configuration_descriptor configuration_desc;
	struct usb_interface_descriptor interface_desc[1];
	struct usb_endpoint_descriptor data_eps[NUM_ENDPOINTS];
} __attribute__ ((packed));

static struct fastboot_config_desc
fastboot_cfg_desc = {
	.configuration_desc = {
		.bLength =
			sizeof(struct usb_configuration_descriptor),
		.bDescriptorType = USB_DT_CONFIG,
		.wTotalLength =
			cpu_to_le16(sizeof
					(struct fastboot_config_desc)),
		.bNumInterfaces = 1,
		.bConfigurationValue = 1,
		.iConfiguration = STRING_CFG,
		.bmAttributes =
			BMATTRIBUTE_SELF_POWERED | BMATTRIBUTE_RESERVED,
		.bMaxPower = 0xfa
	},
	.interface_desc = {
		{
			.bLength = sizeof(struct usb_interface_descriptor),
			.bDescriptorType = USB_DT_INTERFACE,
			.bInterfaceNumber = 0,
			.bAlternateSetting = 0,
			.bNumEndpoints = NUM_ENDPOINTS,
			.bInterfaceClass = USB_CLASS_VENDOR_SPEC,
			.bInterfaceSubClass = 0x42,
			.bInterfaceProtocol = 3,
			.iInterface = STRING_DATA
		},
	},
	.data_eps = {
		{
			.bLength = sizeof(struct usb_endpoint_descriptor),
			.bDescriptorType = USB_DT_ENDPOINT,
			.bEndpointAddress = (1) | USB_DIR_OUT,
			.bmAttributes = USB_ENDPOINT_XFER_BULK,
			.wMaxPacketSize = cpu_to_le16(64),
			.bInterval = 0xFF,
		},
		{
			.bLength = sizeof(struct usb_endpoint_descriptor),
			.bDescriptorType = USB_DT_ENDPOINT,
			.bEndpointAddress = (2) | USB_DIR_IN,
			.bmAttributes = USB_ENDPOINT_XFER_BULK,
			.wMaxPacketSize = cpu_to_le16(64),
			.bInterval = 0xFF,
		},
	},
};

/*-------------------------------------------------------------------------*/



static int fastboot_configured_flag = 0;

static void fastboot_init_endpoints(void);
static void fastboot_event_handler(struct usb_device_instance *device,
				   usb_device_event_t event, int data)
{
#if defined(CONFIG_USBD_HS)
	int i;
#endif
	switch (event) {
	case DEVICE_RESET:
	case DEVICE_BUS_INACTIVE:
		fastboot_configured_flag = 0;

		break;
	case DEVICE_CONFIGURED:
		fastboot_configured_flag = 1;
		break;

	case DEVICE_ADDRESS_ASSIGNED:
#if defined(CONFIG_USBD_HS)
		/*
		 * is_usbd_high_speed routine needs to be defined by
		 * specific gadget driver
		 * It returns TRUE if device enumerates at High speed
		 * Retuns FALSE otherwise
		 */
		for (i = 0; i < NUM_ENDPOINTS; i++) {
			if (((ep_desc_ptrs[i]->bmAttributes &
			      USB_ENDPOINT_XFERTYPE_MASK) ==
			     USB_ENDPOINT_XFER_BULK)
			    && is_usbd_high_speed()) {

				ep_desc_ptrs[i]->wMaxPacketSize = 512;
			}

			fastboot_ep_instance[i + 1].tx_packetSize =
			    ep_desc_ptrs[i]->wMaxPacketSize;
			fastboot_ep_instance[i + 1].rcv_packetSize =
			    ep_desc_ptrs[i]->wMaxPacketSize;
		}
#endif
		fastboot_init_endpoints();

	default:
		break;
	}
}

/* utility function for converting char* to wide string used by USB */
static void str2wide (char *str, u16 * wide)
{
	int i;
	for (i = 0; i < strlen (str) && str[i]; i++){
		#if defined(__LITTLE_ENDIAN)
			wide[i] = (u16) str[i];
		#elif defined(__BIG_ENDIAN)
			wide[i] = ((u16)(str[i])<<8);
		#else
			#error "__LITTLE_ENDIAN or __BIG_ENDIAN undefined"
		#endif
	}
}

static struct usb_string_descriptor *fastboot_str_tab[10];

static void fastboot_init_strings (void)
{
	struct usb_string_descriptor *string;

	static u8 wstrLang[4] = {4,USB_DT_STRING,0x9,0x4};
	static u8 wstrManufacturer[2 + 2*(sizeof(FASTBOOT_MANUFACTURER)-1)];
	static u8 wstrProduct[2 + 2*(sizeof(FASTBOOT_PRODUCT_NAME)-1)];
	static u8 wstrSerial[2 + 2*(sizeof(serial_number) - 1)];
	static u8 wstrConfiguration[2 + 2*(sizeof(FASTBOOT_CONFIGURATION_STR)-1)];
	static u8 wstrDataInterface[2 + 2*(sizeof(FASTBOOT_DATA_INTERFACE_STR)-1)];



	fastboot_str_tab[0] =
		(struct usb_string_descriptor*)wstrLang;

	string = (struct usb_string_descriptor *) wstrManufacturer;
	string->bLength = sizeof(wstrManufacturer);
	string->bDescriptorType = USB_DT_STRING;
	str2wide (FASTBOOT_MANUFACTURER, string->wData);
	fastboot_str_tab[STRING_MANUFACTURER]=string;


	string = (struct usb_string_descriptor *) wstrProduct;
	string->bLength = sizeof(wstrProduct);
	string->bDescriptorType = USB_DT_STRING;
	str2wide (FASTBOOT_PRODUCT_NAME, string->wData);
	fastboot_str_tab[STRING_PRODUCT]=string;


	string = (struct usb_string_descriptor *) wstrSerial;
	string->bLength = sizeof(wstrSerial);
	string->bDescriptorType = USB_DT_STRING;
	str2wide (serial_number, string->wData);
	fastboot_str_tab[STRING_SERIALNUMBER]=string;

	string = (struct usb_string_descriptor *) wstrConfiguration;
	string->bLength = sizeof(wstrConfiguration);
	string->bDescriptorType = USB_DT_STRING;
	str2wide (FASTBOOT_CONFIGURATION_STR, string->wData);
	fastboot_str_tab[STRING_CFG]=string;

	string = (struct usb_string_descriptor *) wstrDataInterface;
	string->bLength = sizeof(wstrDataInterface);
	string->bDescriptorType = USB_DT_STRING;
	str2wide (FASTBOOT_DATA_INTERFACE_STR, string->wData);
	fastboot_str_tab[STRING_DATA]=string;

	/* Now, initialize the string table for ep0 handling */
	usb_strings = fastboot_str_tab;
}
#define init_wMaxPacketSize(x)	le16_to_cpu(get_unaligned(\
			&ep_desc_ptrs[(x) - 1]->wMaxPacketSize));

static void fastboot_init_instances(void)
{
	int i;

	/* initialize device instance */
	memset(fastboot_dev_instance, 0, sizeof(struct usb_device_instance));
	fastboot_dev_instance->device_state = STATE_INIT;
	fastboot_dev_instance->device_descriptor = &device_desc;
#if defined(CONFIG_USBD_HS)
	fastboot_dev_instance->qualifier_descriptor = &dev_qualifier;
#endif
	fastboot_dev_instance->event = fastboot_event_handler;
	fastboot_dev_instance->bus = fastboot_bus_instance;
	fastboot_dev_instance->configurations = 1;
	fastboot_dev_instance->configuration_instance_array =
	    &fastboot_cfg_instance;

	/* initialize bus instance */
	memset(fastboot_bus_instance, 0, sizeof(struct usb_bus_instance));
	fastboot_bus_instance->device = fastboot_dev_instance;
	fastboot_bus_instance->endpoint_array = fastboot_ep_instance;
	fastboot_bus_instance->max_endpoints = 1;
	fastboot_bus_instance->maxpacketsize = 64;
	fastboot_bus_instance->serial_number_str = serial_number;

	/* configuration instance */
	memset(&fastboot_cfg_instance, 0,
	       sizeof(struct usb_configuration_instance));
	fastboot_cfg_instance.interfaces = 1;
	fastboot_cfg_instance.configuration_descriptor =
	    (struct usb_configuration_descriptor *)
	    &fastboot_cfg_desc;
	fastboot_cfg_instance.interface_instance_array =
	    fastboot_intf_instance;

	/* interface instance */
	memset(fastboot_intf_instance, 0,
	       sizeof(struct usb_interface_instance));
	fastboot_intf_instance->alternates = 1;
	fastboot_intf_instance->alternates_instance_array =
	    fastboot_alternate_instance;

	/* alternates instance */
	memset(fastboot_alternate_instance, 0,
	       sizeof(struct usb_alternate_instance));
	fastboot_alternate_instance->interface_descriptor = &data_intf;
	fastboot_alternate_instance->endpoints = NUM_ENDPOINTS;
	fastboot_alternate_instance->endpoints_descriptor_array = ep_desc_ptrs;

	/* endpoint instances */
	memset(&fastboot_ep_instance[0], 0,
	       sizeof(struct usb_endpoint_instance));
	fastboot_ep_instance[0].endpoint_address = 0;
	fastboot_ep_instance[0].rcv_packetSize = 64;
	fastboot_ep_instance[0].rcv_attributes = USB_ENDPOINT_XFER_CONTROL;
	fastboot_ep_instance[0].tx_packetSize = 64;
	fastboot_ep_instance[0].tx_attributes = USB_ENDPOINT_XFER_CONTROL;
	udc_setup_ep(fastboot_dev_instance, 0,
		     &fastboot_ep_instance[0]);

	for (i = 1; i <= NUM_ENDPOINTS; i++) {
		memset(&fastboot_ep_instance[i], 0,
		       sizeof(struct usb_endpoint_instance));

		fastboot_ep_instance[i].endpoint_address =
		    ep_desc_ptrs[i - 1]->bEndpointAddress;

		fastboot_ep_instance[i].rcv_attributes =
		    ep_desc_ptrs[i - 1]->bmAttributes;

		fastboot_ep_instance[i].rcv_packetSize =
		    init_wMaxPacketSize(i);

		fastboot_ep_instance[i].tx_attributes =
		    ep_desc_ptrs[i - 1]->bmAttributes;

		fastboot_ep_instance[i].tx_packetSize =
		    init_wMaxPacketSize(i);

		fastboot_ep_instance[i].tx_attributes =
		    ep_desc_ptrs[i - 1]->bmAttributes;

		urb_link_init(&fastboot_ep_instance[i].rcv);
		urb_link_init(&fastboot_ep_instance[i].rdy);
		urb_link_init(&fastboot_ep_instance[i].tx);
		urb_link_init(&fastboot_ep_instance[i].done);

		if (fastboot_ep_instance[i].endpoint_address & USB_DIR_IN)
			fastboot_ep_instance[i].tx_urb =
			    usbd_alloc_urb(fastboot_dev_instance,
					   &fastboot_ep_instance[i]);
		else
			fastboot_ep_instance[i].rcv_urb =
			    usbd_alloc_urb(fastboot_dev_instance,
					   &fastboot_ep_instance[i]);
	}
}

static void fastboot_init_endpoints(void)
{
	int i;

	fastboot_bus_instance->max_endpoints = NUM_ENDPOINTS + 1;
	for (i = 1; i <= NUM_ENDPOINTS; i++) {
		udc_setup_ep(fastboot_dev_instance, i,
			     &fastboot_ep_instance[i]);
	}
}

struct usb_endpoint_instance * fastboot_get_out_ep(void)
{
	int i;
	int ep_addr;

	for (i = 1; i <= NUM_ENDPOINTS; i++) {
		ep_addr = fastboot_ep_instance[i].endpoint_address;
		if ((ep_addr != 0) && ((ep_addr & 0x80) == 0))
			return &fastboot_ep_instance[i];
	}

	serial_printf("no out endpoint for fastboot\n");
	return NULL;
}

struct usb_endpoint_instance * fastboot_get_in_ep(void)
{
	int i;

	for (i = 1; i <= NUM_ENDPOINTS; i++) {
		if (fastboot_ep_instance[i].endpoint_address & USB_DIR_IN)
			return &fastboot_ep_instance[i];
	}

	serial_printf("no in endpoint for fastboot\n");
	return NULL;

}

int fastboot_configured (void)
{
	return fastboot_configured_flag;
}

/*-------------------------------------------------------------------------*/

/*
 * Initialize the usb client port.
 *
 */
int drv_fastboot_init(void)
{
	ep_desc_ptrs[0] = &fastboot_cfg_desc.data_eps[0];
	ep_desc_ptrs[1] = &fastboot_cfg_desc.data_eps[1];

	/* Now, set up USB controller and infrastructure */

	fastboot_init_strings ();
	fastboot_init_instances();

	fastboot_init_endpoints();

	udc_startup_events(fastboot_dev_instance);	/* Enable dev, init udc pointers */
	udc_connect();		/* Enable pullup for host detection */

	return 0;
}

