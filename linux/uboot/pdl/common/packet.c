//#define DEBUG

#include <asm/types.h>
#include <common.h>
#include "packet.h"
#include "pdl_channel.h"
#include <linux/string.h>
#include "pdl_debug.h"
#include "cmd_defs.h"
#include <asm/unaligned.h>

static enum {
    PKT_IDLE = 0,
    PKT_HEAD,
    PKT_DATA,
    PKT_ERROR
} pkt_state;

const static char *pdl_device_rsp[] = {
	[ACK] =	"device ack",
	[PACKET_ERROR] = "packet error",
	[INVALID_CMD] = "invalid cmd",
	[UNKNOWN_CMD] = "unknown cmd",
	[INVALID_ADDR] = "invalid address",
	[INVALID_BAUDRATE] = "invalid baudrate",
	[INVALID_PARTITION] = "invalid partition",
	[INVALID_SIZE] = "invalid size",
	[WAIT_TIMEOUT] = "wait timeout",
	[VERIFY_ERROR] = "verify error",
	[CHECKSUM_ERROR] = "checksum error",
	[OPERATION_FAILED] = "operation failed",
	[DEVICE_ERROR] = "device error",
	[NO_MEMORY] = "no memory",
	[DEVICE_INCOMPATIBLE] = "device incompatible",
	[HW_TEST_ERROR] = "hardware test error",
	[MD5_ERROR] = "md5 error",
	[ACK_AGAIN_ERASE] = "ack again erase",
	[ACK_AGAIN_FLASH] = "ack again flash",
	[MAX_RSP] = "max response",
};

static int pkt_error = 0;
static unsigned char cmd_buf[PDL_MAX_PKT_SIZE] __attribute__((__aligned__(32)));
static struct pdl_channel *channel;

int pdl_init_packet_channel(void)
{
	channel = pdl_get_channel();
	return 0;
}

/*
  * return value: 1,   get packet ok;
  *		0,   not a host packet;
  *		-1, something error, check pkt_error for true cause;
  *		-2, don't get any packet
  */
int pdl_get_cmd_packet(struct pdl_packet *pkt)
{
	u32 pkt_size = 0;
	u32 hdrsz = sizeof(struct packet_header);
	u32 cmd_hdrsz = sizeof(struct command_header);
	u32 bufsz = 0, totalsz = 0;
	int i, wait = 0;

	if (!pkt) {
		pdl_error("packet error\n");
		return -1;
	}

	if (!channel->tstc())
		return -2;

	/* looking for HOST_PACKET_TAG 0xAE */
	while (1) {
		bufsz = channel->tstc();
		if (!bufsz)
			return -2;
		channel->read(cmd_buf, bufsz);
		if (cmd_buf[0] == HOST_PACKET_TAG) {
			/*pdl_info("Found host packet tag, %u bytes\n", bufsz);*/
			break;
		} else {
			pdl_dbg("Drop a %u bytes packet:", bufsz);
			for(i = 0; i < 8; i++)
				pdl_dbg(" %02x", cmd_buf[i]);
			pdl_dbg("\n");
		}
	}
	totalsz = bufsz;

	/* is a valid packet header? */
	if (totalsz < hdrsz) {
		pdl_error("Invaild packet, it's too small (%u bytes)\n", totalsz);
		pkt_state = PKT_IDLE;
		return -2;
	}

	/* packet header check */
	pkt_state = PKT_HEAD;
	memcpy(&pkt_size, &cmd_buf[1], 4);
	if (cmd_buf[5] != HOST_PACKET_FLOWID ||
		pkt_size > PDL_MAX_PKT_SIZE ||
		pkt_size < sizeof(u32)) {
		pdl_error("Invalid packet, flowid 0x%x, pkt_size %u\n",
				cmd_buf[5], pkt_size);
		pdl_dbg("packet data:");
		for (i = 0; i < 8; i++)
			pdl_dbg(" %02x", cmd_buf[i]);
		pdl_dbg("\n");

		pkt_state = PKT_IDLE;
		return -2;
	}
	/*pdl_info("Got command packet, packet size %u bytes\n", pkt_size);*/

	/* get all data */
	if (pkt_size > totalsz - hdrsz) {
		wait = 0;
		while (1) {
			bufsz = channel->tstc();
			if (bufsz) {
				u32 last = pkt_size - (totalsz - hdrsz);
				bufsz = (bufsz > last) ? last : bufsz;
				channel->read(&cmd_buf[totalsz], bufsz);
				totalsz += bufsz;
				wait = 0;
				if (bufsz == last)
					break;
			} else {
				if (wait++ >= 1000) {
					pdl_error("wait packet data timeout, got %u bytes, need %u\n",
								totalsz - hdrsz, pkt_size);
					pkt_error = PACKET_ERROR;
					return -1;
				}
				udelay(1000);
			}
		}
	}

	pkt_state = PKT_DATA;
	memset(pkt, 0, sizeof(struct pdl_packet));
	if (pkt_size < cmd_hdrsz) {
		memcpy(pkt, &cmd_buf[hdrsz], pkt_size);
	} else {
		memcpy(pkt, &cmd_buf[hdrsz], cmd_hdrsz);
		pkt->data = &cmd_buf[hdrsz+cmd_hdrsz];

		/* packet data crc checking, if have 4 bytes crc in tail */
		if (pkt_size >= cmd_hdrsz + pkt->cmd_header.data_size + sizeof(u32)) {
			i = hdrsz + cmd_hdrsz + pkt->cmd_header.data_size;
			u32 crc = 0, crc_now = 0;
			memcpy(&crc, &cmd_buf[i], sizeof(u32));
			crc_now = crc32(0, pkt->data, pkt->cmd_header.data_size);
			if (crc != crc_now) {
				pdl_error("packet data crc verify failed, expect %#x, got %#x\n", crc, crc_now);
				pkt_error = CHECKSUM_ERROR;
				return -1;
			}
		}
	}
	return 1;
}


