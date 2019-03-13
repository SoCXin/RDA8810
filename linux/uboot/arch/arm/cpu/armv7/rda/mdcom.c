#include "common.h"
#include <errno.h>

#include <asm/io.h>
#include <asm/arch/hardware.h>
#include <asm/arch/reg_sysctrl.h>
#include <asm/arch/reg_mdcom.h>
#include <asm/arch/reg_xcpu.h>
#include <asm/arch/defs_mdcom.h>
#include <asm/arch/mdcom.h>
#include <asm/arch/factory.h>
#include <asm/arch/rda_iomap.h>
#include <asm/arch/reg_cfg_regs.h>


static u32 modem_logic_base_addr = 0x02000000;
void rda_mdcom_set_logic_base_addr(u32 base)
{
	modem_logic_base_addr = base & 0xffff0000;
	printf("## Set modem logic base address %#x\n", modem_logic_base_addr);
}

#define RDA_MODEM_IN_DDR(addr, modem_logic_base_addr)	\
	(((u32)(addr) & 0x0F000000) == (modem_logic_base_addr & 0x0F000000))

/*
 * Convert a modem address to an AP address
 */
u32 rda_mdcom_address_modem2ap(u32 addr)
{
#if defined(_TGT_MODEM_MEM_SIZE) && (_TGT_MODEM_MEM_SIZE > 0)
	if (RDA_MODEM_IN_DDR(addr, modem_logic_base_addr))
		return (RDA_MODEM_RAM_BASE - modem_logic_base_addr + addr);
	else
		return RDA_ADD_M2A(addr);
#else
	return RDA_ADD_M2A(addr);
#endif
}

/*
 * Setup modem running env
 */
int rda_mdcom_setup_run_env(u32 pc, u32 param)
{
	RDA_BOOT_HST_MONITOR_X_CTX_T *ctx =
		(RDA_BOOT_HST_MONITOR_X_CTX_T *)RDA_BOOT_CTX_ADD;
	ctx->cmdType = 0xFF;
	ctx->pc = pc;
	ctx->sp = 0;
	ctx->param = (void *)param;
	ctx->returnedValue = (void *)0;

#if defined(_TGT_MODEM_MEM_SIZE) && (_TGT_MODEM_MEM_SIZE > 0)
	// Set DDR access offset
	if (RDA_MODEM_IN_DDR(pc, modem_logic_base_addr)) {
#ifdef CONFIG_MACH_RDA8850E
		u32 offs = RDA_MODEM_RAM_BASE - modem_logic_base_addr;

		/* 0x1000000 alignment */
		if(offs & 0xffffff) {
			printf("\nERROR: modem memory map offset %#x is not aligned to 0x1000000.\n", offs);
			printf("ERROR: modem cannot work, abort.\n");
			return -1;
		}

		hwp_configRegs->Mem_mode_Sel = 0;
		hwp_configRegs->H2X_DDR_Offset = offs;
		hwp_configRegs->H2X_WD_DDR_Offset = offs;
#else
		u32 offs = CFG_REGS_H2X_DDR_OFFSET((RDA_MODEM_RAM_BASE - modem_logic_base_addr) >> 24);
		hwp_configRegs->H2X_DDR_Offset = offs;
#endif
	};
#endif

	return 0;
}

/*
 * Modem calibration data section address
 */
void rda_mdcom_get_calib_section(u32 *addr, u32 *len)
{
	if (addr)
		*addr = RDA_MODEM_CAL_ADDR;
	if (len)
		*len = RDA_MODEM_CAL_LEN;
}

/*
 * Modem extended calibration data section address
 */
void rda_mdcom_get_ext_calib_section(u32 *addr, u32 *len)
{
	if (addr)
		*addr = RDA_MODEM_EXT_CAL_ADDR;
	if (len)
		*len = RDA_MODEM_EXT_CAL_LEN;
}

/*
 * Modem factory section address
 */
void rda_mdcom_get_factory_section(u32 *addr, u32 *len)
{
	if (addr)
		*addr = RDA_MODEM_FACT_ADDR;
	if (len)
		*len = RDA_MODEM_FACT_LEN;
}

/*
 * AP factory section address
 */
