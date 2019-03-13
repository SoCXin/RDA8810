//#define DEBUG
#include <common.h>
#include <asm/types.h>
#include "cmd_defs.h"
#include <asm/arch/rda_sys.h>
#include "packet.h"
#include "pdl_engine.h"
#include "pdl_debug.h"

static struct pdl_cmd cmd_handler_table[MAX_CMD];

#define cmd_is_valid(cmd) ((cmd >= MIN_CMD) && (cmd < MAX_CMD))

static const char *pdl_commands[MAX_CMD] = {
	[CONNECT] = "connect",
	[ERASE_FLASH] = "erase flash",
	[ERASE_PARTITION] = "erase partition",
	[ERASE_ALL] = "erase all",
	[START_DATA] = "start data",
	[MID_DATA] = "midst data",
	[END_DATA] = "end data",
	[EXEC_DATA] = "execute data",
	[READ_FLASH] = "read flash",
	[READ_PARTITION] = "read partition",
	[NORMAL_RESET] = "normal reset",
	[READ_CHIPID] = "read chipid",
	[SET_BAUDRATE] = "set baudrate",
	[FORMAT_FLASH] = "format flash",
	[READ_PARTITION_TABLE] = "read partition table",
	[READ_IMAGE_ATTR] = "read image attribute",
	[GET_VERSION] = "get version",
	[SET_FACTMODE] = "set factory mode",
	[SET_CALIBMODE] = "set calibration mode",
	[SET_PDL_DBG] = "set pdl debuglevel",
	[CHECK_PARTITION_TABLE] = "check partition table",
	[POWER_OFF] = "power off",
	[IMAGE_LIST] = "download image list",
	[GET_SECURITY] = "get security capabilities",
	[HW_TEST] = "hardware test",
	[GET_PDL_LOG] = "get pdl log",
	[DOWNLOAD_FINISH] = "download finish",
};

int pdl_cmd_register(int cmd_type, pdl_cmd_handler handler)
{
	if (!cmd_is_valid(cmd_type))
		return -1;

	cmd_handler_table[cmd_type].handler = handler;
	return 0;
}

int pdl_handle_connect(u32 timeout)
{
	struct pdl_packet pkt;
	int cmd;
	int res;

	res = pdl_get_connect_packet(&pkt, timeout);
	if (res != 1) {
		return -1;
	}

	cmd = pkt.cmd_header.cmd_type;
	if (cmd == CONNECT)
		cmd_handler_table[cmd].handler(&pkt, 0);
	else
		goto err;

	pdl_put_cmd_packet(&pkt);
	return 0;
err:
	pdl_put_cmd_packet(&pkt);
	return -1;
}

#define CONNECT_TIMEOUT  180000	/*3 minutes*/
static void pdl_command_loop(void)
{
	struct pdl_packet pkt;
	int res;
	u32 cmd;
	int error;
	int cnt = CONNECT_TIMEOUT;
	int check_connect = 1;

	while (1) {
		res = pdl_get_cmd_packet(&pkt);
		if (res == -2) {
			if (!check_connect) {
				mdelay(1);
				continue;
			} else if (--cnt > 0) {
				mdelay(1);
				continue;
			}

			if (cnt <= 0) {
				pdl_error("PDL get command timeout...\n");
				break;
			}
		}
		if (check_connect)
			check_connect = 0;

		if (res == 0)
			continue;

		//if error send response to pc.
		if (res == -1) {
			error = pdl_get_packet_error();
			pdl_send_rsp(error);
			pdl_put_cmd_packet(&pkt);
			continue;
		}
		cmd = pkt.cmd_header.cmd_type;

		if (!cmd_is_valid(cmd)) {
			pdl_send_rsp(INVALID_CMD);
			pdl_put_cmd_packet(&pkt);
			continue;
		}

		pdl_dbg("get and exec cmd '%s'\n", pdl_commands[cmd]);

		if (cmd_handler_table[cmd].handler)
			res = cmd_handler_table[cmd].handler(&pkt, 0);
		pdl_put_cmd_packet(&pkt);
	}

	pdl_info("shutdown machine...\n");
	shutdown_system();
}

int pdl_handler(void *arg)
{
	for (;;) {
		pdl_command_loop();
	}
	return 0;
}