int pdl_get_connect_packet(struct pdl_packet *pkt, u32 timeout)
{
	char ch = 0;
	u8 header_buf[sizeof(struct packet_header)];
	u32 pkt_size = 0;
	u32 cnt = timeout; //timeout is in ms;

	while (cnt) {
		if(!channel->tstc()) {
			mdelay(1);
			//pdl_error("try to get\n");
			cnt--;
			continue;
		}

		ch = channel->getchar();
		if (ch == HOST_PACKET_TAG) {
			pkt_state = PKT_HEAD;
			break;
		} else {
			pdl_info("oops, get 0x%x\n", ch);
			continue;
		}
	}

	if (cnt == 0 ) {
		pkt_error = WAIT_TIMEOUT;
		pdl_error("get connect timeout\n");
		return -1;
	}
	header_buf[0] = ch;
	while (1) {
		u8 flowid;

		switch (pkt_state) {
		case PKT_IDLE:
			break;
		case PKT_HEAD:
			channel->read(&header_buf[1], 5);
			flowid = header_buf[5];
			pkt_size = 0;
			if (flowid == HOST_PACKET_FLOWID) {
				//ok, this is the header
				memcpy(&pkt_size, &header_buf[1], 4);
				pkt_size = le32_to_cpu(pkt_size);
				pkt_state = PKT_DATA;
				pdl_vdbg("header right data_size 0x%x\n", pkt_size);
			} else {
				pkt_state = PKT_IDLE;
				return 0;
			}
			break;
		case PKT_DATA:
			//diff the command data size and packet data size,check if it correct;
			if (pkt_size >  (sizeof(struct command_header) + 1)) {
				pkt_error = INVALID_SIZE;
				return -1;
			}
			channel->read(cmd_buf, pkt_size);
			memcpy(pkt, cmd_buf, sizeof(struct command_header));

			pdl_vdbg("pkt cmd type 0x%x data start 0x%x, size %x\n",
				pkt->cmd_header.cmd_type,
				pkt->cmd_header.data_addr,
				pkt->cmd_header.data_size);
			return 1;
		case PKT_ERROR:
			break;
		}
	}
	return 1;
}
/*
static void pdl_put_packet(struct packet *pkt)
{
}
*/
void pdl_put_cmd_packet(struct pdl_packet *pkt)
{
	memset(pkt, 0, sizeof (struct pdl_packet));
	pkt_state = PKT_IDLE;
	pkt_error = 0;
}

int pdl_get_packet_error(void)
{
	return pkt_error;
}

static int pdl_send_data(const u8 *data, u32 size, u8 flowid)
{
	struct packet_header *header = (struct packet_header *)cmd_buf;
	int hdr_sz = sizeof(struct packet_header);

	if(size > PDL_MAX_PKT_SIZE - hdr_sz) {
		pdl_error("packet size is too large.\n");
		return -1;
	}

	memset(cmd_buf, PDL_MAX_PKT_SIZE, 0);
	memcpy(&cmd_buf[hdr_sz], data, size);

	header->tag = PACKET_TAG;
	header->flowid = flowid;
	/* carefully, header->pkt_size may be unaligned */
	put_unaligned(size, &header->pkt_size); /* header->pkt_size = size; */

	channel->write(cmd_buf, hdr_sz + size);
	return 0;
}

void pdl_send_rsp(int rsp)
{
	/*
	 * if wait timeout, the client may be exit, we dont send
	 * this error, for the new client will get a extra error
	 * response
	*/
	if (rsp == WAIT_TIMEOUT)
		return;

	if (rsp != ACK) {
		if (rsp < ACK || rsp > MAX_RSP)
			rsp = DEVICE_ERROR;
		pdl_info("send rsp '%s'\n", pdl_device_rsp[rsp]);
	}

	pdl_send_data((const u8 *)&rsp, sizeof(int), (rsp == ACK) ? FLOWID_ACK : FLOWID_ERROR);
	pdl_dbg("send rsp '%s'\n", pdl_device_rsp[rsp]);
}

int pdl_send_pkt(const u8 *data, u32 size)
{
	return pdl_send_data(data, size, FLOWID_DATA);
}