void rda_mdcom_get_ap_factory_section(u32 *addr, u32 *len)
{
	if (addr)
		*addr = RDA_AP_FACT_ADDR;
	if (len)
		*len = RDA_AP_FACT_LEN;
}

/*
 * Reset cause
 */
u32 rda_mdcom_get_reset_cause(void)
{
	return ((RDA_AP_MBX_HEARTBEAT_T *)RDA_AP_MBX_HEARTBEAT_ADD)->resetCause;
}

/*
 * Interface version
 */
u32 rda_mdcom_get_interface_version(void)
{
	return ((RDA_AP_MBX_HEARTBEAT_T *)RDA_AP_MBX_HEARTBEAT_ADD)->version;
}

/*
 * Magic number
 */
int rda_mdcom_system_started_before(void)
{
	RDA_AP_MBX_MAGIC_NUMBER_T *magic =
		(RDA_AP_MBX_MAGIC_NUMBER_T *)RDA_AP_MBX_MAGIC_NUMBER_ADD;
	return magic->sysStarted == RDA_MAGIC_SYSTEM_STARTED_FLAG;
}

void rda_mdcom_set_system_started_flag(void)
{
	RDA_AP_MBX_MAGIC_NUMBER_T *magic =
		(RDA_AP_MBX_MAGIC_NUMBER_T *)RDA_AP_MBX_MAGIC_NUMBER_ADD;
	magic->sysStarted = RDA_MAGIC_SYSTEM_STARTED_FLAG;
}

int rda_mdcom_modem_crashed_before(void)
{
	RDA_AP_MBX_MAGIC_NUMBER_T *magic =
		(RDA_AP_MBX_MAGIC_NUMBER_T *)RDA_AP_MBX_MAGIC_NUMBER_ADD;
	return magic->modemCrashed == RDA_MAGIC_MODEM_CRASH_FLAG;
}

int rda_mdcom_calib_update_cmd_valid(void)
{
	RDA_AP_MBX_MAGIC_NUMBER_T *magic =
		(RDA_AP_MBX_MAGIC_NUMBER_T *)RDA_AP_MBX_MAGIC_NUMBER_ADD;
	if (magic->factUpdateCmd == RDA_MAGIC_FACT_UPD_CMD_FLAG &&
			(magic->factUpdateType &
			 RDA_MAGIC_FACT_UPD_TYPE_FLAG_MASK) ==
			RDA_MAGIC_FACT_UPD_TYPE_FLAG &&
			(magic->factUpdateType & RDA_MAGIC_FACT_UPD_TYPE_CALIB))
		return 1;
	else
		return 0;
}

int rda_mdcom_factory_update_cmd_valid(void)
{
	RDA_AP_MBX_MAGIC_NUMBER_T *magic =
		(RDA_AP_MBX_MAGIC_NUMBER_T *)RDA_AP_MBX_MAGIC_NUMBER_ADD;
	if (magic->factUpdateCmd == RDA_MAGIC_FACT_UPD_CMD_FLAG &&
			(magic->factUpdateType &
			 RDA_MAGIC_FACT_UPD_TYPE_FLAG_MASK) ==
			RDA_MAGIC_FACT_UPD_TYPE_FLAG &&
			(magic->factUpdateType & RDA_MAGIC_FACT_UPD_TYPE_FACT))
		return 1;
	else
		return 0;
}

int rda_mdcom_ap_factory_update_cmd_valid(void)
{
	RDA_AP_MBX_MAGIC_NUMBER_T *magic =
		(RDA_AP_MBX_MAGIC_NUMBER_T *)RDA_AP_MBX_MAGIC_NUMBER_ADD;
	if (magic->factUpdateCmd == RDA_MAGIC_FACT_UPD_CMD_FLAG &&
			(magic->factUpdateType &
			 RDA_MAGIC_FACT_UPD_TYPE_FLAG_MASK) ==
			RDA_MAGIC_FACT_UPD_TYPE_FLAG &&
			(magic->factUpdateType & RDA_MAGIC_FACT_UPD_TYPE_AP_FACT))
		return 1;
	else
		return 0;
}

/*
 * Modem log info
 */
