#include <asm/types.h>
#include <common.h>
#include <command.h>
#include <asm/arch/rda_sys.h>
#include <malloc.h>
#include "pdl_command.h"
#include "packet.h"
#include "cmd_defs.h"
#include "pdl_engine.h"
#include "pdl_debug.h"
#include "pdl_channel.h"
#include <usb/usbserial.h>

int pdl_main(void)
{
	int ret = 0;
	pdl_info("start pdl2...\n");

	drv_usbser_init();
	pdl_usb_channel_register();
	pdl_init_packet_channel();
	ret = pdl_command_init();
	if(ret) {
		pdl_info("pdl init failed, power down..\n");
		pdl_send_rsp(ret);
		shutdown_system();
	}

	/* hack serial_puts(), to save all prints to log buffer */
	pdl_init_serial();

	pdl_cmd_register(CONNECT, sys_connect);
	pdl_cmd_register(START_DATA, data_start);
	pdl_cmd_register(MID_DATA, data_midst);
	pdl_cmd_register(END_DATA, data_end);
	pdl_cmd_register(EXEC_DATA, data_exec);
	pdl_cmd_register(READ_PARTITION, read_partition);
	pdl_cmd_register(READ_PARTITION_TABLE, read_partition_table);
	pdl_cmd_register(FORMAT_FLASH, format_flash);
	pdl_cmd_register(READ_IMAGE_ATTR, read_image_attr);
	pdl_cmd_register(ERASE_PARTITION, erase_partition);
	pdl_cmd_register(NORMAL_RESET, reset_machine);
	pdl_cmd_register(POWER_OFF, poweroff_machine);
	pdl_cmd_register(GET_VERSION, get_pdl_version);
	pdl_cmd_register(SET_PDL_DBG, set_pdl_dbg);
	pdl_cmd_register(CHECK_PARTITION_TABLE, check_partition_table);
	pdl_cmd_register(IMAGE_LIST, recv_image_list);
	pdl_cmd_register(HW_TEST, hw_test);
	pdl_cmd_register(GET_PDL_LOG, get_pdl_log);
	pdl_cmd_register(DOWNLOAD_FINISH, download_finish);

	pdl_send_rsp(ACK);
	pdl_handler(NULL);
	return 0;
}

#if defined(CONFIG_CMD_PDL2)
int do_pdl2(cmd_tbl_t * cmdtp, int flag, int argc, char *const argv[])
{
	pdl_main();
	return 0;
}

U_BOOT_CMD(pdl2, 1, 1, do_pdl2,
           "android fastboot protocol", "flash image to nand");
#endif
