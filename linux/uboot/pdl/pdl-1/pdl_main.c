#include <common.h>
#include "pdl_command.h"
#include "packet.h"
#include "cmd_defs.h"
#include "pdl_engine.h"
#include "pdl_debug.h"
#include "pdl_channel.h"

int pdl_main(void)
{
	pdl_info("start PDL1 main loop\n");
	pdl_usb_channel_register();
	pdl_init_packet_channel();

	pdl_cmd_register(CONNECT, sys_connect);
	pdl_cmd_register(START_DATA, data_start);
	pdl_cmd_register(MID_DATA, data_midst);
	pdl_cmd_register(END_DATA, data_end);
	pdl_cmd_register(EXEC_DATA, data_exec);
	pdl_cmd_register(NORMAL_RESET, reset_machine);
	pdl_cmd_register(GET_VERSION, get_pdl_version);
	pdl_cmd_register(GET_SECURITY, get_pdl_security);

#if 0
	serial_puts("wait for connect....\n");
	if (pdl_handle_connect(CONNECT_TIMEOUT) != 0){
		pdl_error("cannot handle connect\n");
		return -1;
	}
#endif
	pdl_send_rsp(ACK);
	pdl_handler(NULL);
	return 0;
}