static int rda_mdcom_modem_address_valid(u32 addr)
{
	if ((addr >= RDA_MODEM_INTSRAM_BASE &&
				addr < RDA_MODEM_INTSRAM_END) ||
			(addr >= RDA_MODEM_RAM_BASE &&
			 addr < RDA_MODEM_RAM_END)) {
		return 1;
	}
	return 0;
}

void rda_mdcom_init_all_log_info(void)
{
	/* Init log info */
	RDA_AP_MBX_LOG_BUF_INFO_T *log =
		(RDA_AP_MBX_LOG_BUF_INFO_T *)RDA_AP_MBX_LOG_BUF_INFO_ADD;
	memset(log, 0, sizeof(*log));
	/* Init modem crash flag */
	RDA_AP_MBX_MAGIC_NUMBER_T *magic =
		(RDA_AP_MBX_MAGIC_NUMBER_T *)RDA_AP_MBX_MAGIC_NUMBER_ADD;
	magic->modemCrashed = 0;
	magic->factUpdateCmd = 0;
	magic->factUpdateType = 0;
	/* Init heartbeat structure members */
	memset((RDA_AP_MBX_HEARTBEAT_T *)RDA_AP_MBX_HEARTBEAT_ADD, 0,
			sizeof(RDA_AP_MBX_HEARTBEAT_T));
}

void rda_mdcom_get_modem_log_info(u32 *addr, u32 *len)
{
	RDA_AP_MBX_LOG_BUF_INFO_T *log =
		(RDA_AP_MBX_LOG_BUF_INFO_T *)RDA_AP_MBX_LOG_BUF_INFO_ADD;
	u32 log_addr = rda_mdcom_address_modem2ap(log->modemAddr);
	u32 log_len = log->modemLen;

	/* Check whether the address is valid */
	if (!rda_mdcom_modem_address_valid(log_addr) ||
			log_len > RDA_AP_MBX_MAX_MODEM_LOG_LEN) {
		log_addr = 0;
		log_len = 0;
	}

	if (addr)
		*addr = log_addr;
	if (len)
		*len = log_len;
}

void rda_mdcom_get_modem_exception_info(u32 *addr, u32 *len)
{
	RDA_AP_MBX_LOG_BUF_INFO_T *log =
		(RDA_AP_MBX_LOG_BUF_INFO_T *)RDA_AP_MBX_LOG_BUF_INFO_ADD;
	u32 exc_addr = RDA_ADD_M2A(log->modemExcAddr);
	u32 exc_len = log->modemExcLen;

	/* Check whether the address is valid */
	if (!rda_mdcom_modem_address_valid(exc_addr) ||
			exc_len > RDA_AP_MBX_MAX_MODEM_EXC_LEN) {
		exc_addr = 0;
		exc_len = 0;
	}

	if (addr)
		*addr = exc_addr;
	if (len)
		*len = exc_len;
}

/*
 * Modem XCPU info
 */
void rda_mdcom_show_xcpu_info(void)
{
	printf("\n");
	printf("Modem XCPU info:\n");
	printf("----------------\n");
	printf("Cause = 0x%08x\n", hwp_xcpu->cp0_Cause);
	printf("Status = 0x%08x\n", hwp_xcpu->cp0_Status);
	printf("BadAddr = 0x%08x\n", hwp_xcpu->cp0_BadVAddr);
	printf("EPC = 0x%08x\n", hwp_xcpu->cp0_EPC);
	printf("RA = 0x%08x\n", hwp_xcpu->Regfile_RA);
	printf("PC = 0x%08x\n", hwp_xcpu->rf0_addr);
	printf("----------------\n");
}

/*
 * Modem software version info
 */
