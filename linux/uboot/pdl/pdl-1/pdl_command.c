#include <common.h>
#include "packet.h"
#include <linux/string.h>
#include "cmd_defs.h"
#include "pdl_debug.h"
#include <asm/arch/rda_sys.h>
#include <asm/arch/rda_crypto.h>

struct dl_file_info {
	unsigned long start_address;
	unsigned long total_size;
	unsigned long recv_size;
	unsigned long next_address;
};

static struct dl_file_info exec_file;


int sys_connect(struct pdl_packet *packet, void *arg)
{
	pdl_info("connect with pc\n");
	pdl_send_rsp(ACK);
	return 0;
}

int data_start(struct pdl_packet *packet, void *arg)
{
	unsigned long start_addr = packet->cmd_header.data_addr;
	unsigned long file_size = packet->cmd_header.data_size;

#if 0
	if ((start_addr < MEMORY_START) || (start_addr >= MEMORY_START + MEMORY_SIZE)) {
		pdl_send_rsp(INVALID_ADDR);
		return 0;
	}

	if ((start_addr + file_size) > (MEMORY_START + MEMORY_SIZE)) {
		pdl_send_rsp(INVALID_SIZE);
		return 0;
	}
#endif
	pdl_info("start addr %lx, file size %lx\n", start_addr, file_size);

	exec_file.start_address = start_addr;
	exec_file.total_size = file_size;
	exec_file.recv_size = 0;
	exec_file.next_address = start_addr;

//	memset((void*)start_addr, 0, file_size);
	pdl_send_rsp(ACK);
	return 0;
}

int data_midst(struct pdl_packet *packet, void *arg)
{
	short data_len = packet->cmd_header.data_size;

	if ((exec_file.recv_size + data_len) > exec_file.total_size) {
		pdl_send_rsp(INVALID_SIZE);
		return 0;
	}

	memcpy((void *)exec_file.next_address, packet->data, data_len);
	exec_file.next_address += data_len;
	exec_file.recv_size += data_len;

	pdl_dbg("write to addr %lx, receive size %ld\n", exec_file.next_address,
				 exec_file.recv_size);
	pdl_send_rsp(ACK);
	return 0;
}

int data_end(struct pdl_packet *packet, void *arg)
{
	pdl_info("receive pdl2 finish\n");
	pdl_send_rsp(ACK);
	return 0;
}

static unsigned long
do_go_exec (ulong (*entry)(int, char * const []), int argc, char * const argv[])
{
        return entry (argc, argv);
}

int data_exec(struct pdl_packet *packet, void *arg)
{
#ifdef  CONFIG_SIGNATURE_CHECK_IMAGE
	pdl_info("Verify executable 0x%lx, #%ld\n", 
	         exec_file.start_address,exec_file.recv_size);
	if (image_sign_verify((const void *)exec_file.start_address,
	                      exec_file.recv_size)) {
		printf("Verify failed, aborting execute.\n");
		pdl_send_rsp(VERIFY_ERROR);
		return 0;
	}
#endif
	pdl_info("Execute download from 0x%lx\n", exec_file.start_address);
	do_go_exec((void *)exec_file.start_address, 1, NULL);
	return 0;
}

int get_pdl_version(struct pdl_packet *packet, void *arg)
{
	static const unsigned char pdl1_version[] = "PDL1";

	pdl_info("get pdl version\n");

	pdl_send_pkt(pdl1_version, sizeof(pdl1_version));
	return 0;
}

#include <asm/arch/rom_api_trampolin.h>

int get_pdl_security(struct pdl_packet *packet, void *arg)
{
	struct {
		char buffer[16];
		struct chip_id chip_id;
		struct chip_security_context chip_security_context;
		struct chip_unique_id chip_unique_id;
		struct pubkey pubkey;
		u8     random[32];
	} response;

	pdl_info("get pdl security\n");

	memset(&response, 0, sizeof(response));

	if (memcmp(romapi->magic, "RDA API", 8) == 0) {
		int n;
		get_chip_id(&response.chip_id);
		get_chip_unique(&response.chip_unique_id);
		n = get_chip_security_context(&response.chip_security_context,
		                              &response.pubkey);
		get_chip_true_random(response.random, 32);
		sprintf(response.buffer, "RDASEC: v%d/%d", romapi->version, n);
	}
	else
		sprintf(response.buffer, "RDASEC: none");

	pdl_send_pkt((const u8 *)&response, sizeof(response));

	return 0;
}


int reset_machine(struct pdl_packet *packet, void *arg)
{
	u8 reboot_mode = REBOOT_TO_NORMAL_MODE;

	pdl_info("reset machine....\n");

	if (packet->cmd_header.data_size == 1) {
		reboot_mode = packet->data[0];
	}
	switch (reboot_mode) {
		case REBOOT_TO_NORMAL_MODE:
		case REBOOT_TO_DOWNLOAD_MODE:
		case REBOOT_TO_FASTBOOT_MODE:
		case REBOOT_TO_RECOVERY_MODE:
		case REBOOT_TO_CALIB_MODE:
			break;
		default:
			reboot_mode = REBOOT_TO_NORMAL_MODE;
	}

	pdl_info("reboot_mode: %d\n", reboot_mode);
	pdl_send_rsp(ACK); // cheat and send ack before reset

	rda_reboot(reboot_mode);

	/*should never here */
	return 0;
}

