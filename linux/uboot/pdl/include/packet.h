#ifndef _PACKET_H_
#define _PACKET_H_
#include "config.h"

/* host define, for receiving packet */
#define HOST_PACKET_TAG 0xAE
#define HOST_PACKET_FLOWID 0xFF

/* client define, for sending packet */
#define PACKET_TAG		0xAE
#define FLOWID_DATA		0xBB
#define FLOWID_ACK		0xFF
#define FLOWID_ERROR	0xEE

/*
 * for yaffs image, the packet length must be aligned to the (page_size+oob)
 */
#if defined(CONFIG_RDA_PDL2)
# define PDL_MAX_DATA_SIZE  270336  //(256*(1024 + 32)) == (64 * 4224)
#elif defined(CONFIG_RDA_PDL1)
# define PDL_MAX_DATA_SIZE (4*1024)
#else
#error "no valid pdl config"
#endif


struct packet_header {
	u8 tag;
	u32 pkt_size;
	u8 flowid;
}__attribute__((packed));

struct command_header {
	u32 cmd_type;
	u32 data_addr;
	u32 data_size;
};

#define PDL_MAX_PKT_SIZE (PDL_MAX_DATA_SIZE + \
		sizeof(struct command_header) + sizeof(struct packet_header))

struct pdl_packet {
	struct command_header cmd_header;
	u8 *data;
};


struct packet{
	struct packet_header pkt_header;
	struct pdl_packet *pdl_pkt;
	int state;
};

int  pdl_get_cmd_packet(struct pdl_packet *pkt);
void pdl_put_cmd_packet(struct pdl_packet *pkt);
int pdl_get_connect_packet(struct pdl_packet *pkt, u32 timeout);

void pdl_send_rsp(int rsp);
int pdl_send_pkt(const u8 *data, u32 size);
int pdl_get_packet_error(void);
int pdl_init_packet_channel(void);
#endif