void rda_mdcom_show_software_version(void)
{
	int i = 0;
	char buf[256];
	char *str;
	RDA_MODEM_MAP_VERSION_T *version;
	RDA_MODEM_MAP_MODULE_T *map;
	RDA_MODEM_MAP_MODULE_T **ptr =
		(RDA_MODEM_MAP_MODULE_T **)RDA_MODEM_MAP_PTR;

	printf("\n");
	map = (RDA_MODEM_MAP_MODULE_T *)rda_mdcom_address_modem2ap(
			(u32)*ptr);
	if (!ptr || !rda_mdcom_modem_address_valid((u32)map)) {
		printf("No modem software versions\n");
		return;
	}

	printf("Modem software versions:\n");
	printf("------------------------\n");
	for (i = 0; i < RDA_MODEM_MAP_QTY; i++, map++) {
		if (!map->version)
			continue;
		version = (RDA_MODEM_MAP_VERSION_T *)
			rda_mdcom_address_modem2ap((u32)map->version);
		if (!rda_mdcom_modem_address_valid((u32)version))
			continue;
		printf("[%02d]\n", i);
		printf("revision = %d / 0x %07x\n",
				version->revision,
				version->revision);
		printf("number = %d\n", version->number);
		printf("date = %d\n", version->date);
		str = (char *)rda_mdcom_address_modem2ap((u32)version->string);
		if (version->string &&
				rda_mdcom_modem_address_valid((u32)str)) {
			snprintf(buf, sizeof(buf), "%s", str);
			buf[sizeof(buf) - 1] = '\0';
			printf("string = %s\n", buf);
		} else {
			printf("string = <n/a>\n");
		}
	}
	printf("------------------------\n");
}

/*
 * Communication channels
 */
static struct rda_mdcom_channel rda_rda_mdcom_chn[] = {
	/* AT command channel */
	{
		RDA_MDCOM_LINE_AT_CMD,
		RDA_MDCOM_LINE_AT_CMD_FC,
		(struct rda_mdcom_channel_head*)RDA_MDCOM_CHN_AT_HEAD_ADD_READ,
		(void *)RDA_MDCOM_CHN_AT_BUF_ADD_READ,
		RDA_MDCOM_CHN_AT_BUF_LEN_READ-1,
		(struct rda_mdcom_channel_head*)RDA_MDCOM_CHN_AT_HEAD_ADD_WRITE,
		(void *)RDA_MDCOM_CHN_AT_BUF_ADD_WRITE,
		RDA_MDCOM_CHN_AT_BUF_LEN_WRITE-1
	},

	/* SYSTEM command channel */
	{
		RDA_MDCOM_LINE_SYSTEM,
		RDA_MDCOM_LINE_SYSTEM_FC,
		(struct rda_mdcom_channel_head*)RDA_MDCOM_CHN_SYS_HEAD_ADD_READ,
		(void *)RDA_MDCOM_CHN_SYS_BUF_ADD_READ,
		RDA_MDCOM_CHN_SYS_BUF_LEN_READ-1,
		(struct rda_mdcom_channel_head*)RDA_MDCOM_CHN_SYS_HEAD_ADD_WRITE,
		(void *)RDA_MDCOM_CHN_SYS_BUF_ADD_WRITE,
		RDA_MDCOM_CHN_SYS_BUF_LEN_WRITE-1
	},

	/* TRACE communciation channel */
	{
		RDA_MDCOM_LINE_TRACE,
		RDA_MDCOM_LINE_TRACE_FC,
		(struct rda_mdcom_channel_head*)RDA_MDCOM_CHN_TRACE_HEAD_ADD_READ,
		(void *)RDA_MDCOM_CHN_TRACE_BUF_ADD_READ,
		RDA_MDCOM_CHN_TRACE_BUF_LEN_READ-1,
		(struct rda_mdcom_channel_head*)RDA_MDCOM_CHN_TRACE_HEAD_ADD_WRITE,
		(void *)RDA_MDCOM_CHN_TRACE_BUF_ADD_WRITE,
		RDA_MDCOM_CHN_TRACE_BUF_LEN_WRITE-1
	}
};

