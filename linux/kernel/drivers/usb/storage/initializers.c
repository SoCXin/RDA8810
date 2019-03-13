/* Special Initializers for certain USB Mass Storage devices
 *
 * Current development and maintenance by:
 *   (c) 1999, 2000 Matthew Dharm (mdharm-usb@one-eyed-alien.net)
 *
 * This driver is based on the 'USB Mass Storage Class' document. This
 * describes in detail the protocol used to communicate with such
 * devices.  Clearly, the designers had SCSI and ATAPI commands in
 * mind when they created this document.  The commands are all very
 * similar to commands in the SCSI-II and ATAPI specifications.
 *
 * It is important to note that in a number of cases this class
 * exhibits class-specific exemptions from the USB specification.
 * Notably the usage of NAK, STALL and ACK differs from the norm, in
 * that they are used to communicate wait, failed and OK on commands.
 *
 * Also, for certain devices, the interrupt endpoint is used to convey
 * status of a command.
 *
 * Please see http://www.one-eyed-alien.net/~mdharm/linux-usb for more
 * information about this driver.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2, or (at your option) any
 * later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <linux/errno.h>

#include "usb.h"
#include "initializers.h"
#include "debug.h"
#include "transport.h"

/* This places the Shuttle/SCM USB<->SCSI bridge devices in multi-target
 * mode */
int usb_stor_euscsi_init(struct us_data *us)
{
	int result;

	usb_stor_dbg(us, "Attempting to init eUSCSI bridge...\n");
	us->iobuf[0] = 0x1;
	result = usb_stor_control_msg(us, us->send_ctrl_pipe,
			0x0C, USB_RECIP_INTERFACE | USB_TYPE_VENDOR,
			0x01, 0x0, us->iobuf, 0x1, 5000);
	usb_stor_dbg(us, "-- result is %d\n", result);

	return 0;
}

/* This function is required to activate all four slots on the UCR-61S2B
 * flash reader */
int usb_stor_ucr61s2b_init(struct us_data *us)
{
	struct bulk_cb_wrap *bcb = (struct bulk_cb_wrap*) us->iobuf;
	struct bulk_cs_wrap *bcs = (struct bulk_cs_wrap*) us->iobuf;
	int res;
	unsigned int partial;
	static char init_string[] = "\xec\x0a\x06\x00$PCCHIPS";

	usb_stor_dbg(us, "Sending UCR-61S2B initialization packet...\n");

	bcb->Signature = cpu_to_le32(US_BULK_CB_SIGN);
	bcb->Tag = 0;
	bcb->DataTransferLength = cpu_to_le32(0);
	bcb->Flags = bcb->Lun = 0;
	bcb->Length = sizeof(init_string) - 1;
	memset(bcb->CDB, 0, sizeof(bcb->CDB));
	memcpy(bcb->CDB, init_string, sizeof(init_string) - 1);

	res = usb_stor_bulk_transfer_buf(us, us->send_bulk_pipe, bcb,
			US_BULK_CB_WRAP_LEN, &partial);
	if (res)
		return -EIO;

	usb_stor_dbg(us, "Getting status packet...\n");
	res = usb_stor_bulk_transfer_buf(us, us->recv_bulk_pipe, bcs,
			US_BULK_CS_WRAP_LEN, &partial);
	if (res)
		return -EIO;

	return 0;
}

/* This places the HUAWEI E220 devices in multi-port mode */
int usb_stor_huawei_e220_init(struct us_data *us)
{
	int result;

	result = usb_stor_control_msg(us, us->send_ctrl_pipe,
				      USB_REQ_SET_FEATURE,
				      USB_TYPE_STANDARD | USB_RECIP_DEVICE,
				      0x01, 0x0, NULL, 0x0, 1000);
	usb_stor_dbg(us, "Huawei mode set result is %d\n", result);
	return 0;
}

int usb_stor_huawei_scsi_init(struct us_data *us)
{
	int result = 0;
	int act_len = 0;

	unsigned char cmd[] = { 0x55, 0x53, 0x42, 0x43, 0x12, 0x34, 0x56, 0x78,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x11,
		0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
	};
	result =
	    usb_stor_bulk_transfer_buf(us, us->send_bulk_pipe, cmd, sizeof(cmd),
				       &act_len);
	US_DEBUGPX
	    ("usb_stor_bulk_transfer_buf performing result is %d, transfer the actual length=%d\n",
	     result, act_len);
	return result;
}

int usb_stor_zte_scsi_init(struct us_data *us)
{
	int result = 0;
	int act_len = 0;

	unsigned char cmd[] = { 0x55, 0x53, 0x42, 0x43, 0xe0, 0xf6, 0x18, 0xff,
		0xc0, 0x00, 0x00, 0x00, 0x80, 0x00, 0x06, 0x9f,
		0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
	};
	result =
	    usb_stor_bulk_transfer_buf(us, us->send_bulk_pipe, cmd, sizeof(cmd),
				       &act_len);
	US_DEBUGPX
	    ("usb_stor_bulk_transfer_buf performing result is %d, transfer the actual length=%d\n",
	     result, act_len);
	return result;
}

static int usb_stor_init_ignor(struct us_data *us)
{
	static struct timeval pre_time = { 0, 0 };
	static int pre_devnum = -1;
	struct timeval now_time;
	int ret;

	jiffies_to_timeval(jiffies, &now_time);

	if (us->ifnum > 1) {
		US_DEBUGPX("USB device has been switched, ifnum %d \n",
			  us->ifnum);
		ret = 1;
		goto done;
	}

	if ((pre_devnum == us->pusb_dev->devnum) &&
	    (now_time.tv_sec - pre_time.tv_sec) < 2) {
		US_DEBUGPX("USB device is switching.\n");
		ret = 1;
		goto done;
	}

	ret = 0;

done:
	pre_devnum = us->pusb_dev->devnum;
	jiffies_to_timeval(jiffies, &pre_time);
	return ret;

}

