#ifndef  _MDCOM_H_
#define  _MDCOM_H_

#ifdef RDA_MDCOM_DEBUG
#define rdamddbg(fmt,args...)	printf (fmt ,##args)
#else
#define rdamddbg(fmt,args...)
#endif /* RDA_MDCOM_DEBUG */

/*
 * Note :
 * The number of version will be updated according to version of BP.
 */
#define RDA_BP_VER		0x00010001


typedef enum
{
	/* Normal cause, ie power up */
	RDA_RESET_CAUSE_NORMAL = 0x0,
	/* The reset was caused by a watchdog */
	RDA_RESET_CAUSE_WATCHDOG = 0x1,
	/* The reset was caused by a soft restart, triggered by the function */
	RDA_RESET_CAUSE_RESTART = 0x2,
	/* The reset was initiated from the Host Interface. */
	RDA_RESET_CAUSE_HOST_DEBUG = 0x3,
	/* The reset was caused by alarm, from the calendar. */
	RDA_RESET_CAUSE_ALARM = 0x4,

	RDA_RESET_CAUSE_CHARGER = 0x80,

	RDA_RESET_CAUSE_QTY

} RDA_RESET_CAUSE_T;

/*
 * PORT, LINE
 */
typedef enum {
	RDA_MDCOM_PORT0,
	RDA_MDCOM_PORT1,
} rda_mdcom_port_id_t;

typedef enum {
	RDA_MDCOM_LINE_MODEM_RESET,   // inform AP Modem reset done
	RDA_MDCOM_LINE_DL_HANDSHAKE,  // for AP download BB code
	RDA_MDCOM_LINE_CMD_START,     // for port1 command start
	RDA_MDCOM_LINE_SLEEP_WAKEUP,  // for sleep/wakeup status
	RDA_MDCOM_LINE_EXCEPTION,     // for exception
} rda_mdcom_port0_line_id_t;

typedef enum {
	RDA_MDCOM_LINE_AT_CMD,        // for AT command
	RDA_MDCOM_LINE_AT_CMD_FC,     // for AT command Flow control
	RDA_MDCOM_LINE_SYSTEM,        // for SYSTEM command
	RDA_MDCOM_LINE_SYSTEM_FC,     // for AT command Flow control
	RDA_MDCOM_LINE_TRACE,		  // for Trace communciation
	RDA_MDCOM_LINE_TRACE_FC,	  // for Trace communciation Flow control
} rda_mdcom_port1_line_id_t;

void rda_mdcom_set_logic_base_addr(u32 base);

u32 rda_mdcom_address_modem2ap(u32 addr);

int rda_mdcom_setup_run_env(u32 pc, u32 param);

void rda_mdcom_get_calib_section(u32 *addr, u32 *len);
void rda_mdcom_get_ext_calib_section(u32 *addr, u32 *len);
void rda_mdcom_get_factory_section(u32 *addr, u32 *len);
void rda_mdcom_get_ap_factory_section(u32 *addr, u32 *len);

u32 rda_mdcom_get_reset_cause(void);
u32 rda_mdcom_get_interface_version(void);

int rda_mdcom_system_started_before(void);
void rda_mdcom_set_system_started_flag(void);
int rda_mdcom_modem_crashed_before(void);
int rda_mdcom_calib_update_cmd_valid(void);
int rda_mdcom_factory_update_cmd_valid(void);
int rda_mdcom_ap_factory_update_cmd_valid(void);

void rda_mdcom_init_all_log_info(void);
void rda_mdcom_get_modem_log_info(u32 *addr, u32 *len);
void rda_mdcom_get_modem_exception_info(u32 *addr, u32 *len);

void rda_mdcom_show_xcpu_info(void);
void rda_mdcom_show_software_version(void);

int rda_mdcom_init_port(int port_id);

int rda_mdcom_line_set(int port_id, int line_id);
int rda_mdcom_line_set_check(int port_id, int line_id);
int rda_mdcom_line_set_wait(int port_id, int line_id, int waittime);

int rda_mdcom_line_clear(int port_id, int line_id);
int rda_mdcom_line_clear_check(int port_id, int line_id);
int rda_mdcom_line_clear_wait(int port_id, int line_id, int waittime);

void rda_mdcom_port_show(void);

typedef enum {
	RDA_MDCOM_CHANNEL_AT,		// for AT command
	RDA_MDCOM_CHANNEL_SYSTEM,	// for SYSTEM command
	RDA_MDCOM_CHANNEL_TRACE,	// for Trace communication
} rda_mdcom_channel_id_t;

struct rda_mdcom_channel_head {
	volatile int read_offset;	// offset of the read pointer
	volatile int write_offset;	// offset of the write pointer
	volatile int reserved0;		// reserved
	volatile int reserved1;		// reserved
};

struct rda_mdcom_channel {
	int data_line_id;
	int fc_line_id;

	// the address of 16-byte buffer head
	struct rda_mdcom_channel_head* read_buf_head;
	void* read_buf;			// the start address of modem to AP buffer
	int read_buf_size_mask;		// the mask of modem to AP buffer size

	// the address of 16-byte buffer head
	struct rda_mdcom_channel_head* write_buf_head;
	void* write_buf;		// the start address of AP to modem buffer
	int write_buf_size_mask;	// the mask of AP to modem buffer size
};

int rda_mdcom_channel_init(const unsigned int channel);
int rda_mdcom_channel_all_init(void);
int rda_mdcom_channel_buf_send_stream(const unsigned int channel, void *buf, int size, int waittime);
int rda_mdcom_channel_buf_send_dgram(const unsigned int channel, void *buf, int size, int waittime);
int rda_mdcom_channel_buf_recv_stream(const unsigned int channel, void *buf, int size, int waittime);
int rda_mdcom_channel_buf_recv_dgram(const unsigned int channel, void *buf, int size, int waittime);
int rda_mdcom_channel_buf_send_available(const unsigned int channel);
int rda_mdcom_channel_buf_recv_available(const unsigned int channel);
void rda_mdcom_channel_show(const unsigned int channel);

/*
 * AT COMMAND Channel
 */
/* emulate a tty device */
int rda_mdcom_tstc(const unsigned int channel);
int rda_mdcom_getc(const unsigned int channel);
void rda_mdcom_putc(const char c, const unsigned int channel);
void rda_mdcom_puts(const char *s, const unsigned int channel);

/*
 * SYSTEM MESSAGE Channel
 */
int rda_mdcom_send_sys_msg(
	struct rda_mdcom_channel *channel,
	int msg_id,
	void *send_msg, int send_size,
	void *response, int resp_buf_size,
	int *resp_size
);

int rda_mdcom_parse_sys_msg(
	struct rda_mdcom_channel *channel,
	int msg_id,
	void *send_msg, int send_size,
	void *response, int *resp_size
);

#endif //  _MDCOM_H_