int rda_mdcom_init_port(int port_id)
{
	switch (port_id) {
	case RDA_MDCOM_PORT0:
		hwp_mdComregs->Mask_Set = COMREGS_IRQ0_MASK_SET(0);
		hwp_mdComregs->Mask_Clr = COMREGS_IRQ0_MASK_CLR(0xFF);
		hwp_mdComregs->ItReg_Set = COMREGS_IRQ0_SET(0);
		hwp_mdComregs->ItReg_Clr = COMREGS_IRQ0_CLR(0xFF);
		return 0;

	case RDA_MDCOM_PORT1:
		hwp_mdComregs->Mask_Set = COMREGS_IRQ1_MASK_SET(0);
		hwp_mdComregs->Mask_Clr = COMREGS_IRQ1_MASK_CLR(0xFF);
		hwp_mdComregs->ItReg_Set = COMREGS_IRQ1_SET(0);
		hwp_mdComregs->ItReg_Clr = COMREGS_IRQ1_CLR(0xFF);
		return 0;

	default:
		return -EINVAL;
	}
}

int rda_mdcom_line_set(int port_id, int line_id)
{
	if (line_id & ~0x7) {
		return -EINVAL;
	}

	switch (port_id) {
	case RDA_MDCOM_PORT0:
		hwp_mdComregs->ItReg_Set = COMREGS_IRQ0_SET(1 << line_id);
		return 0;

	case RDA_MDCOM_PORT1:
		hwp_mdComregs->ItReg_Set = COMREGS_IRQ1_SET(1 << line_id);
		return 0;

	default:
		return -EINVAL;
	}
}

int rda_mdcom_line_clear(int port_id, int line_id)
{
	if (line_id & ~0x7) {
		return -EINVAL;
	}

	switch (port_id) {
	case RDA_MDCOM_PORT0:
		hwp_mdComregs->ItReg_Clr = COMREGS_IRQ0_CLR(1 << line_id);
		return 0;

	case RDA_MDCOM_PORT1:
		hwp_mdComregs->ItReg_Clr = COMREGS_IRQ0_CLR(1 << line_id);
		return 0;

	default:
		return -EINVAL;
	}
}

int rda_mdcom_line_set_check(int port_id, int line_id)
{
	if (line_id & ~0x7) {
		return 0;
	}

	switch (port_id) {
	case RDA_MDCOM_PORT0:
		return !!(hwp_mdComregs->ItReg_Clr & COMREGS_IRQ0_CLR(1 << line_id));

	case RDA_MDCOM_PORT1:
		return !!(hwp_mdComregs->ItReg_Clr & COMREGS_IRQ1_CLR(1 << line_id));

	default:
		return 0;
	}
}

int rda_mdcom_line_clear_check(int port_id, int line_id)
{
	if (line_id & ~0x7) {
		return 0;
	}

	switch (port_id) {
	case RDA_MDCOM_PORT0:
		return !(hwp_mdComregs->ItReg_Set & COMREGS_IRQ0_SET(1 << line_id));

	case RDA_MDCOM_PORT1:
		return !(hwp_mdComregs->ItReg_Set & COMREGS_IRQ0_SET(1 << line_id));

	default:
		return 0;
	}
}

int rda_mdcom_line_set_wait(int port_id, int line_id, int waittime)
{
	unsigned long end_time = get_ticks() + waittime;

	if (line_id & ~0x7) {
		return 0;
	}

	switch (port_id) {
	case RDA_MDCOM_PORT0:
		while (!(hwp_mdComregs->ItReg_Clr & COMREGS_IRQ0_CLR(1 << line_id))) {
			if ((waittime >= 0) && (end_time <= get_ticks()))
				return 0;
		}
		return 1;

	case RDA_MDCOM_PORT1:
		while (!(hwp_mdComregs->ItReg_Clr & COMREGS_IRQ1_CLR(1 << line_id))) {
			if ((waittime >= 0) && (end_time <= get_ticks()))
				return 0;
		}
		return 1;

	default:
		return 0;
	}
}

int rda_mdcom_line_clear_wait(int port_id, int line_id, int waittime)
{
	unsigned long end_time = get_ticks() + waittime;

	if (line_id & ~0x7) {
		return 0;
	}

	switch (port_id) {
	case RDA_MDCOM_PORT0:
		while (hwp_mdComregs->ItReg_Set & COMREGS_IRQ0_SET(1 << line_id)) {
			if ((waittime >= 0) && (end_time <= get_ticks()))
				return 0;
		}
		return 1;

	case RDA_MDCOM_PORT1:
		while (hwp_mdComregs->ItReg_Set & COMREGS_IRQ0_SET(1 << line_id)) {
			if ((waittime >= 0) && (end_time <= get_ticks()))
				return 0;
		}
		return 1;

	default:
		return 0;
	}
}

