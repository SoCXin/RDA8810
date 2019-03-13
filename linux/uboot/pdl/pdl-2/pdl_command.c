#include <common.h>
#include <version.h>
#include <asm/types.h>
#include "packet.h"
#include <linux/string.h>
#include <malloc.h>
#include <asm/arch/mtdparts_def.h>
#include <asm/arch/rda_sys.h>
#include <asm/arch/rda_crypto.h>
#include <asm/arch/hw_test.h>
#include <asm/arch/prdinfo.h>
#include "cmd_defs.h"
#include "pdl_debug.h"
#include "pdl.h"
#include "pdl_command.h"
#include "pdl_nand.h"
#include "pdl_emmc.h"
#include <linux/mtd/mtd.h>

#define MAX_PART_NAME	30
#define MAX_TEST_LIST_SIZE		60

#ifdef CONFIG_PDL_FORCE_HW_TEST_FULL
static int hw_tested = 0;
static int hw_test_run(const char *test_list, int fast_mode);
#endif

uint8_t pdl_extended_status = 0;

static int media_type = MEDIA_NAND;
int sys_connect(struct pdl_packet *packet, void *arg)
{
	pdl_info("connect with pc\n");
	pdl_send_rsp(ACK);
	return 0;
}

struct exec_file_info {
	unsigned long start_address;
	unsigned long total_size;
	unsigned long recv_size;
	unsigned long next_address;
};
static struct exec_file_info exec_file;
static int frame_count = -1;

int data_start(struct pdl_packet *packet, void *arg)
{
	unsigned long start_addr = packet->cmd_header.data_addr;
	unsigned long file_size = packet->cmd_header.data_size;
	int ret;
	char part_name[MAX_PART_NAME + 1] = {'\0'};

	strncpy(part_name, (char *)packet->data, MAX_PART_NAME);
	frame_count = -1;

#ifdef CONFIG_PDL_FORCE_HW_TEST_FULL
	if(!hw_tested &&
		strstr(part_name, "system")) {
		ret = hw_test_run("all", 0);
		if(ret) {
			pdl_send_rsp(ret);
			return 0;
		}
		hw_tested = 1;
	}
#endif

	if (strlen(part_name) == 0) {
		pdl_info("invalid partition name\n");
		pdl_send_rsp(INVALID_PARTITION);
		return -1;
	}


	/* the image is pdl1.bin or pdl2.bin */
	memset(&exec_file, 0, sizeof(exec_file));
	if(strncmp(part_name, "pdl", 3) == 0 ||
		strncmp(part_name, "PDL", 3) == 0) {
		exec_file.start_address = start_addr;
		exec_file.total_size = file_size;
		exec_file.recv_size = 0;
		exec_file.next_address = start_addr;

		pdl_send_rsp(ACK);
		return 0;
	}

	if (media_type == MEDIA_MMC)
		ret = emmc_data_start(part_name, file_size);
	else
		ret = nand_data_start(part_name, file_size);
	if (ret) {
		pdl_send_rsp(ret);
		return -1;
	}

	pdl_send_rsp(ACK);
	return 0;
}

int data_midst(struct pdl_packet *packet, void *arg)
{
	u32 size = packet->cmd_header.data_size;
	int ret;
	u32 frame_num = packet->cmd_header.data_addr;

	/* pdl1.bin or pdl2.bin */
	if(exec_file.total_size > 0) {
		if ((exec_file.recv_size + size) > exec_file.total_size) {
			pdl_send_rsp(INVALID_SIZE);
			return 0;
		}

		memcpy((void *)exec_file.next_address, packet->data, size);
		exec_file.next_address += size;
		exec_file.recv_size += size;

		pdl_dbg("write to addr %lx, receive size %ld\n", exec_file.next_address,
					 exec_file.recv_size);

		pdl_send_rsp(ACK);
		return 0;
	}

	/* check frame number */
	if(frame_count >= 0) {
		if(frame_num < frame_count) { /* ignore this frame */
			pdl_send_rsp(ACK);
			return 0;
		}
		if(frame_num > frame_count) {
			pdl_send_rsp(PACKET_ERROR);
			pdl_error("expect frame %d, not %d\n", frame_count, frame_num);
			return -1;
		}
	} else {
		frame_count = frame_num;
	}

	pdl_dbg("frame count %u\n", frame_count);
	frame_count++;
	if (media_type == MEDIA_MMC)
		ret = emmc_data_midst(packet->data, size);
	else
		ret = nand_data_midst(packet->data, size);
	if (ret) {
		pdl_send_rsp(ret);
		return -1;
	}

	pdl_send_rsp(ACK);
	return 0;
}

