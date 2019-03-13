#ifndef  _DEFS_MDCOM_H_
#define  _DEFS_MDCOM_H_

#include <asm/arch/hardware.h>

#ifdef RDA_MDCOM_DEBUG
#define rdamddbg(fmt,args...)	printf (fmt ,##args)
#else
#define rdamddbg(fmt,args...)
#endif /* RDA_MDCOM_DEBUG */

/*
 * Convert modem address to AP address
 */
#define RDA_ADD_M2A(x)		(((x) & 0x0FFFFFFF) | 0x10000000)

/*
 * Modem boot context address
 */
#define RDA_BOOT_CTX_ADD	0x11C00100

/*
 * Modem map pointer
 */
#define RDA_MODEM_MAP_PTR	0x11C0027C
#define RDA_MODEM_MAP_QTY	0x1A

/*
 * Modem RAM info
 */
#if defined(_TGT_MODEM_MEM_SIZE) && (_TGT_MODEM_MEM_SIZE > 0)
#define RDA_MODEM_RAM_BASE	(PHYS_SDRAM_1 + ((_TGT_MODEM_MEM_BASE) << 20))
#define RDA_MODEM_RAM_SIZE	(_TGT_MODEM_MEM_SIZE << 20)
#define RDA_GSM_MODEM_RAM_END (RDA_MODEM_RAM_BASE + (_TGT_MODEM_GSM_MEM_SIZE << 20))
#define RDA_MODEM_RAM_END	RDA_GSM_MODEM_RAM_END
#ifdef _TGT_MODEM_WCDMA_MEM_SIZE
#define RDA_WCDMA_MODEM_RAM_END (RDA_GSM_MODEM_RAM_END + (_TGT_MODEM_WCDMA_MEM_SIZE << 20))
#endif
#else
#define RDA_MODEM_RAM_BASE	RDA_MD_PSRAM_BASE
#define RDA_MODEM_RAM_SIZE	RDA_MD_PSRAM_SIZE
#define RDA_MODEM_RAM_END	(RDA_MODEM_RAM_BASE + RDA_MODEM_RAM_SIZE)
#endif

#define RDA_MODEM_INTSRAM_BASE	(0x11C00000)
#define RDA_MODEM_INTSRAM_SIZE	(0x18000)
#define RDA_MODEM_INTSRAM_END	\
	(RDA_MODEM_INTSRAM_BASE + RDA_MODEM_INTSRAM_SIZE)

/*
 * HEARTBEAT data
 */
#define RDA_AP_MBX_HEARTBEAT_ADD	(RDA_MD_MAILBOX_BASE + 0)

/*
 * CHANNEL
 */
#define RDA_MDCOM_CHN_AT_HEAD_ADD_READ		0x00200430
#define RDA_MDCOM_CHN_AT_HEAD_ADD_WRITE		0x00200C40
#define RDA_MDCOM_CHN_AT_BUF_ADD_READ		0x00200440
#define RDA_MDCOM_CHN_AT_BUF_ADD_WRITE		0x00200C50
#define RDA_MDCOM_CHN_AT_BUF_LEN_READ		2048
#define RDA_MDCOM_CHN_AT_BUF_LEN_WRITE		2048

#define RDA_MDCOM_CHN_SYS_HEAD_ADD_READ		0x00200010
#define RDA_MDCOM_CHN_SYS_HEAD_ADD_WRITE	0x00200220
#define RDA_MDCOM_CHN_SYS_BUF_ADD_READ		0x00200020
#define RDA_MDCOM_CHN_SYS_BUF_ADD_WRITE		0x00200230
#define RDA_MDCOM_CHN_SYS_BUF_LEN_READ		512
#define RDA_MDCOM_CHN_SYS_BUF_LEN_WRITE		512

#define RDA_MDCOM_CHN_TRACE_HEAD_ADD_READ	0x00201450
#define RDA_MDCOM_CHN_TRACE_HEAD_ADD_WRITE	0x00201860
#define RDA_MDCOM_CHN_TRACE_BUF_ADD_READ	0x00201460
#define RDA_MDCOM_CHN_TRACE_BUF_ADD_WRITE	0x00201870
#define RDA_MDCOM_CHN_TRACE_BUF_LEN_READ	1024
#define RDA_MDCOM_CHN_TRACE_BUF_LEN_WRITE	512

/*
 * MAGIC NUMBER
 */
#define RDA_AP_MBX_MAGIC_NUMBER_ADD	(RDA_MD_MAILBOX_BASE + 0x1A70)
#define RDA_MAGIC_SYSTEM_STARTED_FLAG	0x057a67ed
#define RDA_MAGIC_MODEM_CRASH_FLAG	0x9db09db0
#define RDA_MAGIC_FACT_UPD_CMD_FLAG	0xfac40c3d
#define RDA_MAGIC_FACT_UPD_TYPE_FLAG	0x496efa00
#define RDA_MAGIC_FACT_UPD_TYPE_FLAG_MASK	0xFFFFFF00
#define RDA_MAGIC_FACT_UPD_TYPE_CALIB	0x1
#define RDA_MAGIC_FACT_UPD_TYPE_FACT	0x2
#define RDA_MAGIC_FACT_UPD_TYPE_AP_FACT	0x4

/*
 * LOG BUFFER INFO
 */
#define RDA_AP_MBX_LOG_BUF_INFO_ADD	(RDA_MD_MAILBOX_BASE + 0x1A80)
#define RDA_AP_MBX_MAX_MODEM_LOG_LEN	0x10000
#define RDA_AP_MBX_MAX_MODEM_EXC_LEN	0x1000


/*
 * BOOT_HST_MONITOR_X_CTX_T
 * This structure is  used by the HOST execution command
 * It can store PC, SP, param ptr and returned value ptr
 * The command type field could reveal itself as really
 * relevant in the future
 */
typedef volatile struct {
	u32	cmdType;         // 0x00000000
	u32	pc;              // 0x00000004
	u32	sp;              // 0x00000008
	void*	param;           // 0x0000000c
	void*	returnedValue;   // 0x00000010
} RDA_BOOT_HST_MONITOR_X_CTX_T;

typedef struct
{
	u32	revision;	//0x00000000
	u32	number;		//0x00000004
	u32	date;		//0x00000008
	u8*	string;		//0x0000000C
} RDA_MODEM_MAP_VERSION_T; //Size : 0x10

typedef struct
{
    RDA_MODEM_MAP_VERSION_T*	version;	//0x00000000
    void*			access;		//0x00000004
} RDA_MODEM_MAP_MODULE_T; //Size : 0x8

typedef struct
{
	/* Modem heartbeat counter */
	volatile u32 bpCounter;
	/* AP heartbeat counter */
	volatile u32 apCounter;
	/* System reset cause */
	u32 resetCause;
	/* Communication interface version */
	u32 version;
} RDA_AP_MBX_HEARTBEAT_T;

typedef struct {
	u32 sysStarted;
	u32 modemCrashed;
	u32 factUpdateCmd;
	u32 factUpdateType;
} RDA_AP_MBX_MAGIC_NUMBER_T;

typedef struct {
	u32 modemAddr;
	u32 modemLen;
	u32 modemExcAddr;
	u32 modemExcLen;
	struct {
		u32 apAddr;
		u32 apLen;
		u32 reserved[2];
	} apLog[5];
} RDA_AP_MBX_LOG_BUF_INFO_T;

#endif //  _DEFS_MDCOM_H_