void rda_mdcom_port_show(void)
{
	printf("RDA MDCOM ports stauts:\n");
	printf("REGISTER Cause     = 0x%.16x\n", hwp_mdComregs->Cause);
	printf("REGISTER Mask_Set  = 0x%.16x\n", hwp_mdComregs->Mask_Set);
	printf("REGISTER Mask_Clr  = 0x%.16x\n", hwp_mdComregs->Mask_Clr);
	printf("REGISTER ItReg_Set = 0x%.16x\n", hwp_mdComregs->ItReg_Set);
	printf("REGISTER ItReg_Clr = 0x%.16x\n", hwp_mdComregs->ItReg_Clr);
	printf("RDA MDCOM ports stauts end.\n");
}

int rda_mdcom_channel_init(const unsigned int channel)
{
	if (channel >= sizeof(rda_rda_mdcom_chn) / sizeof(struct rda_mdcom_channel)) {
		return -EINVAL;
	}

	memset(rda_rda_mdcom_chn[channel].read_buf_head, 0,
	       sizeof(struct rda_mdcom_channel_head));
	memset(rda_rda_mdcom_chn[channel].write_buf_head, 0,
	       sizeof(struct rda_mdcom_channel_head));
	return 0;
}

int rda_mdcom_channel_all_init(void)
{
	return rda_mdcom_channel_init(RDA_MDCOM_CHANNEL_AT) ||
		rda_mdcom_channel_init(RDA_MDCOM_CHANNEL_SYSTEM) ||
		rda_mdcom_channel_init(RDA_MDCOM_CHANNEL_TRACE);
}

int rda_mdcom_channel_buf_send_stream(const unsigned int channel, void *buf,
				      int size, int waittime)
{
	unsigned long end_time = get_ticks() + waittime;
	struct rda_mdcom_channel* channel_ptr;
	struct rda_mdcom_channel_head* channel_head_ptr;
	int buffer_size_mask;
	int count = 0;

	if (channel >= sizeof(rda_rda_mdcom_chn) / sizeof(struct rda_mdcom_channel)) {
		return -EINVAL;
	}

	if (size < 0) {
		return -EINVAL;
	}

	channel_ptr = &rda_rda_mdcom_chn[channel];
	channel_head_ptr = channel_ptr->write_buf_head;
	buffer_size_mask = channel_ptr->write_buf_size_mask;

	while (1) {
		int write_offset = channel_head_ptr->write_offset;
		int remain_len = (buffer_size_mask + 1) -
				 ((write_offset - channel_head_ptr->read_offset) & buffer_size_mask);

		if (remain_len > buffer_size_mask + 1 - write_offset) {
			remain_len = buffer_size_mask + 1 - write_offset;
		}

		if (remain_len) {
			if (remain_len > size) {
				remain_len = size;
			}

			memcpy(((char*)channel_ptr->write_buf) + write_offset, buf, remain_len);
			channel_head_ptr->write_offset = (write_offset + remain_len) & buffer_size_mask;
			size -= remain_len;
			buf = (char*)buf + remain_len;
			count += remain_len;

			if (!size) {
				return count;
			}
		} else if ((waittime >= 0) && (end_time <= get_ticks())) {
			return count;
		}
	}
}