int data_end(struct pdl_packet *packet, void *arg)
{
	int ret;
	uint32_t data_size = packet->cmd_header.data_size;
	uint32_t data_crc = 0;

	/* pdl1.bin or pdl2.bin */
	if(exec_file.total_size > 0) {
		pdl_info("receive pdl1/pdl2 finish\n");
		pdl_send_rsp(ACK);
		return 0;
	}

	frame_count = -1;
	if(data_size == 4)
		memcpy((void *)&data_crc, (void *)packet->data, 4);

	if (media_type == MEDIA_MMC)
		ret = emmc_data_end(data_crc);
	else
		ret = nand_data_end(data_crc);
	if (ret) {
		pdl_send_rsp(ret);
		return -1;
	}

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

int read_partition(struct pdl_packet *packet, void *arg)
{
	size_t size = packet->cmd_header.data_size;
	char part_name[MAX_PART_NAME + 1] = {'\0'};
	int ret;
	size_t actual_len = 0;
	unsigned char *data;

	strncpy(part_name, (char *)packet->data, MAX_PART_NAME);
	if (strlen(part_name) == 0) {
		pdl_info("invalid partition name\n");
		pdl_send_rsp(INVALID_PARTITION);
		return -1;
	}

	pdl_info("%s from %s\n", __func__, part_name);

	data = malloc(size);
	if (!data) {
		pdl_info("no memory!\n");
		pdl_send_rsp(NO_MEMORY);
		return -1;
	}

	if (media_type == MEDIA_MMC)
		ret = emmc_read_partition(part_name, data, size,
				&actual_len);
	else
		ret = nand_read_partition(part_name, data, size,
				&actual_len);
	if (ret) {
		pdl_send_rsp(ret);
		ret = -1;
		goto out;
	}

	pdl_send_pkt(data, actual_len);
	pdl_dbg("send ok\n");
out:
	free(data);
	return ret;
}

extern int mtdparts_ptbl_check(int *same);
extern int mmc_check_parts(int *same);
int check_partition_table(struct pdl_packet *packet, void *arg)
{
	int ret;
	int same = 0;
	unsigned char data;

	if (media_type == MEDIA_MMC)
		ret = mmc_check_parts(&same);
	else
		ret = mtdparts_ptbl_check(&same);

	if(ret) {
		pdl_send_rsp(DEVICE_ERROR);
		return -1;
	}

	data = (u8)(same & 0xff);
	pdl_send_pkt(&data, 1);
	return 0;
}

int read_partition_table(struct pdl_packet *packet, void *arg)
{
	u32 size;
	const char *parts = getenv("mtdparts");

	pdl_info("%s, parts %s\n", __func__, parts);

	size = strlen(parts);
	pdl_send_pkt((const u8*)parts, size);
	return 0;
}
extern struct mtd_device *current_mtd_dev;

int format_flash(struct pdl_packet *packet, void *arg)
{
	int ret;

	pdl_info("format the whole flash part\n");

	if (media_type == MEDIA_MMC)
		ret = emmc_format_flash();
	else
		ret = nand_format_flash();
	if (ret) {
		pdl_send_rsp(ret);
		return -1;
	}

#ifdef CONFIG_PDL_FORCE_HW_TEST_FULL
	if(!hw_tested) {
		ret = hw_test_run("all", 0);
		if(ret) {
			pdl_send_rsp(ret);
			return 0;
		}
		hw_tested = 1;
	}
#endif

	pdl_send_rsp(ACK);
	return 0;
}

int read_image_attr(struct pdl_packet *packet, void *arg)
{
	const char attrs[] = IMAGE_ATTR;

	pdl_info("fs image attrs: %s\n", attrs);

	pdl_send_pkt((const u8*)attrs, sizeof(attrs));
	return 0;
}

int erase_partition(struct pdl_packet *packet, void *arg)
{
	int ret;
	char part_name[MAX_PART_NAME + 1] = {'\0'};

	strncpy(part_name, (char *)packet->data, MAX_PART_NAME);

	if (strlen(part_name) == 0) {
		pdl_info("invalid partition name\n");
		pdl_send_rsp(INVALID_PARTITION);
		return -1;
	}

	if (media_type == MEDIA_MMC)
		ret = emmc_erase_partition(part_name);
	else
		ret = nand_erase_partition(part_name);
	if (ret) {
		pdl_send_rsp(ret);
		return -1;
	}

	/* if the part is 'userdata', erase 'fat' part also */
	if (strcmp(part_name, "userdata") == 0) {
		strcpy(part_name, "fat");
		if (media_type == MEDIA_MMC)
			emmc_erase_partition(part_name);
		else
			nand_erase_partition(part_name);
	}

	pdl_send_rsp(ACK);
	return 0;
}

int get_pdl_version(struct pdl_packet *packet, void *arg)
{
	u32 flag = packet->cmd_header.data_addr;
	u32 size;
#define MAX_VER_LEN	100
	char str[MAX_VER_LEN+1];

	memset(str, 0, MAX_VER_LEN + 1);
	strncpy(str, version_string, MAX_VER_LEN);

#ifdef BUILD_DISPLAY_ID
	/* return BUILD_DISPLAY_ID instead of pdl version */
	if(flag == 1)
		strncpy(str, BUILD_DISPLAY_ID, MAX_VER_LEN);
#endif

	pdl_info("send version string: [%s]\n", str);

	size = strlen(str) + 1;
	pdl_send_pkt((const u8*)str, size);
	return 0;
}

int reset_machine(struct pdl_packet *packet, void *arg)
{
	pdl_info("reset machine....\n");

	u8 reboot_mode = REBOOT_TO_NORMAL_MODE;
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

	/* should never here */
	return 0;
}

int poweroff_machine(struct pdl_packet *packet, void *arg)
{
	pdl_info("waiting USB cable plug out...\n");
	while(usb_cable_connected()){
		mdelay(20);
	}
	pdl_info("machine power off...\n");
	shutdown_system();
	return 0;
}

int recv_image_list(struct pdl_packet *packet, void *arg)
{
	pdl_info("receive download image list...\n");
	pdl_info("\t%s\n", (char *)packet->data);
	prdinfo_init((char *)packet->data);
	pdl_send_rsp(ACK);
	return 0;
}

static int hw_test_run(const char *test_list, int fast_mode)
{
	int __attribute__((unused)) test_all = 0;

	if(!test_list)
		return HW_TEST_ERROR;

	if(strcmp(test_list, "all") == 0)
		test_all = 1;

	pdl_info("start %s hardware test...[%s]\n", fast_mode ? "fast" : "full", test_list);

#ifdef CONFIG_VPU_STA_TEST
	if(test_all || strstr(test_list, "vpu")) {
		vpu_sta_test(fast_mode ? 10 : 200);
		pdl_info("VPU stable test..pass!\n");
	}
#endif

#ifdef CONFIG_VPU_MD5_TEST
	if(test_all || strstr(test_list, "vpu_md5")) {
		int ret = vpu_md5_test(fast_mode ? 10 : 200);
		if(ret)
			return MD5_ERROR;
		pdl_info("VPU MD5 test..pass!\n");
	}
#endif

#ifdef CONFIG_CPU_TEST
	if(test_all || strstr(test_list, "cpu")) {
		cpu_pll_test(fast_mode ? 2000 : 4000);
		pdl_info("CPU test..pass!\n");
	}
#endif

	pdl_info("end %s hardware test.\n", fast_mode ? "fast" : "full");
	return ACK;
}

int hw_test(struct pdl_packet *packet, void *arg)
{
	unsigned long param_size = packet->cmd_header.data_size;
	char test_list[MAX_TEST_LIST_SIZE + 1] = {'\0'};
	int ret = 0;

	if(param_size > 0)
		strncpy(test_list, (char *)packet->data, MAX_TEST_LIST_SIZE);
	else
		strcpy(test_list, "all");

	ret = hw_test_run(test_list, 0);
	pdl_send_rsp(ret);
	return 0;
}

int get_pdl_log(struct pdl_packet *packet, void *arg)
{
	u32 size;

	pdl_info("get pdl log\n");

	size = strlen(pdl_log_buff) + 1;
	pdl_send_pkt((const u8*)pdl_log_buff, size);
	return 0;
}

int download_finish(struct pdl_packet *packet, void *arg)
{
	u32 ftm_tag = packet->cmd_header.data_addr;
	pdl_info("download finish!\n");

	prdinfo_set_pdl_result(1, ftm_tag);
	pdl_send_rsp(ACK);
	return 0;
}

int set_pdl_dbg(struct pdl_packet *packet, void *arg)
{
	u32 dbg_settings = packet->cmd_header.data_addr;

	pdl_info("%s, dbg_settings :%x\n", __func__, dbg_settings);

	if (dbg_settings & PDL_DBG_PDL) {
		pdl_info("open pdl debug\n");
		pdl_dbg_pdl = 1;
	}

	if (dbg_settings & PDL_DBG_PDL_VERBOSE) {
		pdl_info("open pdl verbose debug\n");
		pdl_dbg_pdl = 1;
		pdl_vdbg_pdl = 1;
	}

	if (dbg_settings & PDL_DBG_USB_EP0) {
		pdl_info("open pdl usb ep0 debug\n");
		pdl_dbg_usb_ep0 = 1;
	}

	if (dbg_settings & PDL_DBG_RW_CHECK) {
		pdl_info("open pdl flash write/read/compare debug\n");
		pdl_dbg_rw_check = 1;
	}

	if (dbg_settings & PDL_DBG_USB_SERIAL) {
		pdl_info("open pdl usb serial debug\n");
		pdl_dbg_usb_serial = 1;
	}

	if (dbg_settings & PDL_DBG_FACTORY_PART) {
		pdl_info("open pdl factory part debug\n");
		pdl_dbg_factory_part = 1;
	}
	if (dbg_settings & PDL_EXTENDED_STATUS) {
		pdl_info("Enable extended status\n");
		pdl_extended_status = 1;
	}

	pdl_send_rsp(ACK);
	return 0;
}

int pdl_command_init(void)
{
	int ret;

	media_type = rda_media_get();
	if (media_type == MEDIA_MMC) {
		ret = pdl_emmc_init();
	} else {
		ret = pdl_nand_init();
	}

#ifdef CONFIG_PDL_FORCE_HW_TEST
	ret = hw_test_run("all", 1);
#endif

	return ret;
}