#define RESPONSE_LEN 1024
static int usb_stor_dev_init(struct us_data *us, char *usb_msg, int msg_len,
			     int resp)
{
	char *buffer;
	int result;

	buffer = kzalloc(RESPONSE_LEN, GFP_KERNEL);
	if ((buffer == NULL) || (msg_len <= 0))
		return USB_STOR_TRANSPORT_ERROR;

	memcpy(buffer, usb_msg, msg_len);
	result = usb_stor_bulk_transfer_buf(us,
					    us->send_bulk_pipe,
					    buffer, msg_len, NULL);
	if (result != USB_STOR_XFER_GOOD) {
		result = USB_STOR_XFER_ERROR;
		goto out;
	}

	/* Some of the devices need to be asked for a response, but we don't
	 * care what that response is.
	 */
	if (resp) {
		US_DEBUGPX("Receive USB device response. \n");
		usb_stor_bulk_transfer_buf(us,
					   us->recv_bulk_pipe,
					   buffer, RESPONSE_LEN, NULL);
	}

	/* Read the CSW */
	result = usb_stor_bulk_transfer_buf(us,
					   us->recv_bulk_pipe,
					   buffer, 13, NULL);
out:
	kfree(buffer);

	US_DEBUGPX("USB device switch result: %d \n", result);
	return result;
}

int usb_stor_zte_mu318_init(struct us_data *us)
{
	char msg[] = {
		0x55, 0x53, 0x42, 0x43, 0x08, 0x80, 0xd1, 0x88,
		0x20, 0x00, 0x00, 0x00, 0x80, 0x00, 0x0a, 0x85,
		0x01, 0x01, 0x01, 0x18, 0x01, 0x01, 0x01, 0x01,
		0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
	};

	if (usb_stor_init_ignor(us)) {
		return 0;
	}

	US_DEBUGPX("ZTE MU318: %s", "DEVICE MODE SWITCH\n");
	return usb_stor_dev_init(us, msg, sizeof(msg), 1);
}

int usb_stor_eject_cd(struct us_data *us)
{
	char msg[] = {
		0x55, 0x53, 0x42, 0x43, 0x12, 0x34, 0x56, 0x78,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 0x1b,
		0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
	};

	if (usb_stor_init_ignor(us)) {
		return 0;
	}

	US_DEBUGPX("eject cd: %s", "DEVICE MODE SWITCH\n");
	usb_stor_dev_init(us, msg, sizeof(msg), 0);
	return 0;
}

int usb_stor_eject_cd_resp(struct us_data *us)
{
	char msg[] = {
		0x55, 0x53, 0x42, 0x43, 0x12, 0x34, 0x56, 0x78,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 0x1b,
		0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
	};

	if (usb_stor_init_ignor(us)) {
		return 0;
	}

	US_DEBUGPX("eject cd: %s", "DEVICE MODE SWITCH\n");
	usb_stor_dev_init(us, msg, sizeof(msg), 1);
	return 0;
}

int usb_stor_tenda_w6(struct us_data *us)
{
	char msg1[] = {
		0x55, 0x53, 0x42, 0x43, 0xa0, 0xdc, 0xf8, 0x88,
		0x80, 0x80, 0x00, 0x00, 0x80, 0x01, 0x0a, 0x66,
		0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
	};
	char msg2[] = {
		0x55, 0x53, 0x42, 0x43, 0xa0, 0xdc, 0xf8, 0x88,
		0x80, 0x80, 0x00, 0x00, 0x80, 0x01, 0x0a, 0x77,
		0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
	};

	if (usb_stor_init_ignor(us)) {
		return 0;
	}

	US_DEBUGPX("Tenda W6: %s", "DEVICE MODE SWITCH\n");
	usb_stor_dev_init(us, msg1, sizeof(msg1), 1);
	usb_stor_dev_init(us, msg2, sizeof(msg2), 1);

	return 0;
}

int usb_stor_huawei1_init(struct us_data *us)
{
	char msg[] = {
		0x55, 0x53, 0x42, 0x43, 0x12, 0x34, 0x56, 0x78,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x11,
		0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
	};

	if (usb_stor_init_ignor(us)) {
		return 0;
	}

	US_DEBUGPX("Huawei1 3G Dongle: %s", "DEVICE MODE SWITCH\n");
	usb_stor_dev_init(us, msg, sizeof(msg), 0);
	return 0;
}

int usb_stor_huawei2_init(struct us_data *us)
{
	char msg[] = {
		0x55, 0x53, 0x42, 0x43, 0x12, 0x34, 0x56, 0x78,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 0x01,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
	};

	if (usb_stor_init_ignor(us)) {
		return 0;
	}

	US_DEBUGPX("Huawei2 3G Dongle: %s", "DEVICE MODE SWITCH\n");
	usb_stor_dev_init(us, msg, sizeof(msg), 0);
	return 0;
}

int usb_stor_huawei3_init(struct us_data *us)
{
	char msg[] = {
		0x55, 0x53, 0x42, 0x43, 0x12, 0x34, 0x56, 0x78,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x11,
		0x06, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
	};

	if (usb_stor_init_ignor(us)) {
		return 0;
	}

	US_DEBUGPX("Huawei3 3G Dongle: %s", "DEVICE MODE SWITCH\n");
	usb_stor_dev_init(us, msg, sizeof(msg), 0);
	return 0;
}