int rda_mdcom_channel_buf_send_dgram(const unsigned int channel, void *buf,
				     int size, int waittime)
{
	unsigned long end_time = get_ticks() + waittime;
	struct rda_mdcom_channel* channel_ptr;
	struct rda_mdcom_channel_head* channel_head_ptr;
	int buffer_size_mask, write_offset;

	if (channel >= sizeof(rda_rda_mdcom_chn) / sizeof(struct rda_mdcom_channel)) {
		return -EINVAL;
	}

	if (size < 0) {
		return -EINVAL;
	}

	channel_ptr = &rda_rda_mdcom_chn[channel];
	channel_head_ptr = channel_ptr->write_buf_head;
	buffer_size_mask = channel_ptr->write_buf_size_mask;
	write_offset = ALIGN(channel_head_ptr->write_offset, 4) & buffer_size_mask;

	while (1) {
		int remain_len = (buffer_size_mask + 1) -
				 ((write_offset - channel_head_ptr->read_offset) & buffer_size_mask);

		if (remain_len >= size) {
			remain_len = buffer_size_mask + 1 - write_offset;

			if (remain_len > size) {
				remain_len = size;
			}

			memcpy(((char*)channel_ptr->write_buf) + write_offset, buf,
			       remain_len);

			if (remain_len != size) {
				memcpy(channel_ptr->write_buf, (char*)buf + remain_len,
				       size - remain_len);
			}
			channel_head_ptr->write_offset = (write_offset + size) &
							 buffer_size_mask;
			return 0;
		}

		if ((waittime >= 0) && (end_time <= get_ticks())) {
			return -EAGAIN;
		}
	}
}

int rda_mdcom_channel_buf_send_available(const unsigned int channel)
{
	struct rda_mdcom_channel* channel_ptr;
	struct rda_mdcom_channel_head* channel_head_ptr;
	int buffer_size_mask;
	int read_offset;
	int write_offset;

	if (channel >= sizeof(rda_rda_mdcom_chn)/sizeof(struct rda_mdcom_channel)) {
		return 0;
	}

	channel_ptr = &rda_rda_mdcom_chn[channel];
	channel_head_ptr = channel_ptr->write_buf_head;
	buffer_size_mask = channel_ptr->write_buf_size_mask;
	read_offset = channel_head_ptr->read_offset;
	write_offset = channel_head_ptr->write_offset;

	return ((buffer_size_mask + 1) - ((write_offset - read_offset) & buffer_size_mask));
}

int rda_mdcom_channel_buf_recv_stream(const unsigned int channel, void *buf,
				      int size, int waittime)
{
	unsigned long end_time = get_ticks() + waittime;
	struct rda_mdcom_channel* channel_ptr;
	struct rda_mdcom_channel_head* channel_head_ptr;
	int buffer_size_mask;
	int count = 0;
	int buf_offset = 0;

	if (channel >= sizeof(rda_rda_mdcom_chn) / sizeof(struct rda_mdcom_channel)) {
		return -EINVAL;
	}

	if (size < 0) {
		return -EINVAL;
	}

	channel_ptr = &rda_rda_mdcom_chn[channel];
	channel_head_ptr = channel_ptr->read_buf_head;
	buffer_size_mask = channel_ptr->read_buf_size_mask;

	while (1) {
		int read_offset = channel_head_ptr->read_offset;
		int remain_len = channel_head_ptr->write_offset - read_offset;

		if (remain_len < 0) {
			remain_len = buffer_size_mask + 1 - read_offset;
		}

		if (remain_len) {
			if (remain_len > size) {
				remain_len = size;
			}

			memcpy((char *)buf + buf_offset, ((char*)channel_ptr->read_buf) + read_offset, remain_len);
			channel_head_ptr->read_offset = (read_offset + remain_len) &
							buffer_size_mask;
			size -= remain_len;
			count += remain_len;
			buf_offset += remain_len;

			if (!size) {
				return count;
			}
		} else if ((waittime >= 0) && (end_time <= get_ticks())) {
			return count;
		}
	}
}

int rda_mdcom_channel_buf_recv_dgram(const unsigned int channel, void *buf,
				     int size, int waittime)
{
	unsigned long end_time = get_ticks() + waittime;
	struct rda_mdcom_channel* channel_ptr;
	struct rda_mdcom_channel_head* channel_head_ptr;
	int buffer_size_mask, read_offset;

	if (channel >= sizeof(rda_rda_mdcom_chn) / sizeof(struct rda_mdcom_channel)) {
		return -EINVAL;
	}

	if (size < 0) {
		return -EINVAL;
	}

	channel_ptr = &rda_rda_mdcom_chn[channel];
	channel_head_ptr = channel_ptr->read_buf_head;
	buffer_size_mask = channel_ptr->read_buf_size_mask;
	read_offset = ALIGN(channel_head_ptr->read_offset, 4) & buffer_size_mask;

	while (1) {
		int remain_len = (channel_head_ptr->write_offset - read_offset +
				  buffer_size_mask + 1) & buffer_size_mask;

		if (remain_len >= size) {
			remain_len = buffer_size_mask + 1 - read_offset;

			if (remain_len > size) {
				remain_len = size;
			}

			memcpy(buf, ((char*)channel_ptr->read_buf) + read_offset, remain_len);

			if (remain_len != size) {
				memcpy(((char *)buf + remain_len), channel_ptr->read_buf, size - remain_len);
			}
			channel_head_ptr->read_offset = (read_offset + size) & buffer_size_mask;

			return 0;
		}

		if ((waittime >= 0) && (end_time <= get_ticks())) {
			return -EAGAIN;
		}
	}
}

int rda_mdcom_channel_buf_recv_available(const unsigned int channel)
{
	struct rda_mdcom_channel* channel_ptr;
	struct rda_mdcom_channel_head* channel_head_ptr;
	int buffer_size_mask;
	int read_offset;
	int write_offset;

	if (channel >= sizeof(rda_rda_mdcom_chn) / sizeof(struct rda_mdcom_channel)) {
		return 0;
	}

	channel_ptr = &rda_rda_mdcom_chn[channel];
	channel_head_ptr = channel_ptr->write_buf_head;
	buffer_size_mask = channel_ptr->write_buf_size_mask;
	read_offset = channel_head_ptr->read_offset;
	write_offset = channel_head_ptr->write_offset;

	return ((write_offset - read_offset) & buffer_size_mask);
}

void rda_mdcom_channel_show(const unsigned int channel)
{
	printf("RDA MDCOM channel %d stauts:\n", channel);

	if (channel >= sizeof(rda_rda_mdcom_chn) / sizeof(struct rda_mdcom_channel)) {
		printf("Channel number is out of range, ERROR!\n");
	} else {
		struct rda_mdcom_channel* channel_ptr = &rda_rda_mdcom_chn[channel];
		struct rda_mdcom_channel_head* channel_head_ptr;
		printf("Read buffer head address  = 0x%p\n",
		       channel_ptr->read_buf_head);
		printf("Read buffer address       = 0x%p\n",
		       channel_ptr->read_buf);
		printf("Read buffer length        = %d\n",
		       channel_ptr->read_buf_size_mask + 1);
		channel_head_ptr = channel_ptr->read_buf_head;
		printf("Read buffer offset read   = 0x%x\n",
		       channel_head_ptr->read_offset);
		printf("Read buffer offset write  = 0x%x\n",
		       channel_head_ptr->write_offset);

		printf("Write buffer head address = 0x%p\n",
		       channel_ptr->write_buf_head);
		printf("Write buffer address      = 0x%p\n",
		       channel_ptr->write_buf);
		printf("Write buffer length       = %d\n",
		       channel_ptr->write_buf_size_mask + 1);
		channel_head_ptr = channel_ptr->write_buf_head;
		printf("Write buffer offset read  = 0x%x\n",
		       channel_head_ptr->read_offset);
		printf("Write buffer offset write = 0x%x\n",
		       channel_head_ptr->write_offset);
	}

	printf("RDA MDCOM channel %d stauts end.\n", channel);
}

int rda_mdcom_tstc(const unsigned int channel)
{
	struct rda_mdcom_channel_head* channel_head_ptr;

	if (channel >= sizeof(rda_rda_mdcom_chn) / sizeof(struct rda_mdcom_channel)) {
		return 0;
	}

	channel_head_ptr = rda_rda_mdcom_chn[channel].read_buf_head;

	return (channel_head_ptr->write_offset != channel_head_ptr->read_offset);
}

int rda_mdcom_getc(const unsigned int channel)
{
	char c = 0;

	rda_mdcom_channel_buf_recv_stream(channel, &c, 1, -1);
	return c;
}

void rda_mdcom_putc(const char c, const unsigned int channel)
{
	rda_mdcom_channel_buf_send_stream(channel, (void*)&c, 1, -1);
}

void rda_mdcom_puts(const char *s, const unsigned int channel)
{
	rda_mdcom_channel_buf_send_stream(channel, (void*)s, strlen(s), -1);
}

